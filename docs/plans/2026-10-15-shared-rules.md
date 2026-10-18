# NIMBY pack shared rules

Version 1.2. Status: v1.0 approved by Jeff Fabish, October 15, 2026; v1.1 amendment (content addons, section 6.1; key bindings, section 5; six new addons split by what a Bohemia update breaks) and v1.2 amendment (names: Hands, Ears, Central Scrutinizer; configurable key bindings and the Menu addon, section 5) approved by Jeff Fabish, October 16, 2026. Owner: Jeff Fabish.

These rules govern the in-house mod plans in `docs/plans/` - Legs (`NimbyAutorun`), Hands (`NimbyHands`), Central Scrutinizer (`NimbyAdmin`), Puff (`NimbySmoke`), Buzz (`NimbySmokeFx`), Sprout (`NimbyGrow`), Scrawl (`NimbyNotes`), Tack (`NimbyTack`), and Ears (`NimbyEarplugs`) - and the orchestrator session that coordinates them. Each plan links here instead of carrying its own copy, so there is one text to keep current. A plan that needs an exception to a rule records the exception in its own decisions table.

## 1. Introduction

### 1.1. Scope

Branching, shared files, identifiers, key bindings, save footprint, test tooling, quality gates, and release for every addon in `mods/own/nimby-pack/`.

### 1.2. What this is not

- Not a plan. Phases, tests, and acceptance criteria live in each mod's plan.
- Not a coding standard for Enforce Script beyond the identifier and logging rules in section 4.
- Not a live-server runbook. Restart chains and panel procedures live in `docs/operations/`.

### 1.3. Requirements language

The keywords MUST, MUST NOT, REQUIRED, SHALL, SHALL NOT, SHOULD, SHOULD NOT, RECOMMENDED, MAY, and OPTIONAL in this document are to be interpreted as described in RFC 2119.

## 2. Branches, worktrees, and git

### 2.1. Worktrees

Each mod works on its own branch in its own git worktree under `D:\GitHub\nimby-badlands-server-wt\<branch-name>`:

| Mod | Branch | Worktree |
| --- | --- | --- |
| Legs | `feat/autorun` | `D:\GitHub\nimby-badlands-server-wt\feat-autorun` |
| Hands | `feat/hands` | `D:\GitHub\nimby-badlands-server-wt\feat-hands` |
| Central Scrutinizer | `feat/nimby-admin` | `D:\GitHub\nimby-badlands-server-wt\feat-nimby-admin` |
| Puff | `feat/puff` | `D:\GitHub\nimby-badlands-server-wt\feat-puff` |
| Buzz | `feat/buzz` | `D:\GitHub\nimby-badlands-server-wt\feat-buzz` |
| Sprout | `feat/sprout` | `D:\GitHub\nimby-badlands-server-wt\feat-sprout` |
| Scrawl | `feat/scrawl` | `D:\GitHub\nimby-badlands-server-wt\feat-scrawl` |
| Tack | `feat/tack` | `D:\GitHub\nimby-badlands-server-wt\feat-tack` |
| Ears | `feat/ears` | `D:\GitHub\nimby-badlands-server-wt\feat-ears` |

### 2.2. Commit and push policy

Decided by Jeff, October 15, 2026.

- An implementation session MAY commit and push on its own `feat/*` branch. Commits MUST be small, described, and limited to that mod's folder plus its docs.
- A pushed `feat/*` branch is unreviewed work. Nothing on it is released, referenced in the Notion hub, or described as done until Jeff merges it.
- Jeff runs every merge to `main` and every push of `main`. Sessions MUST NOT merge, rebase onto, or push `main`.
- Changes to orchestrator-owned files (section 3) land on `main` through Jeff's commit, never on a `feat/*` branch.

## 3. Shared files

Only the orchestrator edits these files. A session that needs a change there asks the orchestrator and waits.

| File | Why it is shared |
| --- | --- |
| `mods/own/nimby-pack/pack.json` | Addon rows, boot check, and test server for every mod |
| `scripts/Build-NimbyPack.ps1` | Build, sign, verify, boot check, and the server-mod output (section 8) |
| `scripts/modules/*.psm1` | PBO packer and Steam helpers |
| `scripts/Start-NimbyTestServer.ps1` | The interactive test server every plan's tests run on |
| `docs/plans/2026-10-15-shared-rules.md` | This file |

