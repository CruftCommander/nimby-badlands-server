// -----------------------------------------------------------------------------
// NimbyAdminServer - rules: the profile paths, the file shapes, and every engine-free decision
// the server half makes (shared rules 8). No engine call in this file except string methods.
// Plan  : docs/plans/2026-10-15-nimby-admin-plan.md v0.8: N3 (roles), N4, N5, N8 (profile files),
//         N9b (capabilities), the data-format tables; recon sections 4 to 7 and the October 31,
//         2026 rulings (questions 3 to 7). A constant change is a plan change first.
// Reads : the wire definition in NimbyAdmin (3_Game), which also loads on the server; this PBO
//         lists NimbyAdmin in requiredAddons[]. The reverse reference is forbidden (8.1).
// Style : one statement per line.
// -----------------------------------------------------------------------------

class NimbyAdminServerRules
{
    static const string DIR = "$profile:NimbyAdmin";
    static const string DIR_AUDIT = "$profile:NimbyAdmin\\audit";
    static const string FILE_CONFIG = "$profile:NimbyAdmin\\config.json";
    static const string FILE_PERMISSIONS = "$profile:NimbyAdmin\\permissions.json";
    static const string FILE_PERMISSIONS_LKG = "$profile:NimbyAdmin\\permissions.json.lkg";
    static const string FILE_BANS = "$profile:NimbyAdmin\\bans.json";
    static const string FILE_BANS_LKG = "$profile:NimbyAdmin\\bans.json.lkg";
    static const string FILE_DEBUG = "$profile:NimbyAdmin\\debug";
    // The self-test flag is per pack.json prefix (Build-NimbyPack creates <profile>\<Prefix>\selftest).
    static const string FILE_SELFTEST = "$profile:NimbyAdminServer\\selftest";

    // Recon 4.3 step 3: limits applied before any parsing.
    static const int RATE_BURST = 10;
    static const float AGGREGATE_PER_SECOND = 40.0;
    static const int AGGREGATE_BURST = 80;
    // Denied and invalid audit records per sender: 30 a minute (recon 4.3, disk bound). In
    // aggregate over all senders: 120 a minute (security review of 1180702: bounds the day file a
    // finding 4 readback reads; at most about 173,000 such records a day).
    static const float DENIAL_RECORDS_PER_SECOND = 0.5;
    static const int DENIAL_RECORDS_BURST = 30;
    static const float DENIAL_RECORDS_AGGREGATE_PER_SECOND = 2.0;
    static const int DENIAL_RECORDS_AGGREGATE_BURST = 120;
    // Replay window per sender per connection (recon 4.3 step 7; review finding 1, October 31,
    // 2026; re-review November 1): the SEEN_MAX highest accepted ids, no time-based expiry.
    static const int SEEN_MAX = 256;

    // Discord (recon 7, ruling 7).
    static const int DISCORD_TEXT_BYTES = 1900;
    static const string WEBHOOK_HOST_1 = "https://discord.com/api/webhooks/";
    static const string WEBHOOK_HOST_2 = "https://discordapp.com/api/webhooks/";

    // Capabilities (N9b). observe.* are reserved for Phase 6 and 7 and granted to nobody yet.
    static const string CAP_PLAYER_LIST = "player.list";
    static const string CAP_PLAYER_TELEPORT = "player.teleport";
    static const string CAP_PERMISSIONS_MANAGE = "permissions.manage";

    // A Steam64 id is 17 digits and starts with 7656.
    static bool IsSteam64(string value)
    {
        if (value.Length() != 17)
        {
            return false;
        }
        if (value.Substring(0, 4) != "7656")
        {
            return false;
        }
        for (int i = 0; i < 17; i++)
        {
            int c = value.Get(i).ToAscii();
            if (c < 48 || c > 57)
            {
                return false;
            }
        }
        return true;
    }

