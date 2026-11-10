# Plan 13 recon - Music (`NimbyTapes`)

Phase 0 step 1 record for [Plan 13](2026-11-09-music-plan.md) (v0.3), November 10, 2026. Seat: the orchestrator session (Claude, Cowork), on Jeff's go, writing into worktree `D:\GitHub\nimby-badlands-server-wt\nimby-music`, branch `feat/nimby-music` at `6f00542` (pull requests #28 and #29 merged, stub present). Read-only recon: no addon code changed, and the plan file is untouched.

Every `file:line` is relative to `vendor/dayz-1.30-scripts/` (build `1.30.164200`) unless it starts with `ZenMusic:` (`vendor/ZenMusic` at commit `3de1452`) or `mods/`. A native declaration (`proto`) is evidence that an API exists, not of how it behaves. Each answer is marked supported, contradicted, or unresolved, and an unresolved answer names the test row that settles it.

## Prerequisites at the start of step 1

| # | Prerequisite | State | Evidence |
| --- | --- | --- | --- |
| PR1 | Pull request #28 merged | Done | Merge commit `6f00542`, November 10, 2026, 09:47 Central |
| PR2 | Kickoff pull request | Done | Pull request #29, merge commit `d9a9f41`; boot check `logs\build-20261110-093817\build.log` in the kickoff worktree: `SELFTEST Tapes ran=yes cases=3 pass=3 fail=0`, `SUMMARY PASS` |
| PR3 | `ffmpeg` and `ffprobe` | Done | `ffprobe -version` prints 9.0.2 with `--enable-libvorbis` (Jeff, November 10, 2026) |
| PR4 | Zen reference clones | Done | Unchanged since November 9, 2026 |
| PR5 | `feat/nimby-music` holds `main` | Done | Fast-forward to `6f00542`, working tree clean |
| PR6 | Second client | Not re-checked | As the plan states |
| PR7 | Script reference is current | Done for the replica | The next section |
| PR8 | `ImageToPAA` | Not re-checked | As the plan states |

The plan's own prerequisites table still shows PR1, PR2, PR3, PR5, and PR7 as open. It is corrected when the plan goes to v0.4.

## Script reference check (PR7)

| Item | Value |
| --- | --- |
| File hashed | `D:\DayZServer\dta\scripts.pbo`, 14,366,572 bytes, written October 21, 2026, 22:40 Central |
| SHA-256 | `FF0AD7EE5297C75DA74364569A830BC0223315C0131579A0C11A7875D64CB2E2` |
| Baseline | `docs/baselines/2026-10-31-dayz-1.30-164200-scripts-reference.md`, same value |
| Result | Match. No re-extract is needed |

Not checked: the WinSCP mirror of the live server (`D:\HostHavoc\...`) is not a folder this session can read. The baseline record states that the replica's file is byte-identical to the Steam server install.

## Answers in short

| # | Question | Answer | Status | Settled by |
| --- | --- | --- | --- | --- |
| 1a | Do the vanilla switch actions show for an own class once `SetActions()` adds them? | Yes. The actions are attached per class by `AddAction`, and Zen's own device class does exactly this | Supported | T0-04 to T0-06 |
| 1b | Can the switch-on prompt be hidden for a deck with no tape? | Not with the vanilla actions. Their conditions ask only the energy manager, which consults no entity callback | Contradicted | T0-03 keeps the server-side refusal |
| 2 | Wave kind, and does the sound scene's `SetSoundVolume` scale it? | The kind is `WAVEEFFECTEX` by default. Whether the scene's sound volume scales that kind is native | Unresolved | T0-18 |
| 3 | Does a streamed sample honor `SetStartOffset`, and what if the header is not loaded? | The wave object exists as soon as `Play3D` returns, header or not. The offset's unit and its behavior before the header loads are native | Unresolved | T0-14, T0-15 |
| 4 | Which `sound3DProcessingType` and `volumeCurve`? | Start from the `ChristmasMusic_SoundSet` values without `loop`. Nothing in source says how a 4-minute sample behaves | Unresolved | T0-16 |
| 5a | Does `EEItemLocationChanged` fire on the client for the deck's own moves? | Vanilla runs client-only code in the callbacks this event drives | Supported by inference | T0-12 |
| 5b | Is a callback raised on a descendant when an ancestor moves? | Only when the ancestor enters or leaves a player's inventory: `OnInventoryEnter` and `OnInventoryExit`. No callback for any other ancestor move | Supported | T0-12, T0-13; the 0.5-second root check stays |
| 6a | Is a net-synced `int` delivered whole? | One variable, registered without a range, is not quantized | Supported | T0-14 |
| 6b | In what order do the variable, the tape attachment, and the energy state reach a new client? | The energy state travels as variables of the deck itself. The order against the attachment is native | Unresolved | T0-15 |
| 7 | What happens to an attached vanilla battery when the parent class no longer resolves? | Nothing in script decides it, and no earlier test record in this repository covers an attachment of a removed class | Unresolved | T0-21 |
| 8 | Load order, and does `OnWorkStart` fire before the tape exists? | `SwitchOn` runs inside the deck's `OnStoreLoad`, before any child. `OnWorkStart` cannot fire there, because the deck has no power yet. It fires from inside the battery's attach event, and the tape may or may not be attached at that moment | Supported in part | T0-20 |

