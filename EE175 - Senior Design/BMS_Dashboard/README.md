# BMS Dashboard

Desktop launcher for the BMS dashboard UI built with PyQt6 + embedded web frontend.

## Download (Latest)

Direct download links (always target the newest release assets):

- Windows `.exe`: [Download Windows Installer](https://github.com/kaushikvada3/UCR-Computer-Science-Electrical-Engineering/releases/latest/download/BMSDashboard-windows-x64-setup.exe)
- macOS `.dmg`: [Download macOS DMG](https://github.com/kaushikvada3/UCR-Computer-Science-Electrical-Engineering/releases/latest/download/BMSDashboard-macos-universal2.dmg)
- Linux `.AppImage`: [Download Linux AppImage](https://github.com/kaushikvada3/UCR-Computer-Science-Electrical-Engineering/releases/latest/download/BMSDashboard-linux-x64.AppImage)

Fallback releases page:

- https://github.com/kaushikvada3/UCR-Computer-Science-Electrical-Engineering/releases

## Development (VS Code)

The source workflow remains unchanged.

```bash
pip install -r requirements.txt
python gui_launcher.py
```

Core frontend files are auto-watched and reloaded:
- `frontend/index.html`
- `frontend/scene.js`
- `frontend/style.css`
- `frontend/qwebchannel.js`

## Runtime Options

```bash
python gui_launcher.py --serial-port auto --baudrate 115200 --update-channel stable
```

Useful flags:
- `--entrypoint path/to/index.html`
- `--serial-port COM5` (or `/dev/ttyACM0`)
- `--serial-port auto`
- `--baudrate 115200`
- `--no-auto-update-check`
- `--update-repo owner/repo`

## Icons

`BMS Logo.png` is the canonical icon source.

Generate platform icons:

```bash
python scripts/generate_icons.py --input "BMS Logo.png" --output-dir assets/icons
```

Outputs:
- `assets/icons/app_icon.ico`
- `assets/icons/app_icon.icns`
- `assets/icons/app_icon.png`

## Build / Release

See `docs/release.md` for:
- local packaging commands
- signing and notarization setup
- CI release workflow
- rollback process
