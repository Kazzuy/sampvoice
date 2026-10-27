---

description: "Task list for porting SampVoice to SA:MP 0.3.DL client and open.mp server"
---

# Tasks: Port SampVoice to SA:MP 0.3.DL Client and open.mp Server

**Input**: Design documents from `/specs/001-port-03dl-openmp/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/host-adapter.md, quickstart.md

**Tests**: No automated test tasks (repo has no test suite; spec asks for none). Verification tasks follow `quickstart.md` and are manual.

**Organization**: Grouped by user story. US1 = DL client, US2 = open.mp server, US3 = no regression.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1, US2, US3
- Paths are relative to repository root

---

## Phase 1: Setup

**Purpose**: Find build facts and a working test rig.

- [X] T001 Find where `SAMP_R1` and `SAMP_R3` are defined (not in `client/client.vcxproj` PreprocessorDefinitions); record the answer in `specs/001-port-03dl-openmp/research.md` under R-01
- [X] T002 [P] Confirm `sampvoice.sln` builds `client`, `voice`, `control` unchanged (Release); record any toolset or SDK blockers in `specs/001-port-03dl-openmp/quickstart.md`
- [ ] T003 [P] Prepare test rig: 0.3.DL client, 0.3.7 R1/R3 client, open.mp server, SA:MP 0.3.7 R2 server, and a test gamemode that calls every native in `documents/sampvoice.inc`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Decision gate R-03. Server work in US2 depends on the result.

**CRITICAL**: Do T004 and T005 before any US2 task. US1 can start in parallel.

- [X] T004 Spike: load unmodified `control` plugin in open.mp as legacy plugin (`legacy_plugins` in `config.json`); log whether `ppPluginData` and `ppPluginData[PLUGIN_DATA_NETGAME]` are non-null in `AmxLoad` (`control/main.cpp`); keep the logging patch local, not committed
- [X] T005 (result: FAIL, see research.md R-03) Record spike result in `specs/001-port-03dl-openmp/research.md` R-03 as PASS (NetGame valid) or FAIL; on FAIL stop US2 and ask the user to amend FR-008 in `specs/001-port-03dl-openmp/spec.md`
- [ ] T006 [P] Spike: load unmodified `voice` plugin in open.mp; confirm startup log and that `voice.cfg` is read; record in `specs/001-port-03dl-openmp/research.md`

**Checkpoint**: Gate decided. US1 may already be running.

---

## Phase 3: User Story 1 - Player uses voice chat on a 0.3.DL client (Priority: P1) MVP

**Goal**: Client plugin loads and works on 0.3.DL.

**Independent Test**: quickstart scenarios 2 and 3 with any server that already works (SA:MP 0.3.7 R2 or open.mp).

- [X] T007 [US1] Extract the 12 per-version offsets needed by `client/libraries/util/addresses.hpp` from the 0.3.DL `samp.dll` (e.g. `RakClientInitialization`, `BassInitialization`, `BassConfiguration`); write the table to `specs/001-port-03dl-openmp/research.md` R-01
- [X] T008 [US1] Add a `SAMP_DL` branch beside each `SAMP_R1`/`SAMP_R3` branch in `client/libraries/util/addresses.hpp` using the T007 table
- [X] T009 [US1] Diff struct layouts of 0.3.DL against R1 for the types the plugin touches (`client/libraries/samp/CNetGame.h`, `CPlayerPool.h`, `CRemotePlayer.h`, `CLocalPlayer.h`, `CChat.h`, `CInput.h`, `CDialog.h`, `CScoreboard.h`); list which files need a DL variant in `specs/001-port-03dl-openmp/research.md`
- [X] T010 [US1] Add `SAMP_DL` branches in the `client/libraries/samp/*.h` and `*.cpp` files listed by T009 only; leave other files untouched (constitution I)
- [X] T011 [US1] Accept `SAMP_DL` in the default-version guard in `client/libraries/samp/commonSA.h` and document it in `README.md`. R1/R3 are chosen by passing the macro, not by a vcxproj configuration (T001), so DL follows the same way; no `.sln` or vcxproj change
- [X] T011a [US1] Design runtime build check for FR-006: read the loaded `samp.dll` version at startup in `client/main.cpp` and compare with the build macro (`SAMP_R1`/`SAMP_R3`/`SAMP_DL`); record the chosen check in `specs/001-port-03dl-openmp/research.md` R-01
- [X] T012 [US1] Implement unsupported-build handling (FR-006) using the T011a check: on unknown or mismatched game build, show a clear message and disable the plugin without hooking, in `client/main.cpp`
- [X] T013 [US1] Build `client` for DL and fix compile errors in `client/`
- [ ] T014 [US1] Manual verify quickstart scenario 2 (two-way audio, mic icon, menu) and scenario 3 (unsupported build) on 0.3.DL; record results in `specs/001-port-03dl-openmp/quickstart.md`

**Checkpoint**: 0.3.DL players have voice. MVP done.

---

## Phase 4: User Story 2 - Server owner runs voice on open.mp (Priority: P1)

**Goal**: A native open.mp component provides `control` (FR-008); `voice` stays a separate process.

**Independent Test**: quickstart scenarios 1 and 4 on the clean server in `D:\gg\omptest` with the 0.3.DL client.

**Gate**: T005 = FAIL, so the legacy-plugin route is closed; this phase is the native-component route (R-08).

- [X] T015 [US2] Clone `openmultiplayer/open.mp-sdk` and `open.mp-network` (MIT) outside the repo; record commit hashes and the cmake command in `specs/001-port-03dl-openmp/quickstart.md`
- [X] T016 [US2] Create `control/omp/CMakeLists.txt` and `control/omp/Component.cpp`: a minimal `IComponent` (own UID, name `SampVoiceControl`) that loads in `omp-server.exe` and logs one line; build x86 Release
- [X] T017 [US2] Verify T016 in `D:\gg\omptest\components`: log shows the component loaded
- [X] T018 [US2] Create `control/omp/net_omp.hpp`: network layer with the same callbacks as `control/libraries/util/raknet.hpp` (connect, packet, disconnect, send) built on `NetworkInEventHandler`/`NetworkOutEventHandler`; read the `0xDEADC0DE` handshake from ClientJoin (RPC 25) and range-check every length (FR-009)
- [X] T019 [US2] Register `SvGetVersion` and `SvHasMicro` natives through `IPawnComponent` in `control/omp/pawn_omp.hpp`; verify with the `svtest` gamemode and the DL client that `[svtest] voice version: 0x04000000` is printed
- [X] T020 [US2] Move the host-specific `pNetGame` reads in `control/main.cpp` behind `control/host.hpp` (contract `contracts/host-adapter.md`); SA:MP behavior stays identical; implement the open.mp variant with `IPlayerPool`, vehicles and objects components
- [ ] T021 [US2] Port the command service link to `voice` and the remaining natives so quickstart scenario 4 passes (all natives in `documents/sampvoice.inc`)
- [X] T021a [US2] Input validation review on the open.mp path (FR-009, constitution IV)
- [ ] T021b [US2] Manual verify on Linux open.mp (`cmake` build) and gamemode reload (quickstart scenarios 7 and 8)

**Checkpoint**: open.mp servers have voice.

---

## Phase 5: User Story 3 - Existing SA:MP users are not broken (Priority: P2)

**Goal**: No regression on SA:MP 0.3.7.

**Independent Test**: quickstart scenario 5.

- [ ] T022 [US3] Build `client` for R1 and R3 after US1 changes; confirm no diff in behavior from the pre-port build
- [ ] T023 [US3] Manual verify quickstart scenario 5 on R1/R3 clients with a SA:MP 0.3.7 R2 server; record results in `specs/001-port-03dl-openmp/quickstart.md`
- [X] T024 [P] [US3] Confirm `documents/sampvoice.inc` and `documents/sampvoice.hnd` have no diff (FR-004, constitution III)

---

## Phase 6: Polish & Cross-Cutting

- [X] T025 [P] Update install docs for 0.3.DL players and open.mp servers (installer and manual) in `README.md`; note the legacy plugin setup and the `legacy_plugins` key
- [X] T026 [P] No change to `setup/setup.iss`: the installer packs the one `sampvoice.asi` that was built, so each client version gets its own installer; the README states which macro builds which version
- [ ] T027 Run quickstart scenario 6 soak test (30 min, 10+ players, DL client plus open.mp); record result in `specs/001-port-03dl-openmp/quickstart.md`
- [X] T028 Review diff against constitution I-V; confirm no folder or `.sln` path renames and no secrets or agent folders (`.claude`, `.cursor`, `.specify`) in upstream-bound commits

---

## Dependencies & Execution Order

- Phase 1 first. T001 feeds T011.
- Phase 2: T004 then T005. T006 is independent. T005 gates all of Phase 4.
- Phase 3 (US1) needs only Phase 1; it can run in parallel with Phase 2 and Phase 4.
- Phase 4 (US2) is the native-component route (T005 = FAIL closed the legacy route); it needs no other phase.
- Phase 5 (US3): T022 needs only Phase 3. T023 needs Phase 4 done (it checks the refactored server code on SA:MP R2). If gate T005 = FAIL, run T022 only and re-plan T023.
- Phase 6 needs Phases 3-5.

Within US1: T007 -> T008, T009 -> T010 -> T011 -> T011a -> T012 -> T013 -> T014.
Within US2: T015 -> T016 -> T017 -> T018 -> T019 -> T020 -> T021 -> T021a, T021b.

## Parallel Opportunities

- T002 and T003 together.
- T006 with T004.
- US1 (T007-T014) with the Phase 2 spike.
- T018 with T015-T017.
- T024, T025, T026 together.

## Implementation Strategy

- **MVP**: Phase 1 + US1 (0.3.DL client). It needs no server change.
- Run the R-03 spike early. If it fails, FR-008 changes and US2 gets re-planned (`/speckit-clarify`, then `/speckit-plan`).
- Deliver US1, then US2, then US3 checks, then polish.

## Notes

- T007 and T009 need reverse engineering of 0.3.DL `samp.dll`; no public offset table was confirmed. Budget the most time here.
- Commit after each task or logical group. Use Conventional Commits.
