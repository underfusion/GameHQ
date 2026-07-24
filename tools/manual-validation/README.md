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

## HDR hardware validation

The HDR helper removes the first-launch and manual JSON-edit steps. It creates a
disposable portable run, enables the experimental HDR replay path and system
audio, shortens the replay ring to 30 seconds, and writes a focused checklist:

```powershell
$run = .\tools\manual-validation\Prepare-HdrValidation.ps1 `
  -WorkspaceRoot 'I:\GameHQ-HDR-validation' `
  -OpenDisplaySettings `
  -Launch
```

Close every normal GameHQ instance before using `-Launch`; otherwise the normal
single-instance forwarding behavior would activate the wrong profile.

After saving and visually checking one replay from a real HDR game, close the
fixture and generate the machine-readable report:

```powershell
.\tools\manual-validation\Test-HdrValidation.ps1 `
  -RunRoot $run `
  -VisualResult Pass `
  -Notes 'Highlights, colours and shadow detail match the live scene.' `
  -RequireComplete
```

The checklist also verifies that the Advanced-page HDR state changes within two
seconds when Windows HDR is toggled. The report verifies the hidden flag, active
Windows HDR log, armed FP16 path, absence of tone-map fallback/failure, MP4
structure, WASAPI attachment and an audio track in the saved replay. Visual
quality and A/V synchronization still require a person because scripts cannot
judge whether the saved SDR image and sound match the live scene.

Current limitation: native HDR JPEG XR screenshots and their SDR companion pair
are not implemented. The experimental path produces tone-mapped SDR PNG/JPEG
screenshots and SDR H.264 replay.
