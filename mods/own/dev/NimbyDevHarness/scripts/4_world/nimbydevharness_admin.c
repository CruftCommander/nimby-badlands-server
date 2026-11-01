// -----------------------------------------------------------------------------
// NimbyDevHarness - Central Scrutinizer forged requests (Plan 3 v0.8 Phase 0 gate; recon 9;
// ruling 2, October 31, 2026: wire-level forging with two clients).
// Purpose : The harness loads with -mod= on the client too, so it MUST NOT name a NimbyAdminServer
//           class (shared rules 8.1). It forges at the wire instead: it writes the request
//           Param4 into a ScriptReadWriteContext (3_Game/DayZ/gameplay.c:134-141) and calls the
//           real PlayerBase.OnRPC (4_World/DayZ/Entities/ManBase/PlayerBase.c:5727) with a
//           connected player's real identity, so the router, the stores and the audit journal
//           all run as for a real packet. It names only NimbyAdminRules (client half, 3_Game).
//           It reads the outcome from the audit file and from the players' positions.
// Arming  : $profile:NimbyDevHarness\forge holds "admin" and a line "admin-owner=<steam64>" (the
//           owner in permissions.json); the cases run once, 10 s after the second player is in
//           the world. The word "sandstorm" arms the item 3 row 4 case on the first connect.
// Session : the server numbers sessions in InvokeOnConnect order (a new number for a new
//           identity object); the harness mirrors that numbering from the same hook to build
//           the teleport args.
// Persists: Nothing. Forged request ids start at 100000000 plus the UTC second of the day x 100.
// Vanilla : ScriptCallQueue.CallLater (2_GameLib/DayZ/tools.c:61), from 4_World only (shared rules
//           7.1); Weather.GetOvercast, GetWindMagnitude, GetSandstorm (3_Game/DayZ/Weather.c:183,
//           205,210); WeatherPhenomenon.Set (:49); SandstormController.Start, Stop, IsActive
//           (3_Game/DayZ/Sandstorm.c:20,27,32); vanilla raises overcast and wind first
//           (3_Game/DayZ/DayZGame.c:3425-3436); FGets (1_Core/DayZ/proto/EnSystem.c:501).
// -----------------------------------------------------------------------------

class NimbyDevHarnessAdmin
{
    static const string PREFIX = "[NIMBY][DevHarness]";
    static const string FORGE_FILE = "$profile:NimbyDevHarness\\forge";
    static const int START_DELAY_MS = 10000;
    static const int STEP_MS = 3000;

    protected static bool s_Scheduled;
    protected static bool s_SandstormScheduled;
    protected static ref map<string, int> s_Sessions;
    protected static ref map<string, PlayerIdentity> s_Identities;
    protected static int s_NextSession = 1;
    protected static int s_Base;
    protected static ref array<string> s_Lines;
    protected static int s_Failures;
    protected static PlayerBase s_Owner;
    protected static PlayerBase s_Other;
    protected static vector s_OwnerBefore;
    protected static vector s_OtherBefore;

    // ---- forge file ------------------------------------------------------------------------------

    static string ForgeValue(string key)
    {
        if (!FileExist(FORGE_FILE))
        {
            return "";
        }
        FileHandle handle = OpenFile(FORGE_FILE, FileMode.READ);
        if (handle == 0)
        {
            return "";
        }
        string found = "";
        string line = "";
        while (FGets(handle, line) >= 0)
        {
            line = line.Trim();
            if (line.IndexOf(key + "=") == 0)
            {
                found = line.Substring(key.Length() + 1, line.Length() - key.Length() - 1);
            }
            line = "";
        }
        CloseFile(handle);
        return found;
    }

    // ---- hook ------------------------------------------------------------------------------------

