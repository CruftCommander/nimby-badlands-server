# NIMBY client pack

One signed client-and-server mod, `@NIMBY`, that carries every small mod the server runs, so players download one pack and the server controls each version.

## What this is not

- Not a private download. Decided October 15, 2026: `@NIMBY` is distributed as one public Steam Workshop item, so every third-party piece keeps its original attribution and license terms inside the pack and in its `NOTICE`.
- Not a place for secrets. The signing private key (`.biprivatekey`) never enters the repository.

## Contents

| Addon | Origin | License | Status |
| --- | --- | --- | --- |
| `NimbyAutorun` | Fork of [SchanaModAutorun](https://github.com/schana/dayz-mod-autorun) (commit 1048f71) | Apache-2.0; see `NimbyAutorun/LICENSE` and `NimbyAutorun/NOTICE` | Source forked October 14, 2026; server-side RPC validation added; per-frame RPC fixed (review CR-004). Built (`@NIMBY-20261014-175505`) and solo-tested on 1.29; see `docs/testing/2026-10-14-solo-ingame-test.md`. Display name Legs (decided October 15, 2026). Plan: `docs/plans/2026-10-15-autorun-plan.md` |
| `NimbyHands` | Fork of Give&Take, Workshop [2903112334](https://steamcommunity.com/sharedfiles/filedetails/?id=2903112334) | Author permits repacking, modification, and redistribution (Workshop description; dated capture in `NimbyHands/PERMISSION.md`, a placeholder until Jeff supplies the text; re-confirm before release) | Imported October 14, 2026; review fixes CR-001, CR-003, CR-005, CR-006 applied; Hands Phase 1 (in-flight registry, lifecycle log, pockets G4) and Phase 2 (rename, G2 controls) on `feat/hands`, October 19, 2026 (see `NimbyHands/NOTICE`). Boot-checked on 1.30; two-client test pending. Display name Hands. Plan: `docs/plans/2026-10-15-hands-plan.md` |
| Zens Core, Zens Zippo Lighter, Zens Music | Repack of Zenarchist's Workshop items | Author permits repacking; Zippo model is CC BY-NC | Theme drop 1; not started |

Legs: if you played on a build before October 19, 2026, reset "Legs: toggle autorun" to default in Options > Controls > Others; the game keeps your old binding (numpad plus) otherwise (NIMBY-T-004).

## Layout when built

```
@NIMBY/
  mod.cpp
  addons/
    NimbyAutorun.pbo
    NimbyAutorun.pbo.nimby.bisign
    NimbyHands.pbo
    NimbyHands.pbo.nimby.bisign
  keys/
    nimby.bikey
```

## Build and sign

`scripts/Build-NimbyPack.ps1` builds the pack from `pack.json`:

1. Packs each addon into an uncompressed PBO with the pack's own packer (`scripts/modules/NimbyPbo.psm1`) and signs it with DayZ Tools `DSSignFile` using the key in `D:\Keys\nimby` (created on first run; the private key never leaves that folder). DayZ Tools Addon Builder is not used: its DayZ build ignores command-line arguments and opens its window.
2. Verifies each PBO: checksum, signature present, prefix matches `pack.json`, and every source file matching `includePatterns` is inside.
3. Boot check: starts the local DayZ Server with the built mod on Nasdara, port 2402, instance 999, creates `<profile>\<Prefix>\selftest` for every addon, and fails on any script error, any `Scripted variables corrupted` line, or any `[SelfTest] FAIL` line in the RPT or script log. `-Qualification` boots with `storageAutoFix = 0`; the default quick check keeps `1`.

Output goes to `build/@NIMBY-<timestamp>/` and logs to `logs/build-<timestamp>/build.log`; both folders are git-ignored. Exit code 0 is PASS, 1 is FAIL, 2 is UNKNOWN (built and verified, boot check skipped or inconclusive). Add an addon by adding a row to `pack.json`; the script needs no change. A row with `"serverOnly": true` builds unsigned to `build/@NIMBY-Server-<timestamp>/` for `-servermod=` and never enters the Workshop output. `-Dev` also builds the harness named in `pack.json` `dev` to `build/@NIMBY-Dev-<timestamp>/` with the separate `nimby_dev` key (shared rules, section 8).

## Test plan

Each addon passes these before the pack ships, on a local DayZ server first, then on the private HostHavoc instance. For the local run, `scripts/Start-NimbyTestServer.ps1` starts the server with the newest build whose `build.log` records `SUMMARY PASS` (or the one given with `-BuildPath`), installs the key, boots with `BattlEye = 1` and `storageAutoFix = 0`, stops the server with an RCON `#shutdown` when you press Enter, and summarizes every `[NIMBY]` log line. `-ProfileName <name>` reuses `profiles/<name>/` between runs.

1. The server boots with `@NIMBY` and the RPT has zero `SCRIPT (E)` and zero `Can't compile` lines naming a Nimby addon.
2. A client joins with signature verification on and is not kicked.
3. Autorun: numpad + starts and stops autorun; W, S, A, D, fire, raise, and lean each stop it; stamina drains and speed drops from sprint to run at zero stamina; getting into a vehicle stops it. Review CR-008: get knocked unconscious (or restrained, or into a vehicle) while autorunning, then recover - the player must stand still, not resume running.
4. Autorun security: a second client cannot start or stop the first client's autorun. The server log shows a `[NIMBY][Autorun] Rejected request` line when this is attempted.
5. Hands, with two clients (two Steam accounts) on the local server. The full matrix is the Hands plan's Phase 3 and `docs/testing/2026-10-19-hands-phase1.md`:
   1. Offer (hold O) and Take: the item moves from A's hands to B's inventory. Server script log shows `REQUESTED` then `CONFIRMED` with the same `id`.
   2. Request (hold O, empty hands) and Give: the item moves from A's hands to B's inventory, not B's hands.
   3. B binds "Hands: open or close pockets" (Options > Controls > Others; unbound by default) and opens pockets, then A puts an item in B's pockets. Toggling again closes them; the action disappears.
   4. Give or Take when the receiver's inventory is full: no prompt, or a `REFUSED` line with `detail=no space in receiver inventory`.
   5. Two takers on one offer at the same moment: one `CONFIRMED`, one `REFUSED detail=item in flight`; the item exists once.
   6. Repeat 1 and 2 ten times each, then drop and re-pick every received item on both clients. Any item that cannot be dropped or that shows in two places is a failure.
   7. Repeat 6 with 200 ms of added latency on one client (for example with clumsy on Windows).
