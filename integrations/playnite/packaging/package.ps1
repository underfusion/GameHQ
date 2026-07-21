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

$requiredFiles = @("GameHQ.Playnite.dll", "BouncyCastle.Cryptography.dll", "extension.yaml")
foreach ($file in $requiredFiles) {
    $source = Join-Path $publishDir $file
    if (-not (Test-Path $source)) {
        throw "Required packaged file missing: $source"
    }
    Copy-Item $source $stagingDir
}
$licenseSource = Join-Path $root "LICENSES\BouncyCastle.Cryptography.txt"
$licenseTarget = Join-Path $stagingDir "LICENSES"
New-Item -ItemType Directory -Path $licenseTarget -Force | Out-Null
Copy-Item -LiteralPath $licenseSource -Destination $licenseTarget

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

# Compress-Archive rejects the .pext extension outright even though a .pext
# is just a zip, so build it as .zip and rename.
$zipPath = [System.IO.Path]::ChangeExtension($pextPath, "zip")
if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}
Compress-Archive -Path (Join-Path $stagingDir "*") -DestinationPath $zipPath
Move-Item $zipPath $pextPath
Remove-Item $stagingDir -Recurse -Force

Write-Host "[playnite-package] wrote $pextPath"
