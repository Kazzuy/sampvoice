# Feature Specification: Build Server Plugin and Pawn Include Library

**Feature Branch**: `002-server-plugin-inc-build`

**Created**: 2026-10-27

**Status**: Draft

**Input**: User description: "build server plugin & library inc"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Server owner gets a ready-to-install server package (Priority: P1)

A server owner wants the SampVoice server side without compiling it. They obtain one package per
server platform (SA:MP 0.3.7 R2 and open.mp). Each package holds the server plugin or component,
the voice relay, default config files, and an install note. They copy it into the server folder and
start the server.

**Why this priority**: Without built server binaries nobody can run voice chat. This is the core
deliverable.

**Independent Test**: On a clean server folder, copy one package, follow the install note, start the
server, and confirm the log shows the ready line from FR-005.

**Acceptance Scenarios**:

1. **Given** a clean open.mp server folder, **When** the open.mp package is copied in and the server starts, **Then** the log shows the ready line from FR-005.
2. **Given** a clean SA:MP 0.3.7 R2 server folder, **When** the SA:MP package is copied in and the server starts, **Then** the log shows the ready line from FR-005.
3. **Given** a package, **When** the owner inspects it, **Then** every file needed to run is present and no build-machine path or secret is inside.

---

### User Story 2 - Script author gets the Pawn include library (Priority: P1)

A gamemode author wants the SampVoice Pawn include (`sampvoice.inc`) in a form that compiles on
their toolchain. They drop it into the include folder and compile a script that uses the voice API.

**Why this priority**: Scripts cannot call the voice natives without the include.

**Independent Test**: Compile a test gamemode that calls every native of the include and uses every `SV_FILTER` value
with the standard Pawn compiler and with the open.mp compiler setup. Both must succeed.

**Acceptance Scenarios**:

1. **Given** the include in a clean include folder, **When** a test gamemode that uses every native is compiled, **Then** compile ends with zero errors and zero warnings from the include.
2. **Given** a server running the package from Story 1, **When** the compiled gamemode runs, **Then** each native behaves as documented.
3. **Given** the include shipped in the package, **When** it is compared with the repository copy, **Then** it is byte-identical (no signature or meaning change).

---

### User Story 3 - Maintainer rebuilds everything with one documented procedure (Priority: P2)

A maintainer wants to rebuild all server artifacts from a fresh checkout. They follow one written
procedure and get the same package layout every time.

**Why this priority**: Repeatable builds make releases and upstream merges safe, but the first
package can be made by hand.

**Independent Test**: Follow the procedure on a machine with only the listed prerequisites and
compare the produced package layout with the previous build.

**Acceptance Scenarios**:

1. **Given** a fresh checkout and the listed prerequisites, **When** the maintainer runs the procedure, **Then** all server packages are produced without manual edits.
2. **Given** a missing prerequisite, **When** the procedure runs, **Then** it stops with a message that names the missing item.

---

### Edge Cases

- A prerequisite (compiler toolset, SDK, open.mp SDK) is missing or the wrong version: the build stops with a named error, not a partial package.
- The owner copies the SA:MP package into an open.mp server: the install note states that open.mp blocks a legacy plugin named `sampvoice`, so the wrong package is easy to spot.
- A very old `sampvoice.inc` already sits in the compiler include path: the install note tells the author which copy wins.
- Build output from a previous run exists: a rebuild must not mix old and new files in the package.
- A Linux build is requested: out of scope unless the existing Linux build already works (see Assumptions).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The build MUST produce a server package for open.mp that contains the SampVoice component with the voice relay built into it (no separate relay executable to start), default configuration files, and an install note.
- **FR-002**: The build MUST produce a server package for SA:MP 0.3.7 R2 that contains the SampVoice plugin, the voice relay, default configuration files, and an install note.
- **FR-003**: The build MUST ship the Pawn include library (`documents/sampvoice.inc` plus `documents/sampvoice.hnd`) unchanged in each package.
- **FR-004**: The shipped include MUST compile without error or warning in a test gamemode that uses every native it declares and every `SV_FILTER` value.
- **FR-005**: A server started with a package MUST log `[sv:omp] SampVoice control component ready` (open.mp) or the banner line `SampVoice by MOR loaded` (SA:MP).
- **FR-006**: The build MUST stop with a message naming the missing item when a prerequisite is absent.
- **FR-007**: Each package MUST contain only files needed to run; it MUST NOT contain secrets, build-machine paths, or agent tooling folders.
- **FR-008**: Each package MUST have a documented folder layout that is the same on every build.
- **FR-009**: The build procedure MUST be written down and MUST NOT require manual file edits.
- **FR-010**: The build MUST NOT change the public Pawn API or move existing top-level folders.

### Key Entities

- **Server package**: A folder or archive for one server platform; holds plugin or component, voice relay, configs, include library, install note.
- **Include library**: The Pawn include file and its companion file that define the public script API.
- **Build procedure**: The written steps and prerequisites that turn a checkout into packages.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A server owner installs a package into a clean server folder and reaches the FR-005 log line in under 5 minutes using only the install note.
- **SC-002**: 100% of natives and `SV_FILTER` values declared in the include compile in the test gamemode with zero errors and zero warnings.
- **SC-003**: A maintainer produces all packages from a fresh checkout in one run with 0 manual edits.
- **SC-004**: Two builds from the same commit produce packages with identical file lists.
- **SC-005**: The include in every package is byte-identical to the repository copy.

## Assumptions

- Both server platforms (SA:MP 0.3.7 R2, open.mp) are in scope. The open.mp component already exists from feature 001; this feature packages and verifies it.
- The Windows toolchain recorded in the constitution is the supported build environment. A Linux package is produced only if the existing `Makefile` build already works; otherwise it is out of scope for now.
- The open.mp SDK stays outside the repository and is a documented prerequisite.
- No new third-party dependency is added.
- Audio between two clients and the 0.3.7 R1/R3 client regression were not verified in feature 001 and stay outside this feature.
- Distribution (upload, release page) is out of scope; only the built package is produced.
