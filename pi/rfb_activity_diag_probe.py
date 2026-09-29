#!/usr/bin/env python3
"""Pi peer for checkpoint 03B-DIAG lifecycle-stage instrumentation."""

from __future__ import annotations

import socket
import struct
import threading
import time
from datetime import datetime

import wire_protocol as wire

HOST = "192.168.50.1"
PORT = 5959
SESSION_ID = 0x030B0001
RFB_PAYLOAD_BYTES = 328
PRE_BLOCKED_ROUNDS = 128
POST_BLOCKED_ROUNDS = 128
TOTAL_FRAMES = 1 + PRE_BLOCKED_ROUNDS + 1 + POST_BLOCKED_ROUNDS
IDLE_SECONDS = 60.0
READ_CHUNK = 11
TELEMETRY_PORT = 5999
TELEMETRY_MAGIC = 0x52334447
TELEMETRY = struct.Struct(">IIIII")

STAGE_NAMES = {
    1: "DIAG_READY",
    10: "CONSUMER_DONE_WAIT_ENTER",
    11: "CONSUMER_DONE_WAIT_RETURN",
    12: "ACTIVITY_SNAPSHOT_ENTER",
    13: "ACTIVITY_SNAPSHOT_RETURN",
    14: "AVAILABLE_ENTER",
    15: "AVAILABLE_RETURN",
    16: "PASS_SEND_ENTER",
    17: "PASS_SEND_RETURN",
    18: "DONE_WAIT_ENTER",
    19: "DONE_WAIT_RETURN",
    20: "DORMANT_WAIT_ENTER",
    21: "DORMANT_WAIT_RETURN",
    22: "DELETE_THREAD_ENTER",
    23: "DELETE_THREAD_RETURN",
    24: "DELETE_DONE_SEMA_ENTER",
    25: "DELETE_DONE_SEMA_RETURN",
    26: "DELETE_PROGRESS_SEMA_ENTER",
    27: "DELETE_PROGRESS_SEMA_RETURN",
    28: "DELETE_START_SEMA_ENTER",
    29: "DELETE_START_SEMA_RETURN",
    30: "DELETE_SNAPSHOT_SEMA_ENTER",
    31: "DELETE_SNAPSHOT_SEMA_RETURN",
    32: "FLOW_RELEASE_ENTER",
    33: "FLOW_RELEASE_RETURN",
    34: "PHYSICAL_RELEASE_ENTER",
    35: "PHYSICAL_RELEASE_RETURN",
    36: "NETWORK_CLOSE_ENTER",
    37: "NETWORK_CLOSE_RETURN",
    38: "OSDSYS_ENTER",
    90: "FAILURE_PATH_ENTER",
}

CONTROL = struct.Struct(">III")
CONTROL_MAGIC = 0x52303342
CONTROL_READY = 1
CONTROL_IDLE_READY = 2
CONTROL_IDLE_RESULT = 3
CONTROL_PASS = 4
CONTROL_DONE = 5

def stamp(message: str) -> None:
    print(f"{datetime.now().astimezone().isoformat()} {message}", flush=True)

def udp_observer(stop_event: threading.Event) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as receiver:
        receiver.settimeout(0.5)
        receiver.bind((HOST, TELEMETRY_PORT))
        stamp(f"UDP_LISTEN {HOST}:{TELEMETRY_PORT}")

        while not stop_event.is_set():
            try:
                payload, sender = receiver.recvfrom(4096)
            except socket.timeout:
                continue

            if len(payload) != TELEMETRY.size:
                stamp(
                    f"UDP_BAD_SIZE bytes={len(payload)} "
                    f"sender={sender[0]}:{sender[1]}"
                )
                continue

            magic, sequence, stage, value1, value2 = TELEMETRY.unpack(payload)
            if magic != TELEMETRY_MAGIC:
                stamp(
                    f"UDP_BAD_MAGIC magic=0x{magic:08x} "
                    f"sender={sender[0]}:{sender[1]}"
                )
                continue

            name = STAGE_NAMES.get(stage, f"UNKNOWN_{stage}")
            stamp(
                f"STAGE seq={sequence} id={stage} name={name} "
                f"value1={value1} value2={value2}"
            )

def payload_for(frame_index: int) -> bytes:
    return bytes(
        ((frame_index * 17) ^ (byte_index * 7) ^ 0x5A) & 0xFF
        for byte_index in range(RFB_PAYLOAD_BYTES)
    )

def recv_exact(conn: socket.socket, length: int) -> bytes:
    parts: list[bytes] = []
    remaining = length
    while remaining:
        chunk = conn.recv(min(remaining, READ_CHUNK))
        if not chunk:
            raise ConnectionError("peer closed before requested bytes completed")
        parts.append(chunk)
        remaining -= len(chunk)
    return b"".join(parts)

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
    cuts = (1, 2, 3, 5, 8, 13, 21)
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

def send_rfb(
    conn: socket.socket,
    sequence: int,
    frame_index: int,
) -> int:
    frame = wire.encode_channel_frame(
        wire.FRAME_DATA,
        wire.CHANNEL_RFB,
        sequence,
        payload_for(frame_index),
    )
    send_fragmented(conn, frame)
    return sequence + 1

