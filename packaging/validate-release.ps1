[CmdletBinding()]
param(
    [string]$ReleaseDirectory = 'dist\releases',
    [string]$GitTag = '',
    [switch]$SkipTests
)

$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$releaseRoot = if ([System.IO.Path]::IsPathRooted($ReleaseDirectory)) {
    [System.IO.Path]::GetFullPath($ReleaseDirectory)
} else { [System.IO.Path]::GetFullPath((Join-Path $root $ReleaseDirectory)) }
$releaseRoot = $releaseRoot.TrimEnd('\', '/')
$approvedRoot = [System.IO.Path]::GetFullPath((Join-Path $root 'dist\releases')).TrimEnd('\', '/')
if (-not [System.StringComparer]::OrdinalIgnoreCase.Equals($releaseRoot, $approvedRoot)) {
    throw "Release validation is restricted to $approvedRoot"
}
$version = (Get-Content (Join-Path $root 'VERSION') -Raw).Trim()
if ($version -notmatch '^\d+\.\d+\.\d+$') { throw 'VERSION is not valid semantic version text.' }
if ($GitTag -and $GitTag.TrimStart('v') -ne $version) {
    throw "Git tag $GitTag does not match VERSION $version."
}

$portableName = "GameHQ-$version-win64-portable.zip"
$updateName = "GameHQ-$version-win64-update.zip"
$portableZip = Join-Path $releaseRoot $portableName
$updateZip = Join-Path $releaseRoot $updateName
$checksum = "$updateZip.sha256"
foreach ($required in @($portableZip, $updateZip, $checksum)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) { throw "Missing release artifact: $required" }
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
function Get-ZipEntries([string]$Path) {
    $archive = [System.IO.Compression.ZipFile]::OpenRead($Path)
    try { return @($archive.Entries | ForEach-Object { $_.FullName.Replace('\', '/') }) }
    finally { $archive.Dispose() }
}
$portableEntries = Get-ZipEntries $portableZip
$updateEntries = Get-ZipEntries $updateZip
foreach ($required in @('GameHQ.exe', 'GameHQUpdater.exe', 'portable.flag', 'app/GameHQ.exe')) {
    if ($portableEntries -notcontains $required) { throw "Portable ZIP is missing $required" }
}
foreach ($required in @('GameHQ.exe', 'GameHQUpdater.pending.exe', 'app/GameHQ.exe', 'update-package.json')) {
    if ($updateEntries -notcontains $required) { throw "Update ZIP is missing $required" }
}
$allowedRoots = @('GameHQ.exe', 'GameHQUpdater.pending.exe', 'README.txt', 'LICENSE.txt',
    'THIRD_PARTY_NOTICES.md', 'app', 'licenses', 'update-package.json')
$forbiddenRoots = @('portable.flag', 'Captures', 'gamehq-data', 'saveplay-data', 'playhq-data', 'GameHQUpdater.exe')
foreach ($entry in $updateEntries) {
    $rootName = $entry.TrimEnd('/').Split('/')[0]
    if ($rootName -notin $allowedRoots) { throw "Update ZIP contains unexpected path: $entry" }
    if ($rootName -in $forbiddenRoots) { throw "Update ZIP contains forbidden path: $entry" }
}

$archive = [System.IO.Compression.ZipFile]::OpenRead($updateZip)
try {
    $entry = $archive.GetEntry('update-package.json')
    $reader = [System.IO.StreamReader]::new($entry.Open(), [System.Text.Encoding]::UTF8)
    try { $manifest = ($reader.ReadToEnd() | ConvertFrom-Json) } finally { $reader.Dispose() }
} finally { $archive.Dispose() }
if ($manifest.schemaVersion -ne 1 -or $manifest.productId -ne 'underfusion.gamehq' `
    -or $manifest.appVersion -ne $version -or $manifest.layoutVersion -ne 1 `
    -or $manifest.minimumUpdaterVersion -notmatch '^\d+\.\d+\.\d+$') {
    throw 'update-package.json is missing required or matching metadata.'
}
$expectedChecksum = "^([0-9a-f]{64}) \*$([regex]::Escape($updateName))$"
$checksumText = (Get-Content -LiteralPath $checksum -Raw).Trim()
if ($checksumText -notmatch $expectedChecksum) { throw 'Update checksum file has an invalid format or filename.' }
$actualHash = (Get-FileHash -LiteralPath $updateZip -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actualHash -ne $Matches[1]) { throw 'Update ZIP checksum does not match.' }

$packagedApp = Join-Path $root 'dist\GameHQ\app\GameHQ.exe'
$versionCheck = Start-Process -FilePath $packagedApp `
    -ArgumentList @('--assert-version', $version) -WindowStyle Hidden -Wait -PassThru
if ($versionCheck.ExitCode -ne 0) { throw 'Packaged application version does not match VERSION.' }
if (-not $SkipTests) {
    $ctest = Join-Path $root 'tools\cmake\bin\ctest.exe'
    $env:PATH = (Join-Path $root 'tools\Qt\6.8.3\mingw_64\bin') + ';' + $env:PATH
    & $ctest --test-dir (Join-Path $root 'out') -R 'tst_(updatedownloader|updatepreflight|updateinstaller|updatertransaction)' --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Updater validation tests failed.' }
}
Write-Host "[validate] release $version passed"
