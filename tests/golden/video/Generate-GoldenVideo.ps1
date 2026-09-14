[CmdletBinding()]
param(
    [Parameter()]
    [string]$FfmpegPath = 'ffmpeg',

    [Parameter()]
    [string]$FfprobePath = 'ffprobe',

    [Parameter()]
    [string]$OutputDirectory = (Join-Path $PSScriptRoot 'generated'),

    [Parameter()]
    [switch]$ValidateOnly,

    [Parameter()]
    [switch]$RawOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$manifestPath = Join-Path $PSScriptRoot 'fixtures-v1.json'
& (Join-Path $PSScriptRoot 'Test-GoldenVideoManifest.ps1') -ManifestPath $manifestPath
$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
$outputRoot = [System.IO.Path]::GetFullPath($OutputDirectory)
$allowedRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'generated'))
$allowedPrefix = $allowedRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
if (-not $outputRoot.Equals($allowedRoot, [System.StringComparison]::OrdinalIgnoreCase) -and
    -not $outputRoot.StartsWith($allowedPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputDirectory must remain inside $allowedRoot"
}
[System.IO.Directory]::CreateDirectory($outputRoot) | Out-Null

if ($ValidateOnly -and $RawOnly) {
    throw 'ValidateOnly and RawOnly are mutually exclusive'
}

$ffmpeg = if (-not $RawOnly) { (Get-Command $FfmpegPath -ErrorAction Stop).Source } else { $null }
$ffprobe = if (-not $RawOnly) { (Get-Command $FfprobePath -ErrorAction Stop).Source } else { $null }

if (-not $ValidateOnly) {
    Add-Type -Language CSharp -TypeDefinition @'
using System;
using System.IO;

public static class SpaceRhythmVideoFixtures
{
    private const int Width = 160;
    private const int Height = 90;

    private static void Pixel(byte[] frame, int x, int y, int r, int g, int b)
    {
        if (x < 0 || x >= Width || y < 0 || y >= Height) return;
        int offset = (y * Width + x) * 4;
        frame[offset] = (byte)Math.Clamp(b, 0, 255);
        frame[offset + 1] = (byte)Math.Clamp(g, 0, 255);
        frame[offset + 2] = (byte)Math.Clamp(r, 0, 255);
        frame[offset + 3] = 255;
    }

    private static void Solid(byte[] frame, int r, int g, int b)
    {
        for (int y = 0; y < Height; ++y)
            for (int x = 0; x < Width; ++x)
                Pixel(frame, x, y, r, g, b);
    }

    private static void Checker(byte[] frame, bool second, double blend)
    {
        int[] a0 = {18, 32, 52};
        int[] a1 = {55, 78, 105};
        int[] b0 = {165, 72, 35};
        int[] b1 = {238, 188, 88};
        for (int y = 0; y < Height; ++y) {
            for (int x = 0; x < Width; ++x) {
                bool alternate = ((x / 10) + (y / 10)) % 2 != 0;
                int[] from = alternate ? a1 : a0;
                int[] to = alternate ? b1 : b0;
                double amount = second ? 1.0 : blend;
                Pixel(frame, x, y,
                    (int)Math.Round(from[0] * (1.0 - amount) + to[0] * amount),
                    (int)Math.Round(from[1] * (1.0 - amount) + to[1] * amount),
                    (int)Math.Round(from[2] * (1.0 - amount) + to[2] * amount));
            }
        }
    }

    private static void Texture(byte[] frame, int offsetX, int variant)
    {
        int[,] palettes = {
            {18, 35, 68, 52, 90, 145},
            {190, 70, 24, 245, 152, 55},
            {24, 105, 55, 78, 175, 96},
            {88, 38, 126, 175, 94, 210}
        };
        for (int y = 0; y < Height; ++y) {
            for (int x = 0; x < Width; ++x) {
                int worldX = x + offsetX;
                bool alternate = ((worldX / 8) + (y / 8)) % 2 != 0;
                int baseIndex = alternate ? 3 : 0;
                int grid = (worldX % 24 == 0 || y % 18 == 0) ? 28 : 0;
                Pixel(frame, x, y,
                    palettes[variant, baseIndex] + grid,
                    palettes[variant, baseIndex + 1] + grid,
                    palettes[variant, baseIndex + 2] + grid);
            }
        }
    }

    private static void Box(byte[] frame, int x, int y, int size, int r, int g, int b)
    {
        for (int row = y; row < y + size; ++row)
            for (int column = x; column < x + size; ++column)
                Pixel(frame, column, row, r, g, b);
    }

    private static void BoxSubpixel(byte[] frame, double x, double y, int size, int r, int g, int b)
    {
        int firstColumn = Math.Max(0, (int)Math.Floor(x));
        int lastColumn = Math.Min(Width - 1, (int)Math.Ceiling(x + size) - 1);
        int firstRow = Math.Max(0, (int)Math.Floor(y));
        int lastRow = Math.Min(Height - 1, (int)Math.Ceiling(y + size) - 1);
        for (int row = firstRow; row <= lastRow; ++row) {
            double verticalCoverage = Math.Max(0.0, Math.Min(row + 1.0, y + size) - Math.Max(row, y));
            for (int column = firstColumn; column <= lastColumn; ++column) {
                double horizontalCoverage = Math.Max(0.0, Math.Min(column + 1.0, x + size) - Math.Max(column, x));
                double coverage = horizontalCoverage * verticalCoverage;
                int offset = (row * Width + column) * 4;
                frame[offset] = (byte)Math.Round(frame[offset] * (1.0 - coverage) + b * coverage);
                frame[offset + 1] = (byte)Math.Round(frame[offset + 1] * (1.0 - coverage) + g * coverage);
                frame[offset + 2] = (byte)Math.Round(frame[offset + 2] * (1.0 - coverage) + r * coverage);
            }
        }
    }

    private static int FrameCount(string id)
    {
        switch (id) {
            case "VV-HARD-CUT-001": return 90;
            case "VV-DISSOLVE-001": return 120;
            case "VV-FLASH-001": return 90;
            case "VV-GLOBAL-PAN-001": return 90;
            case "VV-LOCAL-IMPACT-001": return 90;
            case "VV-STATIC-001": return 90;
            case "VV-SLOW-MOTION-001": return 240;
            case "VV-VFR-REVERSAL-001": return 7;
            case "VV-COMPRESSION-NOISE-001": return 90;
            case "VV-FAST-CUT-ACTION-001": return 75;
            default: throw new ArgumentException("Unknown fixture " + id);
        }
    }

    private static void Render(string id, int frameIndex, byte[] frame)
    {
        switch (id) {
            case "VV-HARD-CUT-001":
                if (frameIndex < 30) Solid(frame, 32, 64, 96);
                else Solid(frame, 220, 40, 20);
                return;
            case "VV-DISSOLVE-001":
                if (frameIndex < 30) Checker(frame, false, 0.0);
                else if (frameIndex < 60) Checker(frame, false, (frameIndex - 29) / 30.0);
                else Checker(frame, true, 1.0);
                return;
            case "VV-FLASH-001":
                if (frameIndex == 30) Solid(frame, 255, 255, 255);
                else Checker(frame, false, 0.0);
                return;
            case "VV-GLOBAL-PAN-001": {
                int offset;
                if (frameIndex < 15) offset = frameIndex * frameIndex * 2 / 15;
                else if (frameIndex < 75) offset = 30 + (frameIndex - 15) * 2;
                else {
                    int tail = frameIndex - 75;
                    offset = 150 + tail * 2 - tail * tail / 15;
                }
                Texture(frame, offset, 0);
                return;
            }
            case "VV-LOCAL-IMPACT-001": {
                Texture(frame, 0, 0);
                double timeSeconds = frameIndex / 30.0;
                double x = 20.0 + 40.0 * timeSeconds;
                double y;
                if (timeSeconds <= 1.5) {
                    double falling = timeSeconds / 1.5;
                    y = 20.0 + 45.0 * falling * falling;
                } else {
                    double rebound = (timeSeconds - 1.5) / 1.5;
                    y = 65.0 - 45.0 * (2.0 * rebound - rebound * rebound);
                }
                BoxSubpixel(frame, x, y, 16, 250, 230, 40);
                return;
            }
            case "VV-STATIC-001":
                Texture(frame, 0, 0);
                return;
            case "VV-SLOW-MOTION-001": {
                Texture(frame, 0, 0);
                double normalized = Math.Min(frameIndex / 150.0, 1.0);
                double eased = 1.0 - (1.0 - normalized) * (1.0 - normalized);
                double x = 10.0 + 58.0 * eased;
                BoxSubpixel(frame, x, 35.0, 20, 245, 225, 42);
                return;
            }
            case "VV-VFR-REVERSAL-001": {
                int[] positions = {10, 20, 35, 50, 35, 20, 10};
                Texture(frame, 0, 0);
                Box(frame, positions[frameIndex], 34, 20, 250, 230, 40);
                return;
            }
            case "VV-COMPRESSION-NOISE-001": {
                Texture(frame, 0, 0);
                uint state = 0x9e3779b9u ^ (uint)frameIndex;
                for (int y = 0; y < Height; y += 8) {
                    for (int x = 0; x < Width; x += 8) {
                        state = state * 1664525u + 1013904223u;
                        int delta = (int)((state >> 28) & 7u) - 3;
                        for (int row = y; row < Math.Min(y + 8, Height); ++row) {
                            for (int column = x; column < Math.Min(x + 8, Width); ++column) {
                                int offset = (row * Width + column) * 4;
                                frame[offset] = (byte)Math.Clamp(frame[offset] + delta, 0, 255);
                                frame[offset + 1] = (byte)Math.Clamp(frame[offset + 1] + delta, 0, 255);
                                frame[offset + 2] = (byte)Math.Clamp(frame[offset + 2] + delta, 0, 255);
                            }
                        }
                    }
                }
                return;
            }
            case "VV-FAST-CUT-ACTION-001": {
                int scene = frameIndex < 15 ? 0 : (frameIndex < 30 ? 1 : (frameIndex < 45 ? 2 : 3));
                Texture(frame, 0, scene);
                int x = frameIndex < 38 ? 28 : 104;
                Box(frame, x, 36, 16, 250, 240, 55);
                return;
            }
            default:
                throw new ArgumentException("Unknown fixture " + id);
        }
    }

    public static int Write(string path, string id)
    {
        int count = FrameCount(id);
        byte[] frame = new byte[Width * Height * 4];
        using (var stream = new BufferedStream(new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.None), 1 << 20)) {
            for (int index = 0; index < count; ++index) {
                Render(id, index, frame);
                stream.Write(frame, 0, frame.Length);
            }
        }
        return count;
    }
}
'@

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

    foreach ($fixture in $manifest.fixtures) {
        $rawPath = Join-Path $outputRoot ([string]$fixture.id + '.bgra')
        $frameCount = [SpaceRhythmVideoFixtures]::Write($rawPath, [string]$fixture.id)
        try {
            if ($RawOnly) {
                continue
            }
            $inputRate = if ($fixture.id -eq 'VV-SLOW-MOTION-001') {
                '60/1'
            } elseif ($fixture.id -eq 'VV-VFR-REVERSAL-001') {
                '25/1'
            } else {
                '30/1'
            }
            $arguments = @(
                '-f','rawvideo','-pixel_format','bgra','-video_size','160x90','-framerate',$inputRate,
                '-i',$rawPath,'-frames:v',[string]$frameCount
            )
            if ($fixture.id -eq 'VV-VFR-REVERSAL-001') {
                $filter = 'settb=expr=1/1000,setpts=if(eq(N\,0)\,0\,if(eq(N\,1)\,40\,if(eq(N\,2)\,100\,if(eq(N\,3)\,140\,if(eq(N\,4)\,240\,if(eq(N\,5)\,400\,600))))))'
                $arguments += @('-vf',$filter,'-fps_mode','vfr','-enc_time_base','1:1000')
            }
            $arguments += @(
                '-map','0:v:0','-an','-c:v','ffv1','-level','3','-pix_fmt','bgr0',
                '-color_range','pc','-color_primaries','bt709','-color_trc','bt709','-colorspace','bt709',
                '-fflags','+bitexact','-flags:v','+bitexact','-map_metadata','-1',
                '-metadata','creation_time=1970-01-01T00:00:00Z'
            )
            Invoke-Ffmpeg -OutputFile ([string]$fixture.outputFile) -Arguments $arguments
        }
        finally {
            if (-not $RawOnly -and (Test-Path -LiteralPath $rawPath)) {
                Remove-Item -LiteralPath $rawPath -Force
            }
        }
    }
}

