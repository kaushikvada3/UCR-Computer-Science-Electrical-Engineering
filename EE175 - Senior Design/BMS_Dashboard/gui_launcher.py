"""Standalone launcher for the BMS dashboard frontend."""

from __future__ import annotations

import argparse
import ctypes
import http.server
import json
import logging
import socketserver
import sys
import threading
import time
from pathlib import Path
from typing import Optional

from PyQt6.QtCore import QFileSystemWatcher, QObject, QThread, QTimer, Qt, QUrl, pyqtSignal, pyqtSlot
from PyQt6.QtGui import QAction, QIcon
from PyQt6.QtWebChannel import QWebChannel
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtWidgets import (
    QApplication,
    QFileDialog,
    QInputDialog,
    QMainWindow,
    QMessageBox,
    QProgressDialog,
)

from backend.data_stream import SerialWorker
from backend.settings_store import SettingsStore
from backend.updater import ReleaseUpdater, UpdateError, UpdateInfo, detect_repo_slug
from backend.version import APP_VERSION

logger = logging.getLogger("DashboardLauncher")

APP_NAME = "BMS Dashboard"
APP_USER_MODEL_ID = "UCR.BMSDashboard"
SERIAL_AUTO_LABEL = "Auto-detect"
SERIAL_PORT_DEFAULT_SENTINEL = "__default__"


def runtime_root() -> Path:
    if getattr(sys, "frozen", False):
        meipass = getattr(sys, "_MEIPASS", None)
        if meipass:
            return Path(meipass)
    return Path(__file__).resolve().parent


def source_root() -> Path:
    return Path(__file__).resolve().parent


def resolve_resource(*parts: str) -> Path:
    candidates = [runtime_root().joinpath(*parts), source_root().joinpath(*parts)]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return candidates[0]


