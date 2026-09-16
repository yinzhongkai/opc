[CmdletBinding()]
param(
    [ValidateSet('windows-msvc-x64-debug', 'windows-msvc-x64-release', 'ci-windows-msvc-x64')]
    [string]$Preset = 'windows-msvc-x64-debug',

    [ValidateSet('T-021', 'T-022', 'T-041')]
    [string]$Task = 'T-021',

    [string]$QtRoot = 'C:\sr\q\qt6112',
    [string]$VcpkgRoot = 'C:\sr\tools\vcpkg-2026.07.29',
    [string]$EvidenceRoot = '',
    [ValidateRange(1, 64)][int]$Parallel = 10,
    [switch]$Clean,
    [switch]$AllowWdacFallback
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$sourceRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$buildRoot = Join-Path $sourceRoot "out\build\$Preset"
$taskLabel = if ($Task -eq 'T-041') { 't022' } else { $Task.ToLowerInvariant().Replace('-', '') }
$commit = (& git.exe -C $sourceRoot rev-parse --short=12 HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Unable to resolve the source Git commit.' }
$utcStamp = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ')
$runId = "$utcStamp-$commit-$Preset-01"
if ([string]::IsNullOrWhiteSpace($EvidenceRoot)) {
    $EvidenceRoot = Join-Path $sourceRoot "out\evidence\$Task\$runId"
}
$EvidenceRoot = [System.IO.Path]::GetFullPath($EvidenceRoot)
$workRoot = Join-Path $EvidenceRoot 'work'
New-Item -ItemType Directory -Force -Path $EvidenceRoot, $workRoot | Out-Null

$env:TEMP = $workRoot
$env:TMP = $workRoot
$env:TZ = 'UTC'
$env:LANG = 'C'
$env:QT_QPA_PLATFORM = 'offscreen'
$env:QSG_RHI_BACKEND = 'software'
$env:SPACE_RHYTHM_TEST_SEED = '0x5350414345524859'

$buildScript = Join-Path $sourceRoot 'tooling\windows\Invoke-ProjectBuild.ps1'
$buildEvidence = Join-Path $EvidenceRoot 'build'
$buildArguments = @{
    Preset = $Preset
    QtRoot = $QtRoot
    VcpkgRoot = $VcpkgRoot
    EvidenceRoot = $buildEvidence
    Parallel = $Parallel
    UseExistingDependencies = $true
}
if ($Clean) { $buildArguments.Clean = $true }
if ($AllowWdacFallback) { $buildArguments.AllowWdacFallback = $true }

$result = [ordered]@{
    schemaVersion = 1
    task = $Task
    runId = $runId
    preset = $Preset
    status = 'not-run'
    exitCode = $null
    startedUtc = [DateTime]::UtcNow.ToString('o')
    finishedUtc = $null
    durationMs = $null
    junit = 'ctest-junit.xml'
    label = $taskLabel
    suiteReuse = if ($Task -eq 'T-041') { 'T-022 engineering gate (post-migration independent rerun)' } else { $null }
    excludedScopes = if ($Task -eq 'T-021') {
        @('T-022', 'package/', 'scripts/__pycache__/')
    }
    else {
        @('T-029 product evaluation', 'T-042 release rebuild', 'package/', 'scripts/__pycache__/')
    }
}
$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

function Get-SafeCim {
    param([Parameter(Mandatory)][string]$ClassName)
    try { return Get-CimInstance -ClassName $ClassName -ErrorAction Stop }
    catch { return $null }
}

try {
    $trackedBefore = @(& git.exe -C $sourceRoot status --porcelain --untracked-files=no)
    if ($Task -eq 'T-041' -and $trackedBefore.Count -ne 0) {
        throw 'T-041 requires a clean tracked worktree before configure/build/test.'
    }
    if ($Task -eq 'T-041') {
        $audioGenerator = Join-Path $sourceRoot 'tests\golden\audio\Generate-AudioDspVectors.ps1'
        $audioGeneratorOutput = & $audioGenerator 2>&1
        $audioGeneratorOutput | Set-Content -LiteralPath (
            Join-Path $EvidenceRoot 'prebuild-audio-golden.log') -Encoding utf8
        $trackedAfterPrebuild = @(& git.exe -C $sourceRoot status --porcelain --untracked-files=no)
        if ($trackedAfterPrebuild.Count -ne 0) {
            throw 'T-041 audio golden prebuild changed tracked inputs.'
        }
    }
    & $buildScript @buildArguments -Stage Configure
    $buildArguments.Remove('Clean')
    & $buildScript @buildArguments -Stage Build

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vsRoot = (& $vswhere -latest -products Microsoft.VisualStudio.Product.BuildTools `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath).Trim()
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($vsRoot)) {
        throw 'Unable to locate Visual Studio Build Tools.'
    }
    $ctest = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
    if (-not (Test-Path -LiteralPath $ctest -PathType Leaf)) {
        throw "CTest was not found: $ctest"
    }

    $runtimeBin = if ($Preset -eq 'windows-msvc-x64-debug') {
        Join-Path $sourceRoot 'out\vcpkg_installed\x64-windows-space-rhythm\debug\bin'
    }
    else {
        Join-Path $sourceRoot 'out\vcpkg_installed\x64-windows-space-rhythm\bin'
    }
    $env:Path = "$runtimeBin;$(Join-Path $QtRoot 'bin');$env:Path"

    $os = Get-SafeCim -ClassName Win32_OperatingSystem
    $computer = Get-SafeCim -ClassName Win32_ComputerSystem
    $cpu = @(Get-SafeCim -ClassName Win32_Processor)
    $gpu = @(Get-SafeCim -ClassName Win32_VideoController)
    $disk = @(Get-SafeCim -ClassName Win32_LogicalDisk)
    $audio = @(Get-SafeCim -ClassName Win32_SoundDevice)
    $cpuNames = @($cpu | Where-Object { $null -ne $_ } | ForEach-Object { [string]$_.Name })
    if ($cpuNames.Count -eq 0) { $cpuNames = @('unknown') }
    $gpuInventory = @($gpu | Where-Object { $null -ne $_ } | ForEach-Object {
        [ordered]@{ name = $_.Name; driverVersion = $_.DriverVersion }
    })
    if ($gpuInventory.Count -eq 0) {
        $gpuInventory = @([ordered]@{ name = 'unknown'; driverVersion = 'unknown' })
    }
    $diskInventory = @($disk | Where-Object { $null -ne $_ } | ForEach-Object {
        [ordered]@{ device = $_.DeviceID; sizeBytes = $_.Size; freeBytes = $_.FreeSpace }
    })
    if ($diskInventory.Count -eq 0) {
        $diskInventory = @([ordered]@{ device = 'unknown'; sizeBytes = 'unknown'; freeBytes = 'unknown' })
    }
    $audioNames = @($audio | Where-Object { $null -ne $_ } | ForEach-Object { [string]$_.Name })
    if ($audioNames.Count -eq 0) { $audioNames = @('unknown') }
    try { $displayLogPixels = (Get-ItemProperty -LiteralPath 'HKCU:\Control Panel\Desktop' -Name LogPixels -ErrorAction Stop).LogPixels }
    catch { $displayLogPixels = 'unknown' }
    try { $fontFileCount = @(Get-ChildItem -LiteralPath (Join-Path $env:WINDIR 'Fonts') -File -ErrorAction Stop).Count }
    catch { $fontFileCount = 'unknown' }
    try { $powerPlan = ((& powercfg.exe /getactivescheme 2>&1) | Out-String).Trim() }
    catch { $powerPlan = 'unknown' }
    $vcpkgStatus = Get-Content -Raw -LiteralPath (Join-Path $sourceRoot 'out\vcpkg_installed\vcpkg\status')
    $gtestVersion = if ($vcpkgStatus -match 'Package: gtest\r?\nVersion: ([^\r\n]+)\r?\nPort-Version: ([^\r\n]+)') {
        "$($Matches[1])#$($Matches[2])"
    }
    else { 'unknown' }
    $ffmpegVersion = if ($vcpkgStatus -match 'Package: ffmpeg\r?\nVersion: ([^\r\n]+)\r?\nPort-Version: ([^\r\n]+)') {
        "$($Matches[1])#$($Matches[2])"
    }
    else { 'unknown' }
    $cmake = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    $ninja = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
    $vcpkgRootFull = [System.IO.Path]::GetFullPath($VcpkgRoot)
    $vcpkgSafeDirectory = $vcpkgRootFull.Replace('\', '/')
    $vcpkgBaseline = (& git.exe -c "safe.directory=$vcpkgSafeDirectory" `
        -C $vcpkgRootFull rev-parse HEAD).Trim()
    $environment = [ordered]@{
        schemaVersion = 1
        task = $Task
        runId = $runId
        gitCommit = (& git.exe -C $sourceRoot rev-parse HEAD).Trim()
        trackedWorktreeDirty = [bool]((& git.exe -C $sourceRoot status --porcelain --untracked-files=no) | Select-Object -First 1)
        preset = $Preset
        architecture = $env:VSCMD_ARG_TGT_ARCH
        osCaption = if ($os) { $os.Caption } else { 'unknown' }
        osVersion = if ($os) { $os.Version } else { 'unknown' }
        osBuild = if ($os) { $os.BuildNumber } else { 'unknown' }
        cpu = $cpuNames
        logicalProcessors = if ($computer) { $computer.NumberOfLogicalProcessors } else { 'unknown' }
        memoryBytes = if ($computer) { [uint64]$computer.TotalPhysicalMemory } else { 'unknown' }
        hypervisorPresent = if ($computer) { $computer.HypervisorPresent } else { 'unknown' }
        gpu = $gpuInventory
        disk = $diskInventory
        audioDevices = $audioNames
        displayLogPixels = $displayLogPixels
        fontFileCount = $fontFileCount
        powerPlan = $powerPlan
        timezone = (Get-TimeZone).Id
        culture = [Globalization.CultureInfo]::CurrentCulture.Name
        uiCulture = [Globalization.CultureInfo]::CurrentUICulture.Name
        qpaPlatform = $env:QT_QPA_PLATFORM
        rhiBackend = $env:QSG_RHI_BACKEND
        randomSeedHex = $env:SPACE_RHYTHM_TEST_SEED
        randomSeedDecimal = '6003370060466505817'
        clockMode = 'manual for contract state machines; bounded real event loop for Qt smoke'
        tempRoot = $workRoot
        qtRoot = [System.IO.Path]::GetFullPath($QtRoot)
        qtVersion = '6.11.2'
        vcpkgRoot = [System.IO.Path]::GetFullPath($VcpkgRoot)
        vcpkgBaseline = $vcpkgBaseline
        msvc = ((& cl.exe 2>&1 | Out-String).Trim() -split "`r?`n")[0]
        windowsSdk = $env:WindowsSDKVersion
        cmake = (& $cmake --version | Select-Object -First 1)
        ninja = (& $ninja --version | Select-Object -First 1)
        ctest = (& $ctest --version | Select-Object -First 1)
        gtest = $gtestVersion
        ffmpeg = $ffmpegVersion
        crt = if ($Preset -eq 'windows-msvc-x64-debug') { 'dynamic debug /MDd' } else { 'dynamic /MD' }
        runtimeBin = $runtimeBin
        inputSha256 = @(
            'CMakePresets.json',
            'tests/CMakeLists.txt',
            'tests/contract/core_public_contract_test.cpp',
            'tests/contract/system_public_contract_test.cpp',
            'tests/contract/media_public_contract_test.cpp',
            'tests/contract/golden-media-audit-v1.json',
            'tests/contract/Test-GoldenMediaAudit.ps1',
            'tooling/windows/Invoke-HeadlessTests.ps1',
            'tests/cmake/RunQtCoreSmoke.cmake',
            'tests/cmake/RunQtTestWithReport.cmake',
            'tests/unit/media_golden_test.cpp',
            'tests/golden/media/fixtures-v1.json',
            'tests/golden/media/generated/actual-hashes-and-probe-v1.json'
            if ($Task -in @('T-022', 'T-041')) {
                '../artifacts/A-016-cpp-qt-test-strategy-and-traceability.md'
                'tests/qt/ui_integration_test.cpp'
                'tests/unit/playback_export_test.cpp'
                'tests/performance/engineering_quality_measurement.cpp'
                'tests/performance/audio_analysis_benchmark.cpp'
                'tests/performance/audio_render_benchmark.cpp'
                'tests/performance/video_analysis_benchmark.cpp'
                'tests/golden/media/Generate-GoldenMedia.ps1'
                'tests/golden/media/LICENSE.md'
                'tests/golden/video/Generate-GoldenVideo.ps1'
                'tests/golden/video/fixtures-v1.json'
                'tests/golden/video/LICENSE.md'
                'tests/golden/video/generated/actual-hashes-and-probe-v1.json'
                'tests/golden/audio/Generate-AudioDspVectors.ps1'
                'tests/golden/audio/fixtures-v1.json'
                'tests/golden/audio/LICENSE.md'
                'tests/golden/audio/algorithm-oracles-v1.json'
                'tests/golden/audio/render-oracles-v1.json'
                'tests/golden/audio/generated/actual-hashes-v1.json'
            }
        ) | ForEach-Object {
            $inputPath = Join-Path $sourceRoot $_
            [ordered]@{
                path = $_
                sha256 = (Get-FileHash -LiteralPath $inputPath -Algorithm SHA256).Hash.ToLowerInvariant()
            }
        }
    }
    $environment | ConvertTo-Json -Depth 8 |
        Set-Content -LiteralPath (Join-Path $EvidenceRoot 'environment.json') -Encoding utf8

    $discoveryPath = Join-Path $EvidenceRoot 'ctest-discovery.json'
    $discovery = & $ctest --test-dir $buildRoot --show-only=json-v1 -L $taskLabel
    if ($LASTEXITCODE -ne 0) { throw 'CTest discovery failed.' }
    $discovery | Set-Content -LiteralPath $discoveryPath -Encoding utf8

    $labelsPath = Join-Path $EvidenceRoot 'ctest-labels.log'
    & $ctest --test-dir $buildRoot --print-labels 2>&1 |
        Tee-Object -LiteralPath $labelsPath
    if ($LASTEXITCODE -ne 0) { throw 'CTest label enumeration failed.' }

    $junitPath = Join-Path $EvidenceRoot 'ctest-junit.xml'
    $ctestLog = Join-Path $EvidenceRoot 'ctest.log'
    & $ctest --test-dir $buildRoot --output-on-failure --no-tests=error `
        --parallel $Parallel -L $taskLabel --output-junit $junitPath 2>&1 |
        Tee-Object -LiteralPath $ctestLog
    $testExitCode = $LASTEXITCODE
    $result.exitCode = $testExitCode
    if ($testExitCode -ne 0) {
        $result.status = 'fail'
        throw "CTest failed with exit code $testExitCode."
    }
    if ($Task -in @('T-022', 'T-041')) {
        $measurementsRoot = Join-Path $EvidenceRoot 'measurements'
        New-Item -ItemType Directory -Force -Path $measurementsRoot | Out-Null
        $measurementCommands = @(
            [ordered]@{
                name = 'audio-analysis'
                executable = Join-Path $buildRoot 'space_rhythm_audio_analysis_benchmark.exe'
                arguments = @('--output', (Join-Path $measurementsRoot 'audio-analysis.json'))
            },
            [ordered]@{
                name = 'audio-render'
                executable = Join-Path $buildRoot 'space_rhythm_audio_render_benchmark.exe'
                arguments = @('--output', (Join-Path $measurementsRoot 'audio-render.json'))
            },
            [ordered]@{
                name = 'video-analysis'
                executable = Join-Path $buildRoot 'space_rhythm_video_analysis_benchmark.exe'
                arguments = @(
                    '--output', (Join-Path $measurementsRoot 'video-analysis.json'),
                    '--build-preset', $Preset,
                    '--execution-environment', 'TIGER-local-personal-unsigned',
                    '--warmup-runs', '3',
                    '--measured-runs', '10',
                    '--cancellation-runs', '20'
                )
            },
            [ordered]@{
                name = 'sync-recovery'
                executable = Join-Path $buildRoot 'space_rhythm_engineering_quality_measurement.exe'
                arguments = @('--output', (Join-Path $measurementsRoot 'sync-recovery.json'))
            }
        )
        foreach ($measurement in $measurementCommands) {
            if (-not (Test-Path -LiteralPath $measurement.executable -PathType Leaf)) {
                throw "Measurement executable was not found: $($measurement.executable)"
            }
            $measurementOutput = & $measurement.executable @($measurement.arguments) 2>&1
            $measurementExitCode = $LASTEXITCODE
            $measurementOutput | Set-Content -LiteralPath (
                Join-Path $measurementsRoot "$($measurement.name).log") -Encoding utf8
            $measurementOutput | ForEach-Object { Write-Host $_ }
            if ($measurementExitCode -ne 0) {
                throw "T-022 measurement '$($measurement.name)' failed with exit code $measurementExitCode."
            }
        }
    }
    if ($Task -eq 'T-041') {
        $repositoryRoot = (& git.exe -C $sourceRoot rev-parse --show-toplevel).Trim()
        if ($LASTEXITCODE -ne 0) { throw 'Unable to resolve the repository root for T-041.' }
        $expectedRelativeSourceRoot = 'projects/space-rhythm/workspace'
        $relativeSourceRoot = [System.IO.Path]::GetRelativePath($repositoryRoot, $sourceRoot).Replace('\', '/')
        $legacyEntries = @(
            'CMakeLists.txt',
            'CMakePresets.json',
            'vcpkg.json',
            'cmake',
            'src',
            'tests',
            'tooling',
            'docs',
            '.github/workflows'
        )
        $legacyEntriesPresent = @($legacyEntries | Where-Object {
            Test-Path -LiteralPath (Join-Path $repositoryRoot $_)
        })
        $cachePath = Join-Path $buildRoot 'CMakeCache.txt'
        $cacheText = Get-Content -Raw -LiteralPath $cachePath
        $cmakeHome = if ($cacheText -match '(?m)^CMAKE_HOME_DIRECTORY:INTERNAL=(.+)$') {
            $Matches[1].Trim()
        }
        else { 'missing' }
        $vcpkgInstalled = if ($cacheText -match '(?m)^VCPKG_INSTALLED_DIR:PATH=(.+)$') {
            $Matches[1].Trim()
        }
        else { 'missing' }
        $sourceOutRoot = [System.IO.Path]::GetFullPath((Join-Path $sourceRoot 'out'))
        $trackedAfter = @(& git.exe -C $sourceRoot status --porcelain --untracked-files=no)
        $pathAudit = [ordered]@{
            schemaVersion = 1
            task = 'T-041'
            repositoryRoot = $repositoryRoot
            sourceRoot = $sourceRoot
            relativeSourceRoot = $relativeSourceRoot
            expectedRelativeSourceRoot = $expectedRelativeSourceRoot
            buildRoot = $buildRoot
            evidenceRoot = $EvidenceRoot
            temporaryRoot = $workRoot
            trackedBeforeClean = ($trackedBefore.Count -eq 0)
            trackedAfterClean = ($trackedAfter.Count -eq 0)
            legacyEntriesPresent = $legacyEntriesPresent
            repositoryRootOutExists = (Test-Path -LiteralPath (Join-Path $repositoryRoot 'out'))
            cmakeHomeDirectory = $cmakeHome
            cmakeHomeMatchesSourceRoot = ([System.IO.Path]::GetFullPath($cmakeHome) -eq $sourceRoot)
            vcpkgInstalledDirectory = $vcpkgInstalled
            vcpkgInstalledUnderWorkspaceOut = [System.IO.Path]::GetFullPath($vcpkgInstalled).StartsWith(
                $sourceOutRoot,
                [StringComparison]::OrdinalIgnoreCase
            )
            buildUnderWorkspaceOut = $buildRoot.StartsWith(
                $sourceOutRoot,
                [StringComparison]::OrdinalIgnoreCase
            )
            evidenceUsesNewWorkspacePath = $EvidenceRoot.Replace('\', '/').Contains(
                '/projects/space-rhythm/workspace/out/evidence/T-041/'
            )
            temporaryUnderEvidence = $workRoot.StartsWith(
                $EvidenceRoot.TrimEnd('\') + '\',
                [StringComparison]::OrdinalIgnoreCase
            )
        }
        $pathAudit.pass = (
            $pathAudit.relativeSourceRoot -eq $expectedRelativeSourceRoot -and
            $pathAudit.trackedBeforeClean -and
            $pathAudit.trackedAfterClean -and
            $pathAudit.legacyEntriesPresent.Count -eq 0 -and
            -not $pathAudit.repositoryRootOutExists -and
            $pathAudit.cmakeHomeMatchesSourceRoot -and
            $pathAudit.vcpkgInstalledUnderWorkspaceOut -and
            $pathAudit.buildUnderWorkspaceOut -and
            $pathAudit.evidenceUsesNewWorkspacePath -and
            $pathAudit.temporaryUnderEvidence
        )
        $pathAudit | ConvertTo-Json -Depth 6 |
            Set-Content -LiteralPath (Join-Path $EvidenceRoot 'path-isolation.json') -Encoding utf8
        if (-not $pathAudit.pass) { throw 'T-041 path isolation audit failed.' }
    }
    $result.status = 'pass'
}
catch {
    if ($result.status -eq 'not-run') { $result.status = 'blocked' }
    $result.error = $_.Exception.Message
    throw
}
finally {
    $stopwatch.Stop()
    $result.finishedUtc = [DateTime]::UtcNow.ToString('o')
    $result.durationMs = $stopwatch.ElapsedMilliseconds
    $temporary = Join-Path $buildRoot 'Testing\Temporary'
    foreach ($name in @('LastTest.log', 'LastTestsFailed.log')) {
        $source = Join-Path $temporary $name
        if (Test-Path -LiteralPath $source -PathType Leaf) {
            Copy-Item -LiteralPath $source -Destination (Join-Path $EvidenceRoot $name) -Force
        }
    }
    $result | ConvertTo-Json -Depth 6 |
        Set-Content -LiteralPath (Join-Path $EvidenceRoot 'result.json') -Encoding utf8
    $hashRecords = @(Get-ChildItem -LiteralPath $EvidenceRoot -Recurse -File |
        Where-Object { $_.Name -ne 'sha256.json' } |
        Sort-Object FullName |
        ForEach-Object {
            [ordered]@{
                path = [System.IO.Path]::GetRelativePath($EvidenceRoot, $_.FullName).Replace('\', '/')
                sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
                sizeBytes = $_.Length
            }
        })
    [ordered]@{ schemaVersion = 1; task = $Task; files = $hashRecords } |
        ConvertTo-Json -Depth 6 |
        Set-Content -LiteralPath (Join-Path $EvidenceRoot 'sha256.json') -Encoding utf8
    Write-Host "$Task evidence: $EvidenceRoot"
}
