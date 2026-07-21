# SignPath Foundation Application Packet

This packet prepares the external application without claiming approval or
submitting terms on the maintainer's behalf.

## Project

- Name: GameHQ
- Repository and homepage: https://github.com/underfusion/GameHQ
- License: MIT (OSI approved)
- Maintainer: https://github.com/underfusion
- Public releases: `v0.5.55` and `v0.6.3`
- Product: local-first Windows screenshot and rolling replay capture application
- Security reports: https://github.com/underfusion/GameHQ/security/advisories/new
- Privacy policy: https://github.com/underfusion/GameHQ/blob/main/docs/security-and-privacy.md
- Code signing policy: https://github.com/underfusion/GameHQ/blob/main/docs/code-signing-policy.md

## Eligibility evidence

- The repository, build scripts, MIT license, documentation, changelog, and
  public release artifacts are visible in one maintained project.
- GameHQ has no account, telemetry, game-process injection, hidden service,
  scheduled task, obfuscator, packer, Defender exclusion, or SmartScreen bypass.
- Installer changes are announced, per-user, reversible, and preserve user data.
- Maintainers with repository/signing access are required by policy to use MFA.
- GitHub Private Vulnerability Reporting is enabled and named in SECURITY.md.
- Build/release identity, pinned toolchains, exact artifact hashes and trust mode
  are recorded by the release gate.

## Requested signing scope

Only GameHQ-owned binaries may receive the project signature:

```text
GameHQ.exe
app/GameHQ.exe
GameHQUpdater.exe
GameHQ.Playnite.dll
GameHQ-<version>-win64-setup.exe
Inno Setup generated uninstaller
```

Qt, FFmpeg, MinGW runtime, Playnite SDK and other upstream binaries remain
unsigned or retain their upstream signatures. They must never be signed as
GameHQ-owned code.

## Proposed verifiable workflow

1. A reviewed tag fixes source, version, release notes and build scripts.
2. GitHub Actions builds with pinned CMake, Qt, miniz and Inno Setup inputs.
3. Automated tests and the release gate validate payload ownership, artifact
   versions, hashes and `signed` trust mode.
4. The signing request is manually approved through the assigned approver role.
5. Final signed artifacts are reverified and published with release evidence.

## External decisions required before submission

- Confirm the maintainer accepts SignPath Foundation's current OSS terms and
  that the visible publisher may be **SignPath Foundation**.
- Confirm MFA is active for the GitHub and future SignPath accounts.
- Decide whether the existing public portable release satisfies the provider's
  “already released in the form that should be signed” condition, or publish an
  approved unsigned Setup Beta first.
- Submit the application through https://signpath.org/ and record its application
  identifier and approved, rejected, or pending state in the canonical plan.
