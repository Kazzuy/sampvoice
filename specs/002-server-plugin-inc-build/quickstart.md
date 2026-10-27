# Quickstart: validation scenarios

## Prerequisites

- VS 2022 Build Tools with toolset `v141_xp` (see 001 quickstart), CMake, `pawncc`.
- open.mp SDK checkouts (001 quickstart pins `08cc69a` and `f223fbd`); set `OMP_DEPS_DIR`.
- Clean server folders: open.mp rig `D:\gg\omptest`, SA:MP R2 rig `D:\gg\svtest` (reset to clean copies first).

## Build

```powershell
./setup/package-server.ps1 -OmpDepsDir $env:OMP_DEPS_DIR -PawncPath <pawncc.exe>
```

Expect: `dist/sampvoice-server-omp/` and `dist/sampvoice-server-samp/` match
[contracts/package-layout.md](contracts/package-layout.md); exit code 0.

## Scenarios

1. **Prerequisite error (FR-006)**: run with `-OmpDepsDir` pointing at an empty folder. Expect a message naming `open.mp-sdk`, non-zero exit, no `dist/sampvoice-server-omp`.
2. **Layout (FR-001/002/007/008)**: compare `MANIFEST.txt` file lists against the contract; run twice, diff the file lists (SC-004).
3. **Include identity (FR-003, SC-005)**: SHA-256 of `include/sampvoice.inc` and `.hnd` equals the `documents/` copies.
4. **Include compile (FR-004, SC-002)**: `pawncc tests/pawn/svcoverage.pwn -i dist/sampvoice-server-omp/include`. Expect zero errors, zero warnings from the include.
5. **open.mp start (FR-005, SC-001)**: copy package into a clean open.mp folder, start server. Expect "Successfully loaded component SampVoice Control" and `[sv:omp] SampVoice control component ready`. No separate relay: see scenario 9.
6. **SA:MP R2 start (FR-005, SC-001)**: copy package into clean `svtest`, add `plugins sampvoice`, start. Expect the banner line `SampVoice by MOR loaded`.
7. **Native behavior (US2 scenario 2)**: run `svcoverage` gamemode on the open.mp rig; 001 verified 8 natives, remaining natives are new coverage.
8. **Stale output (edge case)**: put a stray file in `dist/sampvoice-server-omp/`, rerun. Expect it gone.
9. **Embedded relay (FR-001, FR-005)**: start the open.mp rig with no `sampvoice_voice.exe`. Expect UDP 2020 owned by `omp-server.exe` and `voice service opened` in `voice.log`. Stop with Ctrl+C. Expect exit code 0 within 5 s and `failed to initialize command service` as the last `voice.log` line. Running a second relay against the same server is not supported.

## Not covered

Linux package; audio between two clients; 0.3.7 R1/R3 client regression.

## Build procedure (T023)

Prerequisites: VS 2022 Build Tools with toolset `v141_xp`; CMake (ships with Build Tools); open.mp SDK checkouts
`open.mp-sdk` @ `08cc69a` and `open.mp-network` @ `f223fbd` in one folder (here `D:\depeloper\Kazzuy\vibe\omp-deps`);
a `pawncc.exe` (here `D:\depeloper\Kazzuy\kazzuyOMP\qawno\pawncc.exe`).

```powershell
$env:OMP_DEPS_DIR = 'D:\depeloper\Kazzuy\vibe\omp-deps'
./setup/package-server.ps1 -PawncPath D:\depeloper\Kazzuy\kazzuyOMP\qawno\pawncc.exe
```

Options: `-Platform omp|samp|all`, `-SkipPawn` (prints `NOT VERIFIED: FR-004, SC-002`, still exit 0).
Output: `dist/sampvoice-server-omp/` and `dist/sampvoice-server-samp/` as in [contracts/package-layout.md](contracts/package-layout.md).
The open.mp CMake build tree lives in `%LOCALAPPDATA%\sampvoice_build_omp` (not under `%TEMP%`, MSB8029).

## Results (2026-10-27)

Rigs were fresh copies (`D:\gg\omptest_002`, `D:\gg\svtest_002`) with the old plugin/component and relay deleted first;
copied files matched `MANIFEST.txt` SHA-256 (0 mismatches).

