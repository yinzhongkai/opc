[CmdletBinding()]
param(
    [string[]]$BuildRoots = @(
        'Debug=out\build\windows-msvc-x64-debug',
        'Release=out\build\windows-msvc-x64-release',
        'CI=out\build\ci-windows-msvc-x64'
    ),

    [string]$QtRoot = 'C:\sr\q\qt6112',
    [string]$ExecutableName = 'space_rhythm_core_tests.exe',
    [string]$LaunchArguments = '--gtest_list_tests',
    [string]$EvidenceRoot = '',
    [ValidateRange(1, 20)][int]$RepeatCount = 3,
    [ValidateRange(1, 168)][int]$HistoricalHours = 72,
    [ValidateRange(0, 10000)][int]$EventSettleMilliseconds = 2000
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$sourceRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$commit = (& git.exe -C $sourceRoot rev-parse --short=12 HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Unable to resolve the source Git commit.' }
$utcStamp = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ')
if ([string]::IsNullOrWhiteSpace($EvidenceRoot)) {
    $EvidenceRoot = Join-Path $sourceRoot "out\evidence\T021-ENV-001\$utcStamp-$commit"
}
$EvidenceRoot = [System.IO.Path]::GetFullPath($EvidenceRoot)
New-Item -ItemType Directory -Force -Path $EvidenceRoot | Out-Null

function Write-JsonFile {
    param(
        [Parameter(Mandatory)][object]$InputObject,
        [Parameter(Mandatory)][string]$LiteralPath,
        [int]$Depth = 10
    )
    $InputObject | ConvertTo-Json -Depth $Depth |
        Set-Content -LiteralPath $LiteralPath -Encoding utf8
}

function Get-ExceptionRecord {
    param([Parameter(Mandatory)][System.Exception]$Exception)
    $nativeErrorCode = $null
    $messages = [System.Collections.Generic.List[string]]::new()
    $current = $Exception
    while ($null -ne $current) {
        $messages.Add("$($current.GetType().FullName): $($current.Message)")
        if ($null -eq $nativeErrorCode -and $current -is [System.ComponentModel.Win32Exception]) {
            $nativeErrorCode = $current.NativeErrorCode
        }
        $current = $current.InnerException
    }
    return [ordered]@{
        type = $Exception.GetType().FullName
        message = $Exception.Message
        messages = @($messages)
        hresultDecimal = $Exception.HResult
        hresultHex = ('0x{0:X8}' -f ($Exception.HResult -band 0xffffffffL))
        nativeErrorCode = $nativeErrorCode
    }
}

function Invoke-NativeCapture {
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][string[]]$Arguments,
        [Parameter(Mandatory)][string]$OutputPath
    )
    try {
        $lines = @(& $FilePath @Arguments 2>&1 | ForEach-Object { $_.ToString() })
        $exitCode = $LASTEXITCODE
        $lines | Set-Content -LiteralPath $OutputPath -Encoding utf8
        return [ordered]@{
            filePath = $FilePath
            arguments = $Arguments
            exitCode = $exitCode
            exception = $null
            output = [System.IO.Path]::GetRelativePath($EvidenceRoot, $OutputPath)
        }
    }
    catch {
        $_.Exception.ToString() | Set-Content -LiteralPath $OutputPath -Encoding utf8
        return [ordered]@{
            filePath = $FilePath
            arguments = $Arguments
            exitCode = $null
            exception = Get-ExceptionRecord -Exception $_.Exception
            output = [System.IO.Path]::GetRelativePath($EvidenceRoot, $OutputPath)
        }
    }
}

