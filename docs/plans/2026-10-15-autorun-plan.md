# Plan 1 - Legs (`NimbyAutorun`)

Draft v0.8, October 20, 2026. Owner: Jeff Fabish. Implementer: a Claude Code session in worktree `feat/legs-swim` (rounds 3 and 4; rounds 1 and 2 were `feat/autorun`, merged in `f7a50fe`; round 3 merged in `cfc9a92`), coordinated by the orchestrator. Status: round 4 built in `@NIMBY-20261019-213201`; the round 3 water run (R3-2) stopped on the vanilla `swimming` input exclude before it reached the override question, and R4-1 to R4-4 repeat it; Phase 2 two-player rows still owed.

Changes since v0.7: round 3 merged to `main` in `cfc9a92` and ran twice on the replica with `@NIMBY-20261019-205139`. R3-2 (`stopOnSwim` false; server `config requestsPerSecond=10 stopOnSwim=false` at 21:19:23, client `Settings received ... stopOnSwim=false` at 21:21:33): on entering the water the client logged `Swim ... entered=true running=true idx=2` and, in the same frame, `Toggle ... running=false ... cause=control-disabled` (21:22:32.305), then four `Start refused ... cause=control-disabled` while swimming, so the run never reached the override question. Cause: vanilla `PlayerBase.OnCommandSwimStart` adds the `swimming` input exclude group (`PlayerBase.c:4248`; `OnCommandSwimFinish` removes it at `:4263`), and `MissionGameplay.IsControlDisabled()` is true while any exclude group, any restriction, or the legacy mode is active (`missionGameplay.c:1119-1131`). The `swimming` group excludes only `UATempRaiseWeapon` and its gamepad twin (`bin/specific.xml:407-410`), so movement inputs stay live. Round 4 (`4d59bfe`): the engine-free rule `NimbyAutorunRules.ControlDisabledForAutorun` counts a restriction, the legacy mode, or any exclude group other than `swimming` as disabled controls, and the `swimming` group alone only while `stopOnSwim` is true. The client feeds it from `m_ActiveInputExcludeGroups`, `m_ActiveInputRestrictions`, and `m_ControlDisabledMode` at both call sites, the stop (`NimbyAutorunMenuCause`) and the start refusal (`NimbyAutorunStartRefusal`). With `stopOnSwim` true nothing changes. Self-test 100 to 108 cases. The branch was fast-forwarded to `main` (`f2e86c2`) before the change, so the round 4 build carries the NIMBY Controls tab (NIMBY-T-007) and shared rules v1.3 lists `6550103`. The test record gains a round 4 table, and R3-2 carries the run note.

Changes since v0.6: the Jay session of October 19, 2026 (`@NIMBY-20261019-165705`, Jeff solo at the dam spawn, Jay watching) passed the stops, the A5 guard, and the A6 cycle, and raised NIMBY-T-006: autorun stops on entering water, and Jay and Jeff want it to carry through. Jeff ruled to run the feasibility experiment first. Round 3 (`b8ed4b0`): the new `config.json` key `stopOnSwim` (default `true`, the v0.6 behavior) makes the swim stop a server choice, sent to the owning client with the new RPC `6550103` on every start, and a `Swim ... entered=` debug line records each swim transition while autorun is on, so one build answers the question both ways; decision A8 stays open until the dam run. NIMBY-T-005 fixed: the forced-clear line names the player from a cache filled at connect, reconnect, and each request. The config moves to version 2 and its start line always shows `stopOnSwim`. The pack README test step now describes the double tap, the A5 stops as built, A6, and A7. NIMBY-T-007 (the Controls tab is NIMBY, proven October 19, 2026 evening on `feat/inputs-sorting`): this plan, the README, and `NOTICE` say Options > Controls > NIMBY; `Data/Inputs.xml` itself changes only on that branch. Self-test 84 to 100 cases. The test record gains a round 3 table.

Changes since v0.5: the solo block of October 19, 2026 ran on `@NIMBY-20261019-152515` (27 rows filled; NIMBY-T-004, a saved binding from an earlier build survives the new default). Jeff's rulings the same day: A2 decided (lean stays a stop input) and the P0-3 expectation corrected (lean stops autorun at sprint and at run on 1.30); P0-5 ruled (the October 16 replica control is the Legs control); A5 now says Left Ctrl changes speed and does not stop. A6 (return to sprint after stamina recovers) and A7 (no start while the weapon is raised) added and built (`29edf0a`, 84 self-test cases). NIMBY-T-004 documented in `NOTICE` and the pack README (`03eb21a`). The test record gains a round 2 re-run table.

Changes since v0.4: Phase 0 items 1, 2, and 4 done (baseline `docs/baselines/2026-10-19-legs-phase0-baseline.md`; the API evidence table below, with three CHANGED rows: 1.30 has no `UAJump`, Space is `UAGetOver`; `OverrideMovementSpeed`/`OverrideMovementAngle` take a `HumanInputControllerOverrideType`; vanilla locks lean while sprinting, the reference-side cause of T-001; quick and qualification boots both pass); Phase 1 built on `feat/autorun` (`0d2e6e5`, `NOTICE` `685e2fe`) and boot-checked on 1.30 Nasdara; current-state table rewritten for the Phase 1 source; Phase 1 implementation notes record the choices the plan left open (pass-through menus, the double-tap guard, ladder and water, the server backstop).

Changes since v0.3: the stop rule is stated as taking control back (A5, Jeff); key bindings are player-configurable per shared rules 5.

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30.164078 / Nasdara.
Release: independent, gated (see [shared rules, 9.2](2026-10-15-shared-rules.md)).

