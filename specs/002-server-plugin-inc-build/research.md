# Research: Build Server Plugin and Pawn Include Library

Evidence base: feature 001 `research.md` and `quickstart.md`, `control/omp/CMakeLists.txt`,
`control/Makefile`, existing `control/binaries` and `voice/binaries`, test rig `D:\gg\omptest`.

## R-01: One script, not a CI pipeline

- **Decision**: `setup/package-server.ps1` runs locally and does everything.
- **Rationale**: Single maintainer, Windows-only toolchain, SDK outside repo. CI would need the same
  toolset install and adds secrets/runner cost for no current need.
- **Alternatives**: GitHub Actions (deferred); MSBuild post-build targets (hard to cover CMake
  component and the include copy in one place); Inno Setup (`setup.iss` is client-only, installers
  are wrong for server folders).

## R-02: open.mp SDK stays outside the repo

- **Decision**: Script takes `-OmpDepsDir` (default env `OMP_DEPS_DIR`). It fails with a named error
  if `open.mp-sdk` or `open.mp-network` is missing.
- **Rationale**: Already decided in 001 (`CMakeLists.txt` requires `-DOMP_DEPS_DIR`). Pinned
  commits are recorded in the 001 quickstart (`08cc69a`, `f223fbd`); script prints them in the error.
- **Alternatives**: git submodule (churns upstream merges, violates Principle I); CMake FetchContent
  (needs network at build, not reproducible offline).

## R-03: Binary names and relay naming

- **Decision**: Ship `control` as `plugins/sampvoice.dll` (SA:MP package) and
  `components/SampVoiceControl.dll` (open.mp package). Ship `voice` as `sampvoice_voice.exe` in the
  SA:MP package only; the open.mp component embeds the relay (R-10), so that package has no exe.
- **Rationale**: SA:MP loads the plugin by name from `server.cfg`. open.mp blocks a legacy plugin
  named `sampvoice` (001 R-03), hence the component. The rig `D:\gg\omptest` already uses
  `sampvoice_voice.exe`, which avoids clashing with the plugin name in the same folder. `voice`'s
  default build output is `voice/binaries/sampvoice.exe`; the script renames on copy.
- **Alternatives**: keep `sampvoice.exe` (works, but easy to confuse with the plugin).
- **Update 2026-10-27**: the open.mp package no longer ships the exe (R-10).

## R-04: Config files

- **Decision**: Ship the existing `control/control.cfg` and `voice/voice.cfg` unchanged (all values
  commented, defaults apply).
- **Rationale**: Both load from the process working directory (`config.Load("control.cfg")`, `"voice.cfg"`).
  Defaults give localhost:2020 control and UDP 2020 voice, which fits a single-machine server.
- **Alternatives**: generate configs per build (adds state, no gain).

## R-05: Linux package deferred

- **Decision**: Package Windows only. Linux needs `g++ -m32` plus Linux open.mp SDK build; neither was
  verified in 001 (scenario 8 not run, no Linux host on the dev machine).
- **Rationale**: Spec assumption allows deferral. Faking an unbuilt package breaks FR-005.
- **Alternatives**: WSL build (unverified, needs 32-bit toolchain install); revisit as a follow-up
  feature once a Linux host exists.

## R-06: SA:MP 0.3.7 R2 plugin build

- **Decision**: Build `control/control.vcxproj` and `voice/voice.vcxproj` with MSBuild Release|Win32.
  The existing `control/binaries/sampvoice.dll` and `voice/binaries/sampvoice.exe` show this builds.
- **Rationale**: Toolchain recorded in constitution and 001 quickstart. Not yet smoke-tested on a
  clean SA:MP R2 folder with this commit; the rig `D:\gg\svtest` exists for that.
- **Alternatives**: none; the solution is the primary toolchain.

## R-07: Pawn include test

- **Decision**: `tests/pawn/svcoverage.pwn` calls every native and uses every `SV_FILTER` value
  from `documents/sampvoice.inc`. Compile with `pawncc -i documents`. Script parses `native` lines and `SV_FILTER`
  members from the include and fails if one is missing from the test.
- **Rationale**: 001 gotcha: an old `sampvoice.inc` in the server `pawno/include` shadows the repo
  one, so `-i documents` must come first. Parsing the include keeps coverage honest as the API grows.
- **Alternatives**: compile an empty script (proves syntax only, not coverage).

## R-08: Reproducibility

- **Decision**: Script wipes the `dist/<package>` folder before assembling, emits sorted
  `MANIFEST.txt` with relative paths and SHA-256 per file, and excludes `.pdb`/`.ipdb`/`.iobj`/`.exp`/`.lib`.
- **Rationale**: Satisfies SC-004 (identical file lists), FR-007 and FR-008. Hashes of binaries may
  differ between builds (timestamps); only the file list is required to be identical.
- **Alternatives**: byte-for-byte reproducible builds (needs `/Brepro` and pinned paths; overkill now).

## R-09: Secrets and path leakage

- **Decision**: `Test-Package` scans text files and the bytes of `.dll`/`.exe` (ASCII and UTF-16) for the repo
  path, the open.mp deps path and the user profile path; any hit fails the package. `Build-ControlOmp` builds
  with `/experimental:deterministic` and `/pathmap` so `__FILE__` strings carry neutral prefixes.
- **Rationale**: FR-007. Found during convergence: the SDK's `span-lite` embedded the deps path in the component.
  cl ignores `/pathmap` without `/experimental:deterministic` (warning D9007).
- **Alternatives**: rebuild with `/PDBALTPATH` (touches vcxproj, conflicts with Principle I); `subst` drive
  (invasive). `voice` and `control` (vcxproj) binaries pass the scan without changes.

## R-10: Voice relay embedded in the open.mp component

- **Decision**: `voice/embed.cpp` compiles `voice/main.cpp` unchanged inside `namespace sv_relay` (with
  `#define main sv_relay_main`). `control/omp/CMakeLists.txt` adds it to the component; `Component.cpp` calls
  `SvRelayStart()` after the control core loads and `SvRelayStop()` after `Unload()`. The relay still talks to
  the control service over loopback TCP, so the wire protocol is unchanged.
- **Rationale**: server owners should not have to start a second program. The namespace is needed because
  `control` defines its own `Logger` (`control/libraries/util/logger.hpp`) and the two global structs would merge
  in one dll (one-definition rule). `common/relay.hpp` is the only header shared with `control/omp`, so
  Constitution II holds. `voice/command.hpp` gets `gCommandAbort` (4 lines, Constitution I):
  `CommandService::Initialize` retried `Connect` forever, so without it the relay never exited and server
  shutdown hung (seen as a 40 s hang before the fix). Standalone `voice` never sets the flag.
  `voice/Makefile` skips `embed.cpp`.
- **Alternatives**: component spawns `sampvoice_voice.exe` (keeps crash isolation, still ships two binaries);
  keep the exe as before (no code change, what the SA:MP package still does).
- **Risk**: a relay crash now takes the game server down. Do not run a separate exe against the same server:
  the first relay that connects to control wins.

## Unresolved

None. Open item for tasks: confirm `tests/` top-level folder is acceptable (see plan Structure Decision).

## R-07 note (T003, 2026-10-27)

No `pawncc.exe` in the test rigs. Found: `D:\depeloper\Kazzuy\kazzuyOMP\qawno\pawncc.exe` (open.mp compiler, used for the coverage test) and `pawncc.exe` in `FloresRoleplay\pawno` and `ImperiunRoleplay\pawno`.
