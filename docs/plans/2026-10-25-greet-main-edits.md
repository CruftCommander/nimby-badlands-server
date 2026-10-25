# Greet: edits that land on `main` through Jeff

October 25, 2026. Shared rules section 3 reserves `pack.json` and the shared-rules file for the orchestrator, and section 2.2 reserves `main` for Jeff. This note is the exact text of the two edits the `feat/greet` session needs on `main` before Phase 0 can build, plus the stub commit to cherry-pick. Delete this file when both have landed.

## 1. Cherry-pick the stub

Commit `greet: NimbyGreet stub` on `feat/greet` adds four files under `mods/own/nimby-pack/NimbyGreet/`: `config.cpp`, `NOTICE`, `Scripts/3_Game/nimbygreet_rules.c` (constants plus nine self-test cases), and `Scripts/5_Mission/nimbygreet_missionbase.c` (the self-test hook). The boot check creates `$profile:NimbyGreet\selftest` for every `pack.json` row and fails a silent addon, so the stub carries a real self-test from day one.

```powershell
git -C D:\GitHub\nimby-badlands-server cherry-pick <stub-commit-sha>
```

## 2. `mods/own/nimby-pack/pack.json`

Add the row after `NimbyTack`:

```json
        {
            "folder": "NimbyGreet",
            "pboName": "NimbyGreet",
            "prefix": "NimbyGreet",
            "logPrefix": "Greet"
        }
```

Add to `includePatterns`, after `"*.layout"`, so Phase 0 media is inside the completeness check (plan G15; `*.csv` and `*.json` are already there for the stringtable and the media manifest):

```json
        "*.mp4",
        "*.edds",
        "*.paa",
```

## 3. `docs/plans/2026-10-15-shared-rules.md`, amendment v1.4

Header: append `v1.4 amendment (October 25, 2026: Greet rows in 2.1, 4.1, and 5; no rule changed)`.

Section 2.1 table, new row:

```
| Greet | `feat/greet` | `D:\GitHub\nimby-badlands-server-wt\greet` |
```

Section 4.1 table, new row before Menu:

```
| Greet | Greet | `NimbyGreet` | `[NIMBY][Greet]` | `65511xx` (`6551101` request, client to server; `6551102` chunk and `6551103` done, server to the owning client; plan G5) |
```

Section 5 table, new row:

```
| Greet open window | Insert (`kInsert`); `bin/constants.xml:586`; 0 uses in `bin/preset_X1MouseKey.xml`, so a plain press is free. `kF12` is free in game but is Steam's screenshot key and is excluded | Decided by Jeff (G7, Q2), October 25, 2026 |
```

Section 6, the utility-addon sentence ("The rules above apply to utility addons: Legs, Hands, Central Scrutinizer, Buzz, and Ears"): add Greet. Section 8.1, the single-PBO sentence ("Legs, Hands, Puff, Buzz, Sprout, Scrawl, Tack, and Ears stay single-PBO"): add Greet.

Section 2.1 note: the fleet convention is `...-wt\feat-<name>`; the Greet worktree was created as `...-wt\greet` on October 25, 2026, matching the newer `ears-r1`, `scrawl-r1`, and `tack-placement` folders rather than the table, so the row records the folder that exists.

Section 3, known `pack.json` edits: append `the NimbyGreet row and the three media include patterns at Greet Phase 0`.

## 4. Open questions the plan asks Jeff

In `docs/plans/2026-10-21-greet-plan.md`, section "Open questions for Jeff". Q1 to Q4 were decided by Jeff on October 25, 2026 and are recorded there. Q5 is this file. Q6 (whether the checkbox in a manually opened window writes `seen.json`) is open and does not block Phase 0.
