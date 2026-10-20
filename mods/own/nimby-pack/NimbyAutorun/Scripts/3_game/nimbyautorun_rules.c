// -----------------------------------------------------------------------------
// NimbyAutorun - Legs. Engine-free rules.
// Purpose : Every decision Legs makes that does not need the world, as pure
//           functions and two small state classes, so the self-test checks them
//           without a client (shared rules 8): request validation (movement index,
//           angle), the server's speed resolution, the client's speed and start
//           rules, the ladder and swim rule (stopOnSwim, NIMBY-T-006), the menu
//           and focus stop rule, the double-tap guard (A5), the
//           per-sender request throttle with its stop bypass, the log gate, and
//           the config clamp. The 4_world and 5_mission layers only feed them
//           facts they read from the game.
// -----------------------------------------------------------------------------
class NimbyAutorunRpc
{
    // Shared rules 4.1. Distinct from the original (-44201) so both mods can coexist.
    // Client to server: Param3<bool running, int movementIdx, int angle>.
    static const int TOGGLE = 6550101;
    // Server to the owning client (A4): Param1<string reason>. The client clears its override
    // and remembered state and does not re-send start.
    static const int CANCEL = 6550102;
    // Server to the owning client, round 3: Param1<bool stopOnSwim>, sent with every accepted
    // start so the client's swim rule follows the server config. Inside the Legs block (shared
    // rules 4.1 lists 6550101 and 6550102; this id is reported to the orchestrator).
    static const int SETTINGS = 6550103;
}

// Request budget per sender. A fixed one-second window: the first `limit` requests in a window
// are admitted, the rest dropped and counted. A stop is admitted without being counted.
class NimbyAutorunThrottle
{
    static const float WINDOW_SECONDS = 1.0;

    protected int m_Limit;
    protected float m_WindowStart = -1000.0;
    protected int m_Count;
    protected int m_Dropped;
    protected bool m_CancelSent;

    void NimbyAutorunThrottle(int limit)
    {
        m_Limit = limit;
    }

    bool Admit(float now, bool isStop)
    {
        if (isStop)
        {
            return true;
        }
        // A clock that moved backwards (a new mission) also opens a new window.
        if (now - m_WindowStart >= WINDOW_SECONDS || now < m_WindowStart)
        {
            m_WindowStart = now;
            m_Count = 0;
            m_CancelSent = false;
        }
        if (m_Count >= m_Limit)
        {
            m_Dropped = m_Dropped + 1;
            return false;
        }
        m_Count = m_Count + 1;
        return true;
    }

    // At most one cancellation RPC per window for dropped requests, so a flood is not answered
    // with a flood.
    bool TakeCancel()
    {
        if (m_CancelSent)
        {
            return false;
        }
        m_CancelSent = true;
        return true;
    }

    // Drops since the last call; the Throttled log line reports this as count=<n>.
    int TakeDropped()
    {
        int dropped = m_Dropped;
        m_Dropped = 0;
        return dropped;
    }
}

// One log line per interval. The rejection log and the throttle log each own one, so neither
// limit is shared with the request throttle (plan, Phase 1 server).
class NimbyAutorunLogGate
{
    protected float m_Last = -1000.0;

    bool Allow(float now, float interval)
    {
        if (now - m_Last < interval && now >= m_Last)
        {
            return false;
        }
        m_Last = now;
        return true;
    }
}

class NimbyAutorunRules
{
    // Minimum seconds between rejection lines, and between throttle lines, for one player.
    static const float LOG_INTERVAL_SECONDS = 10.0;
    // The PC keyboard doubleclick limit (bin/constants.xml:457). After a stop caused by a
    // UATurbo press, a toggle inside this window is the second press of the same double tap and
    // does not start autorun again (A5).
    static const float DOUBLE_TAP_GUARD_SECONDS = 0.5;
    // After chat, gestures, or the radial quickbar closes, the input exclude it set may outlive
    // the menu by a frame or more (GesturesMenu.c:143-148 removes it in the destructor). Inside
    // this window a control-disabled state is not a stop.
    static const float MENU_GRACE_SECONDS = 0.5;
    static const int REQUESTS_PER_SECOND_MIN = 1;
    static const int REQUESTS_PER_SECOND_MAX = 100;
    // NimbyAutorunConfig.version written by this build.
    static const int CONFIG_VERSION = 2;

