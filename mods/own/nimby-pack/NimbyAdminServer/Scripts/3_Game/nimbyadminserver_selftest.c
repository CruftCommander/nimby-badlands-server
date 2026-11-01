// -----------------------------------------------------------------------------
// NimbyAdminServer - self-test (shared rules 8; component AdminServer). Engine-free cases for every
// server decision: Steam64, config bounds (permissionsReloadSeconds at every edge, ruling 5), the
// webhook split, permission and ban rows, roles and capabilities (with the fake capability, N9),
// ban expiry and the ruling 3 admission rule, N8 source decisions, ISO time, JSON and Discord
// text, the retry schedule (ruling 7), the token bucket, the duplicate window, teleport args and
// the session match. Lines: [NIMBY][AdminServer][SelfTest] PASS|FAIL <case>.
// Style : one statement per line.
// -----------------------------------------------------------------------------

class NimbyAdminServerSelfTest
{
    static int Run(out array<string> results)
    {
        results = new array<string>;
        int failures = 0;
        failures += IdentityCases(results);
        failures += ConfigCases(results);
        failures += AccessCases(results);
        failures += TextCases(results);
        failures += LimitCases(results);
        return failures;
    }

    protected static int IdentityCases(array<string> results)
    {
        int failures = 0;
        failures += Case(results, "steam64: 17 digits starting 7656 accepted", NimbyAdminServerRules.IsSteam64("76561198354705597"));
        failures += Case(results, "steam64: 16 digits refused", NimbyAdminServerRules.IsSteam64("7656119835470559") == false);
        failures += Case(results, "steam64: 18 digits refused", NimbyAdminServerRules.IsSteam64("765611983547055970") == false);
        failures += Case(results, "steam64: letters refused", NimbyAdminServerRules.IsSteam64("7656119835470559x") == false);
        failures += Case(results, "steam64: a wrong prefix refused", NimbyAdminServerRules.IsSteam64("12345678901234567") == false);
        failures += Case(results, "steam64: BI hash refused", NimbyAdminServerRules.IsSteam64("h8o5rWLPVptWLIPrMHl9JnGQMT0bM8CfAyoaOMMnH5Q=") == false);
        failures += Case(results, "rules: selftest flag lives under $profile:NimbyAdminServer", NimbyAdminServerRules.FILE_SELFTEST.IndexOf("$profile:NimbyAdminServer") == 0);
        return failures;
    }

    protected static bool ReloadBound(int value, int expected)
    {
        NimbyAdminServerConfig config = new NimbyAdminServerConfig();
        config.permissionsReloadSeconds = value;
        array<string> notes;
        config.Normalize(notes);
        return config.permissionsReloadSeconds == expected;
    }

    protected static int ConfigCases(array<string> results)
    {
        int failures = 0;
        failures += Case(results, "config: permissionsReloadSeconds 0 takes the default 5", ReloadBound(0, 5));
        failures += Case(results, "config: permissionsReloadSeconds 1 kept", ReloadBound(1, 1));
        failures += Case(results, "config: permissionsReloadSeconds 300 kept", ReloadBound(300, 300));
        failures += Case(results, "config: permissionsReloadSeconds 301 takes the default 5", ReloadBound(301, 5));
        failures += Case(results, "config: permissionsReloadSeconds -1 takes the default 5", ReloadBound(-1, 5));
        NimbyAdminServerConfig spawn = new NimbyAdminServerConfig();
        spawn.spawnMaxItems = 51;
        spawn.requestsPerSecond = 21;
        spawn.discordQueueMax = 5;
        array<string> spawnNotes;
        spawn.Normalize(spawnNotes);
        failures += Case(results, "config: out-of-range values take defaults and are named", spawn.spawnMaxItems == 50 && spawn.requestsPerSecond == 5 && spawn.discordQueueMax == 200 && spawnNotes.Count() == 3);
        NimbyAdminServerConfig clean = new NimbyAdminServerConfig();
        array<string> cleanNotes;
        clean.Normalize(cleanNotes);
        failures += Case(results, "config: the defaults pass unchanged", cleanNotes.Count() == 0 && clean.HasWebhook() == false);
        NimbyAdminServerConfig bad = new NimbyAdminServerConfig();
        bad.bootstrapOwnerSteam64 = "1234";
        bad.webhookUrl = "https://example.com/hook";
        array<string> badNotes;
        bad.Normalize(badNotes);
        bool noValue = true;
        foreach (string note : badNotes)
        {
            if (note.IndexOf("example.com") >= 0)
            {
                noValue = false;
            }
        }
        failures += Case(results, "config: a bad bootstrap and a non-Discord webhook are cleared, the URL never named", bad.bootstrapOwnerSteam64 == "" && bad.webhookUrl == "" && badNotes.Count() == 2 && noValue);
        string base;
        string path;
        bool split = NimbyAdminServerRules.SplitWebhook("https://discord.com/api/webhooks/123/abc", base, path);
        failures += Case(results, "webhook: a Discord URL splits into host and path", split && base == "https://discord.com/api/webhooks/" && path == "123/abc");
        failures += Case(results, "webhook: discordapp.com is accepted", NimbyAdminServerRules.SplitWebhook("https://discordapp.com/api/webhooks/1/x", base, path));
        failures += Case(results, "webhook: http is refused", NimbyAdminServerRules.SplitWebhook("http://discord.com/api/webhooks/1/x", base, path) == false);
        failures += Case(results, "webhook: a path without a token is refused", NimbyAdminServerRules.SplitWebhook("https://discord.com/api/webhooks/123", base, path) == false);
        failures += Case(results, "webhook: a space is refused", NimbyAdminServerRules.SplitWebhook("https://discord.com/api/webhooks/1/a b", base, path) == false);
        return failures;
    }

