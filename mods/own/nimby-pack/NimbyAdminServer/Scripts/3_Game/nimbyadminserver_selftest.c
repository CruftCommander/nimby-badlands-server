// -----------------------------------------------------------------------------
// NimbyAdminServer - self-test (shared rules 8; component AdminServer). Engine-free cases for every
// server decision: Steam64, config bounds (permissionsReloadSeconds at every edge, ruling 5), the
// webhook split, permission and ban rows, roles and capabilities (with the fake capability, N9),
// ban expiry and the ruling 3 admission rule, N8 source decisions, the store recovery branches
// over an in-memory file fake (review findings 3 and 9), ISO time, JSON and Discord
// text, the retry schedule (ruling 7), the token bucket, the replay window (review finding 1),
// teleport args and the session match. Lines: [NIMBY][AdminServer][SelfTest] PASS|FAIL <case>.
// Style : one statement per line.
// -----------------------------------------------------------------------------

// An in-memory stand-in for the profile files (review finding 3): a path exists when it is in
// m_Files; a path in m_Unreadable exists but cannot be read; notes are collected, not printed.
class NimbyAdminServerFakeIo extends NimbyAdminServerStoreIo
{
    ref map<string, string> m_Files = new map<string, string>;
    ref array<string> m_Unreadable = new array<string>;
    ref array<string> m_Notes = new array<string>;

    override bool Exists(string path)
    {
        return m_Files.Contains(path);
    }

    override bool Read(string path, out string text)
    {
        text = "";
        if (!m_Files.Contains(path) || m_Unreadable.Find(path) >= 0)
        {
            return false;
        }
        text = m_Files.Get(path);
        return true;
    }

    override bool Write(string path, string text)
    {
        m_Files.Set(path, text);
        return true;
    }

