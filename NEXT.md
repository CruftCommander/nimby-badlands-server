# Next actions

Updated October 27, 2026.

1. First-week watch: after each 12-hour restart, Jeff runs `scripts\Get-ServerLogs.ps1`
   and Claude reads the new boots per `docs/operations/2026-10-27-release-1-first-week-watch.md`
   (section 1 lists the four checks). Append one row per pull. The pull after the first
   evening with players on is the one that matters.
2. Jeff: refresh the mirror - WinSCP pull of `HostHavocDayZServer` and
   `mpmissions/dayzOffline.nasdara` (download only, Direction Local) - then
   `pwsh -NoProfile -ExecutionPolicy Bypass -File D:\GitHub\nimby-badlands-server\scripts\Sync-LiveReplica.ps1`
   so `server/live/` shows the applied loot values. Expect the `eol` static check to flag the
   LF copies again.
3. Jeff: rule the Discord invite (`discord.gg/kH3eJ5mdEv` in Greet vs `discord.gg/bQDNepCRF`
   in the panel hostname). If Greet is wrong, the fix is a `greet.json` edit with `version`
   raised to 2 and an upload at a restart; no pack rebuild.
4. Scrawl live check: with a pen on live, write a note, pin it with Tack, restart, read it
   back (S6-08 to S6-11 on live). Record in the six-addon session doc.
5. Greet NIMBY-T-059 diagnostic build (client render cap on long bodies).
6. Hands NIMBY-T-010 two-client rows (S6-20 to S6-23) when the second client is available.
7. NimbyKit: orchestrator adds the identifiers row to shared rules 4.1, then build per
   `docs/plans/2026-10-26-testkit-brief.md`.
8. Jeff: rule on the Halloween overlay (recommended: skip this year). If wanted, port
   `halloweenOffline.sakhal` rows, log each in `LOOT-CHANGELOG.md`, apply at a restart,
   revert after November 6.
9. After a clean first week: delete the rollback set (`D:\DayZServer\_release1-upload\`
   and the three `storage_998` zips) - Jeff's call.
