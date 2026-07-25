[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
. (Join-Path $PSScriptRoot 'resolve-ninja.ps1')

$testRoot = Join-Path $root 'build\.ninja-resolution-tests'
$approvedTestRoot = [System.IO.Path]::GetFullPath(
    (Join-Path $root 'build\.ninja-resolution-tests'))
if (Test-Path -LiteralPath $testRoot) {
    $resolvedTestRoot = (Resolve-Path -LiteralPath $testRoot).Path
    if (-not [System.StringComparer]::OrdinalIgnoreCase.Equals(
            $resolvedTestRoot, $approvedTestRoot)) {
        throw "Unexpected Ninja test cleanup target: $resolvedTestRoot"
    }
    Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $testRoot -Force | Out-Null

$originalPath = $env:PATH
$originalNinjaExe = $env:NINJA_EXE
try {
    $fakeRepository = Join-Path $testRoot 'repository'
    $explicitDirectory = Join-Path $testRoot 'explicit'
    $environmentDirectory = Join-Path $testRoot 'environment'
    $pathDirectory = Join-Path $testRoot 'path'
    $emptyPathDirectory = Join-Path $testRoot 'empty-path'
    foreach ($directory in @($fakeRepository, $explicitDirectory, $environmentDirectory,
            $pathDirectory, $emptyPathDirectory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }

    $explicitNinja = Join-Path $explicitDirectory 'ninja.exe'
    $environmentNinja = Join-Path $environmentDirectory 'ninja.exe'
    $pathNinja = Join-Path $pathDirectory 'ninja.exe'
    foreach ($target in @($explicitNinja, $environmentNinja, $pathNinja)) {
        Copy-Item -LiteralPath $env:ComSpec -Destination $target
    }

    $env:NINJA_EXE = $environmentNinja
    $env:PATH = $pathDirectory
    $resolved = Resolve-GameHQNinja -NinjaPath $explicitNinja `
        -RepositoryRoot $fakeRepository
    if ($resolved -ne (Resolve-Path -LiteralPath $explicitNinja).Path) {
        throw 'Explicit -NinjaPath did not take precedence.'
    }
    Write-Host '[ninja-test] explicit path passed'

    $env:NINJA_EXE = $environmentNinja
    $resolved = Resolve-GameHQNinja -RepositoryRoot $fakeRepository
    if ($resolved -ne (Resolve-Path -LiteralPath $environmentNinja).Path) {
        throw 'NINJA_EXE was not resolved.'
    }
    Write-Host '[ninja-test] NINJA_EXE passed'

    $env:NINJA_EXE = ''
    $env:PATH = $pathDirectory
    $resolved = Resolve-GameHQNinja -RepositoryRoot $fakeRepository
    if ($resolved -ne (Resolve-Path -LiteralPath $pathNinja).Path) {
        throw 'Ninja was not resolved through PATH.'
    }
    Write-Host '[ninja-test] PATH discovery passed'

    $env:PATH = $emptyPathDirectory
    $missingDiagnostic = ''
    try {
        Resolve-GameHQNinja -RepositoryRoot $fakeRepository | Out-Null
    } catch {
        $missingDiagnostic = $_.Exception.Message
    }
    if ($missingDiagnostic -notlike 'Ninja executable was not found.*') {
        throw "Missing Ninja diagnostic was not precise: $missingDiagnostic"
    }
    Write-Host '[ninja-test] missing-tool diagnostic passed'

    $env:PATH = $originalPath
    $archivePath = Join-Path $testRoot 'source.zip'
    & git -C $root archive --format=zip --prefix='GameHQ-source/' `
        --output=$archivePath HEAD -- . ':(exclude)docs/plans/**'
    if ($LASTEXITCODE -ne 0) { throw 'Could not create the source-archive test fixture.' }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($archivePath)
    try {
        $entries = @($archive.Entries | ForEach-Object {
            $_.FullName.Replace('\', '/').ToLowerInvariant()
        })
    } finally {
        $archive.Dispose()
    }
    if ($entries -contains 'gamehq-source/tools/ninja.exe') {
        throw 'Source archive contains tools/ninja.exe.'
    }
    Write-Host '[ninja-test] source archive excludes tools/ninja.exe'
} finally {
    $env:PATH = $originalPath
    $env:NINJA_EXE = $originalNinjaExe
    if (Test-Path -LiteralPath $testRoot) {
        $resolvedTestRoot = (Resolve-Path -LiteralPath $testRoot).Path
        if ([System.StringComparer]::OrdinalIgnoreCase.Equals(
                $resolvedTestRoot, $approvedTestRoot)) {
            Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force
        }
    }
}

Write-Host '[ninja-test] all clean-source Ninja resolution tests passed'
