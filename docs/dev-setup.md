# Dev Setup & Toolchain

> The toolchain is project-local under `tools/` (git-ignored): no system installs or admin rights are required.

## Layout

| Tool | Version | Location |
|---|---|---|
| Qt (MinGW 64-bit kit) | 6.8.3 | `tools/Qt/6.8.3/mingw_64` |
| MinGW-w64 (GCC) | 13.1.0 | `tools/Qt/Tools/mingw1310_64` |
| CMake | 4.3.3 | `tools/cmake` |
| Ninja | 1.13.2 | `tools/Ninja/1.13.2/ninja.exe` |
| aqt (Qt installer CLI) | 3.3.0 | `tools/aqt.exe` |

## Recreating `tools/` from scratch

```bat
cd /d C:\path\to\GameHQ
curl -L -o tools\aqt.exe https://github.com/miurahr/aqtinstall/releases/latest/download/aqt.exe
powershell -ExecutionPolicy Bypass -File packaging\bootstrap-ninja.ps1
:: cmake: unzip the windows-x86_64 release zip from github.com/Kitware/CMake as tools\cmake
tools\aqt.exe install-qt   windows desktop 6.8.3 win64_mingw   --outputdir tools\Qt
tools\aqt.exe install-tool windows desktop tools_mingw1310     --outputdir tools\Qt
```

## Configure and build

Compiler internals belong in `out/`; `build/` is reserved for the clean local
portable package.

```bat
cd /d C:\path\to\GameHQ
set GAMEHQ_ROOT=%CD%
set PATH=%GAMEHQ_ROOT%\tools\Qt\Tools\mingw1310_64\bin;%GAMEHQ_ROOT%\tools\Qt\6.8.3\mingw_64\bin;%PATH%
tools\cmake\bin\cmake.exe -S . -B out -G Ninja -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_PREFIX_PATH=%GAMEHQ_ROOT%/tools/Qt/6.8.3/mingw_64 ^
  -DCMAKE_C_COMPILER=%GAMEHQ_ROOT%/tools/Qt/Tools/mingw1310_64/bin/gcc.exe ^
  -DCMAKE_CXX_COMPILER=%GAMEHQ_ROOT%/tools/Qt/Tools/mingw1310_64/bin/g++.exe ^
  -DCMAKE_MAKE_PROGRAM=%GAMEHQ_ROOT%/tools/Ninja/1.13.2/ninja.exe
tools\cmake\bin\cmake.exe --build out
```

## Assemble runtime and run

The real executable needs its Qt libraries, plugins, and QML imports in one
runtime tree. Assemble them below `build/app/`, then use the root launcher:

```bat
powershell -ExecutionPolicy Bypass -File packaging\assemble-package.ps1 -BuildDirectory out -Destination build -PreserveUserData
build\GameHQ.exe
```

Normally run `start.bat` instead. It configures `out/` when missing, performs an
incremental build, safely refreshes the clean `build/` package while preserving
captures and application data, and restarts GameHQ.

Portable dev data lands in `build/gamehq-data/` and captures in
`build/Captures/` (see [storage.md](storage.md)). Package structure and release
assembly are documented in [packaging.md](packaging.md).

## Tests

Automated tests are opt-in, so a normal build and `start.bat` are unaffected.
Configure once with the flag on, then build and run:

```sh
tools/cmake/bin/cmake.exe -S . -B out -DGAMEHQ_BUILD_TESTS=ON
tools/cmake/bin/cmake.exe --build out
tools/cmake/bin/ctest.exe --test-dir out --output-on-failure
```

`ctest` prepends the Qt runtime directory to each test's `PATH` itself
(`ENVIRONMENT_MODIFICATION` in `tests/CMakeLists.txt`), so the command above
needs no environment setup. Without it Windows answered with one modal
"Qt6Core.dll was not found" box per test and `ctest` hung until each was
dismissed.

Running a test exe **directly** still needs Qt on `PATH`. In Git Bash use POSIX
paths — a `I:/...`-style entry is not translated and the test dies with
`0xc0000135` (DLL not found):

```sh
export PATH="/i/PROJECTS/Apps/GameHQ/tools/Qt/6.8.3/mingw_64/bin:/i/PROJECTS/Apps/GameHQ/tools/Qt/Tools/mingw1310_64/bin:$PATH"
```

Tests that reach `GameIconCache::iconPathForExecutable` must use `QTEST_MAIN`
(a `QApplication`). Its `QFileIconProvider` fallback is a QtWidgets class and
segfaults under `QTEST_GUILESS_MAIN`.

Every test carries a CTest `TIMEOUT`: 120 s by default, 300 s for the ones that
drive child processes (`tst_integrationservice`, `tst_integrationclient`,
`tst_portableprofileimporter`) and 600 s for the updater end-to-end pair
(`tst_updatertransaction`, `tst_updateinstaller`). Without a bound, one hung
test consumes the whole CI job budget and reports nothing useful.

CI also re-runs the concurrency-sensitive suites three times each
(`ctest --repeat until-fail:3 -R 'tst_(integrationservice|integrationclient|integrationprotocol|capturepublisher|capturedatabaserepair|processidentity|portableprofileimporter|updatertransaction)'`).
A race in an IPC handshake, an interrupted update, importer recovery, capture
naming or database repair passes most runs; one green run is not evidence it is
gone. Run the same command locally before touching any of those paths.

The Playnite plugin pins its minimum SDK in `integrations/playnite/global.json`,
so `dotnet restore --locked-mode` and `dotnet test` fail loudly on an SDK older
than the projects need instead of producing a subtly different build.

Test exes land in `out/` and can be run directly (`./out/tst_gameidentity.exe`).
Their console output is not always captured when run from a tool-driven shell;
`-o file,txt` writes the full per-test report regardless:

```sh
./out/tst_gameidentity.exe -o results.txt,txt
```

Scope is **pure logic only** — no database, no GUI, no game process:
`tst_gameidentity` (folder names, identity keys, path→game inference),
`tst_configmanager` (defaults vs overrides, reset, save/reload, forward
compatibility) and `tst_gamerowrepair` (duplicate display-name preference).
Each test compiles the few sources it needs directly, since `GameHQ` is an
executable with no library to link against. If a unit needs half the app to
build, it is not pure logic and does not belong here.

The one deliberate exception is storage: correctness there is about what
survives a failure, which cannot be checked without a real database. Those
tests use a throwaway SQLite file under `QTemporaryDir`. `tst_capturedatabaserepair`
injects a failure into each durable step of the startup repair pass with a
SQLite trigger that raises `ABORT`, then asserts the whole pass rolled back —
including the "repairs done" marker, so the pass is retried instead of skipped
forever.

## MSVC note

The production project currently uses the MinGW raw-ABI path for Windows
Graphics Capture and Media Foundation. CMake remains compiler-agnostic if a
future migration to an MSVC Qt kit becomes useful.
