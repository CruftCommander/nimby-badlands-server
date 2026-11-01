// -----------------------------------------------------------------------------
// NimbyAdminServer - sessions, the authenticated router, and the Phase 0 handlers (Plan 3 v0.8,
// security contract; recon 4.3 as accepted October 31, 2026, with rulings 2 to 8).
// Router  : in order, cheapest first: sender present; the targeted PlayerBase is the sender's own;
//           per-sender and aggregate rate limits; Param4 read; envelope; actor Steam64 from the
//           connection (GetPlainId, 3_Game/DayZ/gameplay.c:370), never from the client;
//           duplicate id; known command; args codec and the command's own validator; role and
//           capability, with permissions.json re-read when older than permissionsReloadSeconds
//           (ruling 5); target resolved against the current sessions by BI id and session
//           number; audit intent (a failed append blocks the command); handler; audit result;
//           Discord; result to the sender only.
// Stale   : a selection made before the target reconnected carries an old session number and is
//           refused (reason "target session ended"). Request ids go through the replay window
//           per sender per connection (NimbyAdminServerSeen, review finding 1): a duplicate is
//           dropped with one gated line, an id below the floor is "stale"; a protocol other than
//           1 is refused.
// Logs    : every refusal a client can cause is gated per sender (NimbyAdminLog.Gated); denied and
//           invalid audit records are bounded per sender by BI id (30 a minute) and in aggregate
//           (120 a minute) unless the actor holds a role; what the limits hold back is written as
//           one audit.suppressed record a minute. Client text reaches an ordinary log line only through NimbyAdminLog.Safe (in
//           WhoFrom) or CommandForLog (an unknown command logs as <unknown len=N>), review
//           finding 7; the audit journal keeps the command, JSON-escaped and cut to 32 bytes.
// Gates   : server only (recon R11). The client half's PlayerBase override passes 6550301 to super.
// Vanilla : PlayerBase.OnRPC (4_World/DayZ/Entities/ManBase/PlayerBase.c:5727); Object.SetPosition,
//           GetPosition, GetDirection, GetOrientation (3_Game/DayZ/Entities/Object.c:294,287,314,305);
//           CGame.IsBoxColliding (3_Game/DayZ/Global/Game.c:1333); CGame.RPCSingleParam (:1013);
//           DayZPlayerImplement.IsInVehicle (4_World/DayZ/Entities/DayZPlayerImplement.c:468);
//           Object.IsAlive (Object.c:517); vector.Distance; JsonSerializer.WriteToString
//           (3_Game/DayZ/gameplay.c:68); NimbySharedChunkSender (Plan 11 C7).
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyAdminServerSession
{
    int number;
    PlayerIdentity identity;
    string biId;
    string steam64;
    string name;
    string lastRole = "";
    int pushSequence;
    ref NimbyAdminServerSeen seen = new NimbyAdminServerSeen();
}

class NimbyAdminServerActor
{
    string steam64;
    string biId;
    string name;
    PlayerIdentity identity;
    PlayerBase player;
}

class NimbyAdminServerCore
{
    protected static bool s_Ready;
    protected static ref NimbyAdminServerConfig s_Config;
    protected static ref NimbyAdminServerPermissionStore s_Permissions;
    protected static ref NimbyAdminServerBanStore s_Bans;
    protected static ref map<string, ref NimbyAdminServerSession> s_Sessions;
    protected static ref map<string, ref NimbyAdminServerBucket> s_Rates;
    protected static ref NimbyAdminServerBucket s_Aggregate;
    protected static ref NimbyAdminServerBucket s_DenialAggregate;
    protected static ref map<string, ref NimbyAdminServerBucket> s_DenialRates;
    protected static ref NimbyAdminServerSuppressed s_Suppressed;
    protected static float s_SuppressedSince;
    protected static int s_NextSession = 1;

    // ---- lifecycle (MissionServer, nimbyadminserver_missionserver.c) ----------------------------