    static bool IsValidMovement(int movementIdx)
    {
        if (movementIdx == DayZPlayerConstants.MOVEMENTIDX_WALK)
        {
            return true;
        }
        if (movementIdx == DayZPlayerConstants.MOVEMENTIDX_RUN)
        {
            return true;
        }
        return movementIdx == DayZPlayerConstants.MOVEMENTIDX_SPRINT;
    }

    // The client always sends angle 1 while running and 0 on a stop; a stop is valid whatever it
    // carries, because stopping is always safe.
    static bool IsValidAngle(bool running, int angle)
    {
        if (!running)
        {
            return true;
        }
        return angle == 1;
    }

    // "" when a running request is well formed, otherwise the reason for the Rejected line.
    static string ValidateRequest(bool running, int movementIdx, int angle)
    {
        if (!running)
        {
            return "";
        }
        if (!IsValidMovement(movementIdx) || !IsValidAngle(running, angle))
        {
            return "movement out of range: idx=" + movementIdx.ToString() + " angle=" + angle.ToString();
        }
        return "";
    }

    // The movement index the server applies: sprint drops to run when the player cannot sprint.
    static int ResolveServerIdx(int movementIdx, bool canSprint)
    {
        if (movementIdx == DayZPlayerConstants.MOVEMENTIDX_SPRINT && !canSprint)
        {
            return DayZPlayerConstants.MOVEMENTIDX_RUN;
        }
        return movementIdx;
    }

    // Autorun starts at sprint when the player can sprint and has stamina, otherwise at run.
    static int StartIdx(bool canSprint, bool hasStamina)
    {
        if (canSprint && hasStamina)
        {
            return DayZPlayerConstants.MOVEMENTIDX_SPRINT;
        }
        return DayZPlayerConstants.MOVEMENTIDX_RUN;
    }

    // The client's speed while autorun stays on. The walk toggle (double tap Left Ctrl,
    // preset_X1MouseKey.xml:27-31) slows to walk; a single Left Ctrl press (UAWalkRunTemp, :24-26)
    // goes to run; sprint drops to run when sprint or stamina runs out. A Shift press no longer
    // selects sprint: under A5 it stops autorun. A6: run returns to sprint once sprint and
    // stamina allow again, as a held Shift would, unless speedChosen says the player picked walk
    // or run with Left Ctrl since the start; that choice holds until the next start.
    static int ClientSpeed(int currentIdx, bool canSprint, bool hasStamina, bool speedChosen, bool walkTogglePressed, bool walkTempPressed)
    {
        if (walkTogglePressed)
        {
            return DayZPlayerConstants.MOVEMENTIDX_WALK;
        }
        if (walkTempPressed)
        {
            return DayZPlayerConstants.MOVEMENTIDX_RUN;
        }
        bool sprintAllowed = canSprint && hasStamina;
        if (currentIdx == DayZPlayerConstants.MOVEMENTIDX_SPRINT && !sprintAllowed)
        {
            return DayZPlayerConstants.MOVEMENTIDX_RUN;
        }
        if (currentIdx == DayZPlayerConstants.MOVEMENTIDX_RUN && sprintAllowed && !speedChosen)
        {
            return DayZPlayerConstants.MOVEMENTIDX_SPRINT;
        }
        return currentIdx;
    }

    // A ladder always hands control back to vanilla movement. Water does so only while
    // stopOnSwim is true (NIMBY-T-006 experiment); with it false the swim command is not a stop
    // and not a start refusal, and only the in-game run shows whether the override moves the
    // swimmer.
    static string MovementCause(bool onLadder, bool swimming, bool stopOnSwim)
    {
        if (onLadder)
        {
            return "ladder";
        }
        if (swimming && stopOnSwim)
        {
            return "swim";
        }
        return "";
    }

    // Whether disabled controls count against autorun, from the facts vanilla
    // MissionGameplay.IsControlDisabled reads (missionGameplay.c:1119-1131): any input exclude
    // group, any restriction, or the legacy mode. Vanilla adds the "swimming" group on every swim
    // start (PlayerBase.c:4248) and it excludes only the weapon raise (bin/specific.xml:407-410),
    // so movement stays live; that group alone counts only while stopOnSwim is true (NIMBY-T-006
    // round 4). "ladderclimbing" needs no case: the ladder is a movement cause first.
    static bool ControlDisabledForAutorun(int excludeCount, bool swimmingActive, int restrictionCount, bool legacyDisabled, bool stopOnSwim)
    {
        if (restrictionCount > 0)
        {
            return true;
        }
        if (legacyDisabled)
        {
            return true;
        }
        int otherExcludes = excludeCount;
        if (swimmingActive)
        {
            otherExcludes = otherExcludes - 1;
        }
        if (otherExcludes > 0)
        {
            return true;
        }
        return swimmingActive && stopOnSwim;
    }

