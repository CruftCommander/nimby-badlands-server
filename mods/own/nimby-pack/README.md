# NIMBY client pack

One signed client-and-server mod, `@NIMBY`, that carries every small mod the server runs, so players download one pack and the server controls each version.

## What this is not

- Not a public Workshop release. The pack exists for this server; if it is ever published, every third-party piece keeps its original attribution and license terms.
- Not a place for secrets. The signing private key (`.biprivatekey`) never enters the repository.

## Contents

| Addon | Origin | License | Status |
| --- | --- | --- | --- |
| `NimbyAutorun` | Fork of [SchanaModAutorun](https://github.com/schana/dayz-mod-autorun) (commit 1048f71) | Apache-2.0; see `NimbyAutorun/LICENSE` and `NimbyAutorun/NOTICE` | Source forked October 14, 2026; server-side RPC validation added. Not yet built or tested |
| `NimbyGiveTake` | Fork of Give&Take, Workshop [2903112334](https://steamcommunity.com/sharedfiles/filedetails/?id=2903112334) | Author permits repacking, modification, and redistribution (Workshop description; re-confirm before release) | Waiting on the Workshop files; see `scripts/Copy-WorkshopItem.ps1` |
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

The build script is not written yet. It will pack each addon folder with the DayZ Tools Addon Builder, sign each PBO with Jeff's private key, write a log to `logs/`, and run on Windows PowerShell 5.1 and PowerShell 7.

## Test plan

Each addon passes these before the pack ships, on a local DayZ server first, then on the private HostHavoc instance:

1. The server boots with `@NIMBY` and the RPT has zero `SCRIPT (E)` and zero `Can't compile` lines naming a Nimby addon.
2. A client joins with signature verification on and is not kicked.
3. Autorun: the toggle starts and stops autorun; each interrupt input stops it; stamina drains and speed drops from sprint to run at zero stamina; getting into a vehicle stops it.
4. Autorun security: a second client cannot start or stop the first client's autorun. The server log shows a `[NIMBY][Autorun] Rejected request` line when this is attempted.
5. Give n Take: offering, requesting, and pocket access work between two clients; nothing duplicates when either client disconnects mid-transfer.
