# Plan 1 - Legs (`NimbyAutorun`)

Draft v0.5, October 19, 2026. Owner: Jeff Fabish. Implementer: a Claude Code session in worktree `feat/autorun`, coordinated by the orchestrator. Status: Phase 1 built in `@NIMBY-20261019-152515`, awaiting the solo test block (`docs/testing/2026-10-19-legs-phase1.md`). Phase 0 items 1, 2, and 4 done; items 3, 5, and 6 need a client.

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

## Current state (verified October 19, 2026, `feat/autorun` at `685e2fe`)

Script paths below are under `mods/own/nimby-pack/NimbyAutorun/Scripts/`.

| Item | State | Source |
| --- | --- | --- |
| Source | `config.cpp` (3_game, 4_world, 5_mission modules), `Data/Inputs.xml`, five scripts, `NOTICE`, `LICENSE` | Repo |
| Origin and license | Fork of SchanaModAutorun commit 1048f71, Apache-2.0; `NOTICE` lists every change since October 14, newest first | `NimbyAutorun/NOTICE` |
| Engine-free rules | Validation, server and client speed rules, menu and focus stop rule, double-tap guard, request throttle with stop bypass, log gate, config clamp; 70 self-test cases | `3_game/nimbyautorun_rules.c` |
| Config | `$profile:NimbyAutorun\config.json`, `version` 1, `requestsPerSecond` 10 (clamped 1-100), written with defaults when absent | `3_game/nimbyautorun_config.c:15-21, 84-133` |
| Server validation | Ownership from the authenticated sender before the payload is read; movement index and angle; dead, unconscious, restrained, or in a vehicle refuses; sprint resolves to run when `CanSprint()` is false | `4_world/nimbyautorun_playerbase.c:116-184` |
| Server rate limiting | 10 requests per second per player; a stop bypasses it; the first drop in a window stops both sides. Rejection and throttle log lines each have their own 10 s gate | `nimbyautorun_playerbase.c:107-114, 198-232` |
| Server-to-client cancellation | `6550102`, `Param1<string reason>`, to the owner only, on every forced clear and every refused or dropped running request | `nimbyautorun_playerbase.c:186-196, 234-258` |
| Server forced clears | Hooks: vehicle, unconscious, restrained, dead, disconnect, delete. Backstop in `CommandHandler` (`via=tick`) also re-evaluates `CanSprint` | `nimbyautorun_playerbase.c:271-337` |
| Client interrupts | A5 list (toggle, WASD, Space, fire, raise, lean per A2, any Shift press), state (dead, unconscious, restrained, vehicle), ladder, water, focus, dialog, any menu but chat, gestures, and the quickbar, disabled controls | `5_mission/nimbyautorun_missiongameplay.c:93-155` |
| Client reset | Player replacement or respawn (new object), `OnMissionFinish` (disconnect, teardown), RPC `6550102` | `nimbyautorun_missiongameplay.c:52-67, 184-195`; `nimbyautorun_playerbase.c:343-354` |
| Default key | Double tap Left Shift, `doubleclick` event, Controls > Others, "Legs: toggle autorun" | `Data/Inputs.xml` |
| RPC ids | `6550101` toggle, `6550102` cancel | `3_game/nimbyautorun_rules.c:12-20` |
| Save footprint | None; no `OnStoreSave`/`OnStoreLoad`, no net-synced variables | Grep |
| Player id in logs | `GetId()` and `GetPlainId()` on every line that names a player | `3_game/nimbyautorun_config.c:46-66` |
| Last build | `build\@NIMBY-20261019-152515`, 1.30 Nasdara, `SUMMARY PASS`, Autorun 70/70, 0 `SCRIPT (E)` | `docs/baselines/2026-10-19-legs-phase0-baseline.md` |
| Solo test (1.29) | Toggle, WASD, fire, raise, stamina drop: PASS. Lean: FAIL (NIMBY-T-001). Vehicle and unconscious recovery: NOT RUN (NIMBY-T-003) | `docs/testing/2026-10-14-solo-ingame-test.md` |
| On 1.30 in game | Not run yet | `docs/testing/2026-10-19-legs-phase1.md` |

## API evidence (Phase 0 item 2, October 19, 2026)

Paths are under `vendor/dayz-1.30-scripts/` (build `1.30.164078.38`, `scripts.pbo` hash in the baseline). VERIFIED means the declaration exists as used; CHANGED means 1.30 differs from what the October 14 source assumed, and the fix is named. A declaration is not proof of behavior: the in-game rows in the test record exercise each one.

