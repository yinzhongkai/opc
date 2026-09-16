#!/usr/bin/env python3
"""Prepare and freeze USER-01 evaluation sessions without committing media bytes."""

from __future__ import annotations

import argparse
import hashlib
import json
import random
import secrets
import shutil
import subprocess
from fractions import Fraction
from pathlib import Path
from typing import Any


SOURCE_ROOT = Path(__file__).resolve().parents[3]
PROJECT_ROOT = SOURCE_ROOT.parent
REPOSITORY_ROOT = PROJECT_ROOT.parents[1]
ACCEPTANCE_SCOPE = "personal-single-user-acceptance"
USER_ID = "USER-01"
WORKFLOW_VERSION = "0.1.0"


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode(
        "utf-8"
    )


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def run_json(command: list[str]) -> Any:
    completed = subprocess.run(command, check=True, capture_output=True, text=True, encoding="utf-8")
    return json.loads(completed.stdout)


def frame_times_ns(ffprobe: Path, media_path: Path) -> list[int]:
    payload = run_json(
        [
            str(ffprobe),
            "-v",
            "error",
            "-select_streams",
            "v:0",
            "-show_entries",
            "stream=time_base:frame=best_effort_timestamp",
            "-of",
            "json",
            str(media_path),
        ]
    )
    streams = payload.get("streams", [])
    if len(streams) != 1 or not streams[0].get("time_base"):
        raise ValueError(f"{media_path}: missing unique video time_base")
    time_base = Fraction(streams[0]["time_base"])
    times = [
        int(Fraction(int(frame["best_effort_timestamp"])) * time_base * 1_000_000_000)
        for frame in payload.get("frames", [])
        if frame.get("best_effort_timestamp") not in (None, "N/A")
    ]
    if not times or any(right <= left for left, right in zip(times, times[1:])):
        raise ValueError(f"{media_path}: frame PTS are empty, duplicate, or non-monotonic")
    first = times[0]
    return [value - first for value in times]


def timing_sha256(times: list[int]) -> str:
    return sha256_bytes(",".join(str(value) for value in times).encode("ascii"))


def transcode_preview(ffmpeg: Path, source: Path, target: Path) -> None:
    if target.exists() and target.stat().st_mtime_ns >= source.stat().st_mtime_ns:
        return
    target.parent.mkdir(parents=True, exist_ok=True)
    temporary = target.with_suffix(".partial.mp4")
    completed = subprocess.run(
        [
            str(ffmpeg),
            "-hide_banner",
            "-loglevel",
            "error",
            "-y",
            "-i",
            str(source),
            "-map",
            "0:v:0",
            "-an",
            "-fps_mode",
            "passthrough",
            "-c:v",
            "h264_mf",
            "-rate_control",
            "quality",
            "-quality",
            "80",
            "-movflags",
            "+faststart",
            "-video_track_timescale",
            "1000000",
            str(temporary),
        ],
        capture_output=True,
        text=True,
        encoding="utf-8",
    )
    if completed.returncode != 0:
        temporary.unlink(missing_ok=True)
        raise RuntimeError(f"preview transcode failed for {source.name}: {completed.stderr.strip()}")
    temporary.replace(target)


def url_from_source(path: Path) -> str:
    resolved = path.resolve()
    relative = resolved.relative_to(SOURCE_ROOT.resolve())
    return "/" + relative.as_posix()


def url_from_repository(path: Path) -> str:
    resolved = path.resolve()
    relative = resolved.relative_to(REPOSITORY_ROOT.resolve())
    return "/" + relative.as_posix()


