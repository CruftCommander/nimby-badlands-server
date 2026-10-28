# Workshop item: @NIMBY (release 1, October 26, 2026)

Gate step 7. Published by hand from DayZ Tools Publisher on Jeff's Steam account (decision 5). Artifact: `build\@NIMBY-20261026-214828` (six signed PBOs, `keys\nimby.bikey`, `mod.cpp`). Preview: `mods/own/nimby-pack/preview.png` (512 x 512, 14 KB, original artwork, no third-party marks). Visibility: Public. Tags: Mod, Types: Server, Mechanics.

## Title

NIMBY

## Description (paste)

Server pack for NIMBY | HARDCORE | BADLANDS | 1PP (Nasdara). Required to join the server. One item, six small mods; every key is rebindable under Options > Controls > NIMBY.

[b]Legs[/b] - autorun. Double-tap Left Shift; any move, fire or raise input stops it.
[b]Ears[/b] - earplugs. N lowers effect volume, N again restores. Voice chat is unaffected.
[b]Scrawl[/b] - notes. Pen in hands, drop a sheet on it, hold the action key to write; hold left mouse to read. Paper stacks to 5.
[b]Tack[/b] - pin notes to walls with a hammer and nail, duct tape, epoxy putty or a knife. Take them down again.
[b]Hands[/b] - hand an item to another player. Hold it and press O to offer; empty hands and O asks for theirs. Bind "Hands: open or close pockets" to let others put items in your pockets.
[b]Greet[/b] - the server's welcome window with the rules and how-tos. Insert reopens it; so does Server welcome in the pause menu.

No client-side settings to edit; no other items required.

Source is public: https://github.com/CruftCommander/nimby-badlands-server (MIT). Issues and ideas: the NIMBY Discord, https://discord.gg/kH3eJ5mdEv (the invite in the reviewed Greet text; the panel hostname shows discord.gg/bQDNepCRF, Jeff confirms which is current).

Credits. Legs is a fork of SchanaModAutorun (Apache-2.0); Hands is a fork of Give&Take by affenb3rt (Workshop 2903112334), repacked and modified with the author's stated permission; each fork's NOTICE in the repository lists every change. Everything else is written for this server.

## Publisher steps (Jeff)

1. Steam > DayZ Tools > Publisher. New item. Source folder: `D:\GitHub\nimby-badlands-server\build\@NIMBY-20261026-214828` (it must contain `addons\`, `keys\`, `mod.cpp`; nothing else). Every update from release 1 patch 1 on also carries `meta.cpp` (`publishedid = 3828109139`), which the build writes from `pack.json` `workshopId`; without it the server reports Workshop id 0 and the launcher cannot download the pack (NIMBY-T-061).
2. Title `NIMBY`, preview `D:\GitHub\nimby-badlands-server\mods\own\nimby-pack\preview.png`, description above, visibility Public, tags as above. Change note: `Release 1: Legs, Hands, Ears, Scrawl, Tack, Greet (build 2026-10-26-214828).`
3. Publish. Record the item ID below and in the release-gate record (step 7).
4. HostHavoc panel, service 246248: `Steam Workshop (installed)` > install the item by ID; confirm `@NIMBY\keys\nimby.bikey` was copied to the server `keys\` folder (`Non-Workshop Mods` or the File Manager shows it); if not, upload `build\@NIMBY-20261026-214828\keys\nimby.bikey` to `/keys/` over SFTP.
5. Launch line (`Commandline Manager`): `-mod=@NIMBY` (today the line is `-servermod="" -mod=""`). Applies at the step 8 restart.

### Every Workshop update, added October 27, 2026 (NIMBY-T-061)

The launcher downloads the pack only if the server reports the item id, which it reads from `/@NIMBY/meta.cpp`. Release 1 went out without it and no new player could join for about 19 hours (`docs/operations/2026-10-27-incident-launcher-workshop-id.md`).

6. Before publishing: the source folder is a `Build-NimbyPack.ps1` build from `main` at or after d1a40b1, and its `meta.cpp` reads `publishedid = 3828109139;`.
7. After the server install: `/@NIMBY/meta.cpp` on the server (panel File Manager or SFTP) reads `publishedid = 3828109139;`. The panel's installer has written `publishedid = 0;` before; if it does, stop the server, upload the right file over SFTP (the running server locks it), start the server.
8. After the restart: `https://dayzsalauncher.com/api/v1/query/104.192.227.46/2303` returns `status 0` and lists `NIMBY` with `steamWorkshopId 3828109139`.
9. Before announcing: join through the DayZ launcher from the test Steam account with NIMBY unsubscribed; the launcher offers the download and the join completes.

Item ID: **3828109139** (https://steamcommunity.com/sharedfiles/filedetails/?id=3828109139). Published October 26, 2026, 23:36 Central, by Claude driving DayZ Tools Publisher at Jeff's request: Public, tags Equipment and Mechanics, content 711.4 KB (Publisher) / 728.5 KB (Workshop page), "all signed". Verified from outside at 23:38: title, author Road Dog, description and preview visible to an anonymous visitor.
