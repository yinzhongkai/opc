[CmdletBinding()]
param(
    [string]$BenchmarkPath = '',
    [string]$RawMeasurementPath = '',
    [string]$EvidencePath = '',
    [ValidateRange(1, 100)]
    [int]$WarmupRuns = 1,
    [ValidateRange(1, 100)]
    [int]$MeasuredRuns = 5,
    [ValidateRange(1, 100)]
    [int]$CancellationRuns = 5,
    [ValidateRange(0, 100)]
    [int]$PriorInvalidAttemptCount = 0,
    [string]$PriorInvalidAttemptReason = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$sourceRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $sourceRoot '..'))
$repositoryRoot = (& git.exe -C $sourceRoot rev-parse --show-toplevel).Trim()
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($repositoryRoot)) {
    throw 'Unable to resolve the Git repository root from the product workspace'
}
$repositoryRoot = [System.IO.Path]::GetFullPath($repositoryRoot)
if ([string]::IsNullOrWhiteSpace($BenchmarkPath)) {
    $BenchmarkPath = Join-Path $sourceRoot 'out\build\windows-msvc-x64-release\space_rhythm_video_analysis_benchmark.exe'
}
if ([string]::IsNullOrWhiteSpace($RawMeasurementPath)) {
    $RawMeasurementPath = Join-Path $projectRoot 'evidence\T-029\t028-release-windows-measurement-v1.json'
}
if ([string]::IsNullOrWhiteSpace($EvidencePath)) {
    $EvidencePath = Join-Path $projectRoot 'evidence\T-029\windows-release-baseline-v1.json'
}

