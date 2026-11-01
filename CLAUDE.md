# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Configuration, in-house mods, PowerShell tooling, and runbooks for **NIMBY | HARDCORE | BADLANDS | 1pp**, a public DayZ 1.30 server on the Nasdara (Badlands) map, hosted at HostHavoc (Chicago). Not part of the AFS platform. Owner: Jeff Fabish. In-game admin: Jay (Trendkill).

The decision record is the Notion hub "DayZ Badlands Server - Project Hub". This repository holds the artifacts. `STATUS.md` (current state) and `NEXT.md` (ordered actions) were rewritten October 27, 2026, after release 1; when they disagree with a dated doc under `docs/`, the newer file wins.

## Hard rules

- No change reaches the live server without Jeff's approval. Back up first. Deploy only inside the restart chain (`docs/operations/restart-schedule.md`). Jeff types RCON passwords.
- Never commit a password, RCON password, Steam credential, or Discord token. `passwordAdmin` stays empty in `server/serverDZ.cfg`; the live value exists only in the HostHavoc panel file.
- The signing private key lives in `D:\Keys\nimby` and never enters the repository.
- Do not reopen these settled decisions: CF/COT removal, `lightingConfig` 3, 1PP, or the hardcore profile. Never write the name of the armed vehicle.
- PowerShell scripts must run in both Windows PowerShell 5.1 and PowerShell 7 (no `&&`), use `Set-StrictMode -Version Latest`, and write a log file. Each script's comment-based help lists what it Reads, Writes, and Never touches. Keep that block accurate when you change a script.
- Generic DayZ tooling that is not specific to this server belongs in `D:\GitHub\Dogs DayZ ToolZ`, not here.

## Commands

Pester covers the tooling; the boot check covers the addons. The Pester 5 suite in `scripts\tests\` tests the PowerShell that decides PASS, packs PBOs, speaks BERCon, resolves the join password, and guards the replica, under both Windows PowerShell 5.1 and PowerShell 7. The boot check starts a real local DayZ server with the built pack, scans the logs, and fails when any addon's self-test fails or stays silent.

The hosted static gate (`.github/workflows/static-gate.yml`, GitHub `windows-latest`) runs Pester under both shells and `scripts\Test-NimbyRepo.ps1` on every push to `main` and every pull request. It needs no Steam, DayZ, signing key, or secret, so it cannot boot the pack; the boot check stays local until the self-hosted runner exists.

```powershell
# Pester suite (Pester 5.9.1: Install-Module Pester -RequiredVersion 5.9.1 -Scope CurrentUser -Force -SkipPublisherCheck), in both shells
pwsh -NoProfile -Command "Invoke-Pester .\scripts\tests"
powershell.exe -NoProfile -Command "Import-Module Pester -RequiredVersion 5.9.1; Invoke-Pester .\scripts\tests"

# Static checks the CI gate runs: secrets, em dashes, double ampersands, CfgMods coverage and NOTICE, unsigned pack, CRLF
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Test-NimbyRepo.ps1

# Build, sign, verify, and boot-check @NIMBY (exit 0 = PASS, 1 = FAIL, 2 = UNKNOWN)
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build-NimbyPack.ps1

# Build and verify only, no server boot
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build-NimbyPack.ps1 -SkipBootCheck

# Qualification boot (storageAutoFix = 0) plus the dev harness (mods\own\dev\NimbyDevHarness, forged requests
# against the real Legs and Hands handlers; SELFTEST DevHarness in build.log) under the separate nimby_dev key.
# -DebugAddons NimbyHands also creates the Hands debug flag in the boot profile, so the step 2 lines are written
# (the harness's bot transfer case, NIMBY-T-063)
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build-NimbyPack.ps1 -Dev -Qualification
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build-NimbyPack.ps1 -ServerPath D:\DayZServer -BootInstanceId 998 -Dev -Qualification -DebugAddons NimbyHands

