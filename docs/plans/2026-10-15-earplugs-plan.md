# Plan 7 - Ears (`NimbyEarplugs`)

Draft v0.7, October 17, 2026. Changes since v0.6: Phase 0 test block run on the replica (test record `docs/testing/2026-10-16-ears-phase0.md`): the reduced level held across every menu including the Sound slider with no reapply, so the E9 sampler stays as a safety net; the vanilla notification leak found and fixed (E4); the binding moves to the vanilla Others tab because 1.30 ignores a new `<sorting>` group from `modded_inputs` (EP-005 passed, E11); the client `$profile:` is `%LOCALAPPDATA%\DayZ` (E10 corrected). Status: Phase 1 rebuild pending with the two fixes. Changes since v0.5: Phase 1 written ahead of the Phase 0 test block (Jeff, October 16, 2026): the toggle reapplies the level whenever the scene drifts from the expected value (E9), which covers every reset point Phase 0 can find; earplugs come out at death (E7); the saved level is read live (E8); logging follows shared rules 4.3 with the `$profile:NimbyEarplugs\debug` flag. Status: Phase 0 and Phase 1 built in `build\@NIMBY-20261016-163053` and later, awaiting the in-game test block. Changes since v0.4: the `pack.json` row lands at Phase 0, not Phase 1 (E6, Jeff), because the Phase 0 probe is a client addon that has to build and load; `missionGameplay.c:1690` identified as `OnPlayerRespawned`, and the other vanilla writes to the scene volume listed, so Phase 0 item 3 names the real candidates. Changes since v0.3: default key N, not End (E2, Jeff); End is free again. Owner: Jeff Fabish. Implementer: a Claude Code session in worktree `feat/ears`, coordinated by the orchestrator. Status: draft for Jeff's review. Changes since v0.2: display name Ears (Jeff, October 16, 2026; was Hush), worktree `feat/ears`; EP-005 is now the pack's first proof that a NIMBY binding is rebindable in the vanilla Controls page (shared rules 5). Changes since v0.1: display name Hush (E1).

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30.164078 / Nasdara.
Release: independent, gated (see [shared rules, 9.2](2026-10-15-shared-rules.md)).
Feasibility: `docs/design/puff-sprout-scrawl-earplugs-feasibility.md`.

Ears lets a player press one key to lower the game's effect volume and press it again to restore it. It is client-only: no server code, no RPC, no item, no save footprint. Voice chat is untouched so a player in a vehicle can still hear the passengers.

## What this is not

- Not an item. No earplugs entity exists or spawns; the key is the whole feature.
- Not a server mod. The addon has no server-side script and sends nothing.
- Not a volume setting. The player's saved volume (`g_Game.m_volume_sound`) is never written; the toggle applies a temporary level on top of it.

## Current state (verified October 15, 2026)

| Item | State | Source |
| --- | --- | --- |
| Source | None. New addon folder `mods/own/nimby-pack/NimbyEarplugs/` | - |
| Volume API | `g_Game.GetSoundScene().SetSoundVolume(volume, fadeSeconds)` | `missionGameplay.c:1690`, `MainMenuVideo.c:32-42` |
| Saved level | `g_Game.m_volume_sound` from `GetSoundScene().GetSoundVolume()` | `DayZGame.c:1147` |
| VOIP level | Separate (`GetVOIPVolume`, `:1150`) | `DayZGame.c:1150` |
| Vanilla reset points | `missionGameplay.c:1690` is `OnPlayerRespawned` (restores `m_volume_sound`, 1 s fade). Others: death `DayZPlayerImplement.c:870` (0, instant), unconscious `PlayerBase.c:3778` (0, 2 s), waking `PlayerBase.c:3832` (restores `m_volume_sound`, 1 s). The Options > Sound effects slider goes through `NumericOptionsAccess` (`OptionsMenuSounds.c:49,80,88`), engine side; inventory and pause menu have no script write | Which of these, if any, the toggle must survive is recorded in Phase 0 |
| Default key | `kN`; `bin/constants.xml:507`; in `bin/preset_X1MouseKey.xml` it appears only as the modifier of the developer terrain-brush inputs `UABuldBrushOuterUp` and `UABuldBrushOuterDown` (`kN` plus mouse wheel, `:881-890`), so a plain N press is free in play | Shared rules 5 |
| Legs input precedent | `Data/Inputs.xml` with `<modded_inputs>` | `mods/own/nimby-pack/NimbyAutorun/Data/Inputs.xml` |

