# Assembles the release-ready portable folder at dist\GameHQ.
#
#   powershell -ExecutionPolicy Bypass -File packaging\make-dist.ps1

$ErrorActionPreference = 'Stop'

& (Join-Path $PSScriptRoot 'assemble-package.ps1') `
    -BuildDirectory 'out' `
    -Destination 'dist\GameHQ'
