from __future__ import annotations

import ctypes
import hashlib
import json
import logging
import os
import re
import shlex
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Dict, Optional
from urllib.parse import urljoin

import requests
from packaging.version import InvalidVersion, Version

logger = logging.getLogger(__name__)


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


DEFAULT_UPDATE_REPO_SLUG = "kaushikvada3/UCR-Computer-Science-Electrical-Engineering"


def _default_update_state_root() -> Path:
    if sys.platform.startswith("win"):
        local_appdata = os.environ.get("LOCALAPPDATA", "").strip()
        base_dir = Path(local_appdata) if local_appdata else Path.home() / "AppData" / "Local"
    elif sys.platform == "darwin":
        base_dir = Path.home() / "Library" / "Application Support"
    else:
        state_home = os.environ.get("XDG_STATE_HOME", "").strip()
        base_dir = Path(state_home) if state_home else Path.home() / ".local" / "state"
    return base_dir / "BMS Dashboard"


def default_update_log_path() -> Path:
    return _default_update_state_root() / "logs" / "update.log"


def default_update_result_path() -> Path:
    return _default_update_state_root() / "updates" / "last-result.json"


def _append_update_log(message: str, log_path: Optional[Path] = None) -> Path:
    target = log_path or default_update_log_path()
    try:
        target.parent.mkdir(parents=True, exist_ok=True)
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        with target.open("a", encoding="utf-8") as fh:
            fh.write(f"[{timestamp}] {message}\n")
    except Exception:
        logger.exception("Failed to write updater log")
    return target


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
        return DEFAULT_UPDATE_REPO_SLUG

    if not output:
        return DEFAULT_UPDATE_REPO_SLUG

    ssh_match = re.match(r"git@github\.com:(?P<slug>.+?)(?:\.git)?$", output)
    if ssh_match:
        return ssh_match.group("slug")

    https_match = re.match(r"https://github\.com/(?P<slug>.+?)(?:\.git)?/?$", output)
    if https_match:
        return https_match.group("slug")

    # Packaged builds do not include .git metadata, so keep a stable fallback.
    return DEFAULT_UPDATE_REPO_SLUG


