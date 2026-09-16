[CmdletBinding()]
param(
    [ValidatePattern('^[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?$')]
    [string]$Version = '0.1.0-dev',

    [ValidateSet('windows-msvc-x64-release')]
    [string]$Preset = 'windows-msvc-x64-release',

    [string]$QtRoot = 'C:\sr\q\qt6112',
    [string]$VcpkgRoot = 'C:\sr\tools\vcpkg-2026.07.29',
    [string]$OutputRoot = '',
    [string]$EvidenceRoot = '',
    [ValidateRange(1, 64)]
    [int]$Parallel = 10,
    [switch]$AllowDirtySource
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$sourceRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$repositoryRoot = (& git.exe -C $sourceRoot rev-parse --show-toplevel).Trim()
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($repositoryRoot)) {
    throw 'Unable to resolve the Git repository root from the product workspace'
}
$repositoryRoot = [System.IO.Path]::GetFullPath($repositoryRoot)
$outRoot = [System.IO.Path]::GetFullPath((Join-Path $sourceRoot 'out'))
$buildRoot = [System.IO.Path]::GetFullPath((Join-Path $outRoot "build\$Preset"))
$vcpkgInstalledRoot = [System.IO.Path]::GetFullPath(
    (Join-Path $outRoot 'vcpkg_installed\x64-windows-space-rhythm'))
$bundleName = "space-rhythm-$Version-unsigned"
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $outRoot 'release\T-037\unsigned'
}
if ([string]::IsNullOrWhiteSpace($EvidenceRoot)) {
    $EvidenceRoot = Join-Path $outRoot 'evidence\T-037\unsigned-current'
}
$OutputRoot = [System.IO.Path]::GetFullPath($OutputRoot)
$EvidenceRoot = [System.IO.Path]::GetFullPath($EvidenceRoot)
$bundleRoot = Join-Path $OutputRoot $bundleName
$payloadRoot = Join-Path $bundleRoot 'payload\SpaceRhythm'
$payloadBin = Join-Path $payloadRoot 'bin'
$manifestRoot = Join-Path $payloadRoot 'manifest'
$zipPath = Join-Path $OutputRoot "$bundleName.zip"

$expectedQtCoreSha256 = '94E697C5C7B861E1F9072CB2FB251D3C807ECA053D7EE0F1B4A3BA08023AC1AC'
$expectedQtSummarySha256 = '4BCBDAA6DCBCB98BF2F44F700B77FB35D8EE2982996E1B2051C278C2355F49A3'
$expectedWindeployqtSha256 = '889DFACFB42270715D7640687CB2C5DA82FCFAAA829C2E1588BC9DE00ED8D93D'
$expectedVcpkgBaseline = '9e593bb18ea69cc5095e012465dcd675a822ed0d'
$script:CommandLog = Join-Path $EvidenceRoot 'commands.log'
$script:SourceByRelativePath = [System.Collections.Generic.Dictionary[string, object]]::new(
    [System.StringComparer]::OrdinalIgnoreCase)
$script:SystemDependencies = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::OrdinalIgnoreCase)
$script:VcpkgPackages = @{}

function Assert-ExistingFile {
    param([Parameter(Mandatory)][string]$LiteralPath, [Parameter(Mandatory)][string]$Description)
    if (-not (Test-Path -LiteralPath $LiteralPath -PathType Leaf)) {
        throw "$Description was not found: $LiteralPath"
    }
}

function Assert-ExpectedHash {
    param(
        [Parameter(Mandatory)][string]$LiteralPath,
        [Parameter(Mandatory)][string]$Expected,
        [Parameter(Mandatory)][string]$Description
    )
    Assert-ExistingFile -LiteralPath $LiteralPath -Description $Description
    $actual = (Get-FileHash -LiteralPath $LiteralPath -Algorithm SHA256).Hash
    if ($actual -ne $Expected) {
        throw "$Description SHA-256 mismatch. Expected $Expected, found $actual"
    }
}

