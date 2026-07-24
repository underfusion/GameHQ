#Requires -Version 5.1
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RunRoot,
    [ValidateSet('NotChecked', 'Pass', 'Fail')]
    [string]$VisualResult = 'NotChecked',
    [string]$Notes = '',
    [switch]$RequireComplete
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$run = Assert-ValidationRun $RunRoot
$metadata = Get-Content -LiteralPath (Join-Path $run '.gamehq-validation-run.json') -Raw |
    ConvertFrom-Json
$fixture = Assert-ChildOfValidationRun -Path $metadata.fixtureRoot -RunRoot $run
$configPath = Join-Path $fixture 'gamehq-data\config.json'
$logPath = Join-Path $fixture 'gamehq-data\logs\gamehq.log'

$flagEnabled = $false
$audioEnabled = $false
if (Test-Path -LiteralPath $configPath -PathType Leaf) {
    $config = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json
    $flagProperty = $config.PSObject.Properties['internal.capture.experimental_hdr']
    $flagEnabled = $null -ne $flagProperty -and [bool]$flagProperty.Value
    $audioProperty = $config.PSObject.Properties['audio.enabled']
    $audioEnabled = $null -ne $audioProperty -and [bool]$audioProperty.Value
}

$log = if (Test-Path -LiteralPath $logPath -PathType Leaf) {
    Get-Content -LiteralPath $logPath -Raw
} else {
    ''
}
$displayHdrActive = $log -match 'Hdr:\s+Windows HDR:\s+Active on at least one display'
$experimentalArmed = $log -match 'FramePump:\s+experimental HDR path armed'
$toneMappedScreenshot = $log -match 'Screenshot:\s+captured tone-mapped HDR frame'
$toneMapFailure = $log -match 'HDR tone-map apply\(\) failed'
$fallbackUsed = $log -match (
    'GPU lacks FP16 texture/sample support|' +
    'experimental HDR tone-mapper init failed|' +
    'CreateFreeThreaded\(FP16\) failed'
)
$saveReplayRequested = $log -match 'ReplaySave\[[^\]]+\]: request begin'
$saveReplaySucceeded = $log -match 'ReplaySave\[[^\]]+\]: remux ok'
$audioCaptureAttached = $log -match 'FramePump:\s+audio attached'

