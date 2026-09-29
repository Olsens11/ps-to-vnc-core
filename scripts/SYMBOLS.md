# Symbols — `scripts`

DIRECTORY=scripts
GENERATION=STAGED_RECONSTRUCTION
COVERAGE=CURRENT

This directory currently owns the pinned, provenance-guarded checkpoint build
entry point.

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
