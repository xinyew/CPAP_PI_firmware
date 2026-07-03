#!/usr/bin/env python3
"""
RTT -> WebSocket bridge for the CPAP PI web portal.

Reads the firmware's binary sensor stream from SEGGER RTT up-buffer 1
("data") over the debug probe, re-frames it on the 0xC9A5 magic, and
broadcasts complete frames to WebSocket clients (the portal's
"RTT (wired)" mode connects to ws://localhost:8765).

Requires: pip install pylink-square websockets
Note: close any other RTT session (commander-cli / RTT Viewer) first —
the probe supports one debug connection at a time.

Usage:
    python rtt_bridge.py [--port 8765] [--device NRF52840_XXAA]
"""

import argparse
import asyncio
import struct
import sys

import pylink
import websockets

MAGIC = 0xC9A5
FRAME_LEN = {0x01: 176, 0x02: 41}  # type -> length (comm_protocol.h)
RTT_CHANNEL = 1

clients = set()
stats = {"data": 0, "status": 0, "bytes": 0}


class Reframer:
    """Byte stream -> complete frames, resyncing on the magic."""

    def __init__(self):
        self.buf = bytearray()

    def feed(self, chunk: bytes):
        self.buf.extend(chunk)
        frames = []
        while True:
            # hunt for magic
            while len(self.buf) >= 2 and struct.unpack_from("<H", self.buf, 0)[0] != MAGIC:
                self.buf.pop(0)
            if len(self.buf) < 3:
                break
            length = FRAME_LEN.get(self.buf[2])
            if length is None:
                self.buf.pop(0)  # false magic — resync
                continue
            if len(self.buf) < length:
                break
            frames.append(bytes(self.buf[:length]))
            del self.buf[:length]
        return frames


async def rtt_reader(jlink):
    reframer = Reframer()
    while True:
        data = jlink.rtt_read(RTT_CHANNEL, 4096)
        if data:
            stats["bytes"] += len(data)
            for frame in reframer.feed(bytes(data)):
                stats["data" if frame[2] == 0x01 else "status"] += 1
                if clients:
                    websockets.broadcast(clients, frame)
            await asyncio.sleep(0.005)
        else:
            await asyncio.sleep(0.02)


async def stats_printer():
    while True:
        await asyncio.sleep(5)
        print(f"[bridge] {stats['data']} data / {stats['status']} status frames, "
              f"{stats['bytes'] / 1024:.1f} kB read, {len(clients)} client(s)",
              flush=True)


async def handler(websocket):
    clients.add(websocket)
    print(f"[bridge] portal connected ({len(clients)} client(s))", flush=True)
    try:
        await websocket.wait_closed()
    finally:
        clients.discard(websocket)
        print(f"[bridge] portal disconnected", flush=True)


async def main():
    parser = argparse.ArgumentParser(description="CPAP PI RTT->WebSocket bridge")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--device", default="NRF52840_XXAA")
    parser.add_argument("--serial", type=int, default=None,
                        help="J-Link probe serial number (optional)")
    args = parser.parse_args()

    jlink = pylink.JLink()
    jlink.open(serial_no=args.serial)
    jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
    jlink.connect(args.device, speed=4000)
    jlink.rtt_start()
    print(f"[bridge] RTT attached to {args.device}", flush=True)

    # Wait for the RTT control block / channel 1 to be discovered
    for _ in range(50):
        try:
            if jlink.rtt_get_num_up_buffers() > RTT_CHANNEL:
                break
        except pylink.errors.JLinkRTTException:
            pass
        await asyncio.sleep(0.1)

    async with websockets.serve(handler, "localhost", args.port):
        print(f"[bridge] serving ws://localhost:{args.port}", flush=True)
        await asyncio.gather(rtt_reader(jlink), stats_printer())


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
