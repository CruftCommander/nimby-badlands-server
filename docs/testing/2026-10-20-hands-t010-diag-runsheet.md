# Hands NIMBY-T-010 diagnostic - two-client run sheet

Branch `diag/hands-t010` (never merged). Implements section 7 of `docs/design/2026-10-20-hands-t010-feasibility.md` under Jeff's rulings of October 20, 2026 (J1 approved, J5 V3 included; J2, J3 deferred; J4 toast built in the fix round, not here).

**D** is the desktop, `Road Dog (2)`. **L** is the G15 laptop, `Road Dog`. Jeff drives both. Write the clock time of every attempt; the results are read from the logs by time.

## 0. Before the session

1. The build must pass the boot check first. Until Jeff says `D:\DayZServer` is free (the NIMBY-T-011 lifetime run), only `-SkipBootCheck` builds exist (`@NIMBY-20261020-221359`: pack and signatures PASS, scripts not compiled). When it is free: `pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build-NimbyPack.ps1` from the worktree; it must end `SUMMARY PASS`. Done October 20, 22:23: `@NIMBY-20261020-222301` `SUMMARY PASS` (Hands self-test 104 pass, 0 fail; no `SCRIPT (E)`).
2. No server running: `Get-Process DayZServer_x64 -ErrorAction SilentlyContinue` prints nothing.

## 1. Setup (desktop PowerShell, one window, keep it open)

```powershell
$wt    = 'D:\GitHub\nimby-badlands-server-wt\hands-t010'
$stamp = '20261020-222301'
$build = "$wt\build\@NIMBY-$stamp"
$prof  = "$wt\profiles\hands-t010-diag"
New-Item -ItemType File -Force -Path "$prof\NimbyHands\debug" | Out-Null
Set-Content -Path "$prof\NimbyHands\diag-variant" -Value 'V0' -Encoding ascii
Compress-Archive -Path $build -DestinationPath "\\Purgatory\server\NIMBY-$stamp.zip" -Force
```

Laptop:

```powershell
$stamp = '20261020-222301'
Expand-Archive -Path "\\Purgatory\server\NIMBY-$stamp.zip" -DestinationPath 'D:\Github\nimby-badlands-server\build' -Force
Test-Path "$env:LOCALAPPDATA\DayZ\NimbyHands\debug"
```

The last line must print `True` (the October 20 client flag; on the desktop too). The October 20 firewall rule still applies; rerun it only if the laptop's address changed.

## 2. Start

1. Server (desktop): `pwsh -NoProfile -ExecutionPolicy Bypass -File "$wt\scripts\Start-NimbyTestServer.ps1" -ServerPath D:\DayZServer -InstanceId 999 -BuildPath $build -ProfileName hands-t010-diag`
2. D (second desktop window, same `$wt`, `$build`, `$prof`): `pwsh -NoProfile -ExecutionPolicy Bypass -File "$wt\scripts\Start-NimbyTestClient.ps1" -BuildPath $build -Connect 127.0.0.1 -Password (Get-Content "$prof\join-password.txt" -Raw).Trim()`
3. L, with `$server` set to the desktop's 192.168.8.x address as on October 20: `pwsh -NoProfile -ExecutionPolicy Bypass -File D:\Github\nimby-badlands-server\scripts\Start-NimbyTestClient.ps1 -BuildPath "D:\Github\nimby-badlands-server\build\@NIMBY-$stamp" -Connect $server -Password (Read-Host 'Join password')`
4. Server RPT (`$prof\DayZServer_x64_*.RPT`): `[NIMBY][Hands] loaded; server=true debug=true`. Each client script log: `loaded; server=false debug=true`.
5. Stand face to face about 1 m apart in the dam yard, both still. D holds the Wrench in hands and has a free slot in his own inventory big enough for it (V3 needs it). L has empty hands and free cargo in the KameezShirt.

## 3. The three verbs

| Code | Do |
| --- | --- |
| T (Take) | D presses O once (the offer loop starts). L aims at D and presses the action key for Take |
| G (Give) | L, empty hands, presses O once (the request loop starts). D aims at L and presses the action key for Give |
| P (Put In Pockets) | L presses his pockets key once (toast: pockets open). D aims at L and holds the action key (2 s) for Put In Pockets. L closes pockets after the block |

After every attempt wait 6 s (the terminal line comes at 5 s) before the next. If the Wrench arrived with L, L drags it from his inventory to the ground and D picks it up into hands before the next attempt. If an offer or request loop is still running after an attempt, press O again to stop it.

## 4. Blocks, in order

To change the variant between blocks (no restart; it is read at every dispatch), in the desktop setup window:

```powershell
Set-Content -Path "$prof\NimbyHands\diag-variant" -Value 'V1' -Encoding ascii
```

| Block | Variant file | Attempts | What changes in game |
| --- | --- | --- | --- |
| 1 | `V0` (set in setup) | T T T, G G G, P P P | Nothing; today's dispatch with the new lines |
| 2 | `V1` | T T T, G G G, P P P | Nothing visible; the dispatching action releases its lock on the other player first |
| 3 | `V2` | T T T, G G G, P P P | On T, D's offer animation stops when L's Take fires; on G, L's request animation stops when D's Give fires; the item moves (if it moves) after both animations end |
| 4 | `V3` | T T T, G G G (P P P if time allows) | As block 3, but the Wrench first goes into D's own inventory, then to L |
| 5 | `V0` | Vanilla control, below | L is restrained; run last |

