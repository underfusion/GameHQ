#Requires -Version 5.1
<#
.SYNOPSIS
    Sanity-checks the plugin scaffold: VERSION/extension.yaml agreement,
    required files present, and the identifiers reserved in the main plan
    (p1-1) haven't drifted.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$failures = @()

$version = (Get-Content (Join-Path $root "VERSION") -Raw).Trim()
if ($version -notmatch '^\d+\.\d+\.\d+$') {
    $failures += "VERSION '$version' is not a plain X.Y.Z"
}

$extensionYamlPath = Join-Path $root "src\GameHQ.Playnite\extension.yaml"
$extensionYaml = Get-Content $extensionYamlPath -Raw
if ($extensionYaml -notmatch "Id:\s*GameHQ_Integration") {
    $failures += "extension.yaml Id must be GameHQ_Integration"
}
if ($extensionYaml -notmatch "Version:\s*$([regex]::Escape($version))") {
    $failures += "extension.yaml Version ($extensionYaml) does not match VERSION ($version)"
}

$requiredPaths = @(
    "src\GameHQ.Playnite\GameHQ.Playnite.csproj",
    "src\GameHQ.Playnite\GameHQPlugin.cs",
    "InstallerManifest.yaml",
    "CHANGELOG.md",
    "README.md"
)
foreach ($relative in $requiredPaths) {
    if (-not (Test-Path (Join-Path $root $relative))) {
        $failures += "Missing required file: $relative"
    }
}

if ($failures.Count -gt 0) {
    Write-Host "[playnite-verify] FAILED:"
    foreach ($f in $failures) { Write-Host "  - $f" }
    exit 1
}

Write-Host "[playnite-verify] OK (version $version)"
