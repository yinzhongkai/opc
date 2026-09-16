[CmdletBinding()]
param(
    [ValidateSet('Configure', 'Build', 'Install', 'Smoke', 'Manifest', 'All')]
    [string]$Stage = 'All',

    [ValidateRange(1, 32)]
    [int]$Parallel = 10,

    [string]$QtSourceArchive = 'C:\sr\downloads\qt-everywhere-src-6.11.2.tar.xz',
    [string]$QtSourceRoot = 'C:\sr\s\qt6112',
    [string]$QtBuildRoot = 'C:\sr\b\qt6112',
    [string]$QtInstallRoot = 'C:\sr\q\qt6112',
    [string]$EvidenceRoot = 'C:\sr\evidence\T-012',
    [string]$PythonRoot = 'C:\sr\tools\python-3.13.15\tools',

    [switch]$CleanBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true

$expectedArchiveSize = 1019661552L
$expectedArchiveSha256 = '6DCFBCA271D76A6502741A2C0DC6FC98EF7DD0B7B4CFD0ABCEBB285A86A26F33'
$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$smokeSourceRoot = Join-Path $PSScriptRoot 'smoke'
$nativeSmokeSource = Join-Path $PSScriptRoot 'native-smoke\main.cpp'
$smokeBuildRoot = 'C:\sr\b\qt6112-smoke'
$smokeStageRoot = 'C:\sr\stage\qt6112-smoke'

function Assert-ExactChildPath {
    param(
        [Parameter(Mandatory)] [string]$Path,
        [Parameter(Mandatory)] [string]$Parent
    )

    $candidate = [System.IO.Path]::GetFullPath($Path).TrimEnd('\')
    $root = [System.IO.Path]::GetFullPath($Parent).TrimEnd('\')
    if ($candidate -eq $root -or
        -not $candidate.StartsWith("$root\", [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing operation outside the expected root '$root': $candidate"
    }
}

function Invoke-LoggedNative {
    param(
        [Parameter(Mandatory)] [string]$FilePath,
        [Parameter(Mandatory)] [AllowEmptyCollection()] [string[]]$ArgumentList,
        [Parameter(Mandatory)] [string]$LogPath
    )

    & $FilePath @ArgumentList 2>&1 | Tee-Object -FilePath $LogPath
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE. See $LogPath"
    }
}

function Import-MsvcEnvironment {
    param([Parameter(Mandatory)] [string]$VsDevCmd)

    $developerCommand = "call `"$VsDevCmd`" -no_logo -arch=x64 -host_arch=x64 >nul && set"
    $environmentLines = & $env:ComSpec /d /c $developerCommand
    if ($LASTEXITCODE -ne 0) {
        throw "VsDevCmd failed with exit code $LASTEXITCODE"
    }

    foreach ($line in $environmentLines) {
        if ($line -match '^([^=]+)=(.*)$') {
            Set-Item -LiteralPath "Env:$($Matches[1])" -Value $Matches[2]
        }
    }
}

function Write-HashedManifest {
    param(
        [Parameter(Mandatory)] [string]$Root,
        [Parameter(Mandatory)] [string]$OutputPath,
        [string[]]$Extensions
    )

    $rootPath = [System.IO.Path]::GetFullPath($Root).TrimEnd('\')
    $files = Get-ChildItem -LiteralPath $rootPath -Recurse -File | Sort-Object FullName
    if ($Extensions) {
        $files = $files | Where-Object { $Extensions -contains $_.Extension.ToLowerInvariant() }
    }

    $files | ForEach-Object {
        [pscustomobject]@{
            Path = $_.FullName.Substring($rootPath.Length + 1).Replace('\', '/')
            Bytes = $_.Length
            SHA256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        }
    } | Export-Csv -LiteralPath $OutputPath -NoTypeInformation -Encoding utf8
}

foreach ($requiredPath in @(
    $QtSourceArchive,
    (Join-Path $QtSourceRoot 'configure.bat'),
    (Join-Path $PythonRoot 'python.exe'),
    $smokeSourceRoot,
    $nativeSmokeSource
)) {
    if (-not (Test-Path -LiteralPath $requiredPath)) {
        throw "Required input is missing: $requiredPath"
    }
}

$archive = Get-Item -LiteralPath $QtSourceArchive
if ($archive.Length -ne $expectedArchiveSize) {
    throw "Qt source archive size mismatch: expected $expectedArchiveSize, got $($archive.Length)"
}
$archiveHash = (Get-FileHash -LiteralPath $archive.FullName -Algorithm SHA256).Hash
if ($archiveHash -ne $expectedArchiveSha256) {
    throw "Qt source archive SHA-256 mismatch: expected $expectedArchiveSha256, got $archiveHash"
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw "vswhere was not found: $vswhere"
}

$requiredComponents = @(
    'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
    'Microsoft.VisualStudio.Component.Windows11SDK.26100',
    'Microsoft.VisualStudio.Component.VC.CMake.Project'
)
$vsInstallRoot = & $vswhere -latest -products '*' -requires @requiredComponents -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $vsInstallRoot) {
    throw 'A complete Visual Studio 2022 Build Tools instance with MSVC, Windows SDK 26100, and CMake tools was not found.'
}
$vsInstallRoot = $vsInstallRoot.Trim()
$vsDevCmd = Join-Path $vsInstallRoot 'Common7\Tools\VsDevCmd.bat'
Import-MsvcEnvironment -VsDevCmd $vsDevCmd
$env:VSLANG = '1033'
$env:Path = "$PythonRoot;$env:Path"

$cmake = Join-Path $vsInstallRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ninja = Join-Path $vsInstallRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
$python = Join-Path $PythonRoot 'python.exe'
$cl = (Get-Command cl.exe -ErrorAction Stop).Source
$link = (Get-Command link.exe -ErrorAction Stop).Source
$dumpbin = (Get-Command dumpbin.exe -ErrorAction Stop).Source
$rc = (Get-Command rc.exe -ErrorAction Stop).Source
$git = (Get-Command git.exe -ErrorAction Stop).Source
$debugCrt = Get-ChildItem -LiteralPath (Join-Path $vsInstallRoot 'VC\Redist\MSVC') -Recurse -Filter 'vcruntime140d.dll' -File |
    Where-Object { $_.FullName -like '*\debug_nonredist\x64\Microsoft.VC143.DebugCRT\vcruntime140d.dll' -and $_.FullName -notlike '*\onecore\*' } |
    Sort-Object FullName -Descending |
    Select-Object -First 1
if (-not $debugCrt) {
    throw 'The MSVC x64 Debug CRT was not found.'
}
$debugCrtRoot = $debugCrt.Directory.FullName
$qtBuildBin = Join-Path $QtBuildRoot 'qtbase\bin'
$env:Path = "$qtBuildBin;$debugCrtRoot;$env:Path"

foreach ($tool in @($cmake, $ninja, $python, $cl, $link, $dumpbin, $rc, $git)) {
    if (-not (Test-Path -LiteralPath $tool)) {
        throw "Required tool is missing: $tool"
    }
}
if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
    throw "VsDevCmd target architecture is '$env:VSCMD_ARG_TGT_ARCH', expected 'x64'."
}

New-Item -ItemType Directory -Path $EvidenceRoot -Force | Out-Null

$toolchainEvidence = @(
    "CapturedAt=$((Get-Date).ToString('o'))"
    "Repository=$repoRoot"
    "QtSourceArchive=$($archive.FullName)"
    "QtSourceArchiveBytes=$($archive.Length)"
    "QtSourceArchiveSHA256=$archiveHash"
    "QtSourceRoot=$QtSourceRoot"
    "QtBuildRoot=$QtBuildRoot"
    "QtInstallRoot=$QtInstallRoot"
    "VisualStudioInstallRoot=$vsInstallRoot"
    "VSCMD_ARG_HOST_ARCH=$env:VSCMD_ARG_HOST_ARCH"
    "VSCMD_ARG_TGT_ARCH=$env:VSCMD_ARG_TGT_ARCH"
    "VSLANG=$env:VSLANG"
    "WindowsSdkDir=$env:WindowsSdkDir"
    "WindowsSDKVersion=$env:WindowsSDKVersion"
    "MsvcDebugCrtRoot=$debugCrtRoot"
    (& $vswhere -latest -products '*' -format json | Out-String).TrimEnd()
    (& $cmake --version | Out-String).TrimEnd()
    "ninja $(& $ninja --version)"
    (& $python --version 2>&1 | Out-String).TrimEnd()
    (& $git --version | Out-String).TrimEnd()
)
$toolchainEvidence | Set-Content -LiteralPath (Join-Path $EvidenceRoot 'toolchain.txt') -Encoding utf8

if ($Stage -in @('Configure', 'All')) {
    $nativeExe = Join-Path $EvidenceRoot 'msvc-x64-smoke.exe'
    $nativeObject = Join-Path $EvidenceRoot 'msvc-x64-smoke.obj'
    Invoke-LoggedNative -FilePath $cl -ArgumentList @(
        '/nologo', '/Bv', '/std:c++20', '/EHsc', '/MD', '/W4', '/WX',
        "/Fo:$nativeObject", "/Fe:$nativeExe", $nativeSmokeSource
    ) -LogPath (Join-Path $EvidenceRoot 'msvc-x64-compile.log')
    Invoke-LoggedNative -FilePath $dumpbin -ArgumentList @('/headers', $nativeExe) -LogPath (Join-Path $EvidenceRoot 'msvc-x64-headers.log')
    Invoke-LoggedNative -FilePath $dumpbin -ArgumentList @('/dependents', $nativeExe) -LogPath (Join-Path $EvidenceRoot 'msvc-x64-dependents.log')
    Invoke-LoggedNative -FilePath $nativeExe -ArgumentList @() -LogPath (Join-Path $EvidenceRoot 'msvc-x64-run.log')
}

$configureArguments = @(
    '-prefix', $QtInstallRoot,
    '-opensource',
    '-confirm-license',
    '-shared',
    '-debug-and-release',
    '-submodules', 'qtbase,qtdeclarative,qtshadertools,qtmultimedia',
    '-skip', 'qtimageformats,qtlanguageserver,qtsvg,qtquick3d,qtquicktimeline',
    '-nomake', 'examples',
    '-nomake', 'tests',
    '-no-feature-ffmpeg',
    '--',
    '-DQT_INSTALL_CONFIG_INFO_FILES=ON'
)
$configureArguments | Set-Content -LiteralPath (Join-Path $EvidenceRoot 'configure-arguments.txt') -Encoding utf8

if ($Stage -in @('Configure', 'All')) {
    Assert-ExactChildPath -Path $QtBuildRoot -Parent 'C:\sr\b'
    if ((Test-Path -LiteralPath $QtBuildRoot) -and $CleanBuild) {
        Remove-Item -LiteralPath $QtBuildRoot -Recurse -Force
    }
    if (Test-Path -LiteralPath (Join-Path $QtBuildRoot 'CMakeCache.txt')) {
        throw "The build directory is already configured. Pass -CleanBuild for a verified clean configure: $QtBuildRoot"
    }
    New-Item -ItemType Directory -Path $QtBuildRoot -Force | Out-Null
    Push-Location $QtBuildRoot
    try {
        Invoke-LoggedNative -FilePath (Join-Path $QtSourceRoot 'configure.bat') -ArgumentList $configureArguments -LogPath (Join-Path $EvidenceRoot 'configure.log')
    }
    finally {
        Pop-Location
    }
}

if ($Stage -in @('Build', 'All')) {
    foreach ($configuration in @('Release', 'Debug')) {
        Invoke-LoggedNative -FilePath $cmake -ArgumentList @(
            '--build', $QtBuildRoot,
            '--config', $configuration,
            '--parallel', $Parallel.ToString()
        ) -LogPath (Join-Path $EvidenceRoot "build-$($configuration.ToLowerInvariant()).log")
    }
}

if ($Stage -in @('Install', 'All')) {
    foreach ($configuration in @('Release', 'Debug')) {
        Invoke-LoggedNative -FilePath $cmake -ArgumentList @(
            '--install', $QtBuildRoot,
            '--config', $configuration
        ) -LogPath (Join-Path $EvidenceRoot "install-$($configuration.ToLowerInvariant()).log")
    }
}

if ($Stage -in @('Smoke', 'All')) {
    foreach ($path in @($smokeBuildRoot, $smokeStageRoot)) {
        Assert-ExactChildPath -Path $path -Parent 'C:\sr'
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -Recurse -Force
        }
    }

    Invoke-LoggedNative -FilePath $cmake -ArgumentList @(
        '-S', $smokeSourceRoot,
        '-B', $smokeBuildRoot,
        '-G', 'Ninja Multi-Config',
        "-DCMAKE_PREFIX_PATH=$QtInstallRoot",
        "-DCMAKE_INSTALL_PREFIX=$smokeStageRoot"
    ) -LogPath (Join-Path $EvidenceRoot 'smoke-configure.log')

    foreach ($configuration in @('Release', 'Debug')) {
        $configurationStage = Join-Path $smokeStageRoot $configuration
        Invoke-LoggedNative -FilePath $cmake -ArgumentList @(
            '--build', $smokeBuildRoot,
            '--config', $configuration,
            '--parallel', $Parallel.ToString()
        ) -LogPath (Join-Path $EvidenceRoot "smoke-build-$($configuration.ToLowerInvariant()).log")
        Invoke-LoggedNative -FilePath $cmake -ArgumentList @(
            '--install', $smokeBuildRoot,
            '--config', $configuration,
            '--prefix', $configurationStage
        ) -LogPath (Join-Path $EvidenceRoot "smoke-install-$($configuration.ToLowerInvariant()).log")

        $smokeExe = Join-Path $configurationStage 'bin\space_rhythm_qt_smoke.exe'
        $deployDirectory = Split-Path -Parent $smokeExe
        $windeployqt = Join-Path $QtInstallRoot 'bin\windeployqt.exe'
        $deployMode = if ($configuration -eq 'Debug') { '--debug' } else { '--release' }
        Invoke-LoggedNative -FilePath $windeployqt -ArgumentList @(
            $deployMode,
            '--compiler-runtime',
            '--include-plugins', 'qoffscreen',
            '--qmldir', $smokeSourceRoot,
            '--dir', $deployDirectory,
            $smokeExe
        ) -LogPath (Join-Path $EvidenceRoot "windeployqt-$($configuration.ToLowerInvariant()).log")

        Invoke-LoggedNative -FilePath $dumpbin -ArgumentList @('/headers', $smokeExe) -LogPath (Join-Path $EvidenceRoot "smoke-headers-$($configuration.ToLowerInvariant()).log")
        Invoke-LoggedNative -FilePath $dumpbin -ArgumentList @('/dependents', $smokeExe) -LogPath (Join-Path $EvidenceRoot "smoke-dependents-$($configuration.ToLowerInvariant()).log")

        $previousQpaPlatform = $env:QT_QPA_PLATFORM
        $env:QT_QPA_PLATFORM = 'offscreen'
        Push-Location $deployDirectory
        try {
            Invoke-LoggedNative -FilePath $smokeExe -ArgumentList @() -LogPath (Join-Path $EvidenceRoot "smoke-run-$($configuration.ToLowerInvariant()).log")
        }
        finally {
            Pop-Location
            $env:QT_QPA_PLATFORM = $previousQpaPlatform
        }
    }
}

if ($Stage -in @('Manifest', 'All')) {
    Write-HashedManifest -Root $QtInstallRoot -OutputPath (Join-Path $EvidenceRoot 'qt-sdk-binaries.csv') -Extensions @('.dll', '.exe', '.lib')
    Write-HashedManifest -Root $QtInstallRoot -OutputPath (Join-Path $EvidenceRoot 'qt-sdk-symbols.csv') -Extensions @('.pdb')
    Write-HashedManifest -Root $smokeStageRoot -OutputPath (Join-Path $EvidenceRoot 'smoke-runtime.csv')

    foreach ($binary in @(
        (Join-Path $QtInstallRoot 'bin\Qt6Core.dll'),
        (Join-Path $QtInstallRoot 'bin\Qt6Cored.dll')
    )) {
        if (Test-Path -LiteralPath $binary) {
            $name = [System.IO.Path]::GetFileNameWithoutExtension($binary)
            Invoke-LoggedNative -FilePath $dumpbin -ArgumentList @('/dependents', $binary) -LogPath (Join-Path $EvidenceRoot "$name-dependents.log")
        }
    }
}
