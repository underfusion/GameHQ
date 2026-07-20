#Requires -Version 5.1
<#
.SYNOPSIS
    Builds the plugin and packs it into a Playnite .pext extension archive.
.DESCRIPTION
    A .pext is a plain zip: extension.yaml + the built assembly (+ any
    dependencies) at its root. This script does not require the Playnite
    Toolbox — it assembles the archive directly so it can run in CI.
#>
[CmdletBinding()]
param(
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$version = (Get-Content (Join-Path $root "VERSION") -Raw).Trim()

& (Join-Path $PSScriptRoot "build.ps1") -Configuration $Configuration

$publishDir = Join-Path $root "src\GameHQ.Playnite\bin\$Configuration\net462"
if (-not (Test-Path $publishDir)) {
    throw "Build output not found at $publishDir"
}

$stagingDir = Join-Path $root "packaging\.staging"
if (Test-Path $stagingDir) {
    Remove-Item $stagingDir -Recurse -Force
}
New-Item -ItemType Directory -Path $stagingDir | Out-Null

$requiredFiles = @("GameHQ.Playnite.dll", "extension.yaml")
foreach ($file in $requiredFiles) {
    $source = Join-Path $publishDir $file
    if (-not (Test-Path $source)) {
        throw "Required packaged file missing: $source"
    }
    Copy-Item $source $stagingDir
}

$iconSource = Join-Path $root "icon.png"
if (Test-Path $iconSource) {
    Copy-Item $iconSource $stagingDir
}

$outDir = Join-Path $root "dist"
New-Item -ItemType Directory -Path $outDir -Force | Out-Null
$pextPath = Join-Path $outDir "GameHQ_Integration_$($version -replace '\.', '_').pext"
if (Test-Path $pextPath) {
    Remove-Item $pextPath -Force
}

Compress-Archive -Path (Join-Path $stagingDir "*") -DestinationPath $pextPath
Remove-Item $stagingDir -Recurse -Force

Write-Host "[playnite-package] wrote $pextPath"
