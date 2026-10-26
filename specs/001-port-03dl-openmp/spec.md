# Feature Specification: Port SampVoice to SA:MP 0.3.DL Client and open.mp Server

**Feature Branch**: `001-port-03dl-openmp`

**Created**: 2026-10-26

**Status**: Draft

**Input**: User description: "porting ke client 03DL dan openmp"

## Clarifications

### Session 2026-10-26

- Q: Dalam bentuk apa server SampVoice harus tersedia untuk open.mp? → A: Plugin SA:MP lama yang dimuat open.mp (`voice` dan `control` yang ada); component native out of scope. (Superseded 2026-10-26, see next entry.)
- Q: Setelah spike R-03 gagal, bagaimana server open.mp disediakan? → A: Komponen open.mp native untuk `control` (protokol v4); `voice` tetap proses terpisah.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Player uses voice chat on a 0.3.DL client (Priority: P1)

A player runs the SA:MP 0.3.DL client, installs the SampVoice client plugin, joins a server that has
voice enabled, and can hear other players and speak to them.

**Why this priority**: 0.3.DL is the client players use on servers with custom models. Without client
support these players cannot use voice at all.

**Independent Test**: Install the client on a 0.3.DL game, join a voice-enabled test server with a second
player, and confirm two-way audio and working menu/mic icon.

**Acceptance Scenarios**:

1. **Given** a 0.3.DL client with the plugin installed, **When** the player connects to a voice-enabled server, **Then** the plugin loads without crash and shows its connected state.
2. **Given** two connected players, **When** one speaks, **Then** the other hears the audio with the server-defined routing.
3. **Given** a 0.3.DL client with an unsupported build, **When** the plugin starts, **Then** it shows a clear message and does not crash the game.

---

### User Story 2 - Server owner runs voice on open.mp (Priority: P1)

A server owner running open.mp installs the SampVoice server side, loads it with their gamemode, and
players get voice chat with the same features as on a SA:MP server.

**Why this priority**: open.mp is the actively developed server platform that replaces SA:MP. Without
support, servers that migrate lose voice chat.

**Independent Test**: Start an open.mp server with SampVoice installed, connect a client, and confirm
streams, mic control, and script callbacks work.

**Acceptance Scenarios**:

1. **Given** an open.mp server with SampVoice installed, **When** the server starts, **Then** SampVoice loads and reports a ready state in the server log.
2. **Given** a gamemode using the existing Pawn voice API, **When** it runs on open.mp, **Then** every existing native and callback behaves as on SA:MP.
3. **Given** a voice stream bound to a player or object, **When** players move, **Then** audio follows as before.

---

### User Story 3 - Existing SA:MP users are not broken (Priority: P2)

Servers and players still on SA:MP 0.3.7 (client R1/R3, server R2) keep working after the port.

**Why this priority**: The fork must not regress current users or upstream compatibility.

**Independent Test**: Run the existing SA:MP 0.3.7 setup with the new build and repeat the baseline voice test.

**Acceptance Scenarios**:

1. **Given** a SA:MP 0.3.7 R1/R3 client and R2 server, **When** voice is used, **Then** behavior matches the pre-port build.

---

### Edge Cases

- A 0.3.7 client joins an open.mp server (and the reverse): voice MUST either work or fail with a clear message, never crash.
- A client with an unknown or mismatched game build runs the plugin: plugin MUST disable itself safely.
- A server restarts or reloads its gamemode mid-session: connected clients MUST reconnect to voice without a game restart.
- Server runs on Windows vs Linux: behavior MUST match on both.
- Existing Pawn scripts compiled against the old include file MUST still load.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The client plugin MUST load and run on the SA:MP 0.3.DL client.
- **FR-002**: The client plugin MUST keep every current feature on 0.3.DL: capture, playback, effects, menu, mic icon, and player-list indicators.
- **FR-003**: The server side MUST load and run on open.mp.
- **FR-004**: The existing Pawn API (`documents/sampvoice.inc`) MUST work on open.mp with no signature or meaning changes.
- **FR-005**: Existing SA:MP 0.3.7 client (R1, R3) and server (R2) support MUST NOT regress.
- **FR-006**: The client MUST detect an unsupported game build and disable itself with a clear message instead of crashing.
- **FR-007**: Installation for 0.3.DL players and open.mp servers MUST be documented, including the installer and manual options.
- **FR-008**: The server side MUST run on open.mp as a native open.mp component that provides the `control` function (same v4 protocol and Pawn API as the SA:MP plugin); the `voice` server stays a separate process; the SA:MP 0.3.7 R2 plugin build stays supported. Legacy-plugin loading is not used (open.mp blocks a legacy plugin named `sampvoice`, and `control` needs the RakNet server export that open.mp does not provide; see research.md R-03).
- **FR-009**: All network input from clients MUST remain validated on the new server path (constitution Principle IV).

### Key Entities

- **Client build target**: A game client version the plugin supports (0.3.7 R1, R3, 0.3.DL), with a detection result per launch.
- **Server host**: The server software the voice side runs in (SA:MP 0.3.7 R2, open.mp), with its load method.
- **Voice stream**: Audio route between players, bound to players, objects, or positions; unchanged by this feature.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A player on 0.3.DL joins a voice-enabled server and has working two-way audio within 2 minutes of install.
- **SC-002**: A server owner installs SampVoice on open.mp and sees voice active within 10 minutes using the docs alone.
- **SC-003**: 100% of existing Pawn API natives and callbacks pass the same check on open.mp as on SA:MP.
- **SC-004**: Zero regressions on the SA:MP 0.3.7 baseline voice test.
- **SC-005**: A 30-minute session with 10 or more players runs on 0.3.DL plus open.mp without a crash.

## Assumptions

- "03DL" means the SA:MP 0.3.DL client (the 0.3.7-based client with custom model support).
- Players and servers on 0.3.7 R1/R3/R2 stay supported; this is an addition, not a replacement.
- Windows and Linux are both in scope for the server; the client is Windows only.
- The open.mp component is built with the open.mp SDK (MIT). The existing open.mp port at `AmyrAhmady/sampvoice` (MIT, protocol v3.x) is a reference for the integration only; its protocol is not compatible with v4 clients.
- Existing wire protocol between client and server stays compatible unless a change is unavoidable.
- A test environment with a 0.3.DL client and an open.mp server is available.
