#!/usr/bin/env python3
"""Pi peer for checkpoint 03A: logical RFB byte-channel qualification."""

from __future__ import annotations

import socket
import struct
import time
from datetime import datetime

import wire_protocol as wire

HOST = "192.168.50.1"
PORT = 5959
SESSION_ID = 0x030A0001
IDLE_SECONDS = 60.0
READ_CHUNK = 11

PRE_IDLE_FRAMES = (
    (0, 700),
    (700, 800),
    (1500, 900),
    (2400, 0),
    (2400, 3),
    (2403, 5),
)
POST_IDLE_OFFSET = 2408
POST_IDLE_LENGTH = 257

def stamp(message: str) -> None:
    print(f"{datetime.now().astimezone().isoformat()} {message}", flush=True)

def stream_bytes(offset: int, length: int) -> bytes:
    return bytes((((offset + index) * 13) + 0x5A) & 0xFF for index in range(length))

def recv_exact(conn: socket.socket, length: int) -> bytes:
    chunks: list[bytes] = []
    remaining = length

    while remaining:
        chunk = conn.recv(min(remaining, READ_CHUNK))
        if not chunk:
            raise ConnectionError("peer closed before requested bytes completed")
        chunks.append(chunk)
        remaining -= len(chunk)

    return b"".join(chunks)

def recv_frame(
    conn: socket.socket,
    expected_sequence: int,
) -> tuple[wire.WireHeader, bytes]:
    header = wire.decode_header(recv_exact(conn, wire.HEADER_BYTES))
    if header.sequence != expected_sequence:
        raise ValueError(
            f"sequence mismatch expected={expected_sequence} actual={header.sequence}"
        )
    return header, recv_exact(conn, header.payload_length)

def send_fragmented(conn: socket.socket, frame: bytes) -> None:
    cuts = (1, 2, 4, 7, 13, 23)
    position = 0

    for size in cuts:
        if position >= len(frame):
            break
        end = min(len(frame), position + size)
        conn.sendall(frame[position:end])
        position = end
        time.sleep(0.001)

    if position < len(frame):
        conn.sendall(frame[position:])

def establish(conn: socket.socket) -> tuple[int, int]:
    header, payload = recv_frame(conn, 1)

    if not wire.is_hello_header(header):
        raise ValueError(f"expected HELLO header, got {header!r}")

    versions = wire.decode_hello_payload(payload)
    expected = (wire.WIRE_VERSION, wire.PRODUCT_ESTABLISHMENT_VERSION)
    if versions != expected:
        raise ValueError(f"HELLO versions expected={expected!r} actual={versions!r}")

    send_fragmented(conn, wire.encode_accept_frame(SESSION_ID, sequence=1))
    stamp(f"Q4_ACCEPT=PASS session_id={SESSION_ID}")
    return 2, 2

def send_rfb_data(
    conn: socket.socket,
    sequence: int,
    offset: int,
    length: int,
) -> int:
    payload = stream_bytes(offset, length)
    frame = wire.encode_channel_frame(
        wire.FRAME_DATA,
        wire.CHANNEL_RFB,
        sequence,
        payload,
    )
    send_fragmented(conn, frame)
    return sequence + 1

def require_control(
    header: wire.WireHeader,
    payload: bytes,
    expected: bytes,
) -> None:
    if header.kind != wire.FRAME_DATA:
        raise ValueError(f"unexpected control kind {header.kind}")
    if header.channel != wire.CHANNEL_CONTROL:
        raise ValueError(f"unexpected control channel {header.channel}")
    if header.flags != 0:
        raise ValueError(f"unexpected control flags {header.flags}")
    if payload != expected:
        raise ValueError(f"control payload expected={expected!r} actual={payload!r}")

def main() -> int:
    stamp(f"LISTEN {HOST}:{PORT}")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((HOST, PORT))
        server.listen(1)
        conn, address = server.accept()

        with conn:
            conn.settimeout(120.0)
            conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            stamp(f"ACCEPT peer={address[0]}:{address[1]}")

            receive_sequence, send_sequence = establish(conn)

            for offset, length in PRE_IDLE_FRAMES:
                send_sequence = send_rfb_data(
                    conn, send_sequence, offset, length
                )

            header, payload = recv_frame(conn, receive_sequence)
            require_control(header, payload, b"RFB03A_IDLE_READY")
            receive_sequence += 1
            stamp("PRE_IDLE_QUEUE=PASS")
            stamp(f"IDLE_BEGIN seconds={IDLE_SECONDS:.0f}")
            time.sleep(IDLE_SECONDS)

            send_sequence = send_rfb_data(
                conn,
                send_sequence,
                POST_IDLE_OFFSET,
                POST_IDLE_LENGTH,
            )

            header, payload = recv_frame(conn, receive_sequence)
            if header.kind != wire.FRAME_DATA or header.channel != wire.CHANNEL_CONTROL:
                raise ValueError(f"invalid result envelope {header!r}")
            if len(payload) != 16 or payload[:4] != b"R03A":
                raise ValueError(f"invalid result payload {payload!r}")

            empty_polls, generation, available = struct.unpack(">III", payload[4:])
            if empty_polls == 0:
                raise ValueError("idle wake reported zero empty polls")
            if generation != 6:
                raise ValueError(
                    f"activity generation expected=6 actual={generation}"
                )
            if available != 0:
                raise ValueError(
                    f"final queue availability expected=0 actual={available}"
                )

            stamp(
                f"IDLE_WAKE=PASS empty_polls={empty_polls} "
                f"generation={generation} available={available}"
            )

            done = wire.encode_channel_frame(
                wire.FRAME_DATA,
                wire.CHANNEL_CONTROL,
                send_sequence,
                b"RFB03A_DONE",
            )
            send_fragmented(conn, done)
            stamp("FINAL_HANDSHAKE=PASS")
            stamp("CHECKPOINT=PASS")
            return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        stamp(f"CHECKPOINT=FAIL error={exc!r}")
        raise SystemExit(1)
