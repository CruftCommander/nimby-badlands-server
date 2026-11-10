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
| PR7 | Script reference is current | Partly done: the replica matches; the live mirror and the Steam install are not hashed | The next section |
| PR8 | `ImageToPAA` | Not re-checked | As the plan states |

The plan's own prerequisites table still shows PR1, PR2, PR3, and PR5 as open. It is corrected when the plan goes to v0.4. PR7 stays open until the two hashes below are read.

## Script reference check (PR7)

| Item | Value |
| --- | --- |
| File hashed | `D:\DayZServer\dta\scripts.pbo`, 14,366,572 bytes, written October 21, 2026, 22:40 Central |
| SHA-256 | `FF0AD7EE5297C75DA74364569A830BC0223315C0131579A0C11A7875D64CB2E2` |
| Baseline | `docs/baselines/2026-10-31-dayz-1.30-164200-scripts-reference.md`, same value |
| Result | Match. No re-extract is needed |

Not checked, so PR7 is not closed. The plan asks for the hash of the server the replica mirrors, and the WinSCP mirror of the live server (`D:\HostHavoc\...`) is not a folder this session can read. The Steam server install was not hashed either, and it is the install the boot checks ran on (`F:\SteamLibrary\steamapps\common\DayZServer`, the `BOOT START` line of each `build.log`). The baseline record states that the replica's file was byte-identical to the Steam install's on October 31, 2026. Two `Get-FileHash` readings on Jeff's desktop close the row.

## Answers in short

| # | Question | Answer | Status | Settled by |
| --- | --- | --- | --- | --- |
| 1a | Do the vanilla switch actions show for an own class once `SetActions()` adds them? | Yes. The actions are attached per class by `AddAction`, and Zen's own device class does exactly this | Supported | T0-04 to T0-06 |
| 1b | Can the switch-on prompt be hidden for a deck with no tape? | Yes, but not through the energy manager. The vanilla actions' conditions ask only the manager. The deck can filter its own action list in a `GetActions` override, or use own action classes | Supported; not adopted for Phase 0 | T0-03 keeps the server-side refusal |
| 2 | Wave kind, and does the sound scene's `SetSoundVolume` scale it? | The kind is `WAVEEFFECTEX` by default. Whether the scene's sound volume scales that kind is native | Unresolved | T0-18 |
| 3 | Does a streamed sample honor `SetStartOffset`, and what if the header is not loaded? | The wave object exists as soon as `Play3D` returns, header or not. The offset's unit and its behavior before the header loads are native | Unresolved | T0-14, T0-15 |
| 4 | Which `sound3DProcessingType` and `volumeCurve`? | Start from the `ChristmasMusic_SoundSet` values without `loop`. Nothing in source says how a 4-minute sample behaves | Unresolved | T0-16 |
| 5a | Does `EEItemLocationChanged` fire on the client for the deck's own moves? | Vanilla runs client-only code in the callbacks this event drives | Supported by inference | T0-12 |
| 5b | Is a callback raised on a descendant when an ancestor moves? | Only when the ancestor enters or leaves a player's inventory: `OnInventoryEnter` and `OnInventoryExit`. No callback for any other ancestor move | Supported | T0-12, T0-13; the 0.5-second root check stays |
| 6a | Is a net-synced `int` delivered whole? | One variable, registered without a range, is not quantized, by the declaration's comment | Supported by a declaration only | T0-14 |
| 6b | In what order do the variable, the tape attachment, and the energy state reach a new client? | The energy state travels as variables of the deck itself. The order against the attachment is native | Unresolved | T0-15 |
| 7 | What happens to an attached vanilla battery when the parent class no longer resolves? | Nothing in script decides it, and no earlier test record in this repository covers an attachment of a removed class | Unresolved | T0-21 |
| 8 | Load order, and does `OnWorkStart` fire before the tape exists? | `SwitchOn` runs inside the deck's `OnStoreLoad`. `OnWorkStart` fires at the first moment the deck is both switched on and powered: inside the battery's attach event when the battery attaches after `OnStoreLoad`, or inside `OnStoreLoad` when it attaches before. Script does not show which, nor whether the tape is attached by then | Supported in part | T0-20 |

## Question 1 - switch actions

A plain `ItemBase` has five actions and none is a switch (`scripts/4_World/DayZ/Entities/ItemBase.c:246-253`). A class gains the switch actions by calling `AddAction` in its own `SetActions()`. Zen's device, an addon class on `ItemBase`, adds the same four vanilla actions the plan names (`ZenMusic:ZenMusicBase/scripts/4_World/ZenMusicBase/Entities/ZenMusic/Devices/Zen_MusicDeviceBase.c:398-405`). Fallback (c) is not expected to be needed.