    static void OnPlayerConnected(PlayerBase player)
    {
        if (!GetGame().IsServer() || !player)
        {
            return;
        }
        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
        {
            return;
        }
        if (!s_Sessions)
        {
            s_Sessions = new map<string, int>;
            s_Identities = new map<string, PlayerIdentity>;
        }
        string biId = identity.GetId();
        PlayerIdentity known;
        if (!s_Identities.Find(biId, known) || known != identity)
        {
            s_Sessions.Set(biId, s_NextSession);
            s_Identities.Set(biId, identity);
            s_NextSession = s_NextSession + 1;
        }
        if (NimbyDevHarness.ForgeHas("sandstorm") && !s_SandstormScheduled)
        {
            s_SandstormScheduled = true;
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarnessAdmin.SandstormStart, 15000, false);
        }
        if (!NimbyDevHarness.ForgeHas("admin") || s_Scheduled)
        {
            return;
        }
        if (s_Sessions.Count() < 2)
        {
            return;
        }
        s_Scheduled = true;
        Print(PREFIX + " admin cases scheduled in " + (START_DELAY_MS / 1000).ToString() + " s");
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarnessAdmin.Step1, START_DELAY_MS, false);
    }

    // ---- helpers ---------------------------------------------------------------------------------

    protected static void Forge(PlayerBase targetObject, PlayerIdentity sender, int protocol, int requestId, string command, string args)
    {
        ScriptReadWriteContext ctx = new ScriptReadWriteContext;
        Param4<int, int, string, string> request = new Param4<int, int, string, string>(protocol, requestId, command, args);
        request.Serialize(ctx.GetWriteContext());   // the same field order RPCSingleParam writes (1_Core/DayZ/param.c:134-137)
        targetObject.OnRPC(sender, NimbyAdminRules.RPC_REQUEST, ctx.GetReadContext());
    }

    protected static string AuditPath()
    {
        int year;
        int month;
        int day;
        GetYearMonthDayUTC(year, month, day);
        string mm = month.ToString();
        if (month < 10)
        {
            mm = "0" + mm;
        }
        string dd = day.ToString();
        if (day < 10)
        {
            dd = "0" + dd;
        }
        return "$profile:NimbyAdmin\\audit\\audit-" + year.ToString() + "-" + mm + "-" + dd + ".log";
    }

    // The audit lines for one request id.
    protected static array<string> AuditFor(int requestId)
    {
        array<string> found = new array<string>;
        string path = AuditPath();
        if (!FileExist(path))
        {
            return found;
        }
        FileHandle handle = OpenFile(path, FileMode.READ);
        if (handle == 0)
        {
            return found;
        }
        string needle = "{\"id\":\"" + requestId.ToString() + "\"";
        string line = "";
        while (FGets(handle, line) >= 0)
        {
            if (line.IndexOf(needle) == 0)
            {
                found.Insert(line);
            }
            line = "";
        }
        CloseFile(handle);
        return found;
    }

    protected static int CountWith(array<string> lines, string fragment)
    {
        int count = 0;
        foreach (string line : lines)
        {
            if (line.IndexOf(fragment) >= 0)
            {
                count = count + 1;
            }
        }
        return count;
    }

    protected static void Record(string name, bool pass, string detail)
    {
        string verdict = "PASS";
        if (!pass)
        {
            verdict = "FAIL";
            s_Failures = s_Failures + 1;
        }
        s_Lines.Insert(verdict);
        Print(PREFIX + "[Forge] " + verdict + " admin: " + name + " detail=" + detail);
    }

    protected static bool FindPlayers()
    {
        string ownerSteam = ForgeValue("admin-owner");
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        s_Owner = null;
        s_Other = null;
        foreach (Man man : players)
        {
            PlayerBase player = PlayerBase.Cast(man);
            if (!player || !player.GetIdentity())
            {
                continue;
            }
            if (player.GetIdentity().GetPlainId() == ownerSteam)
            {
                s_Owner = player;
            }
            else if (!s_Other)
            {
                s_Other = player;
            }
        }
        return s_Owner && s_Other;
    }

    protected static string TeleportArgs(PlayerBase target)
    {
        NimbyAdminArgs args = new NimbyAdminArgs();
        string biId = target.GetIdentity().GetId();
        args.Set("target", biId);
        args.Set("session", s_Sessions.Get(biId).ToString());
        return args.Encode();
    }

    protected static int Id(int n)
    {
        return s_Base + n;
    }

    // ---- the cases -------------------------------------------------------------------------------

    // Step 1: the second client (no role) forges a teleport, and aims a request at the owner's body.
    static void Step1()
    {
        s_Lines = new array<string>;
        s_Failures = 0;
        int hour;
        int minute;
        int second;
        GetHourMinuteSecondUTC(hour, minute, second);
        s_Base = 100000000 + (hour * 3600 + minute * 60 + second) * 100;
        if (!FindPlayers())
        {
            Print(PREFIX + "[Forge] SKIP admin: need the owner (forge admin-owner=) and a second player in the world");
            return;
        }
        s_OwnerBefore = s_Owner.GetPosition();
        s_OtherBefore = s_Other.GetPosition();
        Forge(s_Other, s_Other.GetIdentity(), 1, Id(1), NimbyAdminRules.CMD_TELEPORT_TO, TeleportArgs(s_Owner));
        Forge(s_Owner, s_Other.GetIdentity(), 1, Id(2), NimbyAdminRules.CMD_TELEPORT_TO, TeleportArgs(s_Owner));
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarnessAdmin.Step2, STEP_MS, false);
    }

    // Step 2: read step 1, then the owner sends six malformed requests.
    static void Step2()
    {
        array<string> denied = AuditFor(Id(1));
        float otherMoved = vector.Distance(s_OtherBefore, s_Other.GetPosition());
        bool deniedOk = denied.Count() == 1 && CountWith(denied, "\"result\":\"denied\"") == 1 && CountWith(denied, "\"reason\":\"no role\"") == 1;
        Record("non-admin forges a teleport", deniedOk && otherMoved < 0.5, "records=" + denied.Count().ToString() + " moved=" + otherMoved.ToString());
        array<string> notOwner = AuditFor(Id(2));
        Record("request aimed at another player's body is refused before read", notOwner.Count() == 0, "records=" + notOwner.Count().ToString() + " (expect a sender-not-owner line in the script log)");
        PlayerIdentity owner = s_Owner.GetIdentity();
        Forge(s_Owner, owner, 2, Id(3), NimbyAdminRules.CMD_HELLO, "");
        Forge(s_Owner, owner, 1, Id(4), "player.kill", "");
        Forge(s_Owner, owner, 1, Id(5), NimbyAdminRules.CMD_TELEPORT_TO, NimbySharedChunks.FillBytes("a", 901));
        Forge(s_Owner, owner, 1, Id(6), NimbyAdminRules.CMD_TELEPORT_TO, "target=%41;session=1");
        string seventeen = "";
        int i = 0;
        while (i < 17)
        {
            if (i > 0)
            {
                seventeen = seventeen + ";";
            }
            seventeen = seventeen + "k" + i.ToString() + "=1";
            i = i + 1;
        }
        Forge(s_Owner, owner, 1, Id(7), NimbyAdminRules.CMD_TELEPORT_TO, seventeen);
        Forge(s_Owner, owner, 1, Id(8), NimbyAdminRules.CMD_TELEPORT_TO, "target=nobody;session=1");
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarnessAdmin.Step3, STEP_MS, false);
    }

    protected static void ExpectInvalid(string name, int requestId, string reason)
    {
        array<string> lines = AuditFor(requestId);
        bool ok = lines.Count() == 1 && CountWith(lines, "\"result\":\"invalid\"") == 1 && CountWith(lines, "\"reason\":\"" + reason + "\"") == 1;
        Record(name, ok, "records=" + lines.Count().ToString() + " reason=" + reason);
    }

    // Step 3: read the malformed cases, then the owner sends one teleport twice with the same id.
    static void Step3()
    {
        ExpectInvalid("protocol 2", Id(3), "protocol");
        ExpectInvalid("unknown command", Id(4), "unknown command");
        ExpectInvalid("args of 901 bytes", Id(5), "args too long");
        ExpectInvalid("unknown escape", Id(6), "bad escape");
        ExpectInvalid("17 pairs", Id(7), "too many args");
        ExpectInvalid("target that is not a session", Id(8), "target session ended");
        s_OwnerBefore = s_Owner.GetPosition();
        string args = TeleportArgs(s_Other);
        Forge(s_Owner, s_Owner.GetIdentity(), 1, Id(9), NimbyAdminRules.CMD_TELEPORT_TO, args);
        Forge(s_Owner, s_Owner.GetIdentity(), 1, Id(9), NimbyAdminRules.CMD_TELEPORT_TO, args);
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarnessAdmin.Step4, STEP_MS, false);
    }

    // Step 4: read the duplicate case, then the second client bursts 50 hellos in one frame.
    static void Step4()
    {
        array<string> lines = AuditFor(Id(9));
        int intents = CountWith(lines, "\"phase\":\"intent\"");
        int results = CountWith(lines, "\"result\":\"ok\"");
        float near = vector.Distance(s_Owner.GetPosition(), s_Other.GetPosition());
        Record("duplicate request id runs once", intents == 1 && results == 1 && lines.Count() == 2 && near < 3.0, "intents=" + intents.ToString() + " results=" + results.ToString() + " distance=" + near.ToString());
        int n = 0;
        while (n < 50)
        {
            Forge(s_Other, s_Other.GetIdentity(), 1, Id(100 + n), NimbyAdminRules.CMD_HELLO, "");
            n = n + 1;
        }
        Record("burst of 50 sent", true, "sent=50; the script log holds one rate-limited line for the sender");
        Print(PREFIX + " admin forge complete; cases=" + s_Lines.Count().ToString() + " failures=" + s_Failures.ToString());
    }

    // ---- item 3 row 4: sandstorm on a dedicated server -------------------------------------------

    static void SandstormStart()
    {
        Weather weather = GetGame().GetWeather();
        if (!weather || !weather.GetSandstorm())
        {
            Print(PREFIX + "[Forge] FAIL sandstorm: no Weather or no SandstormController");
            return;
        }
        weather.GetOvercast().Set(0.8, 10, 300);
        weather.GetWindMagnitude().Set(18.0, 10, 300);
        bool started = weather.GetSandstorm().Start(120, false);
        Print(PREFIX + "[Forge] sandstorm: Start(120, false) returned " + NimbyDevHarness.Bool(started));
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarnessAdmin.SandstormCheck, 5000, false, started);
    }

    static void SandstormCheck(bool started)
    {
        bool active = GetGame().GetWeather().GetSandstorm().IsActive();
        string verdict = "FAIL";
        if (started && active)
        {
            verdict = "PASS";
        }
        Print(PREFIX + "[Forge] " + verdict + " sandstorm: Start=" + NimbyDevHarness.Bool(started) + " active=" + NimbyDevHarness.Bool(active));
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarnessAdmin.SandstormStop, 85000, false);
    }

    static void SandstormStop()
    {
        GetGame().GetWeather().GetSandstorm().Stop(30, false);
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarnessAdmin.SandstormStopped, 40000, false);
    }

    static void SandstormStopped()
    {
        bool active = GetGame().GetWeather().GetSandstorm().IsActive();
        string verdict = "PASS";
        if (active)
        {
            verdict = "FAIL";
        }
        Print(PREFIX + "[Forge] " + verdict + " sandstorm: stopped active=" + NimbyDevHarness.Bool(active));
    }
}
