# Plan 10 - Greet (`NimbyGreet`)

Draft v0.2, October 21, 2026 (evening). Changes since v0.1: the Discord invite is a button that opens the Steam overlay plus a copy button (G6, G10); the caps rise and the window becomes a section list with a text pane (G9); the content is now specified (G11): the one rule, how to join, restarts, difficulty, the Enhanced Vanilla changes, every installed mod with its default key, and questions; the draft text lives in the repository at `server/profile-overrides/NimbyGreet/greet.json` (G12). Owner: Jeff Fabish. Implementer: a Claude Code session in worktree `feat/greet`, coordinated by the orchestrator. Status: draft for Jeff's review. Jeff ruled on October 21, 2026 that Greet joins the first Workshop release with Legs, Ears, and Scrawl, so this plan is written before Central Scrutinizer starts and the release gate checklist (`docs/operations/2026-10-20-release-gate-legs-ears-scrawl.md`) gains a Greet column once Phase 1 passes.

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30.164078 / Nasdara.
Release: with the first Workshop release, gated (see [shared rules, 9.2](2026-10-15-shared-rules.md)). Greet persists nothing on any entity, so the content-addon boots in 6.1 do not apply.
Origin: the Greet row in `docs/research/candidate-features.md` (proposed and named by Jeff, October 16, 2026).

Greet shows a welcome window after a player spawns: server rules, the Discord invite, a few tips, and the restart schedule. The text is a JSON file in the server profile folder, sent to the client at connect, so Jeff edits it without a client update. Each client remembers which version it has dismissed, so a returning player sees the window again only when the text changes. The vanilla hourly `messages.xml` lines stay; the per-mod join lines move into the window once Greet is live.

## What this is not

- Not a rules engine. Greet displays text; it enforces nothing and records nothing about who read it.
- Not a replacement for `messages.xml`. The hourly server messages stay vanilla; only the on-connect join lines move into the window, and only at Greet's release.
- Not a motd that nags. A dismissed version never shows again on that client unless the text version changes or the player opens it from the pause menu.
- Not the Menu addon. Greet has no settings page and no key binding; "show again" lives in the pause menu as one button until Menu exists (shared rules 5).
- Not a save-footprint change: no entity data, no `OnStoreSave`/`OnStoreLoad`, no persistent class.

## Current state (verified October 21, 2026)

Every `file:line` is relative to `vendor/dayz-1.30-scripts/scripts/` (build 1.30.164078.38).

| Item | State | Source |
| --- | --- | --- |
| Source | None. New addon folder `mods/own/nimby-pack/NimbyGreet/` | - |
| Connect hook, server | `MissionServer.OnEvent`: `ClientNewEventTypeID` (`:326`) fires when the identity connects; `ClientReadyEventTypeID` (`:345`) fires when the player is in the world and calls `OnClientReadyEvent` then `InvokeOnConnect` (`:356-357`, defined `:435`). Greet sends on `ClientReadyEventTypeID`, the later of the two, so the client has a player to attach the window to | `5_Mission/DayZ/mission/missionServer.c:309-357,435` |
| Server to client RPC | `GetGame().RPCSingleParam(target, rpc_type, param, guaranteed, recipient)`; `recipient` is the `PlayerIdentity`, so one client receives the text | `3_Game/DayZ/Global/Game.c:1013`; `ScriptRPC` at `3_Game/DayZ/gameplay.c:104` |
| Client receive | `PlayerBase.OnRPC(sender, rpc_type, ctx)` (`:5727`) is the per-player RPC entry the fleet already uses (Legs `6550102`, `6550103`) | `4_World/DayZ/Entities/ManBase/PlayerBase.c:5727` |
| Spawn-complete point, client | `PlayerBase.OnPlayerLoaded` (`:2569`) runs on the controlled player once the character is loaded; it calls `g_Game.GetUIManager().CloseAll()` (`:2594`) and then `mission.OnPlayerRespawned(this)` (`:2596`). **Any menu opened before this point is closed by `CloseAll()`**, so Greet opens after `OnPlayerLoaded`, never on receipt of the RPC | `PlayerBase.c:2569-2601`; `missionGameplay.c:1677` (`OnPlayerRespawned`) |
| Connecting dialogue | `MissionGameplay` enters `MENU_CONNECTION_DIALOGUE` (`3_Game/DayZ/constants.c:221`, id 46) when the input buffer is full (`missionGameplay.c:1752,1797`) and closes it when the player dies or goes unconscious (`:1767-1769`) or the buffer drains (`:1802-1805`). Greet MUST NOT open while that menu is up and MUST yield to it if it appears | `5_Mission/DayZ/mission/missionGameplay.c:1740-1805`; `3_Game/DayZ/constants.c:221` |
| Scripted menu | `UIScriptedMenu` (`3_Game/DayZ/tools/UIScriptedMenu.c`); shown with `UIManager.ShowScriptedMenu(menu, parent)` (`UIManager.c:9`), hidden with `HideScriptedMenu` (`:10`); `GetMenu()` (`:59`) reports the current one. Scrawl's note menu is the fleet precedent (`mods/own/nimby-pack/NimbyNotes/Scripts/5_Mission/nimbynotes_notemenu.c`) | `3_Game/DayZ/tools/UIManager.c:4-59` |
| JSON | `JsonFileLoader<T>.LoadFile(filename, out data, out errorMessage)` (`:7`) and `SaveFile` (`:42`) return `bool` with an error string; the older `JsonLoadFile`/`JsonSaveFile` (`:105,134`) swallow errors and are not used | `3_Game/DayZ/tools/JsonFileLoader.c:7,42,105,134` |
| Client profile path | `$profile:` on a client is `%LOCALAPPDATA%\DayZ` unless the client runs with `-profiles=` (Ears E10) | Ears plan v0.8, E10 |
| Loading screen | `class LoadingScreen` (`DayZGame.c:704`) is Splash's concern, not Greet's; Greet never touches it | `3_Game/DayZ/DayZGame.c:704` |
| RPC block | `65511xx` is the next free block after Menu's `65510xx` (shared rules 4.1) | `docs/plans/2026-10-15-shared-rules.md` section 4.1 |