    // Why a double tap does not start autorun, or "" when it starts. The first cause in this
    // order wins: player state, ladder or water, an open menu or focus loss, disabled controls,
    // the raised weapon (A7), then the double-tap guard (A5).
    static string StartRefusal(string blockedCause, string movementCause, string menuCause, bool controlDisabled, bool raised, bool guardActive)
    {
        if (blockedCause != "")
        {
            return blockedCause;
        }
        if (movementCause != "")
        {
            return movementCause;
        }
        if (menuCause != "")
        {
            return menuCause;
        }
        if (controlDisabled)
        {
            return "control-disabled";
        }
        if (raised)
        {
            return "raised";
        }
        if (guardActive)
        {
            return "double-tap-guard";
        }
        return "";
    }

    // A5: the second press of a double tap that a UATurbo press just stopped is not a start.
    static bool SwallowStart(float now, float turboStopAt)
    {
        return now >= turboStopAt && now - turboStopAt < DOUBLE_TAP_GUARD_SECONDS;
    }

    // Chat, gestures, and the radial quickbar never stop autorun (A5). Menu ids: constants.c:181,
    // :192, :200, :204.
    static bool IsPassThroughMenu(int menuId)
    {
        if (menuId == MENU_CHAT_INPUT || menuId == MENU_CHAT)
        {
            return true;
        }
        return menuId == MENU_GESTURES || menuId == MENU_RADIAL_QUICKBAR;
    }

    static string MenuName(int menuId)
    {
        if (menuId == MENU_INVENTORY)
        {
            return "inventory";
        }
        if (menuId == MENU_INGAME)
        {
            return "pause";
        }
        return "menu-" + menuId.ToString();
    }

    // The interrupt rule for focus, dialogs, menus, and disabled controls. "" means no stop;
    // anything else is the cause for the Toggle line. passThroughSeenAt is the last time a
    // pass-through menu was seen open.
    static string MenuStopCause(bool appActive, bool dialogVisible, bool hasMenu, int menuId, bool controlDisabled, float now, float passThroughSeenAt)
    {
        if (!appActive)
        {
            return "focus";
        }
        if (dialogVisible)
        {
            return "dialog";
        }
        if (hasMenu)
        {
            if (IsPassThroughMenu(menuId))
            {
                return "";
            }
            return MenuName(menuId);
        }
        if (controlDisabled)
        {
            if (now >= passThroughSeenAt && now - passThroughSeenAt < MENU_GRACE_SECONDS)
            {
                return "";
            }
            return "control-disabled";
        }
        return "";
    }

    // Clamp the config; returns how many fields were changed. stopOnSwim needs no clamp: a bool
    // has no out-of-range value, and a value JSON cannot read as a bool fails the whole load,
    // which falls back to the defaults (NimbyAutorunConfigLoader.Load).
    static int Sanitize(NimbyAutorunConfig config)
    {
        int changed = 0;
        int before = config.requestsPerSecond;
        if (before < REQUESTS_PER_SECOND_MIN)
        {
            config.requestsPerSecond = REQUESTS_PER_SECOND_MIN;
        }
        if (before > REQUESTS_PER_SECOND_MAX)
        {
            config.requestsPerSecond = REQUESTS_PER_SECOND_MAX;
        }
        if (config.requestsPerSecond != before)
        {
            changed = changed + 1;
        }
        return changed;
    }

