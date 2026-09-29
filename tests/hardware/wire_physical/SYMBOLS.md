# Symbols — `tests/hardware/wire_physical`

DIRECTORY=tests/hardware/wire_physical
GENERATION=STAGED_RECONSTRUCTION
COVERAGE=CURRENT

This checkpoint directly exercises the admitted physical Wire implementation.
It deliberately contains no logical transport runtime or rider semantics.

| Name | Kind | Owner | Scope | Description |
|---|---|---|---|---|
| `ACTIVE_ROUNDS` | macro | active workload | file | 128 framed request/echo rounds per phase. |
| `ACTIVE_POLL_LIMIT` | macro | active readiness | file | Bounds empty readiness polls while expecting active peer traffic. |
| `IDLE_POLL_LIMIT` | macro | idle discriminator | file | Bounds the long-idle readiness loop. |
| `READINESS_TIMEOUT_US` | macro | physical readiness | file | Calls product `wait_readable()` with 1000 microseconds. |
| `EMPTY_POLL_YIELD_US` | macro | EE scheduling | file | Yields 1 ms after each empty readiness poll, matching ledge runtime policy. |
| `payload_sizes` | constant array | framed workload | file | Cycles payloads from 0 through the 8192-byte Wire maximum. |
| `fill_payload` | function | payload generation | file static | Generates deterministic phase/round/index bytes. |
| `wait_until_readable` | function | readiness loop | file static | Repeatedly exercises product select-based readiness plus EE yield. |
| `receive_data_frame` | function | framed receive | file static | Waits, receives one physical frame, and validates opaque test envelope/payload. |
| `run_active_phase` | function | active workload | file static | Executes numbered variable-size bidirectional framed traffic. |
| `send_marker` / `receive_marker` | function family | checkpoint control | file static | Uses test-only DATA/control-channel markers without introducing rider semantics. |
| `run_idle_wake` | function | long-idle discriminator | file static | Announces idle, waits via product readiness path, consumes Pi wake, and reports empty-poll count. |
| `main` | function | checkpoint coordinator | entry point | Initializes platform/network, performs Q4, active→idle/wake→active traffic, final handshake, and success exit. |