$fixturePrefix = $fixture.TrimEnd('\') + '\'
$runningFixtureProcesses = @(
    Get-Process -Name 'GameHQ' -ErrorAction SilentlyContinue |
        Where-Object {
            $processPath = try { $_.Path } catch { $null }
            $processPath -and (
                $processPath.Equals(
                    (Join-Path $fixture 'GameHQ.exe'),
                    [System.StringComparison]::OrdinalIgnoreCase
                ) -or
                $processPath.StartsWith(
                    $fixturePrefix,
                    [System.StringComparison]::OrdinalIgnoreCase
                )
            )
        } |
        ForEach-Object {
            [pscustomobject]@{
                id = $_.Id
                path = try { $_.Path } catch { '' }
            }
        }
)
$gameHqStillRunning = $runningFixtureProcesses.Count -gt 0

$ringSegments = @(
    Get-ChildItem -LiteralPath (Join-Path $fixture 'gamehq-data\replay-cache') `
        -Filter '*.mp4' -File -Recurse -ErrorAction SilentlyContinue
)
$ringSegmentsPresent = $ringSegments.Count -gt 0

$clips = @(
    Get-ChildItem -LiteralPath (Join-Path $fixture 'Captures') -Filter '*.mp4' `
        -File -Recurse -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc
)
$clipRecords = @()
$mp4Checker = Join-Path $projectRoot 'tools\check_mp4.py'
foreach ($clip in $clips) {
    $checkerOutput = @(& python $mp4Checker $clip.FullName 2>&1)
    $valid = $LASTEXITCODE -eq 0
    $audioTrackPresent = ($checkerOutput -join "`n") -match 'INFO:\s+tracks\s*=.*audio'
    $clipRecords += [pscustomobject]@{
        path = $clip.FullName.Substring($fixture.Length).TrimStart('\', '/')
        size = [int64]$clip.Length
        sha256 = (Get-FileHash -LiteralPath $clip.FullName -Algorithm SHA256).Hash
        structurallyValid = $valid
        audioTrackPresent = $audioTrackPresent
        validatorOutput = ($checkerOutput -join "`n")
    }
}

$validClipPresent = @($clipRecords | Where-Object structurallyValid).Count -gt 0
$validAudioClipPresent = @(
    $clipRecords | Where-Object { $_.structurallyValid -and $_.audioTrackPresent }
).Count -gt 0
$guidance = @()
if (-not $validClipPresent) {
    if (-not $saveReplayRequested -and $ringSegmentsPresent) {
        $guidance += 'Only temporary replay-buffer segments exist; keep the HDR game foreground and press Ctrl+Shift+E once.'
    } elseif (-not $saveReplayRequested) {
        $guidance += 'No replay save was requested; wait for Replay buffer: Recording, then press Ctrl+Shift+E while the HDR game is foreground.'
    } elseif (-not $saveReplaySucceeded) {
        $guidance += 'A replay save was requested but did not complete successfully; inspect the ReplaySave entries in gamehq.log.'
    } else {
        $guidance += 'Replay export succeeded in the log, but no final MP4 was found under the fixture Captures directory.'
    }
}
if ($gameHqStillRunning) {
    $guidance += 'The disposable GameHQ instance is still running; close it only after the Replay saved confirmation appears.'
}
if (-not $audioEnabled) {
    $guidance += 'System audio is disabled in this fixture; enable Replay > System audio and record a new clip.'
} elseif (-not $audioCaptureAttached) {
    $guidance += 'System audio was requested but WASAPI did not attach; inspect AudioCapture entries in gamehq.log.'
} elseif (-not $validAudioClipPresent) {
    $guidance += 'No structurally valid saved replay contains an audio track; record and save a new clip after audio attaches.'
}

$automatedPass = $flagEnabled -and $displayHdrActive -and $experimentalArmed -and
    -not $toneMapFailure -and -not $fallbackUsed -and $saveReplaySucceeded -and
    $validClipPresent -and $toneMappedScreenshot -and $audioEnabled -and
    $audioCaptureAttached -and $validAudioClipPresent
$complete = $automatedPass -and $VisualResult -ne 'NotChecked'
$pass = $automatedPass -and $VisualResult -eq 'Pass'

$report = [ordered]@{
    schemaVersion = 3
    checkedAtUtc = [DateTime]::UtcNow.ToString('o')
    runRoot = $run
    fixtureRoot = $fixture
    complete = $complete
    pass = $pass
    automated = [ordered]@{
        pass = $automatedPass
        experimentalFlagEnabled = $flagEnabled
        systemAudioEnabled = $audioEnabled
        audioCaptureAttached = $audioCaptureAttached
        displayHdrActive = $displayHdrActive
        experimentalPathArmed = $experimentalArmed
        toneMappedScreenshotCaptured = $toneMappedScreenshot
        toneMapFailureLogged = $toneMapFailure
        fallbackLogged = $fallbackUsed
        saveReplayRequested = $saveReplayRequested
        saveReplaySucceeded = $saveReplaySucceeded
        ringSegmentsPresent = $ringSegmentsPresent
        validClipPresent = $validClipPresent
        validAudioClipPresent = $validAudioClipPresent
        gameHqStillRunning = $gameHqStillRunning
    }
    visual = [ordered]@{
        result = $VisualResult
        notes = $Notes
    }
    clips = $clipRecords
    runningFixtureProcesses = $runningFixtureProcesses
    guidance = $guidance
    nativeHdrScreenshotGate = [ordered]@{
        pass = $false
        blockedBy = 'Native HDR JPEG XR output and its SDR companion pair are not implemented yet.'
    }
}
$reportPath = Join-Path $run 'hdr-validation-report.json'
[System.IO.File]::WriteAllText(
    $reportPath,
    ($report | ConvertTo-Json -Depth 8),
    [System.Text.UTF8Encoding]::new($false)
)

Write-Host ''
Write-Host 'HDR validation summary'
Write-Host "  Experimental flag:  $flagEnabled"
Write-Host "  System audio enabled:$audioEnabled"
Write-Host "  Audio capture attached:$audioCaptureAttached"
Write-Host "  Windows HDR logged:  $displayHdrActive"
Write-Host "  FP16 path armed:      $experimentalArmed"
Write-Host "  Tone-mapped screenshot:$toneMappedScreenshot"
Write-Host "  Tone-map failure:     $toneMapFailure"
Write-Host "  SDR fallback logged:  $fallbackUsed"
Write-Host "  Save requested:       $saveReplayRequested"
Write-Host "  Save completed:       $saveReplaySucceeded"
Write-Host "  Ring segments present:$ringSegmentsPresent"
Write-Host "  Valid replay present: $validClipPresent"
Write-Host "  Replay audio present: $validAudioClipPresent"
Write-Host "  GameHQ still running: $gameHqStillRunning"
Write-Host "  Visual result:        $VisualResult"
Write-Host "  Overall replay gate:  $(if ($pass) { 'PASS' } elseif ($complete) { 'FAIL' } else { 'INCOMPLETE' })"
Write-Host "  Report:               $reportPath"
Write-Host ''
foreach ($item in $guidance) {
    Write-Host "ACTION: $item" -ForegroundColor Yellow
}
if ($guidance.Count -gt 0) {
    Write-Host ''
}
Write-Host 'Native HDR screenshot gate remains open: JPEG XR plus its SDR companion are not implemented yet.'

if ($RequireComplete -and -not $complete) {
    exit 2
}
if ($complete -and -not $pass) {
    exit 3
}
Write-Output $reportPath
