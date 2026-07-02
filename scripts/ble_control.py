#!/usr/bin/env python3
"""
CPAP PI Sensor Body BLE control — sensor sampling-interval setter.

Connects to a "CPAP_PI_Body" device and writes a 16-bit interval value
(ms, little-endian) via Write Without Response to a custom GATT
characteristic (UUID 0xFFE1).

Usage:
    python ble_control.py                   # interactive mode
    python ble_control.py --interval 100    # one-shot
    python ble_control.py -i 100            # short form

Requires: pip install bleak
"""

import argparse
import asyncio
import struct
import sys

from bleak import BleakClient, BleakScanner

# Must match firmware — see ble_interface.c
SERVICE_UUID = "0000ffe0-0000-1000-8000-00805f9b34fb"
CHAR_UUID    = "0000ffe1-0000-1000-8000-00805f9b34fb"
DEVICE_NAME  = "CPAP_PI_Body"


def pack_interval(ms: int) -> bytes:
    """Pack a 16-bit interval into little-endian bytes."""
    return struct.pack("<H", ms)


async def discover() -> str | None:
    """Scan for the CPAP PI device and return its BLE address."""
    print(f"Scanning for \"{DEVICE_NAME}\" ...", flush=True)

    device = await BleakScanner.find_device_by_filter(
        lambda d, ad: d.name == DEVICE_NAME,
        timeout=10.0,
    )

    if device is None:
        print(f"Device \"{DEVICE_NAME}\" not found.", file=sys.stderr)
        return None

    print(f"Found {device.name} [{device.address}]")
    return device.address


async def set_interval(address: str, ms: int):
    """Connect, set interval, disconnect."""
    print(f"Connecting to {address} ...", flush=True)
    async with BleakClient(address) as client:
        print(f"Connected (MTU={client.mtu_size})", flush=True)
        data = pack_interval(ms)
        await client.write_gatt_char(CHAR_UUID, data, response=False)
        print(f"Sent: {ms} ms → {data.hex()}", flush=True)


async def interactive(address: str):
    """REPL loop for interval changes."""
    print("Enter sampling interval in ms (10–10000), or 'q' to quit.")
    print("Changes apply instantly via Write Without Response.\n")

    async with BleakClient(address) as client:
        print(f"Connected (MTU={client.mtu_size})\n", flush=True)
        while True:
            try:
                cmd = input("ms> ").strip()
            except (EOFError, KeyboardInterrupt):
                print()
                break

            if cmd.lower() in ("q", "quit", "exit"):
                break

            if not cmd:
                continue

            try:
                ms = int(cmd)
            except ValueError:
                print(f"  Invalid: {cmd}")
                continue

            if ms < 10 or ms > 10000:
                print(f"  Out of range (10–10000): {ms}")
                continue

            data = pack_interval(ms)
            await client.write_gatt_char(CHAR_UUID, data, response=False)
            print(f"  → {ms} ms")


async def main():
    parser = argparse.ArgumentParser(description="CPAP PI Sensor Body BLE interval control")
    parser.add_argument("-i", "--interval", type=int, default=None,
                        help="Sampling interval in ms (one-shot mode)")
    args = parser.parse_args()

    address = await discover()
    if address is None:
        sys.exit(1)

    if args.interval is not None:
        await set_interval(address, args.interval)
    else:
        await interactive(address)


if __name__ == "__main__":
    asyncio.run(main())
