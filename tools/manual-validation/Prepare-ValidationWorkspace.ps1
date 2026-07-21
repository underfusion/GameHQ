#Requires -Version 5.1
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$WorkspaceRoot,
    [Parameter(Mandatory = $true)][string]$PortablePackage,
    [string]$UpdatePackage,
    [string]$SetupPackage
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$workspace = Assert-SafeWorkspaceRoot -Path $WorkspaceRoot -ProjectRoot $projectRoot -Create

$portable = (Resolve-Path -LiteralPath $PortablePackage -ErrorAction Stop).Path
if ([System.IO.Path]::GetExtension($portable) -ne '.zip' -or
    [System.IO.Path]::GetFileName($portable) -notmatch '-win64-portable\.zip$') {
    throw "PortablePackage must be a GameHQ win64 portable ZIP: $portable"
}

$optionalArtifacts = @()
foreach ($candidate in @($UpdatePackage, $SetupPackage)) {
    if ($candidate) {
        $optionalArtifacts += (Resolve-Path -LiteralPath $candidate -ErrorAction Stop).Path
    }
}
if ($UpdatePackage -and [System.IO.Path]::GetFileName($UpdatePackage) -notmatch '-win64-update\.zip$') {
    throw "UpdatePackage must be a GameHQ win64 update ZIP"
}
if ($SetupPackage -and [System.IO.Path]::GetFileName($SetupPackage) -notmatch '-win64-setup\.exe$') {
    throw "SetupPackage must be a GameHQ win64 Setup executable"
}

$runName = 'run-{0}-{1}' -f (Get-Date -Format 'yyyyMMdd-HHmmss'), ([guid]::NewGuid().ToString('N').Substring(0, 8))
$runRoot = Join-Path $workspace $runName
New-Item -ItemType Directory -Path $runRoot | Out-Null
$artifactRoot = Join-Path $runRoot 'artifacts'
$fixtureRoot = Join-Path $runRoot 'portable'
$snapshotRoot = Join-Path $runRoot 'snapshots'
New-Item -ItemType Directory -Path $artifactRoot, $fixtureRoot, $snapshotRoot | Out-Null

$artifactRecords = @()
foreach ($source in @($portable) + $optionalArtifacts) {
    $destination = Join-Path $artifactRoot ([System.IO.Path]::GetFileName($source))
    Copy-Item -LiteralPath $source -Destination $destination
    $artifactRecords += [pscustomobject]@{
        fileName = [System.IO.Path]::GetFileName($source)
        source = $source
        size = [int64](Get-Item -LiteralPath $source).Length
        sha256 = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
    }
}

Expand-Archive -LiteralPath (Join-Path $artifactRoot ([System.IO.Path]::GetFileName($portable))) -DestinationPath $fixtureRoot
foreach ($required in @('GameHQ.exe', 'app\GameHQ.exe', 'portable.flag')) {
    if (-not (Test-Path -LiteralPath (Join-Path $fixtureRoot $required))) {
        throw "Portable fixture is missing required path: $required"
    }
}

$captures = Join-Path $fixtureRoot 'Captures'
$data = Join-Path $fixtureRoot 'gamehq-data'
New-Item -ItemType Directory -Path $captures, $data -Force | Out-Null
[System.IO.File]::WriteAllText((Join-Path $captures 'validation-sentinel.txt'), 'GameHQ validation media sentinel')
[System.IO.File]::WriteAllText((Join-Path $data 'validation-sentinel.txt'), 'GameHQ validation data sentinel')

$metadata = [ordered]@{
    schemaVersion = 1
    createdAtUtc = [DateTime]::UtcNow.ToString('o')
    runRoot = $runRoot
    fixtureRoot = $fixtureRoot
    artifacts = $artifactRecords
}
[System.IO.File]::WriteAllText(
    (Join-Path $runRoot '.gamehq-validation-run.json'),
    ($metadata | ConvertTo-Json -Depth 6)
)

& (Join-Path $PSScriptRoot 'Snapshot-ValidationState.ps1') `
    -RunRoot $runRoot `
    -FixtureRoot $fixtureRoot `
    -OutputPath (Join-Path $snapshotRoot 'before.json')

Write-Host "[validation] disposable run prepared: $runRoot"
Write-Output $runRoot
