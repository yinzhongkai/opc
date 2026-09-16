# Qt 6.11.2 Windows SDK build

`Invoke-Qt6112Build.ps1` is the reproducible T-012 entry point. It enforces the confirmed baseline:

- Visual Studio 2022 Build Tools with MSVC x64, Windows SDK 10.0.26100, CMake, and Ninja;
- the MSVC English resources with `VSLANG=1033`, so Ninja dependency parsing and logs are locale-stable;
- the official Qt 6.11.2 source archive with the locked byte size and SHA-256;
- shared LGPLv3 Qt libraries, Release and Debug configurations, and no developer build;
- the `qtbase`, `qtdeclarative`, `qtshadertools`, and `qtmultimedia` module whitelist;
- explicit exclusion of their optional repository dependencies (`qtimageformats`, `qtlanguageserver`, `qtsvg`, `qtquick3d`, and `qtquicktimeline`);
- Qt Multimedia's native Windows backend with its FFmpeg feature disabled for this SDK baseline.

The default short paths are under `C:\sr`. Source, shadow build, installed SDK, smoke deployment, and evidence remain separate. Run from PowerShell 7 in a normal shell; the script imports the x64 developer environment itself.

The entry point also adds the shadow-build `qtbase\bin` directory and the matching MSVC x64 `debug_nonredist` CRT directory to child-process `PATH`. This is required for Debug host tools such as `rcc.exe`, whose executable and Debug Qt DLL live in adjacent output directories.

The native compiler probe runs during `Configure`/`All`; later stages reuse that evidence instead of repeatedly replacing and launching a new probe executable. The Qt consumer smoke test is a separate CMake project. It builds with `/MD` and `/MDd`, asks `windeployqt` to include `qoffscreen`, and starts both staged configurations with `QT_QPA_PLATFORM=offscreen`. Its console checkpoints report the Qt version, x64 ABI, QML load, Multimedia object construction, and event-loop completion.

```powershell
Set-Location projects/space-rhythm/workspace
pwsh -File .\tooling\qt\Invoke-Qt6112Build.ps1 -Stage All -Parallel 10
```

To repeat configuration in a known-clean build directory, pass `-CleanBuild`. The script only permits recursive cleanup beneath the fixed `C:\sr\b`/`C:\sr` working roots and validates the exact source archive before every stage.

Evidence is written to `C:\sr\evidence\T-012`. It includes the full configure arguments and log, compiler/SDK/tool versions, native and Qt smoke results, PE architecture/dependency inspection, deployment logs, and SHA-256 manifests for SDK binaries, symbols, and staged runtime files.

The Debug staged directory is development evidence only. `windeployqt --debug --compiler-runtime` copies MSVC Debug CRT files, which are not a redistributable product runtime, and the Debug UCRT still comes from the initialized Windows SDK environment. Only a later release-packaging task may decide the distributable runtime layout; T-012 does not create an installer or release package.
