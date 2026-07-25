# Contributing to GameHQ

Bug reports, documentation improvements, and focused pull requests are welcome.

## Before opening an issue

- Check existing issues and the latest release notes.
- Include the GameHQ version, Windows version, controller/backend if relevant,
  clear reproduction steps, and the expected versus actual result.
- For crashes or capture failures, attach the diagnostic summary from
  **Settings → Advanced** and the relevant log excerpt. Remove personal paths
  or game/account information first.
- Report security problems privately as described in [SECURITY.md](SECURITY.md).

## Development workflow

1. Build the project using [docs/dev-setup.md](docs/dev-setup.md).
2. Keep changes focused and update affected documentation.
3. Use an English Conventional Commit message, such as
   `fix(input): preserve secondary binding after reset`.
4. Verify a clean build and launch before opening a pull request.

Pull requests should explain the behavior change, why it is needed, and how it
was validated. Avoid committing build output, runtime databases, logs, captures,
toolchains, or editor-specific files.

## Contribution licenses

GameHQ uses an inbound-equals-outbound policy with no contributor license
agreement or copyright assignment:

- contributions to first-party GameHQ core code and documentation are
  submitted under GPL-3.0-only;
- contributions under `integrations/playnite/` and to the public integration
  protocol are submitted under their reviewed local MIT licenses;
- third-party material remains under its documented upstream license and must
  not be copied into the project unless that license is compatible with the
  destination and all attribution and source obligations are met.

By submitting a contribution, you confirm that you created it or otherwise
have the authority to submit it under the applicable license. Employer,
school, client, or collaborator rights must be resolved before submission.
Do not submit proprietary code, use-restricted media, private data, secrets, or
material copied from an incompatible source.

AI-assisted work is permitted only when the contributor reviews and takes
responsibility for the result and can account for the provenance of included
code, media, models, datasets, prompts, or generated assets. Uncertain origin
or licensing must be disclosed and resolved before review.

The project does not request a proprietary dual-licensing grant. A Developer
Certificate of Origin or other attestation would require a separate future
project decision.
