import logging
import math
import json
import re
from collections import deque
from threading import Lock
from typing import Any, Dict, List, Optional
import serial
import serial.tools.list_ports
from PyQt6.QtCore import QObject, pyqtSignal
import time

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("SerialMonitor")
NUMERIC_TOKEN_RE = re.compile(r'(?<![A-Za-z0-9])[-+]?(?:\d+\.\d+|\d+|\.\d+)(?:[eE][-+]?\d+)?')
CELLS_FRAME_RE = re.compile(r'cells\s*(?:\([^)]+\))?\s*:\s*\[(.*?)\]', re.IGNORECASE)
CELL_VOLTAGES_FRAME_RE = re.compile(
    r'cell\s*voltages?\s*(?:\((?P<unit>[^)]*)\))?\s*:\s*\[(?P<payload>.*?)\]',
    re.IGNORECASE,
)
FRAME_HEADER_RE = re.compile(r'bms(?:\s+\w+){0,3}\s+emulator\s+status', re.IGNORECASE)
NTC_1_4_FRAME_RE = re.compile(r'ntc\s*1\s*-\s*4\s*:\s*\[(.*?)\]', re.IGNORECASE)
NTC_1_5_FRAME_RE = re.compile(r'ntc\s*1\s*-\s*5(?:\s*\([^)]+\))?\s*:\s*\[(.*?)\]', re.IGNORECASE)
NTC_6_10_FRAME_RE = re.compile(r'ntc\s*6\s*-\s*10(?:\s*\([^)]+\))?\s*:\s*\[(.*?)\]', re.IGNORECASE)
INDEXED_CELL_TOKEN_RE = re.compile(
    r'\bC(\d+)\s*:\s*([-+]?(?:\d+\.\d+|\d+|\.\d+)(?:[eE][-+]?\d+)?)',
    re.IGNORECASE,
)
INDEXED_TEMP_TOKEN_RE = re.compile(
    r'\bT(\d+)\s*:\s*([-+]?(?:\d+\.\d+|\d+|\.\d+)(?:[eE][-+]?\d+)?)',
    re.IGNORECASE,
)
CURRENT_FRAME_RE = re.compile(
    r'\bcurrent\s*:\s*([-+]?(?:\d+\.\d+|\d+|\.\d+)(?:[eE][-+]?\d+)?)\s*(?:a|amp|amps)?\b',
    re.IGNORECASE,
)
NON_VOLTAGE_HINT_RE = re.compile(
    r'\b(?:ntc|status|bms|rpm|fan|eload|current|temp|fault|ctrl)\b',
    re.IGNORECASE,
)

