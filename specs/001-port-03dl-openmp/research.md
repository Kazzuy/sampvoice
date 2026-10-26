# Research: 0.3.DL client and open.mp server port

## R-01 Client: how versions are split today
- Decision: Add a third compile-time variant `SAMP_DL` next to `SAMP_R1` and `SAMP_R3`.
- Rationale: `addresses.hpp` and about 99 files in `client/libraries/samp` already branch on
  `#if defined(SAMP_R1)` / `#elif defined(SAMP_R3)`. Same pattern keeps the diff small and mergeable.
- Alternatives: Runtime detection with one binary. Rejected: offsets and struct sizes are compile-time
  today, so it is a large refactor that hurts upstream sync.
- Unknown: DL offsets for the 12 getters (e.g. `RakClientInitialization`, `BassInitialization`) and any
  changed struct layouts. Must come from the DL `samp.dll`. No public reference was confirmed.
- T001 result: `SAMP_R1`/`SAMP_R3` are not set in the vcxproj. `client/libraries/samp/commonSA.h:27` defines
  `SAMP_R1` when neither macro is given (default R1). README.md:153 says to pass `SAMP_R1` or `SAMP_R3` to
  the compiler for other builds. Add `SAMP_DL` to the guard in `commonSA.h` and add a build configuration
  that passes the macro.
- Also version-split: `client/libraries/util/game_utils.hpp` (`disable_anticheat` patches fixed
  `samp.dll` offsets; `#error Unknown SAMP version` otherwise; R3 widescreen radar tweak). DL needs these
  offsets too (T007).
- No runtime version check exists today. `OnSampLoad` (`client/main.cpp:879`) runs after `samp.dll`
  loads and applies fixed offsets. A DL/R1/R3 mismatch writes to wrong addresses. T011a MUST add the check
  before `Addresses::Initialize` / any patch.

- T011a decision: identify the loaded `samp.dll` by PE `AddressOfEntryPoint`, checked in `OnSampLoad`
  before `Addresses::Initialize`. Local files read (header only, 2026-10-26):
  - `D:\gg\GTA San Andreas\samp.dll` and `D:\gg\semPois\samp.dll` (identical, sha256 prefix `bccdb297464bd382`):
    entry `0xFDB60`, size 1466368, timestamp `0x5A6A3130` (Jan 2018), SizeOfImage `0x2BE000`. Confirmed 0.3.DL R1:
    byte-identical (same sha256 prefix) to `samp.dll` extracted, without running it, from
    `https://sa-mp.co.id/files/sa-mp-0.3.DL-R1-install.exe` (NSIS, unsigned, 15656217 bytes).
  - `D:\gg\editor\samp.dll`: entry `0xCBC90`, size 1204224, timestamp `0x6372C39E` (Nov 2022). Not R1/R3.
  - No R1 or R3 `samp.dll` available, so their entry points are unverified.
  - Result: DL build requires entry `0xFDB60`; R1/R3 builds only reject `0xFDB60`. Tighten R1/R3 later.

## R-05 T009 scope: what the client plugin really touches in samp.dll (measured 2026-10-26)
`client/libraries/samp` holds 1807 `SAMP_ADDROF` lines, but plugin code uses only this subset:
- Hook/patch addresses (12, `addresses.hpp`): RakClientInitialization, BassInitialization,
  BassConfiguration, SampInitialization, SampDestruction, OpenChat, SwitchMode, OpenScoreboard,
  CreatePlayerInPool, DeletePlayerFromPool, SpawnLocalPlayer, DrawLabel.
- Anticheat patch offsets in `game_utils.hpp` `disable_anticheat` (R1 has 3 patches, R3 has 2).
- Global pointers: `SAMP::pNetGame`, `pChat`, `pScoreboard`, `pInputBox`, `pGame`.
- Methods: `CChat::AddEntry`, `CInput::AddCommand`, `CInput::Close`, `CScoreboard::Close`,
  `CGame::IsMenuVisible`, `CGame::SetCursorMode`, `CGame::ProcessInputEnabling`,
  `CNetGame::GetPlayerPool`, `CPlayerPool::GetPlayer`.