function Import-VsDevEnvironment {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
        throw "vswhere.exe was not found: $vswhere"
    }
    $installationPath = (& $vswhere -latest -products Microsoft.VisualStudio.Product.BuildTools `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath).Trim()
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($installationPath)) {
        throw 'MSVC 2022 Build Tools with the x64 C++ workload was not found.'
    }
    $script:VsInstallRoot = $installationPath
    $vsDevCmd = Join-Path $installationPath 'Common7\Tools\VsDevCmd.bat'
    $developerCommand = "call `"$vsDevCmd`" -no_logo -arch=x64 -host_arch=x64 >nul && set"
    $environmentLines = @(& $env:ComSpec /d /c $developerCommand)
    if ($LASTEXITCODE -ne 0) { throw 'VsDevCmd.bat failed to initialize the x64 environment.' }
    $developerPathLine = $environmentLines |
        Where-Object { $_ -match '^Path=' -and $_ -like '*\VC\Tools\MSVC\*' } |
        Select-Object -First 1
    if ([string]::IsNullOrWhiteSpace($developerPathLine)) {
        throw 'VsDevCmd.bat did not return a canonical PATH value.'
    }
    foreach ($line in $environmentLines) {
        if ($line -match '^([^=]+)=(.*)$') {
            if ($Matches[1] -ine 'Path') {
                Set-Item -LiteralPath "Env:$($Matches[1])" -Value $Matches[2]
            }
        }
    }
    Remove-Item -LiteralPath Env:PATH -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath Env:Path -ErrorAction SilentlyContinue
    Set-Item -LiteralPath Env:Path -Value $developerPathLine.Substring(5)
    $script:DumpbinPath = (Get-Command dumpbin.exe -ErrorAction Stop).Source
    $script:NinjaPath = Join-Path $installationPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
}

function Get-BuildEntry {
    param([Parameter(Mandatory)][string]$Entry)
    $separator = $Entry.IndexOf('=')
    if ($separator -lt 1 -or $separator -eq ($Entry.Length - 1)) {
        throw "BuildRoots entries must use Label=Path syntax: $Entry"
    }
    $label = $Entry.Substring(0, $separator).Trim()
    $path = $Entry.Substring($separator + 1).Trim()
    if (-not [System.IO.Path]::IsPathRooted($path)) { $path = Join-Path $sourceRoot $path }
    return [ordered]@{
        label = $label
        buildRoot = [System.IO.Path]::GetFullPath($path)
    }
}

function Get-PeRecord {
    param([Parameter(Mandatory)][string]$LiteralPath)
    $bytes = [System.IO.File]::ReadAllBytes($LiteralPath)
    if ($bytes.Length -lt 64) { throw "File is too short to contain a PE header: $LiteralPath" }
    $peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
    if ($peOffset -lt 0 -or ($peOffset + 26) -gt $bytes.Length) {
        throw "Invalid PE header offset in $LiteralPath"
    }
    $signature = [Text.Encoding]::ASCII.GetString($bytes, $peOffset, 4)
    $machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
    $optionalMagic = [BitConverter]::ToUInt16($bytes, $peOffset + 24)
    return [ordered]@{
        dosSignature = [Text.Encoding]::ASCII.GetString($bytes, 0, 2)
        peOffset = $peOffset
        peSignatureHex = ([BitConverter]::ToString($bytes, $peOffset, 4) -replace '-', '')
        peSignatureText = $signature.TrimEnd([char]0)
        machineHex = ('0x{0:X4}' -f $machine)
        machine = switch ($machine) { 0x8664 { 'x64' } 0x014c { 'x86' } 0xaa64 { 'arm64' } default { 'unknown' } }
        optionalHeaderMagicHex = ('0x{0:X4}' -f $optionalMagic)
        peFormat = switch ($optionalMagic) { 0x020b { 'PE32+' } 0x010b { 'PE32' } default { 'unknown' } }
    }
}

function Get-AclRecord {
    param([Parameter(Mandatory)][string]$LiteralPath)
    try {
        $acl = Get-Acl -LiteralPath $LiteralPath -ErrorAction Stop
        return [ordered]@{
            owner = $acl.Owner
            group = $acl.Group
            sddl = $acl.Sddl
            access = @($acl.Access | ForEach-Object {
                [ordered]@{
                    identity = $_.IdentityReference.Value
                    rights = $_.FileSystemRights.ToString()
                    accessControlType = $_.AccessControlType.ToString()
                    isInherited = $_.IsInherited
                    inheritanceFlags = $_.InheritanceFlags.ToString()
                    propagationFlags = $_.PropagationFlags.ToString()
                }
            })
            error = $null
        }
    }
    catch {
        return [ordered]@{ owner = $null; group = $null; sddl = $null; access = @(); error = Get-ExceptionRecord -Exception $_.Exception }
    }
}

