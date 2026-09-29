# Symbols — `tests/hardware/platform_network`

DIRECTORY=tests/hardware/platform_network
GENERATION=STAGED_RECONSTRUCTION
COVERAGE=CURRENT

This directory owns checkpoint-only PS2 code used to qualify the platform and
network foundation. It is not product Transport/Wire code.

| Name | Kind | Owner | Scope | Description |
|---|---|---|---|---|
| `MAGIC` | macro | checkpoint record | file | Network-order record discriminator `0x50535456`. |
| `TYPE_PING` | macro | checkpoint protocol | file | Ordinary request/echo record type. |
| `TYPE_IDLE_READY` | macro | checkpoint protocol | file | PS2 marker announcing entry into the controlled idle phase. |
| `TYPE_WAKE` | macro | checkpoint protocol | file | Pi-to-PS2 record sent after the deliberate idle interval. |
| `TYPE_WOKE` | macro | checkpoint protocol | file | PS2 acknowledgement proving the wake record was consumed. |
| `TYPE_DONE` | macro | checkpoint protocol | file | Final Pi-to-PS2 record authorizing successful checkpoint exit. |
| `PHASE_ONE` / `PHASE_TWO` | macro | checkpoint protocol | file | Distinguish the active exchanges before and after the idle wake. |
| `ROUND_COUNT` | macro | checkpoint workload | file | Sets each active phase to 128 request/echo exchanges. |
| `checkpoint_record` | structure | checkpoint protocol | file | Four 32-bit network-order fields: magic, type, phase, sequence. |
| `make_record` | function | record construction | file static | Writes one checkpoint record in network byte order. |
| `send_all` | function | socket I/O | file static | Sends the complete caller-supplied byte range or fails. |
| `receive_exact` | function | socket I/O | file static | Receives exactly the requested byte count or fails. |
| `exchange_rounds` | function | active workload | file static | Performs one complete numbered request/echo phase and verifies exact echoes. |
| `main` | function | checkpoint coordinator | entry point | Initializes platform/network, executes both active phases and idle wake handshake, then exits intentionally on success. |
