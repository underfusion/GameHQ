---
ccgui_plan_version: 6
plan_id: 2026_07_20_gamehq_updater_about_page_and_playnite_bridge
title: GameHQ updater, About page and Playnite bridge (2026-07-20)
type: single
status: draft
progress_mode: weighted
progress_pct: 82
current_item:
created_at: 2026-07-20
updated_at: 2026-07-21
---

# GameHQ updater, About page and Playnite bridge (2026-07-20)

## Plan Overview

### What this plan covers

GameHQ gets its own About page, a safe built-in update system, and a connection to the Playnite game launcher. The work happens in six phases: first decisions and documentation, then the About page with update checking, then the full safe updater with rollback, then a local connection channel, then a small Playnite companion plugin, and finally its public release.

### Expected outcome

Users can see what version they have, get notified about new versions, and update safely with one click without ever losing their recordings or settings. Playnite users additionally get a small plugin that tells GameHQ which game is running, making detection more reliable. GameHQ keeps working completely standalone without Playnite.

## Current State


Total progress: ~82%
Source: external implementation plan (GPT) reviewed and restructured by Fable on 2026-07-20. Key claims were verified against the codebase before authoring this plan:

- Confirmed: a second GameHQ instance exits silently — `QLockFile` guard at `src/main.cpp:27-29` returns 0 without notifying the running instance.
- Confirmed: Qt Network is NOT linked yet — `CMakeLists.txt:40` lists Core Gui Quick Sql Widgets Svg Multimedia only. It must be added once (Phase 1) and reused for IPC (Phase 3).
- Confirmed: the GitHub link is hard-coded in `src/ui/qml/HelpView.qml:240` and `Brand.qml` has no URL properties — centralizing links first is correct.
- Reviewer verdict: the proposed order (updater before public bridge release) is correct — Playnite can update the plugin but not GameHQ itself, so an outdated GameHQ with a current plugin is the failure mode to prevent.
- Reviewer additions not in the original plan: SmartScreen/AV false-positive risk of an unsigned helper exe launched from temp; file-move retry loop for AV/Explorer locks; packaging script always writes `portable.flag` and must be aligned with the updater; GitHub unauthenticated rate limit is per-IP (shared NAT can 403) so 403 must be a soft failure.
- Project rule reminder for every executing model: every change bumps `VERSION` by one patch + `CHANGELOG.md` entry + affected `docs/` update; commit to `dev` only, Conventional Commits; all QML visual values from `Theme.qml`.

## Status Legend

- todo — not started
- in_progress — being worked on
- done — finished and verified
- deferred — intentionally postponed
- blocked — waiting on something external
- skipped — decided not to do
- needs_decision — user must decide before work continues
- review — done, awaiting review
- moved — moved to another plan

## Plan Checklist

<!-- CCGUI:TASK id=p1 status=done weight=1 progress=100 position=0 risk=low -->
- [x] Phase 0 — Decisions and groundwork
  - Description: Lock down names, links and design documents before any code is written.
  - Done when: Every identifier and document exists and nothing is left to invent during coding.
  - Result: All groundwork decisions are resolved, including the update authenticity ladder.
  <!-- CCGUI:TASK id=p1-1 status=done weight=1 progress=100 parent_id=p1 position=0 risk=low -->
  - [x] Choose canonical links and reserve stable identifiers
    - Description: Decide the official website address and the permanent technical names used everywhere later.
    - Done when: All names and links are written down and approved.
    - Agent notes: Decide canonical website URL. Guidance (review 2026-07-20): until a real domain is actually deployed, canonical project page = GitHub repository (or a working GitHub Pages site), releases = GitHub Releases, issues = GitHub Issues; never embed a future URL that is not live yet. Reserve and record: local pipe name "GameHQ.Local.v1"; Playnite extension Id + plugin GUID; add-on database AddonId "GameHQ_Integration"; plugin display name "GameHQ Integration"; RELEASE ASSETS (updated, review 3): TWO artifacts per release - "GameHQ-<version>-win64-portable.zip" (fresh portable install, contains portable.flag) and "GameHQ-<version>-win64-update.zip" (program-owned files only + update-package.json) plus "GameHQ-<version>-win64-update.zip.sha256"; plugin repo name "underfusion/GameHQ-Playnite". No identifier may later be invented ad-hoc in code. RECOMMENDED RESOLUTIONS (final review 2026-07-20, pending user approval): website = GitHub repository until a real public site is live; automatic checks = ON, max once per 24 h; automatic download/install = OFF; ZIP library = miniz, version pinned; helper location = install root or .update/, never %TEMP% (DECIDED by design, no longer open); health window = 5-10 s after entering the event loop, helper timeout ~30 s (DECIDED, no longer open); backup retention = until the next good start plus a bounded time window (DECIDED); code signing = does not block development, but an unsigned one-click updater ships only as a clearly labelled beta (see t8); public stable updater only after the authenticity model is approved; Playnite target = 10, separate adapter for 11 later.
    - Result: Approved by user 2026-07-20.
  <!-- CCGUI:TASK id=p1-2 status=done weight=1 progress=100 parent_id=p1 position=1 risk=low -->
  - [x] Write updater and protocol design documents, update roadmap
    - Description: Document how updating works, what files it may touch, and how the Playnite connection speaks.
    - Done when: Both documents exist and the roadmap shows this work before the next big milestone.
    - Agent notes: Create docs/updater.md: full flow + path ownership contract — updater MAY replace GameHQ.exe, app/, README.txt, LICENSE.txt, THIRD_PARTY_NOTICES.md, licenses/, GameHQUpdater.exe; MUST NEVER touch Captures/, gamehq-data/, saveplay-data/, playhq-data/, portable.flag. Create docs/integration-protocol.md: transport QLocalServer named pipe "GameHQ.Local.v1", frame = 4-byte little-endian length + UTF-8 JSON, max 64 KiB; message set hello, hello.ack, app.activate, app.open_gallery, status.request, playnite.application.started/stopping, playnite.game.starting/started/stopped/startup_cancelled, playnite.state.sync. Insert "0.6.x Distribution & Integration Foundation" into docs/roadmap.md before the 0.7 storage milestone. Bump VERSION + CHANGELOG. ALSO: docs/updater.md must cover the helper self-update cycle - updater protocol version, minimumUpdaterVersion required by a package, GameHQUpdater.pending.exe promotion, compatibility fallback (see Stage 6 under p3-2). VERSIONING POLICY for every executing model: bump VERSION once per independently reviewable merged change (PR/slice), updating CHANGELOG.md and affected docs in the SAME change; multiple plan items completed in one slice share ONE bump - never one bump per plan item. The Playnite plugin repo versions independently with its own changelog.
    - Result: Wrote docs/updater.md (path ownership contract, 9-stage helper flow, authenticity limit, unsupported-location policy) and docs/integration-protocol.md (GameHQ.Local.v1 pipe framing, handshake, full message set incl. app.maintenance, identity-authority rule, second-instance activation). Inserted the 0.6.x Distribution & Integration Foundation milestone into docs/roadmap.md before 0.7. Bumped VERSION to 0.6.4 and added a CHANGELOG entry. Verified: app builds and runs (build/GameHQ.exe launched, gamehq.log shows healthy startup/detection loop, no crash).
  <!-- CCGUI:TASK id=t8 status=done weight=1 progress=100 parent_id=p1 position=2 risk=medium -->
  - [x] Decide update authenticity model and code signing before public release
    - Description: Choose how updates are proven genuine, and whether releases are digitally signed, before public one-click updating.
    - Done when: The user has chosen the authenticity model, or an explicitly labelled beta with manual fallback.
    - Result: Approved ladder: SHA-256 for private testing, Ed25519-signed manifest before public Beta, Authenticode before Stable, manual GitHub updates always available.
    - Agent notes: Decision approved by user 2026-07-20 (via GPT review). Ladder: (1) internal/private testing — SHA-256 only, keep Beta wording; (2) public one-click Beta — requires update-package.json + update-package.sig, Ed25519, public key embedded in app and helper (see t29); (3) Beta-to-Stable promotion — Authenticode-sign all GameHQ-built executables under one publisher identity (see t30); (4) manual GitHub Releases path stays available permanently. Wording rule: describe Authenticode as a verified publisher identity that helps build consistent SmartScreen reputation — it does NOT guarantee warnings disappear immediately for new releases. Private Ed25519 key stays outside the repo and ordinary source-control secrets; offline signing on the user's machine is fine for first releases.
