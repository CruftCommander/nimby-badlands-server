# Plan 3 Phase 1 recon - Central Scrutinizer carried items, then the Phase 1 commands

Recon for [Plan 3](2026-10-15-nimby-admin-plan.md) v0.10, Phase 1. Seat: Claude Code in worktree `D:\GitHub\nimby-badlands-server-wt\nimby-admin`, branch `feat/nimby-admin`, from head `648f783` (November 1, 2026). Brief: `D:\GitHub\nimby-badlands-server-wt\brief-nimby-admin-phase1-2026-11-01.md`. Section 0 is Stop A (the items carried out of Phase 0); the Phase 1 recon proper (Stop B) follows after the A3 answer. Vanilla citations are `file:line` inside `vendor/dayz-1.30-scripts/scripts/` (build 164200).

## 0. Carried items

### 0.1. A1 - harness session mirror

Cause (Phase 0 test record, duplicate row): the server keys a session on the `PlayerIdentity` object (`SessionFor`, `nimbyadminserver_core.c:211-231` at `648f783`) and opens a new session, with a cleared high water, when a request's sender object differs from the stored one. The harness mirrored session numbers in its connect hook and forged with `PlayerBase.GetIdentity()`. The `-163708` server script log (`profiles\nimby-admin-p0\script_2026-11-01_16-55-25.log`) shows the effect on both players: L's real hello at 17:15:55 was `session=2`, the step 1 forges at 17:16:04 opened session 3 for L (seen at the burst hello, `role sync ... session=3 reason=hello` 17:16:13), D's step 2 forges opened session 4, and D's own real teleport at 17:18:54 opened session 5 (`session ended number=5` for D at 17:36:27). So on 164200 `PlayerBase.GetIdentity()` returned one object per player across steps 1 to 4 that was not the object of that player's real requests. A3 measures this directly.

Fix (this commit set):

- `nimbydevharness_missionserver.c`: `InvokeOnConnect` passes its `identity` argument to `NimbyDevHarness.OnPlayerConnected(player, identity)`, which passes it to `NimbyDevHarnessAdmin.OnPlayerConnected(player, identity)`.
- `NimbyDevHarnessSessionMirror` (engine-free, `nimbydevharness_admin.c`): one number per identity object per BI id in connect order, by the server's rule (a new number when the object differs from the stored one), and the stored object per BI id, held without `ref` so a destroyed identity reads back as null, as on the server.
- `FindPlayers` finds the two bodies as before, then takes each one's stored `InvokeOnConnect` identity (`ConnectIdentityOf`); a player not seen at `InvokeOnConnect` is a `SKIP`. That object is the sender of every forge, numbers the teleport `session=` argument (`TeleportArgs(PlayerIdentity)`), keys the id base (`IdFor`), and names the actor Steam64 the audit lines are matched on (`AuditFor`, `AuditIn`). `PlayerBase.GetIdentity()` is used only to find a body's BI id, a string.
- Every argument of every call in the admin forge path is a local computed before the call (the 164200 argument-slot rule, `23b4e46`), the `OnRPC` call itself included (`ctx.GetReadContext()` was an argument beside `sender`).
- A run line before step 1: `[NIMBY][DevHarness][Forge] admin run: mirrored sessions owner=<n> other=<n>; senders are the InvokeOnConnect identities`.

Self-test (DevHarness, boot): `admin-mirror-forges-with-the-connect-identity`. Two mirrors stand for the server's `SessionFor` and the harness, with plain script objects as identities, because `PlayerIdentityBase` has a private constructor (`3_Game/DayZ/gameplay.c:383`) and script cannot make two identity objects for one BI id. The case passes only when: the mirror's sender is the connect object; a request sent with it keeps the server number; a respawn (the same object again) keeps the number on both sides; a request sent with a different object for the same BI id, as before A1, opens a new server session; and a reconnect (a new object) moves both sides to the same new number.