Known `pack.json` edits and their timing: Nasdara mission in `bootCheck` and `testServer` before any worktree branches; the `NimbyAdmin` rows at Central Scrutinizer Phase 0; the Hands rename at Hands Phase 2; one row each for `NimbySmoke`, `NimbySmokeFx`, `NimbyGrow`, `NimbyNotes`, `NimbyTack`, and `NimbyEarplugs` at that addon's Phase 1.

## 4. Identifiers

### 4.1. Names

| Mod | Display name | Code and PBO prefix | Log prefix | RPC id block |
| --- | --- | --- | --- | --- |
| Legs | Legs | `NimbyAutorun` | `[NIMBY][Autorun]` | `65501xx` (`6550101` toggle, `6550102` server cancellation) |
| Hands | Hands (Gimme until October 16, 2026) | `NimbyHands` | `[NIMBY][Hands]` | `65502xx` (reserved; Hands uses the action framework today) |
| Central Scrutinizer | Central Scrutinizer (Golden God until October 16, 2026) | `NimbyAdmin` | `[NIMBY][Admin]` | `65503xx` (`6550350`-`6550399` reserved for observation feeds, N9) |
| Puff | Puff | `NimbySmoke` | `[NIMBY][Smoke]` | `65504xx` (reserved; Puff uses the action framework and a script event) |
| Buzz | Buzz | `NimbySmokeFx` | `[NIMBY][SmokeFx]` | `65508xx` (`6550801` effect sync, server to owning client) |
| Sprout | Sprout | `NimbyGrow` | `[NIMBY][Grow]` | `65505xx` (reserved) |
| Scrawl | Scrawl | `NimbyNotes` | `[NIMBY][Notes]` | `65506xx` (reserved) |
| Tack | Tack | `NimbyTack` | `[NIMBY][Tack]` | `65509xx` (reserved; action framework) |
| Ears | Ears (Hush until October 16, 2026) | `NimbyEarplugs` | `[NIMBY][Earplugs]` | `65507xx` (reserved; client only) |
| Menu | Menu | `NimbyMenu` | `[NIMBY][Menu]` | `65510xx` (reserved; not planned yet, see section 5) |

A display name changes only `mod.cpp`, Workshop text, string tables, and menu titles. Code prefixes, log prefixes, and RPC ids MUST NOT change for a display-name decision.

### 4.2. Player identity in logs

Two identities exist: `PlayerIdentity.GetId()` is the BI hash that the server `.ADM` log uses; the Steam64 id (expected `GetPlainId()`, verified in each Phase 0) is what bans and permissions key on. Every log line that names a player MUST carry both. Log prefixes MUST appear exactly as listed so `Start-NimbyTestServer.ps1` can collect them.

### 4.3. Logging levels

Enforce Script has no runtime log level. Each addon MUST read one flag file at mission start, `$profile:<Prefix>\debug`, and write debug lines only when it exists. Production logs carry forced clears, refusals, audit records, and failures; never per-toggle or per-frame lines.

## 5. Key bindings

| Mod | Default binding | Status |
| --- | --- | --- |
| Legs | Double-tap Left Shift (`kLShift` with `<event name="doubleclick"/>`) | Decided (A3), revised by Jeff October 15, 2026; replaces numpad plus |
| Hands offer/request | `O` (`kO`) | Decided |
| Hands pocket toggle | Unbound; Controls label "Hands: open or close pockets" | Decided (G2), October 15, 2026 |
| Central Scrutinizer menu | Home, then F10; first verified-free wins; otherwise unbound. End left the list on October 15, 2026 for Ears; Ears moved to N on October 16, so End is free again but stays off the list. Home is free in `bin/preset_X1MouseKey.xml`; F10 is bound (`UAGestureSlot10`, `EmoteThroat`, `UABuldRunScript` at `:446,480,824`), so Home is expected | Decided (N2), revised October 15, 2026 |
| Ears toggle | N (`kN`); `bin/constants.xml:507`; in `bin/preset_X1MouseKey.xml` only as the modifier of the developer brush inputs (`kN` plus wheel, `:881-890`), so a plain press is free | Decided by Jeff (E2), October 16, 2026; was End on October 15 |
| Puff, Buzz, Sprout, Scrawl, Tack | None; action-menu entries only | Decided |

