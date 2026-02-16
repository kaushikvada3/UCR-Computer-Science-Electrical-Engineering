#!/usr/bin/env python3
"""Background update helper used by packaged app runs."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile
from datetime import datetime, timezone
from pathlib import Path


def wait_for_pid(pid: int, timeout: int = 30) -> bool:
    start = time.time()
    while time.time() - start < timeout:
        try:
            os.kill(pid, 0)
        except (OSError, ProcessLookupError):
            return True
        time.sleep(0.5)
    return False


def mount_dmg(dmg_path: Path) -> tuple[bool, str]:
    try:
        result = subprocess.run(
            ["hdiutil", "attach", "-nobrowse", "-quiet", str(dmg_path)],
            capture_output=True,
            text=True,
            check=True,
        )
    except subprocess.CalledProcessError as exc:
        print(f"Failed to mount DMG: {exc}", file=sys.stderr)
        return False, ""

    text = (result.stdout or "") + "\n" + (result.stderr or "")
    for line in text.splitlines():
        if "/Volumes/" in line:
            mount_point = line[line.index("/Volumes/"):].split("\t")[0].strip()
            if mount_point:
                return True, mount_point
    return False, ""


def unmount_dmg(mount_point: str) -> bool:
    try:
        subprocess.run(
            ["hdiutil", "detach", mount_point, "-quiet"],
            capture_output=True,
            check=True,
        )
        return True
    except subprocess.CalledProcessError:
        return False


def _find_app_bundle(root: Path) -> Path | None:
    direct = sorted(root.glob("*.app"))
    if direct:
        return direct[0]
    for entry in sorted(root.iterdir()):
        if not entry.is_dir():
            continue
        nested = sorted(entry.glob("*.app"))
        if nested:
            return nested[0]
    return None


def _ensure_writable_target(target_app: Path) -> None:
    parent = target_app.parent
    if not parent.exists():
        raise RuntimeError(f"Install location does not exist: {parent}")
    if not os.access(parent, os.W_OK):
        raise PermissionError(f"No write permission for install location: {parent}")


def atomic_replace_app_bundle(source_app: Path, target_app: Path) -> None:
    _ensure_writable_target(target_app)
    if not source_app.exists():
        raise FileNotFoundError(f"Source app not found: {source_app}")

    target_parent = target_app.parent
    staging_app = target_parent / f"{target_app.name}.new"
    backup_app = target_parent / f"{target_app.name}.old"

    for stale in (staging_app, backup_app):
        if stale.exists():
            if stale.is_dir():
                shutil.rmtree(stale)
            else:
                stale.unlink()

    try:
        shutil.copytree(source_app, staging_app, symlinks=True)
        if not (staging_app / "Contents" / "Info.plist").exists():
            raise RuntimeError("Staged bundle is incomplete")

        if target_app.exists():
            target_app.rename(backup_app)

        staging_app.rename(target_app)

        if backup_app.exists():
            try:
                shutil.rmtree(backup_app)
            except Exception:
                # Backup cleanup should not fail the completed swap.
                pass
    except Exception:
        try:
            if staging_app.exists():
                shutil.rmtree(staging_app)
        except Exception:
            pass

        try:
            if backup_app.exists() and not target_app.exists():
                backup_app.rename(target_app)
        except Exception:
            pass
        raise


def replace_windows_exe(source_exe: Path, target_exe: Path) -> None:
    backup_path = target_exe.with_suffix(".exe.old")
    if backup_path.exists():
        backup_path.unlink()

    try:
        if target_exe.exists():
            shutil.move(str(target_exe), str(backup_path))
        shutil.copy2(str(source_exe), str(target_exe))
        if backup_path.exists():
            backup_path.unlink()
    except Exception:
        if backup_path.exists() and not target_exe.exists():
            shutil.move(str(backup_path), str(target_exe))
        raise


def _install_from_zip(installer_path: Path, target_app: Path) -> None:
    with tempfile.TemporaryDirectory(prefix="bms_update_extract_") as temp_dir:
        temp_root = Path(temp_dir)
        with zipfile.ZipFile(installer_path, "r") as zip_ref:
            zip_ref.extractall(temp_root)
        source_app = _find_app_bundle(temp_root)
        if source_app is None:
            raise RuntimeError("No .app bundle found in update ZIP payload")
        atomic_replace_app_bundle(source_app, target_app)


def _install_from_dmg(installer_path: Path, target_app: Path) -> None:
    success, mount_point = mount_dmg(installer_path)
    if not success or not mount_point:
        raise RuntimeError("Failed to mount DMG")

    try:
        source_app = _find_app_bundle(Path(mount_point))
        if source_app is None:
            raise RuntimeError("No .app bundle found in DMG")
        atomic_replace_app_bundle(source_app, target_app)
    finally:
        unmount_dmg(mount_point)


def _relaunch_target(target_app: Path) -> None:
    if sys.platform == "darwin":
        subprocess.Popen(["open", "-a", str(target_app)])
        return
    if sys.platform.startswith("win"):
        subprocess.Popen([str(target_app)])
        return
    subprocess.Popen([str(target_app)])


def _write_result(
    result_file: Path | None,
    *,
    status: str,
    version: str,
    target: Path,
    installer: Path,
    error: str = "",
) -> None:
    if result_file is None:
        return
    payload = {
        "status": status,
        "version": version,
        "target": str(target),
        "installer": str(installer),
        "error": error,
        "completed_utc": datetime.now(timezone.utc).isoformat(),
    }
    try:
        result_file.parent.mkdir(parents=True, exist_ok=True)
        temp_file = result_file.with_suffix(result_file.suffix + ".tmp")
        temp_file.write_text(json.dumps(payload, indent=2), encoding="utf-8")
        temp_file.replace(result_file)
    except Exception as exc:
        print(f"Failed to write helper result file: {exc}", file=sys.stderr)


def run_update_helper(
    *,
    installer: Path,
    target_app: Path,
    wait_pid: int | None = None,
    relaunch: bool = False,
    result_file: Path | None = None,
    version: str = "",
) -> int:
    installer_path = installer.expanduser().resolve()
    target_path = target_app.expanduser().resolve()
    result_path = result_file.expanduser() if result_file else None
    update_version = str(version or "").strip()

    if wait_pid:
        if not wait_for_pid(wait_pid):
            message = f"Timeout waiting for process {wait_pid} to exit"
            print(message, file=sys.stderr)
            _write_result(
                result_path,
                status="error",
                version=update_version,
                target=target_path,
                installer=installer_path,
                error=message,
            )
            return 1
        time.sleep(0.75)

    suffix = installer_path.suffix.lower()
    try:
        if sys.platform.startswith("win") and suffix == ".exe":
            replace_windows_exe(installer_path, target_path)
        elif suffix == ".zip":
            _install_from_zip(installer_path, target_path)
        elif suffix == ".dmg" and sys.platform == "darwin":
            _install_from_dmg(installer_path, target_path)
        else:
            message = f"Unsupported update payload for this platform: {installer_path}"
            print(message, file=sys.stderr)
            _write_result(
                result_path,
                status="error",
                version=update_version,
                target=target_path,
                installer=installer_path,
                error=message,
            )
            return 1
    except Exception as exc:
        message = f"Update helper failed: {exc}"
        print(message, file=sys.stderr)
        _write_result(
            result_path,
            status="error",
            version=update_version,
            target=target_path,
            installer=installer_path,
            error=str(exc),
        )
        return 1

    relaunch_error = ""
    if relaunch:
        try:
            time.sleep(0.35)
            _relaunch_target(target_path)
        except Exception as exc:
            relaunch_error = f"Relaunch failed: {exc}"
            print(relaunch_error, file=sys.stderr)

    _write_result(
        result_path,
        status="success",
        version=update_version,
        target=target_path,
        installer=installer_path,
        error=relaunch_error,
    )
    return 0


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="BMS Dashboard Update Helper")
    parser.add_argument("--installer", required=True, help="Path to update payload")
    parser.add_argument("--target-app", required=True, help="Path to installed app target")
    parser.add_argument("--wait-pid", type=int, help="PID to wait for before applying update")
    parser.add_argument("--relaunch", action="store_true", help="Relaunch app after install")
    parser.add_argument("--result-file", type=Path, default=None, help="Path to write helper result JSON")
    parser.add_argument("--update-version", default="", help="Version string being applied")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = _build_parser().parse_args(argv)
    return run_update_helper(
        installer=Path(args.installer),
        target_app=Path(args.target_app),
        wait_pid=args.wait_pid,
        relaunch=bool(args.relaunch),
        result_file=args.result_file,
        version=args.update_version,
    )


if __name__ == "__main__":
    sys.exit(main())