- Fields: `CChat::m_nMode`, `CInput::m_bEnabled`, `m_szInput`, `m_nCommandCount`,
  `CScoreboard::m_bIsEnabled`, plus player fields read via `GetPlayer` in `plugin_menu.hpp`,
  `black_list.hpp`, `speaker_list.hpp`, `stream.hpp`, `game_utils.hpp`.
- Conclusion: about 30 items to locate in the DL `samp.dll`, not 1807. Every item needs a runtime test
  in a 0.3.DL client; a wrong offset crashes the game.
- Staged plan: (1) 12 hook addresses + anticheat patches, (2) five globals, (3) methods and fields.
  Test each stage in a copy of the DL client before the next.

## R-06 DL-R1 offsets for T007/T008 (2026-10-26)
Target: `samp.dll` 0.3.DL R1 (sha256 prefix `bccdb297464bd382`). Method: (a) found offsets by reading the
DL `samp.dll` code (BASS call sites derived from first principles, R1/R3 diff of 0x30 between
BassInitialization and BassConfiguration matches), (b) cross-checked against the address table inside the
third-party DL build `ompclient/sampvoice.asi.custom-bak` (read only; only offset values taken, no code),
(c) checked each offset in `samp.dll` for a sane instruction/function start.

| Getter | R1 | R3 | DL | Check in DL `samp.dll` |
|---|---|---|---|---|
| RakClientInitialization | 0x2401D3 | 0xB658 | 0xB678 | `mov [esi+0x2C], eax` (bytes `89 46 2C`, matches `0x4689` test in `raknet.hpp`) |
| BassInitialization | 0x628DF | 0x65D2F | 0x65F1F | `call BASS_Init` thunk, found independently |
| BassConfiguration | 0x6290F | 0x65D5F | 0x65F4F | `push eax; push 5; call BASS_SetConfig` = 8 bytes, found independently |
| SampInitialization | 0x2565E2 | 0xC57E2 | 0xC6614 | `mov [g_flag], 1` (10 bytes, hookable) |
| SampDestruction | 0x9380 | 0x9510 | 0x9570 | function start |
| OpenChatFunction | 0x657E0 | 0x68D10 | 0x68EC0 | function start |
| SwitchModeFunction | 0x5D7B0 | 0x60B50 | 0x60D40 | function start |
| OpenScoreboardFunction | 0x6AD30 | 0x6EC80 | 0x6EE10 | function start |
| CreatePlayerInPoolFunction | 0x10D50 | 0x13E80 | 0x14100 | function start (SEH prologue) |
| DeletePlayerFromPoolFunction | 0x10B90 | 0x13CB0 | 0x13F40 | function start (`cmp ax, 0x3EC`) |
| SpawnLocalPlayerFunction | 0x3AD0 | 0x3AD0 | 0x3B20 | function start |
| DrawLabelFunction | 0x686C0 | 0x6C630 | 0x6C7E0 | function start |
| anticheat patch 1 | 0x286923 / 0x298116 | 0xC4DC0 | 0xC5C10 | function start; patch `B8 45 00 00 00 C2 1C 00` |
| anticheat patch 2 | 0x99250 | 0x9D1A0 | 0x9D6F0 | function start; patch `C3` |

Not verified at runtime. DL has no R3 widescreen radar tweak applied (unknown; revisit in T014).
Remaining for T009/T010: global pointers and struct layouts (see R-05).

## R-07 DL struct and method findings for T009/T010 (2026-10-26)
Approach: DL builds reuse the R3 layouts (`SAMP_DL` also defines `SAMP_R3` in `commonSA.h`); only items the plugin
uses are overridden. Evidence is from the DL `samp.dll` code, read statically.