    protected static NimbyAdminServerAdmin Admin(string steam64, string role)
    {
        NimbyAdminServerAdmin row = new NimbyAdminServerAdmin();
        row.steam64 = steam64;
        row.role = role;
        return row;
    }

    protected static NimbyAdminServerBan Ban(string steam64, string expires)
    {
        NimbyAdminServerBan row = new NimbyAdminServerBan();
        row.steam64 = steam64;
        row.reason = "test";
        row.actor = "76561198354705597";
        row.createdUtc = "2026-10-31T20:00:00Z";
        row.expiresUtc = expires;
        return row;
    }

    protected static int AccessCases(array<string> results)
    {
        int failures = 0;
        NimbyAdminServerPermissions file = new NimbyAdminServerPermissions();
        file.admins.Insert(Admin("76561198354705597", "owner"));
        file.admins.Insert(Admin("76561190000000001", "admin"));
        file.admins.Insert(Admin("7656119000000000", "admin"));
        file.admins.Insert(Admin("76561190000000002", "superuser"));
        file.admins.Insert(Admin("76561190000000001", "owner"));
        map<string, string> active;
        array<string> notes;
        bool built = file.Build(active, notes);
        failures += Case(results, "permissions: valid rows kept, bad id, bad role and duplicate skipped", built && active.Count() == 2 && notes.Count() == 3 && active.Get("76561190000000001") == "admin");
        failures += Case(results, "permissions: an owner is found", NimbyAdminServerPermissions.HasOwner(active));
        NimbyAdminServerPermissions future = new NimbyAdminServerPermissions();
        future.version = 2;
        map<string, string> futureActive;
        array<string> futureNotes;
        failures += Case(results, "permissions: version 2 refused whole", future.Build(futureActive, futureNotes) == false && futureActive.Count() == 0);
        failures += Case(results, "roles: owner holds permissions.manage", NimbyAdminServerRules.RoleGrants("owner", NimbyAdminServerRules.CAP_PERMISSIONS_MANAGE));
        failures += Case(results, "roles: admin lacks permissions.manage", NimbyAdminServerRules.RoleGrants("admin", NimbyAdminServerRules.CAP_PERMISSIONS_MANAGE) == false);
        failures += Case(results, "roles: admin may teleport and list", NimbyAdminServerRules.RoleGrants("admin", NimbyAdminServerRules.CAP_PLAYER_TELEPORT) && NimbyAdminServerRules.RoleGrants("admin", NimbyAdminServerRules.CAP_PLAYER_LIST));
        failures += Case(results, "roles: none and unknown roles grant nothing", NimbyAdminServerRules.RoleGrants("none", NimbyAdminServerRules.CAP_PLAYER_LIST) == false && NimbyAdminServerRules.RoleGrants("Owner", NimbyAdminServerRules.CAP_PLAYER_LIST) == false);
        failures += Case(results, "roles: an unknown capability is granted to nobody", NimbyAdminServerRules.RoleGrants("owner", "observe.map") == false);
        NimbyAdminServerRules.RegisterCapability("selftest.fake", true);
        bool fakeOwner = NimbyAdminServerRules.RoleGrants("owner", "selftest.fake");
        bool fakeAdmin = NimbyAdminServerRules.RoleGrants("admin", "selftest.fake");
        NimbyAdminServerRules.UnregisterCapability("selftest.fake");
        failures += Case(results, "roles: a fake owner-only capability registers and unregisters (N9 seam)", fakeOwner && !fakeAdmin && NimbyAdminServerRules.RoleGrants("owner", "selftest.fake") == false);
        failures += Case(results, "commands: hello needs no capability, teleport needs player.teleport", NimbyAdminServerRules.CapabilityFor("session.hello") == "" && NimbyAdminServerRules.CapabilityFor("player.teleportTo") == NimbyAdminServerRules.CAP_PLAYER_TELEPORT);
        failures += Case(results, "commands: an unknown command is not known", NimbyAdminServerRules.IsKnownCommand("player.kill") == false && NimbyAdminServerRules.IsKnownCommand("players.list"));
        NimbyAdminServerBans bans = new NimbyAdminServerBans();
        bans.bans.Insert(Ban("76561190000000003", ""));
        bans.bans.Insert(Ban("76561190000000004", "2026-11-01T00:00:00Z"));
        bans.bans.Insert(Ban("76561190000000005", "tomorrow"));
        NimbyAdminServerBan longReason = Ban("76561190000000006", "");
        longReason.reason = NimbySharedChunks.FillBytes("r", 201);
        bans.bans.Insert(longReason);
        map<string, string> banned;
        array<string> banNotes;
        bool bansBuilt = bans.Build(banned, banNotes);
        failures += Case(results, "bans: a bad expiry and a 201-byte reason are skipped", bansBuilt && banned.Count() == 2 && banNotes.Count() == 2);
        failures += Case(results, "bans: a permanent ban holds", NimbyAdminServerBans.IsBanned(banned, "76561190000000003", "2030-01-01T00:00:00Z"));
        failures += Case(results, "bans: a timed ban holds before its expiry", NimbyAdminServerBans.IsBanned(banned, "76561190000000004", "2026-10-31T23:59:59Z"));
        failures += Case(results, "bans: a timed ban ends at its expiry", NimbyAdminServerBans.IsBanned(banned, "76561190000000004", "2026-11-01T00:00:00Z") == false);
        failures += Case(results, "bans: an unlisted id is not banned", NimbyAdminServerBans.IsBanned(banned, "76561198354705597", "2026-10-31T00:00:00Z") == false);
        failures += Case(results, "admission: no usable ban list admits only the permission set (ruling 3)", NimbyAdminServerBans.AdmissionRefused(true, false, "none") && NimbyAdminServerBans.AdmissionRefused(true, false, "admin") == false);
        failures += Case(results, "admission: with a ban list only a ban refuses", NimbyAdminServerBans.AdmissionRefused(false, true, "owner") && NimbyAdminServerBans.AdmissionRefused(false, false, "none") == false);
        failures += Case(results, "n8: a good file is the source and edits are allowed", NimbyAdminServerSource.Decide(true, true, true) == "file" && NimbyAdminServerSource.EditsRefused("file") == false);
        failures += Case(results, "n8: a corrupt reload keeps the set in memory and refuses edits", NimbyAdminServerSource.Decide(false, true, true) == "kept" && NimbyAdminServerSource.EditsRefused("kept"));
        failures += Case(results, "n8: a corrupt first load uses .lkg and refuses edits", NimbyAdminServerSource.Decide(false, false, true) == "lkg" && NimbyAdminServerSource.EditsRefused("lkg"));
        failures += Case(results, "n8: no file and no .lkg is none, never an empty working set", NimbyAdminServerSource.Decide(false, false, false) == "none" && NimbyAdminServerSource.EditsRefused("none"));
        return failures;
    }

