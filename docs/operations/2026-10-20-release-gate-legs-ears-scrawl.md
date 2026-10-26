# Release gate - Legs, Ears, Scrawl, Greet (first release)

Prepared October 20, 2026, after the two-client session (records committed at `1473a3b`). Shared rules 9.2 (release model) and 9.3 (distribution) govern; this checklist applies them to the first release. Release order and the release model are not reopened here.

## 1. Scope

| Mod | Addon | Gate status | Why |
| --- | --- | --- | --- |
| Legs | `NimbyAutorun` | Ready after row G-1 | Two-client rows PASS; NIMBY-T-005 fixed. Row G-1 (ladder, solo) runs before the gate; P2-5 (vehicle) and P2-10, P2-11 (dev harness) are waived for release 1 as known gaps (Jeff, October 20, 2026; `docs/testing/2026-10-19-legs-phase1.md`) |
| Ears | `NimbyEarplugs` | Ready | EP-002 PASS (`docs/testing/2026-10-16-ears-phase0.md`) |
| Scrawl | `NimbyNotes` | Ready | Second-player read and Phase 2a rows PASS; SC-2a-6 not provable in vanilla (`docs/testing/2026-10-16-scrawl-phase0.md`) |
| Greet | `NimbyGreet` | Ready after section 4's open rows | Phase 1 closed October 26, 2026 (`docs/testing/2026-10-26-greet-phase1.md`); joins this release per Jeff's October 21, 2026 ruling (plan G8). Before step 5: Jeff reviews `server/profile-overrides/NimbyGreet/greet.json` (`version` 1, `forceShow` false) and `scripts/Test-GreetText.ps1` passes on it |
| Tack | `NimbyTack` | Held | NIMBY-T-013 and NIMBY-T-014: mud-brick rows TK-P1 to TK-P10 on `study/tack-t013-t014`. NIMBY-T-011 is closed on the replica; its four `types.xml` rows ship with Tack |
| Hands | `NimbyHands` | Held | NIMBY-T-010 (high): no transfer completes; feasibility study brief `logs\brief-hands-t010-study-20261020.md` |

## 2. Decisions Jeff makes before step 1

