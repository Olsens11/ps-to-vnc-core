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
