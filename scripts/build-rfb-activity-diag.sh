#!/usr/bin/env bash
# Build hardware checkpoint 03B in the pinned PS2DEV image.
# Verifies current admitted product/dependency bytes before compiling the real
# EE-thread RFB activity-rendezvous discriminator.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="ps2dev/ps2dev@sha256:8fba50ecc2229acd7f8da63d34302f12939b7d4fa6848dda1e6a0ce083321a11"
BUILD_DIR="$ROOT/build/hardware/rfb-activity-diag"
ELF="$BUILD_DIR/PS-to-VNC-rfb-activity-diag.ELF"

cd "$ROOT"
sha256sum -c provenance/CURRENT_SOURCE.sha256
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR/generated"

docker run --rm     --entrypoint /bin/sh     -e HOST_UID="$(id -u)"     -e HOST_GID="$(id -g)"     -v "$ROOT:/repo"     -w /repo     "$IMAGE"     -lc '
set -eu
export PATH=/usr/local/ps2dev/bin:/usr/local/ps2dev/ee/bin:/usr/local/ps2dev/iop/bin:/usr/local/ps2dev/dvp/bin:/usr/local/ps2dev/ps2sdk/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
CC=mips64r5900el-ps2-elf-gcc
CFLAGS="-D_EE -G0 -O2 -Wall -gdwarf-2 -gz"
INCS="-I/usr/local/ps2dev/ps2sdk/ee/include -I/usr/local/ps2dev/ps2sdk/common/include -I. -Isrc -Isrc/platform -Isrc/transport"
B=build/hardware/rfb-activity-diag
G=$B/generated

bin2c /usr/local/ps2dev/ps2sdk/iop/irx/freesio2.irx $G/SIO2MAN_irx.c SIO2MAN_irx
bin2c /usr/local/ps2dev/ps2sdk/iop/irx/freepad.irx $G/PADMAN_irx.c PADMAN_irx
bin2c /usr/local/ps2dev/ps2sdk/iop/irx/ps2dev9.irx $G/DEV9_irx.c DEV9_irx
bin2c /usr/local/ps2dev/ps2sdk/iop/irx/netman.irx $G/NETMAN_irx.c NETMAN_irx
bin2c /usr/local/ps2dev/ps2sdk/iop/irx/smap.irx $G/SMAP_irx.c SMAP_irx

$CC $CFLAGS $INCS -c tests/hardware/rfb_activity_diag/main.c -o $B/checkpoint_main.o
$CC $CFLAGS $INCS -c src/platform/ps2_system.c -o $B/ps2_system.o
$CC $CFLAGS $INCS -c src/platform/ps2_network.c -o $B/ps2_network.o
$CC $CFLAGS $INCS -c src/transport/protocol.c -o $B/transport_protocol.o
$CC $CFLAGS $INCS -c src/transport/physical_stream.c -o $B/transport_physical_stream.o
$CC $CFLAGS $INCS -c src/transport/rfb_channel.c -o $B/transport_rfb_channel.o
$CC $CFLAGS $INCS -c src/transport/rfb_flow.c -o $B/transport_rfb_flow.o

$CC $CFLAGS $INCS -c $G/SIO2MAN_irx.c -o $B/SIO2MAN_irx.o
$CC $CFLAGS $INCS -c $G/PADMAN_irx.c -o $B/PADMAN_irx.o
$CC $CFLAGS $INCS -c $G/DEV9_irx.c -o $B/DEV9_irx.o
$CC $CFLAGS $INCS -c $G/NETMAN_irx.c -o $B/NETMAN_irx.o
$CC $CFLAGS $INCS -c $G/SMAP_irx.c -o $B/SMAP_irx.o

$CC -T/usr/local/ps2dev/ps2sdk/ee/startup/linkfile -O2     -o $B/PS-to-VNC-rfb-activity-diag.ELF     $B/checkpoint_main.o     $B/ps2_system.o     $B/ps2_network.o     $B/transport_protocol.o     $B/transport_physical_stream.o     $B/transport_rfb_channel.o     $B/transport_rfb_flow.o     $B/SIO2MAN_irx.o     $B/PADMAN_irx.o     $B/DEV9_irx.o     $B/NETMAN_irx.o     $B/SMAP_irx.o     -L/usr/local/ps2dev/ps2sdk/ee/lib     -Wl,-zmax-page-size=128     -lnetman vendor/ps2ip/libps2ip_mtu1458_wscale128.a -lpatches

chown -R "$HOST_UID:$HOST_GID" build
'

test -f "$ELF"
echo "BUILD=PASS"
echo "CHECKPOINT=rfb-activity-diag"
echo "ELF=$ELF"
sha256sum "$ELF"
