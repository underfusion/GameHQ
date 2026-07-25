#Requires -Version 5.1
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$WorkspaceRoot,
    [string]$PortablePackage,
    [switch]$OpenDisplaySettings,
    [switch]$Launch
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

if (-not $PortablePackage) {
    $version = (Get-Content -LiteralPath (Join-Path $projectRoot 'VERSION') -Raw).Trim()
    $PortablePackage = Join-Path $projectRoot (
        'dist\releases\GameHQ-{0}-win64-portable.zip' -f $version
    )
}

$prepared = @(& (Join-Path $PSScriptRoot 'Prepare-ValidationWorkspace.ps1') `
    -WorkspaceRoot $WorkspaceRoot `
    -PortablePackage $PortablePackage)
$run = Assert-ValidationRun ([string]$prepared[-1])
$metadataPath = Join-Path $run '.gamehq-validation-run.json'
$metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
$fixture = Assert-ChildOfValidationRun -Path $metadata.fixtureRoot -RunRoot $run
$dataRoot = Join-Path $fixture 'gamehq-data'
$configPath = Join-Path $dataRoot 'config.json'

$config = [ordered]@{}
if (Test-Path -LiteralPath $configPath -PathType Leaf) {
    $existing = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json
    foreach ($property in $existing.PSObject.Properties) {
        $config[$property.Name] = $property.Value
    }
}

# This profile is disposable. Keep normal game detection enabled so the terminal
# or another foreground app cannot become the replay target during setup.
$config['internal.capture.experimental_hdr'] = $true
$config['capture.mode'] = 'only_in_games'
$config['replay.auto'] = $true
$config['replay.length_seconds'] = 30
$config['replay.fps'] = 30
$config['replay.resolution'] = '1920x1080'
$config['audio.enabled'] = $true
New-Item -ItemType Directory -Path $dataRoot -Force | Out-Null
[System.IO.File]::WriteAllText(
    $configPath,
    ($config | ConvertTo-Json -Depth 8),
    [System.Text.UTF8Encoding]::new($false)
)

$hdrMetadata = [ordered]@{
    schemaVersion = 1
    preparedAtUtc = [DateTime]::UtcNow.ToString('o')
    runRoot = $run
    fixtureRoot = $fixture
    portablePackage = (Resolve-Path -LiteralPath $PortablePackage).Path
    experimentalFlag = $true
    automatedGate = 'HDR SDR capture and replay system audio'
    knownLimitation = 'Native HDR JPEG XR output plus its SDR companion pair are not implemented yet.'
}
[System.IO.File]::WriteAllText(
    (Join-Path $run 'hdr-validation.json'),
    ($hdrMetadata | ConvertTo-Json -Depth 6),
    [System.Text.UTF8Encoding]::new($false)
)

$checklist = @"
GameHQ HDR validation
=====================

Prepared fixture:
$fixture

1. Enable Windows HDR on the monitor that will show the game.
2. Close every other GameHQ instance.
3. Start this fixture:
   $fixture\GameHQ.exe
4. Open Settings > Advanced and confirm "Windows HDR is active".
5. Keep Advanced open and press Win+Alt+B to turn HDR off. Within two seconds,
   confirm the status changes to "Windows HDR is inactive". Press Win+Alt+B
   again and confirm it returns to "Windows HDR is active".
6. Start a real HDR game on that same monitor, keep it in the foreground and wait
   until GameHQ identifies that game and reports Replay buffer: Recording.
7. Show a scene with bright highlights, saturated colour, skin/neutral tones and dark detail.
8. Take one screenshot and open it. Confirm highlights, colours and dark detail
   match the live scene without the washed-out white HDR/GDI appearance.
9. Keep the game in the foreground for at least 30 seconds, then save one replay:
   - press Ctrl+Shift+E once; or
   - hold the controller Share/Create button for about 2 seconds.
   Wait for GameHQ's "Replay saved" confirmation before closing anything.
10. Open the saved MP4 and compare it with the live scene:
   - highlights remain distinct instead of becoming one white patch;
   - colours are not grey, neon or strongly shifted;
   - mid-tones and shadows remain readable;
   - system audio is present and stays synchronized with the picture;
   - playback has no black frames, corruption or severe stutter.
11. Close GameHQ, then run:
   .\tools\manual-validation\Test-HdrValidation.ps1 -RunRoot "$run" -VisualResult Pass

Known limitation:
Native HDR JPEG XR screenshots are not implemented yet. This test validates the
tone-mapped SDR PNG/JPEG companion and SDR H.264 replay paths.
"@
[System.IO.File]::WriteAllText(
    (Join-Path $run 'HDR-TEST-CHECKLIST.txt'),
    $checklist,
    [System.Text.UTF8Encoding]::new($false)
)

if ($OpenDisplaySettings) {
    Start-Process 'ms-settings:display-advanced'
}

if ($Launch) {
    $running = @(Get-Process -Name 'GameHQ' -ErrorAction SilentlyContinue)
    if ($running.Count -ne 0) {
        Write-Warning 'The disposable run is ready, but another GameHQ instance is still running.'
        foreach ($process in $running) {
            $processPath = try { $process.Path } catch { '(path unavailable)' }
            Write-Host "  PID $($process.Id): $processPath"
        }
        Write-Host '[hdr] Close that GameHQ window, then launch this prepared fixture with:'
        Write-Host "Start-Process -FilePath '$fixture\GameHQ.exe' -WorkingDirectory '$fixture'"
    } else {
        Start-Process -FilePath (Join-Path $fixture 'GameHQ.exe') -WorkingDirectory $fixture
    }
}

Write-Host "[hdr] disposable HDR run prepared: $run"
Write-Host "[hdr] experimental flag written; no first launch or manual JSON edit is needed"
Write-Host "[hdr] checklist: $(Join-Path $run 'HDR-TEST-CHECKLIST.txt')"
Write-Output $run