    static void Init()
    {
        s_Sessions = new map<string, ref NimbyAdminServerSession>;
        s_Rates = new map<string, ref NimbyAdminServerBucket>;
        s_Aggregate = new NimbyAdminServerBucket();
        s_DenialAggregate = new NimbyAdminServerBucket();
        s_DenialRates = new map<string, ref NimbyAdminServerBucket>;
        s_Suppressed = new NimbyAdminServerSuppressed();
        NimbyAdminServerFiles.EnsureDirs();
        s_Config = NimbyAdminServerFiles.LoadConfig();
        NimbyAdminServerDiscord.Init(s_Config);
        float now = GetGame().GetTickTime();
        s_Permissions = new NimbyAdminServerPermissionStore();
        s_Permissions.m_Bootstrap = s_Config.bootstrapOwnerSteam64;
        string permissionsProblem;
        s_Permissions.Load(now, permissionsProblem);
        // Review finding 3: the bootstrap file only on a true first install; a missing primary
        // with a .lkg is a recovery (the .lkg set, edits refused), and the bootstrap owner then
        // applies only as the overlay when that set has no owner (ApplyBootstrap).
        if (s_Permissions.IsFirstInstall() && s_Config.bootstrapOwnerSteam64 != "")
        {
            WriteBootstrap();
            s_Permissions.Load(now, permissionsProblem);
        }
        s_Permissions.ApplyBootstrap();
        Alert(permissionsProblem);
        s_Bans = new NimbyAdminServerBanStore();
        string bansProblem;
        s_Bans.Load(now, bansProblem);
        Alert(bansProblem);
        if (s_Bans.NoUsableList())
        {
            Alert("bans.json and bans.json.lkg unusable: admission is refused to everyone outside permissions.json (enforced from Phase 1)");
        }
        s_Ready = true;
        string line = "server ready; permissions source=" + s_Permissions.m_Source + " entries=" + s_Permissions.m_Active.Count().ToString();
        line = line + " bootstrap=" + NimbyAdminLog.Bool(s_Permissions.m_BootstrapActive) + " editsRefused=" + NimbyAdminLog.Bool(s_Permissions.m_EditsRefused);
        line = line + "; bans source=" + s_Bans.m_Source + " entries=" + s_Bans.m_Active.Count().ToString();
        line = line + "; webhook=" + WebhookState() + " requestsPerSecond=" + s_Config.requestsPerSecond.ToString() + " permissionsReloadSeconds=" + s_Config.permissionsReloadSeconds.ToString();
        NimbyAdminLog.Info(line);
    }

    protected static string WebhookState()
    {
        if (NimbyAdminServerDiscord.IsEnabled())
        {
            return "set";
        }
        return "unset";
    }

    protected static void Alert(string problem)
    {
        if (problem == "")
        {
            return;
        }
        NimbyAdminLog.Info(problem);
        NimbyAdminServerDiscord.Enqueue("[Central Scrutinizer] " + problem);
    }

    // Recon 5.4: first boot only; audited as an intent and result pair with actor "system".
    protected static void WriteBootstrap()
    {
        NimbyAdminServerAuditRecord intent = SystemRecord("permissions.bootstrap", "intent");
        NimbyAdminServerJsonObject target = new NimbyAdminServerJsonObject();
        target.Add("steam64", s_Config.bootstrapOwnerSteam64);
        target.Add("role", NimbyAdminRules.ROLE_OWNER);
        intent.target = target.ToJson();
        if (!NimbyAdminServerAudit.Append(intent))
        {
            NimbyAdminLog.Info("bootstrap owner not written: audit unavailable (the owner is still active in memory)");
            return;
        }
        string why;
        bool written = s_Permissions.WriteBootstrapFile(why);
        NimbyAdminServerAuditRecord result = SystemRecord("permissions.bootstrap", "result");
        result.target = intent.target;
        if (written)
        {
            result.result = "ok";
            NimbyAdminLog.Info("permissions.json created with the bootstrap owner steam64=" + s_Config.bootstrapOwnerSteam64);
        }
        else
        {
            result.result = "failed";
            result.reason = why;
            NimbyAdminLog.Info("permissions.json not created: " + why);
        }
        NimbyAdminServerAudit.Append(result);
    }

    protected static NimbyAdminServerAuditRecord SystemRecord(string command, string phase)
    {
        NimbyAdminServerAuditRecord record = new NimbyAdminServerAuditRecord();
        record.id = "system-" + NimbyAdminServerAudit.NowUtc();
        record.phase = phase;
        record.actorSteam64 = "system";
        record.actorBiId = "system";
        record.actorName = "system";
        record.command = command;
        return record;
    }

