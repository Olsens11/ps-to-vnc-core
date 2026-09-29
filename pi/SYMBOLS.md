# Symbols — `pi`

DIRECTORY=pi
GENERATION=STAGED_RECONSTRUCTION
COVERAGE=CURRENT

This directory currently contains the Raspberry Pi peer for hardware checkpoint
01. Its record constants mirror the PS2 checkpoint harness deliberately.

| Name | Kind | Owner | Scope | Description |
|---|---|---|---|---|
| `HOST` / `PORT` | constant | listener | module | Bind the checkpoint peer to `192.168.50.1:5959`. |
| `MAGIC` | constant | checkpoint record | module | Record discriminator matching the PS2 harness. |
| `TYPE_PING` | constant | checkpoint protocol | module | Ordinary request/echo record type. |
| `TYPE_IDLE_READY` | constant | checkpoint protocol | module | Expected PS2 marker before deliberate application silence. |
| `TYPE_WAKE` | constant | checkpoint protocol | module | Pi-to-PS2 receive-after-idle stimulus. |
| `TYPE_WOKE` | constant | checkpoint protocol | module | Expected PS2 acknowledgement of the wake. |
| `TYPE_DONE` | constant | checkpoint protocol | module | Final Pi-to-PS2 success terminator. |
| `PHASE_ONE` / `PHASE_TWO` | constant | checkpoint protocol | module | Active phases before and after idle. |
| `ROUND_COUNT` | constant | workload | module | 128 exchanges per active phase. |
| `IDLE_SECONDS` | constant | idle discriminator | module | Holds application traffic silent for 60 seconds before wake. |
| `RECORD` | struct codec | checkpoint record | module | Encodes/decodes four unsigned 32-bit network-order fields. |
| `stamp` | function | evidence logging | module | Emits offset-aware timestamps with each checkpoint milestone. |
| `recv_exact` | function | socket I/O | module | Receives exactly one requested byte count or raises on premature close. |
| `receive_record` | function | record parsing | module | Reads and decodes one complete checkpoint record. |
| `expect_record` | function | protocol validation | module | Requires an exact type/phase/sequence tuple. |
| `echo_rounds` | function | active workload | module | Validates and echoes one numbered 128-record phase. |
| `main` | function | checkpoint coordinator | entry point | Accepts one PS2 connection, runs active-idle-wake-active qualification, and emits final PASS/FAIL evidence. |