Why the case cannot fail on the old harness: the defect was not in the mirror's numbering rule (unchanged) but in which engine object fed it and which object the forges carried, and the old code had no unit that chooses a sender. Telling the two engine objects apart needs a connected client, so the proof that the old wiring fails and the new one passes is the in-game duplicate row: FAIL on `-163708`, rerun in A2. The case's "a different sender opens a new server session" clause shows the failure mode engine-free; a before-fix boot of the case would not compile (no mirror class), so it fails by construction, as the Phase 0 cases for new APIs did.

Build: `build\@NIMBY-20261101-183847` (`Build-NimbyPack.ps1 -Dev`, default boot check, port 2402, fresh `storage_999`), `SUMMARY PASS`, `SELFTEST DevHarness ran=yes cases=20 pass=20 fail=0` (19 before), `SELFTEST TOTAL pass=866 fail=0`; `PASS admin-mirror-forges-with-the-connect-identity detail=sender is the connect object=true kept=true respawn kept=true a different sender opens a new server session=true reconnect moves both=true`. That build also carries the A3 debug check (server half only). `Test-NimbyRepo.ps1`: `PASS: 0 failure(s)` (`logs\static-20261101-184011`). The first try (`-183753`) failed to compile on a local named `debug` in the A3 check, renamed `debugOn`.

Status: done in code; the in-game proof is A2.

### 0.2. A2 - harness rerun on the current build

A1 changes the identity behind every forge, so the whole admin set reruns: non-admin forged teleport, request aimed at another player's body, the malformed six, the duplicate id, and the burst. The build carries A1, the A3 debug check and every A4 fix, so the session also exercises the A4 code paths in game.

