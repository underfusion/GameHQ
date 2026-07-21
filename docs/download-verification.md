# Download Verification

## Official source

Download GameHQ only from:

https://github.com/underfusion/GameHQ/releases

Expected Windows artifacts use these names:

```text
GameHQ-<version>-win64-setup.exe
GameHQ-<version>-win64-portable.zip
GameHQ-<version>-win64-update.zip
GameHQ-<version>-win64-update.zip.sha256
gamehq-release.json
gamehq-release.sig
```

Setup is the recommended first-install package. Portable keeps its state beside
the application. The update ZIP belongs to GameHQ's staged updater and is not a
replacement installer.

## Check SHA-256

When a release publishes a SHA-256 value, calculate the downloaded file's hash
in PowerShell:

```powershell
Get-FileHash .\GameHQ-<version>-win64-setup.exe -Algorithm SHA256
```

Compare all 64 hexadecimal characters with the value on the same official
release. A mismatch means the file must not be run. A hash protects against
corruption only when the expected value comes from a trusted source; it is not
publisher identity by itself.

## Check Authenticode

For signed releases:

```powershell
Get-AuthenticodeSignature .\GameHQ-<version>-win64-setup.exe |
  Format-List Status,StatusMessage,SignerCertificate,TimeStamperCertificate
```

Require `Status: Valid`, the publisher stated in that release's security notes,
and a timestamp. Do not continue when the signature is invalid, absent from a
release advertised as signed, or names an unexpected publisher.

## Unsigned Beta builds

An unsigned Beta can show **Unknown publisher** even when downloaded from the
official repository. Before proceeding, confirm the official Releases URL,
exact version and filename, and every published hash or manifest. Do not use a
generic mirror. Never bypass a specific Defender malware/PUA detection; follow
[Troubleshooting](troubleshooting.md) and report it privately.

The Ed25519 release-manifest verifier is not yet active. Until it is shipped,
the release page and SHA-256 files cannot protect against a compromised release
account; this limitation is intentional and documented rather than hidden.
