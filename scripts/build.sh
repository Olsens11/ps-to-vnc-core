#!/usr/bin/env bash
# Build the current clean hardware checkpoint in the pinned PS2DEV image.
# Verifies imported ledge file/dependency hashes before compiling so a build
# cannot silently drift away from the recorded reconstruction provenance.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="ps2dev/ps2dev@sha256:8fba50ecc2229acd7f8da63d34302f12939b7d4fa6848dda1e6a0ce083321a11"
BUILD_DIR="$ROOT/build/hardware/platform-network"
ELF="$BUILD_DIR/PS-to-VNC-platform-network.ELF"

cd "$ROOT"
sha256sum -c provenance/CURRENT_SOURCE.sha256
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR/generated"

docker run --rm \
    --entrypoint /bin/sh \
    -e HOST_UID="$(id -u)" \
    -e HOST_GID="$(id -g)" \
    -v "$ROOT:/repo" \
    -w /repo \
    "$IMAGE" \
    -lc '
set -eu
export PATH=/usr/local/ps2dev/bin:/usr/local/ps2dev/ee/bin:/usr/local/ps2dev/iop/bin:/usr/local/ps2dev/dvp/bin:/usr/local/ps2dev/ps2sdk/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
CC=mips64r5900el-ps2-elf-gcc
CFLAGS="-D_EE -G0 -O2 -Wall -gdwarf-2 -gz"
INCS="-I/usr/local/ps2dev/ps2sdk/ee/include -I/usr/local/ps2dev/ps2sdk/common/include -I. -Isrc -Isrc/platform"
B=build/hardware/platform-network
G=$B/generated

bin2c /usr/local/ps2dev/ps2sdk/iop/irx/freesio2.irx $G/SIO2MAN_irx.c SIO2MAN_irx
bin2c /usr/local/ps2dev/ps2sdk/iop/irx/freepad.irx $G/PADMAN_irx.c PADMAN_irx
bin2c /usr/local/ps2dev/ps2sdk/iop/irx/ps2dev9.irx $G/DEV9_irx.c DEV9_irx
bin2c /usr/local/ps2dev/ps2sdk/iop/irx/netman.irx $G/NETMAN_irx.c NETMAN_irx
bin2c /usr/local/ps2dev/ps2sdk/iop/irx/smap.irx $G/SMAP_irx.c SMAP_irx

$CC $CFLAGS $INCS -c tests/hardware/platform_network/main.c -o $B/checkpoint_main.o
$CC $CFLAGS $INCS -c src/platform/ps2_system.c -o $B/ps2_system.o
$CC $CFLAGS $INCS -c src/platform/ps2_network.c -o $B/ps2_network.o
$CC $CFLAGS $INCS -c $G/SIO2MAN_irx.c -o $B/SIO2MAN_irx.o
$CC $CFLAGS $INCS -c $G/PADMAN_irx.c -o $B/PADMAN_irx.o
$CC $CFLAGS $INCS -c $G/DEV9_irx.c -o $B/DEV9_irx.o
$CC $CFLAGS $INCS -c $G/NETMAN_irx.c -o $B/NETMAN_irx.o
$CC $CFLAGS $INCS -c $G/SMAP_irx.c -o $B/SMAP_irx.o

$CC -T/usr/local/ps2dev/ps2sdk/ee/startup/linkfile -O2 \
    -o $B/PS-to-VNC-platform-network.ELF \
    $B/checkpoint_main.o \
    $B/ps2_system.o \
    $B/ps2_network.o \
    $B/SIO2MAN_irx.o \
    $B/PADMAN_irx.o \
    $B/DEV9_irx.o \
    $B/NETMAN_irx.o \
    $B/SMAP_irx.o \
    -L/usr/local/ps2dev/ps2sdk/ee/lib \
    -Wl,-zmax-page-size=128 \
    -lnetman vendor/ps2ip/libps2ip_mtu1458_wscale128.a -lpatches

chown -R "$HOST_UID:$HOST_GID" build
'

test -f "$ELF"
echo "BUILD=PASS"
echo "CHECKPOINT=platform-network"
echo "ELF=$ELF"
sha256sum "$ELF"
