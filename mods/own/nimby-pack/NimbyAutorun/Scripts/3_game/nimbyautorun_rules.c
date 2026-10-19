// -----------------------------------------------------------------------------
// NimbyAutorun - Legs. Engine-free rules.
// Purpose : Every decision Legs makes that does not need the world, as pure
//           functions and two small state classes, so the self-test checks them
//           without a client (shared rules 8): request validation (movement index,
//           angle), the server's speed resolution, the client's speed and start
//           rules, the menu and focus stop rule, the double-tap guard (A5), the
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
    // selects sprint: under A5 it stops autorun.
    static int ClientSpeed(int currentIdx, bool canSprint, bool hasStamina, bool walkTogglePressed, bool walkTempPressed)
    {
        if (walkTogglePressed)
        {
            return DayZPlayerConstants.MOVEMENTIDX_WALK;
        }
        if (walkTempPressed)
        {
            return DayZPlayerConstants.MOVEMENTIDX_RUN;
        }
        if (currentIdx == DayZPlayerConstants.MOVEMENTIDX_SPRINT && (!canSprint || !hasStamina))
        {
            return DayZPlayerConstants.MOVEMENTIDX_RUN;
        }
        return currentIdx;
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

    // Clamp the config; returns how many fields were changed.
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

        failures += Case(results, "client sprint holds", ClientSpeed(sprint, true, true, false, false) == sprint);
        failures += Case(results, "client sprint drops on stamina", ClientSpeed(sprint, true, false, false, false) == run);
        failures += Case(results, "client sprint drops on cannot sprint", ClientSpeed(sprint, false, true, false, false) == run);
        failures += Case(results, "client walk stays walk without stamina", ClientSpeed(walk, true, false, false, false) == walk);
        failures += Case(results, "client walk toggle slows to walk", ClientSpeed(sprint, true, true, true, false) == walk);
        failures += Case(results, "client walk toggle wins over walk temp", ClientSpeed(sprint, true, true, true, true) == walk);
        failures += Case(results, "client walk temp goes to run", ClientSpeed(walk, true, true, false, true) == run);

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
        failures += Case(results, "config default version 1", defaults.version == 1);
        failures += Case(results, "config default ten per second", defaults.requestsPerSecond == 10);
        failures += Case(results, "sanitize leaves defaults alone", Sanitize(defaults) == 0);
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
