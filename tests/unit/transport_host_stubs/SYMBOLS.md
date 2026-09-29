# Symbols — `tests/unit/transport_host_stubs`

DIRECTORY=tests/unit/transport_host_stubs
GENERATION=STAGED_RECONSTRUCTION
COVERAGE=CURRENT

These headers are host-only compile seams for PS2SDK APIs required by the real
physical-stream implementation.

| Name | Kind | File | Description |
|---|---|---|---|
| `ee_sema_t` | structure | kernel.h | Minimal semaphore descriptor shape used by product code. |
| `ee_thread_t` / `ee_thread_status_t` | structure | kernel.h | Minimal thread declarations retained from ledge fixtures. |
| semaphore/thread function declarations | API seam | kernel.h | Declarations implemented deterministically by individual fixtures. |
| `DIntr` / `EIntr` | inline function | kernel.h | Host mutex model of short EE interrupt-disabled critical sections. |
| PS2IP include guard | placeholder | ps2ip.h | Allows product source to include PS2SDK `ps2ip.h` during host builds. |
