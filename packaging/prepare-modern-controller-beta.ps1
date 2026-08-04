[CmdletBinding()]
param(
    [string]$ReleaseDirectory = 'dist\releases',
    [string]$BuildDirectory = '.tmp\t28-clean-build'
)

$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$releaseRoot = [System.IO.Path]::GetFullPath((Join-Path $root $ReleaseDirectory))
$approvedReleaseRoot = [System.IO.Path]::GetFullPath((Join-Path $root 'dist\releases'))
if (-not [System.StringComparer]::OrdinalIgnoreCase.Equals(
        $releaseRoot.TrimEnd('\'), $approvedReleaseRoot.TrimEnd('\'))) {
    throw "Beta input is restricted to $approvedReleaseRoot"
}

$cache = Join-Path $root "$BuildDirectory\CMakeCache.txt"
if (-not (Test-Path -LiteralPath $cache -PathType Leaf) -or
    -not (Select-String -LiteralPath $cache -SimpleMatch 'CMAKE_BUILD_TYPE:STRING=Debug')) {
    throw 'The hardware beta must come from the accepted Debug build.'
}

$version = (Get-Content (Join-Path $root 'VERSION') -Raw).Trim()
$sourceName = "GameHQ-$version-win64-portable.zip"
$sourcePackage = Join-Path $releaseRoot $sourceName
if (-not (Test-Path -LiteralPath $sourcePackage -PathType Leaf)) {
    throw "Missing accepted Portable package: $sourcePackage"
}

$betaRoot = Join-Path $root 'dist\modern-controller-beta'
if (Test-Path -LiteralPath $betaRoot) {
    Remove-Item -LiteralPath $betaRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $betaRoot -Force | Out-Null

$betaName = "GameHQ-$version-modern-controller-beta-portable.zip"
$betaPackage = Join-Path $betaRoot $betaName
Copy-Item -LiteralPath $sourcePackage -Destination $betaPackage
foreach ($document in @(
        'docs\testing\modern-controller-beta.md',
        'docs\testing\modern-controller-model-matrix.md',
        'docs\testing\controller-reporter-response-drafts.md',
        'tools\manual-validation\MODERN-CONTROLLER-VM-CHECKLIST.md')) {
    Copy-Item -LiteralPath (Join-Path $root $document) -Destination $betaRoot
}

$hash = (Get-FileHash -LiteralPath $betaPackage -Algorithm SHA256).Hash.ToLowerInvariant()
$sourceOffer = Get-Content (Join-Path $root 'dist\.program-payload\licenses\SOURCE_OFFER.txt') -Raw
if ($sourceOffer -notmatch '(?m)^Revision: ([0-9a-f]{40})\r?$') {
    throw 'Accepted package source revision is missing from SOURCE_OFFER.txt.'
}
$packageRevision = $Matches[1]
$manifest = [ordered]@{
    schemaVersion = 1
    purpose = 'instrumented-modern-controller-beta'
    version = $version
    packageRevision = $packageRevision
    bundleRevision = (& git -C $root rev-parse HEAD).Trim()
    branch = (& git -C $root branch --show-current).Trim()
    package = $betaName
    sha256 = $hash
    gameSirG7Pro = 'Unverified'
    eightBitDoUltimate2 = 'Unverified'
    stableRelease = $false
} | ConvertTo-Json
[System.IO.File]::WriteAllText((Join-Path $betaRoot 'beta-manifest.json'), $manifest,
    [System.Text.UTF8Encoding]::new($false))
[System.IO.File]::WriteAllText((Join-Path $betaRoot "$betaName.sha256"),
    "$hash *$betaName`n", [System.Text.UTF8Encoding]::new($false))

Write-Host "[modern-controller-beta] ready: $betaRoot"
Get-ChildItem -LiteralPath $betaRoot -File | ForEach-Object {
    Write-Host ("  " + $_.Name + " (" + $_.Length + " bytes)")
}
