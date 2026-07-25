# GameHQ licensing authority audit

Status: Passed for repository authorship and inbound human contributions.

Audit date: 2026-07-22

Repository: `underfusion/GameHQ`

Audited branch: `dev`

Audited revision: `8a1b3484eed003dd54736e88f8e2505e7426436a`

This is an engineering due-diligence record, not jurisdiction-specific legal
advice. It intentionally omits personal email addresses.

## Decision

The visible GameHQ product history has one human contributor identity:
`underfusion`. No external human contribution, co-author, imported patch, fork
pull request, or conflicting copyright claim was found. The repository's
current MIT notice identifies `underfusion` as the 2026 copyright holder, and
the maintainer has decided to retain MIT for the current project line.

The authorship gate therefore passes for the first-party repository work. The
separate dependency and distributed-asset audit remains required for release
provenance and third-party notice accuracy.

If undisclosed employer, school, client, or contractual ownership applies to
any first-party work, this conclusion is invalid and distribution must stop for
a maintainer or legal decision.

## Repository evidence

| Check | Evidence | Result |
|---|---|---|
| Repository identity | Public, non-fork repository `underfusion/GameHQ`; owner and only listed collaborator is `underfusion` | Pass |
| Product history | 68 commits reachable from `dev`, all authored as `underfusion` using one stable redacted email identity | Pass |
| Public contributors | GitHub contributors API lists only `underfusion` | Pass |
| Pull requests | PRs 1 through 7 are same-repository PRs authored, committed, and merged by `underfusion` | Pass |
| Attribution trailers | No `Co-authored-by`, `Signed-off-by`, `Reviewed-by`, `Acked-by`, `Patch-by`, or equivalent contributor trailers | Pass |
| Identity mapping | No `.mailmap`; no additional human identity requires consolidation | Pass |
| Submodules | No Git submodules | Pass |
| Automation-only history | Two `CCGUI Plan Sync` commits exist on a disconnected automation metadata branch, not the product history | Excluded from product authorship |
| Working state | Audit began from a clean working tree; `main` is an ancestor of audited `dev` | Pass |

The public PR review used the GitHub repository data as well as local Git
history. No external fork owner or external PR author was present.

## First-party and external boundaries

The following are treated as first-party work for the authorship audit because
their tracked history is entirely under the sole product contributor:

- GameHQ application and updater source;
- CMake, installer, packaging, release, and development scripts;
- first-party tests and documentation;
- the Playnite integration source and protocol documentation;
- first-party QML and SVG interface sources.

The following are not silently classified as first-party merely because they
were committed by the maintainer:

- Qt, FFmpeg, compiler-runtime, miniz, Monocypher, and BouncyCastle components;
- icons, screenshots, sounds, fonts, themes, sample media, or generated assets
  whose original provenance must be independently verified;
- future AI models, voices, datasets, SDKs, or prompt packs.

No vendored upstream dependency source was found among the tracked files.
Existing third-party license texts and notices are evidence inputs for `t60`,
not proof that every packaged component or asset is already cleared.

The tracked binary assets were introduced by an `underfusion` commit and expose
no embedded author metadata. That establishes repository provenance only; `t60`
must still establish their origin and redistribution rights before release.

## Licensing assessment

The repository uses the standard MIT/Expat license at its root. The Playnite
integration and public protocol include local MIT copies so separately packaged
artifacts retain an explicit grant. Third-party components remain under their
documented upstream terms.

The maintainer reviewed an unpublished GPL migration and then chose to keep
GameHQ under MIT. The local migration commit was removed from the branch before
publication, no public tag or release used GPL, and no historical license grant
was rewritten. This audit remains useful evidence for contribution authority,
but it does not authorize publishing a release or submitting SignPath; those
actions retain separate approval boundaries.

## Reproduction checklist

The audit used these evidence classes:

```text
git rev-parse --show-toplevel
git remote -v
git status --short
git shortlog -sne --all
git log --all (authors, committers, bodies, trailers, merges and roots)
git submodule status
git ls-files
GitHub repository, contributor, collaborator and PR metadata
```

Email-bearing output was reviewed only in redacted or fingerprinted form and is
not copied into this document.

## Dependency-audit boundary

`t60` verifies the provenance, license version, compatibility, notice
requirements, source availability, and packaged presence of every dependency
and distributed asset. Any unknown or restricted item blocks release until it
is removed, replaced, reimplemented, or explicitly cleared.
