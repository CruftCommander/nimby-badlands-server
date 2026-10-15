# Map switch: Chernarus test instance to Nasdara (Badlands)

Run this once DayZ 1.30 with Nasdara is on the server. It moves the live server from the
Chernarus test instance to Nasdara and re-applies every per-mission setting. Expect about
30 minutes, with 5-10 minutes of server downtime. All panel work is in the HostHavoc panel
(service 246248); times are Central.

## Before you start

- 1.30 is on the server. The 12:00 AM / 12:00 PM restart task runs a Steam Update, so the
  first scheduled restart after Bohemia publishes 1.30 installs it. To install it sooner,
  use **Update Server Files** in the panel.
- Players need the Badlands DLC to join Nasdara (Jeff, October 15, 2026). Jay owns it.
- Nobody is on the server, or you have warned them in Discord. This is a fresh world.

## What carries over and what doesn't

| Item | Carries over? | Where it lives |
| --- | --- | --- |
| `serverDZ.cfg` (name, hardcore profile, logging, ports) | Yes, except `template` | `serverDZ_By_HostHavoc.cfg` |
| Scheduled restarts, warnings, lock, update, backup | Yes | Panel > Scheduled Tasks |
| DZSA Launcher listing | Yes; it updates map and version itself | - |
| Join and hourly messages | **No** | `mpmissions/<mission>/db/messages.xml` |
| Gameplay overrides | **No** | `mpmissions/<mission>/cfggameplay.json` |
| World, bases, characters | **No** (fresh world) | `mpmissions/<mission>/storage_1/` |

## Steps

1. **Confirm the build.** Open the panel's **DayZSA Launcher** link (the server check for
   `104.192.227.46:2303`). The version must read 1.30 or later. If it still reads
   `1.29.163709`, run **Update Server Files** and wait for it to finish.
2. **Find the mission folder.** In **File Manager**, open `mpmissions`. Note the new folder's
   exact, case-sensitive name (expected pattern `dayzOffline.<world>`). If no new folder
   exists, Nasdara's server files did not ship: stop here and record it in the hub.
3. **Back up.** Click **Create Backup** and wait for "created".
4. **Stop the server.** Click **Stop**. The server holds `serverDZ_By_HostHavoc.cfg` locked
   while it runs.
5. **Point the server at Nasdara.** In **Configuration Files > serverDZ_By_HostHavoc.cfg >
   Text Editor**, change only the `template = "dayzOffline.chernarusplus";` line to the folder
   name from step 2. Save. Make the same change in the repo's `server/serverDZ.cfg`, bump its
   status line to v0.4, and update the mission comment.
6. **Messages.** In the repo, change "Last wipe: October 14, 2026" in
   `server/mission-overrides/db/messages.xml` to today's date. In **File Manager**, open
   `mpmissions/<nasdara>/db/messages.xml` (create it if missing) and replace its contents
   with the repo file. Save.
7. **Gameplay overrides.** Open `mpmissions/<nasdara>/cfggameplay.json`. Keep the file as
   shipped and set only the keys in `server/mission-overrides/cfggameplay.patch.json`
   (personal light off, hit direction off, stamina 5 kg / 2.0, the two
   building checks). Do not copy Chernarus's file: Nasdara has its own temperatures. Save.
   If the mission has no `cfggameplay.json`, stop and ask; do not create one from Chernarus.
8. **Start the server.** Click **Start** (the first click sometimes does nothing; check the
   status). Expect 87-100 seconds to `Player connect enabled` on Nasdara 1.30, against 65-69 on Chernarus 1.29.
9. **Check the listing.** The DZSA server check shows the Nasdara map name, version 1.30, and
   `NIMBY Badlands | Enhanced Vanilla | RP | 1PP`.
10. **Pull and read the logs.** In PowerShell 7:

    ```powershell
    pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Get-ServerLogs.ps1
    ```

    In the newest RPT, confirm `Hostname of server` shows the name, the storage directory is
    `mpmissions\<nasdara>\storage_1\`, `Player connect enabled` appears, and there are zero
    `SCRIPT (E)` lines. Then write `docs/baselines/<date>-nasdara-1.30-server-logs.md` the
    same way as the Chernarus baseline: the vanilla noise on a new map and build differs.
11. **Join test.** Join through DZSA Launcher. Confirm: first person only, no crosshair, both
    join messages appear, and nights are dark with no personal light. Days run at 4x, so night
    can be up to about 3 real hours away; check the in-game clock or repeat the night check later.
12. **Record.** Fill in "Outcome (fill in Oct 15)" in the Notion hub: build, mission folder,
    and DZSA result.

## Rollback

If Nasdara will not boot or is broken: stop the server, set `template` back to
`dayzOffline.chernarusplus`, and start it. The Chernarus world and its `messages.xml` and
`cfggameplay.json` are untouched by this procedure. Restore the step-3 backup only if
files outside the Nasdara mission folder were damaged.

## After the switch

- The first 12:00 restart cycle runs the graceful chain against Nasdara; check it with the
  steps in `restart-schedule.md`.
- Loot (`types.xml`) tuning starts from Nasdara's stock file (decided 2026-10-14).
- CF and COT were removed on October 15, 2026. They return only under the re-add criteria in
  `docs/design/nimby-admin-feasibility.md`; the NIMBY pack follows the hub's readiness procedure.
