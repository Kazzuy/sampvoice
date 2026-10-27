# SampVoice server install

Pick the package that matches your server. Do not mix them.

| Package | Server |
|---------|--------|
| `sampvoice-server-omp` | open.mp |
| `sampvoice-server-samp` | SA:MP 0.3.7 R2 |

open.mp blocks a legacy plugin named `sampvoice`. If you copy the SA:MP package into an open.mp server, the plugin will not load. Use the open.mp package there.

## Files

| File | Destination |
|------|-------------|
| `components/SampVoiceControl.dll` (open.mp) | server `components/` folder |
| `plugins/sampvoice.dll` (SA:MP) | server `plugins/` folder |
| `sampvoice_voice.exe` | server root, next to the server executable |
| `control.cfg` | server root (read by the plugin or component) |
| `voice.cfg` | server root (read by `sampvoice_voice.exe`) |
| `include/sampvoice.inc`, `include/sampvoice.hnd` | your Pawn include folder |

## Steps

1. Copy the files to the destinations above.
2. SA:MP only: add `plugins sampvoice` to `server.cfg` (use `plugins sampvoice.dll` on Windows if your server needs the extension). open.mp: nothing to add, the component loads from `components/`.
3. Start the server. Expect one of these log lines:
   - open.mp: `[sv:omp] SampVoice control component ready`
   - SA:MP: the banner line `SampVoice by MOR loaded`
4. Start `sampvoice_voice.exe` from the server root so it finds `voice.cfg`.

## Ports

Defaults need no edits when everything runs on one machine: control service TCP 2020 (localhost), voice service UDP 2020. Open UDP 2020 in the firewall. Change values in `control.cfg` and `voice.cfg`.

## Compile scripts

Compile with the package `include` folder first, for example `pawncc gamemode.pwn -iinclude`. An older `sampvoice.inc` in the compiler include folder can win over the new one if it is searched first.