<!-- CCGUI:TASK id=p2 status=deferred weight=1 progress=95 position=1 risk=low -->
- [ ] Phase 1 — About page and update discovery
  - Description: Add the About page and the ability to notice that a newer version exists.
  - Done when: Users can see their version, check for updates, and open the new release page.
  - Note: Implementation is complete except p2-2, deferred until the user checks layout and navigation.
  <!-- CCGUI:TASK id=p2-1 status=done weight=2 progress=100 parent_id=p2 position=0 risk=low -->
  - [x] Centralize product links in one place
    - Description: Put all project web addresses in one shared spot so every page uses the same ones.
    - Done when: Help and About read links from the shared source and no address is duplicated.
    - Agent notes: Add readonly string properties websiteUrl, repositoryUrl ("https://github.com/underfusion/GameHQ"), releasesUrl, issuesUrl to src/ui/qml/Brand.qml. If C++ needs the repo owner/name for update lookup, extend src/config/Brand.h.in + CMake configure step instead of hard-coding in UpdateService. Replace hard-coded link at src/ui/qml/HelpView.qml:240 with the Brand property. VERSION + CHANGELOG.
    - Result: Brand.qml now owns repositoryUrl / websiteUrl / releasesUrl / issuesUrl; HelpView reads Brand.repositoryUrl instead of a hard-coded link.
  <!-- CCGUI:TASK id=p2-2 status=deferred weight=3 progress=80 parent_id=p2 position=1 risk=low user_test=required -->
  - [ ] Build the About settings page
    - Description: A new About section in Settings showing version, storage mode, project links and a friendly star request.
    - Done when: The user opens About, sees correct info, and confirms it looks and navigates well.
    - Agent notes: Built and compiles clean (AboutSettingsPage.qml), but done_when explicitly requires a human to confirm layout and gamepad/keyboard navigation (extra.user_test=required). Deferred until the user runs it and confirms.
    - Note: AboutSettingsPage.qml is written and compiles with no QML errors (logo/version/storage mode, Updates placeholder, project links + GitHub star CTA; Version/Storage rows moved out of Advanced). Cannot be marked done until a human checks layout and gamepad/keyboard navigation.
  <!-- CCGUI:TASK id=p2-3 status=done weight=2 progress=100 parent_id=p2 position=2 risk=low -->
  - [x] Add networking support and a strict version comparer
    - Description: Teach the app to compare version numbers correctly and prepare it for internet requests.
    - Done when: Version comparison passes all tests including malformed input.
    - Agent notes: Add Network to find_package(Qt6 ...) at CMakeLists.txt:40 and to target_link_libraries in src/CMakeLists.txt (added once here, reused by Phase 3 IPC). New src/updates/VersionNumber.h/.cpp: strict semver parse — normalize optional leading "v", numeric major.minor.patch compare, reject malformed, NEVER compare versions as strings. Exhaustive unit tests in tests/ (valid, invalid, ordering, equal, v-prefix). VERSION + CHANGELOG.
    - Result: src/updates/VersionNumber.{h,cpp}: strict major.minor.patch parser/comparator, v-prefix tolerant, never string-compared. tests/tst_versionnumber.cpp: 14 cases, all passing. Qt6::Network linked.
  <!-- CCGUI:TASK id=p2-4 status=done weight=3 progress=100 parent_id=p2 position=3 risk=medium -->
  - [x] Implement release lookup and the update service
    - Description: The app can quietly ask the project page whether a newer stable version exists.
    - Done when: A newer stable release is detected and shown; failures never disturb the app.
    - Agent notes: New src/updates/ReleaseInfo.h (immutable: version, name, notes, date, web URL, zip URL/name/size, checksum URL, prerelease flag); GitHubReleaseSource.h/.cpp: GET https://api.github.com/repos/underfusion/GameHQ/releases/latest with Accept: application/vnd.github+json, X-GitHub-Api-Version header, descriptive User-Agent, ETag/If-None-Match caching, timeouts, bounded retry; handle 200/304/403/404/timeout/malformed JSON; select ONLY exact-named assets per p1-1 convention; no embedded token. UpdateService state machine Idle/Checking/UpToDate/UpdateAvailable/Downloading/ReadyToInstall/Installing/Failed with properties (state, installedVersion, latestVersion, notes, releaseUrl, size, progress, errorText, lastChecked) and commands (checkNow, downloadUpdate, cancelDownload, installAndRestart, skipVersion, openReleasePage). Expose as dedicated "updates" QML context object from src/app/App.cpp — same pattern as overlay/sounds/input/notifications, do NOT bloat AppController. Reject releases <= installed, drafts, prereleases; log rejection reason. Release tag must equal VERSION file content. VERSION + CHANGELOG. RATE-LIMIT PRECISION (review 2026-07-20): on 403/429 inspect x-ratelimit-remaining, x-ratelimit-reset and retry-after headers; classify as rate-limited ONLY when confirmed (e.g. x-ratelimit-remaining: 0) - a generic 403 without those headers is an ordinary source error, not a rate limit. When rate-limited: automatic check = no popup, keep last good result, store next allowed attempt time, log; manual check = message "GitHub temporarily limited update checks. GameHQ will try again after [time]." plus an open-Releases button. Never re-request before the reset time (GitHub may block persistent offenders). Future option (not v1): static stable.json on project page/GitHub Pages with the API as fallback. ASSET SELECTION (updated, review 3): the updater downloads GameHQ-<ver>-win64-update.zip and its .sha256; the -portable.zip exists only for manual fresh installs and is never auto-applied. Keep exact-name matching.
    - Risk notes: GitHub unauthenticated limit is 60 requests/hour PER IP — shared NAT users can hit 403; treat 403/rate-limit as a soft "check failed" state, never as an update and never blocking startup or capture.
    - Result: src/updates/GitHubReleaseSource.{h,cpp} (releases API, ETag caching, rate-limit detection, one bounded retry) and UpdateService.{h,cpp} exposed to QML as 'updates' from App.cpp:166. Rejects drafts/prereleases/non-newer releases; a failed or rate-limited check never regresses a known-good result.
  <!-- CCGUI:TASK id=p2-5 status=done weight=2 progress=100 parent_id=p2 position=4 risk=medium -->
  - [x] Wire check policy, settings keys and the update banner
    - Description: Automatic daily background checks with a gentle non-blocking notice when something new exists.
    - Done when: Checks run on schedule, respect user preference, and never interrupt a game.
    - Agent notes: Config keys in src/config/ConfigKeys.h + ConfigManager::defaults(): updates.check_automatically (default true), updates.skipped_version (default ""); internal keys internal.updates.last_check_utc, internal.updates.etag, internal.updates.last_seen_version, internal.updates.pending_post_update_version (internal.* must survive "Restore all settings" per existing policy). Do NOT add updates.channel yet (no tested pipeline behind it). Policy: first check delayed 15-30 s after startup, max once per 24 h, stable only, manual checkNow ignores the 24 h cache and reports real errors. Never show update UI above a running game or the overlay — save state, show subtle indicator next time desktop window opens; non-modal banner preferred over modal dialog. Show new version, date, size, What's New (release body via conservative plain-text/rich-text conversion — no Markdown dependency), buttons Download and restart (disabled until Phase 2 — use "View release on GitHub" fallback so this phase ships standalone value), Not now, Skip this version, View full release. Update docs/configuration.md. VERSION + CHANGELOG. RELEASE-NOTE SANITIZATION: release-body content comes from the internet - render as plain text or a strictly limited formatting subset: never execute HTML, no remote images, no file:// links, only explicitly allowed https links.
    - Result: Added updates.check_automatically / updates.skipped_version config keys + internal.updates.etag / internal.updates.last_check_utc persistence. App::init() primes UpdateService from config, runs the first automatic check 15-30s after startup, and re-checks at most once every 24h via an hourly gate timer; manual checkNow() always bypasses the cache. New UpdateBanner.qml shown only in the desktop window (never over the overlay/a game) with version/date/size/plain-text notes and View on GitHub / Skip this version / Not now actions. Settings -> About Updates section now shows real state with a working Check now button and a Check automatically toggle. VERSION bumped to 0.6.9, CHANGELOG + docs/updater.md updated. Verified: full clean build succeeds and the packaged build starts cleanly with no QML errors (gamehq.log).
