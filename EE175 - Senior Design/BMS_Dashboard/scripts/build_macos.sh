#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

VERSION="${1:-$(python -c 'from backend.version import APP_VERSION; print(APP_VERSION)')}"

if [[ -f "BMS Logo.png" ]]; then
  python scripts/generate_icons.py --input "BMS Logo.png" --output-dir "assets/icons" --require-icns
else
  echo "BMS Logo.png not found; using existing assets/icons files."
fi

rm -rf build dist

pyinstaller \
  --noconfirm \
  --clean \
  --windowed \
  --name "BMSDashboard" \
  --icon "assets/icons/app_icon.icns" \
  --add-data "frontend:frontend" \
  --add-data "assets/icons:assets/icons" \
  --add-data "backend/update_helper.py:backend" \
  gui_launcher.py

KEEP_LOCALES="${KEEP_QTWEBENGINE_LOCALES:-en-US}"
python scripts/optimize_pyinstaller_bundle.py --bundle-dir "dist/BMSDashboard.app" --keep-locales "${KEEP_LOCALES}"

# Compatibility shim for older updater paths that search next to the app binary.
cp -f backend/update_helper.py "dist/BMSDashboard.app/Contents/MacOS/update_helper.py"

# Re-sign the bundle with an ad-hoc signature after modifying its contents.
# PyInstaller ad-hoc signs the bundle during build; any file added afterwards
# (like the update_helper.py shim above) invalidates that signature and causes
# macOS Gatekeeper to report the app as "damaged".
codesign --force --deep --sign - "dist/BMSDashboard.app"

APP_BUNDLE="dist/BMSDashboard.app"
DMG_PATH="dist/BMSDashboard-${VERSION}-macos-universal2.dmg"
ZIP_PATH="dist/BMSDashboard-${VERSION}-macos-universal2.zip"

if [[ -n "${APPLE_SIGN_IDENTITY:-}" ]]; then
  codesign --force --deep --options runtime --sign "$APPLE_SIGN_IDENTITY" "$APP_BUNDLE"
fi

hdiutil create -volname "BMS Dashboard" -srcfolder "$APP_BUNDLE" -ov -format UDZO -imagekey zlib-level=9 "$DMG_PATH"
ditto -c -k --sequesterRsrc --keepParent "$APP_BUNDLE" "$ZIP_PATH"

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
cp -f "$ZIP_PATH" "$RELEASE_DIR/"
cp -f "$ZIP_PATH" "$RELEASE_DIR/BMSDashboard-macos-universal2.zip"
{
  shasum -a 256 "$DMG_PATH"
  shasum -a 256 "$ZIP_PATH"
} > "$RELEASE_DIR/sha256.txt"

echo "macOS release artifacts: $DMG_PATH, $ZIP_PATH"
