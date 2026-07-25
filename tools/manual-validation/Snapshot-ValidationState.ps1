#Requires -Version 5.1
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RunRoot,
    [Parameter(Mandatory = $true)][string]$FixtureRoot,
    [Parameter(Mandatory = $true)][string]$OutputPath
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$run = Assert-ValidationRun $RunRoot
$fixture = Assert-ChildOfValidationRun -Path $FixtureRoot -RunRoot $run
$output = Assert-ChildOfValidationRun -Path $OutputPath -RunRoot $run
$outputParent = Split-Path -Parent $output
New-Item -ItemType Directory -Path $outputParent -Force | Out-Null

$snapshot = [ordered]@{
    schemaVersion = 1
    capturedAtUtc = [DateTime]::UtcNow.ToString('o')
    runRoot = $run
    fixtureRoot = $fixture
    files = @(Get-ValidationFileRecords -FixtureRoot $fixture)
}
[System.IO.File]::WriteAllText($output, ($snapshot | ConvertTo-Json -Depth 6))
Write-Host "[validation] snapshot: $output ($($snapshot.files.Count) files)"
