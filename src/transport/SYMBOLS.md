# Symbols — `src/transport`

DIRECTORY=src/transport
GENERATION=STAGED_RECONSTRUCTION
COVERAGE=CURRENT

Rung 02 currently admits only the backend-independent PSTV Wire codec and the
PS2 physical-stream owner. Logical-channel queues, dispatch, credit policy, and
runtime threads are deliberately absent.

| Name | Kind | File | Owner | Scope | Description | Rung 02 status |
|---|---|---|---|---|---|---|
| `PSTVNC_TRANSPORT_MAGIC` | macro | protocol.h | Wire header | public | Fixed `PSTV` network-order magic word. | qualified target |
| `PSTVNC_TRANSPORT_VERSION` | macro | protocol.h | Wire header | public | Fixed framing/header version 1. | qualified target |
| `PSTVNC_TRANSPORT_HEADER_SIZE` | macro | protocol.h | Wire header | public | Fixed 16-byte header size. | qualified target |
| `PSTVNC_TRANSPORT_MAX_PAYLOAD` | macro | protocol.h | Wire header | public | Maximum framed payload, 8192 bytes. | qualified target |
| `pstvnc_transport_frame_kind_t` | enum | protocol.h | Wire vocabulary | public | Stable frame-kind identities including HELLO/DATA/CREDIT/ERROR/ACCEPT. | framing qualified; higher semantics deferred |
| `pstvnc_transport_channel_t` | enum | protocol.h | Wire vocabulary | public | Stable logical channel identities. | identity qualified; rider semantics deferred |
| `pstvnc_transport_header_t` | structure | protocol.h | Wire header | public | Decoded version/kind/channel/flags/sequence/payload-length header. | qualified target |
| `pstvnc_transport_read_be32` / `write_be32` | function family | protocol.[ch] | Wire integer codec | public | Convert exact unsigned 32-bit network-order values. | qualified target |
| `pstvnc_transport_header_encode` / `decode` | function family | protocol.[ch] | Wire header | public | Encode/decode and validate exact 16-byte PSTV headers. | qualified target |
| `pstvnc_wire_hello_payload_t` | structure | protocol.h | Q4 establishment | public | Carries Wire and product-establishment versions. | qualified target |
| `pstvnc_wire_accept_payload_t` | structure | protocol.h | Q4 establishment | public | Carries the nonzero Pi-authoritative session ID. | qualified target |
| `pstvnc_wire_not_accepted_payload_t` | structure | protocol.h | Q4 establishment | public | Carries a typed rejection reason. | qualified target |
| `pstvnc_wire_*_payload_encode/decode` | function family | protocol.[ch] | Q4 establishment | public | Exact HELLO/ACCEPT/NOT_ACCEPTED payload codecs. | qualified target |
| `pstvnc_transport_header_is_wire_*` | function family | protocol.[ch] | Q4 establishment | public | Classify exact control-channel establishment envelopes. | qualified target |
| RFB provider-failure codec family | type/function family | protocol.[ch] | future RFB rider | public | Encodes typed channel-1 provider-terminal reason. | present, not rung-02-qualified semantically |
| MPEG generation codec family | type/function family | protocol.[ch] | future MPEG rider | public | Encodes MPEG START/RETIRE controls and identities. | present, not rung-02-qualified semantically |
| `pstvnc_transport_physical_stream_t` | structure | physical_stream.h | physical PSTV owner | public/internal | Owns one adopted socket, send semaphore, and direction-local sequence state. | qualified target |
| `pstvnc_transport_physical_stream_adopt` | function | physical_stream.[ch] | physical PSTV owner | public/internal | Transfers one valid caller socket into initialized physical ownership. | qualified target |
| `pstvnc_transport_physical_stream_establish_client` | function | physical_stream.[ch] | Q4 establishment | public/internal | Adopts a socket and performs HELLO→ACCEPT/NOT_ACCEPTED sequence-1 transaction. | qualified target |
| `pstvnc_transport_physical_stream_transfer_established` | function | physical_stream.[ch] | ownership move | public/internal | Moves an established sequence-2 lineage without arbitrary reseeding. | host-qualified; runtime use deferred |
| `pstvnc_transport_physical_stream_send_frame` | function | physical_stream.[ch] | framed send | public/internal | Serializes header+payload and advances sequence only after complete send. | qualified target |
| `pstvnc_transport_physical_stream_wait_readable` | function | physical_stream.[ch] | receive readiness | public/internal | Uses `select()` to report readable/timeout/failure without consuming bytes. | primary rung-02 discriminator |
| `pstvnc_transport_physical_stream_receive_frame` | function | physical_stream.[ch] | framed receive | public/internal | Reads exact header+payload, validates sequence/capacity, then advances receive sequence. | qualified target |
| `pstvnc_transport_physical_stream_shutdown_io` | function | physical_stream.[ch] | I/O interruption | public/internal | Calls `shutdown` without releasing descriptor ownership. | host-qualified; lifecycle use deferred |
| `pstvnc_transport_physical_stream_release` | function | physical_stream.[ch] | ownership release | public/internal | Closes the adopted socket, deletes send semaphore, and resets lineage state. | qualified target |

## Rung 03A logical RFB byte channel

| Name | Kind | File | Owner | Scope | Description | Rung 03A status |
|---|---|---|---|---|---|---|
| `pstvnc_transport_rfb_channel_t` | structure | rfb_channel.h | logical RFB storage | public/internal | Caller-storage-backed circular byte stream plus producer activity generation. | qualified target |
| `pstvnc_transport_rfb_channel_initialize` | function | rfb_channel.[ch] | logical RFB storage | public/internal | Binds caller-owned storage and resets an empty channel. | qualified target |
| `pstvnc_transport_rfb_channel_commit` | function | rfb_channel.[ch] | producer commit | public/internal | Atomically commits one complete inbound DATA payload when capacity permits. | qualified target |
| `pstvnc_transport_rfb_channel_read_available` | function | rfb_channel.[ch] | consumer read | public/internal | Consumes up to a requested count from already committed bytes. | qualified target |
| `pstvnc_transport_rfb_channel_read_exact` | function | rfb_channel.[ch] | consumer read | public/internal | Consumes exactly the requested count or leaves the queue unchanged. | qualified target |
| `pstvnc_transport_rfb_channel_discard_residual` | function | rfb_channel.[ch] | terminal discard | public/internal | Discards exactly the expected residual without treating it as parser consumption/activity. | qualified target |
| `pstvnc_transport_rfb_channel_available` | function | rfb_channel.[ch] | queue observation | public/internal | Reports committed unread bytes. | qualified target |
| `pstvnc_transport_rfb_channel_activity_generation` | function | rfb_channel.[ch] | producer activity | public/internal | Reports generation incremented by each non-empty successful commit. | qualified target |