    static void Tick(float now)
    {
        if (!s_Ready)
        {
            return;
        }
        NimbyAdminServerDiscord.Tick(now);
        WriteSuppressedSummaryIfDue(now);
    }

    // MissionServer.InvokeOnConnect (5_Mission/DayZ/mission/missionServer.c:435), at new and ready.
    static void OnConnect(PlayerBase player, PlayerIdentity identity)
    {
        if (!s_Ready || !identity)
        {
            return;
        }
        SessionFor(identity);
    }

    // MissionServer.PlayerDisconnected (missionServer.c:690); the identity may already be gone, so
    // the session is found by uid (the BI id).
    static void OnDisconnect(string uid)
    {
        if (!s_Ready)
        {
            return;
        }
        if (s_Sessions.Contains(uid))
        {
            NimbyAdminServerSession session = s_Sessions.Get(uid);
            NimbyAdminLog.Debug("session ended number=" + session.number.ToString() + " " + NimbyAdminLog.WhoFrom(session.name, session.biId, session.steam64));
            s_Sessions.Remove(uid);
        }
        NimbyAdminHighWater.Clear(uid);
    }

    protected static NimbyAdminServerSession SessionFor(PlayerIdentity identity)
    {
        string biId = identity.GetId();
        NimbyAdminServerSession session;
        if (s_Sessions.Find(biId, session))
        {
            if (session.identity == identity)
            {
                return session;
            }
        }
        session = new NimbyAdminServerSession();
        session.number = s_NextSession;
        s_NextSession = s_NextSession + 1;
        session.identity = identity;
        session.biId = biId;
        session.steam64 = identity.GetPlainId();
        session.name = identity.GetName();
        s_Sessions.Set(biId, session);
        NimbyAdminHighWater.Clear(biId);
        return session;
    }

    // ---- the router ------------------------------------------------------------------------------

    static void HandleRequest(PlayerBase target, PlayerIdentity sender, ParamsReadContext ctx)
    {
        string key = NimbyAdminLog.Key(sender);
        if (!s_Ready)
        {
            return;
        }
        if (!sender)
        {
            NimbyAdminLog.Gated(key, "request refused reason=null-sender");
            return;
        }
        PlayerIdentity owner = target.GetIdentity();
        if (!owner || owner.GetId() != sender.GetId())
        {
            NimbyAdminLog.Gated(key, "request refused reason=sender-not-owner sender " + NimbyAdminLog.Who(sender) + " target " + NimbyAdminLog.Who(owner));
            return;
        }
        float now = GetGame().GetTickTime();
        NimbyAdminServerBucket rate;
        if (!s_Rates.Find(sender.GetId(), rate))
        {
            rate = new NimbyAdminServerBucket();
            s_Rates.Set(sender.GetId(), rate);
        }
        string limited = NimbyAdminServerRules.Admit(rate, s_Aggregate, now, s_Config.requestsPerSecond, NimbyAdminServerRules.RATE_BURST);
        if (limited != "")
        {
            NimbyAdminLog.Gated("rate:" + key, "rate-limited " + NimbyAdminLog.Who(sender) + " aggregate=" + NimbyAdminLog.Bool(limited == NimbyAdminServerRules.LIMIT_AGGREGATE));
            return;
        }
        Param4<int, int, string, string> request = new Param4<int, int, string, string>(0, 0, "", "");
        if (!ctx.Read(request))
        {
            NimbyAdminLog.Gated(key, "request refused reason=unreadable " + NimbyAdminLog.Who(sender));
            return;
        }
        int requestId = request.param2;
        string command = request.param3;
        string argsText = request.param4;
        NimbyAdminServerSession session = SessionFor(sender);
        NimbyAdminServerActor actor = new NimbyAdminServerActor();
        actor.steam64 = sender.GetPlainId();
        actor.biId = sender.GetId();
        actor.name = sender.GetName();
        actor.identity = sender;
        actor.player = target;
        string envelope = NimbyAdminRules.CheckEnvelope(request.param1, requestId, command, argsText);
        if (envelope != "")
        {
            Reject(actor, session, requestId, command, NimbyAdminRules.CODE_INVALID, envelope);
            return;
        }
        if (!NimbyAdminServerRules.IsSteam64(actor.steam64))
        {
            Reject(actor, session, requestId, command, NimbyAdminRules.CODE_INVALID, "actor identity");
            return;
        }
        string replay = session.seen.Check(requestId);
        if (replay == NimbyAdminServerSeen.DUPLICATE)
        {
            NimbyAdminLog.Gated("dup:" + key, "duplicate id=" + requestId.ToString() + " command=" + NimbyAdminServerRules.CommandForLog(command) + " " + NimbyAdminLog.Who(sender));
            return;
        }
        if (replay == NimbyAdminServerSeen.STALE)
        {
            Reject(actor, session, requestId, command, NimbyAdminRules.CODE_INVALID, "stale");
            return;
        }
        session.seen.Accept(requestId);
        NimbyAdminHighWater.Publish(session.biId, session.seen.Highest());
        if (!NimbyAdminServerRules.IsKnownCommand(command))
        {
            Reject(actor, session, requestId, command, NimbyAdminRules.CODE_INVALID, "unknown command");
            return;
        }
        NimbyAdminArgs args;
        string argsWhy;
        if (!NimbyAdminArgs.Parse(argsText, args, argsWhy))
        {
            Reject(actor, session, requestId, command, NimbyAdminRules.CODE_INVALID, argsWhy);
            return;
        }
        ReloadPermissionsIfDue(now);
        string role = s_Permissions.RoleOf(actor.steam64);
        if (command == NimbyAdminRules.CMD_HELLO)
        {
            HandleHello(actor, session, role);
            return;
        }
        string capability = NimbyAdminServerRules.CapabilityFor(command);
        if (!NimbyAdminServerRules.RoleGrants(role, capability))
        {
            string denyReason = "capability";
            if (role == NimbyAdminRules.ROLE_NONE)
            {
                denyReason = "no role";
            }
            Reject(actor, session, requestId, command, NimbyAdminRules.CODE_DENIED, denyReason);
            return;
        }
        if (command == NimbyAdminRules.CMD_PLAYERS)
        {
            HandlePlayers(actor, requestId);
            return;
        }
        if (command == NimbyAdminRules.CMD_TELEPORT_TO)
        {
            HandleTeleportTo(actor, session, requestId, args);
            return;
        }
    }