Legs lets a player toggle continuous forward movement with one key, using vanilla movement and stamina rules. It must stop on its own whenever player state or input makes continued movement inappropriate, and a stopped state must be the same on the client and the server.

Changes since v0.2: shared rules moved to their own file; current-state and decisions tables restored; every test row names its evidence; lean reverts to a conditional requirement (A2); the server cancellation RPC is reserved; the debug-log mechanism and self-test are specified; display name set to Legs (A1).

## What this is not

- Not a movement cheat. The server validates every request; autorun never moves faster than the player could on foot and never runs in a vehicle, unconscious, restrained, or dead.
- Not vehicle cruise control.
- Not a change to movement speed or stamina balance.
- Not a rewrite. The Apache-2.0 fork, its attribution, and its server-side validation stay.

## Current state (verified October 20, 2026, `feat/legs-swim` at `4d59bfe`)

Script paths below are under `mods/own/nimby-pack/NimbyAutorun/Scripts/`.

| Item | State | Source |
| --- | --- | --- |
| Source | `config.cpp` (3_game, 4_world, 5_mission modules), `Data/Inputs.xml`, five scripts, `NOTICE`, `LICENSE` | Repo |
| Origin and license | Fork of SchanaModAutorun commit 1048f71, Apache-2.0; `NOTICE` lists every change since October 14, newest first | `NimbyAutorun/NOTICE` |
| Engine-free rules | Validation, server and client speed rules (A6 sprint return), the ladder and swim rule (`stopOnSwim`), the disabled-controls rule (the vanilla `swimming` exclude group counts only while `stopOnSwim` is true), start refusal order (A7), menu and focus stop rule, double-tap guard, request throttle with stop bypass, log gate, config clamp, the cached-identity log form; 108 self-test cases | `3_game/nimbyautorun_rules.c` |
| Config | `$profile:NimbyAutorun\config.json`, `version` 2, `requestsPerSecond` 10 (clamped 1-100), `stopOnSwim` `true`; written with defaults when absent; a version 1 file is rewritten once at version 2 with its values kept and `stopOnSwim` at its default; the start line `config requestsPerSecond=<n> stopOnSwim=<bool>` is always written | `3_game/nimbyautorun_config.c:16-27, 119-161` |
| Server validation | Ownership from the authenticated sender before the payload is read; movement index and angle; dead, unconscious, restrained, or in a vehicle refuses; sprint resolves to run when `CanSprint()` is false | `4_world/nimbyautorun_playerbase.c:166-240` |
| Server rate limiting | 10 requests per second per player; a stop bypasses it; the first drop in a window stops both sides. Rejection and throttle log lines each have their own 10 s gate | `nimbyautorun_playerbase.c:157-164, 254-288` |
| Server-to-client cancellation | `6550102`, `Param1<string reason>`, to the owner only, on every forced clear and every refused or dropped running request | `nimbyautorun_playerbase.c:242-252, 290-326` |
| Server-to-client settings | `6550103`, `Param1<bool stopOnSwim>`, to the owner only, on every accepted start (not on speed changes) | `nimbyautorun_playerbase.c:233-237, 302-312, 440-454` |
| Server forced clears | Hooks: vehicle, unconscious, restrained, dead, disconnect, delete. Backstop in `CommandHandler` (`via=tick`) also re-evaluates `CanSprint`. The line names the player from the live identity or, once it is gone, from the cache (NIMBY-T-005) | `nimbyautorun_playerbase.c:314-326, 339-421` |
| Client interrupts | A5 list (toggle, WASD, Space, fire, raise, lean per A2, any Shift press), state (dead, unconscious, restrained, vehicle), ladder, water only while the server's `stopOnSwim` is true, focus, dialog, any menu but chat, gestures, and the quickbar, disabled controls (any input exclude group, restriction, or the legacy mode; the vanilla `swimming` group alone only while `stopOnSwim` is true) | `5_mission/nimbyautorun_missiongameplay.c:102-204`; `nimbyautorun_rules.c:207-250` |
| Client swim evidence | `Swim player=... entered=<bool> running=true idx=<n>` once per swim transition while autorun is on (debug flag) | `nimbyautorun_missiongameplay.c:153-171, 233-234` |
| Client speed and start | Sprint drops to run when sprint or stamina runs out and returns when vanilla's held-Shift gate allows again, unless Left Ctrl chose walk or run since the start (A6). A start is refused while the weapon is raised, after the other causes in `StartRefusal` order (A7) | `nimbyautorun_missiongameplay.c:94-100, 206-215, 241-272`; `nimbyautorun_rules.c:180-282` |
| Client reset | Player replacement or respawn (new object), `OnMissionFinish` (disconnect, teardown), RPC `6550102`; the reset also clears the A6 speed choice and the swim tracker. A new player object starts with `stopOnSwim` true until its first accepted start | `nimbyautorun_missiongameplay.c:62-81, 222-230`; `nimbyautorun_playerbase.c:425-438` |
| Default key | Double tap Left Shift, `doubleclick` event, Options > Controls > NIMBY, "Legs: toggle autorun" (the `nimby` sorting inside `<inputs>`, NIMBY-T-007, merged to `main` in `2791281` and on this branch since the fast-forward to `f2e86c2`) | `Data/Inputs.xml` |
| RPC ids | `6550101` toggle, `6550102` cancel, `6550103` settings (round 3; inside the Legs block, listed in shared rules 4.1 since v1.3, `f2e86c2`) | `3_game/nimbyautorun_rules.c:12-25` |
| Save footprint | None; no `OnStoreSave`/`OnStoreLoad`, no net-synced variables; the identity cache is in memory | Grep |
| Player id in logs | `GetId()` and `GetPlainId()` on every line that names a player; server lines after the identity is gone use the name and ids cached at `OnConnect`, `OnReconnect`, and each request | `3_game/nimbyautorun_config.c:53-84`; `nimbyautorun_playerbase.c:54-78, 371-384` |
| Last build | `build\@NIMBY-20261019-213201` (quick boot, `storageAutoFix=1`), 1.30 Nasdara, `SUMMARY PASS`, Autorun 108/108 (the log counts each case twice, `pass=216`), pack total `pass=500 fail=0`, 0 `SCRIPT (E)` | `logs\build-20261019-213201\build.log` |
| Solo test (1.29) | Toggle, WASD, fire, raise, stamina drop: PASS. Lean: FAIL (NIMBY-T-001). Vehicle and unconscious recovery: NOT RUN (NIMBY-T-003) | `docs/testing/2026-10-14-solo-ingame-test.md` |
| On 1.30 in game | Solo block and round 2 re-run, October 19, 2026: Phase 1 PASS, A6 and A7 PASS. Jay session the same evening (`@NIMBY-20261019-165705`): stops, guard, A6 cycle PASS; swim stop as designed, then NIMBY-T-006. Round 3 on the replica (`@NIMBY-20261019-205139`, from `main` at `cfc9a92`): R3-2 stopped with `cause=control-disabled` from the vanilla `swimming` exclude group (fixed in round 4); the other R3 results are the orchestrator's to record. Round 4 rows R4-1 to R4-4 not yet run | `docs/testing/2026-10-19-legs-phase1.md` |