    override void Note(string line)
    {
        m_Notes.Insert(line);
    }
}

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
        failures += StoreCases(results);
        return failures;
    }

    protected static int IdentityCases(array<string> results)
    {
        int failures = 0;
        bool pass66 = NimbyAdminServerRules.IsSteam64("76561198354705597");
        failures += Case(results, "steam64: 17 digits starting 7656 accepted", pass66);
        bool pass67 = NimbyAdminServerRules.IsSteam64("7656119835470559") == false;
        failures += Case(results, "steam64: 16 digits refused", pass67);
        bool pass68 = NimbyAdminServerRules.IsSteam64("765611983547055970") == false;
        failures += Case(results, "steam64: 18 digits refused", pass68);
        bool pass69 = NimbyAdminServerRules.IsSteam64("7656119835470559x") == false;
        failures += Case(results, "steam64: letters refused", pass69);
        bool pass70 = NimbyAdminServerRules.IsSteam64("12345678901234567") == false;
        failures += Case(results, "steam64: a wrong prefix refused", pass70);
        bool pass71 = NimbyAdminServerRules.IsSteam64("h8o5rWLPVptWLIPrMHl9JnGQMT0bM8CfAyoaOMMnH5Q=") == false;
        failures += Case(results, "steam64: BI hash refused", pass71);
        bool pass72 = NimbyAdminServerRules.FILE_SELFTEST.IndexOf("$profile:NimbyAdminServer") == 0;
        failures += Case(results, "rules: selftest flag lives under $profile:NimbyAdminServer", pass72);
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
        bool pass88 = ReloadBound(0, 5);
        failures += Case(results, "config: permissionsReloadSeconds 0 takes the default 5", pass88);
        bool pass89 = ReloadBound(1, 1);
        failures += Case(results, "config: permissionsReloadSeconds 1 kept", pass89);
        bool pass90 = ReloadBound(300, 300);
        failures += Case(results, "config: permissionsReloadSeconds 300 kept", pass90);
        bool pass91 = ReloadBound(301, 5);
        failures += Case(results, "config: permissionsReloadSeconds 301 takes the default 5", pass91);
        bool pass92 = ReloadBound(-1, 5);
        failures += Case(results, "config: permissionsReloadSeconds -1 takes the default 5", pass92);
        NimbyAdminServerConfig spawn = new NimbyAdminServerConfig();
        spawn.spawnMaxItems = 51;
        spawn.requestsPerSecond = 21;
        spawn.discordQueueMax = 5;
        array<string> spawnNotes;
        spawn.Normalize(spawnNotes);
        bool pass99 = spawn.spawnMaxItems == 50 && spawn.requestsPerSecond == 5 && spawn.discordQueueMax == 200 && spawnNotes.Count() == 3;
        failures += Case(results, "config: out-of-range values take defaults and are named", pass99);
        NimbyAdminServerConfig clean = new NimbyAdminServerConfig();
        array<string> cleanNotes;
        clean.Normalize(cleanNotes);
        bool pass103 = cleanNotes.Count() == 0 && clean.HasWebhook() == false;
        failures += Case(results, "config: the defaults pass unchanged", pass103);
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
        bool pass117 = bad.bootstrapOwnerSteam64 == "" && bad.webhookUrl == "" && badNotes.Count() == 2 && noValue;
        failures += Case(results, "config: a bad bootstrap and a non-Discord webhook are cleared, the URL never named", pass117);
        string base;
        string path;
        bool split = NimbyAdminServerRules.SplitWebhook("https://discord.com/api/webhooks/123/abc", base, path);
        failures += Case(results, "webhook: a Discord URL splits into host and path", split && base == "https://discord.com/api/webhooks/" && path == "123/abc");
        bool pass122 = NimbyAdminServerRules.SplitWebhook("https://discordapp.com/api/webhooks/1/x", base, path);
        failures += Case(results, "webhook: discordapp.com is accepted", pass122);
        bool pass123 = NimbyAdminServerRules.SplitWebhook("http://discord.com/api/webhooks/1/x", base, path) == false;
        failures += Case(results, "webhook: http is refused", pass123);
        bool pass124 = NimbyAdminServerRules.SplitWebhook("https://discord.com/api/webhooks/123", base, path) == false;
        failures += Case(results, "webhook: a path without a token is refused", pass124);
        bool pass125 = NimbyAdminServerRules.SplitWebhook("https://discord.com/api/webhooks/1/a b", base, path) == false;
        failures += Case(results, "webhook: a space is refused", pass125);
        bool pass126 = NimbyAdminServerRules.WebhookRequestPath("1/abc") == "1/abc?wait=true" && NimbyAdminServerRules.WebhookRequestPath("1/abc?thread_id=9") == "1/abc?thread_id=9&wait=true" && NimbyAdminServerRules.WebhookRequestPath("1/abc?wait=true") == "1/abc?wait=true";
        failures += Case(results, "webhook: wait=true is added at send time, with & after an existing query, never twice", pass126);
        failures += DiscordQueueCases(results);
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
        NimbyAdminServerPermissions file = NimbyAdminServerPermissions.Empty();
        // Phase 1 A4: every value is a local before the call (the 164200 argument-slot rule).
        NimbyAdminServerAdmin owner1 = Admin("76561198354705597", "owner");
        NimbyAdminServerAdmin admin1 = Admin("76561190000000001", "admin");
        NimbyAdminServerAdmin shortId = Admin("7656119000000000", "admin");
        NimbyAdminServerAdmin badRole = Admin("76561190000000002", "superuser");
        NimbyAdminServerAdmin duplicate1 = Admin("76561190000000001", "owner");
        file.admins.Insert(owner1);
        file.admins.Insert(admin1);
        file.admins.Insert(shortId);
        file.admins.Insert(badRole);
        file.admins.Insert(duplicate1);
        map<string, string> active;
        array<string> notes;
        bool built = file.Build(active, notes);
        bool pass168 = built && active.Count() == 2 && notes.Count() == 3 && active.Get("76561190000000001") == "admin";
        failures += Case(results, "permissions: valid rows kept, bad id, bad role and duplicate skipped", pass168);
        bool pass169 = NimbyAdminServerPermissions.HasOwner(active);
        failures += Case(results, "permissions: an owner is found", pass169);
        NimbyAdminServerPermissions future = NimbyAdminServerPermissions.Empty();
        future.version = 2;
        map<string, string> futureActive;
        array<string> futureNotes;
        bool pass174 = future.Build(futureActive, futureNotes) == false && futureActive.Count() == 0;
        failures += Case(results, "permissions: version 2 refused whole", pass174);
        bool pass175 = NimbyAdminServerRules.RoleGrants("owner", NimbyAdminServerRules.CAP_PERMISSIONS_MANAGE);
        failures += Case(results, "roles: owner holds permissions.manage", pass175);
        bool pass176 = NimbyAdminServerRules.RoleGrants("admin", NimbyAdminServerRules.CAP_PERMISSIONS_MANAGE) == false;
        failures += Case(results, "roles: admin lacks permissions.manage", pass176);
        bool pass177 = NimbyAdminServerRules.RoleGrants("admin", NimbyAdminServerRules.CAP_PLAYER_TELEPORT) && NimbyAdminServerRules.RoleGrants("admin", NimbyAdminServerRules.CAP_PLAYER_LIST);
        failures += Case(results, "roles: admin may teleport and list", pass177);
        bool pass178 = NimbyAdminServerRules.RoleGrants("none", NimbyAdminServerRules.CAP_PLAYER_LIST) == false && NimbyAdminServerRules.RoleGrants("Owner", NimbyAdminServerRules.CAP_PLAYER_LIST) == false;
        failures += Case(results, "roles: none and unknown roles grant nothing", pass178);
        bool pass179 = NimbyAdminServerRules.RoleGrants("owner", "observe.map") == false;
        failures += Case(results, "roles: an unknown capability is granted to nobody", pass179);
        NimbyAdminServerRules.RegisterCapability("selftest.fake", true);
        bool fakeOwner = NimbyAdminServerRules.RoleGrants("owner", "selftest.fake");
        bool fakeAdmin = NimbyAdminServerRules.RoleGrants("admin", "selftest.fake");
        NimbyAdminServerRules.UnregisterCapability("selftest.fake");
        bool pass184 = fakeOwner && !fakeAdmin && NimbyAdminServerRules.RoleGrants("owner", "selftest.fake") == false;
        failures += Case(results, "roles: a fake owner-only capability registers and unregisters (N9 seam)", pass184);
        bool pass185 = NimbyAdminServerRules.CapabilityFor("session.hello") == "" && NimbyAdminServerRules.CapabilityFor("player.teleportTo") == NimbyAdminServerRules.CAP_PLAYER_TELEPORT;
        failures += Case(results, "commands: hello needs no capability, teleport needs player.teleport", pass185);
        bool pass186 = NimbyAdminServerRules.IsKnownCommand("player.kill") == false && NimbyAdminServerRules.IsKnownCommand("players.list");
        failures += Case(results, "commands: an unknown command is not known", pass186);
        bool pass187 = NimbyAdminServerRules.CommandForLog("players.list") == "players.list" && NimbyAdminServerRules.CommandForLog("x\n[NIMBY] y") == "<unknown len=11>";
        failures += Case(results, "log: a known command logs as itself, an unknown one only as its length (finding 7)", pass187);
        NimbyAdminServerBans bans = NimbyAdminServerBans.Empty();
        NimbyAdminServerBan permanent = Ban("76561190000000003", "");
        NimbyAdminServerBan dated = Ban("76561190000000004", "2026-11-01T00:00:00Z");
        NimbyAdminServerBan badDate = Ban("76561190000000005", "tomorrow");
        bans.bans.Insert(permanent);
        bans.bans.Insert(dated);
        bans.bans.Insert(badDate);
        NimbyAdminServerBan longReason = Ban("76561190000000006", "");
        longReason.reason = NimbySharedChunks.FillBytes("r", 201);
        bans.bans.Insert(longReason);
        map<string, string> banned;
        array<string> banNotes;
        bool bansBuilt = bans.Build(banned, banNotes);
        bool pass201 = bansBuilt && banned.Count() == 2 && banNotes.Count() == 2;
        failures += Case(results, "bans: a bad expiry and a 201-byte reason are skipped", pass201);
        bool pass202 = NimbyAdminServerBans.IsBanned(banned, "76561190000000003", "2030-01-01T00:00:00Z");
        failures += Case(results, "bans: a permanent ban holds", pass202);
        bool pass203 = NimbyAdminServerBans.IsBanned(banned, "76561190000000004", "2026-10-31T23:59:59Z");
        failures += Case(results, "bans: a timed ban holds before its expiry", pass203);
        bool pass204 = NimbyAdminServerBans.IsBanned(banned, "76561190000000004", "2026-11-01T00:00:00Z") == false;
        failures += Case(results, "bans: a timed ban ends at its expiry", pass204);
        bool pass205 = NimbyAdminServerBans.IsBanned(banned, "76561198354705597", "2026-10-31T00:00:00Z") == false;
        failures += Case(results, "bans: an unlisted id is not banned", pass205);
        bool pass206 = NimbyAdminServerBans.AdmissionRefused(true, false, "none") && NimbyAdminServerBans.AdmissionRefused(true, false, "admin") == false;
        failures += Case(results, "admission: no usable ban list admits only the permission set (ruling 3)", pass206);
        bool pass207 = NimbyAdminServerBans.AdmissionRefused(false, true, "owner") && NimbyAdminServerBans.AdmissionRefused(false, false, "none") == false;
        failures += Case(results, "admission: with a ban list only a ban refuses", pass207);
        bool pass208 = NimbyAdminServerSource.Decide(true, true, true) == "file" && NimbyAdminServerSource.EditsRefused("file") == false;
        failures += Case(results, "n8: a good file is the source and edits are allowed", pass208);
        bool pass209 = NimbyAdminServerSource.Decide(false, true, true) == "kept" && NimbyAdminServerSource.EditsRefused("kept");
        failures += Case(results, "n8: a corrupt reload keeps the set in memory and refuses edits", pass209);
        bool pass210 = NimbyAdminServerSource.Decide(false, false, true) == "lkg" && NimbyAdminServerSource.EditsRefused("lkg");
        failures += Case(results, "n8: a corrupt first load uses .lkg and refuses edits", pass210);
        bool pass211 = NimbyAdminServerSource.Decide(false, false, false) == "none" && NimbyAdminServerSource.EditsRefused("none");
        failures += Case(results, "n8: no file and no .lkg is none, never an empty working set", pass211);
        return failures;
    }

    protected static int TextCases(array<string> results)
    {
        int failures = 0;
        bool pass218 = NimbyAdminServerRules.FormatIsoUtc(2026, 1, 2, 3, 4, 5) == "2026-01-02T03:04:05Z";
        failures += Case(results, "time: FormatIsoUtc pads", pass218);
        string formatted = NimbyAdminServerRules.FormatIsoUtc(2026, 10, 31, 23, 59, 59);
        bool ok210 = NimbyAdminServerRules.IsIsoUtc(formatted);
        failures += Case(results, "time: a formatted time validates", ok210);
        bool pass222 = NimbyAdminServerRules.IsIsoUtc("2026-13-01T00:00:00Z") == false;
        failures += Case(results, "time: month 13 refused", pass222);
        bool pass223 = NimbyAdminServerRules.IsIsoUtc("2026-10-31T00:00:00+") == false;
        failures += Case(results, "time: a missing Z refused", pass223);
        bool pass224 = NimbyAdminServerRules.IsIsoUtc("2027-02-29T00:00:00Z") == false;
        failures += Case(results, "time: February 29 refused in 2027 (finding 10)", pass224);
        bool pass225 = NimbyAdminServerRules.IsIsoUtc("2028-02-29T00:00:00Z");
        failures += Case(results, "time: February 29 accepted in 2028", pass225);
        bool pass226 = NimbyAdminServerRules.IsIsoUtc("2026-04-31T00:00:00Z") == false && NimbyAdminServerRules.IsIsoUtc("2026-12-31T23:59:59Z");
        failures += Case(results, "time: April 31 refused, December 31 accepted", pass226);
        bool pass227 = NimbyAdminServerRules.IsIsoUtc("2100-02-29T00:00:00Z") == false && NimbyAdminServerRules.IsIsoUtc("2000-02-29T00:00:00Z") && NimbyAdminServerRules.IsIsoUtc("2027-02-31T00:00:00Z") == false;
        failures += Case(results, "time: February 29 refused in 2100, accepted in 2000; February 31 refused", pass227);
        bool pass228 = NimbyAdminServerRules.JsonEscape("a\"b\\c\nd") == "a\\\"b\\\\c d";
        failures += Case(results, "json: quote, backslash and control bytes are escaped", pass228);
        NimbyAdminServerAuditRecord record = new NimbyAdminServerAuditRecord();
        record.id = "7";
        record.phase = "intent";
        record.ts = "2026-10-31T20:00:00Z";
        record.actorSteam64 = "76561198354705597";
        record.actorBiId = "BI";
        record.actorName = "Je\"ff";
        record.command = "player.teleportTo";
        string intent = record.ToJson();
        bool pass238 = intent.IndexOf("\"result\"") < 0 && intent.IndexOf("{\"id\":\"7\",\"phase\":\"intent\"") == 0;
        failures += Case(results, "audit: an intent record has no result field", pass238);
        bool pass239 = intent.IndexOf("\"name\":\"Je\\\"ff\"") > 0;
        failures += Case(results, "audit: the actor name is escaped", pass239);
        record.phase = "result";
        record.result = "denied";
        record.reason = "no role";
        string denied = record.ToJson();
        bool pass244 = denied.IndexOf("\"result\":\"denied\"") > 0 && denied.IndexOf("\"reason\":\"no role\"") > 0;
        failures += Case(results, "audit: a result record carries result and reason", pass244);
        NimbyAdminServerJsonObject effects = new NimbyAdminServerJsonObject();
        effects.Add("offset", "behind");
        effects.AddInt("n", 2);
        bool pass248 = effects.ToJson() == "{\"offset\":\"behind\",\"n\":2}";
        failures += Case(results, "audit: an effects object is valid JSON text", pass248);
        bool pass249 = NimbyAdminServerAudit.IsLastRecord(denied, denied) && NimbyAdminServerAudit.IsLastRecord(denied + "\r", denied) && NimbyAdminServerAudit.IsLastRecord(denied + "\r\n", denied);
        failures += Case(results, "audit readback: the record read back matches, also with a CR or LF left on", pass249);
        int deniedBytes = denied.Length();
        string deniedCut = denied.Substring(0, deniedBytes - 1);
        bool otherMatches = NimbyAdminServerAudit.IsLastRecord(intent, denied);
        bool cutMatches = NimbyAdminServerAudit.IsLastRecord(deniedCut, denied);
        bool emptyMatches = NimbyAdminServerAudit.IsLastRecord("", denied);
        bool nothingMatches = NimbyAdminServerAudit.IsLastRecord("", "");
        bool ok239 = !otherMatches && !cutMatches && !emptyMatches && !nothingMatches;
        failures += Case(results, "audit readback: another record, a cut record, or nothing does not match (finding 4)", ok239);
        bool pass258 = denied == DeniedCopy(record);
        failures += Case(results, "audit files: a record marked denied is the same JSON shape as an unmarked one", pass258);
        failures += AuditFileCases(results);
        string payload = NimbyAdminServerRules.DiscordPayload("@everyone hi \"x\"");
        failures += Case(results, "discord: mentions are disabled and text escaped", payload == "{\"content\":\"@everyone hi \\\"x\\\"\",\"allowed_mentions\":{\"parse\":[]}}");
        string longText = NimbySharedChunks.FillBytes("x", 3000);
        string longPayload = NimbyAdminServerRules.DiscordPayload(longText);
        bool pass264 = longPayload.Length() == 1900 + 46;
        failures += Case(results, "discord: text is cut to 1,900 bytes", pass264);
        return failures;
    }

    // Gate-session fix round: the Discord delivery state machine. Before the fix every error
    // class was retried, so a delivered POST that the engine reported through OnError was sent
    // again (teleport 3 posted four times in the gate session); these cases call the new queue
    // class, so they fail before by construction.
    protected static int DiscordQueueCases(array<string> results)
    {
        int failures = 0;
        int id;
        int seq;
        string payload;

        NimbyAdminServerDiscordQueue ok = new NimbyAdminServerDiscordQueue();
        ok.Add("a");
        ok.Add("b");
        bool sentA = ok.NextSend(0, id, seq, payload) && id == 1 && seq == 1 && payload == "a";
        bool blocked = !ok.NextSend(0, id, seq, payload);
        string okOutcome = ok.OnCallback(1, 1, NimbyAdminServerDiscordQueue.KIND_OK, 0);
        bool sentB = ok.NextSend(0, id, seq, payload) && id == 2 && payload == "b";
        failures += Case(results, "discord queue: one POST in flight; a success delivers once and the next message goes", sentA && blocked && okOutcome == NimbyAdminServerDiscordQueue.DELIVERED && sentB);

        NimbyAdminServerDiscordQueue err = new NimbyAdminServerDiscordQueue();
        err.Add("a");
        err.NextSend(0, id, seq, payload);
        string errOutcome = err.OnCallback(1, 1, "other", 0);
        bool pass292 = errOutcome == NimbyAdminServerDiscordQueue.DROPPED && err.Count() == 0 && !err.NextSend(100, id, seq, payload);
        failures += Case(results, "discord queue: a non-timeout error drops the message at once, no retry", pass292);
        string repeat = err.OnCallback(1, 1, "other", 1);
        bool pass294 = repeat == NimbyAdminServerDiscordQueue.IGNORED && err.Count() == 0;
        failures += Case(results, "discord queue: a repeated OnError for a settled message is ignored", pass294);

        NimbyAdminServerDiscordQueue slow = new NimbyAdminServerDiscordQueue();
        slow.Add("a");
        slow.NextSend(0, id, seq, payload);
        string t1 = slow.OnCallback(1, 1, NimbyAdminServerDiscordQueue.KIND_TIMEOUT, 0);
        bool wait1 = !slow.NextSend(0.5, id, seq, payload) && slow.NextSend(1.0, id, seq, payload) && seq == 2;
        string t2 = slow.OnCallback(1, 2, NimbyAdminServerDiscordQueue.KIND_TIMEOUT, 1);
        bool wait2 = !slow.NextSend(5.5, id, seq, payload) && slow.NextSend(6.0, id, seq, payload) && seq == 3;
        string t3 = slow.OnCallback(1, 3, NimbyAdminServerDiscordQueue.KIND_TIMEOUT, 6);
        bool wait3 = !slow.NextSend(35.0, id, seq, payload) && slow.NextSend(36.0, id, seq, payload) && seq == 4;
        string t4 = slow.OnCallback(1, 4, NimbyAdminServerDiscordQueue.KIND_TIMEOUT, 36);
        bool retries = t1 == NimbyAdminServerDiscordQueue.RETRY && t2 == NimbyAdminServerDiscordQueue.RETRY && t3 == NimbyAdminServerDiscordQueue.RETRY;
        bool pass307 = retries && wait1 && wait2 && wait3 && t4 == NimbyAdminServerDiscordQueue.DROPPED && slow.Count() == 0;
        failures += Case(results, "discord queue: a timeout retries after 1 s, 5 s and 30 s, then drops", pass307);

        NimbyAdminServerDiscordQueue late = new NimbyAdminServerDiscordQueue();
        late.Add("a");
        late.NextSend(0, id, seq, payload);
        late.OnCallback(1, 1, NimbyAdminServerDiscordQueue.KIND_TIMEOUT, 0);
        string lateOk = late.OnCallback(1, 1, NimbyAdminServerDiscordQueue.KIND_OK, 0.5);
        bool pass314 = lateOk == NimbyAdminServerDiscordQueue.DELIVERED && late.Count() == 0 && !late.NextSend(10, id, seq, payload);
        failures += Case(results, "discord queue: a late success from a timed-out attempt cancels the retry", pass314);

        NimbyAdminServerDiscordQueue racing = new NimbyAdminServerDiscordQueue();
        racing.Add("a");
        racing.Add("b");
        racing.NextSend(0, id, seq, payload);
        racing.OnCallback(1, 1, NimbyAdminServerDiscordQueue.KIND_TIMEOUT, 0);
        racing.NextSend(1, id, seq, payload);
        string oldOk = racing.OnCallback(1, 1, NimbyAdminServerDiscordQueue.KIND_OK, 1.2);
        bool heldBack = !racing.NextSend(2, id, seq, payload);
        string retryAnswer = racing.OnCallback(1, 2, NimbyAdminServerDiscordQueue.KIND_OK, 2);
        bool nextGoes = racing.NextSend(2, id, seq, payload) && id == 2 && payload == "b";
        failures += Case(results, "discord queue: a late success while the retry is in flight settles once; the retry's answer is ignored", oldOk == NimbyAdminServerDiscordQueue.DELIVERED && heldBack && retryAnswer == NimbyAdminServerDiscordQueue.IGNORED && nextGoes);

        NimbyAdminServerDiscordQueue full = new NimbyAdminServerDiscordQueue();
        full.m_Max = 2;
        bool pass330 = full.Add("a") && full.Add("b") && !full.Add("c") && full.Count() == 2;
        failures += Case(results, "discord queue: a full queue refuses a message", pass330);

        bool pass332 = NimbyAdminServerRules.DiscordFailedLine("other", 13, NimbyAdminServerDiscordQueue.DROPPED, 0) == "discord failed class=other code=13 queued=0; message dropped, not retried" && NimbyAdminServerRules.DiscordFailedLine("timeout", -1, NimbyAdminServerDiscordQueue.RETRY, 1) == "discord failed class=timeout queued=1; retry scheduled";
        failures += Case(results, "discord: the failed line names class and code and says dropped, not retried", pass332);
        return failures;
    }

    protected static string DeniedCopy(NimbyAdminServerAuditRecord record)
    {
        bool was = record.denied;
        record.denied = true;
        string json = record.ToJson();
        record.denied = was;
        return json;
    }

    protected static NimbyAdminServerAuditRecord FileCase(string phase, bool denied)
    {
        NimbyAdminServerAuditRecord record = new NimbyAdminServerAuditRecord();
        record.phase = phase;
        record.ts = "2026-11-01T12:00:00Z";
        record.denied = denied;
        return record;
    }

    // Re-review finding 13: the file-selection rule. Before the fix every record went to the one
    // day file the intent readback reads.
    protected static int AuditFileCases(array<string> results)
    {
        int failures = 0;
        string commandPath = NimbyAdminServerRules.DIR_AUDIT + "\\audit-2026-11-01.log";
        string deniedPath = NimbyAdminServerRules.DIR_AUDIT + "\\audit-denied-2026-11-01.log";
        NimbyAdminServerAuditRecord intent = FileCase("intent", false);
        NimbyAdminServerAuditRecord ran = FileCase("result", false);
        NimbyAdminServerAuditRecord refused = FileCase("result", true);
        NimbyAdminServerAuditRecord summary = FileCase("result", true);
        summary.command = "audit.suppressed";
        NimbyAdminServerAuditRecord markedIntent = FileCase("intent", true);
        string intentStream = NimbyAdminServerAudit.StreamFor(intent);
        string ranStream = NimbyAdminServerAudit.StreamFor(ran);
        string refusedStream = NimbyAdminServerAudit.StreamFor(refused);
        string summaryStream = NimbyAdminServerAudit.StreamFor(summary);
        string intentPath = NimbyAdminServerAudit.PathFor(intent.ts, intentStream);
        string ranPath = NimbyAdminServerAudit.PathFor(ran.ts, ranStream);
        string refusedPath = NimbyAdminServerAudit.PathFor(refused.ts, refusedStream);
        string summaryPath = NimbyAdminServerAudit.PathFor(summary.ts, summaryStream);
        bool ok348 = intentPath == commandPath && ranPath == commandPath;
        bool ok349 = refusedPath == deniedPath && summaryPath == deniedPath;
        failures += Case(results, "audit files: an intent and its executed result go to audit-<date>.log", ok348);
        failures += Case(results, "audit files: a denied or invalid result and a suppression summary go to audit-denied-<date>.log", ok349);
        bool pass379 = NimbyAdminServerAudit.StreamFor(markedIntent) == NimbyAdminServerAudit.STREAM_COMMAND;
        failures += Case(results, "audit files: an intent record always goes to the command file the readback reads", pass379);
        return failures;
    }

    protected static int LimitCases(array<string> results)
    {
        int failures = 0;
        bool pass386 = NimbyAdminServerRules.RetryDelay(1) == 1.0 && NimbyAdminServerRules.RetryDelay(2) == 5.0 && NimbyAdminServerRules.RetryDelay(3) == 30.0 && NimbyAdminServerRules.RetryDelay(4) < 0;
        failures += Case(results, "retry: 1 s, 5 s, 30 s, then drop (ruling 7)", pass386);
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
        bool pass399 = bucket.Take(100.3, 5.0, 10) && bucket.Take(100.3, 5.0, 10) == false;
        failures += Case(results, "rate: one token returns after 0.3 s at 5 a second", pass399);
        NimbyAdminServerBucket flooder = new NimbyAdminServerBucket();
        NimbyAdminServerBucket aggregate = new NimbyAdminServerBucket();
        int admitted = 0;
        int tries = 0;
        while (tries < 100)
        {
            if (NimbyAdminServerRules.Admit(flooder, aggregate, 200.0, 5.0, 10) == "")
            {
                admitted = admitted + 1;
            }
            tries = tries + 1;
        }
        int left = 0;
        while (aggregate.Take(200.0, NimbyAdminServerRules.AGGREGATE_PER_SECOND, NimbyAdminServerRules.AGGREGATE_BURST))
        {
            left = left + 1;
        }
        failures += Case(results, "rate: a flooding sender spends aggregate tokens only for its 10 admitted requests (finding 5)", admitted == 10 && left == NimbyAdminServerRules.AGGREGATE_BURST - 10);
        NimbyAdminServerBucket quiet = new NimbyAdminServerBucket();
        NimbyAdminServerBucket empty = new NimbyAdminServerBucket();
        int drained = 0;
        while (empty.Take(300.0, NimbyAdminServerRules.AGGREGATE_PER_SECOND, NimbyAdminServerRules.AGGREGATE_BURST))
        {
            drained = drained + 1;
        }
        bool pass425 = NimbyAdminServerRules.Admit(quiet, empty, 300.0, 5.0, 10) == NimbyAdminServerRules.LIMIT_AGGREGATE;
        failures += Case(results, "rate: an empty aggregate refuses with the aggregate reason", pass425);
        bool pass426 = NimbyAdminServerRules.ShouldLogRoleSync("", "none") && NimbyAdminServerRules.ShouldLogRoleSync("", "owner");
        failures += Case(results, "hello: the first role sync of a connection is logged (finding 6)", pass426);
        bool pass427 = NimbyAdminServerRules.ShouldLogRoleSync("none", "none") == false && NimbyAdminServerRules.ShouldLogRoleSync("admin", "admin") == false;
        failures += Case(results, "hello: a repeated hello with the same role is not logged", pass427);
        bool pass428 = NimbyAdminServerRules.ShouldLogRoleSync("none", "admin") && NimbyAdminServerRules.ShouldLogRoleSync("owner", "none");
        failures += Case(results, "hello: a role change is logged", pass428);
        bool pass429 = NimbyAdminServerRules.RoleSyncLine("admin", "admin", "who", "file", 3, "hello") == "" && NimbyAdminServerRules.RoleSyncLine("none", "none", "who", "none", 3, "hello") == "";
        failures += Case(results, "hello: a repeated hello with the same role writes no line, debug included (re-review 6)", pass429);
        bool pass430 = NimbyAdminServerRules.RoleSyncLine("", "owner", "who", "file", 3, "hello") == "role sync who role=owner source=file session=3 reason=hello";
        failures += Case(results, "hello: the first push writes the one role sync line", pass430);
        failures += ReplayCases(results);
        NimbyAdminArgs args;
        string why;
        NimbyAdminArgs.Parse("target=abc;session=4", args, why);
        bool pass435 = NimbyAdminServerRules.CheckTeleportArgs(args) == "";
        failures += Case(results, "teleport: target and session pass", pass435);
        NimbyAdminArgs.Parse("target=abc", args, why);
        bool pass437 = NimbyAdminServerRules.CheckTeleportArgs(args) == "session";
        failures += Case(results, "teleport: a missing session refused", pass437);
        NimbyAdminArgs.Parse("target=abc;session=x", args, why);
        bool pass439 = NimbyAdminServerRules.CheckTeleportArgs(args) == "session";
        failures += Case(results, "teleport: a non-numeric session refused", pass439);
        NimbyAdminArgs.Parse("target=abc;session=4;extra=1", args, why);
        bool pass441 = NimbyAdminServerRules.CheckTeleportArgs(args) == "unexpected args";
        failures += Case(results, "teleport: an extra arg refused", pass441);
        bool pass442 = NimbyAdminServerRules.SessionMatches(4, "4") && NimbyAdminServerRules.SessionMatches(5, "4") == false;
        failures += Case(results, "session: the current number matches, an old one does not", pass442);
        return failures;
    }

    // Accepts first to last (inclusive) in order; each must pass Check.
    protected static bool AcceptRange(NimbyAdminServerSeen seen, int first, int last)
    {
        bool ok = true;
        int id = first;
        while (id <= last)
        {
            if (seen.Check(id) != NimbyAdminServerSeen.ACCEPT)
            {
                ok = false;
            }
            seen.Accept(id);
            id = id + 1;
        }
        return ok;
    }

    // Review finding 1 and the re-review: duplicate inside the set, stale below the floor, gaps
    // accepted, a replay after 300 accepted ids refused, the re-review's evicted-id scenario, and
    // the harness ids (highest plus 1,000,000, at most 127 a run) leave the client's next id
    // acceptable.
    protected static int ReplayCases(array<string> results)
    {
        int failures = 0;
        NimbyAdminServerSeen dup = new NimbyAdminServerSeen();
        dup.Accept(5);
        bool pass472 = dup.Check(5) == NimbyAdminServerSeen.DUPLICATE;
        failures += Case(results, "replay: an id inside the set is a duplicate", pass472);
        NimbyAdminServerSeen gap = new NimbyAdminServerSeen();
        gap.Accept(10);
        bool pass475 = gap.Check(7) == NimbyAdminServerSeen.ACCEPT && gap.Highest() == 10;
        failures += Case(results, "replay: a new id below the highest is accepted (gap)", pass475);
        NimbyAdminServerSeen full = new NimbyAdminServerSeen();
        bool filled = AcceptRange(full, 1, NimbyAdminServerRules.SEEN_MAX + 1);
        bool pass478 = filled && full.Count() == NimbyAdminServerRules.SEEN_MAX && full.Floor() == 2;
        failures += Case(results, "replay: the set holds SEEN_MAX ids and the floor is the lowest of them", pass478);
        bool pass479 = full.Check(1) == NimbyAdminServerSeen.STALE;
        failures += Case(results, "replay: an id below the floor of a full set is stale", pass479);
        bool pass480 = full.Check(2) == NimbyAdminServerSeen.DUPLICATE;
        failures += Case(results, "replay: the floor itself is still a duplicate", pass480);
        NimbyAdminServerSeen replay = new NimbyAdminServerSeen();
        bool ran = AcceptRange(replay, 1, 301);
        bool pass483 = ran && replay.Check(1) == NimbyAdminServerSeen.STALE;
        failures += Case(results, "replay: an id replayed after 300 accepted ids is refused", pass483);
        // The re-review's scenario: 1000 first, then 1 through 256. Before the fix 1000 was evicted
        // first (acceptance order) while above the floor, and was accepted again.
        NimbyAdminServerSeen hostile = new NimbyAdminServerSeen();
        hostile.Accept(1000);
        bool low = AcceptRange(hostile, 1, 256);
        bool pass489 = low && hostile.Check(1000) == NimbyAdminServerSeen.DUPLICATE;
        failures += Case(results, "replay: accept 1000 then 1 to 256, a replay of 1000 is a duplicate", pass489);
        bool pass490 = hostile.Floor() == 2 && hostile.Check(1) == NimbyAdminServerSeen.STALE;
        failures += Case(results, "replay: accept 1000 then 1 to 256, a replay of 1 (evicted) is stale", pass490);
        NimbyAdminServerSeen mixed = new NimbyAdminServerSeen();
        bool client = AcceptRange(mixed, 1, 200);
        int base = mixed.Highest() + 1000000;
        bool harness = AcceptRange(mixed, base, base + 126);
        bool pass495 = client && harness && mixed.Count() == NimbyAdminServerRules.SEEN_MAX && mixed.Check(201) == NimbyAdminServerSeen.ACCEPT;
        failures += Case(results, "replay: after 127 harness ids at highest + 1000000 the client's next id is accepted", pass495);
        bool pass496 = mixed.Check(1) == NimbyAdminServerSeen.STALE;
        failures += Case(results, "replay: after the harness ids a client replay below the floor is still stale", pass496);
        bool more = AcceptRange(mixed, 201, 500);
        bool pass498 = more && mixed.Check(base) == NimbyAdminServerSeen.DUPLICATE && mixed.Check(base + 126) == NimbyAdminServerSeen.DUPLICATE && mixed.Check(501) == NimbyAdminServerSeen.ACCEPT;
        failures += Case(results, "replay: harness ids stay in the set after 300 more client ids and replay as duplicates", pass498);
        return failures;
    }

    // ---- stores over the in-memory fake (review findings 3 and 9) -------------------------------
    // Every text below is valid JSON syntax: a syntax error would make the engine print a
    // SCRIPT (E) JSON ERROR line at boot (Greet NIMBY-T-042), which fails the boot check.

    static const string BOOT_A = "76561198354705597";
    static const string OWNER_B = "{\"version\":1,\"admins\":[{\"steam64\":\"76561190000000011\",\"name\":\"B\",\"role\":\"owner\"}]}";
    static const string ADMIN_C = "{\"version\":1,\"admins\":[{\"steam64\":\"76561190000000012\",\"name\":\"C\",\"role\":\"admin\"}]}";
    static const string BAD_V2 = "{\"version\":2,\"admins\":[]}";
    static const string BANS_ONE = "{\"version\":1,\"bans\":[{\"steam64\":\"76561190000000013\",\"reason\":\"x\",\"actor\":\"76561198354705597\",\"createdUtc\":\"2026-10-31T20:00:00Z\",\"expiresUtc\":\"\"}]}";

    protected static NimbyAdminServerPermissionStore PermStore(NimbyAdminServerFakeIo io)
    {
        NimbyAdminServerPermissionStore store = new NimbyAdminServerPermissionStore();
        store.m_Io = io;
        store.m_Bootstrap = BOOT_A;
        return store;
    }

    protected static NimbyAdminServerBanStore BanStore(NimbyAdminServerFakeIo io)
    {
        NimbyAdminServerBanStore store = new NimbyAdminServerBanStore();
        store.m_Io = io;
        return store;
    }

    protected static int StoreCases(array<string> results)
    {
        int failures = 0;
        string problem;
        string why;
        string p = NimbyAdminServerRules.FILE_PERMISSIONS;
        string pl = NimbyAdminServerRules.FILE_PERMISSIONS_LKG;

        NimbyAdminServerFakeIo first = new NimbyAdminServerFakeIo();
        NimbyAdminServerPermissionStore fresh = PermStore(first);
        fresh.Load(0, problem);
        fresh.ApplyBootstrap();
        bool pass539 = fresh.m_Source == "none" && !fresh.m_EditsRefused && fresh.IsFirstInstall() && fresh.RoleOf(BOOT_A) == "owner" && problem == "";
        failures += Case(results, "store: a true first install is source none with edits allowed and the bootstrap owner", pass539);

        NimbyAdminServerFakeIo missing = new NimbyAdminServerFakeIo();
        missing.m_Files.Set(pl, OWNER_B);
        NimbyAdminServerPermissionStore lkg = PermStore(missing);
        lkg.Load(0, problem);
        lkg.ApplyBootstrap();
        bool wrote = lkg.WriteBootstrapFile(why);
        bool pass547 = lkg.m_Source == "lkg" && lkg.m_EditsRefused && lkg.RoleOf("76561190000000011") == "owner" && lkg.RoleOf(BOOT_A) == "none" && !lkg.IsFirstInstall() && !wrote && missing.m_Files.Contains(p) == false;
        failures += Case(results, "store: a missing primary with a .lkg loads the .lkg, refuses edits, and grants no bootstrap", pass547);

        // Re-review finding 3: primary missing, recovered from .lkg, then the backup disappears.
        // Before the fix the reload took the first-install branch: source none, edits allowed,
        // and the bootstrap file could be written.
        NimbyAdminServerFakeIo vanish = new NimbyAdminServerFakeIo();
        vanish.m_Files.Set(pl, OWNER_B);
        NimbyAdminServerPermissionStore recovered = PermStore(vanish);
        recovered.Load(0, problem);
        recovered.ApplyBootstrap();
        bool fromLkg = recovered.m_Source == "lkg" && recovered.m_EditsRefused;
        vanish.m_Files.Remove(pl);
        recovered.Load(10, problem);
        recovered.ApplyBootstrap();
        bool wroteLate = recovered.WriteBootstrapFile(why);
        bool pass562 = fromLkg && recovered.m_Source == "lkg" && recovered.m_EditsRefused && !recovered.IsFirstInstall() && !wroteLate && !vanish.m_Files.Contains(p) && recovered.RoleOf(BOOT_A) == "none" && recovered.RoleOf("76561190000000011") == "owner";
        failures += Case(results, "store: a .lkg recovery whose backup then disappears still refuses edits and never bootstraps", pass562);

        // Third-pass residual of finding 3: an unreadable (or invalid) primary, no .lkg, no prior
        // load, so recovery ends with source none; then the primary is deleted. Before the fix the
        // reload took the first-install branch (edits allowed, bootstrap writable).
        NimbyAdminServerFakeIo blind = new NimbyAdminServerFakeIo();
        blind.m_Files.Set(p, OWNER_B);
        blind.m_Unreadable.Insert(p);
        NimbyAdminServerPermissionStore never = PermStore(blind);
        never.Load(0, problem);
        bool noneFirst = never.m_Source == "none" && never.m_EditsRefused;
        blind.m_Files.Remove(p);
        blind.m_Unreadable.Clear();
        never.Load(10, problem);
        bool wroteBlind = never.WriteBootstrapFile(why);
        bool pass577 = noneFirst && never.m_Source == "none" && never.m_EditsRefused && !never.IsFirstInstall() && !wroteBlind && !blind.m_Files.Contains(p);
        failures += Case(results, "store: an unreadable primary with no .lkg, then deleted, still refuses edits and never bootstraps", pass577);
        NimbyAdminServerFakeIo junk = new NimbyAdminServerFakeIo();
        junk.m_Files.Set(p, BAD_V2);
        NimbyAdminServerPermissionStore invalid = PermStore(junk);
        invalid.Load(0, problem);
        bool invalidFirst = invalid.m_Source == "none" && invalid.m_EditsRefused;
        junk.m_Files.Remove(p);
        invalid.Load(10, problem);
        bool wroteJunk = invalid.WriteBootstrapFile(why);
        bool pass586 = invalidFirst && invalid.m_EditsRefused && !invalid.IsFirstInstall() && !wroteJunk && !junk.m_Files.Contains(p);
        failures += Case(results, "store: an invalid primary with no .lkg, then deleted, still refuses edits and never bootstraps", pass586);

        NimbyAdminServerFakeIo locked = new NimbyAdminServerFakeIo();
        locked.m_Files.Set(p, OWNER_B);
        locked.m_Unreadable.Insert(p);
        locked.m_Files.Set(pl, ADMIN_C);
        NimbyAdminServerPermissionStore unreadable = PermStore(locked);
        unreadable.Load(0, problem);
        unreadable.ApplyBootstrap();
        bool pass595 = unreadable.m_Source == "lkg" && unreadable.m_EditsRefused && unreadable.RoleOf("76561190000000012") == "admin" && problem.IndexOf("unreadable") > 0;
        failures += Case(results, "store: an unreadable primary with a .lkg loads the .lkg and refuses edits", pass595);
        bool pass596 = unreadable.RoleOf(BOOT_A) == "owner" && unreadable.m_EditsRefused;
        failures += Case(results, "store: a recovered set with no owner gets the bootstrap owner only, edits still refused (Q4)", pass596);

        NimbyAdminServerFakeIo reload = new NimbyAdminServerFakeIo();
        reload.m_Files.Set(p, OWNER_B);
        NimbyAdminServerPermissionStore kept = PermStore(reload);
        kept.Load(0, problem);
        bool editableFirst = !kept.m_EditsRefused && kept.m_Source == "file";
        reload.m_Files.Remove(p);
        kept.Load(10, problem);
        bool pass605 = editableFirst && kept.m_Source == "kept" && kept.m_EditsRefused && kept.RoleOf("76561190000000011") == "owner" && problem.IndexOf("missing") > 0;
        failures += Case(results, "store: a reload that cannot read the primary keeps the set and refuses edits (not the old flag)", pass605);
        int notesBefore = reload.m_Notes.Count();
        kept.Load(20, problem);
        bool pass608 = reload.m_Notes.Count() == notesBefore && problem == "" && kept.m_EditsRefused;
        failures += Case(results, "store: a repeated read failure writes no second line", pass608);

        NimbyAdminServerFakeIo repair = new NimbyAdminServerFakeIo();
        repair.m_Files.Set(p, OWNER_B);
        NimbyAdminServerPermissionStore fixedStore = PermStore(repair);
        fixedStore.Load(0, problem);
        repair.m_Files.Set(p, BAD_V2);
        fixedStore.Load(10, problem);
        bool refusedOnBad = fixedStore.m_EditsRefused && fixedStore.m_Source == "kept";
        repair.m_Files.Set(p, OWNER_B);
        fixedStore.Load(20, problem);
        failures += Case(results, "store: restoring the exact last good text clears recovery (finding 9)", refusedOnBad && fixedStore.m_Source == "file" && !fixedStore.m_EditsRefused);
        repair.m_Files.Set(p, BAD_V2);
        fixedStore.Load(30, problem);
        failures += Case(results, "store: after that repair the same bad text refuses edits again", fixedStore.m_Source == "kept" && fixedStore.m_EditsRefused && problem != "");

        NimbyAdminServerFakeIo banMissing = new NimbyAdminServerFakeIo();
        banMissing.m_Files.Set(NimbyAdminServerRules.FILE_BANS_LKG, BANS_ONE);
        NimbyAdminServerBanStore bans = BanStore(banMissing);
        bans.Load(0, problem);
        bool pass628 = bans.m_Source == "lkg" && bans.m_EditsRefused && bans.m_Active.Count() == 1 && !bans.NoUsableList();
        failures += Case(results, "bans: a missing bans.json with a .lkg loads the .lkg and refuses edits", pass628);
        banMissing.m_Files.Remove(NimbyAdminServerRules.FILE_BANS_LKG);
        bans.Load(10, problem);
        bool pass631 = bans.m_Source == "lkg" && bans.m_EditsRefused && bans.m_Active.Count() == 1 && !bans.IsFirstInstall();
        failures += Case(results, "bans: a .lkg recovery whose backup then disappears still refuses edits", pass631);
        NimbyAdminServerFakeIo banLocked = new NimbyAdminServerFakeIo();
        banLocked.m_Files.Set(NimbyAdminServerRules.FILE_BANS, BANS_ONE);
        banLocked.m_Unreadable.Insert(NimbyAdminServerRules.FILE_BANS);
        NimbyAdminServerBanStore noList = BanStore(banLocked);
        noList.Load(0, problem);
        bool pass637 = noList.m_Source == "none" && noList.m_EditsRefused && noList.NoUsableList();
        failures += Case(results, "bans: an unreadable bans.json with no .lkg is no usable list (ruling 3)", pass637);
        failures += ShapeCases(results);
        failures += EngineCases(results);
        return failures;
    }

    // The one engine case (security review of 1180702): the finding 4 readback reads lines with
    // FGets, and a line FGets cut short would block every command. A real audit record stays near
    // 1,000 bytes at most; this writes and reads back 1,500 bytes through the same WriteLine, on a
    // scratch file in this addon's self-test folder, deleted after.
    static const string ENGINE_FILE = "$profile:NimbyAdminServer\\selftest-audit.tmp";

    protected static int EngineCases(array<string> results)
    {
        int failures = 0;
        if (FileExist(ENGINE_FILE))
        {
            DeleteFile(ENGINE_FILE);
        }
        string longLine = "{\"id\":\"" + NimbySharedChunks.FillBytes("x", 1491) + "\"}";
        string first = NimbyAdminServerAudit.WriteLine(ENGINE_FILE, "{\"id\":\"1\"}", true);
        string second = NimbyAdminServerAudit.WriteLine(ENGINE_FILE, longLine, true);
        string third = NimbyAdminServerAudit.WriteLine(ENGINE_FILE, "{\"id\":\"3\"}", true);
        int longBytes = longLine.Length();
        string longName = "engine: an appended line of " + longBytes.ToString() + " bytes reads back whole as the last line";
        bool okLong = first == "" && second == "" && longBytes == 1500;
        failures += Case(results, longName, okLong);
        failures += Case(results, "engine: after it a short line is the last line", third == "");
        DeleteFile(ENGINE_FILE);
        NimbyAdminServerBucket sender = new NimbyAdminServerBucket();
        NimbyAdminServerBucket all = new NimbyAdminServerBucket();
        int kept = 0;
        int n = 0;
        while (n < 100)
        {
            if (NimbyAdminServerRules.DenialRecordAllowed(sender, all, 50.0))
            {
                kept = kept + 1;
            }
            n = n + 1;
        }
        int allLeft = 0;
        while (all.Take(50.0, NimbyAdminServerRules.DENIAL_RECORDS_AGGREGATE_PER_SECOND, NimbyAdminServerRules.DENIAL_RECORDS_AGGREGATE_BURST))
        {
            allLeft = allLeft + 1;
        }
        failures += Case(results, "denial records: one sender keeps 30 and spends 30 of the 120 aggregate", kept == 30 && allLeft == 90);
        bool pass681 = NimbyAdminServerRules.DenialRecordExempt("owner") && NimbyAdminServerRules.DenialRecordExempt("admin") && !NimbyAdminServerRules.DenialRecordExempt("none");
        failures += Case(results, "denial records: an owner or admin is never held back, a player without a role is", pass681);
        NimbyAdminServerSuppressed held = new NimbyAdminServerSuppressed();
        held.Add("76561190000000021", 0);
        held.Add("76561190000000021", 0);
        held.Add("76561190000000021", 0);
        held.Add("76561190000000022", 0);
        string summary = held.EffectsJson();
        bool pass688 = held.Total() == 4 && held.Senders() == 2 && summary.IndexOf("{\"suppressed\":4,\"senders\":2,\"listed\":[{\"steam64\":\"76561190000000021\",\"count\":3}") == 0;
        failures += Case(results, "denial records: held-back records are counted per sender for the summary", pass688);
        NimbyAdminServerSuppressed many = new NimbyAdminServerSuppressed();
        int s = 0;
        while (s < 12)
        {
            int suffix = 10 + s;
            string senderId = "765611900000001" + suffix.ToString();
            many.Add(senderId, 0);
            s = s + 1;
        }
        string manyJson = many.EffectsJson();
        int listedCount = 0;
        int at = manyJson.IndexOf("\"count\"");
        while (at >= 0)
        {
            listedCount = listedCount + 1;
            at = manyJson.IndexOfFrom(at + 1, "\"count\"");
        }
        many.Clear();
        bool pass707 = listedCount == NimbyAdminServerSuppressed.MAX_LISTED && manyJson.IndexOf("\"senders\":12") > 0 && many.Total() == 0 && many.Senders() == 0;
        failures += Case(results, "denial records: the summary lists at most 10 senders and Clear empties it", pass707);
        // Re-review finding 12: the summary state machine. Before the fix the counts were cleared
        // after the append whatever it returned, and the line said "written".
        NimbyAdminServerSuppressed retry = new NimbyAdminServerSuppressed();
        retry.Add("76561190000000031", 100.0);
        retry.Add("76561190000000031", 110.0);
        retry.Add("76561190000000032", 120.0);
        bool pass714 = !retry.Due(159.0) && retry.Due(160.0);
        failures += Case(results, "suppressed: the summary is due 60 s after the first held-back record", pass714);
        string failed = retry.Finish(false, 160.0);
        bool pass716 = failed == NimbyAdminServerSuppressed.DEFERRED && retry.Total() == 3 && retry.Senders() == 2 && !retry.Due(219.0) && retry.Due(220.0);
        failures += Case(results, "suppressed: a failed append keeps the counts and defers to the next due time", pass716);
        retry.Add("76561190000000033", 200.0);
        int retryTotal = retry.Total();
        int retrySenders = retry.Senders();
        string deferredLine = NimbyAdminServerSuppressed.SummaryLine(retryTotal, retrySenders, failed);
        bool pass721 = deferredLine.IndexOf("; summary deferred") > 0 && deferredLine.IndexOf("written") < 0 && deferredLine.IndexOf("suppressed=4 senders=3") > 0;
        failures += Case(results, "suppressed: the line after a failed append says summary deferred, not written", pass721);
        string written = retry.Finish(true, 220.0);
        string writtenLine = NimbyAdminServerSuppressed.SummaryLine(4, 3, written);
        bool pass724 = written == NimbyAdminServerSuppressed.WRITTEN && retry.Total() == 0 && retry.Senders() == 0 && !retry.Due(10000.0) && writtenLine.IndexOf("; summary written") > 0;
        failures += Case(results, "suppressed: a successful append clears the counts and the line says summary written", pass724);
        return failures;
    }

    // Review finding 8: "{}" and {"version":1} are not stores; {"version":1,"admins":[]} is.
    protected static bool ShapeRefused(string text, bool bansStore)
    {
        NimbyAdminServerFakeIo io = new NimbyAdminServerFakeIo();
        string problem;
        if (bansStore)
        {
            io.m_Files.Set(NimbyAdminServerRules.FILE_BANS, BANS_ONE);
            NimbyAdminServerBanStore bans = BanStore(io);
            bans.Load(0, problem);
            io.m_Files.Set(NimbyAdminServerRules.FILE_BANS, text);
            bans.Load(10, problem);
            return bans.m_Source == "kept" && bans.m_EditsRefused && bans.m_Active.Count() == 1 && io.m_Files.Get(NimbyAdminServerRules.FILE_BANS_LKG) == BANS_ONE;
        }
        io.m_Files.Set(NimbyAdminServerRules.FILE_PERMISSIONS, OWNER_B);
        NimbyAdminServerPermissionStore store = PermStore(io);
        store.Load(0, problem);
        io.m_Files.Set(NimbyAdminServerRules.FILE_PERMISSIONS, text);
        store.Load(10, problem);
        return store.m_Source == "kept" && store.m_EditsRefused && store.RoleOf("76561190000000011") == "owner" && io.m_Files.Get(NimbyAdminServerRules.FILE_PERMISSIONS_LKG) == OWNER_B;
    }

    protected static int ShapeCases(array<string> results)
    {
        int failures = 0;
        NimbyAdminServerPermissions blank = new NimbyAdminServerPermissions();
        map<string, string> blankActive;
        array<string> blankNotes;
        bool pass756 = blank.Build(blankActive, blankNotes) == false && blankNotes.Get(0) == "version 0 is not 1";
        failures += Case(results, "shape: a fresh permissions object is not a valid store (sentinels)", pass756);
        bool pass757 = NimbyAdminServerRules.HasTopLevelKey("{\"version\":1,\"admins\":[]}", "admins") && NimbyAdminServerRules.HasTopLevelKey("{ \"admins\" : [ ] }", "admins");
        failures += Case(results, "shape: a top-level key is found, also with spaces", pass757);
        bool pass758 = !NimbyAdminServerRules.HasTopLevelKey("{\"version\":1}", "admins") && !NimbyAdminServerRules.HasTopLevelKey("{\"x\":{\"admins\":[]}}", "admins") && !NimbyAdminServerRules.HasTopLevelKey("{\"name\":\"admins\"}", "admins");
        failures += Case(results, "shape: a missing, nested, or value-only key is not a top-level key", pass758);
        bool pass759 = !NimbyAdminServerRules.HasTopLevelKey("{\"n\":\"a\\\",\\\"admins\\\":[\"}", "admins");
        failures += Case(results, "shape: an escaped quote inside a string does not end it", pass759);
        bool pass760 = ShapeRefused("{}", false);
        failures += Case(results, "shape: permissions {} is refused; the good set and .lkg stay", pass760);
        bool pass761 = ShapeRefused("{\"version\":1}", false);
        failures += Case(results, "shape: permissions {\"version\":1} is refused; the good set and .lkg stay", pass761);
        bool pass762 = ShapeRefused("{}", true);
        failures += Case(results, "shape: bans {} is refused; the good set and .lkg stay", pass762);
        bool pass763 = ShapeRefused("{\"version\":1}", true);
        failures += Case(results, "shape: bans {\"version\":1} is refused; the good set and .lkg stay", pass763);
        NimbyAdminServerFakeIo io = new NimbyAdminServerFakeIo();
        io.m_Files.Set(NimbyAdminServerRules.FILE_PERMISSIONS, "{\"version\":1,\"admins\":[]}");
        NimbyAdminServerPermissionStore empty = PermStore(io);
        string problem;
        empty.Load(0, problem);
        bool pass769 = empty.m_Source == "file" && !empty.m_EditsRefused && empty.m_Active.Count() == 0;
        failures += Case(results, "shape: {\"version\":1,\"admins\":[]} is a valid empty store", pass769);
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
