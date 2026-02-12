#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


def parse_keep_locales(raw: str) -> set[str]:
    items = set()
    for token in raw.split(","):
        name = token.strip()
        if not name:
            continue
        if name.lower().endswith(".pak"):
            name = name[:-4]
        items.add(name)
    if not items:
        items.add("en-US")
    return items


def iter_locale_dirs(bundle_dir: Path) -> list[Path]:
    candidates = [
        bundle_dir / "_internal" / "PyQt6" / "Qt6" / "translations" / "qtwebengine_locales",
        bundle_dir / "PyQt6" / "Qt6" / "translations" / "qtwebengine_locales",
        bundle_dir
        / "Contents"
        / "Frameworks"
        / "PyQt6"
        / "Qt6"
        / "translations"
        / "qtwebengine_locales",
    ]
    return [path for path in candidates if path.is_dir()]


def prune_locales(bundle_dir: Path, keep_locales: set[str], dry_run: bool) -> tuple[int, int]:
    removed_files = 0
    removed_bytes = 0
    locale_dirs = iter_locale_dirs(bundle_dir)
    if not locale_dirs:
        print(f"[warn] No qtwebengine_locales directory found under {bundle_dir}")
        return 0, 0

    keep_files = {f"{name}.pak" for name in keep_locales}
    for locale_dir in locale_dirs:
        print(f"[info] Processing locales in {locale_dir}")
        for locale_file in locale_dir.glob("*.pak"):
            if locale_file.name in keep_files:
                continue
            size = locale_file.stat().st_size
            print(f"[remove] {locale_file.name} ({size} bytes)")
            removed_files += 1
            removed_bytes += size
            if not dry_run:
                locale_file.unlink(missing_ok=True)
    return removed_files, removed_bytes


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Trim PyInstaller bundle size by pruning Qt WebEngine locale files."
    )
    parser.add_argument(
        "--bundle-dir",
        type=Path,
        required=True,
        help="Path to the built bundle root (e.g., dist/BMSDashboard or dist/BMSDashboard.app).",
    )
    parser.add_argument(
        "--keep-locales",
        default="en-US",
        help="Comma-separated locale names to keep (default: en-US).",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be removed without deleting files.",
    )
    args = parser.parse_args()

    bundle_dir = args.bundle_dir.resolve()
    if not bundle_dir.exists():
        raise FileNotFoundError(f"Bundle directory not found: {bundle_dir}")

    keep_locales = parse_keep_locales(args.keep_locales)
    print(f"[info] Keeping locales: {', '.join(sorted(keep_locales))}")
    removed_files, removed_bytes = prune_locales(bundle_dir, keep_locales, args.dry_run)
    print(f"[summary] Removed {removed_files} files, reclaimed {removed_bytes} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
