# GameHQ licensing

## Active boundary

Beginning with version 0.7.1, the GameHQ core is licensed under **GNU GPL
version 3 only** (`GPL-3.0-only`). This covers the native application,
launcher, updater, installer and release scripts, tests, first-party tools, UI
resources, assets, and general documentation unless a more specific license is
stated.

The Playnite integration under `integrations/playnite/` remains under its local
MIT license. The public protocol specification at
`docs/integration-protocol.md` also remains MIT under
`docs/integration-protocol.LICENSE`. Dependencies and redistributed runtimes
retain the licenses identified in `THIRD_PARTY_NOTICES.md`,
`docs/dependency-licenses.md`, and `licenses/`.

`SPDX-License-Identifier: GPL-3.0-only` describes first-party core files unless
a file or its nearest directory has an explicit different license marker.
Explicit third-party notices and the two MIT exceptions take precedence over
that repository default.

## Historical releases

GameHQ revisions and releases before 0.7.1 remain available under the MIT terms
granted at their publication. Those permissions are not withdrawn. The exact
previous root license is preserved at `licenses/MIT-legacy.txt`.

The transition does not rewrite Git history or old tags. It adds no restriction
on commercial use, modification, forks, or paid redistribution beyond the
standard GNU GPL version 3 conditions.

## Release obligations

Every GameHQ binary release from 0.7.1 onward must publish a corresponding
`GameHQ-<version>-source.zip` and SHA-256 checksum built from the exact tagged
revision. Setup, Portable, and updater packages include
`licenses/SOURCE_OFFER.txt` binding the binary version, revision, tag, source
filename, hash, and public source URL.

The release gate rejects a modified GPL text, changed MIT exception text,
missing source artifacts, stale first-party MIT claims, or binary packages that
do not carry the matching source notice.

## Branding and support

Software permissions come from the applicable licenses. `TRADEMARKS.md`
separately explains how the GameHQ name and visual identity distinguish
official builds. Optional future funding or paid support would not change the
license already granted for a published version.
