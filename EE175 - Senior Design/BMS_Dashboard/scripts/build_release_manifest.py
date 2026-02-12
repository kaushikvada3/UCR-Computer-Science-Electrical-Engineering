#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional


def sha256_of(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 256), b""):
            digest.update(chunk)
    return digest.hexdigest()


def github_asset_url(repo: str, tag: str, filename: str) -> str:
    return f"https://github.com/{repo}/releases/download/{tag}/{filename}"


def add_asset(
    manifest_assets: dict,
    platform_key: str,
    repo: str,
    tag: str,
    file_path: Optional[Path],
    sha256: Optional[str],
    signature_path: Optional[Path],
) -> None:
    if not file_path:
        return
    if not file_path.exists():
        raise FileNotFoundError(f"Missing artifact for {platform_key}: {file_path}")

    computed_sha = sha256 or sha256_of(file_path)
    payload = {
        "url": github_asset_url(repo, tag, file_path.name),
        "sha256": computed_sha,
        "signature": "",
    }
    if signature_path:
        if not signature_path.exists():
            raise FileNotFoundError(f"Missing signature for {platform_key}: {signature_path}")
        payload["signature"] = github_asset_url(repo, tag, signature_path.name)
    manifest_assets[platform_key] = payload


def main() -> int:
    parser = argparse.ArgumentParser(description="Build release-manifest.json for updater.")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--channel", default="stable")
    parser.add_argument("--published-at", default="")
    parser.add_argument("--notes", default="")
    parser.add_argument("--notes-file", type=Path, default=None)
    parser.add_argument("--repo", required=True, help="GitHub repository slug (owner/repo).")
    parser.add_argument("--tag", required=True, help="Release tag, e.g. v1.2.3")

    parser.add_argument("--windows-file", type=Path, default=None)
    parser.add_argument("--windows-sha256", default="")
    parser.add_argument("--windows-signature", type=Path, default=None)

    parser.add_argument("--macos-file", type=Path, default=None)
    parser.add_argument("--macos-sha256", default="")
    parser.add_argument("--macos-signature", type=Path, default=None)

    parser.add_argument("--linux-file", type=Path, default=None)
    parser.add_argument("--linux-sha256", default="")
    parser.add_argument("--linux-signature", type=Path, default=None)

    args = parser.parse_args()

    notes = args.notes
    if args.notes_file:
        notes = args.notes_file.read_text(encoding="utf-8")

    published_at = args.published_at or datetime.now(timezone.utc).isoformat()

    assets = {}
    add_asset(
        assets,
        "windows-x64",
        repo=args.repo,
        tag=args.tag,
        file_path=args.windows_file,
        sha256=args.windows_sha256 or None,
        signature_path=args.windows_signature,
    )
    add_asset(
        assets,
        "macos-universal2",
        repo=args.repo,
        tag=args.tag,
        file_path=args.macos_file,
        sha256=args.macos_sha256 or None,
        signature_path=args.macos_signature,
    )
    add_asset(
        assets,
        "linux-x64",
        repo=args.repo,
        tag=args.tag,
        file_path=args.linux_file,
        sha256=args.linux_sha256 or None,
        signature_path=args.linux_signature,
    )

    manifest = {
        "version": args.version,
        "channel": args.channel,
        "published_at": published_at,
        "notes": notes,
        "assets": assets,
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