    static bool IsDigits(string value)
    {
        if (value.Length() == 0 || value.Length() > 9)
        {
            return false;
        }
        int i = 0;
        while (i < value.Length())
        {
            int c = value.Get(i).ToAscii();
            if (c < 48 || c > 57)
            {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    // ---- capabilities ---------------------------------------------------------------------------

    protected static ref array<string> s_Capabilities;
    protected static ref array<string> s_OwnerOnly;

    protected static void EnsureCapabilities()
    {
        if (s_Capabilities)
        {
            return;
        }
        s_Capabilities = new array<string>;
        s_OwnerOnly = new array<string>;
        s_Capabilities.Insert(CAP_PLAYER_LIST);
        s_Capabilities.Insert(CAP_PLAYER_TELEPORT);
        s_Capabilities.Insert(CAP_PERMISSIONS_MANAGE);
        s_OwnerOnly.Insert(CAP_PERMISSIONS_MANAGE);
    }

    // N9b seam: a capability registers with no schema change (the self-test registers a fake one).
    static void RegisterCapability(string capability, bool ownerOnly)
    {
        EnsureCapabilities();
        if (s_Capabilities.Find(capability) < 0)
        {
            s_Capabilities.Insert(capability);
        }
        if (ownerOnly && s_OwnerOnly.Find(capability) < 0)
        {
            s_OwnerOnly.Insert(capability);
        }
    }

    static void UnregisterCapability(string capability)
    {
        EnsureCapabilities();
        int at = s_Capabilities.Find(capability);
        if (at >= 0)
        {
            s_Capabilities.Remove(at);
        }
        int ownerAt = s_OwnerOnly.Find(capability);
        if (ownerAt >= 0)
        {
            s_OwnerOnly.Remove(ownerAt);
        }
    }

    // N3: owner holds every capability; admin every one but the owner-only set; anything else none.
    static bool RoleGrants(string role, string capability)
    {
        EnsureCapabilities();
        if (s_Capabilities.Find(capability) < 0)
        {
            return false;
        }
        if (role == NimbyAdminRules.ROLE_OWNER)
        {
            return true;
        }
        if (role == NimbyAdminRules.ROLE_ADMIN)
        {
            return s_OwnerOnly.Find(capability) < 0;
        }
        return false;
    }

    // The capability a command needs; "" for one every connected player may send.
    static string CapabilityFor(string command)
    {
        if (command == NimbyAdminRules.CMD_PLAYERS)
        {
            return CAP_PLAYER_LIST;
        }
        if (command == NimbyAdminRules.CMD_TELEPORT_TO)
        {
            return CAP_PLAYER_TELEPORT;
        }
        return "";
    }

    static bool IsKnownCommand(string command)
    {
        if (command == NimbyAdminRules.CMD_HELLO)
        {
            return true;
        }
        if (command == NimbyAdminRules.CMD_PLAYERS)
        {
            return true;
        }
        if (command == NimbyAdminRules.CMD_TELEPORT_TO)
        {
            return true;
        }
        return false;
    }

    // Review finding 7: a command for an ordinary log line. A known command is a constant of this
    // file; any other text is never logged, only its length.
    static string CommandForLog(string command)
    {
        if (IsKnownCommand(command))
        {
            return command;
        }
        return "<unknown len=" + command.Length().ToString() + ">";
    }

    // player.teleportTo: target (BI id, 1 to 64 bytes) and session (digits) are required.
    static string CheckTeleportArgs(NimbyAdminArgs args)
    {
        if (!args.Has("target"))
        {
            return "target";
        }
        string target = args.Get("target");
        if (target.Length() < 1 || target.Length() > 64)
        {
            return "target";
        }
        if (!args.Has("session"))
        {
            return "session";
        }
        if (!IsDigits(args.Get("session")))
        {
            return "session";
        }
        if (args.Count() != 2)
        {
            return "unexpected args";
        }
        return "";
    }

    // Recon 4.3 step 10: a selection is valid only for the session it was made in.
    static bool SessionMatches(int current, string requested)
    {
        if (!IsDigits(requested))
        {
            return false;
        }
        return requested.ToInt() == current;
    }

    // ---- limits and log rules -------------------------------------------------------------------

    static const string LIMIT_SENDER = "sender";
    static const string LIMIT_AGGREGATE = "aggregate";

    // Review finding 5 (October 31, 2026): the sender's own bucket first; an aggregate token is
    // spent only for a request the sender's bucket allowed, so one flooding sender cannot drain
    // the aggregate for everyone else. "" admitted, else LIMIT_SENDER or LIMIT_AGGREGATE.
    static string Admit(NimbyAdminServerBucket sender, NimbyAdminServerBucket aggregate, float now, float senderRate, int senderBurst)
    {
        if (!sender.Take(now, senderRate, senderBurst))
        {
            return LIMIT_SENDER;
        }
        if (!aggregate.Take(now, AGGREGATE_PER_SECOND, AGGREGATE_BURST))
        {
            return LIMIT_AGGREGATE;
        }
        return "";
    }

    // The denied and invalid audit-record limit: the sender's bucket first, then the aggregate
    // (the finding 5 order, so one sender cannot spend the aggregate for everyone else). The
    // sender bucket is keyed by BI id in the core, so a reconnect does not refill it; what the
    // limit holds back is counted and summarized (NimbyAdminServerSuppressed).
    static bool DenialRecordAllowed(NimbyAdminServerBucket sender, NimbyAdminServerBucket aggregate, float now)
    {
        if (!sender.Take(now, DENIAL_RECORDS_PER_SECOND, DENIAL_RECORDS_BURST))
        {
            return false;
        }
        return aggregate.Take(now, DENIAL_RECORDS_AGGREGATE_PER_SECOND, DENIAL_RECORDS_AGGREGATE_BURST);
    }

    // An actor who holds a role is never held back by the denial-record limits: a denied command
    // from an owner or admin is the record that matters most, and the sender rate already bounds
    // it (security review of 84a6fca).
    static bool DenialRecordExempt(string role)
    {
        return NimbyAdminRules.IsKnownRole(role);
    }

    // Review finding 6: the role sync line is written once per connection (the session's first
    // push) and again only when the role changes; the role reply itself is always sent.
    static bool ShouldLogRoleSync(string lastPushedRole, string role)
    {
        if (lastPushedRole == "")
        {
            return true;
        }
        return lastPushedRole != role;
    }

    // Re-review finding 6: the only line a role push writes, or "" for a repeated hello with the
    // same role (no debug line either; the gated line and the role reply are enough).
    static string RoleSyncLine(string lastPushedRole, string role, string who, string source, int session, string reason)
    {
        if (!ShouldLogRoleSync(lastPushedRole, role))
        {
            return "";
        }
        return "role sync " + who + " role=" + role + " source=" + source + " session=" + session.ToString() + " reason=" + reason;
    }

    // ---- time -----------------------------------------------------------------------------------

    static string Pad2(int value)
    {
        if (value < 10)
        {
            return "0" + value.ToString();
        }
        return value.ToString();
    }

    static string FormatIsoUtc(int year, int month, int day, int hour, int minute, int second)
    {
        return year.ToString() + "-" + Pad2(month) + "-" + Pad2(day) + "T" + Pad2(hour) + ":" + Pad2(minute) + ":" + Pad2(second) + "Z";
    }

    // Exactly YYYY-MM-DDTHH:MM:SSZ; strings in this form order the same as the times they name.
    static bool IsIsoUtc(string value)
    {
        if (value.Length() != 20)
        {
            return false;
        }
        int i = 0;
        while (i < 20)
        {
            string ch = value.Get(i);
            bool ok = true;
            if (i == 4 || i == 7)
            {
                ok = ch == "-";
            }
            else if (i == 10)
            {
                ok = ch == "T";
            }
            else if (i == 13 || i == 16)
            {
                ok = ch == ":";
            }
            else if (i == 19)
            {
                ok = ch == "Z";
            }
            else
            {
                int c = ch.ToAscii();
                ok = c >= 48 && c <= 57;
            }
            if (!ok)
            {
                return false;
            }
            i = i + 1;
        }
        int year = value.Substring(0, 4).ToInt();
        int month = value.Substring(5, 2).ToInt();
        int day = value.Substring(8, 2).ToInt();
        int hour = value.Substring(11, 2).ToInt();
        int minute = value.Substring(14, 2).ToInt();
        int second = value.Substring(17, 2).ToInt();
        if (month < 1 || month > 12 || day < 1)
        {
            return false;
        }
        // Review finding 10: the day must exist in that month of that year.
        if (day > DaysInMonth(year, month))
        {
            return false;
        }
        if (hour > 23 || minute > 59 || second > 59)
        {
            return false;
        }
        return true;
    }

    // Gregorian: a year divisible by 4 is a leap year, except centuries not divisible by 400.
    static bool IsLeapYear(int year)
    {
        if (year % 400 == 0)
        {
            return true;
        }
        if (year % 100 == 0)
        {
            return false;
        }
        return year % 4 == 0;
    }

    static int DaysInMonth(int year, int month)
    {
        if (month == 2)
        {
            if (IsLeapYear(year))
            {
                return 29;
            }
            return 28;
        }
        if (month == 4 || month == 6 || month == 9 || month == 11)
        {
            return 30;
        }
        return 31;
    }

    // ---- JSON text the server writes (never parses from a client) -------------------------------

    // Finding 8: true when key is a key of the outermost JSON object of text. Strings (with their
    // escapes) are skipped, so a value that spells the key or a key of a nested object does not
    // count. Engine-free; only for text that already parsed as JSON.
    static bool HasTopLevelKey(string text, string key)
    {
        int depth = 0;
        bool inString = false;
        bool escaped = false;
        int stringStart = -1;
        int i = 0;
        int length = text.Length();
        while (i < length)
        {
            string ch = text.Get(i);
            if (inString)
            {
                if (escaped)
                {
                    escaped = false;
                }
                else if (ch == "\\")
                {
                    escaped = true;
                }
                else if (ch == "\"")
                {
                    inString = false;
                    if (depth == 1 && IsKeyAt(text, i + 1) && text.Substring(stringStart, i - stringStart) == key)
                    {
                        return true;
                    }
                }
            }
            else if (ch == "\"")
            {
                inString = true;
                stringStart = i + 1;
            }
            else if (ch == "{" || ch == "[")
            {
                depth = depth + 1;
            }
            else if (ch == "}" || ch == "]")
            {
                depth = depth - 1;
            }
            i = i + 1;
        }
        return false;
    }

    // True when the next byte after optional whitespace at from is ':'.
    protected static bool IsKeyAt(string text, int from)
    {
        int i = from;
        while (i < text.Length())
        {
            string ch = text.Get(i);
            if (ch == ":")
            {
                return true;
            }
            int c = ch.ToAscii();
            if (c != 32 && c != 9 && c != 10 && c != 13)
            {
                return false;
            }
            i = i + 1;
        }
        return false;
    }

    static string JsonEscape(string text)
    {
        string result = "";
        int i = 0;
        while (i < text.Length())
        {
            string ch = text.Get(i);
            int c = ch.ToAscii();
            if (ch == "\\")
            {
                result = result + "\\\\";
            }
            else if (ch == "\"")
            {
                result = result + "\\\"";
            }
            else if (c >= 0 && c < 32)
            {
                result = result + " ";
            }
            else
            {
                result = result + ch;
            }
            i = i + 1;
        }
        return result;
    }

    static string JsonString(string text)
    {
        return "\"" + JsonEscape(text) + "\"";
    }

    // Discord: escaped, mentions disabled, cut to DISCORD_TEXT_BYTES on a character boundary.
    static string DiscordPayload(string text)
    {
        string cut = NimbyAdminRules.CutBytes(text, DISCORD_TEXT_BYTES);
        return "{\"content\":" + JsonString(cut) + ",\"allowed_mentions\":{\"parse\":[]}}";
    }

    // Ruling 7: every error class and timeout retries after 1 s, 5 s, 30 s, then drops (-1).
    static float RetryDelay(int failedAttempts)
    {
        if (failedAttempts == 1)
        {
            return 1.0;
        }
        if (failedAttempts == 2)
        {
            return 5.0;
        }
        if (failedAttempts == 3)
        {
            return 30.0;
        }
        return -1.0;
    }

    // Gate-session fix round (orchestrator ruling, November 1, 2026): the POST path asks Discord
    // to answer 200 with a body (wait=true) instead of 204 with none. Added at send time, never
    // stored; joined with & when the stored URL already carries a query.
    static string WebhookRequestPath(string path)
    {
        if (path.IndexOf("wait=") >= 0)
        {
            return path;
        }
        if (path.IndexOf("?") >= 0)
        {
            return path + "&wait=true";
        }
        return path + "?wait=true";
    }

    // The one local line for a failed attempt; names the class and the engine code, never the URL.
    static string DiscordFailedLine(string kind, int errorCode, string outcome, int queued)
    {
        string line = "discord failed class=" + kind;
        if (errorCode >= 0)
        {
            line = line + " code=" + errorCode.ToString();
        }
        line = line + " queued=" + queued.ToString();
        if (outcome == NimbyAdminServerDiscordQueue.RETRY)
        {
            return line + "; retry scheduled";
        }
        return line + "; message dropped, not retried";
    }

    // The webhook URL split for RestApi.GetRestContext (base) and POST (path). False for anything
    // that is not a Discord webhook. The URL itself is never logged (N5).
    static bool SplitWebhook(string url, out string base, out string path)
    {
        base = "";
        path = "";
        string host = "";
        if (url.IndexOf(WEBHOOK_HOST_1) == 0)
        {
            host = WEBHOOK_HOST_1;
        }
        else if (url.IndexOf(WEBHOOK_HOST_2) == 0)
        {
            host = WEBHOOK_HOST_2;
        }
        if (host == "")
        {
            return false;
        }
        string rest = url.Substring(host.Length(), url.Length() - host.Length());
        if (rest.Length() < 3 || rest.IndexOf(" ") >= 0 || rest.IndexOf("/") <= 0)
        {
            return false;
        }
        base = host;
        path = rest;
        return true;
    }
}

// ---- file shapes (plan, Data formats) ----------------------------------------------------------

class NimbyAdminServerConfig
{
    int version = 1;
    string bootstrapOwnerSteam64 = "";
    string webhookUrl = "";
    int spawnMaxItems = 50;
    int spawnMaxVehicles = 1;
    int requestsPerSecond = 5;
    int permissionsReloadSeconds = 5;
    int discordQueueMax = 200;
    int auditRetentionDays = 90;
    int restrainTimeoutMinutes = 10;
    int vehicleRegistryRefreshSeconds = 60;
    int densityTileMeters = 200;
    int densityRefreshSeconds = 300;
    int densityAlertObjects = 400;
    int densityPlayerAlertObjects = 400;

    // Out-of-range values take the default; each correction names its field in notes. The webhook
    // value is never written to notes, only that it was refused.
    void Normalize(out array<string> notes)
    {
        notes = new array<string>;
        spawnMaxItems = Bound("spawnMaxItems", spawnMaxItems, 1, 50, 50, notes);
        spawnMaxVehicles = Bound("spawnMaxVehicles", spawnMaxVehicles, 0, 1, 1, notes);
        requestsPerSecond = Bound("requestsPerSecond", requestsPerSecond, 1, 20, 5, notes);
        permissionsReloadSeconds = Bound("permissionsReloadSeconds", permissionsReloadSeconds, 1, 300, 5, notes);
        discordQueueMax = Bound("discordQueueMax", discordQueueMax, 10, 1000, 200, notes);
        auditRetentionDays = Bound("auditRetentionDays", auditRetentionDays, 1, 3650, 90, notes);
        restrainTimeoutMinutes = Bound("restrainTimeoutMinutes", restrainTimeoutMinutes, 1, 120, 10, notes);
        vehicleRegistryRefreshSeconds = Bound("vehicleRegistryRefreshSeconds", vehicleRegistryRefreshSeconds, 10, 3600, 60, notes);
        densityTileMeters = Bound("densityTileMeters", densityTileMeters, 50, 2000, 200, notes);
        densityRefreshSeconds = Bound("densityRefreshSeconds", densityRefreshSeconds, 30, 3600, 300, notes);
        densityAlertObjects = Bound("densityAlertObjects", densityAlertObjects, 10, 100000, 400, notes);
        densityPlayerAlertObjects = Bound("densityPlayerAlertObjects", densityPlayerAlertObjects, 10, 100000, 400, notes);
        if (bootstrapOwnerSteam64 != "" && !NimbyAdminServerRules.IsSteam64(bootstrapOwnerSteam64))
        {
            bootstrapOwnerSteam64 = "";
            notes.Insert("bootstrapOwnerSteam64 is not a Steam64 id; no bootstrap owner");
        }
        string base;
        string path;
        if (webhookUrl != "" && !NimbyAdminServerRules.SplitWebhook(webhookUrl, base, path))
        {
            webhookUrl = "";
            notes.Insert("webhookUrl is not a Discord webhook URL; Discord delivery off");
        }
    }

    protected static int Bound(string field, int value, int low, int high, int fallback, array<string> notes)
    {
        if (value < low || value > high)
        {
            notes.Insert(field + " out of range " + low.ToString() + " to " + high.ToString() + "; default " + fallback.ToString() + " used");
            return fallback;
        }
        return value;
    }

    bool HasWebhook()
    {
        return webhookUrl != "";
    }
}

class NimbyAdminServerAdmin
{
    string steam64;
    string name;
    string role;
}

// Review finding 8 (October 31, 2026): a key missing from the file keeps its initializer (Tack
// TK-F9 d, October 24, 2026), so version starts at the sentinel 0 and "{}" is refused. The
// collection cannot use a null sentinel: on 164200 the serializer allocates the array even when
// its key is absent (this seat's boot, October 31, 2026: {"version":1} was adopted with admins
// null-initialized). So the store also requires the collection key on the text itself
// (NimbyAdminServerRules.HasTopLevelKey) before it adopts a file or copies it to .lkg; Build
// still refuses a null collection.
class NimbyAdminServerPermissions
{
    int version = 0;
    ref array<ref NimbyAdminServerAdmin> admins;

    // A valid empty file, for the writers (bootstrap) and the self-test.
    static NimbyAdminServerPermissions Empty()
    {
        NimbyAdminServerPermissions data = new NimbyAdminServerPermissions();
        data.version = 1;
        data.admins = new array<ref NimbyAdminServerAdmin>;
        return data;
    }

    // The active set (Steam64 to role). Invalid and duplicate rows are skipped and named in notes;
    // they grant nothing. False when the file as a whole is unusable (version not exactly 1, or
    // no admins collection); the first note then names why.
    bool Build(out map<string, string> active, out array<string> notes)
    {
        active = new map<string, string>;
        notes = new array<string>;
        if (version != 1)
        {
            notes.Insert("version " + version.ToString() + " is not 1");
            return false;
        }
        if (!admins)
        {
            notes.Insert("admins missing");
            return false;
        }
        int i = 0;
        while (i < admins.Count())
        {
            NimbyAdminServerAdmin row = admins.Get(i);
            if (!row)
            {
                notes.Insert("row " + i.ToString() + " empty; skipped");
            }
            else if (!NimbyAdminServerRules.IsSteam64(row.steam64))
            {
                notes.Insert("row " + i.ToString() + " steam64 invalid; skipped");
            }
            else if (!NimbyAdminRules.IsKnownRole(row.role))
            {
                notes.Insert("row " + i.ToString() + " role invalid; skipped");
            }
            else if (active.Contains(row.steam64))
            {
                notes.Insert("row " + i.ToString() + " duplicate steam64; skipped");
            }
            else
            {
                active.Insert(row.steam64, row.role);
            }
            i = i + 1;
        }
        return true;
    }

    static bool HasOwner(map<string, string> active)
    {
        foreach (string id, string role : active)
        {
            if (role == NimbyAdminRules.ROLE_OWNER)
            {
                return true;
            }
        }
        return false;
    }
}

class NimbyAdminServerBan
{
    string steam64;
    string reason;
    string actor;
    string createdUtc;
    string expiresUtc;
}

// Finding 8: sentinels as for NimbyAdminServerPermissions.
class NimbyAdminServerBans
{
    int version = 0;
    ref array<ref NimbyAdminServerBan> bans;

    static NimbyAdminServerBans Empty()
    {
        NimbyAdminServerBans data = new NimbyAdminServerBans();
        data.version = 1;
        data.bans = new array<ref NimbyAdminServerBan>;
        return data;
    }

    bool Build(out map<string, string> active, out array<string> notes)
    {
        active = new map<string, string>;
        notes = new array<string>;
        if (version != 1)
        {
            notes.Insert("version " + version.ToString() + " is not 1");
            return false;
        }
        if (!bans)
        {
            notes.Insert("bans missing");
            return false;
        }
        int i = 0;
        while (i < bans.Count())
        {
            NimbyAdminServerBan row = bans.Get(i);
            string why = CheckRow(row);
            if (why != "")
            {
                notes.Insert("row " + i.ToString() + " " + why + "; skipped");
            }
            else if (!active.Contains(row.steam64))
            {
                active.Insert(row.steam64, row.expiresUtc);
            }
            i = i + 1;
        }
        return true;
    }

    static string CheckRow(NimbyAdminServerBan row)
    {
        if (!row)
        {
            return "empty";
        }
        if (!NimbyAdminServerRules.IsSteam64(row.steam64))
        {
            return "steam64 invalid";
        }
        if (row.reason.Length() < 1 || row.reason.Length() > 200)
        {
            return "reason length";
        }
        if (!NimbyAdminServerRules.IsSteam64(row.actor))
        {
            return "actor invalid";
        }
        if (!NimbyAdminServerRules.IsIsoUtc(row.createdUtc))
        {
            return "createdUtc invalid";
        }
        if (row.expiresUtc != "" && !NimbyAdminServerRules.IsIsoUtc(row.expiresUtc))
        {
            return "expiresUtc invalid";
        }
        return "";
    }

    // active maps Steam64 to expiresUtc ("" for permanent); nowUtc in the IsIsoUtc form.
    static bool IsBanned(map<string, string> active, string steam64, string nowUtc)
    {
        string expires;
        if (!active.Find(steam64, expires))
        {
            return false;
        }
        if (expires == "")
        {
            return true;
        }
        return expires > nowUtc;
    }

    // Ruling 3 (October 31, 2026): a corrupt bans.json with no .lkg refuses admission to everyone
    // except the active permission set. Enforcement at connection is Phase 1 (plan line 4).
    static bool AdmissionRefused(bool noUsableBanList, bool banned, string role)
    {
        if (noUsableBanList)
        {
            return !NimbyAdminRules.IsKnownRole(role);
        }
        return banned;
    }
}

// ---- limits --------------------------------------------------------------------------------------

// A token bucket: rate tokens a second up to burst; Take spends one.
class NimbyAdminServerBucket
{
    float m_Tokens = -1.0;
    float m_Last;

    bool Take(float now, float rate, int burst)
    {
        if (m_Tokens < 0)
        {
            m_Tokens = burst;
            m_Last = now;
        }
        float elapsed = now - m_Last;
        if (elapsed > 0)
        {
            m_Tokens = m_Tokens + elapsed * rate;
            if (m_Tokens > burst)
            {
                m_Tokens = burst;
            }
            m_Last = now;
        }
        if (m_Tokens < 1.0)
        {
            return false;
        }
        m_Tokens = m_Tokens - 1.0;
        return true;
    }
}

// The replay window of one sender on one connection (review finding 1, October 31, 2026;
// re-review, November 1). It keeps the SEEN_MAX highest accepted ids, the highest of them, and
// the floor: the lowest id in that set once the set is full. An id in the set is a duplicate. An
// id below the floor of a full set is stale. Any other id is accepted, also below the highest (a
// gap), which lets the dev harness forge above a real client's ids. Eviction removes the lowest
// id, so an evicted id is always below the new floor and can never be accepted again; the floor
// only rises. No time-based expiry.
class NimbyAdminServerSeen
{
    static const string ACCEPT = "";
    static const string DUPLICATE = "duplicate";
    static const string STALE = "stale";

    protected ref array<int> m_Ids = new array<int>;
    protected ref map<int, bool> m_Set = new map<int, bool>;
    protected int m_Highest;
    protected int m_Floor;

    // ACCEPT, DUPLICATE or STALE; changes nothing.
    string Check(int requestId)
    {
        if (m_Set.Contains(requestId))
        {
            return DUPLICATE;
        }
        if (m_Ids.Count() >= NimbyAdminServerRules.SEEN_MAX && requestId < m_Floor)
        {
            return STALE;
        }
        return ACCEPT;
    }

    // Records an id Check accepted; evicts the lowest id while the set is over SEEN_MAX.
    void Accept(int requestId)
    {
        m_Ids.Insert(requestId);
        m_Set.Set(requestId, true);
        if (requestId > m_Highest)
        {
            m_Highest = requestId;
        }
        while (m_Ids.Count() > NimbyAdminServerRules.SEEN_MAX)
        {
            int at = LowestIndex();
            int gone = m_Ids.Get(at);
            // Order does not matter, so the unordered Remove (EnScript.c:463).
            m_Ids.Remove(at);
            m_Set.Remove(gone);
        }
        if (m_Ids.Count() >= NimbyAdminServerRules.SEEN_MAX)
        {
            m_Floor = m_Ids.Get(LowestIndex());
        }
    }

    int Highest()
    {
        return m_Highest;
    }

    // 0 until the set is full.
    int Floor()
    {
        return m_Floor;
    }

    int Count()
    {
        return m_Ids.Count();
    }

    protected int LowestIndex()
    {
        int best = 0;
        int i = 1;
        while (i < m_Ids.Count())
        {
            if (m_Ids.Get(i) < m_Ids.Get(best))
            {
                best = i;
            }
            i = i + 1;
        }
        return best;
    }
}

// The denied and invalid records the limits held back since the last summary (security review
// of 84a6fca): counted per Steam64, written as one audit record at most once a minute, so the
// suppression is itself on the record. Engine-free; self-tested.
class NimbyAdminServerSuppressed
{
    static const int MAX_LISTED = 10;
    static const float SUMMARY_SECONDS = 60.0;

    static const string WRITTEN = "written";
    static const string DEFERRED = "deferred";

    protected ref map<string, int> m_Counts = new map<string, int>;
    protected ref array<string> m_Order = new array<string>;
    protected int m_Total;
    protected float m_DueAt;

    // now: the tick time; the first held-back record sets the summary due SUMMARY_SECONDS later.
    void Add(string steam64, float now)
    {
        if (m_Total == 0)
        {
            m_DueAt = now + SUMMARY_SECONDS;
        }
        int count = 0;
        if (m_Counts.Find(steam64, count))
        {
            m_Counts.Set(steam64, count + 1);
        }
        else
        {
            m_Counts.Set(steam64, 1);
            m_Order.Insert(steam64);
        }
        m_Total = m_Total + 1;
    }

    int Total()
    {
        return m_Total;
    }

    int Senders()
    {
        return m_Order.Count();
    }

    void Clear()
    {
        m_Counts.Clear();
        m_Order.Clear();
        m_Total = 0;
    }

    bool Due(float now)
    {
        return m_Total > 0 && now >= m_DueAt;
    }

    // Re-review finding 12: called after the summary append. The counts are cleared only when the
    // append returned true; on failure they are kept (new ones keep adding) and the summary is
    // retried at the next due time, SUMMARY_SECONDS later. Returns WRITTEN or DEFERRED.
    string Finish(bool appended, float now)
    {
        if (appended)
        {
            Clear();
            return WRITTEN;
        }
        m_DueAt = now + SUMMARY_SECONDS;
        return DEFERRED;
    }

    // The one script-log line per summary attempt.
    static string SummaryLine(int total, int senders, string outcome)
    {
        string line = "denied or invalid audit records held back suppressed=" + total.ToString() + " senders=" + senders.ToString();
        if (outcome == WRITTEN)
        {
            return line + "; summary written";
        }
        return line + "; summary deferred: the audit append failed, counts kept, retry in " + SUMMARY_SECONDS.ToString() + " s";
    }

    // {"suppressed":N,"senders":M,"listed":[{"steam64":"..","count":n},...]}, the first
    // MAX_LISTED senders in the order they were first held back.
    string EffectsJson()
    {
        string listed = "";
        int i = 0;
        while (i < m_Order.Count() && i < MAX_LISTED)
        {
            string steam64 = m_Order.Get(i);
            NimbyAdminServerJsonObject entry = new NimbyAdminServerJsonObject();
            entry.Add("steam64", steam64);
            entry.AddInt("count", m_Counts.Get(steam64));
            if (listed != "")
            {
                listed = listed + ",";
            }
            listed = listed + entry.ToJson();
            i = i + 1;
        }
        NimbyAdminServerJsonObject effects = new NimbyAdminServerJsonObject();
        effects.AddInt("suppressed", m_Total);
        effects.AddInt("senders", m_Order.Count());
        effects.Raw("listed", "[" + listed + "]");
        return effects.ToJson();
    }
}

// N8 as a decision: where the active set came from, and whether edits are refused.
class NimbyAdminServerSource
{
    static const string FILE = "file";
    static const string KEPT = "kept";
    static const string LKG = "lkg";
    static const string NONE = "none";

    // fileOk: the file parsed and built. keptActive: a good set is already in memory (a reload).
    static string Decide(bool fileOk, bool keptActive, bool lkgOk)
    {
        if (fileOk)
        {
            return FILE;
        }
        if (keptActive)
        {
            return KEPT;
        }
        if (lkgOk)
        {
            return LKG;
        }
        return NONE;
    }

    static bool EditsRefused(string source)
    {
        return source != FILE;
    }
}