# Release artifact: only the named addons (pack.json untouched), booted against storage_998, the copy of
# live storage_1 that Sync-LiveReplica.ps1 makes (release gate decisions 1 and 2, October 26, 2026).
# -ServerPath D:\DayZServer is required here: without it the script finds the Steam server install first
# and boots that install's storage_998, not the replica's
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build-NimbyPack.ps1 -ServerPath D:\DayZServer -Addons NimbyAutorun,NimbyEarplugs,NimbyNotes,NimbyGreet -BootInstanceId 998 -Qualification

# Interactive in-game test server: newest SUMMARY PASS build, or a specific one
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Start-NimbyTestServer.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Start-NimbyTestServer.ps1 -BuildPath .\build\@NIMBY-<stamp> -ProfileName <name>
# A -BuildPath build that is not SUMMARY PASS, or whose PBOs differ from its build log, needs -Force
# -BuildPath, -ServerModPath, and -DevPath are resolved to absolute paths before -mod= is built, and a
# missing folder is refused by name. The server reads -mod= from its own install folder, not the shell's:
# on October 21, 2026 a relative -BuildPath loaded only the dev harness and crashed the server
# (NIMBY-T-017, study/hands-t010 record). A standing join password is never printed: the start-up line
# reads "Join password: standing value (not shown)" plus its source (NIMBY_JOIN_PASSWORD or
# scripts\testserver.local.json). Only a -JoinPassword, profile, or random value is shown.

# Against the live replica: 998 = copy of live storage_1, 999 = fresh world; -Vanilla = no mods (control boot)
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Start-NimbyTestServer.ps1 -ServerPath D:\DayZServer -InstanceId 998
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Start-NimbyTestServer.ps1 -ServerPath D:\DayZServer -InstanceId 998 -Vanilla

# Read a test-server run (newest logs\testserver-* by default, or a logs\build-<stamp> boot check): Hands state
# chains with timings, UNCONFIRMED and open ids, SCRIPT (E) and corrupted-variable hits, crash files, self-test
# lines, VERDICT PASS|FAIL in <run>\evaluation.log (exit 0 PASS, 1 FAIL, 2 UNKNOWN). Use it on every run.
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Read-NimbyTestRun.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Read-NimbyTestRun.ps1 -RunPath .\logs\testserver-<stamp>

# Refresh the local replica at D:\DayZServer from the WinSCP mirror of the live server (-WhatIf lists only)
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Sync-LiveReplica.ps1

# Pull live RPT, ADM, and log files over SFTP (needs Posh-SSH; Jeff types the password;
# host, port, and account come from scripts\testserver.local.json or the parameters)
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Get-ServerLogs.ps1