def recv_control(
    conn: socket.socket,
    expected_sequence: int,
    expected_type: int,
    expected_value: int | None = None,
) -> tuple[int, int]:
    header, payload = recv_frame(conn, expected_sequence)
    if header.kind != wire.FRAME_DATA or header.channel != wire.CHANNEL_CONTROL:
        raise ValueError(f"invalid control envelope {header!r}")
    if len(payload) != CONTROL.size:
        raise ValueError(f"invalid control size {len(payload)}")

    magic, control_type, value = CONTROL.unpack(payload)
    if magic != CONTROL_MAGIC or control_type != expected_type:
        raise ValueError(
            f"control mismatch type={control_type} value={value}"
        )
    if expected_value is not None and value != expected_value:
        raise ValueError(
            f"control value expected={expected_value} actual={value}"
        )
    return expected_sequence + 1, value

def send_control(
    conn: socket.socket,
    sequence: int,
    control_type: int,
    value: int,
) -> int:
    payload = CONTROL.pack(CONTROL_MAGIC, control_type, value)
    frame = wire.encode_channel_frame(
        wire.FRAME_DATA,
        wire.CHANNEL_CONTROL,
        sequence,
        payload,
    )
    send_fragmented(conn, frame)
    return sequence + 1

def establish(conn: socket.socket) -> tuple[int, int]:
    header, payload = recv_frame(conn, 1)
    if not wire.is_hello_header(header):
        raise ValueError(f"expected HELLO, got {header!r}")
    versions = wire.decode_hello_payload(payload)
    expected = (wire.WIRE_VERSION, wire.PRODUCT_ESTABLISHMENT_VERSION)
    if versions != expected:
        raise ValueError(f"HELLO versions expected={expected!r} actual={versions!r}")
    send_fragmented(conn, wire.encode_accept_frame(SESSION_ID, sequence=1))
    stamp(f"Q4_ACCEPT=PASS session_id={SESSION_ID}")
    return 2, 2

def main() -> int:
    stop_event = threading.Event()
    observer = threading.Thread(
        target=udp_observer,
        args=(stop_event,),
        daemon=True,
    )
    observer.start()
    time.sleep(0.1)

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

            # Frame 0 is deliberately published after the consumer snapshot but
            # before it enters wait_activity().
            send_sequence = send_rfb(conn, send_sequence, 0)
            stamp("PREPUBLISH_SENT frame=0")

            for frame_index in range(1, PRE_BLOCKED_ROUNDS + 1):
                receive_sequence, _ = recv_control(
                    conn,
                    receive_sequence,
                    CONTROL_READY,
                    frame_index,
                )
                send_sequence = send_rfb(
                    conn,
                    send_sequence,
                    frame_index,
                )

            stamp(f"PRE_BLOCKED=PASS rounds={PRE_BLOCKED_ROUNDS}")

            idle_index = 1 + PRE_BLOCKED_ROUNDS
            receive_sequence, _ = recv_control(
                conn,
                receive_sequence,
                CONTROL_IDLE_READY,
                idle_index,
            )
            stamp(f"IDLE_BEGIN seconds={IDLE_SECONDS:.0f}")
            time.sleep(IDLE_SECONDS)
            send_sequence = send_rfb(conn, send_sequence, idle_index)

            receive_sequence, empty_polls = recv_control(
                conn,
                receive_sequence,
                CONTROL_IDLE_RESULT,
            )
            if empty_polls == 0:
                raise ValueError("idle result reported zero empty polls")
            stamp(f"IDLE_WAKE=PASS empty_polls={empty_polls}")

            first_post = idle_index + 1
            for frame_index in range(
                first_post,
                TOTAL_FRAMES,
            ):
                receive_sequence, _ = recv_control(
                    conn,
                    receive_sequence,
                    CONTROL_READY,
                    frame_index,
                )
                send_sequence = send_rfb(
                    conn,
                    send_sequence,
                    frame_index,
                )

            stamp(f"POST_BLOCKED=PASS rounds={POST_BLOCKED_ROUNDS}")

            receive_sequence, final_activity = recv_control(
                conn,
                receive_sequence,
                CONTROL_PASS,
                TOTAL_FRAMES,
            )
            if final_activity != TOTAL_FRAMES:
                raise ValueError(
                    f"activity expected={TOTAL_FRAMES} actual={final_activity}"
                )

            send_sequence = send_control(
                conn,
                send_sequence,
                CONTROL_DONE,
                TOTAL_FRAMES,
            )
            stamp(
                f"ACTIVITY=PASS frames={TOTAL_FRAMES} "
                f"final_sequence={final_activity}"
            )
            stamp("DONE_SENT_HOLDING_TCP_OPEN")

            conn.settimeout(120.0)
            try:
                trailing = conn.recv(1)
            except socket.timeout:
                stamp("PS2_TCP_CLOSE=TIMEOUT")
                time.sleep(2.0)
                stop_event.set()
                observer.join(timeout=2.0)
                return 2

            if trailing:
                stamp(f"PS2_TCP_CLOSE=UNEXPECTED_DATA byte={trailing.hex()}")
                time.sleep(2.0)
                stop_event.set()
                observer.join(timeout=2.0)
                return 3

            stamp("PS2_TCP_CLOSE=PASS")
            time.sleep(2.0)
            stop_event.set()
            observer.join(timeout=2.0)
            stamp("DIAGNOSTIC=COMPLETE")
            return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        stamp(f"CHECKPOINT=FAIL error={exc!r}")
        raise SystemExit(1)
