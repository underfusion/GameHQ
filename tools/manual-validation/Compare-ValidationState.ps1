#Requires -Version 5.1
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RunRoot,
    [Parameter(Mandatory = $true)][string]$BeforeSnapshot,
    [Parameter(Mandatory = $true)][string]$OutputPath
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$run = Assert-ValidationRun $RunRoot
$beforePath = Assert-ChildOfValidationRun -Path $BeforeSnapshot -RunRoot $run
$output = Assert-ChildOfValidationRun -Path $OutputPath -RunRoot $run
$before = Get-Content -LiteralPath $beforePath -Raw | ConvertFrom-Json
$fixture = Assert-ChildOfValidationRun -Path $before.fixtureRoot -RunRoot $run

$beforeMap = @{}
foreach ($record in @($before.files)) { $beforeMap[$record.path.ToLowerInvariant()] = $record }
$afterRecords = @(Get-ValidationFileRecords -FixtureRoot $fixture)
$afterMap = @{}
foreach ($record in $afterRecords) { $afterMap[$record.path.ToLowerInvariant()] = $record }

$changes = @()
$protectedFailures = @()
foreach ($key in ($beforeMap.Keys | Sort-Object)) {
    $old = $beforeMap[$key]
    $new = $afterMap[$key]
    $status = if ($null -eq $new) { 'missing' } elseif ($old.sha256 -ne $new.sha256) { 'changed' } else { 'unchanged' }
    $protected = $old.path.StartsWith('Captures\', [System.StringComparison]::OrdinalIgnoreCase) -or
        $old.path.Equals('gamehq-data\validation-sentinel.txt', [System.StringComparison]::OrdinalIgnoreCase)
    $entry = [pscustomobject]@{ path = $old.path; status = $status; protected = $protected }
    $changes += $entry
    if ($protected -and $status -ne 'unchanged') { $protectedFailures += $entry }
}
$added = @($afterRecords | Where-Object { -not $beforeMap.ContainsKey($_.path.ToLowerInvariant()) } | Select-Object -ExpandProperty path)

$report = [ordered]@{
    schemaVersion = 1
    comparedAtUtc = [DateTime]::UtcNow.ToString('o')
    runRoot = $run
    fixtureRoot = $fixture
    pass = ($protectedFailures.Count -eq 0)
    protectedFailures = @($protectedFailures)
    changes = @($changes)
    added = $added
}
New-Item -ItemType Directory -Path (Split-Path -Parent $output) -Force | Out-Null
[System.IO.File]::WriteAllText($output, ($report | ConvertTo-Json -Depth 7))
if ($report.pass) {
    Write-Host "[validation] PASS: protected capture and sentinel data are unchanged"
} else {
    Write-Host "[validation] FAIL: protected user data changed or disappeared"
    foreach ($failure in $protectedFailures) { Write-Host "  $($failure.status): $($failure.path)" }
    exit 2
}
