#!/usr/bin/env python3
"""File synopsis:
Defines the Raspberry Pi product-side PSTV Wire framing and provisional
establishment representation shared conceptually with src/transport/protocol.*.

This module owns bytes only: fixed headers, HELLO, ACCEPT, NOT_ACCEPTED, exact
RFB DATA/CREDIT/provider-terminal framing, exact AUDIO DATA/CREDIT/producer-done
framing, exact MPEG START/RETIRE/DATA/CREDIT framing, envelope classification,
and unsigned-field validation. It owns no
sockets, listener/session lifecycle, rider dispatch, MPEG producer state, RFB
provider lifecycle, reconnect policy, or systemd behavior.

Context: docs/ledge/LEDGE_AUDIT_A001_TRANSPORT_RFB.md.
"""

from __future__ import annotations

from dataclasses import dataclass
import struct

# Every PSTV frame starts with the same 16-byte network-order header:
# magic, header version, kind, logical channel, flags, sequence, payload length.
# The header version describes framing itself; WIRE_VERSION below is the
# separately negotiated product Wire compatibility version carried by HELLO.
MAGIC = b"PSTV"
HEADER = struct.Struct(">4sBBBBII")
HEADER_BYTES = HEADER.size
WIRE_HEADER_VERSION = 1
MAX_PAYLOAD_BYTES = 8192

# Product protocol frame identities. ERROR=7 was a dormant framing reservation
# until R16A assigned the exact channel-1 provider-terminal contract below.
FRAME_HELLO = 1
FRAME_DATA = 3
FRAME_CREDIT = 4
FRAME_ERROR = 7
FRAME_MPEG_RETIRE = 10
FRAME_MPEG_START = 11
FRAME_ACCEPT = 12
FRAME_NOT_ACCEPTED = 13

CHANNEL_CONTROL = 0
CHANNEL_RFB = 1
CHANNEL_AUDIO = 2
CHANNEL_MPEG2 = 4

# HELLO carries two independent compatibility words. Fixed framing and the
# existing Wire mechanics remain version 1. Product-establishment version 2 is
# the deliberate Q4 compatibility fence for R16A's post-Q4 channel-1 ERROR
# semantics: a peer that knows only product version 1 is rejected before ACTIVE.
WIRE_VERSION = 1
PRODUCT_ESTABLISHMENT_VERSION = 2

# HELLO is exactly two uint32 values. ACCEPT, NOT_ACCEPTED, CREDIT, and the R16A
# RFB provider-terminal ERROR payload each carry one network-order uint32 word.
HELLO = struct.Struct(">II")
ONE_WORD = struct.Struct(">I")
CREDIT = ONE_WORD
RFB_PROVIDER_FAILURE = ONE_WORD
MPEG_RETIRE = struct.Struct(">III")
MPEG_START = struct.Struct(">11I")
MPEG_GENERATION_CONTROL_VERSION = 1

REJECT_WIRE_VERSION = 1
REJECT_PRODUCT_VERSION = 2
REJECT_MALFORMED = 3

# R16A terminal causes are deliberately mechanism-level and finite. READ covers
# either provider EOF or a provider recv/read failure because both are terminal
# observations at the same accepted provider-read boundary.
RFB_PROVIDER_FAILURE_CONNECT = 1
RFB_PROVIDER_FAILURE_READ = 2
RFB_PROVIDER_FAILURE_WRITE = 3

UINT32_MAX = 0xFFFFFFFF


class WireProtocolError(ValueError):
    """Reject malformed or unrepresentable product Wire bytes."""


@dataclass(frozen=True)
class WireHeader:
    version: int
    kind: int
    channel: int
    flags: int
    sequence: int
    payload_length: int


@dataclass(frozen=True)
class MpegRetireControl:
    session_id: int
    generation: int


@dataclass(frozen=True)
class MpegStartControl:
    session_id: int
    generation: int
    base_x: int
    base_y: int
    base_width: int
    base_height: int
    suppression_x: int
    suppression_y: int
    suppression_width: int
    suppression_height: int


def _require_u32(value: int, name: str) -> None:
    if not isinstance(value, int) or value < 0 or value > UINT32_MAX:
        raise WireProtocolError(f"{name} is outside uint32 range")


