#Requires -Version 5.1
[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'High')]
param(
    [Parameter(Mandatory = $true)][string]$RunRoot,
    [Parameter(Mandatory = $true)][int]$ProcessId,
    [Parameter(Mandatory = $true)]
    [ValidateSet('staged', 'data_snapshotted', 'swapped', 'validating')]
    [string]$Phase,
    [ValidateRange(1, 300)][int]$TimeoutSeconds = 60
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$run = Assert-ValidationRun $RunRoot
$fixture = Assert-ChildOfValidationRun -Path (Join-Path $run 'portable') -RunRoot $run
$process = Get-Process -Id $ProcessId -ErrorAction Stop
$processPath = $process.Path
if (-not $processPath -or -not (Test-SameOrChildPath -Candidate $processPath -Parent $fixture)) {
    throw "Refusing to terminate PID $ProcessId because its executable is outside the disposable fixture."
}
if ([System.IO.Path]::GetFileName($processPath) -ne 'GameHQUpdater.exe') {
    throw "Refusing to terminate PID $ProcessId because it is not GameHQUpdater.exe."
}

$observation = & (Join-Path $PSScriptRoot 'Wait-ForUpdatePhase.ps1') -RunRoot $run -Phase $Phase -TimeoutSeconds $TimeoutSeconds
if ($WhatIfPreference) { Write-Output $observation; return }
if ($PSCmdlet.ShouldProcess("$processPath (PID $ProcessId)", "Terminate at durable phase '$Phase'")) {
    Stop-Process -Id $ProcessId -Force -ErrorAction Stop
    $process.WaitForExit()
    $evidence = [ordered]@{
        schemaVersion = 1; killedAtUtc = [DateTime]::UtcNow.ToString('o')
        phase = $Phase; processId = $ProcessId; executable = $processPath
    }
    $path = Join-Path $run "snapshots\interruption-$Phase.json"
    [System.IO.File]::WriteAllText($path, ($evidence | ConvertTo-Json -Depth 4))
    Write-Host "[validation] updater terminated at $Phase; evidence: $path"
}
