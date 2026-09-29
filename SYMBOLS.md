# Symbols — repository root

DIRECTORY=.
GENERATION=STAGED_RECONSTRUCTION
COVERAGE=CURRENT

The root currently owns only the Make-based PS2 checkpoint build graph. Product
symbols are indexed in their owning source directories.

| Name | Kind | File | Owner | Scope | Description |
|---|---|---|---|---|---|
| `BUILD_DIR` | Make variable | Makefile | checkpoint build | build | Output directory for the platform/network hardware checkpoint. |
| `GEN_DIR` | Make variable | Makefile | embedded IRX generation | build | Holds generated C wrappers for linked IOP module images. |
| `EE_BIN` | Make variable | Makefile | checkpoint link | build | Names the produced PS2 checkpoint ELF. |
| `PS2IP_LIB` | Make variable | Makefile | network dependency | build | Names the qualified frozen PS2IP archive. |
| `EE_OBJS` | Make variable | Makefile | checkpoint link | build | Enumerates objects linked into the checkpoint ELF. |
| `EE_INCS` | Make variable | Makefile | checkpoint compile | build | Adds clean source/platform include roots. |
| `EE_LIBS` | Make variable | Makefile | checkpoint link | build | Adds NETMAN, frozen PS2IP, and patches libraries. |
| `all` | Make target | Makefile | checkpoint build | build | Builds the current hardware checkpoint ELF. |
| `clean` | Make target | Makefile | build hygiene | build | Removes disposable build artifacts. |
