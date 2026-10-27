# Data Model: Build Server Plugin and Pawn Include Library

No runtime data. Entities describe build inputs and outputs.

## Server package

| Field | Rule |
|-------|------|
| `platform` | `omp` or `samp`; fixes the plugin file name and folder |
| `root` | `dist/sampvoice-server-<platform>/` |
| `files` | exactly the set in [contracts/package-layout.md](contracts/package-layout.md) |
| `manifest` | `MANIFEST.txt`: sorted `relative/path  sha256` lines |

State: `absent` → `assembled` → `verified`. A package is `verified` only after all checks pass
(include hash, no leaked paths, forbidden extensions absent). A failed check leaves it `assembled`
and the script exits non-zero.

## Include library

| Field | Rule |
|-------|------|
| `sampvoice.inc` | SHA-256 equals `documents/sampvoice.inc` |
| `sampvoice.hnd` | SHA-256 equals `documents/sampvoice.hnd` |

## Build procedure inputs

| Input | Source | Missing → |
|-------|--------|-----------|
| MSBuild with `v141_xp` | VS 2022 Build Tools | error naming toolset |
| CMake | Build Tools folder or PATH | error naming CMake |
| `OMP_DEPS_DIR` | param or env | error naming both checkouts and pinned commits |
| `pawncc` | param `-PawncPath` | smoke test skipped only with `-SkipPawn`; otherwise error |
