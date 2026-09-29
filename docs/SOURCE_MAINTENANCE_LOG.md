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
