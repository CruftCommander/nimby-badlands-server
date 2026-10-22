# Profile-folder overrides

Files that belong in the server's profile folder (`$profile:`), not the mission. The repository copy is the record; the host copy is what the server reads. Each subfolder is one addon's profile folder and is copied to the host as a whole at that addon's release and on every later change.

| Folder | Host path | Owner plan | Notes |
| --- | --- | --- | --- |
| `NimbyGreet/` | `$profile:NimbyGreet\` | Plan 10 (Greet), G12 | `greet.json` is the welcome window text. Raise `version` on every edit or returning players never see the change. Text supplied by Jeff, October 21, 2026; the Mods section is rewritten at each release to match what the Workshop item ships |

Never put a secret here: the webhook URL for Central Scrutinizer lives only on the host (Plan 3, N5).
