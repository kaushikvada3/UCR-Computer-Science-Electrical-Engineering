"""Standalone launcher for the BMS dashboard frontend."""

from __future__ import annotations

import argparse
import ctypes
import http.server
import json
import logging
import os
import shutil
import socketserver
import sys
import sysconfig
import threading
import time
from pathlib import Path
from typing import Any, Optional

from packaging.version import InvalidVersion, Version


def _bootstrap_macos_qt_runtime() -> None:
    """Work around Qt plugin loading issues from iCloud-backed virtualenv paths."""
    if sys.platform != "darwin" or getattr(sys, "frozen", False):
        return
    if os.environ.get("QT_QPA_PLATFORM_PLUGIN_PATH"):
        return

    purelib = Path(sysconfig.get_paths().get("purelib", ""))
    qt_root = purelib / "PyQt6" / "Qt6"
    if not qt_root.exists():
        return

    qt_root_text = str(qt_root).lower()
    if "mobile documents" not in qt_root_text and "clouddocs" not in qt_root_text:
        return

    cache_root = Path.home() / "Library" / "Caches" / "BMSDashboard"
    cached_qt_root = cache_root / "qt6-runtime"
    marker_file = cached_qt_root / ".source-path"

    try:
        cache_root.mkdir(parents=True, exist_ok=True)
        source_marker = str(qt_root)
        marker_matches = (
            marker_file.exists() and marker_file.read_text(encoding="utf-8").strip() == source_marker
        )
        if not marker_matches:
            staging_root = cache_root / f"qt6-runtime-staging-{os.getpid()}"
            if staging_root.exists():
                shutil.rmtree(staging_root)
            # copy() avoids preserving problematic metadata from File Provider paths.
            shutil.copytree(qt_root, staging_root, copy_function=shutil.copy)
            (staging_root / ".source-path").write_text(source_marker, encoding="utf-8")
            if cached_qt_root.exists():
                shutil.rmtree(cached_qt_root)
            staging_root.rename(cached_qt_root)
    except Exception as exc:  # pragma: no cover - environment-specific fallback
        print(f"[BMS] Qt runtime bootstrap skipped: {exc}", file=sys.stderr)
        return

    os.environ.setdefault("QT_PLUGIN_PATH", str(cached_qt_root / "plugins"))
    os.environ.setdefault(
        "QT_QPA_PLATFORM_PLUGIN_PATH",
        str(cached_qt_root / "plugins" / "platforms"),
    )
    os.environ.setdefault("DYLD_FRAMEWORK_PATH", str(cached_qt_root / "lib"))


_bootstrap_macos_qt_runtime()

from PyQt6.QtCore import (
    QFileSystemWatcher,
    QObject,
    QRect,
    QThread,
    QTimer,
    Qt,
    QUrl,
    pyqtSignal,
    pyqtSlot,
)
from PyQt6.QtGui import QAction, QColor, QIcon
from PyQt6.QtWebChannel import QWebChannel
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtWidgets import (
    QApplication,
    QDialog,
    QFileDialog,
    QHBoxLayout,
    QInputDialog,
    QLabel,
    QMainWindow,
    QMessageBox,
    QProgressBar,
    QPushButton,
    QVBoxLayout,
)

from backend.data_stream import SerialWorker
from backend.settings_store import SettingsStore
from backend.update_helper import run_update_helper
from backend.updater import (
    ReleaseUpdater,
    UpdateError,
    UpdateInfo,
    default_update_log_path,
    default_update_result_path,
    detect_repo_slug,
)
from backend.version import APP_VERSION

logger = logging.getLogger("DashboardLauncher")