    // Engine-free validators for the $profile:NimbyAutorun\selftest flag (shared rules 8).
    static int SelfTest(out array<string> results)
    {
        int failures = 0;
        int idle = DayZPlayerConstants.MOVEMENTIDX_IDLE;
        int walk = DayZPlayerConstants.MOVEMENTIDX_WALK;
        int run = DayZPlayerConstants.MOVEMENTIDX_RUN;
        int sprint = DayZPlayerConstants.MOVEMENTIDX_SPRINT;

        failures += Case(results, "walk index valid", IsValidMovement(walk));
        failures += Case(results, "run index valid", IsValidMovement(run));
        failures += Case(results, "sprint index valid", IsValidMovement(sprint));
        failures += Case(results, "idle index refused for running", !IsValidMovement(idle));
        failures += Case(results, "index 4 refused", !IsValidMovement(4));
        failures += Case(results, "index -1 refused", !IsValidMovement(-1));
        failures += Case(results, "running angle 1 valid", IsValidAngle(true, 1));
        failures += Case(results, "running angle 0 refused", !IsValidAngle(true, 0));
        failures += Case(results, "running angle 2 refused", !IsValidAngle(true, 2));
        failures += Case(results, "stop valid with any angle", IsValidAngle(false, 7));
        failures += Case(results, "request sprint angle 1 accepted", ValidateRequest(true, sprint, 1) == "");
        failures += Case(results, "request idx 9 rejected", ValidateRequest(true, 9, 1) == "movement out of range: idx=9 angle=1");
        failures += Case(results, "request bad angle rejected", ValidateRequest(true, run, 3) != "");
        failures += Case(results, "stop request with junk accepted", ValidateRequest(false, 99, 99) == "");

        failures += Case(results, "server keeps sprint when allowed", ResolveServerIdx(sprint, true) == sprint);
        failures += Case(results, "server drops sprint to run", ResolveServerIdx(sprint, false) == run);
        failures += Case(results, "server keeps walk without sprint", ResolveServerIdx(walk, false) == walk);
        failures += Case(results, "start at sprint", StartIdx(true, true) == sprint);
        failures += Case(results, "start at run without stamina", StartIdx(true, false) == run);
        failures += Case(results, "start at run without sprint", StartIdx(false, true) == run);

        failures += Case(results, "client sprint holds", ClientSpeed(sprint, true, true, false, false, false) == sprint);
        failures += Case(results, "client sprint drops on stamina", ClientSpeed(sprint, true, false, false, false, false) == run);
        failures += Case(results, "client sprint drops on cannot sprint", ClientSpeed(sprint, false, true, false, false, false) == run);
        failures += Case(results, "client walk stays walk without stamina", ClientSpeed(walk, true, false, false, false, false) == walk);
        failures += Case(results, "client walk toggle slows to walk", ClientSpeed(sprint, true, true, false, true, false) == walk);
        failures += Case(results, "client walk toggle wins over walk temp", ClientSpeed(sprint, true, true, false, true, true) == walk);
        failures += Case(results, "client walk temp goes to run", ClientSpeed(walk, true, true, false, false, true) == run);
        failures += Case(results, "A6 run returns to sprint when allowed", ClientSpeed(run, true, true, false, false, false) == sprint);
        failures += Case(results, "A6 run stays run without stamina", ClientSpeed(run, true, false, false, false, false) == run);
        failures += Case(results, "A6 run stays run without sprint", ClientSpeed(run, false, true, false, false, false) == run);
        failures += Case(results, "A6 Left Ctrl run choice holds", ClientSpeed(run, true, true, true, false, false) == run);
        failures += Case(results, "A6 Left Ctrl walk choice holds", ClientSpeed(walk, true, true, true, false, false) == walk);
        failures += Case(results, "A6 walk temp at sprint goes to run", ClientSpeed(sprint, true, true, true, false, true) == run);

        failures += Case(results, "A7 start refused while raised", StartRefusal("", "", "", false, true, false) == "raised");
        failures += Case(results, "A7 start allowed when lowered", StartRefusal("", "", "", false, false, false) == "");
        failures += Case(results, "start refusal state wins over raised", StartRefusal("dead", "", "", false, true, false) == "dead");
        failures += Case(results, "start refusal ladder", StartRefusal("", "ladder", "", false, false, false) == "ladder");
        failures += Case(results, "start refusal menu", StartRefusal("", "", "inventory", true, false, false) == "inventory");
        failures += Case(results, "start refusal control disabled", StartRefusal("", "", "", true, true, false) == "control-disabled");
        failures += Case(results, "start refusal raised wins over guard", StartRefusal("", "", "", false, true, true) == "raised");
        failures += Case(results, "start refusal double-tap guard", StartRefusal("", "", "", false, false, true) == "double-tap-guard");

        failures += Case(results, "ladder stops with stopOnSwim true", MovementCause(true, false, true) == "ladder");
        failures += Case(results, "ladder stops with stopOnSwim false", MovementCause(true, false, false) == "ladder");
        failures += Case(results, "ladder wins over swim", MovementCause(true, true, true) == "ladder");
        failures += Case(results, "swim stops with stopOnSwim true", MovementCause(false, true, true) == "swim");
        failures += Case(results, "swim ignored with stopOnSwim false", MovementCause(false, true, false) == "");
        failures += Case(results, "dry land no movement cause", MovementCause(false, false, true) == "");
        failures += Case(results, "swim start refused with stopOnSwim true", StartRefusal("", MovementCause(false, true, true), "", false, false, false) == "swim");
        failures += Case(results, "swim start allowed with stopOnSwim false", StartRefusal("", MovementCause(false, true, false), "", false, false, false) == "");

        failures += Case(results, "swimming group alone not disabled with stopOnSwim false", !ControlDisabledForAutorun(1, true, 0, false, false));
        failures += Case(results, "swimming group alone disabled with stopOnSwim true", ControlDisabledForAutorun(1, true, 0, false, true));
        failures += Case(results, "swimming group plus menu disabled", ControlDisabledForAutorun(2, true, 0, false, false));
        failures += Case(results, "restriction alone disabled", ControlDisabledForAutorun(0, false, 1, false, false));
        failures += Case(results, "nothing active not disabled", !ControlDisabledForAutorun(0, false, 0, false, true));
        failures += Case(results, "legacy mode alone disabled", ControlDisabledForAutorun(0, false, 0, true, false));
        failures += Case(results, "swimming with stopOnSwim false is no menu stop", MenuStopCause(true, false, false, 0, ControlDisabledForAutorun(1, true, 0, false, false), 5.0, -1000.0) == "");
        failures += Case(results, "swim start allowed past swimming group", StartRefusal("", MovementCause(false, true, false), "", ControlDisabledForAutorun(1, true, 0, false, false), false, false) == "");

        failures += Case(results, "double tap second press swallowed", SwallowStart(10.2, 10.0));
        failures += Case(results, "start allowed after guard", !SwallowStart(10.6, 10.0));
        failures += Case(results, "start allowed with no turbo stop", !SwallowStart(10.0, -1000.0));

        failures += Case(results, "chat input passes through", IsPassThroughMenu(MENU_CHAT_INPUT));
        failures += Case(results, "gestures pass through", IsPassThroughMenu(MENU_GESTURES));
        failures += Case(results, "quickbar passes through", IsPassThroughMenu(MENU_RADIAL_QUICKBAR));
        failures += Case(results, "inventory does not pass through", !IsPassThroughMenu(MENU_INVENTORY));
        failures += Case(results, "focus loss stops", MenuStopCause(false, false, false, 0, false, 5.0, -1000.0) == "focus");
        failures += Case(results, "dialog stops", MenuStopCause(true, true, false, 0, false, 5.0, -1000.0) == "dialog");
        failures += Case(results, "inventory stops", MenuStopCause(true, false, true, MENU_INVENTORY, true, 5.0, -1000.0) == "inventory");
        failures += Case(results, "pause stops", MenuStopCause(true, false, true, MENU_INGAME, true, 5.0, -1000.0) == "pause");
        failures += Case(results, "other menu stops", MenuStopCause(true, false, true, MENU_MAP, false, 5.0, -1000.0) == "menu-22");
        failures += Case(results, "chat open does not stop", MenuStopCause(true, false, true, MENU_CHAT_INPUT, true, 5.0, 5.0) == "");
        failures += Case(results, "control disabled stops", MenuStopCause(true, false, false, 0, true, 5.0, -1000.0) == "control-disabled");
        failures += Case(results, "control disabled inside grace passes", MenuStopCause(true, false, false, 0, true, 5.2, 5.0) == "");
        failures += Case(results, "control disabled after grace stops", MenuStopCause(true, false, false, 0, true, 5.6, 5.0) == "control-disabled");
        failures += Case(results, "nothing open does not stop", MenuStopCause(true, false, false, 0, false, 5.0, -1000.0) == "");

        NimbyAutorunThrottle throttle = new NimbyAutorunThrottle(3);
        failures += Case(results, "throttle admits first", throttle.Admit(1.0, false));
        failures += Case(results, "throttle admits second", throttle.Admit(1.1, false));
        failures += Case(results, "throttle admits third", throttle.Admit(1.2, false));
        failures += Case(results, "throttle drops fourth in window", !throttle.Admit(1.3, false));
        failures += Case(results, "throttle stop bypasses full window", throttle.Admit(1.4, true));
        failures += Case(results, "throttle drops fifth in window", !throttle.Admit(1.5, false));
        failures += Case(results, "throttle counts two drops", throttle.TakeDropped() == 2);
        failures += Case(results, "throttle drop count resets", throttle.TakeDropped() == 0);
        failures += Case(results, "throttle first cancel sent", throttle.TakeCancel());
        failures += Case(results, "throttle second cancel held", !throttle.TakeCancel());
        failures += Case(results, "throttle window resets after one second", throttle.Admit(2.0, false));
        failures += Case(results, "throttle cancel rearms with window", throttle.TakeCancel());
        failures += Case(results, "throttle resets on clock rewind", throttle.Admit(0.5, false));
        NimbyAutorunThrottle stops = new NimbyAutorunThrottle(1);
        int admittedStops = 0;
        for (int i = 0; i < 50; i++)
        {
            if (stops.Admit(3.0, true))
            {
                admittedStops = admittedStops + 1;
            }
        }
        failures += Case(results, "throttle admits fifty stops", admittedStops == 50);
        failures += Case(results, "throttle stops leave budget", stops.Admit(3.0, false));

        NimbyAutorunLogGate gate = new NimbyAutorunLogGate();
        failures += Case(results, "log gate allows first", gate.Allow(100.0, LOG_INTERVAL_SECONDS));
        failures += Case(results, "log gate holds inside interval", !gate.Allow(105.0, LOG_INTERVAL_SECONDS));
        failures += Case(results, "log gate allows after interval", gate.Allow(110.0, LOG_INTERVAL_SECONDS));

        NimbyAutorunConfig defaults = new NimbyAutorunConfig();
        failures += Case(results, "config default version 2", defaults.version == CONFIG_VERSION && CONFIG_VERSION == 2);
        failures += Case(results, "config default ten per second", defaults.requestsPerSecond == 10);
        failures += Case(results, "config default stopOnSwim true", defaults.stopOnSwim);
        failures += Case(results, "sanitize leaves defaults alone", Sanitize(defaults) == 0);
        NimbyAutorunConfig wet = new NimbyAutorunConfig();
        wet.stopOnSwim = false;
        failures += Case(results, "sanitize leaves stopOnSwim false alone", Sanitize(wet) == 0);
        failures += Case(results, "sanitize keeps stopOnSwim false", !wet.stopOnSwim);
        NimbyAutorunConfig dry = new NimbyAutorunConfig();
        dry.stopOnSwim = true;
        dry.requestsPerSecond = 0;
        Sanitize(dry);
        failures += Case(results, "sanitize keeps stopOnSwim true beside a clamp", dry.stopOnSwim);
        NimbyAutorunConfig low = new NimbyAutorunConfig();
        low.requestsPerSecond = 0;
        failures += Case(results, "sanitize counts low budget", Sanitize(low) == 1);
        failures += Case(results, "sanitize raises budget to 1", low.requestsPerSecond == REQUESTS_PER_SECOND_MIN);
        NimbyAutorunConfig high = new NimbyAutorunConfig();
        high.requestsPerSecond = 5000;
        Sanitize(high);
        failures += Case(results, "sanitize caps budget at 100", high.requestsPerSecond == REQUESTS_PER_SECOND_MAX);

        failures += Case(results, "toggle rpc id", NimbyAutorunRpc.TOGGLE == 6550101);
        failures += Case(results, "cancel rpc id", NimbyAutorunRpc.CANCEL == 6550102);
        failures += Case(results, "settings rpc id", NimbyAutorunRpc.SETTINGS == 6550103);

        failures += Case(results, "who from cache names the player", NimbyAutorunLog.WhoFrom("Road Dog", "BIHASH=", "76561190000000000") == "player=Road Dog (BIHASH=, 76561190000000000)");
        failures += Case(results, "who from empty cache is unknown", NimbyAutorunLog.WhoFrom("", "", "") == "player=unknown (unknown, unknown)");
        failures += Case(results, "who without identity is unknown", NimbyAutorunLog.Who(null) == "player=unknown (unknown, unknown)");
        return failures;
    }

    protected static int Case(out array<string> results, string name, bool passed)
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
