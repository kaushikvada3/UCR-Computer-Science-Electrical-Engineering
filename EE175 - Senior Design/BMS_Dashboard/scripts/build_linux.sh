#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

VERSION="${1:-$(python -c 'from backend.version import APP_VERSION; print(APP_VERSION)')}"

if [[ -f "BMS Logo.png" ]]; then
  python scripts/generate_icons.py --input "BMS Logo.png" --output-dir "assets/icons"
else
  echo "BMS Logo.png not found; using existing assets/icons files."
fi

rm -rf build dist

pyinstaller \
  --noconfirm \
  --clean \
  --onedir \
  --name "BMSDashboard" \
  --icon "assets/icons/app_icon.png" \
  --add-data "frontend:frontend" \
  --add-data "assets/icons:assets/icons" \
  --add-data "backend/update_helper.py:backend" \
  gui_launcher.py

KEEP_LOCALES="${KEEP_QTWEBENGINE_LOCALES:-en-US}"
python scripts/optimize_pyinstaller_bundle.py --bundle-dir "dist/BMSDashboard" --keep-locales "${KEEP_LOCALES}"

# Compatibility shim for older updater paths that search next to the executable.
cp -f backend/update_helper.py dist/BMSDashboard/update_helper.py

APPDIR="dist/AppDir"
mkdir -p "$APPDIR/usr/lib/bms-dashboard" "$APPDIR/usr/share/icons/hicolor/256x256/apps"
cp -R dist/BMSDashboard/* "$APPDIR/usr/lib/bms-dashboard/"
cp -f assets/icons/app_icon.png "$APPDIR/usr/share/icons/hicolor/256x256/apps/bms-dashboard.png"
cp -f assets/icons/app_icon.png "$APPDIR/bms-dashboard.png"

cat > "$APPDIR/BMSDashboard.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=BMS Dashboard
Exec=BMSDashboard
Icon=bms-dashboard
Categories=Utility;Development;
Terminal=false
EOF

cat > "$APPDIR/AppRun" <<'EOF'
#!/usr/bin/env bash
HERE="$(cd "$(dirname "$0")" && pwd)"
exec "$HERE/usr/lib/bms-dashboard/BMSDashboard" "$@"
EOF
chmod +x "$APPDIR/AppRun"

APPIMAGETOOL_BIN="${APPIMAGETOOL:-}"
if [[ -z "$APPIMAGETOOL_BIN" ]]; then
  if command -v appimagetool >/dev/null 2>&1; then
    APPIMAGETOOL_BIN="$(command -v appimagetool)"
  else
    mkdir -p "$ROOT_DIR/tools"
    APPIMAGETOOL_BIN="$ROOT_DIR/tools/appimagetool.AppImage"
    if [[ ! -f "$APPIMAGETOOL_BIN" ]]; then
      curl -L -o "$APPIMAGETOOL_BIN" \
        https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
      chmod +x "$APPIMAGETOOL_BIN"
    fi
  fi
fi

APPIMAGE_OUT="dist/BMSDashboard-${VERSION}-linux-x64.AppImage"
if [[ "$APPIMAGETOOL_BIN" == *.AppImage ]]; then
  chmod +x "$APPIMAGETOOL_BIN"
fi

if ! ARCH=x86_64 "$APPIMAGETOOL_BIN" "$APPDIR" "$APPIMAGE_OUT"; then
  if [[ "$APPIMAGETOOL_BIN" == *.AppImage ]]; then
    echo "appimagetool failed in FUSE mode, retrying with APPIMAGE_EXTRACT_AND_RUN=1"
    ARCH=x86_64 APPIMAGE_EXTRACT_AND_RUN=1 "$APPIMAGETOOL_BIN" "$APPDIR" "$APPIMAGE_OUT"
  else
    echo "appimagetool failed and no AppImage fallback is available" >&2
    exit 1
  fi
fi

if [[ -n "${LINUX_GPG_KEY_ID:-}" ]] && command -v gpg >/dev/null 2>&1; then
  gpg --batch --yes --detach-sign --armor -u "$LINUX_GPG_KEY_ID" "$APPIMAGE_OUT"
fi

RELEASE_DIR="dist/release/linux-x64"
mkdir -p "$RELEASE_DIR"
cp -f "$APPIMAGE_OUT" "$RELEASE_DIR/"
cp -f "$APPIMAGE_OUT" "$RELEASE_DIR/BMSDashboard-linux-x64.AppImage"
if [[ -f "${APPIMAGE_OUT}.asc" ]]; then
  cp -f "${APPIMAGE_OUT}.asc" "$RELEASE_DIR/"
fi
sha256sum "$APPIMAGE_OUT" > "$RELEASE_DIR/sha256.txt"

echo "Linux release artifact: $APPIMAGE_OUT"
