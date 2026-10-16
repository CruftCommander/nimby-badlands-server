# Plan 7 - Ears (`NimbyEarplugs`)

Draft v0.4, October 16, 2026. Changes since v0.3: default key N, not End (E2, Jeff); End is free again. Owner: Jeff Fabish. Implementer: a Claude Code session in worktree `feat/ears`, coordinated by the orchestrator. Status: draft for Jeff's review. Changes since v0.2: display name Ears (Jeff, October 16, 2026; was Hush), worktree `feat/ears`; EP-005 is now the pack's first proof that a NIMBY binding is rebindable in the vanilla Controls page (shared rules 5). Changes since v0.1: display name Hush (E1).

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
| Vanilla reset point | `missionGameplay.c:1690` restores `m_volume_sound` on its own schedule | To be recorded in Phase 0 |
| Default key | `kN`; `bin/constants.xml:507`; in `bin/preset_X1MouseKey.xml` it appears only as the modifier of the developer terrain-brush inputs `UABuldBrushOuterUp` and `UABuldBrushOuterDown` (`kN` plus mouse wheel, `:881-890`), so a plain N press is free in play | Shared rules 5 |
| Legs input precedent | `Data/Inputs.xml` with `<modded_inputs>` | `mods/own/nimby-pack/NimbyAutorun/Data/Inputs.xml` |

## Decisions

| # | Decision | Choice | Status |
| --- | --- | --- | --- |
| E1 | Display name | Ears. Code prefix `NimbyEarplugs`, log prefix `[NIMBY][Earplugs]`, Controls entry "Ears: toggle earplugs" | Decided by Jeff, October 16, 2026 (Hush on October 15) |
| E2 | Default key | N (`kN`), input `UANimbyEarplugsToggle`. End was the October 15 choice and is free again | Decided by Jeff, October 16, 2026 |
| E3 | Reduced level | 15 percent of the player's saved volume, 0.5 s fade in and out; both in a client-side constant block, not a profile file, because the client has no profile write path the pack relies on | Claude default |
| E4 | Indicator | A small vanilla notification on each toggle ("Earplugs in" and "Earplugs out"); no HUD icon | Claude default |
| E5 | Relog | Earplugs come out on every mission start | Claude default |

## Phase 0 - prove set and restore

1. Record builds and reference hashes in `docs/baselines/`.
2. On the local test server, call `SetSoundVolume(0.15, 0.5)` from a key press and confirm effect audio drops while VOIP is unchanged.
3. Open and close the inventory, the pause menu, and the Controls menu with earplugs in; record which of them triggers the `missionGameplay.c:1690` reset and whether the reduced level survives.
4. Confirm `kN` reaches the input and nothing else reacts to a plain press; confirm N plus mouse wheel does nothing on a release build (the brush inputs are developer tools).

Exit: the reduced level can be held across menus, or the reset point is known and the toggle can reapply after it.

## Phase 1 - toggle

- `config.cpp` (`CfgPatches` requiring `DZ_Data`), `Data/Inputs.xml`, `Scripts/5_Mission` modded `MissionGameplay` reading the input in `OnUpdate`, the toggle state, the reapply hook from Phase 0, and the notifications (E4).
- Self-test: the toggle state machine (in, out, reapply) as an engine-free class.
- `pack.json` row added by the orchestrator.

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

## Risks

| Risk | Mitigation |
| --- | --- |
| The vanilla reset fires every frame or on an event Earplugs cannot hook | Phase 0 item 3 is a stop condition; fallback is reapplying in `OnUpdate` when the scene volume differs from the expected level |
| Central Scrutinizer wanted End as a fallback key | Central Scrutinizer N2 (shared rules 5) now lists Home, then F10, and F10 is already bound in vanilla (`preset_X1MouseKey.xml:446,480,824`), so Central Scrutinizer takes Home |
