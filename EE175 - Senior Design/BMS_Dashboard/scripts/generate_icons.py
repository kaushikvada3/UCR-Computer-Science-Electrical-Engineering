#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

try:
    from PIL import Image
except ImportError as exc:
    raise SystemExit("Pillow is required: pip install Pillow") from exc


ICO_SIZES = [(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
ICNS_SIZES = [16, 32, 64, 128, 256, 512, 1024]


def _save_icns_with_iconutil(image: Image.Image, output: Path) -> None:
    iconutil = shutil.which("iconutil")
    if not iconutil:
        raise RuntimeError("iconutil is not available on this system")

    with tempfile.TemporaryDirectory() as tmp_dir:
        iconset = Path(tmp_dir) / "app.iconset"
        iconset.mkdir(parents=True, exist_ok=True)

        for size in ICNS_SIZES:
            base = image.resize((size, size), Image.Resampling.LANCZOS)
            base.save(iconset / f"icon_{size}x{size}.png", format="PNG")
            if size <= 512:
                retina = image.resize((size * 2, size * 2), Image.Resampling.LANCZOS)
                retina.save(iconset / f"icon_{size}x{size}@2x.png", format="PNG")

        subprocess.run(
            [iconutil, "-c", "icns", str(iconset), "-o", str(output)],
            check=True,
        )


def generate_icons(input_path: Path, output_dir: Path, require_icns: bool) -> None:
    if not input_path.exists():
        raise FileNotFoundError(f"Input logo not found: {input_path}")

    output_dir.mkdir(parents=True, exist_ok=True)
    image = Image.open(input_path).convert("RGBA")

    png_out = output_dir / "app_icon.png"
    ico_out = output_dir / "app_icon.ico"
    icns_out = output_dir / "app_icon.icns"

    image.resize((1024, 1024), Image.Resampling.LANCZOS).save(png_out, format="PNG")
    image.save(ico_out, format="ICO", sizes=ICO_SIZES)

    icns_error = None
    try:
        image.save(icns_out, format="ICNS")
    except Exception as exc:
        icns_error = exc
        try:
            _save_icns_with_iconutil(image, icns_out)
            icns_error = None
        except Exception as iconutil_exc:
            icns_error = iconutil_exc

    if icns_error and require_icns:
        raise RuntimeError(f"Unable to generate ICNS: {icns_error}") from icns_error

    if icns_error:
        print(f"[warn] ICNS generation skipped: {icns_error}", file=sys.stderr)

    print(f"Generated icons in {output_dir}")
    print(f" - {png_out}")
    print(f" - {ico_out}")
    if icns_out.exists():
        print(f" - {icns_out}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate app icon formats from a source PNG.")
    parser.add_argument(
        "--input",
        type=Path,
        default=Path("BMS Logo.png"),
        help="Input logo PNG path (default: BMS Logo.png).",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("assets") / "icons",
        help="Output icon directory (default: assets/icons).",
    )
    parser.add_argument(
        "--require-icns",
        action="store_true",
        help="Fail if ICNS generation is unavailable.",
    )
    args = parser.parse_args()

    generate_icons(args.input, args.output_dir, args.require_icns)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
