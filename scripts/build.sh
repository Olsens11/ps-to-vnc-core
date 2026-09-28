#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="ps2dev/ps2dev@sha256:8fba50ecc2229acd7f8da63d34302f12939b7d4fa6848dda1e6a0ce083321a11"
TOOLCHAIN_PATH="/usr/local/ps2dev/bin:/usr/local/ps2dev/ee/bin:/usr/local/ps2dev/iop/bin:/usr/local/ps2dev/dvp/bin:/usr/local/ps2dev/ps2sdk/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
ELF="$ROOT/build/hardware/platform-network/PS-to-VNC-platform-network.ELF"

cd "$ROOT"
sha256sum -c provenance/LEDGE_IMPORTS.sha256
rm -rf build/hardware/platform-network

docker run --rm \
    --entrypoint /bin/sh \
    -e HOST_UID="$(id -u)" \
    -e HOST_GID="$(id -g)" \
    -v "$ROOT:/repo" \
    -w /repo \
    "$IMAGE" \
    -lc "set -eu; apk add --no-cache build-base >/dev/null; export PATH='$TOOLCHAIN_PATH'; make; chown -R \"\$HOST_UID:\$HOST_GID\" build"

test -f "$ELF"
echo "BUILD=PASS"
echo "CHECKPOINT=platform-network"
echo "ELF=$ELF"
sha256sum "$ELF"
