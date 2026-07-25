[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$toolchain = Import-PowerShellDataFile (Join-Path $PSScriptRoot 'ninja-toolchain.psd1')
$installRoot = Join-Path $root "tools\Ninja\$($toolchain.Version)"
$ninja = Join-Path $installRoot 'ninja.exe'

if (-not (Test-Path -LiteralPath $ninja -PathType Leaf)) {
    $downloadRoot = Join-Path $root 'tools\.downloads'
    $archive = Join-Path $downloadRoot "ninja-$($toolchain.Version)-win.zip"
    $extractRoot = Join-Path $downloadRoot "ninja-$($toolchain.Version)-extract"
    New-Item -ItemType Directory -Path $downloadRoot -Force | Out-Null
    if (-not (Test-Path -LiteralPath $archive -PathType Leaf)) {
        Invoke-WebRequest -UseBasicParsing -Uri $toolchain.Url -OutFile $archive
    }

    $actualHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actualHash -ne $toolchain.Sha256) {
        throw "Ninja archive checksum mismatch: $actualHash"
    }

    if (Test-Path -LiteralPath $extractRoot) {
        Remove-Item -LiteralPath $extractRoot -Recurse -Force
    }
    New-Item -ItemType Directory -Path $extractRoot -Force | Out-Null
    try {
        Expand-Archive -LiteralPath $archive -DestinationPath $extractRoot
        $extractedNinja = Join-Path $extractRoot 'ninja.exe'
        if (-not (Test-Path -LiteralPath $extractedNinja -PathType Leaf)) {
            throw 'Pinned Ninja archive does not contain ninja.exe.'
        }
        New-Item -ItemType Directory -Path $installRoot -Force | Out-Null
        Copy-Item -LiteralPath $extractedNinja -Destination $ninja -Force
    } finally {
        if (Test-Path -LiteralPath $extractRoot) {
            Remove-Item -LiteralPath $extractRoot -Recurse -Force
        }
    }
}

$version = (& $ninja --version).Trim()
if ($LASTEXITCODE -ne 0 -or $version -ne $toolchain.Version) {
    throw "Pinned Ninja version mismatch: expected $($toolchain.Version), got '$version'."
}

$resolved = (Resolve-Path -LiteralPath $ninja).Path
Write-Host "[ninja] ready: $resolved ($version)"
Write-Output $resolved
