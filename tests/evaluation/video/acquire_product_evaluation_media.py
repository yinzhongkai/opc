#!/usr/bin/env python3
"""Acquire A-031 product-evaluation windows without committing source media.

This is test/evaluation tooling, not a production runtime dependency.  yt-dlp is
used only to resolve the public source page and a video-only stream.  PyAV seeks,
decodes and writes the approved time window as a muted proxy under an ignored
output directory.  The committed manifest contains hashes and probe facts, but
never the expiring signed media URL or original media bytes.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import statistics
import sys
from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path
from typing import Any

import av
import yt_dlp


TABLE_ROW = re.compile(
    r"^\|\s*(SR-BILI-[A-Z]+-\d{3})\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|"
    r"\s*\[([^]]+)\]\((https://www\.bilibili\.com/video/(BV[0-9A-Za-z]+)/)\)\s*([^|]*?)\s*\|"
    r"\s*(\d{2}:\d{2})～(\d{2}:\d{2})\s*\|\s*([^|]+?)\s*\|$"
)
REPLACEMENT_CLIP_IDS = {
    "SR-BILI-GAME-004",
    "SR-BILI-TRAVEL-001",
    "SR-BILI-TRAVEL-007",
    "SR-BILI-LIFE-007",
}
ARTIFACT_VERSION = re.compile(r"^- 版本：(?P<version>\S+)\s*$", re.MULTILINE)
INPUT_VERSION = re.compile(
    r"^- 协议版本：`productEvaluationInputVersion=(?P<version>[^`]+)`\s*$",
    re.MULTILINE,
)


@dataclass(frozen=True)
class ClipSpec:
    clip_id: str
    partition: str
    category: str
    source_label: str
    source_url: str
    bvid: str
    source_title_hint: str
    start_seconds: int
    end_seconds: int
    expected_coverage: tuple[str, ...]


def parse_clock(value: str) -> int:
    minutes, seconds = value.split(":")
    return int(minutes) * 60 + int(seconds)


def parse_specs(path: Path) -> list[ClipSpec]:
    specs: list[ClipSpec] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        match = TABLE_ROW.match(line)
        if not match:
            continue
        (
            clip_id,
            partition,
            category,
            source_label,
            source_url,
            bvid,
            title_hint,
            start,
            end,
            coverage,
        ) = match.groups()
        specs.append(
            ClipSpec(
                clip_id=clip_id,
                partition=partition.strip(),
                category=category.strip(),
                source_label=source_label.strip(),
                source_url=source_url,
                bvid=bvid,
                source_title_hint=title_hint.strip(),
                start_seconds=parse_clock(start),
                end_seconds=parse_clock(end),
                expected_coverage=tuple(item.strip() for item in coverage.split("、")),
            )
        )
    if len(specs) != 40:
        raise ValueError(f"expected 40 A-031 rows, found {len(specs)}")
    if len({spec.clip_id for spec in specs}) != len(specs):
        raise ValueError("duplicate clipId in A-031")
    return specs


def parse_source_versions(path: Path) -> tuple[str, str]:
    text = path.read_text(encoding="utf-8")
    artifact_match = ARTIFACT_VERSION.search(text)
    input_match = INPUT_VERSION.search(text)
    if not artifact_match or not input_match:
        raise ValueError("A-031 artifact or product-evaluation input version is missing")
    return artifact_match.group("version"), input_match.group("version")


def validate_declared_quotas(specs: list[ClipSpec]) -> dict[str, Any]:
    partition_counts = {
        name: sum(spec.partition == name for spec in specs)
        for name in ("calibration", "tuning", "final_evaluation")
    }
    expected_partitions = {"calibration": 4, "tuning": 16, "final_evaluation": 20}
    if partition_counts != expected_partitions:
        raise ValueError(f"partition quota mismatch: {partition_counts}")

    category_counts: dict[str, int] = {}
    for spec in specs:
        category_counts[spec.category] = category_counts.get(spec.category, 0) + 1
    if sorted(category_counts.values()) != [10, 10, 10, 10]:
        raise ValueError(f"category quota mismatch: {category_counts}")

    slice_counts: dict[str, dict[str, int]] = {}
    for spec in specs:
        for slice_name in spec.expected_coverage:
            counts = slice_counts.setdefault(slice_name, {"all": 0, "final_evaluation": 0})
            counts["all"] += 1
            if spec.partition == "final_evaluation":
                counts["final_evaluation"] += 1
    shortfalls = {
        name: counts
        for name, counts in slice_counts.items()
        if counts["all"] < 3 or counts["final_evaluation"] < 1
    }
    return {
        "status": (
            "provisional_shortfall_pending_human_review" if shortfalls else "declared_coverage_ok"
        ),
        "partitionCounts": partition_counts,
        "categoryCounts": dict(sorted(category_counts.items())),
        "declaredExpectedCoverageCounts": dict(sorted(slice_counts.items())),
        "declaredExpectedCoverageShortfalls": dict(sorted(shortfalls.items())),
        "probeOrHumanDerivedSlicesNotJudgedHere": ["cfr", "vfr"],
    }


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def sha256_json(value: Any) -> str:
    payload = json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()


def extract_source_info(spec: ClipSpec) -> dict[str, Any]:
    options = {
        "quiet": True,
        "no_warnings": True,
        "noplaylist": True,
        "skip_download": True,
    }
    with yt_dlp.YoutubeDL(options) as downloader:
        info = downloader.extract_info(spec.source_url, download=False)
    duration = float(info.get("duration") or 0.0)
    if duration + 0.001 < spec.end_seconds:
        raise ValueError(
            f"{spec.clip_id}: source duration {duration:.3f}s does not cover "
            f"approved end {spec.end_seconds}s"
        )

    formats = [
        item
        for item in info.get("formats", [])
        if item.get("vcodec") not in (None, "none")
        and item.get("acodec") in (None, "none")
        and item.get("url")
        and int(item.get("width") or 0) <= 1280
        and int(item.get("height") or 0) <= 1280
        and min(int(item.get("width") or 0), int(item.get("height") or 0)) <= 720
    ]
    if not formats:
        raise ValueError(f"{spec.clip_id}: no video-only proxy stream at or below 720p")
    formats.sort(
        key=lambda item: (
            min(int(item.get("width") or 0), int(item.get("height") or 0)),
            int(item.get("width") or 0) * int(item.get("height") or 0),
            float(item.get("tbr") or 0.0),
            str(item.get("format_id") or ""),
        )
    )
    selected = formats[-1]
    return {
        "id": str(info.get("id") or ""),
        "title": str(info.get("title") or ""),
        "uploader": str(info.get("uploader") or ""),
        "uploaderId": str(info.get("uploader_id") or ""),
        "uploadDate": str(info.get("upload_date") or ""),
        "durationSeconds": duration,
        "format": selected,
        "httpHeaders": selected.get("http_headers") or info.get("http_headers") or {},
    }


def target_geometry(width: int, height: int) -> tuple[int, int]:
    if width <= 0 or height <= 0:
        raise ValueError("invalid source geometry")
    scale = min(1.0, 1280.0 / width, 720.0 / height) if width >= height else min(
        1.0, 720.0 / width, 1280.0 / height
    )
    target_width = max(2, int(math.floor(width * scale / 2.0) * 2))
    target_height = max(2, int(math.floor(height * scale / 2.0) * 2))
    return target_width, target_height


def av_http_options(headers: dict[str, Any], source_url: str) -> dict[str, str]:
    options: dict[str, str] = {"referer": source_url}
    user_agent = headers.get("User-Agent")
    if user_agent:
        options["user_agent"] = str(user_agent)
    return options


def summarize_timing(frame_times_ns: list[int]) -> dict[str, Any]:
    if not frame_times_ns:
        raise ValueError("cannot summarize an empty frame timestamp sequence")
    deltas = [right - left for left, right in zip(frame_times_ns, frame_times_ns[1:]) if right > left]
    median_delta = int(statistics.median(deltas)) if deltas else 0
    deviating = sum(abs(delta - median_delta) > 1_000_000 for delta in deltas)
    frame_rate_mode = "vfr" if deltas and deviating / len(deltas) > 0.05 else "cfr"
    timing_digest = hashlib.sha256(
        ",".join(str(value) for value in frame_times_ns).encode("ascii")
    ).hexdigest()
    duration_ns = (frame_times_ns[-1] - frame_times_ns[0]) + median_delta
    return {
        "frameRateMode": frame_rate_mode,
        "frameCount": len(frame_times_ns),
        "firstFrameTimeNs": frame_times_ns[0],
        "lastFrameTimeNs": frame_times_ns[-1],
        "durationNs": duration_ns,
        "medianFrameDeltaNs": median_delta,
        "deviatingDeltaCount": deviating,
        "frameTimesSha256": timing_digest,
    }


def transcode_window(spec: ClipSpec, source: dict[str, Any], output_path: Path) -> dict[str, Any]:
    selected = source["format"]
    source_width = int(selected.get("width") or 0)
    source_height = int(selected.get("height") or 0)
    width, height = target_geometry(source_width, source_height)
    temporary_path = output_path.with_suffix(output_path.suffix + ".partial")
    temporary_path.unlink(missing_ok=True)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    with av.open(
        str(selected["url"]),
        mode="r",
        options=av_http_options(source["httpHeaders"], spec.source_url),
    ) as input_container:
        video_stream = next((stream for stream in input_container.streams if stream.type == "video"), None)
        if video_stream is None:
            raise ValueError(f"{spec.clip_id}: resolved stream has no video")
        input_container.seek(
            spec.start_seconds * av.time_base,
            backward=True,
            any_frame=False,
        )
        with av.open(str(temporary_path), mode="w", format="matroska") as output_container:
            selected_fps = float(selected.get("fps") or 0.0)
            if not math.isfinite(selected_fps) or selected_fps < 1.0 or selected_fps > 240.0:
                selected_fps = float(video_stream.average_rate or Fraction(30, 1))
            rate = Fraction(str(selected_fps)).limit_denominator(1001)
            output_stream = output_container.add_stream("mpeg4", rate=rate)
            output_stream.width = width
            output_stream.height = height
            output_stream.pix_fmt = "yuv420p"
            output_stream.bit_rate = 6_000_000
            output_stream.gop_size = max(1, int(round(float(rate))))
            output_stream.time_base = Fraction(1, 1_000_000)
            output_stream.codec_context.time_base = Fraction(1, 60_000)
            first_source_time_ns: int | None = None
            source_frame_times_ns: list[int] = []
            non_monotonic_or_duplicate_frames_dropped = 0
            encoded_frames = 0
            for frame in input_container.decode(video_stream):
                if frame.pts is None or frame.time_base is None:
                    continue
                source_time_ns = int(Fraction(frame.pts) * frame.time_base * 1_000_000_000)
                if source_time_ns < spec.start_seconds * 1_000_000_000:
                    continue
                if source_time_ns >= spec.end_seconds * 1_000_000_000:
                    break
                if source_frame_times_ns and source_time_ns <= source_frame_times_ns[-1]:
                    non_monotonic_or_duplicate_frames_dropped += 1
                    continue
                if first_source_time_ns is None:
                    first_source_time_ns = source_time_ns
                source_frame_times_ns.append(source_time_ns)
                if frame.width != width or frame.height != height:
                    frame = frame.reformat(width=width, height=height, format="yuv420p")
                elif frame.format.name != "yuv420p":
                    frame = frame.reformat(format="yuv420p")
                proxy_pts_us = int(round((source_time_ns - first_source_time_ns) / 1_000.0))
                frame.pts = proxy_pts_us
                frame.time_base = Fraction(1, 1_000_000)
                try:
                    for packet in output_stream.encode(frame):
                        output_container.mux(packet)
                except Exception as error:
                    raise RuntimeError(
                        f"{spec.clip_id}: proxy encode failed at accepted frame {encoded_frames}; "
                        f"sourceTimeNs={source_time_ns}, proxyPtsUs={proxy_pts_us}, "
                        f"sourceGeometry={source_width}x{source_height}, "
                        f"proxyGeometry={width}x{height}, selectedFps={selected_fps}, rate={rate}"
                    ) from error
                encoded_frames += 1
            for packet in output_stream.encode():
                output_container.mux(packet)
            if first_source_time_ns is None or encoded_frames == 0:
                raise ValueError(f"{spec.clip_id}: approved window produced no frames")
    temporary_path.replace(output_path)
    timing = summarize_timing(source_frame_times_ns)
    timing["status"] = "decoded_selected_platform_stream"
    timing["timeDomain"] = "source_stream_absolute_timeNs"
    timing["proxyNormalization"] = "first selected source frame maps to proxy timeNs 0"
    timing["nonMonotonicOrDuplicateFramesDropped"] = non_monotonic_or_duplicate_frames_dropped
    timing["probeDigestSha256"] = sha256_json(timing)
    return timing


def probe_proxy(path: Path) -> dict[str, Any]:
    frame_times_ns: list[int] = []
    with av.open(str(path), mode="r") as container:
        stream = next((item for item in container.streams if item.type == "video"), None)
        if stream is None:
            raise ValueError(f"proxy has no video stream: {path}")
        codec_name = stream.codec_context.name
        pixel_format = stream.codec_context.format.name if stream.codec_context.format else "unknown"
        width = stream.codec_context.width
        height = stream.codec_context.height
        time_base = str(stream.time_base)
        average_rate = str(stream.average_rate) if stream.average_rate else "unavailable"
        for frame in container.decode(stream):
            if frame.pts is None or frame.time_base is None:
                continue
            frame_times_ns.append(int(Fraction(frame.pts) * frame.time_base * 1_000_000_000))
    if not frame_times_ns:
        raise ValueError(f"proxy decoded zero timestamped frames: {path}")
    timing = summarize_timing(frame_times_ns)
    probe = {
        "codecName": codec_name,
        "pixelFormat": pixel_format,
        "width": width,
        "height": height,
        "timeBase": time_base,
        "averageFrameRate": average_rate,
        **timing,
    }
    probe["probeDigestSha256"] = sha256_json(probe)
    return probe


def acquire_one(
    spec: ClipSpec,
    media_dir: Path,
    reuse_existing: bool,
    previous_record: dict[str, Any] | None,
) -> dict[str, Any]:
    print(f"ACQUIRE_BEGIN {spec.clip_id} {spec.bvid}", flush=True)
    source = extract_source_info(spec)
    if source["id"] != spec.bvid:
        raise ValueError(f"{spec.clip_id}: resolved id {source['id']} != {spec.bvid}")
    output_path = media_dir / f"{spec.clip_id}.mkv"
    reused_proxy = reuse_existing and output_path.exists()
    selected_source_timing = None
    if not reused_proxy:
        selected_source_timing = transcode_window(spec, source, output_path)
    probe = probe_proxy(output_path)
    selected = source["format"]
    media_sha256 = sha256_file(output_path)
    if (
        selected_source_timing is None
        and previous_record
        and previous_record.get("source", {}).get("bvid") == spec.bvid
        and previous_record.get("mediaSha256") == media_sha256
    ):
        selected_source_timing = previous_record.get("selectedSourceTiming")
    if selected_source_timing and selected_source_timing.get("status") == "decoded_selected_platform_stream":
        duration_error_ns = abs(
            int(selected_source_timing["durationNs"]) - int(probe["durationNs"])
        )
        median_delta_error_ns = abs(
            int(selected_source_timing["medianFrameDeltaNs"])
            - int(probe["medianFrameDeltaNs"])
        )
        pts_preservation_check = {
            "status": "pass",
            "frameCountMatch": selected_source_timing["frameCount"] == probe["frameCount"],
            "frameRateModeMatch": (
                selected_source_timing["frameRateMode"] == probe["frameRateMode"]
            ),
            "durationErrorNs": duration_error_ns,
            "medianFrameDeltaErrorNs": median_delta_error_ns,
            "timingErrorToleranceNs": 1_000_000,
        }
        if not (
            pts_preservation_check["frameCountMatch"]
            and pts_preservation_check["frameRateModeMatch"]
            and duration_error_ns <= pts_preservation_check["timingErrorToleranceNs"]
            and median_delta_error_ns <= pts_preservation_check["timingErrorToleranceNs"]
        ):
            pts_preservation_check["status"] = "fail"
    else:
        pts_preservation_check = {
            "status": "not-evaluated",
            "reason": "selected source stream was not re-decoded for this reused proxy",
        }
    record = {
        "clipId": spec.clip_id,
        "datasetPartition": spec.partition,
        "productCategory": spec.category,
        "contentCategoriesExpectedPendingHumanReview": list(spec.expected_coverage),
        "source": {
            "provider": "bilibili_public_page",
            "bvid": spec.bvid,
            "pageUrl": spec.source_url,
            "pageLabel": spec.source_label,
            "title": source["title"],
            "uploader": source["uploader"],
            "uploaderId": source["uploaderId"],
            "uploadDate": source["uploadDate"],
            "sourceDurationSeconds": source["durationSeconds"],
            "selectedFormatId": str(selected.get("format_id") or ""),
            "selectedFormatCodec": str(selected.get("vcodec") or ""),
            "selectedFormatWidth": int(selected.get("width") or 0),
            "selectedFormatHeight": int(selected.get("height") or 0),
            "selectedFormatFps": float(selected.get("fps") or 0.0),
        },
        "sourceWindow": {
            "requestedStartNs": spec.start_seconds * 1_000_000_000,
            "requestedEndNs": spec.end_seconds * 1_000_000_000,
        },
        "sourceSelectionDecision": "D-010" if spec.clip_id in REPLACEMENT_CLIP_IDS else "D-009",
        "readOnlyLocationToken": f"t029-media/{spec.clip_id}.mkv",
        "sourceAndUsagePermission": {
            "scope": "space-rhythm-internal-test-only",
            "confirmedBy": "project-default-final-confirmer",
            "decisionId": "D-009",
            "confirmationDate": "2026-09-14",
            "legalConclusion": False,
            "redistributable": False,
        },
        "privacyAndDistribution": {
            "containsPeople": "pending_human_review",
            "sensitiveContent": "pending_human_review",
            "publicEvidenceMayContainMediaBytes": False,
        },
        "mediaSha256": media_sha256,
        "byteLength": output_path.stat().st_size,
        "selectedSourceTiming": selected_source_timing or {
            "status": "not_redecoded_reused_proxy",
            "timeDomain": "unavailable",
            "proxyNormalization": "existing proxy retained and independently re-probed",
        },
        "ptsPreservationCheck": pts_preservation_check,
        "probe": probe,
    }
    print(
        f"ACQUIRE_OK {spec.clip_id} bytes={record['byteLength']} "
        f"frames={probe['frameCount']} mode={probe['frameRateMode']}",
        flush=True,
    )
    return record


def audit_one(spec: ClipSpec) -> dict[str, Any]:
    print(f"AUDIT_BEGIN {spec.clip_id} {spec.bvid}", flush=True)
    source = extract_source_info(spec)
    if source["id"] != spec.bvid:
        raise ValueError(f"{spec.clip_id}: resolved id {source['id']} != {spec.bvid}")
    selected = source["format"]
    record = {
        "clipId": spec.clip_id,
        "datasetPartition": spec.partition,
        "productCategory": spec.category,
        "contentCategoriesExpectedPendingHumanReview": list(spec.expected_coverage),
        "source": {
            "provider": "bilibili_public_page",
            "bvid": spec.bvid,
            "pageUrl": spec.source_url,
            "pageLabel": spec.source_label,
            "title": source["title"],
            "uploader": source["uploader"],
            "uploaderId": source["uploaderId"],
            "uploadDate": source["uploadDate"],
            "sourceDurationSeconds": source["durationSeconds"],
            "selectedFormatId": str(selected.get("format_id") or ""),
            "selectedFormatCodec": str(selected.get("vcodec") or ""),
            "selectedFormatWidth": int(selected.get("width") or 0),
            "selectedFormatHeight": int(selected.get("height") or 0),
            "selectedFormatFps": float(selected.get("fps") or 0.0),
        },
        "sourceWindow": {
            "requestedStartNs": spec.start_seconds * 1_000_000_000,
            "requestedEndNs": spec.end_seconds * 1_000_000_000,
            "coveredBySourceDuration": True,
        },
        "sourceAndUsagePermission": {
            "scope": "space-rhythm-internal-test-only",
            "confirmedBy": "project-default-final-confirmer",
            "decisionId": "D-009",
            "confirmationDate": "2026-09-14",
            "legalConclusion": False,
            "redistributable": False,
        },
        "signedMediaUrlPersisted": False,
    }
    print(
        f"AUDIT_OK {spec.clip_id} duration={source['durationSeconds']:.3f}s "
        f"format={selected.get('format_id')} "
        f"geometry={selected.get('width')}x{selected.get('height')}",
        flush=True,
    )
    return record


def validate_actual_quotas(
    records: list[dict[str, Any]],
    failures: list[dict[str, str]],
    declared_quotas: dict[str, Any],
    full_dataset_requested: bool,
    metadata_only: bool,
) -> dict[str, Any]:
    if metadata_only or not full_dataset_requested:
        return {
            "status": "not-evaluated",
            "reason": "actual quotas require a complete acquired dataset",
        }

    partition_counts = {
        name: sum(record["datasetPartition"] == name for record in records)
        for name in ("calibration", "tuning", "final_evaluation")
    }
    category_counts: dict[str, int] = {}
    proxy_mode_counts: dict[str, int] = {"cfr": 0, "vfr": 0}
    for record in records:
        category = record["productCategory"]
        category_counts[category] = category_counts.get(category, 0) + 1
        mode = record["probe"]["frameRateMode"]
        proxy_mode_counts[mode] = proxy_mode_counts.get(mode, 0) + 1
    vfr_final_count = sum(
        record["datasetPartition"] == "final_evaluation"
        and record["probe"]["frameRateMode"] == "vfr"
        for record in records
    )
    replacement_probe = [
        {
            "clipId": record["clipId"],
            "selectedSourceFrameRateMode": record["selectedSourceTiming"].get("frameRateMode"),
            "proxyFrameRateMode": record["probe"]["frameRateMode"],
            "ptsPreservationStatus": record["ptsPreservationCheck"]["status"],
        }
        for record in records
        if record["clipId"] in REPLACEMENT_CLIP_IDS
    ]
    structural_pass = (
        not failures
        and len(records) == 40
        and partition_counts == {"calibration": 4, "tuning": 16, "final_evaluation": 20}
        and sorted(category_counts.values()) == [10, 10, 10, 10]
    )
    vfr_pass = proxy_mode_counts.get("vfr", 0) >= 3 and vfr_final_count >= 1
    return {
        "status": "fail" if not structural_pass or not vfr_pass else "not-evaluated",
        "structural": {
            "status": "pass" if structural_pass else "fail",
            "clipCount": len(records),
            "partitionCounts": partition_counts,
            "categoryCounts": dict(sorted(category_counts.items())),
        },
        "frameRateMode": {
            "status": "pass" if vfr_pass else "fail",
            "proxyCounts": dict(sorted(proxy_mode_counts.items())),
            "vfrFinalEvaluationCount": vfr_final_count,
            "requiredVfrMinimum": 3,
            "requiredVfrFinalEvaluationMinimum": 1,
            "replacementProbe": replacement_probe,
        },
        "semanticSlices": {
            "status": "not-evaluated",
            "declaredExpectedCoverageStatus": declared_quotas["status"],
            "declaredExpectedCoverageCounts": declared_quotas[
                "declaredExpectedCoverageCounts"
            ],
            "reason": "slow_motion and other visual or semantic slices require adjudicated human annotation",
        },
    }


def build_manifest(args: argparse.Namespace) -> tuple[dict[str, Any], int]:
    specs = parse_specs(args.source_doc)
    artifact_version, input_version = parse_source_versions(args.source_doc)
    declared_quotas = validate_declared_quotas(specs)
    full_dataset_requested = not args.clip_id and not args.limit
    if args.clip_id:
        requested_ids = set(args.clip_id)
        known_ids = {spec.clip_id for spec in specs}
        unknown_ids = sorted(requested_ids - known_ids)
        if unknown_ids:
            raise ValueError(f"unknown --clip-id values: {unknown_ids}")
        specs = [spec for spec in specs if spec.clip_id in requested_ids]
    if args.limit:
        specs = specs[: args.limit]
    previous_records: dict[str, dict[str, Any]] = {}
    for previous_manifest in args.previous_manifest:
        previous = json.loads(previous_manifest.read_text(encoding="utf-8"))
        for record in previous.get("clips", []):
            previous_records[record["clipId"]] = record
    args.output_dir.mkdir(parents=True, exist_ok=True)
    records: list[dict[str, Any]] = []
    failures: list[dict[str, str]] = []
    for spec in specs:
        try:
            records.append(
                audit_one(spec)
                if args.metadata_only
                else acquire_one(
                    spec,
                    args.output_dir,
                    args.reuse_existing,
                    previous_records.get(spec.clip_id),
                )
            )
        except Exception as error:  # noqa: BLE001 - preserve per-source acquisition evidence.
            print(f"ACQUIRE_FAILED {spec.clip_id} {type(error).__name__}: {error}", flush=True)
            failures.append(
                {"clipId": spec.clip_id, "errorType": type(error).__name__, "message": str(error)}
            )
    manifest: dict[str, Any] = {
        "schemaVersion": 1,
        "datasetVersion": args.dataset_version or input_version,
        "productEvaluationInputVersion": input_version,
        "sourceArtifact": f"A-031@{artifact_version}",
        "sourceArtifactSha256": sha256_file(args.source_doc),
        "permissionDecision": "D-009",
        "sourceSelectionDecisions": ["D-009", "D-010"],
        "acquisitionTool": {
            "script": "tests/evaluation/video/acquire_product_evaluation_media.py",
            "ytDlpVersion": yt_dlp.version.__version__,
            "pyAvVersion": av.__version__,
            "ffmpegLibraries": {name: list(version) for name, version in av.library_versions.items()},
            "proxyCodec": "mpeg4",
            "proxyPixelFormat": "yuv420p",
            "audioIncluded": False,
        },
        "mediaRootToken": "out/evaluation/T-029/media",
        "mediaBytesCommittedToGit": False,
        "mode": "metadata_only" if args.metadata_only else "acquire_proxy_media",
        "declaredQuotaValidation": declared_quotas,
        "requestedClipCount": len(specs),
        "auditedClipCount": len(records) if args.metadata_only else 0,
        "acquiredClipCount": 0 if args.metadata_only else len(records),
        "failedClipCount": len(failures),
        "clips": sorted(records, key=lambda item: item["clipId"]),
        "failures": failures,
    }
    manifest["actualQuotaValidation"] = validate_actual_quotas(
        records,
        failures,
        declared_quotas,
        full_dataset_requested,
        args.metadata_only,
    )
    manifest["datasetManifestSha256"] = sha256_json(manifest)
    if failures or len(records) != len(specs):
        return manifest, 1
    if manifest["actualQuotaValidation"]["status"] == "fail":
        return manifest, 2
    return manifest, 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--source-doc",
        type=Path,
        default=Path("projects/space-rhythm/artifacts/A-031-t029-video-product-evaluation-input.md"),
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("out/evaluation/T-029/media"),
    )
    parser.add_argument("--manifest-out", type=Path, required=True)
    parser.add_argument(
        "--dataset-version",
        default="",
        help="dataset version; defaults to A-031 productEvaluationInputVersion",
    )
    parser.add_argument("--limit", type=int, default=0)
    parser.add_argument("--clip-id", action="append", default=[])
    parser.add_argument("--previous-manifest", action="append", type=Path, default=[])
    parser.add_argument("--reuse-existing", action="store_true")
    parser.add_argument("--metadata-only", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    manifest, exit_code = build_manifest(args)
    args.manifest_out.parent.mkdir(parents=True, exist_ok=True)
    args.manifest_out.write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    print(
        f"ACQUISITION_SUMMARY requested={manifest['requestedClipCount']} "
        f"audited={manifest['auditedClipCount']} acquired={manifest['acquiredClipCount']} "
        f"failed={manifest['failedClipCount']} "
        f"quota={manifest['actualQuotaValidation']['status']} "
        f"manifestSha256={manifest['datasetManifestSha256']}",
        flush=True,
    )
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
