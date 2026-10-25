# Plan 10 - Greet (`NimbyGreet`)

Draft v0.3, October 25, 2026. Changes since v0.2, all from Jeff's requirement list of October 25, 2026: the section body is a markdown subset rendered through `RichTextWidget` markup instead of plain text (G13); any number of links per section, each rendered as a link button that opens the Steam overlay browser plus a copy button (G10 revised); `greet.json` carries the text in more than one language and the client picks its own (G14); a hotkey reopens the window (G7 revised, supersedes "no key binding"); animated images ship as looping muted `.mp4` clips played by `VideoWidget`, referenced from the text by id (G15); an accessibility baseline: full keyboard and controller navigation, a text-size control, no color-only meaning, and a motion-off switch (G16); the text is fetched by the client after spawn and sent in chunks per language instead of pushed whole at connect (G3 and G5 revised); the "do not show again" model gains a per-version override flag (G4 revised). v0.2 (October 21, evening) made the Discord invite a button plus a copy button, raised the caps, made the window a section list with a text pane, specified the content (G11), and placed the draft text at `server/profile-overrides/NimbyGreet/greet.json` (G12). Owner: Jeff Fabish. Implementer: a Claude Code session in worktree `feat/greet` (`D:\GitHub\nimby-badlands-server-wt\greet`, created October 25, 2026). Status: draft for Jeff's review. Jeff ruled on October 21, 2026 that Greet joins the first Workshop release with Legs, Ears, and Scrawl; the release gate checklist (`docs/operations/2026-10-20-release-gate-legs-ears-scrawl.md`) gains a Greet column once Phase 1 passes.

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30.164078 / Nasdara.
Release: with the first Workshop release, gated (see [shared rules, 9.2](2026-10-15-shared-rules.md)). Greet persists nothing on any entity, so the content-addon boots in 6.1 do not apply.
Origin: the Greet row in `docs/research/candidate-features.md` (proposed and named by Jeff, October 16, 2026).

Greet shows a welcome window after a player spawns: server rules, the Discord invite, how to join, the restart schedule, the mod list, and whatever else Jeff puts in it. The text is a JSON file in the server profile folder, written in a markdown subset, in as many languages as Jeff supplies, and fetched by the client after it spawns, so Jeff edits it without a client update. Each client remembers which version it has dismissed, so a returning player sees the window again only when the text changes and Jeff asks for it, or when the player opens it with the hotkey or from the pause menu. Images and short looping clips ship inside the client PBO and are placed by id from the text. The vanilla hourly `messages.xml` lines stay; the per-mod join lines move into the window once Greet is live.

## Requirements (Jeff, October 25, 2026)

| # | Requirement | Where it is met |
| --- | --- | --- |
| R1 | The window must be easily customizable | `greet.json` on the server (G3, G12); no layout change needed for a text change |
| R2 | Formatting, preferably markdown | Markdown subset (G13), converted on the client to `RichTextWidget` markup |
| R3 | Add and remove tabs or categories, each with its own content | Sections are the tabs (G6); add or remove a `sections` entry |
| R4 | Accessible, as far as a game needs | G16: keyboard, controller, text size, contrast, motion-off |
| R5 | Multiple languages | G14: one `languages` object per language code; client picks its own, falls back to the default |
| R6 | Reopen the window after connect, by hotkey | G7: `UANimbyGreetOpen`, default Insert, rebindable; pause-menu button stays |
| R7 | Dynamic or animated images such as GIFs (a plus) | G15: GIF converted to a looping muted `.mp4`, played by `VideoWidget`; shipped in the PBO |
| R8 | Hyperlinks that open in at least the Steam web browser | G10: each link is a button calling `OpenURL`, which opens the Steam overlay browser, plus a copy button |
| R9 | Dismiss, with "never show again" | G4: checkbox, per client, with Jeff's `forceShow` override per version |

Two constraints follow from the engine and are stated here so nobody expects otherwise: text changes are live (server file), but every image or clip is a Workshop release, because a client can only load media that is inside a PBO it has (G15); and nothing inside a `RichTextWidget` is clickable, so a link in the text is always paired with a button (G10).


## What this is not

- Not a rules engine. Greet displays text; it enforces nothing and records nothing about who read it.
- Not a replacement for `messages.xml`. The hourly server messages stay vanilla; only the on-connect join lines move into the window, and only at Greet's release.
- Not a motd that nags. A dismissed window never opens by itself again on that client unless Jeff raises `version` with `forceShow` true (G4). The player can always open it with the hotkey or from the pause menu.
- Not a browser. Links open in the Steam overlay browser through `OpenURL`; Greet renders no web content.
- Not a media channel. Images and clips are release-bound PBO content (G15); the server file only places them.
- Not the Menu addon. Greet has one key binding (G7) and the two in-window preferences in G16, stored in its own `$profile:NimbyGreet\prefs.json`; when Menu exists they register with it. Nothing depends on Menu.
- Not a save-footprint change: no entity data, no `OnStoreSave`/`OnStoreLoad`, no persistent class.