Profile `profiles\nimby-admin-p0\` (git-ignored), as in Phase 0: `NimbyAdmin\debug` and `NimbyShared\debug` present, `config.json` with the bootstrap owner and `webhook=set` (the duplicate case's one teleport posts one message to #admin-audit), `permissions.json` from the Phase 0 bootstrap. The forge file now holds `admin` and `admin-owner=76561198354705597` only; `sandstorm` was removed, since that visual moved to Phase 3.

Build under test: `build\@NIMBY-20261101-190457` plus `@NIMBY-Server-20261101-190457` and `@NIMBY-Dev-20261101-190457` (`Build-NimbyPack.ps1 -ServerPath D:\DayZServer -BootInstanceId 998 -Dev -Qualification`, from `3cc5e19`): `SUMMARY PASS`; `BOOT MODE qualification storageAutoFix=0 storage=D:\DayZServer\mpmissions\dayzOffline.nasdara\storage_998`; Admin 69/69, AdminServer 132/132, DevHarness 20/20, `SELFTEST TOTAL pass=866 fail=0 flags=10 expected=10`. `Test-NimbyRepo.ps1`: `PASS: 0 failure(s)` (`logs\static-20261101-190558`). Client and dev folders mirrored to `\\purgatory\server\nimby\` (`logs\seat-mirror-purgatory-20261101-190457.txt`, 19 and 4 files).

Evidence owed from the session: the `[Forge] admin run: mirrored sessions` line; the five `[NIMBY][DevHarness][Forge]` verdict lines (non-admin teleport, aimed at another body, the malformed six, duplicate, burst); the matching audit records by id in `audit-` and `audit-denied-2026-11-01.log`; `duplicate id=` once; and no new `session=` number for either player from a forge (the A3 `rpc` lines show `same.session=true`).

Status: built; waiting on Jeff's two-client session.

### 0.3. A3 - identity-keyed sessions check

Question: does the engine hand the same `PlayerIdentity` object for one connection at `InvokeOnConnect`, at every `OnRPC` sender, and from `PlayerBase.GetIdentity()`, across a session, a reconnect, and a death and respawn?

Why it matters: `HandleRequest` calls `SessionFor(sender)`, which opens a new session and resets that player's replay window whenever the sender object differs from the stored one. If real traffic can ever present a different object, an id that already ran can run again.

The engine path, 164200 (`5_Mission/DayZ/mission/missionServer.c`):

| Event | Hook | Calls `InvokeOnConnect` | Body |
| --- | --- | --- | --- |
| New character (first join, and after a respawn) | `OnEvent` `ClientNewEventTypeID` (:326-343), `OnClientNewEvent` (:549) | Yes, with the event's identity (:335-336) | The new `PlayerBase` |
| Existing character from the database | `OnEvent` `ClientReadyEventTypeID` (:345-364), `OnClientReadyEvent` (:599) | Yes (:357) | The loaded `PlayerBase` |
| Respawn chosen (dead or unconscious) | `OnEvent` `ClientRespawnEventTypeID` (:366-378), `OnClientRespawnEvent` (:611) | No; the new body follows as a `ClientNewEvent` | The old body |
| Reconnect to the old character | `OnEvent` `ClientReconnectEventTypeID` (:380-393), `OnClientReconnectEvent` (:633) | No | The old body; the reconnect cache is disabled in vanilla (:421-422, :667-671, :699-700) |
| Disconnect | `OnClientDisconnectedEvent` (:641), then `PlayerDisconnected` (:690) after the logout timer, or at once when dead or without a hive (:679-687) | No | `PlayerDisconnected` notes "identity can be already deleted" (:692) |

Pre-evidence, from the `-163708` server log (section 0.1): one player's `PlayerBase.GetIdentity()` gave one object across harness steps 1 to 4 that was not the object of that player's real requests, and real requests from one connection kept one session all day (D at `session=1` from 17:15 to the harness forge). So a real sender appears to match the `InvokeOnConnect` object and the body's `GetIdentity()` appears not to. A3 measures each point directly.

Measurement (`a939348`, debug only, `$profile:NimbyAdmin\debug`): one line per point, `[NIMBY][Admin][Debug] identity check at=<connect|rpc|respawn|reconnect|disconnect> player=(<bi-id>, <steam64>) "<name>" session=<n|none> same.session=<true|false|none> same.body=<true|false|none> same.bodyTwice=<true|false|none> playerId=<n>`. `same.session` compares the object at that point with the stored session's object (before `SessionFor` runs, so a `false` at `connect` or `rpc` is the moment a new session would open); `same.body` compares it with the body's `GetIdentity()`; `same.bodyTwice` compares two `GetIdentity()` calls on one body; `playerId` is the engine's per-connection id (`PlayerIdentityBase.GetPlayerId`, `3_Game/DayZ/gameplay.c:372`, "is reused after player disconnects"). Points: `InvokeOnConnect` (`OnConnect`), every request after the ownership check (`HandleRequest`), `OnClientRespawnEvent`, `OnClientReconnectEvent`, `PlayerDisconnected`. Nothing in the check changes a session.

Session steps for Jeff, in the A2 session after the harness run: L dies (any way, for example a long fall) and presses Respawn; D presses Refresh in the menu after each of L's steps; L disconnects and rejoins after the logout timer; optionally, L disconnects and tries to rejoin at once, inside the timer. Every spawn sends a hello, so each new body gives an `rpc` line.

Status: instrumented and built; waiting on the A2 session.

### 0.4. A4 - argument-slot audit of the shipped code

The behavior: on 164200 a call inside an argument list can overwrite another argument of the same call (`Forge(s_Other, s_Other.GetIdentity(), ..., TeleportArgs(s_Owner))` delivered the owner's identity as the sender, Phase 0 gate rerun 1, fixed in `23b4e46`). The exact engine rule is not known; the observed case had the same engine function (`GetIdentity`) called once as an argument and again inside a later argument. Two calls of different functions inside one string argument came out right in the same run (`[Forge] sent id=1000002 ... sender=(L) target=(D)` at 17:16:04), and every gate row that depends on the router's values passed, so no shipped call is known to have produced a wrong value. The audit treats every call inside an argument list as exposed and fixes it by computing locals first.

Method: `argscan.py` (seat scratchpad) lists every call whose argument list contains another call (comments and strings blanked; `new X()` counts as a call), over every `.c` file of `NimbyAdmin` and `NimbyAdminServer`. At `a939348`: 333 calls, 102 in production code (16 files) and 231 in the two self-test files. Beyond the scan, the router's two same-function double calls inside one expression in a decision were split as well: the ownership check `owner.GetId() != sender.GetId()` and the in-vehicle check (both `core.c`, `87f0016`).

"Wrong value" column: **Actor** (could change who acts or what the router believes about the sender), **Target** (who or what is targeted), **Audit** (what is written to the audit journal, or whether a record is written), **No** (log, Discord, debug, client display, or the self-test gate's count; named).

| Call (file:line at `a939348`) | Wrong value |
| --- | --- |
| `nimbyadminserver_core.c:278` `Debug` (session ended) | No: debug line |
| `core.c:324` `Gated` (sender-not-owner line) | No: log |
| `core.c:330`, `:333` `s_Rates.Find`, `Set` (`sender.GetId()`) | Actor: the rate bucket a sender spends |
| `core.c:338`, `:344`, `:371` `Gated` (rate-limited, unreadable, duplicate) | No: log; the gate key decides only which line is held back |
| `core.c:380` `NimbyAdminHighWater.Publish` | No: read only by the dev harness |
| `core.c:429` `Gated` (reject line) | No: log |
| `core.c:431` `DenialRecordExempt(RoleOf(...))` | Audit: whether a denied or invalid record is written |
| `core.c:432` `DenialRecordAllowed(DenialBucketFor(...), ...)` | Audit: whether the record is written, and which sender's bucket pays |
| `core.c:434` `ActorRecord(..., CutBytes(command), ...)` | Audit: the record's `command` |
| `core.c:477` `Info(SummaryLine(...))` | No: log |
| `core.c:516` `Info` (permissions reloaded) | No: log |
| `core.c:539` `PlayerBase.Cast(session.identity.GetPlayer())` (`PushRole`) | Target: which body receives the role push |
| `core.c:549` `RoleSyncLine(..., WhoFrom(...), RoleSource(...), ...)` | No: log (the role sync line, with both ids) |
| `core.c:594` `PlayerBase.Cast(...GetPlayer())` (`HandlePlayers`) | No: list content (alive and distance per row) |
| `core.c:604` `vector.Distance(here, player.GetPosition())` | No: list distance |
| `core.c:622` `Debug` (players list) | No: debug line |
| `core.c:635` `s_Sessions.Find(args.Get("target"), ...)` | Target: the target session |
| `core.c:640` `SessionMatches(number, args.Get("session"))` | Target: the stale-target check |
| `core.c:645` `PlayerBase.Cast(targetSession.identity.GetPlayer())` | Target: the target body |
| `core.c:671` `target.AddVector("position", targetPlayer.GetPosition())` | Audit: `target.position` |
| `core.c:673`, `:674` `argsJson.Add(..., args.Get(...))` | Audit: `args.target`, `args.session` |
| `core.c:680` `Info` (request failed, audit unavailable) | No: log |
| `core.c:701`, `:703` `Info` (result not written, teleport done) | No: log |
| `core.c:704` `Enqueue` (teleport Discord text) | No: Discord text |
| `core.c:744` `IsBoxColliding(center, target.GetOrientation(), ...)` | Audit: the destination, so `effects.to` and `effects.offset` |
| `nimbyadminserver_rules.c:216` `IsDigits(args.Get("session"))` | Target: the session argument check |
| `server rules.c:593` `url.Substring(host.Length(), ...)` | No: the webhook path check |
| `server rules.c:659`, `:708`, `:722`, `:726`, `:730`, `:734`, `:787`, `:802` `notes.Insert` | No: store and config notes (log, Discord alert) |
| `server rules.c:954` `m_Ids.Get(LowestIndex())` | Actor: the replay window floor, so whether an id is stale |
| `server rules.c:1084`, `:1094` `entry.AddInt`, `effects.AddInt` | Audit: `audit.suppressed` counts |
| `nimbyadminserver_audit.c:86`, `:91`, `:96`, `:101` `Raw(key, ...)` | Audit: every JSON field value, positions included |
| `audit.c:181` `WriteLine(path, record.ToJson(), record.phase == "intent")` | Audit: the line written and whether it is read back (the intent gate) |
| `nimbyadminserver_discord.c:46` `OnFinished(..., ErrorClass(...), ...)` | No: Discord retry class |
| `discord.c:240` `s_Queue.Add(DiscordPayload(text))` | No: Discord payload |
| `discord.c:245`, `:296`, `:302`, `:305` log lines | No: log |
| `discord.c:282` `context.POST(callback, WebhookRequestPath(...), payload)` | No: Discord request path |
| `nimbyadminserver_files.c:207`, `:238` `m_Io.Note` | No: store notes |
| `files.c:243` `Recover("invalid (" + CutBytes(...) + ")", problem)` | No: recovery reason text (log, Discord) |
| `nimbyadminserver_missionbase.c:30` `Print` (AdminServer completion line) | No: the self-test gate's `cases=` and `failures=` |
| `nimbyadminserver_missionserver.c:25` `Tick(GetTickTime())` | No: Discord and summary timing |
| `nimbyadmin_log.c:53` `WhoFrom(GetName(), GetId(), GetPlainId())` | No: log, but it is the two ids every player line carries (shared rules 4.2) |
| `log.c:58` `Escape(Safe(name))`; `:78` `bytes.Insert(ByteAt(...))` | No: log text |
| `nimbyadmin_rules.c:234` `Escape(m_Values.Get(key))` (`Encode`) | Target: the args the client sends, so the target it names |
| `client rules.c:395` `pair.Substring(eq + 1, pair.Length() - eq - 1)` (`Parse`, run on the server) | Target: every value the server reads from a request |
| `nimbyadmin_playerbase.c:58`, `:84` log lines | No: client log |
| `nimbyadmin_menu.c:75`, `:177`, `:240`, `:354`, `:362`, `:394` | No: client display |
| `nimbyadmin_pauseentry.c:25`, `:41` `MissionGameplay.Cast(GetMission())` | No: client display |
| `nimbyadmin_missionbase.c:31`, `:33` `Register(new ...)`; `:32`, `:34`, `:36`, `:38`, `:40` `Case` | No: shell self-test verdicts |
| `missionbase.c:66` `Print` (Admin completion line) | No: the self-test gate's count |
| `nimbyadmin_missiongameplay.c:48`, `:49` `Register(new ...)` | No: pause entry and Players page registration |
| `missiongameplay.c:50`, `:70`, `:102`, `:107`, `:117`, `:135`, `:149`, `:165`, `:187`, `:196`, `:199`, `:316` log lines; `:140` `NimbyAdminStatus` | No: client log and status line |
| `missiongameplay.c:217` `PlayerBase.Cast(GetGame().GetPlayer())` | No: which local body sends (the client's own) |
| `missiongameplay.c:238` `m_NimbyAdminList.Begin(requestId, GetTickTime())` | No: the list timeout |
| `missiongameplay.c:247` `args.Set("session", target.session.ToString())` | Target: the session number the client sends |
| `missiongameplay.c:248` `NimbyAdminSend(CMD_TELEPORT_TO, args.Encode())` | Target: the args the client sends |

Self-test files (231 calls; a wrong value could change only a case's verdict, so a defect could hide behind a false PASS):

- `nimbyadmin_selftest.c`, 91 calls. `list.players.Insert` (2): 79, 80; `after.players.Insert` (2): 85, 86; `gone.players.Insert` (1): 90; `Case` (59): 101, 102, 103, 104, 105, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 150, 154, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 178, 179, 180, 202, 203, 206, 207, 209, 216, 217, 219, 221, 223, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238; `NimbyAdminRules.CheckEnvelope` (2): 120, 121; `NimbyAdminArgs.Parse` (1): 160; `ParseIs` (4): 168, 169, 170, 171; `NimbyAdminLog.Safe` (1): 232; `Units` (19): 234, 234, 234, 235, 235, 235, 235, 235, 235, 235, 235, 236, 236, 236, 237, 237, 237, 237, 237.
- `nimbyadminserver_selftest.c`, 140 calls. `Case` (122): 66, 67, 68, 69, 70, 71, 72, 88, 89, 90, 91, 92, 99, 103, 117, 122, 123, 124, 125, 126, 162, 163, 168, 169, 170, 171, 172, 173, 178, 179, 180, 181, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 209, 210, 211, 212, 213, 214, 215, 216, 217, 227, 228, 233, 237, 238, 239, 240, 245, 273, 275, 288, 295, 311, 313, 348, 349, 350, 357, 370, 396, 397, 398, 399, 400, 401, 406, 408, 410, 412, 413, 443, 446, 449, 450, 451, 454, 460, 461, 466, 467, 469, 510, 518, 533, 548, 557, 566, 567, 576, 579, 599, 602, 608, 631, 652, 659, 676, 683, 685, 688, 691, 723, 724, 725, 726, 727, 728, 729, 730, 736; `file.admins.Insert` (5): 154, 155, 156, 157, 158; `bans.bans.Insert` (3): 183, 184, 185; `NimbyAdminServerRules.IsIsoUtc` (1): 210; `NimbyAdminServerAudit.IsLastRecord` (1): 239; `NimbyAdminServerAudit.IsLastRecord(denied.Substring` (1): 239; `NimbyAdminServerRules.DiscordPayload` (1): 244; `NimbyAdminServerAudit.PathFor` (4): 348, 348, 349, 349; `many.Add` (1): 664; `NimbyAdminServerSuppressed.SummaryLine` (1): 687.

Fixes, one commit per file, each after a green `-Dev` build (`Build-NimbyPack.ps1 -Dev`, default boot check, port 2402, fresh `storage_999`):

| File | Calls | Commit | Green build |
| --- | --- | --- | --- |
| `NimbyAdminServer/.../nimbyadminserver_core.c` | 30, plus the two split comparisons | `87f0016` | `@NIMBY-20261101-184344` |
| `.../nimbyadminserver_rules.c` | 13 | `d209603` | `@NIMBY-20261101-184547` |
| `.../nimbyadminserver_audit.c` | 5 | `8a234fd` | `@NIMBY-20261101-184841` |
| `.../nimbyadminserver_discord.c` | 7 | `f091709` | `@NIMBY-20261101-184949` |
| `.../nimbyadminserver_files.c` | 3 | `6b7bf7b` | `@NIMBY-20261101-185051` |
| `.../nimbyadminserver_missionbase.c` | 1 | `8ea2c8b` | `@NIMBY-20261101-185153` |
| `.../nimbyadminserver_missionserver.c` | 1 | `ac2bd29` | `@NIMBY-20261101-185256` |
| `NimbyAdmin/.../nimbyadmin_log.c` | 3 | `c4ac203` | `@NIMBY-20261101-185358` |
| `.../nimbyadmin_rules.c` | 2 | `785ec9d` | `@NIMBY-20261101-185500` |
| `.../nimbyadmin_playerbase.c` | 2 | `cdc2fe3` | `@NIMBY-20261101-185603` |
| `.../nimbyadmin_menu.c` | 6 | `b3dd63d` | `@NIMBY-20261101-185704` |
| `.../nimbyadmin_pauseentry.c` | 2 | `f31998a` | `@NIMBY-20261101-185807` |
| `.../nimbyadmin_missionbase.c` | 8 | `f2023e4` | `@NIMBY-20261101-185909` |
| `.../nimbyadmin_missiongameplay.c` | 19 | `99296d0` | `@NIMBY-20261101-190011` |
| `.../nimbyadmin_selftest.c` | 91 | `7a80171` | `@NIMBY-20261101-190143` |
| `.../nimbyadminserver_selftest.c` | 140 | `3cc5e19` | `@NIMBY-20261101-190312` |

Every build: `SUMMARY PASS`, `SELFTEST Admin ran=yes cases=69 pass=69 fail=0`, `AdminServer 132/132`, `DevHarness 20/20`, `TOTAL pass=866 fail=0`. Case counts did not change; a refactor that broke a case's value would show as a FAIL or a missing case, and none did. A rescan at `3cc5e19` finds 0 calls with a call in their argument list in either PBO.

Not covered: the dev harness outside the admin forge path (A1 fixed that path; the Legs and Hands online cases and the sandstorm block still have nested calls, and they decide nothing on a server), and `NimbyShared`, which this branch must not edit (shared rules 8.1). The rule for Phase 1 code: no call inside an argument list, and no two calls of one function in one expression that decides something.

Status: done; the in-game run of the changed paths is A2.