## API evidence (Phase 0 item 2, October 19, 2026)

Paths are under `vendor/dayz-1.30-scripts/` (build `1.30.164078.38`, `scripts.pbo` hash in the baseline). VERIFIED means the declaration exists as used; CHANGED means 1.30 differs from what the October 14 source assumed, and the fix is named. A declaration is not proof of behavior: the in-game rows in the test record exercise each one.

| Symbol | Status | Evidence | Note |
| --- | --- | --- | --- |
| `MissionGameplay.OnInit` | VERIFIED | `scripts/5_Mission/DayZ/mission/missionGameplay.c:102` | Stop-input list built here |
| `MissionGameplay.OnUpdate` | VERIFIED | `missionGameplay.c:287` | Client loop |
| `MissionGameplay.OnMissionFinish` | VERIFIED | `missionGameplay.c:266`; base `scripts/3_Game/DayZ/gameplay.c:702` | Client teardown reset |
| `MissionBaseWorld` (`Mission.OnInit`) | VERIFIED | `scripts/4_World/DayZ/Classes/MissionBaseWorld.c:1`; `gameplay.c:700` | Debug flag, config, self-test; Tack's pattern |
| `IsControlDisabled` | VERIFIED | `missionGameplay.c:1119-1131`; base `gameplay.c:792` | True for any input exclude or restriction, so chat (`"menu"`, `ShowChat` `:1242-1253`) and gestures and the quickbar (`"radialmenu"`, `:667-675`) raise it, and so does every swim start (`"swimming"`, rows below). Legs passes those three menus through and allows a 0.5 s grace after they close, because the gesture exclude is removed in the menu destructor (`scripts/5_Mission/DayZ/GUI/GesturesMenu.c:143-148`). Since round 4 Legs does not call it: it reads the same three facts through `NimbyAutorunRules.ControlDisabledForAutorun`, so the `swimming` group alone can be ignored while `stopOnSwim` is false |
| `MissionGameplay.m_ActiveInputExcludeGroups`, `m_ActiveInputRestrictions`, `m_ControlDisabledMode`; `INPUT_EXCLUDE_ALL` | VERIFIED | `missionGameplay.c:38-40` (protected members, readable from the modded class); groups added at `:1008-1017`, restrictions at `:1064-1070`; legacy mode set at `:926`, reset to -1 at `:874`, `:1098`; `scripts/3_Game/DayZ/constants.c:691` (`INPUT_EXCLUDE_ALL = 0`) | Round 4 inputs to `ControlDisabledForAutorun`: the same three facts `IsControlDisabled` reads, with `Find("swimming")` on the groups |
| Swim exclude: `PlayerBase.OnCommandSwimStart`, `OnCommandSwimFinish` | VERIFIED | `PlayerBase.c:4217` and `:4248` (`AddActiveInputExcludes({"swimming"})`); `:4251` and `:4263` (removes it) | The R3-2 cause: `cause=control-disabled` in the same frame as `Swim ... entered=true` (NIMBY-T-006 round 4) |
| `swimming` exclude group | VERIFIED | `bin/specific.xml:407-410`: `UATempRaiseWeapon`, `UATempRaiseWeaponGamepad` only | Movement inputs stay live while swimming, so the group does not mean the player lost control |
| `IsPaused` | VERIFIED | `missionGameplay.c:1294-1297` | `MENU_INGAME` open; Legs reads the menu id directly |
| `GetGame().GetUIManager()`, `UIManager.GetMenu`, `IsDialogVisible` | VERIFIED | `scripts/3_Game/DayZ/Global/Game.c:943`; `scripts/3_Game/DayZ/tools/UIManager.c:59, :13` | Input-capturing menu and dialog |
| `UIScriptedMenu.GetID` | VERIFIED | `scripts/3_Game/DayZ/tools/UIScriptedMenu.c:151` | |
| Menu ids | VERIFIED | `scripts/3_Game/DayZ/constants.c:181` (`MENU_CHAT`), `:184` (`MENU_INGAME`), `:186` (`MENU_INVENTORY`), `:192` (`MENU_CHAT_INPUT`), `:197` (`MENU_MAP`), `:200` (`MENU_GESTURES`), `:204` (`MENU_RADIAL_QUICKBAR`) | |
| `Game.IsAppActive` (focus) | VERIFIED | `Game.c:228`; vanilla use `missionGameplay.c:296` | Focus-loss interrupt |
| `PlayerBase.OnRPC` | VERIFIED | `scripts/4_World/DayZ/Entities/ManBase/PlayerBase.c:5727`; `scripts/3_Game/DayZ/Entities/EntityAI.c:3507` | |
| `Game.RPCSingleParam` | VERIFIED | `Game.c:1013` | `recipient` names the owner for `6550102` |
| `Game.GetTickTime` | VERIFIED | `Game.c:914` | Throttle, log gates, guards |
| `GetUApi`, `UAInputAPI.GetInputByName`, `UAInput.LocalPress` | VERIFIED | `scripts/3_Game/DayZ/InputAPI/UAInput.c:279, :218, :60` | |
| `UAMoveForward`, `UAMoveBack`, `UAMoveLeft`, `UAMoveRight` | VERIFIED | `bin/constants.xml:5, 6, 9, 10`; `bin/preset_X1MouseKey.xml:5-20` (W, S, A, D) | |
| `UAFire` | VERIFIED | `constants.xml:42`; preset `:229-232` (left mouse) | |
| `UATempRaiseWeapon` | VERIFIED | `constants.xml:48`; preset `:295-297` (right mouse) | |
| `UALeanLeft`, `UALeanRight` | VERIFIED | `constants.xml:19-20`; preset `:68-73` (Q, E); `scripts/4_World/DayZ/Entities/DayZPlayerImplement.c:3013-3020` locks both on sprint start, `:3022-3034` unlocks on sprint end | The reference suggested no press while sprinting; in game (P0-3) a lean press stops autorun at sprint and at run, so the lock does not hide `LocalPress` under the override. Lean stays a stop input (A2) |
| `UATurbo` | VERIFIED | `constants.xml:114`; preset `:21-23` (Left Shift hold) | A5 stop input, arms the double-tap guard |
| `UAWalkRunToggle` | VERIFIED | `constants.xml:28`; preset `:27-31` (double tap Left Ctrl) | A3 precedent; slows autorun to walk |
| `UAWalkRunTemp` | VERIFIED | `constants.xml:29`; preset `:24-26` (Left Ctrl) | Sets autorun to run |
| `UAJump` | CHANGED | Not declared anywhere in 1.30 (`bin/` and `scripts/`, 0 matches). Space is `UAGetOver`, `constants.xml:137`, preset `:51-56` | Fix: `UAGetOver` is the Space stop input |
| `kLShift`, `doubleclick` | VERIFIED | `constants.xml:500`; limit 0.5 s `:457`; vanilla Others sorting `:915` | Left Shift also carries `UAADSToggle` as a click (preset `:169-172`), active only with the weapon raised. Since NIMBY-T-007 (October 19, 2026) the entry no longer sorts into Others: `Data/Inputs.xml` declares the `nimby` sorting inside `<inputs>` and the binding shows under Options > Controls > NIMBY (change on `feat/inputs-sorting`) |
| `HumanInputController.OverrideMovementSpeed`, `OverrideMovementAngle` | CHANGED | `scripts/3_Game/DayZ/human.c:253, :256` take `HumanInputControllerOverrideType` (`:7-12`: `DISABLED`, `ENABLED`, `ONE_FRAME`), not `bool` | A `bool` still converts (vanilla `DayZPlayerImplement.c:3865` passes one), but Legs now passes `ENABLED` and `DISABLED` explicitly |
| `Human.GetInputController` | VERIFIED | `human.c:1458` | |
| `PlayerBase.CanSprint` | VERIFIED | `PlayerBase.c:5612-5629` | Raised, heavy item, injury level 3, throwing mode, broken legs; stamina is separate |
| `PlayerBase.CanConsumeStamina(EStaminaConsumers.SPRINT)` | VERIFIED | `PlayerBase.c:4799-4810`; vanilla sprint gate `:3485` (`LimitsDisableSprint(!(CanConsumeStamina(EStaminaConsumers.SPRINT) && CanSprint()))`); consumer rule `StaminaHandler.c:65-103`, sprint threshold `:876-879` (`staminaMinCap + 15`) | A6 stamina test, replacing `GetStamina() > 0`: false once stamina is depleted, true again only above the threshold, so autorun returns to sprint when a held Shift would and does not flicker at zero. Vanilla calls it every frame on the move command, so the extra call changes no state |
| `DayZPlayerImplement.IsRaised` | VERIFIED | `DayZPlayerImplement.c:4005-4008`; set from the movement state in each `CommandHandler` at `:2321` | A7 start refusal `cause=raised` |
| `PlayerBase.GetStaminaHandler`, `StaminaHandler.GetStamina` | VERIFIED | `PlayerBase.c:2042`; `scripts/4_World/DayZ/Classes/StaminaHandler.c:1081` | |
| `IsInVehicle` | VERIFIED | `DayZPlayerImplement.c:468-471` | |
| `IsUnconscious` | VERIFIED | `PlayerBase.c:3925-3928` | |
| `IsRestrained` | VERIFIED | `PlayerBase.c:2219-2222` | |
| `IsAlive` | VERIFIED | `scripts/3_Game/DayZ/Entities/Object.c:517` | |
| `GetCommand_Ladder`, `GetCommand_Swim` | VERIFIED | `human.c:1512, :1520` | Client ladder interrupt; water interrupt only while `stopOnSwim` is true; the swim tracker for the `Swim ... entered=` line |
| Override inside the swim command (NIMBY-T-006) | UNKNOWN | `OverrideMovementSpeed` and `OverrideMovementAngle` are `proto native` (`human.c:253, :256`) with an empty `//!` comment and nothing on which commands consume them. `HumanCommandSwim` exposes only `StopSwimming` and the static `WaterLevelCheck` (`human.c:693-704`); no speed or input hook. Vanilla treats movement index 3 in water as fast swim (`DayZPlayerImplement.c:2070`, the zoom rule `IsSwimming() && m_MovementState.m_iMovement == 3`); `IsSwimming` is `COMMANDID_SWIM` (`:473-476`) | The reference neither says the swim command honors the override nor that it ignores it. Only the in-game rows R3-2 to R3-4 decide. Follow-up, not built: if index 3 in water is the fast swim and drains stamina, the swim entry may need the index capped at 2; A6 already drops 3 to 2 when stamina runs out |
| Identity hooks: `OnConnect`, `OnReconnect` | VERIFIED | `PlayerBase.c:7623, :7638`; called from `MissionServer.InvokeOnConnect` (`missionServer.c:435-440`, from the new-player and ready events at `:336, :357`) and `OnClientReconnectEvent` (`:637`) | NIMBY-T-005 identity cache, server only |
| Vehicle entry hook: `OnCommandVehicleStart` | VERIFIED | `PlayerBase.c:4342`; callback `human.c:1691` | Server forced clear `reason=vehicle` |
| Unconscious hook: `OnUnconsciousStart` | VERIFIED | `PlayerBase.c:3756`, called from `CommandHandler` at `:3381` on both machines | Server only acts (`IsServer`) |
| Restraint hook: `SetRestrained` | VERIFIED | `PlayerBase.c:2206`; server callers `scripts/4_World/DayZ/Classes/UserActionsComponent/Actions/Continuous/ActionRestrainTarget.c:225`, `scripts/4_World/DayZ/Entities/ItemBase/HandcuffsLocked.c:40` | Acts on `true` only |
| Death hook: `EEKilled` | VERIFIED | `PlayerBase.c:1214` | |
| Removal hooks: `OnDisconnect`, `EEDelete` | VERIFIED | `PlayerBase.c:7647` (called by `MissionServer.InvokeOnDisconnect`, `scripts/5_Mission/DayZ/mission/missionServer.c:442-447`); `PlayerBase.c:1393` | Cleared before `super`. The October 19 kill test (R2-7b) showed the identity can already be gone when the removal line is written (NIMBY-T-005), so the line names the player from the cache |
| Server backstop: `CommandHandler` | VERIFIED | `PlayerBase.c:3206` | `via=tick` clears and the `CanSprint` re-evaluation |
| `PlayerIdentity.GetId`, `GetPlainId`, `GetName` | VERIFIED | `gameplay.c:368, :370, :362` | Vanilla's comment on `GetPlainId` reads "plaintext unique id of player (cannot be used in database or logs)" (`:369`); shared rules 4.2 log it anyway. The in-game run proves it is the Steam64 |
| `JsonFileLoader.LoadFile`, `JsonSaveFile`; `FileExist`; `MakeDirectory` | VERIFIED | `scripts/3_Game/DayZ/tools/JsonFileLoader.c:7, :134`; `scripts/1_Core/DayZ/proto/EnSystem.c:397, :525` | |
| `DayZPlayerConstants.MOVEMENTIDX_*` | VERIFIED | `scripts/3_Game/DayZ/dayzplayer.c:657-660` (0 idle, 1 walk, 2 run, 3 sprint) | |