def prepare_reference(args: argparse.Namespace) -> int:
    manifest_path = args.product_manifest.resolve()
    manifest = read_json(manifest_path)
    media_root = args.media_root.resolve()
    workspace_root = args.workspace_root.resolve()
    clips: list[dict[str, Any]] = []
    for record in manifest.get("clips", []):
        media_path = media_root / Path(record["readOnlyLocationToken"]).name
        if not media_path.is_file():
            raise FileNotFoundError(f"missing media for {record['clipId']}: {media_path}")
        actual_media_hash = sha256_file(media_path)
        if actual_media_hash != record["mediaSha256"]:
            raise ValueError(f"{record['clipId']}: media SHA-256 changed")
        times = frame_times_ns(args.ffprobe.resolve(), media_path)
        if timing_sha256(times) != record["probe"]["frameTimesSha256"]:
            raise ValueError(f"{record['clipId']}: decoded frame-time SHA-256 changed")
        preview_path = workspace_root / "previews" / f"{record['clipId']}.mp4"
        if not args.skip_previews:
            transcode_preview(args.ffmpeg.resolve(), media_path, preview_path)
        preview_times: list[int] = []
        preview_mapping_error_ns: int | None = None
        if preview_path.is_file():
            preview_times = frame_times_ns(args.ffprobe.resolve(), preview_path)
            if len(preview_times) != len(times):
                raise ValueError(
                    f"{record['clipId']}: browser preview changed the displayed frame count"
                )
            preview_mapping_error_ns = max(
                abs(preview_time - source_time)
                for preview_time, source_time in zip(preview_times, times)
            )
        clips.append(
            {
                "clipId": record["clipId"],
                "datasetPurpose": "product_representative",
                "datasetPartition": record["datasetPartition"],
                "productCategory": record["productCategory"],
                "mediaSha256": actual_media_hash,
                "sourceFrameTimesSha256": timing_sha256(times),
                "frameTimesNs": times,
                "previewFrameTimesNs": preview_times,
                "previewToSourceMapping": (
                    {
                        "status": "pass",
                        "method": "presentation-order ordinal mapping",
                        "displayedFrameCount": len(preview_times),
                        "sourceFrameCount": len(times),
                        "maxAbsTimestampDifferenceNs": preview_mapping_error_ns,
                    }
                    if preview_times
                    else {"status": "pending_generation"}
                ),
                "previewUrl": url_from_source(preview_path),
                "previewStatus": "ready" if preview_path.is_file() else "pending_generation",
                "referenceAudioPolicy": "muted",
            }
        )
    session: dict[str, Any] = {
        "schemaVersion": 1,
        "workflowVersion": WORKFLOW_VERSION,
        "phase": "reference_authoring",
        "sessionId": args.session_id,
        "acceptanceScope": ACCEPTANCE_SCOPE,
        "reviewerAnonymousId": USER_ID,
        "annotationProtocolVersion": "0.2.0",
        "productEvaluationInput": "A-031@0.5",
        "productEvaluationInputSha256": sha256_file(args.a031.resolve()),
        "datasetManifest": url_from_repository(manifest_path),
        "datasetManifestFileSha256": sha256_file(manifest_path),
        "datasetManifestContentSha256": manifest.get("datasetManifestSha256"),
        "mediaBytesCommittedToGit": False,
        "blindRating": {
            "status": "locked",
            "unlockCondition": "freeze single_user_reference, run classic, and create hidden randomized A/B renders",
        },
        "manualCorrection": {
            "status": "locked",
            "unlockCondition": "submit the valid blind trial for each clip before exposing classic events",
        },
        "clips": clips,
    }
    session["sessionDefinitionSha256"] = sha256_bytes(canonical_bytes(session))
    output = workspace_root / "reference-session-v1.json"
    write_json(output, session)
    print(f"USER01_REFERENCE_SESSION=READY clips={len(clips)} output={output}")
    return 0


