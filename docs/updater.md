# Update System

How GameHQ checks for new releases, downloads and verifies them, and installs
them safely while guaranteeing user data and the portable/installed choice
are never touched. This document is the design reference; see
[roadmap.md](roadmap.md) for the delivery phases and
[packaging.md](packaging.md) for how release archives are assembled.

## Goals and non-goals

- Users always know their current version and whether a newer stable release
  exists, without any action required.
- A one-click update never loses recordings, settings, or the portable
  marker, and a failed update always rolls back to a working install.
- The updater never touches a game in progress and never blocks capture.
- Out of scope for v1: auto-download without consent, delta patching,
  multiple release channels, elevation/UAC, non-Windows targets.

## Canonical identifiers

These names are fixed by the Phase 0 decision item and must never be
reinvented elsewhere in code, docs, or scripts:

- Local IPC pipe name: `GameHQ.Local.v1` (see
  [integration-protocol.md](integration-protocol.md)).
- Release repository: `underfusion/GameHQ` on GitHub. Canonical project page,
  releases, and issue tracker are the GitHub repository, its Releases tab,
  and its Issues tab, respectively, until a real public website is deployed.
  No future domain is embedded before it is live.
- Release assets, three per release:
  - `GameHQ-<version>-win64-portable.zip` — fresh portable install, contains
    `portable.flag`. Never auto-applied by the updater; manual download only.
  - `GameHQ-<version>-win64-update.zip` — program-owned files only, plus
    `update-package.json`. This is what the updater downloads and applies.
  - `GameHQ-<version>-win64-update.zip.sha256` — checksum of the update zip.
- Helper executable: `GameHQUpdater.exe`, itself program-owned and shipped
  inside every release. Its self-updated replacement stages as
  `GameHQUpdater.pending.exe` before promotion (see Stage 8).

## Path ownership contract

The updater and its helper operate on a strict allowlist. Anything not
listed is never moved, replaced, or deleted.

**Program-owned (updater MAY replace):**

- `GameHQ.exe` (root launcher)
- `app/` (real executable, Qt/FFmpeg/MinGW runtime, QML)
- `README.txt`, `LICENSE.txt`, `THIRD_PARTY_NOTICES.md`, `licenses/`
- `GameHQUpdater.exe`

**User-owned (updater MUST NEVER touch):**

- `Captures/` — screenshots and clips
- `gamehq-data/`, `saveplay-data/`, `playhq-data/` — config, database,
  thumbnails, logs
- `portable.flag` — see the four-never rule below

### `portable.flag` four-never rule

The release update zip does not legitimately need `portable.flag` (only the
portable *install* zip does), but packaging must be treated as adversarial
input to the swap step regardless. The flag is:

1. Never installed — stripped from staging before any file is applied.
2. Never deleted — an existing flag survives every update.
3. Never backed up as part of a rollback set — its state is not part of the
   transaction.
4. Never touched even if stripping regresses — the swap step works from a
   positive allowlist of program-owned paths, so an unlisted file can never
   move regardless of what staging contains.

The user's pre-update state (flag present or absent) must be bit-for-bit
identical after any update, successful or rolled back.

## Update flow overview

1. **Discovery** — `UpdateService` asks the GitHub Releases API for the
   latest stable release, compares it against the installed `VERSION`, and
   surfaces a non-modal banner and a distinct About-sidebar state when a newer
   stable release exists. Automatic
   checks run at most once every 24 hours and are silent on failure. The
   first automatic check fires 15-30s after startup (`App::init()`); a
   coarse hourly timer then re-checks the 24h window rather than scheduling
   a single long-lived timer. `updates.check_automatically` (default on)
   lets the user turn automatic checks off entirely; `updates.skipped_version`
   suppresses the banner for a release the user dismissed with "Skip this
   version" until a newer one ships. Manual "Check now" (Settings → About)
   always bypasses the 24h cache and surfaces real errors. The banner itself
   lives only in the desktop gallery window
   (`Main.qml`), never in `OverlayWindow.qml`, so it can never appear over a
   running game or the pad overlay. The compact About / What's New modal exposes
   the same state and primary action without duplicating the detailed controls
   on Settings -> About.
2. **Download** — the update zip and its `.sha256` are streamed to a
   `.partial` file in a staging directory, then atomically published and
   verified against the checksum. Downloads accept HTTPS only (including
   redirects), enforce the release asset size and a 2 GiB package ceiling,
   and remove incomplete files on cancellation or the next startup. The
   checksum file must contain one SHA-256 entry naming the expected package;
   any malformed, mismatched or corrupted download is deleted before it can
   become installable. A package downloaded earlier must be hashed again and
   its release revalidated immediately before installation. See Stage 2 below.