Block 5, vanilla control (study block B0): D cuffs L (Handcuffs from the kit). D, Wrench in hands, opens the inventory and drags the Wrench from his hands into L's KameezShirt cargo in the vicinity panel. Then D drags it back. D uncuffs L with the keys.

## 5. The lines that decide each hypothesis

Every line is `[NIMBY][Hands][Debug] diag=<name> side=<server|client> t=<ms> id=<corr|-> ...` in the server RPT or a client script log. One attempt is one `id`.

| Question | Line | Reading |
| --- | --- | --- |
| Variant actually in force | server `diag=predispatch ... variant=V<n>` | Must match the block; `V0` in blocks 2 to 4 means the file was not read |
| **H-C**: does the engine's anti-cheat refuse a live receiver? | server `diag=predispatch ... cmtd=<b>`; in block 4 also `cmtdReceiver=<b>` | `cmtd=false` (blocks 1 to 3) or `cmtdReceiver=false` (block 4): **H-C true**, fix is P3. `true`: H-C false on the server |
| Did the deferred hand event leave the queue? | server `diag=watch` lines after `predispatch`: `proc=true` then `proc=false` | `proc` never returns to `false` within 3 s: section 3.1 of the study is wrong |
| **H-D**: is the juncture played back? | `diag=juncture ... juncture=SJ_INVENTORY` naming D: on the server (`side=server`) and in D's client log (`side=client controlled=true`), within about 1 s of `predispatch` | Missing on either side: **H-D**. `juncture=SJ_INVENTORY_FAILURE`: a refusal at queue time |
| **H-A**: does the action's target lock block the move? | block 2: `diag=clearjuncture ... cleared=1` before the attempt; then `state=CONFIRMED` | Block 1 `UNCONFIRMED`, block 2 `CONFIRMED`: **H-A**. A `jGiverItem=true` in a block 2 watch line and never in block 1 supports it (the 50 ms sampling can miss a short juncture) |
| **H-B**: does the animated hide fail while the giver is in an action? | server `diag=watch` (and D's client `diag=watch` started by its own `SJ_INVENTORY`): `state=UNKNOWN idle=false` (the animated move started), then back to `state=Equipped idle=true` with `loc=HANDS:...@<D>`, `hcwFin=true`, and no `signal=EEItemLocationChanged` | Seen in blocks 1 and 2: **H-B**. Block 3 `CONFIRMED` for all three verbs: H-B (or H-A) was the cause and **P2 works** |
| The server refused before the hand state machine | `SJ_INVENTORY` on the server, but the server watch never leaves `state=Equipped idle=true` | The execution checks refused (H-A or H-C); read `cmtd` and block 2 |
| V2 timing | `diag=interrupt` (T and G), then `diag=wait ... result=free waited=<ms>` | `result=cap` and `REFUSED detail=action did not end within 3000 ms (diag V2)`: a timing finding, not a transfer result |
| **P3 end to end** (block 4) | `REQUESTED ... diag V3 step 1`, `diag=step1 result=arrived`, `diag=step2 ... result=true`, `state=CONFIRMED` with the same `id` | `CONFIRMED`: P3 works. `diag=step2 result=refused` or `REFUSED detail=diag V3 step 2: ...`: the Wrench is in D's own inventory, nothing lost |
| Vanilla control (block 5) | `signal=EEItemLocationChanged.begin ... HANDS:...@<D> -> CARGO:KameezShirt_...@<L>` on the server | Present: the animated cross-player hand move works on 1.30 when the giver is idle. Absent: P3 only |

Combined reading, from the study's section 7.5: `cmtd=false` decides P3 (J2). Otherwise block 3 `CONFIRMED` decides P2 (J3). Both blocks 3 and 4 `CONFIRMED` gives Jeff the choice.

## 6. After the session

1. Desktop window running the server: press Enter (RCON `#shutdown` and summary).
2. Laptop, every launch of the day, not only the last:

```powershell
New-Item -ItemType Directory -Force -Path '\\Purgatory\server\laptop-hands-diag' | Out-Null
Get-ChildItem "$env:LOCALAPPDATA\DayZ\script_*.log" | Where-Object LastWriteTime -ge (Get-Date).Date | Copy-Item -Destination '\\Purgatory\server\laptop-hands-diag'
```

3. Desktop: `Copy-Item -Path '\\Purgatory\server\laptop-hands-diag' -Destination "$wt\logs\" -Recurse -Force`

The session that reads the result needs: the server RPT in `$prof`, D's client logs from `%LOCALAPPDATA%\DayZ`, the laptop logs in `$wt\logs\laptop-hands-diag`, and the attempt times.

## 7. Result (October 20, 2026)

Run 23:29 to 23:50 on profile `hands-t010-diag`. H-C true: `cmtd=false` on every V0, V1 and V2 dispatch, V2 included with both players idle. V3 (P3) `CONFIRMED` for Take both ways, Give and Put In Pockets; B0 PASS. Jeff ruled J2: P3; J3 is moot. Full record, ids and times: the "Hands T-010 diagnostic, October 20, 2026" section of `docs/testing/2026-10-19-hands-phase1.md` on `main`.
