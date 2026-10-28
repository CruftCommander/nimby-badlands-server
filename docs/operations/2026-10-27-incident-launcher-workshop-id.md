# Incident: the launcher could not download @NIMBY (NIMBY-T-061, October 27, 2026)

Severity high: from the release-1 launch (October 26, 2026, 23:57 Central) until 18:47 Central on October 27, no player without a NIMBY subscription could join the live server. Found by Jeff from a player's launcher screenshot at 17:09; fixed live at 18:47; confirmed by Jeff at 19:02 from a clean test Steam account (G15 laptop) and from his desktop.

## What players saw

On join, the DayZ launcher showed "Content required by server: NIMBY" with only Ignore, Find on Steam Workshop and Find on the web. The automatic subscribe-and-download never appeared.

## Cause

The server advertises each `-mod=` folder to the launcher with the Workshop id it reads from that folder's `meta.cpp` (`publishedid`). The launcher subscribes the player to that id. Ours was 0:

1. `Build-NimbyPack.ps1` wrote `addons\`, `keys\` and `mod.cpp`, never `meta.cpp`, so the Workshop item 3828109139 was published without one.
2. The HostHavoc panel's Manual Workshop Installer (Friendly Name mode, folder `@NIMBY`) then wrote its own `meta.cpp` into the server folder: `protocol = 1;`, `publishedid = 0;`, `name = "NIMBY";`, `timestamp = 5250972744356698013;`.
3. The server reported NIMBY with id 0. The DZSA launcher query (`https://dayzsalauncher.com/api/v1/query/104.192.227.46/2303`) returned `The mod 'NIMBY' has a Steam Workshop ID of '0'`, and the launcher had nothing to subscribe to.

Nothing tested this path before release. Every test client loaded the build folder directly with `-mod=`, and Jeff's own clients had the mod locally, so the first person to need the download was a real player.

## Not the cause

The Workshop item itself was public and subscribable throughout. A first read of its page source suggested a takedown; the "removed from the community" and "incompatible" notices are hidden templates in every Workshop page's code, not displayed. Read signed out and as the author: no removal notice, owner controls shown. Don't diagnose a Workshop item from its raw page source.

## Fix

- Live (Jeff's approval, Jeff's password): replaced `/@NIMBY/meta.cpp` with `publishedid = 3828109139;`. The running DayZ server holds `meta.cpp` open (an SFTP delete failed with `I/O error` and an overwrite with `File is already exclusively open.`), so the fix was panel Stop (18:45, 0 players), upload with `logs\upload-meta-20261027.ps1` (old file in `logs\live-meta-backup-20261027-184631\`), Start (18:47:00). The launcher query at 18:49 listed `NIMBY` with `steamWorkshopId 3828109139`.
- Permanent: PR #2 (d1a40b1). `pack.json` carries `"workshopId": "3828109139"` (required, digits, not 0), and `Build-NimbyPack.ps1` writes `meta.cpp` into every client build, so each Workshop update ships the id.

## How to avoid it

1. Every Workshop update is published from a build made with `Build-NimbyPack.ps1` on `main` after d1a40b1. Check the build folder has `meta.cpp` with `publishedid = 3828109139` before publishing.
2. After each Workshop install or update on the server, read `/@NIMBY/meta.cpp` in the panel File Manager or over SFTP. If the installer wrote `publishedid = 0` again, stop the server, upload the right file, start it.
3. After each restart that follows a Workshop change, run the launcher query above. Clean means `status 0` and `mods` lists `NIMBY` with `steamWorkshopId 3828109139`. Any other answer blocks the announcement.
4. Before telling players about a release or update, join once through the DayZ launcher from the test Steam account with NIMBY unsubscribed, and confirm the launcher offers the download. This is the only check that exercises the path a new player takes.
5. Changing `meta.cpp` on the live server needs the server stopped (the file is locked while it runs). Use Stop and Start around the upload; with players online, use the scheduled restart window.

Steps 1 to 4 are in the Workshop procedure, `docs/operations/2026-10-26-workshop-item.md`. A one-line addition to the release gate in shared rules 9.2 (orchestrator-owned) is proposed: "After the release restart, the launcher query lists every Workshop item with its real id, and a client with no subscription is offered the download."

## Open follow-ups

- After the patch-1 Workshop update, confirm the installer keeps our `meta.cpp` instead of writing id 0 again.
- `Write-MetaCpp` writes no `name` line; the installer's file has one. Add it in the next tooling change.
- Optional tooling: a script that runs the launcher query and fails unless every mod has a nonzero `steamWorkshopId`, run after each restart pull.
