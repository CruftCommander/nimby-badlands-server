# Candidate features and mods

> **Superseded in part, October 15, 2026.** The smoking, cannabis, Zippo, and hookah rows below predate the 1.30 script reference. Vanilla 1.30 ships cannabis seeds, a six-stage cannabis plant, a `Cannabis` crop, three cigarette-pack brands, a `PetrolLighter`, and persisted written notes. The current design is `docs/design/puff-sprout-scrawl-earplugs-feasibility.md` and plans 4-9 in `docs/plans/`. New candidate recorded October 15, 2026, not planned: a base-size governor that detects an oversized base (item count or render cost affecting other players) and removes items lowest-priority first; Jeff raised it alongside the note-lifetime decision.

Research v0.2, October 14, 2026, the day before Badlands (update 1.30) releases. Covers roadmap phases 3 and 4 plus the two quality-of-life mods in the launch set. Owner: Jeff Fabish. Jeff ruled on five decisions on October 14, 2026 (see Decisions); everything else waits for him.

Every Workshop mod below was last tested by its author on 1.29 or earlier, and no author has said anything about 1.30. Bohemia warns that 1.30 changes the animation graph, the navmesh format, and the mod filesystem, and that animation-related mods may break. Treat every row as "re-check on October 15".

## What this is not

- Not a mod list. The final list waits for the October 15 readiness check and Jeff's call on how each mod is included.
- Not a license opinion. The terms below are quoted or summarized from each Workshop page; read the page before relying on them.
- Not a performance test. Nothing here has run on a server yet.

## How a mod can be included

Each recommendation names one of four paths. The license decides which paths are open.

| Path | Meaning | Needs |
| --- | --- | --- |
| Workshop dependency | Players subscribe to the author's Workshop item; we ship nothing | Author keeps it working on new builds |
| Repack | We bundle the author's files into our own NIMBY pack | Terms that allow repacking |
| Fork | We take the source, maintain our own copy, and fix it on new builds ourselves | Source plus terms that allow modification |
| Own build | We write it from scratch | Our time; for models, 3D work |

## What 1.30 ships natively

| Item | Finding | Status |
| --- | --- | --- |
| Regional clothing | "New regional firearms, clothing, and loot"; FNO uniform in night desert camouflage | Confirmed (Bohemia dev blog recap, September 22, 2026) |
| Shemagh | Already vanilla in four colors | Confirmed (vanilla) |
| Keffiyeh, turban, pakol, shalwar, sandals | Named by third-party sites and listed as upcoming on the community wiki | Reported, not official |
| Thobe, long beards, hookah, cigarettes, lighters, music | Not found anywhere | Unknown; assume absent |
| Jana 50 moped, Bitrak 682 dirt bike | New motorbikes on all maps | Confirmed (1.30 experimental change log) |
| Helicopters | Not announced | Assume absent |
| Which cars spawn on Nasdara | No spawn data published | Unknown until October 15 |

The motorbikes matter to the rigged-vehicle mod: Jeff put them on its allowlist (see Decisions), and whether they derive from `CarScript` and have a horn is checked when the 1.30 scripts publish.

## Recommendations by category

### Launch quality of life