| Symbol | Status | Evidence | Note |
| --- | --- | --- | --- |
| `MissionGameplay.OnInit` | VERIFIED | `scripts/5_Mission/DayZ/mission/missionGameplay.c:102` | Stop-input list built here |
| `MissionGameplay.OnUpdate` | VERIFIED | `missionGameplay.c:287` | Client loop |
| `MissionGameplay.OnMissionFinish` | VERIFIED | `missionGameplay.c:266`; base `scripts/3_Game/DayZ/gameplay.c:702` | Client teardown reset |
| `MissionBaseWorld` (`Mission.OnInit`) | VERIFIED | `scripts/4_World/DayZ/Classes/MissionBaseWorld.c:1`; `gameplay.c:700` | Debug flag, config, self-test; Tack's pattern |
| `IsControlDisabled` | VERIFIED | `missionGameplay.c:1119-1131`; base `gameplay.c:792` | True for any input exclude or restriction, so chat (`"menu"`, `ShowChat` `:1242-1253`) and gestures and the quickbar (`"radialmenu"`, `:667-675`) raise it. Legs passes those three menus through and allows a 0.5 s grace after they close, because the gesture exclude is removed in the menu destructor (`scripts/5_Mission/DayZ/GUI/GesturesMenu.c:143-148`) |
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
| `UALeanLeft`, `UALeanRight` | VERIFIED, behavior CHANGED | `constants.xml:19-20`; preset `:68-73` (Q, E); `scripts/4_World/DayZ/Entities/DayZPlayerImplement.c:3013-3020` locks both on sprint start, `:3022-3034` unlocks on sprint end | A locked input reports no press, so lean is expected to stop autorun at run or walk and never at sprint: the likely cause of T-001. Kept in the list until Phase 0 item 3 settles A2 |
| `UATurbo` | VERIFIED | `constants.xml:114`; preset `:21-23` (Left Shift hold) | A5 stop input, arms the double-tap guard |
| `UAWalkRunToggle` | VERIFIED | `constants.xml:28`; preset `:27-31` (double tap Left Ctrl) | A3 precedent; slows autorun to walk |
| `UAWalkRunTemp` | VERIFIED | `constants.xml:29`; preset `:24-26` (Left Ctrl) | Sets autorun to run |
| `UAJump` | CHANGED | Not declared anywhere in 1.30 (`bin/` and `scripts/`, 0 matches). Space is `UAGetOver`, `constants.xml:137`, preset `:51-56` | Fix: `UAGetOver` is the Space stop input |
| `kLShift`, `doubleclick` | VERIFIED | `constants.xml:500`; limit 0.5 s `:457`; Others sorting `:915` | Left Shift also carries `UAADSToggle` as a click (preset `:169-172`), active only with the weapon raised |
| `HumanInputController.OverrideMovementSpeed`, `OverrideMovementAngle` | CHANGED | `scripts/3_Game/DayZ/human.c:253, :256` take `HumanInputControllerOverrideType` (`:7-12`: `DISABLED`, `ENABLED`, `ONE_FRAME`), not `bool` | A `bool` still converts (vanilla `DayZPlayerImplement.c:3865` passes one), but Legs now passes `ENABLED` and `DISABLED` explicitly |
| `Human.GetInputController` | VERIFIED | `human.c:1458` | |
| `PlayerBase.CanSprint` | VERIFIED | `PlayerBase.c:5612-5629` | Raised, heavy item, injury level 3, throwing mode, broken legs; stamina is separate |
| `PlayerBase.GetStaminaHandler`, `StaminaHandler.GetStamina` | VERIFIED | `PlayerBase.c:2042`; `scripts/4_World/DayZ/Classes/StaminaHandler.c:1081` | |
| `IsInVehicle` | VERIFIED | `DayZPlayerImplement.c:468-471` | |
| `IsUnconscious` | VERIFIED | `PlayerBase.c:3925-3928` | |
| `IsRestrained` | VERIFIED | `PlayerBase.c:2219-2222` | |
| `IsAlive` | VERIFIED | `scripts/3_Game/DayZ/Entities/Object.c:517` | |
| `GetCommand_Ladder`, `GetCommand_Swim` | VERIFIED | `human.c:1512, :1520` | Client ladder and water interrupts |
| Vehicle entry hook: `OnCommandVehicleStart` | VERIFIED | `PlayerBase.c:4342`; callback `human.c:1691` | Server forced clear `reason=vehicle` |
| Unconscious hook: `OnUnconsciousStart` | VERIFIED | `PlayerBase.c:3756`, called from `CommandHandler` at `:3381` on both machines | Server only acts (`IsServer`) |
| Restraint hook: `SetRestrained` | VERIFIED | `PlayerBase.c:2206`; server callers `scripts/4_World/DayZ/Classes/UserActionsComponent/Actions/Continuous/ActionRestrainTarget.c:225`, `scripts/4_World/DayZ/Entities/ItemBase/HandcuffsLocked.c:40` | Acts on `true` only |
| Death hook: `EEKilled` | VERIFIED | `PlayerBase.c:1214` | |
| Removal hooks: `OnDisconnect`, `EEDelete` | VERIFIED | `PlayerBase.c:7647` (called by `MissionServer.InvokeOnDisconnect`, `scripts/5_Mission/DayZ/mission/missionServer.c:442-447`); `PlayerBase.c:1393` | Cleared before `super`, while the identity is attached |
| Server backstop: `CommandHandler` | VERIFIED | `PlayerBase.c:3206` | `via=tick` clears and the `CanSprint` re-evaluation |
| `PlayerIdentity.GetId`, `GetPlainId`, `GetName` | VERIFIED | `gameplay.c:368, :370, :362` | Vanilla's comment on `GetPlainId` reads "plaintext unique id of player (cannot be used in database or logs)" (`:369`); shared rules 4.2 log it anyway. The in-game run proves it is the Steam64 |
| `JsonFileLoader.LoadFile`, `JsonSaveFile`; `FileExist`; `MakeDirectory` | VERIFIED | `scripts/3_Game/DayZ/tools/JsonFileLoader.c:7, :134`; `scripts/1_Core/DayZ/proto/EnSystem.c:397, :525` | |
| `DayZPlayerConstants.MOVEMENTIDX_*` | VERIFIED | `scripts/3_Game/DayZ/dayzplayer.c:657-660` (0 idle, 1 walk, 2 run, 3 sprint) | |

