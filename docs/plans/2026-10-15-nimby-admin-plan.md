# Revised plan 3 — NimbyAdmin

**Draft v0.2 — October 15, 2026**  
**Target:** NIMBY | HARDCORE | BADLANDS | 1pp, Host Havoc, DayZ 1.30.164078 / Nasdara  
**Release:** Independent. Neither Autorun nor Gimme is a prerequisite.

## Purpose and proposed defaults

Provide an in-game administration menu for Jeff and Jay, implemented against vanilla DayZ without CF.

- Scope: player management, spawning, weather/time controls, and announcements.
- Exclude spectate, free camera, ESP, live player maps, arbitrary script execution, and anticheat.
- Retain `NimbyAdmin` as the addon, class prefix, and display name.
- Roles: `owner` can manage permissions; `admin` can use operational commands.
- Bootstrap the first owner through the server profile file.
- Use a rebindable menu input. Check default bindings in this order: Home, End, F10; use the first verified conflict-free choice. If none is free, ship the action unbound with setup instructions.
- Use a profile-based ban list checked against authenticated identity at connection.
- Keep local audit records authoritative; Discord is an asynchronous delivery channel.
- Block state-changing commands when the required local audit record cannot be appended.
- Add no custom entity serialization or mod-dependent persistent entity classes.

These are proposed defaults. Independent release is confirmed by Jeff.

## Phase 0 — Time-boxed feasibility and security gate

This phase proves the architecture. It does not commit the project to the original completion estimate.

### Build and environment

- Record exact engine versions, source revision, and script archive hashes.
- Verify APIs against stable 1.30 and test native behavior.
- Add `.layout` and any actually required menu assets to package inclusion checks.
- Verify resource loading from the signed PBO on a real client.
- Configure build and interactive tests for Nasdara.
- Use a pinned artifact, reusable profile, disposable storage, and graceful shutdown.
- Check Host Havoc profile access and outbound HTTPS feasibility before relying on hosted webhook delivery.

### Minimum vertical slice

Implement:

- Menu shell and player selection.
- Authenticated server router.
- Permission loading and first-owner bootstrap.
- Local audit journal.
- Asynchronous Discord delivery.
- One command: teleport to another player.

### Security contract

Every request must carry a supported protocol version, request identifier, known command, and bounded arguments.

The server must:

- Derive the actor from the authenticated connection.
- Resolve identity using the verified Steam64 API, not a client-supplied ID or display name.
- Authorize every command and sensitive query.
- Resolve targets against the current player session.
- Reject stale, malformed, unknown, unauthorized, and duplicate requests.
- Apply per-sender and aggregate limits before expensive processing.
- Return results only to the appropriate recipient.

Permission updates affect subsequent authorization immediately. Unknown roles and invalid entries grant no access. Malformed files must not silently replace an existing configuration with an empty one.

### Gate tests

Test non-admin access, forged requests, duplicate mutations, permission revocation, malformed payloads, disconnect/reconnect, profile restart persistence, audit-write failure, and webhook failure.

Use isolated test tooling and signing trust; ship no testing backdoors.

**Go/no-go:** Continue only after the slice passes and remaining uncertainties are documented. If it fails, retain Host Havoc panel administration. Reintroducing CF/COT requires a separate compatibility decision and persistence test.

## Phase 1 — Permissions, player management, and recovery

### Permissions

- Support owner-managed role changes.
- Prevent removal or demotion of the last owner through the menu.
- Validate the full proposed configuration before replacing the active one.
- Preserve a last-known-good copy and document offline recovery.
- Audit permission changes and denied attempts.

### Player commands

Provide player list, teleport to, bring, heal, kill, kick, ban/unban, and private message.

- Player lists identify current sessions; selection must not silently retarget after reconnect.
- Teleport and bring reject unsafe player states and invalid destinations. Avoid occupied geometry, unsuitable terrain, and unsafe placement near vehicles.
- Require explicit confirmation for kill and ban.
- Define **Heal** as restoring health, blood, and shock; stopping bleeding; and repairing fractures. Disease, hunger, thirst, temperature, and equipment condition remain separate from this command.
- Ban records contain Steam64, reason, actor, creation time, and optional UTC expiry. Applying a ban to an online player also removes that session.
- Persist permission and ban changes before reporting success.
- Corrupt or unavailable ban data disables admission until corrected; it must not silently become an empty list.
- Document that removing/disabling NimbyAdmin removes its connection-time ban enforcement unless bans have been transferred to another system.

