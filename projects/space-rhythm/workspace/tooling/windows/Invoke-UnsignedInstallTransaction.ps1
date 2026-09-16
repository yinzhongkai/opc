[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('Validate', 'Install', 'Repair', 'Rollback', 'Uninstall')]
    [string]$Action,

    [string]$BundleRoot = (Split-Path -Parent $PSScriptRoot),

    [Parameter(Mandatory)]
    [string]$InstallRoot,

    [Parameter(Mandatory)]
    [string]$StateRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-NormalizedPath {
    param([Parameter(Mandatory)][string]$LiteralPath)
    return [System.IO.Path]::GetFullPath($LiteralPath).TrimEnd('\')
}

function Assert-SafeExplicitRoot {
    param(
        [Parameter(Mandatory)][string]$LiteralPath,
        [Parameter(Mandatory)][string]$Description
    )
    $resolved = Get-NormalizedPath -LiteralPath $LiteralPath
    $root = [System.IO.Path]::GetPathRoot($resolved).TrimEnd('\')
    if ($resolved -eq $root -or [string]::IsNullOrWhiteSpace((Split-Path -Leaf $resolved))) {
        throw "$Description must not be a drive root: $resolved"
    }
    return $resolved
}

function Get-PathKey {
    param([Parameter(Mandatory)][string]$Value)
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($Value.ToUpperInvariant())
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try { return ([Convert]::ToHexString($sha.ComputeHash($bytes))).ToLowerInvariant() }
    finally { $sha.Dispose() }
}

function Read-Manifest {
    param([Parameter(Mandatory)][string]$PayloadRoot)
    $manifestPath = Join-Path $PayloadRoot 'manifest\payload-files.sha256.csv'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        throw "Payload manifest is missing: $manifestPath"
    }
    return @(Import-Csv -LiteralPath $manifestPath)
}

function Assert-Payload {
    param([Parameter(Mandatory)][string]$PayloadRoot)
    $resolvedPayload = Get-NormalizedPath -LiteralPath $PayloadRoot
    $manifest = Read-Manifest -PayloadRoot $resolvedPayload
    $expected = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    if ($manifest.Count -eq 0) {
        throw "Payload manifest is empty: $manifestPath"
    }
    foreach ($entry in $manifest) {
        $relative = $entry.RelativePath.Replace('/', '\')
        if ([System.IO.Path]::IsPathRooted($relative) -or $relative -match '(^|\\)\.\.(\\|$)') {
            throw "Manifest contains an unsafe relative path: $relative"
        }
        if (-not $expected.Add($relative)) {
            throw "Manifest contains a duplicate relative path: $relative"
        }
        $path = Join-Path $resolvedPayload $relative
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Manifest file is missing: $relative"
        }
        $file = Get-Item -LiteralPath $path
        if ($file.Length -ne [long]$entry.Length) {
            throw "Manifest length mismatch: $relative"
        }
        $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        if ($hash -ne $entry.SHA256) {
            throw "Manifest SHA-256 mismatch: $relative"
        }
    }
    foreach ($file in (Get-ChildItem -LiteralPath $resolvedPayload -Recurse -File)) {
        $relative = [System.IO.Path]::GetRelativePath($resolvedPayload, $file.FullName).Replace('/', '\')
        if ($relative -ieq 'manifest\payload-files.sha256.csv') {
            continue
        }
        if (-not $expected.Contains($relative)) {
            throw "Payload contains an unregistered file: $relative"
        }
    }
    $inputsPath = Join-Path $resolvedPayload 'manifest\build-inputs.json'
    $inputs = Get-Content -Raw -LiteralPath $inputsPath | ConvertFrom-Json
    if ($inputs.packageKind -ne 'unsigned-engineering' -or $inputs.candidateEligible -ne $false) {
        throw 'The transaction tool only accepts an unsigned, non-candidate engineering payload'
    }
    return [pscustomobject]@{
        Version = $inputs.version
        SourceCommit = $inputs.sourceCommit
        ManifestSha256 = (Get-FileHash -LiteralPath (Join-Path $resolvedPayload 'manifest\payload-files.sha256.csv') -Algorithm SHA256).Hash
        FileCount = $manifest.Count
    }
}

function Write-State {
    param([Parameter(Mandatory)][object]$State, [Parameter(Mandatory)][string]$StatePath)
    $parent = Split-Path -Parent $StatePath
    New-Item -ItemType Directory -Force -Path $parent | Out-Null
    $temporary = "$StatePath.$([guid]::NewGuid().ToString('N')).tmp"
    try {
        $State | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $temporary -Encoding utf8
        Move-Item -LiteralPath $temporary -Destination $StatePath -Force
    }
    finally {
        if (Test-Path -LiteralPath $temporary) {
            Remove-Item -LiteralPath $temporary -Force
        }
    }
}

function Read-State {
    param([Parameter(Mandatory)][string]$StatePath)
    if (-not (Test-Path -LiteralPath $StatePath -PathType Leaf)) {
        throw "Install state is missing: $StatePath"
    }
    return Get-Content -Raw -LiteralPath $StatePath | ConvertFrom-Json
}

$bundle = Assert-SafeExplicitRoot -LiteralPath $BundleRoot -Description 'BundleRoot'
$install = Assert-SafeExplicitRoot -LiteralPath $InstallRoot -Description 'InstallRoot'
$state = Assert-SafeExplicitRoot -LiteralPath $StateRoot -Description 'StateRoot'
$payload = Join-Path $bundle 'payload\SpaceRhythm'
if (-not (Test-Path -LiteralPath $payload -PathType Container)) {
    throw "Bundle payload is missing: $payload"
}

foreach ($pair in @(@($install, $state), @($install, $payload), @($state, $payload))) {
    $left = $pair[0].TrimEnd('\') + '\'
    $right = $pair[1].TrimEnd('\') + '\'
    if ($left.StartsWith($right, [System.StringComparison]::OrdinalIgnoreCase) -or
        $right.StartsWith($left, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Install, state and payload roots must not contain one another: $($pair[0]) ; $($pair[1])"
    }
}

$payloadInfo = Assert-Payload -PayloadRoot $payload
$installParent = Split-Path -Parent $install
$installLeaf = Split-Path -Leaf $install
if (-not (Test-Path -LiteralPath $installParent -PathType Container)) {
    New-Item -ItemType Directory -Force -Path $installParent | Out-Null
}
$statePath = Join-Path $state ("install-" + (Get-PathKey -Value $install) + '.json')

if ($Action -eq 'Validate') {
    if (Test-Path -LiteralPath $install -PathType Container) {
        [void](Assert-Payload -PayloadRoot $install)
        Write-Host "Installed payload validation passed: $install"
    }
    else {
        Write-Host "Bundle payload validation passed: $payload"
    }
    return
}

if ($Action -in @('Install', 'Repair')) {
    $previousState = $null
    $previousBackup = ''
    if (Test-Path -LiteralPath $statePath -PathType Leaf) {
        $previousState = Read-State -StatePath $statePath
        if ($previousState.installRoot -ine $install) {
            throw 'Existing install state does not match the explicit InstallRoot'
        }
        if (-not [string]::IsNullOrWhiteSpace($previousState.backupPath)) {
            $previousBackup = Get-NormalizedPath -LiteralPath $previousState.backupPath
            if ((Split-Path -Parent $previousBackup) -ine $installParent -or
                -not (Split-Path -Leaf $previousBackup).StartsWith(".$installLeaf.backup.", [System.StringComparison]::OrdinalIgnoreCase)) {
                throw "Registered backup path is outside the controlled install parent: $previousBackup"
            }
            if (Test-Path -LiteralPath $previousBackup) {
                [void](Assert-Payload -PayloadRoot $previousBackup)
            }
        }
    }
    if (Test-Path -LiteralPath $install -PathType Container) {
        if ($null -eq $previousState -or -not $previousState.installed) {
            throw 'Refusing to replace an installation that is not registered in the explicit StateRoot'
        }
        $installedInfo = Assert-Payload -PayloadRoot $install
        if ($previousState.payloadManifestSha256 -ne $installedInfo.ManifestSha256) {
            throw 'Installed payload does not match its registered manifest hash'
        }
    }
    elseif ($Action -eq 'Repair') {
        throw 'Repair requires an existing registered installation'
    }
    elseif ($null -ne $previousState -and $previousState.installed) {
        throw 'Install state reports an installation, but the explicit InstallRoot is missing'
    }

    $operationId = [guid]::NewGuid().ToString('N')
    $staging = Join-Path $installParent ".$installLeaf.staging.$operationId"
    $backup = Join-Path $installParent ".$installLeaf.backup.$operationId"
    $movedExisting = $false
    $installedNew = $false
    try {
        Copy-Item -LiteralPath $payload -Destination $staging -Recurse
        [void](Assert-Payload -PayloadRoot $staging)
        if (Test-Path -LiteralPath $install) {
            Move-Item -LiteralPath $install -Destination $backup
            $movedExisting = $true
        }
        Move-Item -LiteralPath $staging -Destination $install
        $installedNew = $true
        $newState = [ordered]@{
            schemaVersion = 1
            installed = $true
            installRoot = $install
            version = $payloadInfo.Version
            sourceCommit = $payloadInfo.SourceCommit
            payloadManifestSha256 = $payloadInfo.ManifestSha256
            backupPath = if ($movedExisting) { $backup } else { '' }
            lastAction = $Action.ToLowerInvariant()
            updatedUtc = (Get-Date).ToUniversalTime().ToString('o')
        }
        Write-State -State $newState -StatePath $statePath
        if (-not [string]::IsNullOrWhiteSpace($previousBackup) -and
            $previousBackup -ine $backup -and (Test-Path -LiteralPath $previousBackup)) {
            try {
                Remove-Item -LiteralPath $previousBackup -Recurse -Force
            }
            catch {
                Write-Warning "The new payload is committed, but the superseded validated backup could not be removed: $previousBackup"
            }
        }
    }
    catch {
        if ($installedNew -and (Test-Path -LiteralPath $install) -and
            -not (Test-Path -LiteralPath $staging)) {
            Move-Item -LiteralPath $install -Destination $staging
        }
        if ((-not (Test-Path -LiteralPath $install)) -and $movedExisting -and (Test-Path -LiteralPath $backup)) {
            Move-Item -LiteralPath $backup -Destination $install
        }
        if (Test-Path -LiteralPath $staging) {
            Remove-Item -LiteralPath $staging -Recurse -Force
        }
        throw
    }
    Write-Host "$Action completed: $install"
    return
}

if ($Action -eq 'Rollback') {
    $currentState = Read-State -StatePath $statePath
    if ($currentState.installRoot -ine $install) {
        throw 'Install state does not match the explicit InstallRoot'
    }
    if (-not $currentState.installed -or [string]::IsNullOrWhiteSpace($currentState.backupPath)) {
        throw 'No installed backup is registered for rollback'
    }
    $backup = Get-NormalizedPath -LiteralPath $currentState.backupPath
    if ((Split-Path -Parent $backup) -ine $installParent -or
        -not (Split-Path -Leaf $backup).StartsWith(".$installLeaf.backup.", [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Registered backup path is outside the controlled install parent: $backup"
    }
    $currentInfo = Assert-Payload -PayloadRoot $install
    if ($currentState.payloadManifestSha256 -ne $currentInfo.ManifestSha256) {
        throw 'Installed payload does not match its registered manifest hash'
    }
    [void](Assert-Payload -PayloadRoot $backup)
    $swap = Join-Path $installParent ".$installLeaf.backup.$([guid]::NewGuid().ToString('N'))"
    $movedCurrent = $false
    $movedBackup = $false
    try {
        Move-Item -LiteralPath $install -Destination $swap
        $movedCurrent = $true
        Move-Item -LiteralPath $backup -Destination $install
        $movedBackup = $true
        $restored = Assert-Payload -PayloadRoot $install
        $currentState.version = $restored.Version
        $currentState.sourceCommit = $restored.SourceCommit
        $currentState.payloadManifestSha256 = $restored.ManifestSha256
        $currentState.backupPath = $swap
        $currentState.lastAction = 'rollback'
        $currentState.updatedUtc = (Get-Date).ToUniversalTime().ToString('o')
        Write-State -State $currentState -StatePath $statePath
    }
    catch {
        if ($movedBackup -and (Test-Path -LiteralPath $install) -and
            -not (Test-Path -LiteralPath $backup)) {
            Move-Item -LiteralPath $install -Destination $backup
        }
        if ($movedCurrent -and (-not (Test-Path -LiteralPath $install)) -and (Test-Path -LiteralPath $swap)) {
            Move-Item -LiteralPath $swap -Destination $install
        }
        throw
    }
    Write-Host "Rollback completed: $install"
    return
}

if ($Action -eq 'Uninstall') {
    $currentState = Read-State -StatePath $statePath
    if ($currentState.installRoot -ine $install) {
        throw 'Install state does not match the explicit InstallRoot'
    }
    $installExists = Test-Path -LiteralPath $install -PathType Container
    if ($currentState.installed -and -not $installExists) {
        throw 'Install state reports an installation, but the explicit InstallRoot is missing'
    }
    if (-not $currentState.installed -and $installExists) {
        throw 'Install state reports uninstalled, but the explicit InstallRoot still exists'
    }
    if ($installExists) {
        $installedInfo = Assert-Payload -PayloadRoot $install
        if ($currentState.payloadManifestSha256 -ne $installedInfo.ManifestSha256) {
            throw 'Installed payload does not match its registered manifest hash'
        }
    }
    $backup = ''
    if (-not [string]::IsNullOrWhiteSpace($currentState.backupPath)) {
        $backup = Get-NormalizedPath -LiteralPath $currentState.backupPath
        if ((Split-Path -Parent $backup) -ine $installParent -or
            -not (Split-Path -Leaf $backup).StartsWith(".$installLeaf.backup.", [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Registered backup path is outside the controlled install parent: $backup"
        }
        if (Test-Path -LiteralPath $backup) {
            [void](Assert-Payload -PayloadRoot $backup)
        }
    }
    $operationId = [guid]::NewGuid().ToString('N')
    $installQuarantine = Join-Path $installParent ".$installLeaf.uninstall.$operationId"
    $backupQuarantine = Join-Path $installParent ".$installLeaf.uninstall-backup.$operationId"
    $movedInstall = $false
    $movedBackup = $false
    try {
        if ($installExists) {
            Move-Item -LiteralPath $install -Destination $installQuarantine
            $movedInstall = $true
        }
        if (-not [string]::IsNullOrWhiteSpace($backup) -and (Test-Path -LiteralPath $backup)) {
            Move-Item -LiteralPath $backup -Destination $backupQuarantine
            $movedBackup = $true
        }
        $currentState.installed = $false
        $currentState.backupPath = ''
        $currentState.lastAction = 'uninstall'
        $currentState.updatedUtc = (Get-Date).ToUniversalTime().ToString('o')
        Write-State -State $currentState -StatePath $statePath
    }
    catch {
        if ($movedBackup -and (Test-Path -LiteralPath $backupQuarantine) -and
            -not (Test-Path -LiteralPath $backup)) {
            Move-Item -LiteralPath $backupQuarantine -Destination $backup
        }
        if ($movedInstall -and (Test-Path -LiteralPath $installQuarantine) -and
            -not (Test-Path -LiteralPath $install)) {
            Move-Item -LiteralPath $installQuarantine -Destination $install
        }
        throw
    }
    if ($movedInstall) {
        Remove-Item -LiteralPath $installQuarantine -Recurse -Force
    }
    if ($movedBackup) {
        Remove-Item -LiteralPath $backupQuarantine -Recurse -Force
    }
    Write-Host "Uninstall completed; external user data was not touched: $install"
}