## Current state (verified October 21, 2026; rows from "Client to server RPC" down added October 25, 2026)

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
| Client to server RPC | `ScriptRPC` (`gameplay.c:104`): written on the client, `Send` evaluates on the server; the server answers with `RPCSingleParam` to the one identity. The fleet precedent is Legs `6550101` | `3_Game/DayZ/gameplay.c:104-112`; `Game.c:1013` |
| Rich text | `RichTextWidget extends TextWidget` (`EnWidgets.c:224`): `GetContentHeight`, `SetContentOffset` (scrolling), `GetNumLines`. The markup vanilla uses in scripts and the stringtable: `<image set=... name=.../>` (518 uses in `languagecore/stringtable.csv`), `<color hex=...>` and `<color rgba=...>`, `<b>`, `<p>`. `<i>`, `<u>`, `<font size=...>`, and `<br/>` are not used by vanilla and are unverified until Phase 0 item 7 | `1_Core/DayZ/proto/EnWidgets.c:189-233`; `grep` of `scripts/` and `languagecore/` on October 25, 2026 |
| Clickable text | None. No `RichTextWidget` or `Widget` member and no `ScriptedWidgetEventHandler` event reports a link click; `grep -rn "OnLink\|href"` over `scripts/` finds nothing. Every clickable thing is a `ButtonWidget` or a widget with `OnClick` | `1_Core/DayZ/proto/EnWidgets.c`; `3_Game/DayZ/tools/ScriptedWidgetEventHandler.c` |
| Open a URL | `GetGame().OpenURL(url)` (`Game.c:1401`) opens the Steam overlay browser; vanilla uses it for the Discord invite (`MainMenuNewsfeed.c:71`), the feedback tracker (`InGameMenu.c:424`), and Workshop mod pages (`ModsMenuDetailedEntry.c:202`). `CopyToClipboard` at `Game.c:663` | `3_Game/DayZ/Global/Game.c:663,1401`; `5_Mission/DayZ/GUI/NewUI/MainMenu/MainMenuNewsfeed.c:66-91` |
| Images | `ImageWidget.LoadImageFile(num, name)` loads up to 8 images per widget (`num` 0 to 7) from a data path; `SetImage(num)` selects one. No GIF decoder exists in the engine or the scripts | `1_Core/DayZ/proto/EnWidgets.c:247-262` |
| Video | `VideoWidget extends Widget` (`EnWidgets.c:543`): `Load(name, looping)`, `Play`, `Pause`, `Stop`, `Unload`, `SetLooping`, `IsPlaying`, `SetCallback(VideoCallback.ON_ERROR, ...)`. Vanilla plays `video\DayZ_onboarding_MASTER.mp4` from the main menu and the DLC promo loops a clip | `1_Core/DayZ/proto/EnWidgets.c:543-585`; `5_Mission/DayZ/GUI/NewUI/MainMenu/MainMenuVideo.c:14`; `MainMenuPromo.c:104-133`; `VideoPlayer.c:62-64` |
| Client language | Two reads. `Widget.TranslateString(stringId)` (`EnWidgets.c:112`) resolves a `#key` through every loaded `stringtable.csv` in the client's display language, so a key whose translation in each column is that column's code returns the code. `DayZGame.GetCurrentDisplayLanguageIdx()` (`DayZGame.c:3958`) returns the index of the language option; `ListOptionsAccess.GetItemText` (`gameplay.c:1437`) names it. The vanilla table has 14 columns: `original`, `english`, `czech`, `german`, `russian`, `polish`, `hungarian`, `italian`, `spanish`, `french`, `chinese`, `japanese`, `portuguese`, `chinesesimp` | `1_Core/DayZ/proto/EnWidgets.c:112`; `3_Game/DayZ/DayZGame.c:3958-3970`; `3_Game/DayZ/gameplay.c:1431-1437`; `languagecore/stringtable.csv:1` |
| Hotkey | `kInsert` is declared at `bin/constants.xml:586` and has no binding in `bin/preset_X1MouseKey.xml` (0 matches on October 25, 2026). `kPause` (`constants.xml:577`), `kPageUp`, `kPageDown`, and `kBackslash` are also unbound. `kF12` is unbound in game but is Steam's screenshot key, so it is excluded. `kHome` is Central Scrutinizer's (shared rules 5) | `bin/constants.xml:577,586`; `bin/preset_X1MouseKey.xml` |
| Key binding file | Ears' `Data/Inputs.xml` is the fleet precedent: `<sorting>` inside `<inputs>`, group `nimby`, default in `<preset>` | `mods/own/nimby-pack/NimbyEarplugs/Data/Inputs.xml` |

