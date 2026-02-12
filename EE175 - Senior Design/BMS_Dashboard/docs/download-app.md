# Download And Install BMS Dashboard

This guide is for anyone who wants to install the desktop app without running the source code.

## Direct Latest Links

- Windows `.exe`: https://github.com/kaushikvada3/UCR-Computer-Science-Electrical-Engineering/releases/latest/download/BMSDashboard-windows-x64-setup.exe
- macOS `.dmg`: https://github.com/kaushikvada3/UCR-Computer-Science-Electrical-Engineering/releases/latest/download/BMSDashboard-macos-universal2.dmg
- Linux `.AppImage`: https://github.com/kaushikvada3/UCR-Computer-Science-Electrical-Engineering/releases/latest/download/BMSDashboard-linux-x64.AppImage

## 1) Open The Releases Page

Go to:

`https://github.com/kaushikvada3/UCR-Computer-Science-Electrical-Engineering/releases`

Pick the newest release (for example: `v0.1.0`).

## 2) Download The Right File For Your OS

From the **Assets** section:

- **Windows**: `BMSDashboard-<version>-windows-x64-setup.exe`
- **macOS**: `BMSDashboard-<version>-macos-universal2.dmg`
- **Linux**: `BMSDashboard-<version>-linux-x64.AppImage`

## 3) Install / Run

### Windows

1. Double-click the `...windows-x64-setup.exe` file.
2. Follow the installer prompts.
3. Open **BMS Dashboard** from Start Menu or Desktop shortcut.

### macOS

1. Open the `...macos-universal2.dmg` file.
2. Drag **BMS Dashboard.app** into **Applications**.
3. Open it from Applications.

### Linux

1. Make the AppImage executable:
   ```bash
   chmod +x BMSDashboard-<version>-linux-x64.AppImage
   ```
2. Run it:
   ```bash
   ./BMSDashboard-<version>-linux-x64.AppImage
   ```

## 4) First Launch Setup

1. Connect the BMS hardware over USB.
2. In the app toolbar, use:
   - `Serial Port...` (choose `Auto-detect` or specific COM/tty port)
   - `Baudrate...` (typically `115200`)
3. Verify telemetry updates in the dashboard.

## 5) Updating

- The app can check for new releases.
- You can also manually install a newer release by downloading the latest file from the Releases page and running it.

## Troubleshooting

- **Windows SmartScreen warning**: if shown, click `More info` -> `Run anyway`.
- **No serial data**:
  - confirm the board is connected,
  - verify the correct serial port and baudrate,
  - close other apps that may already be using the same COM/tty port.
