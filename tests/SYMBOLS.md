# Symbols — `tests`

DIRECTORY=tests
GENERATION=STAGED_RECONSTRUCTION
COVERAGE=CURRENT

This directory owns the host-side unit-test build graph for admitted clean
modules. Hardware checkpoint code remains under `tests/hardware`.

| Name | Kind | File | Owner | Scope | Description |
|---|---|---|---|---|---|
| `COMMON_CFLAGS` | Make variable | Makefile | host test build | build | Strict C99 warning/error flags and clean source include root. |
| `BUILD_DIR` | Make variable | Makefile | host artifacts | build | Disposable `.build` output directory. |
| `PROTOCOL_TEST` | Make variable | Makefile | Wire codec test | build | Exact protocol fixture executable. |
| `PHYSICAL_TEST` | Make variable | Makefile | physical-stream test | build | Imported ledge physical-stream fixture executable. |
| `READINESS_TEST` | Make variable | Makefile | readiness test | build | Direct host `select()` readiness fixture executable. |
| `unit` | Make target | Makefile | host test suite | build | Builds/runs C fixtures and the Pi Wire codec test. |
| `clean` | Make target | Makefile | build hygiene | build | Removes disposable host test binaries. |