## Decisions

| # | Decision | Choice | Status |
| --- | --- | --- | --- |
| G1 | Display name | Greet. Code prefix `NimbyGreet`, log prefix `[NIMBY][Greet]`, RPC block `65511xx` (`6551101` request, client to server; `6551102` chunk and `6551103` done, server to the owning client; G5) | Name decided by Jeff, October 16, 2026; prefixes and block are Claude defaults pending the shared rules 4.1 row |
| G2 | Release | With the first Workshop release (Legs, Ears, Scrawl), not after Central Scrutinizer | Decided by Jeff, October 21, 2026 |
| G3 | Text source | `$profile:NimbyGreet\greet.json` on the server, read at mission start and re-read on every client request (G5) so an edit takes effect for the next joiner without a restart. A missing file writes defaults; a malformed file logs one `[NIMBY][Greet] greet.json invalid: <error>` line and serves the last good copy loaded this boot, or nothing on first boot. The server also keeps a `.lkg` copy of the last valid file (shared rules 6) | Claude default; v0.3 moves the read to the request |
| G4 | Versioning and dismissal | `version` is an integer in `greet.json` that Jeff raises when the text changes. The window has one checkbox, "Don't show this again", on by default. Closing with it on writes `$profile:NimbyGreet\seen.json` with `dismissedVersion` and `neverShow` true. From then on the window never opens by itself, whatever the version, unless the received `greet.json` has `forceShow` true and a `version` above `dismissedVersion`; then it opens once and the client records the new version. Closing with the checkbox off writes nothing, so the window shows again next spawn. A missing or malformed `seen.json` means never dismissed. The hotkey and the pause-menu button always open it and never write `seen.json` | Claude default, revised October 25, 2026 for R9. Open question Q1: whether `forceShow` exists at all, or "never" means never |
| G5 | Fetch and open | The client drives. After `OnPlayerLoaded` on the controlled player, delayed by `openDelaySeconds` (default 2), the client sends `6551101` (request: its language code, its `dismissedVersion`, its `neverShow`). The server answers with `6551102` chunks of the requested language (or the default language when the requested one is absent), each chunk at most 1,500 characters of the serialized payload, then `6551103` (done: chunk count, version, `forceShow`). If `neverShow` is set and `forceShow` is false, the server answers `6551103` alone with the version and no chunks, so a dismissed client costs one RPC. The client opens the window only once the payload is complete, no scripted menu is open (`GetMenu()` is null), and the player is alive and conscious; otherwise it retries every second for up to `openRetrySeconds` (default 30), then gives up for this spawn and logs once. The hotkey path (G7) sends the same request with a `manual` flag; the server always sends the text for a manual request | Claude default, revised October 25, 2026: the server does not know the client's language, and one language per client keeps the payload small |
| G6 | What it shows | One window: title and tagline at the top, a section list down the left (the tabs, R3), the selected section's rendered body on the right in a scrolling pane, the section's link buttons under the body (G10), a footer with the "Don't show this again" checkbox, the text-size and motion controls (G16), a language selector when more than one language is served (G14), and one Close button. A section's body is a vertical stack of blocks: text blocks (`RichTextWidget`), media blocks (`ImageWidget` or `VideoWidget`, G15), and rules. Sections, headings, bodies, links, and media ids come from `greet.json`; nothing is hardcoded but the layout and the chrome strings, which are in `stringtable.csv` | Claude default, revised October 25, 2026 |
| G7 | Reopen | Two ways, both always available. A key binding `UANimbyGreetOpen`, default Insert (`kInsert`, `constants.xml:586`, 0 uses in `preset_X1MouseKey.xml`), declared in `Data/Inputs.xml` in the `nimby` group the way Ears does, so the player rebinds it in Options > Controls; pressing it with the window open closes the window. And a "Server info" button in the vanilla pause menu (`modded class InGameMenu`). Both send a manual request (G5) so the player sees the current text, and neither writes `seen.json`. This supersedes v0.2's "no key binding" | Jeff's requirement R6, October 25, 2026; the default key is a Claude default, open question Q2 |
| G8 | Join lines | At Greet's release the per-mod on-connect lines leave `db/messages.xml` and become Greet sections; the hourly lines stay. The release gate's step 5 is amended to remove the on-connect blocks instead of adding them | Claude default following Jeff's October 19 rule that join lines ship with each mod |
| G9 | Limits | Per language: 16 sections, each body 4,000 characters of markdown source, 8 links and 4 media blocks per section, 40,000 characters total. Chunked delivery (G5) lifts the single-RPC ceiling that set v0.2's caps. Longer input is truncated with a `greet.json truncated` log line, never rejected, so a typo in the file cannot leave players with no window. The chunk size (1,500) and the single-RPC ceiling are measured in Phase 0 item 8 | Claude default, raised October 25, 2026 |
| G10 | Links | `RichTextWidget` has no clickable text (current state, "Clickable text"), so every markdown link `[label](https://...)` does two things: in the text it renders as the label in the link color, underlined if `<u>` proves out in Phase 0 item 7, followed by a numbered marker `[1]`; and under the body it becomes a row with an **Open** button (`[1] label`, calls `GetGame().OpenURL(url)`, `Game.c:1401`, which opens the Steam overlay browser) and a **Copy** button (`CopyToClipboard`, `Game.c:663`). The URL itself is printed in small text in the row for a client with the overlay disabled. Only `https://` links are accepted; anything else renders as plain text and logs once. Phase 0 item 5 confirms `OpenURL` works on a dedicated-server client. The v0.2 `url` field on a section is retired: links live in the body | Claude default, revised October 25, 2026 for R8 |
| G11 | Content | Sections, in order: Welcome (name, tagline "Where all hope dies and the Badlands begin", fresh world date); The one rule ("Don't be a dick"; glitch bases are removed); Discord (invite, with the buttons); How to join; Restarts; Difficulty (including the "3rd & 1st Person" browser note); Enhanced Vanilla changes; Mods (one entry per installed NIMBY mod: what it does and its default key, written by hand at each release alongside the join lines in G8); Questions. Text supplied by Jeff on October 21, 2026; the draft is in the repository (G12) and is reformatted into the G13 subset (headings, bullets, the Discord invite as a link) in Phase 1 | Jeff supplied the text; structure is Claude's |
| G12 | Draft location | `server/profile-overrides/NimbyGreet/greet.json` in the repository, the profile-folder twin of `server/mission-overrides`; copied to `$profile:NimbyGreet\greet.json` on the host at release and on every text change, with `version` raised each time. The repository copy is the record; the host copy is what players see. A `scripts/Test-GreetText.ps1` validator (Greet-owned, not orchestrator-owned) parses the repository copy with the same rules as the client and prints every truncation, bad link, unknown media id, and missing language before the file goes to the host | Claude default; the validator added October 25, 2026 |
| G13 | Formatting | The body is a markdown subset, converted on the client to `RichTextWidget` markup by an engine-free converter in `3_Game`. Supported: `#` and `##` headings (rendered bold, `##` smaller), paragraphs separated by a blank line, `**bold**`, `*italic*`, `- ` bullet lists, `1. ` numbered lists, `[label](https://url)` links (G10), `![alt](media-id)` media blocks on their own line (G15), `---` as a horizontal rule, and `\` before a markdown character to print it. Not supported, and printed as written: tables, nested lists, code blocks, raw HTML, images by URL. Any `<` or `>` in the source is escaped before conversion so a player-visible text can never inject markup. The converter is self-tested (shared rules 8) with one case per construct plus escaping, nesting, and an unterminated emphasis | Claude default for R2; the subset is open question Q3 |
| G14 | Languages | `greet.json` has `defaultLanguage` and a `languages` object keyed by the vanilla column names in `languagecore/stringtable.csv:1` (`english`, `german`, `russian`, and so on); each value holds `title`, `tagline`, and `sections`. The client reads its own language by translating `#STR_NimbyGreet_Language`, a key in Greet's `stringtable.csv` whose text in each column is that column's name; `GetCurrentDisplayLanguageIdx` (`DayZGame.c:3958`) is the Phase 0 cross-check. It requests that language; the server serves it, or `defaultLanguage` when it is absent, and tells the client which. A language selector in the footer lists the languages the server has and overrides the detected one; the choice is saved in `prefs.json`. The window chrome (Close, the checkbox, Open, Copy, the size and motion labels) is in Greet's `stringtable.csv` in all 14 columns, machine-translated and marked as such in `NOTICE` until a speaker reviews them. Jeff supplies only `english` at release; the structure is ready for the rest | Claude default for R5 |
| G15 | Media | GIFs are not an engine format. An animated image is converted offline to a looping, silent `.mp4` (`ffmpeg -i in.gif -an -movflags faststart -pix_fmt yuv420p -vf "scale=trunc(iw/2)*2:trunc(ih/2)*2" out.mp4`) and shipped at `NimbyGreet/Media/<id>.mp4`; a still image is a `.edds` (or `.paa`) at `NimbyGreet/Media/<id>.edds`. The text places one with `![alt](id)`; the client renders a `VideoWidget` (`Load(path, true)`, `Play`) or an `ImageWidget`, sized to the pane width with the aspect ratio kept, and prints `alt` under it. An unknown id renders as `[alt]` in text and logs once. A clip is stopped when its section is hidden and unloaded when the window closes. A media add is a Workshop release, so each release's media list is recorded in `NOTICE` with its source and license. Media caps: 10 seconds, 640 pixels wide, 2 MB per clip, 8 clips per release, so the Workshop download stays small (shared rules 9.3). Fallback if a PBO-bundled `.mp4` will not load in Phase 0 item 9: frame flipping on an `ImageWidget` (up to 8 frames per widget, `LoadImageFile` 0 to 7, timer-driven), with the plan revised | Claude default for R7 |
| G16 | Accessibility | Keyboard and controller: Tab or the shoulder buttons move between the section list, the body, the link rows, and the footer; Up and Down or the stick move within a group; Page Up and Page Down and the mouse wheel scroll the body; Enter or A activates; Escape or B closes. The section list and every button show a visible focus state, not just a hover state. Text size: a three-step control (normal, large, larger; 1.0, 1.25, 1.5) that rescales the body and the list, saved in `prefs.json`. Contrast: body text on the pane background at 7:1 or better; the link color at 4.5:1 and never the only cue (underline or marker, G10). Motion: a "Play animations" switch, on by default, saved in `prefs.json`; off shows the clip's first frame, paused. No timed auto-close, no sound. Controller support is verified on the Steam Input preset in Phase 1 | Claude default for R4 |
| G17 | Preferences file | `$profile:NimbyGreet\prefs.json` holds `textScale`, `playAnimations`, and `language` (empty for detected), written atomically with a temp file and rename (shared rules 6); `seen.json` holds only the dismissal (G4). Both are client-local and never leave the client | Claude default |

