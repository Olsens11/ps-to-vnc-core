# Tested checkpoints

This file records actual qualification, not plans.

## 01 - Platform and network

Status: HARDWARE QUALIFIED

Qualified authority:
- Git commit: `21590f4c2425c68271254e8f38ce09189cae9c0f`
- ELF SHA-256: `14d8faf57139482f0561d68fbb50fca8ee63b7aee82b63147f3ccc4987af3b26`
- ELF bytes: `2241164`
- deployment/readback: PASS
- hardware test ID: `CORE-CP01-STABILITY`

Qualified behavior:
- deterministic PS2 IOP/platform startup;
- private Ethernet link and TCP connection to `192.168.50.1:5959`;
- 128 consecutive bidirectional application exchanges;
- 60 seconds with no application traffic;
- Pi-initiated receive-after-idle wake consumed by the PS2;
- 128 additional bidirectional exchanges after the idle wake;
- intentional return to FreeMcBoot after final Pi-to-PS2 DONE.

Preserved evidence:
- `evidence/hardware/CORE-CP01-STABILITY/deployment.json`
- `evidence/hardware/CORE-CP01-STABILITY/probe.log`

The imported platform source and frozen PS2IP archive remained byte-identical to
their ledge import manifest throughout this qualification.

### Earlier handshake attempts

Attempt 01 used commit
`c4c0d41c149eeda4b2028b614e8c90ab91d1014a` and ELF
`1c395616eb1752ff0a0367c3f22b685a2b933498771fed4a6f7f84e33b1f61e6`.

Result: FAIL / HARNESS-SHUTDOWN-AMBIGUOUS.

The Pi accepted the connection and exact HELLO and sent ACK, but did not receive
the immediate final PASS before its timeout. That harness could not distinguish
a receive failure from an exit/shutdown race.

Attempt 02 reran that exact binary with a final DONE handshake. It completed
HELLO -> ACK -> PASS -> DONE and returned to FreeMcBoot, proving ordinary
bidirectional TCP operation with that imported platform foundation.

### Stability qualification

The strengthened test removed the immediate-exit ambiguity and exercised the
specific idle/wake behavior that matters for later Transport work.

Observed timestamps:
- 20:54:05.876 - TCP accepted from the PS2.
- 20:54:06.070 - phase 1 PASS, 128/128 exchanges.
- 20:54:06.071 - deliberate 60-second application-idle interval began.
- 20:55:06.073 - Pi-to-PS2 idle wake PASS.
- 20:55:06.267 - phase 2 PASS, 128/128 exchanges.
- 20:55:06.267 - checkpoint PASS.

Checkpoint 01 is therefore the first clean-repository hardware baseline.
No Transport, Wire, RFB, framebuffer, input, audio, or MPEG implementation is
included in this baseline.

## 02 - Physical PSTV Wire

Status: HARDWARE QUALIFIED

Qualified authority:
- Git commit: `262ad00dcb0989abcdbd2701b17b5a579ad51c09`
- ELF SHA-256: `c2b59600e829bd3f9b0333ba16420a0328739e240695677de66a737cfd1258eb`
- ELF bytes: `2300344`
- PT_LOAD SHA-256: `e8ab4f13c0fe99f55fd2c4524c0cf5cd9d783e5ebdf8eabe145eaaa5f34601ed`
- PT_LOAD bytes: `363784`
- deployment/readback: PASS
- hardware test ID: `CORE-CP02-WIRE-PHYSICAL`

Qualified behavior:
- exact Wire HELLO/ACCEPT establishment;
- direction-local sequence continuation from sequence 2;
- 128 bidirectional framed rounds before idle;
- payload sizes from 0 through the 8192-byte Wire maximum;
- deliberately fragmented Pi-to-PS2 frame delivery;
- 60 seconds of application silence while the PS2 repeatedly executed
  `pstvnc_transport_physical_stream_wait_readable(..., 1000us)`;
- 57,513 empty readiness polls observed before wake;
- Pi-initiated fragmented Wire DATA wake detected and consumed after idle;
- 128 additional bidirectional framed rounds after wake;
- final framed PASS/DONE handshake and intentional return to FreeMcBoot.

Preserved evidence:
- `evidence/hardware/CORE-CP02-WIRE-PHYSICAL/HARDWARE-AUTHORITY.env`
- `evidence/hardware/CORE-CP02-WIRE-PHYSICAL/deployment.json`
- `evidence/hardware/CORE-CP02-WIRE-PHYSICAL/probe.log`

Scope:
This qualifies `protocol.[ch]`, `physical_stream.[ch]`, and generic Pi
`wire_protocol.py` framing/Q4 behavior in isolation on top of checkpoint 01.
It does not qualify logical queues, credit/rendezvous policy, runtime dispatch,
RFB parsing, audio, MPEG, or the full application.

Interpretation:
The isolated physical-stream `select()` readiness mechanism survives a
60-second idle interval and detects subsequent inbound framed data on real PS2
hardware. This does not by itself prove the complete ledge runtime owner/dispatch
architecture, but it rules out a simple inherent long-idle failure of the
physical-stream readiness/framing layer under this qualified workload.
