# Release gate - Legs, Ears, Scrawl, Greet (first release)

Prepared October 20, 2026, after the two-client session (records committed at `1473a3b`). Shared rules 9.2 (release model) and 9.3 (distribution) govern; this checklist applies them to the first release. Release order and the release model are not reopened here.

## 1. Scope

| Mod | Addon | Gate status | Why |
| --- | --- | --- | --- |
| Legs | `NimbyAutorun` | Ready after row G-1 | Two-client rows PASS; NIMBY-T-005 fixed. Row G-1 (ladder, solo) runs before the gate; P2-5 (vehicle) and P2-10, P2-11 (dev harness) are waived for release 1 as known gaps (Jeff, October 20, 2026; `docs/testing/2026-10-19-legs-phase1.md`) |
| Ears | `NimbyEarplugs` | Ready | EP-002 PASS (`docs/testing/2026-10-16-ears-phase0.md`) |
| Scrawl | `NimbyNotes` | Ready | Second-player read and Phase 2a rows PASS; SC-2a-6 not provable in vanilla (`docs/testing/2026-10-16-scrawl-phase0.md`) |
| Greet | `NimbyGreet` | Ready after one fix build and the step 0 session (section 4) | Phase 1 closed October 26, 2026 (`docs/testing/2026-10-26-greet-phase1.md`); joins this release per Jeff's October 21, 2026 ruling (plan G2). Before step 5: Jeff reviews `server/profile-overrides/NimbyGreet/greet.json` (`version` 1, `forceShow` false) and `scripts/Test-GreetText.ps1` passes on it |
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
| 0 | Finish the replica tests first: Legs row G-1, the Tack mud-brick rows, the Hands diagnostic, and the Greet session (Jeff, October 26, 2026): the reviewed `greet.json` in the replica profile, every tab read at 1920x1080, then Greet G-25 and the rest of G-28 (section 4). `Sync-LiveReplica.ps1` overwrites the replica mission, which removes the test spawn, the test kit (`init.c`), and the Tack rows in `types.xml` | Jeff | Pending |
| 1 | Refresh the WinSCP mirror from live (download only; the mirror is never written to live), then run `Sync-LiveReplica.ps1`: live `storage_1` becomes `storage_998`, and the replica mission is restored from the mirror | Jeff | Ready |
| 2 | Build the release artifact (Legs, Ears, Scrawl, Greet only) with `-Qualification` (`storageAutoFix = 0`) and boot it on instance 998. RPT: 0 `Scripted variables corrupted`, 0 `SCRIPT (E)`, no new save-load errors. This step also runs Scrawl's A5 steps 1 and 2 (write one note, a stack of 5, a stack of 2, drop them, pin one if Tack is on the artifact, graceful stop, boot again, count the `written flag synced from=load written=true` lines), deferred here by Jeff on October 21, 2026; record them in the Scrawl test record, Release-1 round, A5 | Jeff runs; Claude reads the logs | Blocked on decisions 1 and 2 |
| 2a | Removal boot, for every content addon on the artifact (shared rules 6.1, last bullet; Scrawl today, Tack when it ships): boot the storage that step 2 saved with no `-mod=` (`Start-NimbyTestServer.ps1 -ServerPath D:\DayZServer -InstanceId 998 -Vanilla`, `storageAutoFix = 0`). RPT: 0 `Scripted variables corrupted`, 0 `SCRIPT (E)`, no VME at `Paper.c:17`; only the addon's own items missing. Record what Scrawl's stacks became and the `MorsecodeLeaflet_*` count before and after (test record, Release-1 round, A5). The same boot is Greet's G-10 (0 `Scripted variables corrupted`; Greet persists nothing in storage) | Jeff runs; Claude reads the logs | Blocked on decisions 1 and 2 |
| 3 | Vanilla control boot on the same build and storage copy; compare the save-load lines | Jeff runs; Claude compares | Blocked on decision 3 |
| 4 | Back up live mission storage and every live `$profile:Nimby*` folder together; keep the previous package | Jeff | Ready (at the release restart) |
| 5 | Live files at the release restart, each backed up first: upload `server/profile-overrides/NimbyGreet/greet.json` to the server profile as `$profile:NimbyGreet\greet.json`, after `scripts/Test-GreetText.ps1` passes on it; apply `types.scrawl.patch.xml` (Scrawl N3 lifetime, N4 loot). The upload is in place before the first boot with Greet (without it the server writes the built-in default text first). Use SFTP (WinSCP) to the HostHavoc server's profile folder (plan G12; the exact host path is not yet recorded in the repository). Plan G8: the per-mod blocks in `server/mission-overrides/db/messages.nimby-mods.patch.xml` (Legs, Hands, Ears, Scrawl, Tack) are not copied into `db/messages.xml`; Greet's Mods tab carries that text. If one of those per-mod blocks is already in the live `db/messages.xml`, remove it; the server's own on-connect lines (the welcome with `#name` and the location and wipe line) and the hourly lines stay | Jeff | Ready once Jeff has reviewed `greet.json` |
| 6 | First release only: secrets history check, `LICENSE` (MIT) present, flip `CruftCommander/nimby-badlands-server` public | Jeff | Ready |
| 7 | Publish the `@NIMBY` Workshop item from the qualified artifact; description links the repository | Jeff | Blocked on decision 5 |
| 8 | Deploy at a scheduled restart (`docs/operations/restart-schedule.md`); the launch-line change lands in the restart window | Jeff | After 1 to 7 |
| 9 | After the restart, on a client whose `%LOCALAPPDATA%\DayZ\NimbyGreet\seen.json` is deleted (a test client keeps its dismissal across servers): the Greet window opens at spawn with the uploaded text; the server script log has `[NIMBY][Greet] greet.json loaded source=greet.json version=1` with `truncations=0` and the expected `languages=`, and no Greet `invalid` line or engine `JSON ERROR`; none of the per-mod on-connect chat lines shows (the server's welcome and location lines still do); the RPT shows each addon `loaded` and 0 `SCRIPT (E)`; document the client rollback (previous Workshop version) | Jeff runs; Claude reads the logs | After 8 |

## 4. Greet column (plan `docs/plans/2026-10-21-greet-plan.md`, tests G-0 to G-30)

Added October 26, 2026, when Greet Phase 1 closed (record `docs/testing/2026-10-26-greet-phase1.md`, Exit). Plan G2 (Jeff, October 21, 2026): Greet ships with Legs, Ears and Scrawl. Plan G8: its Mods tab replaces their on-connect chat lines (steps 5 and 9). The ids below are Greet's (plan test table), not the Legs row G-1 in section 1. Evidence ids are the Phase 1 record's G-P1 rows. Only G-P1-44 to G-P1-48 ran on build `-153558`, the first with the NIMBY-T-055 to T-058 fixes; that build packed all six addons and was not a `-Qualification` build, so step 2 repeats G-0 on the release artifact. Before step 0, one fix build (Jeff, October 26, 2026: before release 1) carries the five self-review items listed in the plan's Phase 2, with G-7 and G-9 repeated on it.

| Test | What | State | Evidence |
| --- | --- | --- | --- |
| G-0 | Self-test | PASS; repeats on the release artifact in step 2 | Build `-153558`: `Greet cases=113 pass=113` |
| G-1 | Fresh spawn opens the window | PASS | G-P1-01, G-P1-21 |
| G-2 | Dismissed: no window on relog | PASS | G-P1-08 |
| G-3, G-3b | `version` raised with and without `forceShow` | PASS | G-P1-16, G-P1-17 (G-P1-14 void) |
| G-4 | Unticked close: the window opens on every join | Covered in substance, no row of its own | Four run 6 connections, every close `written=false`; `seen.json` was not deleted first as the plan asks |
| G-5 | No second window after a respawn in one connection | PASS | G-P1-26 |
| G-5b | Checkbox starts unticked | Covered, no row of its own | Run 6 spawn screenshots |
| G-6, G-6b | Pause-menu button; Insert opens and closes | PASS | G-P1-06, G-P1-05, G-P1-12, G-P1-22, G-P1-43 |
| G-6c | Rebind the hotkey | In part; waived for release 1 (Jeff, October 26, 2026) | G-P0-47 (Phase 0 probe): unbound, Insert did nothing; rebound to Insert, it opened. A different key was not tried |
| G-7 | Malformed `greet.json` (test server only) | PASS on a running server; boot in part, waived for release 1 (Jeff, October 26, 2026); repeated after the fix build | G-P1-48 (one engine error, NIMBY-T-056 fixed); G-P1-37 (boot serves the `.lkg`, but on the build before the fix, two engine errors); the boot was not repeated after the fix |
| G-8 | Waits behind another menu | In part; waived for release 1 (Jeff, October 26, 2026) | G-P1-15 (behind the pause menu), G-P1-43 (through the respawn menus). The connecting-dialogue scenario and a give-up at `openRetrySeconds` on current code were not run (the run 5 give-up was the NIMBY-T-051 defect) |
| G-9 | Oversized text | PASS; repeated after the fix build | G-P1-45 (NIMBY-T-055 fixed) |
| G-10 | Boot without Greet on storage that ran with it | Owed: runs in step 2a | - |
| G-11, G-12 | Discord Open and Copy | PASS | G-P1-04 |
| G-13 | Sixteen sections, a 4,000-character body, wheel and trigger scroll | PASS for the wheel; triggers owed with G-17b | G-P1-45 (16 tabs, Long body cut to 4,000), G-P1-46 (Page keys) |
| G-14 | The markdown sheet (every G13 construct, a typed `<b>`, a lone `<`, an unterminated `**`) | Not run in game; waived for release 1 (Jeff, October 26, 2026) | Converter self-test cases only; the real draft's constructs rendered in G-P1-03 and G-P1-32 |
| G-23 | Two-column table plus a three-cell row | In part; waived for release 1 (Jeff, October 26, 2026) | Tables at both resolutions: G-P1-03, G-P1-32; the three-cell row was not shown in game |
| G-15 | German client, German text, selector | PASS on the old body format | G-P1-07, G-P1-10; not repeated on the line format (same code path as English, G-P1-44, G-P1-45) |
| G-15b | Czech client, English-only file | PASS | G-P1-39 |
| G-16, G-16b, G-19 | Media | Not applicable to release 1 | No media ships |
| G-17 | Keyboard only | PASS | G-P1-40 (navigation), G-P1-46 (Page keys, zeroing blocked) |
| G-17b | Controller | Waived for release 1 (Jeff, October 26, 2026); a controller player opens Greet from the pause menu and closes it with B | - |
| G-18 | Text size | PASS | G-P1-09, G-P1-32 |
| G-20 | Non-https link | PASS | G-P1-33 |
| G-21 | Chunk loss | PASS | G-P1-13, G-P1-23 |
| G-22 | Hit, unconscious, drift | PASS; drift held 100 s and 52 s, not the two minutes asked | G-P1-42 (death: G-P1-43) |
| G-24 | Rate limit | PASS | G-P1-24 |
| G-25 | Hotkey with the inventory open, in a vehicle, while dead | Not run: runs in step 0 (Jeff, October 26, 2026) | - |
| G-26 | Untick clears the dismissal | PASS | G-P1-08 |
| G-27 | Player input blocked with the window open | PASS for movement, mouse and interaction | G-P1-02; F on a door or item, the inventory and voice keys, and the check after each kind of close were not recorded separately |
| G-28 | Scroll clamp | In part: the rest runs in step 0 (Jeff, October 26, 2026) | Run 6 screenshots stop cleanly at the end of a long body; past the top and a short body not checked |
| G-29 | One press, one action | PASS (ten taps, the plan says twenty) | G-P1-05, G-P1-12, G-P1-22 |
| G-30 | `openDelaySeconds` default and 3 s | PASS | G-P1-25, G-P1-28 |

Guardrails: no live-server change without Jeff's approval; never take the server offline outside a restart window; RCON stays on loopback; no secrets in the repository; no em dashes.
