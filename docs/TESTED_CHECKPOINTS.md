# Tested checkpoints

This file records actual qualification, not plans.

## 01 - Platform and network

Status: NOT YET HARDWARE QUALIFIED

Required proof:
- pinned-toolchain build succeeds;
- deterministic IOP/platform initialization succeeds;
- Ethernet link reaches UP;
- PS2 connects to 192.168.50.1:5959;
- Pi receives PSTVNC_CORE_PLATFORM_NETWORK_HELLO;
- PS2 receives the Pi ACK;
- Pi receives PSTVNC_CORE_PLATFORM_NETWORK_PASS;
- exact Git commit and ELF SHA-256 are recorded here.

Until all items are observed on hardware, checkpoint 01 is not a baseline.

### Attempt 01

Result: FAIL / HARNESS-SHUTDOWN-AMBIGUOUS

Authority:
- Git commit: c4c0d41c149eeda4b2028b614e8c90ab91d1014a
- ELF SHA-256: 1c395616eb1752ff0a0367c3f22b685a2b933498771fed4a6f7f84e33b1f61e6

Observed:
- Pi accepted TCP from 192.168.50.2.
- Pi received the exact HELLO.
- Pi sent the expected ACK.
- Pi did not receive the final PASS before its 10-second timeout.
- PS2 returned to FreeMcBoot.

The attempt does not distinguish failure to consume the ACK from successful ACK
consumption followed by loss of the final PASS during immediate product exit.
Checkpoint 01 therefore remains unqualified. Attempt 02 adds a final Pi-to-PS2
DONE handshake so the test cannot race shutdown after PASS.