## Decisions

| # | Decision | Choice | Status |
| --- | --- | --- | --- |
| G1 | Display name | Greet. Code prefix `NimbyGreet`, log prefix `[NIMBY][Greet]`, RPC block `65511xx` (`6551101` text push, server to the owning client) | Name decided by Jeff, October 16, 2026; prefixes and block are Claude defaults pending the shared rules 4.1 row |
| G2 | Release | With the first Workshop release (Legs, Ears, Scrawl), not after Central Scrutinizer | Decided by Jeff, October 21, 2026 |
| G3 | Text source | `$profile:NimbyGreet\greet.json` on the server, read at mission start and re-read on every `ClientReadyEventTypeID` so an edit takes effect for the next joiner without a restart. A missing file writes defaults; a malformed file logs one `[NIMBY][Greet] greet.json invalid: <error>` line and sends the last good copy loaded this boot, or nothing on first boot | Claude default |
| G4 | Versioning | `version` is an integer in `greet.json` that Jeff raises when the text changes. The client stores the last dismissed version in `$profile:NimbyGreet\seen.json`; the window opens when the received version is greater than the stored one. A missing or malformed `seen.json` means version 0, so the window shows | Claude default |
| G5 | When it opens | After `OnPlayerLoaded` on the controlled player, delayed by `openDelaySeconds` (default 2) through the gameplay call queue, and only if no scripted menu is open (`GetMenu()` is null) and the player is alive and conscious. If a menu is open, retry every second for up to `openRetrySeconds` (default 30), then give up for this spawn and log once | Claude default; the connecting-dialogue interaction is Phase 0 item 3 |
| G6 | What it shows | One window: title, a section list on the left, the selected section's text on the right, a **Join Discord** button and a **Copy invite** button under the text whenever the section carries a `url`, a "Do not show again for this version" checkbox checked by default, and one Close button. Sections, headings, body, and urls come from `greet.json`; nothing is hardcoded but the layout | Claude default; revised October 21, 2026 evening for Jeff's hyperlink request |
| G7 | Show again | A "Server info" button in the vanilla pause menu (`modded class InGameMenu`) reopens the last received text; it never changes `seen.json`. No key binding | Claude default |
| G8 | Join lines | At Greet's release the per-mod on-connect lines leave `db/messages.xml` and become Greet sections; the hourly lines stay. The release gate's step 5 is amended to remove the on-connect blocks instead of adding them | Claude default following Jeff's October 19 rule that join lines ship with each mod |
| G9 | Limits | Text payload capped at 12,000 characters total and 12 sections of 2,000 characters each; longer input is truncated with a log line, never rejected, so a typo in the file cannot leave players with no window. Raised from 4,000 and 6 on October 21, 2026 because the server-information text Jeff supplied is longer on its own | Claude default |
| G10 | Discord link | DayZ scripted menus have no clickable text, so the invite is two buttons: **Join Discord** calls `GetGame().OpenURL(url)` (`3_Game/DayZ/Global/Game.c:1401`), which opens the Steam overlay browser on the client; **Copy invite** calls `GetGame().CopyToClipboard(url)` (`Game.c:663`). The URL is also printed as text under the buttons for a client with the overlay disabled. Phase 0 confirms `OpenURL` works on a dedicated-server client (the addendum F6 note) | Claude default |
| G11 | Content | Sections, in order: Welcome (name, tagline "Where all hope dies and the Badlands begin", fresh world date); The one rule ("Don't be a dick"; glitch bases are removed); Discord (invite, with the buttons); How to join; Restarts; Difficulty (including the "3rd & 1st Person" browser note); Enhanced Vanilla changes; Mods (one entry per installed NIMBY mod: what it does and its default key, written by hand at each release alongside the join lines in G8); Questions. Text supplied by Jeff on October 21, 2026; the draft is in the repository (G12) | Jeff supplied the text; structure is Claude's |
| G12 | Draft location | `server/profile-overrides/NimbyGreet/greet.json` in the repository, the profile-folder twin of `server/mission-overrides`; copied to `$profile:NimbyGreet\greet.json` on the host at release and on every text change, with `version` raised each time. The repository copy is the record; the host copy is what players see | Claude default |

