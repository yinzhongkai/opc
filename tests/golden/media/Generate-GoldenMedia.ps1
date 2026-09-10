[CmdletBinding()]
param(
    [Parameter()]
    [string]$FfmpegPath = 'ffmpeg',

    [Parameter()]
    [string]$FfprobePath = 'ffprobe',

    [Parameter()]
    [string]$OutputDirectory = (Join-Path $PSScriptRoot 'generated'),

    [Parameter()]
    [switch]$ValidateOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$manifestPath = Join-Path $PSScriptRoot 'fixtures-v1.json'
& (Join-Path $PSScriptRoot 'Test-GoldenMediaManifest.ps1') -ManifestPath $manifestPath
if ($ValidateOnly) {
    Write-Output 'GOLDEN_MEDIA_GENERATION=SKIPPED_VALIDATE_ONLY'
    return
}

$ffmpeg = (Get-Command $FfmpegPath -ErrorAction Stop).Source
$ffprobe = (Get-Command $FfprobePath -ErrorAction Stop).Source
$outputRoot = [System.IO.Path]::GetFullPath($OutputDirectory)
$allowedRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'generated'))
$allowedPrefix = $allowedRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
if (-not $outputRoot.Equals($allowedRoot, [System.StringComparison]::OrdinalIgnoreCase) -and
    -not $outputRoot.StartsWith($allowedPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputDirectory must remain inside $allowedRoot"
}
[System.IO.Directory]::CreateDirectory($outputRoot) | Out-Null

function Invoke-Ffmpeg {
    param(
        [Parameter(Mandatory)][string[]]$Arguments,
        [Parameter(Mandatory)][string]$OutputFile
    )

    $extension = [System.IO.Path]::GetExtension($OutputFile)
    $partial = Join-Path $outputRoot (([System.IO.Path]::GetFileNameWithoutExtension($OutputFile)) + '.partial' + $extension)
    $final = Join-Path $outputRoot $OutputFile
    if (Test-Path -LiteralPath $partial) { Remove-Item -LiteralPath $partial -Force }
    & $ffmpeg -y -hide_banner -loglevel error @Arguments $partial
    if ($LASTEXITCODE -ne 0) { throw "ffmpeg failed while producing $OutputFile" }
    Move-Item -LiteralPath $partial -Destination $final -Force
}

$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
foreach ($fixture in $manifest.fixtures) {
    switch ([string]$fixture.id) {
        'GM-CFR-001' {
            Invoke-Ffmpeg -OutputFile $fixture.outputFile -Arguments @(
                '-f','lavfi','-i','testsrc2=size=16x16:rate=25:duration=0.2',
                '-f','lavfi','-i','anullsrc=r=48000:cl=mono',
                '-t','0.2','-map','0:v:0','-map','1:a:0','-c:v','ffv1','-level','3','-c:a','pcm_s16le',
                '-fflags','+bitexact','-flags:v','+bitexact','-flags:a','+bitexact','-metadata','creation_time=1970-01-01T00:00:00Z'
            )
        }
        'GM-VFR-001' {
            $filter = 'settb=expr=1/1000,setpts=if(eq(N\,0)\,0\,if(eq(N\,1)\,40\,if(eq(N\,2)\,100\,if(eq(N\,3)\,140\,240))))'
            Invoke-Ffmpeg -OutputFile $fixture.outputFile -Arguments @(
                '-f','lavfi','-i','testsrc2=size=16x16:rate=25:duration=0.2','-vf',$filter,
                '-fps_mode','vfr','-enc_time_base','1:1000','-c:v','ffv1','-level','3','-fflags','+bitexact','-flags:v','+bitexact'
            )
        }
        'GM-ROT-SAR-001' {
            $baseName = 'rotated_sar_base.mkv'
            Invoke-Ffmpeg -OutputFile $baseName -Arguments @(
                '-f','lavfi','-i','testsrc2=size=16x8:rate=2:duration=1',
                '-vf','setsar=4/3,setparams=range=limited:color_primaries=bt709:color_trc=bt709:colorspace=bt709',
                '-c:v','ffv1','-level','3','-pix_fmt','yuv420p','-fflags','+bitexact','-flags:v','+bitexact'
            )
            Invoke-Ffmpeg -OutputFile $fixture.outputFile -Arguments @(
                '-display_rotation','-90','-i',(Join-Path $outputRoot $baseName),
                '-map','0:v:0','-c','copy','-fflags','+bitexact'
            )
            Remove-Item -LiteralPath (Join-Path $outputRoot $baseName) -Force
        }
        'GM-MULTI-001' {
            Invoke-Ffmpeg -OutputFile $fixture.outputFile -Arguments @(
                '-f','lavfi','-i','testsrc2=size=16x16:rate=25:duration=0.2',
                '-f','lavfi','-i','anullsrc=r=48000:cl=mono',
                '-f','lavfi','-i','anullsrc=r=44100:cl=mono','-t','0.2',
                '-map','0:v:0','-map','1:a:0','-map','2:a:0','-c:v','ffv1','-level','3','-c:a','pcm_s16le',
                '-metadata:s:a:0','language=eng','-metadata:s:a:1','language=jpn','-disposition:a:0','default','-disposition:a:1','0',
                '-fflags','+bitexact','-flags:v','+bitexact','-flags:a','+bitexact'
            )
        }
        'GM-AUDIO-44100-001' {
            Invoke-Ffmpeg -OutputFile $fixture.outputFile -Arguments @(
                '-f','lavfi','-i','anullsrc=r=44100:cl=mono','-t','0.1','-c:a','pcm_s16le','-fflags','+bitexact','-flags:a','+bitexact'
            )
        }
        'GM-AUDIO-48000-001' {
            Invoke-Ffmpeg -OutputFile $fixture.outputFile -Arguments @(
                '-f','lavfi','-i','anullsrc=r=48000:cl=mono','-t','0.1','-c:a','pcm_s16le','-fflags','+bitexact','-flags:a','+bitexact'
            )
        }
        'GM-CORRUPT-001' {
            $bytes = [Convert]::FromHexString('1A45DFA34286810142F7810142F2810442F381084282846D6174726F736B61')
            [System.IO.File]::WriteAllBytes((Join-Path $outputRoot $fixture.outputFile), $bytes[0..26])
        }
        'GM-MISSING-VIDEO-001' {
            Invoke-Ffmpeg -OutputFile $fixture.outputFile -Arguments @(
                '-f','lavfi','-i','anullsrc=r=48000:cl=mono','-t','0.1','-map','0:a:0','-c:a','pcm_s16le','-fflags','+bitexact','-flags:a','+bitexact'
            )
        }
        'GM-MISSING-AUDIO-001' {
            Invoke-Ffmpeg -OutputFile $fixture.outputFile -Arguments @(
                '-f','lavfi','-i','testsrc2=size=16x16:rate=25:duration=0.2','-map','0:v:0','-c:v','ffv1','-level','3','-fflags','+bitexact','-flags:v','+bitexact'
            )
        }
        'GM-NEG-START-001' {
            Invoke-Ffmpeg -OutputFile $fixture.outputFile -Arguments @(
                '-copyts','-f','lavfi','-i','testsrc2=size=16x16:rate=25:duration=0.16','-vf','settb=expr=1/1000,setpts=-80+40*N',
                '-fps_mode','passthrough','-enc_time_base','1:1000','-avoid_negative_ts','disabled','-c:v','ffv1','-level','3','-fflags','+bitexact','-flags:v','+bitexact'
            )
        }
        'GM-LONG-001' {
            Invoke-Ffmpeg -OutputFile $fixture.outputFile -Arguments @(
                '-f','lavfi','-i','testsrc2=size=16x16:rate=25:duration=20',
                '-f','lavfi','-i','anullsrc=r=48000:cl=mono','-t','20',
                '-map','0:v:0','-map','1:a:0','-c:v','ffv1','-level','3','-c:a','pcm_s16le',
                '-fflags','+bitexact','-flags:v','+bitexact','-flags:a','+bitexact'
            )
        }
        'GM-DYNAMIC-001' {
            $partAName = 'dynamic_format_part_a.ts'
            $partBName = 'dynamic_format_part_b.ts'
            Invoke-Ffmpeg -OutputFile $partAName -Arguments @(
                '-f','lavfi','-i','testsrc2=size=16x16:rate=25:duration=0.2',
                '-map','0:v:0','-c:v','mpeg2video','-g','1','-bf','0','-f','mpegts','-fflags','+bitexact','-flags:v','+bitexact'
            )
            Invoke-Ffmpeg -OutputFile $partBName -Arguments @(
                '-f','lavfi','-i','testsrc2=size=32x16:rate=25:duration=0.2',
                '-map','0:v:0','-c:v','mpeg2video','-g','1','-bf','0','-f','mpegts','-fflags','+bitexact','-flags:v','+bitexact'
            )
            $finalPath = Join-Path $outputRoot $fixture.outputFile
            $destination = [System.IO.File]::Create($finalPath)
            try {
                foreach ($partName in @($partAName, $partBName)) {
                    $partPath = Join-Path $outputRoot $partName
                    $source = [System.IO.File]::OpenRead($partPath)
                    try { $source.CopyTo($destination) }
                    finally { $source.Dispose() }
                }
            }
            finally {
                $destination.Dispose()
                Remove-Item -LiteralPath (Join-Path $outputRoot $partAName) -Force
                Remove-Item -LiteralPath (Join-Path $outputRoot $partBName) -Force
            }
        }
        default { throw "No generator is defined for fixture $($fixture.id)" }
    }
}

$ffmpegVersionOutput = & $ffmpeg -version 2>&1
if ($LASTEXITCODE -ne 0) { throw 'ffmpeg -version failed' }
$ffprobeVersionOutput = & $ffprobe -version 2>&1
if ($LASTEXITCODE -ne 0) { throw 'ffprobe -version failed' }
$configurationLine = [string]($ffmpegVersionOutput | Where-Object { $_ -like 'configuration:*' } | Select-Object -First 1)
$configuration = $configurationLine -replace '^configuration:\s*', ''

$probeRecords = foreach ($fixture in $manifest.fixtures) {
    $path = Join-Path $outputRoot $fixture.outputFile
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
    $probeJson = $null
    & $ffprobe -v error -show_format -show_streams -show_frames -of json $path 2>$null | Out-String | ForEach-Object { $probeJson = $_ }
    $ffprobeExitCode = $LASTEXITCODE
    [pscustomobject]@{
        fixtureId = [string]$fixture.id
        outputFile = [string]$fixture.outputFile
        recipeSha256 = [string]$fixture.recipeSha256
        mediaSha256 = $hash
        ffprobeExitCode = $ffprobeExitCode
        probeJson = $probeJson
    }
}

$hashManifestPath = Join-Path $outputRoot 'actual-hashes-and-probe-v1.json'
$evidence = [pscustomobject]@{
    schemaVersion = 1
    mediaContractVersion = [string]$manifest.mediaContractVersion
    ffmpegVersion = [string]($ffmpegVersionOutput | Select-Object -First 1)
    ffprobeVersion = [string]($ffprobeVersionOutput | Select-Object -First 1)
    configuration = $configuration
    configurationSha256 = Get-FileHash -InputStream ([System.IO.MemoryStream]::new([Text.Encoding]::UTF8.GetBytes($configuration))) -Algorithm SHA256 | Select-Object -ExpandProperty Hash
    enabledManifestFeatures = @('avcodec','avdevice','avfilter','avformat','ffmpeg','ffprobe','swresample','swscale','version3')
    defaultFeaturesEnabled = $false
    gplEnabled = $configuration -match '(^|\s)--enable-gpl(\s|$)'
    nonfreeEnabled = $configuration -match '(^|\s)--enable-nonfree(\s|$)'
    fixtures = @($probeRecords)
}
$evidence.configurationSha256 = $evidence.configurationSha256.ToLowerInvariant()
$evidence | ConvertTo-Json -Depth 100 | Set-Content -LiteralPath $hashManifestPath -Encoding utf8NoBOM
Write-Output "GOLDEN_MEDIA_GENERATION=PASS fixtures=$($manifest.fixtures.Count) manifest=$hashManifestPath"
