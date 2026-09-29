# Symbols — `scripts`

DIRECTORY=scripts
GENERATION=STAGED_RECONSTRUCTION
COVERAGE=CURRENT

This directory owns pinned, current-source-guarded PS2 checkpoint build entry
points. `build.sh` reproduces checkpoint 01; `build-wire-physical.sh` builds the
rung-02 physical Wire discriminator.

| Name | Kind | File | Owner | Scope | Description |
|---|---|---|---|---|---|
| `ROOT` | shell variable | build.sh | build entry point | script | Resolves the clean repository root independent of caller working directory. |
| `IMAGE` | shell variable | build.sh | toolchain authority | script | Pins the exact PS2DEV container image digest used for checkpoint builds. |
| `BUILD_DIR` | shell variable | build.sh | build artifacts | script | Names the disposable platform/network output directory. |
| `ELF` | shell variable | build.sh | build result | script | Names the exact checkpoint ELF expected after the container build. |
| `CC` | shell variable | build.sh | container compile | inner shell | Names the EE compiler inside the pinned image. |
| `CFLAGS` | shell variable | build.sh | container compile | inner shell | Holds the qualified EE optimization/warning/debug flags. |
| `INCS` | shell variable | build.sh | container compile | inner shell | Holds PS2SDK and clean source include paths. |
| `B` / `G` | shell variable | build.sh | container build | inner shell | Short names for build and generated-IRX directories. |
| `ROOT` | shell variable | build-wire-physical.sh | build entry point | script | Resolves the clean repository root. |
| `IMAGE` | shell variable | build-wire-physical.sh | toolchain authority | script | Pins the same exact PS2DEV image digest used by checkpoint 01. |
| `BUILD_DIR` / `ELF` | shell variable family | build-wire-physical.sh | checkpoint 02 artifacts | script | Name the wire-physical output directory and ELF. |
| `CC` / `CFLAGS` / `INCS` | shell variable family | build-wire-physical.sh | container compile | inner shell | Define compiler, flags, and platform/transport include roots. |
| `B` / `G` | shell variable family | build-wire-physical.sh | container build | inner shell | Short names for output and generated-IRX directories. |
| `BUILD_DIR` / `ELF` | shell variable family | build-rfb-channel.sh | checkpoint 03A artifacts | script | Name the logical-RFB-channel output directory and ELF. |
| `CC` / `CFLAGS` / `INCS` | shell variable family | build-rfb-channel.sh | container compile | inner shell | Define pinned EE compilation for platform + physical Wire + RFB channel. |
| `BUILD_DIR` / `ELF` | shell variable family | build-rfb-activity.sh | checkpoint 03B artifacts | script | Name the synchronized-RFB-activity output directory and ELF. |
| `CC` / `CFLAGS` / `INCS` | shell variable family | build-rfb-activity.sh | container compile | inner shell | Define pinned EE compilation for platform + Wire + logical queue + activity extraction. |