## Decisions

| # | Decision | Choice | Status |
| --- | --- | --- | --- |
| A1 | Display name | Legs. Code prefix stays `NimbyAutorun`; Controls entry reads "Legs: toggle autorun" | Decided October 15, 2026 |
| A2 | Lean interrupt (T-001) | Lean interrupts if Phase 0 shows `UALeanLeft`/`UALeanRight` report a press while sprinting; otherwise lean is dropped from the interrupt list and documented. Reference evidence, October 19, 2026: vanilla locks `UALeanLeft` and `UALeanRight` on sprint start and unlocks them on sprint end (`DayZPlayerImplement.c:3013-3034`), so a press is not expected while sprinting; the Phase 1 build keeps lean in the list, where it can only act at run or walk, until the in-game check | Decided, conditional; Phase 0 item 3 open |
| A3 | Default key | Double-tap Left Shift: `<btn name="kLShift"><event name="doubleclick"/></btn>` in `Data/Inputs.xml` (vanilla precedent `UAWalkRunToggle` on `kLControl`, `bin/preset_X1MouseKey.xml:27-31`); players rebind. Phase 0 adds a check that the double tap toggles autorun without leaving `UATurbo` (the `kLShift` hold, `:21-22`) in a stuck state. Replaces numpad plus | Decided, revised by Jeff October 15, 2026 |
| A5 | Stop rule | Autorun stops when the player takes control back: any movement key (WASD), a single Shift press (the sprint hold; a double tap while stopped starts autorun, any Shift press while running stops it), Space (jump or vault), fire, weapon raise, lean (per A2), or the toggle itself; and on vehicle entry, unconsciousness, restraint, death, and the menu and focus cases in Phase 1. It does not stop on unrelated input such as chat, gestures, or the inventory hotbar | Decided by Jeff, October 16, 2026 |
| A4 | Server cancellation RPC | `6550102`, server to owning client; on receipt the client clears its override and remembered state and does not re-send start | Decided (shared rules 4.1) |

## Phase 0 - baseline and test environment

Depends on Plan 0 (shared tooling) being on `main`.

1. Record the client and server build, source revision, and the hashes of the shared 1.30 script reference in `docs/baselines/`.
2. Verify every vanilla symbol the mod touches against the shared reference and record file:line: `MissionGameplay.OnInit`/`OnUpdate`, `IsControlDisabled`, `PlayerBase.OnRPC`, `GetUApi().GetInputByName`, `UAInput.LocalPress`, the `UA*` input names in the interrupt list, `HumanInputController.OverrideMovementSpeed`/`OverrideMovementAngle`, `CanSprint`, `GetStaminaHandler`, `IsInVehicle`, `IsUnconscious`, `IsRestrained`, and the vehicle-entry, unconsciousness, restraint, death, and player-removal hooks the server will use. Exercise each in game; a declaration is not proof.
3. Settle A2: on the test server, hold lean while sprinting and record whether `LocalPress` fires. Record the result in this plan.
4. Build and boot on 1.30 Nasdara with the shared tooling; confirm the quick boot check and the qualification run are separate invocations.
5. Establish the same-build vanilla control. Investigate existing copied-world save-load errors separately; do not exempt them automatically.
6. Settle the A3 double tap: bind `UANimbyAutorunToggle` to `kLShift` with `<event name="doubleclick"/>` on the test server and record whether the double tap toggles autorun, whether the two short `UATurbo` presses it also fires leave the sprint hold in any stuck state, and whether a held Shift still sprints normally.