function Assert-OutputTarget {
    param([Parameter(Mandatory)][string]$LiteralPath)
    $resolved = [System.IO.Path]::GetFullPath($LiteralPath)
    $requiredPrefix = $outRoot.TrimEnd('\') + '\'
    if (-not $resolved.StartsWith($requiredPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to modify a release path outside the repository output root: $resolved"
    }
    if ($resolved -eq $outRoot) {
        throw "Refusing to modify the entire output root: $resolved"
    }
}

function Reset-OutputDirectory {
    param([Parameter(Mandatory)][string]$LiteralPath)
    Assert-OutputTarget -LiteralPath $LiteralPath
    if (Test-Path -LiteralPath $LiteralPath) {
        Remove-Item -LiteralPath $LiteralPath -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $LiteralPath | Out-Null
}

function Import-VsDevEnvironment {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    Assert-ExistingFile -LiteralPath $vswhere -Description 'vswhere.exe'
    $installationPath = (& $vswhere -latest -products Microsoft.VisualStudio.Product.BuildTools `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath).Trim()
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($installationPath)) {
        throw 'MSVC 2022 Build Tools with the x64 C++ workload was not found'
    }

    $vsDevCmd = Join-Path $installationPath 'Common7\Tools\VsDevCmd.bat'
    Assert-ExistingFile -LiteralPath $vsDevCmd -Description 'VsDevCmd.bat'
    $developerCommand = "call `"$vsDevCmd`" -no_logo -arch=x64 -host_arch=x64 >nul && set"
    $environmentLines = & $env:ComSpec /d /c $developerCommand
    if ($LASTEXITCODE -ne 0) {
        throw 'VsDevCmd.bat failed to initialize the x64 compiler environment'
    }
    $developerPathLine = $environmentLines |
        Where-Object { $_ -match '^Path=' -and $_ -like '*\VC\Tools\MSVC\*' } |
        Select-Object -First 1
    if ([string]::IsNullOrWhiteSpace($developerPathLine)) {
        throw 'VsDevCmd.bat did not return a canonical PATH value'
    }
    foreach ($line in $environmentLines) {
        if ($line -match '^([^=]+)=(.*)$' -and $Matches[1] -ine 'Path') {
            Set-Item -LiteralPath "Env:$($Matches[1])" -Value $Matches[2]
        }
    }
    Remove-Item -LiteralPath Env:PATH -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath Env:Path -ErrorAction SilentlyContinue
    Set-Item -LiteralPath Env:Path -Value $developerPathLine.Substring(5)

    return [pscustomobject]@{
        InstallRoot = $installationPath
        CMake = Join-Path $installationPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
        Dumpbin = (Get-Command dumpbin.exe -ErrorAction Stop).Source
    }
}

function Invoke-CapturedProcess {
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][string[]]$Arguments,
        [Parameter(Mandatory)][string]$LogStem
    )
    $command = [ordered]@{ executable = $FilePath; arguments = $Arguments }
    Add-Content -LiteralPath $script:CommandLog -Value ($command | ConvertTo-Json -Compress) -Encoding utf8

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $true
    foreach ($argument in $Arguments) {
        $startInfo.ArgumentList.Add($argument)
    }
    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    try {
        if (-not $process.Start()) {
            throw "Process did not start: $FilePath"
        }
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        $exitCode = $process.ExitCode
    }
    catch {
        $failure = [ordered]@{
            executable = $FilePath
            arguments = $Arguments
            exception = $_.Exception.ToString()
        }
        $failure | ConvertTo-Json -Depth 5 |
            Set-Content -LiteralPath (Join-Path $EvidenceRoot "$LogStem-start-failure.json") -Encoding utf8
        throw
    }
    finally {
        $process.Dispose()
    }

    Set-Content -LiteralPath (Join-Path $EvidenceRoot "$LogStem.stdout.log") -Value $stdout -Encoding utf8
    Set-Content -LiteralPath (Join-Path $EvidenceRoot "$LogStem.stderr.log") -Value $stderr -Encoding utf8
    $result = [pscustomobject]@{ ExitCode = $exitCode; StdOut = $stdout; StdErr = $stderr }
    if ($result.ExitCode -ne 0) {
        throw "Command failed with exit code $($result.ExitCode): $FilePath (see $EvidenceRoot)"
    }
    return $result
}

function Add-SourceRecord {
    param(
        [Parameter(Mandatory)][string]$RelativePath,
        [Parameter(Mandatory)][string]$SourcePath,
        [Parameter(Mandatory)][string]$Component,
        [Parameter(Mandatory)][string]$ComponentVersion,
        [Parameter(Mandatory)][string]$BuildConfigurationHash
    )
    $normalized = $RelativePath.Replace('/', '\')
    $script:SourceByRelativePath[$normalized] = [pscustomobject]@{
        SourcePath = [System.IO.Path]::GetFullPath($SourcePath)
        Component = $Component
        ComponentVersion = $ComponentVersion
        BuildConfigurationHash = $BuildConfigurationHash
    }
}

function Get-InstalledVcpkgPackage {
    param([Parameter(Mandatory)][string]$Name)
    $statusPath = Join-Path (Split-Path -Parent $vcpkgInstalledRoot) 'vcpkg\status'
    Assert-ExistingFile -LiteralPath $statusPath -Description 'vcpkg installed status'
    $blocks = (Get-Content -Raw -LiteralPath $statusPath) -split '(?:\r?\n){2,}'
    $block = $blocks | Where-Object {
        $_ -match "(?m)^Package: $([regex]::Escape($Name))\r?$" -and
        $_ -notmatch '(?m)^Feature:'
    } | Select-Object -First 1
    if ([string]::IsNullOrWhiteSpace($block)) {
        throw "Installed vcpkg package metadata is missing: $Name"
    }
    $values = @{}
    foreach ($line in ($block -split '\r?\n')) {
        if ($line -match '^([^:]+):\s*(.*)$') {
            $values[$Matches[1]] = $Matches[2]
        }
    }
    foreach ($field in @('Version', 'Architecture', 'Abi')) {
        if (-not $values.ContainsKey($field) -or [string]::IsNullOrWhiteSpace($values[$field])) {
            throw "Installed vcpkg metadata for $Name lacks $field"
        }
    }
    $version = $values.Version
    if ($values.ContainsKey('Port-Version') -and $values['Port-Version'] -ne '0') {
        $version += "#$($values['Port-Version'])"
    }
    return [pscustomobject]@{
        Name = $Name
        Version = $version
        Architecture = $values.Architecture
        Abi = $values.Abi
    }
}

function Get-PeDependencies {
    param([Parameter(Mandatory)][string]$LiteralPath, [Parameter(Mandatory)][string]$DumpbinPath)
    $result = Invoke-CapturedProcess -FilePath $DumpbinPath -Arguments @('/nologo', '/dependents', $LiteralPath) `
        -LogStem ("dumpbin-" + (Get-FileHash -LiteralPath $LiteralPath -Algorithm SHA256).Hash.Substring(0, 16))
    $dependencies = [System.Collections.Generic.List[string]]::new()
    $inDependencies = $false
    foreach ($line in ($result.StdOut -split "`r?`n")) {
        if ($line -match '^\s*Image has the following (?:delay load )?dependencies:\s*$') {
            $inDependencies = $true
            continue
        }
        if ($inDependencies -and $line -match '^\s+([A-Za-z0-9_.+-]+\.dll)\s*$') {
            $dependencies.Add($Matches[1])
            continue
        }
        if ($inDependencies -and $line -match '^\s*Summary\s*$') {
            $inDependencies = $false
        }
    }
    return @($dependencies | Sort-Object -Unique)
}

function Assert-X64Pe {
    param([Parameter(Mandatory)][string]$LiteralPath, [Parameter(Mandatory)][string]$DumpbinPath)
    $result = Invoke-CapturedProcess -FilePath $DumpbinPath -Arguments @('/nologo', '/headers', $LiteralPath) `
        -LogStem ("headers-" + (Get-FileHash -LiteralPath $LiteralPath -Algorithm SHA256).Hash.Substring(0, 16))
    if ($result.StdOut -notmatch '(?im)^\s*8664 machine \(x64\)') {
        throw "Runtime PE is not x64: $LiteralPath"
    }
}

function Resolve-RuntimeClosure {
    param([Parameter(Mandatory)][string]$DumpbinPath)
    $vcpkgBin = Join-Path $vcpkgInstalledRoot 'bin'
    $processed = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)

    while ($true) {
        $added = $false
        $packagePeFiles = @(Get-ChildItem -LiteralPath $payloadBin -Recurse -File |
            Where-Object { $_.Extension -in @('.exe', '.dll') })
        $packageNames = [System.Collections.Generic.HashSet[string]]::new(
            [System.StringComparer]::OrdinalIgnoreCase)
        foreach ($file in $packagePeFiles) {
            [void]$packageNames.Add($file.Name)
        }

        foreach ($file in $packagePeFiles) {
            if (-not $processed.Add($file.FullName)) {
                continue
            }
            Assert-X64Pe -LiteralPath $file.FullName -DumpbinPath $DumpbinPath
            foreach ($dependency in (Get-PeDependencies -LiteralPath $file.FullName -DumpbinPath $DumpbinPath)) {
                if ($packageNames.Contains($dependency)) {
                    continue
                }
                if ($dependency -match '^(api-ms-win-|ext-ms-win-)') {
                    [void]$script:SystemDependencies.Add($dependency)
                    continue
                }
                $vcpkgCandidate = Join-Path $vcpkgBin $dependency
                if (Test-Path -LiteralPath $vcpkgCandidate -PathType Leaf) {
                    $destination = Join-Path $payloadBin $dependency
                    Copy-Item -LiteralPath $vcpkgCandidate -Destination $destination
                    Add-SourceRecord -RelativePath "bin\$dependency" -SourcePath $vcpkgCandidate `
                        -Component $(if ($dependency -like 'kissfft-*') { 'KissFFT' } elseif ($dependency -like 'opencv*') { 'OpenCV' } else { 'FFmpeg' }) `
                        -ComponentVersion $(if ($dependency -like 'kissfft-*') { $script:VcpkgPackages.kissfft.Version } elseif ($dependency -like 'opencv*') { $script:VcpkgPackages.opencv4.Version } else { $script:VcpkgPackages.ffmpeg.Version }) `
                        -BuildConfigurationHash $(if ($dependency -like 'kissfft-*') { "vcpkg-abi:$($script:VcpkgPackages.kissfft.Abi)" } elseif ($dependency -like 'opencv*') { "vcpkg-abi:$($script:VcpkgPackages.opencv4.Abi)" } else { "vcpkg-abi:$($script:VcpkgPackages.ffmpeg.Abi)" })
                    [void]$packageNames.Add($dependency)
                    $added = $true
                    continue
                }
                $systemCandidate = Join-Path $env:SystemRoot "System32\$dependency"
                if (Test-Path -LiteralPath $systemCandidate -PathType Leaf) {
                    [void]$script:SystemDependencies.Add($dependency)
                    continue
                }
                throw "Unresolved runtime dependency $dependency imported by $($file.FullName)"
            }
        }
        if (-not $added) {
            break
        }
    }
}

function Copy-LicenseEvidence {
    $licenseSources = @(
        @{ Source = 'C:\sr\s\qt6112\LICENSES\LGPL-3.0-only.txt'; Target = 'licenses\qt\LGPL-3.0-only.txt' },
        @{ Source = 'C:\sr\s\qt6112\LICENSES\GPL-3.0-only.txt'; Target = 'licenses\qt\GPL-3.0-only.txt' },
        @{ Source = 'C:\sr\s\qt6112\LICENSES\Qt-GPL-exception-1.0.txt'; Target = 'licenses\qt\Qt-GPL-exception-1.0.txt' },
        @{ Source = (Join-Path $vcpkgInstalledRoot 'share\ffmpeg\copyright'); Target = 'licenses\ffmpeg\copyright.txt' },
        @{ Source = (Join-Path $vcpkgInstalledRoot 'share\opencv4\copyright'); Target = 'licenses\opencv\copyright.txt' },
        @{ Source = (Join-Path $vcpkgInstalledRoot 'share\kissfft\copyright'); Target = 'licenses\kissfft\BSD-3-Clause.txt' },
        @{ Source = (Join-Path $vcpkgInstalledRoot 'share\gtest\copyright'); Target = 'licenses\gtest\copyright.txt' }
    )
    foreach ($entry in $licenseSources) {
        Assert-ExistingFile -LiteralPath $entry.Source -Description "license source for $($entry.Target)"
        $target = Join-Path $payloadRoot $entry.Target
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
        Copy-Item -LiteralPath $entry.Source -Destination $target
    }

    $upstreamRoot = Join-Path $payloadRoot 'sbom\upstream'
    foreach ($qtSpdx in (Get-ChildItem -LiteralPath (Join-Path $QtRoot 'sbom') -File -Filter '*.spdx')) {
        $targetRoot = Join-Path $upstreamRoot 'qt'
        New-Item -ItemType Directory -Force -Path $targetRoot | Out-Null
        Copy-Item -LiteralPath $qtSpdx.FullName -Destination (Join-Path $targetRoot $qtSpdx.Name)
    }
    foreach ($package in @('ffmpeg', 'opencv4', 'kissfft', 'gtest')) {
        $source = Join-Path $vcpkgInstalledRoot "share\$package\vcpkg.spdx.json"
        Assert-ExistingFile -LiteralPath $source -Description "$package vcpkg SPDX"
        $targetRoot = Join-Path $upstreamRoot 'vcpkg'
        New-Item -ItemType Directory -Force -Path $targetRoot | Out-Null
        Copy-Item -LiteralPath $source -Destination (Join-Path $targetRoot "$package.spdx.json")
    }
}

function Get-RuntimeRecord {
    param([Parameter(Mandatory)][System.IO.FileInfo]$File, [Parameter(Mandatory)][string]$DumpbinPath)
    $relativePath = [System.IO.Path]::GetRelativePath($payloadRoot, $File.FullName).Replace('/', '\')
    if (-not $script:SourceByRelativePath.ContainsKey($relativePath)) {
        throw "Runtime file has no controlled source record: $relativePath"
    }
    $source = $script:SourceByRelativePath[$relativePath]
    $signature = Get-AuthenticodeSignature -LiteralPath $File.FullName
    return [pscustomobject][ordered]@{
        RelativePath = $relativePath
        Length = $File.Length
        SHA256 = (Get-FileHash -LiteralPath $File.FullName -Algorithm SHA256).Hash
        Component = $source.Component
        ComponentVersion = $source.ComponentVersion
        BuildConfigurationHash = $source.BuildConfigurationHash
        SourcePath = $source.SourcePath
        Architecture = 'x86_64'
        Linkage = 'dynamic'
        AuthenticodeStatus = $signature.Status.ToString()
        SignerSubject = if ($null -ne $signature.SignerCertificate) { $signature.SignerCertificate.Subject } else { '' }
        SignerThumbprint = if ($null -ne $signature.SignerCertificate) { $signature.SignerCertificate.Thumbprint } else { '' }
    }
}

function Write-SpdxDocument {
    param(
        [Parameter(Mandatory)][object[]]$RuntimeRecords,
        [Parameter(Mandatory)][string]$GitCommit,
        [Parameter(Mandatory)][datetimeoffset]$CreatedUtc
    )
    $created = $CreatedUtc.ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    $packages = @(
        [ordered]@{ SPDXID = 'SPDXRef-Package-SpaceRhythm'; name = 'Space Rhythm'; versionInfo = $Version; downloadLocation = 'NOASSERTION'; filesAnalyzed = $false; licenseConcluded = 'NOASSERTION'; licenseDeclared = 'NOASSERTION'; copyrightText = 'NOASSERTION' },
        [ordered]@{ SPDXID = 'SPDXRef-Package-Qt'; name = 'Qt'; versionInfo = '6.11.2'; downloadLocation = 'https://download.qt.io/archive/qt/6.11/6.11.2/single/qt-everywhere-src-6.11.2.tar.xz'; filesAnalyzed = $false; licenseConcluded = 'LGPL-3.0-only'; licenseDeclared = 'LGPL-3.0-only'; copyrightText = 'NOASSERTION'; externalRefs = @([ordered]@{ referenceCategory = 'PACKAGE-MANAGER'; referenceType = 'purl'; referenceLocator = 'pkg:generic/qt@6.11.2' }) },
        [ordered]@{ SPDXID = 'SPDXRef-Package-FFmpeg'; name = 'FFmpeg'; versionInfo = '8.1.2#3'; downloadLocation = 'NOASSERTION'; filesAnalyzed = $false; licenseConcluded = 'LGPL-3.0-or-later'; licenseDeclared = 'LGPL-3.0-or-later'; copyrightText = 'NOASSERTION'; externalRefs = @([ordered]@{ referenceCategory = 'PACKAGE-MANAGER'; referenceType = 'purl'; referenceLocator = 'pkg:github/ffmpeg/ffmpeg@8.1.2' }) },
        [ordered]@{ SPDXID = 'SPDXRef-Package-OpenCV'; name = 'OpenCV'; versionInfo = '4.12.0#7'; downloadLocation = 'NOASSERTION'; filesAnalyzed = $false; licenseConcluded = 'Apache-2.0'; licenseDeclared = 'Apache-2.0'; copyrightText = 'NOASSERTION'; externalRefs = @([ordered]@{ referenceCategory = 'PACKAGE-MANAGER'; referenceType = 'purl'; referenceLocator = 'pkg:github/opencv/opencv@4.12.0' }) },
        [ordered]@{ SPDXID = 'SPDXRef-Package-KissFFT'; name = 'KissFFT'; versionInfo = '131.2.0'; downloadLocation = 'NOASSERTION'; filesAnalyzed = $false; licenseConcluded = 'BSD-3-Clause'; licenseDeclared = 'BSD-3-Clause'; copyrightText = 'NOASSERTION'; externalRefs = @([ordered]@{ referenceCategory = 'PACKAGE-MANAGER'; referenceType = 'purl'; referenceLocator = 'pkg:github/mborgerding/kissfft@131.2.0' }) },
        [ordered]@{ SPDXID = 'SPDXRef-Package-GTest'; name = 'GoogleTest'; versionInfo = '1.17.0#3'; downloadLocation = 'NOASSERTION'; filesAnalyzed = $false; licenseConcluded = 'BSD-3-Clause'; licenseDeclared = 'BSD-3-Clause'; copyrightText = 'NOASSERTION'; externalRefs = @([ordered]@{ referenceCategory = 'PACKAGE-MANAGER'; referenceType = 'purl'; referenceLocator = 'pkg:github/google/googletest@1.17.0' }); primaryPackagePurpose = 'LIBRARY'; comment = 'Build and test dependency only; no runtime files are packaged.' },
        [ordered]@{ SPDXID = 'SPDXRef-Package-MicrosoftD3DRedist'; name = 'Microsoft Windows SDK D3D Redist'; versionInfo = $env:WindowsSDKVersion.TrimEnd('\'); supplier = 'Organization: Microsoft Corporation'; downloadLocation = 'NOASSERTION'; filesAnalyzed = $false; licenseConcluded = 'NOASSERTION'; licenseDeclared = 'NOASSERTION'; copyrightText = 'NOASSERTION'; comment = 'Copied only when selected by the frozen windeployqt mapping; redistribution review remains open.' }
    )
    $files = [System.Collections.Generic.List[object]]::new()
    $relationships = [System.Collections.Generic.List[object]]::new()
    $relationships.Add([ordered]@{ spdxElementId = 'SPDXRef-DOCUMENT'; relationshipType = 'DESCRIBES'; relatedSpdxElement = 'SPDXRef-Package-SpaceRhythm' })
    foreach ($componentId in @('Qt', 'FFmpeg', 'OpenCV', 'KissFFT', 'GTest', 'MicrosoftD3DRedist')) {
        $relationships.Add([ordered]@{ spdxElementId = 'SPDXRef-Package-SpaceRhythm'; relationshipType = 'DEPENDS_ON'; relatedSpdxElement = "SPDXRef-Package-$componentId" })
    }
    $index = 0
    foreach ($record in $RuntimeRecords) {
        $index++
        $fileId = "SPDXRef-File-$('{0:D4}' -f $index)"
        $files.Add([ordered]@{
            SPDXID = $fileId
            fileName = ('./' + $record.RelativePath.Replace('\', '/'))
            checksums = @([ordered]@{ algorithm = 'SHA256'; checksumValue = $record.SHA256.ToLowerInvariant() })
            licenseConcluded = 'NOASSERTION'
            copyrightText = 'NOASSERTION'
        })
        $ownerId = switch ($record.Component) {
            'Qt' { 'SPDXRef-Package-Qt' }
            'FFmpeg' { 'SPDXRef-Package-FFmpeg' }
            'OpenCV' { 'SPDXRef-Package-OpenCV' }
            'KissFFT' { 'SPDXRef-Package-KissFFT' }
            'Microsoft Windows SDK D3D Redist' { 'SPDXRef-Package-MicrosoftD3DRedist' }
            default { 'SPDXRef-Package-SpaceRhythm' }
        }
        $relationships.Add([ordered]@{ spdxElementId = $ownerId; relationshipType = 'CONTAINS'; relatedSpdxElement = $fileId })
    }
    $document = [ordered]@{
        spdxVersion = 'SPDX-2.3'
        dataLicense = 'CC0-1.0'
        SPDXID = 'SPDXRef-DOCUMENT'
        name = "$bundleName-runtime"
        documentNamespace = "https://space-rhythm.local/spdx/$Version/$GitCommit"
        creationInfo = [ordered]@{ created = $created; creators = @('Tool: tooling/windows/Invoke-UnsignedRelease.ps1') }
        documentDescribes = @('SPDXRef-Package-SpaceRhythm')
        packages = $packages
        files = @($files)
        relationships = @($relationships)
    }
    $document | ConvertTo-Json -Depth 12 |
        Set-Content -LiteralPath (Join-Path $payloadRoot 'sbom\space-rhythm.spdx.json') -Encoding utf8
}

function Write-HashManifest {
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][string]$Destination,
        [string[]]$ExcludeRelativePaths = @()
    )
    $excluded = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    foreach ($path in $ExcludeRelativePaths) {
        [void]$excluded.Add($path.Replace('/', '\'))
    }
    Get-ChildItem -LiteralPath $Root -Recurse -File |
        ForEach-Object {
            $relative = [System.IO.Path]::GetRelativePath($Root, $_.FullName).Replace('/', '\')
            if (-not $excluded.Contains($relative)) {
                [pscustomobject][ordered]@{
                    RelativePath = $relative
                    Length = $_.Length
                    SHA256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
                }
            }
        } |
        Sort-Object RelativePath |
        Export-Csv -LiteralPath $Destination -NoTypeInformation -Encoding utf8
}

function New-DeterministicZip {
    param(
        [Parameter(Mandatory)][string]$SourceDirectory,
        [Parameter(Mandatory)][string]$Destination,
        [Parameter(Mandatory)][datetimeoffset]$Timestamp
    )
    Assert-OutputTarget -LiteralPath $Destination
    if (Test-Path -LiteralPath $Destination) {
        Remove-Item -LiteralPath $Destination -Force
    }
    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $stream = [System.IO.File]::Open($Destination, [System.IO.FileMode]::CreateNew)
    try {
        $archive = [System.IO.Compression.ZipArchive]::new(
            $stream, [System.IO.Compression.ZipArchiveMode]::Create, $false)
        try {
            $base = Split-Path -Parent $SourceDirectory
            foreach ($file in (Get-ChildItem -LiteralPath $SourceDirectory -Recurse -File | Sort-Object FullName)) {
                $entryName = [System.IO.Path]::GetRelativePath($base, $file.FullName).Replace('\', '/')
                $entry = $archive.CreateEntry($entryName, [System.IO.Compression.CompressionLevel]::Optimal)
                $entry.LastWriteTime = $Timestamp
                $entryStream = $entry.Open()
                try {
                    $inputStream = [System.IO.File]::OpenRead($file.FullName)
                    try { $inputStream.CopyTo($entryStream) } finally { $inputStream.Dispose() }
                }
                finally { $entryStream.Dispose() }
            }
        }
        finally { $archive.Dispose() }
    }
    finally { $stream.Dispose() }
}

Reset-OutputDirectory -LiteralPath $EvidenceRoot
New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
Assert-OutputTarget -LiteralPath $bundleRoot
if (Test-Path -LiteralPath $bundleRoot) {
    Remove-Item -LiteralPath $bundleRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $payloadBin, $manifestRoot | Out-Null

$vs = Import-VsDevEnvironment
Assert-ExistingFile -LiteralPath $vs.CMake -Description 'CMake executable'
Assert-ExistingFile -LiteralPath $vs.Dumpbin -Description 'dumpbin.exe'

$qtRootFull = [System.IO.Path]::GetFullPath($QtRoot)
$windeployqt = Join-Path $qtRootFull 'bin\windeployqt.exe'
$windowsD3dRedistRoot = [System.IO.Path]::GetFullPath(
    (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Redist\D3D\x64'))
Assert-ExpectedHash -LiteralPath (Join-Path $qtRootFull 'bin\Qt6Core.dll') `
    -Expected $expectedQtCoreSha256 -Description 'Qt6Core.dll'
Assert-ExpectedHash -LiteralPath (Join-Path $qtRootFull 'config.summary') `
    -Expected $expectedQtSummarySha256 -Description 'Qt config.summary'
Assert-ExpectedHash -LiteralPath $windeployqt -Expected $expectedWindeployqtSha256 `
    -Description 'windeployqt.exe'

$vcpkgRootFull = [System.IO.Path]::GetFullPath($VcpkgRoot)
$vcpkgCommit = (& git.exe -c "safe.directory=$($vcpkgRootFull.Replace('\', '/'))" `
    -C $vcpkgRootFull rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $vcpkgCommit -ne $expectedVcpkgBaseline) {
    throw "vcpkg checkout must be $expectedVcpkgBaseline; found $vcpkgCommit"
}
foreach ($packageName in @('ffmpeg', 'opencv4', 'kissfft', 'gtest')) {
    $script:VcpkgPackages[$packageName] = Get-InstalledVcpkgPackage -Name $packageName
}

$gitCommit = (& git.exe -C $repositoryRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $gitCommit -notmatch '^[0-9a-f]{40}$') {
    throw 'Unable to resolve the source Git commit'
}
$commitTimestampText = (& git.exe -C $repositoryRoot show -s --format=%cI $gitCommit).Trim()
$commitTimestamp = [datetimeoffset]::Parse($commitTimestampText).ToUniversalTime()
$gitStatus = @(& git.exe -C $repositoryRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to inspect the source worktree status'
}
if ($gitStatus.Count -gt 0 -and -not $AllowDirtySource) {
    throw 'The source worktree is not clean. Commit the intended source or pass -AllowDirtySource for a non-candidate engineering package.'
}

$cmakeCache = Join-Path $buildRoot 'CMakeCache.txt'
Assert-ExistingFile -LiteralPath $cmakeCache -Description 'configured Release CMake cache'
$cacheText = Get-Content -Raw -LiteralPath $cmakeCache
foreach ($expectedCacheEntry in @(
    'CMAKE_BUILD_TYPE:STRING=Release',
    'SPACE_RHYTHM_QT_ROOT:PATH=C:/sr/q/qt6112',
    'VCPKG_TARGET_TRIPLET:STRING=x64-windows-space-rhythm')) {
    if ($cacheText -notmatch "(?m)^$([regex]::Escape($expectedCacheEntry))\r?$") {
        throw "Release build cache does not contain the frozen input: $expectedCacheEntry"
    }
}
$expectedSourceRoot = $sourceRoot.Replace('\', '/')
if ($cacheText -notmatch "(?m)^CMAKE_HOME_DIRECTORY:INTERNAL=$([regex]::Escape($expectedSourceRoot))\r?$") {
    throw "Release build cache does not use the migrated product workspace: $sourceRoot"
}
[void](Invoke-CapturedProcess -FilePath $vs.CMake `
    -Arguments @('--build', $buildRoot, '--parallel', $Parallel.ToString()) `
    -LogStem 'cmake-build-release')

$applicationSource = Join-Path $buildRoot 'space-rhythm.exe'
$workerSource = Join-Path $buildRoot 'space-rhythm-worker.exe'
Assert-ExistingFile -LiteralPath $applicationSource -Description 'Release application'
Assert-ExistingFile -LiteralPath $workerSource -Description 'Release worker'
Copy-Item -LiteralPath $applicationSource -Destination (Join-Path $payloadBin 'space-rhythm.exe')
Copy-Item -LiteralPath $workerSource -Destination (Join-Path $payloadBin 'space-rhythm-worker.exe')
Add-SourceRecord -RelativePath 'bin\space-rhythm.exe' -SourcePath $applicationSource `
    -Component 'Space Rhythm' -ComponentVersion $Version -BuildConfigurationHash "git:$gitCommit"
Add-SourceRecord -RelativePath 'bin\space-rhythm-worker.exe' -SourcePath $workerSource `
    -Component 'Space Rhythm' -ComponentVersion $Version -BuildConfigurationHash "git:$gitCommit"

$deployArguments = @(
    '--release', '--no-translations', '--no-compiler-runtime',
    '--skip-plugin-types', 'qmltooling,generic',
    '--include-plugins', 'qoffscreen',
    '--qmldir', (Join-Path $sourceRoot 'src\app\qml'),
    '--dir', $payloadBin,
    (Join-Path $payloadBin 'space-rhythm.exe'))
$dryRunArguments = @('--dry-run', '--list', 'mapping') + $deployArguments
$dryRun = Invoke-CapturedProcess -FilePath $windeployqt -Arguments $dryRunArguments -LogStem 'windeployqt-dry-run'
$mappings = [System.Collections.Generic.List[object]]::new()
foreach ($line in ($dryRun.StdOut -split "`r?`n")) {
    if ($line -match '^"([^"]+)"\s+"([^"]+)"$') {
        $source = [System.IO.Path]::GetFullPath($Matches[1])
        $target = $Matches[2].Replace('/', '\')
        if ([System.IO.Path]::IsPathRooted($target) -or $target -match '(^|\\)\.\.(\\|$)') {
            throw "windeployqt returned an unsafe target path: $target"
        }
        $qtPrefix = $qtRootFull.TrimEnd('\') + '\'
        $component = 'Qt'
        $componentVersion = '6.11.2'
        if (-not $source.StartsWith($qtPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
            $d3dPrefix = $windowsD3dRedistRoot.TrimEnd('\') + '\'
            $allowedD3dTargets = @('d3dcompiler_47.dll', 'dxcompiler.dll', 'dxil.dll')
            if (-not $source.StartsWith($d3dPrefix, [System.StringComparison]::OrdinalIgnoreCase) -or
                $target -notin $allowedD3dTargets) {
                throw "windeployqt mapping escaped the frozen Qt/Windows SDK Redist roots: $source"
            }
            $signature = Get-AuthenticodeSignature -LiteralPath $source
            if ($signature.Status -ne [System.Management.Automation.SignatureStatus]::Valid -or
                $null -eq $signature.SignerCertificate -or
                $signature.SignerCertificate.Subject -notmatch 'O=Microsoft Corporation') {
                throw "Windows SDK D3D runtime is not validly Microsoft-signed: $source"
            }
            $component = 'Microsoft Windows SDK D3D Redist'
            $componentVersion = $env:WindowsSDKVersion.TrimEnd('\')
        }
        if ($target -match '^(qmltooling|generic)(\\|$)' -or $target -ieq 'vc_redist.x64.exe') {
            throw "windeployqt dry-run contains a forbidden release item: $target"
        }
        $mappings.Add([pscustomobject]@{
            Source = $source
            Target = $target
            Component = $component
            ComponentVersion = $componentVersion
        })
    }
}
if ($mappings.Count -eq 0) {
    throw 'windeployqt dry-run returned no parseable mapping'
}
foreach ($required in @('Qt6QmlMeta.dll', 'platforms\qwindows.dll', 'platforms\qoffscreen.dll')) {
    if (-not ($mappings.Target -icontains $required)) {
        throw "windeployqt dry-run omitted required runtime file: $required"
    }
}
$mappings | Sort-Object Target | Export-Csv -LiteralPath (Join-Path $EvidenceRoot 'windeployqt-mapping.csv') `
    -NoTypeInformation -Encoding utf8

[void](Invoke-CapturedProcess -FilePath $windeployqt -Arguments $deployArguments -LogStem 'windeployqt-deploy')
foreach ($mapping in $mappings) {
    $deployed = Join-Path $payloadBin $mapping.Target
    Assert-ExistingFile -LiteralPath $deployed -Description "deployed Qt runtime $($mapping.Target)"
    $sourceHash = (Get-FileHash -LiteralPath $mapping.Source -Algorithm SHA256).Hash
    $deployedHash = (Get-FileHash -LiteralPath $deployed -Algorithm SHA256).Hash
    if ($sourceHash -ne $deployedHash) {
        throw "Deployed Qt runtime hash mismatch: $($mapping.Target)"
    }
    Add-SourceRecord -RelativePath ("bin\" + $mapping.Target) -SourcePath $mapping.Source `
        -Component $mapping.Component -ComponentVersion $mapping.ComponentVersion `
        -BuildConfigurationHash $(if ($mapping.Component -eq 'Qt') { "qt-config-summary:$expectedQtSummarySha256" } else { "windows-sdk:$($env:WindowsSDKVersion.TrimEnd('\'))" })
}

Resolve-RuntimeClosure -DumpbinPath $vs.Dumpbin
foreach ($forbidden in @('qmltooling', 'generic')) {
    if (Test-Path -LiteralPath (Join-Path $payloadBin $forbidden)) {
        throw "Forbidden development plugin directory entered the payload: $forbidden"
    }
}
if (Test-Path -LiteralPath (Join-Path $payloadBin 'vc_redist.x64.exe')) {
    throw 'vc_redist.x64.exe entered the payload before the runtime strategy was confirmed'
}

Copy-LicenseEvidence
New-Item -ItemType Directory -Force -Path (Join-Path $payloadRoot 'docs'), (Join-Path $payloadRoot 'notices'), `
    (Join-Path $payloadRoot 'signing'), (Join-Path $payloadRoot 'sbom') | Out-Null

$notices = @"
Space Rhythm unsigned engineering package $Version

This notice is an engineering inventory, not legal advice or a production release approval.

- Qt 6.11.2: dynamically linked under the project's confirmed LGPLv3 path. See licenses/qt and sbom/upstream/qt.
- FFmpeg 8.1.2#3: vcpkg LGPLv3-compatible feature set; GPL/nonfree features are disabled. See licenses/ffmpeg.
- OpenCV 4.12.0#7: build-time classic-analysis dependency; no OpenCV DLL is included unless the PE closure requires it. See licenses/opencv.
- KissFFT 131.2.0: BSD-3-Clause. See licenses/kissfft.
- GoogleTest 1.17.0#3: build/test dependency only; it is not part of the runtime closure. See licenses/gtest.
- Microsoft system/D3D components: system dependencies and any Microsoft-signed files are inventoried in the manifests; redistribution review remains open.

The authoritative per-file hashes and upstream SPDX materials are under manifest/ and sbom/.
"@
Set-Content -LiteralPath (Join-Path $payloadRoot 'notices\THIRD-PARTY-NOTICES.txt') `
    -Value $notices -Encoding utf8

$replacementGuide = @"
# Qt source, build and replacement guide

This unsigned engineering package uses Qt 6.11.2 shared libraries. The source archive is
`qt-everywhere-src-6.11.2.tar.xz`, SHA-256
`6dcfbca271d76a6502741a2c0dc6fc98ef7dd0b7b4cfd0abcebb285a86a26f33`, from the official Qt archive.
The validated local build inputs and Qt module SPDX documents are recorded in `manifest/build-inputs.json`
and `sbom/upstream/qt/`.

To test a compatible replacement, first copy the entire installed application directory to a backup.
Replace only the application-private Qt DLL/plugin/QML files with x64 MSVC 19.44-compatible Qt 6.11.2
shared builds, preserving relative paths. Recompute SHA-256, verify PE x64 and imports, then run the App
and Worker smoke paths. Restore the backup on any load, ABI, QML, or functional failure. Do not replace
Windows system files and do not register Qt globally.

This engineering instruction does not change the applicable license terms and is not legal advice.
"@
Set-Content -LiteralPath (Join-Path $payloadRoot 'docs\qt-source-build-and-replacement.md') `
    -Value $replacementGuide -Encoding utf8

$knownLimitations = @"
# Known limitations of this unsigned engineering package

- This package is limited by D-012/D-013 to the project owner on self-owned Windows computers. It must
  not be publicly distributed or delivered to a third party, and makes no SAC/WDAC compatibility claim.
- Every Space Rhythm, self-built Qt and vcpkg binary remains unsigned. A computer that blocks unsigned
  Win32 programs may reject the applications, tools, or DLLs; no trusted publisher chain is provided.
- The included explicit-path transaction tool is the personal engineering delivery mechanism. A separate
  GUI installer, auto-update and file association are outside the confirmed scope.
- The VC Runtime deployment strategy and minimum Windows version are unconfirmed. No `vc_redist.x64.exe`
  or private VC Runtime is bundled.
- Product container/H.264/AAC choices are unconfirmed; export remains explicitly `testOnly`.
- The application currently embeds development CC0 test timbres. No product default timbre is approved,
  so this package is never candidate-eligible.
- Qt Quick Controls styles are not pruned until the product style is confirmed.
- T-038 established the pre-migration personal-delivery baseline. Every newly generated package still
  requires current-host validation; packaging success alone is not release approval.
"@
Set-Content -LiteralPath (Join-Path $payloadRoot 'docs\known-limitations.md') `
    -Value $knownLimitations -Encoding utf8

$runtimeFiles = @(Get-ChildItem -LiteralPath $payloadBin -Recurse -File |
    Where-Object { $_.Extension -in @('.exe', '.dll') })
$runtimeRecords = @($runtimeFiles | ForEach-Object { Get-RuntimeRecord -File $_ -DumpbinPath $vs.Dumpbin } |
    Sort-Object RelativePath)
$runtimeRecords | Export-Csv -LiteralPath (Join-Path $manifestRoot 'runtime-files.sha256.csv') `
    -NoTypeInformation -Encoding utf8

$missingInputs = @(
    'minimum supported Windows version',
    'VC Runtime redistribution strategy',
    'production container/H.264/AAC backend',
    'current-package current-host personal delivery validation evidence',
    'product timbre and visual style approval')
$buildInputs = [ordered]@{
    schemaVersion = 3
    packageKind = 'unsigned-engineering'
    product = 'Space Rhythm'
    version = $Version
    sourceCommit = $gitCommit
    sourceWorktreeClean = ($gitStatus.Count -eq 0)
    sourceWorktreeStatus = @($gitStatus)
    candidateEligible = $false
    pathLayout = [ordered]@{
        repositoryRoot = $repositoryRoot
        productWorkspaceRelativePath = [System.IO.Path]::GetRelativePath($repositoryRoot, $sourceRoot).Replace('\', '/')
        productSourceRoot = $sourceRoot
        productOutputRoot = $outRoot
        releaseBuildRoot = $buildRoot
        vcpkgInstalledRoot = $vcpkgInstalledRoot
        packageOutputRoot = $OutputRoot
    }
    deliveryScope = [ordered]@{
        mode = 'personal-unsigned'
        intendedUser = 'project owner'
        selfOwnedWindowsOnly = $true
        publicDistributionAllowed = $false
        thirdPartyDeliveryAllowed = $false
        sacWdacCompatibilityClaim = 'none'
        validationHost = 'TIGER'
        decisionIds = @('D-012', 'D-013')
    }
    preset = $Preset
    architecture = 'x86_64'
    toolchain = [ordered]@{
        msvc = '19.44.35228'
        windowsSdk = $env:WindowsSDKVersion.TrimEnd('\')
        cmake = (& $vs.CMake --version | Select-Object -First 1)
        qt = '6.11.2'
        qtRoot = $qtRootFull
        qtCoreSha256 = $expectedQtCoreSha256
        qtConfigSummarySha256 = $expectedQtSummarySha256
        windeployqtSha256 = $expectedWindeployqtSha256
        vcpkgBaseline = $expectedVcpkgBaseline
    }
    deployment = [ordered]@{
        windeployqtArguments = $deployArguments
        dryRunMappingCount = $mappings.Count
        runtimeFileCount = $runtimeRecords.Count
        excluded = @('qmltooling', 'generic', 'translations', 'vc_redist.x64.exe', 'debug CRT', 'test executables', 'build tools')
        systemDependencies = @($script:SystemDependencies | Sort-Object)
    }
    components = @(
        [ordered]@{
            name = 'Space Rhythm'; version = $Version; purl = "pkg:generic/space-rhythm@$Version"
            sourceUrl = 'NOASSERTION (repository-local source)'; sourceCommitOrArchiveHash = "git:$gitCommit"
            buildConfigurationHash = "git:$gitCommit"; architecture = 'x86_64'; linkage = 'application'
            licenseExpression = 'NOASSERTION'; licenseTextPath = ''; copyrightNotice = 'NOASSERTION'
            modified = $true; patches = @(); redistributionBasis = 'Project-owned application code'
            supplier = 'Organization: Space Rhythm project'
            runtimeRelativePath = @($runtimeRecords | Where-Object { $_.Component -eq 'Space Rhythm' } | ForEach-Object { $_.RelativePath })
            fileSha256 = @($runtimeRecords | Where-Object { $_.Component -eq 'Space Rhythm' } | ForEach-Object { [ordered]@{ relativePath = $_.RelativePath; sha256 = $_.SHA256 } })
            authenticodeStatus = @($runtimeRecords | Where-Object { $_.Component -eq 'Space Rhythm' } | ForEach-Object { $_.AuthenticodeStatus } | Sort-Object -Unique)
            signerThumbprint = @($runtimeRecords | Where-Object { $_.Component -eq 'Space Rhythm' -and $_.SignerThumbprint } | ForEach-Object { $_.SignerThumbprint } | Sort-Object -Unique)
            taskEvidence = @('projects/space-rhythm/artifacts/A-033-windows-unsigned-deployment-and-transaction-pipeline.md')
        },
        [ordered]@{
            name = 'Qt'; version = '6.11.2'; purl = 'pkg:generic/qt@6.11.2'
            sourceUrl = 'https://download.qt.io/archive/qt/6.11/6.11.2/single/qt-everywhere-src-6.11.2.tar.xz'
            sourceCommitOrArchiveHash = 'sha256:6DCFBCA271D76A6502741A2C0DC6FC98EF7DD0B7B4CFD0ABCEBB285A86A26F33'
            buildConfigurationHash = "qt-config-summary:$expectedQtSummarySha256"; architecture = 'x86_64'; linkage = 'dynamic'
            licenseExpression = 'LGPL-3.0-only'; licenseTextPath = 'licenses/qt/LGPL-3.0-only.txt'
            copyrightNotice = 'See licenses/qt and sbom/upstream/qt'; modified = $false; patches = @()
            redistributionBasis = 'Project-confirmed LGPLv3 dynamic-link path; legal review not implied'
            supplier = 'Organization: The Qt Company and Qt Project contributors'
            runtimeRelativePath = @($runtimeRecords | Where-Object { $_.Component -eq 'Qt' } | ForEach-Object { $_.RelativePath })
            fileSha256 = @($runtimeRecords | Where-Object { $_.Component -eq 'Qt' } | ForEach-Object { [ordered]@{ relativePath = $_.RelativePath; sha256 = $_.SHA256 } })
            authenticodeStatus = @($runtimeRecords | Where-Object { $_.Component -eq 'Qt' } | ForEach-Object { $_.AuthenticodeStatus } | Sort-Object -Unique)
            signerThumbprint = @($runtimeRecords | Where-Object { $_.Component -eq 'Qt' -and $_.SignerThumbprint } | ForEach-Object { $_.SignerThumbprint } | Sort-Object -Unique)
            taskEvidence = @('projects/space-rhythm/evidence/T-012/verification-summary.md', 'sbom/upstream/qt')
        },
        [ordered]@{
            name = 'FFmpeg'; version = $script:VcpkgPackages.ffmpeg.Version; purl = 'pkg:github/ffmpeg/ffmpeg@8.1.2'
            sourceUrl = 'git+https://github.com/ffmpeg/ffmpeg@n8.1.2'
            sourceCommitOrArchiveHash = 'sha512:c72f4062aecc16d8b2b1e8678d5efe3af4cfaa0cc7c0997052248f9e499e60c2463acf07877cf3b78b246ce3e8078cb043e8d97e90a6b50d06af32ff7369a788'
            buildConfigurationHash = "vcpkg-abi:$($script:VcpkgPackages.ffmpeg.Abi)"; architecture = 'x86_64'; linkage = 'dynamic'
            licenseExpression = 'LGPL-3.0-or-later'; licenseTextPath = 'licenses/ffmpeg/copyright.txt'
            copyrightNotice = 'See licenses/ffmpeg and sbom/upstream/vcpkg/ffmpeg.spdx.json'; modified = $true
            patches = @('Recorded in sbom/upstream/vcpkg/ffmpeg.spdx.json'); redistributionBasis = 'vcpkg LGPLv3-compatible feature set; GPL/nonfree disabled'
            supplier = 'Organization: FFmpeg project contributors'
            runtimeRelativePath = @($runtimeRecords | Where-Object { $_.Component -eq 'FFmpeg' } | ForEach-Object { $_.RelativePath })
            fileSha256 = @($runtimeRecords | Where-Object { $_.Component -eq 'FFmpeg' } | ForEach-Object { [ordered]@{ relativePath = $_.RelativePath; sha256 = $_.SHA256 } })
            authenticodeStatus = @($runtimeRecords | Where-Object { $_.Component -eq 'FFmpeg' } | ForEach-Object { $_.AuthenticodeStatus } | Sort-Object -Unique)
            signerThumbprint = @($runtimeRecords | Where-Object { $_.Component -eq 'FFmpeg' -and $_.SignerThumbprint } | ForEach-Object { $_.SignerThumbprint } | Sort-Object -Unique)
            taskEvidence = @('projects/space-rhythm/artifacts/A-015-ffmpeg-media-pipeline.md', 'sbom/upstream/vcpkg/ffmpeg.spdx.json')
        },
        [ordered]@{
            name = 'OpenCV'; version = $script:VcpkgPackages.opencv4.Version; purl = 'pkg:github/opencv/opencv@4.12.0'
            sourceUrl = 'git+https://github.com/opencv/opencv@4.12.0'
            sourceCommitOrArchiveHash = 'sha512:8ac63ddd61e22cc0eaeafee4f30ae6e1cab05fc4929e2cea29070203b9ca8dfead12cc0fd7c4a87b65c1e20ec6b9ab4865a1b83fad33d114fc0708fdf107c51b'
            buildConfigurationHash = "vcpkg-abi:$($script:VcpkgPackages.opencv4.Abi)"; architecture = 'x86_64'; linkage = 'build-input'
            licenseExpression = 'Apache-2.0'; licenseTextPath = 'licenses/opencv/copyright.txt'
            copyrightNotice = 'See licenses/opencv and sbom/upstream/vcpkg/opencv4.spdx.json'; modified = $true
            patches = @('Recorded in sbom/upstream/vcpkg/opencv4.spdx.json'); redistributionBasis = 'Build input; no OpenCV runtime file is currently packaged'
            supplier = 'Organization: OpenCV project contributors'; runtimeRelativePath = @()
            fileSha256 = @(); authenticodeStatus = @(); signerThumbprint = @()
            taskEvidence = @('projects/space-rhythm/artifacts/A-020-classic-video-analysis-pipeline.md', 'sbom/upstream/vcpkg/opencv4.spdx.json')
        },
        [ordered]@{
            name = 'KissFFT'; version = $script:VcpkgPackages.kissfft.Version; purl = 'pkg:github/mborgerding/kissfft@131.2.0'
            sourceUrl = 'git+https://github.com/mborgerding/kissfft@131.2.0'
            sourceCommitOrArchiveHash = 'sha512:5d02802a9e191e7cb77c26e9a34659a5d47c4e85bcfdf86a7cffdda66d8b79261f7fe5795ffabd78644b6094c01b32a84841669fbc0009ac9268ae1ba521af9e'
            buildConfigurationHash = "vcpkg-abi:$($script:VcpkgPackages.kissfft.Abi)"; architecture = 'x86_64'; linkage = 'dynamic'
            licenseExpression = 'BSD-3-Clause'; licenseTextPath = 'licenses/kissfft/BSD-3-Clause.txt'
            copyrightNotice = 'See licenses/kissfft and sbom/upstream/vcpkg/kissfft.spdx.json'; modified = $true
            patches = @('Recorded in sbom/upstream/vcpkg/kissfft.spdx.json'); redistributionBasis = 'BSD-3-Clause redistribution'
            supplier = 'Person: KissFFT contributors'
            runtimeRelativePath = @($runtimeRecords | Where-Object { $_.Component -eq 'KissFFT' } | ForEach-Object { $_.RelativePath })
            fileSha256 = @($runtimeRecords | Where-Object { $_.Component -eq 'KissFFT' } | ForEach-Object { [ordered]@{ relativePath = $_.RelativePath; sha256 = $_.SHA256 } })
            authenticodeStatus = @($runtimeRecords | Where-Object { $_.Component -eq 'KissFFT' } | ForEach-Object { $_.AuthenticodeStatus } | Sort-Object -Unique)
            signerThumbprint = @($runtimeRecords | Where-Object { $_.Component -eq 'KissFFT' -and $_.SignerThumbprint } | ForEach-Object { $_.SignerThumbprint } | Sort-Object -Unique)
            taskEvidence = @('projects/space-rhythm/artifacts/A-026-audio-analysis-pipeline-and-gates.md', 'sbom/upstream/vcpkg/kissfft.spdx.json')
        },
        [ordered]@{
            name = 'GoogleTest'; version = $script:VcpkgPackages.gtest.Version; purl = 'pkg:github/google/googletest@1.17.0'
            sourceUrl = 'git+https://github.com/google/googletest@v1.17.0'
            sourceCommitOrArchiveHash = 'sha512:0f57e9ef06925e5b7722df1eb92ef5850e8dce79220ea16a8aaff586a71c0b01460ef1713649ee24ffedb2e6ad5a51e9198c5a5ae1b2789e43feb1f494e7d45c'
            buildConfigurationHash = "vcpkg-abi:$($script:VcpkgPackages.gtest.Abi)"; architecture = 'x86_64'; linkage = 'development'
            licenseExpression = 'BSD-3-Clause'; licenseTextPath = 'licenses/gtest/copyright.txt'
            copyrightNotice = 'See licenses/gtest and sbom/upstream/vcpkg/gtest.spdx.json'; modified = $true
            patches = @('Recorded in sbom/upstream/vcpkg/gtest.spdx.json'); redistributionBasis = 'Build/test dependency only; excluded from runtime'
            supplier = 'Organization: GoogleTest contributors'; runtimeRelativePath = @()
            fileSha256 = @(); authenticodeStatus = @(); signerThumbprint = @()
            taskEvidence = @('sbom/upstream/vcpkg/gtest.spdx.json')
        },
        [ordered]@{
            name = 'Microsoft Windows SDK D3D Redist'; version = $env:WindowsSDKVersion.TrimEnd('\')
            purl = "pkg:generic/microsoft-windows-sdk-d3d-redist@$($env:WindowsSDKVersion.TrimEnd('\'))"
            sourceUrl = 'NOASSERTION (installed Windows SDK D3D x64 Redist)'
            sourceCommitOrArchiveHash = 'NOASSERTION'; buildConfigurationHash = "windows-sdk:$($env:WindowsSDKVersion.TrimEnd('\'))"
            architecture = 'x86_64'; linkage = 'dynamic'; licenseExpression = 'NOASSERTION'; licenseTextPath = ''
            copyrightNotice = 'Copyright Microsoft Corporation'; modified = $false; patches = @()
            redistributionBasis = 'Windows SDK D3D x64 Redist; redistribution review remains open'
            supplier = 'Organization: Microsoft Corporation'
            runtimeRelativePath = @($runtimeRecords | Where-Object { $_.Component -eq 'Microsoft Windows SDK D3D Redist' } | ForEach-Object { $_.RelativePath })
            fileSha256 = @($runtimeRecords | Where-Object { $_.Component -eq 'Microsoft Windows SDK D3D Redist' } | ForEach-Object { [ordered]@{ relativePath = $_.RelativePath; sha256 = $_.SHA256 } })
            authenticodeStatus = @($runtimeRecords | Where-Object { $_.Component -eq 'Microsoft Windows SDK D3D Redist' } | ForEach-Object { $_.AuthenticodeStatus } | Sort-Object -Unique)
            signerThumbprint = @($runtimeRecords | Where-Object { $_.Component -eq 'Microsoft Windows SDK D3D Redist' -and $_.SignerThumbprint } | ForEach-Object { $_.SignerThumbprint } | Sort-Object -Unique)
            taskEvidence = @('projects/space-rhythm/evidence/T-036/verification-summary.md')
        })
    missingInputs = $missingInputs
    securityBoundary = [ordered]@{
        signaturesApplied = $false
        signingCredentialAccess = 'none'
        wdacPolicyChanged = $false
        fallbackUsed = $false
        sacWdacCompatibilityValidated = $false
    }
}
$buildInputs | ConvertTo-Json -Depth 10 |
    Set-Content -LiteralPath (Join-Path $manifestRoot 'build-inputs.json') -Encoding utf8

$signingRequest = [ordered]@{
    schemaVersion = 1
    status = 'not-required-for-personal-unsigned-scope'
    packageKind = 'unsigned-engineering'
    sourceCommit = $gitCommit
    credentialAccess = 'none'
    decisionIds = @('D-012', 'D-013')
    requiredAuthorization = @('signing subject', 'certificate or managed signing service', 'timestamp service', 'channel policy')
    files = @($runtimeRecords | ForEach-Object {
        [ordered]@{ relativePath = $_.RelativePath; sha256 = $_.SHA256; authenticodeStatus = $_.AuthenticodeStatus }
    })
}
$signingRequest | ConvertTo-Json -Depth 8 |
    Set-Content -LiteralPath (Join-Path $payloadRoot 'signing\signing-request.json') -Encoding utf8

Write-SpdxDocument -RuntimeRecords $runtimeRecords -GitCommit $gitCommit -CreatedUtc $commitTimestamp
Write-HashManifest -Root $payloadRoot -Destination (Join-Path $manifestRoot 'payload-files.sha256.csv') `
    -ExcludeRelativePaths @('manifest\payload-files.sha256.csv')

New-Item -ItemType Directory -Force -Path (Join-Path $bundleRoot 'tools') | Out-Null
$transactionTool = Join-Path $sourceRoot 'tooling\windows\Invoke-UnsignedInstallTransaction.ps1'
Assert-ExistingFile -LiteralPath $transactionTool -Description 'unsigned install transaction tool'
Copy-Item -LiteralPath $transactionTool -Destination (Join-Path $bundleRoot 'tools\Invoke-UnsignedInstallTransaction.ps1')

$bundleReadme = @"
# Space Rhythm $Version unsigned engineering bundle

This bundle is unsigned, not candidate-eligible, and not approved for production release.
Under D-012/D-013 it is only for the project owner on self-owned Windows computers. Public distribution
and third-party delivery are prohibited, and no SAC/WDAC compatibility claim is made.
Verify the separately published ZIP SHA-256 first, then verify `manifest/bundle-files.sha256.csv`
before executing the included tool. That manifest detects corruption but does not authenticate this unsigned bundle.
`payload/SpaceRhythm` is the application-private payload; its nested manifest is
`payload/SpaceRhythm/manifest/payload-files.sha256.csv`.

The included PowerShell transaction tool is the personal engineering delivery mechanism; no separate GUI
installer is provided. Invoke it with explicit, non-system `-InstallRoot` and `-StateRoot` paths. It never deletes
user projects, media, settings, autosaves, cache, or logs outside those explicit roots.
"@
Set-Content -LiteralPath (Join-Path $bundleRoot 'README.md') -Value $bundleReadme -Encoding utf8
New-Item -ItemType Directory -Force -Path (Join-Path $bundleRoot 'manifest') | Out-Null
Write-HashManifest -Root $bundleRoot -Destination (Join-Path $bundleRoot 'manifest\bundle-files.sha256.csv') `
    -ExcludeRelativePaths @('manifest\bundle-files.sha256.csv')

New-DeterministicZip -SourceDirectory $bundleRoot -Destination $zipPath -Timestamp $commitTimestamp

$summary = [ordered]@{
    schemaVersion = 1
    packageKind = 'unsigned-engineering'
    candidateEligible = $false
    deliveryScope = 'personal-unsigned'
    sacWdacCompatibilityClaim = 'none'
    sourceCommit = $gitCommit
    sourceWorktreeClean = ($gitStatus.Count -eq 0)
    pathLayout = $buildInputs.pathLayout
    bundleRoot = $bundleRoot
    zipPath = $zipPath
    zipLength = (Get-Item -LiteralPath $zipPath).Length
    zipSha256 = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash
    runtimeFileCount = $runtimeRecords.Count
    runtimeBytes = ($runtimeFiles | Measure-Object -Property Length -Sum).Sum
    windeployqtMappingCount = $mappings.Count
    authenticode = @($runtimeRecords | Group-Object AuthenticodeStatus | Sort-Object Name |
        ForEach-Object { [ordered]@{ status = $_.Name; count = $_.Count } })
    missingInputs = $missingInputs
    policy = [ordered]@{ wdacChanged = $false; fallbackUsed = $false; credentialsAccessed = $false }
}
$summary | ConvertTo-Json -Depth 8 |
    Set-Content -LiteralPath (Join-Path $EvidenceRoot 'unsigned-package-summary.json') -Encoding utf8
Write-Host "Unsigned engineering bundle: $bundleRoot"
Write-Host "Archive: $zipPath"
Write-Host "SHA-256: $($summary.zipSha256)"
Write-Warning 'This output is unsigned, personal-use only, not SAC/WDAC-validated, and not a production release.'
