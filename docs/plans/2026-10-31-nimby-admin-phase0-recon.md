# Plan 3 Phase 0 recon - Central Scrutinizer protocol, stores, and proofs

Draft v0.1, October 31, 2026 (night). Seat: Claude Code in worktree `D:\GitHub\nimby-badlands-server-wt\nimby-admin`, branch `feat/nimby-admin`, cut from `origin/main` at `abe2e12` (PR #16, the Phase 0 stubs). Written against Plan 3 v0.7, shared rules v1.9, Plan 11 v0.3. Status: Stop A, read-only. No code is written until the go.

Citations are `file:line` inside `vendor/dayz-1.30-scripts/` (relative to its `scripts/` folder unless the path starts with `bin/`). Read section 10, risk R1, first: the reference is the 164078 extract and the server now runs 164200.

## 1. Baseline record (Phase 0 item 1)

The record lands in Stop B as `docs/baselines/<date>-central-scrutinizer-phase0-baseline.md`, with the values read on October 31, 2026:

| Item | Value | Source |
| --- | --- | --- |
| Server engine | `1.30.164.200` | `DayZServer_x64.exe` FileVersion, both `F:\SteamLibrary\steamapps\common\DayZServer\` and the replica `D:\DayZServer\` |
| Server `dta\scripts.pbo` SHA-256 | `FF0AD7EE5297C75DA74364569A830BC0223315C0131579A0C11A7875D64CB2E2`, written 2026-10-21 22:40 | Same file in both installs |
| Shared reference | `vendor/dayz-1.30-scripts/`, build `1.30.164078.38`, `scripts.pbo` SHA-256 `E7851E289E6434494E4232F9A3E86D6D6675B5D35922A4044450F943F9935189` | `docs/baselines/2026-10-15-dayz-1.30-scripts-reference.md` |
| Client `scripts.pbo` | Not readable from this seat: `F:\SteamLibrary\steamapps\common\DayZ\dta\` holds no `scripts.pbo` on this machine | Jeff reads it, or the re-extraction record does (R1) |
| Source revision | `main` at `abe2e12`; the build's `feat/nimby-admin` commit is written at Stop B | `git rev-parse` at build time |
| Build and boot | The `@NIMBY-<stamp>` and `@NIMBY-Server-<stamp>` folders and the `SERVERMOD` line of the first green `build.log` | Stop B |

The two `scripts.pbo` hashes differ, so the reference no longer matches the binary the replica and the live server run (R1, question 1).

## 2. Cited, not re-proven (Phase 0 item 2)

| Fact | Proof | Reused in Central Scrutinizer |
| --- | --- | --- |
| RPC in both directions with `Param3<int, int, string>` | Greet G5; `docs/testing/2026-10-25-greet-phase0.md:58` (G-P0-31); NimbyShared SH-11 `docs/testing/2026-10-31-nimby-shared-phase1.md:69` | Result, push, chunk, and done RPCs keep that shape (section 4) |
| One RPC string is intact at 1,000 bytes and corrupt at 1,100 | NIMBY-T-037, `2026-10-25-greet-phase0.md:57,80` | `NimbySharedRules.CHUNK_BYTES`; the request is capped below it (section 4.2) |
| Chunked payloads above 1,000 bytes, reassembled with byte count and hash | G-P0-31 (74 chunks, 73,543 bytes); SH-11 (29 chunks) | `NimbySharedChunkSender` (server) and `NimbySharedChunkReceiver` (client), `NimbyShared/Scripts/3_Game/nimbyshared_chunks.c:149,397`, for the Players refresh |
| Profile JSON load and save through `JsonFileLoader` | Greet G-P0-17 `2026-10-25-greet-phase0.md:44`; `seen.json` and `prefs.json` in G-P1-16 and SH-16 | The temp, validate, copy, delete write of `nimbygreet_missionserver.c:372-388` and `nimbygreet_clientfiles.c:34-47`, for `config.json`, `permissions.json`, `bans.json` |
| A malformed JSON file prints the engine's own `SCRIPT (E): JSON ERROR` | NIMBY-T-042, `2026-10-25-greet-phase0.md:85` | Shapes risk R8 and the request codec (section 4.2) |
| A `.layout` loads from a signed client PBO on a real client | Greet G-P1-01 `2026-10-26-greet-phase1.md:15`; NimbyShared SH-02 `2026-10-31-nimby-shared-phase1.md:60` | `nimbyshared_window.layout` plus the addon's own content layout |
| Menu open and close with the `menu` input exclude and the zeroing guard | NIMBY-T-039 closed by G-P1-02 `2026-10-26-greet-phase1.md:16`; NIMBY-T-058 by SH-05 and SH-06 `2026-10-31-nimby-shared-phase1.md:63-64` | `NimbySharedWindow` base, unmodified |
| The hotkey is read in one place | NIMBY-T-046, `2026-10-25-greet-phase0.md:70,89`; G-P1-05 `2026-10-26-greet-phase1.md:19`; SH-03 | The shape of `NimbyGreetHandleHotkey` (`nimbygreet_missiongameplay.c:565`) in `MissionGameplay.OnUpdate` |
| `Data/Inputs.xml` bindings rebind in Options > Controls | Ears EP-005 `docs/testing/2026-10-16-ears-phase0.md:31`; Greet G7; SH-08 `2026-10-31-nimby-shared-phase1.md:66` | The nesting of `NimbyShared/Data/Inputs.xml` (`<sorting>` inside `<inputs>`, group `nimby`) |
| No `CallLater` of a 5_Mission function | NIMBY-T-052, G-P1-18 to G-P1-20 `2026-10-26-greet-phase1.md:32-34`; shared rules 7.1 | No deferred call at all: every timer is a tick-time comparison in `OnUpdate` (section 8.5) |
| `GetPlainId()` is the 17-digit Steam64 on a dedicated server | Legs P0-steam `docs/testing/2026-10-19-legs-phase1.md:31` (on 164078) | The actor key; re-shown on 164200 by item 3 row 1 |
| Ownership of the targeted `PlayerBase` is checked before a request is read; refusal lines are gated per sender | Greet GREET-R2-02, `nimbygreet_playerbase.c:131-158`; `nimbygreet_log.c:69-115` | The router's first two checks and its gated denial logging (section 4.3) |
| A line that names a player puts both ids first and the escaped name last | Greet GREET-R2-01, `nimbygreet_log.c:42-58` | `NimbyAdminLog.Who` (same format) |
| Server-only build output and `-servermod=` | `scripts/Build-NimbyPack.ps1` help lines 21-27 and the `SERVERMOD` manifest line; `scripts/Start-NimbyTestServer.ps1 -ServerModPath` (lines 73, 553, 558) | Code exists; Stop B's first build is its first use with a real `serverOnly` row (plan item 6) |

## 3. In-game proofs on the replica (Phase 0 item 3)

| # | Proof | Vanilla call (`file:line`) | How the seat exercises it | Evidence |
| --- | --- | --- | --- | --- |
| 1 | `GetPlainId()` returns the 17-digit Steam64 on 164200 | `PlayerIdentity.GetPlainId`, `3_Game/DayZ/gameplay.c:370` (`GetId` :368, `GetName` :362); vanilla reads it at `3_Game/DayZ/Client/SyncPlayerList.c:18` | Every connect runs the role sync (section 8.4), which logs the identity | `[NIMBY][Admin] role sync player=(<bi-id>, 76561198354705597) "<name>" role=owner source=bootstrap`; the second id matches Jeff's Steam profile and passes `IsSteam64` |
| 2 | `RestApi` POSTs from the server to a webhook | `CreateRestApi` and `GetRestApi`, `3_Game/DayZ/Http/RestApi.c:181,183`; `GetRestContext` :155; `RestContext.POST` :123; `SetHeader` :141 (Content-Type only); `SetOption` :175, `ERestOption` :28; `RestCallback.OnSuccess` :73, `OnError` :55, `OnTimeout` :64; `ERestResultState` :8. No vanilla script calls `GetRestApi()`, so behavior is wholly unproven | The teleport row's Discord message, posted through the queue (section 7) to Jeff's throwaway webhook | `[NIMBY][Admin][Debug] discord delivered seq=<n> bytes=<n>` and the message in the throwaway channel (Jeff's screenshot). A failure shows the error class (R6) |
| 3 | Player position set, with the teleport surface check | `DeveloperTeleport.GetPlayerRootForTeleporting`, `4_World/DayZ/Plugins/PluginBase/PluginDeveloper/DeveloperTeleport.c:85`; the release path of `SetPlayerPosition` :115-149 (sea-level clamp, then `Object.SetPosition` on the server, `3_Game/DayZ/Entities/Object.c:294`); `CGame.IsBoxColliding`, `3_Game/DayZ/Global/Game.c:1333`; `SurfaceY` :1176; `SurfaceIsSea` :1196 | `player.teleportTo` from Jeff to the second client: destination 1 m behind the target, a 0.6 x 1.8 x 0.6 m box test excluding both players, four fallback offsets, then the target's position plus 0.2 m | Audit `result=ok` with `effects.from`, `effects.to`, `effects.offset`; `[NIMBY][Admin] teleport done ... distance=<m>`; both clients see the admin beside the target |
| 4 | `SandstormController.Start` and `Stop` take effect on a dedicated server | `Sandstorm.c:20` (`Start(duration, calledByWeather)`, server only, storm must be inactive), :27 (`Stop`), :32 (`IsActive`); `Weather.GetSandstorm`, `3_Game/DayZ/Weather.c:210`. Vanilla raises overcast to at least 0.8 and wind to at least 18 before it starts one (`3_Game/DayZ/DayZGame.c:3425-3436`) | A dev-harness online case armed by the word `sandstorm` in the forge file (question 2): overcast and wind as vanilla does, `Start(120, false)`, `IsActive` read 5 s later, `Stop(30, false)` 90 s later, `IsActive` read again | `[NIMBY][DevHarness][Forge] PASS sandstorm: Start=true active=true` then `... stopped active=false`; Jeff sees the storm come and go |
| 5 | Home is free | `bin/constants.xml:578` declares `kHome`; zero uses in `bin/preset_X1MouseKey.xml`, `_Alternate`, `preset_PS4MouseKey.xml`, `_Alternate`. F10 is bound in all four (`preset_X1MouseKey.xml:446,480,824`) | `UANimbyAdminOpen` on `kHome` in `NimbyAdmin/Data/Inputs.xml`; an admin presses Home, then rebinds it | Menu opens on Home; Options > Controls lists "Central Scrutinizer: open menu" in the NIMBY group; after a rebind Home does nothing and the new key opens (the shared rules 5 rebind case) |

Item 5 of Plan 3 (the HostHavoc `-servermod=` upload and outbound HTTPS to `discord.com`) stays Jeff's on the live panel. The seat prepares the `@NIMBY-Server` folder and the exact lines at Stop B; no server-only code depends on the live host before that row passes.

## 4. The protocol

### 4.1. RPC ids

Inside `65503xx`, outside `6550350`-`6550399` (N9d). Commands are strings inside the request, so one request id serves every command and the block stays mostly free.

| Id | Direction | Params | Purpose |
| --- | --- | --- | --- |
| `6550301` | client to server | `Param4<int protocol, int requestId, string command, string args>` (`1_Core/DayZ/param.c:119`) | Every request, the session hello included |
| `6550302` | server to client | `Param3<int requestId, int code, string detail>` | The result, to the sender only |
| `6550303` | server to client | `Param3<int kind, int sequence, string payload>` | `NimbyAdminPush` (N9a); kind 1 = `role` is the only kind in this plan |
| `6550304` | server to client | `Param3<int requestId, int index, string text>` | Codec chunk (`NimbySharedChunkSender.Send`) |
| `6550305` | server to client | `Param3<int requestId, int chunkCount, string doneJson>` | Codec done; `NimbyAdminDone` (`bytes`, `hash`, `kind`) |

`6550306` to `6550349` stay unassigned. Every RPC addresses the recipient's own `PlayerBase` (`CGame.RPCSingleParam` with recipient, `3_Game/DayZ/Global/Game.c:1013`), as Greet does; `PlayerBase.OnRPC` is at `4_World/DayZ/Entities/ManBase/PlayerBase.c:5727`, reached through `DayZGame.OnRPC` (`3_Game/DayZ/DayZGame.c:3141,3151`).

### 4.2. Request envelope

| Field | Rule |
| --- | --- |
| `protocol` | Exactly `1`. Any other value is `invalid reason=protocol` |
| `requestId` | A positive int from a per-client counter (the `NimbyGreetClient.NextRequestId` shape); unique per actor per connection |
| `command` | One of the registered commands, at most 32 bytes. Phase 0: `session.hello`, `players.list`, `player.teleportTo` |
| `args` | The seat's own `key=value` list separated by `;`, values percent-escaped for `;`, `=`, `%`, and control bytes; at most 16 pairs, keys `[a-z0-9.]` 1 to 32 bytes, each value at most 200 bytes, the whole string at most 900 bytes. An engine-free parser in 3_Game, fully self-tested |

Why not JSON for `args`: parsing a malformed string through `JsonSerializer.ReadFromString` (`3_Game/DayZ/gameplay.c:100`) is the code path that printed the engine's own `SCRIPT (E): JSON ERROR` in NIMBY-T-042 (expected, not yet shown, for strings). A hostile client could then write `SCRIPT (E)` lines at will, and the boot check and every log reader treat those as failures. The server writes JSON (audit, Discord, done records); it never parses JSON a client sent (question 6).

### 4.3. Router validation order

Each step is cheaper than the next; nothing expensive runs before the rate limits.

1. `sender` is not null and has an identity.
2. The targeted `PlayerBase` is the sender's own (`GetIdentity().GetId()` equals `sender.GetId()`); otherwise `refused reason=sender-not-owner`, gated (Greet pattern).
3. Per-sender limit: a token bucket of `requestsPerSecond` (default 5) with a burst of 10, keyed by the BI id. Aggregate limit: 40 requests per second for all senders. Over either: dropped with a gated `rate-limited` line, no reply.
4. `ctx.Read` of the `Param4`; unreadable is `invalid reason=unreadable` (gated).
5. `protocol`, `requestId`, `command` length, `args` length.
6. Actor: `sender.GetPlainId()` must pass `IsSteam64`; the actor record is `steam64`, `biId`, and the escaped name, all from the connection. Nothing the client sent names the actor.
7. Duplicate and stale: per actor per connection, the last 64 accepted request ids are kept. An id in that set is a duplicate: no work, no second audit record, one gated `[NIMBY][Admin] duplicate id=<n> player=(...)` line. An id below the lowest id in a full set is stale: `invalid reason=stale`.
8. Command known; then `args` parsed and checked by the command's own engine-free validator: `invalid reason=<field>`.
9. Authorization: the actor's role from the permissions store, read as section 5.4 says, must grant the command's capability (`player.teleport`, `player.list`; `session.hello` needs none). Otherwise `denied reason=no role` or `denied reason=capability`.
10. Target: resolved at execution time from the current sessions by the `target` BI id plus the `session` number the server assigned at that connect (section 8.4). A reconnected target has a new number, so an old selection is `invalid reason=target session ended`, never a retarget.
11. Audit `intent` append (section 6). A failed append is `failed reason=audit unavailable` and nothing changes.
12. The handler runs.
13. Audit `result` append, Discord enqueue, the result RPC to the sender only.

Denied and invalid requests get one audit record (`phase` `result`, no `intent`, because nothing changed), and their log lines and records are rate-limited per sender (one per 10 s with a `suppressed=<n>` count on the next), so a hostile client cannot fill the disk.

### 4.4. Result envelope

`code`: `0` ok, `1` partial, `2` denied, `3` invalid, `4` failed. `detail`: the reason, or a short `key=value` effects list for the menu's status line; at most 900 bytes; never a secret, never another player's data unless the command returns it. A request dropped by the rate limit or the ownership check gets no result.

### 4.5. Push envelope

`kind` 1 is `role`; its `payload` is `owner`, `admin`, or `none`. `sequence` rises by one per push per connection, so the client keeps only the newest. The client registry of known kinds is engine-free (3_Game); the self-test registers a fake kind 99 and proves an unknown kind is dropped (N9 seam). A role push is sent on the session hello and whenever a permissions reload changes that player's role.

## 5. Server stores

### 5.1. Files

All under `$profile:NimbyAdmin\`: `config.json`, `permissions.json` and `.lkg`, `bans.json` and `.lkg`, `audit\audit-<YYYY-MM-DD>.log` (UTC date), and the `debug` flag. The server PBO's self-test flag is `$profile:NimbyAdminServer\selftest` (stub, `nimbyadminserver_rules.c`). `server/profile-overrides/NimbyAdmin/config.example.json` carries every field with its default and an empty `webhookUrl`.

### 5.2. Loaders and validation

| File | On load | Invalid row or value | Malformed or unreadable |
| --- | --- | --- | --- |
| `config.json` | `JsonFileLoader<NimbyAdminConfig>.LoadFile` (`3_Game/DayZ/tools/JsonFileLoader.c:7`); a missing file is written from the defaults (no bootstrap owner, empty `webhookUrl`) | Each integer is bounded (for example `requestsPerSecond` 1 to 20); out of range takes the default, one line per field | Defaults with no webhook and no bootstrap owner, one line. The webhook state is logged as `webhook=set` or `webhook=unset`, never the value |
| `permissions.json` | `version` must be 1; each row needs `IsSteam64(steam64)` and a role of `owner` or `admin` | Skipped, one line with the row index; never granted | `.lkg` is used, one line, one Discord message, edits refused (5.3). With no `.lkg`: the active set is empty plus the bootstrap owner (question 4) |
| `bans.json` | Each row needs `IsSteam64`, `reason` at most 200 bytes, an `actor` Steam64, `createdUtc`, and an optional `expiresUtc` that parses as ISO 8601 UTC | Skipped and logged | `.lkg` is used, logged, posted, edits refused. With no `.lkg`, N8's "admission is denied" applies (question 3) |

Every adopted parse refreshes that file's `.lkg` from the text just parsed (Greet's rule), and a file that does not change is not parsed again (NIMBY-T-056), so a bad file costs one engine `JSON ERROR` line per distinct text, not one per read.

### 5.3. Atomic write and the refuse-edits state

There is no rename in 1.30: `EnSystem.c` has `CopyFile` (:531), `DeleteFile` (:528), `MakeDirectory` (:525), and no move. The write is therefore copy-based: `SaveFile` to `<name>.tmp`, `LoadFile` it back and validate, `CopyFile` the current good file to `.lkg`, `CopyFile` `.tmp` over the file, `LoadFile` the result and compare, `DeleteFile` `.tmp`. A crash inside the second copy can tear the file; the `.lkg` is then the recovery (R4).

Refuse-edits (N8): a store whose active set came from `.lkg` or from nothing sets `editsRefused`, and every write path (the bootstrap write now, the Phase 1 menu edits later) returns `failed reason=<file> needs repair` until a re-read parses clean. Phase 0 has no edit command, so the state is shown by the bootstrap case and a self-test.

### 5.4. Re-read and first-owner bootstrap

`permissions.json` is re-read on a request when the cached copy is older than 5 s, so a hand edit (the "revoked mid-session" row) reaches the next request, and a burst costs one read (question 5). A reload that changes a connected player's role sends that player a role push.

Bootstrap: when the active set has no `owner` and `config.json` names a valid `bootstrapOwnerSteam64`, that id is `owner` in memory with `source=bootstrap`. It is written to `permissions.json` only when the file does not exist (first boot), through 5.3, and audited as an `intent` and `result` pair with actor `system`. It never overwrites an existing file.

## 6. Audit journal

- One file per UTC day, `$profile:NimbyAdmin\audit\audit-<YYYY-MM-DD>.log`, one JSON object per line, the fields of the plan's table. `ts` is built from `GetYearMonthDayUTC` and `GetHourMinuteSecondUTC` (`1_Core/DayZ/proto/EnSystem.c:100,76`) as `2026-10-31T21:05:03Z`. Strings are escaped by an engine-free writer (backslash, quote, control bytes), self-tested.
- Append: `OpenFile(path, FileMode.APPEND)` (`EnSystem.c:417`, `FileMode` :382-386), `FPrintln` (:481), `CloseFile` (:443), one open per record so nothing is lost on a crash between records. `Append` returns false when `OpenFile` returns 0.
- Intent before change: the router calls `Append(intent)` and runs the handler only when it returns true; otherwise the result to the sender is `failed reason=audit unavailable`, one `[NIMBY][Admin] audit unavailable` line per failure episode, and a Discord message. The `result` record is appended after the handler; if that append fails, the result still goes to the sender, the line names the request id, and the open `intent` stays in the file as the record that something ran.
- `FPrintln` returns nothing, so a write that fails after a good open (disk full) is not detected (R5). The locked-file gate row covers the open failure.
- Rotation by `auditRetentionDays` is Phase 3; daily names make it a delete of old files.

## 7. Discord queue

- Created once at mission start when `webhookUrl` is set: `CreateRestApi` if `GetRestApi` is null, timeouts set with `SetOption` (`ERestOption`, 10 s), a `RestContext` for the URL kept in memory only, `SetHeader("application/json")`. `EnableDebug` (`RestApi.c:165`) is never called, since engine debug output could print the URL.
- Payload: `{"content": <escaped text>, "allowed_mentions": {"parse": []}}`, text cut to 1,900 bytes on a character boundary (the `ChunkEnd` rule), no player-supplied text unescaped.
- A bounded queue (`discordQueueMax`, 200), one request in flight, pumped from `MissionServer.OnUpdate` with tick-time comparisons (`CGame.GetTickTime`, `Game.c:914`), so there is no `CallLater` anywhere. A full queue drops the newest with one overflow line per episode.
- Failure: `RestCallback` reports an `ERestResultState`, not an HTTP status, so a `429` cannot be told apart from another client error (R6, question 7). Proposed: any error or timeout retries the same message after 1 s, 5 s, 30 s, then drops it; `OnError` can fire more than once per request (`RestApi.c:53`), so each callback object ignores calls after its first. One local line per failure class per episode (`[NIMBY][Admin] discord failed class=client|server|timeout|other queued=<n>`), reset by the next success.
- A command never waits on Discord and is never repeated because its notification failed; the audit file is the record.

## 8. The client (`NimbyAdmin`)

### 8.1. Config and layout

`config.cpp` gains `inputs = "NimbyAdmin/Data/Inputs.xml"` and a `worldScriptModule` (`PlayerBase.OnRPC` lives in 4_World); `NimbyAdminServer/config.cpp` gains one too. Both stay `requiredAddons[] = { "DZ_Data", "NimbyShared" }`. Files: `3_Game` (rules, args codec, push kinds, capability table, request-id counter, invokers, self-test), `4_World` (`PlayerBase` RPC plumbing, client only), `5_Mission` (window, pages, hotkey, pause entry, self-test hook), `GUI/nimbyadmin_content.layout`, `Data/Inputs.xml`, `stringtable.csv`.

### 8.2. Window and page registry

`NimbyAdminMenu extends NimbySharedWindow`, menu id 65503. `ContentLayout()` is a player list on the left (row buttons in a `ScrollWidget`), a detail pane on the right (`BodyScroll`, so the shared page keys work), and a footer with "Teleport to", "Refresh", and a status line, after the mockup's layout with vanilla widgets only. The page registry (N9c) is `NimbyAdminPage` subclasses (`Id`, `Label`, `Capability`, `Build`, `OnResult`) registered in `MissionGameplay.OnInit`; `SectionCount`, `SectionLabel`, and `OnSectionSelected` read it. Phase 0 registers the Players page; the self-test registers and removes one fake page, as NimbyShared's registry cases do. The base class needs no new method.

### 8.3. Players refresh

`players.list` returns each session's `session` number, BI id, Steam64, escaped name, alive, and distance from the admin, through the codec (40 players at about 150 bytes is about 6 KB, 6 chunks). The client's `NimbySharedChunkReceiver` is capped at 64 KB.

### 8.4. Admin-state receiver and role sync

After `OnPlayerLoaded` on the controlled player (the Greet bridge), the client sends `session.hello`; the server assigns that connection its `session` number, logs the role sync line (item 3 row 1), and pushes the role. A push sent before the client's `PlayerBase` exists would be lost, so the client asks, rather than the server pushing at `ClientReady` (question 8). The client stores the role and renders; it decides nothing.

### 8.5. Home, the pause entry, and timers

`UANimbyAdminOpen` on `kHome`, read only in `MissionGameplay.OnUpdate` (NIMBY-T-046). With role `none` a press does nothing and sends nothing (one debug line). `NimbyAdminPauseEntry` registers with `NimbySharedPauseButtons`; `IsVisible` returns "role is not none". The request timeout (10 s without a result shows "no answer") is a tick-time check in `OnUpdate`; nothing is queued.

## 9. Dev-harness cases

The harness loads with `-mod=` beside the pack on the test server and on Jeff's client (`Start-NimbyTestServer.ps1:626-628`; the Hands record's `-mod=` line, `docs/testing/2026-10-19-hands-phase1.md:228`). A client has no `NimbyAdminServer`, so a harness class that names a server-only class does not compile there (shared rules 8.1). Proposed (question 2): the harness forges at the wire. It builds the `Param4` in a `ScriptReadWriteContext` (`3_Game/DayZ/gameplay.c:134-141`, the harness's Legs precedent at `nimbydevharness.c:271`) and calls the real `PlayerBase.OnRPC(identity, 6550301, ctx)` with a connected player's real identity, naming only `NimbyAdminRules` constants from the client PBO. Cases run when the forge file holds `admin` and two players are in the world; the owner's Steam64 comes from a forge line `admin-owner=<steam64>`.

| Case | Forged as | Expected | Evidence |
| --- | --- | --- | --- |
| Non-admin forges a teleport | The second client (no role), target Jeff | Denied; neither position changes | Audit `result=denied reason=no role`; `[NIMBY][DevHarness][Forge] PASS` with both positions |
| Request aimed at another player's `PlayerBase` | The second client's identity on Jeff's player | Refused before read | `refused reason=sender-not-owner` |
| Malformed: protocol 2, id 0, unknown command, `args` 901 bytes, bad escape, 17 pairs | Jeff | Each `invalid` with its reason | One audit `result=invalid` per case (gated) |
| Duplicate request id | Jeff, `player.teleportTo` the second client, same id twice | Second ignored | One `intent`, one `result`; one `duplicate id=` line |
| Burst | Jeff, 50 requests in one frame | 10 accepted, the rest dropped | One `rate-limited` line with `suppressed=` |
| Sandstorm (item 3 row 4) | none | As section 3 | As section 3 |

Without a second client, the engine-free half of every rule runs in the self-tests (section 11), and the harness cases report `SKIP need two players`.

## 10. Risks the plan does not list

| # | Risk | Effect | Proposal |
| --- | --- | --- | --- |
| R1 | The shared reference is the 164078 extract; the server runs 164200 (different `scripts.pbo` hash, section 1) | Every `file:line` in this recon and in Plan 3 may be off, and a changed API would pass review | The orchestrator re-extracts 164200 (shared rules 7 makes it theirs) before Stop B; the seat re-checks every citation here against it (question 1) |
| R2 | The dev harness loads on clients, so it cannot name a server-only class | The plan's "forged requests in the harness" cannot reach the router directly | Wire-level forging (section 9) or a server-only dev PBO (orchestrator-owned tooling) (question 2) |
| R3 | The engine's own comment on `GetPlainId` says it "cannot be used in database or logs" (`gameplay.c:369`) | Shared rules 4.2 logs it anyway | Not reopened: rule 4.2 is decided and Legs proved the value. Recorded so a reviewer does not raise it as new. The Discord text carries the Steam64 only for actor and target in a private channel |
| R4 | No rename or move API; the atomic write is copy-based | A crash mid-copy can tear a store | `.lkg` recovery (5.3); the self-test covers the validate step |
| R5 | `FPrintln` returns nothing | A failed write after a good open goes unseen | Open failure blocks; disk exhaustion stays untestable (plan Phase 3) |
| R6 | `RestCallback` gives an error class, not the HTTP status | "Fixed backoff on 429" cannot be implemented as written | Retry every error class on the fixed schedule (question 7) |
| R7 | No vanilla script uses `RestApi`; it is one process-wide object | Server-side behavior unknown; another mod calling `SetOption` changes our timeouts | Item 3 row 2 proves it; no other mod uses it today |
| R8 | Parsing client JSON can print engine `SCRIPT (E)` lines (NIMBY-T-042 class) | A hostile client fills the log with what every reader treats as failure | Own `args` codec (4.2); the server never parses client JSON |
| R9 | A forged request string above 1,100 bytes may throw in the server's `ctx.Read`, as it did on the client (NIMBY-T-037) | An engine VM exception per packet | Rate limit first; the harness cannot reach the network path, so this stays untested and recorded |
| R10 | `GetPlayerRootForTeleporting` returns the vehicle for a seated player (`DeveloperTeleport.c:85`) | Teleporting a driver moves the car | Phase 0 refuses an actor or target in a vehicle (`IsInVehicle`, `4_World/DayZ/Entities/DayZPlayerImplement.c:468`): `invalid reason=in vehicle` |
| R11 | Both PBOs mod `PlayerBase.OnRPC` and `MissionBase.OnInit`, and the client PBO also runs on the server | A client-side handler running on the server | Every client handler is gated on `IsClient()`, every server handler on `IsServer()`; the request id is handled only in the server PBO |
| R12 | The NimbyShared window is new code on main since PR #15 | A shell defect shows first in this menu | Plan 11 C3 own-branch rule; any shell gap is reported, not patched here |

## 11. Self-test cases (both components)

`[NIMBY][Admin][SelfTest]` (client PBO, runs on both machines): args codec (round trip, escapes, every bound), request envelope checks, request-id counter, push-kind registry with the fake kind, capability table with the fake capability `selftest.fake`, role parsing, codec receiver cap; 5_Mission: fake page registered and removed.

`[NIMBY][AdminServer][SelfTest]` (server PBO): `IsSteam64` (stub cases kept), permissions row validation, role to capability, ban row validation and `IsBanned` with and without expiry, config bounds, ISO timestamp, audit JSON escaping and record shape, Discord payload escaping, mention block and length cut, queue bound and backoff schedule, token bucket, duplicate and stale window, target session match, refuse-edits state.

## 12. Plan lines to change

1. Gate row 1 evidence: every client sends `session.hello` at spawn, so the evidence reads "no request line from a Home press" rather than "no request line for that player".
2. Gate row "Duplicate request id", "Non-admin forges a teleport", and "Admin teleports to a player" need a second connected client; the Phase 0 gate session is a two-client session (Jeff plus Jay or a second client).
3. Audit table: a denied or invalid request writes one `result` record and no `intent`; "two per command" applies to commands that reach the handler.
4. Ban enforcement at connection (`CGame.DisconnectPlayer`, `Game.c:399`, vanilla call `missionServer.c:721`) moves to Phase 1 with its two-client test; Phase 0 ships the ban store's loader, validator, `.lkg`, and self-test so the N18 review covers it.
5. Phase 3 Discord text: "fixed backoff on `429`" becomes "fixed backoff on any error class" (R6).
6. Phase 0 item 1 names engine 164200 while the reference is 164078 (R1); the item gains "re-extract the reference first".

## 13. Questions

One at a time through Jeff, each with the recommendation first.

1. **Stale reference (R1).** The orchestrator re-extracts the 164200 scripts before Stop B, and the seat re-checks every citation in this file against it. (A) Yes, before Stop B code (recommended). (B) Proceed on the 164078 extract and re-check at Phase 4. (C) The seat re-extracts into a new folder itself.
2. **Harness reach (R2).** (A) Wire-level forging in the existing harness with two clients: the seat adds `nimbydevharness_admin.c`, a two-line hook in `OnPlayerConnected`, `NimbyAdmin` in the harness `requiredAddons[]`, and the sandstorm case (recommended; the Hands seat also edits the harness, so the merge order matters). (B) The orchestrator adds a server-only dev PBO to the tooling. (C) No harness cases in Phase 0.
3. **`bans.json` corrupt with no `.lkg`.** (A) Refuse admission to everyone except the active permission set (recommended, so an owner can still join and repair). (B) Refuse everyone.
4. **`permissions.json` corrupt with no `.lkg`.** (A) Bootstrap owner only, edits refused (recommended). (B) No admins at all.
5. **Hand-edit reach.** (A) Re-read `permissions.json` on a request when older than 5 s, a constant (recommended). (B) Also a `config.json` field. (C) Only at restart.
6. **Request `args` format.** (A) The seat's `key=value` codec (recommended, R8). (B) JSON.
7. **Discord retries (R6).** (A) Retry every error class at 1 s, 5 s, 30 s, then drop (recommended). (B) Retry only server errors and timeouts, drop client errors at once.
8. **Role sync trigger.** (A) The client's `session.hello` after `OnPlayerLoaded` (recommended). (B) A server push at `ClientReady`.

## Status, October 31, 2026, 23:30 (WRAPPING)

Done (Stop B, on `feat/nimby-admin`, pushed):

- Plan 3 v0.8 with the six accepted plan lines and `permissionsReloadSeconds` (`8012a3d`).
- Client half `NimbyAdmin` (`795514e`), server half `NimbyAdminServer` (`39afab9`), dev-harness cases and the sandstorm proof plus the CRLF fix (`4cc7f54`), baseline and test record (this commit).
- Build `@NIMBY-20261031-231332` (`-Dev -Qualification`, replica `storage_998`): `SUMMARY PASS`, `SERVERMOD` present, `Admin` 54/54, `AdminServer` 64/64, `DevHarness` 16/16, total 779/779. `Test-NimbyRepo.ps1` PASS.

Blocked:

- The two-client session and every in-game row: at 23:14 the main clone's test server started (`profiles\patch-1`, port 2302, the Hands seat), on the same port and `storage_998` this session needs. Not stopped; waiting.
- The rebuild from `4cc7f54` (line endings only since `-231332`): the build script refuses to boot while any DayZ server runs.

Next:

1. When no `DayZServer_x64` runs: `Build-NimbyPack.ps1 -ServerPath D:\DayZServer -BootInstanceId 998 -Dev -Qualification`, then update the build line of the test record.
2. Jeff pastes the throwaway webhook into `profiles\nimby-admin-p0\NimbyAdmin\config.json` and starts `Start-NimbyTestServer.ps1` with the command in the test record from a console he keeps open.
3. Run the gate table and the item 3 rows in `docs/testing/2026-10-31-central-scrutinizer-phase0.md`; then stop for the N18 review.

## Status, November 1, 2026, 00:25 (Part 1 done, waiting on the second review pass)

Done (resume brief Part 1, on `feat/nimby-admin`):

- The eleven Codex findings on `439b729`, each fixed with self-test cases, in `a521807`, `7741466`, `dd6e636`, `e7802d2`, `1180702`, `7c70299`, `f8d99c0`, `fd91248`; follow-ups from the background security review in `84a6fca`, `6ea46cc`, `4935842`, `5bad47f`. Every commit had a green `-Dev` build first; the last is `@NIMBY-20261101-001917`: `SUMMARY PASS`, Admin 69/69, AdminServer 106/106, DevHarness 16/16, total 836/836.
- The review copied to `docs/reviews/2026-10-31-central-scrutinizer-phase0-codex-review.md`; the dispositions table and the revised deviations in the test record.
- Orchestrator ruling on finding 1 (harness ids from the server's highest plus 1024, at most 127 forges per run), recorded as a deviation.

Waiting:

- Part 2: Jeff's short Codex pass on `git diff 439b729..HEAD`; Part 3 does not start before it comes back clean or ruled.
- Plan question raised for the orchestrator: write denied and invalid audit records to their own day file, so the finding 4 readback (whole day file per intent; 1.30 has no seek) reads only admin activity.

Next (Part 3): with no `DayZServer_x64` running, the gate rebuild (`-ServerPath D:\DayZServer -BootInstanceId 998 -Dev -Qualification`) and `Test-NimbyRepo.ps1`; then the gate rows and the item 3 proofs with Jeff and a second client.

## Status, November 1, 2026, 13:00 (re-review fixes done, waiting on the last review pass)

Done (orchestrator relay of the Codex re-review of `22c7226`: 8 closed, 1, 3 and 6 partial, 12 and 13 new):

- Re-review findings fixed, one commit each, each with self-test cases: 1 in `97ba30c` (the replay set keeps the 256 highest ids; harness offset highest plus 1,000,000 per the orchestrator ruling, 127-per-run cap kept), 3 in `8329297` (a store that ever loaded or recovered is never a first install), 6 in `4c53391` (no repeated-hello debug line), 12 in `e00b647` (held-back counts cleared only after a successful summary append), 13 in `967dd32` (denied and invalid records in `audit-denied-<date>.log`; Plan 3 v0.9).
- Before-fix boots for 1 and 3 failed on exactly the new cases (`build-20261101-124346`, `build-20261101-124616`); the last verification build `@NIMBY-20261101-125412` is `SUMMARY PASS`, Admin 69/69, AdminServer 121/121, DevHarness 16/16, total 851/851.
- The re-review copied to `docs/reviews/2026-11-01-central-scrutinizer-phase0-codex-rereview.md`; the test record carries the re-review dispositions table, the revised deviations and the operations notes. The plan question of 00:25 (a separate day file for denied records) is closed by finding 13.

Waiting:

- Jeff's last Codex re-review over `git diff 439b729..HEAD`; Part 3 does not start before it comes back clean or ruled.

Next (Part 3): unchanged from the 00:25 status.

