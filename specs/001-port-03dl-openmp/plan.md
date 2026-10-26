# Implementation Plan: Port SampVoice to SA:MP 0.3.DL Client and open.mp Server

**Branch**: `001-port-03dl-openmp` | **Date**: 2026-10-26 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/001-port-03dl-openmp/spec.md`

## Summary

Add a 0.3.DL build target to the `client` plugin and make the `voice` and `control` server plugins
load and work under open.mp as legacy plugins (spec FR-008). Client work is per-version address and
struct tables (compile-time split already used for R1/R3). Server work is mostly in `control`, which
depends on SA:MP/YSF internals (`pNetGame`) that open.mp may not expose. Research found this is the
main risk: see Risks and the decision gate in research.md R-03.

## Technical Context

**Language/Version**: C++17 (`voice` Makefile uses `-std=c++17`; `client`/`control` via existing vcxproj)

**Primary Dependencies**: Existing: BASS, ImGui, RakNet, Pawn AMX SDK, YSF headers (`control/libraries/ysf`)

**Storage**: N/A (config files only: `voice.cfg`, `control.cfg`)

**Testing**: Manual integration on real game client + server (no automated test suite in repo)

**Target Platform**: Client Windows x86; server Windows x86 and Linux x86 (`-m32`)

**Project Type**: Native plugins (3 modules + shared header lib)

**Performance Goals**: No regression versus 0.3.7 baseline in audio latency or server tick cost

**Constraints**: Constitution I-V; `documents/sampvoice.inc` unchanged; wire protocol unchanged

**Scale/Scope**: 12 per-version address getters in `client/libraries/util/addresses.hpp`; about 99 files
in `client/libraries/samp` carry `SAMP_R1`/`SAMP_R3` splits; `control/main.cpp` uses `pNetGame` in many places

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Note |
|---|---|---|
| I Upstream-Sync Friendly | Pass | Add a DL variant beside R1/R3. No folder or `.sln` path renames. |
| II Module Boundaries | Pass | Version tables stay in `client`. Host adapter stays in `control`. `common` untouched. |
| III Pawn API Stability | Pass | No `.inc` signature change. |
| IV Real-Time and Network Safety | Pass | Protocol unchanged. Host adapter MUST null-check host data. |
| V Verify Before Done | Pass | Quickstart defines build and manual checks. |

Post-design re-check: Pass, with one open risk (R-03) that may force a spec change to FR-008.

## Project Structure

### Documentation (this feature)

```text
specs/001-port-03dl-openmp/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   └── host-adapter.md
└── tasks.md             # created by /speckit-tasks
```

### Source Code (repository root)

```text
client/
├── libraries/util/addresses.hpp   # add DL offsets (13 getters)
├── libraries/samp/*               # add DL struct variants where layout differs
└── client.vcxproj                 # add DL build configuration
control/
├── main.cpp                       # route host data access through adapter
├── libraries/ysf/*                # unchanged for SA:MP, bypassed for open.mp
└── host.hpp (new)                 # small host adapter, see contracts/host-adapter.md
voice/
└── Makefile / voice.vcxproj       # verify load under open.mp; likely no code change
documents/
└── sampvoice.inc                  # unchanged
README.md                          # install docs for DL and open.mp
```

**Structure Decision**: Extend existing folders only. One new header (`control/host.hpp`) isolates the
server-host differences so SA:MP and open.mp share the rest of `control`.

## Risks

- **R-03 (high)**: `control` reads players, vehicles and objects through `pNetGame` from
  `PLUGIN_DATA_NETGAME`. open.mp docs say YSF is not needed, and list `sampvoice` under `components`,
  not legacy plugins. The legacy-plugin choice (FR-008) may not be feasible for `control` without a
  native component. Decision gate: a spike first. If it fails, amend FR-008.
- **R-01 (high)**: 0.3.DL client offsets and struct layouts are unknown in this repo. Need reverse
  engineering of the DL `samp.dll`.
- **R-02 (medium)**: Mixed 0.3.7 and 0.3.DL clients on open.mp need a defined behavior (spec edge case).

## Complexity Tracking

No constitution violations.