    // A denied or invalid request: one audit result record (bounded per sender and in aggregate),
    // one gated line,
    // and the result to the sender (plan line 3).
    protected static void Reject(NimbyAdminServerActor actor, NimbyAdminServerSession session, int requestId, string command, int code, string reason)
    {
        string resultName = NimbyAdminRules.CodeName(code);
        NimbyAdminLog.Gated("reject:" + actor.biId, "request " + resultName + " id=" + requestId.ToString() + " command=" + NimbyAdminServerRules.CommandForLog(command) + " reason=" + reason + " " + NimbyAdminLog.WhoFrom(actor.name, actor.biId, actor.steam64));
        float now = GetGame().GetTickTime();
        bool exempt = NimbyAdminServerRules.DenialRecordExempt(s_Permissions.RoleOf(actor.steam64));
        if (exempt || NimbyAdminServerRules.DenialRecordAllowed(DenialBucketFor(actor.biId), s_DenialAggregate, now))
        {
            NimbyAdminServerAuditRecord record = ActorRecord(actor, requestId, NimbyAdminRules.CutBytes(command, NimbyAdminRules.MAX_COMMAND_BYTES), "result");
            record.result = resultName;
            record.reason = reason;
            NimbyAdminServerAudit.Append(record);
        }
        else
        {
            if (s_Suppressed.Total() == 0)
            {
                s_SuppressedSince = now;
            }
            s_Suppressed.Add(actor.steam64);
        }
        SendResult(actor, requestId, code, reason);
    }

    // Keyed by BI id, like s_Rates, so a reconnect does not refill it (security review of 84a6fca).
    protected static NimbyAdminServerBucket DenialBucketFor(string biId)
    {
        NimbyAdminServerBucket bucket;
        if (!s_DenialRates.Find(biId, bucket))
        {
            bucket = new NimbyAdminServerBucket();
            s_DenialRates.Set(biId, bucket);
        }
        return bucket;
    }