Globals (RVA): pNetGame 0x2ACA24, pChat 0x2ACA10, pInputBox 0x2ACA14, pScoreboard 0x2AC9DC, pGame 0x2ACA3C.
Each equals R3 + 0x3E148; all five also appear, and no other value in that range, in the third-party DL build.

Methods used (R3 -> DL): CChat::AddEntry 0x67460 -> 0x67650 (touches `m_entry` at 0x132, 252-byte entries);
CInput::AddCommand 0x69000 -> 0x691B0 (`cmp [this+0x14DC], 0x90`); CInput::Close 0x68E10 -> 0x68FC0;
CScoreboard::Close 0x6E270 -> 0x6E410; CGame::IsMenuVisible 0xA0210 -> 0xA0760; SetCursorMode 0x9FFE0 -> 0xA0530;
ProcessInputEnabling 0x9FEC0 -> 0xA0410; CNetGame::GetPlayerPool 0x1160 -> 0x1170, GetVehiclePool 0x1170 -> 0x1180,
GetObjectPool 0x2DF0 -> 0x2E40 (each returns an entry of the pools struct at `[this+0x3DE]`);
CPlayerPool::IsConnected 0x10B0 and GetPlayer 0x10F0 unchanged; GetLocalPlayer 0x1A30 -> 0x1A80;
GetName 0x16F00 -> 0x170D0; CRemotePlayer::GetColorAsRGBA 0x15C00 -> 0x15E20; CObjectPool::GetObject 0x2DC0 -> 0x2E10.

Fields read directly (R1 and R3 offsets agree except where noted):
- CInput `m_nCommandCount` 0x14DC, `m_bEnabled` 0x14E0: same in DL. CScoreboard `m_bIsEnabled` offset 0: same.
- CVehiclePool `m_pObject` 0x1134, `m_bNotEmpty` 0x3074, `m_pGameObject` 0x4FB4: same in DL.
- CObjectPool: DL has 2100 objects (R3 1000). `m_bNotEmpty` at 4, `m_pObject` at 0x20D4. `MAX_OBJECTS` is 2100 for DL.
- CPedSA `m_pGamePed` 0x2A4: same in DL (CPedSA methods read `[this+0x2A4]`).
- CRemotePlayer: DL has the id at offset 0 and the ped pointer at offset 4 (R3: ped at 0). Read through
  `samp_compat::remote_player_ped`.
- CLocalPlayer `m_bIsActive`: R1 0xC, R3 0xF4, DL unknown. DL uses the ped pointer (offset 0, confirmed in DL
  `Spawn`) as a stand-in via `samp_compat::local_player_active`.
- Not verified: `CObjectSA::m_pGameEntity` (0x40 in R1 and R3), `CChat::m_nMode` (offset 8).

Build state: `SAMP_DL`, `SAMP_R3`, and default R1 all compile with 0 errors. Nothing is runtime-tested.

## R-02 open.mp serves 0.3.DL and 0.3.7
- Finding: open.mp beta 9 can serve 0.3.7 and 0.3.DL clients from one binary, with 0.3.7 controlled by a
  config setting. Source: https://open.mp/blog/server-beta-9
- Decision: Mixed-client behavior follows the open.mp setting. The plugin MUST not crash on either client.

## R-03 open.mp and legacy plugins (decision gate)
- Finding: open.mp docs put legacy plugins in `plugins` plus the `legacy_plugins` key, but list `sampvoice`
  among items to put in `components`. YSF is "not needed" there.
  Source: https://open.mp/docs/server/Installation
- Not found: any statement that `PLUGIN_DATA_NETGAME` returns a usable `CNetGame` under open.mp.
- Impact: `voice` uses its own socket plus the command channel from `control`, so it likely loads.
  `control` uses `pNetGame` for positions and stream binding. High risk.
- Decision: Run a spike first. Load the unmodified `control` in open.mp and log whether
  `ppPluginData[PLUGIN_DATA_NETGAME]` is non-null and structurally valid.
