from __future__ import annotations

import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Optional
from urllib.parse import urljoin

import requests
from packaging.version import InvalidVersion, Version


class UpdateError(RuntimeError):
    pass


@dataclass
class UpdateAsset:
    platform_key: str
    url: str
    sha256: str
    signature: str = ""

    @property
    def filename(self) -> str:
        return Path(self.url.split("?")[0]).name


@dataclass
class UpdateInfo:
    version: str
    published_at: str
    notes: str
    channel: str
    asset: UpdateAsset
    manifest_url: str
    tag_name: str


def detect_repo_slug(project_root: Optional[Path] = None) -> Optional[str]:
    env_value = os.environ.get("BMS_UPDATE_REPO", "").strip()
    if env_value:
        return env_value

    try:
        cmd = ["git", "config", "--get", "remote.origin.url"]
        output = subprocess.check_output(
            cmd,
            cwd=str(project_root) if project_root else None,
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip()
    except Exception:
        return None

    if not output:
        return None

    ssh_match = re.match(r"git@github\.com:(?P<slug>.+?)(?:\.git)?$", output)
    if ssh_match:
        return ssh_match.group("slug")

    https_match = re.match(r"https://github\.com/(?P<slug>.+?)(?:\.git)?/?$", output)
    if https_match:
        return https_match.group("slug")

    return None


class ReleaseUpdater:
    def __init__(
        self,
        repo_slug: Optional[str],
        current_version: str,
        channel: str = "stable",
        timeout_s: int = 15,
    ):
        self.repo_slug = (repo_slug or "").strip()
        self.current_version = self._to_version(current_version)
        self.channel = (channel or "stable").strip().lower() or "stable"
        self.timeout_s = timeout_s
        self.session = requests.Session()
        self.session.headers.update({"User-Agent": "BMSDashboardUpdater/1.0"})
        token = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN")
        if token:
            self.session.headers.update({"Authorization": f"Bearer {token}"})

    @staticmethod
    def platform_key() -> str:
        if sys.platform.startswith("win"):
            return "windows-x64"
        if sys.platform == "darwin":
            return "macos-universal2"
        return "linux-x64"

    @staticmethod
    def _to_version(value: str) -> Version:
        text = str(value).strip()
        if text.startswith("v"):
            text = text[1:]
        try:
            return Version(text)
        except InvalidVersion as exc:
            raise UpdateError(f"Invalid version string: {value}") from exc

    def _fetch_releases(self) -> list[Dict[str, Any]]:
        if not self.repo_slug:
            return []
        url = f"https://api.github.com/repos/{self.repo_slug}/releases"
        response = self.session.get(url, timeout=self.timeout_s)
        response.raise_for_status()
        payload = response.json()
        if not isinstance(payload, list):
            raise UpdateError("Unexpected release API response")
        return payload

    def _fetch_manifest(self, manifest_url: str) -> Dict[str, Any]:
        response = self.session.get(manifest_url, timeout=self.timeout_s)
        response.raise_for_status()
        payload = response.json()
        if not isinstance(payload, dict):
            raise UpdateError("Invalid manifest content")
        return payload

    def check_for_update(self) -> Optional[UpdateInfo]:
        platform_key = self.platform_key()
        releases = self._fetch_releases()
        if not releases:
            return None

        for release in releases:
            if release.get("draft") or release.get("prerelease"):
                continue

            tag_name = str(release.get("tag_name", "")).strip()
            if not tag_name:
                continue

            try:
                release_version = self._to_version(tag_name)
            except UpdateError:
                continue

            if release_version <= self.current_version:
                continue

            assets = release.get("assets", [])
            manifest_asset = None
            for asset in assets:
                if asset.get("name") == "release-manifest.json":
                    manifest_asset = asset
                    break
            if not manifest_asset:
                continue

            manifest_url = str(manifest_asset.get("browser_download_url", "")).strip()
            if not manifest_url:
                continue

            manifest = self._fetch_manifest(manifest_url)
            manifest_channel = str(manifest.get("channel", "stable")).strip().lower()
            if manifest_channel != self.channel:
                continue

            assets_map = manifest.get("assets")
            if not isinstance(assets_map, dict):
                continue

            target = assets_map.get(platform_key)
            if not isinstance(target, dict):
                continue

            url = str(target.get("url", "")).strip()
            sha256 = str(target.get("sha256", "")).strip().lower()
            signature = str(target.get("signature", "")).strip()
            if not url or not sha256:
                continue

            # Accept relative URLs in manifest.
            url = urljoin(manifest_url, url)
            if signature:
                signature = urljoin(manifest_url, signature)

            return UpdateInfo(
                version=str(manifest.get("version", str(release_version))),
                published_at=str(
                    manifest.get("published_at", release.get("published_at", ""))
                ),
                notes=str(manifest.get("notes", release.get("body", ""))),
                channel=manifest_channel,
                asset=UpdateAsset(
                    platform_key=platform_key,
                    url=url,
                    sha256=sha256,
                    signature=signature,
                ),
                manifest_url=manifest_url,
                tag_name=tag_name,
            )

        return None

    def download_update(self, info: UpdateInfo, target_dir: Optional[Path] = None) -> Path:
        destination = target_dir or Path(tempfile.gettempdir()) / "BMSDashboardUpdates"
        destination.mkdir(parents=True, exist_ok=True)
        file_path = destination / info.asset.filename

        with self.session.get(info.asset.url, timeout=self.timeout_s, stream=True) as response:
            response.raise_for_status()
            with file_path.open("wb") as fh:
                for chunk in response.iter_content(chunk_size=1024 * 256):
                    if chunk:
                        fh.write(chunk)

        self.verify_sha256(file_path, info.asset.sha256)
        if info.asset.signature:
            self.verify_signature(file_path, info.asset.signature)

        return file_path

    @staticmethod
    def verify_sha256(path: Path, expected_sha256: str) -> None:
        digest = hashlib.sha256()
        with path.open("rb") as fh:
            for chunk in iter(lambda: fh.read(1024 * 256), b""):
                digest.update(chunk)
        actual = digest.hexdigest().lower()
        if actual != expected_sha256.lower():
            raise UpdateError(
                f"SHA256 mismatch for {path.name}: expected {expected_sha256}, got {actual}"
            )

    def verify_signature(self, file_path: Path, signature_url: str) -> None:
        sig_path = file_path.with_suffix(file_path.suffix + ".sig")
        with self.session.get(signature_url, timeout=self.timeout_s, stream=True) as response:
            response.raise_for_status()
            with sig_path.open("wb") as fh:
                for chunk in response.iter_content(chunk_size=1024 * 64):
                    if chunk:
                        fh.write(chunk)

        try:
            result = subprocess.run(
                ["gpg", "--verify", str(sig_path), str(file_path)],
                capture_output=True,
                text=True,
                check=False,
            )
        except FileNotFoundError as exc:
            raise UpdateError("GPG is required for signature verification but was not found") from exc

        if result.returncode != 0:
            raise UpdateError(
                "Signature verification failed: "
                + (result.stderr.strip() or result.stdout.strip() or "unknown error")
            )

    @staticmethod
    def launch_guided_install(installer_path: Path) -> str:
        if sys.platform.startswith("win"):
            subprocess.Popen([str(installer_path)], shell=False)
            return "Installer launched. Complete setup, then reopen BMS Dashboard."

        if sys.platform == "darwin":
            subprocess.Popen(["open", str(installer_path)])
            return "DMG opened. Drag BMS Dashboard to Applications, then relaunch."

        # Linux AppImage flow
        installer_path.chmod(installer_path.stat().st_mode | 0o111)
        subprocess.Popen([str(installer_path)])
        return "AppImage launched. Confirm and restart into the new version."
