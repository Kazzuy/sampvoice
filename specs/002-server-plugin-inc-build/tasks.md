---

description: "Task list for building server packages and the Pawn include library"
---

# Tasks: Build Server Plugin and Pawn Include Library

**Input**: Design documents from `/specs/002-server-plugin-inc-build/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/package-layout.md, quickstart.md

**Tests**: No separate test-suite tasks. The Pawn coverage gamemode (US2) and rig runs are the verification the spec requires (FR-004, FR-005).

**Organization**: Grouped by user story. `setup/package-server.ps1` is a single file, so script tasks are sequential (no `[P]`).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1, US2, US3 from spec.md
- Paths are relative to repo root `D:\depeloper\Kazzuy\vibe\sampvoice`

## Phase 1: Setup

- [X] T001 Add the line `/dist/` to `.gitignore` (next to `/binaries/`) so package output is never committed
- [X] T002 Create folder `tests/pawn/` (placeholder `.gitkeep` removed once T016 lands); keep it unless the user asks to move it under `specs/002-server-plugin-inc-build/` (plan.md Structure Decision)
- [X] T003 Locate a `pawncc.exe` on this machine (check `D:\gg\omptest`, `D:\gg\svtest` and their `pawno` folders) and note its path in `specs/002-server-plugin-inc-build/research.md` under R-07

---

## Phase 2: Foundational (blocks all stories)

**Purpose**: Script skeleton with parameters, prerequisite checks and build steps. Every story reuses it.

- [X] T004 Create `setup/package-server.ps1` with `[CmdletBinding()]` params `-OmpDepsDir` (default `$env:OMP_DEPS_DIR`), `-PawncPath`, `-SkipPawn` (switch), `-Platform` (`omp|samp|all`, default `all`); `$ErrorActionPreference = 'Stop'`; repo root resolved from `$PSScriptRoot\..`
- [X] T005 In `setup/package-server.ps1` add `Test-Prerequisites`: find `MSBuild.exe` via `vswhere` and confirm toolset `v141_xp` exists, find `cmake.exe` (PATH, then VS Build Tools `Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`), confirm `$OmpDepsDir\open.mp-sdk` and `$OmpDepsDir\open.mp-network` exist (only when platform is `omp` or `all`), confirm `-PawncPath` exists unless `-SkipPawn`. Each failure throws a message naming the missing item; the `open.mp-sdk` message prints pinned commits `08cc69a` and `f223fbd` (FR-006, research R-02)
- [X] T006 In `setup/package-server.ps1` add `Build-Voice` and `Build-ControlSamp`: run `MSBuild.exe voice\voice.vcxproj` and `control\control.vcxproj` with `/p:Configuration=Release /p:Platform=Win32`, throw on non-zero exit code, then confirm `voice\binaries\sampvoice.exe` and `control\binaries\sampvoice.dll` exist and are newer than the start of the build
- [X] T007 In `setup/package-server.ps1` add `Build-ControlOmp`: `cmake -S control/omp -B <scratch>\build_omp -G "Visual Studio 17 2022" -A Win32 -DOMP_DEPS_DIR=$OmpDepsDir`, then `cmake --build <scratch>\build_omp --config Release`; locate `SampVoiceControl.dll` in the build tree and throw if absent. Build tree goes outside the repo (`$env:TEMP`)

**Checkpoint**: `./setup/package-server.ps1 -SkipPawn` builds all three binaries and exits 0 (packaging not yet added).

---

## Phase 3: User Story 1 - Ready-to-install server package (Priority: P1) 🎯 MVP

**Goal**: Two `dist/` packages matching `contracts/package-layout.md` that start on clean servers.

**Independent Test**: Quickstart scenarios 2, 5, 6: copy a package into a clean server folder, start, log shows SampVoice ready.

- [X] T008 [P] [US1] Write `documents/INSTALL-SERVER.md` covering exactly the points listed under "Rules" in `specs/002-server-plugin-inc-build/contracts/package-layout.md`: which package fits which server, file destinations, `plugins sampvoice` line for SA:MP only, `sampvoice_voice.exe` runs beside the server with `voice.cfg`, default port 2020 (control TCP, voice UDP), open.mp blocks a legacy plugin named `sampvoice`, compile with `-i include` first
- [X] T009 [US1] In `setup/package-server.ps1` add `New-Package -Name -Layout`: delete `dist\<name>` if present, create it, copy files per a layout hashtable (source → relative target), fail if a source is missing (edge case: stale output)
- [X] T010 [US1] In `setup/package-server.ps1` define the two layouts from `contracts/package-layout.md`: omp (`components\SampVoiceControl.dll`, `sampvoice_voice.exe`, `control.cfg`, `voice.cfg`, `include\sampvoice.inc`, `include\sampvoice.hnd`, `INSTALL-SERVER.md`) and samp (`plugins\sampvoice.dll` instead of the component). Sources: `control/control.cfg`, `voice/voice.cfg`, `documents/*`
- [X] T011 [US1] In `setup/package-server.ps1` add `Write-Manifest`: write `MANIFEST.txt` with lines `<relative path with / separators>  <SHA-256>`, sorted ordinally, excluding `MANIFEST.txt` itself (research R-08)
- [X] T012 [US1] In `setup/package-server.ps1` add `Test-Package`: fail if any file has extension `.pdb .ipdb .iobj .exp .lib .obj .tlog .user`; fail if any file is not in the layout; fail if any text file (`.md .cfg .txt .inc .hnd`) contains the repo absolute path or `$env:USERPROFILE` (FR-007, research R-09). Set package state `verified` only when all pass
- [X] T013 [US1] In `setup/package-server.ps1` wire the main flow: prerequisites → builds → `New-Package` + `Write-Manifest` + `Test-Package` for each selected platform → print a one-line result per package and exit non-zero if any failed. Run `./setup/package-server.ps1 -SkipPawn` and confirm both `dist/` folders exist
- [X] T014 [US1] Rig test open.mp (quickstart scenario 5): copy `dist/sampvoice-server-omp/` content into a copy of `D:\gg\omptest` (keep its `config.json`) after deleting its old `components\SampVoiceControl.dll` and `sampvoice_voice.exe`; compare SHA-256 against `MANIFEST.txt`, start `omp-server.exe` and `sampvoice_voice.exe`; record log lines "Successfully loaded component SampVoice Control" and `[sv:omp] SampVoice control component ready` in `specs/002-server-plugin-inc-build/quickstart.md` under a new "Results" section
- [X] T015 [US1] Rig test SA:MP R2 (quickstart scenario 6): copy `dist/sampvoice-server-samp/` content into a copy of `D:\gg\svtest` after deleting its old `plugins\sampvoice.dll` and `sampvoice_voice.exe`; compare SHA-256 against `MANIFEST.txt`, add `plugins sampvoice` to its `server.cfg`, start the server and the relay; record the banner line `SampVoice by MOR loaded` in the same "Results" section. If the plugin does not load, record the exact error and stop (do not claim FR-005 met)

**Checkpoint**: US1 independently valid; MVP.

---

## Phase 4: User Story 2 - Pawn include library (Priority: P1)

**Goal**: The shipped include is unchanged and compiles in a script that uses every native and every `SV_FILTER` value.

**Independent Test**: Quickstart scenarios 3, 4, 7.

- [X] T016 [P] [US2] Create `tests/pawn/svcoverage.pwn`: `#include <sampvoice>`; call every `native` declared in `documents/sampvoice.inc` (37 at 4.0.0; read the file to enumerate them) and pass each `SV_FILTER` enum member to `SvAppendFilter` and `SvRemoveFilter`. Keep each call minimal and guarded so the gamemode starts without a client
- [X] T017 [US2] In `setup/package-server.ps1` add `Test-IncludeIdentity`: SHA-256 of `include\sampvoice.inc` and `include\sampvoice.hnd` in each package must equal `documents\` copies (FR-003, SC-005); call it from `Test-Package` (T012)
- [X] T018 [US2] In `setup/package-server.ps1` add `Test-IncludeCoverage`: parse `native` names and `SV_FILTER` members from `documents\sampvoice.inc`, fail naming any missing from `tests\pawn\svcoverage.pwn`; then run `pawncc tests\pawn\svcoverage.pwn -i<package>\include` and fail on any error or warning mentioning `sampvoice.inc` (FR-004, SC-002). With `-SkipPawn` print `NOT VERIFIED: FR-004, SC-002` in the final summary and still exit 0; never print it as passed
- [X] T019 [US2] Run `./setup/package-server.ps1 -PawncPath <path from T003>` and confirm coverage and identity checks pass. Record the result in `specs/002-server-plugin-inc-build/quickstart.md` "Results"
- [X] T020 [US2] Rig test natives (quickstart scenario 7): copy the compiled `svcoverage.amx` to `D:\gg\omptest\gamemodes`, run the open.mp server, record which natives ran without error; list any native not exercised (001 verified 8 only)

**Checkpoint**: US1 and US2 both valid.

---

## Phase 5: User Story 3 - Repeatable documented procedure (Priority: P2)

**Goal**: One written procedure; failures name the missing item; rebuilds give the same file list.

**Independent Test**: Quickstart scenarios 1, 2, 8.

- [X] T021 [US3] Run `./setup/package-server.ps1 -OmpDepsDir C:\does\not\exist -SkipPawn`; confirm exit code non-zero, message names `open.mp-sdk`, and no `dist/sampvoice-server-omp` is left behind. If a partial folder remains, fix `setup/package-server.ps1` so prerequisites run before any `dist` change (scenario 1)
- [X] T022 [US3] Run the script twice; `diff` the two `MANIFEST.txt` path columns (cut hashes); confirm identical (SC-004). Place a stray file in `dist/sampvoice-server-omp/` before the second run and confirm it disappears (scenario 8)
- [X] T023 [P] [US3] Add a "Build procedure" section to `specs/002-server-plugin-inc-build/quickstart.md` (or update its "Build" section) with: prerequisite list, the exact command, expected `dist/` tree, and where `OMP_DEPS_DIR` points on this machine (`D:\depeloper\Kazzuy\vibe\omp-deps`)

**Checkpoint**: All stories valid.

---

## Phase 6: Polish

- [X] T024 Re-read `documents/sampvoice.inc` and `documents/sampvoice.hnd` with `git diff --stat -- documents/sampvoice.inc documents/sampvoice.hnd`; confirm no diff (constitution III, FR-010)
- [X] T025 Run `git status --short` and confirm only these paths changed or were added: `.gitignore`, `setup/package-server.ps1`, `documents/INSTALL-SERVER.md`, `tests/pawn/svcoverage.pwn`, `specs/002-server-plugin-inc-build/**` (constitution I)
- [X] T026 Update the 001 memory note `sampvoice-dl-openmp-port.md` only if new gotchas appeared during T014/T015/T020 (name them); otherwise skip
- [X] T027 Commit in separate Conventional Commits: `feat(setup): add server package script`, `docs: add server install note and pawn coverage test`, `docs: add 002 spec, plan, tasks`

---

## Dependencies & Execution Order

- Phase 1 → Phase 2 → stories. US1 first (MVP). US2 needs the package from US1 (T013) for T017–T019 but T016 can start any time.
- US3 needs T013 (script complete) and T017–T018 for a full run.
- Polish after all stories.

### Within stories

- Script tasks T004–T007, T009–T013, T017–T018 edit one file: run in order.
- Rig tests T014, T015, T020 need the built package; T014 and T015 can run on different rigs in parallel.

### Parallel opportunities

- T008 (install note) and T016 (coverage gamemode) touch separate files: run alongside script tasks.
- T023 (docs) runs alongside T021–T022.
- T014 and T015 on `D:\gg\omptest` and `D:\gg\svtest` at the same time (ports differ; note two game clients on one PC are not needed here).

## Implementation Strategy

### MVP first

1. Phase 1 and 2: script builds three binaries.
2. Phase 3 (US1): packages exist and start on both rigs.
3. Stop and validate with quickstart scenarios 2, 5, 6.

### Incremental

1. Add US2: include identity and compile checks.
2. Add US3: failure paths, determinism, written procedure.
3. Polish and commit.

## Notes

- Do not edit `control/`, `voice/`, `client/`, `common/` sources or any `.vcxproj` in this feature.
- Do not claim FR-005 for a platform until its rig log line is recorded (T014, T015).
- Linux package, two-client audio and R1/R3 regression remain out of scope.

---

## Phase 7: Convergence

- [X] T028 Remove build-machine paths from binaries and make the check catch them in `setup/package-server.ps1`: `dist/sampvoice-server-omp/components/SampVoiceControl.dll` embeds `D:\depeloper\Kazzuy\vibe\omp-deps\open.mp-sdk\lib\span-lite\include\nonstd/span`. Pass `/pathmap:<OmpDepsDir>=/omp-deps` and `/pathmap:<repo root>=/sampvoice` through `-DCMAKE_CXX_FLAGS` in `Build-ControlOmp` (do not edit `control/omp/CMakeLists.txt`), extend `Test-Package` to scan `.dll`/`.exe` bytes (ASCII and UTF-16) for the repo root, `$OmpDepsDir` and `$env:USERPROFILE` and fail naming the file, then rebuild and confirm both packages PASS and the string is gone per FR-007 (partial)
- [X] T029 Measure SC-001: time copy plus start plus ready line on the `D:\gg\omptest_002` and `D:\gg\svtest_002` rigs using only `INSTALL-SERVER.md`, and record seconds in `specs/002-server-plugin-inc-build/quickstart.md` "Results" per SC-001 (partial)
- [X] T030 Exercise the remaining FR-006 failures and record them in `specs/002-server-plugin-inc-build/quickstart.md` "Results": run `setup/package-server.ps1` with a nonexistent `-PawncPath` and with no `-PawncPath` and no `-SkipPawn`; expect a message naming `pawncc.exe`, exit non-zero, no `dist/` change per FR-006 (partial)

---

## Phase 8: Convergence

> Note (T032): T010 and T014 above describe the layout before the relay was embedded. The current layout is `contracts/package-layout.md`: the open.mp package has no `sampvoice_voice.exe`.

- [X] T031 CRITICAL: exclude `voice/embed.cpp` from the Linux build in `voice/Makefile` (its `g++ ... *.cpp` line compiles every `.cpp` in `voice/`, so `embed.cpp` is built into the standalone relay too; use `$(filter-out embed.cpp,$(wildcard *.cpp))`), then build `voice` with that `Makefile` on a Linux or WSL host (`g++ -m32`, C++17 for the new `inline` variable `gCommandAbort` in `voice/command.hpp`) and record the result in `specs/002-server-plugin-inc-build/quickstart.md` "Results"; if no host is available, record "Makefile build unverified" there per Constitution V (partial)
- [X] T032 Update the artifacts that still say the open.mp package ships `sampvoice_voice.exe` so they match `setup/package-server.ps1` and `contracts/package-layout.md` (the relay is built into `components/SampVoiceControl.dll`): `specs/002-server-plugin-inc-build/research.md` R-03, `plan.md` Summary and Source Code tree, `quickstart.md` scenario 5 and its Results entry, `tasks.md` T010/T014 wording only through a note under Phase 8, and add the embedded-relay requirement to `spec.md` FR-001 per FR-001 (contradicts)
- [X] T033 Record the embedded-relay decision in `specs/002-server-plugin-inc-build/research.md` as R-10 (alternatives: auto-spawn the exe from the component, separate exe as before): why `voice/embed.cpp` wraps `voice/main.cpp` in namespace `sv_relay` (control defines its own `Logger`; two global `Logger` structs in one dll break the one-definition rule), the 4-line `gCommandAbort` change in `voice/command.hpp` (Constitution I), that `common/relay.hpp` is the only header shared with `control/omp` (Constitution II), and the risk that a relay crash takes the game server down, per plan: embedded relay (unrequested)
- [X] T034 Add quickstart scenario 9 and a Results entry for the embedded relay in `specs/002-server-plugin-inc-build/quickstart.md`: start the open.mp rig with no `sampvoice_voice.exe` and confirm UDP 2020 is owned by `omp-server.exe` and `voice.log` shows `voice service opened`; stop it with Ctrl+C and confirm exit code 0 within 5 s with `failed to initialize command service` as the last `voice.log` line; note that a second relay against the same server is not supported per FR-005 (partial)