## Decisions

| # | Decision | Choice | Status |
| --- | --- | --- | --- |
| A1 | Display name | Legs. Code prefix stays `NimbyAutorun`; Controls entry reads "Legs: toggle autorun" | Decided October 15, 2026 |
| A2 | Lean interrupt (T-001) | Lean stays a stop input. The solo block (P0-3) showed a lean press stops autorun at sprint and at run on 1.30; the Phase 0 expectation from the reference (vanilla locks lean on sprint start, `DayZPlayerImplement.c:3013-3034`, so no press while sprinting) was wrong. No code change | Decided by Jeff, October 19, 2026 |
| A3 | Default key | Double-tap Left Shift: `<btn name="kLShift"><event name="doubleclick"/></btn>` in `Data/Inputs.xml` (vanilla precedent `UAWalkRunToggle` on `kLControl`, `bin/preset_X1MouseKey.xml:27-31`); players rebind under Options > Controls > NIMBY (the `nimby` sorting replaced the vanilla Others sorting, NIMBY-T-007, proven October 19, 2026). Phase 0 adds a check that the double tap toggles autorun without leaving `UATurbo` (the `kLShift` hold, `:21-22`) in a stuck state. Replaces numpad plus | Decided, revised by Jeff October 15, 2026 |
| A5 | Stop rule | Autorun stops when the player takes control back: any movement key (WASD), a single Shift press (the sprint hold; a double tap while stopped starts autorun, any Shift press while running stops it), Space (jump or vault), fire, weapon raise, lean (per A2), or the toggle itself; and on vehicle entry, unconsciousness, restraint, death, and the menu and focus cases in Phase 1. It does not stop on unrelated input such as chat, gestures, or the inventory hotbar. Left Ctrl while autorunning is a speed change, not a stop: a double tap slows to walk, a single press sets run (P1-14) | Decided by Jeff, October 16, 2026; Left Ctrl confirmed as intended October 19, 2026 |
| A4 | Server cancellation RPC | `6550102`, server to owning client; on receipt the client clears its override and remembered state and does not re-send start | Decided (shared rules 4.1) |
| A6 | Sprint after stamina recovers | Autorun returns to sprint by itself once stamina and `CanSprint()` allow again, as a held Shift would. It never returns to sprint while the player has chosen walk or run with Left Ctrl; that choice holds until the next start or until the player changes it again. Client debug line `Toggle ... running=true idx=3 side=client cause=speed`; the change-only RPC stays | Decided by Jeff, October 19, 2026; built in `@NIMBY-20261019-163450` |
| A7 | Start while the weapon is raised | The double tap does nothing while the weapon is raised; the client logs `Start refused ... cause=raised` (debug flag). Lowering the weapon and double-tapping again starts normally | Decided by Jeff, October 19, 2026; built in `@NIMBY-20261019-163450` |
| A8 | Autorun in water (NIMBY-T-006) | Experiment first: `config.json` `stopOnSwim` (default `true`) chooses whether the swim command stops autorun and refuses a start. With `false`, nothing in Legs reacts to swimming (since round 4 that includes the vanilla `swimming` input exclude group): the override stays applied and every stop input and server clear still works. The R3 rows record whether the swimmer keeps moving under the override or stops with autorun still on. The design decision, and the default, follow the dam run | Open. Experiment ruled by Jeff, October 19, 2026; built in `@NIMBY-20261019-202135`. The round 3 run (`@NIMBY-20261019-205139`) stopped on the vanilla `swimming` exclude group before the question (R3-2); round 4 built in `@NIMBY-20261019-213201` |

