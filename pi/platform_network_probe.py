#!/usr/bin/env python3
"""One-shot peer for hardware checkpoint 01."""

from __future__ import annotations
import socket
from datetime import datetime

HOST = "192.168.50.1"
PORT = 5959
HELLO = b"PSTVNC_CORE_PLATFORM_NETWORK_HELLO\n"
ACK = b"PSTVNC_CORE_PLATFORM_NETWORK_ACK\n"
PASS = b"PSTVNC_CORE_PLATFORM_NETWORK_PASS\n"
DONE = b"PSTVNC_CORE_PLATFORM_NETWORK_DONE\n"

def stamp(message: str) -> None:
    print(f"{datetime.now().astimezone().isoformat()} {message}", flush=True)

def recv_exact(conn: socket.socket, length: int) -> bytes:
    chunks = []
    remaining = length
    while remaining:
        data = conn.recv(remaining)
        if not data:
            raise ConnectionError("peer closed before checkpoint message completed")
        chunks.append(data)
        remaining -= len(data)
    return b"".join(chunks)

def main() -> int:
    stamp(f"LISTEN {HOST}:{PORT}")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((HOST, PORT))
        server.listen(1)
        conn, address = server.accept()
        with conn:
            conn.settimeout(10.0)
            stamp(f"ACCEPT peer={address[0]}:{address[1]}")
            hello = recv_exact(conn, len(HELLO))
            if hello != HELLO:
                stamp(f"CHECKPOINT=FAIL bad_hello={hello!r}")
                return 1
            stamp("HELLO=PASS")
            conn.sendall(ACK)

            result = recv_exact(conn, len(PASS))
            if result != PASS:
                stamp(f"CHECKPOINT=FAIL bad_result={result!r}")
                return 1
            stamp("PASS_MESSAGE=RECEIVED")
            conn.sendall(DONE)
            stamp("CHECKPOINT=PASS")
            return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        stamp(f"CHECKPOINT=FAIL error={exc!r}")
        raise SystemExit(1)
