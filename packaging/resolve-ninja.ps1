function Resolve-GameHQNinja {
    [CmdletBinding()]
    param(
        [string]$NinjaPath = '',
        [string]$RepositoryRoot = ''
    )

    if ([string]::IsNullOrWhiteSpace($RepositoryRoot)) {
        $RepositoryRoot = [System.IO.Path]::GetFullPath(
            (Split-Path -Parent $PSScriptRoot))
    } else {
        $RepositoryRoot = [System.IO.Path]::GetFullPath($RepositoryRoot)
    }

    function Resolve-Candidate([string]$Candidate, [string]$SourceLabel) {
        $candidatePath = if ([System.IO.Path]::IsPathRooted($Candidate)) {
            [System.IO.Path]::GetFullPath($Candidate)
        } else {
            [System.IO.Path]::GetFullPath((Join-Path $RepositoryRoot $Candidate))
        }
        if (-not (Test-Path -LiteralPath $candidatePath -PathType Leaf)) {
            throw "$SourceLabel does not point to a Ninja executable: $candidatePath"
        }
        return (Resolve-Path -LiteralPath $candidatePath).Path
    }

    if (-not [string]::IsNullOrWhiteSpace($NinjaPath)) {
        return Resolve-Candidate $NinjaPath '-NinjaPath'
    }
    if (-not [string]::IsNullOrWhiteSpace($env:NINJA_EXE)) {
        return Resolve-Candidate $env:NINJA_EXE 'NINJA_EXE'
    }

    $pathCommand = Get-Command ninja -CommandType Application -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($pathCommand) {
        return [System.IO.Path]::GetFullPath($pathCommand.Source)
    }

    $developerFallback = Join-Path $RepositoryRoot 'tools\ninja.exe'
    if (Test-Path -LiteralPath $developerFallback -PathType Leaf) {
        return (Resolve-Path -LiteralPath $developerFallback).Path
    }

    throw ('Ninja executable was not found. Pass -NinjaPath, set NINJA_EXE, ' +
        'add ninja to PATH, or install the optional tools\ninja.exe developer fallback.')
}