    protected static int TextCases(array<string> results)
    {
        int failures = 0;
        failures += Case(results, "time: FormatIsoUtc pads", NimbyAdminServerRules.FormatIsoUtc(2026, 1, 2, 3, 4, 5) == "2026-01-02T03:04:05Z");
        failures += Case(results, "time: a formatted time validates", NimbyAdminServerRules.IsIsoUtc(NimbyAdminServerRules.FormatIsoUtc(2026, 10, 31, 23, 59, 59)));
        failures += Case(results, "time: month 13 refused", NimbyAdminServerRules.IsIsoUtc("2026-13-01T00:00:00Z") == false);
        failures += Case(results, "time: a missing Z refused", NimbyAdminServerRules.IsIsoUtc("2026-10-31T00:00:00+") == false);
        failures += Case(results, "json: quote, backslash and control bytes are escaped", NimbyAdminServerRules.JsonEscape("a\"b\\c\nd") == "a\\\"b\\\\c d");
        NimbyAdminServerAuditRecord record = new NimbyAdminServerAuditRecord();
        record.id = "7";
        record.phase = "intent";
        record.ts = "2026-10-31T20:00:00Z";
        record.actorSteam64 = "76561198354705597";
        record.actorBiId = "BI";
        record.actorName = "Je\"ff";
        record.command = "player.teleportTo";
        string intent = record.ToJson();
        failures += Case(results, "audit: an intent record has no result field", intent.IndexOf("\"result\"") < 0 && intent.IndexOf("{\"id\":\"7\",\"phase\":\"intent\"") == 0);
        failures += Case(results, "audit: the actor name is escaped", intent.IndexOf("\"name\":\"Je\\\"ff\"") > 0);
        record.phase = "result";
        record.result = "denied";
        record.reason = "no role";
        string denied = record.ToJson();
        failures += Case(results, "audit: a result record carries result and reason", denied.IndexOf("\"result\":\"denied\"") > 0 && denied.IndexOf("\"reason\":\"no role\"") > 0);
        NimbyAdminServerJsonObject effects = new NimbyAdminServerJsonObject();
        effects.Add("offset", "behind");
        effects.AddInt("n", 2);
        failures += Case(results, "audit: an effects object is valid JSON text", effects.ToJson() == "{\"offset\":\"behind\",\"n\":2}");
        string payload = NimbyAdminServerRules.DiscordPayload("@everyone hi \"x\"");
        failures += Case(results, "discord: mentions are disabled and text escaped", payload == "{\"content\":\"@everyone hi \\\"x\\\"\",\"allowed_mentions\":{\"parse\":[]}}");
        string longPayload = NimbyAdminServerRules.DiscordPayload(NimbySharedChunks.FillBytes("x", 3000));
        failures += Case(results, "discord: text is cut to 1,900 bytes", longPayload.Length() == 1900 + 46);
        return failures;
    }

