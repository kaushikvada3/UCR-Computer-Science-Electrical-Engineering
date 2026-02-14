#!/usr/bin/env python3
"""
Auto-update helper script that replaces the app bundle and relaunches.
This runs as a separate process after the main app exits.
"""

import argparse
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path


def wait_for_pid(pid: int, timeout: int = 30) -> bool:
    """Wait for a process to exit."""
    start = time.time()
    while time.time() - start < timeout:
        try:
            os.kill(pid, 0)  # Check if process exists
            time.sleep(0.5)
        except (OSError, ProcessLookupError):
            return True  # Process has exited
    return False


def mount_dmg(dmg_path: Path) -> tuple[bool, str]:
    """Mount a DMG file and return the mount point."""
    try:
        result = subprocess.run(
            ["hdiutil", "attach", "-nobrowse", "-quiet", str(dmg_path)],
            capture_output=True,
            text=True,
            check=True,
        )
        # Parse mount point from output
        for line in result.stdout.splitlines():
            if "/Volumes/" in line:
                mount_point = line.split("/Volumes/")[-1].split()[0]
                return True, f"/Volumes/{mount_point}"
        return False, ""
    except subprocess.CalledProcessError as e:
        print(f"Failed to mount DMG: {e}", file=sys.stderr)
        return False, ""


def unmount_dmg(mount_point: str) -> bool:
    """Unmount a DMG."""
    try:
        subprocess.run(
            ["hdiutil", "detach", mount_point, "-quiet"],
            capture_output=True,
            check=True,
        )
        return True
    except subprocess.CalledProcessError:
        return False


def replace_app_bundle(source_app: Path, target_app: Path) -> bool:
    """Replace the target app bundle with the source."""
    try:
        # Remove old app
        if target_app.exists():
            shutil.rmtree(target_app)

        # Copy new app
        shutil.copytree(source_app, target_app, symlinks=True)

        # Preserve executable permissions
        for root, dirs, files in os.walk(target_app):
            for name in files:
                file_path = Path(root) / name
                if file_path.suffix == "" or "MacOS" in str(file_path):
                    try:
                        file_path.chmod(0o755)
                    except Exception:
                        pass

        return True
    except Exception as e:
        print(f"Failed to replace app bundle: {e}", file=sys.stderr)
        return False


def extract_zip(zip_path: Path, extract_to: Path) -> bool:
    """Extract a ZIP file."""
    try:
        import zipfile
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(extract_to)
        return True
    except Exception as e:
        print(f"Failed to extract ZIP: {e}", file=sys.stderr)
        return False


def replace_windows_exe(source_exe: Path, target_exe: Path) -> bool:
    """Replace Windows executable."""
    try:
        # Backup old exe
        backup_path = target_exe.with_suffix('.exe.old')
        if backup_path.exists():
            backup_path.unlink()

        if target_exe.exists():
            shutil.move(str(target_exe), str(backup_path))

        # Copy new exe
        shutil.copy2(str(source_exe), str(target_exe))

        # Remove backup on success
        if backup_path.exists():
            backup_path.unlink()

        return True
    except Exception as e:
        print(f"Failed to replace executable: {e}", file=sys.stderr)
        # Try to restore backup
        backup_path = target_exe.with_suffix('.exe.old')
        if backup_path.exists() and not target_exe.exists():
            try:
                shutil.move(str(backup_path), str(target_exe))
            except Exception:
                pass
        return False


def main():
    parser = argparse.ArgumentParser(description="BMS Dashboard Update Helper")
    parser.add_argument("--installer", required=True, help="Path to installer (DMG, ZIP, or EXE)")
    parser.add_argument("--target-app", required=True, help="Path to app bundle/executable to replace")
    parser.add_argument("--wait-pid", type=int, help="Wait for this PID to exit")
    parser.add_argument("--relaunch", action="store_true", help="Relaunch app after update")

    args = parser.parse_args()

    installer_path = Path(args.installer)
    target_app = Path(args.target_app)
    is_windows = sys.platform.startswith("win")

    # Wait for main app to exit
    if args.wait_pid:
        print(f"Waiting for PID {args.wait_pid} to exit...")
        if not wait_for_pid(args.wait_pid):
            print("Timeout waiting for app to exit", file=sys.stderr)
            return 1
        time.sleep(1)  # Extra grace period

    # Handle Windows EXE (direct replacement)
    if is_windows and installer_path.suffix.lower() == ".exe":
        print(f"Replacing {target_app} with {installer_path}")
        if not replace_windows_exe(installer_path, target_app):
            return 1

        if args.relaunch:
            print(f"Relaunching {target_app}")
            time.sleep(0.5)
            subprocess.Popen([str(target_app)])

        print("Update completed successfully")
        return 0

    # Handle DMG
    if installer_path.suffix.lower() == ".dmg":
        print(f"Mounting DMG: {installer_path}")
        success, mount_point = mount_dmg(installer_path)
        if not success:
            return 1

        try:
            # Find .app bundle in mount point
            mount_path = Path(mount_point)
            app_bundles = list(mount_path.glob("*.app"))
            if not app_bundles:
                print("No .app bundle found in DMG", file=sys.stderr)
                return 1

            source_app = app_bundles[0]
            print(f"Replacing {target_app} with {source_app}")

            if not replace_app_bundle(source_app, target_app):
                return 1

        finally:
            print(f"Unmounting {mount_point}")
            unmount_dmg(mount_point)

    # Handle ZIP
    elif installer_path.suffix.lower() == ".zip":
        print(f"Extracting ZIP: {installer_path}")
        temp_dir = installer_path.parent / "extracted"
        temp_dir.mkdir(exist_ok=True)

        if not extract_zip(installer_path, temp_dir):
            return 1

        try:
            # Find .app bundle in extracted folder
            app_bundles = list(temp_dir.glob("*.app"))
            if not app_bundles:
                app_bundles = list(temp_dir.glob("*/*.app"))

            if not app_bundles:
                print("No .app bundle found in ZIP", file=sys.stderr)
                return 1

            source_app = app_bundles[0]
            print(f"Replacing {target_app} with {source_app}")

            if not replace_app_bundle(source_app, target_app):
                return 1

        finally:
            # Clean up extracted files
            try:
                shutil.rmtree(temp_dir)
            except Exception:
                pass

    else:
        print(f"Unsupported installer format: {installer_path.suffix}", file=sys.stderr)
        return 1

    # Relaunch app
    if args.relaunch:
        print(f"Relaunching {target_app}")
        time.sleep(0.5)
        subprocess.Popen(["open", "-a", str(target_app)])

    print("Update completed successfully")
    return 0


if __name__ == "__main__":
    sys.exit(main())
