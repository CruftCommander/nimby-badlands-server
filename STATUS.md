# Status

Updated October 27, 2026. For how things got here, read the git log and `docs/operations/`;
the decision record is the Notion hub "DayZ Badlands Server - Project Hub".

## Live

- **Server:** live since October 26, 2026, 23:57:31 Central. HostHavoc Chicago, service
  246248, `104.192.227.46:2302`, 40 slots, 1PP hardcore, Nasdara. Restarts every 12 hours
  (12:00 AM and 12:00 PM Central; the panel clock is 2 hours behind Central). Backups:
  panel `manual-backup-2026-10-26_21-03.zip` and mirror
  `D:\DayZServer\_backups\live-pre-release1-20261026-2318.zip` (pre-release-1 rollback set).
- **Release 1 (October 26, 2026):** the six Phase 0 addons - Legs (`NimbyAutorun`), Hands,
  Ears (`NimbyEarplugs`), Scrawl (`NimbyNotes`), Tack, Greet - ship as one signed pack,
  `@NIMBY`, Workshop item 3828109139 (public), key `nimby.bikey`. Launch line
  `"-servermod=" "-mod=@NIMBY"`. Release artifact `@NIMBY-20261026-214828` (480 self-test
  cases). Gate record: `docs/operations/2026-10-20-release-gate-legs-ears-scrawl.md` (steps
  1 to 9 done with evidence, plus the post-release restart check). Test record:
  `docs/testing/2026-10-26-six-addon-session.md` (S6-01 to S6-25; Jeff declared the session
  a pass on green logs; Scrawl S6-08 to 11, Tack S6-12/14/16/18/19 and Hands S6-20 to 23 not
  run).
- **Loot update (applied October 26, 2026, 23:47):** `docs/operations/2026-10-26-release-1-loot-update.md`;
  22 rows in `server/mission-overrides/LOOT-CHANGELOG.md`, patches under
  `server/mission-overrides/`. Standing rule: every loot change is logged there with who,
  before, after, when and why.
- **Greet text:** `greet.json` version 1, 990-character body, six mods listed plus "Coming
  later: the admin tool".
- **First-week watch:** `docs/operations/2026-10-27-release-1-first-week-watch.md`. Pull 1
  (October 27, 10:28): four release boots clean, server idle since launch.
- **Repository:** public (`CruftCommander/nimby-badlands-server`, MIT). `main` is the
  release line; every feature branch is merged and pushed. Hosted static gate on every push
  to `main` and every pull request.

## Known gaps on live

| Item | State | Detail |
| --- | --- | --- |
| Hands NIMBY-T-010 | Open | Two-client transfer rows never ran; Hands accepted on the solo pass. The first live report of a failed hand-over goes here |
| Greet NIMBY-T-059 | Open, not release-blocking | The client renders about 1,100 characters of a long body; a diagnostic build is owed. The release text is under the cap |
| Scrawl on live | Not run | No in-game write and restart read yet. Pens spawn on live now |
| `server/live/` reference copies | Stale | Still show pre-release-1 values until the next WinSCP pull plus `scripts\Sync-LiveReplica.ps1` |
| Discord invite | Done October 28, 2026 | Jeff ruled `discord.gg/kH3eJ5mdEv` (never expires; `bQDNepCRF` expires November 14, 2026). The panel hostname was changed by Claude from the panel (stop with 0 players, one-line edit of `serverDZ_By_HostHavoc.cfg`, start at 14:08 Central); the DZSA launcher shows the new name; Greet and `messages.xml` already used it |
| Halloween overlay | Declined (recommended), not ruled | Bohemia ships no Nasdara Halloween mission; a hand-made overlay would be about 25 rows |

## Not yet built

- **NimbyKit** (test-item kit): brief at `docs/plans/2026-10-26-testkit-brief.md`; a separate
  server-only addon, live only in announced test windows. Needs an identifiers row in shared
  rules 4.1 first (orchestrator-owned).
- **Central Scrutinizer** (admin tool): takes the next build slot after the kit; no other
  Phase 0 runs in parallel with it.
- **Tweaker** (vanilla tweaks), **Beards** (model project): on the mod board, not scheduled.