APP_NAME = "BMS Dashboard"
APP_USER_MODEL_ID = "UCR.BMSDashboard"
SERIAL_AUTO_LABEL = "Auto-detect"
SERIAL_PORT_DEFAULT_SENTINEL = "__default__"
DEFAULT_WINDOW_WIDTH = 1400
DEFAULT_WINDOW_HEIGHT = 900
STARTUP_MIN_DURATION_MS = 3000
BOOT_POLL_INTERVAL_MS = 120
BOOT_POLL_TIMEOUT_MS = 30000


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
    if sys.platform.startswith("win"):
        candidate_sets = [
            ("assets", "icons", "app_icon.ico"),
            ("assets", "icons", "app_icon.png"),
            ("assets", "icons", "app_icon.icns"),
            ("BMS Logo.png",),
        ]
    elif sys.platform == "darwin":
        candidate_sets = [
            ("assets", "icons", "app_icon.icns"),
            ("assets", "icons", "app_icon.png"),
            ("assets", "icons", "app_icon.ico"),
            ("BMS Logo.png",),
        ]
    else:
        candidate_sets = [
            ("assets", "icons", "app_icon.png"),
            ("assets", "icons", "app_icon.icns"),
            ("assets", "icons", "app_icon.ico"),
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

    @pyqtSlot(str, result=str)
    def sendCommand(self, command: str) -> str:
        print(f"[BRIDGE] sendCommand received: {command!r}", flush=True)
        self.serial_worker.send_command(command)
        return "ok"


class UpdateProgressDialog(QDialog):
    STATUS_TITLES = {
        "checking": "Checking for update",
        "downloading": "Downloading update",
        "verifying": "Verifying package",
        "preparing": "Preparing install",
        "permission_prompt": "Waiting for administrator permission",
        "installing": "Applying update",
        "restarting": "Restarting application",
        "success": "Update complete",
        "error": "Update failed",
    }

    def __init__(self, parent: Optional[QMainWindow] = None) -> None:
        super().__init__(parent)
        self.setWindowTitle("BMS Dashboard Updater")
        self.setModal(True)
        self.setWindowModality(Qt.WindowModality.ApplicationModal)
        self.setMinimumWidth(560)
        self.setWindowFlag(Qt.WindowType.WindowContextHelpButtonHint, False)
        self._allow_close = False

        root = QVBoxLayout(self)
        root.setContentsMargins(16, 16, 16, 16)
        root.setSpacing(10)

        self.phase_label = QLabel("Preparing update…", self)
        self.phase_label.setStyleSheet("font-weight: 600;")
        root.addWidget(self.phase_label)

        self.detail_label = QLabel("Waiting to start…", self)
        self.detail_label.setWordWrap(True)
        root.addWidget(self.detail_label)

        self.progress = QProgressBar(self)
        self.progress.setRange(0, 0)
        self.progress.setFormat("...")
        root.addWidget(self.progress)

        self.target_title = QLabel("Install target:", self)
        self.target_title.setStyleSheet("color: #666; font-size: 11px;")
        root.addWidget(self.target_title)

        self.target_path = QLabel("", self)
        self.target_path.setWordWrap(True)
        self.target_path.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
        root.addWidget(self.target_path)

        self.error_label = QLabel("", self)
        self.error_label.setWordWrap(True)
        self.error_label.setStyleSheet("color: #b00020;")
        self.error_label.setVisible(False)
        root.addWidget(self.error_label)

        self.log_path_label = QLabel("", self)
        self.log_path_label.setWordWrap(True)
        self.log_path_label.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
        self.log_path_label.setVisible(False)
        root.addWidget(self.log_path_label)

        button_row = QHBoxLayout()
        button_row.addStretch(1)
        self.close_button = QPushButton("Close", self)
        self.close_button.setEnabled(False)
        self.close_button.clicked.connect(self.close)
        button_row.addWidget(self.close_button)
        root.addLayout(button_row)

    @staticmethod
    def normalize_status(status: str) -> str:
        normalized = (status or "").strip().lower()
        if normalized == "staging":
            return "preparing"
        if normalized == "restart_required":
            return "restarting"
        if normalized == "close_required":
            return "installing"
        return normalized

    def start_session(self, *, version: str, target_path: Path) -> None:
        self._allow_close = False
        self.error_label.clear()
        self.error_label.setVisible(False)
        self.log_path_label.clear()
        self.log_path_label.setVisible(False)
        self.close_button.setEnabled(False)
        self.phase_label.setText(f"Preparing update to v{version}")
        self.detail_label.setText("Starting updater workflow…")
        self.progress.setRange(0, 0)
        self.progress.setFormat("...")
        self.target_path.setText(str(target_path))
        self.show()
        self.raise_()
        self.activateWindow()

    def apply_state(self, status: str, detail: str, current: int, total: int) -> None:
        normalized = self.normalize_status(status)
        title = self.STATUS_TITLES.get(normalized, normalized.replace("_", " ").title())
        if title:
            self.phase_label.setText(title)
        if detail:
            self.detail_label.setText(detail)

        if normalized == "downloading":
            self.progress.setVisible(True)
            if total > 0:
                percent = int(max(0.0, min(100.0, (current / total) * 100.0)))
                self.progress.setRange(0, 100)
                self.progress.setValue(percent)
                self.progress.setFormat(f"{percent}%")
            else:
                self.progress.setRange(0, 0)
                self.progress.setFormat("...")
            return

        if normalized in {"success", "restarting"}:
            self.progress.setVisible(True)
            self.progress.setRange(0, 100)
            self.progress.setValue(100)
            self.progress.setFormat("100%")
            return

        if normalized == "error":
            self.progress.setVisible(False)
            self.error_label.setText(detail or "Update failed.")
            self.error_label.setVisible(True)
            self.close_button.setEnabled(True)
            return

        if normalized in {"checking", "verifying", "preparing", "permission_prompt", "installing"}:
            self.progress.setVisible(True)
            self.progress.setRange(0, 0)
            self.progress.setFormat("...")
            return

        if normalized in {"", "idle"}:
            self.progress.setVisible(False)

    def set_error(self, message: str, log_path: str) -> None:
        self._allow_close = True
        self.apply_state("error", message, 0, 0)
        if log_path:
            self.log_path_label.setText(f"Updater log: {log_path}")
            self.log_path_label.setVisible(True)

    def closeEvent(self, event) -> None:
        app = QApplication.instance()
        if not self._allow_close and app is not None and not app.closingDown():
            event.ignore()
            return
        super().closeEvent(event)


class DashboardWindow(QMainWindow):
    """Main window hosting the WebEngine view."""
    update_check_finished = pyqtSignal(object, object, bool)
    update_install_finished = pyqtSignal(object, object)
    update_download_progress = pyqtSignal(int, int)
    update_state_changed = pyqtSignal(str, str, int, int)

    def __init__(
        self,
        entrypoint: Path,
        settings: SettingsStore,
        serial_port: Optional[str],
        baudrate: int,
        updater: Optional[ReleaseUpdater],
        auto_update_check: bool,
        is_packaged: bool,
        app_icon: Optional[QIcon] = None,
    ) -> None:
        super().__init__()
        self.setWindowTitle("BMS Command Surface")

        # Size the window to fit the screen, capping at the default max
        screen = QApplication.primaryScreen()
        if screen is not None:
            avail = screen.availableGeometry()
            w = min(DEFAULT_WINDOW_WIDTH, avail.width() - 40)
            h = min(DEFAULT_WINDOW_HEIGHT, avail.height() - 40)
            self.resize(w, h)
        else:
            self.resize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT)

        self.settings = settings
        self.baudrate = baudrate
        self.updater = updater
        self.is_packaged = is_packaged
        self._update_thread: Optional[threading.Thread] = None
        self._update_download_thread: Optional[threading.Thread] = None
        self._update_download_started_at = 0.0
        self._update_download_version = ""
        self._update_action: Optional[QAction] = None
        self._update_state_label: Optional[QLabel] = None
        self._update_progress_bar: Optional[QProgressBar] = None
        self._update_progress_dialog: Optional[UpdateProgressDialog] = None
        self._active_update_target: Optional[Path] = None
        self._pending_update_result: Optional[dict[str, Any]] = self._consume_update_result()
        self._staged_restart_prompted = False
        self._has_serial_data = False
        self._current_connected_port = ""
        self.latest_bms_data = {}
        self.latest_eload_data = {}
        self.toolbar = None
        self._normal_window_flags = self.windowFlags()
        self._startup_mode_active: bool = True
        self._startup_started_at: float = time.monotonic()
        self._startup_min_duration_ms: int = STARTUP_MIN_DURATION_MS
        self._boot_poll_interval_ms: int = BOOT_POLL_INTERVAL_MS
        self._boot_poll_timeout_ms: int = BOOT_POLL_TIMEOUT_MS
        self._boot_last_state: Optional[dict[str, Any]] = None
        self._startup_transition_done: bool = False
        self._chrome_cue_applied: bool = False
        self._startup_connection_target_last: Optional[bool] = None
        self._boot_poll_timer = QTimer(self)
        self._boot_poll_timer.setInterval(self._boot_poll_interval_ms)
        self._boot_poll_timer.timeout.connect(self._poll_boot_state_once)

        if app_icon and not app_icon.isNull():
            self.setWindowIcon(app_icon)

        self.view = QWebEngineView(self)
        self.setCentralWidget(self.view)

        self.entrypoint = entrypoint.resolve()
        if not self.entrypoint.exists():
            raise FileNotFoundError(f"Unable to find {self.entrypoint}")

        self.http_port = 8765
        self.start_http_server()

        # BMS Serial Worker
        self.serial_worker = SerialWorker(port=serial_port, baudrate=baudrate)
        self.serial_thread = QThread()
        self.serial_worker.moveToThread(self.serial_thread)
        self.serial_thread.started.connect(self.serial_worker.start_monitoring)
        self.serial_worker.data_received.connect(self.handle_bms_data)
        self.serial_worker.connection_status.connect(self.handle_connection_status)
        self.serial_worker.data_activity.connect(self.handle_data_activity)
        self.serial_worker.raw_line_received.connect(self.handle_raw_serial_line)
        self.serial_worker.connected_port_changed.connect(self.handle_connected_port_change)
        self.serial_thread.start()

        # E-Load Serial Worker (always created; starts paused if no port configured)
        eload_port = settings.eload_port()
        eload_baud = settings.eload_baudrate()
        self.eload_worker = SerialWorker(port=eload_port, baudrate=eload_baud)
        self.eload_thread = QThread()
        self.eload_worker.moveToThread(self.eload_thread)
        self.eload_thread.started.connect(self.eload_worker.start_monitoring)
        self.eload_worker.data_received.connect(self.handle_eload_data)
        self.eload_worker.connection_status.connect(self.handle_eload_connection_status)
        self.eload_worker.connected_port_changed.connect(self.handle_eload_connected_port_change)
        self.eload_thread.start()
        if eload_port:
            print(f"[E-Load] E-Load serial worker started on {eload_port}")
        else:
            print("[E-Load] E-Load serial worker created (paused, no port configured)")

        self.bridge = Bridge(self.serial_worker)
        self.channel = QWebChannel()
        self.channel.registerObject("backend", self.bridge)
        self.view.page().setWebChannel(self.channel)

        self._build_toolbar()
        self._build_update_indicator()
        self.update_check_finished.connect(self._handle_update_result)
        self.update_install_finished.connect(self._handle_install_handoff)
        self.update_download_progress.connect(self._handle_download_progress)
        self.update_state_changed.connect(self._handle_update_state_changed)
        self.view.loadFinished.connect(self._on_view_load_finished)
        self._enter_startup_shell_mode()
        self._install_watcher()
        self.load_page()
        self._refresh_status_bar()

        # Poll JS command queue (fallback for when QWebChannel sendCommand doesn't work)
        self._cmd_poll_timer = QTimer(self)
        self._cmd_poll_timer.setInterval(50)
        self._cmd_poll_timer.timeout.connect(self._poll_js_command_queue)
        self._cmd_poll_timer.start()

        if self.updater and auto_update_check and self.is_packaged:
            QTimer.singleShot(2500, lambda: self.check_for_updates_async(manual=False))

    def handle_bms_data(self, data: dict):
        """Handle BMS serial data"""
        self.latest_bms_data = data
        self.merge_and_send_data()

    def handle_eload_data(self, data: dict):
        """Handle E-Load serial data"""
        self.latest_eload_data = data
        self.merge_and_send_data()

    def merge_and_send_data(self):
        """Merge BMS and E-Load data and send to frontend"""
        merged = self.latest_bms_data.copy()

        # Add E-Load data if available
        if self.latest_eload_data:
            merged['eload'] = self.latest_eload_data.get('eload', {})

        # Send to frontend
        json_str = json.dumps(merged)
        self.view.page().runJavaScript(
            f"if(window.updateDashboard) window.updateDashboard({json_str});"
        )

    def _poll_js_command_queue(self):
        """Poll the JS command queue and forward commands to serial."""
        self.view.page().runJavaScript(
            "window.__bmsDrainCommands ? window.__bmsDrainCommands() : ''",
            self._process_drained_commands,
        )

    def _process_drained_commands(self, result):
        """Callback for drained JS commands. Routes by prefix."""
        if not result:
            return
        try:
            cmds = json.loads(result)
            for cmd in cmds:
                cmd_str = str(cmd)
                print(f"[CMD-POLL] Forwarding: {cmd_str!r}", flush=True)
                if cmd_str.startswith("SERIAL:"):
                    self._handle_serial_command(cmd_str)
                elif cmd_str.startswith("ELOAD:"):
                    if self.eload_worker:
                        fw_cmd = self._translate_eload_command(cmd_str)
                        if fw_cmd:
                            self.eload_worker.send_command(fw_cmd)
                    else:
                        print(f"[CMD-POLL] E-Load worker not available, dropping: {cmd_str!r}", flush=True)
                else:
                    self.serial_worker.send_command(cmd_str)
        except Exception as e:
            print(f"[CMD-POLL] Error: {e}", flush=True)

    def _translate_eload_command(self, cmd):
        """Translate dashboard ELOAD: commands to firmware serial protocol."""
        if cmd == "ELOAD:ON":
            return "E 1"
        elif cmd == "ELOAD:OFF":
            return "E 0"
        elif cmd == "ELOAD:STATUS":
            return "S"
        elif cmd.startswith("ELOAD:CH:"):
            # ELOAD:CH:1:1  or  ELOAD:CH:3:0  — per-channel toggle
            parts = cmd.split(":")
            if len(parts) == 4:
                try:
                    ch = int(parts[2])
                    state = int(parts[3])
                    if 1 <= ch <= 4 and state in (0, 1):
                        return f"L {ch} {state}"
                except ValueError:
                    pass
            print(f"[CMD-POLL] Invalid channel command: {cmd!r}", flush=True)
            return None
        else:
            print(f"[CMD-POLL] Unknown ELOAD command: {cmd!r}", flush=True)
            return None

    def _handle_serial_command(self, cmd_str: str):
        """Handle SERIAL:* commands from the frontend UI for port management.

        Format: SERIAL:<SCAN|BMS|ELOAD>:<CONNECT|DISCONNECT>[:<port>:<baud>]
        Examples:
          SERIAL:SCAN
          SERIAL:BMS:CONNECT:/dev/cu.usbmodem14101:115200
          SERIAL:BMS:DISCONNECT
          SERIAL:ELOAD:CONNECT:/dev/cu.usbmodem14201:115200
          SERIAL:ELOAD:DISCONNECT
        """
        parts = cmd_str.split(":")
        # parts[0]=SERIAL, parts[1]=device, parts[2]=verb, parts[3]=port, parts[4]=baud
        device = parts[1] if len(parts) > 1 else ""
        verb = parts[2] if len(parts) > 2 else ""

        if device == "SCAN":
            ports = SerialWorker.list_available_ports()
            ports_json = json.dumps(ports)
            self.view.page().runJavaScript(
                f"if(window.__bmsUpdatePortList) window.__bmsUpdatePortList({ports_json});"
            )
            return

        if device == "BMS":
            if verb == "CONNECT" and len(parts) >= 5:
                port = parts[3]
                baud = int(parts[4]) if parts[4].isdigit() else 115200
                print(f"[SERIAL] BMS connect: {port} @ {baud}", flush=True)
                self.serial_worker.set_baudrate(baud)
                self.serial_worker.set_target_port(port)
                self.settings.set_serial(port, baud)
            elif verb == "DISCONNECT":
                print("[SERIAL] BMS disconnect", flush=True)
                self.serial_worker.pause()
            return

        if device == "ELOAD":
            if verb == "CONNECT" and len(parts) >= 5:
                port = parts[3]
                baud = int(parts[4]) if parts[4].isdigit() else 115200
                print(f"[SERIAL] E-Load connect: {port} @ {baud}", flush=True)
                self.eload_worker.set_baudrate(baud)
                self.eload_worker.set_target_port(port)
                self.settings.set_eload(port, baud)
            elif verb == "DISCONNECT":
                print("[SERIAL] E-Load disconnect", flush=True)
                self.eload_worker.pause()
                self.settings.set_eload(None, self.settings.eload_baudrate())
            return

        print(f"[SERIAL] Unknown serial command: {cmd_str!r}", flush=True)

    def handle_eload_connection_status(self, connected: bool):
        """Handle E-Load connection status changes"""
        status_text = "Connected to E-Load" if connected else "E-Load Disconnected"
        print(f"[E-Load] {status_text}")
        connected_js = "true" if connected else "false"
        port = self.eload_worker.get_connected_port() or "" if self.eload_worker else ""
        port_js = json.dumps(port)
        self.view.page().runJavaScript(
            f"if(window.__eloadSyncSerialConfigPanel) window.__eloadSyncSerialConfigPanel({connected_js}, {port_js});"
        )

    def handle_eload_connected_port_change(self, connected_port: str):
        """Handle E-Load connected port change"""
        port_js = json.dumps(connected_port or "")
        connected_js = "true" if connected_port else "false"
        self.view.page().runJavaScript(
            f"if(window.__eloadSyncSerialConfigPanel) window.__eloadSyncSerialConfigPanel({connected_js}, {port_js});"
        )

    def handle_connection_status(self, is_connected: bool):
        self._set_frontend_connection_state(is_connected)
        if not is_connected:
            self._has_serial_data = False
            self.view.page().runJavaScript(
                'if(window.clearDashboardData) window.clearDashboardData("disconnect");'
            )
        self._refresh_status_bar()

    def handle_data_activity(self):
        self._has_serial_data = True
        self._refresh_status_bar()

    def handle_raw_serial_line(self, line: str):
        """Forward raw serial lines to the frontend terminal."""
        escaped = json.dumps(line)
        self.view.page().runJavaScript(
            f"if(window.__bmsTerminalAppend) window.__bmsTerminalAppend({escaped});"
        )

    def handle_connected_port_change(self, connected_port: str):
        self._current_connected_port = connected_port or ""
        if not self._startup_transition_done:
            self._send_startup_connection_target()
        # Forward port name to frontend serial config panel
        port_js = json.dumps(connected_port or "")
        self.view.page().runJavaScript(
            f"if(window.__bmsSyncSerialConfigPanel) window.__bmsSyncSerialConfigPanel(!!{port_js}, {port_js});"
        )
        self._refresh_status_bar()

    def _set_frontend_connection_state(self, connected: bool):
        state_js = "true" if connected else "false"
        if not self._startup_transition_done:
            self._startup_connection_target_last = bool(connected)
            self.view.page().runJavaScript(
                f"if(window.__bmsSetStartupConnectionTarget) window.__bmsSetStartupConnectionTarget({state_js});"
            )
            return
        self.view.page().runJavaScript(
            f"if(window.setConnectionStatus) window.setConnectionStatus({state_js});"
        )

    def _refresh_status_bar(self):
        if self._startup_mode_active and not self._startup_transition_done:
            return
        target_port = self.serial_worker.get_target_port() or SERIAL_AUTO_LABEL
        connected_port = self._current_connected_port
        if connected_port:
            message = f"Serial connected: {connected_port} @ {self.baudrate}"
        else:
            message = f"Serial disconnected (target: {target_port}) @ {self.baudrate}"
        self.statusBar().showMessage(message)

    def _center_window(self) -> None:
        screen = QApplication.primaryScreen()
        if screen is None:
            return
        geometry = screen.availableGeometry()
        x = geometry.x() + (geometry.width() - self.width()) // 2
        y = geometry.y() + (geometry.height() - self.height()) // 2
        self.move(x, y)

    def _target_dashboard_geometry(self) -> QRect:
        screen = QApplication.primaryScreen()
        if screen is None:
            return QRect(self.x(), self.y(), DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT)
        avail = screen.availableGeometry()
        w = min(DEFAULT_WINDOW_WIDTH, avail.width() - 40)
        h = min(DEFAULT_WINDOW_HEIGHT, avail.height() - 40)
        x = avail.x() + (avail.width() - w) // 2
        y = avail.y() + (avail.height() - h) // 2
        return QRect(x, y, w, h)

    def _enter_startup_shell_mode(self) -> None:
        self.setWindowFlags(self._normal_window_flags | Qt.WindowType.Window)
        self.setGeometry(self._target_dashboard_geometry())
        self.setWindowOpacity(1.0)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground, False)
        self.setAutoFillBackground(True)
        self.setStyleSheet("")
        self.view.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground, False)
        self.view.setStyleSheet("")
        self.view.page().setBackgroundColor(QColor(10, 12, 16))
        if self.toolbar:
            self.toolbar.setVisible(False)
        self.statusBar().setVisible(False)
        self._chrome_cue_applied = False

    def _apply_chrome_cue(self) -> None:
        if self._chrome_cue_applied:
            return
        # Toolbar is permanently hidden — port selection is handled in-page
        self.statusBar().setVisible(True)
        self.statusBar().showMessage("Starting dashboard...")
        self._chrome_cue_applied = True

    def _send_startup_connection_target(self, force: bool = False) -> None:
        if self._startup_transition_done and not force:
            return
        is_connected = self.serial_worker.get_connected_port() is not None
        if not force and self._startup_connection_target_last is is_connected:
            return
        self._startup_connection_target_last = is_connected
        state_js = "true" if is_connected else "false"
        self.view.page().runJavaScript(
            f"if(window.__bmsSetStartupConnectionTarget) window.__bmsSetStartupConnectionTarget({state_js});"
        )

    def _on_view_load_finished(self, ok: bool) -> None:
        if self._startup_transition_done:
            return
        if not ok:
            logger.warning("Initial page load failed. Transitioning to full dashboard window.")
            self._transition_to_full_dashboard_window("page-load-failed")
            return
        self._start_boot_polling()

    def _start_boot_polling(self) -> None:
        if self._startup_transition_done:
            return
        if self._boot_poll_timer.isActive():
            return
        self._send_startup_connection_target(force=True)
        self._boot_poll_timer.start()
        self._poll_boot_state_once()

    def _poll_boot_state_once(self) -> None:
        if self._startup_transition_done:
            if self._boot_poll_timer.isActive():
                self._boot_poll_timer.stop()
            return

        elapsed_ms = (time.monotonic() - self._startup_started_at) * 1000.0
        if elapsed_ms >= self._boot_poll_timeout_ms:
            logger.warning("Boot polling timed out after %.0f ms", elapsed_ms)
            self._transition_to_full_dashboard_window("boot-timeout")
            return

        self.view.page().runJavaScript(
            "window.__bmsBootDebug ? window.__bmsBootDebug() : null",
            self._handle_boot_state,
        )

    def _handle_boot_state(self, state: Any) -> None:
        if self._startup_transition_done:
            return

        state_dict: Optional[dict[str, Any]] = state if isinstance(state, dict) else None
        if state_dict is not None:
            self._boot_last_state = state_dict

        elapsed_ms = (time.monotonic() - self._startup_started_at) * 1000.0
        min_time_met = elapsed_ms >= self._startup_min_duration_ms

        phase = ""
        if state_dict is not None:
            phase = str(state_dict.get("phase", "")).strip().lower()

        if phase == "chrome_cue":
            self._apply_chrome_cue()

        ready_by_js = bool(
            state_dict
            and (phase == "complete" or state_dict.get("hidden") is True)
        )
        errored = bool(
            state_dict
            and (phase == "error" or state_dict.get("errored") is True)
        )

        if ready_by_js and min_time_met:
            self._transition_to_full_dashboard_window("boot-ready")
            return
        if errored and min_time_met:
            self._transition_to_full_dashboard_window("boot-error")
            return

    def _transition_to_full_dashboard_window(self, reason: str) -> None:
        if self._startup_transition_done:
            return
        self._startup_transition_done = True
        self._startup_mode_active = False

        if self._boot_poll_timer.isActive():
            self._boot_poll_timer.stop()

        self._apply_chrome_cue()
        self._finish_startup_transition(reason)

    def _finish_startup_transition(self, reason: str) -> None:

        if reason == "boot-error":
            self.statusBar().showMessage("Startup error: frontend boot failed.", 10000)
        elif reason == "boot-timeout":
            self.statusBar().showMessage("Startup timeout: opening dashboard anyway.", 10000)
        elif reason == "page-load-failed":
            self.statusBar().showMessage("Startup page load failed: opening dashboard anyway.", 10000)
        else:
            self._refresh_status_bar()

        self._send_startup_connection_target(force=True)
        current_connected = self.serial_worker.get_connected_port() is not None
        self._set_frontend_connection_state(current_connected)
        # Re-send port name now that JS is ready
        port_js = json.dumps(self._current_connected_port or "")
        self.view.page().runJavaScript(
            f"if(window.__bmsSyncSerialConfigPanel) window.__bmsSyncSerialConfigPanel({str(current_connected).lower()}, {port_js});"
        )

        # Send available ports to frontend
        ports = SerialWorker.list_available_ports()
        ports_json = json.dumps(ports)
        self.view.page().runJavaScript(
            f"if(window.__bmsUpdatePortList) window.__bmsUpdatePortList({ports_json});"
        )

        # Sync E-Load serial status
        eload_port = self.eload_worker.get_connected_port() or "" if self.eload_worker else ""
        eload_connected_js = "true" if eload_port else "false"
        eload_port_js = json.dumps(eload_port)
        self.view.page().runJavaScript(
            f"if(window.__eloadSyncSerialConfigPanel) window.__eloadSyncSerialConfigPanel({eload_connected_js}, {eload_port_js});"
        )

        self._show_pending_update_result()

        if self.is_packaged:
            QTimer.singleShot(800, self._resume_staged_update_if_ready)

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
        if self._boot_poll_timer.isActive():
            self._boot_poll_timer.stop()
        if hasattr(self, "serial_worker"):
            self.serial_worker.stop()
            self.serial_thread.quit()
            self.serial_thread.wait()
        if hasattr(self, "eload_worker") and self.eload_worker:
            self.eload_worker.stop()
        if hasattr(self, "eload_thread") and self.eload_thread:
            self.eload_thread.quit()
            self.eload_thread.wait()

        if hasattr(self, "httpd"):
            self.httpd.shutdown()

        event.accept()

    def _build_toolbar(self) -> None:
        self.toolbar = self.addToolBar("Controls")
        self.toolbar.setMovable(False)
        self.toolbar.setVisible(False)  # Serial config is now in-page UI

        reload_action = QAction(QIcon.fromTheme("view-refresh"), "Reload", self)
        reload_action.setStatusTip("Force-reload the dashboard surface")
        reload_action.triggered.connect(self.reload)
        self.toolbar.addAction(reload_action)

        open_action = QAction("Open...", self)
        open_action.setStatusTip("Choose a different HTML entrypoint")
        open_action.triggered.connect(self._choose_entrypoint)
        self.toolbar.addAction(open_action)

        serial_action = QAction("Serial Port...", self)
        serial_action.setStatusTip("Choose target serial port or auto-detect mode")
        serial_action.triggered.connect(self._choose_serial_port)
        self.toolbar.addAction(serial_action)

        baud_action = QAction("Baudrate...", self)
        baud_action.setStatusTip("Set serial baudrate")
        baud_action.triggered.connect(self._choose_baudrate)
        self.toolbar.addAction(baud_action)

        self._update_action = QAction("Check Updates", self)
        self._update_action.setStatusTip("Check GitHub Releases for an update")
        self._update_action.triggered.connect(lambda: self.check_for_updates_async(manual=True))
        if not self.is_packaged:
            self._update_action.setEnabled(False)
            self._update_action.setStatusTip(
                "Dev/source mode updates are via git/pull; app self-update is disabled."
            )
        self.toolbar.addAction(self._update_action)

    def _build_update_indicator(self) -> None:
        status_bar = self.statusBar()
        self._update_state_label = QLabel("", self)
        self._update_state_label.setVisible(False)
        self._update_progress_bar = QProgressBar(self)
        self._update_progress_bar.setVisible(False)
        self._update_progress_bar.setMinimum(0)
        self._update_progress_bar.setMaximum(100)
        self._update_progress_bar.setValue(0)
        self._update_progress_bar.setTextVisible(True)
        self._update_progress_bar.setFixedWidth(220)
        status_bar.addPermanentWidget(self._update_state_label)
        status_bar.addPermanentWidget(self._update_progress_bar)

    @staticmethod
    def _normalize_update_status(status: str) -> str:
        normalized = (status or "").strip().lower()
        if normalized == "staging":
            return "preparing"
        if normalized == "restart_required":
            return "restarting"
        if normalized == "close_required":
            return "installing"
        return normalized

    def _consume_update_result(self) -> Optional[dict[str, Any]]:
        path = default_update_result_path()
        if not path.exists():
            return None
        try:
            payload = json.loads(path.read_text(encoding="utf-8"))
            if not isinstance(payload, dict):
                return None
            return payload
        except Exception:
            return None
        finally:
            try:
                path.unlink(missing_ok=True)
            except Exception:
                logger.exception("Failed to delete consumed updater result file: %s", path)

    def _show_pending_update_result(self) -> None:
        payload = self._pending_update_result
        if not payload:
            return
        self._pending_update_result = None

        status = self._normalize_update_status(str(payload.get("status", "")).strip().lower())
        version = str(payload.get("version", "")).strip()
        error = str(payload.get("error", "")).strip()
        log_path = str(default_update_log_path())

        if status == "success":
            message = (
                f"Update completed successfully to v{version}."
                if version
                else "Update completed successfully."
            )
            self.update_state_changed.emit("success", message, 100, 100)
            self.statusBar().showMessage(message, 10000)
            QTimer.singleShot(3000, lambda: self.update_state_changed.emit("idle", "", 0, 0))
            return

        message = error or "A previous update attempt failed."
        self.update_state_changed.emit("error", message, 0, 0)
        QMessageBox.warning(
            self,
            "Updates",
            "A previous update attempt did not complete successfully.\n\n"
            f"{message}\n\n"
            f"Updater log: {log_path}",
        )

    def _ensure_update_progress_dialog(self) -> UpdateProgressDialog:
        if self._update_progress_dialog is None:
            self._update_progress_dialog = UpdateProgressDialog(self)
        return self._update_progress_dialog

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
        if not self.is_packaged:
            message = "Dev/source mode updates are via git/pull; app self-update is disabled."
            self.update_state_changed.emit("error", message, 0, 0)
            if manual:
                QMessageBox.information(self, "Updates", message)
            return

        staged = self.settings.staged_update()
        if staged:
            payload_path = Path(str(staged.get("payload_path", "")).strip())
            staged_version = str(staged.get("version", "")).strip()
            if (
                payload_path.exists()
                and staged.get("platform") == ReleaseUpdater.platform_key()
                and staged_version
                and self._is_newer_version(staged_version, APP_VERSION)
            ):
                self.update_state_changed.emit(
                    "restart_required",
                    f"Update v{staged_version} already staged.",
                    100,
                    100,
                )
                if manual:
                    self._prompt_restart_for_staged_update(staged_version)
                return
            self.settings.clear_staged_update()

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

        self.update_state_changed.emit("checking", "Checking for updates...", 0, 0)

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
            if self.is_packaged:
                self.settings.set_last_checked_now()

        self.update_check_finished.emit(info, error, manual)

    def _handle_update_result(self, info: Optional[UpdateInfo], error: Optional[Exception], manual: bool):
        if error:
            self.update_state_changed.emit("error", "Update check failed.", 0, 0)
            if manual:
                QMessageBox.warning(self, "Updates", f"Update check failed:\n{error}")
            return

        if info is None:
            self.update_state_changed.emit("idle", "", 0, 0)
            self.statusBar().showMessage("You are on the latest stable release.", 6000)
            if manual:
                QMessageBox.information(self, "Updates", "You are already on the latest stable release.")
            return

        self.update_state_changed.emit("idle", "", 0, 0)
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
        if sys.platform == "darwin":
            details = (
                "Download and install this update now?\n\n"
                "The currently installed app bundle will be updated in place, and a progress "
                "window will stay visible during download and installation."
            )
        else:
            details = (
                "Download and stage update in the background now?\n\n"
                "You will be prompted for restart/install actions when the package is ready."
            )
        if notes_preview:
            details += f"\n\nRelease notes:\n{notes_preview}"
        msg.setInformativeText(details)
        msg.setStandardButtons(QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No)
        if msg.exec() != QMessageBox.StandardButton.Yes:
            return

        if self._update_download_thread and self._update_download_thread.is_alive():
            QMessageBox.information(self, "Updates", "Update download already in progress.")
            return

        self._active_update_target = None
        if sys.platform == "darwin":
            target = self._resolve_installed_target()
            if target is None or target.suffix != ".app" or not target.exists():
                self.update_state_changed.emit("error", "Could not locate installed .app bundle.", 0, 0)
                QMessageBox.warning(
                    self,
                    "Updates",
                    "Could not locate the installed app bundle for in-place update.\n\n"
                    "Please reinstall this build and try again.",
                )
                return
            self._active_update_target = target
            self._ensure_update_progress_dialog().start_session(version=info.version, target_path=target)

        self._update_download_started_at = time.monotonic()
        self._update_download_version = info.version
        self.update_state_changed.emit(
            "downloading",
            f"Downloading {info.version}...",
            0,
            0,
        )

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
            self.update_state_changed.emit("downloading", f"Downloading {info.version}...", 0, 0)
            installer = self.updater.download_update(
                info,
                progress_callback=lambda done, total: self.update_download_progress.emit(done, total),
            )
            self.update_state_changed.emit("verifying", "Verifying payload...", 0, 0)
            if sys.platform == "darwin":
                target = self._active_update_target or self._resolve_installed_target()
                if target is None or target.suffix != ".app" or not target.exists():
                    raise UpdateError("Could not resolve installed app bundle for in-place update.")
                self.update_state_changed.emit(
                    "preparing",
                    f"Preparing in-place update for {target.name}…",
                    0,
                    0,
                )
                message = self.updater.install_update_and_restart(
                    installer,
                    app_bundle_path=target,
                    version=info.version,
                )
            else:
                self.update_state_changed.emit("preparing", "Launching installer...", 0, 0)
                message = self.updater.install_update_and_restart(installer)
        except Exception as exc:
            error = exc
            logger.exception("Update install handoff failed")

        self.update_install_finished.emit(message, error)

    def _handle_download_progress(self, downloaded_bytes: int, total_bytes: int):
        downloaded_bytes = max(0, int(downloaded_bytes))
        total_bytes = max(0, int(total_bytes))
        detail = ""
        if total_bytes > 0:
            percent = (downloaded_bytes / total_bytes) * 100.0
            detail = (
                f"Downloading {percent:.1f}% "
                f"({self._format_bytes(downloaded_bytes)} / {self._format_bytes(total_bytes)})"
            )
        else:
            detail = f"Downloading {self._format_bytes(downloaded_bytes)}"

        elapsed = max(0.001, time.monotonic() - self._update_download_started_at)
        speed_bps = downloaded_bytes / elapsed
        detail += f" @ {self._format_bytes(speed_bps)}/s"
        self.update_state_changed.emit("downloading", detail, downloaded_bytes, total_bytes)

    def _handle_install_handoff(self, message: Optional[object], error: Optional[Exception]):
        downloaded_version = self._update_download_version
        self._update_download_started_at = 0.0
        self._update_download_version = ""
        if error:
            self.update_state_changed.emit("error", "Update failed.", 0, 0)
            fallback_log = default_update_log_path()
            if self._update_progress_dialog:
                self._update_progress_dialog.set_error("Update failed.", str(fallback_log))
            QMessageBox.warning(
                self,
                "Updates",
                "Update failed.\n\n"
                f"{error}\n\n"
                "Please close BMS Dashboard and retry Check Updates.\n"
                f"If it still fails, run the downloaded installer manually.\n"
                f"Updater log: {fallback_log}",
            )
            self._active_update_target = None
            return

        status = ""
        status_message = message or "Update package prepared."
        installer_path = ""
        requires_quit = False
        version = ""
        log_path = str(default_update_log_path())
        if isinstance(message, dict):
            status = str(message.get("status", "")).strip().lower()
            status_message = str(message.get("message", status_message))
            installer_path = str(message.get("installer_path", "")).strip()
            requires_quit = str(message.get("requires_quit", "")).lower() == "true"
            version = str(message.get("version", "")).strip()
            log_path = str(message.get("log_path", log_path)).strip() or str(default_update_log_path())

        if status == "restart_required":
            if not installer_path:
                self.update_state_changed.emit("error", "Staged update payload not found.", 0, 0)
                if self._update_progress_dialog:
                    self._update_progress_dialog.set_error("Staged update payload not found.", log_path)
                return
            staged_version = version or downloaded_version or APP_VERSION
            self.settings.set_staged_update(
                version=staged_version,
                payload_path=installer_path,
                platform=ReleaseUpdater.platform_key(),
            )
            self.update_state_changed.emit(
                "restart_required",
                f"Ready to restart into v{staged_version}",
                100,
                100,
            )
            self._prompt_restart_for_staged_update(staged_version)
            self._active_update_target = None
            return

        normalized_status = self._normalize_update_status(status)

        if normalized_status == "error":
            self.update_state_changed.emit("error", status_message, 0, 0)
            if self._update_progress_dialog:
                self._update_progress_dialog.set_error(status_message, log_path)
            QMessageBox.warning(self, "Updates", status_message)
            self._active_update_target = None
            return

        if normalized_status in {"permission_prompt", "installing", "restarting"}:
            self.update_state_changed.emit(normalized_status, status_message, 0, 0)
            if normalized_status == "permission_prompt":
                if requires_quit:
                    choice = QMessageBox.question(
                        self,
                        "Updates",
                        f"{status_message}\n\nClose BMS Dashboard now to continue installation?",
                        QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                        QMessageBox.StandardButton.Yes,
                    )
                    if choice == QMessageBox.StandardButton.Yes:
                        self.update_state_changed.emit("restarting", "Restarting to apply update…", 100, 100)
                        QTimer.singleShot(700, self.close)
                    else:
                        self.update_state_changed.emit("idle", "", 0, 0)
                        self.statusBar().showMessage("Update paused until app closes.", 7000)
                self._active_update_target = None
                return
            if requires_quit:
                if sys.platform.startswith("win"):
                    choice = QMessageBox.question(
                        self,
                        "Updates",
                        f"{status_message}\n\nClose the app now to continue installation?",
                        QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                        QMessageBox.StandardButton.Yes,
                    )
                    if choice == QMessageBox.StandardButton.Yes:
                        QTimer.singleShot(120, self.close)
                    self._active_update_target = None
                    return
                if sys.platform == "darwin":
                    self.update_state_changed.emit("restarting", "Restarting to apply update…", 100, 100)
                    QTimer.singleShot(700, self.close)
                else:
                    QTimer.singleShot(180, self.close)
                self._active_update_target = None
                return
            if normalized_status != "permission_prompt":
                QMessageBox.information(self, "Updates", status_message)
            self._active_update_target = None
            return

        self.update_state_changed.emit("idle", "", 0, 0)
        if status_message:
            self.statusBar().showMessage(status_message, 6000)
        self._active_update_target = None

    def _resume_staged_update_if_ready(self) -> None:
        if not self.is_packaged or self._staged_restart_prompted:
            return
        staged = self.settings.staged_update()
        if not staged:
            return

        if staged.get("platform") != ReleaseUpdater.platform_key():
            self.settings.clear_staged_update()
            return

        payload_path = Path(str(staged.get("payload_path", "")).strip())
        if not payload_path.exists():
            self.settings.clear_staged_update()
            self.update_state_changed.emit("error", "Previously staged update payload is missing.", 0, 0)
            return

        staged_version = str(staged.get("version", "")).strip()
        if not staged_version or not self._is_newer_version(staged_version, APP_VERSION):
            self.settings.clear_staged_update()
            return

        self.update_state_changed.emit(
            "restart_required",
            f"Ready to restart into v{staged_version}",
            100,
            100,
        )
        self._prompt_restart_for_staged_update(staged_version)

    def _prompt_restart_for_staged_update(self, version: str) -> None:
        staged = self.settings.staged_update()
        if not staged:
            return
        self._staged_restart_prompted = True

        prompt = QMessageBox(self)
        prompt.setWindowTitle("Update Ready")
        prompt.setIcon(QMessageBox.Icon.Information)
        prompt.setText(f"Version {version} is staged and ready.")
        prompt.setInformativeText("Restart now to apply the update, or choose Later.")
        restart_btn = prompt.addButton("Restart Now", QMessageBox.ButtonRole.AcceptRole)
        later_btn = prompt.addButton("Later", QMessageBox.ButtonRole.RejectRole)
        prompt.setDefaultButton(restart_btn)
        prompt.exec()

        if prompt.clickedButton() is restart_btn:
            self._start_staged_install()
            return

        if prompt.clickedButton() is later_btn:
            self.statusBar().showMessage("Update staged. Restart later to apply.", 7000)
            self.update_state_changed.emit(
                "restart_required",
                f"Update v{version} staged (restart later).",
                100,
                100,
            )

    def _resolve_installed_target(self) -> Optional[Path]:
        exe_path = Path(sys.executable).resolve()
        if sys.platform == "darwin":
            for parent in exe_path.parents:
                if parent.suffix == ".app":
                    return parent
            return None
        return exe_path

    def _start_staged_install(self) -> None:
        staged = self.settings.staged_update()
        if not staged:
            self.update_state_changed.emit("error", "No staged update is available.", 0, 0)
            return
        if not self.updater:
            self.update_state_changed.emit("error", "Updater is not configured.", 0, 0)
            return

        payload_path = Path(str(staged.get("payload_path", "")).strip())
        if not payload_path.exists():
            self.settings.clear_staged_update()
            self.update_state_changed.emit("error", "Staged update payload no longer exists.", 0, 0)
            return

        target = self._resolve_installed_target()
        if target is None:
            self.update_state_changed.emit("error", "Could not locate installed app path.", 0, 0)
            return

        self.update_state_changed.emit("preparing", "Preparing staged update install...", 0, 0)
        staged_version = str(staged.get("version", "")).strip()
        result = self.updater.install_update_and_restart(
            payload_path,
            app_bundle_path=target,
            version=staged_version,
        )
        status = str(result.get("status", "")).strip().lower()
        normalized = self._normalize_update_status(status)
        if normalized not in {"installing", "permission_prompt", "restarting"}:
            message = str(result.get("message", "Failed to launch updater helper."))
            self.update_state_changed.emit("error", message, 0, 0)
            QMessageBox.warning(self, "Updates", message)
            return

        self.settings.clear_staged_update()
        self.update_state_changed.emit(
            normalized,
            str(result.get("message", "Installing update and restarting...")),
            0,
            0,
        )
        if normalized == "permission_prompt":
            choice = QMessageBox.question(
                self,
                "Updates",
                f"{result.get('message', 'Administrator permission requested.')}\n\n"
                "Close BMS Dashboard now to continue installation?",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                QMessageBox.StandardButton.Yes,
            )
            if choice == QMessageBox.StandardButton.Yes:
                self.update_state_changed.emit("restarting", "Restarting to apply update…", 100, 100)
                QTimer.singleShot(700, self.close)
            else:
                self.update_state_changed.emit("idle", "", 0, 0)
                self.statusBar().showMessage("Update paused until app closes.", 7000)
            return
        if normalized in {"installing", "restarting"}:
            QTimer.singleShot(180, self.close)

    def _handle_update_state_changed(
        self,
        status: str,
        detail: str,
        current: int,
        total: int,
    ) -> None:
        if self._update_state_label is None or self._update_progress_bar is None:
            return
        normalized = self._normalize_update_status(status)

        if self._update_progress_dialog:
            if normalized in {"", "idle"}:
                if self._update_progress_dialog.isVisible():
                    self._update_progress_dialog.hide()
            else:
                self._update_progress_dialog.apply_state(normalized, detail, current, total)

        if normalized in {"", "idle"}:
            self._update_state_label.setVisible(False)
            self._update_progress_bar.setVisible(False)
            return

        label_text = detail.strip() if detail else normalized.replace("_", " ").title()
        self._update_state_label.setText(f"Updater: {label_text}")
        self._update_state_label.setVisible(True)

        if normalized == "downloading":
            self._update_progress_bar.setVisible(True)
            if total > 0:
                percent = int(max(0.0, min(100.0, (current / total) * 100.0)))
                self._update_progress_bar.setRange(0, 100)
                self._update_progress_bar.setValue(percent)
                self._update_progress_bar.setFormat(f"{percent}%")
            else:
                self._update_progress_bar.setRange(0, 0)
                self._update_progress_bar.setFormat("...")
            return

        if normalized in {"restarting", "success"}:
            self._update_progress_bar.setVisible(True)
            self._update_progress_bar.setRange(0, 100)
            self._update_progress_bar.setValue(100)
            self._update_progress_bar.setFormat("100%")
            return

        if normalized in {"checking", "verifying", "preparing", "permission_prompt", "installing", "error"}:
            self._update_progress_bar.setVisible(False)
            return

        self._update_progress_bar.setVisible(False)

    @staticmethod
    def _is_newer_version(candidate: str, baseline: str) -> bool:
        try:
            left = Version(candidate.lstrip("v"))
            right = Version(baseline.lstrip("v"))
            return left > right
        except InvalidVersion:
            return candidate.strip() != baseline.strip()

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
    parser.add_argument("--run-update-helper", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--installer", type=Path, default=None, help=argparse.SUPPRESS)
    parser.add_argument("--target-app", type=Path, default=None, help=argparse.SUPPRESS)
    parser.add_argument("--wait-pid", type=int, default=None, help=argparse.SUPPRESS)
    parser.add_argument("--relaunch", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--result-file", type=Path, default=None, help=argparse.SUPPRESS)
    parser.add_argument("--update-version", default="", help=argparse.SUPPRESS)
    return parser.parse_args()


def main() -> int:
    logging.basicConfig(level=logging.INFO)
    args = parse_args()
    if args.run_update_helper:
        if args.installer is None or args.target_app is None:
            return 2
        return run_update_helper(
            installer=args.installer,
            target_app=args.target_app,
            wait_pid=args.wait_pid,
            relaunch=bool(args.relaunch),
            result_file=args.result_file,
            version=args.update_version,
        )

    settings = SettingsStore()
    stored_baudrate = settings.serial_baudrate()
    is_packaged = bool(getattr(sys, "frozen", False))

    if args.serial_port == SERIAL_PORT_DEFAULT_SENTINEL:
        # Default startup behavior is always auto-detect unless explicitly overridden.
        serial_port = None
    else:
        serial_port = normalize_serial_port_arg(args.serial_port)

    baudrate = args.baudrate if args.baudrate > 0 else stored_baudrate
    settings.set_serial(serial_port, baudrate)
    settings.set_update_channel(args.update_channel)

    entrypoint = resolve_entrypoint(args.entrypoint)
    repo_slug = args.update_repo or detect_repo_slug(source_root())
    updater = None
    if is_packaged and repo_slug:
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
            is_packaged=is_packaged,
            app_icon=icon,
        )
    except (FileNotFoundError, RuntimeError) as exc:
        QMessageBox.critical(None, "Dashboard launcher", str(exc))
        return 1

    window.show()
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
