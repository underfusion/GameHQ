#Requires -Version 5.1
Set-StrictMode -Version Latest

function Get-NormalizedPath {
    param([Parameter(Mandatory = $true)][string]$Path)
    return [System.IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
}

function Test-SameOrChildPath {
    param(
        [Parameter(Mandatory = $true)][string]$Candidate,
        [Parameter(Mandatory = $true)][string]$Parent
    )
    $candidatePath = Get-NormalizedPath $Candidate
    $parentPath = Get-NormalizedPath $Parent
    if ($candidatePath.Equals($parentPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $true
    }
    return $candidatePath.StartsWith(
        $parentPath + [System.IO.Path]::DirectorySeparatorChar,
        [System.StringComparison]::OrdinalIgnoreCase
    )
}

function Assert-SafeWorkspaceRoot {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [switch]$Create
    )
    if (-not [System.IO.Path]::IsPathRooted($Path)) {
        throw "Validation workspace must be an explicit absolute path: $Path"
    }

    $full = Get-NormalizedPath $Path
    $driveRoot = Get-NormalizedPath ([System.IO.Path]::GetPathRoot($full))
    if ($full.Equals($driveRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "A drive root cannot be used as a validation workspace: $full"
    }
    if (Test-SameOrChildPath $full $ProjectRoot) {
        throw "Validation workspace must be outside the GameHQ project: $full"
    }

    $protectedRoots = @(
        [Environment]::GetFolderPath('Windows'),
        [Environment]::GetFolderPath('ProgramFiles'),
        [Environment]::GetFolderPath('ProgramFilesX86')
    ) | Where-Object { $_ }
    foreach ($protected in $protectedRoots) {
        if (Test-SameOrChildPath $full $protected) {
            throw "Protected system path cannot be used as a validation workspace: $full"
        }
    }

    $profile = [Environment]::GetFolderPath('UserProfile')
    if ($profile -and $full.Equals((Get-NormalizedPath $profile), [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "The user profile root cannot be used as a validation workspace: $full"
    }
    if (Test-Path -LiteralPath $full -PathType Leaf) {
        throw "Validation workspace points to a file: $full"
    }
    if (-not (Test-Path -LiteralPath $full -PathType Container)) {
        if (-not $Create) {
            throw "Validation workspace does not exist: $full"
        }
        New-Item -ItemType Directory -Path $full | Out-Null
    }
    return $full
}

function Assert-ValidationRun {
    param([Parameter(Mandatory = $true)][string]$RunRoot)
    $full = Get-NormalizedPath $RunRoot
    $marker = Join-Path $full '.gamehq-validation-run.json'
    if (-not (Test-Path -LiteralPath $marker -PathType Leaf)) {
        throw "Missing GameHQ validation marker: $marker"
    }
    $metadata = Get-Content -LiteralPath $marker -Raw | ConvertFrom-Json
    if (-not $metadata.runRoot -or
        -not $full.Equals((Get-NormalizedPath $metadata.runRoot), [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Validation marker does not belong to this run root: $full"
    }
    return $full
}

function Assert-ChildOfValidationRun {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$RunRoot
    )
    $full = Get-NormalizedPath $Path
    $run = Assert-ValidationRun $RunRoot
    if (-not (Test-SameOrChildPath $full $run) -or
        $full.Equals($run, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Path must be a child of the disposable validation run: $full"
    }
    return $full
}

function Get-ValidationFileRecords {
    param([Parameter(Mandatory = $true)][string]$FixtureRoot)
    $root = Get-NormalizedPath $FixtureRoot
    $records = @()
    foreach ($relativeRoot in @('Captures', 'gamehq-data')) {
        $ownedRoot = Join-Path $root $relativeRoot
        if (-not (Test-Path -LiteralPath $ownedRoot -PathType Container)) {
            continue
        }
        foreach ($file in Get-ChildItem -LiteralPath $ownedRoot -File -Recurse | Sort-Object FullName) {
            $relative = $file.FullName.Substring($root.Length).TrimStart('\', '/') -replace '/', '\'
            $records += [pscustomobject]@{
                path = $relative
                size = [int64]$file.Length
                sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
            }
        }
    }
    return @($records)
}
