# Test kit brief (October 26, 2026)

Asked by Jeff, October 26, 2026, 22:27, during the six-addon session: a debug flag that, when enabled, spawns each survivor with the items the mod tests need (hammer, nails, paper, pens, tape, epoxy, knife, and whatever a later mod adds), the way the replica's `init.c` test kit did in the local sessions. Reusable for every future NIMBY mod test. Ideally it affects only players holding a given role. A restart to take effect is acceptable. Not part of release 1; this brief exists so the next session can start from decisions, not from scratch.

## What exists

- The replica `init.c` (`D:\DayZServer\_backups\init.c.range-candidate-5`) spawns a `Motorbike_01_Red` when the profile holds `NimbyTestKit\motorbike` (Ears record, October 24, 2026). Mission-file code, replica only; `Sync-LiveReplica.ps1` overwrites it.
- Build-NimbyPack.ps1 flags each addon's `$profile:<Prefix>\selftest` and the addons read `$profile:<Prefix>\debug`. A file in the profile folder is the fleet's existing "flag" mechanism and already requires a restart.

## Design the brief proposes

A server-only addon row in `pack.json` (`"serverOnly": true`), so it builds unsigned to `build\@NIMBY-Server-<stamp>\`, loads with `-servermod=`, never enters the Workshop item, and no client needs it (shared rules 8.1). Working name `NimbyKit`, display name to be ruled; code prefix, log prefix and RPC block assigned by the orchestrator in shared rules 4.1 (no RPC is expected: server-side only).

- Config `$profile:NimbyKit\config.json`, read once at mission start (restart to apply, as Jeff accepted): `enabled` (bool), `recipients` (list of Steam64 ids; empty list means nobody, never everybody), `onFreshSpawnOnly` (bool, default true), `items` (list of `{ "class": "Hammer", "quantity": 1 }` entries; quantity applies to stackables such as `Nail` and `Paper`). Defaults written on first start with `enabled=false` and an empty recipient list, so a forgotten file does nothing on live.
- Hook: the server's fresh-spawn equip path (vanilla `MissionServer.EquipCharacter`, cited by file:line from `vendor/dayz-1.30-scripts/` before any code), after vanilla gear; with `onFreshSpawnOnly=false`, also on connect (`OnPlayerLoaded`) if the player's inventory lacks the kit. Nothing on the client; nothing persisted by the addon (utility addon, shared rules 6).
- Role: DayZ has no player roles; "role" means the Steam64 allowlist above. If Central Scrutinizer (`NimbyAdmin`) lands its admin roster first, the recipient list can point at that roster instead of its own; until then the allowlist is the role.
- Logging: one line per kit issued, `[NIMBY][Kit] issued player=<name> (<GetId()>, <Steam64>) items=<n>`, and one refusal line when the file is malformed (log once per change, as the Greet pattern).
- Self-test: the config parser and the quantity rules, flagged like every addon.

## Decisions Jeff owes before a build

1. Separate server-only addon (above), or a feature of Central Scrutinizer? Separate keeps it shippable this month and keeps the admin mod's scope clean; recommended.
2. Fresh spawn only (default), or also top up on connect?
3. The first `items` list. Proposed: `Hammer` 1, `Nail` 20, `Paper` 10, `Pen_Black` 1, `DuctTape` 1, `EpoxyPutty` 1, `KitchenKnife` 1, `Wrench` 1 (Hands and Tack rows), `Whetstone` 1.
4. Whether the live server ever runs with `enabled=true` outside a test window, or only the replica. Recommended: live only inside a test window, flipped back before the next scheduled restart.
