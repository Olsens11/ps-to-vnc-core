# Integration plan

The project grows only from a hardware-qualified baseline. A later module does not get to hide a defect in an earlier one.

## Rules

1. Import ledge product files directly when suitable.
2. Record the exact ledge commit and imported-file hashes.
3. Keep checkpoint harness code separate from product modules.
4. Host tests help, but a rung is qualified only on the real PS2/Pi hardware.
5. Record the Git commit and exact ELF SHA-256 before adding the next module.
6. When a rung fails, change only the smallest responsible layer.
7. Do not import process machinery merely because it existed in ledge.

## Planned rungs

01 Platform/network - IOP bootstrap, Ethernet, TCP round trip. Current.
02 Physical PSTV Wire - protocol + physical stream; repeat active -> long idle -> one inbound frame -> active.
03 Logical RFB transport - queue, credit, rendezvous, long-idle synthetic traffic.
04 RFB + framebuffer - controlled Raw updates first, then required encodings.
05 Display - stable visible desktop at the selected working mode.
06 Input - controller, pointer, buttons, keyboard, OSK incrementally.
07 Configuration/local UI - bindings, modes, calibration, recovery actions.
08 Audio - PCM transport/playback.
09 MPEG - transport, decoder, presentation, calibration.
10 Full application - coordinator plus combined endurance/recovery qualification.

The order may change when hardware evidence justifies it, but qualification never skips a dependency layer.