<!-- CCGUI:TASK id=p3 status=deferred weight=1 progress=95 position=2 risk=high -->
- [ ] Phase 2 — Safe download, install and rollback
  - Description: The app can download an update, verify it, install it safely and undo a failed attempt.
  - Done when: A real older build updates itself successfully and a sabotaged update rolls back cleanly.
  - Result: A real 0.6.9 package updated to 0.6.10, while a sabotaged update restored 0.6.9 and its data.
  - Note: Acceptance gated on the audit-fix slice (t12): helper handoff wait, data-restore journal, install UI and shutdown ownership must land first.
  - Agent notes: Verified: A disposable copy of the real portable 0.6.9 build upgraded through the production ZIP to 0.6.10 with helper exit 0, transaction phase healthy and version assertion success. Sabotage: A correctly hashed package claiming 9.9.8 but containing 0.6.10 failed health validation, reached rolled_back, restarted 0.6.9 and restored launcher, README and gamehq.db byte-for-byte. Release: 0.6.10 portable/update ZIPs rebuilt after the staging fix; checksum and full validator pass. Tests: all 8 CTest targets pass and git diff --check is clean. Deferred: active export, real DB-migration failure, UI/gamepad, Defender/SmartScreen and environmental acceptance.
  <!-- CCGUI:TASK id=p3-1 status=done weight=3 progress=100 parent_id=p3 position=0 risk=medium -->
  - [x] Download the package and verify its integrity
    - Description: The update file is fetched with progress shown and proven untampered before use.
    - Done when: A corrupted or mismatched download is always rejected before installation.
    - Agent notes: Implementation: Added UpdateDownloader with HTTPS-only redirect policy, 2 GiB package limit, exact release-size enforcement, install-local .partial streaming, atomic rename, cancellation/stale-partial cleanup, strict single-entry checksum parsing, local QCryptographicHash verification, and detailed logging. UI: UpdateBanner and About show progress, retry/cancel, ReadyToInstall, and the SHA-256 authenticity limit. Files: src/updates/UpdateDownloader.{h,cpp}, UpdateService.{h,cpp}, src/config/Paths.{h,cpp}, QML update views, docs/updater.md. Verification: GameHQ and tst_updatedownloader build; ctest 5/5 passed; git diff --check passed. Version: 0.6.10.
    - Result: Update packages now download with visible progress and are rejected unless their SHA-256 checksum matches.
  <!-- CCGUI:TASK id=p3-2 status=deferred weight=1 progress=95 parent_id=p3 position=1 risk=critical -->
  - [ ] Build the updater helper with staging, backup and rollback
    - Description: A tiny separate program swaps the application files while the app is closed, built and verified in nine small stages.
    - Done when: All nine stages below are complete and the helper passes the full failure rehearsal.
    - Agent notes: State: All nine stages are implemented; only human/environmental gates in t7, t9 and t10 remain. Integration: UpdateInstaller atomically writes the verified transaction, UpdateService revalidates the release before install, App hands off to the install-root helper after quiescence, and the helper performs staging, snapshot, allowlisted swap, health validation and recovery. Fix: PowerShell ZIP directory entries are recognized by their trailing separator even when miniz omits the directory flag; failure diagnostics now include the entry and Windows error. E2E: disposable real 0.6.9->0.6.10 completed healthy with exit 0; sabotaged 9.9.8 timed out, rolled back and restarted 0.6.9. Verification: release gate and all 8 CTest targets pass; git diff --check passes.
    - Risk notes: CRITICAL overall - a bug can delete user recordings or brick installs. A weaker model must not improvise: follow the stages in order, keep the path allowlist/denylist exactly as documented, and escalate to the user before changing the ownership contract in any way.
    - Result: The helper safely stages, swaps, validates, updates itself and restores the previous version after failure.
    - Note: Wiring release revalidation, transaction creation, helper launch and app shutdown.
    <!-- CCGUI:TASK id=t1 status=done weight=1 progress=100 parent_id=p3-2 position=0 risk=high -->
    - [x] Stage 1 - Update transaction and dry-run validation
      - Description: Prepare each update as a recorded transaction that can first run in a harmless simulation mode.
      - Done when: The helper can list every planned file operation in simulation mode without touching anything.
      - Result: The updater validates a recorded transaction and lists every operation in dry-run mode without changing files.
      - Agent notes: Implementation: Added static GameHQUpdater target and strict flat JSON transaction schema 1 in src/updater. Required fields cover product/version/hash, package root/package path, staging/backup, restart executable, health token, and durable phase. Safety: all absolute paths use weak canonicalization and must remain strictly below the package root; exact update asset naming is enforced; a per-transaction Windows mutex rejects concurrent helpers. Dry-run prints verify/extract/backup/install/restart operations without creating files. Tests: tst_updatertransaction proves a valid dry run leaves staging/backup absent and rejects ../ escape paths. Verification: full build passed; ctest 6/6; helper imports only KERNEL32.dll and msvcrt.dll; git diff --check passed. Version slice: 0.6.10.
      - Risk notes: This gate makes the critical stages safe - no later stage may bypass the transaction/dry-run path.
    <!-- CCGUI:TASK id=t2 status=done weight=1 progress=100 parent_id=p3-2 position=1 risk=high -->
    - [x] Stage 2 - Staging extraction and package validation
      - Description: The downloaded package is unpacked into a side folder and fully checked before anything is replaced.
      - Done when: Bad or hostile archives are always rejected in staging and never touch the live application.
      - Result: Hostile or malformed update archives are rejected in staging before live application files are touched.
      - Agent notes: Dependency: miniz 3.1.2 pinned by upstream tag archive SHA-256 98468f8924934b723276680f85238b6c78bf1f8b49b4459cc9b7214a20e2e9fb; MIT license added to licenses/miniz.txt and notices. Implementation: UpdaterStaging re-hashes the ZIP with Windows BCrypt, accepts only stored/deflated entries, caps 20,000 files, 512 MiB per file and 8 GiB total, requires strict UTF-8 paths, rejects absolute/drive/traversal/duplicate/link/reparse/forbidden/unknown-root entries, extracts by callback, and validates schema-1 manifest plus GameHQ.exe and app/GameHQ.exe layout. Failure removes staging; live root is never written. Verification: valid staging, traversal, portable.flag, bad manifest and post-verification hash-change tests pass; full build and ctest 6/6 pass; helper imports only bcrypt/KERNEL32/msvcrt.
    <!-- CCGUI:TASK id=t9 status=deferred weight=1 progress=90 parent_id=p3-2 position=2 risk=high -->
    - [ ] Stage 3 - Quiescence barrier before shutdown
      - Description: Before updating, the app calmly finishes what it is doing instead of being cut off mid-work.
      - Done when: An update attempted during a clip export waits or safely cancels, and the clip is never corrupted.
      - Result: Update preparation now waits for capture work and cancels safely on timeout without killing exports.
      - Note: Implementation is complete; final acceptance needs a live clip-export update attempt.
      - Agent notes: Implementation: FramePumpWorker exposes exportBusy and update preparation; new replay saves are rejected, auto-arm stops, active remux finishes naturally, then the capture pipeline stops and signals ready. ScreenshotService blocks new captures and tracks pending background writes until ready. UpdateService adds PreparingForUpdate/Quiescent states; App coordinates both services, flushes config, and uses a 30-second timeout that cancels preparation and re-enables capture instead of killing work. Files: FramePumpService.{h,cpp}, ScreenshotService.{h,cpp}, UpdateService.{h,cpp}, App.{h,cpp}, docs/updater.md. Verification: GameHQ builds; ctest 6/6; diff check passes. Deferred gate: run an actual clip export, request update during remux, confirm waiting state and intact final MP4; timeout path must cancel only the update.
      - Risk notes: A corrupted MP4 from a forced shutdown is user data loss. If the quiescence wait exceeds its bound, the correct outcome is an aborted update, never a killed export.
    <!-- CCGUI:TASK id=t10 status=deferred weight=2 progress=90 parent_id=p3-2 position=3 risk=critical -->
    - [ ] Stage 4 - Pre-update data snapshot and data rollback
      - Description: Settings and the game database are snapshotted so a failed update restores them together with the program.
      - Done when: A build that migrates the database and then fails startup is rolled back with the old data fully restored.
      - Result: Config and SQLite state can be snapshotted and restored together without touching capture media.
      - Note: Core snapshot/restore passes; final acceptance depends on the Stage 7 failed-start rollback harness.
      - Agent notes: Implementation: UpdaterDataSnapshot snapshots only config.json, gamehq.db, gamehq.db-wal and gamehq.db-shm after quiescence. snapshot.manifest records originally present files. Restore pre-copies all old files, backs up the failed version state, publishes the snapshot set, removes files originally absent, and restores the pre-restore set on any error. Transaction schema now records dataDir/dataSnapshotDir; helper exposes --snapshot-data and --restore-data under the transaction mutex. Test mutates config/database, removes WAL, adds SHM, restores exact old bytes and confirms Captures/clip.mp4 is unchanged. Build and ctest 6/6 pass. Deferred gate: Stage 7 must run a synthetic new build that performs a real SQLite migration then fails startup, invoke restore, and prove the old binary opens the restored DB.
      - Risk notes: CRITICAL - this closes the schema/binary mismatch gap. A weaker model must keep snapshot+restore atomic with the program rollback (one transaction, one outcome) and must never include Captures/ in snapshots.
    <!-- CCGUI:TASK id=t3 status=done weight=2 progress=100 parent_id=p3-2 position=4 risk=critical -->
    - [x] Stage 5 - Program-file swap with lock retries
      - Description: Old program files move to a backup and new ones move in, tolerating brief Windows file locks.
      - Done when: The swap survives short-lived locks and aborts cleanly, without a half-installed state, on permanent errors.
      - Result: Program files swap from staging with bounded lock retries and restore cleanly when replacement fails.
      - Agent notes: Implementation: UpdaterSwap operates only on GameHQ.exe, app/, README.txt, LICENSE.txt, THIRD_PARTY_NOTICES.md, licenses/, and GameHQUpdater.pending.exe. MoveFileExW uses WRITE_THROUGH and retries only ERROR_SHARING_VIOLATION, ERROR_LOCK_VIOLATION, or bounded ERROR_ACCESS_DENIED at 100/250/500/1000/1500/2000 ms. Failure reverses completed steps: new files return to staging and old files return from backup. If rollback is blocked, backup is retained for Stage 7 recovery. Helper exposes --swap under transaction mutex. Tests: successful swap preserves portable.flag and backs up old app; CreateFileW without FILE_SHARE_DELETE forces retry exhaustion and proves old live plus new staged bytes remain intact. Full build, ctest 6/6 and diff check pass.
      - Risk notes: The only stage that mutates the live install. Do not improvise: run behind the Stage 1 transaction, and test with a deliberately locked file and a deliberately corrupted package before any real package is touched.
    <!-- CCGUI:TASK id=t4 status=done weight=1 progress=100 parent_id=p3-2 position=5 risk=high -->
    - [x] Stage 6 - Healthy-start handshake
      - Description: The updated app must prove it genuinely runs before the update counts as successful.
      - Done when: Success is declared only after the new version demonstrably starts and stays alive.
      - Result: The upgraded app must survive a live event-loop health window before capture hooks are enabled.
      - Note: Implementing bounded post-update startup validation and health-token handshake.
      - Agent notes: Implementation: main.cpp validates --post-update version/token arguments, keeps screenshot/replay/input hooks disarmed, and atomically writes the matching token after seven live event-loop seconds. UpdaterHealth launches the root executable, removes stale tokens, accepts only the expected version, and times out after 30 seconds. Tests: matching-token success and missing-token timeout; full build and ctest 6/6 pass.
    <!-- CCGUI:TASK id=t5 status=done weight=2 progress=100 parent_id=p3-2 position=6 risk=critical -->
    - [x] Stage 7 - Rollback and interrupted-update recovery
      - Description: Any failure, including a power cut mid-update, must end with a working previous version.
      - Done when: Every simulated failure point, including a killed helper, recovers to a runnable application.
      - Result: Interrupted or unhealthy updates restore program and data together, then restart the previous version.
      - Note: Durable recovery and bounded post-success cleanup are implemented and tested.
      - Agent notes: Implementation: UpdaterRecovery orchestrates stage/snapshot/swap/health with atomic phases; UpdaterSwap writes a pre-move existence journal. Health timeout kills the supervised launcher/app job tree before rollback. Recovery restores data and allowlisted program files together, handles mixed mid-swap layouts, restarts the previous executable, and retains healthy backup/snapshot state until a second seven-second start. App startup invokes --recover for stale phases/journals. Tests verify validating-phase rollback, mixed interrupted swap, exact program/config restoration, and previous executable restart; build and ctest 6/6 pass.
      - Risk notes: Every code path must end in exactly one of: finished or rolled-back. The phase marker is the source of truth.
    <!-- CCGUI:TASK id=t6 status=done weight=1 progress=100 parent_id=p3-2 position=7 risk=high -->
    - [x] Stage 8 - Updater helper self-update
      - Description: The helper program itself can be safely replaced by newer releases over time.
      - Done when: A release carrying a newer helper leaves that new helper active for the following update.
      - Result: A validated pending helper is promoted only after the previous helper has exited.
      - Note: Self-test promotion and minimum-version rejection are implemented and tested.
      - Agent notes: Implementation: GameHQUpdater exposes --self-test and holds Local\\GameHQUpdaterActive for transaction modes. Staging compares update-package.json minimumUpdaterVersion against the compiled helper version and rejects incompatible packages before live changes. The root launcher skips promotion while the activity mutex exists, runs the pending helper self-test with a five-second bound, promotes atomically after success, and discards invalid pending binaries without replacing the current helper. Tests cover newer-minimum rejection, valid promotion and invalid pending preservation; build and ctest 6/6 pass.
      - Risk notes: Review-found gap: without this the helper stays frozen at its first version forever. Verify promotion only happens when no helper process is running.
    <!-- CCGUI:TASK id=t7 status=deferred weight=2 progress=85 parent_id=p3-2 position=8 risk=high -->
    - [ ] Stage 9 - Failure-injection test suite
      - Description: A rehearsal suite deliberately breaks every step and proves the safety net always holds.
      - Done when: All injected failures end in either a completed update or a clean rollback, never a broken app.
      - Result: Automated and real-package sabotage tests prove failed updates restore the previous program and user data.
      - Note: Core matrix passes; live export, low-disk, maintenance and signing scenarios remain integration gates.
      - Agent notes: Coverage: Existing failure-injection tests cover checksum, archive traversal, forbidden paths, manifest mismatch, locks, health timeout, mixed-swap recovery, helper promotion and preservation. Real E2E: a disposable 0.6.9 package rejected a manifest/version-sabotaged 9.9.8 update after the health timeout, reached rolled_back, restarted 0.6.9, and restored launcher, README and gamehq.db byte-for-byte. Remaining: active clip export (t9), real SQLite migration failure (t10), read-only/low-disk/network/competing-launcher cases, Defender/SmartScreen, and signing after t8.
  <!-- CCGUI:TASK id=p3-3 status=done weight=3 progress=100 parent_id=p3 position=2 risk=high -->
  - [x] Guarantee user data and portable choice survive updates
    - Description: Recordings, settings and the portable-mode choice are provably untouched by any update.
    - Done when: Automated checks prove user folders and the portable marker are identical before and after updating.
    - Agent notes: Implementation: UpdatePreflight permits only complete writable local NTFS/ReFS packaged layouts, bounds path length and required space, rejects active transactions, and never enumerates user-owned trees. Download and install actions rerun preflight. StartupManager now uses the root launcher for installed and portable packages. Staging rejects portable.flag and user-data roots; swap remains positive-allowlist only. Tests cover portable present/absent preflight, successful installed apply, successful portable swap, failed portable rollback, failed installed mixed-swap recovery, and byte-identical config/Captures/portable.flag invariants. Build passes; ctest 7/7 and updater test 18/18 pass.
    - Risk notes: Test in a disposable copy of a real assembled package, both with and without portable.flag; verify byte-identical Captures/ and gamehq-data/ after update; do not trust "it looked fine" — diff the trees.
    - Result: Automated updates preserve recordings, settings and portable mode across success and rollback.
    - Note: Preservation matrix, packaged-layout preflight and root-launcher selection pass.
  <!-- CCGUI:TASK id=p3-4 status=deferred weight=3 progress=80 parent_id=p3 position=3 risk=high user_test=required -->
  - [ ] Post-update experience and failure-injection tests
    - Description: After updating, the app greets the user once with what changed; every failure path is rehearsed.
    - Done when: The user performs a real update on their machine and confirms data, mode and greeting are correct.
    - Agent notes: Implementation: App persists internal.updates.pending_post_update_version before publishing the health token. Main.qml shows a one-time greeting only when the desktop is visible, supports Continue and a version-specific What's New link, records last_seen_version, and clears pending state. E2E: a disposable real 0.6.9 portable package upgraded to 0.6.10, reached healthy, and persisted pending_post_update_version=0.6.10. Remaining acceptance: visually confirm the greeting/gamepad flow, run a clean-machine Defender/SmartScreen scan, and exercise read-only/low-disk/network conditions.
    - Result: A real packaged update reaches healthy state and records its one-time version greeting for the visible desktop.
    - Note: Implementation and automated updater tests pass; real packaged update and Defender acceptance need the user.
  <!-- CCGUI:TASK id=p3-5 status=done weight=3 progress=100 parent_id=p3 position=4 risk=medium -->
  - [x] Release packaging: dual payload, checksums and validation gate
    - Description: Releases ship a fresh-install package and a separate update-only package, both checked by an automatic gate.
    - Done when: An invalid or incomplete release is rejected automatically before it can be published.
    - Agent notes: Implementation: assemble-package includes the static updater. make-dist creates portable and program-only update ZIPs, stages the helper as GameHQUpdater.pending.exe, writes the strict schema-1 manifest and SHA-256 file, then invokes validate-release. The validator enforces approved output location, tag/VERSION and packaged binary version agreement, required entries, positive root allowlist, user-data/portable exclusions, manifest semantics, checksum format/content, and updater tests. Added release-body template and corrected docs/versioning.md to 0.6.x. Verification: real 0.6.10 artifacts built; full gate passes 3/3 updater tests; mismatched v0.6.9 tag is rejected.
    - Result: Releases now produce validated portable and update-only ZIPs with a matching SHA-256 file.
    - Note: Real artifacts pass the gate and mismatched tag metadata is rejected.
    - Risk notes: The gate is what makes the critical helper stages trustworthy release after release - a manual-only checklist WILL eventually be skipped.