**Exit:** Every command passes authorized, unauthorized, stale-target, and restart tests with Jeff and Jay.

## Phase 2 — Controlled spawning

- Build the catalog from game configuration, but validate classes independently on the server.
- Include only appropriate instantiable item and vehicle classes. Do not assume every config entry is safe to spawn.
- Support inventory, feet, and validated nearby ground placement.
- Default limits: **10 items per request; one vehicle per request**, plus server-side request limits.
- Treat the limit as a maximum, not a promise that every requested object was created.
- Report actual created quantities and partial failures.
- Never silently move failed inventory spawns onto the ground.
- Track entities created by the current request so incomplete construction can be cleaned up safely before success is reported.
- Supply profile-based presets for explicitly supported vehicle classes. Validate parts, fluids, placement, and operability individually.
- Reject unvalidated vehicle presets rather than generating a supposedly complete vehicle from assumptions.

Test occupied space, full inventory, invalid classes, duplicate requests, partial creation failure, and restart persistence.

**Exit:** Supported items and vehicles work after spawning and restart, with accurate audit outcomes.

## Phase 3 — World controls and dependable audit delivery

### World controls

- Provide overcast, rain, fog, wind, time-of-day, and server-wide announcements.
- Bound all values and validate finite numeric input.
- Weather overrides last for the requested duration, then return control to the normal weather system. A newer override replaces the previous one.
- Do not maintain custom weather overrides across restart in v1.
- Set time immediately; subsequent progression follows existing server settings.
- Exclude direct sandstorm control from v1 unless separately specified and verified.

### Local audit

Use structured, correlated records for accepted intent and terminal result. Include:

- Event/request identifier and timestamp with explicit timezone.
- Actor identity and command.
- Target identity/class and relevant positions.
- Validated arguments.
- Result, actual effects, and failure reason.

A crash between intent and completion leaves an unresolved operation; do not claim exactly-once execution or reconstruct success from assumption.

Test unwritable files, disk exhaustion, interrupted writes, rotation, and recovery. Keep denial logging bounded so attackers cannot exhaust storage.

### Discord

- Store the webhook secret only in server profile configuration.
- Use delivery confirmation, escaped text, disabled mentions, and payload limits.
- Use a bounded queue and retries that honor Discord rate-limit responses.
- Distinguish retryable failures from permanent configuration failures.
- Surface queue overflow and delivery failure locally and in the admin UI.
- Never repeat a game command because its Discord notification failed.
- Never include the webhook secret in logs or client synchronization.

**Exit:** Local records remain useful during HTTP failures, and delivery failures cannot stall gameplay or create unlimited work.

## Phase 4 — Hardening and independent release

- Run a command-by-command authorization and malformed-input matrix.
- Test permission changes during active sessions and across restart.
- Test duplicate requests, invalid targets, concurrent admins, and sustained request bursts.
- Exercise every destructive command’s failure and partial-success behavior.
- Run install/update/removal checks on disposable copies of representative world and character storage.
- Compare against the same-build control; investigate all unexplained script and save-load errors.
- Verify normal Workshop installation, stale-client behavior, and packaged menu resources.
- Document setup, permissions, bans, audit failures, backup/restore, and rollback.
- Obtain Jay’s menu usability sign-off and Jeff’s release approval.
- Deploy the independently qualified @NIMBY version at an approved scheduled restart.

Back up mission storage and NimbyAdmin profile data together. Code rollback does not reverse spawned objects or player changes. World restoration is a separate decision because it discards later progress.

## Acceptance, assumptions, and estimate

Release requires:

- Server-authoritative authorization for every command and sensitive query.
- No unexplained script errors or new persistence failures.
- Successful local audit recording before state-changing commands.
- Bounded, non-blocking Discord delivery.
- Passed multiplayer, restart, removal, and rollback tests.
- Verified Workshop distribution and client/server compatibility.

The installed stable scripts are available locally. Remaining external prerequisites are isolated multiplayer testing, hosted HTTPS verification, representative backup data, and a test Discord webhook.

Re-estimate after Phase 0 in engineering effort and tester hours. Treat five-to-six weeks as an initial hypothesis. Do not promise same-day compatibility with future DayZ updates.
