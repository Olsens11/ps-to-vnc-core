# Source maintenance log

This is the cumulative audit trail for comment, naming, dictionary, provenance,
and clean-to-ledge backport conventions.

Do not rewrite completed entries when conventions evolve. Add a new entry.

## 2026-09-28 — Checkpoint 01 post-qualification source review

Purpose: establish the repeatable maintenance process immediately after the
first hardware-qualified clean rung, before importing Wire.

Starting Git authority:
`4f7eae37964d06b4bfde5f45cd17b184e6f67a1b`

Hardware authority under review:
- tested source commit: `21590f4c2425c68271254e8f38ce09189cae9c0f`
- tested ELF SHA-256:
  `14d8faf57139482f0561d68fbb50fca8ee63b7aee82b63147f3ccc4987af3b26`
- checkpoint result: `CORE-CP01-STABILITY = PASS`

Files reviewed:
- `src/platform/ps2_system.c`
- `src/platform/ps2_system.h`
- `src/platform/ps2_network.c`
- `src/platform/ps2_network.h`
- `tests/hardware/platform_network/main.c`
- `pi/platform_network_probe.py`
- `Makefile`
- `scripts/build.sh`

Reviewed source SHA-256 values:
- `ps2_system.c` — `6c5b4b80b610feb55deb778dbd576a2d8d69c69c2807c0ecdb3ff56b860ee3e9`
- `ps2_system.h` — `8e87cb28067bf95d15fd8f844751814d2c7097d1df0d3ed265ce5d8f9ebb85c4`
- `ps2_network.c` — `a950b9741c578a5464977d0eff07a2f61266cfd4d57529f837fbf3c753bf4f96`
- `ps2_network.h` — `561eb49102bfbb1d52cd02f8f952523e4a48967638fd28a76a760a39b6047c8c`

- checkpoint `main.c` —
  `280ae081065192fac32ec4f033c0eb427a79bdc31177a9120e6ace44332238b4`
- Pi `platform_network_probe.py` —
  `ff59119398af24888297c22e2d4aedfc637ad2fd57d4a0c0da347abc1d5a6abc`
- root `Makefile` —
  `f4ad78fb412ef2c3c79ae148f9d3787ee48b85aa7f63ac3490fbdc500f06a67b`
- `scripts/build.sh` —
  `fefd9674f45b4b27885a27e2c6498c588954bc919e81c3ae9fd746f128b6cbf1`

Review findings:
- platform ownership and function naming are clear;
- ledge/reconstruction references in imported file synopses are valid provenance
  context and are intentionally retained;
- `pstvnc_ps2_system_delay_us` contains historical RFB motivation in its
  header comment; this is accurate context but not checkpoint-01 ownership;
- no misleading symbol names or behavioral comments require an immediate
  executable-source edit;
- checkpoint/Pi record names correctly describe the hardware discriminator;
- the repository lacked local symbol dictionaries and a repeatable maintenance
  and backport procedure.

Changes made in this pass:
- added concise file synopses to `Makefile` and `scripts/build.sh`;
- added `docs/SOURCE_MAINTENANCE.md`;
- added root `SYMBOLS.md` for the Make build graph;
- added `scripts/SYMBOLS.md`;
- added `src/platform/SYMBOLS.md`;
- added `tests/hardware/platform_network/SYMBOLS.md`;
- added `pi/SYMBOLS.md`;
- updated `README.md` and `docs/INTEGRATION_PLAN.md` to state the clean-to-ledge workflow;
- added this cumulative log.

Change class: DOCUMENTATION + TOOLING-COMMENT-ONLY.

Qualified C/Python source decision:
No qualified C or Python source is changed in this pass.

Tooling comment equivalence:
- `Makefile` before: `f4ad78fb412ef2c3c79ae148f9d3787ee48b85aa7f63ac3490fbdc500f06a67b`
- `Makefile` after: `f3f41f4691ca15ed409198a3d954c9c324d327c2e55f1402b2a2bd788c57cf89`
- `scripts/build.sh` before: `fefd9674f45b4b27885a27e2c6498c588954bc919e81c3ae9fd746f128b6cbf1`
- `scripts/build.sh` after: `623006d3c9a49a89d2b98ceb98c9e253efa8fb976c6916275d0d74472a5a73a6`

A rebuild after those comment-only tooling edits produced the exact previously
qualified ELF SHA-256
`14d8faf57139482f0561d68fbb50fca8ee63b7aee82b63147f3ccc4987af3b26`.
The checkpoint-01 hardware result therefore remains bound to identical executable
output.

Convention established:
Perform comment/naming/symbol cleanup before final hardware qualification for
future rungs whenever practical. After a clean rung proves a defect and fix,
document the behavioral invariant and backport it deliberately to ledge; the
ledge backport receives its own integration validation and commit authority.

## 2026-09-28 — Rung 02 physical Wire import and prequalification review

Purpose: admit only the physical PSTV Wire layer from ledge before any logical
channel/runtime code enters the clean integration line.

Starting clean authority:
`fbdada3155e1a619454bbf25dfee096510212698`