## Question 1 - switch actions

A plain `ItemBase` has five actions and none is a switch (`scripts/4_World/DayZ/Entities/ItemBase.c:246-253`). A class gains the switch actions by calling `AddAction` in its own `SetActions()`. Zen's device, an addon class on `ItemBase`, adds the same four vanilla actions the plan names (`ZenMusic:ZenMusicBase/scripts/4_World/ZenMusicBase/Entities/ZenMusic/Devices/Zen_MusicDeviceBase.c:398-405`). Fallback (c) is not expected to be needed.

The in-hands condition is `item.HasEnergyManager() && item.GetCompEM().CanSwitchOn() && item.GetCompEM().CanWork()` (`scripts/4_World/DayZ/Classes/UserActionsComponent/Actions/SingleUse/ActionTurnOnWhileInHands.c:52`). The on-ground condition is the same test plus a ground-location check (`.../Actions/Interact/ActionTurnOnWhileOnGround.c:17`). Both methods live on the component:

- `CanSwitchOn()` returns true whenever the switch is off (`scripts/3_Game/DayZ/tools/Component/ComponentEnergyManager.c:752-760`).
- `CanWork()` tests ruin, wetness, and available energy (`ComponentEnergyManager.c:768-851`).

Neither asks the entity anything, so an own class has no method to override that hides the prompt. The two ways to hide it are an own action class with a tape test in its `ActionCondition` (two more small classes, as in fallback (c)) or a `modded class ComponentEnergyManager` (a fifth vanilla class, not proposed). The plan's default stands: the prompt shows, and the server refuses in `OnWorkStart`. Zen uses the same refusal (`ZenMusic:...Zen_MusicDeviceBase.c:355-363`).

Two consequences for step 4:

- **`OnWorkStop` follows a refused start.** `OnWorkStart` is called from inside `DeviceUpdate` (`ComponentEnergyManager.c:1819-1828`). A `SwitchOff()` made there runs `DeviceUpdate` again (`ComponentEnergyManager.c:402-417`), which calls `OnWorkStop` (`ComponentEnergyManager.c:1859-1876`). The deck's `OnWorkStop` therefore runs for a start that never published a track and has to do nothing in that case.
- **The deck's energy manager must not set `autoSwitchOffWhenInCargo`.** With it, entering any cargo switches the device off (`ComponentEnergyManager.c:715-724`, called from `scripts/3_Game/DayZ/Entities/EntityAI.c:1362-1366`), which would break T-2. The vanilla `Radio` block the plan copies does not set it (`DZ/gear/radio/config.cpp:275-282`).

## Question 2 - wave kind and Ears

`EffectSound` sets `m_SoundWaveKind = WaveKind.WAVEEFFECTEX` in its constructor (`scripts/3_Game/DayZ/Effects/EffectSound.c:65`) and applies it to the sound object when the sound loads (`EffectSound.c:327`). `SetSoundWaveKind` changes it and has to be called before playing (`EffectSound.c:788-791`).

The sound scene has five volume controls: radio, speech, music, sound, and VOIP (`scripts/3_Game/DayZ/Sound.c:62-75`). Ears calls `SetSoundVolume` (`mods/own/nimby-pack/NimbyEarplugs/Scripts/5_Mission/nimbyearplugs_missiongameplay.c:238`), the control the game sets from the player's Effects slider (`scripts/5_Mission/DayZ/mission/missionGameplay.c:1690`). Which wave kinds each control scales is native. T0-18 settles it.

Two inferences, neither proven:

