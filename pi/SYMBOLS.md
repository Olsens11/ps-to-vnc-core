# Symbols — `pi`

DIRECTORY=pi
GENERATION=STAGED_RECONSTRUCTION
COVERAGE=CURRENT

The directory now contains one product module, `wire_protocol.py`, plus the
checkpoint-01 platform/network peer. Rung 02 qualifies the product codec's
generic framing and Q4 establishment behavior; rider-specific RFB/audio/MPEG
helpers are present from ledge but remain semantically deferred.

## Product Wire codec

| Name | Kind | Owner | Scope | Description | Rung 02 status |
|---|---|---|---|---|---|
| `MAGIC` / `HEADER` / `HEADER_BYTES` | constant/codec | Wire header | module | Define exact `PSTV` 16-byte network-order framing. | qualified target |
| `WIRE_HEADER_VERSION` | constant | Wire header | module | Fixed framing version 1. | qualified target |
| `MAX_PAYLOAD_BYTES` | constant | Wire header | module | Maximum payload size, 8192 bytes. | qualified target |
| `FRAME_*` | constant family | Wire vocabulary | module | Stable frame-kind identities. | framing qualified; rider semantics deferred |
| `CHANNEL_*` | constant family | Wire vocabulary | module | Stable control/RFB/audio/MPEG channel identities. | identity qualified; rider semantics deferred |
| `WIRE_VERSION` / `PRODUCT_ESTABLISHMENT_VERSION` | constant | Q4 establishment | module | Negotiated compatibility words 1 and 2. | qualified target |
| `WireProtocolError` | exception | Wire validation | module | Rejects malformed/unrepresentable Wire values. | qualified target |
| `WireHeader` | dataclass | Wire header | module | Immutable decoded header representation. | qualified target |
| `encode_header` / `decode_header` | function family | Wire header | module | Encode/decode exact fixed headers with bounds/version checks. | qualified target |
| `encode_channel_frame` / `encode_frame` | function family | generic framing | module | Build zero-flags framed payloads for one selected logical channel. | qualified target |
| HELLO/ACCEPT/NOT_ACCEPTED codec helpers | function family | Q4 establishment | module | Encode/decode exact provisional payloads and control frames. | qualified target |
| RFB codec/classifier helpers | function family | future RFB rider | module | DATA/CREDIT/provider-terminal helpers. | present, semantics deferred |
| audio codec/classifier helpers | function family | future audio rider | module | DATA/CREDIT/producer-done helpers. | present, semantics deferred |
| MPEG codec/classifier helpers | function/type family | future MPEG rider | module | START/RETIRE/DATA/CREDIT helpers and controls. | present, semantics deferred |

## Checkpoint 01 peer

| Name | Kind | Owner | Scope | Description |
|---|---|---|---|---|
| `HOST` / `PORT` | constant | listener | module | Bind the checkpoint peer to `192.168.50.1:5959`. |
| `TYPE_PING` / `TYPE_IDLE_READY` / `TYPE_WAKE` / `TYPE_WOKE` / `TYPE_DONE` | constant family | checkpoint protocol | module | Checkpoint-01 request/idle/wake/final message identities. |
| `PHASE_ONE` / `PHASE_TWO` / `ROUND_COUNT` / `IDLE_SECONDS` | constant family | checkpoint workload | module | Defines the qualified active-idle-active workload. |
| `RECORD` | struct codec | checkpoint record | module | Encodes four unsigned 32-bit network-order fields. |
| `stamp` | function | evidence logging | module | Emits offset-aware timestamps. |
| `recv_exact` / `receive_record` / `expect_record` | function family | peer receive/validation | module | Reads and validates exact checkpoint records. |
| `echo_rounds` | function | active workload | module | Validates and echoes a numbered checkpoint phase. |
| `main` | function | checkpoint coordinator | entry point | Runs checkpoint 01 and emits final PASS/FAIL evidence. |

## Checkpoint 02 peer

`wire_physical_probe.py` is test apparatus. It imports the real
`wire_protocol.py`; framing is not reimplemented in the peer.

| Name | Kind | Owner | Scope | Description |
|---|---|---|---|---|
| `HOST` / `PORT` / `SESSION_ID` | constant family | checkpoint listener/Q4 | module | Bind the private test endpoint and publish one nonzero fixture session ID. |
| `ACTIVE_ROUNDS` / `IDLE_SECONDS` / `PAYLOAD_SIZES` | constant family | workload | module | Define 128-round active phases, 60-second silence, and payloads through 8192 bytes. |
| `recv_exact` / `recv_frame` | function family | framed receive | module | Read exact stream bytes and enforce direction-local Wire sequence. |
| `send_fragmented` | function | framed send | module | Splits Pi-to-PS2 frames across several socket sends. |
| `expected_payload` / `require_data_frame` | function family | validation | module | Independently reconstruct and validate opaque test payloads/envelopes. |
| `run_active_phase` | function | active workload | module | Validates PS2 frames and sends fragmented exact echoes. |
| `establish` | function | Q4 fixture | module | Validates exact HELLO versions and returns sequence-1 ACCEPT. |
| `run_idle_wake` | function | idle discriminator | module | Holds application traffic silent for 60 seconds, injects wake, and records PS2 empty-poll count. |
| `finish` / `main` | function family | checkpoint coordination | module | Complete final handshake and emit timestamped PASS/FAIL evidence. |
