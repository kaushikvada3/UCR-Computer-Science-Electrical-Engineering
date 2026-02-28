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
    "eload": {
        "port": None,  # Set to COM port when E-Load is connected
        "baudrate": 115200,
    },
    "updates": {
        "channel": "stable",
        "last_checked_utc": None,
        "staged": None,
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

    def eload_port(self) -> Optional[str]:
        value = self._data.get("eload", {}).get("port")
        if value is None:
            return None  # E-Load disabled
        text = str(value).strip()
        return text if text else None

    def eload_baudrate(self) -> int:
        value = self._data.get("eload", {}).get("baudrate", 115200)
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

    def set_eload(self, port: Optional[str], baudrate: int) -> None:
        normalized_port = None
        if port is not None:
            text = str(port).strip()
            normalized_port = text if text else None

        self._data.setdefault("eload", {})
        self._data["eload"]["port"] = normalized_port
        self._data["eload"]["baudrate"] = int(baudrate)
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

    def staged_update(self) -> Optional[Dict[str, Any]]:
        value = self._data.get("updates", {}).get("staged")
        if not isinstance(value, dict):
            return None
        version = str(value.get("version", "")).strip()
        payload_path = str(value.get("payload_path", "")).strip()
        created_utc = str(value.get("created_utc", "")).strip()
        platform = str(value.get("platform", "")).strip()
        if not version or not payload_path or not platform:
            return None
        return {
            "version": version,
            "payload_path": payload_path,
            "created_utc": created_utc,
            "platform": platform,
        }

    def set_staged_update(
        self,
        *,
        version: str,
        payload_path: str,
        platform: str,
        created_utc: Optional[str] = None,
    ) -> None:
        normalized_version = str(version).strip()
        normalized_payload = str(payload_path).strip()
        normalized_platform = str(platform).strip()
        if not normalized_version or not normalized_payload or not normalized_platform:
            self.clear_staged_update()
            return
        self._data.setdefault("updates", {})
        self._data["updates"]["staged"] = {
            "version": normalized_version,
            "payload_path": normalized_payload,
            "created_utc": created_utc or datetime.now(timezone.utc).isoformat(),
            "platform": normalized_platform,
        }
        self.save()

    def clear_staged_update(self) -> None:
        self._data.setdefault("updates", {})
        self._data["updates"]["staged"] = None
        self.save()
