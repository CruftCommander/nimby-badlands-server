# Server log baseline - vanilla, empty server (2026-10-14)

What the HostHavoc server's logs look like when nothing is wrong: no players, no mods,
vanilla Chernarus, DayZ 1.29.163709. Compare any later log against this before calling
something a problem. Re-baseline after the 1.30 update, after the Nasdara switch, and
after each mod is added.

## Source

| Item | Value |
| --- | --- |
| Pulled with | `scripts/Get-ServerLogs.ps1` on 2026-10-14 (local copy: `logs/server-20261014-215046/`, git-ignored) |
| Files | 8 RPT (one per boot), 8 `script_*.log`, `server_console.log`. No `.ADM`: `-adminlog` is not on the launch line. |
| Boots covered | 5:51 PM to 9:50 PM CT. Boots from 9:11 PM CT on run `serverDZ.cfg` v0.2; earlier boots ran HostHavoc's default config. |
| Players | 0 in every file. No connect or disconnect lines. |
| Timestamps | RPT and console times are **Central minus 2 hours** (log `19:30:40` = 9:30:40 PM CT). The panel and the task scheduler use Central. |

## Normal boot

Every complete boot follows the same sequence and takes **58-63 seconds** from process start
to `Player connect enabled`:

1. Engine start: two `ENGINE (W)` lines (module info, job worker thread) and two
   `ANIMATION (E)` lines for `sakhal/Anims/cfg/skeletons.anim.xml`.
2. `Hostname of server: "..."` - confirms which config loaded.
3. World load: about 187 `Door '...' is missing geometry components` warnings and 29-33
   `ENTITY (E): Type '...' must be inherited from class '...'` errors (vanilla asset noise).
4. Central Economy: `[CE][Hive] Initializing OFFLINE`, storage restore from
   `storage_1\data\*.bin` (`valid:yes`; the `.001`/`.002` history files show `valid:NO`
   because `lootHistory = 1`), `Storage load took: ~17 seconds`.
5. `[CE][Hive] :: Init sequence finished.` then `Player connect enabled`.
6. `[IdleMode] Entering IN - save processed` about one second later (no players).

## Expected noise per boot

| Line | Count per boot | Notes |
| --- | --- | --- |
| `ENTITY (W): Unknown object class 'pond'` | 150-280, grows with uptime | Vanilla, keeps appearing as the world streams |
| `ENTITY (W): Door '...' is missing geometry components` | ~187 | Fixed per boot |
| `ENTITY (E): Type '...' must be inherited from class '...'` | 29-33 | Vanilla config classes |
| `ANIMATION (E)` sakhal skeleton | 2 | Every boot |
| `MATERIAL (E)` `z_hermit_m.xob` `Material_1.emat` | 1 | Every boot |
| `!!! [CE][LoadPrototype] 5 groups have no points` / `10 Errors during XML parse` | 1 each | Vanilla Chernarus `mapgroupproto.xml` |
| `!!! [CE][offlineDB]` ChristmasTree / Static_FrozenScientist_DE ignored | 1 each | Vanilla types |
| `!!! [CE][DE][SPAWNS]` skipping `VehicleTransitBus` | 1 | Vanilla event list |
| `!!! [CE][offlineDB] :: No Shutdown message present.` | 1 | See finding 1 - currently on every boot |
| `NETWORK (E): Will delete object with !finite or outside world coords` (animal) | 0-1 | Occasional, vanilla |
| `SCRIPT (E)` | **0** | The script log is module loads only, 1558 bytes every boot |

## Normal steady state (empty)

- Memory: 2.0-2.3 GB, flat over 40 minutes.
- Server FPS: hundreds to ~5,000 when empty. Meaningless until players connect; re-baseline then.
- One `Players: 0 in total` and one `Used memory` line per second (see finding 2).

## What abnormal looks like

- Any `SCRIPT (E)`, `Can't compile`, or a `script_*.log` larger than ~1.6 KB.
- An `(E)` line whose type is not in the table above, or counts far outside the ranges.
- Boot slower than ~90 seconds to `Player connect enabled`, or storage load much over 17 s.
- `valid:NO` on a `.bin` storage file (not the `.001`/`.002` history files).
- Memory climbing steadily while empty.
- An RPT that ends without the next boot's start being a scheduled or manual action.

## Findings

1. **Every stop so far is a hard stop.** Each boot logs `No Shutdown message present`, and no
   RPT ends with a shutdown sequence. The panel's Stop, Restart and the Steam Update task
   end the process without DayZ's shutdown save. While empty this costs little (the idle
   save runs), but with players anything since the last autosave can be lost at each 12-hour
   restart. Next step: test an RCON `#shutdown` before the 12:00 update and confirm the next
   boot no longer logs the message.
2. **`logMemory = 1` and `logPlayers = 1` are intervals in seconds**, so the RPT and
   `server_console.log` get two lines per second (`server_console.log` reached 821 KB in four
   hours). Set both to `60`, like `logAverageFps`.
3. **`server_console.log` prints the BattlEye RCON password in plaintext on every boot**
   (`Config entry: RConPassword ...`). Anyone with panel or SFTP access can read it, and so can
   every local pull in `logs/`. RCON listens on the public IP (port 2406) with
   `RestrictRCon 0`. Keep `logs/` out of git (it is) and out of shared folders; rotate the
   RCON password if a pull ever leaves this machine.
4. **`-dologs` is not needed for the FPS, memory and player lines** - they appear on the
   default launch line. `-adminlog` is still needed for the `.ADM` admin log (support ticket
   pending).
5. **Manual restart at 9:16:52 PM CT.** The 9:11 PM boot (`19-11-02.RPT`) ends at 9:16:40 PM
   with no error and the next boot starts 12 seconds later. Jeff restarted it from the panel.
   This is what a panel restart looks like in the logs: no shutdown lines, then a new RPT.