Every NIMBY key binding is player-configurable in game (decided by Jeff, October 16, 2026). Now: every binding is declared in the addon's `Data/Inputs.xml` with a NIMBY group name so it appears in the vanilla Options > Controls page and the player rebinds it there; no script reads a hardcoded key, and every plan's test table has a rebind case (Ears EP-005 is the first proof). Later: a shared `NimbyMenu` addon (display name Menu, RPC block `65510xx`) hosts one NIMBY options page for bindings and other player preferences; addons register their settings with it, and nothing depends on it until it exists.

No two mods share a default binding, and none shadows a vanilla 1.30 default. A double-tap event on a key that vanilla uses as a hold (`kLShift` is `UATurbo`, `preset_X1MouseKey.xml:21-22`) does not shadow the hold; vanilla itself does this with `UAWalkRunToggle` on `kLControl` (`:27-31`). "Verified free" means checked against the unpacked 1.30 `bin/preset_X1MouseKey.xml` (there is no `inputs.xml` in 1.30, see section 7), with the file:line recorded in the plan.

## 6. Save footprint

- No addon MAY override `OnStoreSave` or `OnStoreLoad`, add data to any saved entity, or declare a persistent entity class.
- Net-synced variables are allowed; they are not persisted.
- Per-addon state lives in `$profile:<Prefix>\` as JSON or flag files and MUST be written atomically (temp file, validate, rename) with a `.lkg` copy where the file controls access or admission.
- Adding, updating, or removing any addon MUST NOT change what loads from `storage_1`. The release gate in section 9 proves it.

The rules above apply to utility addons: Legs, Hands, Central Scrutinizer, Buzz, and Ears. Content addons are governed by 6.1. Addons are split by what a Bohemia update is likely to break (decided by Jeff, October 15, 2026): config-heavy content and script-API-heavy behavior ship as separate PBOs, so a broken API disables one PBO and the content stays playable.

### 6.1. Content addons

Decided by Jeff, October 15, 2026: content addons persist like other DayZ items. A *content addon* is one whose purpose is items that live in `storage_1` - Puff, Sprout, Scrawl, and Tack. For a content addon:

- Item classes the addon defines MAY persist the vanilla way. Each plan lists every persistent class it adds.
- The addon MUST NOT override `OnStoreSave` or `OnStoreLoad` on, or add saved data to, any vanilla class (`PlayerBase`, `ItemBase`, `Paper`, and so on). This is the rule that separates a content addon from the October 15 corruption class: removing CF left vanilla classes reading fewer bytes than they had written.
- The addon MAY override `OnStoreSave` and `OnStoreLoad` on a class it defines itself, only with a version guard, and only when the plan records the field list. Removing the addon then removes the class, and the engine drops entities of a class it cannot resolve rather than corrupting the file.
- Player state (effect levels, timers, toggles) MUST NOT be persisted by any mechanism, including a vanilla `ModifierBase` with `IsPersistent()` true (`ModifiersManager.c:237-244`). Net-synced variables and in-memory server state are the only allowed forms.
- A config-only change to a vanilla class (for example a quantity on `Paper`) is allowed when the addon's Phase 0 boots a `storage_1` copy holding existing entities of that class and shows them loading unchanged with 0 "Scripted variables corrupted"; the plan records the field and the evidence.
- Loot rows (`types.xml`, `cfgspawnabletypes.xml`) are drafted in `server/` and applied live by Jeff at a restart with the file backed up; the addon ships no CE files.
- The release gate in 9.2 gains two boots for a content addon: a save-reload round trip of a `storage_1` copy with the addon loaded, and a boot of that saved storage without the addon. The second boot MUST show only the addon's own items missing and 0 "Scripted variables corrupted".

## 7. Shared 1.30 script reference

Before any session writes code, the orchestrator unpacks the 1.30 `scripts.pbo` (DayZ Tools or `D:\GitHub\DayZExtract`) to one read-only folder that all three sessions grep, and records the build number and archive hashes in `docs/baselines/`. No vanilla API is used without a file:line from that folder, and a declaration is not proof of behavior: each plan's Phase 0 exercises the API in game.

Done October 15, 2026: the reference is `vendor/dayz-1.30-scripts/` (read-only, git-ignored), recorded in `docs/baselines/2026-10-15-dayz-1.30-scripts-reference.md` with build `1.30.164078.38` and the `scripts.pbo` hash. There is no `inputs.xml` in 1.30; where a plan says `inputs.xml`, read `bin/preset_X1MouseKey.xml` (PC default bindings) and `bin/constants.xml` (action and button names).

## 8. Test tooling

The tooling below is Plan 0 - shared tooling, owned by the orchestrator and merged to `main` before any worktree branches. Every mod plan's Phase 0 depends on it.

| Change | File | Rule |
| --- | --- | --- |
| Nasdara mission | `pack.json` `bootCheck.mission`, `testServer.mission` | `dayzOffline.nasdara` in both |
| Explicit artifact | `Start-NimbyTestServer.ps1` | Takes `-BuildPath`, or selects only a build whose `build.log` records PASS; never "newest folder by name" |
| Manifest record | both scripts | Each run records the addon list and PBO hashes it ran |
| Reusable profile | `Start-NimbyTestServer.ps1` | `-ProfileName` reuses a designated test profile and its `storage_998` |
| Storage separation | both scripts | Test storage is `storage_998` and `storage_999` only; `storage_1` is never written |
| Storage auto-fix | both scripts | `storageAutoFix = 0` for qualification runs so corruption is visible; `1` is allowed only for the quick compile and boot check |
| Graceful stop | `Start-NimbyTestServer.ps1` | `BattlEye = 1`, a generated `beserver_x64.cfg` with a random RCON password, and an RCON `#shutdown` before any process stop (decided October 15, 2026) |
| Server-mod output | `Build-NimbyPack.ps1` | A second output folder for `-servermod=` PBOs that the Workshop item never contains (section 8.1) |
| `includePatterns` | `pack.json` | Add `*.layout`; a client menu's layout files MUST be inside the completeness check |
| Dev harness | `mods/own/dev/NimbyDevHarness/` | Built only with `-Dev`, signed with a separate dev key, never listed in `pack.json` `addons`; the only place forged-request tests live |
| Self-test | each addon | A `$profile:<Prefix>\selftest` flag runs the addon's engine-free validators at mission start and prints `[NIMBY][<Prefix>][SelfTest] PASS|FAIL <case>`; the boot check fails on any FAIL |

