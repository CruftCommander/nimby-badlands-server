# Central Scrutinizer Phase 0 - test record

Test record for [Plan 3](../plans/2026-10-15-nimby-admin-plan.md) v0.8, Phase 0 (Stop B, the vertical slice). Seat: Claude Code in worktree `D:\GitHub\nimby-badlands-server-wt\nimby-admin`, branch `feat/nimby-admin`. Client steps: Jeff and a second client. Baseline: `docs/baselines/2026-10-31-central-scrutinizer-phase0-baseline.md`. Status: build and static checks green; the two-client session has not run yet.

## Build under test

| Item | Value |
| --- | --- |
| Build | `build\@NIMBY-20261031-231332` plus `build\@NIMBY-Server-20261031-231332` and `build\@NIMBY-Dev-20261031-231332` (`Build-NimbyPack.ps1 -ServerPath D:\DayZServer -BootInstanceId 998 -Dev -Qualification`) |
| Verdict | `SUMMARY PASS`; `BOOT MODE qualification storageAutoFix=0` on the replica `storage_998`; `SERVERMOD` line present (the first build with a server-only output); `SELFTEST Admin ran=yes cases=54 pass=54 fail=0`, `SELFTEST AdminServer ran=yes cases=64 pass=64 fail=0`, `SELFTEST DevHarness ran=yes cases=16 pass=16 fail=0`, `SELFTEST TOTAL pass=779 fail=0 flags=10 expected=10` |
| Static checks | `scripts\Test-NimbyRepo.ps1`: `PASS: 0 failure(s)` (`logs\static-20261031-231448`), after the CRLF fix in `4cc7f54` |
| Note | `-231332` was packed from the same sources before the line-ending normalization of `4cc7f54`. A rebuild from `4cc7f54` names the build the two-client session runs; it waits for the port (below) |
| Boot lines | `[NIMBY][Admin] config.json missing; defaults written (no bootstrap owner, webhook unset)`, then `[NIMBY][Admin] server ready; permissions source=none entries=0 bootstrap=false editsRefused=false; bans source=none entries=0; webhook=unset requestsPerSecond=5 permissionsReloadSeconds=5` |

## Session setup (prepared, not run)

- Profile `profiles\nimby-admin-p0\` (git-ignored): `NimbyAdmin\config.json` from `server\profile-overrides\NimbyAdmin\config.example.json` with `bootstrapOwnerSteam64` `76561198354705597` (Jeff) and an empty `webhookUrl`, which Jeff fills with the throwaway webhook himself; `NimbyAdmin\debug`; `NimbyShared\debug`; `NimbyDevHarness\forge` with `admin`, `admin-owner=76561198354705597`, `sandstorm`.
- Clients: `NimbyAdmin\debug` in each client profile so the client lines (`role received`, `hotkey ignored: no role`, `open source=`) are written. Both clients load the pack folder and the dev harness folder.
- Server, from a console Jeff keeps open (a background shell closes the script's input and stops it):

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Start-NimbyTestServer.ps1 -ServerPath D:\DayZServer -InstanceId 998 -BuildPath <absolute build> -ServerModPath <absolute server build> -DevPath <absolute dev build> -ProfileName nimby-admin-p0
```

## Gate table (Plan 3 Phase 0)