class SerialWorker(QObject):
    """Worker thread that continuously reads from the serial port."""
    data_received = pyqtSignal(dict)  # Signal to emit parsed JSON/Dict data
    connection_status = pyqtSignal(bool)  # Signal to emit connection status
    data_activity = pyqtSignal()  # Signal to indicate any non-empty line was received
    connected_port_changed = pyqtSignal(str)  # Emits current connected port or "" when disconnected
    
    def __init__(self, port=None, baudrate=115200):
        super().__init__()
        self.port = self._normalize_port(port)
        self.baudrate = baudrate
        self.running = False
        self.serial_conn = None
        self.voltage_buffer = deque(maxlen=10)
        self.connected_port = ""
        self._port_lock = Lock()
        self._saw_structured_frame = False

        # Partial frame assembly for multiline firmware output.
        self._pending_frame = {}
        self._pending_started_at = 0.0
        self._pending_expect_ntc = False
        self._pending_timeout_s = 0.30
        self._last_pack_current = 0.0
        self._has_last_pack_current = False

    def start_monitoring(self):
        """Main loop for the worker thread."""
        self.running = True
        
        while self.running:
            if self.serial_conn is None or not self.serial_conn.is_open:
                self._attempt_connection()
            
            if self.serial_conn and self.serial_conn.is_open:
                try:
                    raw = self.serial_conn.readline()
                    if raw:
                        line = raw.decode('utf-8', errors='ignore').rstrip('\r\n')
                        stripped = line.strip()
                    else:
                        stripped = ""

                    if stripped:
                        self.data_activity.emit()

                    parsed = self._parse_structured_serial_line(stripped)
                    if parsed:
                        self._emit_frontend_frame(parsed)
                        continue

                    if self._is_structured_telemetry_line(stripped):
                        continue

                    stale = self._finalize_pending_frame_if_stale()
                    if stale:
                        self._emit_frontend_frame(stale)
                        continue

                    if not stripped:
                        continue

                    if self._saw_structured_frame:
                        # Once we see structured frames, ignore unrelated lines so
                        # telemetry labels and thermistor blocks do not pollute fallback parsing.
                        logger.debug("RAW (ignored non-frame line): %s", stripped)
                        continue

                    if NON_VOLTAGE_HINT_RE.search(stripped):
                        logger.debug("RAW (ignored non-voltage line): %s", stripped)
                        continue

                    # Fallback path: generic numeric stream; accumulate until we have 10.
                    voltages = self._extract_numeric_values(stripped, limit=64)
                    # Keep fallback stream constrained to voltage-like values.
                    voltages = [v for v in voltages if -1.0 <= v <= 6.0]
                    if not voltages:
                        logger.debug("RAW (no voltage payload): %s", stripped)
                        continue

                    self.voltage_buffer.extend(voltages)
                    buffered_voltages = list(self.voltage_buffer)
                    logger.info(
                        "Incoming=%s | Buffer(%d/10)=%s",
                        ", ".join(f"{v:.3f}" for v in voltages),
                        len(buffered_voltages),
                        ", ".join(f"{v:.3f}" for v in buffered_voltages),
                    )

                    # Only publish once we have a full 10-value group.
                    if len(buffered_voltages) < 10:
                        continue

                    frontend_data = self._transform_data({"v": buffered_voltages[:10]})
                    if frontend_data:
                        self.data_received.emit(frontend_data)
                except serial.SerialException as e:
                    logger.error(f"Serial error: {e}")
                    self.connection_status.emit(False)
                    self.connected_port = ""
                    self.connected_port_changed.emit("")
                    self.serial_conn.close()
                    self.serial_conn = None
                    self.voltage_buffer.clear()
                    self._saw_structured_frame = False
                    self._reset_pending_frame()
                    time.sleep(2)  # Wait before retry
                except Exception as e:
                    logger.error(f"Unexpected error: {e}")
                    
            else:
                stale = self._finalize_pending_frame_if_stale()
                if stale:
                    self._emit_frontend_frame(stale)
                time.sleep(1)  # Wait before retrying connection

    @staticmethod
    def _normalize_port(port):
        """Return None for auto mode, otherwise a stripped serial port string."""
        if port is None:
            return None
        text = str(port).strip()
        if not text:
            return None
        if text.lower() == "auto":
            return None
        return text

    @staticmethod
    def list_available_ports():
        """Return a sorted list of currently available serial device paths."""
        ports = [p.device for p in serial.tools.list_ports.comports() if p.device]
        return sorted(set(ports))

    def get_target_port(self):
        with self._port_lock:
            return self.port

    def get_connected_port(self):
        return self.connected_port or None

    def set_baudrate(self, baudrate: int):
        """Update baudrate and reconnect."""
        try:
            normalized = int(baudrate)
        except (TypeError, ValueError):
            normalized = 115200
        if normalized <= 0:
            normalized = 115200
        with self._port_lock:
            self.baudrate = normalized
        if self.serial_conn and self.serial_conn.is_open:
            try:
                self.serial_conn.close()
            except Exception:
                pass
            finally:
                self.serial_conn = None
        self.connection_status.emit(False)
        self.connected_port = ""
        self.connected_port_changed.emit("")
        logger.info("Serial baudrate updated to %d", normalized)

    def set_target_port(self, port):
        """Set a new target port (`None`/`auto` for auto-detect) and reconnect."""
        normalized = self._normalize_port(port)
        with self._port_lock:
            self.port = normalized
        if self.serial_conn and self.serial_conn.is_open:
            try:
                self.serial_conn.close()
            except Exception:
                pass
            finally:
                self.serial_conn = None
        self.connection_status.emit(False)
        self.connected_port = ""
        self.connected_port_changed.emit("")
        logger.info("Serial target port updated to %s", normalized or "auto")

    def _emit_frontend_frame(self, raw_frame: dict):
        """Transform and emit a parsed frame to the GUI."""
        if raw_frame.get("v"):
            self.voltage_buffer.clear()
            self.voltage_buffer.extend(raw_frame["v"][:10])

        frontend_data = self._transform_data(raw_frame)
        if frontend_data:
            self.data_received.emit(frontend_data)

    def _parse_structured_serial_line(self, line: str) -> Optional[dict]:
        """Parse multiline BMS status frames and return one complete raw frame."""
        now = time.time()

        if not line:
            return self._finalize_pending_frame(force=False)

        json_frame = self._try_parse_json_payload(line)
        if json_frame:
            if "i" in json_frame:
                self._last_pack_current = float(json_frame["i"])
                self._has_last_pack_current = True
            self._saw_structured_frame = True
            return json_frame

        current = self._extract_current(line)
        if current is not None:
            self._last_pack_current = float(current)
            self._has_last_pack_current = True
            if self._pending_frame:
                self._pending_frame["i"] = self._last_pack_current
                self._pending_started_at = now
                return self._finalize_pending_frame(force=False)
            return {"i": self._last_pack_current}

        indexed_cells = self._extract_indexed_series(line, INDEXED_CELL_TOKEN_RE)
        indexed_temps = self._extract_indexed_series(line, INDEXED_TEMP_TOKEN_RE)
        if indexed_cells or indexed_temps:
            self._ensure_pending_frame(expect_ntc=bool(indexed_cells))
            if indexed_cells:
                self._pending_frame["v"] = indexed_cells[:10]
                self._pending_started_at = now
            if indexed_temps:
                self._pending_frame["t"] = indexed_temps[:10]
                self._pending_started_at = now
            return self._finalize_pending_frame(force=False)

        if FRAME_HEADER_RE.search(line):
            self._reset_pending_frame(expect_ntc=True)
            return None

        cell_values = self._extract_cells_frame(line)
        if cell_values:
            self._ensure_pending_frame()
            self._pending_frame["v"] = cell_values[:10]
            self._pending_started_at = now
            if "(v)" in line.lower():
                self._pending_expect_ntc = True
            return self._finalize_pending_frame(force=not self._pending_expect_ntc)

        ntc_low = self._extract_ntc_group(line, NTC_1_4_FRAME_RE)
        if not ntc_low:
            ntc_low = self._extract_ntc_group(line, NTC_1_5_FRAME_RE)
        if ntc_low:
            self._ensure_pending_frame(expect_ntc=True)
            self._pending_frame["ntc_1_4"] = ntc_low[:5]
            self._pending_started_at = now
            return self._finalize_pending_frame(force=False)

        ntc_6_10 = self._extract_ntc_group(line, NTC_6_10_FRAME_RE)
        if ntc_6_10:
            self._ensure_pending_frame(expect_ntc=True)
            self._pending_frame["ntc_6_10"] = ntc_6_10[:5]
            self._pending_started_at = now
            return self._finalize_pending_frame(force=False)

        return None

    def _is_structured_telemetry_line(self, line: str) -> bool:
        """Return true if this line belongs to a known framed telemetry format."""
        stripped = line.strip()
        has_json = ("{" in stripped and "}" in stripped)
        return bool(
            has_json
            or
            FRAME_HEADER_RE.search(line)
            or CELLS_FRAME_RE.search(line)
            or CELL_VOLTAGES_FRAME_RE.search(line)
            or NTC_1_4_FRAME_RE.search(line)
            or NTC_1_5_FRAME_RE.search(line)
            or NTC_6_10_FRAME_RE.search(line)
            or INDEXED_CELL_TOKEN_RE.search(line)
            or INDEXED_TEMP_TOKEN_RE.search(line)
            or CURRENT_FRAME_RE.search(line)
        )

    def _reset_pending_frame(self, expect_ntc: bool = False):
        self._pending_frame = {}
        self._pending_started_at = 0.0
        self._pending_expect_ntc = expect_ntc

    def _ensure_pending_frame(self, expect_ntc: bool = False):
        if not self._pending_frame:
            self._pending_started_at = time.time()
        self._pending_expect_ntc = self._pending_expect_ntc or expect_ntc

    def _finalize_pending_frame_if_stale(self) -> Optional[dict]:
        if not self._pending_frame or self._pending_started_at <= 0.0:
            return None
        if time.time() - self._pending_started_at < self._pending_timeout_s:
            return None
        return self._finalize_pending_frame(force=True)

    def _finalize_pending_frame(self, force: bool) -> Optional[dict]:
        if "v" not in self._pending_frame:
            if force:
                self._reset_pending_frame()
            return None

        has_ntc_1_4 = "ntc_1_4" in self._pending_frame
        has_ntc_6_10 = "ntc_6_10" in self._pending_frame
        has_complete_ntc = has_ntc_1_4 and has_ntc_6_10
        has_any_ntc = has_ntc_1_4 or has_ntc_6_10
        has_direct_temps = bool(self._pending_frame.get("t"))
        has_direct_current = "i" in self._pending_frame

        if not force:
            if has_direct_temps and has_direct_current:
                return self._consume_pending_frame()
            if has_direct_temps and not has_direct_current:
                return None
            if has_complete_ntc:
                return self._consume_pending_frame()
            if self._pending_expect_ntc:
                return None
            if not has_any_ntc:
                return self._consume_pending_frame()
            return None

        return self._consume_pending_frame()

    def _consume_pending_frame(self) -> dict:
        raw_out = {"v": [float(v) for v in self._pending_frame.get("v", [])[:10]]}
        if "i" in self._pending_frame:
            raw_out["i"] = float(self._pending_frame["i"])
        elif self._has_last_pack_current:
            raw_out["i"] = float(self._last_pack_current)

        if self._pending_frame.get("t"):
            raw_out["t"] = [float(v) for v in self._pending_frame.get("t", [])[:10]]

        ntc_raw = []
        ntc_raw.extend(self._pending_frame.get("ntc_1_4", []))
        ntc_raw.extend(self._pending_frame.get("ntc_6_10", []))

        if ntc_raw:
            ntc_c = [self._ntc_adc_to_celsius(value) for value in ntc_raw]
            raw_out["ntc_raw"] = ntc_raw
            raw_out["ntc_c"] = ntc_c
            if "t" not in raw_out:
                raw_out["t"] = self._map_thermistors_to_cells(ntc_c, len(raw_out["v"]))

        self._saw_structured_frame = True
        self._reset_pending_frame()
        return raw_out

    def _extract_numeric_values(self, line: str, limit: int = 10):
        """Extract up to `limit` numeric tokens from a serial line."""
        values = []
        for match in NUMERIC_TOKEN_RE.finditer(line):
            try:
                values.append(float(match.group(0)))
            except ValueError:
                continue

            if len(values) >= limit:
                break
        return values

    def _try_parse_json_payload(self, line: str) -> Optional[dict]:
        """
        Parse a JSON object embedded in a serial line.
        Supports raw JSON lines and prefixed logs containing `{...}` payloads.
        """
        text = line.strip()
        if "{" not in text or "}" not in text:
            return None

        start = text.find("{")
        end = text.rfind("}")
        if start < 0 or end <= start:
            return None

        candidate = text[start : end + 1]
        try:
            payload = json.loads(candidate)
        except json.JSONDecodeError:
            return None

        if not isinstance(payload, dict):
            return None

        return self._normalize_json_payload(payload)

    def _normalize_json_payload(self, payload: Dict[str, Any]) -> Optional[dict]:
        """Normalize varied firmware JSON payload shapes into the raw transform format."""
        raw: Dict[str, Any] = {}

        voltages = self._coerce_numeric_list(payload.get("v"), limit=10)
        if not voltages:
            for key in ("voltages", "cell_voltages", "cell_voltage", "cells_v"):
                voltages = self._coerce_numeric_list(payload.get(key), limit=10)
                if voltages:
                    break
        if not voltages:
            cells_value = payload.get("cells")
            if isinstance(cells_value, list):
                if cells_value and isinstance(cells_value[0], dict):
                    parsed_voltages = []
                    parsed_temps = []
                    for cell in cells_value:
                        if not isinstance(cell, dict):
                            continue
                        voltage = self._coerce_numeric(
                            cell.get("voltage", cell.get("v", cell.get("cell_voltage")))
                        )
                        if voltage is not None:
                            parsed_voltages.append(voltage)
                        temp = self._coerce_numeric(
                            cell.get("temperature", cell.get("temp", cell.get("t")))
                        )
                        if temp is not None:
                            parsed_temps.append(temp)
                    voltages = parsed_voltages[:10]
                    if parsed_temps:
                        raw["t"] = parsed_temps[:10]
                else:
                    voltages = self._coerce_numeric_list(cells_value, limit=10)

        if voltages:
            # If telemetry is streamed in mV, normalize to volts.
            if max(abs(v) for v in voltages) > 50.0:
                voltages = [v / 1000.0 for v in voltages]
            raw["v"] = voltages[:10]

        if "t" not in raw:
            for key in ("t", "temps", "temperatures", "cell_temps", "cell_temperatures"):
                temps = self._coerce_numeric_list(payload.get(key), limit=10)
                if temps:
                    raw["t"] = temps[:10]
                    break

        for key in ("i", "current", "pack_current", "current_a"):
            current = self._coerce_numeric(payload.get(key))
            if current is not None:
                raw["i"] = current
                break

        fan_ctrl = payload.get("fan_ctrl")
        if isinstance(fan_ctrl, dict):
            raw["fan_ctrl"] = fan_ctrl
        else:
            normalized_fan_ctrl: Dict[str, Any] = {}
            auto = payload.get("fan_auto")
            duty = payload.get("fan_duty")
            rpm = self._coerce_numeric(payload.get("fan_rpm"))
            if rpm is None:
                fan_values = self._coerce_numeric_list(payload.get("fan"), limit=2)
                if fan_values:
                    rpm = fan_values[0]
            if isinstance(auto, bool) or isinstance(auto, int):
                normalized_fan_ctrl["auto"] = int(bool(auto))
            if isinstance(duty, (int, float)):
                normalized_fan_ctrl["duty"] = int(round(float(duty)))
            if rpm is not None:
                normalized_fan_ctrl["rpm"] = float(rpm)
            if normalized_fan_ctrl:
                raw["fan_ctrl"] = normalized_fan_ctrl

        eload_stats = payload.get("eload_stats")
        if isinstance(eload_stats, dict):
            raw["eload_stats"] = eload_stats
        else:
            eload_payload = payload.get("eload")
            if isinstance(eload_payload, dict):
                raw["eload_stats"] = {
                    "en": eload_payload.get("en", eload_payload.get("enabled", 0)),
                    "i_set": eload_payload.get(
                        "i_set",
                        eload_payload.get("target_current", eload_payload.get("i", 0.0)),
                    ),
                    "v": eload_payload.get("v", eload_payload.get("voltage", 0.0)),
                    "i_act": eload_payload.get("i_act", eload_payload.get("actual_current", 0.0)),
                    "p": eload_payload.get("p", eload_payload.get("power", 0.0)),
                }

        ntc_raw = self._coerce_numeric_list(payload.get("ntc_raw"), limit=16)
        if ntc_raw:
            raw["ntc_raw"] = [int(round(v)) for v in ntc_raw]
        ntc_c = self._coerce_numeric_list(payload.get("ntc_c"), limit=16)
        if ntc_c:
            raw["ntc_c"] = ntc_c

        return raw if raw else None

    @staticmethod
    def _coerce_numeric(value: Any) -> Optional[float]:
        if isinstance(value, (int, float)):
            value_f = float(value)
            if math.isfinite(value_f):
                return value_f
            return None
        if isinstance(value, str):
            text = value.strip()
            if not text:
                return None
            try:
                value_f = float(text)
            except ValueError:
                return None
            if math.isfinite(value_f):
                return value_f
        return None

    def _coerce_numeric_list(self, value: Any, limit: int) -> List[float]:
        if not isinstance(value, list):
            return []
        out: List[float] = []
        for item in value:
            parsed = self._coerce_numeric(item)
            if parsed is None:
                continue
            out.append(parsed)
            if len(out) >= limit:
                break
        return out

    def _extract_cells_frame(self, line: str):
        """Extract numeric values from 'Cells: [..]' serial frames."""
        explicit_match = CELL_VOLTAGES_FRAME_RE.search(line)
        if explicit_match:
            payload = explicit_match.group("payload")
            values = self._extract_numeric_values(payload, limit=64)
            unit_text = (explicit_match.group("unit") or "").lower()
            if "mv" in unit_text:
                return [v / 1000.0 for v in values]
            # Defensive fallback for unlabeled millivolt payloads.
            if values and max(abs(v) for v in values) > 50.0:
                return [v / 1000.0 for v in values]
            return values

        frame_match = CELLS_FRAME_RE.search(line)
        if not frame_match:
            return []
        frame_payload = frame_match.group(1)
        values = self._extract_numeric_values(frame_payload, limit=64)
        if values and max(abs(v) for v in values) > 50.0:
            return [v / 1000.0 for v in values]
        return values

    def _extract_ntc_group(self, line: str, regex: re.Pattern) -> List[int]:
        """Extract integer ADC values from an NTC line."""
        group_match = regex.search(line)
        if not group_match:
            return []
        payload = group_match.group(1)
        values = self._extract_numeric_values(payload, limit=16)
        return [int(round(v)) for v in values]

    def _extract_current(self, line: str) -> Optional[float]:
        """Extract pack current from lines like 'Current: -0.041 A'."""
        match = CURRENT_FRAME_RE.search(line)
        if not match:
            return None
        try:
            return float(match.group(1))
        except (TypeError, ValueError):
            return None

    def _extract_indexed_series(self, line: str, regex: re.Pattern, limit: int = 10) -> List[float]:
        """Extract indexed values like C1: 3.54 ... and return ordered values by index."""
        values_by_index = {}
        for match in regex.finditer(line):
            try:
                index = int(match.group(1))
                value = float(match.group(2))
            except (ValueError, TypeError):
                continue

            if index < 1:
                continue
            if index > limit:
                continue
            values_by_index[index] = value

        if not values_by_index:
            return []

        ordered = []
        for idx in range(1, limit + 1):
            if idx in values_by_index:
                ordered.append(values_by_index[idx])
        return ordered

    def _ntc_adc_to_celsius(
        self,
        adc_raw: float,
        adc_max: float = 4095.0,
        pullup_ohms: float = 10000.0,
        beta: float = 3435.0,
        r0: float = 10000.0,
        t0_c: float = 25.0,
    ) -> float:
        """
        Convert ADC reading to Celsius using a standard Beta-model 10K NTC curve.
        This keeps GUI temperatures meaningful even when firmware streams raw ADC values.
        """
        try:
            adc = float(adc_raw)
            adc = max(1.0, min(adc_max - 1.0, adc))

            resistance = pullup_ohms * adc / (adc_max - adc)
            t0_k = t0_c + 273.15
            inv_t = (1.0 / t0_k) + (math.log(resistance / r0) / beta)
            temp_c = (1.0 / inv_t) - 273.15
            return temp_c
        except Exception:
            return 25.0

    def _map_thermistors_to_cells(self, therm_c: List[float], cell_count: int) -> List[float]:
        """Map thermistor temperatures to cell count, including missing NTC5 interpolation."""
        if cell_count <= 0:
            return []
        if not therm_c:
            return [25.0] * cell_count

        if len(therm_c) >= cell_count:
            return therm_c[:cell_count]

        # Specific mapping for NTC 1-4 and NTC 6-10 (missing NTC5).
        if len(therm_c) == 9 and cell_count == 10:
            sensor_positions = [1, 2, 3, 4, 6, 7, 8, 9, 10]
            by_cell = {pos: therm_c[idx] for idx, pos in enumerate(sensor_positions)}
            mapped = []
            for cell_id in range(1, 11):
                if cell_id in by_cell:
                    mapped.append(by_cell[cell_id])
                    continue

                lower = [pos for pos in by_cell if pos < cell_id]
                upper = [pos for pos in by_cell if pos > cell_id]
                if lower and upper:
                    lo = max(lower)
                    hi = min(upper)
                    mapped.append((by_cell[lo] + by_cell[hi]) / 2.0)
                elif lower:
                    mapped.append(by_cell[max(lower)])
                elif upper:
                    mapped.append(by_cell[min(upper)])
                else:
                    mapped.append(25.0)
            return mapped

        # Generic fallback: extend with nearest available thermistor.
        mapped = []
        for idx in range(cell_count):
            source_idx = min(idx, len(therm_c) - 1)
            mapped.append(therm_c[source_idx])
        return mapped

    def _transform_data(self, raw):
        """
        Transforms compact firmware JSON to verbose Frontend state.
        
        Firmware: {"v":[...], "t":[...], "i":1.5, "fan":[1200, 1200]}
        Frontend: {
            "cells": [{"id":1, "voltage":3.9, "temperature":25}, ...],
            "fan1": {"rpm": 1200},
            "fan2": {"rpm": 1200},
            "pack_current": 1.5
        }
        """
        try:
            # 1. Map Cells
            cells = []
            voltages = raw.get("v", [])
            temps = raw.get("t", [])
            
            # Handle mismatch length safely
            count = max(len(voltages), len(temps))
            
            for i in range(count):
                v = float(voltages[i]) if i < len(voltages) else None
                # Use modulo for temps if we have fewer sensors than cells.
                t = temps[i] if i < len(temps) else (temps[i % len(temps)] if temps else 25.0)
                t = float(t)
                
                cells.append({
                    "id": i + 1,
                    "voltage": v,
                    "temperature": t
                })
                
            # 2. Map Fans
            # New format: "fan_ctrl": {"auto": 1, "duty": 50, "rpm": 1200}
            fan_ctrl = raw.get("fan_ctrl", {})
            fan_rpm = fan_ctrl.get("rpm", 0)
            
            # 3. Map E-Load
            # New format: "eload_stats": {"en": 1, "i_set": 1.5, "v": 24.0, "i_act": 1.4, "p": 33.6}
            eload = raw.get("eload_stats", {})
            
            # Backward compatibility for old firmware (optional)
            if not eload and "eload" in raw:
                 old_eload = raw["eload"]
                 eload = {
                     "en": old_eload.get("en", 0),
                     "i_set": old_eload.get("i", 0.0),
                     "v": 0.0, "i_act": 0.0, "p": 0.0
                 }

            return {
                "cells": cells,
                "fan1": {"rpm": fan_rpm},  # Keeping fan1/fan2 struct for now, mapping both to same rpm
                "fan2": {"rpm": fan_rpm},
                "pack_current": raw.get("i", 0.0),
                "eload": {
                    "enabled": bool(eload.get("en", 0)),
                    "target_current": float(eload.get("i_set", 0.0)),
                    "voltage": float(eload.get("v", 0.0)),
                    "actual_current": float(eload.get("i_act", 0.0)),
                    "power": float(eload.get("p", 0.0))
                },
                "fan_control": {
                    "auto": bool(fan_ctrl.get("auto", True)),
                    "duty": int(fan_ctrl.get("duty", 0))
                },
                "thermistors": {
                    "raw": raw.get("ntc_raw", []),
                    "celsius": raw.get("ntc_c", []),
                },
            }
        except Exception as e:
            logger.error(f"Transformation error: {e}")
            return {}

    def _attempt_connection(self):
        """Tries to connect to the specified port or auto-detect."""
        with self._port_lock:
            target_port = self.port
        
        if target_port is None:
            ports = list(serial.tools.list_ports.comports())
            target_port = self._select_auto_port(ports)
        
        if target_port:
            try:
                self.serial_conn = serial.Serial(target_port, self.baudrate, timeout=1)
                logger.info(f"Connected to {target_port}")
                self.connected_port = target_port
                self.connected_port_changed.emit(target_port)
                self.connection_status.emit(True)
            except serial.SerialException as e:
                logger.error(f"Failed to connect to {target_port}: {e}")
                time.sleep(2)
        else:
            # logger.debug("No STM32 device found. Retrying...")
            pass

    @staticmethod
    def _select_auto_port(ports) -> Optional[str]:
        """Choose the best candidate serial port when running in auto mode."""
        if not ports:
            return None

        def score_port(port_info):
            description = (port_info.description or "").lower()
            manufacturer = (port_info.manufacturer or "").lower()
            hwid = (port_info.hwid or "").lower()
            device = str(port_info.device or "")
            vid = getattr(port_info, "vid", None)

            score = 0
            if vid == 0x0483:
                score += 120  # STMicroelectronics USB VID
            if "stm" in description or "stmicroelectronics" in manufacturer:
                score += 100
            if any(token in description for token in ("usb", "cdc", "virtual com", "serial")):
                score += 35
            if any(token in manufacturer for token in ("ftdi", "silicon", "wch", "arduino", "stmicroelectronics")):
                score += 20
            if any(token in hwid for token in ("usb", "vid", "pid")):
                score += 10
            if "bluetooth" in description:
                score -= 120
            if device.upper() in ("COM1", "COM2"):
                score -= 30

            return (score, device)

        best = max(ports, key=score_port)
        best_score = score_port(best)[0]
        if best_score < 0:
            return None
        return best.device

    def stop(self):
        self.running = False
        if self.serial_conn:
            self.serial_conn.close()
        self.connected_port = ""
        self.connected_port_changed.emit("")

    def send_command(self, cmd_str: str):
        """Send a command string to the serial device."""
        if self.serial_conn and self.serial_conn.is_open:
            try:
                # Ensure newline termination if not present
                if not cmd_str.endswith('\n'):
                    cmd_str += '\n'
                self.serial_conn.write(cmd_str.encode('utf-8'))
                logger.info(f"Sent: {cmd_str.strip()}")
            except Exception as e:
                logger.error(f"Failed to send command: {e}")
        else:
            logger.warning("Cannot send command: Serial not connected")
