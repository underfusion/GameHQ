# GameHQ Integration release checklist

This checklist prepares a `playnite-v*` release without authorizing publication.

## Before publishing

- Run `packaging/package.ps1` and retain the exact generated `.pext` filename.
- Run the plugin unit tests and `packaging/verify.ps1`.
- Confirm `VERSION`, `extension.yaml`, the project version, changelog, and
  `InstallerManifest.yaml` all name the same release.
- Complete the real-Playnite acceptance recorded in plan items `p5-4` and
  `p5-5`, including installed and portable Playnite where applicable.
- Confirm the current GameHQ public download remains supported.
- Capture the settings page in a representative Playnite theme for the release
  description and future add-on database entry.
- Obtain explicit publication approval.

## Publication

- Create the tag `playnite-vX.Y.Z` in the GameHQ monorepo.
- Publish the matching `GameHQ_Integration_X_Y_Z.pext` as the release asset.
- Replace the blank `PackageUrl` in `InstallerManifest.yaml` with the immutable
  release-asset URL and verify it downloads the expected package hash.
- Validate the final manifest and package with Playnite Toolbox.
- Record the release URL, package SHA-256, Toolbox result, and tested Playnite
  version as release evidence.

## After publishing

- Install from the published manifest on a clean Playnite profile.
- Confirm discovery, connection, launch, disable/uninstall, and update behavior.
- Only then begin the separate Playnite add-on database submission task.
