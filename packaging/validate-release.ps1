[CmdletBinding()]
param(
    [string]$ReleaseDirectory = 'dist\releases',
    [string]$GitTag = '',
    [Parameter(Mandatory = $true)]
    [ValidateSet('unsigned-beta', 'signed')]
    [string]$TrustMode,
    [ValidateSet('none', 'test')]
    [string]$ManifestMode = 'none',
    [switch]$SkipTests
)

$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
& (Join-Path $PSScriptRoot 'assert-privacy.ps1') -ReleaseDirectory $ReleaseDirectory
& (Join-Path $PSScriptRoot 'assert-license-compliance.ps1')
$releaseRoot = if ([System.IO.Path]::IsPathRooted($ReleaseDirectory)) {
    [System.IO.Path]::GetFullPath($ReleaseDirectory)
} else { [System.IO.Path]::GetFullPath((Join-Path $root $ReleaseDirectory)) }
$releaseRoot = $releaseRoot.TrimEnd('\', '/')
$approvedRoot = [System.IO.Path]::GetFullPath((Join-Path $root 'dist\releases')).TrimEnd('\', '/')
if (-not [System.StringComparer]::OrdinalIgnoreCase.Equals($releaseRoot, $approvedRoot)) {
    throw "Release validation is restricted to $approvedRoot"
}
$version = (Get-Content (Join-Path $root 'VERSION') -Raw).Trim()
$identity = Import-PowerShellDataFile (Join-Path $PSScriptRoot 'distribution-identity.psd1')
if ($version -notmatch '^\d+\.\d+\.\d+$') { throw 'VERSION is not valid semantic version text.' }
if ($GitTag -and $GitTag.TrimStart('v') -ne $version) {
    throw "Git tag $GitTag does not match VERSION $version."
}
if ($GitTag -and $ManifestMode -eq 'test') {
    throw 'Test-key manifests are local/CI evidence only and can never validate a publishable Git tag.'
}

$portableName = $identity.ArtifactPatterns.Portable -f $version
$updateName = $identity.ArtifactPatterns.Update -f $version
$setupName = $identity.ArtifactPatterns.Setup -f $version
$portableZip = Join-Path $releaseRoot $portableName
$updateZip = Join-Path $releaseRoot $updateName
$setup = Join-Path $releaseRoot $setupName
$checksum = "$updateZip.sha256"
$payloadRoot = Join-Path $root 'dist\.program-payload'
foreach ($required in @($setup, $portableZip, $updateZip, $checksum)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) { throw "Missing release artifact: $required" }
}
if ($ManifestMode -eq 'test') {
    foreach ($required in @('gamehq-release.json', 'gamehq-release.sig')) {
        if (-not (Test-Path -LiteralPath (Join-Path $releaseRoot $required) -PathType Leaf)) {
            throw "Missing test release-manifest artifact: $required"
        }
    }
    $tool = Join-Path $root 'tools\release-manifest\GameHQ.ReleaseManifest.Tool\GameHQ.ReleaseManifest.Tool.csproj'
    & dotnet run --project $tool --configuration Release --no-restore -- verify-test --release-dir $releaseRoot
    if ($LASTEXITCODE -ne 0) { throw 'Test-key release manifest verification failed.' }
} elseif ($TrustMode -eq 'signed') {
    throw 'Signed/Stable validation requires the production manifest mode, which is intentionally gated by t48.'
}
$setupInfo = (Get-Item -LiteralPath $setup).VersionInfo
if ($setupInfo.ProductVersion.Trim() -ne $version -or
    $setupInfo.FileVersion.Trim() -ne "$version.0" -or
    $setupInfo.CompanyName.Trim() -ne $identity.Publisher -or
    $setupInfo.ProductName.Trim() -ne $identity.ProductName) {
    throw 'Setup Windows metadata does not match the distribution identity and VERSION.'
}

$gamehqBuilt = @(
    $setup,
    (Join-Path $payloadRoot 'GameHQ.exe'),
    (Join-Path $payloadRoot 'GameHQUpdater.exe'),
    (Join-Path $payloadRoot 'app\GameHQ.exe')
)
$signatures = @($gamehqBuilt | ForEach-Object {
    $signature = Get-AuthenticodeSignature -FilePath $_
    [pscustomobject]@{ Path = $_; Status = [string]$signature.Status;
        Subject = if ($signature.SignerCertificate) { $signature.SignerCertificate.Subject } else { '' } }
})
if ($TrustMode -eq 'unsigned-beta') {
    $unexpected = @($signatures | Where-Object Status -ne 'NotSigned')
    if ($unexpected.Count -ne 0) {
        throw 'unsigned-beta mode requires every GameHQ-built artifact to be consistently unsigned.'
    }
} else {
    $invalid = @($signatures | Where-Object Status -ne 'Valid')
    if ($invalid.Count -ne 0) {
        throw 'signed trust mode rejects missing or invalid Authenticode signatures.'
    }
    $subjects = @($signatures.Subject | Select-Object -Unique)
    if ($subjects.Count -ne 1 -or [string]::IsNullOrWhiteSpace($subjects[0])) {
        throw 'signed trust mode requires one consistent publisher identity.'
    }
}
if (-not (Test-Path -LiteralPath $payloadRoot -PathType Container)) {
    throw "Missing neutral program payload: $payloadRoot"
}
$payloadRoots = @(Get-ChildItem -LiteralPath $payloadRoot -Force | ForEach-Object Name)
$requiredPayloadRoots = @('GameHQ.exe', 'GameHQUpdater.exe', 'README.txt', 'LICENSE.txt',
    'THIRD_PARTY_NOTICES.md', 'app', 'licenses')
foreach ($required in $requiredPayloadRoots) {
    if ($payloadRoots -notcontains $required) { throw "Neutral payload is missing $required" }
}
foreach ($forbidden in @('portable.flag', 'Captures', 'gamehq-data', 'saveplay-data', 'playhq-data')) {
    if ($payloadRoots -contains $forbidden) { throw "Neutral payload contains forbidden path: $forbidden" }
}
foreach ($entry in $payloadRoots) {
    if ($entry -notin $requiredPayloadRoots) { throw "Neutral payload contains unexpected path: $entry" }
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
function Get-ZipEntries([string]$Path) {
    $archive = [System.IO.Compression.ZipFile]::OpenRead($Path)
    try { return @($archive.Entries | ForEach-Object { $_.FullName.Replace('\', '/') }) }
    finally { $archive.Dispose() }
}
$portableEntries = Get-ZipEntries $portableZip
$updateEntries = Get-ZipEntries $updateZip
foreach ($required in @('GameHQ.exe', 'GameHQUpdater.exe', 'portable.flag', 'app/GameHQ.exe')) {
    if ($portableEntries -notcontains $required) { throw "Portable ZIP is missing $required" }
}
foreach ($required in @('GameHQ.exe', 'GameHQUpdater.pending.exe', 'app/GameHQ.exe', 'update-package.json')) {
    if ($updateEntries -notcontains $required) { throw "Update ZIP is missing $required" }
}
$allowedRoots = @('GameHQ.exe', 'GameHQUpdater.pending.exe', 'README.txt', 'LICENSE.txt',
    'THIRD_PARTY_NOTICES.md', 'app', 'licenses', 'update-package.json')
$forbiddenRoots = @('portable.flag', 'Captures', 'gamehq-data', 'saveplay-data', 'playhq-data', 'GameHQUpdater.exe')
foreach ($entry in $updateEntries) {
    $rootName = $entry.TrimEnd('/').Split('/')[0]
    if ($rootName -notin $allowedRoots) { throw "Update ZIP contains unexpected path: $entry" }
    if ($rootName -in $forbiddenRoots) { throw "Update ZIP contains forbidden path: $entry" }
}

$archive = [System.IO.Compression.ZipFile]::OpenRead($updateZip)
try {
    $entry = $archive.GetEntry('update-package.json')
    $reader = [System.IO.StreamReader]::new($entry.Open(), [System.Text.Encoding]::UTF8)
    try { $manifest = ($reader.ReadToEnd() | ConvertFrom-Json) } finally { $reader.Dispose() }
} finally { $archive.Dispose() }
if ($manifest.schemaVersion -ne 1 -or $manifest.productId -ne $identity.ProductId `
    -or $manifest.appVersion -ne $version -or $manifest.layoutVersion -ne 1 `
    -or $manifest.minimumUpdaterVersion -notmatch '^\d+\.\d+\.\d+$') {
    throw 'update-package.json is missing required or matching metadata.'
}
$expectedChecksum = "^([0-9a-f]{64}) \*$([regex]::Escape($updateName))$"
$checksumText = (Get-Content -LiteralPath $checksum -Raw).Trim()
if ($checksumText -notmatch $expectedChecksum) { throw 'Update checksum file has an invalid format or filename.' }
$actualHash = (Get-FileHash -LiteralPath $updateZip -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actualHash -ne $Matches[1]) { throw 'Update ZIP checksum does not match.' }
$packagedApp = Join-Path $root 'dist\GameHQ\app\GameHQ.exe'
$versionCheck = Start-Process -FilePath $packagedApp `
    -ArgumentList @('--assert-version', $version) -WindowStyle Hidden -Wait -PassThru
if ($versionCheck.ExitCode -ne 0) { throw 'Packaged application version does not match VERSION.' }
if ($ManifestMode -eq 'test') {
    foreach ($probe in @(
        @{ Path = $packagedApp; Arguments = @('--release-trust-self-test') },
        @{ Path = (Join-Path $payloadRoot 'GameHQUpdater.exe'); Arguments = @('--release-trust-self-test') }
    )) {
        $process = Start-Process -FilePath $probe.Path -ArgumentList $probe.Arguments `
            -WindowStyle Hidden -Wait -PassThru
        if ($process.ExitCode -ne 0) { throw "Release trust self-test failed: $($probe.Path)" }
    }
}
if (-not $SkipTests) {
    $localCtest = Join-Path $root 'tools\cmake\bin\ctest.exe'
    $ctest = if (Test-Path -LiteralPath $localCtest -PathType Leaf) {
        $localCtest
    } else {
        $command = Get-Command ctest.exe -ErrorAction Stop
        $command.Source
    }
    $env:PATH = (Join-Path $root 'tools\Qt\6.8.3\mingw_64\bin') + ';' + $env:PATH
    & $ctest --test-dir (Join-Path $root 'out') -R 'tst_(updatedownloader|updatepreflight|updateinstaller|updatertransaction)' --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Updater validation tests failed.' }
}
$toolchain = Import-PowerShellDataFile (Join-Path $PSScriptRoot 'inno-toolchain.psd1')
$evidencePaths = @($setup, $portableZip, $updateZip, $checksum)
if ($ManifestMode -eq 'test') {
    $evidencePaths += (Join-Path $releaseRoot 'gamehq-release.json')
    $evidencePaths += (Join-Path $releaseRoot 'gamehq-release.sig')
}
$artifactEvidence = @($evidencePaths | ForEach-Object {
    [ordered]@{
        fileName = [System.IO.Path]::GetFileName($_)
        size = (Get-Item -LiteralPath $_).Length
        sha256 = (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash.ToLowerInvariant()
    }
})
$evidence = [ordered]@{
    schemaVersion = 1
    version = $version
    trustMode = $TrustMode
    manifestMode = $ManifestMode
    innoSetup = [ordered]@{ version = $toolchain.Version; installerSha256 = $toolchain.Sha256 }
    compliance = [ordered]@{ license = 'passed'; privacy = 'passed' }
    artifacts = $artifactEvidence
    authenticode = $signatures
} | ConvertTo-Json -Depth 6
[System.IO.File]::WriteAllText((Join-Path $releaseRoot 'release-evidence.json'),
    $evidence + "`n", [System.Text.UTF8Encoding]::new($false))
Write-Host "[validate] release $version passed"