def encode_header(header: WireHeader) -> bytes:
    _require_u32(header.sequence, "sequence")
    _require_u32(header.payload_length, "payload_length")
    if header.version != WIRE_HEADER_VERSION:
        raise WireProtocolError("unsupported Wire header version")
    if header.payload_length > MAX_PAYLOAD_BYTES:
        raise WireProtocolError("payload exceeds product Wire maximum")
    for name, value in (
        ("kind", header.kind),
        ("channel", header.channel),
        ("flags", header.flags),
    ):
        if not isinstance(value, int) or value < 0 or value > 0xFF:
            raise WireProtocolError(f"{name} is outside uint8 range")
    return HEADER.pack(
        MAGIC,
        header.version,
        header.kind,
        header.channel,
        header.flags,
        header.sequence,
        header.payload_length,
    )


def decode_header(data: bytes) -> WireHeader:
    if len(data) != HEADER_BYTES:
        raise WireProtocolError("Wire header must be exactly 16 bytes")
    magic, version, kind, channel, flags, sequence, payload_length = HEADER.unpack(data)
    if magic != MAGIC:
        raise WireProtocolError("Wire magic mismatch")
    if version != WIRE_HEADER_VERSION:
        raise WireProtocolError("unsupported Wire header version")
    if payload_length > MAX_PAYLOAD_BYTES:
        raise WireProtocolError("payload exceeds product Wire maximum")
    return WireHeader(
        version=version,
        kind=kind,
        channel=channel,
        flags=flags,
        sequence=sequence,
        payload_length=payload_length,
    )


def encode_channel_frame(
    kind: int,
    channel: int,
    sequence: int,
    payload: bytes,
) -> bytes:
    """Encode one exact zero-flags PSTV frame for a selected logical channel."""

    header = WireHeader(
        version=WIRE_HEADER_VERSION,
        kind=kind,
        channel=channel,
        flags=0,
        sequence=sequence,
        payload_length=len(payload),
    )
    return encode_header(header) + payload


def encode_frame(kind: int, sequence: int, payload: bytes) -> bytes:
    # Establishment helpers use the control channel so Q4 identity cannot drift
    # independently between frame kinds.
    return encode_channel_frame(
        kind,
        CHANNEL_CONTROL,
        sequence,
        payload,
    )


def encode_hello_payload(
    wire_version: int = WIRE_VERSION,
    product_version: int = PRODUCT_ESTABLISHMENT_VERSION,
) -> bytes:
    _require_u32(wire_version, "wire_version")
    _require_u32(product_version, "product_establishment_version")
    return HELLO.pack(wire_version, product_version)


def decode_hello_payload(payload: bytes) -> tuple[int, int]:
    if len(payload) != HELLO.size:
        raise WireProtocolError("HELLO payload must be exactly 8 bytes")
    return HELLO.unpack(payload)


def encode_accept_payload(session_id: int) -> bytes:
    _require_u32(session_id, "session_id")
    # Zero is reserved to mean "no active Wire Session"; ACCEPT may therefore
    # publish only a Pi-authoritative nonzero identity.
    if session_id == 0:
        raise WireProtocolError("ACCEPT session_id must be nonzero")
    return ONE_WORD.pack(session_id)


def decode_accept_payload(payload: bytes) -> int:
    if len(payload) != ONE_WORD.size:
        raise WireProtocolError("ACCEPT payload must be exactly 4 bytes")
    session_id = ONE_WORD.unpack(payload)[0]
    if session_id == 0:
        raise WireProtocolError("ACCEPT session_id must be nonzero")
    return session_id


def _require_rejection_reason(reason: int) -> None:
    if reason not in (
        REJECT_WIRE_VERSION,
        REJECT_PRODUCT_VERSION,
        REJECT_MALFORMED,
    ):
        raise WireProtocolError("unknown NOT_ACCEPTED reason")


def encode_not_accepted_payload(reason: int) -> bytes:
    _require_rejection_reason(reason)
    return ONE_WORD.pack(reason)


def decode_not_accepted_payload(payload: bytes) -> int:
    if len(payload) != ONE_WORD.size:
        raise WireProtocolError("NOT_ACCEPTED payload must be exactly 4 bytes")
    reason = ONE_WORD.unpack(payload)[0]
    _require_rejection_reason(reason)
    return reason


