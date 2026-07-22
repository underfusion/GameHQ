[CmdletBinding()]
param(
    [string]$ReleaseDirectory = 'dist\releases'
)

$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$releaseRoot = if ([System.IO.Path]::IsPathRooted($ReleaseDirectory)) {
    [System.IO.Path]::GetFullPath($ReleaseDirectory)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $root $ReleaseDirectory))
}
$approvedRoot = [System.IO.Path]::GetFullPath((Join-Path $root 'dist\releases'))
if (-not [System.StringComparer]::OrdinalIgnoreCase.Equals(
        $releaseRoot.TrimEnd('\', '/'), $approvedRoot.TrimEnd('\', '/'))) {
    throw "Artifact-set validation is restricted to $approvedRoot"
}

$version = (Get-Content (Join-Path $root 'VERSION') -Raw).Trim()
$identity = Import-PowerShellDataFile (Join-Path $PSScriptRoot 'distribution-identity.psd1')
$evidencePath = Join-Path $releaseRoot 'release-evidence.json'
$expectedPayload = @(
    ($identity.ArtifactPatterns.Setup -f $version),
    ($identity.ArtifactPatterns.Portable -f $version),
    ($identity.ArtifactPatterns.Update -f $version),
    (($identity.ArtifactPatterns.Update -f $version) + '.sha256'),
    'gamehq-release.json',
    'gamehq-release.sig'
)
$expectedFiles = @($expectedPayload + 'release-evidence.json' | Sort-Object)
$actualFiles = @(Get-ChildItem -LiteralPath $releaseRoot -File | ForEach-Object Name | Sort-Object)
if (($expectedFiles -join "`n") -ne ($actualFiles -join "`n")) {
    throw "Release directory does not contain exactly the approved Beta artifact set.`nExpected: $($expectedFiles -join ', ')`nActual: $($actualFiles -join ', ')"
}

$evidence = Get-Content -LiteralPath $evidencePath -Raw | ConvertFrom-Json
if ($evidence.schemaVersion -ne 1 -or $evidence.version -ne $version -or
    $evidence.trustMode -ne 'unsigned-beta' -or $evidence.manifestMode -ne 'test') {
    throw 'Release evidence does not describe the expected unsigned-Beta/test-key build.'
}
if ($evidence.compliance.license -ne 'passed' -or
    $evidence.compliance.privacy -ne 'passed') {
    throw 'Release evidence is missing passed license or privacy compliance.'
}
$recordedNames = @($evidence.artifacts | ForEach-Object fileName | Sort-Object)
if (($expectedPayload | Sort-Object) -join "`n" -ne ($recordedNames -join "`n")) {
    throw 'Release evidence does not list exactly the approved payload artifacts.'
}
foreach ($record in $evidence.artifacts) {
    $path = Join-Path $releaseRoot $record.fileName
    $item = Get-Item -LiteralPath $path
    $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($item.Length -ne $record.size -or $hash -ne $record.sha256) {
        throw "Release evidence mismatch for $($record.fileName)."
    }
}
$unexpectedSignature = @($evidence.authenticode | Where-Object Status -ne 'NotSigned')
if ($unexpectedSignature.Count -ne 0) {
    throw 'Unsigned-Beta evidence contains an unexpectedly signed GameHQ artifact.'
}

Write-Host "[artifact-set] unsigned Beta $version is complete and matches release evidence"
