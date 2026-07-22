# Fails closed when repository or release inputs contain private identity or secret material.
[CmdletBinding()]
param(
    [string]$ReleaseDirectory = 'dist\releases',
    [switch]$SkipRelease
)

$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$failures = [System.Collections.Generic.List[string]]::new()

function Test-Text([string]$DisplayPath, [byte[]]$Bytes, [string[]]$ForbiddenLiterals) {
    if ($Bytes.Length -gt 5MB -or $Bytes -contains 0) { return }
    $text = [Text.Encoding]::UTF8.GetString($Bytes)
    $normalizedDisplayPath = $DisplayPath.Replace('\', '/')
    $isPrivacyScanner = $normalizedDisplayPath -match '(^|/)packaging/assert-privacy\.ps1$'
    if ($text -match '-----BEGIN (RSA |EC |OPENSSH )?PRIVATE KEY-----') {
        $failures.Add("private-key material: $DisplayPath")
    }
    if ($text -match '(?i)\b(gh[pousr]_[A-Za-z0-9_]{20,}|github_pat_[A-Za-z0-9_]{20,}|AKIA[0-9A-Z]{16})\b') {
        $failures.Add("credential-shaped token: $DisplayPath")
    }
    if (-not $isPrivacyScanner -and
        ($text -match '(?i)[A-Z]:\\Users\\(?!Public\\|Default\\|All Users\\|USERNAME\\|<[^>]+>\\)[^\\\s]+\\' -or
         $text -match '(?i)/home/(?!user/|<[^>]+>/)[^/\s]+/')) {
        $failures.Add("personal absolute path: $DisplayPath")
    }
    foreach ($literal in $ForbiddenLiterals) {
        if ($literal -and $text.IndexOf($literal, [StringComparison]::OrdinalIgnoreCase) -ge 0) {
            $failures.Add("private identity literal: $DisplayPath")
        }
    }
}

function Test-PathName([string]$DisplayPath) {
    $normalized = $DisplayPath.Replace('\', '/')
    $name = [IO.Path]::GetFileName($normalized)
    if ($name -match '(?i)^\.env($|\.)|\.(pfx|p12|pem|key|kdbx)$' -or
        $normalized -match '(?i)(^|/)(cookies?|browser-profile|private-backups?)(/|$)' -or
        $name -match '(?i)signpath.*(filled|response|submission|correspondence)' -or
        $name -match '(?i)backup.*\.zip$') {
        $failures.Add("private or secret filename: $DisplayPath")
    }
}

$historyEmails = @(& git -C $root log --all --format='%ae%n%ce' |
    Where-Object { $_ -and $_ -notlike '*@users.noreply.github.com' } | Sort-Object -Unique)
$extraLiterals = @()
if ($env:GAMEHQ_PRIVATE_PII) {
    $extraLiterals = @($env:GAMEHQ_PRIVATE_PII -split '[\r\n;]+' | Where-Object { $_ })
}
$forbiddenLiterals = @($historyEmails + $extraLiterals | Sort-Object -Unique)

$relativeFiles = @(& git -C $root ls-files --cached --others --exclude-standard) |
    Where-Object { $_ -notmatch '^(docs/plans|build|out|dist|tools/Qt|tools/InnoSetup)/' }
foreach ($relative in $relativeFiles) {
    Test-PathName $relative
    $path = Join-Path $root $relative
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        Test-Text $relative ([IO.File]::ReadAllBytes($path)) $forbiddenLiterals
    }
}

if (-not $SkipRelease) {
    $releaseRoot = if ([IO.Path]::IsPathRooted($ReleaseDirectory)) {
        [IO.Path]::GetFullPath($ReleaseDirectory)
    } else { [IO.Path]::GetFullPath((Join-Path $root $ReleaseDirectory)) }
    $approvedRoot = [IO.Path]::GetFullPath((Join-Path $root 'dist\releases'))
    if (-not [StringComparer]::OrdinalIgnoreCase.Equals(
            $releaseRoot.TrimEnd('\', '/'), $approvedRoot.TrimEnd('\', '/'))) {
        throw "Privacy validation is restricted to $approvedRoot"
    }
    if (Test-Path -LiteralPath $releaseRoot -PathType Container) {
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        foreach ($file in Get-ChildItem -LiteralPath $releaseRoot -File) {
            Test-PathName "release/$($file.Name)"
            Test-Text "release/$($file.Name)" ([IO.File]::ReadAllBytes($file.FullName)) $forbiddenLiterals
            if ($file.Extension -in @('.zip', '.pext')) {
                $archive = [IO.Compression.ZipFile]::OpenRead($file.FullName)
                try {
                    foreach ($entry in $archive.Entries) {
                        Test-PathName "$($file.Name)/$($entry.FullName)"
                        if ($entry.Length -le 5MB) {
                            $stream = $entry.Open()
                            try {
                                $memory = [IO.MemoryStream]::new()
                                try { $stream.CopyTo($memory); Test-Text "$($file.Name)/$($entry.FullName)" $memory.ToArray() $forbiddenLiterals }
                                finally { $memory.Dispose() }
                            } finally { $stream.Dispose() }
                        }
                    }
                } finally { $archive.Dispose() }
            }
        }
    }
}

if ($failures.Count) {
    throw "Privacy validation failed:`n - $($failures -join "`n - ")"
}
Write-Host "[privacy] repository and release inputs contain no blocked private data"
