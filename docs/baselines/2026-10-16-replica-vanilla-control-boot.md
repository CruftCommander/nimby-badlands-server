# Baseline - replica vanilla control boot (October 16, 2026)

The local replica at `D:\DayZServer` boots the live Nasdara world with no mods and reproduces the live server's RPT warning classes exactly. This is the same-build vanilla control that shared rules 9.2 compares each addon release against.

## Setup

| Item | Value |
| --- | --- |
| Replica | `D:\DayZServer`, binaries from the Steam `DayZServer` install (build 25460313, DayZ 1.30.164078.38), built by `scripts/Sync-LiveReplica.ps1` runs `replica-20261015-235615` and `replica-20261016-000410` |
| Mirror | `D:\HostHavoc\NIMBY Badlands\104.192.227.46_2302`, WinSCP sync of the HostHavoc server; `storage_1` snapshot taken while the live server ran |
| World | `storage_998`, a mirror copy of the live `storage_1` (427 files) |
| Command | `Start-NimbyTestServer.ps1 -ServerPath D:\DayZServer -ProfileName live-replica -InstanceId 998 -Vanilla` |
| Replica RPT | `profiles\live-replica\DayZServer_x64_2026-10-16_00-04-14.RPT` (5,614 lines, git-ignored) |
| Live RPT compared | mirror `HostHavocDayZServer\DayZServer_x64_2026-10-15_10-01-18.RPT` |

## Counts

| Line class | Replica | Live |
| --- | --- | --- |
| `Scripted variables corrupted` | 0 | 0 |
| `SCRIPT (E)` | 0 | 0 |
| Storage files with load failures | 0 | 0 |
| `items loaded. (0 failed)` | 26 | 26 |
| `[CE][PlayerSpawnPoints] NO VALID SPAWNS` | 3 | 3 |
| `[CE][LoadPrototype] 3 Errors during XML parse` | 1 | 1 |
| `[CE][offlineDB] Type ... will be ignored` | 27 | 27 |
| `[CE][DE] Unable to create child (ZmbM_takiCitizenUrbanB_BlueBright)` | 1 | 1 |
| `[CE][offlineDB] No Shutdown message present` | 1 | 1 |
| `[CE][LoadClusters] failed - check prototype defaults` | 3 | 3 |
| `[Inputs] Attempt to insert input twice` | 83 | 83 |
| Entities loaded (`entity:` line) | 29,789 | 30,162 |
| `Init sequence finished` | 1 | 1 |

`MATERIAL (E)` lines for `player_testing.xob` and `z_hermit_m.xob` and the `FireworksLauncher_Skeleton` bone lines appear in the replica RPT and are engine asset noise, not script errors.

## Reading

- The storage copy loads with zero corruption and zero failed items, so the mirror's `storage_1` is intact and the release-gate boots in shared rules 6.1 and 9.2 can run against `storage_998`.
- Every warning in the replica is also in the live RPT at the same count. A release-gate boot is judged against this table: a new line class, or a higher count in any row, is a finding.
- "No Shutdown message present" is expected for a snapshot taken from a running server. After the clean `#shutdown` of this run it should not recur on the next `storage_998` boot, but every `Sync-LiveReplica.ps1` run re-mirrors the running-server snapshot into `storage_998`, so it returns after each sync. It is not a finding.
- The entity difference is the age of the snapshot, not a load problem.
- Open question for the live server, not the replica: `[CE][PlayerSpawnPoints] :: There are no valid groups` appears three times on live as well. Check `cfgplayerspawnpoints.xml` on Nasdara before assuming fresh spawns land where intended.

## First boot failure, for the record

The first replica boot (`testserver-20261016-000021`) failed with `BattlEye initialization failed` because `Sync-LiveReplica.ps1` v1 excluded the whole `battleye\` folder and the server needs `battleye\BEServer_x64.dll`. Fixed the same night: only `BEServer*.cfg` files are excluded now.
