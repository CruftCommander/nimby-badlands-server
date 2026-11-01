<!-- Copied verbatim on November 1, 2026 from D:\GitHub\nimby-badlands-server-wt\codex-rereview-2-nimby-admin-phase0.md (the Codex read-only third pass over git diff 439b729..a4c3a49). Every path:line below refers to head a4c3a49; the two fixes and their dispositions are in docs/testing/2026-10-31-central-scrutinizer-phase0.md, section Third-pass findings and dispositions. -->

Head: `a4c3a49`. Reviewed `git diff 439b729..HEAD` against the specified contract and reviews. **12 CLOSED, 1 PARTIAL. No remaining critical or high finding identified. One new medium finding in the dev harness.**

Paths:

- `C/` = `mods/own/nimby-pack/NimbyAdmin/Scripts/`
- `S/` = `mods/own/nimby-pack/NimbyAdminServer/Scripts/`
- `H/` = `mods/own/dev/NimbyDevHarness/scripts/`

| # | Status | Evidence in new code and what remains |
| --- | --- | --- |
| 1 | **CLOSED** | `S/3_Game/nimbyadminserver_rules.c:914`: `int at = LowestIndex();` evicts the lowest ID; `:922` recomputes the floor. `:892` rejects retained IDs as duplicates; `:896` rejects IDs below the full window's floor as stale. The floor cannot decrease. After accepting 1000 then 1 through 256, 1000 remains a duplicate and 1 becomes stale. No time expiry remains. |
| 2 | **CLOSED** | `C/5_Mission/nimbyadmin_menu.c:266`: `selectedSession = before.session;`; `:270` calls `Rebind`. `C/3_Game/nimbyadmin_rules.c:530` matches BI ID and `:532` requires `row.session == session`. Reconnection clears the selection. |
| 3 | **PARTIAL** | Missing/unreadable primaries now reach recovery at `S/3_Game/nimbyadminserver_files.c:194`. Successful loads and `.lkg` recovery set `m_EverLoaded` at `:228` and `:265`, fixing the previous disappearing-backup scenario. **Medium, CONFIRMED remains:** start with an invalid or unreadable primary, no usable `.lkg`, and no previous successful load. Recovery refuses edits at `:272`, but never sets `m_EverLoaded`. Delete the primary; the next load passes the first-install condition at `:177` and executes `m_EditsRefused = false;` at `:180`, without any valid repair. `IsFirstInstall()` at `:287` also returns true. Track any prior recovery/failure, including source `NONE`, and exclude it from first-install handling. |
| 4 | **CLOSED** | `S/3_Game/nimbyadminserver_audit.c:181` enables readback for intents. `:233`: `if (!ReadLastLine(path, last) || !IsLastRecord(last, json))` rejects failed verification. The caller blocks bootstrap writing at `S/4_World/nimbyadminserver_core.c:139` and teleport at `:604`. Crash durability remains unproved. |
| 5 | **CLOSED** | `S/3_Game/nimbyadminserver_rules.c:247` rejects an exhausted sender before `aggregate.Take(...)` at `:251`. The router uses this helper at `S/4_World/nimbyadminserver_core.c:261`. |
| 6 | **CLOSED** | `S/4_World/nimbyadminserver_core.c:473` checks `ShouldLogRoleSync`; `:481` logs only a nonempty line. `S/3_Game/nimbyadminserver_rules.c:296` returns `""` for an unchanged role. The repeated-hello debug line is gone; the role reply still sends at core `:480`. |
| 7 | **CLOSED** | Duplicate and rejection logs use `CommandForLog` at `S/4_World/nimbyadminserver_core.c:297` and `:355`. Unknown command text becomes its length at `S/3_Game/nimbyadminserver_rules.c:197`. `C/3_Game/nimbyadmin_log.c:58` sanitizes both identities and the name. |
| 8 | **CLOSED** | `S/3_Game/nimbyadminserver_rules.c:655` and `:738` initialize `version = 0`; builders reject wrong versions and null collections. `S/3_Game/nimbyadminserver_files.c:340` and `:433` require the respective top-level collection key before adoption. |
| 9 | **CLOSED** | The exact-good-text branch at `S/3_Game/nimbyadminserver_files.c:197` restores file source at `:200`, clears edit refusal at `:201`, and clears bad-text state at `:202` and `:203`. |
| 10 | **CLOSED** | `S/3_Game/nimbyadminserver_rules.c:367`: `if (day > DaysInMonth(year, month))`. Month lengths and Gregorian leap-year rules are implemented at `:379` and `:392`. |
| 11 | **CLOSED** | `C/3_Game/nimbyadmin_rules.c:316`: `if (ch == "=")` rejects a raw equals sign. The `%3D` decoding branch at `:341` remains. |
| 12 | **CLOSED** | `S/4_World/nimbyadminserver_core.c:400` captures the append result and passes it to `Finish` at `:402`. `S/3_Game/nimbyadminserver_rules.c:1021` clears counts only when `appended` is true; `:1026` schedules failure retry 60 seconds later. The deferred message at `:1038` accurately says counts were retained. This closes the explicitly reported append-failure path; non-intent writes are still not read back. |
| 13 | **CLOSED** | `S/3_Game/nimbyadminserver_audit.c:146` routes every intent to the command stream, before considering `record.denied` at `:150`. Rejections set `record.denied = true;` at `S/4_World/nimbyadminserver_core.c:363`; suppression summaries do likewise at `:397`. Their records cannot lengthen the command-file readback. |

The harness uses highest plus **1,000,000** at `H/4_world/nimbydevharness_admin.c:211`, cached per identity per step. Its pre-send guard at `:143` caps the entire run at **127 forge calls**, including rejected calls and duplicates. The scheduling guard at `:126` and `:134` permits one automatic run per server process. Thus harness traffic occupies at most 127 of the 256 replay slots, leaving at least 129 client IDs when the set is full. The numerical range issue below is separate from the corrected eviction rule.

For the two-file layout, **failure to append or verify an intent in `audit-<date>.log` blocks the state change**. Failure of `audit-denied-<date>.log` alone does **not** block commands. Results in either file are not read back; a failed command-result append occurs after execution and cannot undo it. Failure flags are separate at `S/3_Game/nimbyadminserver_audit.c:124` and `:125`.

New finding, using the previous severity scale:

| # | Severity | Confidence | Location | Issue | Failure scenario | Smallest fix |
| --- | --- | --- | --- | --- | --- | --- |
| 14 | medium | CONFIRMED | `H/4_world/nimbydevharness_admin.c:211`: `base = NimbyAdminHighWater.Get(biId) + FORGE_OFFSET;`; `:214`: `return base + n;` | Harness ID arithmetic has no upper-bound guard. The 127-call cap bounds quantity, not numeric range. Dev-harness impact only. | Before the armed harness runs, a connected client sends a valid hello with request ID `2147483647`. The envelope accepts positive IDs (`C/3_Game/nimbyadmin_rules.c:102`), and the server publishes the highest at `S/4_World/nimbyadminserver_core.c:306`. Adding 1,000,000 exceeds the integer range, so the harness cannot generate its promised larger positive IDs and its cases no longer exercise their intended paths. Production replay rejection remains intact. | Before addition, require enough integer headroom for the offset and the step's largest `n`; otherwise abort that run with an explicit SKIP reason. |

Only `git diff` and `git log` were run. Nothing was changed. No builds, tests, server sessions, or failure experiments were run; recorded PASS results were not independently verified.