- **Unconscious players.** Vanilla silences an unconscious player with the same call, `SetSoundVolume(0,2)` (`scripts/4_World/DayZ/Entities/ManBase/PlayerBase.c:3778`). If T0-18 passes, an unconscious player stops hearing the deck too.
- **`WAVEMUSIC` is the wrong kind.** Vanilla uses it for the dynamic music player (`scripts/3_Game/DayZ/Systems/DynamicMusicPlayer/DynamicMusicPlayer.c:518`). A deck of that kind would most likely follow the Music slider and ignore Ears. Keep the default kind.

## Question 3 - start offset

What source shows:

- **The wave exists before its header.** `SoundPlayEx` stores the wave returned by `Play3D` and only defers the length read and the event hooks until the header has loaded (`EffectSound.c:173-181`, `EffectSound.c:365-397`). Right after `SoundPlay()` returns true the accessor returns a wave, so a null wave means the play failed.
- **`GetLength()` blocks.** Its declaration carries the warning "Blocking! Waits for header to load" (`Sound.c:184-185`). The plan's fraction uses `trackSeconds` from config, as Zen's does (`ZenMusic:...Zen_MusicDeviceBase.c:337-344`), so the client never has to call it.
- **Vanilla calls `SetStartOffset` straight after `Play3D`.** It does so with no header check (`scripts/4_World/DayZ/Entities/DayZPlayerImplement.c:2172-2173`, and `Sound.c:175-179`).
- **The unit is not shown.** Vanilla passes values between 0.1 and 0.3 on short clothing sounds (`DayZPlayerImplement.c:2125-2128`, `2190-2197`), which reads equally well as a fraction or as seconds. `Skip` is declared in seconds (`Sound.c:198`). Zen passes a fraction.

One more declaration is worth using. `GetCurrPosition()` returns the wave's position as a share of its total length (`Sound.c:186-187`; the debug plugin multiplies it by 100 to print a percentage, `scripts/4_World/DayZ/Plugins/PluginBase/PluginSoundDebug.c:142`). The client can log it:

- **Unit probe, first thing in step 6.** Start the 240-second track with `SetStartOffset(0.5)` and log `GetCurrPosition()` one second later. About 0.5 means the unit is a fraction. About 0.002 means seconds, and fallback (a) starts from `Skip`.
- **Skew without a stopwatch.** `GetCurrPosition() * trackSeconds` against the published elapsed time is the client's own skew figure. It adds to the T0-14 and T0-15 recordings and does not replace them; T-7 asks for one recording that holds both clients.

## Question 4 - sound set values

`ChristmasMusic_SoundSet` is the positional music set the plan cites (`DZ/sounds/hpp/config.cpp:194396-194407`), with its shader at `DZ/sounds/hpp/config.cpp:127070-127078`.

| Value | `ChristmasMusic` | Zen's tape sets | Proposed for the Phase 0 fixtures |
| --- | --- | --- | --- |
| `sound3DProcessingType` | `smallItem3DProcessingType` | Not set | `smallItem3DProcessingType` |
| `volumeCurve` | `LinearCurve` | Not set | `LinearCurve` |
| `spatial` | 1 | Not set | 1 |
| `loop` | 1 | Not set | 0 |
| `volumeFactor` | 0.1 | 0.75 | 1, then tune |
| Shader `volume` | 0.55 | 1 | 1, then tune |
| Shader `range` | 35 | 100 | 25 (the plan's starting value) |

Zen's values are at `ZenMusic:ZenMusicAudio/config.cpp:388` and `ZenMusic:ZenMusicBase/config.cpp:717-723`. `smallItem3DProcessingType` is a `panner` with `range = 2`; `default3DProcessingType` is an `emitter` with `range = 15` and `radius = 3` (`DZ/sounds/hpp/config.cpp:2587-2619`). `LinearCurve` runs from full volume at distance share 0 to silence at 1 (`DZ/sounds/hpp/config.cpp:781-788`). What the processor's `type` and `range` do to a long sample is native. T0-16 tunes it, and the final values go into v0.4.

## Question 5 - following the carrier

**Own moves.** The engine calls `EEItemLocationChanged` on the moved item (`EntityAI.c:976-1018`), which calls `OnItemLocationChanged`. Vanilla guards the server-only parts of such overrides with `IsDedicatedServer()` (`scripts/4_World/DayZ/Entities/ItemBase/Clothing/BurlapSackCover.c:13-17`), and it runs client-only code in a callback this event drives: `EasterEgg.OnInventoryEnter` stops a particle under `g_Game.IsClient()` (`scripts/4_World/DayZ/Entities/ItemBase/Gear/Consumables/EasterEgg.c:137-145`). That is evidence the event reaches clients. It does not prove the event reaches a client for an item it does not own. T0-12 does.

**Ancestor moves.** When an item's owning player changes, `ItemBase.OnItemLocationChanged` walks the whole inventory of the moved item and calls `OnInventoryExit(oldPlayer)` and `OnInventoryEnter(newPlayer)` on every item inside it (`ItemBase.c:1222-1244`; the two events are at `ItemBase.c:3906` and `ItemBase.c:3918`). A deck inside a backpack is therefore told when a player picks the backpack up or drops it. Nothing is raised when the ancestor moves between places with no player change (ground to a vehicle's cargo, one container to another).