## Phase 0 - baseline and test environment

Depends on Plan 0 (shared tooling) being on `main`.

1. Record the client and server build, source revision, and the hashes of the shared 1.30 script reference in `docs/baselines/`.
2. Verify every vanilla symbol the mod touches against the shared reference and record file:line: `MissionGameplay.OnInit`/`OnUpdate`, `IsControlDisabled`, `PlayerBase.OnRPC`, `GetUApi().GetInputByName`, `UAInput.LocalPress`, the `UA*` input names in the interrupt list, `HumanInputController.OverrideMovementSpeed`/`OverrideMovementAngle`, `CanSprint`, `GetStaminaHandler`, `IsInVehicle`, `IsUnconscious`, `IsRestrained`, and the vehicle-entry, unconsciousness, restraint, death, and player-removal hooks the server will use. Exercise each in game; a declaration is not proof.
3. Settle A2: on the test server, hold lean while sprinting and record whether `LocalPress` fires. Record the result in this plan. Result, October 19, 2026: it fires; lean stops autorun at sprint and at run (the expectation of no press while sprinting was wrong).
4. Build and boot on 1.30 Nasdara with the shared tooling; confirm the quick boot check and the qualification run are separate invocations.
5. Establish the same-build vanilla control. Investigate existing copied-world save-load errors separately; do not exempt them automatically.
6. Settle the A3 double tap: bind `UANimbyAutorunToggle` to `kLShift` with `<event name="doubleclick"/>` on the test server and record whether the double tap toggles autorun, whether the two short `UATurbo` presses it also fires leave the sprint hold in any stuck state, and whether a held Shift still sprints normally.

