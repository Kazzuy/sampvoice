# Quickstart: validation scenarios

## Prerequisites
- Visual Studio with the toolset used by `sampvoice.sln`; Linux or WSL with `g++ -m32` for server builds.
- A 0.3.DL client, a 0.3.7 R1 or R3 client, an open.mp server, and a SA:MP 0.3.7 R2 server.

## Build
Toolchain notes (T002, verified 2026-10-26): VS 2022 Build Tools with workload VCTools plus components
`Microsoft.VisualStudio.Component.VC.v141.x86.x64` and `Microsoft.VisualStudio.Component.WinXP`
(projects use toolset `v141_xp`; installer needs an elevated shell). `client` also needs DirectX SDK
June 2010 and `DXSDK_DIR` set to its install folder. With these, `voice`, `control`, `client` (R1 default)
build in Release|Win32 via `MSBuild.exe <proj>.vcxproj /p:Configuration=Release /p:Platform=Win32`.
Outputs: `voice/binaries/sampvoice.exe`, `control/binaries/sampvoice.dll`, `client/binaries/sampvoice.asi`.
1. Open `sampvoice.sln`. Build `client` for the DL configuration, plus existing R1/R3.
2. Build `voice` and `control` (solution, or `Makefile` in each folder).

## Scenarios
1. **Spike (R-03)**: load unmodified `control` in open.mp. Expect a log line showing NetGame available or not.
2. **DL client**: join a voice server with two players. Expect two-way audio, mic icon, menu.
3. **Unsupported build**: run the plugin on an unknown build. Expect a message and no crash (FR-006).
4. **open.mp script API**: run a test gamemode that calls every native in `sampvoice.inc`. Expect parity.
5. **Regression**: repeat scenario 2 on 0.3.7 R1/R3 with the R2 server. Expect no change.
6. **Soak**: 30 minutes, 10+ players, DL client plus open.mp. Expect no crash (SC-005).
7. **Gamemode reload**: reload the gamemode mid-session. Expect clients to reconnect to voice without a game restart.
8. **Linux open.mp**: build `voice` and `control` with their `Makefile` and repeat scenarios 1 and 4. Expect parity with Windows.

## open.mp component build (T015-T017, verified 2026-10-26)
Dependencies (outside the repo, MIT): `open.mp-sdk` @ `08cc69a` (2026-10-14, with submodules) and `open.mp-network`
@ `f223fbd`, cloned into `D:\depeloper\Kazzuyibe\omp-deps`.
```
cmake -S control/omp -B <build dir> -G "Visual Studio 17 2022" -A Win32 -DOMP_DEPS_DIR=<omp-deps dir>
cmake --build <build dir> --config Release
```
`cmake.exe` ships with the VS 2022 Build Tools (`Common7\IDE\CommonExtensions\Microsoft\CMake\CMakein`).
Output `SampVoiceControl.dll` goes to the server `components` folder. Verified on open.mp 1.6.7.3218:
"Successfully loaded component SampVoice Control (4.0.0.0)". The test server is `D:\gg\omptest` (port 7779).

## Test results so far (2026-10-26)
Setup: 0.3.DL R1 client from a copy of the game folder (`D:\gg\dl_test`, ASI loader from `client/files/binaries`), open.mp 1.6.7.3218
server `D:\gg\omptest` (port 7779) with `SampVoiceControl.dll` plus the standalone `voice` process.
- Scenario 1 (spike): legacy plugin route fails (research.md R-03); the native component loads (R-08, R-09, R-10).
- Scenario 2 (DL client), partly: plugin loads, all hooks fire, the F11 menu opens, handshake and `ClientInitialize` reach the
  client, speaker module starts, key bindings and streams (`SpeakerSetKey`, `StreamCreate`, `StreamSetTarget`) arrive.
  Not shown: audio between two clients.
- Scenario 3 (unsupported build): not run. The check exists in `client/main.cpp`; the DL build accepts only entry point `0xFDB60`.
- Scenario 4 (natives): `SvGetVersion`, `SvHasMicro`, `SvCreateStream`, `SvSetKey`, `SvAttachStream`, `SvAttachListener`,
  `SvSetTarget`, `SvSetIcon` verified; the rest of `sampvoice.inc` not run.
- Scenarios 5, 7, 8 and 6: not run (no 0.3.7 R1/R3 client on this machine, no Linux, no second client).
- Two game instances on one PC do not work: the second one never reaches "SA-MP Started"; use a second machine.
- Start the game with `samp.exe`. Starting `gta_sa.exe` directly crashed the old build (`samp.dll` missing); fixed by
  `Addresses::IsInitialized()` in `PluginMenu::Initialize`, which now logs and returns false.
- The Pawn include in some servers' `pawno/include/sampvoice.inc` is the old generation; compile test gamemodes with
  `-i documents` first.