| Scenario | Required result | Evidence | Status |
| --- | --- | --- | --- |
| Non-admin presses the menu key | Nothing opens; no request sent | Second client: `[NIMBY][Admin][Debug] hotkey ignored: no role`; no server `request` line from that press | NOT RUN |
| Non-admin forges a teleport (dev harness) | Denied | `[NIMBY][DevHarness][Forge] PASS admin: non-admin forges a teleport` (audit `result=denied reason=no role`, position unchanged) | NOT RUN |
| Admin teleports to a player | Moves | Audit `intent` then `result=ok` with the same `id`; `[NIMBY][Admin] teleport done ...`; one Discord message | NOT RUN |
| Duplicate request id | Second ignored | `[NIMBY][DevHarness][Forge] PASS admin: duplicate request id runs once`; `[NIMBY][Admin] duplicate id=` line | NOT RUN |
| Permission revoked mid-session | Next request denied | Edit `permissions.json`; within `permissionsReloadSeconds` the next request gives `result=denied reason=no role`, a `role sync ... reason=reload` push, and the open menu closes | NOT RUN |
| Malformed payload | Rejected | Six `[NIMBY][DevHarness][Forge] PASS admin:` malformed cases, each one `result=invalid` audit record with its reason | NOT RUN |
| Disconnect and reconnect | Role re-synced; menu state correct | `[NIMBY][Admin] role sync ... reason=hello` with a new `session=` number at each connect | NOT RUN |
| Restart | Permissions and config persist; no entity data written | `permissions.json`, `config.json` unchanged across an RCON `#shutdown` restart; the `storage_998` file set unchanged; neither PBO defines `OnStoreSave` or `OnStoreLoad` | NOT RUN |
| Audit file locked or read-only (local) | Command blocked | Today's audit file set read-only, then a teleport: `result=failed reason=audit unavailable` to the client, `[NIMBY][Admin] audit unavailable: cannot open ...`, no position change | NOT RUN |
| Webhook wrong or Discord down | Command succeeds | `webhookUrl` pointing at a deleted webhook: teleport `result=ok`; one `[NIMBY][Admin] discord failed class=client queued=<n>` line, not one per retry | NOT RUN |
| Self-test | Validators pass | Build lines above: every `[NIMBY][Admin][SelfTest]` and `[NIMBY][AdminServer][SelfTest]` case PASS | PASS (`-231332`) |
| Security contract review (N18) | No unaddressed finding | Codex read-only review of the router, permissions store, ban store, audit journal; each finding and its disposition | NOT RUN |

## Phase 0 item 3 proofs

| # | Proof | Evidence | Status |
| --- | --- | --- | --- |
| 1 | `GetPlainId()` is the Steam64 on 164200 | `[NIMBY][Admin] role sync player=(<bi-id>, 76561198354705597) "..." role=owner` | NOT RUN |
| 2 | `RestApi` POST from the server | `[NIMBY][Admin][Debug] discord delivered seq=<n> bytes=<n>` and the message in the throwaway channel | NOT RUN |
| 3 | Position set and surface check | Audit `effects.from`, `effects.to`, `effects.offset`; both clients see the admin beside the target | NOT RUN |
| 4 | Sandstorm `Start` and `Stop` on a dedicated server | `[NIMBY][DevHarness][Forge] PASS sandstorm: Start=true active=true`, then `PASS sandstorm: stopped active=false`; Jeff sees it | NOT RUN |
| 5 | Home is free and rebinds | 0 `kHome` uses in the four 164200 presets (`bin/constants.xml:578`, checked October 31, 2026); in game: Home opens for the owner, Options > Controls lists "Central Scrutinizer: open menu" under NIMBY, a rebind moves it | Static half PASS; in-game NOT RUN |

## Deviations from the recon, as built

- Stale requests: the recon's "id below the lowest id of a full window" rule was dropped. The harness's forged ids (100,000,000 and up) and the client's own ids (from 1) share one window, and that rule would have refused the client after a harness run. Built instead: duplicates are refused for 300 s per sender per connection (at most 256 ids kept), a protocol other than 1 is refused, and a selection that outlived the target's connection is refused by its session number. The recon's question list did not cover this; it is recorded here for the N18 review.
- Denied and invalid audit records: bounded per sender to 30 a minute (a token bucket), not one per 10 s, so the harness's six malformed cases each leave their record. Log lines stay gated at one per 10 s per sender.
- `players.list` answers through the codec only and writes no audit record (a query, not a state change); a denied `players.list` writes its `result=denied` record like any other.
