# SignPath Foundation Application Packet

This packet prepares the external application without claiming approval or
submitting terms on the maintainer's behalf.

## Project

- Name: GameHQ
- Repository and homepage: https://github.com/underfusion/GameHQ
- License: MIT for the GameHQ core, Playnite integration, and public protocol
- Maintainer: https://github.com/underfusion
- Public releases: `v0.5.55` and `v0.6.3`
- Product: local-first Windows screenshot and rolling replay capture application
- Security reports: https://github.com/underfusion/GameHQ/security/advisories/new
- Privacy policy: https://github.com/underfusion/GameHQ/blob/main/docs/security-and-privacy.md
- Code signing policy: https://github.com/underfusion/GameHQ/blob/main/docs/code-signing-policy.md

## Eligibility evidence

- The repository, build scripts, MIT license, documentation, changelog, and
  public release artifacts are visible in one maintained project.
- GameHQ has no commercial dual-licensing scheme or proprietary first-party
  bundled component. Third-party runtimes retain their documented upstream
  licenses and are never represented as GameHQ-owned code.
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

## Maintainer decisions

- Applying to SignPath Foundation is approved, and **SignPath Foundation** is
  accepted as the visible publisher. The maintainer must still personally
  review and accept the live Code of Conduct.
- GitHub MFA must be confirmed before submission; enable and confirm SignPath
  MFA as soon as the account is created.
- Do not publish an unsigned Setup Beta solely for this application. Submit
  using the existing public releases and current Setup pipeline evidence. If
  SignPath explicitly requires a publicly released Setup, stop for approval.
- Paid OV is a fallback only if SignPath rejects GameHQ or cannot support it.

## Current application form

The public form at https://signpath.org/apply currently requests the fields
below. Values marked `MAINTAINER` contain personal or historical information
that must not be guessed.

| Form field | Required | Value to enter |
| --- | --- | --- |
| Project Name | Yes | `GameHQ` |
| Repository URL | Yes | `https://github.com/underfusion/GameHQ` |
| Homepage URL | Yes | `https://github.com/underfusion/GameHQ` |
| Download URL | No | `https://github.com/underfusion/GameHQ#download` |
| Privacy Policy URL | No | `https://github.com/underfusion/GameHQ/blob/main/docs/security-and-privacy.md` |
| Wikipedia URL | No | Leave blank. |
| Tagline | Yes | `A controller-first Windows app for screenshots, replay clips, and an in-game capture gallery.` |
| Description | Yes | `GameHQ is a local-first MIT-licensed Windows application for capturing screenshots and recent-gameplay clips, then browsing them in a controller-friendly gallery. The GameHQ core, Playnite integration, and public protocol are open source under MIT. GameHQ requires no account or telemetry.` |
| Reputation | Yes | `GameHQ is a newly public, actively maintained MIT-licensed project with two published releases: v0.5.55 and v0.6.3. It has no commercial dual-licensing scheme or proprietary first-party bundled component; redistributed third-party runtimes retain their upstream licenses. The repository provides complete source, changelog, security, privacy and code-signing policies, reproducible build scripts, automated tests, and release hashes. Repository: https://github.com/underfusion/GameHQ - Releases: https://github.com/underfusion/GameHQ/releases` |
| Maintainer Type | No | `Individual maintainer(s)` |
| Build System | No | `GitHub Actions` |
| First Name | Yes | `MAINTAINER: given name for the SignPath account` |
| Last Name | Yes | `MAINTAINER: family name for the SignPath account` |
| Email | Yes | `MAINTAINER: account and notification address` |
| Company Name | No | Leave blank unless applying for an organization. |
| Primary Discovery Channel | Yes | `MAINTAINER: select the truthful channel` |
| Exact discovery source | No | `MAINTAINER: for example ChatGPT, only if accurate` |
| Code of Conduct consent | Yes | Review the live terms, then personally check this box. |
| Other communications | No | Leave unchecked unless marketing communications are desired. |

The form also runs reCAPTCHA and ends with a **Submit** button. Do not pre-check
consent or press Submit through automation.

## Exact submission sequence

1. Review and publish the current GameHQ documentation changes through the
   normal repository workflow. Do not submit while the public README lacks the
   **Code signing policy** link and required attribution.
2. Confirm GitHub MFA is active for every account with repository or signing
   access.
3. Open https://signpath.org/terms and personally review the current Code of
   Conduct, especially publisher identity, own-binary scope, roles, MFA,
   verifiable builds and manual approval.
4. Open https://signpath.org/apply and enter the table values plus the three
   maintainer-owned identity/discovery fields.
5. Complete reCAPTCHA. Check only the required Code of Conduct consent after
   personal review; leave optional communications unchecked unless desired.
6. Review every URL and statement. Press **Submit** personally.
7. Record the confirmation/application identifier and pending, approved or
   rejected state in canonical `t46`.
8. If SignPath asks for a publicly released Setup, stop and request explicit
   approval before publishing one. Do not activate the production Ed25519 key.
