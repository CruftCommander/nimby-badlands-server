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
    // Denied and invalid audit records per sender: 30 a minute (recon 4.3, disk bound).
    static const float DENIAL_RECORDS_PER_SECOND = 0.5;
    static const int DENIAL_RECORDS_BURST = 30;
    // Replay window per sender per connection (recon 4.3 step 7; review finding 1, October 31,
    // 2026): the last SEEN_MAX accepted ids, no time-based expiry.
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
        int month = value.Substring(5, 2).ToInt();
        int day = value.Substring(8, 2).ToInt();
        int hour = value.Substring(11, 2).ToInt();
        int minute = value.Substring(14, 2).ToInt();
        int second = value.Substring(17, 2).ToInt();
        if (month < 1 || month > 12 || day < 1 || day > 31)
        {
            return false;
        }
        if (hour > 23 || minute > 59 || second > 59)
        {
            return false;
        }
        return true;
    }

    // ---- JSON text the server writes (never parses from a client) -------------------------------

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

class NimbyAdminServerPermissions
{
    int version = 1;
    ref array<ref NimbyAdminServerAdmin> admins = new array<ref NimbyAdminServerAdmin>;

    // The active set (Steam64 to role). Invalid and duplicate rows are skipped and named in notes;
    // they grant nothing. False when the file as a whole is unusable (wrong version).
    bool Build(out map<string, string> active, out array<string> notes)
    {
        active = new map<string, string>;
        notes = new array<string>;
        if (version != 1)
        {
            notes.Insert("version " + version.ToString() + " is not 1");
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

class NimbyAdminServerBans
{
    int version = 1;
    ref array<ref NimbyAdminServerBan> bans = new array<ref NimbyAdminServerBan>;

    bool Build(out map<string, string> active, out array<string> notes)
    {
        active = new map<string, string>;
        notes = new array<string>;
        if (version != 1)
        {
            notes.Insert("version " + version.ToString() + " is not 1");
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

// The replay window of one sender on one connection (review finding 1, October 31, 2026). It
// keeps the highest accepted id, the last SEEN_MAX accepted ids in acceptance order, and the
// floor: the lowest id in that set once the set is full. An id in the set is a duplicate. An id
// below the floor of a full set is stale. Any other id is accepted, also below the highest (a
// gap), which lets the dev harness forge above a real client's ids. No time-based expiry.
// Residual: an id evicted while a lower id is still in the set lies above the floor and would be
// accepted again; only an id accepted out of order (the harness's high ids) can be in that place,
// because a real client's ids rise (NimbyAdminRequestIds).
class NimbyAdminServerSeen
{
    static const string ACCEPT = "";
    static const string DUPLICATE = "duplicate";
    static const string STALE = "stale";

    protected ref array<int> m_Order = new array<int>;
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
        if (m_Order.Count() >= NimbyAdminServerRules.SEEN_MAX && requestId < m_Floor)
        {
            return STALE;
        }
        return ACCEPT;
    }

    // Records an id Check accepted.
    void Accept(int requestId)
    {
        m_Order.Insert(requestId);
        m_Set.Set(requestId, true);
        if (requestId > m_Highest)
        {
            m_Highest = requestId;
        }
        while (m_Order.Count() > NimbyAdminServerRules.SEEN_MAX)
        {
            int gone = m_Order.Get(0);
            m_Order.RemoveOrdered(0);
            m_Set.Remove(gone);
        }
        if (m_Order.Count() >= NimbyAdminServerRules.SEEN_MAX)
        {
            m_Floor = Lowest();
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
        return m_Order.Count();
    }

    protected int Lowest()
    {
        int lowest = m_Order.Get(0);
        foreach (int id : m_Order)
        {
            if (id < lowest)
            {
                lowest = id;
            }
        }
        return lowest;
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