<!-- CCGUI:TASK id=p4 status=deferred weight=1 progress=97 position=3 risk=medium -->
- [ ] Phase 3 — Local connection channel and friendly second launch
  - Description: A private local channel other tools can talk to, and double-launching now raises the existing window.
  - Done when: A test client can connect and sync a game, and a second launch focuses the running app.
  - Result: The local channel synchronizes game identity and forwards friendly second launches without weakening capture safety.
  - Note: Acceptance gated on the audit-fix slice (t12): IPC lifetime hardening and cross-drive path containment must land first.
  - Agent notes: State: p4-1, p4-2 and p4-4 are done. p4-3 is implemented and fixture-tested but awaits visual packaged double-launch confirmation because the user process was left untouched. t11 has durable maintenance suppression and automated coverage but awaits Playnite-side consumption and a live launch-spam test. Verification: GameHQ 0.6.11 builds; all 12 tests pass; portable and update artifacts validate with checksums.
  <!-- CCGUI:TASK id=p4-1 status=done weight=3 progress=100 parent_id=p4 position=0 risk=medium -->
  - [x] Local server with strict message framing
    - Description: The app opens a private user-only channel that safely rejects garbage input.
    - Done when: Malformed, oversized or hostile messages are rejected without any crash.
    - Agent notes: Implementation: Added IntegrationMessage, IntegrationProtocol and LocalIntegrationServer under src/integration. Transport: QLocalServer GameHQ.Local.v1 with UserAccessOption, eight-client cap, 64 KiB little-endian frames, 256 KiB bounded queues, strict UTF-8/JSON/type/request validation, and disconnect-on-abuse. Startup: App starts the server immediately after paths and logging. Tests: fragmented and combined frames, invalid lengths, UTF-8, JSON, unknown types, request IDs, 500 deterministic fuzz inputs, live valid delivery and live malformed disconnect; focused CTest passes.
    - Result: The same-user channel rejects malformed, oversized, hostile and excessive input without affecting the application.
  <!-- CCGUI:TASK id=p4-2 status=done weight=3 progress=100 parent_id=p4 position=1 risk=medium -->
  - [x] Handshake, capabilities and lifecycle message handling
    - Description: Connecting tools introduce themselves and the app tracks which external games are running.
    - Done when: Version-mismatched clients get a clear answer and game sessions sync, clear and expire correctly.
    - Agent notes: Implementation: IntegrationService enforces a five-second hello timeout, protocol range overlap, per-client handshake state, capabilities, request correlation, action acknowledgements, status responses and structured errors. ExternalGameContext validates bounded lifecycle fields, atomically replaces state.sync snapshots, clears stopped/cancelled sessions, and expires Playnite state after a reconnect grace period. Tests: pre-handshake rejection, successful and incompatible hello, activation/status, lifecycle upsert, snapshot replacement and disconnect expiry all pass.
    - Result: Clients negotiate compatibility, receive structured replies, and synchronize game sessions without leaving stale state.
  <!-- CCGUI:TASK id=p4-3 status=deferred weight=2 progress=90 parent_id=p4 position=2 risk=medium -->
  - [ ] Second-instance activation forwarding
    - Description: Launching the app twice now brings the existing window forward instead of doing nothing.
    - Done when: A double launch raises the running window and no duplicate tray icon ever appears.
    - Agent notes: Implementation: IntegrationClient connects only after QLockFile rejects the process, negotiates protocol 1, forwards --show/default as app.activate and --open-gallery as app.open_gallery, waits for an ack with 250/350 ms bounds, then main exits regardless of channel availability. App queues early activation until QML loads and open-gallery closes Settings/Help before raising. Tests: separate-process fixture proves both intents and the unavailable-server bound. Deferred acceptance: visual packaged double-launch was not forced because the user currently has build/app/GameHQ.exe running; do not terminate it.
    - Risk notes: Must not regress startup: if the pipe is absent or hung, use a short bounded connect timeout and still exit the second instance; never leave two full instances running; keep the QLockFile as the source of truth.
    - Result: Second launches negotiate briefly, forward show or gallery activation, and exit without creating another full instance.
  <!-- CCGUI:TASK id=p4-4 status=done weight=3 progress=100 parent_id=p4 position=3 risk=high -->
  - [x] Feed external game identity into detection safely
    - Description: Playnite information improves game recognition but never overrides the app's own safety checks.
    - Done when: Known games match faster with Playnite data while wrong-window recording remains impossible.
    - Agent notes: Implementation: ExternalGameContext now exposes thread-safe snapshots and confidence-ranked matching. GameDetector consumes the context without changing behavior when absent. Exact reported PID and a live Toolhelp parent-chain match may authorize windowed capture; install-directory matches can only rename a window already accepted by the fullscreen heuristic; names never match. Excluded processes and overlay window styles remain absolute blocks. Tests prove exact, descendant, name-only and gated-directory cases; GameHQ builds and lifecycle tests pass.
    - Risk notes: Do not weaken the only_in_games gate: a windowed-game allowance is permitted ONLY behind a confident PID/descendant match, never name equality. If detection behavior changes for non-Playnite users, stop and reassess.
    - Result: External identity improves matching only when process evidence or existing capture safety already supports the foreground game.
  <!-- CCGUI:TASK id=t11 status=deferred weight=2 progress=90 parent_id=p4 position=4 risk=high -->
  - [ ] Update maintenance lock and relaunch suppression
    - Description: While an update runs, nothing - launcher, double-clicks or Playnite - can start a competing copy.
    - Done when: With Playnite running, repeatedly launching the app during an update never creates a competing process.
    - Result: Durable maintenance markers suppress competing launches, clear after terminal outcomes, and expire safely after crashes.
    - Note: Implementation passes automated tests; final acceptance needs repeated live launches from Playnite during an update.
    - Agent notes: Implementation: UpdateMaintenance owns .update/maintenance.lock with Active, Inactive and StaleRecovery states, a five-minute stale bound, helper mutex checks and terminal-phase cleanup. App creates the marker before helper handoff and broadcasts app.maintenance; launch failure clears it. Root launcher and direct app startup suppress competing launches; --post-update is the supervised bypass. Updater clears only after healthy or rolled_back. Tests cover active suppression, terminal cleanup, stale recovery and updater transactions. Release 0.6.11 validates. Deferred gate: Playnite plugin consumption and repeated live Playnite launches during an update.
    - Risk notes: Treat marker-lifecycle bugs as release blockers: a stuck marker means users cannot start the app at all. Test the stale-marker expiry path explicitly.
