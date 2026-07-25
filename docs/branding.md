# Branding

The visible product brand has one source of truth: `src/ui/qml/Brand.qml`.
Change its `name` and `slug` properties for a future product rename. CMake reads
the same file during configuration and generates `Brand.h`, so QML and C++ use
matching values without duplicated user-facing literals.

`Brand.qml` also holds the canonical project links (`websiteUrl`, `repositoryUrl`,
`releasesUrl`, `issuesUrl`), read by the Help and About settings pages so no page
hard-codes a URL. `websiteUrl` points at the GitHub repository until a real
public site is deployed.

Technical compatibility identifiers (the `GameHQ` QML URI, executable names,
data migrations, and legacy registry/database paths) intentionally remain stable
unless a full package-identity migration is required. Keeping those identifiers
stable makes a future visible rename small and prevents another data migration.

After changing `Brand.qml`, reconfigure before building:

```powershell
tools\cmake\bin\cmake.exe -S . -B out
tools\cmake\bin\cmake.exe --build out
```