## Data format

`greet.json` (server):

| Field | Type | Required | Description |
| --- | --- | --- | --- |
| `version` | `integer` | Yes | Raised by Jeff on every text change (G4) |
| `title` | `string` | Yes | Window title, 60 characters maximum |
| `tagline` | `string` | No | One line under the title, 80 characters maximum |
| `sections` | array of `Section` | Yes | 1 to 12 entries (G9) |
| `openDelaySeconds` | `number` | No | Default 2 (G5) |
| `openRetrySeconds` | `number` | No | Default 30 (G5) |

`Section`:

| Field | Type | Required | Description |
| --- | --- | --- | --- |
| `heading` | `string` | Yes | 40 characters maximum |
| `body` | `string` | Yes | Plain text, 2,000 characters maximum; `\n` for line breaks; no markup |
| `url` | `string` | No | When present the section shows the Join and Copy buttons (G10); `https://` only |

`seen.json` (client):

| Field | Type | Required | Description |
| --- | --- | --- | --- |
| `dismissedVersion` | `integer` | Yes | Last version dismissed with the checkbox on |

Default `greet.json` written on first boot carries placeholder sections (Rules, Discord, Tips, Restarts) with the restart schedule from `docs/operations/restart-schedule.md`; the real text is the repository draft (G12), which Jeff reviews and copies to the host at release.

## Phase 0 - prove the window survives spawn

1. Record builds and reference hashes in `docs/baselines/`; add the `NimbyGreet` row and a stub `config.cpp` to `pack.json` on `main` first (the Ears E6 precedent) so the probe builds.
2. Probe: a server `modded class MissionServer` that sends a fixed string on `ClientReadyEventTypeID`; a client `modded class PlayerBase` that receives it in `OnRPC`, stores it, and opens a `UIScriptedMenu` with one text widget and one Close button from `OnPlayerLoaded` after a 2 s delay. Log every step with the `[NIMBY][Greet]` prefix.
3. On the replica (`-ServerPath D:\DayZServer -InstanceId 998`): fresh spawn, relog onto a live character, respawn after death. Record whether the window appears each time, whether `CloseAll()` ever removes it, and what happens when the connecting dialogue (`MENU_CONNECTION_DIALOGUE`) is up at the moment Greet tries to open (force it with a throttled connection or by spamming input during load).
4. Confirm the window takes input focus and releases it on Close, that the inventory and pause menu open normally afterward, and that nothing is logged as `SCRIPT (E)`.
5. Press a probe **Join Discord** button that calls `GetGame().OpenURL("https://discord.gg/kH3eJ5mdEv")` and a **Copy invite** button that calls `CopyToClipboard`; record whether the Steam overlay opens on the invite and whether the clipboard holds the URL afterward (G10).
6. Confirm `JsonFileLoader.LoadFile` reports a malformed `greet.json` with a readable error and that the server keeps running.

Exit: the window shows on all three spawn paths, never fights the connecting dialogue, and closes cleanly. Stop condition: if a scripted menu cannot be held open after `OnPlayerLoaded` on 1.30, Greet falls back to a HUD text panel (no focus, auto-hides after a timeout) and this plan is revised.