def encode_hello_frame(
    wire_version: int = WIRE_VERSION,
    product_version: int = PRODUCT_ESTABLISHMENT_VERSION,
    sequence: int = 1,
) -> bytes:
    # Sequence 1 is the provisional transaction's first client->server frame.
    # Later ordinary Wire traffic begins from the next direction-local sequence
    # rather than reusing this establishment slot.
    return encode_frame(
        FRAME_HELLO,
        sequence,
        encode_hello_payload(wire_version, product_version),
    )


def encode_accept_frame(session_id: int, sequence: int = 1) -> bytes:
    return encode_frame(FRAME_ACCEPT, sequence, encode_accept_payload(session_id))


def encode_not_accepted_frame(reason: int, sequence: int = 1) -> bytes:
    return encode_frame(
        FRAME_NOT_ACCEPTED,
        sequence,
        encode_not_accepted_payload(reason),
    )


def encode_rfb_credit_payload(amount: int) -> bytes:
    _require_u32(amount, "RFB credit")
    if amount == 0:
        raise WireProtocolError("RFB credit must be nonzero")
    return CREDIT.pack(amount)


def decode_rfb_credit_payload(payload: bytes) -> int:
    if len(payload) != CREDIT.size:
        raise WireProtocolError("RFB CREDIT payload must be exactly 4 bytes")
    amount = CREDIT.unpack(payload)[0]
    if amount == 0:
        raise WireProtocolError("RFB credit must be nonzero")
    return amount


def encode_rfb_credit_frame(amount: int, sequence: int) -> bytes:
    return encode_channel_frame(
        FRAME_CREDIT,
        CHANNEL_RFB,
        sequence,
        encode_rfb_credit_payload(amount),
    )


def encode_rfb_data_frame(payload: bytes, sequence: int) -> bytes:
    if len(payload) > MAX_PAYLOAD_BYTES:
        raise WireProtocolError("RFB DATA payload exceeds product Wire maximum")
    return encode_channel_frame(
        FRAME_DATA,
        CHANNEL_RFB,
        sequence,
        payload,
    )


def encode_audio_credit_payload(amount: int) -> bytes:
    _require_u32(amount, "AUDIO credit")
    if amount == 0:
        raise WireProtocolError("AUDIO credit must be nonzero")
    return CREDIT.pack(amount)


def decode_audio_credit_payload(payload: bytes) -> int:
    if len(payload) != CREDIT.size:
        raise WireProtocolError("AUDIO CREDIT payload must be exactly 4 bytes")
    amount = CREDIT.unpack(payload)[0]
    if amount == 0:
        raise WireProtocolError("AUDIO credit must be nonzero")
    return amount


def encode_audio_credit_frame(amount: int, sequence: int) -> bytes:
    return encode_channel_frame(
        FRAME_CREDIT,
        CHANNEL_AUDIO,
        sequence,
        encode_audio_credit_payload(amount),
    )


def encode_audio_data_frame(payload: bytes, sequence: int) -> bytes:
    if not payload:
        raise WireProtocolError(
            "ordinary AUDIO DATA payload must be non-empty"
        )
    if len(payload) > MAX_PAYLOAD_BYTES:
        raise WireProtocolError("AUDIO DATA payload exceeds product Wire maximum")
    return encode_channel_frame(
        FRAME_DATA,
        CHANNEL_AUDIO,
        sequence,
        payload,
    )


def encode_audio_producer_done_frame(sequence: int) -> bytes:
    """Encode the explicit existing zero-length AUDIO producer-done marker."""

    return encode_channel_frame(
        FRAME_DATA,
        CHANNEL_AUDIO,
        sequence,
        b"",
    )


def encode_mpeg_credit_payload(amount: int) -> bytes:
    _require_u32(amount, "MPEG credit")
    if amount == 0:
        raise WireProtocolError("MPEG credit must be nonzero")
    return CREDIT.pack(amount)


def decode_mpeg_credit_payload(payload: bytes) -> int:
    if len(payload) != CREDIT.size:
        raise WireProtocolError("MPEG CREDIT payload must be exactly 4 bytes")
    amount = CREDIT.unpack(payload)[0]
    if amount == 0:
        raise WireProtocolError("MPEG credit must be nonzero")
    return amount


