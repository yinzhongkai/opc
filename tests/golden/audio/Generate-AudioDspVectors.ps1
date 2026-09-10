[CmdletBinding()]
param(
    [string]$ManifestPath = (Join-Path $PSScriptRoot 'fixtures-v1.json'),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot 'generated'),
    [switch]$ValidateOnly,
    [switch]$BootstrapHashes
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not [BitConverter]::IsLittleEndian) {
    throw 'Audio DSP vectors v1 require a little-endian host.'
}

function Get-Sha256Hex {
    param([byte[]]$Bytes)

    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        return [Convert]::ToHexString($sha.ComputeHash($Bytes)).ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }
}

function Get-Utf8Sha256Hex {
    param([string]$Text)
    return Get-Sha256Hex ([Text.Encoding]::UTF8.GetBytes($Text))
}

function Convert-Q23ToSingle {
    param([long]$Value)

    if ($Value -lt -8388608 -or $Value -gt 8388608) {
        throw "Q23 sample out of range: $Value"
    }
    return [single]($Value / 8388608.0)
}

function Get-TruncatedQuotient {
    param([long]$Numerator, [long]$Denominator)

    if ($Denominator -le 0) {
        throw 'Denominator must be positive.'
    }
    if ($Numerator -ge 0) {
        return [long][Math]::Floor($Numerator / [double]$Denominator)
    }
    return [long][Math]::Ceiling($Numerator / [double]$Denominator)
}

function Get-ExactRoundedQuotient {
    param(
        [System.Numerics.BigInteger]$Numerator,
        [System.Numerics.BigInteger]$Denominator,
        [ValidateSet('floor', 'ceil', 'nearest_ties_to_even')]
        [string]$Mode
    )

    if ($Denominator -le [System.Numerics.BigInteger]::Zero) {
        throw 'Exact rounding denominator must be positive.'
    }
    $quotient = [System.Numerics.BigInteger]::Divide($Numerator, $Denominator)
    $remainder = [System.Numerics.BigInteger]::Remainder($Numerator, $Denominator)
    if ($remainder -lt [System.Numerics.BigInteger]::Zero) {
        $quotient -= [System.Numerics.BigInteger]::One
        $remainder += $Denominator
    }

    switch ($Mode) {
        'floor' { return $quotient }
        'ceil' {
            if ($remainder -eq [System.Numerics.BigInteger]::Zero) { return $quotient }
            return $quotient + [System.Numerics.BigInteger]::One
        }
        'nearest_ties_to_even' {
            $twiceRemainder = $remainder * 2
            if ($twiceRemainder -lt $Denominator) { return $quotient }
            if ($twiceRemainder -gt $Denominator) {
                return $quotient + [System.Numerics.BigInteger]::One
            }
            if ($quotient.IsEven) { return $quotient }
            return $quotient + [System.Numerics.BigInteger]::One
        }
    }
}

function Get-FixtureTimeNs {
    param(
        [pscustomobject]$Fixture,
        [long]$SampleIndex,
        [ValidateSet('floor', 'ceil', 'nearest_ties_to_even')]
        [string]$Mode
    )

    $delta = [System.Numerics.BigInteger]$SampleIndex -
        [System.Numerics.BigInteger][long]$Fixture.segmentOriginSampleIndex
    $numerator = $delta * [System.Numerics.BigInteger]1000000000
    $offset = Get-ExactRoundedQuotient $numerator ([System.Numerics.BigInteger][long]$Fixture.sampleRate) $Mode
    $result = [System.Numerics.BigInteger][long]$Fixture.segmentOriginTimeNs + $offset
    if ($result -lt [System.Numerics.BigInteger][long]::MinValue -or
        $result -gt [System.Numerics.BigInteger][long]::MaxValue) {
        throw "TimeNs overflow for $($Fixture.id) at sample $SampleIndex."
    }
    return [long]$result
}

function Assert-LongEqual {
    param([string]$Label, [long]$Expected, [long]$Actual)
    if ($Expected -ne $Actual) {
        throw "$Label mismatch: expected $Expected, actual $Actual."
    }
}

