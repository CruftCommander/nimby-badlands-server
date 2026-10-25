# Greet: edits that land on `main` through Jeff

October 25, 2026. Shared rules section 3 reserves `pack.json` and the shared-rules file for the orchestrator, and section 2.2 reserves `main` for Jeff. This note is the exact text of the two edits the `feat/greet` session needs on `main` before Phase 0 can build, plus the stub commit to cherry-pick. Delete this file when both have landed.

## 1. Cherry-pick the stub

Commit `greet: NimbyGreet stub` on `feat/greet` adds four files under `mods/own/nimby-pack/NimbyGreet/`: `config.cpp`, `NOTICE`, `Scripts/3_Game/nimbygreet_rules.c` (constants plus eight self-test cases), and `Scripts/5_Mission/nimbygreet_missionbase.c` (the self-test hook). The boot check creates `$profile:NimbyGreet\selftest` for every `pack.json` row and fails a silent addon, so the stub carries a real self-test from day one.

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
| Greet open window | Insert (`kInsert`); `bin/constants.xml:586`; 0 uses in `bin/preset_X1MouseKey.xml`, so a plain press is free. `kF12` is free in game but is Steam's screenshot key and is excluded | Claude default (G7), pending Jeff (plan Q2), October 25, 2026 |
```

Section 3, known `pack.json` edits: append `the NimbyGreet row and the three media include patterns at Greet Phase 0`.

## 4. Open questions the plan asks Jeff (Q1 to Q5)

In `docs/plans/2026-10-21-greet-plan.md`, section "Open questions for Jeff". Phase 0 does not wait on Q1, Q3, or Q4; Q2 only changes the default key in `Data/Inputs.xml`; Q5 is this file.