if ($RawOnly) {
    Write-Output "GOLDEN_VIDEO_RAW_GENERATION=PASS fixtures=$($manifest.fixtures.Count) output=$outputRoot"
    return
}

$ffmpegVersionOutput = & $ffmpeg -version 2>&1
if ($LASTEXITCODE -ne 0) { throw 'ffmpeg -version failed' }
$ffprobeVersionOutput = & $ffprobe -version 2>&1
if ($LASTEXITCODE -ne 0) { throw 'ffprobe -version failed' }
$configurationLine = [string]($ffmpegVersionOutput | Where-Object { $_ -like 'configuration:*' } | Select-Object -First 1)
$configuration = $configurationLine -replace '^configuration:\s*', ''
$configurationBytes = [Text.Encoding]::UTF8.GetBytes($configuration)
$configurationHash = Get-FileHash -InputStream ([System.IO.MemoryStream]::new($configurationBytes)) -Algorithm SHA256 | Select-Object -ExpandProperty Hash

$records = foreach ($fixture in $manifest.fixtures) {
    $path = Join-Path $outputRoot ([string]$fixture.outputFile)
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing generated fixture $path"
    }
    $probeText = & $ffprobe -v error -select_streams v:0 -count_frames `
        -show_entries 'stream=codec_name,pix_fmt,width,height,time_base,avg_frame_rate,r_frame_rate,nb_read_frames,duration' `
        -of json $path 2>$null | Out-String
    if ($LASTEXITCODE -ne 0) { throw "ffprobe failed for $($fixture.id)" }
    $probe = $probeText | ConvertFrom-Json
    if (@($probe.streams).Count -ne 1) { throw "Expected one video stream for $($fixture.id)" }
    $stream = $probe.streams[0]
    $frameTimesNs = @()
    if ($fixture.id -eq 'VV-VFR-REVERSAL-001') {
        $framesText = & $ffprobe -v error -select_streams v:0 `
            -show_entries 'frame=best_effort_timestamp_time' -of json $path 2>$null | Out-String
        if ($LASTEXITCODE -ne 0) { throw 'ffprobe frame timing failed for VFR fixture' }
        $framesProbe = $framesText | ConvertFrom-Json
        $frameTimesNs = @($framesProbe.frames | ForEach-Object {
            [int64]([decimal]::Parse([string]$_.best_effort_timestamp_time, [Globalization.CultureInfo]::InvariantCulture) * 1000000000)
        })
    }
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
    if ($fixture.mediaStatus -eq 'generated' -and $fixture.actualMediaSha256 -ne $hash) {
        throw "Frozen media hash mismatch for $($fixture.id): expected $($fixture.actualMediaSha256), actual $hash"
    }
    [pscustomobject]@{
        fixtureId = [string]$fixture.id
        outputFile = [string]$fixture.outputFile
        recipeSha256 = [string]$fixture.recipeSha256
        annotationSha256 = [string]$fixture.annotationSha256
        mediaSha256 = $hash
        byteCount = (Get-Item -LiteralPath $path).Length
        codecName = [string]$stream.codec_name
        pixelFormat = [string]$stream.pix_fmt
        width = [int]$stream.width
        height = [int]$stream.height
        timeBase = [string]$stream.time_base
        averageFrameRate = [string]$stream.avg_frame_rate
        realFrameRate = [string]$stream.r_frame_rate
        frameCount = [int]$stream.nb_read_frames
        durationSeconds = if ($null -ne $stream.PSObject.Properties['duration']) {
            [string]$stream.duration
        } else {
            'unavailable'
        }
        vfrFrameTimesNs = $frameTimesNs
    }
}

$evidence = [ordered]@{
    schemaVersion = 1
    videoAnalysisContractVersion = [string]$manifest.videoAnalysisContractVersion
    mediaContractVersion = [string]$manifest.mediaContractVersion
    manifestVersion = [int]$manifest.manifestVersion
    ffmpegVersion = [string]($ffmpegVersionOutput | Select-Object -First 1)
    ffprobeVersion = [string]($ffprobeVersionOutput | Select-Object -First 1)
    configuration = $configuration
    configurationSha256 = $configurationHash.ToLowerInvariant()
    defaultFeaturesEnabled = $false
    gplEnabled = $configuration -match '(^|\s)--enable-gpl(\s|$)'
    nonfreeEnabled = $configuration -match '(^|\s)--enable-nonfree(\s|$)'
    fixtures = @($records)
}
$evidencePath = Join-Path $outputRoot 'actual-hashes-and-probe-v1.json'
$evidence | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8NoBOM
Write-Output "GOLDEN_VIDEO_GENERATION=PASS fixtures=$($records.Count) evidence=$evidencePath"
