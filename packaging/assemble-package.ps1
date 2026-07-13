# Builds a clean portable GameHQ folder from the separate CMake output tree.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Destination,

    [string]$BuildDirectory = 'out',

    [switch]$PreserveUserData
)

$ErrorActionPreference = 'Stop'

$root = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))

function Get-ProjectPath([string]$Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $root $Path))
}

function Remove-GeneratedPath([string]$Path) {
    # Windows can retain a just-exited process's DLL mapping briefly. Retry the
    # generated path instead of turning a normal app restart into a failed sync.
    for ($attempt = 1; $attempt -le 10; $attempt++) {
        try {
            if (Test-Path -LiteralPath $Path) {
                Remove-Item -LiteralPath $Path -Recurse -Force -ErrorAction Stop
            }
            return
        } catch {
            if ($attempt -eq 10) { throw }
            Start-Sleep -Milliseconds 300
        }
    }
}

$source = Get-ProjectPath $BuildDirectory
$target = Get-ProjectPath $Destination
$devTarget = Get-ProjectPath 'build'
$distTarget = Get-ProjectPath 'dist\GameHQ'
$allowedTargets = @($devTarget, $distTarget)

if ($allowedTargets -notcontains $target) {
    throw "Refusing to assemble outside the approved package folders: $target"
}
if ($PreserveUserData -and $target -ne $devTarget) {
    throw '-PreserveUserData is only valid for the local build package.'
}

$qtBin = Join-Path $root 'tools\Qt\6.8.3\mingw_64\bin'
$realExe = Join-Path $source 'GameHQ.exe'
$launcherExe = Join-Path $source 'GameHQLauncher.exe'
$deployTool = Join-Path $qtBin 'windeployqt.exe'

foreach ($required in @($realExe, $launcherExe, $deployTool)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Missing $required - configure and build $BuildDirectory first."
    }
}

Write-Host "[package] GameHQ -> $target"

if ($PreserveUserData) {
    New-Item -ItemType Directory -Path $target -Force | Out-Null

    # The resolved target was validated above. Retain only user-owned runtime
    # folders; everything else in build/ is generated and can be refreshed.
    # Keep the legacy data folder for one refresh; Paths migrates it atomically
    # to gamehq-data on the first GameHQ launch.
    $preservedNames = @('Captures', 'gamehq-data', 'saveplay-data', 'playhq-data')
    Get-ChildItem -LiteralPath $target -Force | Where-Object {
        $_.Name -notin $preservedNames
    } | ForEach-Object { Remove-GeneratedPath $_.FullName }
} else {
    if (Test-Path -LiteralPath $target) {
        Remove-GeneratedPath $target
    }
    New-Item -ItemType Directory -Path $target -Force | Out-Null
}

$app = Join-Path $target 'app'
New-Item -ItemType Directory -Path $app -Force | Out-Null

# Qt expects the real executable, its shared libraries, QML imports, and plugin
# subdirectories to live in one deployment tree.
Copy-Item -LiteralPath $realExe -Destination $app
& $deployTool --verbose 0 --qmldir (Join-Path $root 'src\ui\qml') --compiler-runtime (Join-Path $app 'GameHQ.exe')
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt failed ($LASTEXITCODE)"
}

# Qt Multimedia loads these FFmpeg libraries dynamically; windeployqt does not
# currently include them in this MinGW deployment.
Copy-Item (Join-Path $qtBin 'av*.dll') $app
Copy-Item (Join-Path $qtBin 'sw*.dll') $app -ErrorAction SilentlyContinue

$version = (Get-Content (Join-Path $root 'VERSION') -Raw).Trim()
Copy-Item -LiteralPath $launcherExe -Destination (Join-Path $target 'GameHQ.exe')
(Get-Content (Join-Path $PSScriptRoot 'README-dist.txt') -Raw).Replace('{VERSION}', $version) |
    Set-Content (Join-Path $target 'README.txt') -NoNewline
Copy-Item -LiteralPath (Join-Path $root 'LICENSE') -Destination (Join-Path $target 'LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $root 'THIRD_PARTY_NOTICES.md') -Destination (Join-Path $target 'THIRD_PARTY_NOTICES.md')
Copy-Item -LiteralPath (Join-Path $root 'licenses') -Destination (Join-Path $target 'licenses') -Recurse

# Qt 6.8+ ships SPDX metadata for its runtime modules. Include the modules used
# by GameHQ so recipients can inspect exact component and license attribution.
$qtSbom = Join-Path $qtBin '..\sbom'
if (Test-Path -LiteralPath $qtSbom -PathType Container) {
    $sbomTarget = Join-Path $target 'licenses\qt-sbom'
    New-Item -ItemType Directory -Path $sbomTarget -Force | Out-Null
    foreach ($module in @('qtbase', 'qtdeclarative', 'qtmultimedia', 'qtsvg')) {
        Get-ChildItem -LiteralPath $qtSbom -Filter ($module + '-*.spdx') -File |
            Where-Object { $_.Name -notlike '*.source.spdx' } |
            Copy-Item -Destination $sbomTarget
    }
}
New-Item -ItemType File -Path (Join-Path $target 'portable.flag') -Force | Out-Null

$programBytes = (Get-ChildItem -LiteralPath $app -Recurse -File |
    Measure-Object Length -Sum).Sum
$programBytes += (Get-Item -LiteralPath (Join-Path $target 'GameHQ.exe')).Length
$programBytes += (Get-Item -LiteralPath (Join-Path $target 'README.txt')).Length
$programBytes += (Get-Item -LiteralPath (Join-Path $target 'LICENSE.txt')).Length
$programBytes += (Get-Item -LiteralPath (Join-Path $target 'THIRD_PARTY_NOTICES.md')).Length
$size = [math]::Round($programBytes / 1MB, 1)
Write-Host "[package] done: $size MB"
Get-ChildItem -LiteralPath $target | ForEach-Object { Write-Host ("  " + $_.Name) }
