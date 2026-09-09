# Windows x64 build baseline

T-013 establishes the build-only skeleton for the Windows application, worker,
core library, media adapter and tests. It deliberately contains no domain,
media-processing or analysis behavior.

## Fixed toolchain

- Visual Studio 2022 Build Tools: MSVC 19.44.35228, x64 host and target.
- Generator: CMake 3.31 or newer with Ninja.
- Runtime: dynamic MSVC CRT (`/MD` for Release and RelWithDebInfo, `/MDd` for Debug).
- Qt: reuse the existing shared Release+Debug Qt 6.11.2 SDK at
  `C:\sr\q\qt6112`. The project validates the exact SDK hashes produced by
  T-012 and never downloads or rebuilds Qt.
- Non-Qt dependencies: vcpkg tag `2026.07.29`, baseline
  `9e593bb18ea69cc5095e012465dcd675a822ed0d`, custom
  `x64-windows-space-rhythm` dynamic-linkage triplet.

The default manifest installs GoogleTest 1.17.0 only. Optional manifest features
lock the agreed future dependency families without activating product behavior:

| Feature | Locked port | Selection boundary |
| --- | --- | --- |
| `media` | FFmpeg 8.1.2 | API libraries only; LGPLv3 mode; GPL and nonfree features excluded |
| `video-analysis` | OpenCV 4.12.0 | classic primitives; DNN, Qt and OpenCV's FFmpeg feature excluded |
| `audio-analysis` | KissFFT 131.2.0 | FFT dependency only |

Do not enable optional features until the owning implementation task is ready to
integrate and verify them. Release license review remains mandatory.

## Local build

Bootstrap the pinned vcpkg checkout once:

```powershell
git clone --branch 2026.07.29 --depth 1 https://github.com/microsoft/vcpkg.git C:\sr\tools\vcpkg-2026.07.29
& C:\sr\tools\vcpkg-2026.07.29\bootstrap-vcpkg.bat -disableMetrics
```

Then invoke the checked entry point from a normal PowerShell session. It locates
Build Tools and imports the x64 developer environment itself:

```powershell
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-debug -Stage All -Clean
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset windows-msvc-x64-release -Stage All -Clean
./tooling/windows/Invoke-ProjectBuild.ps1 -Preset ci-windows-msvc-x64 -Stage All -Clean
```

Direct preset use is also supported after entering an x64 developer environment
and setting `QT_ROOT`, `VCPKG_ROOT` and `NINJA_PATH`:

```powershell
cmake --preset windows-msvc-x64-debug
cmake --build --preset windows-msvc-x64-debug
ctest --preset windows-msvc-x64-debug
```

Build, dependency and install outputs stay under `out/`. The entry script writes
transcripts and an installed-file SHA-256 manifest under `out/evidence/T-013/`.
The test suite includes core/adapter linking, Qt Core, Qt Quick Test, application
QML and worker process smoke tests, plus explicit configuration failures for x86
and ARM64.

`-UseExistingDependencies` is a local recovery option for a host that blocks a
vcpkg helper after a successful manifest install. It sets
`VCPKG_MANIFEST_INSTALL=OFF` for that configure only and requires an already
populated `out/vcpkg_installed`. CI never uses this option.

`-AllowWdacFallback` is separately available for managed development hosts
whose Windows application-control policy blocks newly generated executables.
It is off by default and is not used by CI. When explicitly enabled, only a
process-creation policy failure (or the host's zero/empty-output variant) may
fall back to the pinned SDK's trusted `qml`/`qmltestrunner`; compilation,
existence checks and QML tests still have to pass and the log emits a warning.

## CI runner contract

The Windows workflow uses a self-hosted runner labeled
`space-rhythm-qt6112`. The runner must expose MSVC 2022 Build Tools and the
existing T-012 Qt SDK at `C:\sr\q\qt6112`; CI bootstraps only the pinned vcpkg
tool. This preserves the rule that CI reuses Qt 6.11.2 rather than rebuilding it.
