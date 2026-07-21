#Requires -Version 5.1
[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$RunRoot)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$run = Assert-ValidationRun $RunRoot
$metadata = Get-Content -LiteralPath (Join-Path $run '.gamehq-validation-run.json') -Raw | ConvertFrom-Json
$fixture = Assert-ChildOfValidationRun -Path $metadata.fixtureRoot -RunRoot $run
$evidence = Join-Path $run ('evidence\' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $evidence | Out-Null

foreach ($relative in @('.gamehq-validation-run.json', 'snapshots\before.json', 'snapshots\comparison.json')) {
    $source = Join-Path $run $relative
    if (Test-Path -LiteralPath $source -PathType Leaf) {
        $target = Join-Path $evidence $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
        Copy-Item -LiteralPath $source -Destination $target
    }
}
$logs = Join-Path $fixture 'gamehq-data\logs'
if (Test-Path -LiteralPath $logs -PathType Container) {
    Copy-Item -LiteralPath $logs -Destination (Join-Path $evidence 'logs') -Recurse
}
$artifactHashes = @()
foreach ($file in Get-ChildItem -LiteralPath (Join-Path $run 'artifacts') -File) {
    $artifactHashes += [pscustomobject]@{
        fileName = $file.Name
        size = [int64]$file.Length
        sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
    }
}
$system = Get-CimInstance Win32_OperatingSystem
$summary = [ordered]@{
    collectedAtUtc = [DateTime]::UtcNow.ToString('o')
    runRoot = $run
    os = $system.Caption
    osVersion = $system.Version
    osBuild = $system.BuildNumber
    artifacts = $artifactHashes
}
[System.IO.File]::WriteAllText((Join-Path $evidence 'evidence-summary.json'), ($summary | ConvertTo-Json -Depth 6))
Write-Host "[validation] evidence collected: $evidence"
Write-Output $evidence