    // One audit record for what the denial-record limits held back, SUMMARY_SECONDS after the
    // first held-back record; never subject to the limits itself.
    protected static void WriteSuppressedSummaryIfDue(float now)
    {
        if (s_Suppressed.Total() == 0)
        {
            return;
        }
        if (now - s_SuppressedSince < NimbyAdminServerSuppressed.SUMMARY_SECONDS)
        {
            return;
        }
        NimbyAdminServerAuditRecord record = SystemRecord("audit.suppressed", "result");
        record.result = "ok";
        record.reason = "denial record limit";
        record.effects = s_Suppressed.EffectsJson();
        NimbyAdminServerAudit.Append(record);
        NimbyAdminLog.Info("denied or invalid audit records held back suppressed=" + s_Suppressed.Total().ToString() + " senders=" + s_Suppressed.Senders().ToString() + "; one audit.suppressed record written");
        s_Suppressed.Clear();
    }

    protected static NimbyAdminServerAuditRecord ActorRecord(NimbyAdminServerActor actor, int requestId, string command, string phase)
    {
        NimbyAdminServerAuditRecord record = new NimbyAdminServerAuditRecord();
        record.id = requestId.ToString();
        record.phase = phase;
        record.actorSteam64 = actor.steam64;
        record.actorBiId = actor.biId;
        record.actorName = actor.name;
        record.command = command;
        return record;
    }

    protected static void SendResult(NimbyAdminServerActor actor, int requestId, int code, string detail)
    {
        if (!actor.player || !actor.identity)
        {
            return;
        }
        string cut = NimbyAdminRules.CutBytes(detail, NimbyAdminRules.MAX_DETAIL_BYTES);
        Param3<int, int, string> result = new Param3<int, int, string>(requestId, code, cut);
        GetGame().RPCSingleParam(actor.player, NimbyAdminRules.RPC_RESULT, result, true, actor.identity);
    }

    // Ruling 5: a hand edit reaches the next request once the cached copy is older than the interval.
    protected static void ReloadPermissionsIfDue(float now)
    {
        if (now - s_Permissions.m_LoadedAt < s_Config.permissionsReloadSeconds)
        {
            return;
        }
        string problem;
        bool changed = s_Permissions.Load(now, problem);
        s_Permissions.ApplyBootstrap();
        Alert(problem);
        if (changed)
        {
            NimbyAdminLog.Info("permissions.json reloaded source=" + s_Permissions.m_Source + " entries=" + s_Permissions.m_Active.Count().ToString());
            PushChangedRoles();
        }
    }

    protected static void PushChangedRoles()
    {
        foreach (string biId, NimbyAdminServerSession session : s_Sessions)
        {
            if (!session || !session.identity)
            {
                continue;
            }
            string role = s_Permissions.RoleOf(session.steam64);
            if (session.lastRole != "" && session.lastRole != role)
            {
                PushRole(session, role, "reload");
            }
        }
    }

    protected static void PushRole(NimbyAdminServerSession session, string role, string reason)
    {
        PlayerBase player = PlayerBase.Cast(session.identity.GetPlayer());
        if (!player)
        {
            return;
        }
        bool logIt = NimbyAdminServerRules.ShouldLogRoleSync(session.lastRole, role);
        session.pushSequence = session.pushSequence + 1;
        session.lastRole = role;
        Param3<int, int, string> push = new Param3<int, int, string>(NimbyAdminRules.PUSH_ROLE, session.pushSequence, role);
        GetGame().RPCSingleParam(player, NimbyAdminRules.RPC_PUSH, push, true, session.identity);
        if (!logIt)
        {
            NimbyAdminLog.Debug("role sync repeated session=" + session.number.ToString() + " role=" + role + " reason=" + reason);
            return;
        }
        NimbyAdminLog.Info("role sync " + NimbyAdminLog.WhoFrom(session.name, session.biId, session.steam64) + " role=" + role + " source=" + RoleSource(session.steam64) + " session=" + session.number.ToString() + " reason=" + reason);
    }

    protected static string RoleSource(string steam64)
    {
        if (s_Permissions.m_Active.Contains(steam64))
        {
            return s_Permissions.m_Source;
        }
        if (s_Permissions.m_BootstrapActive && steam64 == s_Permissions.m_Bootstrap)
        {
            return "bootstrap";
        }
        return "none";
    }

    // ---- handlers ----------------------------------------------------------------------------------

