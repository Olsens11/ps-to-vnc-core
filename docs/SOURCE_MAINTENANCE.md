# Source maintenance process

This document defines how PS-to-VNC Core keeps source comments, naming,
symbol dictionaries, provenance, and hardware qualification consistent as the
staged integration line grows.

The goal is not cosmetic uniformity. The goal is that a future reader can tell
what a file owns, where it came from, what changed, and what hardware evidence
still applies.

## 1. Authority and provenance

The clean repository is the integration authority for code that has entered it.
The ledge repository remains valid historical/provenance context and a source
parts bin.

Comments may reference ledge documents, reconstruction packets, issue numbers,
or historical experiments when those references explain why code exists or
where a behavior came from. Such references do not transfer current ownership
back to ledge.

Every imported product file must retain a recorded source authority and
cryptographic identity. A later cleanup must never erase that provenance.

`provenance/LEDGE_IMPORTS.sha256` records immutable original import identities.
New imports are appended, but existing entries are never rewritten to match
later repairs. Once any imported file intentionally diverges, do not run the
whole original-import manifest against the live worktree; verify a new import
directly against the recorded ledge source checkout/commit instead.
`provenance/CURRENT_SOURCE.sha256` records the expected current product/dependency
bytes and is the manifest build tooling verifies. Intentional maintenance or a
proven fix updates the current manifest and is recorded in the maintenance log;
the original import manifest remains unchanged.

## 2. File-level documentation

Every maintained source file should begin with a short synopsis that answers:
- what the file owns;
- what it deliberately does not own when that boundary matters;
- any important historical/provenance context needed to understand the design.

Comments should explain ownership, invariants, hardware constraints, ordering,
failure policy, or non-obvious reasons. They should not narrate obvious syntax.

Names should describe the mechanism or domain meaning directly. Avoid generic
names such as `manager`, `helper`, or `handler` when a narrower name is
available.

Do not mix broad naming/comment cleanup with a behavioral repair unless the
cleanup is required to make that repair understandable.

## 3. Symbol dictionaries

Each source-bearing directory gets a `SYMBOLS.md` once it enters the staged
line.

The dictionary records project-defined symbols that help a reader understand
the directory:
- public functions and declarations;
- file-static functions with meaningful responsibility;
- project constants and macros;
- project structures, enums, and typedefs;
- persistent file/global state;
- linker seams such as embedded IRX images;
- build/test entry points when they are part of that directory's contract.

Ordinary local variables and routine parameters are omitted unless they carry
an unusual ownership or protocol meaning. The dictionary is an architectural
index, not a compiler symbol dump.

Each entry states name, kind, file, owner, scope, description, and context.

## 4. When the documentation pass happens

For a new integration rung, perform the comment/naming/symbol review before its
final hardware qualification whenever practical.

Sequence:
1. import or implement the smallest bounded module set;
2. establish provenance;
3. review file synopses, ownership comments, and names;
4. create/update directory `SYMBOLS.md`;
5. build and run non-hardware checks;
6. hardware-qualify the resulting exact source and ELF;
7. seal the evidence in `docs/TESTED_CHECKPOINTS.md`.

This avoids changing executable source merely for documentation after it has
already become a hardware baseline.

## 5. Changes after hardware qualification

A qualified rung is never silently rewritten.

Documentation-only changes outside executable source
(`*.md`, dictionaries, logs) do not invalidate the recorded ELF. They must
still be committed and logged.

A source-file comment-only change is recorded as source maintenance. Rebuild
the checkpoint and compare the new artifact with the qualified authority.
Because debug information can change even when executable instructions do not,
a changed whole-ELF hash must not be called the exact previously qualified ELF.

If executable/PT_LOAD identity is unchanged, the old hardware result may be
cited as behavioral equivalence with both old and new identities recorded.
If executable identity changes, or equivalence cannot be proved, hardware
qualification must be repeated before the changed source becomes the baseline.

Renames, control-flow changes, constants, ownership changes, protocol changes,
or any other semantic source edit require the normal build/test/hardware gate
for the affected rung.

## 6. Maintenance log

Every deliberate maintenance pass is appended to
`docs/SOURCE_MAINTENANCE_LOG.md`.

Record:
- date and purpose;
- starting Git authority;
- files reviewed;
- files changed;
- whether changes are documentation-only, comment-only source, or semantic;
- relevant pre/post hashes when executable source changes;
- build/test result;
- qualification impact;
- any convention established for future files.

The log is cumulative. Do not rewrite old entries to match later conventions.

## 7. Relationship to ledge

PS-to-VNC Core is a controlled reconstruction and qualification workspace for
the existing ledge product. It is not intended to fork the product permanently.

For each rung:
1. select the smallest coherent ledge module set;
2. import it with exact provenance;
3. make ownership, comments, naming, and symbols understandable in isolation;
4. reproduce or expose any defect without unrelated subsystems present;
5. make the smallest justified fix in the clean rung;
6. qualify the corrected behavior on real PS2/Pi hardware;
7. document exactly what changed and why;
8. carry the proven fix back to the corresponding ledge module;
9. record the ledge backport commit separately from the clean-rung proof.

A clean-rung result proves only the module combination actually tested. A ledge
backport still needs the appropriate ledge integration validation because the
full product has additional interactions that are absent from the clean rung.

When clean and ledge implementations differ structurally, port the proven
behavior/invariant rather than blindly copying a patch.
