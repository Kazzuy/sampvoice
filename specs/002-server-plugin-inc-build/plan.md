# Implementation Plan: Build Server Plugin and Pawn Include Library

**Branch**: `002-server-plugin-inc-build` | **Date**: 2026-10-27 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/002-server-plugin-inc-build/spec.md`

## Summary

Turn the already-compiling server code into two installable packages (open.mp, SA:MP 0.3.7 R2) that
carry the server plugin/component, the `voice` relay (a separate executable on SA:MP, built into the
component on open.mp, research R-10), default configs, an install note and the
unchanged `documents/sampvoice.inc` + `.hnd`. One PowerShell script (`setup/package-server.ps1`)
checks prerequisites, builds `voice`, `control` and `control/omp`, copies a fixed layout into
`dist/`, and a smoke test compiles a gamemode that calls every native. Product source changes are
limited to the embedded relay for open.mp (R-10): `voice/embed.cpp`, `common/relay.hpp`, a 4-line change
in `voice/command.hpp`, a `voice/Makefile` filter and the `control/omp` component and CMake file.

## Technical Context

**Language/Version**: C++17 (existing), PowerShell 5.1 for the script, Pawn for the include test

**Primary Dependencies**: VS 2022 Build Tools (toolset `v141_xp`), CMake (ships with Build Tools), open.mp SDK + network checkouts outside the repo (research R-02), Pawn compiler `pawncc`

**Storage**: Files only (`dist/` output, ignored by git)

**Testing**: Pawn compile of a native-coverage gamemode; start each package in a clean server folder and read the log for the ready line

**Target Platform**: Windows 32-bit packages (SA:MP 0.3.7 R2 server, open.mp server). Linux deferred (R-05)

**Project Type**: Build and packaging tooling over an existing multi-module native project

**Performance Goals**: N/A (build-time feature)

**Constraints**: Deterministic file list (SC-004); no secrets or absolute paths in packages (FR-007); no change to public Pawn API or top-level folders (FR-010)

**Scale/Scope**: 2 packages, 4 binaries, 1 script, 1 test gamemode, 1 install note

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Note |
|-----------|--------|------|
| I. Upstream-Sync Friendly | PASS | Script lives in existing `setup/`; only new files plus one `.gitignore` line (`/dist/`). No moves or renames. |
| II. Module Boundaries | PASS | No source touched. |
| III. Pawn API Stability | PASS | Include copied byte-identical; script verifies hash against repo copy (SC-005). |
| IV. Real-Time and Network Safety | PASS | Not affected. |
| V. Verify Before Done | PASS | Plan builds `sampvoice.sln` projects, `control/omp` via CMake, and runs a smoke test. |
| VI. Legacy Target Non-Regression | PASS | SA:MP 0.3.7 R2 package is a first-class target; nothing DL-specific is added. |

Post-design re-check (after Phase 1): PASS, no change.

## Project Structure

### Documentation (this feature)

```text
specs/002-server-plugin-inc-build/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   └── package-layout.md
└── tasks.md              # /speckit-tasks, not created here
```

### Source Code (repository root)

```text
setup/
└── package-server.ps1          # new: prerequisite check, build, assemble, verify
documents/
├── sampvoice.inc               # unchanged, shipped
├── sampvoice.hnd               # unchanged, shipped
└── INSTALL-SERVER.md           # new: install note copied into both packages
tests/
└── pawn/
    └── svcoverage.pwn          # new: calls every native in sampvoice.inc
voice/
├── embed.cpp                   # new: builds voice/main.cpp as a library in namespace sv_relay
├── command.hpp                 # edited: gCommandAbort flag (4 lines)
└── Makefile                    # edited: skip embed.cpp in the standalone Linux build
common/
└── relay.hpp                   # new: SvRelayStart / SvRelayStop
control/omp/                    # edited: Component.cpp starts/stops the relay, CMakeLists.txt adds embed.cpp
dist/                           # build output, git-ignored
├── sampvoice-server-omp/
└── sampvoice-server-samp/
```

**Structure Decision**: Reuse `setup/` for the script and `documents/` for the install note so no new
top-level folder besides `tests/`, which holds only the Pawn coverage gamemode. If `tests/` is not
wanted upstream, move it under `specs/002-server-plugin-inc-build/` (decide in tasks).

## Complexity Tracking

No constitution violations.
