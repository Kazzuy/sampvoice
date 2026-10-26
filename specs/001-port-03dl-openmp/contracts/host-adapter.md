# Contract: server host adapter (`control/host.hpp`)

Internal interface that hides SA:MP vs open.mp differences from `control/main.cpp`.

| Operation | Behavior |
|---|---|
| `Init(ppData)` | Store plugin data. MUST accept null and report failure, never crash. |
| `IsPlayerConnected(id)` | True if player exists. |
| `PlayerPosition(id, out)` | False if unavailable. |
| `VehiclePosition(id, out)` | False if unavailable. |
| `ObjectPosition(id, out)` | False if unavailable. |
| `PlayerStreamedIn(player, target)` | False if unknown. |

Rule: callers treat "unavailable" as "do not stream" and log once at warning level.

Public Pawn contract: `documents/sampvoice.inc` stays unchanged (FR-004).