The in-hands condition is `item.HasEnergyManager() && item.GetCompEM().CanSwitchOn() && item.GetCompEM().CanWork()` (`scripts/4_World/DayZ/Classes/UserActionsComponent/Actions/SingleUse/ActionTurnOnWhileInHands.c:52`). The on-ground condition is the same test plus a ground-location check (`.../Actions/Interact/ActionTurnOnWhileOnGround.c:17`). Both methods live on the component:

- `CanSwitchOn()` returns true whenever the switch is off (`scripts/3_Game/DayZ/tools/Component/ComponentEnergyManager.c:752-760`).
- `CanWork()` tests ruin, wetness, and available energy (`ComponentEnergyManager.c:768-851`).

Neither condition asks the deck about its tape, so the energy manager offers no override that hides the prompt. Two other routes do. The item's action list comes from an overridable method (`ItemBase.GetActions`, `scripts/4_World/DayZ/Entities/ItemBase.c:235-244`, called from `scripts/4_World/DayZ/Classes/UserActionsComponent/ActionInput.c:317` and `:602`), so the deck can return a filtered copy that leaves out the switch-on action while it holds no tape. It has to be a copy: the map behind the list is shared by every item of the type (`ItemBase.c:228-231`). Or own action classes carry a tape test in `ActionCondition` (two more small classes, as in fallback (c)). Neither is adopted for Phase 0. The plan's default stands: the prompt shows, and the server refuses in `OnWorkStart`, which is what T0-03 tests. Zen's `OnWorkStart` likewise switches the device off when its play call fails (`ZenMusic:...Zen_MusicDeviceBase.c:355-363`).

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

- **Unit probe, first thing in step 6, in up to two plays of the 240-second track.** Play 1: `SetStartOffset(0.5)`, then log `GetCurrPosition()` one second later. About 0.50 means the unit is a fraction, and the probe ends. A value near zero does not say why: seconds would read about 0.006 and an ignored offset about 0.004. Play 2 separates them: `SetStartOffset(120)` and the same reading. About 0.50 means seconds; about 0.004 means the offset is ignored, and fallback (a) starts from `Skip`.
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

- **One variable, no range.** `RegisterNetSyncVariableInt(name, minValue, maxValue)` quantizes only when the two bounds differ (`EntityAI.c:2843-2850`). The packed state must be registered with the name alone, so all 30 bits travel. This rests on the declaration's comment, not on observed behavior; a wrong track or generation in T0-14 would show it.
- **Energy state is a set of the deck's own variables.** `m_EM.m_IsSwichedOn` and `m_EM.m_CanWork` are registered in the `EntityAI` constructor (`EntityAI.c:218-223`) and arrive through the deck's own `OnVariablesSynchronized`. On every sync the base method calls `DeviceUpdate()` and `StartUpdates()` on the client (`EntityAI.c:3069-3111`), which is where client-side `OnWorkStart` and `OnWorkStop` come from.
- **The client keys on the packed integer only.** `OnWorkStart` on a client says the device has power, not which track plays. The plan's reconciliation table already works this way; the deck's override calls `super` first, as the plan says.
- **Order against the attachment is native.** Whether the tape attachment exists on the client when the first sync lands is not visible in script. The hold-and-retry row covers it, and T0-15 measures it.
- **Arrival versus live start.** Vanilla expects `OnWorkStart` on a client for a device that arrives already working, and tells that apart from a live start with `IsInitialized()`: the generator plays its start sound only when it is true (`scripts/4_World/DayZ/Entities/ItemBase/PowerGenerator.c:145-152`). The flag turns true 34 ms after construction (`EntityAI.c:240`, `EntityAI.c:277-285`).

## Question 7 - battery on a removed class

Nothing in script takes part in dropping an entity whose class does not resolve. The repository's earlier removal checks (Tack TK-006, the DPV save-and-reload row for build 58) do not record what happened to a vanilla item attached to a removed class. T0-21 records it. The row already asks for a kept copy of the storage and a loose control battery.

## Question 8 - storage load

The chain, in order:

1. **`OnStoreLoad` on the deck.** After reading the saved switch state it calls `m_EM.SwitchOn()` when the state was on (`EntityAI.c:3047-3050`). Whether the deck's attachments already exist at this point is not shown in script. The only ordering script states is that `AfterStoreLoad` runs "after all children loaded" (`EntityAI.c:1418-1421`). The steps below take the usual reading, that children attach after the parent's `OnStoreLoad`, and say where the other order changes the result.
2. **`SwitchOn` with no power.** It sets the switch flag and calls `DeviceUpdate()` (`ComponentEnergyManager.c:371-392`). `DeviceUpdate` raises `OnWorkStart` only when `m_CanWork` is true and energy was consumed (`ComponentEnergyManager.c:1792`, `1819-1828`). The deck stores no energy of its own (the `Radio` block has no storage values, `DZ/gear/radio/config.cpp:275-282`), so `m_CanWork` is false and nothing is raised. The deck stays switched on. If the battery were already attached and plugged, `m_CanWork` would be true and `OnWorkStart` would fire here, inside `OnStoreLoad`.
3. **The battery attaches.** The deck's `EEItemAttached` hands the battery to the energy manager (`EntityAI.c:1181-1182`), which plugs the deck into it (`ComponentEnergyManager.c:1453-1466`). `PlugInDevice` updates `m_CanWork` and wakes the plugged device (`ComponentEnergyManager.c:1652-1680`, `1557-1576`), which runs the deck's `DeviceUpdate` at once. A 9V is switched on from creation (`switchOnAtSpawn = 1`, `DZ/gear/consumables/config.cpp:588-598`), so with any charge `OnWorkStart` fires here, inside the battery's attach event. One more unknown sits here: a battery is created holding its config charge (`ComponentEnergyManager.c:187-188`), and script does not show whether its saved charge is loaded before the attach event, so a battery saved flat may look charged at this moment.
4. **The tape attaches.** Before or after step 3. The order of a parent's attachments during load is the engine's.
5. **`AfterStoreLoad` on the deck.** After both.

So the plan's concern holds: `OnWorkStart` fires during the load, at the first moment the deck is switched on and powered, and that can be before the tape exists. Whether the moment is the battery's attach event or `OnStoreLoad` itself depends on an order script does not show. A deck that validates its tape inside `OnWorkStart` and switches off on failure would do so at step 3 in that case and never resume. Zen's device is not evidence either way: with no valid cassette it falls back to its radio mode before it refuses (`ZenMusic:...Zen_MusicDeviceBase.c:240-247`).

Proposed for step 4, replacing the wording of After load in the plan's Architecture table (no ruling changes; T-9 stands):

- **`OnWorkStart` defers during init.** On the server, when `IsInitialized()` is false, it records that work has started and returns without validating. Vanilla uses the same test to keep attach-time and start-time reactions out of init (`PowerGenerator.c:149`, `PowerGenerator.c:228`, `ComponentEnergyManager.c:1442`).
- **`AfterStoreLoad` starts the deck.** It calls `super` (`ItemBase.c:3222-3236`), then validates the tape and starts the remembered track from its beginning, or switches off.
- **`DeferredInit` covers a fresh spawn.** A deck created with battery and tape in one frame (the test kit) never gets `AfterStoreLoad`. Its `DeferredInit` override (`EntityAI.c:277-280`) runs the same validate-and-start when work was recorded and nothing has started yet.

Unresolved, for T0-20: whether attachments exist before the parent's `OnStoreLoad`; the order of the two attachments; whether a battery's saved charge is in place at its attach event; and whether the call queue runs between entity loads at server start. The proposed design is indifferent to the first three and depends on the fourth: if the queue runs during the load, `IsInitialized()` can already be true at step 3 and the deferral is skipped. The row's server debug line should carry `init=<0|1> tape=<0|1> battery=<0|1> phase=<storeload|attach|live>` at every `OnWorkStart`, so one restart answers all four.

## Other findings for step 3 and step 4

| # | Finding | Evidence | Effect |
| --- | --- | --- | --- |
| R1 | The energy manager's own tick defaults to 15 seconds, and the `Radio` block sets no `updateInterval` | `ComponentEnergyManager.c:20`, `200`, `223-226` | Battery drain is taken in 15-second steps, and a flat battery is noticed at the next step. The measured run-down time in T0-10 carries that granularity. `updateInterval` is a config value (vanilla configs use values from 1 to 50); changing it does not change the drain rate (`ComponentEnergyManager.c:726`). No change proposed for Phase 0 |
| R2 | Removing the battery stops work in the same call, not at the next tick: the unplug runs `DeviceUpdate`, and the deck has no stored energy to draw on | `ComponentEnergyManager.c:1474-1484`, `447-473`, `1717-1745`, `1252-1278` | Supports the 1-second limit in T0-10. The deck's own `EEItemDetached` handler still publishes stopped |
| R3 | The manager keeps a reference to every effect it creates | `scripts/3_Game/DayZ/EffectManager.c:3`, `40-42`, `153-166` | Confirms the plan's single teardown path |
| R4 | Each start sets the wave to the effect's maximum volume on its first frame | `EffectSound.c:469-486` | The Phase 1 volume steps (T-14) set `SetSoundMaxVolume` (`EffectSound.c:878-883`) before `SoundPlay`, not only `SetSoundVolume` |
| R5 | The effect's frame update is driven from `MissionGameplay.OnUpdate` | `EffectSound.c:455-460` | Client only. Nothing to do on the server |
| R6 | `GetTickTime()` returns seconds since game start as a `float` | `scripts/3_Game/DayZ/Global/Game.c:910-914` | At 12 hours of uptime a `float` still resolves a few milliseconds. No concern for a 12-hour restart cycle |

