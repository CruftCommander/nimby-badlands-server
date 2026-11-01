# Central Scrutinizer Phase 0 baseline (2026-10-31)

Plan 3 v0.8 Phase 0 item 1: the engine, source, and reference this Phase 0 was built and booted against. Seat: Claude Code in worktree `D:\GitHub\nimby-badlands-server-wt\nimby-admin`, branch `feat/nimby-admin`.

| Item | Value | Source |
| --- | --- | --- |
| Server engine | `1.30.164.200` | `DayZServer_x64.exe` FileVersion in `F:\SteamLibrary\steamapps\common\DayZServer\` and the replica `D:\DayZServer\` (read October 31, 2026) |
| Server `dta\scripts.pbo` SHA-256 | `FF0AD7EE5297C75DA74364569A830BC0223315C0131579A0C11A7875D64CB2E2` (written 2026-10-21 22:40, identical in both installs) | `Get-FileHash` |
| Shared reference | `vendor/dayz-1.30-scripts/` is the 164200 extract as of October 31, 2026 (3,745 files, same `scripts.pbo` hash); the 164078 extract is kept as `vendor/dayz-1.30-164078-scripts/` | Orchestrator, answer 1 to the recon; the 86-file diff list is `docs/baselines/2026-10-31-dayz-1.30-164200-scripts-reference.md` (from PR "docs/reference-164200") |
| Citations | Re-read in the 164200 extract as each was used (orchestrator: no cited file changed except `InventoryItem.c`, which this phase does not use) | The `Vanilla :` header of each Central Scrutinizer script |
| Client `scripts.pbo` | Not readable from this seat (`F:\SteamLibrary\steamapps\common\DayZ\dta\` holds no `scripts.pbo` on this machine) | Jeff, or the orchestrator's reference record |
| Base revision | `main` at `abe2e12` (PR #16, the Phase 0 stubs) | `git log` |
| Phase 0 source | `feat/nimby-admin`; the exact commit and build stamp of each gate run are in `docs/testing/2026-10-31-central-scrutinizer-phase0.md` | That record |
| First green builds | `build\@NIMBY-20261031-231005` (quick boot, `storage_999`) and `build\@NIMBY-20261031-231332` (`-Dev -Qualification`, replica `storage_998`, `storageAutoFix = 0`): both `SUMMARY PASS`, `SERVERMOD` present, `SELFTEST Admin cases=54`, `AdminServer cases=64`, `DevHarness cases=16`, `TOTAL pass=779 fail=0` | `logs\build-20261031-231005\build.log`, `logs\build-20261031-231332\build.log` |
