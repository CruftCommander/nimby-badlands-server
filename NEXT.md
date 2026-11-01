# Next actions

Updated October 31, 2026 (item 7; the rest is October 27).

1. First-week watch: after each 12-hour restart, Jeff runs `scripts\Get-ServerLogs.ps1`
   and Claude reads the new boots per `docs/operations/2026-10-27-release-1-first-week-watch.md`
   (section 1 lists the four checks). Append one row per pull. The pull after the first
   evening with players on is the one that matters.
2. Jeff: refresh the mirror - WinSCP pull of `HostHavocDayZServer` and
   `mpmissions/dayzOffline.nasdara` (download only, Direction Local) - then
   `pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Sync-LiveReplica.ps1`
   so `server/live/` shows the applied loot values. Expect the `eol` static check to flag the
   LF copies again.
3. Done October 28, 2026: Jeff ruled the Discord invite `discord.gg/kH3eJ5mdEv` (Greet was
   right; `bQDNepCRF` expires November 14, 2026). The panel hostname now carries it (server
   started 14:08 Central); `server/serverDZ.cfg` records the live hostname.
4. Scrawl live check: with a pen on live, write a note, pin it with Tack, restart, read it
   back (S6-08 to S6-11 on live). Record in the six-addon session doc.
5. Greet NIMBY-T-059 diagnostic build (client render cap on long bodies).
6. Hands NIMBY-T-010 two-client rows (S6-20 to S6-23) when the second client is available.
7. NimbyShared, then Central Scrutinizer (Jeff, October 31, 2026). The Claude Code seat in
   `D:\GitHub\nimby-badlands-server-wt\nimby-shared` runs Plan 11 Phase 0 (recon and the
   move list, then the orchestrator's go) and Phase 1 (the addon, the Greet refactor, the
   Greet regression rows SH-01 to SH-14); PR to `main`. Central Scrutinizer Phase 0 (Plan 3
   v0.7) starts when Plan 11 Phase 1 passes, in a `nimby-admin` worktree cut from
   `feat/nimby-shared`. NimbyKit is absorbed as `spawn.kit` (Plan 3 N17); no separate build.
8. Jeff: rule on the Halloween overlay (recommended: skip this year). If wanted, port
   `halloweenOffline.sakhal` rows, log each in `LOOT-CHANGELOG.md`, apply at a restart,
   revert after November 6.
9. After a clean first week: delete the rollback set (`D:\DayZServer\_release1-upload\`
   and the three `storage_998` zips) - Jeff's call.
