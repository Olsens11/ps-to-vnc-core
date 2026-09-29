# PS-to-VNC Core

A clean, staged reconstruction and qualification workspace for PS-to-VNC.

The existing `Olsens11/PS-to-VNC` ledge repository remains the product source,
historical authority, and parts bin. This repository takes one bounded ledge
module set at a time, makes it understandable and testable in isolation,
qualifies it on real PS2/Pi hardware, and documents any proven fixes for
deliberate backport into ledge.

## Current baseline

Checkpoint 02 - physical PSTV Wire: HARDWARE QUALIFIED

Included product modules:
- `src/platform/ps2_system.*`
- `src/platform/ps2_network.*`
- `src/transport/protocol.*`
- `src/transport/physical_stream.*`
- `pi/wire_protocol.py`

Logical transport queues/runtime, RFB, display, input, audio, and MPEG remain
intentionally absent.

The qualified baseline proves platform/network duplex TCP plus exact Wire Q4,
framed traffic through the 8192-byte payload ceiling, deliberately fragmented
Pi-to-PS2 frames, a 60-second select-based idle/readiness interval, framed wake,
and resumed traffic.

Build with `./scripts/build.sh`. Hardware evidence and exact authorities are
recorded in `docs/TESTED_CHECKPOINTS.md`.

## Working method

For each rung:
1. import the smallest coherent ledge module set with exact provenance;
2. review ownership, comments, naming, and directory symbols;
3. reproduce/fix only defects exposed at that layer;
4. qualify the exact result on hardware;
5. document the fix and behavioral invariant;
6. backport the proven correction into ledge and validate it there.

See:
- `docs/INTEGRATION_PLAN.md`
- `docs/SOURCE_MAINTENANCE.md`
- `docs/SOURCE_MAINTENANCE_LOG.md`
- `docs/TESTED_CHECKPOINTS.md`