class ReleaseUpdater:
    def __init__(
        self,
        repo_slug: Optional[str],
        current_version: str,
        channel: str = "stable",
        timeout_s: int = 15,
    ):
        self.repo_slug = (repo_slug or "").strip()
        try:
            self.current_version = self._to_version(current_version)
        except UpdateError:
            # Backwards compatibility for legacy/non-semver version strings.
            self.current_version = Version("0")
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

    def download_update(
        self,
        info: UpdateInfo,
        target_dir: Optional[Path] = None,
        progress_callback: Optional[Callable[[int, int], None]] = None,
    ) -> Path:
        destination = target_dir or Path(tempfile.gettempdir()) / "BMSDashboardUpdates"
        destination.mkdir(parents=True, exist_ok=True)
        file_path = destination / info.asset.filename

        if file_path.exists():
            try:
                self.verify_sha256(file_path, info.asset.sha256)
                if progress_callback:
                    size = int(file_path.stat().st_size)
                    progress_callback(size, size)
                return file_path
            except Exception:
                file_path.unlink(missing_ok=True)

        partial_path = file_path.with_suffix(file_path.suffix + ".part")
        resume_from = partial_path.stat().st_size if partial_path.exists() else 0
        headers: Dict[str, str] = {}
        if resume_from > 0:
            headers["Range"] = f"bytes={resume_from}-"

        with self.session.get(
            info.asset.url,
            timeout=self.timeout_s,
            stream=True,
            headers=headers or None,
        ) as response:
            response.raise_for_status()
            is_resume_response = response.status_code == 206 and resume_from > 0
            if resume_from > 0 and not is_resume_response:
                # Server ignored range; restart download from scratch.
                resume_from = 0
                partial_path.unlink(missing_ok=True)

            content_length = int(response.headers.get("Content-Length") or 0)
            total_size = content_length + resume_from if content_length > 0 else 0
            downloaded = resume_from
            if progress_callback:
                progress_callback(downloaded, total_size)

            mode = "ab" if resume_from > 0 else "wb"
            with partial_path.open(mode) as fh:
                for chunk in response.iter_content(chunk_size=1024 * 1024):
                    if chunk:
                        fh.write(chunk)
                        downloaded += len(chunk)
                        if progress_callback:
                            progress_callback(downloaded, total_size)

        partial_path.replace(file_path)
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
    def launch_guided_install(
        installer_path: Path,
        wait_for_pid: Optional[int] = None,
        autoclose_app: bool = False,
        update_mode: bool = False,
        log_path: Optional[Path] = None,
    ) -> Dict[str, str]:
        installer_path = installer_path.resolve()
        log_file = _append_update_log(f"Preparing guided install launch: {installer_path}", log_path)

        if sys.platform.startswith("win"):
            launch_args: list[str] = []
            if autoclose_app:
                launch_args.append("/AUTOCLOSEAPP=1")
            if wait_for_pid and wait_for_pid > 0:
                launch_args.append(f"/WAITPID={int(wait_for_pid)}")
            if update_mode:
                launch_args.append("/UPDATE_MODE=1")

            # ShellExecute handles UAC elevation prompts for installers.
            parameters = " ".join(launch_args) if launch_args else None
            _append_update_log(
                f"Launching installer via ShellExecuteW. Args: {parameters or '(none)'}",
                log_file,
            )
            result = ctypes.windll.shell32.ShellExecuteW(
                None,
                "open",
                str(installer_path),
                parameters,
                None,
                1,
            )
            if result <= 32:
                _append_update_log(f"ShellExecuteW failed with code {result}", log_file)
                try:
                    subprocess.Popen(["explorer", "/select,", str(installer_path)])
                except Exception:
                    pass
                return {
                    "status": "manual_required",
                    "message": (
                        "Installer downloaded but could not be auto-launched.\n"
                        f"Please run it manually:\n{installer_path}\n\n"
                        f"Updater log: {log_file}"
                    ),
                    "log_path": str(log_file),
                    "installer_path": str(installer_path),
                }
            _append_update_log("Installer launch dispatched successfully", log_file)
            if wait_for_pid and wait_for_pid > 0 and not autoclose_app:
                return {
                    "status": "close_required",
                    "message": (
                        "Installer window is open and waiting for BMS Dashboard to close.\n\n"
                        "Close the app now to continue installation. If Windows keeps files "
                        "locked, setup may ask for a restart."
                    ),
                    "log_path": str(log_file),
                    "installer_path": str(installer_path),
                }
            return {
                "status": "launched",
                "message": (
                    "Installer launched. Approve UAC if prompted, then follow the setup window.\n\n"
                    "Setup will close running BMS Dashboard processes automatically and may "
                    "request a restart if Windows still locks files."
                ),
                "log_path": str(log_file),
                "installer_path": str(installer_path),
            }

        if sys.platform == "darwin":
            subprocess.Popen(["open", str(installer_path)])
            return {
                "status": "launched",
                "message": "DMG opened. Drag BMS Dashboard to Applications, then relaunch.",
            }

        # Linux AppImage flow
        installer_path.chmod(installer_path.stat().st_mode | 0o111)
        subprocess.Popen([str(installer_path)])
        return {
            "status": "launched",
            "message": "AppImage launched. Confirm and restart into the new version.",
        }

    @staticmethod
    def install_update_and_restart(
        installer_path: Path,
        app_bundle_path: Optional[Path] = None,
        log_path: Optional[Path] = None,
        version: str = "",
        result_path: Optional[Path] = None,
    ) -> Dict[str, str]:
        installer_path = installer_path.resolve()
        log_file = _append_update_log(f"Preparing seamless update: {installer_path}", log_path)

        if not getattr(sys, "frozen", False):
            return {
                "status": "error",
                "message": "Dev/source mode updates are via git/pull; app self-update is disabled.",
                "log_path": str(log_file),
            }

        if app_bundle_path is None:
            if sys.platform == "darwin":
                exe_path = Path(sys.executable).resolve()
                for parent in exe_path.parents:
                    if parent.suffix == ".app":
                        app_bundle_path = parent
                        break
                if app_bundle_path is None:
                    return {
                        "status": "error",
                        "message": "Could not determine installed .app bundle path.",
                        "log_path": str(log_file),
                    }
            else:
                app_bundle_path = Path(sys.executable).resolve()

        if sys.platform == "darwin":
            helper_result_path = (result_path or default_update_result_path()).expanduser()
            helper_args = [
                str(Path(sys.executable).resolve()),
                "--run-update-helper",
                "--installer",
                str(installer_path),
                "--target-app",
                str(app_bundle_path),
                "--wait-pid",
                str(os.getpid()),
                "--relaunch",
                "--result-file",
                str(helper_result_path),
            ]
            if version:
                helper_args.extend(["--update-version", str(version)])
            _append_update_log(f"Launching bundled helper mode: {' '.join(helper_args)}", log_file)

            needs_elevation = not os.access(app_bundle_path.parent, os.W_OK)
            try:
                log_file.parent.mkdir(parents=True, exist_ok=True)
                with log_file.open("ab") as log_stream:
                    if needs_elevation:
                        command = " ".join(shlex.quote(arg) for arg in helper_args)
                        script = f"do shell script {json.dumps(command)} with administrator privileges"
                        _append_update_log(
                            "Install location is protected; requesting administrator privileges.",
                            log_file,
                        )
                        subprocess.Popen(
                            ["osascript", "-e", script],
                            stdout=log_stream,
                            stderr=log_stream,
                            start_new_session=True,
                        )
                    else:
                        subprocess.Popen(
                            helper_args,
                            stdout=log_stream,
                            stderr=log_stream,
                            start_new_session=True,
                        )
            except Exception as exc:
                _append_update_log(f"Failed to launch bundled helper mode: {exc}", log_file)
                return {
                    "status": "error",
                    "message": f"Failed to launch updater helper mode: {exc}",
                    "log_path": str(log_file),
                }

            _append_update_log("Bundled helper mode launched successfully", log_file)
            if needs_elevation:
                return {
                    "status": "permission_prompt",
                    "message": "Administrator permission requested. Applying update in place.",
                    "log_path": str(log_file),
                    "requires_quit": "true",
                    "installer_path": str(installer_path),
                    "result_path": str(helper_result_path),
                }
            return {
                "status": "installing",
                "message": "Installing update in place and restarting application.",
                "log_path": str(log_file),
                "requires_quit": "true",
                "installer_path": str(installer_path),
                "result_path": str(helper_result_path),
            }

        launch_result = ReleaseUpdater.launch_guided_install(
            installer_path,
            wait_for_pid=os.getpid(),
            autoclose_app=False,
            update_mode=True,
            log_path=log_file,
        )
        launch_status = str(launch_result.get("status", "")).strip().lower()
        if launch_status == "manual_required":
            return {
                "status": "error",
                "message": str(launch_result.get("message", "Unable to launch installer.")),
                "log_path": str(launch_result.get("log_path", log_file)),
                "installer_path": str(launch_result.get("installer_path", installer_path)),
            }

        return {
            "status": "installing",
            "message": str(launch_result.get("message", "Installing update.")),
            "log_path": str(launch_result.get("log_path", log_file)),
            "installer_path": str(launch_result.get("installer_path", installer_path)),
            "requires_quit": "true" if launch_status == "close_required" else "false",
        }
