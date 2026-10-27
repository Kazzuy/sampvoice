# Contract: Server package layout

Stable across builds (FR-008). Files outside this list MUST NOT appear.

## open.mp: `dist/sampvoice-server-omp/`

```text
components/SampVoiceControl.dll     # from control/omp (CMake); the voice relay is built into it (voice/embed.cpp)
control.cfg                         # from control/control.cfg
voice.cfg                           # from voice/voice.cfg
include/sampvoice.inc               # byte-identical to documents/sampvoice.inc
include/sampvoice.hnd               # byte-identical to documents/sampvoice.hnd
INSTALL-SERVER.md                   # from documents/INSTALL-SERVER.md
MANIFEST.txt                        # generated
```

## SA:MP 0.3.7 R2: `dist/sampvoice-server-samp/`

```text
plugins/sampvoice.dll               # from control/binaries/sampvoice.dll
sampvoice_voice.exe                 # from voice/binaries/sampvoice.exe
control.cfg
voice.cfg
include/sampvoice.inc
include/sampvoice.hnd
INSTALL-SERVER.md
MANIFEST.txt
```

## Rules

- Forbidden extensions: `.pdb .ipdb .iobj .exp .lib .obj .tlog .user`.
- No file contains the repo absolute path or the user profile path (text files only).
- `INSTALL-SERVER.md` MUST state: which package fits which server, where each file goes, the line to
  add (`plugins sampvoice` in `server.cfg` for SA:MP; nothing for open.mp, the component loads from
  `components/`), that on SA:MP `sampvoice_voice.exe` runs next to the server with `voice.cfg` (on open.mp the relay is inside the component), UDP/TCP port 2020
  defaults, that open.mp blocks a legacy plugin named `sampvoice`, and to compile with `-i include`
  first so an old `sampvoice.inc` does not win.

## Script interface: `setup/package-server.ps1`

| Param | Meaning |
|-------|---------|
| `-OmpDepsDir <path>` | folder with `open.mp-sdk` and `open.mp-network`; default `$env:OMP_DEPS_DIR` |
| `-PawncPath <path>` | `pawncc.exe` for the include smoke test |
| `-SkipPawn` | skip smoke test (reported as skipped, never as passed) |
| `-Platform omp\|samp\|all` | default `all` |

Exit code 0 only if every selected package is `verified`. Every failure prints the missing or
failing item by name.
