param(
    [string]$ManifestPath = (Join-Path $PSScriptRoot "fixtures-v1.json")
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Assert-Manifest {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw "GOLDEN_VIDEO_MANIFEST=FAIL $Message"
    }
}

function Get-TextSha256 {
    param([string]$Value)

    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Value)
        return ([System.BitConverter]::ToString($sha.ComputeHash($bytes))).Replace("-", "").ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }
}

Assert-Manifest (Test-Path -LiteralPath $ManifestPath -PathType Leaf) "manifest_missing=$ManifestPath"
$manifest = Get-Content -Raw -LiteralPath $ManifestPath | ConvertFrom-Json

Assert-Manifest ($manifest.schemaVersion -eq 1) "schemaVersion_expected_1"
Assert-Manifest ($manifest.videoAnalysisContractVersion -ceq "0.1.0") "videoAnalysisContractVersion_expected_0.1.0"
Assert-Manifest ($manifest.coreContractVersion -ceq "0.1.0") "coreContractVersion_expected_0.1.0"
Assert-Manifest ($manifest.coreSchemaVersion -eq 1) "coreSchemaVersion_expected_1"
Assert-Manifest ($manifest.mediaContractVersion -ceq "1.0.0") "mediaContractVersion_expected_1.0.0"
Assert-Manifest ($manifest.mediaSchemaVersion -eq 2) "mediaSchemaVersion_expected_2"
Assert-Manifest ($manifest.manifestVersion -eq 1) "manifestVersion_expected_1"
Assert-Manifest (@($manifest.fixtures).Count -gt 0) "fixtures_empty"

$allowedKinds = @("shot", "motion_peak", "action_peak")
$requiredCoverage = @(
    "fast_cut",
    "slow_cut",
    "flash",
    "global_camera_motion",
    "local_action",
    "static",
    "slow_motion",
    "vfr",
    "compression_noise"
)
$fixtureIds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
$outputFiles = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
$covered = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)

foreach ($fixture in @($manifest.fixtures)) {
    Assert-Manifest (-not [string]::IsNullOrWhiteSpace([string]$fixture.id)) "fixture_id_empty"
    Assert-Manifest ($fixtureIds.Add([string]$fixture.id)) "duplicate_fixture_id=$($fixture.id)"
    Assert-Manifest (-not [string]::IsNullOrWhiteSpace([string]$fixture.outputFile)) "output_file_empty=$($fixture.id)"
    Assert-Manifest ($outputFiles.Add([string]$fixture.outputFile)) "duplicate_output_file=$($fixture.outputFile)"
    Assert-Manifest (-not [string]::IsNullOrWhiteSpace([string]$fixture.source)) "source_empty=$($fixture.id)"
    Assert-Manifest ($fixture.license -ceq "CC0-1.0") "license_not_cc0=$($fixture.id)"
    Assert-Manifest ([int64]$fixture.width -gt 0 -and [int64]$fixture.height -gt 0) "invalid_geometry=$($fixture.id)"
    Assert-Manifest ([int64]$fixture.durationNs -gt 0) "invalid_duration=$($fixture.id)"
    Assert-Manifest (-not [string]::IsNullOrWhiteSpace([string]$fixture.canonicalRecipe)) "recipe_empty=$($fixture.id)"
    Assert-Manifest ([string]$fixture.recipeSha256 -cmatch '^[0-9a-f]{64}$') "recipe_hash_shape=$($fixture.id)"
    Assert-Manifest ((Get-TextSha256 ([string]$fixture.canonicalRecipe)) -ceq [string]$fixture.recipeSha256) "recipe_hash_mismatch=$($fixture.id)"

    foreach ($category in @($fixture.categories)) {
        [void]$covered.Add([string]$category)
    }

    $eventIds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    $positiveKinds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($event in @($fixture.groundTruth)) {
        Assert-Manifest ($eventIds.Add([string]$event.id)) "duplicate_event_id=$($fixture.id)/$($event.id)"
        Assert-Manifest ($allowedKinds -ccontains [string]$event.kind) "unknown_event_kind=$($fixture.id)/$($event.kind)"
        [void]$positiveKinds.Add([string]$event.kind)
        Assert-Manifest ([int64]$event.timeNs -ge 0) "negative_event_time=$($fixture.id)/$($event.id)"
        Assert-Manifest ([int64]$event.durationNs -ge 0) "negative_event_duration=$($fixture.id)/$($event.id)"
        Assert-Manifest ([int64]$event.matchWindowBeforeNs -ge 0 -and [int64]$event.matchWindowAfterNs -ge 0) "negative_match_window=$($fixture.id)/$($event.id)"
        Assert-Manifest (([int64]$event.timeNs + [int64]$event.durationNs) -le [int64]$fixture.durationNs) "event_outside_fixture=$($fixture.id)/$($event.id)"
    }

    $negativeKinds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($kind in @($fixture.negativeKinds)) {
        Assert-Manifest ($allowedKinds -ccontains [string]$kind) "unknown_negative_kind=$($fixture.id)/$kind"
        Assert-Manifest ($negativeKinds.Add([string]$kind)) "duplicate_negative_kind=$($fixture.id)/$kind"
        Assert-Manifest (-not $positiveKinds.Contains([string]$kind)) "kind_is_positive_and_negative=$($fixture.id)/$kind"
    }

    $eventTokens = @(
        $fixture.groundTruth |
            Sort-Object @{ Expression = { [string]$_.kind } }, @{ Expression = { [int64]$_.timeNs } }, @{ Expression = { [string]$_.id } } |
            ForEach-Object {
                "{0}:{1}:{2}:{3}:{4}:{5}" -f $_.id, $_.kind, ([int64]$_.timeNs), ([int64]$_.durationNs), ([int64]$_.matchWindowBeforeNs), ([int64]$_.matchWindowAfterNs)
            }
    )
    $negativeTokens = @($fixture.negativeKinds | Sort-Object)
    $canonicalAnnotation = "v1|{0}|events={1}|negative={2}" -f $fixture.id, ($eventTokens -join ";"), ($negativeTokens -join ",")
    Assert-Manifest ($fixture.annotationCanonical -ceq $canonicalAnnotation) "annotation_canonical_mismatch=$($fixture.id)"
    Assert-Manifest ([string]$fixture.annotationSha256 -cmatch '^[0-9a-f]{64}$') "annotation_hash_shape=$($fixture.id)"
    Assert-Manifest ((Get-TextSha256 $canonicalAnnotation) -ceq [string]$fixture.annotationSha256) "annotation_hash_mismatch=$($fixture.id)"

    if ($fixture.mediaStatus -ceq "specified_not_generated") {
        Assert-Manifest ($null -eq $fixture.actualMediaSha256) "unexpected_media_hash=$($fixture.id)"
    }
    elseif ($fixture.mediaStatus -ceq "generated") {
        Assert-Manifest ([string]$fixture.actualMediaSha256 -cmatch '^[0-9a-f]{64}$') "generated_media_hash_invalid=$($fixture.id)"
    }
    else {
        Assert-Manifest $false "unknown_media_status=$($fixture.id)/$($fixture.mediaStatus)"
    }
}

foreach ($category in $requiredCoverage) {
    Assert-Manifest ($covered.Contains($category)) "required_coverage_missing=$category"
}

Write-Output ("GOLDEN_VIDEO_MANIFEST=PASS fixtures={0} coverage={1} contract={2}" -f @($manifest.fixtures).Count, $requiredCoverage.Count, $manifest.videoAnalysisContractVersion)
