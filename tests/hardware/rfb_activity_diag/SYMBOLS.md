# Symbols — `tests/hardware/rfb_activity_diag`

DIRECTORY=tests/hardware/rfb_activity_diag
GENERATION=DIAGNOSTIC_VARIANT
COVERAGE=CURRENT

This is an instrumentation-only variant of checkpoint 03B. It links the same
product modules as the 03B candidate and adds best-effort UDP stage markers only
inside the hardware harness. It does not alter PSTV Wire semantics, RFB flow
semantics, credit, parser behavior, or product lifecycle code.

| Name | Kind | Owner | Scope | Description |
|---|---|---|---|---|
| `TELEMETRY_MAGIC` / `TELEMETRY_PORT` | macro family | diagnostic sideband | file | Identify 20-byte UDP records sent to Pi port 5999. |
| `STAGE_*` | macro family | post-proof lifecycle map | file | Stable stage IDs for every final 03B boundary from consumer completion through OSDSYS entry. |
| `telemetry_socket` / `telemetry_address` / `telemetry_sequence` | file state | diagnostic sideband | file static | Own the independent UDP observer path and monotonically increasing record sequence. |
| `telemetry_init` | function | diagnostic sideband | file static | Opens the independent UDP socket after private-link readiness. Failure is non-fatal. |
| `telemetry_emit` | function | diagnostic sideband | file static | Sends one fire-and-forget 20-byte network-order record; return value is deliberately ignored. |
| original 03B workload symbols | inherited harness | activity discriminator | file | Same activity-before-wait, blocked wake, long-idle wake, and post-idle workload as checkpoint 03B. |
