param(
    [string]$Version = ""
)

$ErrorActionPreference = "Stop"

$projectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $projectRoot

if (-not $Version) {
    $Version = (python -c "from backend.version import APP_VERSION; print(APP_VERSION)").Trim()
}

$logoPath = Join-Path $projectRoot "BMS Logo.png"
if (Test-Path $logoPath) {
    python scripts/generate_icons.py --input "BMS Logo.png" --output-dir "assets/icons"
} else {
    Write-Host "BMS Logo.png not found; using existing assets/icons files."
}

Remove-Item -Recurse -Force build, dist -ErrorAction SilentlyContinue

pyinstaller `
  --noconfirm `
  --clean `
  --windowed `
  --name "BMSDashboard" `
  --icon "assets/icons/app_icon.ico" `
  --add-data "frontend;frontend" `
  --add-data "assets/icons;assets/icons" `
  --add-data "backend/update_helper.py;backend" `
  gui_launcher.py

$keepLocales = if ($env:KEEP_QTWEBENGINE_LOCALES) { $env:KEEP_QTWEBENGINE_LOCALES } else { "en-US" }
python scripts/optimize_pyinstaller_bundle.py --bundle-dir "dist/BMSDashboard" --keep-locales "$keepLocales"

# Compatibility shim for older updater paths that search next to BMSDashboard.exe.
Copy-Item -Force "backend/update_helper.py" "dist/BMSDashboard/update_helper.py"

$appDir = (Resolve-Path "dist/BMSDashboard").Path
$appExe = (Resolve-Path "dist/BMSDashboard/BMSDashboard.exe").Path
$installerOut = Join-Path $projectRoot "dist/BMSDashboard-$Version-windows-x64-setup.exe"
$iconPath = (Resolve-Path "assets/icons/app_icon.ico").Path

function Sign-Target {
    param([string]$TargetPath)
    $certB64 = $env:WIN_CERT_BASE64
    $certPass = $env:WIN_CERT_PASSWORD
    if (-not $certB64 -or -not $certPass) {
        Write-Host "Skipping code signing for $TargetPath (WIN_CERT_* not set)"
        return
    }

    $signtool = Get-Command signtool -ErrorAction SilentlyContinue
    if (-not $signtool) {
        Write-Warning "signtool not found; skipping signature for $TargetPath"
        return
    }

    $tempCert = Join-Path $env:TEMP "bms-signing-cert.pfx"
    [IO.File]::WriteAllBytes($tempCert, [Convert]::FromBase64String($certB64))
    try {
        & $signtool.Source sign /fd SHA256 /f $tempCert /p $certPass /tr http://timestamp.digicert.com /td SHA256 $TargetPath
    }
    finally {
        Remove-Item -Force $tempCert -ErrorAction SilentlyContinue
    }
}

Sign-Target -TargetPath $appExe

function Resolve-MakeNsis {
    $cmd = Get-Command makensis -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $portableMakensis = Join-Path $projectRoot "tools\\nsis\\makensis.exe"
    if (Test-Path $portableMakensis) {
        return $portableMakensis
    }

    $toolsDir = Join-Path $projectRoot "tools"
    New-Item -ItemType Directory -Path $toolsDir -Force | Out-Null
    $zipPath = Join-Path $toolsDir "nsis-3.11.zip"
    $extractDir = Join-Path $toolsDir "nsis-extract"
    $downloadUrl = "https://prdownloads.sourceforge.net/nsis/nsis-3.11.zip"

    Write-Host "makensis not found. Downloading portable NSIS..."
    @"
import requests
url = r"$downloadUrl"
out = r"$zipPath"
r = requests.get(url, allow_redirects=True, timeout=60)
r.raise_for_status()
with open(out, "wb") as f:
    f.write(r.content)
"@ | python -
    if (Test-Path $extractDir) {
        Remove-Item -Recurse -Force $extractDir
    }
    Expand-Archive -Path $zipPath -DestinationPath $extractDir -Force

    $found = Get-ChildItem -Path $extractDir -Recurse -Filter "makensis.exe" | Select-Object -First 1
    if (-not $found) {
        throw "Portable NSIS download completed but makensis.exe was not found."
    }

    $targetDir = Split-Path $portableMakensis -Parent
    New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
    Copy-Item -Recurse -Force (Split-Path $found.FullName -Parent) $targetDir

    if (-not (Test-Path $portableMakensis)) {
        $found = Get-ChildItem -Path $targetDir -Recurse -Filter "makensis.exe" | Select-Object -First 1
        if ($found) {
            return $found.FullName
        }
        throw "Portable NSIS install failed: makensis.exe not found in tools/nsis."
    }
    return $portableMakensis
}

$makensisPath = Resolve-MakeNsis
& $makensisPath `
  "/V2" `
  "/DAPP_VERSION=$Version" `
  "/DAPP_DIST_DIR=$appDir" `
  "/DOUT_FILE=$installerOut" `
  "/DICON_FILE=$iconPath" `
  "packaging/windows/installer.nsi"

Sign-Target -TargetPath $installerOut

$releaseDir = Join-Path $projectRoot "dist/release/windows-x64"
New-Item -ItemType Directory -Path $releaseDir -Force | Out-Null

Copy-Item -Force $installerOut $releaseDir
$latestInstaller = Join-Path $releaseDir "BMSDashboard-windows-x64-setup.exe"
Copy-Item -Force $installerOut $latestInstaller
$sha = (Get-FileHash $installerOut -Algorithm SHA256).Hash.ToLower()
Set-Content -Encoding ASCII (Join-Path $releaseDir "sha256.txt") "$sha  $(Split-Path -Leaf $installerOut)"

Write-Host "Windows release artifact: $installerOut"
