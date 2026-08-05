# Microsoft GameInput (vendored, pinned 3.5.262)

Vendored from the official `Microsoft.GameInput` NuGet package, version
**3.5.262** (the pinned version for the Modern Controller Backend phase).
Only the two MIT-licensed files are vendored; do not edit them locally.

| File | Origin in package | License | SHA-256 |
|---|---|---|---|
| `include/GameInput.h` | `native/include/GameInput.h` (v3 API) | MIT (in-file banner) | `fbb769bef01b133dbb62e3622a7137482cb52b642ea21de080cb98279ef9610f` |
| `src/GameInput.cpp` | `native/src/GameInput.cpp` (official loader) | MIT (in-file banner) | `0ecb098dc44a6b5be4a010a2267452f8a725cde6f198596997ed5b090680127b` |

Package pin (recorded 2026-08-04):

- Package: https://www.nuget.org/packages/Microsoft.GameInput/3.5.262
- Download: `https://www.nuget.org/api/v2/package/Microsoft.GameInput/3.5.262`
- Package archive SHA-256: `2654e45081588409f6326838e681d6b50ac533e2f24402421dd73c167744d24e`
- Runtime installer `redist/GameInputRedist.msi` SHA-256:
  `768efbb4d1d05fd1700ea71231d18f01e744f3960cf1aa5dd0b0899247253759`
- x64 `GameInputRedist.dll` 3.5.262 inside the MSI (941,736 bytes,
  extractable from the embedded cab for app-local deployment) SHA-256:
  `8e34fa4bd769798ddf49cc144e9aa97fa909e640d36982574e5c5e82d3f9cf2d`

## License split (important)

- The **public header and the loader source/static library are MIT** (changed
  by Microsoft in 3.5; see the package README "Version 3.5" notes).
- The **runtime redistributable** (`GameInputRedist.msi` and the
  `GameInputRedist.dll` it installs) stays under the **Microsoft Software
  License Terms** in the package's `LICENSE.txt`. It is distributed unmodified
  as a separately licensed Microsoft component — see `THIRD_PARTY_NOTICES.md`
  and `licenses/GameInput-MIT.txt`.

## How loading works

`src/GameInput.cpp` implements Microsoft's official resolution: it considers
System32 `GameInput.dll` (in-box v0 line), an installed `GameInputRedist.dll`
(System32 or Program Files) and an **app-local `GameInputRedist.dll` next to
the executable** (Agility-SDK-style side-by-side), loads the highest version
(ties prefer app-local), then resolves `GameInputInitialize`. GameHQ never
links the runtime statically and never requires it at startup — when nothing
resolves, the legacy Sony Raw Input/XInput/WinMM stack stays active.
`GameInputRuntimeProbe.exe` (built from `src/gameinput/`) demonstrates both
paths.

## Updating the pin

1. Download the new `.nupkg`, record its SHA-256 here.
2. Re-extract `native/include/GameInput.h` and `native/src/GameInput.cpp`,
   confirm the MIT banner is still present, refresh the hashes above.
3. Compile-check under the project MinGW toolchain (the header and loader
   need `UNICODE`/`_UNICODE` defined; no other patches are acceptable).
4. Update `THIRD_PARTY_NOTICES.md` and the packaging evidence if the runtime
   license or deployment model changed.