1. **Release artifact contents.** `scripts/Build-NimbyPack.ps1` packs every addon in `pack.json`; nothing selects a subset, and 9.3 puts only release-qualified PBOs in the Workshop item. Options: a build parameter that names the addons to pack (shared-file change, one session), or a release-only pack definition. Recommendation: the parameter, so every later single-mod release uses the same path.
2. **Boot instance.** `pack.json` `bootCheck.instanceId` is 999 (the fresh world). Step 9.2.1 boots against a copy of live `storage_1`, which `Sync-LiveReplica.ps1` places in `storage_998`. Options: a parameter for the instance, or editing `pack.json` for the gate run.
3. **Vanilla control (9.2.2).** The vanilla boot exists: `Start-NimbyTestServer.ps1 -ServerPath D:\DayZServer -InstanceId 998 -Vanilla` boots the same server build with no `-mod=` and keeps the RPT. The missing piece is the comparison: nothing yet sets the release boot's RPT counts beside the vanilla boot's (or `docs/baselines/`) and fails on an increase. Options: a comparison script, or a by-hand count table in the gate record.
4. **Hands permission (9.3).** The Hands permission capture is a release blocker for the Workshop item; with Hands held out of this release, confirm whether `PERMISSION.md` still blocks.
5. **Workshop publishing.** No script publishes; `scripts/Copy-WorkshopItem.ps1` copies a subscribed item into `vendor\`. Publishing is manual (DayZ Tools Publisher) on Jeff's Steam account.

## 3. Steps

| # | Step | Owner | State |
| --- | --- | --- | --- |
| 0 | Finish the replica tests first: Legs row G-1, the Tack mud-brick rows, the Hands diagnostic. `Sync-LiveReplica.ps1` overwrites the replica mission, which removes the test spawn, the test kit (`init.c`), and the Tack rows in `types.xml` | Jeff | Pending |
| 1 | Refresh the WinSCP mirror from live (download only; the mirror is never written to live), then run `Sync-LiveReplica.ps1`: live `storage_1` becomes `storage_998`, and the replica mission is restored from the mirror | Jeff | Ready |
| 2 | Build the release artifact (Legs, Ears, Scrawl, Greet only) with `-Qualification` (`storageAutoFix = 0`) and boot it on instance 998. RPT: 0 `Scripted variables corrupted`, 0 `SCRIPT (E)`, no new save-load errors. This step also runs Scrawl's A5 steps 1 and 2 (write one note, a stack of 5, a stack of 2, drop them, pin one if Tack is on the artifact, graceful stop, boot again, count the `written flag synced from=load written=true` lines), deferred here by Jeff on October 21, 2026; record them in the Scrawl test record, Release-1 round, A5 | Jeff runs; Claude reads the logs | Blocked on decisions 1 and 2 |
| 2a | Removal boot, for every content addon on the artifact (shared rules 6.1, last bullet; Scrawl today, Tack when it ships): boot the storage that step 2 saved with no `-mod=` (`Start-NimbyTestServer.ps1 -ServerPath D:\DayZServer -InstanceId 998 -Vanilla`, `storageAutoFix = 0`). RPT: 0 `Scripted variables corrupted`, 0 `SCRIPT (E)`, no VME at `Paper.c:17`; only the addon's own items missing. Record what Scrawl's stacks became and the `MorsecodeLeaflet_*` count before and after (test record, Release-1 round, A5). The same boot is Greet's G-10 (0 `Scripted variables corrupted`; Greet persists nothing in storage) | Jeff runs; Claude reads the logs | Blocked on decisions 1 and 2 |
| 3 | Vanilla control boot on the same build and storage copy; compare the save-load lines | Jeff runs; Claude compares | Blocked on decision 3 |
| 4 | Back up live mission storage and every live `$profile:Nimby*` folder together; keep the previous package | Jeff | Ready (at the release restart) |
| 5 | Live files at the release restart, each backed up first: upload `server/profile-overrides/NimbyGreet/greet.json` to the server profile as `$profile:NimbyGreet\greet.json`, after `scripts/Test-GreetText.ps1` passes on it; apply `types.scrawl.patch.xml` (Scrawl N3 lifetime, N4 loot). Plan G8 (amended October 26, 2026): the on-connect blocks in `server/mission-overrides/db/messages.nimby-mods.patch.xml` are not copied into `db/messages.xml`; Greet's Mods tab carries that text. If any NIMBY on-connect block is already in the live `db/messages.xml`, remove it; the hourly lines stay | Jeff | Ready once Jeff has reviewed `greet.json` |
| 6 | First release only: secrets history check, `LICENSE` (MIT) present, flip `CruftCommander/nimby-badlands-server` public | Jeff | Ready |
| 7 | Publish the `@NIMBY` Workshop item from the qualified artifact; description links the repository | Jeff | Blocked on decision 5 |
| 8 | Deploy at a scheduled restart (`docs/operations/restart-schedule.md`); the launch-line change lands in the restart window | Jeff | After 1 to 7 |
| 9 | After the restart: the Greet window opens at spawn with the uploaded text (server script log `[NIMBY][Greet] greet.json loaded source=greet.json version=1`), and no NIMBY on-connect chat line shows; the RPT shows each addon `loaded` and 0 `SCRIPT (E)`; document the client rollback (previous Workshop version) | Jeff runs; Claude reads the logs | After 8 |

## 4. Greet column (plan `docs/plans/2026-10-21-greet-plan.md`, tests G-0 to G-30)

Added October 26, 2026, when Greet Phase 1 closed (record `docs/testing/2026-10-26-greet-phase1.md`, Exit). Plan G8: Greet ships with Legs, Ears and Scrawl, and its Mods tab replaces their on-connect chat lines (steps 5 and 9). Evidence ids are the Phase 1 record's G-P1 rows; most ran on builds before `-153558`, the first build with the NIMBY-T-055 to T-058 fixes, so step 2's boot is the release artifact's G-0.

| Test | What | State | Evidence |
| --- | --- | --- | --- |
| G-0 | Self-test | PASS; repeats on the release artifact in step 2 | Build `-153558`: `Greet cases=113 pass=113` |
| G-1 | Fresh spawn opens the window | PASS | G-P1-01, G-P1-21 |
| G-2 | Dismissed: no window on relog | PASS | G-P1-08 |
| G-3, G-3b | `version` raised with and without `forceShow` | PASS | G-P1-16, G-P1-17 (G-P1-14 void) |
| G-4 | Unticked close: the window opens on every join | Covered, no row of its own | Four run 6 connections, every close `written=false` |
| G-5 | No second window after a respawn in one connection | PASS | G-P1-26 |
| G-5b | Checkbox starts unticked | Covered, no row of its own | Run 6 spawn screenshots |
| G-6, G-6b | Pause-menu button; Insert opens and closes | PASS | G-P1-06, G-P1-05, G-P1-12, G-P1-22, G-P1-43 |
| G-6c | Rebind the hotkey | PASS in Phase 0 | Phase 0 record |
| G-7 | Malformed `greet.json` (test server only) | PASS | G-P1-37 (boot, `.lkg`), G-P1-48 (one engine error, NIMBY-T-056 fixed) |
| G-8 | Waits behind another menu | Covered, no row of its own | Run 5 deferral and give-up; G-P1-43 |
| G-9 | Oversized text | PASS | G-P1-45 (NIMBY-T-055 fixed) |
| G-10 | Boot without Greet on storage that ran with it | Owed: runs in step 2a | - |
| G-11, G-12 | Discord Open and Copy | PASS | G-P1-04 |
| G-13, G-14, G-23 | Long body, the markdown sheet, tables | PASS | G-P1-03, G-P1-32 (1920x1080 and 2560x1440) |
| G-15 | German client, German text, selector | PASS on the old body format | G-P1-07, G-P1-10; not repeated on the line format (same code path as English, G-P1-44, G-P1-45) |
| G-15b | Czech client, English-only file | PASS | G-P1-39 |
| G-16, G-16b, G-19 | Media | Not applicable to release 1 | No media ships |
| G-17 | Keyboard only | PASS | G-P1-40 (navigation), G-P1-46 (Page keys, zeroing blocked) |
| G-17b | Controller | Owed; deferred by Jeff, October 26, 2026 | - |
| G-18 | Text size | PASS | G-P1-09, G-P1-32 |
| G-20 | Non-https link | PASS | G-P1-33 |
| G-21 | Chunk loss | PASS | G-P1-13, G-P1-23 |
| G-22 | Hit, unconscious, drift | PASS | G-P1-42 (death: G-P1-43) |
| G-24 | Rate limit | PASS | G-P1-24 |
| G-25 | Hotkey with the inventory open, in a vehicle, while dead | Not run: Jeff runs it or waives it before step 2 | - |
| G-26 | Untick clears the dismissal | PASS | G-P1-08 |
| G-27 | Player input blocked with the window open | PASS | G-P1-02 |
| G-28 | Scroll clamp | In part: Jeff runs the rest or waives it before step 2 | Run 6 screenshots stop cleanly at the end of a long body; past the top and a short body not checked |
| G-29 | One press, one action | PASS | G-P1-05, G-P1-12, G-P1-22 |
| G-30 | `openDelaySeconds` default and 3 s | PASS | G-P1-25, G-P1-28 |

Guardrails: no live-server change without Jeff's approval; never take the server offline outside a restart window; RCON stays on loopback; no secrets in the repository; no em dashes.