Exit: the signed addon boots on 1.30 Nasdara with 0 `SCRIPT (E)`; every touched API is recorded as VERIFIED (file:line) or CHANGED with its fix; A2 and the A3 double tap are settled.

Status, October 19, 2026: item 1 done (`docs/baselines/2026-10-19-legs-phase0-baseline.md`); item 2 done against the reference (API evidence table above; the in-game rows in the test record exercise each symbol); item 4 done: the quick boot check (`@NIMBY-20261019-152118`, `BOOT MODE quick storageAutoFix=1`) and the qualification run (`@NIMBY-20261019-152515`, `-Qualification`, `BOOT MODE qualification storageAutoFix=0`) are separate invocations, and both pass with 0 `SCRIPT (E)` and 0 "Scripted variables corrupted"; items 3, 5, and 6 need a client and are rows P0-3, P0-5, and P0-6 in `docs/testing/2026-10-19-legs-phase1.md`.

Status after the solo block, October 19, 2026: item 3 done (lean registers at sprint and at run; A2 decided). Item 5 ruled by Jeff: the October 16 replica control boot (`docs/baselines/2026-10-16-replica-vanilla-control-boot.md`, same exe and build `1.30.164078.38`) is the Legs vanilla control; no new boot. Item 6: the stop rule, the guard, the plain sprint, and the rebind pass; P0-6a waits for a re-run after the binding reset (NIMBY-T-004).

## Phase 1 - movement lifecycle and request handling

### Client

- Inputs: change `Data/Inputs.xml` from `kAdd` to `kLShift` with the `doubleclick` event per A3; update the Controls label if the binding text is shown there.
- Preserve the toggle, movement, fire, weapon-raise, and lean (per A2) interrupts.
- Treat `IsControlDisabled()`, inventory, pause, any input-capturing menu, and focus loss as interrupts, not only as start blockers. Require a fresh toggle afterward.
- Clear local overrides and remembered state on player replacement, respawn, disconnect, and mission teardown.
- Handle RPC `6550102` per A4.
- Preserve the change-only RPC behavior; do not restore per-frame requests.

### Server

