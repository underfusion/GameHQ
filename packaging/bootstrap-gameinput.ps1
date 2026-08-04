[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$toolchain = Import-PowerShellDataFile (Join-Path $PSScriptRoot 'gameinput-toolchain.psd1')
$installRoot = Join-Path $root "tools\GameInput\$($toolchain.Version)"
$runtime = Join-Path $installRoot 'GameInputRedist.dll'

function Assert-Hash([string]$Path, [string]$Expected, [string]$Label) {
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $Expected) {
        throw "$Label checksum mismatch: $actual"
    }
}

if (Test-Path -LiteralPath $runtime -PathType Leaf) {
    Assert-Hash $runtime $toolchain.X64RuntimeSha256 'GameInput x64 runtime'
    Write-Host "[gameinput] ready: $runtime"
    return
}

$downloadRoot = Join-Path $root 'tools\.downloads'
$package = Join-Path $downloadRoot "Microsoft.GameInput.$($toolchain.Version).nupkg"
$packageZip = Join-Path $downloadRoot "Microsoft.GameInput.$($toolchain.Version).zip"
$packageRoot = Join-Path $downloadRoot "Microsoft.GameInput.$($toolchain.Version)"
$cabPath = Join-Path $downloadRoot "GameInputRedist-$($toolchain.Version).cab"
$cabRoot = Join-Path $downloadRoot "GameInputRedist-$($toolchain.Version)"

New-Item -ItemType Directory -Path $downloadRoot -Force | Out-Null
if (-not (Test-Path -LiteralPath $package -PathType Leaf)) {
    Invoke-WebRequest -UseBasicParsing -Uri $toolchain.PackageUrl -OutFile $package
}
Assert-Hash $package $toolchain.PackageSha256 'Microsoft.GameInput package'

Copy-Item -LiteralPath $package -Destination $packageZip -Force
if (Test-Path -LiteralPath $packageRoot) {
    Remove-Item -LiteralPath $packageRoot -Recurse -Force
}
Expand-Archive -LiteralPath $packageZip -DestinationPath $packageRoot
$msi = Join-Path $packageRoot 'redist\GameInputRedist.msi'
Assert-Hash $msi $toolchain.MsiSha256 'GameInput redistributable MSI'

# The signed MSI stores every architecture in one embedded CAB. Windows
# Installer exposes that stream without installing or registering anything.
$installer = New-Object -ComObject WindowsInstaller.Installer
$database = $installer.OpenDatabase($msi, 0)
$view = $database.OpenView('SELECT `Name`, `Data` FROM `_Streams`')
$view.Execute()
$foundCab = $false
while ($record = $view.Fetch()) {
    if ($record.StringData(1) -notlike '*.cab') { continue }
    $remaining = $record.DataSize(2)
    $stream = [System.IO.File]::Open(
        $cabPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
    try {
        while ($remaining -gt 0) {
            $take = [Math]::Min(32767, $remaining)
            $chunk = $record.ReadStream(2, $take, 1)
            $bytes = [byte[]][char[]]$chunk
            $stream.Write($bytes, 0, $bytes.Length)
            $remaining -= $bytes.Length
        }
    } finally {
        $stream.Dispose()
    }
    $foundCab = $true
    break
}
if (-not $foundCab) { throw 'GameInput MSI contains no embedded CAB.' }

if (Test-Path -LiteralPath $cabRoot) {
    Remove-Item -LiteralPath $cabRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $cabRoot -Force | Out-Null
& (Join-Path $env:SystemRoot 'System32\expand.exe') $cabPath '-F:*' $cabRoot | Out-Null
if ($LASTEXITCODE -ne 0) { throw "GameInput CAB extraction failed ($LASTEXITCODE)." }

$candidate = Get-ChildItem -LiteralPath $cabRoot -File | Where-Object {
    $_.Length -eq 941736 -and
    (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() `
        -eq $toolchain.X64RuntimeSha256
} | Select-Object -First 1
if (-not $candidate) { throw 'Pinned GameInput x64 runtime was not found in the MSI.' }

New-Item -ItemType Directory -Path $installRoot -Force | Out-Null
Copy-Item -LiteralPath $candidate.FullName -Destination $runtime
Assert-Hash $runtime $toolchain.X64RuntimeSha256 'GameInput x64 runtime'
Write-Host "[gameinput] installed $($toolchain.Version): $runtime"
