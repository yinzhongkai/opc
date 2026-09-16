#!/usr/bin/env python3
"""Serve only the USER-01 app and ignored evaluation workspace on loopback."""

from __future__ import annotations

import argparse
import mimetypes
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import unquote, urlsplit


SOURCE_ROOT = Path(__file__).resolve().parents[3]
APP_ROOT = Path(__file__).resolve().parent / "user01-app"
DEFAULT_WORKSPACE_ROOT = SOURCE_ROOT / "out/evaluation/T-029/user01"
WORKSPACE_URL_PREFIX = "/out/evaluation/T-029/user01/"


def safe_child(root: Path, relative: str) -> Path | None:
    try:
        candidate = (root / relative).resolve()
        candidate.relative_to(root.resolve())
    except (OSError, ValueError):
        return None
    return candidate


def make_handler(workspace_root: Path):
    class Handler(BaseHTTPRequestHandler):
        server_version = "SpaceRhythmUser01/0.1"

        def resolve_target(self) -> Path | None:
            path = unquote(urlsplit(self.path).path)
            if path in {"/", "/index.html"}:
                return APP_ROOT / "index.html"
            elif path in {"/app.js", "/styles.css"}:
                return APP_ROOT / path.removeprefix("/")
            elif path.startswith(WORKSPACE_URL_PREFIX):
                return safe_child(workspace_root, path.removeprefix(WORKSPACE_URL_PREFIX))
            return None

        def serve_target(self, include_body: bool) -> None:
            target = self.resolve_target()
            if target is None or not target.is_file():
                self.send_error(404)
                return
            size = target.stat().st_size
            start, end = 0, size - 1
            range_header = self.headers.get("Range")
            if range_header:
                if not range_header.startswith("bytes=") or "," in range_header:
                    self.send_error(416)
                    return
                first, _, last = range_header.removeprefix("bytes=").partition("-")
                try:
                    if first:
                        start = int(first)
                        end = int(last) if last else size - 1
                    else:
                        suffix_length = int(last)
                        start = max(0, size - suffix_length)
                except ValueError:
                    self.send_error(416)
                    return
                if start < 0 or end < start or start >= size:
                    self.send_error(416)
                    return
                end = min(end, size - 1)
            content_type, _ = mimetypes.guess_type(target.name)
            self.send_response(206 if range_header else 200)
            self.send_header("Content-Type", content_type or "application/octet-stream")
            self.send_header("Content-Length", str(end - start + 1))
            self.send_header("Accept-Ranges", "bytes")
            if range_header:
                self.send_header("Content-Range", f"bytes {start}-{end}/{size}")
            self.send_header("Cache-Control", "no-store")
            self.send_header("X-Content-Type-Options", "nosniff")
            self.send_header("Content-Security-Policy", "default-src 'self'; media-src 'self'; script-src 'self'; style-src 'self'; object-src 'none'; base-uri 'none'")
            self.end_headers()
            if not include_body:
                return
            remaining = end - start + 1
            try:
                with target.open("rb") as stream:
                    stream.seek(start)
                    while remaining:
                        block = stream.read(min(1024 * 1024, remaining))
                        if not block:
                            break
                        self.wfile.write(block)
                        remaining -= len(block)
            except (BrokenPipeError, ConnectionResetError):
                pass

        def do_GET(self) -> None:  # noqa: N802 - stdlib handler API
            self.serve_target(include_body=True)

        def do_HEAD(self) -> None:  # noqa: N802 - stdlib handler API
            self.serve_target(include_body=False)

        def log_message(self, fmt: str, *args: object) -> None:
            print(f"USER01_HTTP {self.address_string()} {fmt % args}")

    return Handler


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bind", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--workspace-root", type=Path, default=DEFAULT_WORKSPACE_ROOT)
    args = parser.parse_args()
    if args.bind not in {"127.0.0.1", "::1", "localhost"}:
        raise ValueError("USER-01 server may bind only to the local loopback interface")
    workspace_root = args.workspace_root.resolve()
    expected_root = DEFAULT_WORKSPACE_ROOT.resolve()
    if workspace_root != expected_root:
        raise ValueError(f"workspace root must be exactly {expected_root}")
    server = ThreadingHTTPServer((args.bind, args.port), make_handler(workspace_root))
    print(f"USER01_SERVER=READY url=http://{args.bind}:{args.port}/index.html")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
