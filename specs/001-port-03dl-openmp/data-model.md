# Data Model

No persistent data. Build and runtime matrices only.

## Client build target
| Field | Description |
|---|---|
| id | `R1`, `R3`, `DL` |
| macro | `SAMP_R1`, `SAMP_R3`, `SAMP_DL` |
| addresses | 12 offsets in `client/libraries/util/addresses.hpp` |
| detection result | supported / unsupported per launch (FR-006) |

## Server host
| Field | Description |
|---|---|
| id | `samp-037-r2`, `openmp` |
| load method | legacy plugin in both cases (FR-008) |
| netgame access | available / unavailable (decided by spike R-03) |

## Voice stream
Unchanged. See `control/stream.hpp`.
