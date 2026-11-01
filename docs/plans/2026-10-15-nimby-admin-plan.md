# Plan 3 - Central Scrutinizer (`NimbyAdmin`)

Draft v0.10, November 1, 2026 (v0.9 the same day; v0.8 and v0.7 October 31). Owner: Jeff Fabish. Implementer: a Claude Code session in worktree `D:\GitHub\nimby-badlands-server-wt\nimby-admin` (branch `feat/nimby-admin`, cut from `feat/nimby-shared` once Plan 11 Phase 1 passes, rebased onto `main` after Plan 11 merges), coordinated by the orchestrator; a Codex session reviews the security contract read-only at the Phase 0 gate (N18). Status: Phase 0 Stop B (the vertical slice) in progress on `feat/nimby-admin` after the recon was accepted on October 31, 2026. Plan 0 (shared tooling) has been on `main` since October 15, 2026.

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30.164200 / Nasdara (release 1 was qualified on 1.30.164078; the live binary moved to 164200 on October 22, 2026).
Release: independent, gated (see [shared rules, 9.2](2026-10-15-shared-rules.md)).

Central Scrutinizer is the in-house admin tool for every server admin (today Jeff and Jay; roles per N3): player management, spawning, and world control from an in-game menu, every action audited locally and posted to Discord. It is written from scratch against vanilla DayZ 1.30 with no CF dependency and zero save footprint. The feasibility study (`docs/design/nimby-admin-feasibility.md`) holds the reasoning and cost; where the two differ, this plan is authoritative. The October 16, 2026 addendum (`docs/design/central-scrutinizer-feasibility-addendum.md`, v0.2) assesses player intelligence, the companion service, the map, spectate, and in-game configuration; Jeff ruled its decisions Q1-Q6 the same day (hub decisions D32-D37): the companion is Eyes, its own mod-board row, and nothing from the addendum is in this plan beyond the N9 seams and Phase 5.

Changes since v0.9 (Phase 0 gate session, November 1, 2026, orchestrator ruling): the sandstorm visual (Phase 0 item 3 proof 4) is not verifiable in Phase 0, because 164200 `SandstormController` has no position setter (`3_Game/DayZ/Sandstorm.c:20-83`) and a scripted storm starts at the map edge, about 15 km from the players; it moves to Phase 3 as a Tweaker-study item (World controls). `Start`, `Stop` and `IsActive` are proved on a dedicated server.

Changes since v0.8 (Phase 0 Codex re-review finding 13, ruled by the orchestrator November 1, 2026): the audit journal is two files per UTC day. Denied and invalid records and the `audit.suppressed` summaries go to `audit-denied-<YYYY-MM-DD>.log`; the intent and result records of executed commands stay in `audit-<YYYY-MM-DD>.log`, the only file the intent readback reads, so traffic a non-admin can drive never lengthens it (Audit record section).