    // Ruling 8: the client asks after OnPlayerLoaded; the answer is a role push (no audit record:
    // nothing changes and nothing sensitive is returned). Review finding 6: a repeated hello is
    // answered every time but logged only when the role changed (PushRole).
    protected static void HandleHello(NimbyAdminServerActor actor, NimbyAdminServerSession session, string role)
    {
        PushRole(session, role, "hello");
    }

    protected static void HandlePlayers(NimbyAdminServerActor actor, int requestId)
    {
        NimbyAdminPlayerList list = new NimbyAdminPlayerList();
        vector here = actor.player.GetPosition();
        foreach (string biId, NimbyAdminServerSession session : s_Sessions)
        {
            if (!session || !session.identity)
            {
                continue;
            }
            PlayerBase player = PlayerBase.Cast(session.identity.GetPlayer());
            NimbyAdminPlayerRow row = new NimbyAdminPlayerRow();
            row.session = session.number;
            row.biId = session.biId;
            row.steam64 = session.steam64;
            row.name = session.name;
            row.alive = player && player.IsAlive();
            row.distance = -1;
            if (player)
            {
                row.distance = vector.Distance(here, player.GetPosition());
            }
            list.players.Insert(row);
        }
        JsonSerializer js = new JsonSerializer();
        string payload;
        if (!js.WriteToString(list, false, payload))
        {
            SendResult(actor, requestId, NimbyAdminRules.CODE_FAILED, "list serialize failed");
            return;
        }
        NimbyAdminDone done = new NimbyAdminDone();
        done.bytes = payload.Length();
        done.hash = payload.Hash();
        done.kind = "players";
        string doneJson;
        js.WriteToString(done, false, doneJson);
        int chunks = NimbySharedChunkSender.Send(actor.player, actor.identity, NimbyAdminRules.RPC_CHUNK, NimbyAdminRules.RPC_DONE, requestId, payload, doneJson);
        NimbyAdminLog.Debug("players list id=" + requestId.ToString() + " players=" + list.players.Count().ToString() + " bytes=" + payload.Length().ToString() + " chunks=" + chunks.ToString());
    }

    protected static void HandleTeleportTo(NimbyAdminServerActor actor, NimbyAdminServerSession session, int requestId, NimbyAdminArgs args)
    {
        string command = NimbyAdminRules.CMD_TELEPORT_TO;
        string argsWhy = NimbyAdminServerRules.CheckTeleportArgs(args);
        if (argsWhy != "")
        {
            Reject(actor, session, requestId, command, NimbyAdminRules.CODE_INVALID, argsWhy);
            return;
        }
        NimbyAdminServerSession targetSession;
        if (!s_Sessions.Find(args.Get("target"), targetSession) || !targetSession.identity)
        {
            Reject(actor, session, requestId, command, NimbyAdminRules.CODE_INVALID, "target session ended");
            return;
        }
        if (!NimbyAdminServerRules.SessionMatches(targetSession.number, args.Get("session")))
        {
            Reject(actor, session, requestId, command, NimbyAdminRules.CODE_INVALID, "target session ended");
            return;
        }
        PlayerBase targetPlayer = PlayerBase.Cast(targetSession.identity.GetPlayer());
        if (!targetPlayer || targetPlayer == actor.player)
        {
            Reject(actor, session, requestId, command, NimbyAdminRules.CODE_INVALID, "target is the actor");
            return;
        }
        if (!actor.player.IsAlive())
        {
            Reject(actor, session, requestId, command, NimbyAdminRules.CODE_INVALID, "actor dead");
            return;
        }
        if (!targetPlayer.IsAlive())
        {
            Reject(actor, session, requestId, command, NimbyAdminRules.CODE_INVALID, "target dead");
            return;
        }
        if (actor.player.IsInVehicle() || targetPlayer.IsInVehicle())
        {
            Reject(actor, session, requestId, command, NimbyAdminRules.CODE_INVALID, "in vehicle");
            return;
        }
        NimbyAdminServerJsonObject target = new NimbyAdminServerJsonObject();
        target.Add("steam64", targetSession.steam64);
        target.Add("biId", targetSession.biId);
        target.Add("name", targetSession.name);
        target.AddInt("session", targetSession.number);
        target.AddVector("position", targetPlayer.GetPosition());
        NimbyAdminServerJsonObject argsJson = new NimbyAdminServerJsonObject();
        argsJson.Add("target", args.Get("target"));
        argsJson.Add("session", args.Get("session"));
        NimbyAdminServerAuditRecord intent = ActorRecord(actor, requestId, command, "intent");
        intent.target = target.ToJson();
        intent.args = argsJson.ToJson();
        if (!NimbyAdminServerAudit.Append(intent))
        {
            NimbyAdminLog.Info("request failed id=" + requestId.ToString() + " command=" + command + " reason=audit unavailable " + NimbyAdminLog.WhoFrom(actor.name, actor.biId, actor.steam64));
            NimbyAdminServerDiscord.Enqueue("[Central Scrutinizer] audit unavailable: a " + command + " by " + actor.name + " (" + actor.steam64 + ") was blocked");
            SendResult(actor, requestId, NimbyAdminRules.CODE_FAILED, "audit unavailable");
            return;
        }
        vector from = actor.player.GetPosition();
        string offsetName;
        vector to = TeleportDestination(actor.player, targetPlayer, offsetName);
        actor.player.SetPosition(to);
        float distance = vector.Distance(from, to);
        NimbyAdminServerJsonObject effects = new NimbyAdminServerJsonObject();
        effects.AddVector("from", from);
        effects.AddVector("to", to);
        effects.Add("offset", offsetName);
        NimbyAdminServerAuditRecord result = ActorRecord(actor, requestId, command, "result");
        result.target = intent.target;
        result.args = intent.args;
        result.result = "ok";
        result.effects = effects.ToJson();
        if (!NimbyAdminServerAudit.Append(result))
        {
            NimbyAdminLog.Info("audit result not written id=" + requestId.ToString() + " (the intent record stands)");
        }
        NimbyAdminLog.Info("teleport done id=" + requestId.ToString() + " actor " + NimbyAdminLog.WhoFrom(actor.name, actor.biId, actor.steam64) + " target " + NimbyAdminLog.WhoFrom(targetSession.name, targetSession.biId, targetSession.steam64) + " offset=" + offsetName + " distance=" + distance.ToString());
        NimbyAdminServerDiscord.Enqueue("[Central Scrutinizer] " + actor.name + " (" + actor.steam64 + ") teleported to " + targetSession.name + " (" + targetSession.steam64 + "), " + Math.Round(distance).ToString() + " m, id " + requestId.ToString());
        SendResult(actor, requestId, NimbyAdminRules.CODE_OK, "teleported to " + targetSession.name);
    }

