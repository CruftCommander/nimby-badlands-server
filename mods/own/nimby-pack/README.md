# NIMBY client pack

One signed client-and-server mod, `@NIMBY`, that carries every small mod the server runs, so players download one pack and the server controls each version.

## What this is not

- Not a private download. Decided October 15, 2026: `@NIMBY` is distributed as one public Steam Workshop item, so every third-party piece keeps its original attribution and license terms inside the pack and in its `NOTICE`.
- Not a place for secrets. The signing private key (`.biprivatekey`) never enters the repository.

## Contents

| Addon | Origin | License | Status |
| --- | --- | --- | --- |
| `NimbyAutorun` | Fork of [SchanaModAutorun](https://github.com/schana/dayz-mod-autorun) (commit 1048f71) | Apache-2.0; see `NimbyAutorun/LICENSE` and `NimbyAutorun/NOTICE` | Source forked October 14, 2026; server-side RPC validation added; per-frame RPC fixed (review CR-004). Built (`@NIMBY-20261014-175505`) and solo-tested on 1.29; see `docs/testing/2026-10-14-solo-ingame-test.md`. Display name Legs (decided October 15, 2026). Plan: `docs/plans/2026-10-15-autorun-plan.md` |
| `NimbyHands` | Fork of Give&Take, Workshop [2903112334](https://steamcommunity.com/sharedfiles/filedetails/?id=2903112334) | Author permits repacking, modification, and redistribution (Workshop description captured verbatim in `NimbyHands/PERMISSION.md`, page dated April 1, 2023; approved by Jeff October 26, 2026, CR-007 closed) | Imported October 14, 2026; review fixes CR-001, CR-003, CR-005, CR-006 applied; Hands Phase 1 (in-flight registry, lifecycle log, pockets G4) and Phase 2 (rename, G2 controls) on `feat/hands`, October 19, 2026 (see `NimbyHands/NOTICE`). Boot-checked on 1.30; two-client test pending. Display name Hands. Plan: `docs/plans/2026-10-15-hands-plan.md` |
| Zens Core, Zens Zippo Lighter, Zens Music | Repack of Zenarchist's Workshop items | Author permits repacking; Zippo model is CC BY-NC | Theme drop 1; not started |

Legs: if you played on a build before October 19, 2026, reset "Legs: toggle autorun" to default in Options > Controls > NIMBY; the game keeps your old binding (numpad plus) otherwise (NIMBY-T-004).

## Troubleshooting Tack

Check these before reporting that a note "will not pin" or "went into the wall". Each one is logged on the server as `[NIMBY][Tack] refuse pin reason=<reason> ... wall=<class> door=<index>`, with the player's id and Steam64, so the server log says which one it was. The boxes the player sees are an admin switch: `refusalNotices` in the server's `$profile:NimbyTack\config.json` (1 on, 0 off; takes effect at the next restart). The log lines are written either way. The cap itself is `areaCapCount` (default 20; 0 disables it) and `areaCapRadiusMeters` (default 50) in the same file; the server tells each player's game the values on connect, so the prompt withdraws at the server's limit.

| What the player sees | Cause | Server log | What to do |
| --- | --- | --- | --- |
| No pin prompt at all with a hammer, tape, epoxy or knife in hands | The player holds only a stack of paper (or no paper). Tack pins one sheet, never a stack | Nothing: the prompt is decided on the player's game (`no-single-sheet`) | Split one sheet off the stack (drag it out in the inventory), then aim again |
| No pin prompt, box "Too many notes nearby" | The area cap: 20 pinned notes within 50 m of the spot (server `config.json`, `areaCapCount` and `areaCapRadiusMeters`) | `reason=area-cap nearby=N cap=N` if the player's game let it through | Take a note down nearby, or pin somewhere else |
| The hammering animation ran, then a box "Too many notes nearby" and no note on the wall | Same cap, counted by the server at the finish (the player's game counted fewer) | `reason=area-cap` | As above |
| No prompt on a wall that looks fine | The wall is more than 1.5 m from the player's head, or the face is more than 25 degrees off vertical, or the aim is on a window, a sill, or a thin edge | `pin-check` lines only with the debug flag | Step closer, aim at the flat of the wall |
| Box "There is no wall there" | The thing aimed at is not a map building, fence, or wall, or it was destroyed during the hold | `reason=no-wall detail=<probe reason>` or `reason=wall-gone` | Aim at a map building or wall piece |
| Box "Notes pin on map buildings" | The thing aimed at is a player build (fence, watchtower, tent, shelter): not supported yet (ruled in scope October 21, 2026; its own round) | `pin-check` lines only with the debug flag (`target-not-building`) | Pin on a map building until the player-build round ships |
| Box "The wall is not where it looks" | The building's drawn surface stands more than 20 cm in front of where the game can measure it; the note would be buried | `reason=no-wall detail=physics-ahead` | Pin on another face of the building |
| Box "The knife will not go into that" | A knife pins only on wood | `reason=not-wood surface=<material>` | Use a hammer, tape, or epoxy, or find a wooden wall |
| Note sits inside the wall or floats off it | The building model draws its wall in front of (or behind) every layer the game can measure. Known: `Land_HouseBlock_2C1` (accepted), `Land_House_1C1` front door (small float) | `pin ... placement=... front=... allowance=...` | Report the building class from the `pin` line; the operator adds it to `modelAllowances` or `doorModelAllowances` in the server `config.json` |
| Door notes | A note pins on a door open or closed, swings with the door, and is back on the door after a restart | `pin ... door=<index>/pivot=...` and `door load result=restored` | Nothing; if a door note hangs in the air after a restart, report the building class |

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
3. Autorun: a double tap of Left Shift starts autorun ("Legs: toggle autorun", rebindable in Options > Controls > NIMBY). While it runs, any Shift press, W, A, S, D, Space, fire, weapon raise, lean, or the toggle stops it, and so do the inventory, the pause menu, any other menu except chat, gestures, and the radial quickbar, focus loss, and a ladder; water does not stop it (autorun keeps swimming, stamina draining as in a sprint) unless the server's `config.json` sets `stopOnSwim` to 1; the second tap of a double tap that stopped it does not start it again. Left Ctrl changes speed without stopping (double tap walk, single press run). Stamina drains normally: sprint drops to run when stamina runs out and returns to sprint by itself once stamina recovers, unless Left Ctrl chose walk or run since the start. A double tap while the weapon is raised does nothing. Getting into a vehicle stops it. Review CR-008: get knocked unconscious (or restrained, or into a vehicle) while autorunning, then recover - the player must stand still, not resume running.
4. Autorun security: a second client cannot start or stop the first client's autorun. The server log shows a `[NIMBY][Autorun] Rejected request` line when this is attempted.
5. Hands, with two clients (two Steam accounts) on the local server. The full matrix is the Hands plan's Phase 3 and `docs/testing/2026-10-19-hands-phase1.md`:
   1. Offer (hold O) and Take: the item moves from A's hands to B's inventory. Server script log shows `REQUESTED` then `CONFIRMED` with the same `id`.
   2. Request (hold O, empty hands) and Give: the item moves from A's hands to B's inventory, not B's hands.
   3. B binds "Hands: open or close pockets" (Options > Controls > NIMBY; unbound by default) and opens pockets, then A puts an item in B's pockets. Toggling again closes them; the action disappears.
   4. Give or Take when the receiver's inventory is full: no prompt, or a `REFUSED` line with `detail=no space in receiver inventory`.
   5. Two takers on one offer at the same moment: one `CONFIRMED`, one `REFUSED detail=item in flight`; the item exists once.
   6. Repeat 1 and 2 ten times each, then drop and re-pick every received item on both clients. Any item that cannot be dropped or that shows in two places is a failure.
   7. Repeat 6 with 200 ms of added latency on one client (for example with clumsy on Windows).
