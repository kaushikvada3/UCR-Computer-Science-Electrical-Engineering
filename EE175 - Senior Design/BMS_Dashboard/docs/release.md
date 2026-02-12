# Release Runbook

## Prerequisites

- Python 3.11
- `pip install -r requirements-dev.txt`
- Platform packaging tools:
  - Windows: NSIS (`makensis`)
  - macOS: Xcode command line tools (`codesign`, `xcrun`, `hdiutil`)
  - Linux: `appimagetool`, `gpg`

## Local Build Commands

### Windows

```powershell
./scripts/build_windows.ps1 -Version 0.1.0
```

### macOS

```bash
chmod +x scripts/build_macos.sh
./scripts/build_macos.sh 0.1.0
```

### Linux

```bash
chmod +x scripts/build_linux.sh
./scripts/build_linux.sh 0.1.0
```

## CI Secrets

Required:
- `WIN_CERT_BASE64`
- `WIN_CERT_PASSWORD`
- `APPLE_CERT_P12_BASE64`
- `APPLE_CERT_PASSWORD`
- `APPLE_ID`
- `APPLE_TEAM_ID`
- `APPLE_APP_PASSWORD`
- `LINUX_GPG_PRIVATE_KEY`
- `LINUX_GPG_PASSPHRASE`

Optional:
- `GH_TOKEN` (workflow also works with default `GITHUB_TOKEN` and `contents: write` permission)

## GitHub Release Flow

1. Push all release-ready changes to main branch.
2. Create and push a semantic version tag:
   - `git tag v0.1.0`
   - `git push origin v0.1.0`
3. Workflow `.github/workflows/release.yml` builds:
   - Windows NSIS installer
   - macOS signed/notarized DMG
   - Linux AppImage
4. Workflow signs artifacts (detached `.sig` when GPG key is available), builds `release-manifest.json`, and publishes all files to GitHub Releases.

## Manifest Contract

`release-manifest.json` includes:
- `version`
- `channel` (`stable`)
- `published_at`
- `notes`
- `assets`:
  - `windows-x64`
  - `macos-universal2`
  - `linux-x64`

Each platform asset contains:
- `url`
- `sha256`
- `signature`

## Rollback Strategy

Do not delete released tags.

To roll back:
1. Rebuild the last known-good binary set.
2. Bump patch version (example: from `v0.1.0` to `v0.1.1`).
3. Publish that known-good build as the new stable release.

Clients on stable will move to the highest stable version automatically.
