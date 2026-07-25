#Requires -Version 5.1
<#
.SYNOPSIS
    Builds the GameHQ Integration Playnite plugin in Release configuration.
#>
[CmdletBinding()]
param(
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$csproj = Join-Path $root "src\GameHQ.Playnite\GameHQ.Playnite.csproj"

Write-Host "[playnite-build] dotnet build $csproj -c $Configuration"
dotnet build $csproj -c $Configuration
if ($LASTEXITCODE -ne 0) {
    throw "dotnet build failed with exit code $LASTEXITCODE"
}

Write-Host "[playnite-build] done"