- Fallback if the spike fails: feed positions from Pawn instead of `pNetGame`, or build a native
  component. Either amends FR-008 and needs user approval.

### R-03 spike result (2026-10-26): FAIL for the legacy-plugin route
Setup: clean open.mp 1.6.7.3218 copy in `D:\gg\omptest` (22 stock components, `allow_037_clients`), our `control`
build as a legacy plugin, our `voice` build as a separate process.
1. open.mp refuses a legacy plugin named `sampvoice`: "Skipping legacy plugin 'sampvoice'; There is an open.mp
   compatible version you can find here: https://github.com/AmyrAhmady/sampvoice/releases". The block is by name.
2. With the file renamed (`svcontrol.dll`) the plugin loads, then fails: `[err:raknet:initialize] failed to find
   'GetRakServerInterface' function address` and `failed to initialize control service`. `control` hooks RakNet
   through that export of the server executable; in open.mp RakNet lives inside the LegacyNetwork component.
3. Natives such as `SvGetVersion` stay unregistered ("Function not registered").
4. `voice` is a standalone process and does not depend on the host, but it needs a working `control`.
Conclusion: FR-008 (legacy plugin on open.mp) is not feasible. A native open.mp component is required for the
server side, or the DL client must speak the protocol of an existing open.mp component.

### Protocol generations seen on this machine
- This repo (HEAD `Main (#164)`): `SV_VERSION` 4.0.0 in `documents/sampvoice.inc`, handshake signature
  `0xDEADC0DE` appended to ClientJoin RPC 25 (`control/libraries/util/raknet.hpp`, `client/.../raknet.hpp`).
