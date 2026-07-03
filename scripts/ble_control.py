#!/usr/bin/env python3
"""
CPAP PI Sensor Control BLE utility — NUS stream mode switcher / sniffer.

Connects to the "CPAP_PI_Control" device, optionally sends a mode
command to the NUS RX characteristic, and prints incoming frames
(binary DATA/STATUS headers or JSON debug lines).

Usage:
    python ble_control.py            # subscribe and print frame info
    python ble_control.py --json     # switch firmware to JSON debug mode
    python ble_control.py --binary   # switch firmware back to binary

Requires: pip install bleak
"""

import argparse
import asyncio
import struct
import sys

from bleak import BleakClient, BleakScanner

NUS_RX_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
NUS_TX_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
DEVICE_NAME = "CPAP_PI_Control"

MAGIC = 0xC9A5


def on_notify(_, data: bytearray):
    if len(data) >= 12 and struct.unpack_from("<H", data, 0)[0] == MAGIC:
        ftype, seq = data[2], data[3]
        t_ms = struct.unpack_from("<I", data, 4)[0]
        kind = {1: "DATA", 2: "STATUS"}.get(ftype, f"0x{ftype:02X}")
        print(f"[{t_ms/1000:9.3f}s] {kind:6s} seq={seq:3d} len={len(data)}")
    else:
        print(data.decode(errors="replace").rstrip())


async def main():
    parser = argparse.ArgumentParser(description="CPAP PI NUS stream utility")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--json", action="store_true", help="switch to JSON debug mode")
    group.add_argument("--binary", action="store_true", help="switch to binary mode")
    args = parser.parse_args()

    print(f"Scanning for \"{DEVICE_NAME}\" ...", flush=True)
    device = await BleakScanner.find_device_by_filter(
        lambda d, ad: d.name == DEVICE_NAME, timeout=10.0)
    if device is None:
        print(f"Device \"{DEVICE_NAME}\" not found.", file=sys.stderr)
        sys.exit(1)

    async with BleakClient(device.address) as client:
        print(f"Connected (MTU={client.mtu_size})", flush=True)
        await client.start_notify(NUS_TX_UUID, on_notify)

        if args.json:
            await client.write_gatt_char(NUS_RX_UUID, b"J", response=False)
            print("Switched to JSON debug mode")
        elif args.binary:
            await client.write_gatt_char(NUS_RX_UUID, b"B", response=False)
            print("Switched to binary mode")

        print("Listening (Ctrl+C to quit) ...")
        while True:
            await asyncio.sleep(1)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
