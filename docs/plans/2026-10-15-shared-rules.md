# NIMBY pack shared rules

Version 1.0. Status: Approved by Jeff Fabish, October 15, 2026. Owner: Jeff Fabish.

These rules govern the three in-house mod plans in `docs/plans/` - Legs (`NimbyAutorun`), Gimme (`NimbyGimme`), and Golden God (`NimbyAdmin`) - and the orchestrator session that coordinates them. Each plan links here instead of carrying its own copy, so there is one text to keep current. A plan that needs an exception to a rule records the exception in its own decisions table.

## 1. Introduction

### 1.1. Scope

Branching, shared files, identifiers, key bindings, save footprint, test tooling, quality gates, and release for every addon in `mods/own/nimby-pack/`.

### 1.2. What this is not

- Not a plan. Phases, tests, and acceptance criteria live in each mod's plan.
- Not a coding standard for Enforce Script beyond the identifier and logging rules in section 4.
- Not a live-server runbook. Restart chains and panel procedures live in `docs/operations/`.

### 1.3. Requirements language

The keywords MUST, MUST NOT, REQUIRED, SHALL, SHALL NOT, SHOULD, SHOULD NOT, RECOMMENDED, MAY, and OPTIONAL in this document are to be interpreted as described in RFC 2119.

## 2. Branches, worktrees, and git

### 2.1. Worktrees

Each mod works on its own branch in its own git worktree under `D:\GitHub\nimby-badlands-server-wt\<branch-name>`:

| Mod | Branch | Worktree |
| --- | --- | --- |
| Legs | `feat/autorun` | `D:\GitHub\nimby-badlands-server-wt\feat-autorun` |
| Gimme | `feat/gimme` | `D:\GitHub\nimby-badlands-server-wt\feat-gimme` |
| Golden God | `feat/nimby-admin` | `D:\GitHub\nimby-badlands-server-wt\feat-nimby-admin` |

### 2.2. Commit and push policy

Decided by Jeff, October 15, 2026.

- An implementation session MAY commit and push on its own `feat/*` branch. Commits MUST be small, described, and limited to that mod's folder plus its docs.
- A pushed `feat/*` branch is unreviewed work. Nothing on it is released, referenced in the Notion hub, or described as done until Jeff merges it.
- Jeff runs every merge to `main` and every push of `main`. Sessions MUST NOT merge, rebase onto, or push `main`.
- Changes to orchestrator-owned files (section 3) land on `main` through Jeff's commit, never on a `feat/*` branch.

## 3. Shared files

Only the orchestrator edits these files. A session that needs a change there asks the orchestrator and waits.

| File | Why it is shared |
| --- | --- |
| `mods/own/nimby-pack/pack.json` | Addon rows, boot check, and test server for every mod |
| `scripts/Build-NimbyPack.ps1` | Build, sign, verify, boot check, and the server-mod output (section 8) |
| `scripts/modules/*.psm1` | PBO packer and Steam helpers |
| `scripts/Start-NimbyTestServer.ps1` | The interactive test server every plan's tests run on |
| `docs/plans/2026-10-15-shared-rules.md` | This file |

Known `pack.json` edits and their timing: Nasdara mission in `bootCheck` and `testServer` before any worktree branches; the `NimbyAdmin` rows at Golden God Phase 0; the Gimme rename at Gimme Phase 2.

## 4. Identifiers

### 4.1. Names

| Mod | Display name | Code and PBO prefix | Log prefix | RPC id block |
| --- | --- | --- | --- | --- |
| Legs | Legs | `NimbyAutorun` | `[NIMBY][Autorun]` | `65501xx` (`6550101` toggle, `6550102` server cancellation) |
| Gimme | Gimme | `NimbyGimme` | `[NIMBY][Gimme]` | `65502xx` (reserved; Gimme uses the action framework today) |
| Golden God | Golden God | `NimbyAdmin` | `[NIMBY][Admin]` | `65503xx` |

A display name changes only `mod.cpp`, Workshop text, string tables, and menu titles. Code prefixes, log prefixes, and RPC ids MUST NOT change for a display-name decision.

### 4.2. Player identity in logs

Two identities exist: `PlayerIdentity.GetId()` is the BI hash that the server `.ADM` log uses; the Steam64 id (expected `GetPlainId()`, verified in each Phase 0) is what bans and permissions key on. Every log line that names a player MUST carry both. Log prefixes MUST appear exactly as listed so `Start-NimbyTestServer.ps1` can collect them.

### 4.3. Logging levels

Enforce Script has no runtime log level. Each addon MUST read one flag file at mission start, `$profile:<Prefix>\debug`, and write debug lines only when it exists. Production logs carry forced clears, refusals, audit records, and failures; never per-toggle or per-frame lines.

## 5. Key bindings

| Mod | Default binding | Status |
| --- | --- | --- |
| Legs | numpad plus (`kAdd`) | Decided (A3) |
| Gimme offer/request | `O` (`kO`) | Decided |
| Gimme pocket toggle | Unbound; Controls label "Gimme: open or close pockets" | Decided (G2), October 15, 2026 |
| Golden God menu | Home, End, F10, in that order; first verified-free wins; otherwise unbound | Decided (N2) |