- Script, full run: `PASS sampvoice-server-omp`, `PASS sampvoice-server-samp`, exit 0 (includes include-identity and Pawn coverage compile, zero warnings from `sampvoice.inc`).
- Scenario 1: `-OmpDepsDir C:\does\not\exist` gives `missing 'open.mp-sdk' ...`, exit 1, no `dist/` created.
- Scenario 2/8 (T022): two runs gave identical `MANIFEST.txt` path lists for both packages; a stray file in `dist/sampvoice-server-omp/` was removed by the rerun.
- Scenario 5 (T014, written before the relay was embedded; the package no longer has the exe, see scenario 9), open.mp 1.6.7.3218: `Successfully loaded component SampVoice Control (4.2.0.0)`, `[sv:omp] SampVoice control component ready`; `sampvoice_voice.exe` connected to control (`127.0.0.1:2020`) and opened the voice service. The log also shows `Unable to start legacy network on port 7779. Port in use?` because an unrelated open.mp server (Imperion) was running; unrelated to SampVoice.
- Scenario 6 (T015), SA:MP 0.3.7 R2: `Loading plugin: sampvoice`, banner `SampVoice by MOR loaded`, `Loaded 1 plugins.`; relay connected and opened the voice service.
- Scenario 7 (T020), open.mp rig: `svcoverage` gamemode called all 37 natives (every `SV_FILTER` value included) with no runtime error and printed `[svcoverage] all natives called`. With player 0 absent the player natives only prove the call path, not return values.
- Timing for SC-001 (under 5 minutes) not measured separately; each rig took one copy step and one start.

### Convergence results (2026-10-27)

- T028 (FR-007): `SampVoiceControl.dll` embedded `D:\depeloper\Kazzuy\vibe\omp-deps\...\span.hpp`. `/pathmap` alone was ignored by cl (warning D9007: it requires `/experimental:deterministic`). `Build-ControlOmp` now sets `CXXFLAGS` with `/experimental:deterministic` plus `/pathmap` for the open.mp deps and the repo root (backslash and slash forms), on a fresh build tree. The embedded string is now `/omp-deps\open.mp-sdk\lib\span-lite\include\nonstd/span.hpp`. `Test-Package` now scans `.dll`/`.exe` bytes (ASCII and UTF-16) for the repo root, the deps folder and `%USERPROFILE%`; it failed the old build and passes the new one. Both packages PASS; the component still loads and logs the ready line on the open.mp rig.
- T029 (SC-001): measured from file copy to the ready line, stale logs deleted first: open.mp 0.5 s, SA:MP R2 0.2 s. This excludes the time to read `INSTALL-SERVER.md`, so the 5-minute target is met with a large margin.
- T030 (FR-006): `-PawncPath C:\no\pawncc.exe` and a run with no `-PawncPath` and no `-SkipPawn` both stop with `pawncc.exe not found: pass -PawncPath <pawncc.exe> or -SkipPawn`, exit non-zero, `dist/` unchanged (16 files before and after).

### Embedded relay and Makefile results (2026-10-27)

- Scenario 9 (T034), open.mp 1.6.7.3218 on the copy of the Imperion server (no `sampvoice_voice.exe` present): UDP 2020 owned by `omp-server.exe`; `voice.log` shows `connection to control server established`, `voice service opened`, `launching 7 voice workers...`. Ctrl+C: before the `gCommandAbort` fix the process did not exit in 40 s (`voice.log` ended at `connecting to control server`); after the fix it exited with code 0 after 2.5 s and `voice.log` ended with `failed to initialize command service`. The packaged dll (built with `/pathmap`) later ran on the copy and on the Imperion server and stopped cleanly (same last `voice.log` line); the exit code was not captured for those two runs.
- T031 (Constitution V): `voice/Makefile` now builds `$(filter-out embed.cpp,$(wildcard *.cpp))`. **Makefile build unverified**: this machine has no Linux, WSL, `g++` or `make`. The MSBuild build of `voice.vcxproj` (which does not list `embed.cpp`) and the CMake build of `control/omp` pass through `setup/package-server.ps1`.