## Phase 1 - the window

- `config.cpp` (`CfgPatches` requiring `DZ_Data`; `3_Game`, `4_World`, `5_Mission` modules), `Scripts/3_Game/nimbygreet_text.c` (engine-free: payload parsing, caps per G9, version compare per G4; self-test cases for empty, oversized, malformed, equal, lower, and higher versions), `Scripts/4_World/nimbygreet_playerbase.c` (RPC receive, `OnPlayerLoaded` hook, open scheduling per G5), `Scripts/5_Mission/nimbygreet_missionserver.c` (file load, send on ready), `Scripts/5_Mission/nimbygreet_menu.c` and `GUI/layouts/nimbygreet_window.layout` (vanilla widgets only), `Scripts/5_Mission/nimbygreet_ingamemenu.c` (G7 button), `Scripts/5_Mission/nimbygreet_missionbase.c` (self-test hook, `cases=N` completion line per shared rules 8).
- Client `seen.json` written only when the checkbox is on at Close.
- Logging per shared rules 4.3: production lines for load, send (player name and Steam64 per 4.2, version), open, dismiss; debug flag `$profile:NimbyGreet\debug` adds the retry loop and the raw payload size.

## Phase 2 - release

- Jeff writes the real `greet.json` (rules, Discord invite, tips, restart schedule) and sets `version` to 1.
- The join lines for Legs, Ears, and Scrawl move from the `messages.nimby-mods.patch.xml` on-connect block into Greet sections (G8); the release gate step 5 text is amended.
- Gate per shared rules 9.2 with the first Workshop release; the Greet column on the checklist lists G-0 to G-7 below.

## Tests

| Id | Scenario | Required result | Evidence |
| --- | --- | --- | --- |
| G-0 | Self-test | All cases pass | `[NIMBY][Greet] selftest complete; cases=N failures=0` |
| G-1 | Fresh spawn, no `seen.json` | Window opens after the delay with the server text | `[NIMBY][Greet] open version=1` on the client; screenshot |
| G-2 | Close with the checkbox on, relog | No window | `seen.json` holds `dismissedVersion=1`; no open line |
| G-3 | Jeff raises `version` to 2, player relogs | Window opens again | `open version=2` |
| G-4 | Close with the checkbox off, relog | Window opens again | No `seen.json` write; `open version=2` |
| G-5 | Die and respawn | Window behavior per `seen.json`, never twice in one spawn | One open line at most per spawn |
| G-6 | Pause menu "Server info" | Window opens; `seen.json` unchanged | Open line with `source=menu` |
| G-7 | Malformed `greet.json` on the server | Last good text sent or nothing on first boot; server keeps running | `greet.json invalid:` line, 0 `SCRIPT (E)` |
| G-8 | Connecting dialogue up at open time | Greet waits, then opens or gives up at `openRetrySeconds` | `open deferred` lines, then `open` or `gave up` |
| G-9 | Oversized text (7 sections, 6,000 characters) | Truncated to the caps, window still shows | `greet.json truncated` line |
| G-10 | Boot without Greet on storage that ran with it | Clean | 0 "Scripted variables corrupted" (shared rules 9.2) |
| G-11 | Join Discord button | Steam overlay opens on the invite page | Screenshot; `[NIMBY][Greet] open url` line |
| G-12 | Copy invite button | Clipboard holds the URL | Paste into chat shows `https://discord.gg/kH3eJ5mdEv` |
| G-13 | Twelve sections, one at 2,000 characters | Every section listed and readable; the long one scrolls | Screenshot |

## Risks

| Risk | Mitigation |
| --- | --- |
| `CloseAll()` in `OnPlayerLoaded` or a later vanilla hook closes the window | Open after `OnPlayerLoaded` with a delay (G5); Phase 0 item 3 records every closer found |
| The connecting dialogue and Greet fight for focus on a slow connection | Greet yields and retries (G5); G-8 proves it |
| A text change is forgotten and `version` is not raised | Returning players see nothing new. Mitigation is a line in the ops doc: raise `version` with every edit; the server logs the version it loaded at every boot |
| Players dismiss the window without reading it | Accepted. Greet is information, not enforcement; the rules also live in Discord |
| Release 1 slips while Greet is built | Accepted by Jeff on October 21, 2026 when he folded Greet into the release. Phase 0 is a one-day box; Phase 1 is two to three days of build plus the test block |
