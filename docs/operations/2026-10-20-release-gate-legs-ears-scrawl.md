# Release gate - Legs, Ears, Scrawl (first release)

Prepared October 20, 2026, after the two-client session (records committed at `1473a3b`). Shared rules 9.2 (release model) and 9.3 (distribution) govern; this checklist applies them to the first release. Release order and the release model are not reopened here.

## 1. Scope

| Mod | Addon | Gate status | Why |
| --- | --- | --- | --- |
| Legs | `NimbyAutorun` | Ready after row G-1 | Two-client rows PASS; NIMBY-T-005 fixed. Row G-1 (ladder, solo) runs before the gate; P2-5 (vehicle) and P2-10, P2-11 (dev harness) are waived for release 1 as known gaps (Jeff, October 20, 2026; `docs/testing/2026-10-19-legs-phase1.md`) |
| Ears | `NimbyEarplugs` | Ready | EP-002 PASS (`docs/testing/2026-10-16-ears-phase0.md`) |
| Scrawl | `NimbyNotes` | Ready | Second-player read and Phase 2a rows PASS; SC-2a-6 not provable in vanilla (`docs/testing/2026-10-16-scrawl-phase0.md`) |
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
| 2 | Build the release artifact (Legs, Ears, Scrawl only) with `-Qualification` (`storageAutoFix = 0`) and boot it on instance 998. RPT: 0 `Scripted variables corrupted`, 0 `SCRIPT (E)`, no new save-load errors | Jeff runs; Claude reads the logs | Blocked on decisions 1 and 2 |
| 3 | Vanilla control boot on the same build and storage copy; compare the save-load lines | Jeff runs; Claude compares | Blocked on decision 3 |
| 4 | Back up live mission storage and every live `$profile:Nimby*` folder together; keep the previous package | Jeff | Ready (at the release restart) |
| 5 | Live mission files at the release restart, each backed up first: copy the Legs, Ears, and Scrawl blocks from `server/mission-overrides/db/messages.nimby-mods.patch.xml` into `db/messages.xml` (lines updated October 20: autorun swims, the NIMBY Controls tab, Scrawl's write gesture and left mouse); apply `types.scrawl.patch.xml` (Scrawl N3 lifetime, N4 loot) | Jeff | Ready; Hands and Tack blocks stay out |
| 6 | First release only: secrets history check, `LICENSE` (MIT) present, flip `CruftCommander/nimby-badlands-server` public | Jeff | Ready |
| 7 | Publish the `@NIMBY` Workshop item from the qualified artifact; description links the repository | Jeff | Blocked on decision 5 |
| 8 | Deploy at a scheduled restart (`docs/operations/restart-schedule.md`); the launch-line change lands in the restart window | Jeff | After 1 to 7 |
| 9 | After the restart: the three on-connect lines show; the RPT shows each addon `loaded` and 0 `SCRIPT (E)`; document the client rollback (previous Workshop version) | Jeff runs; Claude reads the logs | After 8 |

Guardrails: no live-server change without Jeff's approval; never take the server offline outside a restart window; RCON stays on loopback; no secrets in the repository; no em dashes.