Exact import commit:
`ec81c0848ac787c022cb72a9b277d189ffdf8e1e`

Ledge source authority:
`048d3dfb082b74760c3fc54bf37559fc68c4f038`

Imported product files:
- `src/transport/protocol.c`
- `src/transport/protocol.h`
- `src/transport/physical_stream.c`
- `src/transport/physical_stream.h`
- `pi/wire_protocol.py`

Original ledge SHA-256 identities:
- `protocol.c` — `ed492b26a41ddacf03d5f96869f5c0f877bd4b4f214925cb2ea066a5114d2995`
- `protocol.h` — `99a0407c154d9c167474a7dd164a7785a7108dfeecb4ba0ebdda5ce2044a5dbf`
- `physical_stream.c` — `39f4c550e952ca26690066ef52583520c3c7c95f79d759aab9f00a91999de57c`
- `physical_stream.h` — `98af3274fbb4c556a50cd74828a6e708a65cb26c1bb8ee853ed064df02b8f596`
- `pi/wire_protocol.py` — `bcc6584dade91bd9d98d50d91ed09c2c080ad4c5980ebe7f2aba2ded23f0f0c8`

Imported direct ledge fixtures and stubs are frozen in
`provenance/RUNG02_TEST_IMPORTS.sha256`.

Prequalification review findings:
- product ownership boundaries and names are clear enough to retain;
- the `physical_stream.h` receive-frame comment was visually attached to the
  readability declaration because two comment blocks were stacked before it;
- this was corrected as comment-only maintenance before hardware qualification;
- maintained `physical_stream.h` SHA-256 is
  `90e02e289caa4d492e8668975e3e4784f5e5b0a170cbe0e033dd63e88dfb1ac8`;
- ledge's direct physical-stream fixture had no coverage for
  `pstvnc_transport_physical_stream_wait_readable()`;
- a clean host socketpair fixture was added specifically for timeout/readable/
  invalid-stream readiness results;
- the imported protocol fixture lacked a file synopsis; one was added without
  changing test behavior.

Qualification-scope warning:
The imported protocol modules also contain RFB provider-failure and MPEG
generation-control codecs, and the Pi codec contains RFB/audio/MPEG helpers.
Those symbols are present because they share the ledge framing module; rung 02
does not claim their rider semantics as hardware-qualified. Rung 02 covers only
fixed framing, Q4 establishment, physical sequence/ownership, generic framed
I/O, receive readiness, and long-idle framed wake.

Provenance convention established:
`provenance/LEDGE_IMPORTS.sha256` is immutable original-import evidence.
`provenance/CURRENT_SOURCE.sha256` is the build-time current-source guard and
may change only through an intentional logged maintenance/fix commit.

### Rung 02 hardware qualification result

Hardware apparatus/source authority:
`262ad00dcb0989abcdbd2701b17b5a579ad51c09`

Exact candidate:
- ELF SHA-256:
  `c2b59600e829bd3f9b0333ba16420a0328739e240695677de66a737cfd1258eb`
- ELF bytes: `2300344`
- PT_LOAD SHA-256:
  `e8ab4f13c0fe99f55fd2c4524c0cf5cd9d783e5ebdf8eabe145eaaa5f34601ed`
- PT_LOAD bytes: `363784`
- two consecutive pinned builds produced the exact same ELF SHA-256;
- FTP archival and rolling-target readback both matched the candidate.

Observed hardware sequence:
- Q4 HELLO/ACCEPT: PASS;
- phase 1: 128/128 framed bidirectional rounds PASS;
- payload set included zero length and the 8192-byte Wire maximum;
- Pi-to-PS2 frames were deliberately fragmented across multiple socket sends;
- deliberate idle began at 2026-09-28T21:34:22.372255-04:00;
- framed wake passed at 2026-09-28T21:35:22.381226-04:00;
- PS2 reported 57,513 empty select-based readiness polls before wake;
- phase 2: 128/128 framed bidirectional rounds PASS;
- final framed handshake: PASS;
- checkpoint: PASS.

Engineering conclusion:
The isolated imported physical-stream readiness/framing layer does not reproduce
the ledge HW1 long-idle stall. Its real `select()` path survived approximately
60 seconds of repeated empty polls and then consumed fragmented inbound Wire
data. Therefore any later reproduction must come from an interaction introduced
above this layer, or from a workload/ownership condition not present in this
bounded checkpoint.

No ledge backport is required from this rung yet because no product behavioral
fix was made. The only product-source divergence is the prequalification
comment-placement correction in `physical_stream.h`; original import identity
remains preserved separately.

## 2026-09-28 — Rung 03A logical RFB channel import/prequalification

Purpose: admit only ledge's backend-independent logical RFB byte-storage module
on top of the hardware-qualified physical Wire baseline. Credit policy,
semaphore rendezvous, receiver/runtime ownership, and the RFB parser remain
absent.

Starting clean authority:
`3e54ecbd22c571113793626bc528e761f0135cfb`

Exact import commit:
`9f4a2fcbe5b5d057964263d8fb737158bc5f4b26`

Ledge source authority:
`048d3dfb082b74760c3fc54bf37559fc68c4f038`

