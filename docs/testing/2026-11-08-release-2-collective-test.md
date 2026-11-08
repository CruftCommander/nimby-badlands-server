# Release 2 collective mod test (November 8, 2026)

One pass over every addon in `@NIMBY-20261108-111218` (build 65, the release 2 candidate: build 64 content on ODOL models) before the release gate 9.2. One or two checks per mod, written by Claude at Jeff's ask (11:40); results filled in during the run. Server: `Start-NimbyTestServer.ps1 -BuildPath .\build\@NIMBY-20261108-111218 -ServerPath 'F:\SteamLibrary\steamapps\common\DayZServer' -ProfileName dpv-phase0 -ThirdPerson` (add `-ServerTime 2026/11/8/1/0` for the night checks). Client: `Start-NimbyTestClient.ps1 -BuildPath D:\GitHub\nimby-badlands-server-wt\dpv\build\@NIMBY-20261108-111218 -Connect 127.0.0.1 -Port 2302 -Password $env:NIMBY_JOIN_PASSWORD -Windowed`. Hands needs a second client (Jay, or the laptop). The test kit (`NimbyKit`) is not in the pack; items come from the DPV spawn flag's `+ <class>` cargo lines or the admin-free way: find them.

Setup lines for the spawn flag (`profiles\dpv-phase0\NimbyDpv\spawn-testcar`), one car with the test items in its cargo:

```
@ 12142 11170
NimbyDpv_Car
= RoofBear Bear_White
= RailTent_L MediumTent
+ Paper
+ Pen_Black
+ Hammer
+ Nail
+ DuctTape
+ Canteen
+ Hatchet
```

| # | Mod | Check | Expected | Result | Notes |
| --- | --- | --- | --- | --- | --- |
| 1 | Greet | Connect | Welcome window opens on join; Mods tab lists seven rows ending with DPV; "What's changed" tab shows release 2 first | PASS | greet.json v2 must be in `profiles\dpv-phase0\NimbyGreet\` for this; copy `server\profile-overrides\NimbyGreet\greet.json` there first |
| 2 | Greet | Reopen | Insert reopens it; Escape, pause menu, Server welcome reopens it | PASS | |
| 3 | Legs | Autorun | Double-tap Left Shift runs; any move key, fire or raise stops it | PASS | |
| 4 | Legs | Vehicle | Autorun does nothing in the DPV driver seat; no script error in the RPT | PASS | |
| 5 | Ears | Toggle | N lowers effect volume (engine, footsteps), N again restores; voice unaffected | PASS | |
| 6 | Ears | Survives death | Toggle on, die (or relog), state resets as the plan says | PASS | |
| 7 | Scrawl | Write | Pen in hands, drop paper on it, hold the action key, type a line, save | PASS | |
| 8 | Scrawl | Read | Hold left mouse on the note: text shows; a second player reads the same text (optional) | PASS | |
| 9 | Tack | Pin | Hammer in hands, nail in inventory, look at a wall, hold the action key: note pinned, readable | PASS | |
| 10 | Tack | Take down | Take down once: sheet returned worn; relog: a pinned note is still on the wall | PASS | |
| 11 | Hands | Offer | Player A holds an item, presses O; player B takes it from A's hands into cargo; both get the notification | SKIPPED | no second client on; rides the release gate two-client session |
| 12 | Hands | Request | B with empty hands presses O at A: A's item moves to B; a full receiver is refused with a notice | SKIPPED | as 11 |
| 13 | DPV | Drive and storage | Enter, start, drive, headlights; bear and tent on the car; "Take to hands" on both | PASS | repeat of the 11:40 look, one line |
| 14 | DPV | Persistence | Graceful restart (Enter in the server window), rejoin: car and items back | SKIPPED | passed at 11:40 on this build (ODOL look, check 8) |
| 15 | Pack | Boot | RPT of the run: no `SCRIPT (E)`, no `Scripted variables corrupted`, every addon's `selftest complete` line present | PASS | all 11 RPTs of November 8 (00:12 to 12:02): 0 hits; the 5 hits Select-String showed were the November 6 15:31 RPT, T-089 history |corrupted'` should print nothing |
| 16 | CE rows (optional) | DPV event spawns | With the four staged mission files from `logs\deploy-20261108-110040\patched\` copied into the test server's mission (back up the originals first; wipe `storage_998`), spawn with `@ 2941 9114` and look around the rocket base: a CE DPV stands at the position (`<pos x="2941.09" z="9114.87"/>`), parts rolled as configured | NOT RUN | release gate |

Pass rule: every row 1 to 15 PASS, or a FAIL with a finding number and a decision (fix before release, or ship with the note). Row 16 is a bonus; the release restart's first CE log is the real check.

## Results

November 8, 2026, 12:03 to 12:14 Central, build 65 `@NIMBY-20261108-111218`, Jeff alone on the local test server (third person, server time 01:00, fresh `storage_998`), greet.json v2 with the What's changed tab. Rows 1 to 10, 13 and 15 PASS; 11, 12 and 14 skipped with reasons in the table; 16 not run. Verdict: the pack is release-ready on this build; the two Hands rows are owed at the release gate's two-client session.

## Findings

None new. Row 15 note for the next run: scope the RPT pattern to the day's files (`DayZServer_x64_<date>_*.RPT`), or old T-089 hits mislead.
