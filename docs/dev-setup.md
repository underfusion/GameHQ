# Dev Setup & Toolchain

> The toolchain is project-local under `tools/` (git-ignored): no system installs or admin rights are required.

## Layout

| Tool | Version | Location |
|---|---|---|
| Qt (MinGW 64-bit kit) | 6.8.3 | `tools/Qt/6.8.3/mingw_64` |
| MinGW-w64 (GCC) | 13.1.0 | `tools/Qt/Tools/mingw1310_64` |
| CMake | 4.3.3 | `tools/cmake` |
| Ninja | 1.13.2 | `tools/ninja.exe` |
| aqt (Qt installer CLI) | 3.3.0 | `tools/aqt.exe` |

## Recreating `tools/` from scratch

```bat
cd /d I:\PROJECTS\Apps\GameHQ\tools
curl -L -o aqt.exe https://github.com/miurahr/aqtinstall/releases/latest/download/aqt.exe
curl -L -o ninja-win.zip https://github.com/ninja-build/ninja/releases/latest/download/ninja-win.zip && tar -xf ninja-win.zip
:: cmake: unzip the windows-x86_64 release zip from github.com/Kitware/CMake as tools\cmake
aqt install-qt   windows desktop 6.8.3 win64_mingw   --outputdir I:\PROJECTS\Apps\GameHQ\tools\Qt
aqt install-tool windows desktop tools_mingw1310     --outputdir I:\PROJECTS\Apps\GameHQ\tools\Qt
```

## Configure and build

Compiler internals belong in `out/`; `build/` is reserved for the clean local
portable package.

```bat
cd /d I:\PROJECTS\Apps\GameHQ
set PATH=I:\PROJECTS\Apps\GameHQ\tools\Qt\Tools\mingw1310_64\bin;I:\PROJECTS\Apps\GameHQ\tools\Qt\6.8.3\mingw_64\bin;%PATH%
tools\cmake\bin\cmake.exe -S . -B out -G Ninja -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_PREFIX_PATH=I:/PROJECTS/Apps/GameHQ/tools/Qt/6.8.3/mingw_64 ^
  -DCMAKE_C_COMPILER=I:/PROJECTS/Apps/GameHQ/tools/Qt/Tools/mingw1310_64/bin/gcc.exe ^
  -DCMAKE_CXX_COMPILER=I:/PROJECTS/Apps/GameHQ/tools/Qt/Tools/mingw1310_64/bin/g++.exe ^
  -DCMAKE_MAKE_PROGRAM=I:/PROJECTS/Apps/GameHQ/tools/ninja.exe
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

`ctest` needs the Qt runtime on `PATH`. In Git Bash use POSIX paths — a
`I:/...`-style entry is not translated and every test dies with
`0xc0000135` (DLL not found):

```sh
export PATH="/i/PROJECTS/Apps/GameHQ/tools/Qt/6.8.3/mingw_64/bin:/i/PROJECTS/Apps/GameHQ/tools/Qt/Tools/mingw1310_64/bin:$PATH"
```

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

## MSVC note

The production project currently uses the MinGW raw-ABI path for Windows
Graphics Capture and Media Foundation. CMake remains compiler-agnostic if a
future migration to an MSVC Qt kit becomes useful.
