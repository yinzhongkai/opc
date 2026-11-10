"""Validate T-003 audit provenance and internal references; never run book examples.

Usage: python -B source-audit-check.py [--source-cache PATH] [--check-git]
The optional cache is an existing HTTPS research cache, not a managed workspace.
No downloads, checkout changes, manuscript writes, or experiment execution occur.
"""

import argparse
import hashlib
import json
import subprocess
from collections import Counter
from pathlib import Path


def digest(data):
    return hashlib.sha256(data).hexdigest()


def git(root, *args):
    return subprocess.check_output(["git", "--no-optional-locks", "-C", str(root), *args])


def require(condition, message):
    if not condition:
        raise ValueError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-cache", type=Path)
    parser.add_argument("--check-git", action="store_true")
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    data = json.loads((here / "source-audit-evidence.json").read_text(encoding="utf-8"))
    report = (here / "source-audit.md").read_text(encoding="utf-8")
    report_lines = report.splitlines()
    require(data["project"] == "book-bitcoin" and data["owner"] == "researcher", "audit identity")
    require(data["artifact_id"] == "A-004" and data["task"] == "T-003", "artifact/task identity")
    require(data["version"] == "0.1" and data["status"] == "draft", "version/status")
    files = data["manuscript"]["files"]
    require(len(files) == 13, "13 manuscript files required")
    require(sum(f["lines"] for f in files) == 8139, "manuscript line count")
    require(sum(f["git_bytes"] for f in files) == 649322, "Git byte count")
    require(sum(f["working_bytes"] for f in files) == 657461, "working-tree byte count")
    scan = data["reference_scan"]
    require(scan["code_ids"] == list(range(1, 171)), "C-1..C-170 structural coverage")
    require(scan["fenced_blocks"] == 229, "code-fence count")
    require(len(scan["bpftrace_names"]) == 18, "18 named script references")
    require(all(x["exists"] for x in scan["core_paths"]), "unresolved Core reference path")

    sources = {s["id"]: s for s in data["sources"]}
    require(len(sources) == len(data["sources"]), "duplicate source ID")
    rows = data["findings"] + data["confirmed_claims"] + data["execution_gaps"]
    require(len({r["id"] for r in rows}) == len(rows), "duplicate evidence ID")
    for group, prefix, result in [("findings", "F", "revise"),
                                  ("confirmed_claims", "C", "supported"),
                                  ("execution_gaps", "G", "not_executed")]:
        require([r["id"] for r in data[group]] ==
                [f"{prefix}-{i:03}" for i in range(1, len(data[group]) + 1)], "non-contiguous IDs")
        for row in data[group]:
            require(row["result"] == result, f"result: {row['id']}")
            chapter = row["chapter"]
            require(0 <= chapter <= 12, f"chapter: {row['id']}")
            require(row["manuscript_path"] == files[chapter]["path"], f"path: {row['id']}")
            require(1 <= row["start"] <= row["end"] <= files[chapter]["lines"], f"location: {row['id']}")
            require(row["sources"], f"missing source: {row['id']}")
            report_rows = [line for line in report_lines
                           if line.startswith(f"| {row['id']} ")]
            require(len(report_rows) == 1, f"report row count: {row['id']}")
            for ref in row["sources"]:
                require(ref["source"] in sources, f"unknown source: {row['id']}")
                source = sources[ref["source"]]
                if ref["start"] is not None:
                    require(1 <= ref["start"] <= ref["end"] <= source["lines"], f"source range: {row['id']}")
                url = source["url"]
                if ref["start"] is not None and url.startswith("https://"):
                    url += f"#L{ref['start']}-L{ref['end']}"
                require(url in report_rows[0], f"report source mismatch: {row['id']}")
    require({r["chapter"] for r in rows} == set(range(13)), "chapter coverage")
    require(len(data["coverage_notes"]) == 13, "coverage notes")

    root = Path(data["manuscript"]["checkout"])
    for f in files:
        content = (root / f["path"]).read_bytes()
        require(digest(content) == f["working_sha256"], f"working fingerprint: {f['path']}")
        require(len(content.decode("utf-8").splitlines()) == f["lines"], f"line count: {f['path']}")
        if args.check_git:
            blob = git(root, "show", data["manuscript"]["revision"] + ":" + f["path"])
            filtered = git(root, "cat-file", "--filters", data["manuscript"]["revision"] + ":" + f["path"])
            require(digest(blob) == f["git_sha256"], f"Git fingerprint: {f['path']}")
            require(content == filtered, f"checkout filters: {f['path']}")
            require(content.replace(b"\r\n", b"\n") == blob.replace(b"\r\n", b"\n"), "normalized content")
    if args.check_git:
        require(git(root, "rev-parse", "HEAD").decode().strip() == data["manuscript"]["revision"], "manuscript HEAD")
        require(git(root, "branch", "--show-current").decode().strip() == "main", "manuscript branch")
        require(git(root, "remote", "get-url", "origin").decode().strip() == data["manuscript"]["repository"], "manuscript origin")
        require(not git(root, "status", "--porcelain=v1").strip(), "manuscript worktree is dirty")

    verified_cache = 0
    if args.source_cache:
        cache = args.source_cache.resolve()
        for source in sources.values():
            if source["cache_relative"]:
                content = (cache / source["cache_relative"]).read_bytes()
                require(digest(content) == source["sha256"], f"source fingerprint: {source['id']}")
                require(len(content.decode("utf-8").splitlines()) == source["lines"], "source line count")
                verified_cache += 1
        for item in data["acquisition_metadata"]:
            require(digest((cache / item["cache_relative"]).read_bytes()) == item["sha256"], "API metadata fingerprint")
        tag = json.loads((cache / "tag-object.json").read_text(encoding="utf-8"))
        require(tag["tag"] == "v31.0" and tag["object"]["sha"] == data["core"]["revision"], "tag dereference")
        commit = json.loads((cache / "commit.json").read_text(encoding="utf-8"))
        require(commit["sha"] == data["core"]["revision"], "Core commit API")
        require(digest((cache / "core.zip").read_bytes()) == data["core"]["archive_sha256"], "Core archive")
    standards = (here / "review-criteria.md").read_bytes()
    require(digest(standards) == next(s["sha256"] for s in sources.values() if s["key"] == "project:A003"), "A-003 changed")

    print(json.dumps({"result": "pass", "scope": "provenance, locations and internal consistency; no runtime or semantic proof",
                      "manuscript_files": len(files), "findings": len(data["findings"]),
                      "supported_claims": len(data["confirmed_claims"]), "unexecuted_groups": len(data["execution_gaps"]),
                      "severity": dict(Counter(r["severity"] for r in data["findings"])),
                      "sources": len(sources), "cache_files_verified": verified_cache, "git_checked": args.check_git},
                     ensure_ascii=False))


if __name__ == "__main__":
    main()