## Data format

`greet.json` (server, `$profile:NimbyGreet\greet.json`; repository draft at `server/profile-overrides/NimbyGreet/greet.json`):

| Field | Type | Required | Description |
| --- | --- | --- | --- |
| `version` | `integer` | Yes | Raised by Jeff on every text change (G4) |
| `forceShow` | `boolean` | No | Default `false`. When `true`, a client that chose "Don't show this again" sees this version once (G4) |
| `defaultLanguage` | `string` | Yes | A key of `languages`; served when the client's language is absent (G14) |
| `languages` | object of `Language` keyed by vanilla column name | Yes | At least `defaultLanguage` (G14) |
| `openDelaySeconds` | `number` | No | Default 2 (G5) |
| `openRetrySeconds` | `number` | No | Default 30 (G5) |

`Language`:

| Field | Type | Required | Description |
| --- | --- | --- | --- |
| `title` | `string` | Yes | Window title, 60 characters maximum |
| `tagline` | `string` | No | One line under the title, 80 characters maximum |
| `sections` | array of `Section` | Yes | 1 to 16 entries (G9); order is display order |

`Section`:

| Field | Type | Required | Description |
| --- | --- | --- | --- |
| `id` | `string` | Yes | Stable key, `[a-z0-9-]`, 32 characters maximum; logged with every open so a test record can name a tab without quoting its heading |
| `heading` | `string` | Yes | The tab label, 40 characters maximum |
| `body` | `string` | Yes | Markdown subset (G13), 4,000 characters maximum; `\n` for line breaks |