function Get-StreamRecord {
    param([Parameter(Mandatory)][string]$LiteralPath)
    $streams = @()
    $streamError = $null
    try {
        $streams = @(Get-Item -LiteralPath $LiteralPath -Stream * -ErrorAction Stop | ForEach-Object {
            [ordered]@{ stream = $_.Stream; length = $_.Length }
        })
    }
    catch { $streamError = Get-ExceptionRecord -Exception $_.Exception }
    $zone = $null
    $zoneError = $null
    try { $zone = Get-Content -LiteralPath $LiteralPath -Stream Zone.Identifier -Raw -ErrorAction Stop }
    catch {
        if ($_.Exception.Message -notmatch 'does not exist|找不到|不存在') {
            $zoneError = Get-ExceptionRecord -Exception $_.Exception
        }
    }
    return [ordered]@{
        streams = $streams
        streamEnumerationError = $streamError
        zoneIdentifierPresent = $null -ne $zone
        zoneIdentifier = $zone
        zoneIdentifierReadError = $zoneError
    }
}

function Get-SignatureRecord {
    param([Parameter(Mandatory)][string]$LiteralPath)
    try {
        $signature = Get-AuthenticodeSignature -LiteralPath $LiteralPath -ErrorAction Stop
        return [ordered]@{
            status = $signature.Status.ToString()
            statusMessage = $signature.StatusMessage
            signatureType = $signature.SignatureType.ToString()
            signerSubject = if ($signature.SignerCertificate) { $signature.SignerCertificate.Subject } else { $null }
            signerThumbprint = if ($signature.SignerCertificate) { $signature.SignerCertificate.Thumbprint } else { $null }
            timestampSubject = if ($signature.TimeStamperCertificate) { $signature.TimeStamperCertificate.Subject } else { $null }
            error = $null
        }
    }
    catch {
        return [ordered]@{ status = 'Error'; statusMessage = $null; signatureType = $null; signerSubject = $null; signerThumbprint = $null; timestampSubject = $null; error = Get-ExceptionRecord -Exception $_.Exception }
    }
}

function Get-CacheRecord {
    param([Parameter(Mandatory)][string]$BuildRoot)
    $cachePath = Join-Path $BuildRoot 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
        return [ordered]@{ path = $cachePath; values = [ordered]@{}; error = 'CMakeCache.txt was not found.' }
    }
    $names = @(
        'CMAKE_BUILD_TYPE', 'CMAKE_CXX_COMPILER', 'CMAKE_CXX_FLAGS',
        'CMAKE_CXX_FLAGS_DEBUG', 'CMAKE_CXX_FLAGS_RELEASE', 'CMAKE_CXX_FLAGS_RELWITHDEBINFO',
        'CMAKE_EXE_LINKER_FLAGS', 'CMAKE_EXE_LINKER_FLAGS_DEBUG', 'CMAKE_EXE_LINKER_FLAGS_RELEASE',
        'CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO', 'CMAKE_GENERATOR', 'CMAKE_MAKE_PROGRAM',
        'CMAKE_TOOLCHAIN_FILE', 'VCPKG_TARGET_TRIPLET'
    )
    $values = [ordered]@{}
    foreach ($line in Get-Content -LiteralPath $cachePath) {
        if ($line -match '^([^#/:][^:]*):[^=]*=(.*)$' -and $names -contains $Matches[1]) {
            $values[$Matches[1]] = $Matches[2]
        }
    }
    return [ordered]@{ path = $cachePath; values = $values; error = $null }
}

