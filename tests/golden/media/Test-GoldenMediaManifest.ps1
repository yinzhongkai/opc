[CmdletBinding()]
param(
    [Parameter()]
    [string]$ManifestPath = (Join-Path $PSScriptRoot 'fixtures-v1.json')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-Sha256Text {
    param([Parameter(Mandatory)][string]$Text)

    $algorithm = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
        return [Convert]::ToHexString($algorithm.ComputeHash($bytes)).ToLowerInvariant()
    }
    finally {
        $algorithm.Dispose()
    }
}

function Get-RoundedQuotient {
    param(
        [Parameter(Mandatory)][System.Numerics.BigInteger]$Numerator,
        [Parameter(Mandatory)][System.Numerics.BigInteger]$Denominator,
        [Parameter(Mandatory)][string]$Mode
    )

    if ($Denominator -le 0) {
        throw 'Denominator must be positive.'
    }

    $quotient = [System.Numerics.BigInteger]::Divide($Numerator, $Denominator)
    $remainder = [System.Numerics.BigInteger]::Remainder($Numerator, $Denominator)
    if ($remainder -eq 0) {
        return $quotient
    }

    switch ($Mode) {
        'toward_zero' { return $quotient }
        'floor' {
            if ($Numerator -lt 0) { return $quotient - 1 }
            return $quotient
        }
        'ceil' {
            if ($Numerator -gt 0) { return $quotient + 1 }
            return $quotient
        }
        'nearest_ties_to_even' {
            $twiceAbsRemainder = [System.Numerics.BigInteger]::Abs($remainder) * 2
            if ($twiceAbsRemainder -lt $Denominator) { return $quotient }
            $step = if ($Numerator -lt 0) { [System.Numerics.BigInteger](-1) } else { [System.Numerics.BigInteger]1 }
            if ($twiceAbsRemainder -gt $Denominator) { return $quotient + $step }
            if (([System.Numerics.BigInteger]::Abs($quotient) % 2) -eq 0) { return $quotient }
            return $quotient + $step
        }
        default { throw "Unknown rounding mode: $Mode" }
    }
}

$resolvedManifest = (Resolve-Path -LiteralPath $ManifestPath).Path
$manifest = Get-Content -Raw -LiteralPath $resolvedManifest | ConvertFrom-Json

if ($manifest.schemaVersion -ne 1) { throw "Unsupported manifest schemaVersion: $($manifest.schemaVersion)" }
if ($manifest.mediaContractVersion -ne '0.1.0') { throw "Unexpected media contract version: $($manifest.mediaContractVersion)" }
if ($manifest.vectorSetVersion -ne 1) { throw "Unexpected vector set version: $($manifest.vectorSetVersion)" }

$fixtureIds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
$outputFiles = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$vectorIds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
$vectorCount = 0

foreach ($fixture in $manifest.fixtures) {
    if (-not $fixtureIds.Add([string]$fixture.id)) { throw "Duplicate fixture id: $($fixture.id)" }
    if (-not $outputFiles.Add([string]$fixture.outputFile)) { throw "Duplicate fixture output: $($fixture.outputFile)" }
    if ($fixture.license -ne 'CC0-1.0') { throw "Fixture $($fixture.id) has an unapproved license: $($fixture.license)" }
    if ([string]::IsNullOrWhiteSpace([string]$fixture.source)) { throw "Fixture $($fixture.id) has no source declaration." }

    $actualRecipeHash = Get-Sha256Text -Text ([string]$fixture.canonicalRecipe)
    if ($actualRecipeHash -cne [string]$fixture.recipeSha256) {
        throw "Fixture $($fixture.id) recipe hash mismatch: expected $($fixture.recipeSha256), actual $actualRecipeHash"
    }

    foreach ($vector in $fixture.expectedTimeVectors) {
        if (-not $vectorIds.Add([string]$vector.id)) { throw "Duplicate time vector id: $($vector.id)" }
        if ($vector.timeBaseNumerator -le 0 -or $vector.timeBaseDenominator -le 0) {
            throw "Vector $($vector.id) has an invalid time base."
        }

        [System.Numerics.BigInteger]$relativeTicks = [long]$vector.ticks - [long]$vector.originTicks
        [System.Numerics.BigInteger]$numerator = $relativeTicks * [long]$vector.timeBaseNumerator * 1000000000
        [System.Numerics.BigInteger]$denominator = [long]$vector.timeBaseDenominator
        $actualTimeNs = Get-RoundedQuotient -Numerator $numerator -Denominator $denominator -Mode ([string]$vector.rounding)
        if ($actualTimeNs -ne [System.Numerics.BigInteger]([long]$vector.expectedTimeNs)) {
            throw "Vector $($vector.id) mismatch: expected $($vector.expectedTimeNs), actual $actualTimeNs"
        }
        $vectorCount++
    }
}

Write-Output "GOLDEN_MEDIA_MANIFEST=PASS fixtures=$($fixtureIds.Count) timeVectors=$vectorCount contract=$($manifest.mediaContractVersion)"