Media (client PBO, `NimbyGreet/Media/`): `<id>.mp4` or `<id>.edds`, `id` in `[a-z0-9-]`, listed in `Media/manifest.json` (`id`, `kind`, `width`, `height`, `durationMs`, `source`, `license`) so the client sizes the block before the file loads and `NOTICE` has one place to read.

`seen.json` (client, `$profile:NimbyGreet\seen.json`):

| Field | Type | Required | Description |
| --- | --- | --- | --- |
| `dismissedVersion` | `integer` | Yes | Version shown when the player last closed with the checkbox on |
| `neverShow` | `boolean` | Yes | `true` once the checkbox has been used; cleared only by deleting the file |

`prefs.json` (client, `$profile:NimbyGreet\prefs.json`): `textScale` (`1.0`, `1.25`, or `1.5`), `playAnimations` (`boolean`), `language` (`string`, empty for detected) (G17).

RPC payloads (`65511xx`):

| Id | Direction | Fields |
| --- | --- | --- |
| `6551101` request | client to server | `language` (string), `dismissedVersion` (int), `neverShow` (bool), `manual` (bool) |
| `6551102` chunk | server to the owning client | `index` (int), `text` (string, at most 1,500 characters of the serialized `Language` JSON) |
| `6551103` done | server to the owning client | `version` (int), `forceShow` (bool), `chunkCount` (int, 0 when nothing was sent), `servedLanguage` (string), `languages` (string, comma-separated list for the selector) |

Default `greet.json` written on first boot carries `english` only with placeholder sections (`welcome`, `rules`, `discord`, `restarts`) and the restart schedule from `docs/operations/restart-schedule.md`; the real text is the repository draft (G12), which Jeff reviews and copies to the host at release.

## Phase 0 - prove the window survives spawn

