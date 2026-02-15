"""Simple COM5 serial monitor for BMS output.

Usage:
  python serial_dump_com5.py
  python serial_dump_com5.py --baudrate 115200
"""

from __future__ import annotations

import argparse
import sys
import time

import serial


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Print live serial data from COM5.")
    parser.add_argument("--port", default="COM5", help="Serial port (default: COM5)")
    parser.add_argument(
        "--baudrate",
        type=int,
        default=115200,
        help="Baudrate (default: 115200)",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=0.5,
        help="Read timeout seconds (default: 0.5)",
    )
    parser.add_argument(
        "--no-data-interval",
        type=float,
        default=3.0,
        help="Seconds between 'no data' notices (default: 3.0)",
    )
    parser.add_argument(
        "--probe",
        default="",
        help=(
            "Optional probe string to send once after connect "
            "(example: --probe \"\\n\" or --probe \"STATUS?\\n\")"
        ),
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    print(f"Opening {args.port} @ {args.baudrate}...")
    try:
        ser = serial.Serial(args.port, args.baudrate, timeout=args.timeout)
    except Exception as exc:
        print(f"Failed to open {args.port}: {exc}")
        return 1

    print("Connected. Streaming serial output. Press Ctrl+C to stop.")
    if args.probe:
        payload = args.probe.encode("utf-8", errors="ignore")
        ser.write(payload)
        print(f"Sent probe ({len(payload)} bytes).")

    last_data_time = time.time()
    last_notice_time = time.time()
    try:
        while True:
            raw = ser.readline()
            if raw:
                last_data_time = time.time()
                text = raw.decode("utf-8", errors="replace").rstrip("\r\n")
                timestamp = time.strftime("%H:%M:%S")
                print(f"[{timestamp}] {text}")
                continue

            now = time.time()
            if now - last_notice_time >= args.no_data_interval:
                idle = now - last_data_time
                print(f"[{time.strftime('%H:%M:%S')}] no data received for {idle:.1f}s")
                last_notice_time = now
    except KeyboardInterrupt:
        print("\nStopped by user.")
    finally:
        ser.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
