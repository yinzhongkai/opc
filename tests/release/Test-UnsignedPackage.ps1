[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$BundleRoot,
    [string]$TestRoot = '',
    [switch]$RunSmoke
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-Condition {
    param([Parameter(Mandatory)][bool]$Condition, [Parameter(Mandatory)][string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-HashManifest {
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][string]$ManifestRelativePath
    )
    $manifestPath = Join-Path $Root $ManifestRelativePath
    Assert-Condition -Condition (Test-Path -LiteralPath $manifestPath -PathType Leaf) `
        -Message "Hash manifest is missing: $manifestPath"
    $entries = @(Import-Csv -LiteralPath $manifestPath)
    Assert-Condition -Condition ($entries.Count -gt 0) -Message "Hash manifest is empty: $manifestPath"
    $expected = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in $entries) {
        $relative = $entry.RelativePath.Replace('/', '\')
        Assert-Condition -Condition (-not [System.IO.Path]::IsPathRooted($relative) -and
            $relative -notmatch '(^|\\)\.\.(\\|$)') -Message "Unsafe manifest path: $relative"
        Assert-Condition -Condition $expected.Add($relative) -Message "Duplicate manifest path: $relative"
        $path = Join-Path $Root $relative
        Assert-Condition -Condition (Test-Path -LiteralPath $path -PathType Leaf) `
            -Message "Manifest file is missing: $relative"
        $file = Get-Item -LiteralPath $path
        Assert-Condition -Condition ($file.Length -eq [long]$entry.Length) `
            -Message "Manifest length mismatch: $relative"
        Assert-Condition -Condition ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -eq $entry.SHA256) `
            -Message "Manifest SHA-256 mismatch: $relative"
    }
    foreach ($file in (Get-ChildItem -LiteralPath $Root -Recurse -File)) {
        $relative = [System.IO.Path]::GetRelativePath($Root, $file.FullName).Replace('/', '\')
        if ($relative -ieq $ManifestRelativePath.Replace('/', '\')) { continue }
        Assert-Condition -Condition $expected.Contains($relative) -Message "Unregistered bundle file: $relative"
    }
}

$sourceRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if ([string]::IsNullOrWhiteSpace($TestRoot)) {
    $TestRoot = Join-Path $sourceRoot 'out\tests\T-037-unsigned-transaction'
}
$testRootFull = [System.IO.Path]::GetFullPath($TestRoot)
$outRoot = [System.IO.Path]::GetFullPath((Join-Path $sourceRoot 'out'))
Assert-Condition -Condition $testRootFull.StartsWith($outRoot.TrimEnd('\') + '\', [System.StringComparison]::OrdinalIgnoreCase) `
    -Message "TestRoot must stay under the repository output root: $testRootFull"
if (Test-Path -LiteralPath $testRootFull) {
    Remove-Item -LiteralPath $testRootFull -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $testRootFull | Out-Null

$bundle = [System.IO.Path]::GetFullPath($BundleRoot)
$payload = Join-Path $bundle 'payload\SpaceRhythm'
$transactionTool = Join-Path $bundle 'tools\Invoke-UnsignedInstallTransaction.ps1'
Assert-HashManifest -Root $bundle -ManifestRelativePath 'manifest\bundle-files.sha256.csv'
Assert-Condition -Condition (Test-Path -LiteralPath $transactionTool -PathType Leaf) `
    -Message 'Bundle transaction tool is missing'

$inputs = Get-Content -Raw -LiteralPath (Join-Path $payload 'manifest\build-inputs.json') | ConvertFrom-Json
Assert-Condition -Condition ($inputs.schemaVersion -eq 2) -Message 'Build input inventory schema is not version 2'
Assert-Condition -Condition ($inputs.packageKind -eq 'unsigned-engineering') -Message 'Package kind is not unsigned-engineering'
Assert-Condition -Condition ($inputs.candidateEligible -eq $false) -Message 'Unsigned package must not be candidate-eligible'
Assert-Condition -Condition ($inputs.securityBoundary.signaturesApplied -eq $false) -Message 'Unsigned package reports applied signatures'
Assert-Condition -Condition ($inputs.securityBoundary.signingCredentialAccess -eq 'none') -Message 'Unsigned package reports credential access'

$requiredComponentFields = @(
    'name', 'version', 'purl', 'sourceUrl', 'sourceCommitOrArchiveHash', 'buildConfigurationHash',
    'architecture', 'linkage', 'licenseExpression', 'licenseTextPath', 'copyrightNotice', 'modified',
    'patches', 'redistributionBasis', 'supplier', 'runtimeRelativePath', 'fileSha256',
    'authenticodeStatus', 'signerThumbprint', 'taskEvidence')
$expectedComponents = @(
    'Space Rhythm', 'Qt', 'FFmpeg', 'OpenCV', 'KissFFT', 'GoogleTest',
    'Microsoft Windows SDK D3D Redist')
foreach ($componentName in $expectedComponents) {
    $component = @($inputs.components | Where-Object { $_.name -eq $componentName })
    Assert-Condition -Condition ($component.Count -eq 1) `
        -Message "Build input inventory must contain exactly one $componentName component"
    foreach ($field in $requiredComponentFields) {
        Assert-Condition -Condition ($component[0].PSObject.Properties.Name -contains $field) `
            -Message "Build input component $componentName lacks required field $field"
    }
    Assert-Condition -Condition (-not [string]::IsNullOrWhiteSpace([string]$component[0].buildConfigurationHash)) `
        -Message "Build input component $componentName lacks a build configuration hash"
    Assert-Condition -Condition (@($component[0].runtimeRelativePath).Count -eq @($component[0].fileSha256).Count) `
        -Message "Build input component $componentName has inconsistent runtime path/hash counts"
}

foreach ($forbidden in @('qmltooling', 'generic')) {
    Assert-Condition -Condition (-not (Test-Path -LiteralPath (Join-Path $payload "bin\$forbidden"))) `
        -Message "Forbidden development plugin directory is present: $forbidden"
}
Assert-Condition -Condition (-not (Test-Path -LiteralPath (Join-Path $payload 'bin\vc_redist.x64.exe'))) `
    -Message 'Unconfirmed VC Redistributable installer is present'
foreach ($required in @('bin\space-rhythm.exe', 'bin\space-rhythm-worker.exe', 'bin\Qt6QmlMeta.dll', 'bin\platforms\qwindows.dll', 'bin\platforms\qoffscreen.dll')) {
    Assert-Condition -Condition (Test-Path -LiteralPath (Join-Path $payload $required) -PathType Leaf) `
        -Message "Required payload file is missing: $required"
}

$spdx = Get-Content -Raw -LiteralPath (Join-Path $payload 'sbom\space-rhythm.spdx.json') | ConvertFrom-Json
Assert-Condition -Condition ($spdx.spdxVersion -eq 'SPDX-2.3') -Message 'Generated SBOM is not SPDX 2.3'
Assert-Condition -Condition ($spdx.dataLicense -eq 'CC0-1.0') -Message 'Generated SPDX data license is invalid'
Assert-Condition -Condition ($spdx.packages.Count -ge 6) -Message 'Generated SPDX component inventory is incomplete'

$installRoot = Join-Path $testRootFull 'install\SpaceRhythm'
$stateRoot = Join-Path $testRootFull 'state'
$externalDataRoot = Join-Path $testRootFull 'external-user-data'
New-Item -ItemType Directory -Force -Path $externalDataRoot | Out-Null
$externalSentinel = Join-Path $externalDataRoot 'must-survive.txt'
Set-Content -LiteralPath $externalSentinel -Value 'outside install and state roots' -Encoding utf8
& $transactionTool -Action Validate -BundleRoot $bundle -InstallRoot $installRoot -StateRoot $stateRoot
& $transactionTool -Action Install -BundleRoot $bundle -InstallRoot $installRoot -StateRoot $stateRoot
Assert-Condition -Condition (Test-Path -LiteralPath (Join-Path $installRoot 'bin\space-rhythm.exe')) `
    -Message 'Transaction install did not create the application'
if ($RunSmoke) {
    $oldPlatform = $env:QT_QPA_PLATFORM
    try {
        $env:QT_QPA_PLATFORM = 'offscreen'
        $appOutput = @(& (Join-Path $installRoot 'bin\space-rhythm.exe') --smoke 2>&1)
        $appExit = $LASTEXITCODE
        Assert-Condition -Condition ($appExit -eq 0) `
            -Message "Installed App smoke failed with exit code $appExit`: $($appOutput -join [Environment]::NewLine)"
        Assert-Condition -Condition (($appOutput -join "`n") -match 'SPACE_RHYTHM_APP_SMOKE_OK Qt=6\.11\.2 arch=x64') `
            -Message 'Installed App smoke did not emit the required marker'
        $workerOutput = @(& (Join-Path $installRoot 'bin\space-rhythm-worker.exe') --smoke 2>&1)
        $workerExit = $LASTEXITCODE
        Assert-Condition -Condition ($workerExit -eq 0) `
            -Message "Installed Worker smoke failed with exit code $workerExit`: $($workerOutput -join [Environment]::NewLine)"
        Assert-Condition -Condition (($workerOutput -join "`n") -match 'SPACE_RHYTHM_WORKER_SMOKE_OK Qt=6\.11\.2 arch=x64') `
            -Message 'Installed Worker smoke did not emit the required marker'
    }
    finally {
        if ($null -eq $oldPlatform) {
            Remove-Item -LiteralPath Env:QT_QPA_PLATFORM -ErrorAction SilentlyContinue
        }
        else {
            $env:QT_QPA_PLATFORM = $oldPlatform
        }
    }
}
$unregisteredPath = Join-Path $installRoot 'unregistered-file.txt'
Set-Content -LiteralPath $unregisteredPath -Value 'must fail closed' -Encoding utf8
$repairRejected = $false
try {
    & $transactionTool -Action Repair -BundleRoot $bundle -InstallRoot $installRoot -StateRoot $stateRoot
}
catch {
    $repairRejected = $true
}
Assert-Condition -Condition $repairRejected -Message 'Repair accepted an unregistered installed file'
Remove-Item -LiteralPath $unregisteredPath -Force
& $transactionTool -Action Repair -BundleRoot $bundle -InstallRoot $installRoot -StateRoot $stateRoot
& $transactionTool -Action Rollback -BundleRoot $bundle -InstallRoot $installRoot -StateRoot $stateRoot
& $transactionTool -Action Validate -BundleRoot $bundle -InstallRoot $installRoot -StateRoot $stateRoot
& $transactionTool -Action Uninstall -BundleRoot $bundle -InstallRoot $installRoot -StateRoot $stateRoot
Assert-Condition -Condition (-not (Test-Path -LiteralPath $installRoot)) `
    -Message 'Transaction uninstall left the application root behind'
Assert-Condition -Condition (Test-Path -LiteralPath $externalSentinel -PathType Leaf) `
    -Message 'Transaction changed data outside the explicit install and state roots'

[pscustomobject][ordered]@{
    Result = 'pass'
    PackageKind = $inputs.packageKind
    Version = $inputs.version
    SourceCommit = $inputs.sourceCommit
    CandidateEligible = $inputs.candidateEligible
    TestRoot = $testRootFull
    Operations = @('validate', 'install') + $(if ($RunSmoke) { @('app-smoke', 'worker-smoke') } else { @() }) + `
        @('reject-unregistered-repair', 'repair', 'rollback', 'validate-installed', 'uninstall', 'preserve-external-data')
} | ConvertTo-Json -Depth 5
