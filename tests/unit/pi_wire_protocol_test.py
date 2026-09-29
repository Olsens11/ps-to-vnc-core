#!/usr/bin/env python3
"""Host checks for the imported Pi-side PSTV Wire codec."""

from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "pi"))

import wire_protocol as wire  # noqa: E402

EXPECTED_HELLO = bytes.fromhex(
    "50535456 01010000 00000001 00000008 "
    "00000001 00000002"
)
EXPECTED_ACCEPT = bytes.fromhex(
    "50535456 010c0000 00000001 00000004 "
    "12345678"
)

def check(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)

def test_exact_establishment_bytes() -> None:
    hello = wire.encode_hello_frame()
    check(hello == EXPECTED_HELLO, "HELLO exact bytes differ")

    accept = wire.encode_accept_frame(0x12345678)
    check(accept == EXPECTED_ACCEPT, "ACCEPT exact bytes differ")

    header = wire.decode_header(hello[: wire.HEADER_BYTES])
    check(header.version == wire.WIRE_HEADER_VERSION, "header version")
    check(header.kind == wire.FRAME_HELLO, "HELLO kind")
    check(header.channel == wire.CHANNEL_CONTROL, "HELLO channel")
    check(header.sequence == 1, "HELLO sequence")
    check(header.payload_length == wire.HELLO.size, "HELLO length")

    versions = wire.decode_hello_payload(hello[wire.HEADER_BYTES :])
    check(
        versions == (
            wire.WIRE_VERSION,
            wire.PRODUCT_ESTABLISHMENT_VERSION,
        ),
        "HELLO versions",
    )

def test_generic_frame_round_trip() -> None:
    payload = bytes((index * 17) & 0xFF for index in range(257))
    encoded = wire.encode_channel_frame(
        wire.FRAME_DATA,
        wire.CHANNEL_RFB,
        77,
        payload,
    )

    header = wire.decode_header(encoded[: wire.HEADER_BYTES])
    check(header.kind == wire.FRAME_DATA, "DATA kind")
    check(header.channel == wire.CHANNEL_RFB, "DATA channel")
    check(header.sequence == 77, "DATA sequence")
    check(header.payload_length == len(payload), "DATA payload length")
    check(encoded[wire.HEADER_BYTES :] == payload, "DATA payload bytes")

def test_reject_invalid_contract() -> None:
    try:
        wire.decode_header(b"short")
    except wire.WireProtocolError:
        pass
    else:
        raise AssertionError("short header accepted")

    try:
        wire.encode_accept_payload(0)
    except wire.WireProtocolError:
        pass
    else:
        raise AssertionError("zero session id accepted")

def main() -> int:
    test_exact_establishment_bytes()
    test_generic_frame_round_trip()
    test_reject_invalid_contract()
    print("pi_wire_protocol_test: PASS")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