No two mods share a default binding, and none shadows a vanilla 1.30 default. "Verified free" means checked against the unpacked 1.30 `inputs.xml`, with the file:line recorded in the plan.

## 6. Save footprint

- No addon MAY override `OnStoreSave` or `OnStoreLoad`, add data to any saved entity, or declare a persistent entity class.
- Net-synced variables are allowed; they are not persisted.
- Per-addon state lives in `$profile:<Prefix>\` as JSON or flag files and MUST be written atomically (temp file, validate, rename) with a `.lkg` copy where the file controls access or admission.
- Adding, updating, or removing any addon MUST NOT change what loads from `storage_1`. The release gate in section 9 proves it.

## 7. Shared 1.30 script reference

Before any session writes code, the orchestrator unpacks the 1.30 `scripts.pbo` (DayZ Tools or `D:\GitHub\DayZExtract`) to one read-only folder that all three sessions grep, and records the build number and archive hashes in `docs/baselines/`. No vanilla API is used without a file:line from that folder, and a declaration is not proof of behavior: each plan's Phase 0 exercises the API in game.

## 8. Test tooling

The tooling below is Plan 0 - shared tooling, owned by the orchestrator and merged to `main` before any worktree branches. Every mod plan's Phase 0 depends on it.

| Change | File | Rule |
| --- | --- | --- |
| Nasdara mission | `pack.json` `bootCheck.mission`, `testServer.mission` | `dayzOffline.nasdara` in both |
| Explicit artifact | `Start-NimbyTestServer.ps1` | Takes `-BuildPath`, or selects only a build whose `build.log` records PASS; never "newest folder by name" |
| Manifest record | both scripts | Each run records the addon list and PBO hashes it ran |
| Reusable profile | `Start-NimbyTestServer.ps1` | `-ProfileName` reuses a designated test profile and its `storage_998` |
| Storage separation | both scripts | Test storage is `storage_998` and `storage_999` only; `storage_1` is never written |
| Storage auto-fix | both scripts | `storageAutoFix = 0` for qualification runs so corruption is visible; `1` is allowed only for the quick compile and boot check |
| Graceful stop | `Start-NimbyTestServer.ps1` | `BattlEye = 1`, a generated `beserver_x64.cfg` with a random RCON password, and an RCON `#shutdown` before any process stop (decided October 15, 2026) |
| Server-mod output | `Build-NimbyPack.ps1` | A second output folder for `-servermod=` PBOs that the Workshop item never contains (section 8.1) |
| `includePatterns` | `pack.json` | Add `*.layout`; a client menu's layout files MUST be inside the completeness check |
| Dev harness | `mods/own/dev/NimbyDevHarness/` | Built only with `-Dev`, signed with a separate dev key, never listed in `pack.json` `addons`; the only place forged-request tests live |
| Self-test | each addon | A `$profile:<Prefix>\selftest` flag runs the addon's engine-free validators at mission start and prints `[NIMBY][<Prefix>][SelfTest] PASS|FAIL <case>`; the boot check fails on any FAIL |

### 8.1. Client and server PBO split

Decided by Jeff, October 15, 2026, for Golden God. Server-only code (permissions, bans, audit, webhook, command handlers) ships in a separate PBO loaded with `-servermod=`. Rules:

- No client-shipped script MAY reference a class defined in a server-only PBO.
- The server-only PBO is not signed for clients and is never part of the Workshop item.
- `Build-NimbyPack.ps1` writes it to `build/@NIMBY-Server-<timestamp>/`, and the boot check loads both outputs.
- HostHavoc support for `-servermod=` is verified in Golden God Phase 0 before any server-only code is written.

Legs and Gimme stay single-PBO unless their plan records otherwise.

## 9. Quality gate and release

### 9.1. Quality gate

A change is done only when `Build-NimbyPack.ps1` exits 0 (build, sign, verify, boot check on Nasdara, self-test), its plan's in-game cases pass with the evidence named in the plan, and the test record names the exact artifact.

### 9.2. Release model

Decided by Jeff, October 15, 2026: independent, gated. Each mod ships on its own when it passes. Before any release, including a single-mod release:

1. Boot the release artifact locally against a copy of live `storage_1` with `storageAutoFix = 0`.
2. The RPT MUST show 0 "Scripted variables corrupted", 0 `SCRIPT (E)`, and no new save-load errors against the same-build vanilla control.
3. Back up live mission storage and every `$profile:Nimby*` folder together.
4. Deploy at a scheduled restart. A launch-line change restarts the server, so it lands in the restart window.
5. Retain the previous package and the pre-release backup, and document how clients obtain the matching rollback version.

### 9.3. Distribution

Decided by Jeff, October 15, 2026: one public Steam Workshop item, `@NIMBY`, containing only release-qualified client PBOs. Consequences: every third-party piece keeps its attribution and permission evidence inside the pack and in `NOTICE`; the Gimme permission capture is a release blocker; the preview image stays small because it is the only real download-size driver; the pack README's "Not a public Workshop release" line is superseded.

## 10. Guardrails

- No live-server change without Jeff's approval. Never take the server offline outside the restart chain.
- Jeff types RCON passwords.
- Do not reopen CF/COT removal, `lightingConfig` 3, 1PP, or the hardcore profile.
- Never write the armed-vehicle name.
