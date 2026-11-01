// -----------------------------------------------------------------------------
// NimbyDevHarness - Central Scrutinizer forged requests (Plan 3 v0.8 Phase 0 gate; recon 9;
// ruling 2, October 31, 2026: wire-level forging with two clients).
// Purpose : The harness loads with -mod= on the client too, so it MUST NOT name a NimbyAdminServer
//           class (shared rules 8.1). It forges at the wire instead: it writes the request
//           Param4 into a ScriptReadWriteContext (3_Game/DayZ/gameplay.c:134-141) and calls the
//           real PlayerBase.OnRPC (4_World/DayZ/Entities/ManBase/PlayerBase.c:5727) with a
//           connected player's real identity, so the router, the stores and the audit journal
//           all run as for a real packet. It names only NimbyAdminRules (client half, 3_Game).
//           It reads the outcome from the two audit day files (audit- and audit-denied-, Plan 3
//           v0.9) and from the players' positions.
// Arming  : $profile:NimbyDevHarness\forge holds "admin" and a line "admin-owner=<steam64>" (the
//           owner in permissions.json); the cases run once, 10 s after the second player is in
//           the world. The word "sandstorm" arms the item 3 row 4 case on the first connect.
// Session : the server numbers sessions in InvokeOnConnect order (a new number for a new
//           identity object); the harness mirrors that numbering from the same hook to build
//           the teleport args.
// Ids     : review finding 1 and the orchestrator rulings (October 31 and November 1, 2026):
//           each step forges for an identity from the server's highest accepted id on that
//           connection plus 1,000,000, read at the start of the step through NimbyAdminHighWater
//           (client half, 3_Game; written by the server). The server's set keeps the 256 highest
//           ids, so the harness ids stay in it; a run forges at most MAX_FORGES_PER_RUN ids, so
//           they take at most 127 of the 256 slots, the set always keeps real client ids, and its
//           floor stays below the client's next id. A run that would exceed it SKIPs with a
//           reason line. The duplicate case reuses an id that is inside the set. Audit lines are
//           matched by id, actor Steam64 and run start, because these ids repeat across runs on
//           the same day.
// Persists: Nothing.
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
    static const int FORGE_OFFSET = 1000000;
    static const int MAX_FORGES_PER_RUN = 127;
    // Steps that read a new base for the same identity: the second client in steps 1 and 4, the
    // owner in steps 2 and 3. Each step's base is the highest the server published, which then
    // includes the previous step's forged ids.
    static const int FORGE_STEPS_PER_IDENTITY = 2;
    // Step 1: 2, step 2: 6, step 3: 2 (one id twice), step 4: 50.
    static const int PLANNED_FORGES = 60;

    protected static bool s_Scheduled;
    protected static bool s_SandstormScheduled;
    protected static ref map<string, int> s_Sessions;
    protected static ref map<string, PlayerIdentity> s_Identities;
    protected static int s_NextSession = 1;
    protected static int s_Forged;
    protected static bool s_NoHeadroom;
    protected static bool s_Aborted;
    protected static string s_RunStart;
    protected static ref map<string, int> s_StepBase;
    protected static int s_IdDenied;
    protected static int s_IdNotOwner;
    protected static ref array<int> s_IdMalformed;
    protected static int s_IdDuplicate;
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
        if (s_NoHeadroom)
        {
            return;
        }
        if (s_Forged >= MAX_FORGES_PER_RUN)
        {
            if (!s_Aborted)
            {
                s_Aborted = true;
                Print(PREFIX + "[Forge] SKIP admin: forge limit " + MAX_FORGES_PER_RUN.ToString() + " reached; the remaining forges are not sent");
            }
            return;
        }
        s_Forged = s_Forged + 1;
        ScriptReadWriteContext ctx = RequestContext(protocol, requestId, command, args);
        // Gate session of November 1, 2026: one line per forge, so a rerun shows which body and
        // which identity each forged request used.
        Print(PREFIX + "[Forge] sent id=" + requestId.ToString() + " command=" + command + " sender=" + IdentityText(sender) + " target=" + IdentityText(targetObject.GetIdentity()));
        targetObject.OnRPC(sender, NimbyAdminRules.RPC_REQUEST, ctx.GetReadContext());
    }

    protected static string IdentityText(PlayerIdentity identity)
    {
        if (!identity)
        {
            return "(none)";
        }
        return "(" + identity.GetId() + ", " + identity.GetPlainId() + ")";
    }

    // The forged request in a context the router reads with one ctx.Read(Param4): one Write of the
    // Param4 object, the counterpart of that Read (1_Core/DayZ/proto/Serializer.c:57-58). Gate
    // session of November 1, 2026: Param4.Serialize wrote the four fields one by one, which the
    // router's Read of the whole object refused as unreadable (boot case
    // admin-forge-context-reads-back-as-the-router).
    static ScriptReadWriteContext RequestContext(int protocol, int requestId, string command, string args)
    {
        ScriptReadWriteContext ctx = new ScriptReadWriteContext;
        Param4<int, int, string, string> request = new Param4<int, int, string, string>(protocol, requestId, command, args);
        ctx.GetWriteContext().Write(request);
        return ctx;
    }

    // Reads a context back the way the router does (one ctx.Read of a Param4) and compares.
    static bool ReadsBackAsRouter(ScriptReadWriteContext ctx, int protocol, int requestId, string command, string args)
    {
        Param4<int, int, string, string> got = new Param4<int, int, string, string>(0, 0, "", "");
        if (!ctx.GetReadContext().Read(got))
        {
            return false;
        }
        return got.param1 == protocol && got.param2 == requestId && got.param3 == command && got.param4 == args;
    }

    protected static string Pad2(int value)
    {
        if (value < 10)
        {
            return "0" + value.ToString();
        }
        return value.ToString();
    }

    protected static string Today()
    {
        int year;
        int month;
        int day;
        GetYearMonthDayUTC(year, month, day);
        return year.ToString() + "-" + Pad2(month) + "-" + Pad2(day);
    }

    // The audit journal's ts form, YYYY-MM-DDTHH:MM:SSZ, which orders like the times it names.
    protected static string NowUtc()
    {
        int hour;
        int minute;
        int second;
        GetHourMinuteSecondUTC(hour, minute, second);
        return Today() + "T" + Pad2(hour) + ":" + Pad2(minute) + ":" + Pad2(second) + "Z";
    }

    protected static string AuditPath()
    {
        return "$profile:NimbyAdmin\\audit\\audit-" + Today() + ".log";
    }

    // Re-review finding 13: denied and invalid records go to their own day file.
    protected static string DeniedAuditPath()
    {
        return "$profile:NimbyAdmin\\audit\\audit-denied-" + Today() + ".log";
    }

    // Third-pass finding 14: whether steps more step bases above highest, each the published
    // highest plus FORGE_OFFSET with up to MAX_FORGES_PER_RUN ids above it, stay positive 32-bit
    // ints (int.MAX, 1_Core/DayZ/proto/EnConvert.c:27). Compared by subtraction, because the sum
    // itself would wrap. A negative highest (none is accepted) or no step has no headroom.
    static bool HasIdHeadroom(int highest, int steps)
    {
        if (highest < 0 || steps < 1)
        {
            return false;
        }
        int perStep = FORGE_OFFSET + MAX_FORGES_PER_RUN;
        return highest <= int.MAX - steps * perStep;
    }

    // Step ids: the server's highest accepted id for this identity plus FORGE_OFFSET, read once
    // per step, plus n. Backstop for finding 14: a step whose base has no headroom (a client
    // raised its highest during the run) stops the run, once, and nothing more is forged.
    protected static void BeginStep()
    {
        s_StepBase = new map<string, int>;
    }

    protected static int IdFor(PlayerIdentity identity, int n)
    {
        string biId = identity.GetId();
        int base;
        if (!s_StepBase.Find(biId, base))
        {
            int highest = NimbyAdminHighWater.Get(biId);
            if (!HasIdHeadroom(highest, 1))
            {
                if (!s_NoHeadroom)
                {
                    s_NoHeadroom = true;
                    Print(PREFIX + "[Forge] SKIP admin: id headroom: highest " + highest.ToString() + " + " + FORGE_OFFSET.ToString() + " + " + MAX_FORGES_PER_RUN.ToString() + " does not fit a positive 32-bit int; the remaining forges are not sent");
                }
                return 0;
            }
            base = highest + FORGE_OFFSET;
            s_StepBase.Set(biId, base);
        }
        return base + n;
    }

    // The audit lines this run wrote for one request id and actor.
    protected static array<string> AuditFor(int requestId, PlayerBase actor)
    {
        array<string> found = AuditIn(AuditPath(), requestId, actor);
        foreach (string line : AuditIn(DeniedAuditPath(), requestId, actor))
        {
            found.Insert(line);
        }
        return found;
    }

    // The audit lines this run wrote for one request id and actor, in one file.
    protected static array<string> AuditIn(string path, int requestId, PlayerBase actor)
    {
        array<string> found = new array<string>;
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
        string who = "\"actor\":{\"steam64\":\"" + actor.GetIdentity().GetPlainId() + "\"";
        string tsKey = "\"ts\":\"";
        string line = "";
        while (FGets(handle, line) >= 0)
        {
            int tsAt = line.IndexOf(tsKey);
            if (line.IndexOf(needle) == 0 && line.IndexOf(who) > 0 && tsAt > 0 && line.Length() >= tsAt + tsKey.Length() + 20)
            {
                string ts = line.Substring(tsAt + tsKey.Length(), 20);
                if (ts >= s_RunStart)
                {
                    found.Insert(line);
                }
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

    // ---- the cases -------------------------------------------------------------------------------

    // Step 1: the second client (no role) forges a teleport, and aims a request at the owner's body.
    static void Step1()
    {
        s_Lines = new array<string>;
        s_Failures = 0;
        s_Forged = 0;
        s_Aborted = false;
        s_NoHeadroom = false;
        s_RunStart = NowUtc();
        if (PLANNED_FORGES > MAX_FORGES_PER_RUN)
        {
            Print(PREFIX + "[Forge] SKIP admin: the run plans " + PLANNED_FORGES.ToString() + " forged ids, above the limit " + MAX_FORGES_PER_RUN.ToString());
            return;
        }
        if (!FindPlayers())
        {
            Print(PREFIX + "[Forge] SKIP admin: need the owner (forge admin-owner=) and a second player in the world");
            return;
        }
        int ownerHighest = NimbyAdminHighWater.Get(s_Owner.GetIdentity().GetId());
        int otherHighest = NimbyAdminHighWater.Get(s_Other.GetIdentity().GetId());
        if (!HasIdHeadroom(ownerHighest, FORGE_STEPS_PER_IDENTITY) || !HasIdHeadroom(otherHighest, FORGE_STEPS_PER_IDENTITY))
        {
            Print(PREFIX + "[Forge] SKIP admin: id headroom: highest owner=" + ownerHighest.ToString() + " other=" + otherHighest.ToString() + "; " + FORGE_STEPS_PER_IDENTITY.ToString() + " x (" + FORGE_OFFSET.ToString() + " + " + MAX_FORGES_PER_RUN.ToString() + ") above it does not fit a positive 32-bit int; nothing forged");
            return;
        }
        s_OwnerBefore = s_Owner.GetPosition();
        s_OtherBefore = s_Other.GetPosition();
        BeginStep();
        s_IdDenied = IdFor(s_Other.GetIdentity(), 0);
        s_IdNotOwner = IdFor(s_Other.GetIdentity(), 1);
        Forge(s_Other, s_Other.GetIdentity(), 1, s_IdDenied, NimbyAdminRules.CMD_TELEPORT_TO, TeleportArgs(s_Owner));
        Forge(s_Owner, s_Other.GetIdentity(), 1, s_IdNotOwner, NimbyAdminRules.CMD_TELEPORT_TO, TeleportArgs(s_Owner));
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarnessAdmin.Step2, STEP_MS, false);
    }

    // Step 2: read step 1, then the owner sends six malformed requests.
    static void Step2()
    {
        array<string> denied = AuditFor(s_IdDenied, s_Other);
        float otherMoved = vector.Distance(s_OtherBefore, s_Other.GetPosition());
        bool deniedOk = denied.Count() == 1 && CountWith(denied, "\"result\":\"denied\"") == 1 && CountWith(denied, "\"reason\":\"no role\"") == 1;
        int inDeniedFile = AuditIn(DeniedAuditPath(), s_IdDenied, s_Other).Count();
        Record("non-admin forges a teleport", deniedOk && inDeniedFile == 1 && otherMoved < 0.5, "records=" + denied.Count().ToString() + " inDeniedFile=" + inDeniedFile.ToString() + " moved=" + otherMoved.ToString());
        array<string> notOwner = AuditFor(s_IdNotOwner, s_Other);
        Record("request aimed at another player's body is refused before read", notOwner.Count() == 0, "records=" + notOwner.Count().ToString() + " (expect a sender-not-owner line in the script log)");
        PlayerIdentity owner = s_Owner.GetIdentity();
        BeginStep();
        s_IdMalformed = new array<int>;
        int k = 0;
        while (k < 6)
        {
            s_IdMalformed.Insert(IdFor(owner, k));
            k = k + 1;
        }
        Forge(s_Owner, owner, 2, s_IdMalformed.Get(0), NimbyAdminRules.CMD_HELLO, "");
        Forge(s_Owner, owner, 1, s_IdMalformed.Get(1), "player.kill", "");
        Forge(s_Owner, owner, 1, s_IdMalformed.Get(2), NimbyAdminRules.CMD_TELEPORT_TO, NimbySharedChunks.FillBytes("a", 901));
        Forge(s_Owner, owner, 1, s_IdMalformed.Get(3), NimbyAdminRules.CMD_TELEPORT_TO, "target=%41;session=1");
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
        Forge(s_Owner, owner, 1, s_IdMalformed.Get(4), NimbyAdminRules.CMD_TELEPORT_TO, seventeen);
        Forge(s_Owner, owner, 1, s_IdMalformed.Get(5), NimbyAdminRules.CMD_TELEPORT_TO, "target=nobody;session=1");
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarnessAdmin.Step3, STEP_MS, false);
    }

    protected static void ExpectInvalid(string name, int requestId, string reason)
    {
        array<string> lines = AuditFor(requestId, s_Owner);
        bool ok = lines.Count() == 1 && CountWith(lines, "\"result\":\"invalid\"") == 1 && CountWith(lines, "\"reason\":\"" + reason + "\"") == 1;
        Record(name, ok, "records=" + lines.Count().ToString() + " reason=" + reason);
    }

    // Step 3: read the malformed cases, then the owner sends one teleport twice with the same id.
    static void Step3()
    {
        ExpectInvalid("protocol 2", s_IdMalformed.Get(0), "protocol");
        ExpectInvalid("unknown command", s_IdMalformed.Get(1), "unknown command");
        ExpectInvalid("args of 901 bytes", s_IdMalformed.Get(2), "args too long");
        ExpectInvalid("unknown escape", s_IdMalformed.Get(3), "bad escape");
        ExpectInvalid("17 pairs", s_IdMalformed.Get(4), "too many args");
        ExpectInvalid("target that is not a session", s_IdMalformed.Get(5), "target session ended");
        s_OwnerBefore = s_Owner.GetPosition();
        string args = TeleportArgs(s_Other);
        BeginStep();
        s_IdDuplicate = IdFor(s_Owner.GetIdentity(), 0);
        // The second forge reuses the id the first one put inside the set.
        Forge(s_Owner, s_Owner.GetIdentity(), 1, s_IdDuplicate, NimbyAdminRules.CMD_TELEPORT_TO, args);
        Forge(s_Owner, s_Owner.GetIdentity(), 1, s_IdDuplicate, NimbyAdminRules.CMD_TELEPORT_TO, args);
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarnessAdmin.Step4, STEP_MS, false);
    }

    // Step 4: read the duplicate case, then the second client bursts 50 hellos in one frame.
    static void Step4()
    {
        array<string> lines = AuditFor(s_IdDuplicate, s_Owner);
        int intents = CountWith(lines, "\"phase\":\"intent\"");
        int results = CountWith(lines, "\"result\":\"ok\"");
        float near = vector.Distance(s_Owner.GetPosition(), s_Other.GetPosition());
        Record("duplicate request id runs once", intents == 1 && results == 1 && lines.Count() == 2 && near < 3.0, "intents=" + intents.ToString() + " results=" + results.ToString() + " distance=" + near.ToString());
        BeginStep();
        int n = 0;
        while (n < 50)
        {
            Forge(s_Other, s_Other.GetIdentity(), 1, IdFor(s_Other.GetIdentity(), n), NimbyAdminRules.CMD_HELLO, "");
            n = n + 1;
        }
        Record("burst of 50 sent", !s_Aborted, "sent=50; the script log holds one rate-limited line for the sender");
        Print(PREFIX + " admin forge complete; cases=" + s_Lines.Count().ToString() + " failures=" + s_Failures.ToString() + " forged=" + s_Forged.ToString() + " limit=" + MAX_FORGES_PER_RUN.ToString());
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
        // Gate session of November 1, 2026: Jeff saw a thunderstorm, not a sandstorm. The harness
        // now follows the vanilla dev command (3_Game/DayZ/DayZGame.c:3415-3436): wind turned to
        // the storm's direction (Weather.WindDirectionToAngle :340, GetWindDirection :199), overcast
        // 0.8, wind 18, and rain held at 0 (GetRain :189), then Start.
        SandstormController storm = weather.GetSandstorm();
        float stormAngle = weather.WindDirectionToAngle(storm.GetDirection());
        weather.GetWindDirection().Set(stormAngle, 10, 300);
        weather.GetOvercast().Set(0.8, 10, 300);
        weather.GetWindMagnitude().Set(18.0, 10, 300);
        weather.GetRain().Set(0, 10, 300);
        bool started = storm.Start(120, false);
        Print(PREFIX + "[Forge] sandstorm: Start(120, false) returned " + NimbyDevHarness.Bool(started) + " windAngle=" + stormAngle.ToString());
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
        if (active)
        {
            // Where the front is, so a rerun can tell "not visible" from "not near the players"
            // (Sandstorm.c:37, :47, :52, :57; valid only while active).
            SandstormController storm = GetGame().GetWeather().GetSandstorm();
            Print(PREFIX + "[Forge] sandstorm: position=" + storm.GetPosition().ToString() + " direction=" + storm.GetDirection().ToString() + " speed=" + storm.GetSpeed().ToString() + " remaining=" + storm.GetRemainingMovementDuration().ToString() + " s; " + SandstormPlayerDistances(storm.GetPosition()));
        }
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NimbyDevHarnessAdmin.SandstormStop, 85000, false);
    }

    // Each connected player's horizontal distance to the storm's position, by Steam64.
    protected static string SandstormPlayerDistances(vector stormAt)
    {
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        string text = "players:";
        foreach (Man man : players)
        {
            PlayerBase player = PlayerBase.Cast(man);
            if (!player || !player.GetIdentity())
            {
                continue;
            }
            vector at = player.GetPosition();
            float dx = at[0] - stormAt[0];
            float dz = at[2] - stormAt[2];
            float meters = Math.Sqrt(dx * dx + dz * dz);
            text = text + " " + player.GetIdentity().GetPlainId() + "=" + meters.ToString() + "m";
        }
        return text;
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