def resolve_entrypoint(requested: Optional[Path]) -> Path:
    if requested is None:
        return resolve_resource("frontend", "index.html")
    if requested.is_absolute():
        return requested
    candidates = [
        Path.cwd() / requested,
        source_root() / requested,
        runtime_root() / requested,
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate.resolve()
    return (Path.cwd() / requested).resolve()


def find_app_icon_path() -> Optional[Path]:
    candidate_sets = [
        ("assets", "icons", "app_icon.ico"),
        ("assets", "icons", "app_icon.png"),
        ("assets", "icons", "app_icon.icns"),
        ("BMS Logo.png",),
    ]
    for parts in candidate_sets:
        path = resolve_resource(*parts)
        if path.exists():
            return path
    return None


def set_windows_app_id(app_id: str) -> None:
    if not sys.platform.startswith("win"):
        return
    try:
        ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(app_id)
    except Exception:
        logger.exception("Failed to set Windows AppUserModelID")


def normalize_serial_port_arg(value: Optional[str]) -> Optional[str]:
    if value is None:
        return None
    text = str(value).strip()
    if not text:
        return None
    if text.lower() == "auto":
        return None
    return text


class Bridge(QObject):
    """Bridge between JavaScript and Python."""

    def __init__(self, serial_worker: SerialWorker):
        super().__init__()
        self.serial_worker = serial_worker

    @pyqtSlot(str, name="sendCommand")
    def sendCommand(self, command: str):
        self.serial_worker.send_command(command)


class DashboardWindow(QMainWindow):
    """Main window hosting the WebEngine view."""
    update_check_finished = pyqtSignal(object, object, bool)
    update_install_finished = pyqtSignal(object, object)
    update_download_progress = pyqtSignal(int, int)

    def __init__(
        self,
        entrypoint: Path,
        settings: SettingsStore,
        serial_port: Optional[str],
        baudrate: int,
        updater: Optional[ReleaseUpdater],
        auto_update_check: bool,
        app_icon: Optional[QIcon] = None,
    ) -> None:
        super().__init__()
        self.setWindowTitle("BMS Command Surface")
        self.resize(1400, 900)

        self.settings = settings
        self.baudrate = baudrate
        self.updater = updater
        self._update_thread: Optional[threading.Thread] = None
        self._update_download_thread: Optional[threading.Thread] = None
        self._update_check_dialog: Optional[QProgressDialog] = None
        self._update_download_dialog: Optional[QProgressDialog] = None
        self._update_download_started_at = 0.0
        self._update_download_version = ""
        self._has_serial_data = False
        self._current_connected_port = ""

        if app_icon and not app_icon.isNull():
            self.setWindowIcon(app_icon)

        self.view = QWebEngineView(self)
        self.setCentralWidget(self.view)

        self.entrypoint = entrypoint.resolve()
        if not self.entrypoint.exists():
            raise FileNotFoundError(f"Unable to find {self.entrypoint}")

        self.http_port = 8765
        self.start_http_server()

        self.serial_worker = SerialWorker(port=serial_port, baudrate=baudrate)
        self.serial_thread = QThread()
        self.serial_worker.moveToThread(self.serial_thread)
        self.serial_thread.started.connect(self.serial_worker.start_monitoring)
        self.serial_worker.data_received.connect(self.handle_data)
        self.serial_worker.connection_status.connect(self.handle_connection_status)
        self.serial_worker.data_activity.connect(self.handle_data_activity)
        self.serial_worker.connected_port_changed.connect(self.handle_connected_port_change)
        self.serial_thread.start()

        self.bridge = Bridge(self.serial_worker)
        self.channel = QWebChannel()
        self.channel.registerObject("backend", self.bridge)
        self.view.page().setWebChannel(self.channel)

        self._build_toolbar()
        self.update_check_finished.connect(self._handle_update_result)
        self.update_install_finished.connect(self._handle_install_handoff)
        self.update_download_progress.connect(self._handle_download_progress)
        self._install_watcher()
        self.load_page()
        self.statusBar().showMessage("Serial: connecting...")

        if self.updater and auto_update_check:
            QTimer.singleShot(2500, lambda: self.check_for_updates_async(manual=False))

    def handle_data(self, data: dict):
        json_str = json.dumps(data)
        self.view.page().runJavaScript(
            f"if(window.updateDashboard) window.updateDashboard({json_str});"
        )

    def handle_connection_status(self, is_connected: bool):
        if not is_connected:
            self._has_serial_data = False
            self._set_frontend_connection_state(False)
        self._refresh_status_bar()

    def handle_data_activity(self):
        self._has_serial_data = True
        self._set_frontend_connection_state(True)
        self._refresh_status_bar()

    def handle_connected_port_change(self, connected_port: str):
        self._current_connected_port = connected_port or ""
        self._refresh_status_bar()

    def _set_frontend_connection_state(self, connected: bool):
        state_js = "true" if connected else "false"
        self.view.page().runJavaScript(
            f"if(window.setConnectionStatus) window.setConnectionStatus({state_js});"
        )

    def _refresh_status_bar(self):
        target_port = self.serial_worker.get_target_port() or SERIAL_AUTO_LABEL
        connected_port = self._current_connected_port
        if connected_port:
            message = f"Serial connected: {connected_port} @ {self.baudrate}"
        else:
            message = f"Serial disconnected (target: {target_port}) @ {self.baudrate}"
        self.statusBar().showMessage(message)

    def start_http_server(self) -> None:
        frontend_dir = self.entrypoint.parent

        class Handler(http.server.SimpleHTTPRequestHandler):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, directory=str(frontend_dir), **kwargs)

            def log_message(self, fmt, *args):
                return

        for port in range(8765, 8775):
            try:
                socketserver.TCPServer.allow_reuse_address = True
                self.httpd = socketserver.TCPServer(("", port), Handler)
                self.http_port = port
                break
            except OSError:
                continue

        if not hasattr(self, "httpd"):
            raise RuntimeError("Could not find a free port for the internal server (8765-8774).")

        self.server_thread = threading.Thread(target=self.httpd.serve_forever, daemon=True)
        self.server_thread.start()
        logger.info("Internal wrapper server started on port %s", self.http_port)

    def load_page(self) -> None:
        timestamp = int(time.time())
        filename = self.entrypoint.name
        url = QUrl(f"http://localhost:{self.http_port}/{filename}?t={timestamp}")
        self.view.load(url)

    def reload(self) -> None:
        self.view.reload()

    def closeEvent(self, event) -> None:
        if hasattr(self, "serial_worker"):
            self.serial_worker.stop()
            self.serial_thread.quit()
            self.serial_thread.wait()

        if hasattr(self, "httpd"):
            self.httpd.shutdown()

        event.accept()

    def _build_toolbar(self) -> None:
        toolbar = self.addToolBar("Controls")
        toolbar.setMovable(False)

        reload_action = QAction(QIcon.fromTheme("view-refresh"), "Reload", self)
        reload_action.setStatusTip("Force-reload the dashboard surface")
        reload_action.triggered.connect(self.reload)
        toolbar.addAction(reload_action)

        open_action = QAction("Open...", self)
        open_action.setStatusTip("Choose a different HTML entrypoint")
        open_action.triggered.connect(self._choose_entrypoint)
        toolbar.addAction(open_action)

        serial_action = QAction("Serial Port...", self)
        serial_action.setStatusTip("Choose target serial port or auto-detect mode")
        serial_action.triggered.connect(self._choose_serial_port)
        toolbar.addAction(serial_action)

        baud_action = QAction("Baudrate...", self)
        baud_action.setStatusTip("Set serial baudrate")
        baud_action.triggered.connect(self._choose_baudrate)
        toolbar.addAction(baud_action)

        update_action = QAction("Check Updates", self)
        update_action.setStatusTip("Check GitHub Releases for an update")
        update_action.triggered.connect(lambda: self.check_for_updates_async(manual=True))
        toolbar.addAction(update_action)

    def _choose_entrypoint(self) -> None:
        dialog = QFileDialog(self, "Select dashboard entrypoint")
        dialog.setFileMode(QFileDialog.FileMode.ExistingFile)
        dialog.setNameFilter("HTML files (*.html *.htm)")
        if dialog.exec():
            selection = dialog.selectedFiles()
            if selection:
                new_path = Path(selection[0]).resolve()
                self.entrypoint = new_path
                self._install_watcher()
                self.load_page()

    def _choose_serial_port(self) -> None:
        ports = SerialWorker.list_available_ports()
        options = [SERIAL_AUTO_LABEL] + ports
        current = self.serial_worker.get_target_port() or SERIAL_AUTO_LABEL
        if current not in options:
            options.append(current)
        current_index = options.index(current)
        selected, ok = QInputDialog.getItem(
            self,
            "Serial Port",
            "Select target port:",
            options,
            current_index,
            False,
        )
        if not ok:
            return

        new_port = None if selected == SERIAL_AUTO_LABEL else selected
        self.serial_worker.set_target_port(new_port)
        self.settings.set_serial(new_port, self.baudrate)
        self._refresh_status_bar()

    def _choose_baudrate(self) -> None:
        value, ok = QInputDialog.getInt(
            self,
            "Serial Baudrate",
            "Baudrate:",
            self.baudrate,
            1200,
            3000000,
            100,
        )
        if not ok:
            return
        self.baudrate = value
        self.serial_worker.set_baudrate(value)
        self.settings.set_serial(self.serial_worker.get_target_port(), self.baudrate)
        self._refresh_status_bar()

    def _install_watcher(self) -> None:
        files_to_watch = [
            self.entrypoint,
            self.entrypoint.parent / "scene.js",
            self.entrypoint.parent / "style.css",
            self.entrypoint.parent / "index.html",
            self.entrypoint.parent / "qwebchannel.js",
        ]
        existing = [str(path) for path in files_to_watch if path.exists()]
        self.watcher = QFileSystemWatcher(existing, self)
        self.watcher.fileChanged.connect(self._debounced_reload)

    def _debounced_reload(self, _path: str) -> None:
        QTimer.singleShot(250, self.reload)

    def check_for_updates_async(self, manual: bool):
        if not self.updater:
            if manual:
                QMessageBox.information(
                    self,
                    "Updates",
                    "Update repository is not configured. Set BMS_UPDATE_REPO or --update-repo.",
                )
            return

        if self._update_thread and self._update_thread.is_alive():
            if manual:
                QMessageBox.information(self, "Updates", "An update check is already running.")
            return

        if manual:
            self._show_busy_dialog(
                title="Updates",
                label="Checking for updates...",
                dialog_attr="_update_check_dialog",
            )
        self.statusBar().showMessage("Checking for updates...")

        self._update_thread = threading.Thread(
            target=self._check_updates_worker,
            args=(manual,),
            daemon=True,
        )
        self._update_thread.start()

    def _check_updates_worker(self, manual: bool):
        info = None
        error = None
        try:
            info = self.updater.check_for_update() if self.updater else None
        except Exception as exc:
            error = exc
            logger.exception("Update check failed")
        finally:
            self.settings.set_last_checked_now()

        self.update_check_finished.emit(info, error, manual)

    def _handle_update_result(self, info: Optional[UpdateInfo], error: Optional[Exception], manual: bool):
        self._close_busy_dialog("_update_check_dialog")
        if error:
            self.statusBar().showMessage("Update check failed.", 6000)
            if manual:
                QMessageBox.warning(self, "Updates", f"Update check failed:\n{error}")
            return

        if info is None:
            self.statusBar().showMessage("You are on the latest stable release.", 6000)
            if manual:
                QMessageBox.information(self, "Updates", "You are already on the latest stable release.")
            return

        self.statusBar().showMessage(f"Update available: {info.version}", 6000)
        self._prompt_install_update(info)

    def _prompt_install_update(self, info: UpdateInfo):
        notes_preview = (info.notes or "").strip()
        if notes_preview:
            lines = notes_preview.splitlines()
            notes_preview = "\n".join(lines[:8])

        msg = QMessageBox(self)
        msg.setWindowTitle("Update Available")
        msg.setIcon(QMessageBox.Icon.Information)
        msg.setText(f"Version {info.version} is available.")
        details = "Download and start guided install now?"
        if notes_preview:
            details += f"\n\nRelease notes:\n{notes_preview}"
        msg.setInformativeText(details)
        msg.setStandardButtons(QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No)
        if msg.exec() != QMessageBox.StandardButton.Yes:
            return

        if self._update_download_thread and self._update_download_thread.is_alive():
            QMessageBox.information(self, "Updates", "Update download already in progress.")
            return

        self._show_busy_dialog(
            title="Updates",
            label=f"Downloading version {info.version}...",
            dialog_attr="_update_download_dialog",
        )
        self._update_download_started_at = time.monotonic()
        self._update_download_version = info.version
        self.statusBar().showMessage(f"Downloading update {info.version}...")

        self._update_download_thread = threading.Thread(
            target=self._download_and_launch_update_worker,
            args=(info,),
            daemon=True,
        )
        self._update_download_thread.start()

    def _download_and_launch_update_worker(self, info: UpdateInfo):
        message = None
        error = None
        try:
            if not self.updater:
                raise UpdateError("Updater is not configured")
            installer = self.updater.download_update(
                info,
                progress_callback=lambda done, total: self.update_download_progress.emit(done, total),
            )
            message = self.updater.launch_guided_install(installer)
        except Exception as exc:
            error = exc
            logger.exception("Update install handoff failed")

        self.update_install_finished.emit(message, error)

    def _handle_download_progress(self, downloaded_bytes: int, total_bytes: int):
        dialog = self._update_download_dialog
        if dialog is None:
            return

        downloaded_bytes = max(0, int(downloaded_bytes))
        total_bytes = max(0, int(total_bytes))

        if total_bytes > 0:
            if dialog.maximum() != total_bytes or dialog.minimum() != 0:
                dialog.setRange(0, total_bytes)
            dialog.setValue(min(downloaded_bytes, total_bytes))
            percent = (downloaded_bytes / total_bytes) * 100.0
            label = (
                f"Downloading version {self._update_download_version}..."
                f"\n{self._format_bytes(downloaded_bytes)} / {self._format_bytes(total_bytes)}"
                f" ({percent:.1f}%)"
            )
        else:
            if dialog.minimum() != 0 or dialog.maximum() != 0:
                dialog.setRange(0, 0)
            dialog.setValue(0)
            label = (
                f"Downloading version {self._update_download_version}..."
                f"\n{self._format_bytes(downloaded_bytes)} downloaded"
            )

        elapsed = max(0.001, time.monotonic() - self._update_download_started_at)
        speed_bps = downloaded_bytes / elapsed
        label += f" @ {self._format_bytes(speed_bps)}/s"
        dialog.setLabelText(label)

    def _handle_install_handoff(self, message: Optional[str], error: Optional[Exception]):
        self._close_busy_dialog("_update_download_dialog")
        self._update_download_started_at = 0.0
        self._update_download_version = ""
        if error:
            self.statusBar().showMessage("Update failed.", 6000)
            QMessageBox.warning(self, "Updates", f"Update failed:\n{error}")
            return

        self.statusBar().showMessage("Update package is ready.", 6000)
        QMessageBox.information(self, "Updates", message or "Update package launched.")

    def _show_busy_dialog(self, title: str, label: str, dialog_attr: str) -> None:
        self._close_busy_dialog(dialog_attr)
        dialog = QProgressDialog(label, "", 0, 0, self)
        dialog.setWindowTitle(title)
        dialog.setWindowModality(Qt.WindowModality.WindowModal)
        dialog.setMinimumDuration(0)
        dialog.setAutoClose(False)
        dialog.setAutoReset(False)
        dialog.setCancelButton(None)
        dialog.setValue(0)
        dialog.show()
        setattr(self, dialog_attr, dialog)

    def _close_busy_dialog(self, dialog_attr: str) -> None:
        dialog = getattr(self, dialog_attr, None)
        if dialog is None:
            return
        dialog.close()
        dialog.deleteLater()
        setattr(self, dialog_attr, None)

    @staticmethod
    def _format_bytes(value: float) -> str:
        units = ["B", "KB", "MB", "GB", "TB"]
        size = float(value)
        for unit in units:
            if size < 1024.0 or unit == units[-1]:
                if unit == "B":
                    return f"{int(size)} {unit}"
                return f"{size:.1f} {unit}"
            size /= 1024.0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Launch the BMS dashboard GUI.")
    parser.add_argument(
        "--entrypoint",
        type=Path,
        default=None,
        help="Path to dashboard HTML entrypoint (defaults to frontend/index.html).",
    )
    parser.add_argument(
        "--serial-port",
        default=SERIAL_PORT_DEFAULT_SENTINEL,
        help="Target serial port (e.g. COM5, /dev/ttyACM0) or 'auto'.",
    )
    parser.add_argument(
        "--baudrate",
        type=int,
        default=0,
        help="Serial baudrate. If omitted, uses saved setting (default 115200).",
    )
    parser.add_argument(
        "--update-channel",
        default="stable",
        choices=["stable"],
        help="Release update channel.",
    )
    parser.add_argument(
        "--no-auto-update-check",
        action="store_true",
        help="Disable automatic update checks on startup.",
    )
    parser.add_argument(
        "--update-repo",
        default=None,
        help="GitHub repo slug for updates (owner/repo).",
    )
    return parser.parse_args()


