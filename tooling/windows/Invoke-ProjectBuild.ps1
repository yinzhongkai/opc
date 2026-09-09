[CmdletBinding()]
param(
    [ValidateSet('windows-msvc-x64-debug', 'windows-msvc-x64-release', 'ci-windows-msvc-x64')]
    [string]$Preset = 'windows-msvc-x64-debug',

    [ValidateSet('Validate', 'Configure', 'Build', 'Test', 'Install', 'All')]
    [string]$Stage = 'All',

    [string]$QtRoot = 'C:\sr\q\qt6112',
    [string]$VcpkgRoot = 'C:\sr\tools\vcpkg-2026.07.29',
    [string]$EvidenceRoot = '',
    [ValidateRange(1, 64)]
    [int]$Parallel = 10,
    [switch]$Clean,
    [switch]$UseExistingDependencies,
    [switch]$AllowWdacFallback
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$sourceRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$outRoot = [System.IO.Path]::GetFullPath((Join-Path $sourceRoot 'out'))
$buildRoot = [System.IO.Path]::GetFullPath((Join-Path $outRoot "build\$Preset"))
$installRoot = [System.IO.Path]::GetFullPath((Join-Path $outRoot "install\$Preset"))
if ([string]::IsNullOrWhiteSpace($EvidenceRoot)) {
    $EvidenceRoot = Join-Path $outRoot "evidence\T-013\$Preset"
}
$EvidenceRoot = [System.IO.Path]::GetFullPath($EvidenceRoot)

$expectedVcpkgBaseline = '9e593bb18ea69cc5095e012465dcd675a822ed0d'
$expectedQtCoreReleaseSha256 = '94E697C5C7B861E1F9072CB2FB251D3C807ECA053D7EE0F1B4A3BA08023AC1AC'
$expectedQtCoreDebugSha256 = '647A6565A3F4B7DAC0410213471C0EFE2F24FE1BBBC513CC589570BEE6254E79'
$expectedQtSummarySha256 = '4BCBDAA6DCBCB98BF2F44F700B77FB35D8EE2982996E1B2051C278C2355F49A3'

function Assert-ExistingPath {
    param([Parameter(Mandatory)][string]$LiteralPath, [Parameter(Mandatory)][string]$Description)
    if (-not (Test-Path -LiteralPath $LiteralPath)) {
        throw "$Description was not found: $LiteralPath"
    }
}

function Assert-FileHash {
    param(
        [Parameter(Mandatory)][string]$LiteralPath,
        [Parameter(Mandatory)][string]$Expected,
        [Parameter(Mandatory)][string]$Description
    )
    Assert-ExistingPath -LiteralPath $LiteralPath -Description $Description
    $actual = (Get-FileHash -LiteralPath $LiteralPath -Algorithm SHA256).Hash
    if ($actual -ne $Expected) {
        throw "$Description SHA-256 mismatch. Expected $Expected, found $actual"
    }
}

function Invoke-NativeChecked {
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(ValueFromRemainingArguments)][string[]]$Arguments
    )
    $commandLine = "> $FilePath $($Arguments -join ' ')"
    Write-Host $commandLine
    Add-Content -LiteralPath $script:NativeLogPath -Value $commandLine -Encoding utf8
    & $FilePath @Arguments 2>&1 | Tee-Object -FilePath $script:NativeLogPath -Append
    $nativeExitCode = $LASTEXITCODE
    if ($nativeExitCode -ne 0) {
        throw "Command failed with exit code ${nativeExitCode}: $FilePath"
    }
}

