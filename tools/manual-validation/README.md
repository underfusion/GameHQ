# GameHQ manual-validation harness

These scripts prepare disposable updater and installer test fixtures without
touching the working GameHQ checkout or the user's real installation.

## Safety model

- `WorkspaceRoot` must be an explicit absolute path outside the GameHQ project,
  drive roots, profile root, Windows, and Program Files.
- Every run receives a unique directory and ownership marker. Scripts refuse to
  operate on paths without that marker.
- The harness never deletes a run, terminates applications, edits registry
  state, simulates disk exhaustion, or launches an updater automatically.
- The portable fixture contains capture and data sentinels. Comparison treats
  every capture and the data sentinel as protected; ordinary config, database,
  cache, and log changes are reported but require scenario-specific review.

## Prepare a run

```powershell
$run = .\tools\manual-validation\Prepare-ValidationWorkspace.ps1 `
  -WorkspaceRoot 'I:\GameHQ-validation' `
  -PortablePackage '.\dist\releases\GameHQ-0.6.24-win64-portable.zip' `
  -UpdatePackage '.\dist\releases\GameHQ-0.6.24-win64-update.zip' `
  -SetupPackage '.\dist\releases\GameHQ-0.6.24-win64-setup.exe'
```

The command copies and hashes the artifacts, extracts a portable fixture, adds
sentinels, and writes `snapshots\before.json`.

## Run a scenario

Use only the generated `$run\portable` fixture. Human-only scenarios such as a
live replay remux, forced process interruption, SmartScreen, and Defender remain
manual. Never point an updater or installer experiment at `build`, `dist`, the
repository, or a real user profile.

## Compare and collect evidence

```powershell
.\tools\manual-validation\Compare-ValidationState.ps1 `
  -RunRoot $run `
  -BeforeSnapshot "$run\snapshots\before.json" `
  -OutputPath "$run\snapshots\comparison.json"

.\tools\manual-validation\Collect-ValidationEvidence.ps1 -RunRoot $run
```

The comparison exits with code 2 if protected data changed or disappeared. The
evidence bundle records artifact hashes, OS information, snapshots, comparison,
and fixture logs. Runs are intentionally retained until the tester reviews and
removes the exact disposable directory.
