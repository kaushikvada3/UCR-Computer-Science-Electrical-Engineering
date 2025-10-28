"""Standalone launcher for the BMS dashboard frontend.

This script wraps the Three.js/GSAP experience inside a PyQt6 desktop window
so the user can treat it as an external application instead of visiting the
page in a browser. The launcher watches the main frontend files and reloads
the view automatically whenever they change, which keeps the workflow tight
while iterating on the UI.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from PyQt6.QtCore import QFileSystemWatcher, QTimer, QUrl
from PyQt6.QtGui import QAction, QIcon
from PyQt6.QtWidgets import QApplication, QFileDialog, QMainWindow, QMessageBox
from PyQt6.QtWebEngineWidgets import QWebEngineView


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

        self._build_toolbar()
        self._install_watcher()
        self.load_page()

    # --------------------------------------------------------------------- util
    def load_page(self) -> None:
        """Load the HTML entrypoint into the embedded browser."""
        url = QUrl.fromLocalFile(str(self.entrypoint))
        self.view.load(url)

    def reload(self) -> None:
        self.view.reload()

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
