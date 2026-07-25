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

- Run `packaging/prepare-release.ps1`; retain its package, SHA-256, Toolbox
  results, notes, and release evidence.
- Create the tag `playnite-vX.Y.Z` in the GameHQ monorepo.
- Publish the matching `GameHQ_Playnite_Integration_X_Y_Z.pext` as the release
  asset in a dedicated plugin release with `make_latest=false`. A plugin
  release must never replace the Latest GameHQ application release.
- Verify the public package URL and SHA-256 before exposing it through the
  installer manifest.
- Prepend the new version to `InstallerManifest.yaml` with its immutable
  `playnite-vX.Y.Z` release-asset URL. Preserve all older package entries.
- Validate the final manifest and package with Playnite Toolbox.
- Record the release URL, package SHA-256, Toolbox result, and tested Playnite
  version as release evidence.
- Keep `AddonManifest.yaml` pointed at the stable raw `main` installer-manifest
  URL. Once the first add-on database pull request is merged, later package
  versions are discovered from `InstallerManifest.yaml` without another
  database pull request.

## After publishing

- Merge the new manifest entry only after its public package URL is verified.
- Validate the raw `main` add-on manifest with Playnite Toolbox.
- Install the previous version on a clean Playnite profile and confirm Playnite
  discovers and installs the new version without losing user settings.
- Confirm discovery, connection, launch, disable/uninstall, and reconnect behavior.
- Only then begin the separate Playnite add-on database submission task.