### 8.1. Client and server PBO split

Decided by Jeff, October 15, 2026, for Central Scrutinizer. Server-only code (permissions, bans, audit, webhook, command handlers) ships in a separate PBO loaded with `-servermod=`. Rules:

- No client-shipped script MAY reference a class defined in a server-only PBO.
- The server-only PBO is not signed for clients and is never part of the Workshop item.
- `Build-NimbyPack.ps1` writes it to `build/@NIMBY-Server-<timestamp>/`, and the boot check loads both outputs.
- HostHavoc support for `-servermod=` is verified in Central Scrutinizer Phase 0 before any server-only code is written.

Legs, Hands, Puff, Buzz, Sprout, Scrawl, Tack, and Ears stay single-PBO unless their plan records otherwise. Between client PBOs the same reference rule applies: no Puff script MAY reference a Buzz class, so Buzz can be removed without touching Puff.

### 8.2. Tooling as built (October 15, 2026)

Plan 0 landed the rules above as follows. Addon rows in `pack.json` take an optional `"serverOnly": true`; those build unsigned to `build/@NIMBY-Server-<stamp>/` and the boot check adds `-servermod=`. The dev harness is the `pack.json` `dev` section (never an `addons` row); `Build-NimbyPack.ps1 -Dev` builds it to `build/@NIMBY-Dev-<stamp>/` with the `nimby_dev` key and loads it beside the pack. The client output is byte-identical with or without `-Dev`. `-Qualification` boots with `storageAutoFix = 0`; the default quick check keeps `1`. `build.log` records `SERVERMOD`, `DEV`, `BOOT MODE`, `SELFTEST`, and `SUMMARY PASS|FAIL|UNKNOWN mod=<path>`. `Start-NimbyTestServer.ps1` selects only a `SUMMARY PASS` build unless `-BuildPath` is given, reuses `<repo>/profiles/<name>/` with `-ProfileName`, writes `<profile>/BattlEye/BEServer_x64.cfg`, and sends `#shutdown` through `scripts/modules/NimbyRcon.psm1` before any process stop. Both scripts refuse an `instanceId` other than 998 or 999.

