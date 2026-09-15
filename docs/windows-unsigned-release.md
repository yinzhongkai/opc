# Windows unsigned release engineering

`tooling/windows/Invoke-UnsignedRelease.ps1` implements the unsigned portion of
T-037. It consumes only the configured `windows-msvc-x64-release` build and the
pinned Qt/vcpkg roots. It does not select a production installer, access signing
credentials, change WDAC/SAC, or approve a release candidate.

## Build the engineering bundle

Run from a normal PowerShell 7 session:

```powershell
./tooling/windows/Invoke-UnsignedRelease.ps1 -Version 0.1.0-dev
```

The default requires a clean Git worktree. `-AllowDirtySource` exists only for a
non-candidate engineering run; the dirty status is embedded in
`manifest/build-inputs.json`, and `candidateEligible` remains `false`.

The script performs these fail-closed checks:

1. validates the fixed Qt 6.11.2 SDK, `windeployqt.exe` hash, MSVC x64 Release
   cache, pinned vcpkg checkout and installed package ABI records;
2. refreshes the Release build and copies only the App and Worker from that
   build tree;
3. parses a `windeployqt --dry-run --list mapping` result, accepts only the
   pinned Qt root and specifically named, validly Microsoft-signed Windows SDK
   D3D x64 redistributables, then performs the real deployment;
4. excludes `qmltooling`, `generic`, translations, Debug runtime, build/test
   tools and `vc_redist.x64.exe` while the VC Runtime strategy is unconfirmed;
5. recursively resolves each PE import only from the package, the pinned vcpkg
   install, or Windows system dependencies and verifies every packaged PE is
   x64;
6. writes runtime/payload/bundle SHA-256 manifests, source/build inputs,
   Authenticode state, upstream SPDX material, a generated SPDX 2.3 document,
   third-party notices, Qt replacement guidance and a credential-free signing
   request manifest;
7. creates a deterministic ZIP whose entry timestamps are fixed to the source
   commit time.

Outputs are under `out/release/T-037/unsigned/`; raw command, mapping, PE and
summary evidence is under `out/evidence/T-037/`. Both locations are ignored by
Git.

## Validate the transaction boundary

The bundle includes `tools/Invoke-UnsignedInstallTransaction.ps1`. It is a
backend-neutral engineering harness, not the selected product installer. Every
operation requires explicit, non-overlapping install and state roots:

```powershell
$bundle = './out/release/T-037/unsigned/space-rhythm-0.1.0-dev-unsigned'
$install = './out/manual-install/SpaceRhythm'
$state = './out/manual-install-state'

& "$bundle/tools/Invoke-UnsignedInstallTransaction.ps1" `
  -Action Install -BundleRoot $bundle -InstallRoot $install -StateRoot $state
& "$bundle/tools/Invoke-UnsignedInstallTransaction.ps1" `
  -Action Rollback -BundleRoot $bundle -InstallRoot $install -StateRoot $state
& "$bundle/tools/Invoke-UnsignedInstallTransaction.ps1" `
  -Action Uninstall -BundleRoot $bundle -InstallRoot $install -StateRoot $state
```

Install and repair verify the source manifest, stage on the target volume,
verify the staged copy, move an existing registered payload to a one-version
backup, and switch directories. Rollback swaps the registered backup. Uninstall
first verifies that the explicit root is a registered payload and deletes only
that payload and its registered backup. User projects, original media, settings,
autosaves, caches and logs outside the two explicit roots are never touched.

Run the repository smoke and transaction test with:

```powershell
./tests/release/Test-UnsignedPackage.ps1 `
  -BundleRoot ./out/release/T-037/unsigned/space-rhythm-0.1.0-dev-unsigned `
  -RunSmoke
```

The smoke option must execute the installed App and Worker and observe their Qt
6.11.2/x64 markers. A WDAC/SAC process or DLL rejection is a test failure; there
is no fallback or retry-to-pass path. Before it invokes the transaction tool,
the test independently verifies every entry in `manifest/bundle-files.sha256.csv`;
the separately reported ZIP hash remains the authenticity handoff for this
unsigned engineering output.

## Remaining gates

This unsigned output is always named and marked `unsigned-engineering` and is
never candidate-eligible. T-037 cannot finish until the authorized decision
owners select the installer/scope/upgrade policy and VC Runtime mode and provide
the signing inputs. H-015 still requires an organization-managed WDAC/SAC trust
route. Minimum Windows, product codec/content decisions, T-022 and clean-machine
T-038 also remain outside this engineering bundle.
