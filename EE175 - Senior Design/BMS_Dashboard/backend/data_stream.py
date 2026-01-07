import json
import logging
import serial
import serial.tools.list_ports
from PyQt6.QtCore import QObject, pyqtSignal, QThread
import time

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("SerialMonitor")

class SerialWorker(QObject):
    """Worker thread that continuously reads from the serial port."""
    data_received = pyqtSignal(dict)  # Signal to emit parsed JSON/Dict data
    connection_status = pyqtSignal(bool) # Signal to emit connection status
    
    def __init__(self, port=None, baudrate=115200):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.running = False
        self.serial_conn = None

    def start_monitoring(self):
        """Main loop for the worker thread."""
        self.running = True
        
        while self.running:
            if self.serial_conn is None or not self.serial_conn.is_open:
                self._attempt_connection()
            
            if self.serial_conn and self.serial_conn.is_open:
                try:
                    line = self.serial_conn.readline().decode('utf-8').strip()
                    if line:
                        # Attempt to parse JSON
                        # Expected format: {"v":[...], "t":[...], "i": 1.0, ...}
                        if line.startswith('{') and line.endswith('}'):
                            data = json.loads(line)
                            
                            # Transform to Frontend Format
                            frontend_data = self._transform_data(data)
                            
                            self.data_received.emit(frontend_data)
                        else:
                            # Log raw lines that aren't JSON (debug messages)
                            logger.debug(f"RAW: {line}")
                            
                except json.JSONDecodeError:
                    logger.warning(f"Invalid JSON received: {line}")
                except serial.SerialException as e:
                    logger.error(f"Serial error: {e}")
                    self.connection_status.emit(False)
                    self.serial_conn.close()
                    self.serial_conn = None
                    time.sleep(2) # Wait before retry
                except Exception as e:
                    logger.error(f"Unexpected error: {e}")
                    
            else:
                time.sleep(1) # Wait before retrying connection

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
                v = voltages[i] if i < len(voltages) else 0.0
                # Use modulo for temps if we have fewer sensors than cells (common in BMS)
                t = temps[i] if i < len(temps) else (temps[i % len(temps)] if temps else 25.0)
                
                cells.append({
                    "id": i + 1,
                    "voltage": v,
                    "temperature": t
                })
                
            # 2. Map Fans
            fans = raw.get("fan", [0, 0])
            fan1_rpm = fans[0] if len(fans) > 0 else 0
            fan2_rpm = fans[1] if len(fans) > 1 else 0
            
            return {
                "cells": cells,
                "fan1": {"rpm": fan1_rpm},
                "fan2": {"rpm": fan2_rpm},
                "pack_current": raw.get("i", 0.0)
            }
        except Exception as e:
            logger.error(f"Transformation error: {e}")
            return {}

    def _attempt_connection(self):
        """Tries to connect to the specified port or auto-detect."""
        target_port = self.port
        
        if target_port is None:
            # Auto-detect logic: Look for STM32 Virtual COM Port
            ports = list(serial.tools.list_ports.comports())
            for p in ports:
                # Common VID/PID for ST-Link VCP or STM32 VCP
                # You might need to adjust this filter based on your specific device
                if "STM" in p.description or "STMicroelectronics" in p.manufacturer:
                    target_port = p.device
                    break
        
        if target_port:
            try:
                self.serial_conn = serial.Serial(target_port, self.baudrate, timeout=1)
                logger.info(f"Connected to {target_port}")
                self.connection_status.emit(True)
            except serial.SerialException as e:
                logger.error(f"Failed to connect to {target_port}: {e}")
                time.sleep(2)
        else:
            # logger.debug("No STM32 device found. Retrying...")
            pass

    def stop(self):
        self.running = False
        if self.serial_conn:
            self.serial_conn.close()
