[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ManifestPath,
    [Parameter(Mandatory)][string]$ActualEvidencePath,
    [Parameter(Mandatory)][string]$FixtureRoot,
    [Parameter(Mandatory)][string]$PolicyPath,
    [Parameter(Mandatory)][string]$LicensePath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-TextSha256 {
    param([Parameter(Mandatory)][string]$Text)
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $digest = [Convert]::ToHexString(
            $sha.ComputeHash([System.Text.Encoding]::UTF8.GetBytes($Text)))
        return $digest.ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }
}

$manifest = Get-Content -Raw -LiteralPath $ManifestPath | ConvertFrom-Json
$actual = Get-Content -Raw -LiteralPath $ActualEvidencePath | ConvertFrom-Json
$policy = Get-Content -Raw -LiteralPath $PolicyPath | ConvertFrom-Json
$licenseText = Get-Content -Raw -LiteralPath $LicensePath

if ($manifest.schemaVersion -ne 1 -or $actual.schemaVersion -ne 1 -or
    $policy.schemaVersion -ne 1) {
    throw 'Golden audit schema version mismatch.'
}
if ($manifest.mediaContractVersion -ne $policy.mediaContractVersion -or
    $actual.mediaContractVersion -ne $policy.mediaContractVersion) {
    throw 'Golden audit media contract version mismatch.'
}
if ($manifest.fixtures.Count -ne 13 -or $policy.fixtures.Count -ne 13 -or
    $actual.fixtures.Count -ne 13) {
    throw "Golden fixture cardinality mismatch: manifest=$($manifest.fixtures.Count), policy=$($policy.fixtures.Count), actual=$($actual.fixtures.Count)."
}
if ($licenseText -notmatch 'CC0-1\.0') {
    throw 'The fixture license file does not declare CC0-1.0.'
}
if ($manifest.licensePolicy -match 'package/' -and
    $manifest.licensePolicy -notmatch 'no package/') {
    throw 'The manifest does not explicitly exclude package/ inputs.'
}

$ids = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::Ordinal)
$timeVectorIds = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::Ordinal)
$timeVectorCount = 0

foreach ($fixture in $manifest.fixtures) {
    $id = [string]$fixture.id
    if (-not $ids.Add($id)) { throw "Duplicate fixture id: $id" }
    if ([string]::IsNullOrWhiteSpace([string]$fixture.source) -or
        [string]$fixture.source -match 'package[/\\]') {
        throw "Fixture $id has an invalid or package-derived source."
    }
    if ($fixture.license -ne 'CC0-1.0') {
        throw "Fixture $id has unexpected license: $($fixture.license)"
    }
    if ([string]::IsNullOrWhiteSpace([string]$fixture.canonicalRecipe)) {
        throw "Fixture $id has no canonical generation recipe."
    }
    $recipeHash = Get-TextSha256 -Text ([string]$fixture.canonicalRecipe)
    if ($recipeHash -cne [string]$fixture.recipeSha256) {
        throw "Fixture $id recipe SHA-256 mismatch."
    }

    $policyEntry = @($policy.fixtures | Where-Object fixtureId -CEQ $id)
    $actualEntry = @($actual.fixtures | Where-Object fixtureId -CEQ $id)
    if ($policyEntry.Count -ne 1 -or $actualEntry.Count -ne 1) {
        throw "Fixture $id does not have exactly one policy and actual-evidence entry."
    }
    if ($policyEntry[0].tolerance.mode -ne 'exact' -or
        [int]$policyEntry[0].tolerance.value -ne 0 -or
        [string]::IsNullOrWhiteSpace([string]$policyEntry[0].tolerance.basis)) {
        throw "Fixture $id lacks a zero-tolerance basis."
    }
    foreach ($expectation in $policyEntry[0].requiredExpectations) {
        if (-not $fixture.PSObject.Properties[[string]$expectation]) {
            throw "Fixture $id lacks required expectation $expectation."
        }
    }

    if ($actualEntry[0].outputFile -cne $fixture.outputFile -or
        $actualEntry[0].recipeSha256 -cne $fixture.recipeSha256) {
        throw "Fixture $id actual-evidence identity mismatch."
    }
    $mediaPath = Join-Path $FixtureRoot ([string]$fixture.outputFile)
    if (-not (Test-Path -LiteralPath $mediaPath -PathType Leaf)) {
        throw "Fixture $id output is missing: $mediaPath"
    }
    $mediaHash = (Get-FileHash -LiteralPath $mediaPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($mediaHash -cne [string]$actualEntry[0].mediaSha256) {
        throw "Fixture $id media SHA-256 mismatch."
    }

    foreach ($vector in $fixture.expectedTimeVectors) {
        if (-not $timeVectorIds.Add([string]$vector.id)) {
            throw "Duplicate golden time-vector id: $($vector.id)"
        }
        if ($null -eq $vector.expectedTimeNs -or
            [string]::IsNullOrWhiteSpace([string]$vector.rounding)) {
            throw "Fixture $id has an incomplete expected time vector."
        }
        $timeVectorCount++
    }
}

if ($timeVectorCount -ne 30) {
    throw "Expected 30 golden time vectors, found $timeVectorCount."
}

Write-Output "T021_GOLDEN_AUDIT=PASS fixtures=$($ids.Count) timeVectors=$timeVectorCount tolerance=exact-zero contract=$($policy.mediaContractVersion)"