def main() -> int:
    logging.basicConfig(level=logging.INFO)
    args = parse_args()

    settings = SettingsStore()
    stored_port = settings.serial_port()
    stored_baudrate = settings.serial_baudrate()

    if args.serial_port == SERIAL_PORT_DEFAULT_SENTINEL:
        serial_port = stored_port
    else:
        serial_port = normalize_serial_port_arg(args.serial_port)

    baudrate = args.baudrate if args.baudrate > 0 else stored_baudrate
    settings.set_serial(serial_port, baudrate)
    settings.set_update_channel(args.update_channel)

    entrypoint = resolve_entrypoint(args.entrypoint)
    repo_slug = args.update_repo or detect_repo_slug(source_root())
    updater = None
    if repo_slug:
        updater = ReleaseUpdater(
            repo_slug=repo_slug,
            current_version=APP_VERSION,
            channel=settings.update_channel(),
        )

    set_windows_app_id(APP_USER_MODEL_ID)

    app = QApplication(sys.argv)
    app.setApplicationName(APP_NAME)
    app.setApplicationDisplayName(APP_NAME)
    app.setApplicationVersion(APP_VERSION)
    app.setOrganizationName("UCR")
    app.setDesktopFileName("bms-dashboard")

    icon = QIcon()
    icon_path = find_app_icon_path()
    if icon_path:
        icon = QIcon(str(icon_path))
        if not icon.isNull():
            app.setWindowIcon(icon)

    try:
        window = DashboardWindow(
            entrypoint=entrypoint,
            settings=settings,
            serial_port=serial_port,
            baudrate=baudrate,
            updater=updater,
            auto_update_check=not args.no_auto_update_check,
            app_icon=icon,
        )
    except (FileNotFoundError, RuntimeError) as exc:
        QMessageBox.critical(None, "Dashboard launcher", str(exc))
        return 1

    window.show()
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
