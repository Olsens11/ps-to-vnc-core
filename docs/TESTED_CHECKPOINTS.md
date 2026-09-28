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