## Decisions

| # | Decision | Choice | Status |
| --- | --- | --- | --- |
| E1 | Display name | Ears. Code prefix `NimbyEarplugs`, log prefix `[NIMBY][Earplugs]`, Controls entry "Ears: toggle earplugs" | Decided by Jeff, October 16, 2026 (Hush on October 15) |
| E2 | Default key | N (`kN`), input `UANimbyEarplugsToggle`. End was the October 15 choice and is free again | Decided by Jeff, October 16, 2026 |
| E3 | Reduced level | 15 percent of the player's saved volume, 0.5 s fade in and out; both in a client-side constant block, not a profile file, because the client has no profile write path the pack relies on | Claude default |
| E4 | Indicator | A small vanilla notification on each toggle ("Earplugs in" and "Earplugs out"); no HUD icon. Ears ships `modded class NotificationUI` overriding `RemoveNotification` with a serial fade-map key, because the vanilla key (`count + show_time`, `NotificationUI.c:79`) repeats for same-length toasts in quick succession and the repeated toast then never leaves the screen (seen October 17, 2026). The override moves to a shared NIMBY base addon when a second mod needs it | Claude default; keep-and-fix decided by Jeff, October 17, 2026 |
| E5 | Relog | Earplugs come out on every mission start | Claude default |
| E6 | `pack.json` row timing | The `NimbyEarplugs` row and a stub `config.cpp` land on `main` before the worktree branches, so the Phase 0 probe builds and `main` stays buildable; exception to shared rules section 3 ("at that addon's Phase 1") | Decided by Jeff, October 16, 2026 |
| E7 | Death and unconsciousness | Vanilla owns the level while the player is dead (0, `DayZPlayerImplement.c:870`) or unconscious (0 over 2 s, `PlayerBase.c:3778`; restore on waking, `:3832`). Ears never reapplies while unconscious, and earplugs come out at death so the vanilla respawn restore (`missionGameplay.c:1690`) and Ears agree | Claude default |
| E8 | Saved level read live | The expected level is `g_Game.m_volume_sound * 0.15` evaluated each second, not a captured copy, so a Sound slider change with earplugs in moves the target with it; the slider value is never written by Ears | Claude default |
| E9 | Reapply policy | After each apply a 1 s settle window ignores the fade; past it, a scene level more than 0.01 from the expected level is written back at most once per second, with a production log line per reapply. A reset that fires every frame is therefore corrected once a second and visible in the log rather than fought per frame | Claude default. Phase 0 (October 17, 2026): no menu reset the level and no reapply fired, so no direct hook is needed; the sampler stays as the safety net |
| E10 | Logging | Production: reapply lines and the death take-out. Debug (`$profile:NimbyEarplugs\debug` on the client; `$profile:` is `%LOCALAPPDATA%\DayZ` unless the client runs with `-profiles=`, not `Documents\DayZ`): toggles, menu changes, RESET, one sample per second, the respawn restore. Self-test runs on client and server when `$profile:NimbyEarplugs\selftest` exists; the hook is on `MissionBase` so the boot check sees it, the only Ears code a server executes | Shared rules 4.3 and 8 |
| E11 | Controls tab | NIMBY bindings appear under the vanilla Unsorted tab. 1.30 ignores a `<sorting>` block in `modded_inputs` both for a new group (`nimby`, October 17 morning) and for an existing one (`others`, October 17 evening rebuild); `UAInputAPI.SortingCount` is engine side. A NIMBY tab waits for the Menu addon (shared rules 5). `Inputs.xml` keeps the `others` block as documentation of the attempt | Claude default, October 17, 2026 |

