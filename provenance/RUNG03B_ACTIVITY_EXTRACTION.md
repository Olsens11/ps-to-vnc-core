# Rung 03B activity-rendezvous extraction

Source authority:
- repository: Olsens11/PS-to-VNC
- branch: ledge/h1-all-guns
- commit: 048d3dfb082b74760c3fc54bf37559fc68c4f038
- source file: src/transport/runtime.c

The clean `src/transport/rfb_flow.[ch]` module is a structural extraction, not a
byte-for-byte ledge file import. It isolates only the RFB queue lock and the
single-waiter activity sequence/rendezvous used by the ledge runtime.

Behavioral source map:
- semaphore construction pattern: runtime.c `pstvnc_transport_runtime_create_semaphore`
- activity publication: runtime.c `pstvnc_transport_runtime_publish_rfb_activity_locked`
- activity signal: runtime.c `pstvnc_transport_runtime_signal_rfb_activity`
- activity snapshot: runtime.c `pstvnc_transport_runtime_rfb_activity_snapshot`
- activity wait/arm/wake validation: runtime.c `pstvnc_transport_runtime_rfb_wait_activity`
- queue storage remains the exact imported `rfb_channel.[ch]` module.

Intentional scope reduction:
- provider-terminal state is deferred;
- stop/receiver lifecycle is deferred;
- RFB credit return and outbound credit are deferred;
- physical I/O ownership is supplied by the hardware checkpoint, not this module;
- zero-length quiesce markers are deferred and are not admitted as ordinary DATA.

If this extraction exposes a behavioral defect, backport the proven invariant to
the corresponding ledge runtime logic. Do not assume this file layout must be
copied into ledge unless a later integration decision explicitly chooses that
refactor.