Changes since v0.7 (Phase 0 recon `2026-10-31-nimby-admin-phase0-recon.md`, commit `7583397`, accepted by the orchestrator October 31, 2026, with Jeff's answers to its eight questions): the six recon plan lines applied, each marked "accepted by the orchestrator, October 31, 2026": gate row 1 evidence (session hello), the two-client gate session, one audit record for a denied or invalid request, ban enforcement at connection moved to Phase 1, Discord backoff on any error class (recon R6), and the 164200 reference. New `config.json` field `permissionsReloadSeconds` (recon question 5). The request `args` use the seat's `key=value` codec, never client JSON (question 6, recon R8). No decision from v0.1-v0.7 is reopened.

Changes since v0.6 (Central Scrutinizer kickoff, October 31, 2026; Jeff's rulings recorded in N16-N18): the menu is built on the shared `NimbySharedWindow` shell and the NimbyShared chunk codec (Plan 11, `2026-10-31-nimby-shared-plan.md`), extracted from Greet first; the NimbyKit brief is absorbed as `spawn.kit` in Phase 2 (N17); the seats are named (N18). Phase 0 is rewritten against what Greet and release 1 proved between October 21 and 26, 2026 (chunked RPC, profile JSON, signed `.layout` on a real client, the `menu` input exclude and zeroing guard, the single-place hotkey read, `Data/Inputs.xml` rebinding, the server-only build output, and the HostHavoc `-servermod=` field), and its time box drops from five days to three. `inputs.xml` references now read `bin/preset_X1MouseKey.xml` (shared rules 7). The Players page refresh and the item catalog are named as chunk-codec payloads. No decision from v0.1-v0.6 is reopened.

Changes since v0.5 (Jeff's rulings, October 21, 2026, evening): persistent-loot density added (N15): a server-side tile counter, a Density page, and per-tile Discord alerts in Phase 5, with per-player alerts once the F15 journal has data; the heatmap overlay joins Phase 6. Sandstorm control promoted into Phase 3 now that `SandstormController.Start`/`Stop` are verified (`3_Game/DayZ/Sandstorm.c:20,27`, Tweaker study T1a). New `config.json` fields `densityTileMeters`, `densityAlertObjects`, `densityPlayerAlertObjects`, `densityRefreshSeconds`.

Changes since v0.4 (Jeff's rulings, October 21, 2026, after the gap analysis against COT and VPP): eight capabilities added. Per-stat editor, cure, and restraint join Phase 1; weather presets join Phase 3; vehicle manager, object tools, and admin notes form a new Phase 5 after the first live release (N10). Restraint mechanism (N11), object-tool scope (N12), and the notes store (N13) decided. The capabilities surfaced by the same analysis and not adopted are recorded under Later (not committed) (N14). New `config.json` field `restrainTimeoutMinutes`; new `notes.json` data format.

Changes since v0.3: display name Central Scrutinizer (Jeff, October 16, 2026; was Golden God); the tool serves every admin, not one person; observation features (live player map, spectate) stay out of scope but the architecture carries the seams for them (N9); key bindings are player-configurable per shared rules 5.

Changes since v0.2: display name Golden God (N1); client and server PBO split (N7); ban and permission files fail to last-known-good, not closed (N8); spawn cap 50 items and one vehicle (N6); decisions and data-format tables restored; Discord retry downgraded to fixed backoff on `429`; audit-failure tests scoped to what runs locally; every test row names its evidence; Phase 0 time box restored at 5 days.

## What this is not

- Not a copy of COT or VPP. No code is taken from either.
- Not an observation tool yet. No spectate, free camera, or live player map ships in this plan, and ESP never does. Jeff decided on October 16, 2026 that a map-based player view and spectating come later, so the architecture carries the seams for them (N9) without implementing them.
- Not a framework and not an anticheat.
- Not arbitrary script execution.
- Not a persistence change: no custom entity serialization, no mod-dependent persistent entity classes, no `OnStoreSave`/`OnStoreLoad`.

## Decisions

| # | Decision | Choice | Status |
| --- | --- | --- | --- |
| N1 | Display name | Central Scrutinizer. Code prefix `NimbyAdmin`, log prefix `[NIMBY][Admin]`, RPC block `65503xx` unchanged | Decided by Jeff, October 16, 2026 (Golden God on October 15) |
| N2 | Menu key | Home, then F10; the first verified free in the 1.30 `bin/preset_X1MouseKey.xml` wins (there is no `inputs.xml` in 1.30, shared rules 7); otherwise unbound with setup instructions. End left the list on October 15, 2026 when Jeff gave it to Ears (D8); Home is free and F10 is bound in vanilla, so Home is expected | Decided; revised October 15, 2026 |
| N3 | Roles | `owner` (manage permissions and everything else) and `admin` (operational commands) | Decided |
| N4 | Bans | Own list in the profile folder, checked against the authenticated identity at connection | Decided |
| N5 | Discord channel | New private admin channel; webhook URL only in `$profile:NimbyAdmin\config.json` | Decided |
| N6 | Spawn limits | 50 items and one vehicle per request, plus server-side request limits; all values in `config.json` | Decided October 15, 2026 |
| N7 | PBO layout | Client PBO `NimbyAdmin` (menu, request sender, admin-state receiver) plus server-only PBO `NimbyAdminServer` loaded with `-servermod=` (router, permissions, bans, audit, webhook, handlers). See shared rules 8.1 | Decided October 15, 2026 |
| N9 | Observation seams | Built now, used later: (a) the server-to-client channel is a typed push envelope (`NimbyAdminPush`: kind, sequence, payload), with `role` the only kind in this plan, so a position feed or spectate stream is a new kind, not a new channel; (b) permissions are capability strings on roles (`player.kick`, `spawn.item`, `world.weather`, and reserved `observe.map`, `observe.spectate`), so observation gets its own capabilities with no schema change; (c) the client menu is a page registry (`NimbyAdminPage` subclasses register at init); a Map or Spectate page registers like any other; (d) RPC ids `6550350`-`6550399` are reserved for observation feeds; (e) a streaming handler variant (server-paced, rate-limited, per-subscriber) is specified when the first feed is built, not now. The self-test registers one fake page, one fake push kind, and one fake capability to prove the seams exist | Decided by Jeff, October 16, 2026 (build for later map and spectator views); seam design is Claude's |
| N8 | Access-file failure | Permissions and bans are written atomically with a `.lkg` copy. On a corrupt load the server uses `.lkg`, logs, posts to Discord, and refuses further edits to that file until repaired. Admission is denied only when no `.lkg` exists | Decided October 15, 2026 |
| N10 | Placement of the October 21 additions | Player verbs (per-stat editor, cure, restrain and release) ship in release 1 inside Phase 1; weather presets inside Phase 3; vehicle manager, object tools, and admin notes form Phase 5, which starts only after the first live release. Release 1 slips about a week; Phase 5 never blocks it | Decided by Jeff, October 21, 2026 |
| N11 | Restraint mechanism | Both: the vanilla restrained state (`PlayerBase.SetRestrained`, `PlayerBase.c:2206`; handcuffed animation, visible to bystanders) and an invisible freeze (`HumanInputController.OverrideMovementSpeed`, `human.c:253`). The admin picks per use. Either form MAY send the target a toast saying an admin has restrained them; the admin chooses whether to send it. Both end on `restrainTimeoutMinutes` (default 10) or explicit release; apply and release are audited; the owner is notified in Discord on every hold, as for spectate (addendum Q5) | Decided by Jeff, October 21, 2026 |
| N12 | Object-tool scope | Dynamic objects only: ground items, vehicles, base-building parts, tents, barrels, fences, flags, and dead bodies (anything with a persistent id or an inventory). Map statics (`House`, `Building`, terrain objects) are selectable for inspection and never deleted or moved, because the change does not persist and desyncs later joiners | Decided by Jeff, October 21, 2026 |
| N13 | Admin notes store | `$profile:NimbyAdmin\notes.json` from Phase 5, same atomic write and `.lkg` as N8; Eyes imports the file when it ships and becomes the viewer, and the server keeps writing locally so an Eyes outage never loses a note | Decided by Jeff, October 21, 2026 |
| N14 | Capabilities not adopted | Recorded under Later (not committed) with their one-line verdicts; nothing scheduled or estimated | Decided by Jeff, October 21, 2026 |
| N15 | Persistent-loot density | Purpose: answer "why does this area have low performance" and catch a player or area hoarding. Phase 5: the registry counts persistent objects per map tile (`densityTileMeters`, default 200) by class group (base parts, tents, containers and barrels, underground stashes, loose ground items, vehicles; infected and animals excluded), a Density page lists the top tiles with counts and a best-guess owner, and a Discord alert fires when a tile passes `densityAlertObjects` (default 400), once per tile per restart. Per-player alerts (`densityPlayerAlertObjects`) use the F15 association journal and switch on when it has data; until then the owner column is marked best-guess. Phase 6 paints the tiles as a heat overlay on the map. Thresholds are starting values, tuned on live data | Decided by Jeff, October 21, 2026 (placement and alert unit); counting rules and defaults are Claude's |
| N16 | Shell and codec | The menu is a `NimbySharedWindow` subclass, and every server-to-client payload above one RPC string (the Players refresh, the item catalog, the audit tail) goes through the NimbyShared chunk codec (Plan 11, C5 and C7). This addon holds no copy of either and never references a `NimbyGreet` class | Decided by Jeff, October 31, 2026 (a shared addon over a copied shell or a Greet dependency) |
| N17 | Test kit | NimbyKit (`docs/plans/2026-10-26-testkit-brief.md`) is not built as its own addon. Phase 2 gains `spawn.kit`: one named item list from `$profile:NimbyAdmin\kits.json` given to one online player, audited and bounded like `spawn.item`. The automatic on-spawn kit is recorded under Later (not committed) | Decided by Jeff, October 31, 2026 |
| N18 | Seats | One Claude Code session in the worktree implements, and runs `Build-NimbyPack.ps1` and the replica server itself; Jeff runs the in-game client steps and pastes the lines; a Codex session reviews the Phase 0 security contract (router, permissions store, ban store, audit journal) read-only at the gate, and every finding is addressed or ruled before the go | Decided by Jeff, October 31, 2026 |

## Architecture

```mermaid
flowchart LR
    Menu[Client menu<br/>NimbyAdmin.pbo] -- "RPC 65503xx: protocol, request id, command, args" --> Router[Server router<br/>NimbyAdminServer.pbo]
    Router -- "Steam64 from connection" --> Perms[Permissions store]
    Perms -- "role or none" --> Router
    Router -- "authorized, deduped, rate-limited" --> Handlers[Command handlers]
    Handlers -- "vanilla API calls" --> World[Players, items, vehicles, weather]
    Handlers -- "intent and result records" --> Audit[Audit journal]
    Audit -- "JSON line" --> File["$profile:NimbyAdmin\audit\audit-&lt;date&gt;.log<br/>and audit-denied-&lt;date&gt;.log"]
    Audit -- "queued, bounded" --> Webhook[Discord webhook]
    Router -- "result or denial to sender only" --> Menu
    Sync[Admin state sync] -- "role at connect and on change" --> Menu
```

| Component | PBO | Responsibility |
| --- | --- | --- |
| Menu | Client | A `NimbySharedWindow` subclass (Plan 11, C5): page registry, layouts, and widgets; sends requests; never decides anything; hidden unless the admin state sync says the player has a role; its pause-menu entry registers through `NimbySharedPauseButtons` and shows only with a role |
| Chunk delivery | Both | `NimbySharedChunkSender` on the server and `NimbySharedChunkReceiver` on the client (Plan 11, C7) carry any payload above one RPC string; the chunk and done RPC ids come from the `65503xx` block |
| Admin state sync | Both | Server tells a client its role at connect and on change through the `NimbyAdminPush` envelope (N9); the client only renders |
| Command router | Server | Reads protocol version, request id, command, args; derives the actor from the connection; authorizes; dedupes; rate-limits; dispatches; returns the result to the sender only |
| Permissions store | Server | `$profile:NimbyAdmin\permissions.json` with `.lkg`; first-owner bootstrap |
| Ban store | Server | `$profile:NimbyAdmin\bans.json` with `.lkg`; checked at connection |
| Command handlers | Server | One class per command; validates arguments through engine-free validators; calls vanilla APIs |
| Audit journal | Server | Appends intent and terminal result records; a failed append blocks the state change |
| Discord delivery | Server | Bounded queue, fixed backoff on `429`, never blocks a command |
| Item catalog | Both | Built from game config at runtime; the server validates every class independently before spawning |

## Data formats

`permissions.json`:

| Field | Type | Required | Description |
| --- | --- | --- | --- |
| `version` | `integer` | Yes | Schema version, starting at 1 |
| `admins` | array of `Admin` | Yes | Empty on first boot; the first owner comes from `config.json` `bootstrapOwnerSteam64` |

`Admin`:

| Field | Type | Required | Description |
| --- | --- | --- | --- |
| `steam64` | `string` | Yes | 17-digit Steam64 id; validated on load; an invalid row is skipped and logged, never granted |
| `name` | `string` | No | Label for logs only; never used for authorization |
| `role` | `string` | Yes | `owner` or `admin`; any other value grants nothing |

`bans.json` entry:

| Field | Type | Required | Description |
| --- | --- | --- | --- |
| `steam64` | `string` | Yes | Banned identity |
| `reason` | `string` | Yes | Escaped free text, maximum 200 characters |
| `actor` | `string` | Yes | Steam64 of the admin who created the ban |
| `createdUtc` | `string` | Yes | ISO 8601 UTC |
| `expiresUtc` | `string` | No | ISO 8601 UTC; absent means permanent |

`config.json`:

| Field | Type | Required | Default | Description |
| --- | --- | --- | --- | --- |
| `version` | `integer` | Yes | 1 | Schema version |
| `bootstrapOwnerSteam64` | `string` | No | none | Granted `owner` on boot when `permissions.json` has no owner |
| `webhookUrl` | `string` | No | none | Discord webhook; never logged or synced to clients |
| `spawnMaxItems` | `integer` | No | 50 | Items per request (N6) |
| `spawnMaxVehicles` | `integer` | No | 1 | Vehicles per request (N6) |
| `requestsPerSecond` | `integer` | No | 5 | Per-sender request limit |
| `permissionsReloadSeconds` | `integer` | No | 5 | A request re-reads `permissions.json` when the cached copy is older than this, so a hand edit reaches the next request; bounds 1 to 300, out of range takes the default (recon question 5, ruled October 31, 2026) |
| `discordQueueMax` | `integer` | No | 200 | Queued messages before overflow is reported |
| `auditRetentionDays` | `integer` | No | 90 | Audit files older than this are rotated out |
| `restrainTimeoutMinutes` | `integer` | No | 10 | A hold (N11) ends on its own after this many minutes |
| `vehicleRegistryRefreshSeconds` | `integer` | No | 60 | Cadence of the vehicle and object registry sweep (Phase 5) |
| `densityTileMeters` | `integer` | No | 200 | Edge of a density tile (N15) |
| `densityRefreshSeconds` | `integer` | No | 300 | Cadence of the density recount; the sweep is spread over ticks |
| `densityAlertObjects` | `integer` | No | 400 | Persistent objects in one tile that trigger a Discord alert |
| `densityPlayerAlertObjects` | `integer` | No | 400 | Objects attributed to one player that trigger an alert, once F15 has data |

`notes.json` (N13) entry:

| Field | Type | Required | Description |
| --- | --- | --- | --- |
| `id` | `string` | Yes | Unique per note; the request id that created it |
| `steam64` | `string` | Yes | The player the note is about |
| `author` | `string` | Yes | Steam64 of the admin who wrote it |
| `createdUtc` | `string` | Yes | ISO 8601 UTC |
| `text` | `string` | Yes | Escaped free text, maximum 500 characters |

Audit record (one JSON object per line; two per command that reaches its handler, `intent` then `result`; a denied or invalid request writes one `result` record and no `intent`, because nothing changed - accepted by the orchestrator, October 31, 2026, recon plan line 3).

Audit files (v0.9, re-review finding 13, November 1, 2026): two files per UTC day under `$profile:NimbyAdmin\audit\`, the same JSON shape in both.

| File | Holds | Read back |
| --- | --- | --- |
| `audit-<YYYY-MM-DD>.log` | The `intent` and `result` records of every command that reached its handler, and the system records of the bootstrap owner write | Each `intent` record, before the command runs; the readback reads only this file |
| `audit-denied-<YYYY-MM-DD>.log` | The one `result` record of a denied or invalid request (capped per sender and in aggregate, owners and admins always recorded), and the `audit.suppressed` summaries of what the caps held back | Never; nothing waits on it |

An `intent` record always goes to the first file. A command's audit trail is in the first file; who was refused, and why, is in the second.

Record fields:

| Field | Type | Required | Description |
| --- | --- | --- | --- |
| `id` | `string` | Yes | Request id from the client, unique per actor |
| `phase` | `string` | Yes | `intent` or `result` |
| `ts` | `string` | Yes | ISO 8601 with explicit offset |
| `actor` | `object` | Yes | `steam64`, `biId`, `name` |
| `command` | `string` | Yes | For example `player.teleportTo`, `spawn.item` |
| `target` | `object` | No | Player identity or item class, with positions where relevant |
| `args` | `object` | No | Validated arguments |
| `result` | `string` | Yes on `result` | `ok`, `partial`, `denied`, `invalid`, or `failed` |
| `effects` | `object` | No | Actual created quantities, entities removed, values applied |
| `reason` | `string` | No | Failure or denial reason |

## Phase 0 - feasibility and security gate (3-day time box)

Depends on Plan 0 (shared tooling, on `main` since October 15, 2026) and on Plan 11 Phase 1 (the shell and the codec on `feat/nimby-shared`).

### Build and environment

1. Record engine versions (1.30.164200), source revision, and the shared 1.30 reference hashes in `docs/baselines/`. The reference is 164200 as of October 31, 2026 (`vendor/dayz-1.30-scripts/`, `scripts.pbo` SHA-256 `FF0AD7EE...`; the 164078 extract kept as `vendor/dayz-1.30-164078-scripts/`) - accepted by the orchestrator, October 31, 2026, recon plan line 6.
2. Cite, do not re-prove: RPC in both directions with `Param3<int, int, string>` and chunked payloads above 1,000 bytes (Greet G5, G-P0-31, NIMBY-T-037); profile JSON load and save through `JsonFileLoader` (Greet G17); a `.layout` loaded from a signed client PBO on a real client (Greet Phase 1); menu open and close with the `menu` input exclude and the zeroing guard (NIMBY-T-039, NIMBY-T-058); the single-place hotkey read (NIMBY-T-046); `Data/Inputs.xml` rebinding (Ears EP-005, Greet G7); the 5_Mission `CallLater` ban (shared rules 7.1); the server-only build output (`Build-NimbyPack.ps1` `serverOnly`, `Start-NimbyTestServer.ps1 -ServerModPath`). The record names the row or finding for each.
3. Verify in game on the replica and record file:line: `PlayerIdentity.GetPlainId()` returns the 17-digit Steam64 on a dedicated server (shared rules 4.2); `RestApi`/`RestContext`/`RestCallback` POST from the server to a throwaway webhook; player position set and the teleport surface check; `SandstormController.Start` and `Stop` take effect on a dedicated server (Phase 3 depends on it); the N2 key `Home` is free in `bin/preset_X1MouseKey.xml` (there is no `inputs.xml` in 1.30, shared rules 7).
4. Orchestrator: `pack.json` carries the `NimbyAdmin` client row and the `NimbyAdminServer` row with `"serverOnly": true`; `*.layout` is already in `includePatterns`; `NimbyShared` is already the first row.
5. HostHavoc: the Commandline Manager already exposes `-servermod=` (the live line is `"-servermod=" "-mod=@NIMBY"`). Still to verify before any server-only code depends on it: a `@NIMBY-Server` folder uploaded over SFTP to the server root loads from that field at a restart window, and outbound HTTPS to `discord.com` works from the host (the same throwaway webhook as item 3, one post, then the webhook is deleted). Both are Jeff's steps on the live panel; the seat prepares the folder and the exact lines.
6. Build, sign, and boot both PBOs on Nasdara on the replica; the menu opens on a real client from the signed client PBO on the `NimbySharedWindow` shell, and a player without a role sees nothing.

### Minimum vertical slice

Menu shell with player selection; authenticated router; permission loading and first-owner bootstrap; audit journal; asynchronous Discord delivery; one command: teleport to player. The ban store's loader, validator, `.lkg`, and self-test ship in Phase 0 so the N18 review covers them; enforcement at connection is Phase 1 (accepted by the orchestrator, October 31, 2026, recon plan line 4).

### Security contract

Every request carries a supported protocol version, a request id, a known command, and bounded arguments. The server:

- derives the actor from the authenticated connection and resolves identity by Steam64, never from a client-supplied id or name;
- authorizes every command and sensitive query;
- resolves targets against current sessions, so a selection does not silently retarget after reconnect;
- rejects stale, malformed, unknown, unauthorized, and duplicate requests;
- applies per-sender and aggregate limits before expensive processing;
- returns results only to the sender.

Permission updates take effect on the next request. Unknown roles and invalid entries grant nothing. A malformed file never replaces a working configuration with an empty one (N8).

### Gate tests

The gate session is a two-client session (Jeff plus Jay or a second client): the duplicate, forged-teleport, and admin-teleport rows need a second connected player (accepted by the orchestrator, October 31, 2026, recon plan line 2).

| Scenario | Required result | Evidence |
| --- | --- | --- |
| Non-admin presses the menu key | Nothing opens; no request sent | No `[NIMBY][Admin]` request line from a Home press (every client sends `session.hello` at spawn; accepted by the orchestrator, October 31, 2026, recon plan line 1) |
| Non-admin forges a teleport (dev harness) | Denied | Audit `result=denied`; no position change |
| Admin teleports to a player | Moves | Audit `intent` then `result=ok` with the same `id`; one Discord message |
| Duplicate request id | Second ignored | One `intent`, one `result`; `[NIMBY][Admin] duplicate id=` line |
| Permission revoked mid-session | Next request denied | `result=denied reason=no role` |
| Malformed payload | Rejected | `result=invalid` |
| Disconnect and reconnect | Role re-synced; menu state correct | Admin state sync line at connect |
| Restart | Permissions and config persist; no entity data written | Files unchanged; `storage_1` copy boot clean |
| Audit file locked or read-only (local) | Command blocked | `result=failed reason=audit unavailable`; no state change |
| Webhook wrong or Discord down | Command succeeds | `result=ok`; one `[NIMBY][Admin] discord failed` line, not one per retry |
| Self-test | Validators pass | `[NIMBY][Admin][SelfTest] PASS` for every case |
| Security contract review (N18) | The Codex read-only review of the router, permissions store, ban store, and audit journal leaves no unaddressed finding | Review verdict and the disposition of each finding recorded in `docs/testing/<date>-central-scrutinizer-phase0.md` |

Go/no-go: continue only when the slice passes and remaining uncertainties are written down. If the time box is exceeded or the menu or RPC plumbing proves unworkable, stop; Jay keeps the HostHavoc panel. Reintroducing CF/COT needs its own compatibility decision and persistence test.

## Phase 1 - permissions, player management, and recovery

### Permissions

- Owner-managed role changes from the menu; the last owner cannot be removed or demoted.
- Validate the whole proposed configuration before replacing the active one; write atomically with `.lkg` (N8).
- Audit permission changes and denied attempts.
- Document offline recovery: which file to restore, from where, and how to verify.

### Player commands

Player list, teleport to, bring, heal, cure, per-stat editor, kill, kick, ban, unban, private message, unstick, restrain and release.

- Teleport and bring reject unsafe player states and invalid destinations; avoid occupied geometry, unsuitable terrain, and placement near vehicles.
- Kill and ban require explicit confirmation in the menu.
- Heal restores health, blood, and shock, stops bleeding, and repairs fractures. Disease, hunger, thirst, temperature, and equipment stay separate.
- Cure is its own verb: clears every agent (`PlayerAgentPool.RemoveAllAgents`, `PlayerAgentPool.c:218`) and resets disease-related modifiers; it touches no other stat.
- Per-stat editor: Health, Blood, Shock, Energy, Water, Heat comfort, and Stamina, each set to an explicit value bounded to the vanilla range for that stat (`SetHealth`, `GetStatEnergy`/`GetStatWater`/`GetStatHeatComfort`, `StaminaHandler.SetStamina` at `StaminaHandler.c:1075`). Lowering a stat ("hurt") is the same command with a lower value; a value that would kill requires the same confirmation as kill. Stamina is not persisted and the edit is advisory until the next stamina tick.
- Unstick: teleport with a 1 m lift, a 2 m forward nudge, and a surface check (addendum F16).
- Restrain and release per N11. The server re-applies the hold if the target struggles free; a disconnect ends the hold; the timeout is enforced server side and does not depend on the admin staying online.
- Ban enforcement at connection (`CGame.DisconnectPlayer`, `3_Game/DayZ/Global/Game.c:399`; vanilla call `5_Mission/DayZ/mission/missionServer.c:721`) lands here with its two-client test; the store itself shipped in Phase 0 (accepted by the orchestrator, October 31, 2026, recon plan line 4).
- A ban on an online player also ends that session. Ban and permission changes persist before success is reported.
- Removing or disabling Central Scrutinizer removes connection-time ban enforcement; the ops doc says so.

Exit: every command passes authorized, unauthorized, stale-target, and restart tests with Jeff and Jay, each with an audit `intent` and `result` pair.

## Phase 2 - controlled spawning

- Build the catalog from game configuration; the server validates each class independently. Not every config entry is safe to spawn.
- Destinations: inventory, feet, or validated nearby ground.
- Limits per N6; the limit is a maximum, not a promise. Report actual created quantities and partial failures as `result=partial` with `effects`.
- Never silently move a failed inventory spawn to the ground.
- Track entities created by the current request so incomplete construction is cleaned up before success is reported. A crash mid-request leaves an unresolved `intent`; do not claim exactly-once.
- Vehicle presets live in `$profile:NimbyAdmin\vehicles.json` for explicitly supported classes; validate parts, fluids, placement, and operability individually. Reject unvalidated presets.
- Kits (N17): `$profile:NimbyAdmin\kits.json` holds named item lists (`name`, `items[]` of `class` and `quantity`), validated on load like every other file (an invalid row is skipped and logged). `spawn.kit` gives one list to one online player through the same catalog validation, destination rules, N6 limits, and `result=partial` reporting as `spawn.item`. The first list is the one the NimbyKit brief proposed (`Hammer` 1, `Nail` 20, `Paper` 10, `Pen_Black` 1, `DuctTape` 1, `EpoxyPutty` 1, `KitchenKnife` 1, `Wrench` 1, `Whetstone` 1), subject to Jeff's edit of the file.

| Scenario | Required result | Evidence |
| --- | --- | --- |
| Spawn 50 items to inventory with room | All created | `result=ok effects.created=50` |
| Spawn with partial room | Some created, rest reported | `result=partial effects.created=<n>`; nothing on the ground |
| Invalid class | Refused | `result=invalid` |
| Duplicate request id | Second ignored | One `result` |
| Vehicle preset for each supported class | Drivable after spawn and after restart | Manual drive; audit `result=ok`; vehicle present after RCON `#shutdown` restart |
| Unvalidated vehicle preset | Refused | `result=invalid reason=preset not validated` |
| `spawn.kit` with the default list | Nine classes in the target's inventory | `result=ok effects.created=<n>`; one audit pair |
| `kits.json` with an unknown class | List skipped and logged, other lists usable | `[NIMBY][Admin] kits.json: list <name> skipped` line |

## Phase 3 - world controls and audit delivery

### World controls

Sandstorm visual (v0.10, carried from Phase 0 item 3 proof 4): a Tweaker-study item. Find out where and how a scripted sandstorm becomes visible on Nasdara (start position, travel time, intensity at a player via `GetIntensity`), with a player placed in its path, before the menu offers sandstorm control; `Start`, `Stop` and `IsActive` already work on a dedicated server.

Overcast, rain, fog, wind, time of day, and server-wide announcements. Named presets are saved in `$profile:NimbyAdmin\weather.json` (name, the five values, duration) and applied as one override; the file is validated on load and an invalid preset is skipped and logged, never applied. Bound every value and validate finite numeric input. Weather overrides last for the requested duration, then return control to the normal system; a newer override replaces the previous one; overrides do not persist across restart in v1. Time is set immediately and then progresses by server settings. Sandstorm: start and stop with a duration through `SandstormController.Start(duration, calledByWeather)` and `Stop(duration, calledByWeather)` (`3_Game/DayZ/Sandstorm.c:20,27`, verified October 21, 2026 in the Tweaker study); the controller's position, direction, and intensity are read-only to the menu. Phase 0 of this plan confirms `Start` takes effect on a dedicated server, since the script declaration is not proof of behavior (shared rules 7).

### Local audit

Records per the audit table above. Test locked files, rotation, and recovery locally; HostHavoc disk exhaustion is recorded as untestable. Keep denial logging rate-limited so a hostile client cannot exhaust storage.

### Discord

- Webhook URL only in `config.json`; never in logs, client sync, or the repo.
- Escaped text, mentions disabled, payload limits enforced.
- Bounded queue (`discordQueueMax`); on any error class or timeout, back off on a fixed schedule (1 s, 5 s, 30 s) and then drop, with one local line per class per episode. `RestCallback` reports an `ERestResultState` error class, not the HTTP status (`3_Game/DayZ/Http/RestApi.c:8,55`), so a `429` cannot be told from another client error (recon R6) and retry-after values are not assumed (accepted by the orchestrator, October 31, 2026, recon plan line 5).
- Distinguish retryable failures from configuration failures; surface overflow and permanent failure locally and in the admin menu.
- Never repeat a game command because its notification failed.

Exit: local records remain useful during HTTP failures; delivery failures cannot stall gameplay or create unbounded work.

## Phase 4 - hardening and release

- Command-by-command authorization and malformed-input matrix from the dev harness.
- Permission changes during active sessions and across restart.
- Duplicate requests, invalid targets, concurrent admins, sustained bursts.
- Every destructive command's failure and partial-success behavior.
- Install, update, and removal checks on disposable copies of representative world and character storage, compared against the same-build control.
- Stale-client behavior and packaged menu resources verified from a normal Workshop install.
- Write `docs/operations/central-scrutinizer.md` (setup, permissions, bans, audit failures, backup and restore, rollback, server-mod upload) and a one-page quick reference for Jay.
- Jay's menu usability sign-off; Jeff's release approval; deploy per shared rules 9.

Back up mission storage and `$profile:NimbyAdmin` together. Code rollback does not reverse spawned objects or player changes; world restoration is a separate decision because it discards later progress.

## Phase 5 - ops pages (after the first live release, N10)

Three new menu pages over the Phase 1-3 plumbing. None adds a persistence hook; the registry below is rebuilt at boot and refreshed on a timer, never saved.

### Vehicle manager

- Server-side registry of every `CarScript` and `BoatScript` entity (class, position, persistent id, last-seen time), filled at load and refreshed every `vehicleRegistryRefreshSeconds`; the same registry the Phase 6 map draws from (addendum F17).
- Per vehicle: teleport to, delete (confirmation required), unstick (1 m lift, surface check, 2 m nudge), refuel (every fluid to capacity), repair (every part and the hull to full health). Map markers for all vehicles ship with Phase 6 and read this registry.

### Object tools

- Select: the object under the admin's look direction, or a pick from a nearby list (radius in the menu). Scope per N12; a map static shows its class and position and offers no verb.
- Delete: confirmation required; the audit `target` carries class, persistent id, and position so the record is enough to respawn a wrongly deleted item by hand.
- Move: set position and yaw; the server re-places with the surface check from teleport and refuses a position inside geometry or more than a bounded distance from the original.

### Density (N15)

- The registry sweep also bins every persistent object into `densityTileMeters` tiles with a count per class group; the recount runs every `densityRefreshSeconds`, spread over ticks so no single frame pays for it.
- Density page: top tiles by total, each with its counts per group, the nearest named location, a best-guess owner from F15 (marked best-guess until the journal is older than the oldest structure in the tile), and a "teleport to" button that reuses the Phase 1 teleport.
- Alerts: one Discord message per tile per restart when the total passes `densityAlertObjects`, naming the tile, the counts, and the best-guess owner; per-player alerts on `densityPlayerAlertObjects` once F15 has data. Alerts are audited like commands (actor `system`).
- Counting is the reporting tool only; nothing is removed automatically. Removal stays an admin's audited object-tool action (N12), and the base-size governor idea recorded October 15, 2026 stays unplanned.

### Admin notes

- Per player (N13): add, list (newest first, 20 lines in game), and delete your own note; an `owner` deletes any. Notes appear on the player card next to first seen and session rows.

| Scenario | Required result | Evidence |
| --- | --- | --- |
| Delete a vehicle | Gone now and after restart | Audit `result=ok`; absent from the registry after RCON `#shutdown` restart |
| Delete a map static | Refused | `result=invalid reason=static object` |
| Move a tent 3 m | Tent at the new position after restart, collision intact | Manual check; audit `effects.position` |
| Restrain, then admin disconnects | Hold ends at timeout | One `result=ok` for apply, one `[NIMBY][Admin] hold expired` line, owner Discord message |
| Note by `admin`, delete by another `admin` | Refused | `result=denied` |
| Corrupt `notes.json` | `.lkg` used, edits refused | Same lines as N8 |
| Tile with 450 persistent objects on the replica (spawned with the dev harness) | One alert, then silence until restart | One Discord message; `[NIMBY][Admin] density alert tile=<x>/<y> total=450` once |
| Recount cost at 40 players and a full replica world | No frame spike | Server FPS log before and after the recount within 5 percent |

## Later (not committed, N14)

Surfaced by the October 21, 2026 gap analysis against COT and VPP and not adopted. A future revision promotes or drops each with the reasoning in view.

| Capability | Verdict |
| --- | --- |
| Automatic removal of over-limit loot (the base-size governor idea, October 15, 2026) | Not adopted: N15 reports and alerts; removal stays a human, audited action |
| Automatic test kit on fresh spawn (the NimbyKit brief, October 26, 2026) | Absorbed (N17): `spawn.kit` in Phase 2 covers the manual case; the on-spawn version is a `NimbyAdminServer` hook over `kits.json` plus a recipient list, built when a test window needs it |
| Invisible and god mode as standalone toggles on self | Feasible (`Entity.SetInvisible`, `Object.SetAllowDamage`); spectate already uses both on the admin's hidden body. Would need Q5-style guards; no roleplay use identified for applying either to another player |
| Map click-to-teleport (self and target) and click-to-drop | Same code paths as teleport and addendum F14 with a position argument; natural Phase 6 extension |
| Saved loadouts applied to a player | Catalog plus a JSON file; useful for events |
| Player inventory view and clear inventory | The most common dispute ask on roleplay servers |
| Ban list page: view, edit duration, ban or unban an offline player | A page over the existing ban store |
| Kick all | One loop over sessions; pairs with a restart countdown |
| Saved teleport points and teleport to coordinates | Cheap once Phase 6 exists |
| In-game log viewer | Eyes' web page is the planned viewer; an in-game last-N-lines audit page is widget work |
| RCON console | Not feasible in script (BattlEye RCON is UDP; Enforce has HTTP only); lives in Eyes. No script API for `#lock`/`#unlock` was found in 1.30 |
| Free camera, ESP, chat commands, password layer, item stats editor, static-object spawner with persistence | Rejected: observation tools stay out, ESP never, and the rest add a second auth or persistence surface for little use |

## Acceptance criteria

- Server-authoritative authorization for every command and sensitive query, with a dev-harness denial test per command.
- No unexplained script errors; the `storage_1`-copy boot shows 0 "Scripted variables corrupted".
- Audit `intent` written before every state change; `result` written after.
- Bounded, non-blocking Discord delivery.
- Every hold or override on a player (restraint, and spectate when it ships) ends on a server-enforced timeout, is audited on apply and release, and notifies the owner.
- Multiplayer, restart, removal, and rollback tests passed against the exact artifact.
- Client PBO in the Workshop item; server PBO on the host only; no secret in either.
- Jay signs off on the menu.

## Estimate

Re-estimate after Phase 0 in engineering effort and tester hours. Treat five to six weeks as an initial hypothesis. No promise of same-day compatibility with future DayZ updates.

## Risks

| Risk | Effect | Mitigation |
| --- | --- | --- |
| Menu layout iteration runs long | Phases 0-2 slip | Functional layouts first, vanilla widgets and `set:dayz_gui` images only; polish after Phase 4 |
| A missed server-side check | Any player acts as admin | Server-only checks in the server PBO; denial test per command; Phase 4 sweep |
| HostHavoc lacks `-servermod=` or outbound HTTPS | N7 layout or Discord blocked | Phase 0 item 4 before any dependent code |
| `RestApi` limited in 1.30 | No Discord posts | Local audit is authoritative; webhook becomes a later item |
| Sandstorm not scriptable | One control missing | Out of v1 |
| DayZ update breaks a used API | Admin tool down | Small surface; boot check fails the build |
| Object delete is irreversible | A wrong click removes a player's base part | Confirmation plus a full audit `target`; restoring is a manual respawn from the record, never a world rollback |
| Restraint abused or forgotten | A player held with no admin present | Server-side timeout, owner notified in Discord, both forms audited; the toast tells the target it is an admin action |
| Plan 11 merges late | The client PBO cannot build against `main` | `feat/nimby-admin` is cut from `feat/nimby-shared` and rebased after the merge; the server-side work of Phase 0 and Phase 1 does not wait |
| Two seats in the tree | Build or test-server port collision with the Hands session | `AGENTS.md` shared-machine rule: check for a running `DayZServer_x64` before any boot |