Added October 15, 2026: `scripts/Sync-LiveReplica.ps1` builds the local replica at `D:\DayZServer` from the WinSCP mirror of the HostHavoc server (`D:\HostHavoc\NIMBY Badlands\104.192.227.46_2302\`): binaries from the Steam install (or the mirror with `-BinarySource Mirror`), the live mission, the live `storage_1` copied to `storage_998` (previous copy zipped to `D:\DayZServer\_backups\`), an empty `storage_999` as the fresh world, and the mission's loot and gameplay files into `server/live/`. It never copies `serverDZ*.cfg`, the BattlEye folder, or the live profile folder. `Start-NimbyTestServer.ps1 -InstanceId 998|999` selects the world per run; `-ServerPath D:\DayZServer` points the tooling at the replica; `-Vanilla` boots with no `-mod=` at all and is the same-build control boot that 9.2 compares against. The content-addon boots in 6.1 and 9.2 run against `storage_998` on the replica. First replica run October 15, 2026: 13 OK, 0 failures; the mirror's binaries matched the Steam install file for file (build 25460313), and the HostHavoc FTP view omits `DayZServer_x64.exe` and `keys\`, so Steam stays the binary source. The vanilla control boot of `storage_998` on October 16, 2026 matched the live RPT on every warning class with 0 "Scripted variables corrupted"; the counts are the baseline in `docs/baselines/2026-10-16-replica-vanilla-control-boot.md`.

## 9. Quality gate and release

### 9.1. Quality gate

A change is done only when `Build-NimbyPack.ps1` exits 0 (build, sign, verify, boot check on Nasdara, self-test), its plan's in-game cases pass with the evidence named in the plan, and the test record names the exact artifact.

### 9.2. Release model

Decided by Jeff, October 15, 2026: independent, gated. Each mod ships on its own when it passes. Before any release, including a single-mod release:

1. Boot the release artifact locally against a copy of live `storage_1` with `storageAutoFix = 0`.
2. The RPT MUST show 0 "Scripted variables corrupted", 0 `SCRIPT (E)`, and no new save-load errors against the same-build vanilla control.
3. Back up live mission storage and every `$profile:Nimby*` folder together.
4. Deploy at a scheduled restart. A launch-line change restarts the server, so it lands in the restart window.
5. Retain the previous package and the pre-release backup, and document how clients obtain the matching rollback version.

### 9.3. Distribution

Decided by Jeff, October 15, 2026: one public Steam Workshop item, `@NIMBY`, containing only release-qualified client PBOs. Consequences: every third-party piece keeps its attribution and permission evidence inside the pack and in `NOTICE`; the Hands permission capture is a release blocker; the preview image stays small because it is the only real download-size driver; the pack README's "Not a public Workshop release" line is superseded.

Decided by Jeff, October 17, 2026: the source of every NIMBY mod is public. The Workshop item description links the GitHub repository (or repositories) that hold `mods/own/nimby-pack/`. Consequences: the repository needs a public remote before the first release (none exists today); the repository MUST NOT contain `serverDZ*.cfg` values, BattlEye configuration, the signing private key, or any profile-folder secret (already section 10); third-party notices in `NOTICE` cover the published source as well as the PBOs; a license file is chosen before the remote goes public.

## 10. Guardrails

- No live-server change without Jeff's approval. Never take the server offline outside the restart chain.
- Jeff types RCON passwords.
- Do not reopen CF/COT removal, `lightingConfig` 3, 1PP, or the hardcore profile.
- Never write the armed-vehicle name.