- `D:\depeloper\Kazzuyibe\Imperion\components\sampvoice.dll` (666624 bytes, open.mp component, "sampvoice open.mp
  port", built from a GitHub Actions run): no `0xDEADC0DE` constant found. Server rejects our DL client with
  "Plugin Voice tidak terdeteksi" (`SvGetVersion == SV_NULL`).
- `FloresRoleplay` and `ImperiunRoleplay` `plugins/sampvoice.dll` (145920 bytes, 2023): no `0xDEADC0DE` either.
- A 0.3.7 R2 `samp-server.exe` answers a 0.3.DL client with "CONNECTION REJECTED: Incorrect Version", so a DL client
  needs a DL or open.mp server.

## R-08 Native open.mp component for `control` (chosen 2026-10-26, spec FR-008)
What `control` needs from the host, and the open.mp equivalent (from the MIT `open.mp-sdk`, `open.mp-network`, and the MIT
reference port `AmyrAhmady/sampvoice` v3.2.0-omp, protocol v3.x, not compatible with v4):
- Network hook (`control/libraries/util/raknet.hpp`, 627 lines, scans for `GetRakServerInterface`): use SDK
  `NetworkInEventHandler::onReceivePacket/onReceiveRPC` and `NetworkOutEventHandler`, registered on every network in
  `core->getNetworks()`; send with `IPlayer::sendRPC/sendPacket`; kick with `IPlayer::kick()`. The ClientJoin RPC (id 25)
  is seen in `onReceiveRPC`; read the trailing `0xDEADC0DE` signature + connect data from the raw bit stream.
- Players, vehicles, objects (replaces `pNetGame` in `control/main.cpp`): `IPlayerPool`, and `queryComponent` for
  `IVehiclesComponent` and `IObjectsComponent`.
- Pawn natives (`control/pawn.hpp`, 153 `Sv*` references): `IPawnComponent` event `onAmxLoad(IPawnScript&)` and
  `amx_Register` through `IPawnComponent::getAmxFunctions()`; `amx.h` from `control/libraries/pawn` stays usable.
- Tick: `ICoreEventHandler::onTick`.
- Unchanged: TCP command service to the `voice` process, `common/`, `voice/`.
Layout: `control/omp/` (inside the `control` module, so constitution II holds), built with CMake and the SDK
cloned outside the repo. The existing component named `sampvoice` in other servers must be removed before use.
Staging: (1) skeleton loads and logs, (2) handshake + `SvGetVersion`/`SvHasMicro`, (3) command service + `voice`,
(4) positions for streams, (5) all natives.

## R-04 Existing weakness in current code
- `control/main.cpp` `AmxLoad` calls `ppPluginData[PLUGIN_DATA_NETGAME]` with no null check.
  The host adapter adds one (constitution Principle IV).

## R-09 Stage 2 result: handshake works with the 0.3.DL client on open.mp (2026-10-26)
Server: open.mp 1.6.7.3218 (`D:\gg\omptest`, port 7779) with the native component `SampVoiceControl.dll` (stage 2).
Client: 0.3.DL R1 with the DL build of this repo (`sampvoice.asi`, trace build).
- Server log: `[sv:omp] player 0 identified (version 0x04020000, micro 1)`; no kick; player stayed connected.
- Gamemode `SvGetVersion` returned 67239936 (= 0x04020000, version 4.2.0).
- open.mp `printf`/`format` reject `%08X`; use `%d` in test scripts.
- Client log: no errors; one known warning `failed to pause record channel (code:5)` at exit (microphone).
- Not done yet: `ClientInitialize` packet to the client, the TCP command service to `voice`, positions for streams.

## R-10 Stage 3 result: full control core inside the open.mp component (2026-10-26)
`control/main.cpp` is compiled unchanged into `SampVoiceControl.dll` (CMake, `SAMPVOICE_OMP`); host access goes through
`util/host.hpp` and `util/raknet.hpp` (SA:MP versions in `control/libraries/util`, open.mp versions in `control/omp/shim`).
Server: open.mp 1.6.7.3218 `D:\gg\omptest` (port 7779) plus the standalone `voice` process. Client: 0.3.DL R1 with the DL build.
- Component start: "SampVoice by MOR loaded"; no "Function not registered" error (all natives of `control/pawn.hpp` register).
- `voice` connects to the component: "connection to control server established".
- Client log: `[sv:dbg:ClientInitialize] host(127.0.0.1), port(2020), key(0x5327F350), id(0)` then the speaker module
  initializes and detects the audio devices; the plugin menu (F11) opened and ran the microphone check. No kick.
- Gamemode: `SvGetVersion` 67239936 (0x04020000), `SvHasMicro` 1.
- Not tested: audio between two clients, `SvCreateStream` and the other natives, stream positions on open.mp, Linux build.
- The SA:MP `control` plugin still builds (`control.vcxproj`, 0 errors) after the host adapter change; no SA:MP runtime test yet
  because no 0.3.7 R1/R3 client is available on this machine and a 0.3.DL client is rejected by 0.3.7 servers.

## R-11 Input validation review of the open.mp path (T021a, 2026-10-26)
Reviewed `control/omp/Component.cpp` and the shims. All lengths and ids from the network or from Pawn are bounded:
- ClientJoin payload: `FindHandshake` rejects payloads shorter than signature + length byte, scans back without underflow
  (`i != SIZE_MAX`), and checks `declared <= size - begin`. The data is copied (at most 255 bytes into a 256-byte buffer)
  before the core byte-swaps it, so the RPC buffer is never modified.
- Player ids: checked against `kMaxPlayers` (1000) in `onReceiveRPC`, `onPlayerDisconnect`, every `host::` function and `SendHook`.
- Vehicle and object ids reach `host::` only after `control/pawn.hpp` checks them against `MAX_VEHICLES` and `MAX_OBJECTS` (shim values 2000).
- Non-voice clients: `OnConnect(player, nullptr, 0)` is ignored by `ControlService::OnRaknetConnect`, as on SA:MP.
- `voice` and the TCP command service are unchanged.
Not covered: fuzzing, and the open.mp `NetworkBitStream` byte count is trusted for incoming RPCs.