1. Record builds and reference hashes in `docs/baselines/`; add the `NimbyGreet` row and a stub `config.cpp` to `pack.json` on `main` first (the Ears E6 precedent) so the probe builds.
2. Probe: a server `modded class MissionServer` that sends a fixed string on `ClientReadyEventTypeID`; a client `modded class PlayerBase` that receives it in `OnRPC`, stores it, and opens a `UIScriptedMenu` with one text widget and one Close button from `OnPlayerLoaded` after a 2 s delay. Log every step with the `[NIMBY][Greet]` prefix.
3. On the replica (`-ServerPath D:\DayZServer -InstanceId 998`): fresh spawn, relog onto a live character, respawn after death. Record whether the window appears each time, whether `CloseAll()` ever removes it, and what happens when the connecting dialogue (`MENU_CONNECTION_DIALOGUE`) is up at the moment Greet tries to open (force it with a throttled connection or by spamming input during load).
4. Confirm the window takes input focus and releases it on Close, that the inventory and pause menu open normally afterward, and that nothing is logged as `SCRIPT (E)`.
5. Press a probe **Join Discord** button that calls `GetGame().OpenURL("https://discord.gg/kH3eJ5mdEv")` and a **Copy invite** button that calls `CopyToClipboard`; record whether the Steam overlay opens on the invite and whether the clipboard holds the URL afterward (G10).
6. Confirm `JsonFileLoader.LoadFile` reports a malformed `greet.json` with a readable error and that the server keeps running.
7. Markup sheet: one `RichTextWidget` showing every tag the converter may emit (`<b>`, `<i>`, `<u>`, `<color hex=...>`, `<font size=...>`, `<p>`, `<br/>`, `<image set=... name=.../>`), with a screenshot. Each tag is recorded as honored, ignored, or printed literally; the G13 converter emits only honored tags. Also record whether `GetContentHeight` and `SetContentOffset` scroll a body longer than the pane.
8. RPC ceiling: send strings of 1,000, 1,500, 2,000, 4,000, and 8,000 characters through `RPCSingleParam` and record the largest that arrives intact; the G5 chunk size is set below it. Send a 16-section, 40,000-character language in chunks and record the time from request to `done` on the replica.
9. Media: bundle a 3-second looping silent `.mp4` (converted from a GIF with the G15 command) and a `.edds` in the probe PBO; load each from its `NimbyGreet\Media\` prefix path in a `VideoWidget` and an `ImageWidget` inside the window. Record whether the clip plays and loops, whether `VideoCallback.ON_ERROR` fires, and whether a second clip in a hidden section costs anything. This item decides between `VideoWidget` and the G15 fallback.
10. Language: translate `#STR_NimbyGreet_Language` with the client set to English and then to German, and compare with `GetCurrentDisplayLanguageIdx`; record both values.
11. Hotkey: bind `UANimbyGreetOpen` to Insert through `Data/Inputs.xml`, confirm it appears in Options > Controls under NIMBY and opens the window in play; rebind it and confirm the new key works (the Ears EP-005 pattern).

Exit: the window shows on all three spawn paths, never fights the connecting dialogue, closes cleanly, the markup sheet shows at least `<b>` and `<color>` honored, a chunked 40,000-character language arrives intact, the language key reads correctly in two languages, and the hotkey opens the window. Stop conditions: if a scripted menu cannot be held open after `OnPlayerLoaded` on 1.30, Greet falls back to a HUD text panel (no focus, auto-hides after a timeout) and this plan is revised; if a PBO-bundled `.mp4` will not load, G15 switches to the frame-flip fallback; if the overlay browser does not open from `OpenURL` on a dedicated-server client, G10 keeps the Copy button and prints the URL, and R8 is reported to Jeff as partly met.

## Phase 1 - the window

