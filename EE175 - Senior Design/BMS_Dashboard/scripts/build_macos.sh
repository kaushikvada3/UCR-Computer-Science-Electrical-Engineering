#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

VERSION="${1:-$(python -c 'from backend.version import APP_VERSION; print(APP_VERSION)')}"

python scripts/generate_icons.py --input "BMS Logo.png" --output-dir "assets/icons" --require-icns

rm -rf build dist

pyinstaller \
  --noconfirm \
  --clean \
  --windowed \
  --name "BMSDashboard" \
  --icon "assets/icons/app_icon.icns" \
  --add-data "frontend:frontend" \
  --add-data "assets/icons:assets/icons" \
  gui_launcher.py

APP_BUNDLE="dist/BMSDashboard.app"
DMG_PATH="dist/BMSDashboard-${VERSION}-macos-universal2.dmg"

if [[ -n "${APPLE_SIGN_IDENTITY:-}" ]]; then
  codesign --force --deep --options runtime --sign "$APPLE_SIGN_IDENTITY" "$APP_BUNDLE"
fi

hdiutil create -volname "BMS Dashboard" -srcfolder "$APP_BUNDLE" -ov -format UDZO "$DMG_PATH"

if [[ -n "${APPLE_SIGN_IDENTITY:-}" ]]; then
  codesign --force --sign "$APPLE_SIGN_IDENTITY" "$DMG_PATH"
fi

if [[ -n "${APPLE_ID:-}" && -n "${APPLE_TEAM_ID:-}" && -n "${APPLE_APP_PASSWORD:-}" ]]; then
  xcrun notarytool submit "$DMG_PATH" \
    --apple-id "$APPLE_ID" \
    --team-id "$APPLE_TEAM_ID" \
    --password "$APPLE_APP_PASSWORD" \
    --wait
  xcrun stapler staple "$APP_BUNDLE" || true
  xcrun stapler staple "$DMG_PATH" || true
fi

RELEASE_DIR="dist/release/macos-universal2"
mkdir -p "$RELEASE_DIR"
cp -f "$DMG_PATH" "$RELEASE_DIR/"
cp -f "$DMG_PATH" "$RELEASE_DIR/BMSDashboard-macos-universal2.dmg"
shasum -a 256 "$DMG_PATH" > "$RELEASE_DIR/sha256.txt"

echo "macOS release artifact: $DMG_PATH"