- Continue deriving ownership from the authenticated RPC sender.
- Validate payload readability, movement values, and allowed player state.
- Re-evaluate sprint eligibility while autorun remains active.
- Clear movement overrides on vehicle entry, unconsciousness, restraint, death, and player removal without depending on a client stop, and send `6550102` to the owning client on every forced clear or rejection that changes state.
- Add bounded request processing per sender (default 10 requests per second, in `$profile:NimbyAutorun\config.json`). Keep rejection-log throttling separate from request throttling. An authenticated stop request bypasses the request throttle.
- Move argument validation and the throttle into engine-free classes and cover them with the self-test (shared rules 8).

### Logging

| Event | Line | Level |
| --- | --- | --- |
| Forced clear | `[NIMBY][Autorun] Forced clear player=<name> (<bi-id>, <steam64>) reason=<vehicle|unconscious|restrained|dead|removed> via=<hook|tick>` | Always |
| Rejected request | `[NIMBY][Autorun] Rejected request from <bi-id>, <steam64>: <reason>` (rate-limited) | Always |
| Request throttled | `[NIMBY][Autorun] Throttled player=... count=<n>` (rate-limited) | Always |
| Toggle on or off | `[NIMBY][Autorun] Toggle player=... running=<bool> idx=<n> side=<client|server> cause=<cause>` | Debug flag only |
| Cancel received (client) | `[NIMBY][Autorun] Cancel received player=... reason=<reason> wasRunning=<bool>` | Debug flag only |
| Start refused (client) | `[NIMBY][Autorun] Start refused player=... cause=<cause>` | Debug flag only |
| Swim transition (client, round 3) | `[NIMBY][Autorun] Swim player=... entered=<true|false> running=true idx=<n>`, once per swim start or end while autorun is on | Debug flag only |
| Settings received (client, round 3) | `[NIMBY][Autorun] Settings received player=... stopOnSwim=<bool>`, when RPC `6550103` changes the client's value (a client starts at `true`, so this line appears only for `false` or a later change back) | Debug flag only |
| Mission start | `[NIMBY][Autorun] loaded; server=<bool> debug=<bool>`; on the server `config loaded|missing|invalid|clamped|upgraded`, then `config requestsPerSecond=<n> stopOnSwim=<bool>` | Always |
| Self-test | `[NIMBY][Autorun][SelfTest] PASS|FAIL <case>`, then `[NIMBY][Autorun] selftest complete; cases=<n> failures=<n>` | Self-test flag only |

`via=hook` is the state hook named in the API evidence table; `via=tick` is the `CommandHandler` backstop and means a hook did not fire first, which is a finding. Client `cause` values: `toggle`, `speed`, an input name (`UATurbo`, `UAMoveForward`, `UAGetOver`, `UAFire`, `UATempRaiseWeapon`, `UALeanLeft`, `UALeanRight`, `UANimbyAutorunToggle`), a state (`dead`, `unconscious`, `restrained`, `vehicle`), `ladder`, `swim` (only while `stopOnSwim` is true), `focus`, `dialog`, `inventory`, `pause`, `menu-<id>`, `control-disabled` (any input exclude group, restriction, or the legacy mode; not the vanilla `swimming` group alone while `stopOnSwim` is false), `player-replaced`, `mission-finish`; start refusals add `raised` (A7) and `double-tap-guard`. Server `cause` values: `client-request`, `client-stop`, `cannot-sprint`. Debug lines need `$profile:NimbyAutorun\debug` on the machine that prints them: the server profile for server lines, the client profile (`%LOCALAPPDATA%\DayZ`) for client lines.

Exit: client and server reach the same stopped state after every interruption, with no automatic resumption, and the self-test passes.

### Implementation notes (October 19, 2026)

Choices the plan left open, made in the Phase 1 build and open to Jeff's review:

