# PS-to-VNC Core

A clean, staged reconstruction of PS-to-VNC.

The existing Olsens11/PS-to-VNC ledge repository remains source/provenance authority and a parts bin. This repository integrates one bounded subsystem at a time and requires a real PS2 hardware checkpoint before the next subsystem enters the working baseline.

## Current checkpoint

01 - platform + private Ethernet + TCP round trip

Included product modules:
- src/platform/ps2_system.*
- src/platform/ps2_network.*

Everything else is intentionally absent.

Build with ./scripts/build.sh. On the Pi, start ./pi/platform_network_probe.py before launching the checkpoint ELF on the PS2. The checkpoint passes only when the Pi prints CHECKPOINT=PASS.

See docs/INTEGRATION_PLAN.md and docs/TESTED_CHECKPOINTS.md.