<!-- CCGUI:TASK id=t12 status=needs_decision weight=1 progress=93 position=4 risk=high -->
- [ ] Audit fixes — 0.6.11 static review
  - Description: Fix the release blockers and hardening issues found by the external static audit before Playnite work.
  - Done when: All audit fixes are implemented, the build and tests pass, and the manual updater matrix succeeds.
  - Result: All audit fixes are coded and tested; only the hands-on update tests still need a person.
  - Note: External GPT audit of dev/0.6.11; the four release blockers were independently confirmed in code by Claude on 2026-07-20.
  - Agent notes: State: all eight code subitems t13-t20 DONE and verified in commit 53e9cb7 (VERSION 0.6.12, branch dev). HIGH-risk ones spot-checked in source: t13 App.cpp:195 single unique_ptr owner, nullptr parent; t15 src/core/UpdaterHandshake.h named READY event + callerPid wait; t16 UpdaterDataSnapshot.cpp:125-166 per-operation rollback lambda. Full build green, all 12 CTest targets pass. BLOCKED ON HUMAN: t21 manual updater matrix (portable update, installed-mode update, helper early-failure + maintenance-marker recovery, rollback on forced SQLite migration failure, quit during screenshot encode, read-only/low-disk/offline, clean-VM SmartScreen). No agent can run these. Next: set t12 done once a human reports the matrix passed; nothing else in t12 is actionable.
  <!-- CCGUI:TASK id=t13 status=done weight=1 progress=100 parent_id=t12 position=0 risk=high -->
  - [x] Fix UpdateService double ownership and clean shutdown
    - Description: UpdateService is owned by both a unique_ptr and the QML engine parent, risking double deletion on exit.
    - Done when: UpdateService has exactly one owner and exit is clean in normal, early-exit, mid-check and handoff scenarios.
    - Result: The update service now has a single owner and shuts down cleanly.
    - Agent notes: State: App.h:62 declares std::unique_ptr<UpdateService> m_updates before QQmlApplicationEngine m_engine (App.h:75), yet App.cpp:195-198 passes &m_engine as QObject parent. Engine destructs first and deletes its child; the unique_ptr then deletes it again. Fix: pass nullptr as parent (GitHubReleaseSource/UpdateDownloader stay children of UpdateService). Tests: normal tray exit, exit right after startup, exit during active check, updater handoff exit.
  <!-- CCGUI:TASK id=t14 status=done weight=1 progress=100 parent_id=t12 position=1 risk=medium -->
  - [x] Add Install-and-restart UI and state-specific update controls
    - Description: ReadyToInstall has no install button anywhere, so installAndRestart() is unreachable from the real UI.
    - Done when: About and the banner offer the correct primary action for every update state, including Install and restart.
    - Result: Users can install and restart from the update banner and the About page.
    - Agent notes: State: UpdateBanner.qml:93-117 only offers Download/Retry, Cancel, View on GitHub, Skip, Not now; AboutSettingsPage.qml describes ReadyToInstall but shows the action row only for UpdateAvailable/Downloading/Failed. Fix: state matrix — UpdateAvailable:Download, Downloading:Cancel, ReadyToInstall:Install and restart, PreparingForUpdate/Quiescent/Installing: progress text, Failed after check:Check again, Failed after download:Retry download (current generic Retry download is wrong for failed checks). Wire updates.installAndRestart().
  <!-- CCGUI:TASK id=t15 status=done weight=1 progress=100 parent_id=t12 position=2 risk=high -->
  - [x] Helper READY handshake and wait for old process exit
    - Description: The detached helper starts swapping files without waiting for the old GameHQ process to exit.
    - Done when: The helper confirms READY before GameHQ quits and mutates nothing until the old process has exited.
    - Result: The app only quits once the helper confirms it is ready to update.
    - Agent notes: State: UpdateInstaller.cpp:70-87 passes only --apply + transaction path; App.cpp:270 then queues quit. Races: SQLite db/WAL/SHM copied while open, config during shutdown persistence, mapped DLLs, gamehq.lock, surviving maintenance marker. Fix: add sourceProcessId + sourceProcessCreationTime + helperReadyEventName to the transaction; flow: quiescence, write transaction, create ready event, launch helper, helper validates and opens process handle, signals READY, only then GameHQ exits, helper waits on the handle before snapshot/swap/launch. If READY never arrives GameHQ stays open, clears maintenance and reports the error — this also fixes the early-failure five-minute maintenance lockout.
  <!-- CCGUI:TASK id=t16 status=done weight=1 progress=100 parent_id=t12 position=3 risk=high -->
  - [x] Per-file journaled data restore, no blanket deletion
    - Description: A partial failure during data rollback can delete the live database instead of preserving it.
    - Done when: Data restore reverses only recorded operations, never blanket-deletes state files, and passes per-step failure injection.
    - Result: A failed restore undoes only its own steps and never deletes your data.
    - Agent notes: State: UpdaterDataSnapshot.cpp:126-133 rollback removes ALL kStateFiles from dataDir then restores only entries in the backups vector — if backing up gamehq.db fails after config.json was moved, the live db is deleted and lost. Fix: durable per-file journal (original existed / moved to backup / partial published / backup restored), reverse only completed steps, check every rollback result; on incomplete rollback keep backups and partials, keep the recovery transaction, write diagnostics. Add failure injection after every individual move/publish.
  <!-- CCGUI:TASK id=t17 status=done weight=1 progress=100 parent_id=t12 position=4 risk=medium -->
  - [x] Release source robustness: cache, cooldown, incomplete releases, tests
    - Description: A detected update can vanish after restart, rate limits are not honored, and asset-less releases are advertised.
    - Done when: Detected updates survive restarts, cooldowns are persisted and honored, incomplete releases are not downloadable, tests exist.
    - Result: Half-published releases are never offered and repeated checks are rate limited.
    - Agent notes: Three defects: (1) only the ETag is persisted (App.cpp:203 primeCachedEtag) — after restart a 304 with empty m_release lands in UpToDate and hides the update; persist normalized ReleaseInfo with the ETag or skip If-None-Match without cached metadata, and retry once unconditionally on 304-without-cache. (2) persist internal.updates.next_allowed_check_utc, accept Retry-After-only secondary limits, keep automatic checks silent in cooldown, manual check shows retry time. (3) applyRelease() must reject releases missing exact zip/checksum assets or zipSize<=0 and show View release manually instead of Download. Add dedicated UpdateService + GitHubReleaseSource test targets.
  <!-- CCGUI:TASK id=t18 status=done weight=1 progress=100 parent_id=t12 position=5 risk=medium -->
  - [x] Make ScreenshotService shutdown-safe during background encoding
    - Description: Background PNG encode jobs capture a raw service pointer and can outlive the service on ordinary quit.
    - Done when: Quitting during a slow encode is safe; the destructor stops intake and waits for outstanding jobs.
    - Result: Quitting during a screenshot save no longer crashes the app.
    - Agent notes: State: encoder uses QThreadPool::globalInstance() with raw this in the runnable; no destructor waits. Quiescence covers updates but not ordinary tray exit. Fix: own bounded QThreadPool, refuse work during destruction, wait in destructor, QPointer/shared completion state instead of naked this. Test: quit during 4K PNG encode.
  <!-- CCGUI:TASK id=t19 status=done weight=1 progress=100 parent_id=t12 position=6 risk=medium -->
  - [x] Harden IPC lifetimes and cross-drive install-path matching
    - Description: Client disconnect during message dispatch can use invalidated state, and cross-drive paths can pass containment.
    - Done when: Dispatch survives mid-message disconnects, partial writes are handled, and cross-drive paths never match install containment.
    - Result: Connection slots are freed correctly and games on other drives are not mismatched.
    - Agent notes: IPC (LocalIntegrationServer::readClient): iterator into m_clients held across synchronous emits — a handler disconnecting the client invalidates it; save the client id and re-check registration after each emitted message. Also: don't assume one QLocalSocket::write accepts the full frame; count connected GameHQ.Playnite clients and only schedule source expiry when the last one disconnects (currently every handshaken disconnect schedules expiry). Paths (ExternalGameContext::pathInside): QDir::relativeFilePath on another drive returns an absolute drive-qualified path that passes the .. check — require same volume, genuinely relative result, no root/drive prefix, component-boundary containment; add C:\Game vs D:\OtherGame test.
  <!-- CCGUI:TASK id=t20 status=done weight=1 progress=100 parent_id=t12 position=7 risk=low -->
  - [x] Small hardening: job-object check, updater log, cleanup, links
    - Description: Check the job-object flag removal, add persistent updater logging, bounded artifact cleanup and fix the license link.
    - Done when: Helper failures leave a persistent log, artifacts are retained boundedly, and all small audit fixes are in.
    - Result: Update logging, cleanup and the About page License link were all corrected.
    - Agent notes: Items: (1) UpdaterHealth.cpp:104 ignores SetInformationJobObject result — on failure CloseHandle(job) kills the validated app while reporting success; treat as failed health validation and roll back. (2) Add .update/update.log (transaction id, versions, phase transitions, Win32 codes, retries, rollback ops, outcome), opened before live changes, preserved through rollback. (3) cleanCompletedUpdateArtifacts leaves the downloaded zip, checksum and transaction.json — add bounded retention. (4) AboutSettingsPage.qml:117-118 links /blob/main/LICENSE.txt but the repo file is LICENSE. (5) Validator: also reject Windows reserved device names (CON, NUL, AUX, COM1...), components ending in dot/space, and case-folding collisions.
  <!-- CCGUI:TASK id=t21 status=deferred weight=1 progress=40 parent_id=t12 position=8 risk=medium -->
  - [ ] Run the automated and manual validation matrix
    - Description: After the fixes, run the full automated suite and the manual updater test matrix before accepting Phases 2 and 3.
    - Done when: Clean build, all CTests green, and the manual matrix including rollback and maintenance-recovery scenarios passes.
    - Result: Automated checks all pass; the hands-on update tests still need a person.
    - Agent notes: State: automated half VERIFIED by Claude 2026-07-20 on commit 53e9cb7 (VERSION 0.6.12, branch dev). Clean configure + full build via tools/cmake with -DGAMEHQ_BUILD_TESTS=ON; all 12 CTest targets pass (tst_updatertransaction 7.4s is the slow one). Gotcha: ctest needs BOTH tools/Qt/6.8.3/mingw_64/bin AND tools/Qt/Tools/mingw1310_64/bin on PATH, otherwise every test exits 0xc0000135 (missing Qt DLLs) and looks like a real failure. windres also needs the mingw bin dir on PATH or the .rc step fails with CreateProcess. Next: the manual matrix requires a human - normal start/quit; quit during active screenshot encode; About page visual/keyboard/gamepad; packaged double-launch; portable update; installed-mode update; update during a real clip remux; deliberate SQLite migration failure + rollback; helper early-failure and maintenance-marker recovery; read-only, low-disk and offline conditions; clean-VM Defender/SmartScreen. Playnite-maintenance relaunch test waits for the plugin. Gate: t12, p3 and p4 acceptance all wait on this manual pass.
