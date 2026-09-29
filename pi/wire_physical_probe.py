#!/usr/bin/env python3
"""Pi peer for hardware checkpoint 02: physical PSTV Wire qualification."""

from __future__ import annotations

import socket
import struct
import time
from datetime import datetime

import wire_protocol as wire

HOST = "192.168.50.1"
PORT = 5959
SESSION_ID = 0x02000001
ACTIVE_ROUNDS = 128
IDLE_SECONDS = 60.0
READ_CHUNK = 7

PAYLOAD_SIZES = (
    0, 1, 2, 3, 7, 15, 16, 17,
    31, 63, 64, 65, 127, 255, 256, 257,
    511, 1024, 2048, 4096, wire.MAX_PAYLOAD_BYTES,
)

def stamp(message: str) -> None:
    print(f"{datetime.now().astimezone().isoformat()} {message}", flush=True)

def recv_exact(conn: socket.socket, length: int) -> bytes:
    chunks: list[bytes] = []
    remaining = length

    while remaining:
        data = conn.recv(min(remaining, READ_CHUNK))
        if not data:
            raise ConnectionError("peer closed before requested bytes completed")
        chunks.append(data)
        remaining -= len(data)

    return b"".join(chunks)

def recv_frame(
    conn: socket.socket,
    expected_sequence: int,
) -> tuple[wire.WireHeader, bytes]:
    raw_header = recv_exact(conn, wire.HEADER_BYTES)
    header = wire.decode_header(raw_header)

    if header.sequence != expected_sequence:
        raise ValueError(
            f"sequence mismatch expected={expected_sequence} actual={header.sequence}"
        )

    payload = recv_exact(conn, header.payload_length)
    return header, payload

def send_fragmented(conn: socket.socket, frame: bytes) -> None:
    offsets = (1, 2, 3, 5, 8, 13)
    position = 0

    for chunk_size in offsets:
        if position >= len(frame):
            break
        end = min(len(frame), position + chunk_size)
        conn.sendall(frame[position:end])
        position = end
        time.sleep(0.001)

    if position < len(frame):
        conn.sendall(frame[position:])

def encode_data(sequence: int, payload: bytes) -> bytes:
    return wire.encode_channel_frame(
        wire.FRAME_DATA,
        wire.CHANNEL_CONTROL,
        sequence,
        payload,
    )

def expected_payload(phase: int, round_index: int, length: int) -> bytes:
    return bytes(
        ((phase * 0x31) ^ (round_index * 0x17) ^ (index * 0x07)) & 0xFF
        for index in range(length)
    )

def require_data_frame(
    header: wire.WireHeader,
    payload: bytes,
    expected_payload_bytes: bytes,
) -> None:
    if header.kind != wire.FRAME_DATA:
        raise ValueError(f"unexpected kind {header.kind}")
    if header.channel != wire.CHANNEL_CONTROL:
        raise ValueError(f"unexpected channel {header.channel}")
    if header.flags != 0:
        raise ValueError(f"unexpected flags {header.flags}")
    if payload != expected_payload_bytes:
        raise ValueError(
            f"payload mismatch expected={len(expected_payload_bytes)} "
            f"actual={len(payload)}"
        )

def run_active_phase(
    conn: socket.socket,
    phase: int,
    receive_sequence: int,
    send_sequence: int,
) -> tuple[int, int]:
    for round_index in range(ACTIVE_ROUNDS):
        length = PAYLOAD_SIZES[round_index % len(PAYLOAD_SIZES)]
        expected = expected_payload(phase, round_index, length)

        header, payload = recv_frame(conn, receive_sequence)
        require_data_frame(header, payload, expected)
        receive_sequence += 1

        send_fragmented(conn, encode_data(send_sequence, payload))
        send_sequence += 1

    stamp(f"PHASE{phase}=PASS rounds={ACTIVE_ROUNDS}")
    return receive_sequence, send_sequence

def establish(conn: socket.socket) -> tuple[int, int]:
    header, payload = recv_frame(conn, 1)

    if not wire.is_hello_header(header):
        raise ValueError(f"expected HELLO header, got {header!r}")

    versions = wire.decode_hello_payload(payload)
    expected_versions = (
        wire.WIRE_VERSION,
        wire.PRODUCT_ESTABLISHMENT_VERSION,
    )
    if versions != expected_versions:
        raise ValueError(
            f"HELLO versions expected={expected_versions!r} actual={versions!r}"
        )

    send_fragmented(conn, wire.encode_accept_frame(SESSION_ID, sequence=1))
    stamp(f"Q4_ACCEPT=PASS session_id={SESSION_ID}")
    return 2, 2

def run_idle_wake(
    conn: socket.socket,
    receive_sequence: int,
    send_sequence: int,
) -> tuple[int, int]:
    header, payload = recv_frame(conn, receive_sequence)
    require_data_frame(header, payload, b"WIRE_IDLE_READY")
    receive_sequence += 1

    stamp(f"IDLE_BEGIN seconds={IDLE_SECONDS:.0f}")
    time.sleep(IDLE_SECONDS)

    send_fragmented(conn, encode_data(send_sequence, b"WIRE_WAKE"))
    send_sequence += 1

    header, payload = recv_frame(conn, receive_sequence)
    if header.kind != wire.FRAME_DATA or header.channel != wire.CHANNEL_CONTROL:
        raise ValueError("invalid WOKE envelope")
    if len(payload) != 8 or payload[:4] != b"WOKE":
        raise ValueError(f"invalid WOKE payload {payload!r}")

    empty_polls = struct.unpack(">I", payload[4:])[0]
    receive_sequence += 1
    stamp(f"IDLE_WAKE=PASS empty_polls={empty_polls}")
    return receive_sequence, send_sequence

def finish(
    conn: socket.socket,
    receive_sequence: int,
    send_sequence: int,
) -> None:
    header, payload = recv_frame(conn, receive_sequence)
    require_data_frame(header, payload, b"WIRE_PASS")

    send_fragmented(conn, encode_data(send_sequence, b"WIRE_DONE"))
    stamp("FINAL_HANDSHAKE=PASS")

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
            receive_sequence, send_sequence = run_active_phase(
                conn, 1, receive_sequence, send_sequence
            )
            receive_sequence, send_sequence = run_idle_wake(
                conn, receive_sequence, send_sequence
            )
            receive_sequence, send_sequence = run_active_phase(
                conn, 2, receive_sequence, send_sequence
            )
            finish(conn, receive_sequence, send_sequence)
            stamp("CHECKPOINT=PASS")
            return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        stamp(f"CHECKPOINT=FAIL error={exc!r}")
        raise SystemExit(1)