3. **Quiescence** — the app confirms no game is currently being recorded and
   no export is finalizing before entering maintenance mode. See Stage 3.
4. **Handoff** — the app writes a transaction file (including its own process
   id as `callerPid`), launches `GameHQUpdater.exe` from the install root, and
   waits on a named READY event the helper signals once it has validated the
   transaction. Only then does the app exit; if the helper never becomes ready
   or exits early, the app cancels the update and stays running.
5. **Swap** — the helper waits for the `callerPid` process to actually exit
   (bounded wait; a timeout aborts before any file is touched), extracts and
   validates the package in staging, backs up the current program-owned files,
   installs the new ones, and restarts the app in post-update validation mode.
6. **Health check** — the new process must reach a running, initialized state
   within a bounded window and write a success token; otherwise the helper
   restores the backup and restarts the previous version.
7. **Post-update greeting** — when the desktop window next becomes active, the
   user sees the About / What's New modal once. Current-version notes come from
   bundled `assets/release-notes.json`, work offline, and are marked read through
   `internal.ui.whats_new_seen_version` when the modal closes. The modal is never
   created in the game overlay.

## Implementation stages

The helper (`src/updater/UpdaterMain.cpp`) is a small static Win32 program
with no Qt dependency, to avoid depending on DLLs the update itself might be
replacing and to keep its attack surface minimal. The work is built and
verified in nine stages; each stage must be independently testable before the
next is attempted. Stages 1, 3 and 4 are app-side preconditions, stages 2 and
5-8 run in the helper, and stage 9 covers both. This list is the authoritative
description of the flow and supersedes any earlier overview that describes a
different stage count or a temp-located helper.

1. **Transaction and dry-run** — the app writes a transaction file describing
   the pending update (target version, package path, expected checksum,
   restart command, success-token path) before launching the helper. A
   dry-run mode validates the transaction and staging plan without touching
   any file, used for automated testing. Transaction schema 1 additionally
   records `productId`, package root, staging and backup directories, and the
   durable phase marker (`download_verified` at handoff), and the writing
   process id (`callerPid`) the helper must wait out before mutating files.
   Every path is canonicalized and must remain below the package root. The
   helper uses a per-transaction Windows mutex to reject a competing updater,
   then prints the full verify/extract/backup/install/restart operation list
   in dry-run mode without creating staging, backup or lock files. Outside
   dry-run every helper outcome is also appended with a timestamp to
   `.update/updater.log`, which survives the post-update artifact cleanup.
2. **Staging extraction and validation** — the helper extracts the update zip
   into a staging directory on the same volume as the install (never a
   different drive, so the final move can be atomic where possible).
   Extraction is hardened against zip-slip: absolute paths, drive-letter
   paths, `..` traversal, entries that escape the staging root after
   canonicalization, and symlinks/reparse points are all rejected. Total
   uncompressed size and file count are capped. `portable.flag`, if present
   in the archive, is stripped from staging before anything else runs.
3. **Quiescence barrier** — the app only starts the handoff once no game is
   actively being recorded and no replay export is finalizing. The app
   broadcasts an `app.maintenance` message (see
   [integration-protocol.md](integration-protocol.md)) over the local pipe
   so connected clients — currently the Playnite plugin — suspend
   auto-launch and reconnect attempts until the update marker clears or a
   bounded retry window passes. An update-related pipe disconnect must never
   be treated as a crash to relaunch from.

   After the transaction is durable, the app atomically publishes
   `.update/maintenance.lock` before launching the helper. The root launcher
   and direct app entry suppress ordinary relaunches while that marker is
   active; the helper clears it only after a durable `healthy` or
   `rolled_back` phase. A missing helper plus a marker older than five minutes
   enters stale recovery, so a crash can never lock the user out permanently.

   Entering this barrier immediately blocks new screenshots and replay saves,
   stops replay auto-arming, and waits for any PNG write or clip remux already
   in flight. The wait is bounded to 30 seconds. A timeout cancels the update,
   re-enables capture, and never cancels or kills the user's export. Only after
   both capture services report ready is configuration flushed and the update
   state allowed to advance to `Quiescent`.
