# Next actions

Updated October 28, 2026, 23:10 Central.

0. **Patch 1 is blocked on NIMBY-T-063 (Hands step 2 crashes the server).** PR #12
   (`fix/hands-t063-tack-t065`, three commits) holds the step 1 wait that works, the Tack
   T-065 fix, and the patch-1 record; it must not merge until Hands transfers confirm.
   Next session, in this order (Jeff, October 28, 23:05):
   - the dev-harness bot-to-bot transfer as a boot case: two dummies, wrench in A's hands,
     real `NimbyHandsMoveItemToPlayer.Execute` through step 1 and step 2, assert
     `CONFIRMED` and the wrench in B's cargo. Needs a harness switch so `CheckPlayer` accepts
     a dummy with no identity (the existing cases expect "giver disconnected" for one), and
     `NimbyHandsServer.Tick()` called after each `Execute` so the wait resolves in the same
     tick. Also update `hands-take-without-consent` and the restrained-deposit case: `Execute`
     now returns true (registered and waiting), the refusal lands after the tick;
   - step 2 instrumented: `GameInventory.LocationSyncMoveEntity(src, dst)` and
     `InventoryInputUserData.SendServerMove(null, InventoryCommandType.SYNC_MOVE, src, dst)`
     called separately with a debug line between (that is all `TakeToDst(SERVER)` does), plus
     the juncture and reservation dump the diag printed before its move;
   - `scripts\Read-NimbyTestRun.ps1`: per-id Hands state chains and timings, UNCONFIRMED
     count, `SCRIPT (E)`, `Scripted variables corrupted`, crash files, for a `testserver-*`
     run folder, with a Pester test;
   - then S6-30 on the new build, two clients, and the rest of the record.
   Open question to settle first with the bot case: the game server binary changed on
   October 22 (1.30.164078 -> 1.30.164200); the October 20 diag's identical step 2 call
   confirmed on the old build.

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
7. NimbyKit: orchestrator adds the identifiers row to shared rules 4.1, then build per
   `docs/plans/2026-10-26-testkit-brief.md`.
8. Jeff: rule on the Halloween overlay (recommended: skip this year). If wanted, port
   `halloweenOffline.sakhal` rows, log each in `LOOT-CHANGELOG.md`, apply at a restart,
   revert after November 6.
9. After a clean first week: delete the rollback set (`D:\DayZServer\_release1-upload\`
   and the three `storage_998` zips) - Jeff's call.
