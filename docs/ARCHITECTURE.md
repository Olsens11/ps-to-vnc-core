# Architecture

The repository intentionally begins smaller than the final product.

src/ contains only product modules that have entered the staged integration line.
tests/hardware/ contains temporary hardware checkpoint executables.
pi/ contains the matching Raspberry Pi test peer.
provenance/ records where imported code came from.
vendor/ contains only dependencies whose exact identity is part of qualification.

The design target is small, explicit modules with one clear owner per mechanism.
Complexity is earned by passing the preceding hardware checkpoint, not by reconstructing the old monolith all at once.
