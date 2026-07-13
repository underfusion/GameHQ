# Branding

The visible product brand has one source of truth: `src/ui/qml/Brand.qml`.
Change its `name` and `slug` properties for a future product rename. CMake reads
the same file during configuration and generates `Brand.h`, so QML and C++ use
matching values without duplicated user-facing literals.

Technical compatibility identifiers (the `GameHQ` QML URI, executable names,
data migrations, and legacy registry/database paths) intentionally remain stable
unless a full package-identity migration is required. Keeping those identifiers
stable makes a future visible rename small and prevents another data migration.

After changing `Brand.qml`, reconfigure before building:

```powershell
tools\cmake\bin\cmake.exe -S . -B out
tools\cmake\bin\cmake.exe --build out
```
