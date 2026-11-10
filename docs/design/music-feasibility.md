# Music feasibility

Draft v0.7, November 9, 2026 (v0.7: rulings M-12 to M-14 from the 22:28 Central decide round. v0.6: self-review corrections, listed under Corrections. v0.5: multi-track tapes added as M-10 and track display as M-11. v0.3: Jeff's rulings of November 9, 21:47 to 22:00 Central, recorded under Decisions; where an earlier section recommends something else, Decisions wins. v0.2: the Zens Music and ZenModCore sources were read; the own-build verdict is now evidenced, the boombox and walkman models have a source, and the offset unit is corrected). Owner: Jeff Fabish. Status: feasibility complete, ordered by Jeff on November 9, 2026; ruled the same day (section Decisions). Phase 0 is approved to start. Working name Music, proposed code prefix `NimbyMusic`, log prefix `[NIMBY][Music]`, RPC block not assigned (see Risks, R8). Every `file:line` is relative to `vendor/dayz-1.30-scripts/`: `scripts/...` for Enforce script, `DZ/...` for the unRaP'd configs. Model facts were read from the DayZ Tools extract at `D:\DayZProjects\DZ`. `ZenMusic:` and `ZenModCore:` paths are relative to those GitHub repositories at the commits listed under Sources.

Target: NIMBY | HARDCORE | BADLANDS | 1pp, HostHavoc, DayZ 1.30 / Nasdara.

## Verdict

| Question | Verdict | Why |
| --- | --- | --- |
| Tape + player + 9V battery plays music that nearby players hear | Feasible, low risk | Vanilla already plays a looping music sound set from a world object (`ChristmasTree.c:5,20`), and 1.30 has the wave calls a late joiner needs to start mid-track (`Sound.c:183,198`) |
| Model purchase needed to start | No | Vanilla 1.30 ships a cassette and a radio-cassette recorder (Unitra Wilga) as `scope = 2` items with no loot rows on live. Phase 0 and Phase 1 run on them at no cost |
| Admins add tracks | Feasible at build time only | One `.ogg` plus one manifest row per tape; a generator writes the config. Every track ships inside the client pack, so a new tape is a pack release, never an in-game action |
| Survivors record their own audio in game | Not feasible | Script can switch microphone capture on and read a loudness number. It cannot read samples, write audio, or play a sound that is not declared in a packed config |
| Survivors make their own tapes | Feasible in two other forms | Mixtapes dubbed in game from tapes that exist (data only), and submitted recordings that ship in the next pack release (how DayZ Underground did it) |
| Own build or Zens Music repack | Own build, no Community Framework, using Zen's sync method with credit | A repack needs Community Framework: Zens Music requires ZenModCore, and ZenModCore requires `JM_CF_Scripts` |

Recommendation: proceed to Phase 0 on vanilla models with one tape. Buy nothing until Phase 0 exits, the same rule Fang follows.

## What this is not

- Not voice recording. No design in this study captures a microphone in game.
- Not streaming. The server never sends audio. It sends which tape is playing and when it started; each client plays its own packed copy.
- Not a radio station system. No frequencies, no map-wide broadcast, no static boomboxes at fixed places. Zens Music and Psyern's Radio Show do those; Music does not.
- Not runtime content. A track that is not in the pack a client downloaded cannot be heard by that client.
- Not legal cover for commercial music. Ruling M-7 allows it and M-6 isolates it in a second Workshop item; the takedown risk stays (see Music rights).

## Current state (vanilla 1.30)

| Item | State | Source |
| --- | --- | --- |
| Cassette item | `Cassette : Inventory_Base`, `scope = 2`, 2x1, 80 g, model `\dz\gear\tools\cassette.p3d`, no behavior. Description string: "Old classic cassette used for recording of music or voice." | `DZ/gear/tools/config.cpp:1200-1211`, `languagecore/stringtable.csv:5356-5357` |
| Cassette model | Binarized ODOL, one texture set (`cassette_co.paa`), and a `zbytek` named selection. A config can retexture it per tape through `hiddenSelections[]`, the same way Zen retextures the cleaning kit into a tape case. Which faces `zbytek` covers was not checked; Phase 0 looks | `D:\DayZProjects\DZ\gear\tools\cassette.p3d` (strings read, November 9, 2026) |
| Radio-cassette recorder | `Radio : Inventory_Base`, `scope = 2`, 2x3, 1,700 g, model `\dz\gear\radio\unitra_wilga.p3d`, `attachments[] = {"BatteryD"}`, `EnergyManager` with `energyUsagePerSecond = 0.02`, `attachmentAction = 1`, `autoSwitchOff = 1`. Black and white textures exist (`unitra_black_co.paa`, `unitra_white_co.paa`) | `DZ/gear/radio/config.cpp:256-281` |
| Radio script | `Radio extends ItemRadio`, `simulation = "ItemRadio"`, a tunable receiver with turn on, turn off, and tune actions. It has no tape handling | `scripts/4_World/DayZ/Entities/ItemBase/Radio.c:1-52` |
| Loot | Neither `Radio` nor `Cassette` has a row in the live `types.xml` (0 matches). `Battery9V` has nominal 66 | `server/live/db/types.xml:3392` |
| 9V battery | `Battery9V`, 1x1, quantity 50, `inventorySlot[] = {"BatteryD"}`, own `EnergyManager` | `DZ/gear/consumables/config.cpp:566-588` |
| Music from an object | `ChristmasTree` plays `ChristmasMusic_SoundSet` with `PlaySoundSetLoop` on the client only and destroys the effect in `EEDelete`. The shader is one sample, `volume = 0.55`, `range = 35` | `scripts/4_World/DayZ/Entities/WoodBase/ChristmasTree.c:5,18-21,24-32`, `DZ/sounds/hpp/config.cpp:127070-127078` |
| Sound follows an object | `EffectSound.SetParent(Object, pivot)` | `scripts/3_Game/DayZ/Effects/EffectSound.c:658` |
| Start mid-track | `AbstractWave.SetStartOffset(float)`, `PlayWithOffset(float)`, `Skip(float timeSec)`, `GetCurrPosition()`. The offset is a fraction of the track, not seconds: `GetCurrPosition` is documented as a percentage of total length, and Zens Music passes elapsed seconds divided by song length | `scripts/3_Game/DayZ/Sound.c:175-198`, `ZenMusic:ZenMusicBase/scripts/4_World/ZenMusicBase/Entities/ZenMusic/Devices/Zen_MusicDeviceBase.c:339-344` |
| Track length | `AbstractWave.GetLength()` is marked "Blocking! Waits for header to load". Durations belong in config, not in a runtime call | `scripts/3_Game/DayZ/Sound.c:184-185` |
| Sound source | `SoundParams(string name)` and `SoundParams.Load(string name)` take a `CfgSoundSets` class name. No constructor takes a file path or a buffer | `scripts/3_Game/DayZ/Sound.c:137-143` |
| Microphone | `EnableMicCapture(bool)` ("Audio heard does NOT automatically get transmitted over network") and `IsMicCapturing()`; the only caller is the Options sound page | `scripts/3_Game/DayZ/Global/Game.c:1076-1085`, `scripts/5_Mission/DayZ/GUI/NewUI/Options/OptionsMenuSounds.c:112-129,231,241` |
| Voice level | `IsPlayerSpeaking()` returns one float (amplitude) | `scripts/3_Game/DayZ/dayzplayer.c:1355`, `scripts/4_World/DayZ/Entities/DayZPlayerImplement.c:2622` |
| File and HTTP | `OpenFile`, `FPrint`, `ReadFile`, `CopyFile` write under `$profile:` and `$saves:`; `RestContext.FILE` downloads to a file. Nothing plays a file from those places | `scripts/1_Core/DayZ/proto/EnSystem.c:382-531`, `scripts/3_Game/DayZ/Http/RestApi.c:113` |

## How it works

Music is a content addon under shared rules 6.1. It defines its own classes and changes no vanilla class.

| Piece | Design |
| --- | --- |
| Player item | `NimbyMusic_Deck : Inventory_Base` on the vanilla Wilga model path, own `EnergyManager`, attachments `BatteryD` and a new slot `NimbyTape`. The vanilla `Radio` class is left alone because its `ItemRadio` simulation is a tuner |
| Tape item | `NimbyMusic_Tape_Base : Inventory_Base`, `inventorySlot[] = {"NimbyTape"}`. One child class per track, generated |
| Actions | Insert and eject come from the attachment system. Play and Stop are two actions on the deck, enabled when a tape and a charged battery are attached |
| State | Two net-synced variables on the deck: elapsed seconds (-1 when stopped, advanced by a server timer) and volume. This is the method Zens Music ships (`Zen_MusicDeviceBase.c:49-50,226-227`). Nothing extra is persisted; attachments and battery charge persist the vanilla way. A deck is stopped after a restart |
| Playback | Client only. When the synced state says playing, the client creates an `EffectSound` from the tape's sound set, parents it to the deck's hierarchy root (the deck on the ground, or the player or vehicle carrying it), and calls `SetStartOffset(elapsed / length)`. A player who walks into range or logs in mid-song hears the same point. `EffectSound` keeps its wave in a protected member, so one accessor on a `modded class EffectSound` is needed, as Zen does (`ZenMusic:ZenMusicBase/scripts/3_Game/ZenMusicBase/Effects/EffectSound.c:1-13`); it adds no saved data |
| End of track | The server holds each track's length from config and switches the deck off when it elapses |
| Power | `EnergyManager` drain. At the vanilla Wilga rate (0.02 per second against a 50-unit battery) one 9V lasts about 41 minutes of play; the rate is one config number |
| Audibility | Shader `range` and `volume` per device: deck near 25 m, boombox near 60 m. Both are tuning values for Phase 0 |
| Ears | Expected: earplugs muffle music with no extra code, if the effect plays on the channel Ears scales. Not verified; Phase 0 check 6 |

### Adding a tape

1. Convert the track to mono Ogg Vorbis, 44.1 kHz. Mono is required for a positional sound.
2. Drop the file into `NimbyMusicTapes/sound/`.
3. Add the track and tape rows to `tapes.json` (title, artist, licence, source link, `copyrighted`; the two-list shape is under Multi-track tapes).
4. Run the build. A generator step reads the manifest, measures each file's length, and writes `CfgVehicles`, `CfgSoundShaders`, and `CfgSoundSets` for every row, plus a draft `types.xml` row and the attribution list.
5. Release the pack and apply the loot row at a restart.

The generator belongs in `Build-NimbyPack.ps1`, which is orchestrator-owned; that edit is Jeff's to approve.

### Size

A mono track at Vorbis quality 4 is roughly 0.5 to 0.7 MB per minute, so a 3.5-minute song is about 2 to 2.5 MB and 30 tapes add 60 to 75 MB. The whole pack is 9.8 MB today (`build\@NIMBY-20261108-123656`). Zens Music was 138 MB on a listing dated January 2025 and Psyern's Radio Show is 457 MB, so this is normal for the genre, but shared rules 9.3 says the preview image is the only real download-size driver, and that stops being true. Audio goes in its own PBO (`NimbyMusicTapes`) so a script fix does not change the audio PBO's hash.

## Models

No purchase is needed through Phase 1. For the boombox, ruling M-4 sets the order: Claude-made, then Zen's, then bought.

| Item | Phase 0 and 1 | Later |
| --- | --- | --- |
| Deck | Vanilla Unitra Wilga. The model carries the black texture and no retexture selection was found, so the white texture in the game files is not selectable by config. The real Wilga RM-301 is a radio-cassette recorder; whether the in-game model shows a tape door was not checked | Keep |
| Tape | Vanilla cassette, told apart by name and description | Per-tape or per-genre textures on the vanilla model through its `zbytek` selection, if Phase 0 shows the selection covers the label. A Claude-made cassette is needed only if it does not |
| Boombox | Not in Phase 1 | Per ruling M-4: Claude-made in Blender first. Fallback 1 is Zen's `zen_boombox.p3d` (editable MLOD, one texture set, a `zbytek` selection, 4x3, one `BatteryD` and one cassette slot; `ZenMusic:ZenMusicBase/config.cpp:157-186`). Fallback 2 is a bought model shipped as ODOL only, as the DPV is (`NimbyDpv/NOTICE`) |
| Walkman | Not planned (ruling M-3) | Zen's `zen_walkman.p3d` (MLOD, 1x2) exists if that ruling changes |
| Tape case | Not in Phase 1 | Zen's approach costs no model: the vanilla `cleaning_kit_wood.p3d` with its `zbytek` selection retextured (`ZenMusic:ZenMusicBase/config.cpp:68-80`). A Claude-made case is the alternative |

Model origin is the one open point on Zen's models: his wiki asks repackers to credit 3D model creators, and the ZenMusic repository names none for the boombox or walkman. ZenModCore carries a `credit.txt` beside the one third-party model it ships, which suggests the music models are his own, but that is an inference. Ask him before release and record the answer in `NOTICE`.

Zen builds every tape on the vanilla cassette model and sets no `hiddenSelections[]` on it (`ZenMusic:ZenMusicBase/config.cpp:345-349`, `ZenMusicAudio/config.cpp`), so his tapes look identical. The model's `zbytek` selection means ours do not have to.

What Claude can and cannot do: a Blender script produces clean hard-surface meshes, LODs, geometry LODs, and named selections, as the DPV mesh work did. The weak part is texture work with fine printed detail (speaker grilles, dials, brand-style lettering). A boombox viewed in first person is where a bought model would look better.

## Player recordings

In-game microphone recording is not feasible on 1.30. Three things are missing, and each alone ends it:

1. No script call returns microphone samples. `EnableMicCapture` turns capture on for the Options page meter; `IsPlayerSpeaking` returns loudness.
2. No script call builds a sound from data or from a file path. Every playable sound is a `CfgSoundSets` class inside a PBO.
3. The server verifies addon signatures, so a client loads only PBOs signed with an accepted key. A file the game wrote to `$profile:` cannot become a sound even if it were valid audio. This third point is Claude's reasoning from how the pack is signed and checked, not a tested result.

This is a "searched and not found" result against the full 1.30 script extract, not a statement from Bohemia. A future engine update could change it.

What works instead, ranked:

| # | Option | What the player gets | Cost | Verdict |
| --- | --- | --- | --- | --- |
| P1 | Mixtapes | A blank tape, a deck with a tape playing, and a Record action copy that track onto the blank. A mixtape holds an ordered list of track ids and plays them in sequence. A pen writes the label (Scrawl already has pens) | One own class with a short id list persisted under a version guard (rule 6.1 allows it for an own class); a second tape slot or a deck-to-deck rule | Set aside by ruling M-5. Was recommended: period-true, no content pipeline |
| P2 | Submitted tapes | A player sends a recording through Discord; an admin reviews it; it ships as a named tape in the next pack release and is handed to the author in game | No new code beyond Phase 1. Moderation time, a rights statement from the submitter, and one pack release per batch | Set aside by ruling M-5. Was recommended as a process; DayZ Underground took community submissions for its cassette tapes |
| P3 | Live voice | A player speaks or plays audio through VoN; the vanilla megaphone carries it further | None; it exists | Already available. It is live, not recorded |
| P4 | In-game instrument | Notes played on an item are stored as events and replayed from packed samples | A separate feature | Out of scope; recorded as an idea |

## Own build or Zens Music

| | Own build | Zens Music repack (Workshop 3412251200) |
| --- | --- | --- |
| Features | Deck, tapes, battery, synced playback; multi-track tapes, track display, infected attraction; boombox, tape case, and DPV mount in Phase 2 | Cassettes, walkman, boombox, car playback, synced playback, radio and static boomboxes, zombie attraction through the noise system |
| Licence | MIT; anything taken from Zen is marked with its source and MPL 2.0 | The Zen pack wiki states all his mods are MPL 2.0 (modify and repack allowed, source stays open, credit authors). The ZenMusic repository itself carries no licence file or file headers; its readme says "You can repack this mod if you like, and do anything else you want with it" |
| Dependencies | None | `ZenMusicBase` requires `ZenModCore` (`ZenMusic:ZenMusicBase/config.cpp:6-11`). `ZenModCore` requires `JM_CF_Scripts`, which is Community Framework (`ZenModCore:config.cpp:9-14`) |
| Save footprint | Own classes only, per rule 6.1 | Zens Music itself is clean: its only store override is on its own `Zen_Cassette_Base` (`Zen_Cassette_Base.c:101-120`). ZenModCore is not: it writes creation and first-touch timestamps on the vanilla `ItemBase` through `CF_OnStoreSave` (`ZenModCore:scripts/4_World/ZenModCore/Entities/ItemBase/ItemBase.c:167-189`) |
| Vanilla classes modded | `EffectSound` (one accessor) | 11 in Zens Music (`PlayerBase`, `CarScript`, `EffectSound`, `Hologram`, `MissionBase`, `MissionGameplay`, `PluginManager`, `ActionConstructor`, `ModItemRegisterCallbacks`, and two inventory GUI classes) plus at least 28 in ZenModCore |
| Size | Claude's estimate: several hundred lines for Phase 1 with M-9, M-10, and M-11 included; not measured | About 2,900 lines in Zens Music and 7,600 in ZenModCore |
| House rules | Self-test, log prefix, RPC block, no CF | Would need each added |
| 1.30 status | Built against 1.30 | The Zens Music repository's last commit is April 21, 2026, labelled 1.29; not tested on 1.30 here |
| Bundled music | None | 61 `.ogg` files; the readme calls them copyright-free classical tracks and his own music. They are not needed |

Recommendation: own build. A repack is not available without reopening the CF removal, which `CLAUDE.md` lists as settled. What Zen's licence gives the own build:

- The boombox model as fallback 1 under ruling M-4 (see Models).
- The sync method: a server timer advances a net-synced elapsed-seconds counter, and the client starts the wave at `elapsed / length` (`Zen_MusicDeviceBase.c:226-227,329-344`). This also answers Risks R1 in part: the method ships in a maintained public mod.
- The structure: one base PBO and one audio PBO, one `CfgVehicles` class plus one shader and one sound set per tape, a custom cassette slot with a ghost icon (`ZenMusic:ZenMusicBase/config.cpp:703-710`), and a cassette case on a retextured vanilla model.
- The vehicle playback code as a reference for the DPV mount in Phase 2.

His files carry no licence header, so each file or method taken gets a comment naming its source file, commit, and MPL 2.0, and `NimbyMusic/NOTICE` credits Zenarchist and lists every file and change, as the Hands and Legs forks do. Code written fresh after reading his is MIT.

The choice does not flip on features any more: radio stations and car stereos would have to be rebuilt without ZenModCore either way.

## Music rights

Every tape is redistributed to the public through a Workshop item. The earlier standing rule in `docs/research/candidate-features.md`, "only music we have the rights to", is superseded for flagged tapes by rulings M-6 and M-7 (November 9, 2026); that file still carries the old wording.

- Two classes of track. Unflagged: CC0, CC BY, public-domain recordings, and recordings Jeff, Jay, or players made. A public-domain composition is not enough; the recording has its own rights. Flagged (`copyrighted = 1`): commercial music.
- Unflagged tracks ship inside `@NIMBY`. Flagged tracks ship in the second Workshop item.
- How Zen's flag works, since M-7 asks for something similar: the check runs on each client when the device state syncs, and a flagged tape is silenced unless that player has switched a music option on (`ZenMusic:ZenMusicBase/scripts/4_World/ZenMusicBase/Entities/ZenMusic/Devices/Zen_MusicDeviceBase.c:172-188`; the option lives in ZenModCore's options page, default off). So Zen's default is opt-in to hear flagged tapes. NIMBY's default is ruling M-12: a one-time prompt, audible when no choice is made.
- `tapes.json` carries licence, author, and source link per track; the build writes them to `NOTICE` and to the track listing.
- Audio files stay out of git, as the DPV model sources do. The manifest is tracked; the files live under `D:\DayZProjects\NimbyMusic\` and on the NAS.
- The flag protects streamers, not the Workshop item. A rights holder's complaint to Steam can remove the second item. This is not legal advice.

## Phases

| Phase | Proves or delivers | Exit |
| --- | --- | --- |
| 0. Pipeline proof | One deck class on the Wilga model, one tape, one 3 to 4 minute track, no loot rows, test storage only (999 fresh world; 998 for the save and reload round trip, the two instance ids `Start-NimbyTestServer.ps1:141` accepts) | See Phase 0 checks |
| 1. Deck and tapes | Manifest and generator, multi-track tapes with Next and Previous (M-10), infected attraction (M-9), 10 to 15 tapes, loot rows drafted, self-test cases, release gate 9.2 with the two content-addon boots. Track display (M-11), the flagged-audio second item (M-13), the first-play prompt (M-12), and generated tape labels (M-14) also land here | Live at a restart |
| 2. Boombox | Larger model, two 9V batteries, longer range, placeable; DPV mount slot | Live |
| 3. Mixtapes and case | Off the plan by ruling M-5 (revisit after Phase 2); the tape case moves to Phase 2 | Not scheduled |

### Phase 0 checks

| # | Check | Why it could fail |
| --- | --- | --- |
| 1 | A 3 to 4 minute mono `.ogg` plays to the end from a deck on the ground | Vanilla's longest object-bound precedent is a loop |
| 2 | `SetStartOffset` starts a second client at the right point, within about one second, after walking into range and after a relog | Offset on a long sample is untested in this pack |
| 3 | The sound follows a deck in hands, in a backpack, and in a vehicle | An item in cargo has no world transform of its own; the parent must be the hierarchy root |
| 4 | Falloff and volume read correctly at 5, 25, and 60 m, indoors and out | Shader values are guesses until heard |
| 5 | A drained or removed battery, an ejected tape, a ruined deck, and a deleted deck each stop the sound on every client | Orphaned `EffectSound` |
| 6 | Earplugs muffle it | Depends on the wave kind the effect uses |
| 7 | Save, restart, reload: the deck returns with tape and battery, stopped. A boot without the addon shows only the Music items missing and 0 "Scripted variables corrupted" | Rule 6.1 |
| 8 | Two decks in range play two tracks without cutting each other | Shader `limitation` |
| 9 | Pack build time, PBO size, and Workshop update size with one track, then with a second added | Sets the tape budget |

Checks 2, 3, 4, and 8 need the second client (the G15).

## Risks

| # | Risk | Effect | Mitigation |
| --- | --- | --- | --- |
| R1 | Offset start drifts or is ignored on long samples | Players hear different points of a song | Phase 0 check 2; fallback is restart-from-top for late joiners |
| R2 | Pack size grows with every tape | Slower first join, larger updates | Separate audio PBO; tape budget set from Phase 0 check 9 |
| R3 | A rights complaint against the Workshop item | Steam can take the whole `@NIMBY` item down, which blocks every join | Rights ledger; a second Workshop item for audio is the stronger isolation and is Jeff's call (it reopens the one-item ruling in 9.3) |
| R4 | Music gives away position | By design on a hardcore server | None needed; say so in Greet |
| R5 | The vanilla cassette's `zbytek` selection might cover the whole body, not the label alone | A per-tape texture is then a full cassette repaint | Phase 0 looks; a full repaint per genre is acceptable, a Claude-made cassette is the fallback |
| R6 | Two 9V batteries on one item | Whether vanilla `EnergyManager` can draw from two battery attachments was not verified; Zen's boombox takes one `BatteryD` | Phase 2 question; fallback is a second slot summed in script, or one battery with a higher drain |
| R7 | Bohemia gives `Radio` or `Cassette` a behavior later | Name or behavior overlap | Own classes; nothing depends on the vanilla classes beyond model paths |
| R8 | RPC block registry is inconsistent | `65512xx` is the DPV row in shared rules 4.1, and the Fang plan and Tweaker study also name it; the Shoulder study names `65513xx` | Jeff assigns Music's block when he amends 4.1. Phase 0 needs no RPC (synced variables and the action framework) |

| R9 | Origin of Zen's boombox and walkman models is not stated | A third party's model in a public pack without credit | Applies only if the Claude-made boombox falls short (ruling M-4): ask Zenarchist then, and record the answer in `NOTICE` |
| R10 | Zens Music is labelled 1.29 | A method that worked on 1.29 could behave differently on 1.30 | Phase 0 check 2 proves the offset start on 1.30 |

## Decisions

Ruled by Jeff Fabish on November 9, 2026, in the Music interview. "Default" marks a value Claude assumed from Jeff's request without asking.

| # | Question | Ruling |
| --- | --- | --- |
| M-1 | Build path | Own build, no Community Framework. Jeff's words: "I'd rather not rely on the Community Framework if viable. If it's too much work, we can. Ideally, if we have to include it, we pack it into Nimby." Zen's sync method is used with credit in `NOTICE` |
| M-2 | Media | Cassettes (default; Jeff's concept, and vanilla has the tape) |
| M-3 | Handheld player | The vanilla Unitra Wilga deck. No walkman |
| M-4 | Boombox model | Claude-made in Blender first and tested in game. Fallback 1: Zen's `zen_boombox.p3d`, after Zenarchist confirms its origin. Fallback 2: a bought model, shipped as ODOL only |
| M-5 | Player-made tapes | Neither mixtapes nor submissions for now. Revisit after Phase 2 |
| M-6 | Where audio ships | Free-licensed and own tapes ship inside `@NIMBY`, so music survives any takedown. Tapes flagged `copyrighted = 1` ship in a second Workshop item. This amends the one-item ruling in shared rules 9.3; the amendment is Jeff's to write |
| M-7 | Music sources | Commercial music is allowed. Jeff's words: commercial music "with a copywrite flag similar to Zen's". Every commercial track carries `copyrighted = 1`. The client mute option is Claude's reading of that; its default is ruling M-12 |
| M-8 | Build slot | Phase 0 starts now on `feat/nimby-music` in its own worktree, as an approved exception to the one-Phase-0 rule beside Central Scrutinizer |
| M-10 | Multi-track tapes | Asked by Jeff, November 9, 21:59 Central: one tape carries several tracks (for example a decade compilation) and the player skips between them on the same cassette. Feasible; design under Multi-track tapes. These are admin-authored compilations, not the player-dubbed mixtapes M-5 set aside |
| M-11 | Track display | Ruled by Jeff, November 9, 22:05 Central: the current track shows in the deck's action prompt and in its inspect window. No toast. Design under Track display |
| M-12 | Flagged-tape default | Ruled November 9, about 22:30 Central: the first time a client would play a flagged track, a one-time prompt asks hear or mute and remembers the answer. With no choice made, flagged tapes are audible. Greet shows the option. This replaces Zen's muted-until-opt-in default |
| M-13 | Phase 1 scope | Track display (M-11) and the second Workshop item for flagged audio both ship in Phase 1. The shared rules 9.3 amendment is therefore owed before the Phase 1 release |
| M-14 | Tape look | Per-tape labels generated by the build: each tape's title and artist drawn on the cassette's label area in a handwriting-style font, so it reads as hand written, then converted to `.paa`. Adding a tape adds no art work. Phase 1, not Phase 0. Fallback, if the label area or the converter does not cooperate: all tapes vanilla, names only |
| M-9 | Infected | Playing music attracts the infected through the vanilla noise system, scaled by volume. Built in Phase 1 |

### Multi-track tapes

A tape is a list of tracks, not one sound. A single-track tape is a list of one, so Phase 0 already uses this shape.

| Piece | Design |
| --- | --- |
| Manifest | `tapes.json` splits into two lists: tracks (file, title, artist, licence, `copyrighted`) and tapes (name, description, ordered track ids). A track can sit on several tapes without its audio being packed twice |
| Config | The generator writes one shader and one sound set per track, and per tape class two arrays: `tracks[]` (sound set names) and `trackSeconds[]` |
| State | The deck's synced state gains a track index beside elapsed seconds. A late joiner reads both and starts the right track at the right fraction |
| Advance | The server timer moves to the next track when the current one's seconds elapse, and switches the deck off after the last |
| Skip | Two actions on the deck, Next track and Previous track. Each sets the index, zeroes elapsed, and every client restarts the wave. A short fast-forward sound between tracks is optional polish |
| Position memory | The tape keeps its track index in a net-synced variable, so ejecting and reinserting resumes the same track within a server session. It is not persisted, so every tape is at track 1 after a restart. Persisting it is allowed for an own class under rule 6.1 (version guard, one integer) and can be added later |
| Flagged audio | `copyrighted` is per track. A tape with any flagged track is generated into the second Workshop item; the streamer mute skips flagged tracks one by one |

The alternative, one long file per tape with chapter offsets, is rejected: shared songs would be packed once per tape, and a skip would depend on offset accuracy that Phase 0 has not measured yet.

Precedent: Zen's static boombox plays a list in order with a Next song action (`ZenMusic:.../UserActionsComponent/ZenMusic/ActionZenStaticBoomboxNextSong.c`), though it swaps cassette entities instead of indexing tracks on one.

### Generated tape labels

Ruling M-14. A build step, not art work.

1. Phase 0 finds where the label sits in the vanilla cassette's texture (`dz\gear\tools\data\cassette_co.paa`) and confirms the `zbytek` selection covers it.
2. For each tape row, the generator draws the title and artist onto a copy of that texture in a handwriting-style font and converts the result to `.paa` with the DayZ Tools converter.
3. The generated tape class sets `hiddenSelections[] = {"zbytek"}` and `hiddenSelectionsTextures[]` to that file.

Open points: the font (an openly licensed handwriting face; only rendered images ship, not the font file), how long a title fits before it is shortened, and the `NOTICE` line for a texture derived from Bohemia's. Each texture adds to pack size; Phase 0 check 9 measures one.

### Track display

Every client holds the tape's track list from config and the deck's track index from its synced state, so the name is a local lookup; nothing extra crosses the network.

| Where | What the player sees | Source |
| --- | --- | --- |
| Action prompt | "Next track (3/12: Song Title)" while holding or looking at the deck | `ActionBase.OnActionInfoUpdate` rewrites an action's text (`scripts/4_World/DayZ/Classes/UserActionsComponent/ActionBase.c:1342`); vanilla uses it for build parts and combination locks |
| Deck inspect window | "Now playing: 3/12 - Artist - Song Title" | `DescriptionOverride(out string)` on the deck (`scripts/3_Game/DayZ/Entities/Object.c:1349`), read through `GetTooltip` (`scripts/3_Game/DayZ/Entities/InventoryItem.c:53`, `scripts/5_Mission/DayZ/GUI/InspectMenuNew.c:160`) |
| Tape inspect window | The tape's full track listing, readable before it is played | The same override on the tape class |

Not built: a toast on track change (`NotificationSystem.AddNotificationExtended`, `scripts/3_Game/DayZ/Client/Notifications/NotificationSystem.c:215`). It would tell a player in earshot what is playing without handling the deck. A holder-only toast stays available as a later option.

With the streamer mute on, a flagged track shows its name with "muted". Titles and artists come from the manifest's track rows.

### What the rulings change

- Phases: Phase 3 (mixtapes, submissions) is off the plan; the tape case moves to Phase 2 with the boombox.
- Packaging: three PBOs in two Workshop items. `NimbyMusic` (script and config) and `NimbyMusicTapes` (free-licensed audio) in `@NIMBY`; `NimbyMusicTapesX` (flagged audio, requires `NimbyMusic`) in the second item, signed with the same `nimby` key and loaded as a second `-mod=` entry.
- Takedown behavior: if the second item is removed, the server drops it from `-mod=`. Flagged tape classes then no longer resolve, and the engine drops those entities from `storage_1` at load, including tapes sitting in decks. Decks, batteries, and free-licensed tapes are unaffected. The release gate for the second item needs the rule 6.1 "boot without the addon" check on its own.
- Rights: the `copyrighted` flag protects streamers, not the Workshop item. A complaint to Steam can still remove the second item. This is Jeff's accepted risk, not legal advice.
- `tapes.json` gains a `copyrighted` field; the generator routes each row to the right audio PBO by that field.
- Not asked, still open: the RPC block (R8) and the tape budget (Phase 0 check 9).

## Corrections

Self-review, November 9, 2026. Each was Claude's error in an earlier version.

| Version | Was | Is | Cause |
| --- | --- | --- | --- |
| v0.1 to v0.5 | The vanilla cassette has no hidden selection; tapes need a Claude-made model to look different | The model has a `zbytek` selection, so per-tape textures are a config entry | A truncated search listing; the name sorted past the cut |
| v0.1 to v0.5 | The Wilga takes a black or white texture by config | No retexture selection found; black only | Texture files exist, but the model does not expose them |
| v0.1 to v0.5 | Phase 0 runs on storage 997 or 999 | 998 and 999 are the only accepted test instances | Not checked against the script |
| v0.1 to v0.5 | Clients on a BattlEye server load only signed PBOs | Signature verification does that, and the point is reasoning, not a test | Wrong mechanism named |
| v0.2 to v0.5 | Files from Zen keep their MPL 2.0 notice | His files carry no notice; MPL 2.0 is stated on his pack wiki | Licence read from the wiki, not the repository |
| v0.3 to v0.5 | Zen's flag mutes for streamers | Zen's flag silences flagged tapes by default until a player opts in | Flag code read only in this review |

## Sources

- [ZenMusic repository](https://github.com/ZenarchistCode/ZenMusic), commit `3de1452` (April 21, 2026, "1.29"), and [ZenModCore repository](https://github.com/ZenarchistCode/ZenModCore), commit `5a64e36` (October 25, 2026), both read November 9, 2026.
- [Zens Music wiki page](https://github.com/ZenarchistCode/ZenModPack/wiki/ZenMusic) and [Zen pack wiki home](https://github.com/ZenarchistCode/ZenModPack/wiki) (licence, how tapes are added, `playSeconds` sync), read November 9, 2026.
- [Zens Music Workshop listing mirror](https://catalogue.smods.ru/?p=326720) (138 MB as of its January 2025 revision, repack statement). The Steam page itself did not load in this session.
- [DayZ Underground cassette player announcement](https://www.dayzunderground.com/?p=6504) (battery, play and stop, community-submitted tapes).
- [Psyern's Radio Show Workshop page](https://steamcommunity.com/sharedfiles/filedetails/?id=3627848296) (457 MB, size comparison).
- [DayZ Modding Wiki, audio chapter](https://stardz-team.github.io/DayZ-Modding-Wiki/en/04-file-formats/04-audio) (Ogg Vorbis, mono for positional sounds).
