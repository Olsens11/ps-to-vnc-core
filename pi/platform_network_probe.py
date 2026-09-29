#!/usr/bin/env python3
"""Pi peer for checkpoint 01 stability qualification."""

from __future__ import annotations

import socket
import struct
import time
from datetime import datetime

HOST = "192.168.50.1"
PORT = 5959
MAGIC = 0x50535456
TYPE_PING = 1
TYPE_IDLE_READY = 2
TYPE_WAKE = 3
TYPE_WOKE = 4
TYPE_DONE = 5
PHASE_ONE = 1
PHASE_TWO = 2
ROUND_COUNT = 128
IDLE_SECONDS = 60.0
RECORD = struct.Struct("!IIII")

def stamp(message: str) -> None:
    print(f"{datetime.now().astimezone().isoformat()} {message}", flush=True)

def recv_exact(conn: socket.socket, length: int) -> bytes:
    chunks: list[bytes] = []
    remaining = length

    while remaining:
        data = conn.recv(remaining)
        if not data:
            raise ConnectionError("peer closed before record completed")
        chunks.append(data)
        remaining -= len(data)
    return b"".join(chunks)

def receive_record(conn: socket.socket) -> tuple[int, int, int, int]:
    return RECORD.unpack(recv_exact(conn, RECORD.size))

def expect_record(
    conn: socket.socket,
    expected_type: int,
    expected_phase: int,
    expected_sequence: int,
) -> None:
    record = receive_record(conn)
    expected = (MAGIC, expected_type, expected_phase, expected_sequence)
    if record != expected:
        raise ValueError(f"record mismatch expected={expected!r} actual={record!r}")

def echo_rounds(conn: socket.socket, phase: int) -> None:
    for sequence in range(ROUND_COUNT):
        raw = recv_exact(conn, RECORD.size)
        record = RECORD.unpack(raw)

        expected = (MAGIC, TYPE_PING, phase, sequence)
        if record != expected:
            raise ValueError(
                f"ping mismatch expected={expected!r} actual={record!r}"
            )
        conn.sendall(raw)

def main() -> int:
    stamp(f"LISTEN {HOST}:{PORT}")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((HOST, PORT))
        server.listen(1)
        conn, address = server.accept()

        with conn:
            conn.settimeout(90.0)
            stamp(f"ACCEPT peer={address[0]}:{address[1]}")

            echo_rounds(conn, PHASE_ONE)
            stamp(f"PHASE1=PASS rounds={ROUND_COUNT}")

            expect_record(conn, TYPE_IDLE_READY, 0, 0)
            stamp(f"IDLE_BEGIN seconds={IDLE_SECONDS:.0f}")
            time.sleep(IDLE_SECONDS)

            conn.sendall(RECORD.pack(MAGIC, TYPE_WAKE, 0, 0))
            expect_record(conn, TYPE_WOKE, 0, 0)
            stamp("IDLE_WAKE=PASS")

            echo_rounds(conn, PHASE_TWO)
            stamp(f"PHASE2=PASS rounds={ROUND_COUNT}")

            conn.sendall(RECORD.pack(MAGIC, TYPE_DONE, 0, 0))
            stamp("CHECKPOINT=PASS")
            return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        stamp(f"CHECKPOINT=FAIL error={exc!r}")
        raise SystemExit(1)
