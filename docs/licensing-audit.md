# Licensing and provenance audit

## Decision

GameHQ core version 0.7.1 and later is distributed under
`GPL-3.0-only`. The Playnite integration and the public integration protocol
remain separately licensed under MIT. Earlier public GameHQ revisions retain
their existing MIT grants.

The maintainer controls the first-party GameHQ core material in this repository
and authorized the transition. No contributor license agreement, copyright
assignment, proprietary dual-license grant, or history rewrite is used.

## Exact license texts

The repository root `LICENSE` is the unmodified GNU GPL version 3 text. Its
SHA-256 is enforced by `packaging/assert-license-compliance.ps1`.

The prior root MIT text is preserved at `licenses/MIT-legacy.txt`. Exact MIT
copies also remain at:

- `integrations/playnite/LICENSE` for the Playnite integration;
- `docs/integration-protocol.LICENSE` for the public protocol.

The compliance gate hashes all four files so an accidental boundary change
fails CI.

## First-party and third-party boundary

First-party core code, tests, packaging, UI resources, and documentation are
GPL-3.0-only unless a more specific local license applies. The Playnite plugin
is shipped separately and keeps its MIT license. The protocol is documentation
intended for independent implementations and keeps its MIT license.

Qt, FFmpeg, MinGW/GCC runtimes, miniz, Monocypher, Inno Setup, Playnite SDK,
Bouncy Castle, and other dependencies retain their upstream terms.
`THIRD_PARTY_NOTICES.md`, `docs/dependency-licenses.md`, package license
files, Qt SBOMs, and dependency pins record those obligations.

## Corresponding source

Release packaging creates source from the exact release commit with
`git archive`, validates the archive layout and checksum, and performs a clean
configure/build check. Binary packages carry a source notice bound to that
revision and public source artifact. Release evidence records the source files
and a passed corresponding-source gate.

## Historical record

The project previously evaluated an unpublished GPL migration and removed it
before publication. That abandoned revision granted no public GPL release.
Version 0.7.1 is the deliberate public transition point. Historical MIT tags and
commits are preserved and are not force-pushed or relabeled.

## Audit limits

This is a repository and release-process audit, not legal advice. A future
dependency, asset, contributor arrangement, protocol change, or distribution
channel must be reviewed before it crosses the documented license boundary.
