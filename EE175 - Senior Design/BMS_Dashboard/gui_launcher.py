"""Standalone launcher for the BMS dashboard frontend.

This script wraps the Three.js/GSAP experience inside a PyQt6 desktop window
so the user can treat it as an external application instead of visiting the
page in a browser. The launcher watches the main frontend files and reloads
the view automatically whenever they change, which keeps the workflow tight
while iterating on the UI.
"""

from __future__ import annotations

import argparse
import http.server
import socketserver
import sys
import threading
from pathlib import Path

from PyQt6.QtCore import QFileSystemWatcher, QTimer, QUrl
from PyQt6.QtGui import QAction, QIcon
from PyQt6.QtWidgets import QApplication, QFileDialog, QMainWindow, QMessageBox
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtCore import QThread, pyqtSlot

from backend.data_stream import SerialWorker


class DashboardWindow(QMainWindow):
    """Main window hosting the WebEngine view."""

    def __init__(self, entrypoint: Path) -> None:
        super().__init__()
        self.setWindowTitle("BMS Command Surface")
        self.resize(1400, 900)

        self.view = QWebEngineView(self)
        self.setCentralWidget(self.view)

        self.entrypoint = entrypoint.resolve()
        if not self.entrypoint.exists():
            raise FileNotFoundError(f"Unable to find {self.entrypoint}")

        # Start HTTP server for proper resource loading
        self.http_port = 8765
        self.start_http_server()

        self._build_toolbar()
        self._install_watcher()
        self.load_page()

    # --------------------------------------------------------------------- HTTP Server
    def start_http_server(self) -> None:
        """Start a simple HTTP server in a background thread to serve the frontend files."""
        frontend_dir = self.entrypoint.parent
        
        class Handler(http.server.SimpleHTTPRequestHandler):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, directory=str(frontend_dir), **kwargs)
            
            def log_message(self, format, *args):
                # Suppress HTTP server logs
                pass
        
        self.httpd = socketserver.TCPServer(("", self.http_port), Handler)
        self.server_thread = threading.Thread(target=self.httpd.serve_forever, daemon=True)
        self.server_thread.start()

    # --------------------------------------------------------------------- util
    def load_page(self) -> None:
        """Load the HTML entrypoint into the embedded browser."""
        # Use HTTP instead of file:// to allow loading 3D models
        filename = self.entrypoint.name
        url = QUrl(f"http://localhost:{self.http_port}/{filename}")
        self.view.load(url)

    def reload(self) -> None:
        self.view.reload()
    
    def closeEvent(self, event) -> None:
        """Cleanup HTTP server and threads on window close."""
        if hasattr(self, 'serial_worker'):
            self.serial_worker.stop()
            self.serial_thread.quit()
            self.serial_thread.wait()
        
        if hasattr(self, 'httpd'):
            self.httpd.shutdown()
        event.accept()

    # ---------------------------------------------------------------- toolbar
    def _build_toolbar(self) -> None:
        toolbar = self.addToolBar("Controls")
        toolbar.setMovable(False)

        reload_action = QAction(QIcon.fromTheme("view-refresh"), "Reload", self)
        reload_action.setStatusTip("Force-reload the dashboard surface")
        reload_action.triggered.connect(self.reload)
        toolbar.addAction(reload_action)

        open_action = QAction("Open…", self)
        open_action.setStatusTip("Choose a different HTML entrypoint")
        open_action.triggered.connect(self._choose_entrypoint)
        toolbar.addAction(open_action)

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

    # -------------------------------------------------------------- file watch
    def _install_watcher(self) -> None:
        """Watch core frontend files so edits trigger a soft reload."""
        files_to_watch = [
            self.entrypoint,
            self.entrypoint.parent / "scene.js",
            self.entrypoint.parent / "style.css",
        ]
        existing = [str(path) for path in files_to_watch if path.exists()]

        self.watcher = QFileSystemWatcher(existing, self)
        self.watcher.fileChanged.connect(self._debounced_reload)

    def _debounced_reload(self, _path: str) -> None:
        # Delay to give the file time to finish writing.
        QTimer.singleShot(250, self.reload)


# --------------------------------------------------------------------------- CLI
def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Launch the BMS dashboard GUI.")
    parser.add_argument(
        "--entrypoint",
        type=Path,
        default=Path(__file__).parent / "frontend" / "index.html",
        help="Path to the HTML dashboard to load (defaults to frontend/index.html).",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    app = QApplication(sys.argv)
    app.setApplicationName("BMS Dashboard")
    window = None
    try:
        window = DashboardWindow(args.entrypoint)
    except FileNotFoundError as exc:
        QMessageBox.critical(None, "Dashboard launcher", str(exc))
        return 1

    window.show()
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