## Phase 0 - prove set and restore

1. Record builds and reference hashes in `docs/baselines/`. The `pack.json` row and stub addon are already on `main` (E6).
2. Build a probe: `Data/Inputs.xml` binding `UANimbyEarplugsToggle` to `kN`, a modded `MissionGameplay` that toggles `SetSoundVolume(0.15, 0.5)` on press and logs every call, the scene level once a second while earplugs are in, and every vanilla reset it can observe, to the client script log with the `[NIMBY][Earplugs]` prefix.
3. On the local replica (`-ServerPath D:\DayZServer -InstanceId 998`), press N and confirm effect audio drops while VOIP is unchanged.
4. Open and close the inventory, the pause menu, and the Options menu (Sound tab, then Controls tab) with earplugs in; then go unconscious and wake, and die and respawn, if the replica allows it in the session; record which of these resets the level.
5. Confirm `kN` reaches the input and nothing else reacts to a plain press; confirm N plus mouse wheel does nothing on a release build (the brush inputs are developer tools).

Exit: the reduced level can be held across menus, or the reset point is known and the toggle can reapply after it.

## Phase 1 - toggle

Written October 16, 2026, in the same build as the probe.

- `config.cpp` (`CfgPatches` requiring `DZ_Data`; `3_Game` and `5_Mission` modules), `Data/Inputs.xml` (`UANimbyEarplugsToggle` on `kN`, vanilla Others sorting group per E11), `Scripts/3_Game/nimbyearplugs_level.c` (engine-free state: in, out, settle, suppress, reapply; 16 self-test cases), `Scripts/3_Game/nimbyearplugs_notificationui.c` (E4 fade-map key fix), `Scripts/5_Mission/nimbyearplugs_missiongameplay.c` (input in `OnUpdate`, toggle, 1 Hz sample and reapply per E9, death and unconscious handling per E7, notifications per E4, debug logging per E10), `Scripts/5_Mission/nimbyearplugs_missionbase.c` (self-test hook).
- Answered by the Phase 0 record (October 17, 2026): E9's once-per-second reapply stays as the safety net (nothing reset the level); the Options Sound slider does not reset the scene level. Rebuild pending with the E4 notification fix and the E11 Others-tab sorting; the rebuild test captures the debug lines the first run missed.

## Phase 2 - release

- Release gate per shared rules 9.1 and 9.2 (no content-addon boots are needed; nothing persists).

## Tests

| Id | Case | Evidence |
| --- | --- | --- |
| EP-001 | N lowers effect volume; N restores it | Test record with the player's volume setting before and after |
| EP-002 | VOIP is audible with earplugs in | Two-client test with Jay |
| EP-003 | Opening and closing each menu from Phase 0 leaves earplugs in | Test record |
| EP-004 | Relog restores the saved volume | Test record |
| EP-005 | Rebinding to another key in Controls works and N no longer toggles | Test record |
| EP-006 | Ears is first through the worktree, build, and release flow (D17); the test record names the artifact | `build.log` SUMMARY PASS |
| EP-007 | Earplugs in, then unconscious and wake: no reapply while unconscious, level back to reduced within 2 s of waking | Test record, `reapply` line after waking |
| EP-008 | Earplugs in, then die and respawn: `earplugs out: player died` line, respawn at the saved level, N works again | Test record |
| EP-009 | Sound slider moved with earplugs in: expected level follows the new saved level | Test record, `sample` lines |

## Risks

| Risk | Mitigation |
| --- | --- |
| The vanilla reset fires every frame or on an event Earplugs cannot hook | Phase 0 item 3 is a stop condition; fallback is reapplying in `OnUpdate` when the scene volume differs from the expected level |
| Central Scrutinizer wanted End as a fallback key | Central Scrutinizer N2 (shared rules 5) now lists Home, then F10, and F10 is already bound in vanilla (`preset_X1MouseKey.xml:446,480,824`), so Central Scrutinizer takes Home |
