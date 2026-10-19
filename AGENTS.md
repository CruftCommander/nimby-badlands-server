# AGENTS.md

This file provides guidance to Codex when working with code in this repository. It mirrors `CLAUDE.md`; keep the two in step.

## What this is

Configuration, in-house mods, PowerShell tooling, and runbooks for **NIMBY | HARDCORE | BADLANDS | 1pp**, a public DayZ 1.30 server on the Nasdara (Badlands) map, hosted at HostHavoc (Chicago). Not part of the AFS platform. Owner: Jeff Fabish. In-game admin: Jay (Trendkill).

The decision record is the Notion hub "DayZ Badlands Server - Project Hub". This repository holds the artifacts. `STATUS.md` and `NEXT.md` were last updated October 14, 2026, before the Nasdara switch and the mod plans, so the git log and `docs/plans/` are more current than they are.

## Hard rules

- No change reaches the live server without Jeff's approval. Back up first. Deploy only inside the restart chain (`docs/operations/restart-schedule.md`). Jeff types RCON passwords.
- Never commit a password, RCON password, Steam credential, or Discord token. `passwordAdmin` stays empty in `server/serverDZ.cfg`; the live value exists only in the HostHavoc panel file.
- The signing private key lives in `D:\Keys\nimby` and never enters the repository.
- Do not reopen these settled decisions: CF/COT removal, `lightingConfig` 3, 1PP, or the hardcore profile. Never write the name of the armed vehicle.
- PowerShell scripts must run in both Windows PowerShell 5.1 and PowerShell 7 (no `&&`), use `Set-StrictMode -Version Latest`, and write a log file. Each script's comment-based help lists what it Reads, Writes, and Never touches. Keep that block accurate when you change a script.
- Generic DayZ tooling that is not specific to this server belongs in `D:\GitHub\Dogs DayZ ToolZ`, not here.

## Commands

There is no unit-test suite. The quality gate is the build script's boot check: it starts a real local DayZ server with the built pack and scans the logs.

```powershell
# Build, sign, verify, and boot-check @NIMBY (exit 0 = PASS, 1 = FAIL, 2 = UNKNOWN)
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build-NimbyPack.ps1

# Build and verify only, no server boot
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build-NimbyPack.ps1 -SkipBootCheck

# Qualification boot (storageAutoFix = 0) plus the dev harness under the separate nimby_dev key
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build-NimbyPack.ps1 -Dev -Qualification

# Interactive in-game test server: newest SUMMARY PASS build, or a specific one
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Start-NimbyTestServer.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Start-NimbyTestServer.ps1 -BuildPath .\build\@NIMBY-<stamp> -ProfileName <name>

# Against the live replica: 998 = copy of live storage_1, 999 = fresh world; -Vanilla = no mods (control boot)
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Start-NimbyTestServer.ps1 -ServerPath D:\DayZServer -InstanceId 998
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Start-NimbyTestServer.ps1 -ServerPath D:\DayZServer -InstanceId 998 -Vanilla

# Refresh the local replica at D:\DayZServer from the WinSCP mirror of the live server (-WhatIf lists only)
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Sync-LiveReplica.ps1

# Pull live RPT, ADM, and log files over SFTP (needs Posh-SSH; Jeff types the password)
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Get-ServerLogs.ps1

# Copy a subscribed Workshop item into vendor\workshop\<id>\<stamp>\ for reading
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Copy-WorkshopItem.ps1 -WorkshopId <id>
```

