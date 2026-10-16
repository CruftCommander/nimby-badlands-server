# VPS move feasibility - leaving HostHavoc for a server Jeff manages

Draft v0.1, October 16, 2026. Owner: Jeff Fabish. Status: feasibility only, written because Jeff ruled on October 16, 2026 that the game server leaves HostHavoc for a VPS he manages and that Eyes (the Central Scrutinizer companion) runs beside it on that box. The move plan is written when Sprout starts (decision Q2 and the added question 10 in `central-scrutinizer-feasibility-addendum.md`). Prices below were read on October 16, 2026 and change quarterly; re-check before ordering.

Target: NIMBY Badlands | Enhanced Vanilla | RP | 1PP, DayZ 1.30 / Nasdara, 80 slots, Chicago-centered player base.

## Verdict

The move is feasible and the main costs are known. Bohemia documents a native Linux dedicated server (app `223350`, mods and BattlEye included) and the Windows build is the one HostHavoc runs today, so either OS works; Linux is recommended because it removes the Windows Server license and gives systemd, ufw, and cron for the tooling that replaces the HostHavoc panel, with Windows as the fallback if the Linux build fails the mod gate on the replica. The hard constraint is hardware: DayZ is single-thread bound, so a high-clock CPU beats core count, and the box also needs 32 GB for 80 slots plus Eyes. The price band for that in the US is wide: a cloud VM with dedicated vCPUs runs about $120 to $240 a month; a Game-range dedicated box with included game-traffic DDoS protection at OVH is $357 a month plus a $357 setup fee after OVH's August 2026 increase; European dedicated boxes (Hetzner AX102, about 122 EUR a month) are the best value per core but sit 100 ms or more from a Chicago player base and are not an option for this server. DDoS protection is the deciding feature, not the CPU: a public DayZ server gets attacked, HostHavoc absorbs that today, and a bare cloud VM does not.

What the move buys: Eyes on the same box with local access to RCON, logs, and mission files (no FTP, no public endpoint); any file editable; restarts, backups, and updates on Jeff's schedule; the `DayZServer_x64.exe` and `keys\` that the HostHavoc FTP view hides. What it costs: Jeff owns uptime, patching, BattlEye updates, DDoS, and the restart chain, at 2 AM as well as at noon.

## What this is not

- Not the move plan. Phases, the rehearsal on the replica, the overlap window, and the announcement are written when Sprout starts.
- Not a provider order. The shortlist below is for comparison; the pick is decision V2.
- Not a change to any mod plan. The pack, the release gate, and the replica tooling are unchanged; `Start-NimbyTestServer.ps1` keeps working against the Windows replica, and a Linux rehearsal box is added if Linux is chosen.

## What HostHavoc does today and what replaces it