Exit: the signed addon boots on 1.30 Nasdara with 0 `SCRIPT (E)`; every touched API is recorded as VERIFIED (file:line) or CHANGED with its fix; A2 and the A3 double tap are settled.

Status, October 19, 2026: item 1 done (`docs/baselines/2026-10-19-legs-phase0-baseline.md`); item 2 done against the reference (API evidence table above; the in-game rows in the test record exercise each symbol); item 4 done: the quick boot check (`@NIMBY-20261019-152118`, `BOOT MODE quick storageAutoFix=1`) and the qualification run (`@NIMBY-20261019-152515`, `-Qualification`, `BOOT MODE qualification storageAutoFix=0`) are separate invocations, and both pass with 0 `SCRIPT (E)` and 0 "Scripted variables corrupted"; items 3, 5, and 6 need a client and are rows P0-3, P0-5, and P0-6 in `docs/testing/2026-10-19-legs-phase1.md`.

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
| Mission start | `[NIMBY][Autorun] loaded; server=<bool> debug=<bool>`; on the server `config loaded|missing|invalid|clamped` | Always |
| Self-test | `[NIMBY][Autorun][SelfTest] PASS|FAIL <case>`, then `[NIMBY][Autorun] selftest complete; cases=<n> failures=<n>` | Self-test flag only |

`via=hook` is the state hook named in the API evidence table; `via=tick` is the `CommandHandler` backstop and means a hook did not fire first, which is a finding. Client `cause` values: `toggle`, `speed`, an input name (`UATurbo`, `UAMoveForward`, `UAGetOver`, `UAFire`, `UATempRaiseWeapon`, `UALeanLeft`, `UALeanRight`, `UANimbyAutorunToggle`), a state (`dead`, `unconscious`, `restrained`, `vehicle`), `ladder`, `swim`, `focus`, `dialog`, `inventory`, `pause`, `menu-<id>`, `control-disabled`, `player-replaced`, `mission-finish`; start refusals add `double-tap-guard`. Server `cause` values: `client-request`, `client-stop`, `cannot-sprint`. Debug lines need `$profile:NimbyAutorun\debug` on the machine that prints them: the server profile for server lines, the client profile (`%LOCALAPPDATA%\DayZ`) for client lines.

Exit: client and server reach the same stopped state after every interruption, with no automatic resumption, and the self-test passes.

### Implementation notes (October 19, 2026)

Choices the plan left open, made in the Phase 1 build and open to Jeff's review:

- **Pass-through menus.** `IsControlDisabled()` is true whenever any input exclude is active, and chat, gestures, and the radial quickbar each set one (API evidence). A5 says those three never stop autorun, so the client lets them through by menu id and ignores a disabled-control state for 0.5 s after one closes. Every other menu, a dialog, focus loss, and a disabled-control state outside that window stop autorun. While one of the three is open, a start is still refused (`control-disabled`).
- **Double-tap guard.** While running, the first press of a double tap is a `UATurbo` press and stops autorun (A5). Without a guard, the second press would fire the toggle and start it again at once, so a toggle within 0.5 s (the keyboard doubleclick limit) of a `UATurbo` stop is refused (`double-tap-guard`).
- **Shift no longer selects sprint.** The October 14 fork raised speed to sprint on a Shift press while running; A5 makes that press a stop. After stamina runs out autorun stays at run until the player stops and double-taps again (open question 2 in the test record).
- **Walk and run.** The vanilla walk toggle (double tap Left Ctrl) slows autorun to walk and a single Left Ctrl press sets run, as in the fork. Sprint drops to run only from sprint, so walking no longer turns into running when stamina is low.
- **Ladder and water.** Not in the Phase 1 server list; the client stops on a ladder or swim command (the Phase 2 row) and sends the stop, so the server clears through the normal request.
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
| Ladder and swimming transitions | Autorun clears; vanilla controls usable | `running=false`; player responds to WASD |
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
| Lean not observable while sprinting | T-001 unfixable as designed | A2 drops lean and documents it |
| Vehicle test blocked | CR-008 unproven | Test-only spawn in the local mission `init.c` |
| Server and client disagree after a forced clear | Player stuck moving or unable to restart | A4 cancellation RPC; Phase 2 reconciliation row |
