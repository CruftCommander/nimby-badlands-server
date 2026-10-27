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

1. Steam > DayZ Tools > Publisher. New item. Source folder: `D:\GitHub\nimby-badlands-server\build\@NIMBY-20261026-214828` (it must contain `addons\`, `keys\`, `mod.cpp`; nothing else).
2. Title `NIMBY`, preview `D:\GitHub\nimby-badlands-server\mods\own\nimby-pack\preview.png`, description above, visibility Public, tags as above. Change note: `Release 1: Legs, Hands, Ears, Scrawl, Tack, Greet (build 2026-10-26-214828).`
3. Publish. Record the item ID below and in the release-gate record (step 7).
4. HostHavoc panel, service 246248: `Steam Workshop (installed)` > install the item by ID; confirm `@NIMBY\keys\nimby.bikey` was copied to the server `keys\` folder (`Non-Workshop Mods` or the File Manager shows it); if not, upload `build\@NIMBY-20261026-214828\keys\nimby.bikey` to `/keys/` over SFTP.
5. Launch line (`Commandline Manager`): `-mod=@NIMBY` (today the line is `-servermod="" -mod=""`). Applies at the step 8 restart.

Item ID: (fill in)