| HostHavoc feature | Replacement on the VPS | Effort |
| --- | --- | --- |
| Panel start, stop, restart | systemd unit (`Restart=on-failure`) on Linux, or a Windows service wrapper (NSSM) on Windows; Eyes exposes restart to admins (F8) | Small |
| Scheduled tasks (warnings, `#lock`, `#shutdown`, Steam update, start) | cron or systemd timers calling an RCON client, then `steamcmd +app_update 223350 validate`, then start; Eyes can own the chain once it exists | Small, and already specified in `docs/operations/` |
| Nightly panel backup | `restic` or `rclone` job: mission folder, `profiles\`, `storage_1`, BattlEye config, to the NAS or an S3 bucket, with retention | Small |
| File manager and FTP | SSH and SFTP; Eyes for mission files in game (F7, scope Q6: everything in the mission folder) | None |
| Mod installer | `steamcmd +workshop_download_item 221100 <id>` plus symlinks into the server root and `.bikey` into `keys\`. Needs a Steam login that owns DayZ (anonymous login is refused for the server depot); a second Steam account that owns DayZ, with Steam Guard handled once, keeps Jeff's main account off the box | Medium |
| BattlEye | Bundled with the server download; `battleye\BEServer_x64.cfg` holds `RConPassword`, `RConPort`, `RConIP`; BE updates arrive with the server | None |
| DDoS protection | Provider feature; included on OVH Game, an add-on or absent elsewhere | Decides the provider |
| Firewall | ufw or Windows Firewall: UDP 2302 game, UDP 27016 Steam query, UDP 2305 RCON (the three ports the guides agree on); SSH or RDP restricted to Jeff's and Jay's addresses | Small |
| Monitoring and alerts | Eyes tails the RPT and posts to Discord; uptime check from outside (any free monitor) | Part of Eyes |
| Config editing | SSH, or Eyes in game | None |
| Support ticket | Nobody. Jeff and Jay are the support | The real cost |

## Sizing

| Resource | Recommendation | Basis |
| --- | --- | --- |
| CPU | 6-8 cores, highest single-thread clock available (4.0 GHz or more); Ryzen X3D parts are what the game-hosting ranges sell | DayZ is single-thread bound; the guides agree a 4.0 GHz quad-core beats a 2.5 GHz sixteen-core box for this workload |
| RAM | 32 GB | Guides range from 12-16 GB (modded, 60 or more players) to 16-32 GB (modded, 20-40 players); 80 slots plus Eyes plus headroom for the next Bohemia update lands at 32 |
| Storage | NVMe, 250 GB or more | Server install 20 GB, mods, `storage_1` growth, local backups before offsite |
| Network | 1 Gbps port, unmetered or 5 TB or more a month | An 80-slot server sustains outbound traffic continuously |
| Location | Chicago, or the nearest US-central or US-east site | The player base is Chicago-centered today; HostHavoc Chicago sets the latency baseline players expect |

## Provider shortlist (read October 16, 2026)

| Option | Class | Spec | Monthly | Setup | DDoS | Location | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| OVHcloud GAME-1 (2026) | Dedicated, game range | Ryzen 7 9800X3D, 8c/16t, 4.7-5.2 GHz; 64 GB DDR5 ECC; 2x 960 GB NVMe; 1 Gbps unmetered | $357 | $357 | Game DDoS included | Hillsboro OR, Vint Hill VA, Beauharnois and Toronto (no Chicago) | 99.95 percent SLA. Price doubled in August 2026 (139.99 EUR to 279.99 EUR in Europe); OVH says DDR5 cost is the driver and updates prices quarterly |
| OVHcloud GAME-2 (2026) | Dedicated, game range | Ryzen 9 9950X3D, 16c/32t; 64 GB; 2x 960 GB NVMe | $401 | $401 | Included | Same | More cores than DayZ uses |
| Vultr Optimized Cloud, General Purpose | Cloud VM, dedicated vCPU | 4 vCPU AMD EPYC, 16 GB, 80 GB NVMe, 6 TB | $120 | None | Not included on the pricing page; an add-on if offered | US sites not confirmed on the pricing page; Chicago is listed in Vultr's location list as far as I know, to confirm | 8 vCPU / 32 GB is $240; the vCPU clock is lower than a desktop Ryzen, so test server FPS at 80 slots before committing |
| Vultr High Frequency | Cloud VM, shared vCPU | 4 vCPU 3 GHz+ Xeon, 16 GB, 384 GB NVMe | $96 | None | Same | Same | Shared vCPU; cheapest plausible box, and the one most likely to stutter at full pop |
| Hetzner AX102 | Dedicated | Ryzen 9 7950X3D, 16c, 128 GB DDR5 ECC, 2x 1.92 TB NVMe | about 122 EUR (June 2026 figure; Hetzner reprices) | Varies by model | Basic network protection, not game-aware | Germany and Finland only | Best value per core; not usable for a Chicago player base (100 ms or more added) |
| US budget dedicated (Server Mania, Limestone, LowEndTalk offers) | Dedicated, older hardware | i7 or Xeon E-2200, 32 GB, NVMe | $50-80 | Varies | Usually absent | Various US cities | Older CPUs with lower clocks; verify single-thread performance and DDoS before considering |
| Not checked yet | Dedicated with game DDoS in the US central region | Hivelocity, ReliableSite, Latitude.sh, Path.net-protected hosts | - | - | - | Chicago and Dallas sites exist in this class | Worth one pass before V2 is decided; these are the providers most DayZ communities that self-host end up on |

HostHavoc today: [YOUR HOSTHAVOC MONTHLY] for 80 slots in Chicago, DDoS and panel included. The comparison that matters is HostHavoc plus Eyes elsewhere (not possible, per Q2) against one box that runs both.

## Operating system

| | Linux (Ubuntu 24.04 LTS) | Windows Server 2025 |
| --- | --- | --- |
| Server build | Native `DayZServer` binary, app `223350`, documented by Bohemia with mods and BattlEye | The build HostHavoc runs; the replica at `D:\DayZServer` already proves it |
| License | None | Add-on at most providers; price not shown on the pages read (Hetzner lists Standard and Datacenter editions as optional) |
| Tooling | systemd, cron, ufw, SSH, `restic`; Eyes runs as a .NET service on Linux without change | Task Scheduler, NSSM, Windows Firewall, RDP; same Eyes binary |
| Risks | Case-sensitive filesystem: a mod whose config references `Data\file.paa` with the wrong case loads on Windows and fails on Linux (inference from the filesystem difference; none of the pages read mentions it; the mod gate on a Linux rehearsal box is the check). Whether the Linux build ships the same day as Windows for each Bohemia update is not stated on the pages read; a lag would hold the server on the old version until it lands | Known path; higher cost; Windows updates and reboots to manage |
| Recommendation | Yes, if the pack passes its release gate on a Linux rehearsal box (`storage_998` copy, same RPT comparison as the baseline) | Fallback if the Linux gate fails or the Linux build lags 1.30.x |

## Carry or wipe

Carry the world. The mirror already copies the live `storage_1` and the replica boot proved it loads with 0 corrupted variables and the same warnings as live, so moving it is a file copy plus a final sync at the cutover restart. Player identities are Steam64 and carry; BattlEye bans carry with `ban.txt`; the only thing that changes is the server address, which players re-favorite. A wipe is reserved for the regular policy (every three months, a content drop, or a forced update), not for the move.

## Cutover shape (for the plan, not for now)

1. Rehearsal: build the box, install the server with `steamcmd`, copy the mirror's mission folder and `storage_1`, boot, compare the RPT against `docs/baselines/2026-10-16-replica-vanilla-control-boot.md`, run the pack's release gate. On Linux this is the mod gate.
2. Overlap: run the new box for a week with a test name while HostHavoc stays live; Jay and a few regulars play on it; Eyes runs against it.
3. Cutover at a scheduled restart: announce in Discord with 7 days' notice (the wipe-notice convention), final `storage_1` sync after `#shutdown` on HostHavoc, start the new box, HostHavoc stays locked for a week as the rollback, then cancel.

