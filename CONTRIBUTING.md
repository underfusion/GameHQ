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