- `config.cpp` (`CfgPatches` requiring `DZ_Data`; `3_Game`, `4_World`, `5_Mission` modules), `Data/Inputs.xml` (G7), `stringtable.csv` (G14 chrome strings and the language key), `Media/` with `manifest.json` (G15).
- `Scripts/3_Game/nimbygreet_rules.c` (engine-free: caps per G9, version and dismissal logic per G4, language pick per G14, link validation per G10), `nimbygreet_markdown.c` (engine-free: the G13 converter, producing a block list of text markup, link rows, media ids, and rules), `nimbygreet_chunks.c` (engine-free: payload split and reassembly per G5), `nimbygreet_selftest.c` (one case per construct, cap, chunk boundary, and dismissal state; prints `cases=N` per shared rules 8).
- `Scripts/4_World/nimbygreet_playerbase.c` (RPC send and receive, `OnPlayerLoaded` hook, open scheduling per G5, the hotkey poll).
- `Scripts/5_Mission/nimbygreet_missionserver.c` (file load with `.lkg`, request handling, chunked send, per-player log lines), `nimbygreet_menu.c` and `GUI/layouts/nimbygreet_window.layout` (vanilla widgets only; one layout, blocks created from `nimbygreet_block_text.layout`, `nimbygreet_block_media.layout`, and `nimbygreet_block_link.layout`), `nimbygreet_ingamemenu.c` (G7 button), `nimbygreet_missionbase.c` (self-test hook).
- `scripts/Test-GreetText.ps1` (G12 validator) and its Pester file in `scripts/tests/`.
- `pack.json` changes the orchestrator makes on `main` (shared rules 3): the `NimbyGreet` row with `"logPrefix": "Greet"`, and `*.mp4`, `*.edds`, `*.paa` added to `includePatterns` so media is inside the completeness check.
- Client `seen.json` written only when the checkbox is on at Close; `prefs.json` written when a footer control changes (G17).
- Logging per shared rules 4.3: production lines for load (version, languages, byte count), request and serve (player name, `GetId()`, Steam64 per 4.2, language asked and served, version, chunk count, `manual`), open (`version`, `source=spawn|hotkey|menu`, `section=<id>`), dismiss (`neverShow`), link open (URL), media error; debug flag `$profile:NimbyGreet\debug` adds the retry loop, each chunk, the converter's block list, and the detected language.

## Phase 2 - release

- Jeff reviews the real `greet.json` (the G11 content in the G13 subset, `english` only) and sets `version` to 1, `forceShow` false. `scripts/Test-GreetText.ps1` passes on it.
- The join lines for Legs, Ears, and Scrawl move from the `messages.nimby-mods.patch.xml` on-connect block into Greet sections (G8); the release gate step 5 text is amended.
- Gate per shared rules 9.2 with the first Workshop release; the Greet column on the checklist lists G-0 to G-7 below.

## Tests

| Id | Scenario | Required result | Evidence |
| --- | --- | --- | --- |
| G-0 | Self-test | All cases pass | `[NIMBY][Greet] selftest complete; cases=N failures=0` |
| G-1 | Fresh spawn, no `seen.json` | Window opens after the delay with the server text in the client's language | `[NIMBY][Greet] open version=1 source=spawn` on the client; screenshot |
| G-2 | Close with the checkbox on, relog | No window; one `done` RPC with `chunkCount=0` | `seen.json` holds `dismissedVersion=1`, `neverShow=true`; serve line shows `chunks=0` |
| G-3 | Jeff raises `version` to 2 with `forceShow` true, player relogs | Window opens once; next relog, nothing | `open version=2`; then `chunks=0` |
| G-3b | Jeff raises `version` to 3 with `forceShow` false, dismissed player relogs | No window | `chunks=0`; no open line |
| G-4 | Close with the checkbox off, relog | Window opens again | No `seen.json` write; `open version=3` |
| G-5 | Die and respawn | Window behavior per `seen.json`, never twice in one spawn | One open line at most per spawn |
| G-6 | Pause menu "Server info" | Window opens with current text; `seen.json` unchanged | `open source=menu` |
| G-6b | Hotkey (Insert) in play, then again with the window open | Opens, then closes; `seen.json` unchanged; inventory and pause menu open normally afterward | `open source=hotkey`, `close`; screenshot |
| G-6c | Rebind the hotkey in Options > Controls | New key opens the window; Insert does nothing | Screenshot of the Controls page; open line |
| G-7 | Malformed `greet.json` on the server | `.lkg` text served or nothing on first boot; server keeps running | `greet.json invalid:` line, 0 `SCRIPT (E)` |
| G-8 | Connecting dialogue up at open time | Greet waits, then opens or gives up at `openRetrySeconds` | `open deferred` lines, then `open` or `gave up` |
| G-9 | Oversized text (17 sections, one body of 6,000 characters) | Truncated to the caps, window still shows | `greet.json truncated` line; `Test-GreetText.ps1` reports the same cuts |
| G-10 | Boot without Greet on storage that ran with it | Clean | 0 "Scripted variables corrupted" (shared rules 9.2) |
| G-11 | Open button on the Discord link | Steam overlay opens on the invite page | Screenshot; `[NIMBY][Greet] open url` line |
| G-12 | Copy button on the Discord link | Clipboard holds the URL | Paste into chat shows `https://discord.gg/kH3eJ5mdEv` |
| G-13 | Sixteen sections, one body at 4,000 characters | Every tab listed and readable; the long one scrolls with the wheel and Page Down | Screenshot |
| G-14 | Markdown sheet section (every G13 construct, plus `<b>` typed in the source) | Headings, bold, italic, both list kinds, rule, link marker, escaped character render as specified; the typed `<b>` prints literally | Screenshot against the Phase 0 item 7 record |
| G-15 | Client language German, `greet.json` has `english` and `german` | German text; selector shows both; switching to English re-renders and persists across relog | `serve language=german`; `prefs.json` holds `language=english` after the switch |
| G-15b | Client language Czech, `greet.json` has `english` only | English text; selector hidden or showing one entry | `serve asked=czech served=english` |
| G-16 | Media section with one clip and one still | Clip plays and loops, still shows, `alt` under each; the clip stops when the tab changes and unloads on close | Screenshot; no `media error` line; 0 `SCRIPT (E)` |
| G-16b | Unknown media id | `[alt]` in text, one log line, window still shows | `media unknown id=` line |
| G-17 | Keyboard only (mouse unplugged or ignored) | Every tab, link row, footer control, and Close reachable and operable; focus visible at each step | Video or screenshot sequence |
| G-17b | Controller (Steam Input, Xbox layout) | Same as G-17 with the stick and face buttons | Video or screenshot sequence |
| G-18 | Text size larger (1.5) | Body and tabs rescale; nothing clips; persists across relog | Screenshot; `prefs.json` holds `textScale=1.5` |
| G-19 | Play animations off | Clip shows its first frame and does not play; persists | Screenshot; `prefs.json` holds `playAnimations=false` |
| G-20 | Non-`https://` link in the body | Renders as plain text, no button, one log line | `link rejected` line |
| G-21 | Chunk loss (debug flag `dropchunk` in the dev harness) | Client never opens a partial window; gives up at the retry limit with one log line | `gave up chunks=` line; no open line |

