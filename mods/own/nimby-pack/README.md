# NIMBY client pack

One signed client-and-server mod, `@NIMBY`, that carries every small mod the server runs, so players download one pack and the server controls each version.

## What this is not

- Not a public Workshop release. The pack exists for this server; if it is ever published, every third-party piece keeps its original attribution and license terms.
- Not a place for secrets. The signing private key (`.biprivatekey`) never enters the repository.

## Contents

| Addon | Origin | License | Status |
| --- | --- | --- | --- |
| `NimbyAutorun` | Fork of [SchanaModAutorun](https://github.com/schana/dayz-mod-autorun) (commit 1048f71) | Apache-2.0; see `NimbyAutorun/LICENSE` and `NimbyAutorun/NOTICE` | Source forked October 14, 2026; server-side RPC validation added; per-frame RPC fixed (review CR-004). Not yet built or tested in game |
| `NimbyGiveTake` | Fork of Give&Take, Workshop [2903112334](https://steamcommunity.com/sharedfiles/filedetails/?id=2903112334) | Author permits repacking, modification, and redistribution (Workshop description; re-confirm before release) | Imported verbatim October 14, 2026 (see `NimbyGiveTake/NOTICE`). Review fixes CR-001, CR-003, CR-005, CR-006 pending |
| Zens Core, Zens Zippo Lighter, Zens Music | Repack of Zenarchist's Workshop items | Author permits repacking; Zippo model is CC BY-NC | Theme drop 1; not started |

## Layout when built

```
@NIMBY/
  mod.cpp
  addons/
    NimbyAutorun.pbo
    NimbyAutorun.pbo.nimby.bisign
    NimbyGiveTake.pbo
    NimbyGiveTake.pbo.nimby.bisign
  keys/
    nimby.bikey
```

## Build and sign

`scripts/Build-NimbyPack.ps1` builds the pack from `pack.json`:

1. Packs each addon with DayZ Tools Addon Builder and signs it with the key in `D:\Keys\nimby` (created on first run; the private key never leaves that folder).
2. Verifies each PBO: signature present, prefix matches `pack.json`, and every source file matching `includePatterns` is inside.
3. Boot check: starts the local DayZ Server with the built mod on port 2402, instance 999, and fails on any script error in the RPT or script log.

Output goes to `build/@NIMBY-<timestamp>/` and logs to `logs/build-<timestamp>/build.log`; both folders are git-ignored. Exit code 0 is PASS, 1 is FAIL, 2 is UNKNOWN (built and verified, boot check skipped or inconclusive). Add an addon by adding a row to `pack.json`; the script needs no change.

## Test plan

Each addon passes these before the pack ships, on a local DayZ server first, then on the private HostHavoc instance:

1. The server boots with `@NIMBY` and the RPT has zero `SCRIPT (E)` and zero `Can't compile` lines naming a Nimby addon.
2. A client joins with signature verification on and is not kicked.
3. Autorun: the toggle starts and stops autorun; each interrupt input stops it; stamina drains and speed drops from sprint to run at zero stamina; getting into a vehicle stops it.
4. Autorun security: a second client cannot start or stop the first client's autorun. The server log shows a `[NIMBY][Autorun] Rejected request` line when this is attempted.
5. Give n Take: offering, requesting, and pocket access work between two clients; nothing duplicates when either client disconnects mid-transfer.