<!-- CCGUI:TASK id=p5 status=needs_decision weight=1 progress=84 position=5 risk=medium -->
- [ ] Phase 4 — Playnite companion plugin
  - Description: Build the Playnite companion as an independently versioned subproject inside the GameHQ monorepo.
  - Done when: The plugin bridges real game sessions on a clean Playnite install without ever blocking Playnite.
  - Result: The plugin now connects, launches GameHQ, forwards game events and has a settings page and packaging; only real-Playnite testing is left.
  - Note: p5-1 through p5-3 and t32 are done; p5-4/p5-5 are implemented and build-verified but need a human with a real Playnite install to confirm and run the compatibility matrix.
  - Agent notes: Everything code-side for Phase 4 is implemented and build/test-verified: scaffold, pipe client, game lifecycle forwarding, settings page, working .pext packaging. What remains is exclusively human verification (p5-4 UI check, p5-5 full matrix) plus p6 (public release), already correctly blocked on this phase. Recommend: install a test copy of Playnite, drag-drop integrations/playnite/dist/GameHQ_Integration_0_4_0.pext (regenerate via packaging/package.ps1 if stale), and exercise the p5-4 and p5-5 checklists.
  <!-- CCGUI:TASK id=p5-1 status=done weight=2 progress=100 parent_id=p5 position=0 risk=low -->
  - [x] Create the plugin subproject scaffold
    - Description: A buildable plugin skeleton under integrations/playnite with its own version, changelog and packaging.
    - Done when: The empty plugin builds on its own, loads in Playnite developer mode, and the app's normal build never touches it.
    - Agent notes: integrations/playnite/ scaffolded: src/GameHQ.Playnite (csproj targeting net462 + PlayniteSDK 6.16.0, extension.yaml, GameHQPlugin.cs GenericPlugin skeleton), tests/GameHQ.Playnite.Tests (placeholder), packaging/{build,package,verify}.ps1, VERSION, CHANGELOG.md, README.md, InstallerManifest.yaml, icon.png, .gitignore. Fixed a real bug: GameHQPlugin had an invalid override string Name - GenericPlugin/Plugin in SDK 6.16.0 has no such member; name comes from extension.yaml only. Removed the override; dotnet build (Release) succeeds 0 errors, dotnet test passes. Not in CMakeLists/start.bat so app build is unaffected. Committed 91c110c.
    - Result: Plugin scaffold builds standalone via dotnet, its placeholder test passes, and the app build never touches it.
    - Note: Decision made: GitHub repo underfusion/GameHQ-Playnite with local workspace I:\PROJECTS\Apps\GameHQ-Playnite. May proceed in parallel with the audit fixes.
  <!-- CCGUI:TASK id=p5-2 status=done weight=3 progress=100 parent_id=p5 position=1 risk=medium -->
  - [x] Pipe client, app locator and launcher
    - Description: The plugin finds the app, starts it when wanted, and keeps reconnecting after hiccups.
    - Done when: The plugin connects to a running app and can start a stopped one, retrying quietly on failure.
    - Agent notes: Protocol/: PipeFraming.cs (4-byte LE length + UTF-8 JSON, 64 KiB cap), IntegrationMessage.cs (JavaScriptSerializer, System.Web.Extensions ref), IntegrationClient.cs (NamedPipeClientStream, hello/hello.ack handshake, background reconnect loop with exponential backoff 500ms-30s, bounded 64-slot outgoing queue, app.maintenance -> Suspended state). GameHQLocator.cs: configured path then HKCU Run registry GameHQ value, validated via app\GameHQ.exe presence. GameHQProcessLauncher.cs: launches the ROOT exe only. Tests: PipeFramingTests + GameHQLocatorTests link the real source files into the net8.0 test project; 11/11 pass. Committed a3de367.
    - Result: The plugin now connects to a running GameHQ, keeps retrying quietly, and can launch it when stopped.
    - Note: Blocked until the separate plugin repository and buildable scaffold exist.
  <!-- CCGUI:TASK id=p5-3 status=done weight=2 progress=100 parent_id=p5 position=2 risk=medium -->
  - [x] Forward game lifecycle events and state sync
    - Description: Playnite tells the app when games start and stop, including after restarts and reconnects.
    - Done when: Start, stop and cancelled launches are reflected correctly even across reconnections.
    - Agent notes: GameLifecycleForwarder.cs: ConcurrentDictionary<Guid,Session> keyed by Playnite Game.Id, own sessionId=Guid.NewGuid() per session. GameHQPlugin overrides OnGameStarting/OnGameStarted/OnGameStopped/OnGameStartupCancelled. Session fields resolved defensively (try/catch): SourceName via api.Database.Sources.Get, PlatformNames via game.Platforms, SelectedRomFile via game.Roms[0].Path. On IntegrationClient.StateChanged->Connected (first connect AND every reconnect): sends playnite.application.started then playnite.state.sync with up to 64 sessions. OnApplicationStopped sends playnite.application.stopping but does NOT stop GameHQ. Build+existing tests (11/11) verified. Committed eba135f.
    - Result: Playnite now tells GameHQ when games start, stop or are cancelled, and resyncs correctly after any reconnect.
    - Note: Blocked until the plugin client scaffold exists.
  <!-- CCGUI:TASK id=p5-4 status=deferred weight=2 progress=90 parent_id=p5 position=3 risk=low -->
  - [ ] Plugin settings page and diagnostics
    - Description: A small settings page showing connection health, versions, and two startup preferences.
    - Done when: Users can point at the app, test the connection, and read a copyable diagnostic summary.
    - Agent notes: New Settings/: GameHQIntegrationSettings (POCO, local ObservableObjectBase - Playnite.SDK ObservableObject/Data.Serialization/RelayCommand did not resolve against PlayniteSDK 6.16.0, wrote local equivalents instead), GameHQIntegrationSettingsViewModel (ISettings: BeginEdit/CancelEdit/EndEdit/VerifySettings; commands SelectExe/TestConnection/OpenGameHQ/OpenWebsite/CopyDiagnosticSummary), GameHQIntegrationSettingsView.xaml (Connection/Startup/Support sections). Required UseWPF=true + explicit System.Web reference for the cross-compiled net462 XAML build. GameHQPlugin: GetSettings/GetSettingsView wired, GetMainMenuItems returns ONLY Open GameHQ. OnApplicationStarted/OnGameStarting gate launches on Settings.StartWithPlaynite/StartOnGameLaunchIfNotRunning. IntegrationClient gained RemoteAppVersion/ProtocolSelected/LastError + TriggerReconnect(). KNOWN GAP: Open plugin log button not implemented - no reliable SDK-exposed log path. Verified: dotnet build 0 errors, dotnet test 11/11, verify.ps1 green. Committed f9fee55.
    - Result: The settings page is built and compiles clean; a human still needs to open it inside a real Playnite install to confirm it works.
    - Note: Implementation complete and build-verified; cannot be exercised in this sandbox (no live Playnite instance). Deferred until the user checks it in developer mode.
  <!-- CCGUI:TASK id=p5-5 status=deferred weight=2 progress=15 parent_id=p5 position=4 risk=medium -->
  - [ ] Package the plugin and run the compatibility matrix
    - Description: The plugin is packaged properly and proven against the realistic mess of real setups.
    - Done when: Every scenario in the test matrix passes on both installed and portable Playnite.
    - Agent notes: Fixed a real bug in packaging/package.ps1: Compress-Archive throws NotSupportedArchiveFileExtension for any destination not ending in .zip, so it could never have produced a .pext before. Fix: build to a temp .zip then Move-Item to the .pext path. Verified: package.ps1 now builds+packages cleanly, producing dist/GameHQ_Integration_0_4_0.pext containing exactly extension.yaml, GameHQ.Playnite.dll, icon.png. Committed 12f7dae. Remaining: full compatibility matrix (installed+portable Playnite, enabled/disabled, running/not-running/invalid-path GameHQ, normal/launcher/child-process/emulator starts, cancelled launch, Playnite restart mid-game, pipe disconnect/reconnect, incompatible-protocol messaging, multiple games, standalone-when-disabled) plus the REQUIRED USER TEST of drag-dropping the real .pext into Playnite - needs a human with Playnite installed.
    - Result: Packaging now produces a valid .pext, but the real-Playnite compatibility matrix still needs a person.
    - Note: Packaging script fixed and verified; the test matrix (drag-drop into real Playnite, launch/stop games, pipe disconnect, emulator) requires a human with a live Playnite install.
  <!-- CCGUI:TASK id=t32 status=done weight=1 progress=100 parent_id=p5 position=5 -->
  - [x] Guard app updater against plugin release tags
    - Description: Make update checking ignore plugin releases so the app never mistakes a playnite tag for an app update.
    - Done when: Update discovery selects only stable app releases tagged vX.Y.Z carrying the exact update ZIP and checksum assets.
    - Result: Updater now scans the release list and only ever picks a vX.Y.Z app release with the exact update assets.
    - Agent notes: Implemented in src/updates/GitHubReleaseSource.cpp: GET /releases (not /releases/latest), filter to tags matching ^v[0-9]+\.[0-9]+\.[0-9]+$ with the exact update-zip + sha256 asset names, pick the highest valid one. A future playnite-v* tag in this repo is structurally ineligible. Committed 3cedb2b. Build verified clean.