## Open questions for Jeff

| # | Question | Claude's default in this draft |
| --- | --- | --- |
| Q1 | Dismissal: does "never show again" mean never, full stop, or never until Jeff flags a version with `forceShow`? | `forceShow` exists and defaults to false (G4). Without it a rules change can never reach a player who dismissed the first version |
| Q2 | Default hotkey: Insert, Pause, or another verified-free key? | Insert (G7). Free in `preset_X1MouseKey.xml`, reachable, and not a Steam or Discord default |
| Q3 | Markdown subset: is the G13 list enough, or is there a construct the content needs (a table for the restart schedule, say)? | The G13 list. A table of two columns can be a bullet list; a real table needs a grid of widgets and is a later release |
| Q4 | Release scope: all of v0.3 in release 1, or the core (window, tabs, links, hotkey, dismissal) in release 1 and markdown, languages, and media as Greet releases 2 and 3? | All in release 1, with the split as the fallback if Phase 1 runs past five days |
| Q5 | Shared rules amendment (v1.4): the Greet row in 2.1 (`feat/greet`, worktree `greet`), 4.1 (`NimbyGreet`, `[NIMBY][Greet]`, `65511xx` with the three ids), and 5 (Insert). Only the orchestrator edits that file, and `pack.json` needs the row and the three `includePatterns` | Jeff makes both edits on `main`; this session prepares the exact text |

## Risks

| Risk | Mitigation |
| --- | --- |
| `CloseAll()` in `OnPlayerLoaded` or a later vanilla hook closes the window | Open after `OnPlayerLoaded` with a delay (G5); Phase 0 item 3 records every closer found |
| The connecting dialogue and Greet fight for focus on a slow connection | Greet yields and retries (G5); G-8 proves it |
| A text change is forgotten and `version` is not raised | Returning players see nothing new. Mitigation is a line in the ops doc: raise `version` with every edit; the server logs the version it loaded at every boot |
| Players dismiss the window without reading it | Accepted. Greet is information, not enforcement; the rules also live in Discord |
| Release 1 slips while Greet is built | Accepted by Jeff on October 21, 2026 when he folded Greet into the release. v0.3 widens Phase 1: Phase 0 is a one-day box; Phase 1 is four to five days of build plus the test block. If that is too long for release 1, the markdown converter, languages, and media can each ship as a later release of the same addon; the window, tabs, links, hotkey, and dismissal are the release-1 core (open question Q4) |
| `RichTextWidget` honors fewer tags than the converter wants | Phase 0 item 7 is the tag sheet; the converter emits only honored tags, and an unsupported construct degrades to plain text, never to a visible tag |
| The RPC ceiling is lower than 1,500 characters | Phase 0 item 8 measures it; the chunk size is a constant set from the measurement |
| A PBO-bundled `.mp4` does not load, or `VideoWidget` costs frame time | Phase 0 item 9; the G15 frame-flip fallback; the clip caps keep any cost small, and animations can be switched off by the player (G16) |
| Media grows the Workshop download | G15 caps (2 MB per clip, 8 clips) and the release-time media list in `NOTICE` |
| Machine translation of the chrome strings is wrong | Marked as unreviewed in `NOTICE`; English is the fallback; a wrong Close label is cosmetic |
| A player dismisses forever, then the rules change | `forceShow` on the version that matters (G4); the pause-menu button and hotkey are always there |