$BenchmarkPath = [System.IO.Path]::GetFullPath($BenchmarkPath)
$RawMeasurementPath = [System.IO.Path]::GetFullPath($RawMeasurementPath)
$EvidencePath = [System.IO.Path]::GetFullPath($EvidencePath)
$allowedEvidenceRoot = [System.IO.Path]::GetFullPath(
    (Join-Path $projectRoot 'evidence\T-029')).TrimEnd('\') + '\'
foreach ($path in @($RawMeasurementPath, $EvidencePath)) {
    if (-not $path.StartsWith($allowedEvidenceRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Evidence output must stay under $allowedEvidenceRoot"
    }
}
if (-not (Test-Path -LiteralPath $BenchmarkPath -PathType Leaf)) {
    throw "Release benchmark PE not found: $BenchmarkPath"
}

function Get-Sha256([string]$LiteralPath) {
    return (Get-FileHash -LiteralPath $LiteralPath -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Read-OptionalText([string]$LiteralPath) {
    if (-not (Test-Path -LiteralPath $LiteralPath -PathType Leaf)) {
        return ''
    }
    $value = Get-Content -Raw -LiteralPath $LiteralPath
    if ($null -eq $value) {
        return ''
    }
    return $value.Trim()
}

function Get-OptionalCim([string]$ClassName, [string]$Namespace = 'root/cimv2') {
    try {
        return @(Get-CimInstance -Namespace $Namespace -ClassName $ClassName -ErrorAction Stop)
    }
    catch {
        return @()
    }
}

function Convert-CimRows($Rows, [string[]]$Properties) {
    return @($Rows | ForEach-Object {
        $row = [ordered]@{}
        foreach ($property in $Properties) {
            $row[$property] = $_.$property
        }
        [pscustomobject]$row
    })
}

$expectedHost = 'TIGER'
$expectedBestPerformanceOverlay = 'ded574b5-45a0-4f42-8737-46345c09c238'
$actualHost = [System.Environment]::MachineName
$powerSchemeText = (& powercfg.exe /getactivescheme | Out-String).Trim()
$powerRegistry = Get-ItemProperty -LiteralPath `
    'HKLM:\SYSTEM\CurrentControlSet\Control\Power\User\PowerSchemes'
$actualOverlay = [string]$powerRegistry.ActiveOverlayAcPowerScheme
$ciPolicy = Get-ItemProperty -LiteralPath `
    'HKLM:\SYSTEM\CurrentControlSet\Control\CI\Policy'
$sacState = [int]$ciPolicy.VerifiedAndReputablePolicyState
if ($actualHost -ine $expectedHost) {
    throw "Expected benchmark host $expectedHost, found $actualHost"
}
if ($actualOverlay -ine $expectedBestPerformanceOverlay) {
    throw "Best Performance AC overlay is not active: $actualOverlay"
}
if ($sacState -ne 0) {
    throw "Unsigned Release PE prerequisite is not met; SAC state is $sacState"
}

$computer = @(Get-OptionalCim 'Win32_ComputerSystem')
$operatingSystem = @(Get-OptionalCim 'Win32_OperatingSystem')
$processorBefore = @(Get-OptionalCim 'Win32_Processor')
$videoControllers = @(Get-OptionalCim 'Win32_VideoController')
$diskDrives = @(Get-OptionalCim 'Win32_DiskDrive')
$batteries = @(Get-OptionalCim 'Win32_Battery')
$thermalBefore = @(Get-OptionalCim 'MSAcpi_ThermalZoneTemperature' 'root/wmi')
$drive = Get-PSDrive -Name C
$recordedAtUtc = [DateTimeOffset]::UtcNow.ToString('o')

$vcpkgBin = Join-Path $sourceRoot 'out\vcpkg_installed\x64-windows-space-rhythm\bin'
$env:Path = "$vcpkgBin;$env:Path"
$tempRoot = Join-Path $sourceRoot 'out\evaluation\T-029\windows-baseline'
New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null
$stdoutPath = Join-Path $tempRoot 'benchmark.stdout.txt'
$stderrPath = Join-Path $tempRoot 'benchmark.stderr.txt'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $RawMeasurementPath) | Out-Null

$arguments = @(
    '--output', $RawMeasurementPath,
    '--build-preset', 'windows-msvc-x64-release',
    '--execution-environment', 'native-windows-tiger-best-performance',
    '--warmup-runs', $WarmupRuns.ToString(),
    '--measured-runs', $MeasuredRuns.ToString(),
    '--cancellation-runs', $CancellationRuns.ToString()
)
$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
$process = Start-Process -FilePath $BenchmarkPath -ArgumentList $arguments -PassThru `
    -WindowStyle Hidden -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath
$peakWorkingSetBytes = 0L
$peakPrivateBytes = 0L
$peakThreadCount = 0
while (-not $process.HasExited) {
    try {
        $process.Refresh()
        $peakWorkingSetBytes = [Math]::Max($peakWorkingSetBytes, [int64]$process.WorkingSet64)
        $peakPrivateBytes = [Math]::Max($peakPrivateBytes, [int64]$process.PrivateMemorySize64)
        $peakThreadCount = [Math]::Max($peakThreadCount, [int]$process.Threads.Count)
    }
    catch {
        # The process may exit between HasExited and Refresh; final status is checked below.
    }
    Start-Sleep -Milliseconds 10
}
$process.WaitForExit()
$stopwatch.Stop()
$stdout = Read-OptionalText $stdoutPath
$stderr = Read-OptionalText $stderrPath
if ($process.ExitCode -ne 0) {
    throw "Release benchmark failed with exit $($process.ExitCode): $stderr"
}
if (-not (Test-Path -LiteralPath $RawMeasurementPath -PathType Leaf)) {
    throw 'Release benchmark did not create its raw measurement JSON'
}
$rawMeasurement = Get-Content -Raw -LiteralPath $RawMeasurementPath | ConvertFrom-Json
if ($rawMeasurement.warmupRuns -ne $WarmupRuns -or
    $rawMeasurement.measuredRuns -ne $MeasuredRuns -or
    $rawMeasurement.cancellationRuns -ne $CancellationRuns) {
    throw 'Release benchmark output does not match the requested run counts'
}
if ($rawMeasurement.opencvReportedThreads -gt 8 -or $rawMeasurement.ffmpegDecoderThreads -ne 2) {
    throw 'Release benchmark did not preserve the fixed OpenCV/FFmpeg thread limits'
}

$processorAfter = @(Get-OptionalCim 'Win32_Processor')
$thermalAfter = @(Get-OptionalCim 'MSAcpi_ThermalZoneTemperature' 'root/wmi')
$sourcePath = Join-Path $sourceRoot 'tests\performance\video_analysis_benchmark.cpp'
$cmakePath = Join-Path $sourceRoot 'tests\CMakeLists.txt'
$gitHead = (& git.exe -c "safe.directory=$($repositoryRoot.Replace('\', '/'))" -C $repositoryRoot rev-parse HEAD).Trim()

$thermalEvidence = if ($thermalBefore.Count -eq 0 -and $thermalAfter.Count -eq 0) {
    [ordered]@{
        status = 'unavailable'
        reason = 'MSAcpi_ThermalZoneTemperature returned no readable sensor rows'
    }
}
else {
    [ordered]@{
        status = 'measured-raw-acpi'
        unit = 'tenths-kelvin'
        before = @(Convert-CimRows $thermalBefore @('InstanceName', 'CurrentTemperature'))
        after = @(Convert-CimRows $thermalAfter @('InstanceName', 'CurrentTemperature'))
    }
}

$evidence = [ordered]@{
    schemaVersion = 1
    taskId = 'T-029'
    status = 'measured'
    measurementScope = 'T-028-release-pe-native-host-readiness-only'
    recordedAtUtc = $recordedAtUtc
    completedAtUtc = [DateTimeOffset]::UtcNow.ToString('o')
    source = [ordered]@{
        productEvaluationInput = 'A-031@0.5'
        productEvaluationInputSha256 = Get-Sha256 (
            Join-Path $projectRoot 'artifacts\A-031-t029-video-product-evaluation-input.md')
        decisions = @('D-011', 'D-014')
        gitHeadAtBuild = $gitHead
        benchmarkSourceSha256 = Get-Sha256 $sourcePath
        benchmarkCmakeSha256 = Get-Sha256 $cmakePath
        benchmarkPeSha256 = Get-Sha256 $BenchmarkPath
        benchmarkPeBytes = (Get-Item -LiteralPath $BenchmarkPath).Length
        rawMeasurementSha256 = Get-Sha256 $RawMeasurementPath
    }
    sequence = [ordered]@{
        warmupRuns = $WarmupRuns
        measuredRuns = $MeasuredRuns
        cancellationRuns = $CancellationRuns
        coldStartExcludedFromSamples = $true
        priorInvalidAttemptCount = $PriorInvalidAttemptCount
        priorInvalidAttemptReason = if ($PriorInvalidAttemptCount -eq 0) {
            $null
        } else {
            $PriorInvalidAttemptReason
        }
    }
    environment = [ordered]@{
        benchmarkId = 'TIGER'
        machineName = $actualHost
        computerSystem = @(Convert-CimRows $computer @('Manufacturer', 'Model', 'TotalPhysicalMemory'))
        operatingSystem = @(Convert-CimRows $operatingSystem @('Caption', 'Version', 'BuildNumber', 'OSArchitecture'))
        processorBefore = @(Convert-CimRows $processorBefore @('Name', 'NumberOfCores', 'NumberOfLogicalProcessors', 'CurrentClockSpeed', 'MaxClockSpeed'))
        processorAfter = @(Convert-CimRows $processorAfter @('Name', 'NumberOfCores', 'NumberOfLogicalProcessors', 'CurrentClockSpeed', 'MaxClockSpeed'))
        videoControllers = @(Convert-CimRows $videoControllers @('Name', 'DriverVersion'))
        diskDrives = @(Convert-CimRows $diskDrives @('Model', 'MediaType', 'Size'))
        systemDriveFreeBytesBefore = [int64]$drive.Free
        batteries = @(Convert-CimRows $batteries @('Name', 'Status', 'BatteryStatus', 'EstimatedChargeRemaining'))
        temperatureDiagnostics = $thermalEvidence
        activePowerSchemeRaw = $powerSchemeText
        activeOverlayAcPowerScheme = $actualOverlay.ToLowerInvariant()
        bestPerformanceOverlayExpected = $expectedBestPerformanceOverlay
        bestPerformanceOverlayStatus = 'pass'
        verifiedAndReputablePolicyState = $sacState
        unsignedPeExecutionPrerequisite = 'pass'
    }
    processObservation = [ordered]@{
        exitCode = $process.ExitCode
        wallDurationMsIncludingDecodeAndProcessOverhead = $stopwatch.ElapsedMilliseconds
        peakWorkingSetBytesPolled = $peakWorkingSetBytes
        peakPrivateBytesPolled = $peakPrivateBytes
        peakThreadCountPolled = $peakThreadCount
        stdout = $stdout
        stderr = $stderr
        pollingIntervalMs = 10
    }
    rawMeasurement = $rawMeasurement
    gates = [ordered]@{
        nativeReleasePeStart = 'pass'
        tigerBestPerformanceOverlay = 'pass'
        t028ReleaseBaseline = 'measured'
        t029FormalPerformance = 'not-evaluated(reason=T-028 synthetic 160x90 fixture is not the confirmed product/VFR scenario matrix)'
        productRepresentativeGate = 'not-evaluated(reason=USER-01 reference, blind-rating and correction records are missing)'
        vfrRobustnessGate = 'not-evaluated(reason=missing_actual_original_VFR_media)'
        overall = 'not-evaluated'
    }
    model = [ordered]@{
        modelEvaluationStarted = $false
        onnxIntroduced = $false
    }
}

$json = $evidence | ConvertTo-Json -Depth 12
[System.IO.File]::WriteAllText($EvidencePath, $json + "`n", [System.Text.UTF8Encoding]::new($false))
Write-Output "T029_WINDOWS_RELEASE_BASELINE=PASS evidence=$EvidencePath"