    protected static int LimitCases(array<string> results)
    {
        int failures = 0;
        failures += Case(results, "retry: 1 s, 5 s, 30 s, then drop (ruling 7)", NimbyAdminServerRules.RetryDelay(1) == 1.0 && NimbyAdminServerRules.RetryDelay(2) == 5.0 && NimbyAdminServerRules.RetryDelay(3) == 30.0 && NimbyAdminServerRules.RetryDelay(4) < 0);
        NimbyAdminServerBucket bucket = new NimbyAdminServerBucket();
        int taken = 0;
        int n = 0;
        while (n < 50)
        {
            if (bucket.Take(100.0, 5.0, 10))
            {
                taken = taken + 1;
            }
            n = n + 1;
        }
        failures += Case(results, "rate: a burst of 50 in one frame takes 10", taken == 10);
        failures += Case(results, "rate: one token returns after 0.3 s at 5 a second", bucket.Take(100.3, 5.0, 10) && bucket.Take(100.3, 5.0, 10) == false);
        NimbyAdminServerSeen seen = new NimbyAdminServerSeen();
        seen.Remember(5, 10.0);
        failures += Case(results, "dedupe: a repeated id inside the window is seen", seen.Contains(5, 20.0));
        failures += Case(results, "dedupe: an id past the window is not", seen.Contains(5, 10.0 + NimbyAdminServerRules.SEEN_SECONDS) == false);
        int id = 0;
        while (id < NimbyAdminServerRules.SEEN_MAX + 10)
        {
            seen.Remember(1000 + id, 50.0);
            id = id + 1;
        }
        failures += Case(results, "dedupe: the window never holds more than SEEN_MAX ids", seen.Count() <= NimbyAdminServerRules.SEEN_MAX);
        NimbyAdminArgs args;
        string why;
        NimbyAdminArgs.Parse("target=abc;session=4", args, why);
        failures += Case(results, "teleport: target and session pass", NimbyAdminServerRules.CheckTeleportArgs(args) == "");
        NimbyAdminArgs.Parse("target=abc", args, why);
        failures += Case(results, "teleport: a missing session refused", NimbyAdminServerRules.CheckTeleportArgs(args) == "session");
        NimbyAdminArgs.Parse("target=abc;session=x", args, why);
        failures += Case(results, "teleport: a non-numeric session refused", NimbyAdminServerRules.CheckTeleportArgs(args) == "session");
        NimbyAdminArgs.Parse("target=abc;session=4;extra=1", args, why);
        failures += Case(results, "teleport: an extra arg refused", NimbyAdminServerRules.CheckTeleportArgs(args) == "unexpected args");
        failures += Case(results, "session: the current number matches, an old one does not", NimbyAdminServerRules.SessionMatches(4, "4") && NimbyAdminServerRules.SessionMatches(5, "4") == false);
        return failures;
    }

    protected static int Case(array<string> results, string name, bool passed)
    {
        if (passed)
        {
            results.Insert("PASS " + name);
            return 0;
        }
        results.Insert("FAIL " + name);
        return 1;
    }
}
