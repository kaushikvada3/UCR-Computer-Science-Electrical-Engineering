from __future__ import annotations

import json
import os
import sys
from copy import deepcopy
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, Optional


APP_NAME = "BMSDashboard"
DEFAULT_SETTINGS: Dict[str, Any] = {
    "serial": {
        "port": None,
        "baudrate": 115200,
    },
    "updates": {
        "channel": "stable",
        "last_checked_utc": None,
    },
}


def _deep_merge(base: Dict[str, Any], override: Dict[str, Any]) -> Dict[str, Any]:
    out = deepcopy(base)
    for key, value in override.items():
        if isinstance(value, dict) and isinstance(out.get(key), dict):
            out[key] = _deep_merge(out[key], value)
        else:
            out[key] = value
    return out


class SettingsStore:
    def __init__(self, path: Optional[Path] = None):
        self.path = path or self.default_path()
        self._data: Dict[str, Any] = deepcopy(DEFAULT_SETTINGS)
        self.load()

    @staticmethod
    def default_path() -> Path:
        if sys.platform.startswith("win"):
            appdata = os.environ.get("APPDATA")
            root = Path(appdata) if appdata else Path.home() / "AppData" / "Roaming"
            return root / APP_NAME / "config.json"
        if sys.platform == "darwin":
            return Path.home() / "Library" / "Application Support" / APP_NAME / "config.json"
        return Path.home() / ".config" / APP_NAME / "config.json"

    def load(self) -> Dict[str, Any]:
        if not self.path.exists():
            self._data = deepcopy(DEFAULT_SETTINGS)
            return self._data

        try:
            raw = json.loads(self.path.read_text(encoding="utf-8"))
            if not isinstance(raw, dict):
                raw = {}
        except (OSError, json.JSONDecodeError):
            raw = {}

        self._data = _deep_merge(DEFAULT_SETTINGS, raw)
        return self._data

    def save(self) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.path.write_text(json.dumps(self._data, indent=2), encoding="utf-8")

    def data(self) -> Dict[str, Any]:
        return deepcopy(self._data)

    def serial_port(self) -> Optional[str]:
        value = self._data.get("serial", {}).get("port")
        if value is None:
            return None
        text = str(value).strip()
        return text or None

    def serial_baudrate(self) -> int:
        value = self._data.get("serial", {}).get("baudrate", 115200)
        try:
            return int(value)
        except (TypeError, ValueError):
            return 115200

    def update_channel(self) -> str:
        value = str(self._data.get("updates", {}).get("channel", "stable")).strip().lower()
        return value or "stable"

    def set_serial(self, port: Optional[str], baudrate: int) -> None:
        normalized_port = None
        if port is not None:
            text = str(port).strip()
            normalized_port = text or None

        self._data.setdefault("serial", {})
        self._data["serial"]["port"] = normalized_port
        self._data["serial"]["baudrate"] = int(baudrate)
        self.save()

    def set_update_channel(self, channel: str) -> None:
        normalized = (channel or "stable").strip().lower()
        if normalized not in {"stable"}:
            normalized = "stable"
        self._data.setdefault("updates", {})
        self._data["updates"]["channel"] = normalized
        self.save()

    def set_last_checked_now(self) -> None:
        self._data.setdefault("updates", {})
        self._data["updates"]["last_checked_utc"] = datetime.now(timezone.utc).isoformat()
        self.save()