Proposed for step 4: re-parent on `EEItemLocationChanged`, on `OnInventoryEnter`, and on `OnInventoryExit`, and keep the 0.5-second root check for the rest.

**Re-parenting a playing sound.** `EffectSound.SetParent` passes the new parent to the live sound object (`EffectSound.c:658-666`), which the engine attaches much as `IEntity.AddChild` would (`Sound.c:117-119`). The base class's note that `SetParent` "only sets the cached variable" (`scripts/3_Game/DayZ/Effect.c:395-403`) describes the base method, not this override. Whether a wave that is already playing follows the new parent is native: T0-12 and T0-13.

## Question 6 - what reaches a client

- **One variable, no range.** `RegisterNetSyncVariableInt(name, minValue, maxValue)` quantizes only when the two bounds differ (`EntityAI.c:2843-2850`). The packed state must be registered with the name alone, so all 30 bits travel.
- **Energy state is a set of the deck's own variables.** `m_EM.m_IsSwichedOn` and `m_EM.m_CanWork` are registered in the `EntityAI` constructor (`EntityAI.c:218-223`) and arrive through the deck's own `OnVariablesSynchronized`. On every sync the base method calls `DeviceUpdate()` and `StartUpdates()` on the client (`EntityAI.c:3069-3111`), which is where client-side `OnWorkStart` and `OnWorkStop` come from.
- **The client keys on the packed integer only.** `OnWorkStart` on a client says the device has power, not which track plays. The plan's reconciliation table already works this way; the deck's override calls `super` first, as the plan says.
- **Order against the attachment is native.** Whether the tape attachment exists on the client when the first sync lands is not visible in script. The hold-and-retry row covers it, and T0-15 measures it.
- **Arrival versus live start.** Vanilla expects `OnWorkStart` on a client for a device that arrives already working, and tells that apart from a live start with `IsInitialized()`: the generator plays its start sound only when it is true (`scripts/4_World/DayZ/Entities/ItemBase/PowerGenerator.c:145-152`). The flag turns true 34 ms after construction (`EntityAI.c:240`, `EntityAI.c:277-285`).

## Question 7 - battery on a removed class

Nothing in script takes part in dropping an entity whose class does not resolve. The repository's earlier removal checks (Tack TK-006, the DPV save-and-reload row for build 58) do not record what happened to a vanilla item attached to a removed class. T0-21 records it. The row already asks for a kept copy of the storage and a loose control battery.

## Question 8 - storage load

The chain, in order:

1. **`OnStoreLoad` on the deck.** After reading the saved switch state it calls `m_EM.SwitchOn()` when the state was on (`EntityAI.c:3047-3050`). This is before any child is loaded: `AfterStoreLoad` is documented as running "after all children loaded" (`EntityAI.c:1418-1421`).
2. **`SwitchOn` with no power.** It sets the switch flag and calls `DeviceUpdate()` (`ComponentEnergyManager.c:371-392`). `DeviceUpdate` raises `OnWorkStart` only when `m_CanWork` is true and energy was consumed (`ComponentEnergyManager.c:1792`, `1819-1828`). The deck stores no energy of its own (the `Radio` block has no storage values, `DZ/gear/radio/config.cpp:275-282`), so `m_CanWork` is false and nothing is raised. The deck stays switched on.
3. **The battery attaches.** The deck's `EEItemAttached` hands the battery to the energy manager (`EntityAI.c:1181-1182`), which plugs the deck into it (`ComponentEnergyManager.c:1453-1466`). `PlugInDevice` updates `m_CanWork` and wakes the plugged device (`ComponentEnergyManager.c:1652-1680`, `1557-1576`), which runs the deck's `DeviceUpdate` at once. A 9V is switched on from creation (`switchOnAtSpawn = 1`, `DZ/gear/consumables/config.cpp:588-598`), so with any charge `OnWorkStart` fires here, inside the battery's attach event.
4. **The tape attaches.** Before or after step 3. The order of a parent's attachments during load is the engine's.
5. **`AfterStoreLoad` on the deck.** After both.