Imported identities:
- `src/transport/rfb_channel.c` —
  `cbefd57ec80335316550c05ea4d60622b32305f9a8e9050503dfd19add049bff`
- `src/transport/rfb_channel.h` —
  `63516a42c009175e30d8bf92744d596f38d670669e8ec3bccc0f4b935d924da9`
- `tests/unit/transport_rfb_channel_test.c` —
  `c56b1dc2f30f767a40bff697495b971f2540098cf76be5d08eb422e3b4f81ae8`

Prequalification review:
- module ownership is narrow and clear;
- no product-source comment or naming correction was required;
- existing host fixture covers capacity rejection, wraparound, incremental and
  exact consumption, producer activity, and residual discard;
- full clean host gate passes with the imported module unchanged;
- build-apparatus wiring was normalized after an initial missing Make target;
  no product behavior was involved.

Provenance-process clarification:
The immutable import manifest records historical ledge bytes and can legitimately
differ from the maintained live tree after a logged edit. New imports are proved
against the exact ledge checkout; the current-source manifest guards live build
inputs.

### Rung 03A hardware qualification result

Hardware apparatus/source authority:
`f0d2bfbc8df8716604087eb0a3fe7e61e32fb348`

Exact candidate:
- ELF SHA-256:
  `eeec1395953aa554c4eea775a732983212cefd245a4230b82d601c82cc1ef966`
- ELF bytes: `2312488`
- PT_LOAD SHA-256:
  `006c19d0992b166a8d1f92957d5a57f1242bd01bb16c3a2b47fcffa16f4e0c9f`
- PT_LOAD bytes: `366088`
- two consecutive pinned builds produced the exact same ELF SHA-256;
- FTP archival and rolling-target readback both matched the candidate.

Observed hardware sequence:
- Q4 establishment: PASS;
- pre-idle logical queue sequence: PASS;
- 1024-byte circular queue wraparound: PASS;
- partial and exact reads across committed-frame boundaries: PASS;
- zero-length commit left activity generation unchanged: PASS;
- failed short exact read preserved queue and caller buffer: PASS;
- exact residual discard semantics: PASS;
- deliberate idle began at 2026-09-28T22:29:55.044409-04:00;
- post-idle RFB DATA commit/read passed at
  2026-09-28T22:30:55.054603-04:00;
- PS2 reported 57,512 empty physical-readiness polls before wake;
- final activity generation: 6;
- final available queue bytes: 0;
- final framed handshake: PASS;
- checkpoint: PASS.

Engineering conclusion:
The imported logical RFB byte-storage primitive does not reproduce the HW1
long-idle failure. It remains correct across circular wrap, mixed consumption,
residual discard, and a post-idle inbound commit/read on real PS2 hardware.
The next bounded target is therefore the RFB credit/activity/rendezvous ownership
above this storage primitive, still without the RFB parser/display/application.

No ledge backport is required from 03A because no product behavioral fix was
made.

## 2026-09-28 — Rung 03B RFB activity-rendezvous extraction

Purpose: isolate the first concurrent logical-RFB mechanism above the qualified
03A byte channel without importing ledge's mixed ~3000-line Transport runtime.

Starting clean authority:
`f337ff1e16e79b8559dfb859ce579401b12fbb86`

Extraction source authority:
`Olsens11/PS-to-VNC@048d3dfb082b74760c3fc54bf37559fc68c4f038`,
`src/transport/runtime.c`.

New product module:
- `src/transport/rfb_flow.c` —
  `cecf562dfed5cb81b0e0ba518bd05a3e52c4863173e70375ee24a131279f0f1c`
- `src/transport/rfb_flow.h` —
  `a17266cd997c73ebb1048f4355557599f6c9e8176087a0faa8f37b18fa788b24`

Exact behavioral source map and intentional scope reductions are recorded in
`provenance/RUNG03B_ACTIVITY_EXTRACTION.md`.

The extraction preserves the ledge activity invariant:
- activity sequence and waiter armed-state share the queue lock;
- a publisher clears the armed-state while still holding that lock;
- the activity semaphore is signaled only after the queue lock is released;
- a returning waiter reacquires the same lock and rejects a wake if the armed
  state was not cleared by the publisher.

Deferred from this rung:
- RFB inbound/outbound credit;
- provider terminal state;
- stop/receiver lifecycle;
- synchronous outbound slot ownership;
- RFB parser/display/application.

Host qualification:
- complete existing clean host suite remains PASS;
- new `transport_rfb_flow_test` proves a genuinely blocked pthread waiter wakes;
- activity published before `wait_activity()` is observed without blocking;
- the first fixture revision incorrectly observed an unsynchronized struct field
  from another pthread and could hang; the fixture was corrected to observe
  waiter entry through a pthread condition inside the semaphore model. No product
  source change resulted from that fixture bug.

03B hardware-apparatus preflight note:
The first target build of `tests/hardware/rfb_activity/main.c` redundantly
redeclared PS2SDK's `_gp` symbol with the wrong type. The target header already
owns that declaration, so the redundant test-only declaration was removed. The
full host suite and target build then passed. No product-source behavior changed.