def encode_mpeg_credit_frame(amount: int, sequence: int) -> bytes:
    return encode_channel_frame(
        FRAME_CREDIT,
        CHANNEL_MPEG2,
        sequence,
        encode_mpeg_credit_payload(amount),
    )


def encode_mpeg_data_frame(payload: bytes, sequence: int) -> bytes:
    if not payload:
        raise WireProtocolError("MPEG DATA payload must be non-empty")
    if len(payload) > MAX_PAYLOAD_BYTES:
        raise WireProtocolError("MPEG DATA payload exceeds product Wire maximum")
    return encode_channel_frame(
        FRAME_DATA,
        CHANNEL_MPEG2,
        sequence,
        payload,
    )


def encode_mpeg_retire_payload(control: MpegRetireControl) -> bytes:
    _require_u32(control.session_id, "MPEG RETIRE session_id")
    _require_u32(control.generation, "MPEG RETIRE generation")
    return MPEG_RETIRE.pack(
        MPEG_GENERATION_CONTROL_VERSION,
        control.session_id,
        control.generation,
    )


def decode_mpeg_retire_payload(payload: bytes) -> MpegRetireControl:
    if len(payload) != MPEG_RETIRE.size:
        raise WireProtocolError("MPEG RETIRE payload must be exactly 12 bytes")
    version, session_id, generation = MPEG_RETIRE.unpack(payload)
    if version != MPEG_GENERATION_CONTROL_VERSION:
        raise WireProtocolError("unsupported MPEG RETIRE control version")
    return MpegRetireControl(
        session_id=session_id,
        generation=generation,
    )


def encode_mpeg_retire_frame(
    control: MpegRetireControl,
    sequence: int,
) -> bytes:
    return encode_channel_frame(
        FRAME_MPEG_RETIRE,
        CHANNEL_CONTROL,
        sequence,
        encode_mpeg_retire_payload(control),
    )


def encode_mpeg_start_payload(control: MpegStartControl) -> bytes:
    values = (
        control.session_id,
        control.generation,
        control.base_x,
        control.base_y,
        control.base_width,
        control.base_height,
        control.suppression_x,
        control.suppression_y,
        control.suppression_width,
        control.suppression_height,
    )
    names = (
        "session_id",
        "generation",
        "base_x",
        "base_y",
        "base_width",
        "base_height",
        "suppression_x",
        "suppression_y",
        "suppression_width",
        "suppression_height",
    )
    for name, value in zip(names, values):
        _require_u32(value, f"MPEG START {name}")
    return MPEG_START.pack(
        MPEG_GENERATION_CONTROL_VERSION,
        *values,
    )


def decode_mpeg_start_payload(payload: bytes) -> MpegStartControl:
    if len(payload) != MPEG_START.size:
        raise WireProtocolError("MPEG START payload must be exactly 44 bytes")
    words = MPEG_START.unpack(payload)
    if words[0] != MPEG_GENERATION_CONTROL_VERSION:
        raise WireProtocolError("unsupported MPEG START control version")
    return MpegStartControl(
        session_id=words[1],
        generation=words[2],
        base_x=words[3],
        base_y=words[4],
        base_width=words[5],
        base_height=words[6],
        suppression_x=words[7],
        suppression_y=words[8],
        suppression_width=words[9],
        suppression_height=words[10],
    )


def encode_mpeg_start_frame(
    control: MpegStartControl,
    sequence: int,
) -> bytes:
    return encode_channel_frame(
        FRAME_MPEG_START,
        CHANNEL_CONTROL,
        sequence,
        encode_mpeg_start_payload(control),
    )


def _require_rfb_provider_failure_reason(reason: int) -> None:
    if reason not in (
        RFB_PROVIDER_FAILURE_CONNECT,
        RFB_PROVIDER_FAILURE_READ,
        RFB_PROVIDER_FAILURE_WRITE,
    ):
        raise WireProtocolError("unknown RFB provider failure reason")


def encode_rfb_provider_failure_payload(reason: int) -> bytes:
    """Encode one exact first-terminal RFB provider mechanism cause."""

    _require_rfb_provider_failure_reason(reason)
    return RFB_PROVIDER_FAILURE.pack(reason)


