# NIMBY client pack

One signed client-and-server mod, `@NIMBY`, that carries every small mod the server runs, so players download one pack and the server controls each version.

## What this is not

- Not a public Workshop release. The pack exists for this server; if it is ever published, every third-party piece keeps its original attribution and license terms.
- Not a place for secrets. The signing private key (`.biprivatekey`) never enters the repository.

## Contents

| Addon | Origin | License | Status |
| --- | --- | --- | --- |
| `NimbyAutorun` | Fork of [SchanaModAutorun](https://github.com/schana/dayz-mod-autorun) (commit 1048f71) | Apache-2.0; see `NimbyAutorun/LICENSE` and `NimbyAutorun/NOTICE` | Source forked October 14, 2026; server-side RPC validation added; per-frame RPC fixed (review CR-004). Not yet built or tested in game |
| `NimbyGiveTake` | Fork of Give&Take, Workshop [2903112334](https://steamcommunity.com/sharedfiles/filedetails/?id=2903112334) | Author permits repacking, modification, and redistribution (Workshop description; re-confirm before release) | Imported October 14, 2026; review fixes CR-001, CR-003, CR-005, CR-006 applied (see `NimbyGiveTake/NOTICE`). Boot-checked; two-client test pending |
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

1. Packs each addon into an uncompressed PBO with the pack's own packer (`scripts/modules/NimbyPbo.psm1`) and signs it with DayZ Tools `DSSignFile` using the key in `D:\Keys\nimby` (created on first run; the private key never leaves that folder). DayZ Tools Addon Builder is not used: its DayZ build ignores command-line arguments and opens its window.
2. Verifies each PBO: checksum, signature present, prefix matches `pack.json`, and every source file matching `includePatterns` is inside.
3. Boot check: starts the local DayZ Server with the built mod on port 2402, instance 999, and fails on any script error in the RPT or script log.

Output goes to `build/@NIMBY-<timestamp>/` and logs to `logs/build-<timestamp>/build.log`; both folders are git-ignored. Exit code 0 is PASS, 1 is FAIL, 2 is UNKNOWN (built and verified, boot check skipped or inconclusive). Add an addon by adding a row to `pack.json`; the script needs no change.

## Test plan

Each addon passes these before the pack ships, on a local DayZ server first, then on the private HostHavoc instance. For the local run, `scripts/Start-NimbyTestServer.ps1` starts the server with the newest build, installs the key, and summarizes every `[NIMBY]` log line when you press Enter:

1. The server boots with `@NIMBY` and the RPT has zero `SCRIPT (E)` and zero `Can't compile` lines naming a Nimby addon.
2. A client joins with signature verification on and is not kicked.
3. Autorun: numpad + starts and stops autorun; W, S, A, D, fire, raise, and lean each stop it; stamina drains and speed drops from sprint to run at zero stamina; getting into a vehicle stops it. Review CR-008: get knocked unconscious (or restrained, or into a vehicle) while autorunning, then recover - the player must stand still, not resume running.
4. Autorun security: a second client cannot start or stop the first client's autorun. The server log shows a `[NIMBY][Autorun] Rejected request` line when this is attempted.
5. Give n Take, with two clients (two Steam accounts) on the local server:
   1. Offer (hold O) and Take: the item moves from A's hands to B's inventory. Server script log shows `REQUESTED` then `OK`.
   2. Request (hold O, empty hands) and Give: the item moves from A's hands to B's inventory, not B's hands.
   3. Shift+O on B, then A puts an item in B's pockets. Shift+O again closes them; the action disappears.
   4. Give or Take when the receiver's inventory is full: no prompt, or a `REFUSED no space` line.
   5. Two takers on one offer at the same moment: one `OK`, one `REFUSED`; the item exists once.
   6. Repeat 1 and 2 ten times each, then drop and re-pick every received item on both clients. Any item that cannot be dropped or that shows in two places is a failure.
   7. Repeat 6 with 200 ms of added latency on one client (for example with clumsy on Windows).
