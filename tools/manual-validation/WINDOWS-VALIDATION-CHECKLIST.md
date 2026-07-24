# GameHQ Windows validation

Record the artifact SHA-256, Windows edition/build, Defender definition version,
SmartScreen setting, Smart App Control state, result, and screenshot filename.

## Automated or script-assisted mechanics

- Install without elevation and into a custom directory.
- Confirm Start Menu/Desktop shortcuts and uninstall registry metadata.
- Upgrade an older Setup, reinstall, uninstall, and verify AppData/Captures remain.
- Confirm portable and installed copies coexist without sharing data.
- Confirm Setup refuses a running GameHQ or active updater transaction.
- Exercise offline update check, cached result, read-only path, locked file, and stale cleanup.
- Run `Start-MpScan -ScanType CustomScan -ScanPath <artifact-or-folder>` when Defender is available.

## Human clean-VM observations

- Launch Setup and Portable and record SmartScreen wording.
- Record Smart App Control audit/enforcement behavior where available.
- Record every Defender malware/PUA result; do not disable protections or add broad exclusions.
- After signing, verify publisher and RFC 3161 timestamp on launcher, app, updater, uninstaller, and Playnite DLL.
- Exercise visible install, upgrade, uninstall, rollback, HidHide elevation, and Playnite maintenance behavior.

Stop release promotion on any Defender detection or signature/hash mismatch.
