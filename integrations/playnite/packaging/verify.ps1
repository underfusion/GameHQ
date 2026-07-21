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

$projectPath = Join-Path $root "src\GameHQ.Playnite\GameHQ.Playnite.csproj"
$projectXml = Get-Content $projectPath -Raw
if ($projectXml -notmatch "<Version>\s*$([regex]::Escape($version))\s*</Version>") {
    $failures += "GameHQ.Playnite.csproj Version does not match VERSION ($version)"
}
$sdkMatch = [regex]::Match(
    $projectXml,
    '<PackageReference\s+Include="PlayniteSDK"\s+Version="(?<version>\d+\.\d+\.\d+)"'
)
if (-not $sdkMatch.Success) {
    $failures += "GameHQ.Playnite.csproj must pin a plain PlayniteSDK version"
}
if ($projectXml -notmatch '<PackageReference\s+Include="BouncyCastle\.Cryptography"\s+Version="2\.6\.2"') {
    $failures += "GameHQ.Playnite.csproj must pin reviewed BouncyCastle.Cryptography 2.6.2"
}

$installerManifestPath = Join-Path $root "InstallerManifest.yaml"
$installerManifest = Get-Content $installerManifestPath -Raw
if ($installerManifest -notmatch "AddonId:\s*GameHQ_Integration") {
    $failures += "InstallerManifest.yaml AddonId must be GameHQ_Integration"
}
if ($installerManifest -notmatch "Version:\s*$([regex]::Escape($version))") {
    $failures += "InstallerManifest.yaml Version does not match VERSION ($version)"
}
if ($sdkMatch.Success -and
    $installerManifest -notmatch "RequiredApiVersion:\s*$([regex]::Escape($sdkMatch.Groups['version'].Value))") {
    $failures += "InstallerManifest.yaml RequiredApiVersion does not match the pinned PlayniteSDK version"
}
if ($installerManifest -match "Initial scaffold|Not yet functional") {
    $failures += "InstallerManifest.yaml still contains scaffold release copy"
}

$requiredPaths = @(
    "src\GameHQ.Playnite\GameHQ.Playnite.csproj",
    "src\GameHQ.Playnite\GameHQPlugin.cs",
    "InstallerManifest.yaml",
    "RELEASE_CHECKLIST.md",
    "CHANGELOG.md",
    "README.md",
    "LICENSES\BouncyCastle.Cryptography.txt"
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
