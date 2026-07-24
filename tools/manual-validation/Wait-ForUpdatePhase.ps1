#Requires -Version 5.1
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RunRoot,
    [Parameter(Mandatory = $true)]
    [ValidateSet('staged', 'data_snapshotted', 'swapped', 'validating', 'healthy', 'rolled_back')]
    [string]$Phase,
    [ValidateRange(1, 300)][int]$TimeoutSeconds = 60,
    [ValidateRange(25, 5000)][int]$PollMilliseconds = 100
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$run = Assert-ValidationRun $RunRoot
$phasePath = Join-Path $run 'portable\.update\transaction.phase'
$deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
do {
    if (Test-Path -LiteralPath $phasePath -PathType Leaf) {
        $observed = (Get-Content -LiteralPath $phasePath -Raw).Trim()
        if ($observed -eq $Phase) {
            Write-Output ([pscustomobject]@{ phase = $observed; observedAtUtc = [DateTime]::UtcNow.ToString('o'); path = $phasePath })
            return
        }
        if ($observed -in @('healthy', 'rolled_back')) {
            throw "Update reached terminal phase '$observed' before '$Phase'."
        }
    }
    Start-Sleep -Milliseconds $PollMilliseconds
} while ([DateTime]::UtcNow -lt $deadline)
throw "Timed out waiting for update phase '$Phase' after $TimeoutSeconds seconds."
