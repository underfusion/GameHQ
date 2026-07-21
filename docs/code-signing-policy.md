# Code-Signing Policy

GameHQ uses two separate trust layers:

- The signed release manifest authenticates release metadata and artifact
  hashes. Its production Ed25519 key is provisioned outside the repository.
- Authenticode identifies the Windows publisher for GameHQ-built executables,
  Setup, and the Inno Setup uninstaller. It does not replace artifact hashes or
  the signed release manifest.

## Release roles and access

Accounts with repository write, release, or signing access must use multi-factor
authentication. Release work is separated into author, reviewer, and approver
roles whenever the signing provider supports them. The author builds the exact
tagged source, the reviewer checks provenance and release evidence, and the
approver authorizes signing only after automated gates pass. No private signing
key may be committed, uploaded as a release asset, printed in logs, or placed in
an ordinary CI secret.

Current project roles:

- Author, committer, and reviewer: [underfusion](https://github.com/underfusion)
- Signing approver: [underfusion](https://github.com/underfusion)

External contributions require maintainer review before merge. Signing requests
require a separate explicit approval after the release evidence passes, even
when one maintainer currently fills multiple roles.

Free code signing provided by SignPath.io, certificate by SignPath Foundation.
This statement becomes applicable to distributed binaries only after the
project is accepted and the signing workflow is active.

Privacy statement: this program will not transfer any information to other
networked systems unless specifically requested by the user or the person
installing or operating it. A user may explicitly enable bounded GitHub release
checks; the complete behavior is described in
[Security & Privacy](security-and-privacy.md).

## Signing order

1. Build from the reviewed tag with pinned toolchains.
2. Sign GameHQ-built inner EXE/DLL files with the approved publisher and RFC
   3161 timestamp, then verify them.
3. Package portable, update, and full offline Setup artifacts.
4. Sign Setup and the generated Inno uninstaller, then verify every signature
   and timestamp again from the final artifacts.
5. Generate hashes and the byte-exact release manifest only after final bytes
   are fixed.

Third-party Qt, FFmpeg, compiler-runtime, and Playnite files keep their upstream
identity and are never re-signed as GameHQ. ZIP files do not carry ordinary
Authenticode; their authenticity comes from signed inner binaries and the
signed release manifest.

## Beta and Stable policy

An unsigned build may be published only as an explicitly labelled
`unsigned-beta` release with honest Unknown-publisher guidance. An unsigned
artifact must never be described as Stable. Stable requires the expected
publisher, valid RFC 3161 timestamps, a signed uninstaller, verified final bytes,
and the clean-Windows trust matrix. SmartScreen reputation can still take time
to develop and is not represented as a cryptographic guarantee.

Signing-provider enrollment and production-key activation are separately
reviewed operations. Revoked publisher certificates or Ed25519 key IDs stop
release publication until the trust set and recovery evidence are reviewed.