## Risks

| Risk | Mitigation |
| --- | --- |
| DDoS on a box without game-aware protection takes the server down and there is no host to call | Pick a provider with game-traffic protection included (V2), or accept the risk knowingly on a cloud VM with the provider's add-on |
| Cloud vCPU clock too low for 80 slots | Rehearsal at full pop with bots is not possible; use the live overlap week and server FPS in the RPT as the measure, and move up a tier before cutover if it drops under 30 |
| Linux build lags a Bohemia update | Windows fallback stays documented; the replica stays Windows |
| Steam login for `steamcmd` breaks on Steam Guard | Second Steam account owning DayZ, Steam Guard approved once on the box, credentials in a root-only file |
| Nobody is on call | Eyes posts every crash and restart to Discord; the systemd unit restarts on failure; the scheduled chain keeps the twice-daily restart |
| Price changes between now and the order | Re-read the provider page the week of V2; OVH updates prices quarterly |

## Decisions for Jeff (when the plan is written)

| # | Decision | Recommendation |
| --- | --- | --- |
| V1 | Operating system | Linux, with the mod gate on a rehearsal box as the condition; Windows if it fails |
| V2 | Provider class | A US dedicated box with game-traffic DDoS protection included, after one pass over the unchecked providers above; OVH GAME-1 in Vint Hill or Hillsboro is the known-good but expensive floor |
| V3 | Carry or wipe | Carry |
| V4 | Overlap window | One week with HostHavoc locked as rollback |
| V5 | Second Steam account for the box | Yes |

## Sources

- [DayZ: Hosting a Linux Server (Bohemia wiki)](https://community.bistudio.com/wiki/DayZ:Hosting_a_Linux_Server) - app ids, launch line, mods, systemd unit
- [DayZ Dedicated Server: 12 Steps (shattered.io, updated September 22, 2026)](https://shattered.io/dayz-dedicated-server/) - sizing, ports, BattlEye config, Steam login requirement
- [DayZ Dedicated Server Hosting Guide (RedSwitches, April 23, 2026)](https://www.redswitches.com/blog/dayz-dedicated-server-hosting-guide/) - sizing tiers, launch flags
- [OVHcloud Game dedicated servers (US)](https://us.ovhcloud.com/bare-metal/game/) - GAME-1 and GAME-2 specs, prices, locations, included DDoS
- [OVHcloud dedicated server price change, H2 2026](https://blog.ovhcloud.com/en/posts/dedicated-servers-pricing-update-h2-2026/) - percentages and effective dates
- [OVH price rise coverage (TechRadar via Yahoo)](https://finance.yahoo.com/technology/ai/articles/ovh-cloud-tells-users-expect-183500106.html)
- [Hetzner AX matrix](https://www.hetzner.com/dedicated-rootserver/matrix-ax/) - AX102 spec, locations, Windows add-on
- [Cheap dedicated servers 2026 (klymentiev.com, June 2026)](https://klymentiev.com/blog/cheap-dedicated-server-2026) - Hetzner and US budget figures
- [Vultr pricing](https://www.vultr.com/pricing/) - Optimized Cloud and High Frequency plans