Read the verdict from `logs\build-<stamp>\build.log`, which ends in `SUMMARY PASS|FAIL|UNKNOWN mod=<path>`. Output goes to `build\@NIMBY-<stamp>\`. `build\`, `logs\`, `profiles\`, and `vendor\` are git-ignored.

## Architecture

### The @NIMBY pack (`mods/own/nimby-pack/`)

One signed client-and-server mod, published as one public Steam Workshop item, that carries every in-house addon. Each addon folder becomes one PBO.

- `pack.json` drives everything: addon rows, `includePatterns`, the `bootCheck` and `testServer` settings, and the `dev` harness. To add an addon, add a row; the build script needs no change. A row with `"serverOnly": true` builds unsigned to `build\@NIMBY-Server-<stamp>\` and loads with `-servermod=`. It never enters the Workshop output, and no client script may reference a class defined in it.
- An addon can start on `main` as a stub: a `config.cpp` with no scripts, so the pack builds with the row already in place while the real body lands from that addon's `feat/*` worktree. `NimbyEarplugs`, `NimbyNotes`, and `NimbyTack` were stubs until their `feat/*` merges on October 17, 2026; all five addons on `main` are real now.
- Addon code follows the DayZ script-module layers `3_game`, `4_world`, and `5_mission`, declared in each `config.cpp` under `CfgMods.defs`. Key bindings go in the addon's `Inputs.xml` so players can rebind them in Options > Controls.
- Two addons are forks of third-party mods: `NimbyAutorun` (SchanaModAutorun, Apache-2.0) and `NimbyHands` (Give&Take, Workshop 2903112334; its permission capture is `PERMISSION.md`). Each addon's `NOTICE` records its origin and every change made. Keep `NOTICE` current; attribution is a release requirement.

### Build pipeline (`scripts/Build-NimbyPack.ps1` and `scripts/modules/`)

1. `NimbyPbo.psm1` packs each addon into an uncompressed PBO. DayZ Tools Addon Builder is deliberately not used: its DayZ build ignores command-line arguments.
2. DayZ Tools `DSSignFile` signs each PBO with the `nimby` key.
3. Each PBO is verified: checksum, `.bisign` present, prefix matches `pack.json`, and every source file that matches `includePatterns` is inside.
4. `DSCheckSignatures` verifies the signatures. It is calibrated on every run against an unsigned control PBO; if the control is not flagged, the result is UNKNOWN.
5. The boot check starts the local DayZ Server on `dayzOffline.nasdara`, creates `$profile:<Prefix>\selftest` for each addon, and fails on any `bootCheck.failurePatterns` match in the RPT or script log. This includes `SCRIPT (E)`, `Scripted variables corrupted`, and `[NIMBY][<Prefix>][SelfTest] FAIL`.

`NimbySteam.psm1` finds DayZ Tools (Steam app 830640) and DayZ Server through Steam app manifests. `NimbyRcon.psm1` sends the graceful `#shutdown` that the test server uses before it stops any process.

Test persistence uses only `storage_998` and `storage_999`. Both scripts refuse any other instance ID, so `storage_1` is never written.

### Server configuration (`server/`)

- `serverDZ.cfg` is deployed by pasting it into the HostHavoc panel file `serverDZ_By_HostHavoc.cfg` while the server is stopped. Lines marked `[panel]` are HostHavoc defaults, kept so the server stays visible in the server browser.
- `mission-overrides/` holds per-mission changes, re-applied on every map change. `cfggameplay.patch.json` is a merge (set only the listed keys), not a file replacement. Its `README.md` records why each value was chosen.
- `live/` is a read-only copy of the live mission's loot and gameplay files, written by `Sync-LiveReplica.ps1`. Do not edit it; live `types.xml` is edited in the HostHavoc panel.

### Plans and shared rules (`docs/plans/`)

`2026-10-15-shared-rules.md` governs every addon plan. Read it before you touch addon code. The parts that bite most often:

- **Worktrees and branches.** Each addon is built on its own `feat/*` branch in `D:\GitHub\nimby-badlands-server-wt\<branch>`. A session may commit and push its own `feat/*` branch. Only Jeff merges to or pushes `main`.
- **Orchestrator-owned files.** `pack.json`, `Build-NimbyPack.ps1`, `scripts/modules/*.psm1`, `Start-NimbyTestServer.ps1`, and the shared-rules file itself. A `feat/*` session must not edit them.
- **Identifiers.** Every addon has a fixed code prefix, log prefix (`[NIMBY][<Name>]`), and RPC ID block, all listed in section 4. A display-name change never changes these. Every log line that names a player carries both `GetId()` and the Steam64 ID.
- **Save footprint.** Utility addons must not override `OnStoreSave` or `OnStoreLoad` or persist anything. Content addons (Puff, Sprout, Scrawl, Tack) may persist only classes they define themselves, with a version guard. Removing an addon must not change what loads from `storage_1`; this is the class of corruption that followed the October 15 CF removal.
- **API evidence.** Cite a file and line in the unpacked 1.30 scripts at `vendor/dayz-1.30-scripts/` for any vanilla API you use (build `1.30.164078.38`; see `docs/baselines/`). 1.30 has no `inputs.xml`; default bindings are in `bin/preset_X1MouseKey.xml`.
- **Release gate (9.2).** Boot the release artifact against a copy of live `storage_1` with `storageAutoFix = 0`. Compare the result to a same-build vanilla control (`-Vanilla`). Back up storage and every `$profile:Nimby*` folder together, then deploy at a scheduled restart.

Display names differ from code prefixes: Legs is `NimbyAutorun`, Hands is `NimbyHands` (log prefix `[NIMBY][Hands]`; renamed from the fork's October 14 name at Hands Phase 2, see its `NOTICE`), Ears is `NimbyEarplugs`, Scrawl is `NimbyNotes`, and Central Scrutinizer is `NimbyAdmin`.

### Other docs

- `docs/operations/`: restart and update schedule; map-switch runbook.
- `docs/design/`: feasibility studies.
- `docs/research/`: 1.30 and Badlands impact research.
- `docs/baselines/`: reference log counts that later boots are compared against.
- `docs/testing/`: in-game test records with findings IDs (`NIMBY-T-nnn`).