function Test-FixtureTimeOracles {
    param([pscustomobject]$Fixture)

    $properties = $Fixture.expected.PSObject.Properties
    $first = [long]$Fixture.firstSampleIndex
    $end = [long]$Fixture.firstSampleIndex + [long]$Fixture.frameCount
    if ($properties['coverageStartNsFloor']) {
        Assert-LongEqual "$($Fixture.id) coverage start" ([long]$Fixture.expected.coverageStartNsFloor) `
            (Get-FixtureTimeNs $Fixture $first 'floor')
    }
    if ($properties['coverageEndNsCeil']) {
        Assert-LongEqual "$($Fixture.id) coverage end" ([long]$Fixture.expected.coverageEndNsCeil) `
            (Get-FixtureTimeNs $Fixture $end 'ceil')
    }
    if ($properties['firstSampleTimeNsNearestEven']) {
        Assert-LongEqual "$($Fixture.id) first sample time" ([long]$Fixture.expected.firstSampleTimeNsNearestEven) `
            (Get-FixtureTimeNs $Fixture $first 'nearest_ties_to_even')
    }
    if ($properties['sampleOneTimeNsNearestEven']) {
        Assert-LongEqual "$($Fixture.id) sample one time" ([long]$Fixture.expected.sampleOneTimeNsNearestEven) `
            (Get-FixtureTimeNs $Fixture 1 'nearest_ties_to_even')
    }

    $indexProperties = @(
        @('impulseSampleIndices', 'sampleTimesNsNearestEven'),
        @('beatSampleIndices', 'beatTimesNsNearestEven')
    )
    foreach ($pair in $indexProperties) {
        if (-not $properties[$pair[0]] -or -not $properties[$pair[1]]) { continue }
        $indices = @($Fixture.expected.($pair[0]))
        $times = @($Fixture.expected.($pair[1]))
        if ($indices.Count -ne $times.Count) {
            throw "$($Fixture.id) time oracle array length mismatch."
        }
        for ($index = 0; $index -lt $indices.Count; ++$index) {
            Assert-LongEqual "$($Fixture.id) sample time[$index]" ([long]$times[$index]) `
                (Get-FixtureTimeNs $Fixture ([long]$indices[$index]) 'nearest_ties_to_even')
        }
    }
}

function New-LcgState {
    param([uint32]$State)
    return [uint32](([uint64]1664525 * [uint64]$State + [uint64]1013904223) -band [uint64]4294967295)
}

function New-FixtureBytes {
    param([pscustomobject]$Fixture)

    $frameCount = [long]$Fixture.frameCount
    $channels = [int]$Fixture.channels
    if ($frameCount -le 0 -or $channels -le 0) {
        throw "Invalid frame/channel count for $($Fixture.id)."
    }

    $capacity = [long]$frameCount * $channels * 4
    if ($capacity -gt [int]::MaxValue) {
        throw "Fixture is too large for the v1 generator: $($Fixture.id)."
    }

    $stream = [IO.MemoryStream]::new([int]$capacity)
    $writer = [IO.BinaryWriter]::new($stream, [Text.Encoding]::UTF8, $true)
    try {
        $generatorId = [string]$Fixture.generation.id
        switch ($generatorId) {
            'impulses_q23_v1' {
                $indices = [Collections.Generic.HashSet[long]]::new()
                foreach ($index in $Fixture.generation.parameters.sampleIndices) {
                    [void]$indices.Add([long]$index)
                }
                $amplitude = [long]$Fixture.generation.parameters.amplitudeQ23
                for ($frame = 0L; $frame -lt $frameCount; ++$frame) {
                    $sample = if ($indices.Contains($frame)) { $amplitude } else { 0L }
                    $writer.Write((Convert-Q23ToSingle $sample))
                }
            }
            'silence_q23_v1' {
                for ($sample = 0L; $sample -lt $frameCount * $channels; ++$sample) {
                    $writer.Write([single]0.0)
                }
            }
            'lcg_noise_q23_v1' {
                $state = [uint32]$Fixture.generation.parameters.seed
                $shift = [int]$Fixture.generation.parameters.attenuationShift
                for ($sample = 0L; $sample -lt $frameCount * $channels; ++$sample) {
                    $state = New-LcgState $state
                    $raw = [long](($state -shr 8) -band 0x00ffffff) - 8388608L
                    $attenuated = Get-TruncatedQuotient $raw ([long]1 -shl $shift)
                    $writer.Write((Convert-Q23ToSingle $attenuated))
                }
            }
            'boundary_stereo_q23_v1' {
                $left = @(-8388608L, -4194304L, -1L, 0L, 1L, 4194304L, 8388607L, 8388608L,
                          8388608L, 8388607L, 4194304L, 1L, 0L, -1L, -4194304L, -8388608L)
                $right = @($left[15..0])
                if ($channels -ne 2 -or $frameCount -ne 16) {
                    throw 'boundary_stereo_q23_v1 requires 16 stereo frames.'
                }
                for ($frame = 0; $frame -lt 16; ++$frame) {
                    $writer.Write((Convert-Q23ToSingle $left[$frame]))
                    $writer.Write((Convert-Q23ToSingle $right[$frame]))
                }
            }
            'non_finite_f32_v1' {
                if ($channels -ne 1 -or $frameCount -ne 4) {
                    throw 'non_finite_f32_v1 requires four mono frames.'
                }
                $writer.Write([single]0.0)
                $writer.Write([single]::NaN)
                $writer.Write([single]::PositiveInfinity)
                $writer.Write([single]::NegativeInfinity)
            }
            'decaying_click_q23_v1' {
                $peak = [long]$Fixture.generation.parameters.peakQ23
                $halfPeriod = [long]$Fixture.generation.parameters.halfPeriodFrames
                for ($frame = 0L; $frame -lt $frameCount; ++$frame) {
                    $envelope = Get-TruncatedQuotient (($frameCount - $frame) * $peak) $frameCount
                    $positive = ((Get-TruncatedQuotient $frame $halfPeriod) % 2) -eq 0
                    $sample = if ($positive) { $envelope } else { -$envelope }
                    $writer.Write((Convert-Q23ToSingle $sample))
                }
            }
            'decaying_triangle_q23_v1' {
                $peak = [long]$Fixture.generation.parameters.peakQ23
                $period = [long]$Fixture.generation.parameters.periodFrames
                $quarter = Get-TruncatedQuotient $period 4
                for ($frame = 0L; $frame -lt $frameCount; ++$frame) {
                    $phase = $frame % $period
                    if ($phase -lt $quarter) {
                        $wave = Get-TruncatedQuotient ($phase * 8388608L) $quarter
                    }
                    elseif ($phase -lt 3 * $quarter) {
                        $wave = 8388608L - (Get-TruncatedQuotient (($phase - $quarter) * 16777216L) (2 * $quarter))
                    }
                    else {
                        $wave = -8388608L + (Get-TruncatedQuotient (($phase - 3 * $quarter) * 8388608L) $quarter)
                    }
                    $scaled = Get-TruncatedQuotient ($wave * $peak) 8388608L
                    $sample = Get-TruncatedQuotient ($scaled * ($frameCount - $frame)) $frameCount
                    $writer.Write((Convert-Q23ToSingle $sample))
                }
            }
            'decaying_noise_q23_v1' {
                $state = [uint32]$Fixture.generation.parameters.seed
                $peak = [long]$Fixture.generation.parameters.peakQ23
                for ($frame = 0L; $frame -lt $frameCount; ++$frame) {
                    $state = New-LcgState $state
                    $raw = [long](($state -shr 8) -band 0x00ffffff) - 8388608L
                    $scaled = Get-TruncatedQuotient ($raw * $peak) 8388608L
                    $sample = Get-TruncatedQuotient ($scaled * ($frameCount - $frame)) $frameCount
                    $writer.Write((Convert-Q23ToSingle $sample))
                }
            }
            default {
                throw "Unknown audio DSP fixture generator: $generatorId"
            }
        }
        $writer.Flush()
        if ($stream.Length -ne $capacity) {
            throw "Generated byte length mismatch for $($Fixture.id): $($stream.Length) != $capacity"
        }
        return $stream.ToArray()
    }
    finally {
        $writer.Dispose()
        $stream.Dispose()
    }
}

$manifestFullPath = [IO.Path]::GetFullPath($ManifestPath)
$outputFullPath = [IO.Path]::GetFullPath($OutputDirectory)
if ($outputFullPath -match '(?i)(^|[\\/])package([\\/]|$)' -or
    $outputFullPath -match '(?i)(^|[\\/])scripts[\\/]__pycache__([\\/]|$)') {
    throw 'Audio DSP vectors must not be written to package/ or scripts/__pycache__/.'
}

$manifest = Get-Content -Raw -LiteralPath $manifestFullPath | ConvertFrom-Json
if ($manifest.schemaVersion -ne 1 -or
    $manifest.dspContractVersion -ne '0.1.0' -or
    $manifest.vectorSetVersion -ne 1 -or
    $manifest.timbreManifestVersion -ne 1) {
    throw 'Audio DSP manifest version mismatch.'
}
$oracleFile = [string]$manifest.algorithmOracle.file
if ([IO.Path]::GetFileName($oracleFile) -cne $oracleFile) {
    throw 'Audio DSP algorithm oracle file must be a basename.'
}
$oraclePath = Join-Path ([IO.Path]::GetDirectoryName($manifestFullPath)) $oracleFile
$oracle = Get-Content -Raw -LiteralPath $oraclePath | ConvertFrom-Json
if ($oracle.schemaVersion -ne 1 -or
    $oracle.oracleSetVersion -ne $manifest.algorithmOracle.oracleSetVersion -or
    $oracle.vectorSetVersion -ne $manifest.vectorSetVersion -or
    $oracle.producer.algorithmId -cne $manifest.algorithmOracle.algorithmId -or
    $oracle.producer.algorithmVersion -cne $manifest.algorithmOracle.algorithmVersion -or
    $oracle.producer.parameterSetVersion -cne $manifest.algorithmOracle.parameterSetVersion -or
    $oracle.producer.backendId -cne $manifest.algorithmOracle.backendId -or
    $oracle.producer.backendVersion -cne $manifest.algorithmOracle.backendVersion) {
    throw 'Audio DSP algorithm oracle metadata mismatch.'
}

$fixtureIds = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$outputFiles = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$results = [Collections.Generic.List[object]]::new()

foreach ($fixture in $manifest.fixtures) {
    if (-not $fixtureIds.Add([string]$fixture.id)) {
        throw "Duplicate audio DSP fixture id: $($fixture.id)"
    }
    if (-not $outputFiles.Add([string]$fixture.outputFile)) {
        throw "Duplicate audio DSP output file: $($fixture.outputFile)"
    }
    if ([IO.Path]::GetFileName([string]$fixture.outputFile) -cne [string]$fixture.outputFile) {
        throw "Audio DSP output file must be a basename: $($fixture.outputFile)"
    }
    if ($fixture.source -ne 'Project-generated deterministic PCM' -or
        $fixture.license -ne 'CC0-1.0' -or
        $fixture.sampleFormat -ne 'f32_le' -or
        -not $fixture.interleaved) {
        throw "Fixture policy mismatch: $($fixture.id)"
    }
    if ($fixture.channelOrder.Count -ne $fixture.channels) {
        throw "Channel order mismatch: $($fixture.id)"
    }
    if ($fixture.sampleRate -le 0 -or $fixture.frameCount -le 0) {
        throw "Invalid sample rate or frame count: $($fixture.id)"
    }
    if ($fixture.resampling.performed -or
        $fixture.resampling.inputSampleRate -ne $fixture.sampleRate -or
        $fixture.resampling.outputSampleRate -ne $fixture.sampleRate -or
        $fixture.resampling.delayInputFramesNumerator -ne 0 -or
        $fixture.resampling.delayInputFramesDenominator -ne 1 -or
        -not $fixture.resampling.delayAccountedInFirstSampleIndex -or
        $fixture.resampling.emittedFromDrain) {
        throw "Identity resampling declaration mismatch: $($fixture.id)"
    }

    $recipeHash = Get-Utf8Sha256Hex ([string]$fixture.canonicalRecipe)
    $bytes = New-FixtureBytes $fixture
    $pcmHash = Get-Sha256Hex $bytes
    $expectedBytes = [long]$fixture.frameCount * [long]$fixture.channels * 4L
    if ($bytes.LongLength -ne $expectedBytes) {
        throw "PCM byte length mismatch: $($fixture.id)"
    }
    Test-FixtureTimeOracles $fixture

    if (-not $BootstrapHashes) {
        if ($fixture.recipeSha256 -cne $recipeHash) {
            throw "Recipe hash mismatch for $($fixture.id): $recipeHash"
        }
        if ($fixture.pcmSha256 -cne $pcmHash) {
            throw "PCM hash mismatch for $($fixture.id): $pcmHash"
        }
    }

    $results.Add([ordered]@{
        id = [string]$fixture.id
        outputFile = [string]$fixture.outputFile
        byteLength = $bytes.LongLength
        frameCount = [long]$fixture.frameCount
        recipeSha256 = $recipeHash
        pcmSha256 = $pcmHash
    })

    if (-not $ValidateOnly) {
        [void][IO.Directory]::CreateDirectory($outputFullPath)
        [IO.File]::WriteAllBytes((Join-Path $outputFullPath $fixture.outputFile), $bytes)
    }
}

if ($fixtureIds.Count -ne 10) {
    throw "Expected 10 audio DSP fixtures, found $($fixtureIds.Count)."
}

$expectedIds = @(
    'AV-IMPULSE-001',
    'AV-FIXED-BEAT-120-001',
    'AV-TEMPO-CHANGE-001',
    'AV-SILENCE-44100-001',
    'AV-NOISE-001',
    'AV-BOUNDARY-001',
    'AV-NONFINITE-001',
    'AT-CLICK-001',
    'AT-LOW-PULSE-001',
    'AT-NOISE-HIT-001'
)
foreach ($expectedId in $expectedIds) {
    if (-not $fixtureIds.Contains($expectedId)) {
        throw "Missing required audio DSP fixture id: $expectedId"
    }
}
$oracleIds = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($fixtureOracle in $oracle.fixtures) {
    $oracleId = [string]$fixtureOracle.fixtureId
    if (-not $oracleIds.Add($oracleId)) {
        throw "Duplicate algorithm oracle fixture id: $oracleId"
    }
    $manifestFixtures = @($manifest.fixtures | Where-Object { $_.id -ceq $oracleId })
    if ($manifestFixtures.Count -ne 1 -or
        $manifestFixtures[0].role -notin @('analysis_input', 'negative_contract_input') -or
        $manifestFixtures[0].pcmSha256 -cne $fixtureOracle.pcmSha256) {
        throw "Algorithm oracle input mismatch: $oracleId"
    }
}
if ($oracleIds.Count -ne 7) {
    throw "Expected seven A-018 analysis algorithm oracles, found $($oracleIds.Count)."
}
foreach ($scenario in $oracle.additionalDeterministicScenarios) {
    if ($scenario.PSObject.Properties['source'] -and
        ($scenario.source -cne 'Project-generated deterministic PCM' -or
         $scenario.license -cne 'CC0-1.0')) {
        throw "Algorithm scenario source/license mismatch: $($scenario.id)"
    }
}
$testTimbreCount = @($manifest.fixtures | Where-Object { $_.role -ceq 'test_timbre' }).Count
if ($testTimbreCount -ne 3) {
    throw "Expected three registered test timbres, found $testTimbreCount."
}

$halfDown = Get-ExactRoundedQuotient ([System.Numerics.BigInteger]1000000000) ([System.Numerics.BigInteger]1024) 'nearest_ties_to_even'
$halfUp = Get-ExactRoundedQuotient ([System.Numerics.BigInteger]3000000000) ([System.Numerics.BigInteger]1024) 'nearest_ties_to_even'
if ($halfDown -ne 976562 -or $halfUp -ne 2929688) {
    throw 'nearest_ties_to_even half-case self-test failed.'
}

if ($BootstrapHashes) {
    foreach ($result in $results) {
        Write-Output ("{0}`trecipe={1}`tpcm={2}`tbytes={3}" -f
            $result.id, $result.recipeSha256, $result.pcmSha256, $result.byteLength)
    }
    exit 0
}

if (-not $ValidateOnly) {
    $evidence = [ordered]@{
        schemaVersion = 1
        dspContractVersion = '0.1.0'
        vectorSetVersion = 1
        timbreManifestVersion = 1
        generatorId = 'space-rhythm-audio-dsp-vector-generator'
        generatorVersion = '1.0.0'
        fixtures = $results
    }
    $evidencePath = Join-Path $outputFullPath 'actual-hashes-v1.json'
    $evidence | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $evidencePath -Encoding utf8
}

Write-Output ("AUDIO_DSP_VECTORS=PASS fixtures={0} mode={1} contract={2}" -f
    $fixtureIds.Count,
    $(if ($ValidateOnly) { 'validate-only' } else { 'generate' }),
    $manifest.dspContractVersion)
