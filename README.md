# nimby-badlands-server

Configuration, mod management, scripts, and runbooks for **NIMBY | HARDCORE | BADLANDS | 1pp**, a public hardcore-survival DayZ server on the Nasdara (Badlands) map, hosted at HostHavoc in Chicago.

- **Owner:** Jeff Fabish - hosting, configuration, mods, backend.
- **In-game admin:** Jay (Trendkill).
- **Running record:** the Notion hub "DayZ Badlands Server - Project Hub" holds decisions, open questions, mod research, roadmap, and the launch gate. Update it there; this repository holds the artifacts.
- **Generic tooling** that is not specific to this server, such as `PboExtract.ps1`, stays in `Dogs DayZ ToolZ`.

## Rules for this repository

- Never commit a password, RCON password, Steam credential, or Discord token. Secrets live in the HostHavoc panel or in git-ignored local files.
- No change reaches the live server without Jeff's explicit approval. Back up the server before every change.
- PowerShell scripts run in both Windows PowerShell 5.1 and PowerShell 7 and write their output to a log file.

## Layout

Folders are created as their first file lands.

| Path | Holds |
| --- | --- |
| `STATUS.md` | Current state, one screen |
| `NEXT.md` | Ordered next actions |
| `server/serverDZ.cfg` | Server configuration |
| `server/launch-params.md` | `-mod=` and `-servermod=` strings per environment |
| `server/mission-overrides/` | `types.xml`, `globals.xml`, `cfggameplay.json`, `messages.xml` changes |
| `server/live/` | Read-only copy of the live mission's loot and gameplay files, written by `scripts/Sync-LiveReplica.ps1`; the plans' Phase 0 sections read these |
| `mods/manifest.json` | Workshop IDs, versions, `.bikey` names, readiness status |
| `mods/own/` | Source for mods built here |
| `scripts/` | `Install-WorkshopMods.ps1`, `Backup-Server.ps1`, `Restore-Server.ps1`, `Deploy-Config.ps1` |
| `discord/` | `server-spec.json` and `Build-DiscordServer.ps1` |
| `docs/` | Runbooks (patch day, wipe, rollback, restart), `launch-gate.md`, `rules.md` |
