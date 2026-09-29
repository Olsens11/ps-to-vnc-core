# Integration plan

The project grows only from a hardware-qualified baseline. A later module does
not get to hide a defect in an earlier one.

PS-to-VNC Core is a reconstruction/qualification workspace for ledge. Proven
fixes are documented here, then deliberately backported into the original
repository and validated in that larger integration context.

## Rules

1. Import ledge product files directly when suitable.
2. Record the exact ledge commit and imported-file hashes.
3. Keep checkpoint harness code separate from product modules.
4. Review comments, naming, ownership, and directory symbols before final
   hardware qualification whenever practical.
5. Host tests help, but a rung is qualified only on the real PS2/Pi hardware.
6. Record the Git commit and exact ELF SHA-256 before adding the next module.
7. When a rung fails, change only the smallest responsible layer.
8. Document the proven defect, fix, and invariant before backporting to ledge.
9. Validate the backport in ledge; clean-rung proof does not automatically prove
   the full combined product.
10. Do not import process machinery merely because it existed in ledge.

## Planned rungs

01 Platform/network - IOP bootstrap, Ethernet, duplex TCP, long-idle receive.
   Status: HARDWARE QUALIFIED.
02 Physical PSTV Wire - protocol + physical stream; active -> long idle ->
   one inbound frame -> active. Status: HARDWARE QUALIFIED.
03 Logical RFB transport - queue, credit, rendezvous, long-idle synthetic traffic.
   Status: NEXT.
04 RFB + framebuffer - controlled Raw updates first, then required encodings.
05 Display - stable visible desktop at the selected working mode.
06 Input - controller, pointer, buttons, keyboard, OSK incrementally.
07 Configuration/local UI - bindings, modes, calibration, recovery actions.
08 Audio - PCM transport/playback.
09 MPEG - transport, decoder, presentation, calibration.
10 Full application - coordinator plus combined endurance/recovery qualification.

The order may change when hardware evidence justifies it, but qualification
never skips a dependency layer.
