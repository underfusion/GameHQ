# Assembles and validates the portable and update-only release artifacts.
#
#   powershell -ExecutionPolicy Bypass -File packaging\make-dist.ps1

[CmdletBinding()]
param([switch]$SkipTests)

$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$version = (Get-Content (Join-Path $root 'VERSION') -Raw).Trim()
$dist = Join-Path $root 'dist'
$portableRoot = Join-Path $dist 'GameHQ'
$releaseRoot = Join-Path $dist 'releases'
$updateRoot = Join-Path $dist '.update-package-staging'

& (Join-Path $PSScriptRoot 'assemble-package.ps1') `
    -BuildDirectory 'out' `
    -Destination 'dist\GameHQ'

foreach ($path in @($releaseRoot, $updateRoot)) {
    if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Recurse -Force }
    New-Item -ItemType Directory -Path $path -Force | Out-Null
}

$portableZip = Join-Path $releaseRoot "GameHQ-$version-win64-portable.zip"
$updateZip = Join-Path $releaseRoot "GameHQ-$version-win64-update.zip"
Compress-Archive -Path (Join-Path $portableRoot '*') -DestinationPath $portableZip -CompressionLevel Optimal

foreach ($name in @('GameHQ.exe', 'README.txt', 'LICENSE.txt', 'THIRD_PARTY_NOTICES.md', 'app', 'licenses')) {
    Copy-Item -LiteralPath (Join-Path $portableRoot $name) -Destination $updateRoot -Recurse
}
Copy-Item -LiteralPath (Join-Path $portableRoot 'GameHQUpdater.exe') `
    -Destination (Join-Path $updateRoot 'GameHQUpdater.pending.exe')
$manifest = [ordered]@{
    schemaVersion = 1
    productId = 'underfusion.gamehq'
    appVersion = $version
    layoutVersion = 1
    minimumUpdaterVersion = $version
} | ConvertTo-Json -Compress
[System.IO.File]::WriteAllText((Join-Path $updateRoot 'update-package.json'), $manifest,
    [System.Text.UTF8Encoding]::new($false))
Compress-Archive -Path (Join-Path $updateRoot '*') -DestinationPath $updateZip -CompressionLevel Optimal
$hash = (Get-FileHash -LiteralPath $updateZip -Algorithm SHA256).Hash.ToLowerInvariant()
[System.IO.File]::WriteAllText("$updateZip.sha256", "$hash *$([System.IO.Path]::GetFileName($updateZip))`n",
    [System.Text.UTF8Encoding]::new($false))
Remove-Item -LiteralPath $updateRoot -Recurse -Force

$validationArguments = @{ ReleaseDirectory = $releaseRoot }
if ($SkipTests) { $validationArguments.SkipTests = $true }
& (Join-Path $PSScriptRoot 'validate-release.ps1') @validationArguments
if ($LASTEXITCODE -ne 0) { throw "Release validation failed ($LASTEXITCODE)" }

Write-Host "[release] ready: $releaseRoot"