So the plan's concern holds in substance and is narrower than written: `OnWorkStart` does not fire from `OnStoreLoad`, and it can fire before the tape exists when the battery loads first. A deck built the way Zen's is would switch itself off at step 3 in that case and never resume.

Proposed for step 4, replacing the wording of After load in the plan's Architecture table (no ruling changes; T-9 stands):

- **`OnWorkStart` defers during init.** On the server, when `IsInitialized()` is false, it records that work has started and returns without validating. Vanilla uses the same test to keep attach-time and start-time reactions out of init (`PowerGenerator.c:149`, `PowerGenerator.c:228`, `ComponentEnergyManager.c:1442`).
- **`AfterStoreLoad` starts the deck.** It calls `super` (`ItemBase.c:3222-3236`), then validates the tape and starts the remembered track from its beginning, or switches off.
- **`DeferredInit` covers a fresh spawn.** A deck created with battery and tape in one frame (the test kit) never gets `AfterStoreLoad`. Its `DeferredInit` override (`EntityAI.c:277-280`) runs the same validate-and-start when work was recorded and nothing has started yet.

Unresolved, for T0-20: whether the call queue runs between entity loads at server start (if it does, `IsInitialized()` can already be true at step 3), and the attachment order. The row's server debug line should carry `init=<0|1> tape=<0|1>` at every `OnWorkStart` so one restart answers both.

## Other findings for step 3 and step 4

| # | Finding | Evidence | Effect |
| --- | --- | --- | --- |
| R1 | The energy manager's own tick defaults to 15 seconds, and the `Radio` block sets no `updateInterval` | `ComponentEnergyManager.c:20`, `200`, `223-226` | Battery drain is taken in 15-second steps, and a flat battery is noticed at the next step. The measured run-down time in T0-10 carries that granularity. `updateInterval` is a config value (vanilla uses 5 to 50); changing it does not change the drain rate (`ComponentEnergyManager.c:726`). No change proposed for Phase 0 |
| R2 | Removing the battery stops work in the same call, not at the next tick: the unplug runs `DeviceUpdate`, and the deck has no stored energy to draw on | `ComponentEnergyManager.c:1474-1484`, `447-473`, `1252-1278` | Supports the 1-second limit in T0-10. The deck's own `EEItemDetached` handler still publishes stopped |
| R3 | The manager keeps a reference to every effect it creates | `scripts/3_Game/DayZ/EffectManager.c:3`, `40-42`, `153-166` | Confirms the plan's single teardown path |
| R4 | Each start sets the wave to the effect's maximum volume on its first frame | `EffectSound.c:469-486` | The Phase 1 volume steps (T-14) set `SetSoundMaxVolume` (`EffectSound.c:878-883`) before `SoundPlay`, not only `SetSoundVolume` |
| R5 | The effect's frame update is driven from `MissionGameplay.OnUpdate` | `EffectSound.c:455-460` | Client only. Nothing to do on the server |
| R6 | `GetTickTime()` returns seconds since game start as a `float` | `scripts/3_Game/DayZ/Global/Game.c:910-914` | At 12 hours of uptime a `float` still resolves a few milliseconds. No concern for a 12-hour restart cycle |

## Plan lines to change at v0.4

None of these needs a ruling.

| Plan location | Change |
| --- | --- |
| Architecture, After load | Replace with the three bullets under Question 8. The trigger is the battery's attach event, not `OnStoreLoad` |
| Architecture, Following the carrier | Add `OnInventoryEnter` and `OnInventoryExit` as re-parent triggers |
| Architecture, Debug lines | Client lines gain `pos=<GetCurrPosition>`; the server `start` line gains `init=` and `tape=` |
| Phase 0 step 6 | Run the `SetStartOffset` unit probe before T0-04 |
| Recon question 1 | Closed: the prompt cannot be hidden with the vanilla actions; the refusal stands |
| Risks K11 | Narrow to "the battery loads before the tape" |
| Prerequisites | Mark PR1, PR2, PR3, PR5, and PR7 done |

## Status

Step 1 is complete. Nothing here blocks step 2 (fixtures). No question for Jeff came out of the recon.