| Feature | Recommended | Path | Why | Alternative |
| --- | --- | --- | --- | --- |
| Autorun | Schana's autorun ([GitHub](https://github.com/schana/dayz-mod-autorun), Apache-2.0) | Fork into the NIMBY pack | Tiny feature, permissive license, and we own the 1.30 fix instead of waiting on an author | Carim ([3468061029](https://steamcommunity.com/sharedfiles/filedetails/?id=3468061029), Apache-2.0, updated July 2026) as a Workshop dependency, but it also adds nametags, a compass, and a map, which conflict with hardcore unless they can be turned off (unverified) |
| Give n Take | Give&Take ([2903112334](https://steamcommunity.com/sharedfiles/filedetails/?id=2903112334)) | Fork | Author allows repack, modification, and redistribution; last updated April 2023, so a fork is the only way it works on 1.30. No source published, so the fork starts from the PBO | Give&Take (Fixed) ([3314183971](https://steamcommunity.com/sharedfiles/filedetails/?id=3314183971)) is also stale and has bug reports |

Both forks are small and share one NIMBY client pack. They are also the two mods the October 15 readiness check depends on, so this changes that check: the question becomes "does our fork compile on 1.30" rather than "did the author update".

### Theme drop 1 (phase 3)

| Feature | Recommended | Path | Why | Alternative |
| --- | --- | --- | --- | --- |
| Zippo lighter | Zens Zippo Lighter ([3170285574](https://steamcommunity.com/sharedfiles/filedetails/?id=3170285574)) | Workshop dependency, or repack | Maintained (January 2026), refuels with gasoline, repack allowed. The 3D model is CC BY-NC, so never sell it. Whether it lights cigarettes is unverified | Build our own if it cannot light our smokables |
| Tapes | Zens Music ([3412251200](https://steamcommunity.com/sharedfiles/filedetails/?id=3412251200)) | Workshop dependency, or repack | Maintained (April 2026), cassettes, walkman, boombox, and car play synced to all players; repack allowed; custom cassettes through `types.xml`. Load only music we have the rights to | FoXyRadio ([3075855922](https://steamcommunity.com/sharedfiles/filedetails/?id=3075855922)), no edits or reuploads |
| Cigarettes, joints, and hookah | One unified smoking system | Own build | No maintained smoking mod allows repacking; the smoking mods use custom animations, which 1.30 puts at risk; and no DayZ hookah mod exists, so hookah is an own build regardless. One shared "smokable" base covers all three | Adopt Smoking_A_Cigarette ([3279662973](https://steamcommunity.com/sharedfiles/filedetails/?id=3279662973), no repacks) for cigarettes if drop 1 cannot wait |
| Cannabis growing | Defer | - | The only maintained option is IceBlade's stack (CannabisPlus [1932611410](https://steamcommunity.com/sharedfiles/filedetails/?id=1932611410), DrugsPLUS [2170927235](https://steamcommunity.com/sharedfiles/filedetails/?id=2170927235), AgricultureCore [3690289718](https://steamcommunity.com/sharedfiles/filedetails/?id=3690289718)): three or four mods, no repacks, and the author warns it can wreck a server economy. Joints come from the smoking system | Adopt the IceBlade stack if growing becomes a goal |
| Regional clothing | Wait for vanilla 1.30 | - | Badlands brings regional clothing; inventory what ships on October 15 before adding anything | - |
| Thobe | Own build, after the vanilla inventory | Own build | No DayZ thobe mod found | - |
| Long beards | Own build | Own build | No DayZ long-beard mod found; this is new character-model work and the most expensive item on the list. Rule from Jeff, October 16, 2026: beards are not customizable; a male character's beard grows continuously from spawn up to a maximum length until he shaves it | StartWithBeard ([2981585492](https://steamcommunity.com/sharedfiles/filedetails/?id=2981585492), server-only, permissive) only gives spawn beards from vanilla |

Zens Zippo and Zens Music share the Zens Core dependency ([3702420204](https://steamcommunity.com/sharedfiles/filedetails/?id=3702420204)), so adopting one makes the second cheaper.

### Vehicles (phase 4)

| Option | Status | Notes |
| --- | --- | --- |
| RedFalcon Flight System Heliz ([2692979668](https://steamcommunity.com/sharedfiles/filedetails/?id=2692979668)) | **Drop** | Description says no longer supported as of 1.29 |
| DayZ Expansion Vehicles ([2291785437](https://steamcommunity.com/workshop/filedetails/?id=2291785437)) | Leading candidate | Actively developed (flight-model work May-August 2026, 1.29 animation graph update). Requires CF, Dabs Framework, Expansion Core, and Expansion Licensed. Licensed content is non-commercial and cannot be reposted, so Workshop dependency only |
| LM's Helicopter Flight Systems ([3624740747](https://steamcommunity.com/workshop/filedetails/?id=3624740747)) | Lighter alternative | No dependencies, updated July 2026, no derivatives and no repacks |

Pick after both publish 1.30 builds; phase 4 has no date pressure.

## Decisions

Made by Jeff on October 14, 2026.

| Decision | Choice | Consequence |
| --- | --- | --- |
| Launch QoL pair | Fork Schana's autorun and Give&Take into one signed NIMBY client pack | The October 15 readiness check tests our fork on 1.30, not an author's update. Credit both authors in the pack description; keep Schana's Apache-2.0 notice |
| Zen ecosystem | Repack Zens Core, Zens Zippo Lighter, and Zens Music into the NIMBY pack for theme drop 1 | We pick up Zen updates by rebuilding the pack. Credit Zenarchist; the Zippo model is CC BY-NC, so the pack is never sold or monetized in a way that sells the model |
| Smoking | One own-build smoking system: cigarettes, joints, and hookah on a shared smokable base, lit by the Zippo | Largest build item in theme drop 1 |
| Cannabis growing | In scope as a later own build on vanilla gardening, after the smoking system ships | Joints come from loot and crafting until then |
| Rig the 1.30 motorbikes | Yes; Jana 50 and Bitrak 682 join the rigged-vehicle allowlist once the 1.30 hooks are confirmed | See the rigged-vehicle design for the condition on horn mode |
| Splash (October 16, 2026) | Custom loading screen: server art, logo, and a text line while the client loads. Client-only `modded class LoadingScreen`; images baked into the PBO; changes rarely. Separate from Greet because a Bohemia change to the loading-screen class breaks only this. Phase 0: confirm the class is still moddable on 1.30 (1.30 changed the mod filesystem). Code prefix `NimbySplash` | Proposed by Jeff; name decided by Jeff, October 16, 2026 |
| Greet (October 16, 2026) | Welcome window after spawn: rules, Discord invite, tips, restart schedule, with a per-client "do not show again for this version" flag. Text is a JSON file in the server profile folder sent by RPC at connect, so it is edited without a client update; vanilla `messages.xml` hourly lines stay, the join line moves into the window. Phase 0: confirm a window at spawn does not fight the vanilla connecting HUD states. Code prefix `NimbyGreet` | Proposed by Jeff; name decided by Jeff, October 16, 2026 |
| Payload (October 16, 2026) | The vehicle-bomb feature is named Payload (Rigged for a few hours on October 16; Rigged ADA before that). Any vehicle with storage can be rigged if its cargo is large enough to hold the explosive; that replaces the class allowlist as the eligibility rule | Supersedes the allowlist line in `docs/design/rigged-ada-feasibility.md` (see its banner) |
| Default-behavior tweaks (October 16, 2026) | A mod dedicated to changing vanilla game behavior where no other NIMBY mod owns the behavior. Unnamed; Jeff names it. First two ideas, recorded only, no plan and no feasibility yet: (1) dust storms dehydrate the player faster (Badlands 1.30 weather event; expected shape is a modded water-drain multiplier while the storm is active); (2) a chance to draw fuel from the wrecked fuel tanker trailers placed on Nasdara (reference photos `docs/design/reference/2026-10-16-nasdara-fuel-tanker-wreck.jpg`, the semi tanker trailer, and `2026-10-16-nasdara-small-tank-trailer.jpg`, the small rusted farm tank trailer, both added by Jeff on October 16, 2026; expected shape is a fill action on those static classes with a per-object chance and a cooldown; whether the small trailer holds fuel or water is a design call for the plan); (3) two riders on a moped instead of one, a passenger seat on the 1.30 motorbikes (Jana 50 and Bitrak 682, the same two Payload rigs; added by Jeff on October 16, 2026). An idea moves to another mod if one owns the behavior | Proposed by Jeff, October 16, 2026; ideas only, not ordered |
| Live loot tuning (October 17, 2026) | Live-server tuning, not a mod: (1) fewer helicopter crash sites; (2) weaker loot at them (too good, too often); (3) fewer Akinaka daggers; (4) fewer golden revolvers (somewhat rare, not gone). Live Nasdara values today: `events.xml` `StaticHeliCrash` nominal 8, min 0, max 0, lifetime 2100 s, restock 0 (`server/live/db/events.xml:859`); heli loot is the heli-crash tier rows in `types.xml`; `Akinaka` nominal 45 (`server/live/db/types.xml:488`); `Magnum_Gold` nominal 24 (`:15413`), `Deagle_Gold` nominal 5 (`:6773`) in case the revolver meant is the Deagle. The change goes through `server/mission-overrides` with a panel backup first and a restart-chain deploy once Jeff rules the numbers | Requested by Jeff, October 17, 2026; recorded only, no numbers yet |
| Bike storage (October 17, 2026) | Mod idea, unnamed and unordered: (1) the Bitrak 682 dirt bike takes a found saddlebag or satchel as an attachment that adds cargo about the size of a buttpack, possibly one per side; (2) the Jana 50 moped has a small built-in cargo slot and takes a found basket attachment for more (a basket may also spawn on the bike). Both are inventory-slot and attachment work on the 1.30 motorbike classes, the same two vehicles as Payload and the two-riders idea; feasibility when ordered. Candidate home: the default-behavior tweaks mod, or its own mod if the attachment models make it large | Proposed by Jeff, October 17, 2026; idea only |
| Rifle slings (October 17, 2026) | Mod idea, unnamed and unordered: a rifle goes on the back only when it carries a sling. Needs a sling item (found loot, maybe craftable from rope or leather) and a sling attachment slot on all or most rifles; without one the rifle stays in hands or in cargo if it fits. Scope when ordered: a new `Sling` item and slot, a `CanPutInCargo` and shoulder-slot condition on `Rifle_Base` (and whether shotguns, crossbows, and long melee follow), the loot row, and what happens to a slung rifle when the sling is removed or ruined. Candidate home: its own mod | Proposed by Jeff, October 17, 2026; idea only |

## Sources

- [Bohemia dev blog recap (2), SteamDB copy](https://steamdb.info/patchnotes/25456423/)
- [Experimental update 1.30 change log, Patchbot mirror](https://patchbot.io/games/dayz/articles/1445-experimental-update-130-change-log)
- [DayZ Badlands, community wiki](https://dayz.wiki.gg/wiki/DayZ_Badlands)
- [low.ms: DayZ Badlands, everything we know](https://low.ms/blog/dayz-badlands-expansion-everything-we-know)
- [PlayFront: Badlands heat and survival mechanics](https://playfront.de/en/dayz-badlands-setzt-auf-extreme-hitze-und-neue-ueberlebensmechaniken/)
- [Expansion Licensed terms](https://steamcommunity.com/workshop/filedetails/?id=2116157322)
- Each Workshop item linked in the tables above, read October 14, 2026.