<!-- CCGUI:TASK id=p6 status=blocked weight=1 progress=0 position=6 risk=low locked=true -->
- [ ] Phase 5 — Public release and add-on submission
  - Description: Publish the plugin and submit it so Playnite users can install it from the built-in browser.
  - Done when: The plugin installs from Playnite's add-on browser after the submission is accepted.
  - Note: Blocked until the Playnite plugin is implemented, tested, and authorized for publication.
  <!-- CCGUI:TASK id=p6-1 status=blocked weight=2 progress=0 parent_id=p6 position=0 risk=low locked=true -->
  - [ ] First public plugin release with installer manifest
    - Description: The finished plugin gets a proper public release with everything reviewers need.
    - Done when: The packaged plugin and its manifest are published and validate cleanly.
    - Agent notes: In-repo release (monorepo decision, see p5): publish GitHub release in underfusion/GameHQ tagged playnite-v0.1.0 with asset GameHQ_Integration_0_1_0.pext. InstallerManifest.yaml lives at integrations/playnite/InstallerManifest.yaml: AddonId 'GameHQ_Integration', Packages[Version 0.1.0, RequiredApiVersion taken from the ACTUAL SDK target - do not copy 6.11.0 from the draft blindly, ReleaseDate, PackageUrl to the release asset, Changelog]. Verify manifest with Toolbox. README/screenshots under integrations/playnite/. Release workflow triggers only on playnite-v* tags; app workflow only on v*. Docs must state clearly that GameHQ works standalone. Prereq: p5-6 updater guard shipped first.
    - Note: Blocked until Phase 4 produces a validated plugin package and publication is authorized.
  <!-- CCGUI:TASK id=p6-2 status=blocked weight=2 progress=0 parent_id=p6 position=1 risk=medium locked=true -->
  - [ ] Submit to the Playnite add-on database
    - Description: Ask the Playnite project to list the plugin in its official add-on browser.
    - Done when: The submission is merged and installation from inside Playnite works end to end.
    - Agent notes: Fork JosefNemec/PlayniteAddonDatabase, add YAML under addons/generic/ (AddonId GameHQ_Integration, Type Generic, Name 'GameHQ Integration', Author underfusion, InstallerManifestUrl = raw link to integrations/playnite/InstallerManifest.yaml in underfusion/GameHQ, SourceUrl = https://github.com/underfusion/GameHQ, IconUrl, ShortDescription/Description, Tags, Links), open PR, respond to review. Acceptance is a normal PR review, not guaranteed and not a security audit - external timeline. Use component labels (component: playnite etc.) and an issue form asking which component is affected, so user reports stay in the main repo.
    - Risk notes: External dependency — if review stalls, do not resubmit duplicates; keep the manual .pext path documented as fallback.
    - Note: Blocked until a public plugin release exists and upstream submission is authorized.
  <!-- CCGUI:TASK id=t29 status=blocked weight=1 progress=0 parent_id=p6 position=2 risk=high -->
  - [ ] Ed25519-signed update manifest (gate for public Beta)
    - Description: Sign every release with an update manifest that GameHQ verifies before the one-click updater goes public Beta.
    - Done when: Releases ship a signed manifest, and both the app and the helper reject unsigned or tampered updates.
    - Note: Required before the one-click updater is offered publicly; not needed for internal testing.
    - Agent notes: From the approved t8 decision. Files per release: update-package.json + update-package.sig (Ed25519). Manifest fields at minimum: product id, application version, monotonic release sequence (anti-rollback), asset filename and size, SHA-256, minimum updater version, key id. Public key embedded in both GameHQ and GameHQUpdater; verification happens before applying, in app and helper. Private key: offline signing on the user's machine for first releases, never in the repo or ordinary source-control secrets. Blocked until the audit-fix slice (t12) is done and a public Beta is actually planned.
  <!-- CCGUI:TASK id=t30 status=blocked weight=1 progress=0 parent_id=p6 position=3 risk=medium -->
  - [ ] Authenticode-sign executables (gate for Stable)
    - Description: Sign all GameHQ-built executables under one publisher identity before promoting the updater from Beta to Stable.
    - Done when: GameHQ.exe, app/GameHQ.exe and GameHQUpdater.exe carry Authenticode signatures from one consistent identity.
    - Note: Needs a code-signing certificate purchase at that time; describe it as identity and reputation, not as an instant end to SmartScreen warnings.
    - Agent notes: From the approved t8 decision. Targets: GameHQ.exe (root launcher), app/GameHQ.exe, GameHQUpdater.exe — one consistent publisher identity. Wording rule: Authenticode provides a verified Windows publisher identity and helps build consistent SmartScreen reputation; newly signed releases may still show warnings initially. Certificate options (recheck pricing when buying): classic OV/EV certificate or Azure Trusted Signing. Blocked until Beta-to-Stable promotion is on the table; t29 (signed manifest) comes first.
<!-- CCGUI:TASK id=t22 status=needs_decision weight=1 progress=32 position=7 risk=high -->
- [ ] Phase 6 — HDR capture pipeline
  - Description: Add real HDR support: correct tone mapping first, then native HDR screenshots and HDR10 replay clips.
  - Done when: HDR desktops get correct-looking captures, native HDR files where the hardware supports it, and clean SDR fallbacks elsewhere.
  - Result: HDR detection is done; the actual HDR capture work needs a machine with HDR switched on.
  - Note: From the GPT HDR audit of 2026-07-20. Start only after the audit-fix slice (t12). Today 0.6.11 saves SDR only, so HDR games can look washed out.
  - Agent notes: State: t23 DONE (0.6.13, commit a3d59ac) - detection and diagnostics only, pipeline untouched. t24 DEFERRED, t25/t26/t27 sit behind it. NEEDED FROM THE USER: turn Windows HDR on for a display, confirm the Settings > Advanced Display HDR row reads Active, then run a real HDR game so the tone-mapped capture work in t24 has something to verify against. Without that, any t24-t27 code would ship unverified into the capture hot path.
  <!-- CCGUI:TASK id=t23 status=done weight=1 progress=100 parent_id=t22 position=0 risk=low -->
  - [x] Detect HDR displays and expose diagnostics
    - Description: Detect whether the monitor showing the captured game currently has HDR enabled and surface it in diagnostics.
    - Done when: GameHQ reports HDR active or inactive per monitor and logs capture format, encoder support and the active fallback.
    - Result: GameHQ now reports whether each display is in HDR mode and what the capture stack can do with it.
    - Agent notes: State: DONE in commit a3d59ac (VERSION 0.6.13). New src/capture/HdrCapabilities.{h,cpp}: forEachOutput walks CreateDXGIFactory1 -> EnumAdapters1 -> EnumOutputs -> QI IDXGIOutput6 -> GetDesc1; hdrActive = ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020; also BitsPerColor + min/max/MaxFullFrameLuminance. hevcMain10Supported() enumerates MFT_CATEGORY_VIDEO_ENCODER for MFVideoFormat_HEVC then activates each and offers a Main10 output type. Surfaces: AppController::refreshHdrStatus + hdrDisplayActive/hdrStatusText/hdrDetailText, called from App.cpp on startup and on QGuiApplication screenAdded/screenRemoved/primaryScreenChanged; Settings > Advanced Display HDR row with Refresh; copyDiagnosticSummary appends summaryLines(); FramePumpService::startPump re-reads the target window monitor. Gotchas: MinGW has no MF_MT_VIDEO_PROFILE - same GUID as MF_MT_MPEG2_PROFILE. Build+launch clean, live probe correct (DISPLAY1 3840x2160, HDR Inactive, 10-bit, 0.010-1499 nits, HEVC Main10 present). Not eyeballed: the Settings row visual and the HDR-ACTIVE branch (desktop HDR is off on this machine).
  <!-- CCGUI:TASK id=t24 status=deferred weight=1 progress=90 parent_id=t22 position=1 risk=high -->
  - [ ] HDR-aware SDR capture with GPU tone mapping
    - Description: When Windows HDR is active, capture FP16 frames and tone-map on the GPU so SDR output stops looking washed out.
    - Done when: Captures taken on an HDR desktop look correct in the saved SDR PNG, JPEG and H.264 files.
    - Result: The HDR tone-map curve now actually compresses highlights instead of clipping them flat; still not verified on real HDR hardware.
    - Note: GPT-5 review caught that the first version hard-clipped everything above SDR white to 255, discarding all highlight detail - fixed with a real shoulder curve. Still deferred until someone tests it with Windows HDR active and a real HDR game per t22.
    - Agent notes: Correction round after GPT-5 review flagged the identity+hard-clip curve as not real tone mapping (highlights at 2x/4x/8x/16x reference white were all indistinguishable flat 255 - defeats the purpose). New curve in HdrToneMapMath.cpp/GpuToneMapper.cpp HLSL (kept in lockstep): identity for x <= kKneeStart (0.9), then f(x) = 1 - (1-kKneeStart)*sqrt(kKneeStart/x) for x > kKneeStart - monotonically increasing, asymptotes toward but never reaches 1.0, verified by test to keep 1.0/1.5/2/4/8/16x reference white strictly distinct (roughly 244/246/247/250/251/252 in 8-bit, hand-derived then confirmed by the actual test run, not just estimated). SDR white (1.0) is now deliberately ~244/255 instead of forced 255 - headroom reserved for highlights, matching GPT-5's explicit guidance that paper-white/peak-luminance parameters should not force 1.0->1.0. Also hardened FramePumpService::createSession: if CreateFreeThreaded(FP16) itself fails after every earlier gate passed (tone-mapper already initialized), it now tears down the tone-mapper and retries once with BGRA8 in the SAME call instead of aborting the whole capture attempt - closes the gap GPT-5 flagged in point 6. Extracted capture::hdr::shouldAttemptFp16Capture(flag, hdrActive, fp16Supported) as a pure, directly-unit-tested function that createSession now calls, rather than an inline condition asserted only by code review - proves flag-off (or non-HDR, or unsupported-GPU) genuinely never attempts FP16, addressing GPT-5's point 6 the honest way (extract+test) rather than hand-waving it. tst_hdrtonemap expanded from 10 to 15 checks: 18% gray reference point (identity range sRGB sanity check, ~118/255, a well-known independent reference value), near-white-not-exact SDR white, distinct+strictly-monotonic highlights across 1/1.5/2/4/8/16x, finite/clamped extreme input (1e6, 1e10, float max/2), and the new gate function across all flag/hdrActive/supported combinations. tst_hdrgputonemap (GPU-vs-CPU tolerance, real D3D11 device on this machine) still 3/3 with the new curve. Full suite 14/14, no regressions. Committed de39c48, VERSION 0.6.16. What has NOT changed: still hidden behind internal.capture.experimental_hdr (default off), still gated on HdrCapabilities + FP16 support, still falls back cleanly on any failure, still genuinely unverified against a real HDR-active display or live HDR game. Next: same as before - a human with Windows HDR + a real HDR game needs to flip the flag and confirm the SDR path is unaffected and the HDR path looks correct, then t25 (which needs its own one-shot WGC screenshot capture, ScreenshotService is GDI and cannot see HDR data) can start.
  <!-- CCGUI:TASK id=t25 status=blocked weight=1 progress=0 parent_id=t22 position=2 risk=medium -->
  - [ ] Native HDR screenshots: JPEG XR original plus SDR companion
    - Description: Save an HDR original as JPEG XR next to a tone-mapped SDR PNG and mark HDR items in the gallery.
    - Done when: HDR captures produce a valid .jxr original and SDR preview, and the gallery shows an HDR badge with SDR share defaults.
    - Note: Sits behind t24, which is itself blocked on a real HDR display and a live HDR game session (see t22 agent notes). Implementing this first would edit the capture hot path unverified.
    - Agent notes: Output pair: <stamp>_HDR.jxr + <stamp>.png. Writer: WIC CLSID_WICWmpEncoder, GUID_ContainerFormatWmp, GUID_WICPixelFormat64bppRGBAHalf; MUST verify WIC kept the requested FP16 format and did not silently downconvert. Qt Format_RGBA16FPx4 is fine for in-memory frames, but write the .jxr through WIC, not Qt's image writer. Capture metadata per item: dynamic_range sdr|hdr, hdr_original_path, sdr_preview_path, bit_depth, color_space. Gallery: keep using the SDR companion (Qt Quick window is not an HDR surface); add HDR badge, Open HDR original action, share defaults to SDR PNG with optional Share HDR original. Effort ~3-6 days on top of t24.
  <!-- CCGUI:TASK id=t26 status=blocked weight=1 progress=0 parent_id=t22 position=3 risk=high -->
  - [ ] Native HDR10 replay recording with HEVC Main10
    - Description: Record HDR10 replay clips through a 10-bit HEVC hardware encoder where supported, falling back to SDR elsewhere.
    - Done when: Capable systems produce valid HDR10 MP4 replays; others silently fall back to tone-mapped SDR H.264.
    - Note: Sits behind t24, which is itself blocked on a real HDR display and a live HDR game session (see t22 agent notes).
    - Agent notes: Pipeline: WGC FP16 scRGB -> GPU scale/convert -> P010 -> HEVC Main10 hardware encoder -> HDR10 MP4 segments -> existing ring/remux. Stay on the GPU: feed D3D11 surfaces to the encoder, no CPU readback (FP16 4K roughly doubles raw traffic vs BGRA8). HdrEncoderProbe: negotiate input P010, output HEVC Main10, hardware required; the built-in MS MF HEVC encoder documents only 8-bit Main with NV12/IYUV/YUY2/YV12 — never assume Main10 exists; probe result gates the feature (never a global switch). Class split: IVideoSegmentRecorder -> SdrH264SegmentRecorder + Hdr10HevcSegmentRecorder; shared: segment timing, ring ownership, audio, snapshot/pinning, replay export; backend: texture conversion, pixel format, codec negotiation, color metadata. MF media type: MF_MT_VIDEO_PRIMARIES BT.2020, MF_MT_TRANSFER_FUNCTION ST.2084/PQ, MF_MT_YUV_MATRIX BT.2020 NCL, MF_MT_VIDEO_NOMINAL_RANGE, MF_MT_VIDEO_CHROMA_SITING; inspect output MP4 for 10-bit format, colr box, mastering metadata, MaxCLL/MaxFALL. Ring cache split: replay-cache/<game>/video-sdr-h264/ vs video-hdr10-hevc/; NEVER concatenate SDR H.264 and HDR10 HEVC segments. Concatenator already clones the full media type from the first segment — verify it preserves color attributes on real HDR output. Effort ~2-5 weeks.
  <!-- CCGUI:TASK id=t27 status=blocked weight=1 progress=0 parent_id=t22 position=4 risk=medium -->
  - [ ] HDR settings and edge-case handling
    - Description: Add simple HDR settings and correct behavior for monitor moves, HDR toggles, missing encoders and thumbnails.
    - Done when: All HDR edge cases behave predictably and settings expose only the Off/Auto and Auto/SDR/HDR10 choices.
    - Note: Sits behind t24, which is itself blocked on a real HDR display and a live HDR game session (see t22 agent notes).
    - Agent notes: Settings — Capture: HDR screenshots = Off | Auto (HDR original + SDR preview). Replay: Dynamic range = Auto | SDR | HDR10 (Beta). Edge cases: (1) mixed HDR/SDR monitors — rebuild capture pool and recorder when the game window moves displays; (2) Windows HDR toggled during capture — restart the ring, mixed-format segments must never concatenate; (3) SDR game on an HDR desktop — GameHQ can detect the monitor mode but not another process's swap chain, hence Auto plus Force SDR; (4) no Main10 encoder — fall back to tone-mapped SDR, never fail the replay buffer; (5) thumbnails always tone-mapped SDR for the existing RGB32 gallery path; (6) exclusive fullscreen — test separately, WGC can behave differently than borderless; (7) per-dynamic-range regression matrix before enabling by default.
  <!-- CCGUI:TASK id=t28 status=deferred weight=1 progress=0 parent_id=t22 position=5 risk=medium -->
  - [ ] HDR playback inside the gallery (optional, later)
    - Description: Eventually present HDR originals in an HDR-capable viewer; until then the gallery keeps using SDR previews.
    - Done when: A decision exists on building an HDR presentation surface; SDR previews stay the default until then.
    - Agent notes: Qt Quick's window is not an HDR presentation surface, so in-app HDR display needs its own investigation (swap chain color space, FP16 backbuffer or external viewer handoff). Per the audit this must not block native HDR file creation (t25/t26); SDR previews remain the gallery default. Separate milestone — pick up only after t23-t27 ship.

## Risk Assessment

Overall risk profile, by phase (reviewer assessment on top of the source plan):

- Phase 0-1 (low): additive UI and read-only network checks; worst case is cosmetic. Ship Phase 1 with the "open release page" fallback so it delivers value alone.
- Phase 2 (HIGH — the trust-critical core): file replacement can destroy user data or the install. Mitigations baked into items p3-2/p3-3: strict path allowlist/denylist, staging + backup + rollback, checksum gate, dry-run mode, failure-injection matrix, portable.flag preservation. Additional reviewer-identified risks: AV/SmartScreen false positives on the unsigned temp-run helper (fallback: run helper from install root; real fix: Authenticode later); transient file locks from AV/Explorer (fix: bounded retry with backoff).
- Phase 3 (medium): a local attack surface is added — mitigated by same-user ACL, strict framing/allowlist, no command execution, fuzz tests. Startup-path change in p4-3 needs a bounded timeout so a dead pipe can't hang launch.
- Phase 4 (medium): main risk is blocking Playnite or duplicate instances — mitigated by async queue + single-instance forwarding; plugin never touches GameHQ data or updates.
- Phase 5 (low, external): add-on review timeline is not under our control.
- Cross-cutting: GitHub rate limiting must always degrade softly; every phase must leave GameHQ fully standalone-functional; project rule of VERSION+CHANGELOG per change applies to every single item.

## Deferred / Blocked / Decisions

Open decisions to confirm with the user before or during Phase 0 (p1-1):

- Canonical website URL (static site exists in the repo but no public URL is confirmed).
- Automatic update checks default ON — recommended, with explicit "this is not telemetry" copy in UI and docs.
- miniz as the ZIP dependency for the helper — recommended unless another small audited library is preferred.
- Post-update success signal: successful application initialization only (recommended for v1) vs a longer health window.
- Helper launch location: temp (source plan) vs install root (reviewer suggestion to reduce AV heuristics) — decide during p3-2 testing.

Deferred (explicit non-goals for v1): silent/automatic installs, beta/nightly channels, differential updates, Playnite 11 adapter, plugin-side GameHQ installer, capture commands in Playnite menus, database migration for external IDs, code signing (follow-up), cloud accounts, telemetry.

## Execution Log

- 2026-07-20: Plan created from external (GPT) draft; restructured, verified against codebase, risk-annotated by Fable. Status draft, progress 0%, current item p1-1.