    // 1 m behind the target, then right, left, front; a 0.6 x 1.8 x 0.6 m box that touches nothing
    // but the two players; otherwise the target's own spot 0.2 m up. The target's height is kept,
    // so a target indoors or upstairs is reached where it stands.
    protected static vector TeleportDestination(PlayerBase actor, PlayerBase target, out string offsetName)
    {
        vector spot = target.GetPosition();
        vector forward = target.GetDirection();
        forward[1] = 0;
        float length = vector.Distance("0 0 0", forward);
        if (length < 0.001)
        {
            forward = "0 0 1";
        }
        else
        {
            forward = forward * (1.0 / length);
        }
        vector right = Vector(forward[2], 0, -forward[0]);
        array<vector> offsets = new array<vector>;
        array<string> names = new array<string>;
        offsets.Insert(forward * -1.0);
        names.Insert("behind");
        offsets.Insert(right);
        names.Insert("right");
        offsets.Insert(right * -1.0);
        names.Insert("left");
        offsets.Insert(forward);
        names.Insert("front");
        array<Object> exclude = new array<Object>;
        exclude.Insert(actor);
        exclude.Insert(target);
        int i = 0;
        while (i < offsets.Count())
        {
            vector candidate = spot + offsets.Get(i);
            vector center = candidate + "0 0.95 0";
            if (!GetGame().IsBoxColliding(center, target.GetOrientation(), "0.6 1.8 0.6", exclude))
            {
                offsetName = names.Get(i);
                return candidate;
            }
            i = i + 1;
        }
        offsetName = "on target";
        return spot + "0 0.2 0";
    }
}

modded class PlayerBase extends ManBase
{
    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        if (rpc_type == NimbyAdminRules.RPC_REQUEST)
        {
            if (GetGame().IsServer())
            {
                NimbyAdminServerCore.HandleRequest(this, sender, ctx);
            }
            return;
        }
        super.OnRPC(sender, rpc_type, ctx);
    }
}
