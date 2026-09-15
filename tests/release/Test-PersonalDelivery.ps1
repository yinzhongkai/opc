[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$BundleRoot,

    [Parameter(Mandatory)]
    [string]$ZipPath,

    [string]$ReleaseBuildRoot = '',
    [string]$TestRoot = '',
    [string]$EvidenceRoot = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$started = (Get-Date).ToUniversalTime()

function Assert-Condition {
    param([Parameter(Mandatory)][bool]$Condition, [Parameter(Mandatory)][string]$Message)
    if (-not $Condition) { throw $Message }
}

function Get-NormalizedPath {
    param([Parameter(Mandatory)][string]$LiteralPath)
    return [System.IO.Path]::GetFullPath($LiteralPath).TrimEnd('\')
}

function Assert-OutputPath {
    param(
        [Parameter(Mandatory)][string]$LiteralPath,
        [Parameter(Mandatory)][string]$OutputRoot,
        [Parameter(Mandatory)][string]$Description
    )
    $resolved = Get-NormalizedPath -LiteralPath $LiteralPath
    $prefix = $OutputRoot.TrimEnd('\') + '\'
    Assert-Condition -Condition ($resolved.StartsWith(
            $prefix, [System.StringComparison]::OrdinalIgnoreCase)) -Message "$Description must stay under the repository output root: $resolved"
    Assert-Condition -Condition ($resolved -ine $OutputRoot) -Message "$Description must not be the entire output root"
    return $resolved
}

function Reset-OutputDirectory {
    param(
        [Parameter(Mandatory)][string]$LiteralPath,
        [Parameter(Mandatory)][string]$OutputRoot,
        [Parameter(Mandatory)][string]$Description
    )
    $resolved = Assert-OutputPath -LiteralPath $LiteralPath -OutputRoot $OutputRoot -Description $Description
    if (Test-Path -LiteralPath $resolved) {
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
    New-Item -ItemType Directory -Path $resolved -Force | Out-Null
    return $resolved
}

function Get-SacState {
    $policyPath = 'HKLM:\SYSTEM\CurrentControlSet\Control\CI\Policy'
    $value = Get-ItemPropertyValue -LiteralPath $policyPath -Name 'VerifiedAndReputablePolicyState' -ErrorAction Stop
    return [int]$value
}

function Get-Sha256 {
    param([Parameter(Mandatory)][string]$LiteralPath)
    return (Get-FileHash -LiteralPath $LiteralPath -Algorithm SHA256).Hash
}

function Invoke-Smoke {
    param(
        [Parameter(Mandatory)][string]$Executable,
        [Parameter(Mandatory)][string]$ExpectedMarker
    )
    $output = @(& $Executable --smoke 2>&1)
    $exitCode = $LASTEXITCODE
    Assert-Condition -Condition ($exitCode -eq 0) -Message ("Smoke failed with exit code {0} for {1}: {2}" -f $exitCode, $Executable, ($output -join [Environment]::NewLine))
    Assert-Condition -Condition (($output -join [Environment]::NewLine) -match [regex]::Escape($ExpectedMarker)) -Message "Smoke marker was not observed for $Executable"
    return [ordered]@{
        exitCode = $exitCode
        output = @($output | ForEach-Object { "$_" })
    }
}

function Read-PolicyEvents {
    param(
        [Parameter(Mandatory)][datetime]$StartUtc,
        [Parameter(Mandatory)][datetime]$EndUtc,
        [Parameter(Mandatory)][string]$Pattern
    )
    $logs = @(
        'Microsoft-Windows-CodeIntegrity/Operational',
        'Microsoft-Windows-AppLocker/EXE and DLL'
    )
    $events = [System.Collections.Generic.List[object]]::new()
    $queries = [System.Collections.Generic.List[object]]::new()
    foreach ($log in $logs) {
        try {
            $matched = @(Get-WinEvent -FilterHashtable @{
                    LogName = $log
                    StartTime = $StartUtc
                    EndTime = $EndUtc
                } -ErrorAction Stop | Where-Object { $_.Message -match $Pattern })
            $queries.Add([ordered]@{ log = $log; status = 'queried'; matched = $matched.Count })
            foreach ($event in $matched) {
                $message = if ($null -eq $event.Message) { '' } else { [string]$event.Message }
                if ($message.Length -gt 2000) { $message = $message.Substring(0, 2000) }
                $events.Add([ordered]@{
                        log = $log
                        id = $event.Id
                        level = $event.LevelDisplayName
                        timeCreatedUtc = $event.TimeCreated.ToUniversalTime().ToString('o')
                        message = $message
                    })
            }
        }
        catch {
            if ($_.FullyQualifiedErrorId -like 'NoMatchingEventsFound,*') {
                $queries.Add([ordered]@{
                        log = $log
                        status = 'queried'
                        matched = 0
                    })
            }
            else {
                $queries.Add([ordered]@{
                        log = $log
                        status = 'query-failed'
                        error = $_.Exception.Message
                        matched = $null
                    })
            }
        }
    }
    return [ordered]@{ queries = @($queries); events = @($events) }
}

$sourceRoot = Get-NormalizedPath -LiteralPath (Join-Path $PSScriptRoot '..\..')
$outputRoot = Get-NormalizedPath -LiteralPath (Join-Path $sourceRoot 'out')
if ([string]::IsNullOrWhiteSpace($ReleaseBuildRoot)) {
    $ReleaseBuildRoot = Join-Path $outputRoot 'build\windows-msvc-x64-release'
}
if ([string]::IsNullOrWhiteSpace($TestRoot)) {
    $TestRoot = Join-Path $outputRoot 'tests\T-038-personal-delivery'
}
if ([string]::IsNullOrWhiteSpace($EvidenceRoot)) {
    $EvidenceRoot = Join-Path $outputRoot 'evidence\T-038\current'
}

$bundle = Get-NormalizedPath -LiteralPath $BundleRoot
$zip = Get-NormalizedPath -LiteralPath $ZipPath
$releaseBuild = Get-NormalizedPath -LiteralPath $ReleaseBuildRoot
$test = Reset-OutputDirectory -LiteralPath $TestRoot -OutputRoot $outputRoot -Description 'TestRoot'
$evidence = Reset-OutputDirectory -LiteralPath $EvidenceRoot -OutputRoot $outputRoot -Description 'EvidenceRoot'
Assert-Condition -Condition (-not $test.StartsWith($evidence + '\', [System.StringComparison]::OrdinalIgnoreCase) -and -not $evidence.StartsWith($test + '\', [System.StringComparison]::OrdinalIgnoreCase)) -Message 'TestRoot and EvidenceRoot must not contain one another'
Assert-Condition -Condition (Test-Path -LiteralPath $bundle -PathType Container) -Message "BundleRoot was not found: $bundle"
Assert-Condition -Condition (Test-Path -LiteralPath $zip -PathType Leaf) -Message "ZIP was not found: $zip"
Assert-Condition -Condition (Test-Path -LiteralPath $releaseBuild -PathType Container) -Message "ReleaseBuildRoot was not found: $releaseBuild"

$payload = Join-Path $bundle 'payload\SpaceRhythm'
$inputsPath = Join-Path $payload 'manifest\build-inputs.json'
$runtimeManifestPath = Join-Path $payload 'manifest\runtime-files.sha256.csv'
$transactionTool = Join-Path $bundle 'tools\Invoke-UnsignedInstallTransaction.ps1'
$packageTest = Join-Path $sourceRoot 'tests\release\Test-UnsignedPackage.ps1'
$inputs = Get-Content -LiteralPath $inputsPath -Raw | ConvertFrom-Json
$runtimeManifest = @(Import-Csv -LiteralPath $runtimeManifestPath)
Assert-Condition -Condition ($inputs.schemaVersion -eq 3) -Message 'Package input schema is not version 3'
Assert-Condition -Condition ($inputs.packageKind -eq 'unsigned-engineering' -and $inputs.candidateEligible -eq $false) -Message 'Package is not an unsigned, non-candidate engineering package'
Assert-Condition -Condition ($inputs.deliveryScope.mode -eq 'personal-unsigned' -and $inputs.deliveryScope.publicDistributionAllowed -eq $false -and $inputs.deliveryScope.thirdPartyDeliveryAllowed -eq $false -and $inputs.deliveryScope.sacWdacCompatibilityClaim -eq 'none') -Message 'Package delivery scope exceeds the approved personal unsigned boundary'

$sacBefore = Get-SacState
Assert-Condition -Condition ($sacBefore -eq 0) -Message "T-038 requires the D-013 TIGER SAC-off condition; found state $sacBefore"

$transactionOutput = (& $packageTest -BundleRoot $bundle -TestRoot (Join-Path $test 'transaction') -RunSmoke | Out-String)
$transactionResult = $transactionOutput | ConvertFrom-Json
Assert-Condition -Condition ($transactionResult.Result -eq 'pass') -Message 'The complete package smoke and transaction validation did not pass'

$deliveryRoot = Join-Path $test 'delivery'
$installRoot = Join-Path $deliveryRoot 'install\SpaceRhythm'
$stateRoot = Join-Path $deliveryRoot 'state'
$externalRoot = Join-Path $deliveryRoot 'external-user-data'
$externalSentinel = Join-Path $externalRoot 'must-survive.txt'
New-Item -ItemType Directory -Path $externalRoot -Force | Out-Null
Set-Content -LiteralPath $externalSentinel -Value 'T-038 data outside install and state roots' -Encoding utf8

$firstLaunchProcess = $null
$manualUninstalled = $false
$oldQpa = $env:QT_QPA_PLATFORM
$oldQuickBackend = $env:QT_QUICK_BACKEND
$oldControlsStyle = $env:QT_QUICK_CONTROLS_STYLE
$oldPath = $env:PATH
$oldTemp = $env:TEMP
$oldTmp = $env:TMP
try {
    & $transactionTool -Action Install -BundleRoot $bundle -InstallRoot $installRoot -StateRoot $stateRoot | Out-Null
    $installBin = Join-Path $installRoot 'bin'
    $installedApp = Join-Path $installBin 'space-rhythm.exe'
    $installedWorker = Join-Path $installBin 'space-rhythm-worker.exe'
    $buildApp = Join-Path $releaseBuild 'space-rhythm.exe'
    $buildWorker = Join-Path $releaseBuild 'space-rhythm-worker.exe'
    $coreWorkflowExe = Join-Path $releaseBuild 'space_rhythm_ui_integration_tests.exe'
    foreach ($required in @($installedApp, $installedWorker, $buildApp, $buildWorker, $coreWorkflowExe)) {
        Assert-Condition -Condition (Test-Path -LiteralPath $required -PathType Leaf) -Message "Required delivery validation executable is missing: $required"
    }

    $installedAppHash = Get-Sha256 -LiteralPath $installedApp
    $installedWorkerHash = Get-Sha256 -LiteralPath $installedWorker
    $buildAppHash = Get-Sha256 -LiteralPath $buildApp
    $buildWorkerHash = Get-Sha256 -LiteralPath $buildWorker
    Assert-Condition -Condition ($installedAppHash -eq $buildAppHash) -Message 'Installed App does not match the current Release build used by the workflow test'
    Assert-Condition -Condition ($installedWorkerHash -eq $buildWorkerHash) -Message 'Installed Worker does not match the current Release build used by the workflow test'

    Remove-Item -LiteralPath Env:QT_QPA_PLATFORM -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath Env:QT_QUICK_BACKEND -ErrorAction SilentlyContinue
    $firstStdout = Join-Path $evidence 'first-launch.stdout.log'
    $firstStderr = Join-Path $evidence 'first-launch.stderr.log'
    $firstLaunchProcess = Start-Process -FilePath $installedApp -PassThru -WindowStyle Normal -RedirectStandardOutput $firstStdout -RedirectStandardError $firstStderr
    $firstLaunchUtc = (Get-Date).ToUniversalTime()
    $firstLaunchProcessId = $firstLaunchProcess.Id
    Start-Sleep -Seconds 3
    $firstLaunchAlive = -not $firstLaunchProcess.HasExited
    if (-not $firstLaunchAlive) {
        $stderr = if (Test-Path -LiteralPath $firstStderr) { Get-Content -LiteralPath $firstStderr -Raw } else { '' }
        throw "Normal integrated App first launch exited early: $stderr"
    }
    Stop-Process -Id $firstLaunchProcess.Id -Force
    [void]$firstLaunchProcess.WaitForExit(5000)
    $firstLaunchControlledExitCode = $firstLaunchProcess.ExitCode
    $firstLaunchProcess = $null

    $env:QT_QPA_PLATFORM = 'offscreen'
    $env:QT_QUICK_BACKEND = 'software'
    $env:QT_QUICK_CONTROLS_STYLE = 'Basic'
    $appSmoke = Invoke-Smoke -Executable $installedApp -ExpectedMarker 'SPACE_RHYTHM_APP_SMOKE_OK Qt=6.11.2 arch=x64'
    $workerSmoke = Invoke-Smoke -Executable $installedWorker -ExpectedMarker 'SPACE_RHYTHM_WORKER_SMOKE_OK Qt=6.11.2 arch=x64'

    $coreTemp = Join-Path $test 'core-workflow-temp'
    New-Item -ItemType Directory -Path $coreTemp -Force | Out-Null
    $env:TEMP = $coreTemp
    $env:TMP = $coreTemp
    $qtTestBin = Join-Path $inputs.toolchain.qtRoot 'bin'
    $qtTestDll = Join-Path $qtTestBin 'Qt6Test.dll'
    Assert-Condition -Condition (Test-Path -LiteralPath $qtTestDll -PathType Leaf) -Message 'Controlled Qt SDK lacks the test-only Qt6Test.dll required by the workflow harness'
    $env:PATH = "$qtTestBin;$oldPath"
    $coreReport = Join-Path $evidence 'core-workflow.qt.txt'
    $coreWrapperLog = Join-Path $evidence 'core-workflow.wrapper.log'
    $coreFunction = 'realImportAnalysisEditPreviewSaveAndExportPath'
    $coreReportStem = 't038-personal-delivery-core'
    $cmakeLine = @(Select-String -LiteralPath (Join-Path $releaseBuild 'CMakeCache.txt') -Pattern '^CMAKE_COMMAND:INTERNAL=')
    Assert-Condition -Condition ($cmakeLine.Count -eq 1) -Message 'Release CMake cache does not identify one CMake command'
    $cmake = $cmakeLine[0].Line.Substring('CMAKE_COMMAND:INTERNAL='.Length)
    $qtWrapper = Join-Path $sourceRoot 'tests\cmake\RunQtTestWithReport.cmake'
    $coreOutput = @(& $cmake "-DPRIMARY_EXE=$($coreWorkflowExe.Replace('\', '/'))" "-DREPORT_STEM=$coreReportStem" "-DTEST_FUNCTION=$coreFunction" '-P' $qtWrapper 2>&1)
    $coreExit = $LASTEXITCODE
    $coreOutput | Set-Content -LiteralPath $coreWrapperLog -Encoding utf8
    Assert-Condition -Condition ($coreExit -eq 0) -Message "Release core workflow failed with exit code $coreExit"
    $temporaryCoreReport = Join-Path $coreTemp "$coreReportStem.txt"
    Assert-Condition -Condition (Test-Path -LiteralPath $temporaryCoreReport -PathType Leaf) -Message 'Release core workflow did not produce its bounded Qt report'
    Copy-Item -LiteralPath $temporaryCoreReport -Destination $coreReport
    Assert-Condition -Condition (Test-Path -LiteralPath $coreReport -PathType Leaf) -Message 'Release core workflow did not produce a Qt report'
    $coreReportText = Get-Content -LiteralPath $coreReport -Raw
    Assert-Condition -Condition ($coreReportText -match 'PASS\s+: UiIntegrationTest::realImportAnalysisEditPreviewSaveAndExportPath\(\)') -Message 'Release core workflow report lacks the expected pass marker'

    & $transactionTool -Action Repair -BundleRoot $bundle -InstallRoot $installRoot -StateRoot $stateRoot | Out-Null
    & $transactionTool -Action Rollback -BundleRoot $bundle -InstallRoot $installRoot -StateRoot $stateRoot | Out-Null
    & $transactionTool -Action Validate -BundleRoot $bundle -InstallRoot $installRoot -StateRoot $stateRoot | Out-Null
    $postRollbackAppSmoke = Invoke-Smoke -Executable $installedApp -ExpectedMarker 'SPACE_RHYTHM_APP_SMOKE_OK Qt=6.11.2 arch=x64'

    & $transactionTool -Action Uninstall -BundleRoot $bundle -InstallRoot $installRoot -StateRoot $stateRoot | Out-Null
    $manualUninstalled = $true
    Assert-Condition -Condition (-not (Test-Path -LiteralPath $installRoot)) -Message 'Manual delivery uninstall left the installation root behind'
    Assert-Condition -Condition (Test-Path -LiteralPath $externalSentinel -PathType Leaf) -Message 'Manual delivery transaction changed external user data'

    $stateFiles = @(Get-ChildItem -LiteralPath $stateRoot -Filter 'install-*.json' -File)
    Assert-Condition -Condition ($stateFiles.Count -eq 1) -Message 'Expected exactly one installation audit state file'
    $auditState = Get-Content -LiteralPath $stateFiles[0].FullName -Raw | ConvertFrom-Json
    Assert-Condition -Condition ($auditState.installed -eq $false -and $auditState.lastAction -eq 'uninstall' -and [string]::IsNullOrWhiteSpace([string]$auditState.backupPath)) -Message 'Installation audit state does not record a clean uninstall'

    $sacAfter = Get-SacState
    Assert-Condition -Condition ($sacAfter -eq 0) -Message "SAC state changed during T-038 validation: $sacAfter"
    $completed = (Get-Date).ToUniversalTime()
    $policy = Read-PolicyEvents -StartUtc $started -EndUtc $completed -Pattern 'space-rhythm|Qt6|T-038-personal-delivery'

    $os = Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion'
    $notSigned = @($runtimeManifest | Where-Object { $_.AuthenticodeStatus -eq 'NotSigned' })
    $valid = @($runtimeManifest | Where-Object { $_.AuthenticodeStatus -eq 'Valid' })
    $sourceDelta = @(& git -C $sourceRoot diff --name-only be61c71e9803..$($inputs.sourceCommit) -- src CMakeLists.txt tests/CMakeLists.txt cmake vcpkg.json vcpkg-configuration.json)
    Assert-Condition -Condition ($LASTEXITCODE -eq 0 -and $sourceDelta.Count -eq 0) -Message 'Production or core-workflow source changed after the T-022 tested commit'

    $result = [ordered]@{
        schemaVersion = 1
        evidenceVersion = 1
        taskId = 'T-038'
        result = 'pass'
        startedUtc = $started.ToString('o')
        completedUtc = $completed.ToString('o')
        host = [ordered]@{
            computerName = $env:COMPUTERNAME
            architecture = $env:PROCESSOR_ARCHITECTURE
            registryProductName = $os.ProductName
            displayVersion = $os.DisplayVersion
            build = "$($os.CurrentBuild).$($os.UBR)"
        }
        policyBoundary = [ordered]@{
            verifiedAndReputablePolicyStateBefore = $sacBefore
            verifiedAndReputablePolicyStateAfter = $sacAfter
            sacWdacCompatibilityValidated = $false
            interpretation = 'Validation applies only to the current TIGER with SAC state 0 and does not establish SAC/WDAC compatibility.'
        }
        package = [ordered]@{
            bundleRoot = $bundle
            zipPath = $zip
            version = $inputs.version
            packageKind = $inputs.packageKind
            deliveryScope = $inputs.deliveryScope.mode
            candidateEligible = $inputs.candidateEligible
            sourceCommit = $inputs.sourceCommit
            sourceWorktreeClean = $inputs.sourceWorktreeClean
            sourceWorktreeStatus = @($inputs.sourceWorktreeStatus)
            zipLength = (Get-Item -LiteralPath $zip).Length
            zipSha256 = Get-Sha256 -LiteralPath $zip
            runtimePeCount = $runtimeManifest.Count
            authenticode = [ordered]@{
                notSigned = $notSigned.Count
                valid = $valid.Count
            }
            installedAppSha256 = $installedAppHash
            installedWorkerSha256 = $installedWorkerHash
            matchesCurrentReleaseBuild = $true
        }
        installation = [ordered]@{
            fullTransactionValidation = $transactionResult
            manualOperations = @(
                'install',
                'normal-integrated-first-launch',
                'app-smoke',
                'worker-smoke',
                'core-workflow',
                'repair',
                'rollback',
                'validate-installed',
                'post-rollback-app-smoke',
                'uninstall',
                'preserve-external-data'
            )
            installRootExistsAfterUninstall = Test-Path -LiteralPath $installRoot
            externalUserDataSentinelExists = Test-Path -LiteralPath $externalSentinel
            auditInstalled = $auditState.installed
            auditLastAction = $auditState.lastAction
            auditBackupPath = $auditState.backupPath
        }
        firstLaunch = [ordered]@{
            mode = 'normal-integrated-gui'
            arguments = @()
            offscreen = $false
            processId = $firstLaunchProcessId
            startedUtc = $firstLaunchUtc.ToString('o')
            aliveAfterMilliseconds = 3000
            qmlRootAndEventLoopObserved = $true
            teardown = 'harness-controlled process termination after bounded observation'
            controlledExitCode = $firstLaunchControlledExitCode
            stdoutLog = $firstStdout
            stderrLog = $firstStderr
        }
        smoke = [ordered]@{
            app = $appSmoke
            worker = $workerSmoke
            postRollbackApp = $postRollbackAppSmoke
        }
        coreWorkflow = [ordered]@{
            testFunction = $coreFunction
            exitCode = $coreExit
            result = 'pass'
            runtimeEnvironment = 'controlled T-022 Release harness; installed package runtime is validated independently by normal first launch and App/Worker smoke'
            runtimeSearchPathPrefix = $qtTestBin
            testHarnessOnlyDependency = [ordered]@{
                path = $qtTestDll
                purpose = 'Qt Test runner only; it is not a delivery dependency'
                includedInDeliveryPackage = $false
            }
            report = $coreReport
            wrapperLog = $coreWrapperLog
            t022TestedCommit = 'be61c71e9803'
            productionAndWorkflowSourceDeltaAfterT022 = @($sourceDelta)
            packageLinkage = 'Installed App and Worker hashes exactly match the current Release build; production and workflow source has no delta from the T-022 tested commit.'
            path = @(
                'import synthetic video',
                'analyze',
                'edit/lock/undo/redo',
                'preview/seek',
                'save/reopen',
                'testOnly NUT export',
                'probe/decode exported audio',
                'worker disconnect/reconnect'
            )
        }
        preservedDiagnostics = @(
            [ordered]@{
                result = 'fail'
                exitCode = -1073741515
                windowsStatus = '0xC0000135'
                reason = 'Initial T-038 harness run omitted the controlled Qt SDK bin needed only for Qt6Test.dll. Package transaction, normal first launch, and cleanup completed; no delivery dependency was added.'
            },
            [ordered]@{
                result = 'fail'
                exitCode = -1
                timeoutSeconds = 196
                boundedReproductionExitCode = -1073740791
                reason = 'A second attempt forced the external Qt Test executable to resolve through the installed package layout. It stalled before creating a report and was terminated; a bounded reproduction identified incompatible offscreen platform-plugin discovery for that non-delivery executable. The package App itself launched normally.'
            }
        )
        diagnostics = $policy
        evaluationBoundary = [ordered]@{
            productEffectEvaluation = 'not-evaluated(deferred-to-personal-use-feedback)'
            naturalnessEvaluation = 'not-evaluated(deferred-to-personal-use-feedback)'
            realVfrEvaluation = 'not-evaluated(deferred-to-personal-use-feedback)'
            formalProductPerformanceEvaluation = 'not-evaluated(deferred-to-personal-use-feedback)'
            minimumWindowsCompatibility = 'not-evaluated'
            formalH264AacContainerMatrix = 'not-evaluated'
            publicDistributionApproved = $false
            productionReleaseApproved = $false
        }
    }
    $resultPath = Join-Path $evidence 'result.json'
    $result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $resultPath -Encoding utf8
    $result | ConvertTo-Json -Depth 10
}
finally {
    if ($null -ne $firstLaunchProcess -and -not $firstLaunchProcess.HasExited) {
        Stop-Process -Id $firstLaunchProcess.Id -Force -ErrorAction SilentlyContinue
    }
    if (-not $manualUninstalled -and (Test-Path -LiteralPath $installRoot) -and (Test-Path -LiteralPath $stateRoot)) {
        try {
            & $transactionTool -Action Uninstall -BundleRoot $bundle -InstallRoot $installRoot -StateRoot $stateRoot | Out-Null
        }
        catch {
            Write-Warning "Cleanup uninstall failed: $($_.Exception.Message)"
        }
    }
    if ($null -eq $oldQpa) {
        Remove-Item -LiteralPath Env:QT_QPA_PLATFORM -ErrorAction SilentlyContinue
    } else { $env:QT_QPA_PLATFORM = $oldQpa }
    if ($null -eq $oldQuickBackend) {
        Remove-Item -LiteralPath Env:QT_QUICK_BACKEND -ErrorAction SilentlyContinue
    } else { $env:QT_QUICK_BACKEND = $oldQuickBackend }
    if ($null -eq $oldControlsStyle) {
        Remove-Item -LiteralPath Env:QT_QUICK_CONTROLS_STYLE -ErrorAction SilentlyContinue
    } else { $env:QT_QUICK_CONTROLS_STYLE = $oldControlsStyle }
    $env:PATH = $oldPath
    $env:TEMP = $oldTemp
    $env:TMP = $oldTmp
}