- **Pass-through menus.** `IsControlDisabled()` is true whenever any input exclude is active, and chat, gestures, and the radial quickbar each set one (API evidence). A5 says those three never stop autorun, so the client lets them through by menu id and ignores a disabled-control state for 0.5 s after one closes. Every other menu, a dialog, focus loss, and a disabled-control state outside that window stop autorun. While one of the three is open, a start is still refused (`control-disabled`).
- **Double-tap guard.** While running, the first press of a double tap is a `UATurbo` press and stops autorun (A5). Without a guard, the second press would fire the toggle and start it again at once, so a toggle within 0.5 s (the keyboard doubleclick limit) of a `UATurbo` stop is refused (`double-tap-guard`).
- **Shift no longer selects sprint.** The October 14 fork raised speed to sprint on a Shift press while running; A5 makes that press a stop. Since A6 the client returns to sprint by itself when vanilla's held-Shift gate (`CanSprint()` and `CanConsumeStamina(EStaminaConsumers.SPRINT)`) allows again; a Left Ctrl choice since the start keeps the chosen speed. The stamina test uses that vanilla gate instead of `GetStamina() > 0`, so sprint drops at depletion and comes back above the sprint threshold, without flickering at zero.
- **Raised weapon (A7).** `IsRaised()` refuses a start (`cause=raised`). It also makes `CanSprint()` false, so before A7 a double tap while raised started autorun at run; a Shift click while raised is also `UAADSToggle`. The refusal order lives in `NimbyAutorunRules.StartRefusal` and the self-test covers it.
- **Walk and run.** The vanilla walk toggle (double tap Left Ctrl) slows autorun to walk and a single Left Ctrl press sets run, as in the fork. Sprint drops to run only from sprint, so walking no longer turns into running when stamina is low.
- **Ladder and water.** Not in the Phase 1 server list; the client stops on a ladder or swim command (the Phase 2 row) and sends the stop, so the server clears through the normal request. Since round 3 the swim stop depends on the server's `stopOnSwim` (A8); the ladder stop does not.
- **How the client learns `stopOnSwim` (round 3).** The config is read only on the server, and the swim check runs on the client, so the server sends the value with RPC `6550103` (inside the Legs block `65501xx`) right before it applies every accepted start. A new player object starts with `true` until then. Consequence for the experiment: with `stopOnSwim` false, the very first double tap of a new character while already swimming is still refused (`cause=swim`); any start on land first, as the R3 rows do, brings the server value before the player reaches water. The alternative, a net-synced variable on `PlayerBase`, was not used: the plan records no net-synced variables, and an RPC to the owner is the pattern Legs already has.
- **Disabled controls while swimming (round 4).** Vanilla `IsControlDisabled()` cannot tell the `swimming` exclude group from a menu's, so with `stopOnSwim` false R3-2 stopped on it. The client now reads the exclude groups, the restrictions, and the legacy mode itself (`NimbyAutorunControlDisabled`) and `NimbyAutorunRules.ControlDisabledForAutorun` decides. Any restriction, the legacy mode, or any other group still counts, so a menu or the inventory opened while swimming still stops autorun and refuses a start. The `ladderclimbing` group needs no case because the ladder is a movement cause first. With `stopOnSwim` true the `swimming` group counts as before, although the `swim` movement cause is checked first.
- **Identity cache (NIMBY-T-005).** The server keeps the name, BI id, and Steam64 in memory on the player from `OnConnect`, `OnReconnect`, each request, and each forced clear that still has the identity. `NimbyAutorunLog.Who` keeps its signature; the cached form is `NimbyAutorunLog.WhoFrom`, and the self-test covers both.
- **Server backstop.** Besides the hooks, `CommandHandler` clears with `via=tick` if a blocked state is seen while autorun is on, and drops sprint to run when `CanSprint()` turns false. The client does the same from its own `CanSprint()` and stamina; the server's drop sends no RPC.
- **Refusals and drops stop both sides.** A refused running request (bad index or angle, blocked state, unreadable payload) clears the server override and sends `6550102`. A dropped request does the same for the first drop in a window, so a flood is answered with one cancellation, not one per request. A request aimed at another player is refused before it is read and sends nothing.
- **Death is local on the client.** The client stops locally on death and sends nothing: the server clears the body in `EEKilled`, and a stop sent from a body without an identity would log a false "sender does not own this player".

## Phase 2 - multiplayer and failure testing

Jeff and Jay on the local test server. Vehicles come from the test mission's `init.c`, never the live mission; do not depend on Central Scrutinizer.

| Scenario | Required result | Evidence |
| --- | --- | --- |
| Toggle on and off; rebind the toggle | Predictable start and stop | Debug toggle lines show matching `running` values |
| Movement, fire, raise, lean (per A2) | Stops | Debug toggle line `running=false`; no further movement |
| Stamina exhaustion or sprint unavailable | Vanilla-legal movement; no sustained prohibited sprint | Debug toggle line `idx=` changes to RUN; no sprint animation at zero stamina |
| Inventory, pause, input-capturing menu, focus loss | Stops; does not resume on return | `running=false` line at open; no `running=true` line without a key press |
| Car and motorbike entry and exit | Stops and stays stopped | `Forced clear ... reason=vehicle` on the server; player stands still after exit |
| Unconsciousness, restraint, death, respawn | Overrides cleared; fresh toggle required | `Forced clear` with the matching reason |
| Ladder and swimming transitions | Ladder: autorun clears; vanilla controls usable. Water: per A8 after the R3 and R4 rows (with `stopOnSwim` true, autorun clears) | `running=false`; player responds to WASD; with `stopOnSwim` false, the `Swim ... entered=` lines |
| Disconnect and reconnect | No retained override | No `running=true` line after reconnect until a key press |
| Server rejects or forcibly clears | Client reconciles to stopped | Server `Forced clear` or `Rejected` line, then no client re-send within 5 s |
| Latency, rapid toggling, malformed requests | No stuck movement; bounded processing | `Throttled` lines appear; player stops on toggle |
| Request targeting another player (dev harness) | Refused; target unaffected | `Rejected request ... sender does not own this player`; target's state unchanged |

Forged requests run only from the dev harness (shared rules 8). Its addon and key never enter a release.

## Phase 3 - release qualification and rollout

Follow shared rules 9. In addition:

- Apply A1: `mod.cpp` and Controls text read Legs; `NOTICE` lists every change since October 14, newest first.
- Update `mods/own/nimby-pack/README.md`: status "Built and tested in game on 1.30 Nasdara".
- Record the run against the exact artifact in `docs/testing/`.

## Acceptance criteria

- Builds, signs, boot-checks, and self-tests on 1.30 Nasdara with 0 `SCRIPT (E)`.
- Every Phase 2 row passes with the named evidence, witnessed by Jeff.
- No save footprint, re-checked by grep at release.
- The `storage_1`-copy boot in shared rules 9.2 shows 0 "Scripted variables corrupted".
- Any unintended movement after cancellation blocks release.

## Estimate

Re-estimate after Phase 0. The earlier 2.5-day figure excluded shared-tool repairs and multiplayer failure testing and is not a commitment.

## Risks

| Risk | Effect | Mitigation |
| --- | --- | --- |
| 1.30 input or animation changes break the movement override | Autorun does nothing or moves oddly | Phase 0 verification before other work |
| Lean not observable while sprinting | T-001 unfixable as designed | Retired October 19, 2026: lean registers at sprint (P0-3); A2 keeps lean |
| Vehicle test blocked | CR-008 unproven | Test-only spawn in the local mission `init.c` |
| Server and client disagree after a forced clear | Player stuck moving or unable to restart | A4 cancellation RPC; Phase 2 reconciliation row |