4. **Pre-update data snapshot** — before the swap, the app records enough
   metadata (file counts, sizes, or hashes of `Captures/` and
   `gamehq-data/`) to let post-update tests prove those directories are
   byte-identical afterward. The snapshot is written next to the transaction
   file so the helper can hand it to the post-update validation run.

   After the old process closes SQLite, the helper snapshots only
   `config.json`, `gamehq.db`, and the database `-wal`/`-shm` sidecars. A
   manifest records which files originally existed. Restore first prepares all
   snapshot copies, preserves the failed version's current state, then
   publishes the old set together; any restore error puts the pre-restore set
   back. `Captures/` is never enumerated, copied, removed or restored.
5. **Program-file swap** — the helper moves only the program-owned allowlist
   into a backup directory, then installs the new files from staging into
   their place. File moves retry with bounded backoff, since antivirus
   scanners and Explorer can briefly lock freshly written files on Windows.
   Retries use 100/250/500/1000/1500/2000 ms delays and only apply to sharing,
   lock, or bounded access-denied errors. A permanent error immediately moves
   already-installed files back to staging and restores the previous files in
   reverse order. If that restoration is itself blocked, the backup is retained
   for interrupted-update recovery and is never deleted as though rollback won.
6. **Healthy-start handshake** — the helper restarts the root launcher with
   a post-update validation flag. The new process performs its normal
   `App::init()` sequence (database, migrations, QML engine, services, event
   loop), stays alive 5-10 seconds inside the running event loop, and only
   then writes its success token; the helper waits up to about 30 seconds for
   that token. The backup and pre-update snapshot are retained until the *next* successful
   start, not deleted at the first token, in case the freshly started
   version later crashes.
7. **Rollback and interrupted recovery** — if the health token is not seen in
   time, the helper restores the backup verbatim and restarts the previous
   version. On next launch, the app also detects and cleans up any stale
   staging or transaction left behind by a helper that never completed
   (crash, forced shutdown, power loss).
   An atomic phase marker and pre-swap journal record which allowlisted old
   and new paths existed. Recovery can therefore restore a mixed mid-swap
   state without guessing; program and data rollback form one outcome, and
   the previous launcher runs only after both have succeeded.
8. **Helper self-update** — a newer package may ship a newer
   `GameHQUpdater.exe`. The new helper stages as
   `GameHQUpdater.pending.exe` and is promoted to `GameHQUpdater.exe` only
   after the swap that shipped it completes successfully, so a bad helper
   build can never strand the updater. `update-package.json` carries a
   `minimumUpdaterVersion`; if the currently installed helper is older than
   that, the app falls back to pointing the user at a manual download instead
   of attempting a self-update it cannot safely perform.
   The helper exposes a no-write `--self-test`; the launcher promotes a pending
   helper only when the global updater activity mutex is absent and that test
   exits successfully. Invalid pending executables are discarded while the
   existing helper remains untouched.
9. **Failure-injection suite** — an automated test matrix exercises every
   failure path deliberately: corrupted zip, checksum mismatch, interrupted
   download, read-only target, insufficient disk space, disconnected
   network, a failed file swap, and a new version that never reaches the
   health token. Every case must end with the previous version intact and
   running, and with `Captures/`/`gamehq-data/`/`portable.flag` unchanged.

## Diagnostics

The helper writes its own persistent log at `.update/update.log`,
independent of the application process (which may not be running for parts
of the swap). After a successful start, GameHQ attaches that log to its
normal diagnostics so a failed or rolled-back update is visible from inside
the app, not only on disk.

## Authenticity limit

The `.sha256` file hosted beside the release zip protects against corrupted
or incomplete downloads. It does **not** protect against a compromised
GitHub repository or account — anyone who could tamper with the zip could
equally tamper with its checksum file. A stronger authenticity model (code
signing, or a signed manifest) is tracked as a separate decision; see the
plan's `t8` item. Until that model is approved, an unsigned one-click
updater ships only as a clearly labelled beta, and this limit is stated
explicitly in both this document and the in-app Updates UI.

## Unsupported locations

Self-update is only offered when the package root is locally writable and
the staging/backup directories fit on the same volume. In every other case
GameHQ falls back to pointing the user at a manual download instead of
attempting an unreliable in-place swap:

- Program Files or another location without write rights for the current user
- UNC paths or network shares
- Removable media (USB) or exFAT/FAT32 volumes
- Paths long enough to risk Windows path-length limits
- A staging location forced onto a different volume than the install

No elevation or UAC prompt is used to work around any of these in v1.
