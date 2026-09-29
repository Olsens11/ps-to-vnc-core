# Symbols — `tests/unit`

DIRECTORY=tests/unit
GENERATION=STAGED_RECONSTRUCTION
COVERAGE=CURRENT

The unit fixtures exercise actual admitted product sources. Imported ledge
fixtures retain their original behavior; new clean fixtures fill explicit
coverage gaps without replacing the code under test.

| File | Purpose | Provenance |
|---|---|---|
| `transport_protocol_test.c` | Exact header bytes, bounds, Q4 codecs, and channel codec identities. | imported from ledge; synopsis added only |
| `transport_physical_stream_test.c` | Deterministic short send/recv, sequence progression, ownership, Q4, shutdown/release behavior. | exact ledge import |
| `transport_physical_readiness_test.c` | Real host socketpair coverage for timeout/readable/error returns from `wait_readable()`. | clean rung-02 addition |
| `pi_wire_protocol_test.py` | Pi exact HELLO/ACCEPT bytes, generic frame round trip, invalid-contract rejection. | clean rung-02 addition |

The imported physical-stream fixture intentionally does not mock product
framing functions; it links the real `physical_stream.c` and `protocol.c`.