## Plan lines to change at v0.4

None of these needs a ruling.

| Plan location | Change |
| --- | --- |
| Architecture, After load | Replace with the three bullets under Question 8. During a load, `OnWorkStart` comes from whichever of `OnStoreLoad` and the battery's attach event first finds the deck switched on and powered |
| Architecture, Following the carrier | Add `OnInventoryEnter` and `OnInventoryExit` as re-parent triggers |
| Architecture, Debug lines | Client lines gain `pos=<GetCurrPosition>`; the server `start` line gains `init=`, `tape=`, `battery=`, and `phase=` |
| Phase 0 step 6 | Run the two-play `SetStartOffset` unit probe before T0-04 |
| Recon question 1 | Closed for Phase 0: the refusal stands. Hiding the prompt is possible through a `GetActions` filter or own action classes; a later phase can choose it |
| Risks K11 | Reword: `OnWorkStart` fires during the load as soon as the deck is switched on and powered; T0-20's debug fields record the order |
| Prerequisites | Mark PR1, PR2, PR3, and PR5 done. PR7 is done for the replica only until the live mirror and the Steam install are hashed |

## Addendum, November 10, 2026: steps 2 to 4 (rules)

Recorded here so the plan's v0.4 picks them up. Each is on `feat/nimby-music`; none is a ruling.

| # | Item | Where | Note |
| --- | --- | --- | --- |
| A1 | `requiredAddons` gains `DZ_Scripts` and `DZ_Sounds_Effects` beside the plan's four | `NimbyTapes/config.cpp` | `CfgSlots` is defined in `DZ_Scripts` (`scripts/config.cpp:11`); the processor and curve the sound sets name are in `DZ_Sounds_Effects`. The DPV config lists both |
| A2 | `doppler = 0` on the sound sets | `NimbyTapes/config.cpp` | Not in the plan or in the Question 4 table. Chosen so a deck in a passing vehicle keeps its pitch in T0-13 |
| A3 | The probe tape has no track | `NimbyTapes/config.cpp` | A deck holding it refuses to start (`config-empty`). It exists for T0-25 only |
| A4 | The tape slot's icon is the vanilla placeholder | `NimbyTapes/config.cpp` | `set:dayz_inventory image:missing` (`scripts/config.cpp:159-163`). An own icon is Phase 1 work |
| A5 | `ValidateTape` also refuses a track length of zero or less, or above 65,535 seconds | `Scripts/3_Game/nimbytapes_rules.c` | Reason `config-bad-length`. Not one of the 18 named cases |
| A6 | The Wilga model has no proxy and no retexture selection | Strings read of `D:\DayZProjects\DZ\gear\radio\unitra_wilga.p3d` | An attached tape is not drawn on the deck. A second vanilla cassette model exists at `dz\gear\radio\cassette.p3d`. Informational, for T0-25 |
| A7 | Fixture hashes (ffmpeg 9.0.2, the same under Windows PowerShell 5.1 and PowerShell 7.6.9) | `logs\tapes-timingtrack-20261110-104554.log`, `-104557.log` in the worktree | `timing_a.ogg` `7203747DDC69751404A70573A8B2417B8416F7F56743DEC654E2DDFAC78A1496`; `timing_b.ogg` `95AD781637FE4BFA7A009F70F5E370208E0157985754C3DB52C6E988BEE03FCC` |
| A9 | Self-review fixes, same day | `tools\Test-TapesAssets.ps1`, `tools\New-TimingTrack.ps1`, `Scripts/3_Game/nimbytapes_rules.c` | The asset check probes every track file the config names (it skipped any outside `sound\`), walks `sound\` at any depth, follows config inheritance, and requires one sample per shader. Both tools take a relative `-AddonPath`. The generator renames a finished track into place. `PreviousTrack` takes the elapsed time unrounded, so 3.5 seconds restarts the track (T-6). This review also corrected Questions 1, 3, 6, and 8 and the PR7 row above |
| A8 | T0-02, self-test half | `logs\build-20261110-114255\build.log` in the worktree, artifact `@NIMBY-20261110-114255` | `SELFTEST Tapes ran=yes cases=18 pass=18 fail=0`; the 18 cases print by name |

## Status

Step 1 is complete. Steps 2 and 3 are complete, and the rules part of step 4 is in (commits `3d13f76`, `ca6c438`, `49fbd44`). The rest of step 4 (deck, tape memory, actions, hand pose, test kit, debug lines) is not started. No question for Jeff came out of the recon.
