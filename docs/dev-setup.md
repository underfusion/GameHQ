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

## MSVC note

The production project currently uses the MinGW raw-ABI path for Windows
Graphics Capture and Media Foundation. CMake remains compiler-agnostic if a
future migration to an MSVC Qt kit becomes useful.