# Copy a subscribed Workshop item into vendor\workshop\<id>\<stamp>\ for reading
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Copy-WorkshopItem.ps1 -WorkshopId <id>
```

Script help: `Get-Help .\scripts\<Script>.ps1 -Full`. Under Windows PowerShell 5.1, `powershell.exe -File <script> -?` prints nothing; use `powershell.exe -Command "& .\scripts\<Script>.ps1 -?"`.

Per-machine values live in the git-ignored `scripts\testserver.local.json` (matched by `*.local.json`): `joinPassword` (the standing test-server join password; the `NIMBY_JOIN_PASSWORD` environment variable wins over it), and `sftpHost`, `sftpPort`, `sftpUser`, `sftpProfileDir` for `Get-ServerLogs.ps1`. No script holds these as literals.

Read the verdict from `logs\build-<stamp>\build.log`, which ends in `SUMMARY PASS|FAIL|UNKNOWN mod=<path>`. Output goes to `build\@NIMBY-<stamp>\`. `build\`, `logs\`, `profiles\`, and `vendor\` are git-ignored.

## Architecture

### The @NIMBY pack (`mods/own/nimby-pack/`)

One signed client-and-server mod, published as one public Steam Workshop item, that carries every in-house addon. Each addon folder becomes one PBO.

- `pack.json` drives everything: addon rows, `includePatterns`, the `bootCheck` and `testServer` settings, and the `dev` harness. To add an addon, add a row; the build script needs no change. A row with `"serverOnly": true` builds unsigned to `build\@NIMBY-Server-<stamp>\` and loads with `-servermod=`. It never enters the Workshop output, and no client script may reference a class defined in it.
- The dev harness `mods/own/dev/NimbyDevHarness/` (the `dev` section of `pack.json`, built only with `-Dev`, signed with the separate `nimby_dev` key, never an `addons` row) runs forged requests against the real Legs and Hands server handlers: boot cases at mission start when `$profile:NimbyDevHarness\selftest` exists, and online cases ten seconds after a player connects when `$profile:NimbyDevHarness\forge` contains `online`. The word `step2-refuse` in that file arms one forced Hands step-2 refusal. The client output is byte-identical with and without `-Dev`; no client PBO may reference a harness class.
- An addon can start on `main` as a stub: a `config.cpp` with no scripts, so the pack builds with the row already in place while the real body lands from that addon's `feat/*` worktree. `NimbyEarplugs`, `NimbyNotes`, and `NimbyTack` were stubs until their `feat/*` merges on October 17, 2026; all five addons on `main` are real now.
- Addon code follows the DayZ script-module layers `3_game`, `4_world`, and `5_mission`, declared in each `config.cpp` under `CfgMods.defs`. Key bindings go in the addon's `Inputs.xml` so players can rebind them in Options > Controls.
- Two addons are forks of third-party mods: `NimbyAutorun` (SchanaModAutorun, Apache-2.0) and `NimbyHands` (Give&Take, Workshop 2903112334; its permission capture is `PERMISSION.md`). Each addon's `NOTICE` records its origin and every change made. Keep `NOTICE` current; attribution is a release requirement.

### Build pipeline (`scripts/Build-NimbyPack.ps1` and `scripts/modules/`)

1. `NimbyPbo.psm1` packs each addon into an uncompressed PBO. DayZ Tools Addon Builder is deliberately not used: its DayZ build ignores command-line arguments.
2. DayZ Tools `DSSignFile` signs each PBO with the `nimby` key.
3. Each PBO is verified: checksum, `.bisign` present, prefix matches `pack.json`, and every source file that matches `includePatterns` is inside.
4. `DSCheckSignatures` verifies the signatures. It is calibrated on every run against an unsigned control PBO; if the control is not flagged, the result is UNKNOWN.
5. The boot check starts the local DayZ Server on `dayzOffline.nasdara`, creates `$profile:<Prefix>\selftest` for each addon, and fails on any `bootCheck.failurePatterns` match in the RPT or script log. This includes `SCRIPT (E)`, `Scripted variables corrupted`, and `[NIMBY][<logPrefix>][SelfTest] FAIL`.
6. The self-test gate maps each addon's code prefix to its `pack.json` `logPrefix` and reads the script log once (the RPT repeats every line). Every flagged addon must print `[NIMBY][<logPrefix>] selftest complete; cases=N failures=M` with N above zero and PASS plus FAIL lines equal to N. A silent addon is `BOOT FAIL self-test gate: <logPrefix> ran=no`. `build.log` gets `SELFTEST <logPrefix> ran=yes|no cases=N pass=N fail=M` per addon and `SELFTEST TOTAL` (250 unique cases on October 21, 2026).

`NimbySteam.psm1` finds DayZ Tools (Steam app 830640) and DayZ Server through Steam app manifests, then `D:\DayZServer`. `NimbyRcon.psm1` sends the graceful `#shutdown` that the test server uses before it stops any process. `NimbyCommon.psm1` holds the helpers every script shares: the manifest writer, the instance guard, the server argument list, the log reader, the running-server guard (no script starts a DayZ server while one runs), the random generator, and the local settings file. `NimbyPbo.psm1` stamps each entry with its source file's last-write time, so unchanged sources give the same PBO hash from build to build.

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