function Import-VsDevEnvironment {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    Assert-ExistingPath -LiteralPath $vswhere -Description 'vswhere.exe'
    $installationPath = & $vswhere -latest -products Microsoft.VisualStudio.Product.BuildTools `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($installationPath)) {
        throw 'MSVC 2022 Build Tools with the x64 C++ workload was not found'
    }

    $script:VsInstallRoot = $installationPath.Trim()
    $vsDevCmd = Join-Path $script:VsInstallRoot 'Common7\Tools\VsDevCmd.bat'
    Assert-ExistingPath -LiteralPath $vsDevCmd -Description 'VsDevCmd.bat'
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
        if ($line -match '^([^=]+)=(.*)$') {
            if ($Matches[1] -ine 'Path') {
                Set-Item -LiteralPath "Env:$($Matches[1])" -Value $Matches[2]
            }
        }
    }
    # The sandbox may inherit both PATH and Path entries. Keep exactly the
    # canonical PATH produced by VsDevCmd so the original Path cannot overwrite it.
    Remove-Item -LiteralPath Env:PATH -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath Env:Path -ErrorAction SilentlyContinue
    Set-Item -LiteralPath Env:Path -Value $developerPathLine.Substring(5)
}

function Assert-CleanTarget {
    param([Parameter(Mandatory)][string]$LiteralPath)
    $resolved = [System.IO.Path]::GetFullPath($LiteralPath)
    $requiredPrefix = $outRoot.TrimEnd('\') + '\'
    if (-not $resolved.StartsWith($requiredPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean outside the repository output directory: $resolved"
    }
    if ($resolved -eq $outRoot) {
        throw "Refusing to clean the entire output root: $resolved"
    }
}

function Invoke-Validation {
    $qtRootFull = [System.IO.Path]::GetFullPath($QtRoot)
    $vcpkgRootFull = [System.IO.Path]::GetFullPath($VcpkgRoot)
    Assert-ExistingPath -LiteralPath (Join-Path $qtRootFull 'lib\cmake\Qt6\Qt6Config.cmake') -Description 'Qt 6 CMake package'
    Assert-ExistingPath -LiteralPath (Join-Path $vcpkgRootFull 'vcpkg.exe') -Description 'vcpkg executable'
    Assert-ExistingPath -LiteralPath (Join-Path $vcpkgRootFull 'scripts\buildsystems\vcpkg.cmake') -Description 'vcpkg CMake toolchain'

    Assert-FileHash -LiteralPath (Join-Path $qtRootFull 'bin\Qt6Core.dll') `
        -Expected $expectedQtCoreReleaseSha256 -Description 'Qt6Core.dll'
    Assert-FileHash -LiteralPath (Join-Path $qtRootFull 'bin\Qt6Cored.dll') `
        -Expected $expectedQtCoreDebugSha256 -Description 'Qt6Cored.dll'
    Assert-FileHash -LiteralPath (Join-Path $qtRootFull 'config.summary') `
        -Expected $expectedQtSummarySha256 -Description 'Qt config.summary'

    $vcpkgCommit = (& git.exe -c "safe.directory=$($vcpkgRootFull.Replace('\', '/'))" `
        -C $vcpkgRootFull rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $vcpkgCommit -ne $expectedVcpkgBaseline) {
        throw "vcpkg checkout must be $expectedVcpkgBaseline; found $vcpkgCommit"
    }

    $compilerBanner = (& cl.exe 2>&1 | Out-String)
    if ($compilerBanner -notmatch '19\.44\.35228') {
        throw "MSVC 19.44.35228 is required by the Qt ABI baseline. cl.exe reported: $compilerBanner"
    }
    if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
        throw "The active Visual Studio target architecture is not x64: $env:VSCMD_ARG_TGT_ARCH"
    }

    Invoke-NativeChecked $script:CMakePath --version
    Invoke-NativeChecked $script:NinjaPath --version
    Invoke-NativeChecked (Join-Path $vcpkgRootFull 'vcpkg.exe') version
    Write-Host 'Toolchain validation passed: MSVC 19.44, Windows x64, Qt 6.11.2, dynamic CRT, pinned vcpkg.'
}

New-Item -ItemType Directory -Force -Path $EvidenceRoot | Out-Null
$runTimestamp = (Get-Date).ToString('yyyyMMdd-HHmmss')
$transcriptPath = Join-Path $EvidenceRoot "build-$runTimestamp.log"
$script:NativeLogPath = Join-Path $EvidenceRoot "native-$runTimestamp.log"
New-Item -ItemType File -Force -Path $script:NativeLogPath | Out-Null
Start-Transcript -LiteralPath $transcriptPath -Force | Out-Null

try {
    $env:VSLANG = '1033'
    Import-VsDevEnvironment
    $QtRoot = [System.IO.Path]::GetFullPath($QtRoot)
    $VcpkgRoot = [System.IO.Path]::GetFullPath($VcpkgRoot)
    $env:QT_ROOT = $QtRoot
    $env:VCPKG_ROOT = $VcpkgRoot
    $env:VCPKG_DISABLE_METRICS = '1'
    $env:CMAKE_BUILD_PARALLEL_LEVEL = $Parallel.ToString()
    if ($AllowWdacFallback) {
        $env:SPACE_RHYTHM_ALLOW_WDAC_FALLBACK = '1'
        Write-Warning 'WDAC fallback is explicitly enabled for this managed-host validation run.'
    }
    else {
        Remove-Item -LiteralPath Env:SPACE_RHYTHM_ALLOW_WDAC_FALLBACK -ErrorAction SilentlyContinue
    }
    $script:CMakePath = Join-Path $script:VsInstallRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    $script:CTestPath = Join-Path $script:VsInstallRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
    $script:NinjaPath = Join-Path $script:VsInstallRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
    Assert-ExistingPath -LiteralPath $script:CMakePath -Description 'CMake executable'
    Assert-ExistingPath -LiteralPath $script:CTestPath -Description 'CTest executable'
    Assert-ExistingPath -LiteralPath $script:NinjaPath -Description 'Ninja executable'
    $env:NINJA_PATH = $script:NinjaPath
    $env:Path = "$(Split-Path -Parent $script:CMakePath);$(Split-Path -Parent $script:NinjaPath);$(Join-Path $QtRoot 'bin');$env:Path"

    if (-not [string]::IsNullOrWhiteSpace($env:VCToolsRedistDir)) {
        $debugCrtRoot = Join-Path $env:VCToolsRedistDir 'debug_nonredist\x64\Microsoft.VC143.DebugCRT'
        if (Test-Path -LiteralPath $debugCrtRoot) {
            $env:Path = "$debugCrtRoot;$env:Path"
        }
    }

    Invoke-Validation
    if ($Stage -eq 'Validate') {
        return
    }

    if ($Clean) {
        foreach ($target in @($buildRoot, $installRoot)) {
            Assert-CleanTarget -LiteralPath $target
            if (Test-Path -LiteralPath $target) {
                Remove-Item -LiteralPath $target -Recurse -Force
            }
        }
    }

    if ($Stage -in @('Configure', 'All')) {
        $configureArguments = @('--preset', $Preset)
        if ($UseExistingDependencies) {
            $configureArguments += '-DVCPKG_MANIFEST_INSTALL=OFF'
            Write-Warning 'Reusing the previously baseline-validated vcpkg install without invoking vcpkg again.'
        }
        Invoke-NativeChecked $script:CMakePath @configureArguments
    }
    if ($Stage -eq 'Configure') {
        return
    }

    if ($Stage -in @('Build', 'All')) {
        Invoke-NativeChecked $script:CMakePath --build --preset $Preset --parallel $Parallel
    }
    if ($Stage -eq 'Build') {
        return
    }

    if ($Stage -in @('Test', 'All')) {
        Invoke-NativeChecked $script:CTestPath --preset $Preset
    }
    if ($Stage -eq 'Test') {
        return
    }

    if ($Stage -in @('Install', 'All')) {
        Invoke-NativeChecked $script:CMakePath --install $buildRoot --prefix $installRoot

        $deployMode = if ($Preset -eq 'windows-msvc-x64-debug') { '--debug' } else { '--release' }
        $applicationPath = Join-Path $installRoot 'bin\space-rhythm.exe'
        $workerPath = Join-Path $installRoot 'bin\space-rhythm-worker.exe'
        $deployDirectory = Join-Path $installRoot 'bin'
        Assert-ExistingPath -LiteralPath $applicationPath -Description 'installed application'
        Assert-ExistingPath -LiteralPath $workerPath -Description 'installed worker'

        Invoke-NativeChecked (Join-Path $QtRoot 'bin\windeployqt.exe') `
            $deployMode --no-translations --include-plugins qoffscreen `
            --qmldir (Join-Path $sourceRoot 'src\app\qml') `
            --dir $deployDirectory $applicationPath

        $env:QT_QPA_PLATFORM = 'offscreen'
        Invoke-NativeChecked -FilePath $script:CMakePath -Arguments @(
            "-DAPP_EXE=$applicationPath"
            "-DQML_EXE=$(Join-Path $QtRoot 'bin\qml.exe')"
            "-DAPP_QML=$(Join-Path $sourceRoot 'src\app\qml\Main.qml')"
            '-P'
            (Join-Path $sourceRoot 'tests\cmake\RunAppSmoke.cmake')
        )
        Invoke-NativeChecked -FilePath $script:CMakePath -Arguments @(
            "-DWORKER_EXE=$workerPath"
            "-DQML_TEST_RUNNER=$(Join-Path $QtRoot 'bin\qmltestrunner.exe')"
            "-DQML_TEST_DIR=$(Join-Path $sourceRoot 'tests\qml')"
            '-P'
            (Join-Path $sourceRoot 'tests\cmake\RunWorkerSmoke.cmake')
        )
        Invoke-NativeChecked dumpbin.exe /headers $applicationPath
        Invoke-NativeChecked dumpbin.exe /headers $workerPath
        Invoke-NativeChecked dumpbin.exe /dependents $applicationPath
        Invoke-NativeChecked dumpbin.exe /dependents $workerPath

        $manifestPath = Join-Path $EvidenceRoot 'installed-files.sha256.csv'
        Get-ChildItem -LiteralPath $installRoot -Recurse -File |
            Sort-Object FullName |
            ForEach-Object {
                [pscustomobject]@{
                    RelativePath = [System.IO.Path]::GetRelativePath($installRoot, $_.FullName)
                    Length = $_.Length
                    SHA256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
                }
            } | Export-Csv -LiteralPath $manifestPath -NoTypeInformation -Encoding utf8
    }
}
finally {
    Stop-Transcript | Out-Null
    Write-Host "Evidence: $EvidenceRoot"
}
