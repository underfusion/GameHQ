# Creates an optional exact-revision source archive and checksum.
[CmdletBinding()]
param(
    [string]$ReleaseDirectory = 'dist\releases',
    [string]$Revision = 'HEAD',
    [string]$GitTag = ''
)

$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$releaseRoot = if ([System.IO.Path]::IsPathRooted($ReleaseDirectory)) {
    [System.IO.Path]::GetFullPath($ReleaseDirectory)
} else { [System.IO.Path]::GetFullPath((Join-Path $root $ReleaseDirectory)) }
$approvedRoot = [System.IO.Path]::GetFullPath((Join-Path $root 'dist\releases'))
if (-not [System.StringComparer]::OrdinalIgnoreCase.Equals(
        $releaseRoot.TrimEnd('\', '/'), $approvedRoot.TrimEnd('\', '/'))) {
    throw "Source preparation is restricted to $approvedRoot"
}

$version = (Get-Content (Join-Path $root 'VERSION') -Raw).Trim()
if ($version -notmatch '^\d+\.\d+\.\d+$') { throw 'VERSION is not valid semantic version text.' }
$identity = Import-PowerShellDataFile (Join-Path $PSScriptRoot 'distribution-identity.psd1')

$commit = (& git -C $root rev-parse --verify "$Revision`^{commit}").Trim()
if ($LASTEXITCODE -ne 0 -or $commit -notmatch '^[0-9a-f]{40}$') {
    throw "Cannot resolve source revision: $Revision"
}
$revisionVersion = (& git -C $root show "$commit`:VERSION").Trim()
if ($LASTEXITCODE -ne 0 -or $revisionVersion -ne $version) {
    throw "Source revision VERSION '$revisionVersion' does not match $version."
}
if ($GitTag) {
    if ($GitTag -ne "v$version") { throw "Source tag $GitTag does not match VERSION $version." }
    $tagCommit = (& git -C $root rev-parse --verify "$GitTag`^{commit}").Trim()
    if ($LASTEXITCODE -ne 0 -or $tagCommit -ne $commit) {
        throw "Source tag $GitTag does not resolve to $commit."
    }
}

New-Item -ItemType Directory -Path $releaseRoot -Force | Out-Null
$sourceName = $identity.ArtifactPatterns.Source -f $version
$checksumName = $identity.ArtifactPatterns.SourceChecksum -f $version
$sourcePath = Join-Path $releaseRoot $sourceName
$checksumPath = Join-Path $releaseRoot $checksumName
foreach ($path in @($sourcePath, $checksumPath)) {
    if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force }
}

& git -C $root archive --format=zip --prefix="GameHQ-$version-source/" --output=$sourcePath `
    $commit -- . ':(exclude)docs/plans/**'
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
    throw 'git archive did not create the source ZIP.'
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::OpenRead($sourcePath)
try {
    $entries = @($archive.Entries | ForEach-Object FullName)
} finally { $archive.Dispose() }
$prefix = "GameHQ-$version-source/"
foreach ($required in @('CMakeLists.txt', 'VERSION', 'LICENSE', 'README.md',
        'docs/dev-setup.md', 'packaging/make-dist.ps1')) {
    if ($entries -notcontains ($prefix + $required)) { throw "Source ZIP is missing $required" }
}
foreach ($entry in $entries) {
    $relative = $entry.Substring($prefix.Length)
    if ($relative -match '(^|/)(\.git|build|out|dist|gamehq-data|Captures|\.env)(/|$)' -or
        $relative -match '\.(pem|pfx|key)$') {
        throw "Source ZIP contains a forbidden path: $entry"
    }
}

$hash = (Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash.ToLowerInvariant()
[System.IO.File]::WriteAllText($checksumPath, "$hash *$sourceName`n",
    [System.Text.UTF8Encoding]::new($false))

Write-Host "[source] $sourceName -> $commit ($hash)"