def decode_rfb_provider_failure_payload(payload: bytes) -> int:
    if len(payload) != RFB_PROVIDER_FAILURE.size:
        raise WireProtocolError("RFB provider failure payload must be exactly 4 bytes")
    reason = RFB_PROVIDER_FAILURE.unpack(payload)[0]
    _require_rfb_provider_failure_reason(reason)
    return reason


def encode_rfb_provider_failure_frame(reason: int, sequence: int) -> bytes:
    return encode_channel_frame(
        FRAME_ERROR,
        CHANNEL_RFB,
        sequence,
        encode_rfb_provider_failure_payload(reason),
    )


def is_rfb_credit_header(header: WireHeader) -> bool:
    return (
        header.kind == FRAME_CREDIT
        and header.channel == CHANNEL_RFB
        and header.flags == 0
        and header.payload_length == CREDIT.size
    )


def is_rfb_data_header(header: WireHeader) -> bool:
    # Zero-length DATA is intentionally valid framing but reserved for the
    # existing RFB quiesce lifecycle rather than raw provider bytes.
    return (
        header.kind == FRAME_DATA
        and header.channel == CHANNEL_RFB
        and header.flags == 0
        and header.payload_length <= MAX_PAYLOAD_BYTES
    )


def is_rfb_provider_failure_header(header: WireHeader) -> bool:
    return (
        header.kind == FRAME_ERROR
        and header.channel == CHANNEL_RFB
        and header.flags == 0
        and header.payload_length == RFB_PROVIDER_FAILURE.size
    )


def is_audio_credit_header(header: WireHeader) -> bool:
    return (
        header.kind == FRAME_CREDIT
        and header.channel == CHANNEL_AUDIO
        and header.flags == 0
        and header.payload_length == CREDIT.size
    )


def is_audio_data_header(header: WireHeader) -> bool:
    return (
        header.kind == FRAME_DATA
        and header.channel == CHANNEL_AUDIO
        and header.flags == 0
        and header.payload_length != 0
        and header.payload_length <= MAX_PAYLOAD_BYTES
    )


def is_audio_producer_done_header(header: WireHeader) -> bool:
    return (
        header.kind == FRAME_DATA
        and header.channel == CHANNEL_AUDIO
        and header.flags == 0
        and header.payload_length == 0
    )


def is_mpeg_credit_header(header: WireHeader) -> bool:
    return (
        header.kind == FRAME_CREDIT
        and header.channel == CHANNEL_MPEG2
        and header.flags == 0
        and header.payload_length == CREDIT.size
    )


def is_mpeg_data_header(header: WireHeader) -> bool:
    return (
        header.kind == FRAME_DATA
        and header.channel == CHANNEL_MPEG2
        and header.flags == 0
        and header.payload_length != 0
        and header.payload_length <= MAX_PAYLOAD_BYTES
    )


def is_mpeg_retire_header(header: WireHeader) -> bool:
    return (
        header.kind == FRAME_MPEG_RETIRE
        and header.channel == CHANNEL_CONTROL
        and header.flags == 0
        and header.payload_length == MPEG_RETIRE.size
    )


def is_mpeg_start_header(header: WireHeader) -> bool:
    return (
        header.kind == FRAME_MPEG_START
        and header.channel == CHANNEL_CONTROL
        and header.flags == 0
        and header.payload_length == MPEG_START.size
    )


def is_hello_header(header: WireHeader) -> bool:
    return (
        header.kind == FRAME_HELLO
        and header.channel == CHANNEL_CONTROL
        and header.flags == 0
        and header.payload_length == HELLO.size
    )


def is_accept_header(header: WireHeader) -> bool:
    return (
        header.kind == FRAME_ACCEPT
        and header.channel == CHANNEL_CONTROL
        and header.flags == 0
        and header.payload_length == ONE_WORD.size
    )


def is_not_accepted_header(header: WireHeader) -> bool:
    return (
        header.kind == FRAME_NOT_ACCEPTED
        and header.channel == CHANNEL_CONTROL
        and header.flags == 0
        and header.payload_length == ONE_WORD.size
    )