def validate_reference(session: dict[str, Any], submission: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if submission.get("phase") != "reference_authoring":
        errors.append("phase must be reference_authoring")
    required = {clip["clipId"]: clip for clip in session.get("clips", [])}
    submitted = {clip.get("clipId"): clip for clip in submission.get("clips", [])}
    if set(required) != set(submitted):
        errors.append("submission clip IDs must exactly match the session")
    event_ids: set[str] = set()
    for clip_id, definition in required.items():
        clip = submitted.get(clip_id, {})
        if clip.get("reviewStatus") != "complete":
            errors.append(f"{clip_id}: reviewStatus must be complete")
        reviewed = set(clip.get("reviewedKinds", []))
        if reviewed != {"shot", "motion_peak", "action_peak", "negative_span"}:
            errors.append(f"{clip_id}: all four event/negative kinds must be explicitly reviewed")
        frame_times = definition["frameTimesNs"]
        for event in clip.get("events", []):
            event_id = event.get("annotationId")
            if not event_id or event_id in event_ids:
                errors.append(f"{clip_id}: annotationId is missing or duplicated")
            event_ids.add(event_id)
            kind = event.get("kind")
            if kind not in {"shot", "motion_peak", "action_peak", "negative_span"}:
                errors.append(f"{clip_id}/{event_id}: invalid kind")
                continue
            ordinal = event.get("sourceDecodeOrdinal")
            if not isinstance(ordinal, int) or ordinal < 0 or ordinal >= len(frame_times):
                errors.append(f"{clip_id}/{event_id}: invalid sourceDecodeOrdinal")
            elif event.get("timeNs") != frame_times[ordinal]:
                errors.append(f"{clip_id}/{event_id}: timeNs is not the saved source frame PTS")
            if kind == "negative_span" and int(event.get("durationNs", 0)) <= 0:
                errors.append(f"{clip_id}/{event_id}: negative_span requires durationNs > 0")
            if event.get("annotatorAnonymousId") != USER_ID:
                errors.append(f"{clip_id}/{event_id}: annotator must be USER-01")
            if event.get("referenceStatus") != "single_user_reference":
                errors.append(f"{clip_id}/{event_id}: referenceStatus must be single_user_reference")
    return errors


def validate_blind(session: dict[str, Any], submission: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if submission.get("phase") != "blind_rating":
        errors.append("phase must be blind_rating")
    required = {trial["clipId"] for trial in session.get("trials", [])}
    submitted = {clip.get("clipId"): clip for clip in submission.get("clips", [])}
    if required != set(submitted):
        errors.append("blind submission clip IDs must exactly match the randomized session")
    dimensions = set(session.get("ratingDimensions", []))
    for clip_id in required:
        record = submitted.get(clip_id, {})
        if record.get("reviewStatus") != "complete":
            errors.append(f"{clip_id}: blind reviewStatus must be complete")
        counts = record.get("playbackCounts", {})
        if any(not isinstance(counts.get(label), int) or not 1 <= counts[label] <= 3 for label in ("A", "B")):
            errors.append(f"{clip_id}: A/B complete playback counts must each be 1..3")
        ratings = record.get("ratings") or {}
        for dimension in dimensions:
            value = ratings.get(dimension)
            if value != "N/A" and (not isinstance(value, int) or not 1 <= value <= 5):
                errors.append(f"{clip_id}: invalid {dimension} rating")
        if "N/A" in ratings.values() and not ratings.get("naReasonToken"):
            errors.append(f"{clip_id}: N/A requires naReasonToken")
        if ratings.get("directExportReadiness") not in {
            "direct_export", "minor_edit", "major_edit", "unusable"
        }:
            errors.append(f"{clip_id}: invalid directExportReadiness")
        if record.get("pairPreference") not in {"A", "B", "tie"}:
            errors.append(f"{clip_id}: invalid pairPreference")
    return errors


def validate_correction(session: dict[str, Any], submission: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if submission.get("phase") != "manual_correction":
        errors.append("phase must be manual_correction")
    required = {clip["clipId"] for clip in session.get("clips", [])}
    submitted = {clip.get("clipId"): clip for clip in submission.get("clips", [])}
    if required != set(submitted):
        errors.append("correction submission clip IDs must exactly match the session")
    for clip_id in required:
        record = submitted.get(clip_id, {})
        if record.get("reviewStatus") != "complete":
            errors.append(f"{clip_id}: correction reviewStatus must be complete")
        if record.get("correctionReadiness") not in {
            "direct_export", "minor_edit", "major_edit", "unusable"
        }:
            errors.append(f"{clip_id}: invalid correctionReadiness")
        if not isinstance(record.get("activeEditMs"), int) or record["activeEditMs"] < 0:
            errors.append(f"{clip_id}: activeEditMs must be a non-negative integer")
        for operation in record.get("correctionLog", []):
            if operation.get("operation") not in {"add", "delete", "move", "reclassify", "lock"}:
                errors.append(f"{clip_id}: invalid correction operation")
    return errors


def verify_session_digest(session: dict[str, Any]) -> None:
    expected = sha256_bytes(
        canonical_bytes({key: value for key, value in session.items() if key != "sessionDefinitionSha256"})
    )
    if session.get("sessionDefinitionSha256") != expected:
        raise ValueError("session definition SHA-256 is invalid")


def prepare_blind(args: argparse.Namespace) -> int:
    reference = read_json(args.reference.resolve())
    if reference.get("status") != "frozen" or reference.get("referenceStatus") != "single_user_reference":
        raise ValueError("blind preparation requires a frozen single_user_reference")
    renders = read_json(args.render_manifest.resolve())
    rows = renders.get("clips", [])
    if not rows:
        raise ValueError("render manifest contains no clips")
    environment = renders.get("reviewEnvironment") or {}
    required_environment = {
        "windowsEditionBuild", "displayRefreshHz", "displayScalePercent", "audioDevice",
        "audioDriver", "sampleRateHz", "systemVolume", "applicationVolume", "headphone",
        "roomEnvironment",
    }
    missing_environment = sorted(required_environment - set(environment))
    if missing_environment:
        raise ValueError("reviewEnvironment is missing: " + ", ".join(missing_environment))
    expected_final = {
        row["clipId"]
        for row in reference.get("clipCoverage", [])
        if row.get("datasetPartition") == "final_evaluation"
    }
    if not expected_final:
        raise ValueError("frozen reference does not define final_evaluation clip coverage")
    if {row.get("clipId") for row in rows} != expected_final:
        raise ValueError("render manifest must exactly cover every frozen final_evaluation clip")
    seed = args.seed or secrets.token_hex(32)
    rng = random.Random(int(sha256_bytes(seed.encode("utf-8")), 16))
    rows = list(rows)
    rng.shuffle(rows)
    output_root = args.workspace_root.resolve()
    blind_media = output_root / "blind-media" / args.session_id
    trials: list[dict[str, Any]] = []
    answer_rows: list[dict[str, Any]] = []
    seen: set[str] = set()
    for ordinal, row in enumerate(rows, start=1):
        clip_id = row.get("clipId")
        if not clip_id or clip_id in seen:
            raise ValueError("render manifest clipId is missing or duplicated")
        seen.add(clip_id)
        if row.get("datasetPartition") != "final_evaluation":
            raise ValueError(f"{clip_id}: blind input must be final_evaluation")
        variants = {name: row.get(name) for name in ("classic", "humanReference")}
        for name, variant in variants.items():
            source = Path(variant.get("path", "")).resolve() if variant else Path()
            if not source.is_file() or sha256_file(source) != variant.get("sha256"):
                raise ValueError(f"{clip_id}/{name}: render is missing or hash changed")
        labels = ["classic", "humanReference"]
        rng.shuffle(labels)
        trial_id = f"{args.session_id}-T{ordinal:03d}"
        public_variants: dict[str, Any] = {}
        answer_mapping: dict[str, str] = {}
        for label, source_name in zip(("A", "B"), labels):
            source = Path(variants[source_name]["path"]).resolve()
            target = blind_media / f"{trial_id}-{label}.mp4"
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
            public_variants[label] = {"mediaUrl": url_from_source(target)}
            answer_mapping[label] = source_name
        trials.append(
            {
                "trialId": trial_id,
                "clipId": clip_id,
                "datasetPurpose": row["datasetPurpose"],
                "datasetPartition": "final_evaluation",
                "variants": public_variants,
            }
        )
        answer_rows.append(
            {
                "trialId": trial_id,
                "clipId": clip_id,
                "mapping": answer_mapping,
                "classicSha256": variants["classic"]["sha256"],
                "humanReferenceSha256": variants["humanReference"]["sha256"],
            }
        )
    session: dict[str, Any] = {
        "schemaVersion": 1,
        "workflowVersion": WORKFLOW_VERSION,
        "phase": "blind_rating",
        "sessionId": args.session_id,
        "acceptanceScope": ACCEPTANCE_SCOPE,
        "reviewerAnonymousId": USER_ID,
        "referenceSha256": reference["singleUserReferenceSha256"],
        "renderManifestSha256": sha256_file(args.render_manifest.resolve()),
        "randomizationSeedSha256": sha256_bytes(seed.encode("utf-8")),
        "answerKeyExcluded": True,
        "playbackPolicy": {"seekAllowed": False, "minimumCompletePlaysPerVariant": 1, "maximumPlaysPerVariant": 3},
        "ratingDimensions": [
            "temporalAlignment", "salienceAccentMatch", "densityAndExtraBeats",
            "continuity", "editReadiness", "overallNaturalness",
        ],
        "reviewEnvironment": environment,
        "trials": trials,
    }
    session["sessionDefinitionSha256"] = sha256_bytes(canonical_bytes(session))
    answer_key = {
        "schemaVersion": 1,
        "status": "hidden_until_blind_submission_frozen",
        "sessionDefinitionSha256": session["sessionDefinitionSha256"],
        "randomizationSeed": seed,
        "trials": answer_rows,
    }
    answer_key["answerKeySha256"] = sha256_bytes(canonical_bytes(answer_key))
    write_json(args.session_out.resolve(), session)
    write_json(args.answer_key_out.resolve(), answer_key)
    print(f"USER01_BLIND_SESSION=READY trials={len(trials)} output={args.session_out.resolve()}")
    return 0


def prepare_correction(args: argparse.Namespace) -> int:
    blind = read_json(args.blind_result.resolve())
    if blind.get("status") != "frozen" or blind.get("phase") != "blind_rating":
        raise ValueError("correction preparation requires a frozen blind result")
    classic = read_json(args.classic_manifest.resolve())
    classic_by_clip = {row["clipId"]: row for row in classic.get("clips", [])}
    completed = [row for row in blind["submission"]["clips"] if row.get("reviewStatus") == "complete"]
    clips: list[dict[str, Any]] = []
    for rated in completed:
        clip_id = rated["clipId"]
        row = classic_by_clip.get(clip_id)
        if not row:
            raise ValueError(f"{clip_id}: missing frozen classic result")
        media_path = Path(row["mediaPath"]).resolve()
        if not media_path.is_file() or sha256_file(media_path) != row["mediaSha256"]:
            raise ValueError(f"{clip_id}: correction media is missing or hash changed")
        clips.append(
            {
                "clipId": clip_id,
                "datasetPurpose": row["datasetPurpose"],
                "datasetPartition": "final_evaluation",
                "mediaUrl": url_from_source(media_path),
                "mediaSha256": row["mediaSha256"],
                "frameTimesNs": row["frameTimesNs"],
                "classicEvents": row["events"],
                "algorithmId": row["algorithmId"],
                "algorithmVersion": row["algorithmVersion"],
                "parametersDigestSha256": row["parametersDigestSha256"],
            }
        )
    session: dict[str, Any] = {
        "schemaVersion": 1,
        "workflowVersion": WORKFLOW_VERSION,
        "phase": "manual_correction",
        "sessionId": args.session_id,
        "acceptanceScope": ACCEPTANCE_SCOPE,
        "reviewerAnonymousId": USER_ID,
        "blindResultSha256": blind["frozenResultSha256"],
        "classicManifestSha256": sha256_file(args.classic_manifest.resolve()),
        "clips": clips,
    }
    session["sessionDefinitionSha256"] = sha256_bytes(canonical_bytes(session))
    write_json(args.session_out.resolve(), session)
    print(f"USER01_CORRECTION_SESSION=READY clips={len(clips)} output={args.session_out.resolve()}")
    return 0


def freeze_submission(args: argparse.Namespace) -> int:
    session = read_json(args.session.resolve())
    submission = read_json(args.submission.resolve())
    verify_session_digest(session)
    if submission.get("sessionDefinitionSha256") != session["sessionDefinitionSha256"]:
        raise ValueError("submission does not bind to this session definition")
    if submission.get("acceptanceScope") != ACCEPTANCE_SCOPE:
        raise ValueError("submission acceptanceScope is invalid")
    if submission.get("reviewerAnonymousId") != USER_ID:
        raise ValueError("submission reviewerAnonymousId is invalid")
    phase = session.get("phase")
    validators = {
        "reference_authoring": validate_reference,
        "blind_rating": validate_blind,
        "manual_correction": validate_correction,
    }
    if phase not in validators:
        raise ValueError(f"unsupported session phase: {phase}")
    errors = validators[phase](session, submission)
    if errors:
        raise ValueError("submission validation failed:\n- " + "\n- ".join(errors))
    frozen: dict[str, Any] = {
        "schemaVersion": 1,
        "status": "frozen",
        "phase": phase,
        "acceptanceScope": ACCEPTANCE_SCOPE,
        "reviewerAnonymousId": (
            "not_applicable(reason=single_user_scope)" if phase == "reference_authoring" else USER_ID
        ),
        "adjudicatorAnonymousId": "not_applicable(reason=single_user_scope)",
        "independentReview": "not_applicable(reason=single_user_scope)",
        "sessionDefinitionSha256": session["sessionDefinitionSha256"],
        "submission": submission,
    }
    if phase == "reference_authoring":
        frozen["referenceStatus"] = "single_user_reference"
        frozen["referenceAuthorAnonymousId"] = USER_ID
        frozen["clipCoverage"] = [
            {
                "clipId": clip["clipId"],
                "datasetPurpose": clip["datasetPurpose"],
                "datasetPartition": clip["datasetPartition"],
            }
            for clip in session["clips"]
        ]
        digest_field = "singleUserReferenceSha256"
    else:
        digest_field = "frozenResultSha256"
    frozen[digest_field] = sha256_bytes(canonical_bytes(frozen))
    write_json(args.frozen_out.resolve(), frozen)
    print(
        f"USER01_{phase.upper()}_FREEZE=PASS "
        f"sha256={frozen[digest_field]} output={args.frozen_out.resolve()}"
    )
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    prepare = commands.add_parser("prepare-reference")
    prepare.add_argument(
        "--product-manifest",
        type=Path,
        default=PROJECT_ROOT / "evidence/T-029/product-dataset-manifest-v2.json",
    )
    prepare.add_argument(
        "--a031",
        type=Path,
        default=PROJECT_ROOT / "artifacts/A-031-t029-video-product-evaluation-input.md",
    )
    prepare.add_argument(
        "--media-root", type=Path, default=SOURCE_ROOT / "out/evaluation/T-029/media"
    )
    prepare.add_argument(
        "--workspace-root", type=Path, default=SOURCE_ROOT / "out/evaluation/T-029/user01"
    )
    prepare.add_argument(
        "--ffprobe",
        type=Path,
        default=SOURCE_ROOT
        / "out/vcpkg_installed/x64-windows-space-rhythm/tools/ffmpeg/ffprobe.exe",
    )
    prepare.add_argument(
        "--ffmpeg",
        type=Path,
        default=SOURCE_ROOT
        / "out/vcpkg_installed/x64-windows-space-rhythm/tools/ffmpeg/ffmpeg.exe",
    )
    prepare.add_argument("--session-id", default="USER01-REFERENCE-PRODUCT-V1")
    prepare.add_argument("--skip-previews", action="store_true")
    prepare.set_defaults(func=prepare_reference)

    blind = commands.add_parser("prepare-blind")
    blind.add_argument("--reference", type=Path, required=True)
    blind.add_argument("--render-manifest", type=Path, required=True)
    blind.add_argument(
        "--workspace-root", type=Path, default=SOURCE_ROOT / "out/evaluation/T-029/user01"
    )
    blind.add_argument("--session-id", default="USER01-BLIND-FINAL-V1")
    blind.add_argument("--seed")
    blind.add_argument("--session-out", type=Path, required=True)
    blind.add_argument("--answer-key-out", type=Path, required=True)
    blind.set_defaults(func=prepare_blind)

    correction = commands.add_parser("prepare-correction")
    correction.add_argument("--blind-result", type=Path, required=True)
    correction.add_argument("--classic-manifest", type=Path, required=True)
    correction.add_argument("--session-id", default="USER01-CORRECTION-FINAL-V1")
    correction.add_argument("--session-out", type=Path, required=True)
    correction.set_defaults(func=prepare_correction)

    freeze = commands.add_parser("freeze")
    freeze.add_argument("--session", type=Path, required=True)
    freeze.add_argument("--submission", type=Path, required=True)
    freeze.add_argument("--frozen-out", type=Path, required=True)
    freeze.set_defaults(func=freeze_submission)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
