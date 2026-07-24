#Requires -Version 5.1
$ErrorActionPreference = 'Stop'
$checklist = Join-Path $PSScriptRoot 'WINDOWS-VALIDATION-CHECKLIST.md'
if (-not (Test-Path -LiteralPath $checklist -PathType Leaf)) { throw "Checklist not found: $checklist" }
Start-Process -FilePath 'notepad.exe' -ArgumentList $checklist
