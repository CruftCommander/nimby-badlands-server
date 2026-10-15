# Restart, update, and backup schedule

How the live HostHavoc server restarts, updates, and backs up, why it is built this way,
and how to check that it worked. All times are Central (the panel's time zone). Server
logs (RPT, `server_console.log`) run **2 hours behind Central**.

## Schedule (panel > Scheduled Tasks)

| Time (AM and PM) | Task | Type | What it does |
| --- | --- | --- | --- |
| 11:30 PM only | NIMBY nightly backup | Create Backup | Online panel backup; runs with players connected |
| 11:45 | NIMBY warn 15 min | RCON | `say -1 Server restart in 15 minutes...` |
| 11:55 | NIMBY warn 5 min | RCON | `say -1 Server restart in 5 minutes. Server is locked...` |
| 11:55 | NIMBY lock | RCON | `#lock` (no new joins) |
| 11:58 | NIMBY warn 1 min | RCON | `say -1 Server restart in 1 minute. Log out now.` |
| 11:59 | NIMBY graceful #shutdown | RCON | `#shutdown`: DayZ saves and exits cleanly; the panel starts it again about 13 s later |
| 12:00 | NIMBY restart + Steam update 12h | Steam Update | Stops the half-booted restart (before anyone can connect), runs SteamCMD, starts the server |

Disabled, kept as fallbacks: `NIMBY restart 12h` (plain panel restart) and `NIMBY warn 1 min
(11:59)`. The 15- and 5-minute warnings run about a minute early relative to the 11:59 shutdown.

## Why this shape

- The panel's Stop, Restart, and Steam Update **kill** the DayZ process: the RPT just stops,
  with no termination line, so anything not yet autosaved is lost.
- RCON `#shutdown` ends with `--- Termination successfully completed ---` (about 7 s), and
  the panel's crash monitor restarts the server. There is no way to stop that restart, so the
  Steam Update at 12:00 stops the restarted process about 55 s into its boot, while it is
  still loading storage (read only) and before `Player connect enabled`.
- Tested 2026-10-14 at 10:22 PM CT: clean termination, mid-boot stop, then a clean boot with
  every storage file `valid:yes` and storage load 17.25 s (normal ~17 s). Total downtime about
  2.5 minutes.

## Verify a restart

Pull logs with `scripts/Get-ServerLogs.ps1`, then check:

1. `server_console.log` has `BattlEye Server: RCon admin #N (...) logged in` at each RCON task
   time (log time = Central minus 2 h). A missing login means that task's RCON password is wrong.
2. The pre-restart RPT ends with `--- Termination successfully completed ---`.
3. The final RPT shows every `.bin` storage file `valid:yes`, `Storage load took` near 17 s,
   `Player connect enabled`, and zero `SCRIPT (E)`. Compare everything else against
   `docs/baselines/2026-10-14-vanilla-server-logs.md`.

## Editing RCON tasks

- Every RCON task stores the RCON password (10 characters; the BattlEye `RConPassword`).
  The edit form shows a 17-character placeholder for a stored password; leave it to keep it.
- **Chrome autofills the field with the panel login** (32 characters). Clear it and type the
  RCON password before saving, or the task fails at run time.
- Save once. A double save created a duplicate task on 2026-10-14.
