#Requires -Version 5.1
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RunRoot,
    [string]$TransactionPath,
    [string]$BeforeSnapshot
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$run = Assert-ValidationRun $RunRoot
$fixture = Assert-ChildOfValidationRun -Path (Join-Path $run 'portable') -RunRoot $run
$transaction = if ($TransactionPath) { Assert-ChildOfValidationRun -Path $TransactionPath -RunRoot $run } else { Join-Path $fixture '.update\transaction.json' }
$before = if ($BeforeSnapshot) { Assert-ChildOfValidationRun -Path $BeforeSnapshot -RunRoot $run } else { Join-Path $run 'snapshots\before.json' }
$updater = Join-Path $fixture 'GameHQUpdater.exe'
foreach ($required in @($transaction, $before, $updater)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) { throw "Required recovery input is missing: $required" }
}

& $updater --recover $transaction
if ($LASTEXITCODE -ne 0) { throw "Updater recovery failed with exit code $LASTEXITCODE." }
$comparison = Join-Path $run 'snapshots\recovery-comparison.json'
& (Join-Path $PSScriptRoot 'Compare-ValidationState.ps1') -RunRoot $run -BeforeSnapshot $before -OutputPath $comparison
if ($LASTEXITCODE -ne 0) { throw "Protected-data comparison failed with exit code $LASTEXITCODE." }

$phasePath = Join-Path $fixture '.update\transaction.phase'
$markerPath = Join-Path $fixture '.update\maintenance.lock'
$phase = if (Test-Path -LiteralPath $phasePath) { (Get-Content -LiteralPath $phasePath -Raw).Trim() } else { '' }
$report = [ordered]@{
    schemaVersion = 1; verifiedAtUtc = [DateTime]::UtcNow.ToString('o')
    pass = ($phase -eq 'rolled_back' -and -not (Test-Path -LiteralPath $markerPath))
    phase = $phase; maintenanceMarkerPresent = (Test-Path -LiteralPath $markerPath)
    comparison = $comparison; updaterLog = (Join-Path $fixture '.update\updater.log')
}
$reportPath = Join-Path $run 'snapshots\recovery-report.json'
[System.IO.File]::WriteAllText($reportPath, ($report | ConvertTo-Json -Depth 5))
if (-not $report.pass) { throw "Recovery verification failed; see $reportPath" }
Write-Host "[validation] PASS: rollback completed and maintenance marker cleared: $reportPath"