function Invoke-LaunchProbe {
    param(
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][string]$BuildRoot,
        [Parameter(Mandatory)][string]$Executable,
        [Parameter(Mandatory)][string]$OutputRoot,
        [Parameter(Mandatory)][int]$Attempt
    )
    $stdoutPath = Join-Path $OutputRoot ("launch-{0:D2}.stdout.txt" -f $Attempt)
    $stderrPath = Join-Path $OutputRoot ("launch-{0:D2}.stderr.txt" -f $Attempt)
    $startedUtc = [DateTime]::UtcNow
    $stopwatch = [Diagnostics.Stopwatch]::StartNew()
    $process = $null
    try {
        $psi = [Diagnostics.ProcessStartInfo]::new()
        $psi.FileName = $Executable
        $psi.Arguments = $LaunchArguments
        $psi.WorkingDirectory = $BuildRoot
        $psi.UseShellExecute = $false
        $psi.RedirectStandardOutput = $true
        $psi.RedirectStandardError = $true
        $psi.CreateNoWindow = $true
        $runtimeBin = if ($Label -match '(?i)debug') {
            Join-Path $sourceRoot 'out\vcpkg_installed\x64-windows-space-rhythm\debug\bin'
        }
        else {
            Join-Path $sourceRoot 'out\vcpkg_installed\x64-windows-space-rhythm\bin'
        }
        $debugCrt = if (-not [string]::IsNullOrWhiteSpace($env:VCToolsRedistDir)) {
            Join-Path $env:VCToolsRedistDir 'debug_nonredist\x64\Microsoft.VC143.DebugCRT'
        }
        else { '' }
        $pathParts = @($runtimeBin, (Join-Path $QtRoot 'bin'))
        if (-not [string]::IsNullOrWhiteSpace($debugCrt) -and (Test-Path -LiteralPath $debugCrt)) { $pathParts += $debugCrt }
        $pathParts += $env:Path
        $psi.Environment['Path'] = $pathParts -join ';'

        $process = [Diagnostics.Process]::new()
        $process.StartInfo = $psi
        $started = $process.Start()
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        $completed = $process.WaitForExit(30000)
        if (-not $completed) {
            $process.Kill($true)
            $process.WaitForExit()
        }
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        $stdout | Set-Content -LiteralPath $stdoutPath -Encoding utf8
        $stderr | Set-Content -LiteralPath $stderrPath -Encoding utf8
        $exitCode = if ($completed) { $process.ExitCode } else { $null }
        $stopwatch.Stop()
        return [ordered]@{
            attempt = $Attempt
            startedUtc = $startedUtc.ToString('o')
            finishedUtc = [DateTime]::UtcNow.ToString('o')
            durationMs = $stopwatch.ElapsedMilliseconds
            processStarted = $started
            completed = $completed
            classification = if (-not $completed) {
                'timeout'
            }
            elseif ($exitCode -eq 0 -and $stdout.Length -eq 0 -and $stderr.Length -eq 0) {
                'started-exit-zero-no-output'
            }
            elseif ($exitCode -eq 0) {
                'started-exit-zero'
            }
            else {
                'started-nonzero-exit'
            }
            exitCode = $exitCode
            exception = $null
            stdout = [System.IO.Path]::GetRelativePath($EvidenceRoot, $stdoutPath)
            stdoutLength = $stdout.Length
            stdoutSha256 = (Get-FileHash -LiteralPath $stdoutPath -Algorithm SHA256).Hash.ToLowerInvariant()
            stderr = [System.IO.Path]::GetRelativePath($EvidenceRoot, $stderrPath)
            stderrLength = $stderr.Length
            stderrSha256 = (Get-FileHash -LiteralPath $stderrPath -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    }
    catch {
        $stopwatch.Stop()
        '' | Set-Content -LiteralPath $stdoutPath -Encoding utf8
        $_.Exception.ToString() | Set-Content -LiteralPath $stderrPath -Encoding utf8
        return [ordered]@{
            attempt = $Attempt
            startedUtc = $startedUtc.ToString('o')
            finishedUtc = [DateTime]::UtcNow.ToString('o')
            durationMs = $stopwatch.ElapsedMilliseconds
            processStarted = $false
            completed = $false
            classification = 'process-start-exception'
            exitCode = $null
            exception = Get-ExceptionRecord -Exception $_.Exception
            stdout = [System.IO.Path]::GetRelativePath($EvidenceRoot, $stdoutPath)
            stdoutLength = 0
            stdoutSha256 = (Get-FileHash -LiteralPath $stdoutPath -Algorithm SHA256).Hash.ToLowerInvariant()
            stderr = [System.IO.Path]::GetRelativePath($EvidenceRoot, $stderrPath)
            stderrLength = (Get-Item -LiteralPath $stderrPath).Length
            stderrSha256 = (Get-FileHash -LiteralPath $stderrPath -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    }
    finally {
        if ($null -ne $process) { $process.Dispose() }
    }
}

function Get-RelevantEvents {
    param(
        [Parameter(Mandatory)][string]$LogName,
        [Parameter(Mandatory)][DateTime]$HistoryStart,
        [Parameter(Mandatory)][DateTime]$ProbeStart,
        [Parameter(Mandatory)][DateTime]$ProbeEnd
    )
    try {
        $logInfo = Get-WinEvent -ListLog $LogName -ErrorAction Stop
        $events = @(Get-WinEvent -FilterHashtable @{ LogName = $LogName; StartTime = $HistoryStart } -ErrorAction Stop |
            Where-Object {
                try { $_.ToXml() -match $script:ExecutablePattern }
                catch { $_.Message -match $script:ExecutablePattern }
            } |
            ForEach-Object {
                $xml = $null
                try { $xml = $_.ToXml() } catch { $xml = $null }
                [ordered]@{
                    id = $_.Id
                    recordId = $_.RecordId
                    provider = $_.ProviderName
                    level = $_.LevelDisplayName
                    timeCreated = if ($_.TimeCreated) { $_.TimeCreated.ToUniversalTime().ToString('o') } else { $null }
                    inProbeWindow = [bool]($_.TimeCreated -ge $ProbeStart -and $_.TimeCreated -le $ProbeEnd)
                    message = $_.Message
                    xml = $xml
                }
            })
        return [ordered]@{
            logName = $LogName
            enabled = $logInfo.IsEnabled
            recordCount = $logInfo.RecordCount
            historyStartUtc = $HistoryStart.ToUniversalTime().ToString('o')
            relevantEventCount = $events.Count
            probeWindowEventCount = @($events | Where-Object { $_.inProbeWindow }).Count
            events = $events
            error = $null
        }
    }
    catch {
        return [ordered]@{
            logName = $LogName
            enabled = $null
            recordCount = $null
            historyStartUtc = $HistoryStart.ToUniversalTime().ToString('o')
            relevantEventCount = 0
            probeWindowEventCount = 0
            events = @()
            error = Get-ExceptionRecord -Exception $_.Exception
        }
    }
}

Import-VsDevEnvironment
$env:QT_QPA_PLATFORM = 'offscreen'
$env:QSG_RHI_BACKEND = 'software'
$script:ExecutablePattern = '(?i)' + [regex]::Escape($ExecutableName)
$probeStart = Get-Date
$records = [System.Collections.Generic.List[object]]::new()

foreach ($entryText in $BuildRoots) {
    $entry = Get-BuildEntry -Entry $entryText
    $label = $entry.label
    $buildRoot = $entry.buildRoot
    $recordRoot = Join-Path $EvidenceRoot $label
    New-Item -ItemType Directory -Force -Path $recordRoot | Out-Null
    $executableCandidate = Join-Path $buildRoot $ExecutableName
    $exists = Test-Path -LiteralPath $executableCandidate -PathType Leaf
    $record = [ordered]@{
        label = $label
        configuredBuildRoot = $buildRoot
        executableCandidate = $executableCandidate
        exists = $exists
        actualPath = $null
        file = $null
        pe = $null
        acl = $null
        alternateDataStreams = $null
        signature = $null
        cmakeCache = Get-CacheRecord -BuildRoot $buildRoot
        dumpbinHeaders = $null
        dumpbinDependents = $null
        dumpbinLoadConfig = $null
        ninjaCommands = $null
        launches = @()
    }
    if ($exists) {
        $actualPath = (Resolve-Path -LiteralPath $executableCandidate).ProviderPath
        $item = Get-Item -LiteralPath $actualPath
        $record.actualPath = $actualPath
        $record.file = [ordered]@{
            length = $item.Length
            creationTimeUtc = $item.CreationTimeUtc.ToString('o')
            lastWriteTimeUtc = $item.LastWriteTimeUtc.ToString('o')
            attributes = $item.Attributes.ToString()
            linkType = if ($item.LinkType) { $item.LinkType.ToString() } else { $null }
            linkTarget = if ($item.Target) { @($item.Target | ForEach-Object { $_.ToString() }) } else { @() }
            sha256 = (Get-FileHash -LiteralPath $actualPath -Algorithm SHA256).Hash.ToLowerInvariant()
            fileVersion = $item.VersionInfo.FileVersion
            productVersion = $item.VersionInfo.ProductVersion
        }
        $record.pe = Get-PeRecord -LiteralPath $actualPath
        $record.acl = Get-AclRecord -LiteralPath $actualPath
        $record.alternateDataStreams = Get-StreamRecord -LiteralPath $actualPath
        $record.signature = Get-SignatureRecord -LiteralPath $actualPath
        $record.dumpbinHeaders = Invoke-NativeCapture -FilePath $DumpbinPath -Arguments @('/headers', $actualPath) -OutputPath (Join-Path $recordRoot 'dumpbin-headers.txt')
        $record.dumpbinDependents = Invoke-NativeCapture -FilePath $DumpbinPath -Arguments @('/dependents', $actualPath) -OutputPath (Join-Path $recordRoot 'dumpbin-dependents.txt')
        $record.dumpbinLoadConfig = Invoke-NativeCapture -FilePath $DumpbinPath -Arguments @('/loadconfig', $actualPath) -OutputPath (Join-Path $recordRoot 'dumpbin-loadconfig.txt')
        $record.ninjaCommands = Invoke-NativeCapture -FilePath $NinjaPath -Arguments @('-C', $buildRoot, '-t', 'commands', $ExecutableName) -OutputPath (Join-Path $recordRoot 'ninja-commands.txt')
        $launches = [System.Collections.Generic.List[object]]::new()
        for ($attempt = 1; $attempt -le $RepeatCount; $attempt++) {
            $launches.Add((Invoke-LaunchProbe -Label $label -BuildRoot $buildRoot -Executable $actualPath -OutputRoot $recordRoot -Attempt $attempt))
        }
        $record.launches = @($launches)
    }
    Write-JsonFile -InputObject $record -LiteralPath (Join-Path $recordRoot 'binary.json') -Depth 12
    $records.Add($record)
}

if ($EventSettleMilliseconds -gt 0) {
    Start-Sleep -Milliseconds $EventSettleMilliseconds
}
$probeEnd = Get-Date
$historyStart = $probeStart.AddHours(-$HistoricalHours)
$eventLogs = @(
    'Microsoft-Windows-CodeIntegrity/Operational',
    'Microsoft-Windows-AppLocker/EXE and DLL',
    'Microsoft-Windows-AppLocker/MSI and Script'
)
$eventRecords = @($eventLogs | ForEach-Object {
    Get-RelevantEvents -LogName $_ -HistoryStart $historyStart -ProbeStart $probeStart -ProbeEnd $probeEnd
})
Write-JsonFile -InputObject $eventRecords -LiteralPath (Join-Path $EvidenceRoot 'windows-events.json') -Depth 12

$policyRoot = Join-Path $EvidenceRoot 'policy'
New-Item -ItemType Directory -Force -Path $policyRoot | Out-Null
$ciTool = Get-Command CiTool.exe -ErrorAction SilentlyContinue
$ciToolCapture = if ($ciTool) {
    Invoke-NativeCapture -FilePath $ciTool.Source -Arguments @('-lp', '-json') -OutputPath (Join-Path $policyRoot 'citool-list-policies.json')
}
else { [ordered]@{ filePath = $null; arguments = @(); exitCode = $null; exception = 'CiTool.exe was not found.'; output = $null } }
$deviceGuard = $null
$deviceGuardError = $null
try {
    $deviceGuardInstance = Get-CimInstance -Namespace 'root\Microsoft\Windows\DeviceGuard' -ClassName Win32_DeviceGuard -ErrorAction Stop
    $deviceGuard = [ordered]@{
        availableSecurityProperties = @($deviceGuardInstance.AvailableSecurityProperties)
        codeIntegrityPolicyEnforcementStatus = $deviceGuardInstance.CodeIntegrityPolicyEnforcementStatus
        requiredSecurityProperties = @($deviceGuardInstance.RequiredSecurityProperties)
        securityFeaturesEnabled = @($deviceGuardInstance.SecurityFeaturesEnabled)
        securityServicesConfigured = @($deviceGuardInstance.SecurityServicesConfigured)
        securityServicesRunning = @($deviceGuardInstance.SecurityServicesRunning)
        usermodeCodeIntegrityPolicyEnforcementStatus = $deviceGuardInstance.UsermodeCodeIntegrityPolicyEnforcementStatus
        version = $deviceGuardInstance.Version
        virtualMachineIsolation = $deviceGuardInstance.VirtualMachineIsolation
        virtualMachineIsolationProperties = @($deviceGuardInstance.VirtualMachineIsolationProperties)
    }
}
catch { $deviceGuardError = Get-ExceptionRecord -Exception $_.Exception }

$summary = [ordered]@{
    schemaVersion = 1
    issue = 'T021-ENV-001'
    executableName = $ExecutableName
    launchArguments = $LaunchArguments
    sourceCommit = (& git.exe -C $sourceRoot rev-parse HEAD).Trim()
    sourceRoot = $sourceRoot
    evidenceRoot = $EvidenceRoot
    repeatCount = $RepeatCount
    historicalHours = $HistoricalHours
    eventSettleMilliseconds = $EventSettleMilliseconds
    probeStartedUtc = $probeStart.ToUniversalTime().ToString('o')
    probeFinishedUtc = $probeEnd.ToUniversalTime().ToString('o')
    safety = [ordered]@{
        policyModified = $false
        allowlistModified = $false
        signingPerformed = $false
        wdacBypassUsed = $false
    }
    toolchain = [ordered]@{
        visualStudioRoot = $VsInstallRoot
        dumpbin = $DumpbinPath
        ninja = $NinjaPath
        qtRoot = [System.IO.Path]::GetFullPath($QtRoot)
        targetArchitecture = $env:VSCMD_ARG_TGT_ARCH
        windowsSdkVersion = $env:WindowsSDKVersion
    }
    binaries = @($records)
    eventLogs = @($eventRecords | ForEach-Object {
        [ordered]@{
            logName = $_.logName
            enabled = $_.enabled
            relevantEventCount = $_.relevantEventCount
            probeWindowEventCount = $_.probeWindowEventCount
            error = $_.error
        }
    })
    policyInventory = [ordered]@{
        ciTool = $ciToolCapture
        deviceGuard = $deviceGuard
        deviceGuardError = $deviceGuardError
    }
}
Write-JsonFile -InputObject $summary -LiteralPath (Join-Path $EvidenceRoot 'summary.json') -Depth 14

$manifest = @(Get-ChildItem -LiteralPath $EvidenceRoot -File -Recurse | Where-Object { $_.Name -ne 'sha256-manifest.json' } | ForEach-Object {
    [ordered]@{
        path = [System.IO.Path]::GetRelativePath($EvidenceRoot, $_.FullName).Replace('\', '/')
        length = $_.Length
        sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
})
Write-JsonFile -InputObject $manifest -LiteralPath (Join-Path $EvidenceRoot 'sha256-manifest.json') -Depth 4

Write-Host "WDAC diagnostic evidence: $EvidenceRoot"
foreach ($record in $records) {
    $launchSummary = @($record.launches | ForEach-Object { "$($_.classification):$($_.exitCode)" }) -join ', '
    Write-Host "$($record.label): exists=$($record.exists); path=$($record.actualPath); launches=[$launchSummary]"
}
foreach ($eventRecord in $eventRecords) {
    Write-Host "$($eventRecord.logName): relevant=$($eventRecord.relevantEventCount); probe-window=$($eventRecord.probeWindowEventCount)"
}
