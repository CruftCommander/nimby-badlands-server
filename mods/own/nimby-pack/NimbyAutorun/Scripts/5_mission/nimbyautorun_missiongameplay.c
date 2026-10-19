// -----------------------------------------------------------------------------
// NimbyAutorun - Legs. Client side.
// Purpose : Reads the toggle and the stop inputs, drives the local movement
//           override, and asks the server to mirror it, sending only changes.
// Origin  : Fork of SchanaModAutorun missiongameplay.c (Apache-2.0). See NOTICE.
// Behavior: A double tap of Left Shift starts autorun at sprint, or at run when
//           the player cannot sprint; sprint drops to run when stamina runs out.
//           Stamina drains normally; autorun is not a stamina cheat. A5: while
//           running, any Shift press, WASD, Space, fire, raise, lean (A2), or the
//           toggle stops it, and so do focus loss, a dialog, any menu but chat,
//           gestures, and the radial quickbar, disabled controls, a ladder, and
//           water. Nothing resumes on its own; the player double-taps again.
// Plan    : docs/plans/2026-10-15-autorun-plan.md, Phase 1.
// -----------------------------------------------------------------------------

modded class MissionGameplay extends MissionBase
{
    static const string NIMBY_AUTORUN_TOGGLE = "UANimbyAutorunToggle";

    // Inputs whose press stops autorun (A5). UATurbo is read on its own because its press also
    // arms the double-tap guard.
    protected ref array<string> m_NimbyAutorunStopInputs;
    // The player the client state belongs to. A respawn or a replacement brings a new object,
    // whose state starts stopped; the old one is cleared locally.
    protected PlayerBase m_NimbyAutorunPlayer;
    protected float m_NimbyAutorunTurboStopAt = -1000.0;
    protected float m_NimbyAutorunPassThroughAt = -1000.0;

    override void OnInit()
    {
        super.OnInit();

        // Names from bin/constants.xml; default keys from bin/preset_X1MouseKey.xml. 1.30 has no
        // UAJump: Space is UAGetOver (jump or vault, preset :51-56).
        m_NimbyAutorunStopInputs = new array<string>;
        m_NimbyAutorunStopInputs.Insert(NIMBY_AUTORUN_TOGGLE);
        m_NimbyAutorunStopInputs.Insert("UAMoveForward");
        m_NimbyAutorunStopInputs.Insert("UAMoveBack");
        m_NimbyAutorunStopInputs.Insert("UAMoveLeft");
        m_NimbyAutorunStopInputs.Insert("UAMoveRight");
        m_NimbyAutorunStopInputs.Insert("UAGetOver");
        m_NimbyAutorunStopInputs.Insert("UAFire");
        m_NimbyAutorunStopInputs.Insert("UATempRaiseWeapon");
        // A2, conditional: vanilla locks both lean inputs while sprinting
        // (DayZPlayerImplement.c:3013-3020), so a lean press is expected to stop autorun at run
        // or walk only. Phase 0 item 3 settles whether they stay in this list.
        m_NimbyAutorunStopInputs.Insert("UALeanLeft");
        m_NimbyAutorunStopInputs.Insert("UALeanRight");
    }

    // missionGameplay.c:266. Runs on disconnect and on every mission teardown.
    override void OnMissionFinish()
    {
        NimbyAutorunReset("mission-finish");
        super.OnMissionFinish();
    }

    protected void NimbyAutorunReset(string cause)
    {
        if (m_NimbyAutorunPlayer && m_NimbyAutorunPlayer.NimbyIsAutorunning())
        {
            m_NimbyAutorunPlayer.NimbyAutorunApply(false, DayZPlayerConstants.MOVEMENTIDX_IDLE);
            m_NimbyAutorunPlayer.NimbyAutorunLogToggle(m_NimbyAutorunPlayer.GetIdentity(), "client", cause);
        }
        m_NimbyAutorunPlayer = null;
        m_NimbyAutorunTurboStopAt = -1000.0;
        m_NimbyAutorunPassThroughAt = -1000.0;
    }

    protected bool NimbyAutorunCheckInput(string inputName)
    {
        if (!GetUApi())
        {
            return false;
        }

        UAInput inp = GetUApi().GetInputByName(inputName);
        return inp && inp.LocalPress();
    }

    protected bool NimbyAutorunHasStamina(PlayerBase player)
    {
        StaminaHandler stamina = player.GetStaminaHandler();
        if (!stamina)
        {
            return false;
        }
        return stamina.GetStamina() > 0;
    }

    // Focus, dialogs, menus, and disabled controls (NimbyAutorunRules.MenuStopCause). Runs every
    // frame so the pass-through grace window is tracked while autorun is off as well.
    protected string NimbyAutorunMenuCause(float now)
    {
        UIManager ui = GetGame().GetUIManager();
        UIScriptedMenu menu = ui.GetMenu();
        bool hasMenu = menu != null;
        int menuId = 0;
        if (hasMenu)
        {
            menuId = menu.GetID();
            if (NimbyAutorunRules.IsPassThroughMenu(menuId))
            {
                m_NimbyAutorunPassThroughAt = now;
            }
        }
        return NimbyAutorunRules.MenuStopCause(GetGame().IsAppActive(), ui.IsDialogVisible(), hasMenu, menuId, IsControlDisabled(), now, m_NimbyAutorunPassThroughAt);
    }

    // A ladder or water hands control back to vanilla movement (human.c:1512, :1520).
    protected string NimbyAutorunMovementCause(PlayerBase player)
    {
        if (player.GetCommand_Ladder())
        {
            return "ladder";
        }
        if (player.GetCommand_Swim())
        {
            return "swim";
        }
        return "";
    }

    protected string NimbyAutorunStopCause(PlayerBase player, string menuCause, float now)
    {
        string cause = player.NimbyAutorunBlockedReason();
        if (cause != "")
        {
            return cause;
        }
        cause = NimbyAutorunMovementCause(player);
        if (cause != "")
        {
            return cause;
        }
        if (menuCause != "")
        {
            return menuCause;
        }
        // A5: a single Shift press takes control back. When it is the first press of a double
        // tap, the guard below keeps the second press from starting autorun again.
        if (NimbyAutorunCheckInput("UATurbo"))
        {
            m_NimbyAutorunTurboStopAt = now;
            return "UATurbo";
        }
        foreach (string inputName : m_NimbyAutorunStopInputs)
        {
            if (NimbyAutorunCheckInput(inputName))
            {
                return inputName;
            }
        }
        return "";
    }

    protected string NimbyAutorunStartRefusal(PlayerBase player, string menuCause, float now)
    {
        string cause = player.NimbyAutorunBlockedReason();
        if (cause != "")
        {
            return cause;
        }
        cause = NimbyAutorunMovementCause(player);
        if (cause != "")
        {
            return cause;
        }
        if (menuCause != "")
        {
            return menuCause;
        }
        if (IsControlDisabled())
        {
            return "control-disabled";
        }
        if (NimbyAutorunRules.SwallowStart(now, m_NimbyAutorunTurboStopAt))
        {
            return "double-tap-guard";
        }
        return "";
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player != m_NimbyAutorunPlayer)
        {
            NimbyAutorunReset("player-replaced");
            m_NimbyAutorunPlayer = player;
        }
        if (!player)
        {
            return;
        }

        float now = GetGame().GetTickTime();
        string menuCause = NimbyAutorunMenuCause(now);

        if (player.NimbyIsAutorunning())
        {
            string stopCause = NimbyAutorunStopCause(player, menuCause, now);
            if (stopCause != "")
            {
                NimbyAutorunSync(player, false, DayZPlayerConstants.MOVEMENTIDX_IDLE, stopCause);
                return;
            }
            int current = player.NimbyGetAutorunIdx();
            int speed = NimbyAutorunRules.ClientSpeed(current, player.CanSprint(), NimbyAutorunHasStamina(player), NimbyAutorunCheckInput("UAWalkRunToggle"), NimbyAutorunCheckInput("UAWalkRunTemp"));
            if (speed != current)
            {
                NimbyAutorunSync(player, true, speed, "speed");
            }
            return;
        }

        if (!NimbyAutorunCheckInput(NIMBY_AUTORUN_TOGGLE))
        {
            return;
        }
        string refusal = NimbyAutorunStartRefusal(player, menuCause, now);
        if (refusal != "")
        {
            NimbyAutorunLog.Debug("Start refused " + NimbyAutorunLog.Who(player.GetIdentity()) + " cause=" + refusal);
            return;
        }
        int startIdx = NimbyAutorunRules.StartIdx(player.CanSprint(), NimbyAutorunHasStamina(player));
        NimbyAutorunSync(player, true, startIdx, "toggle");
    }

    // Applies a change locally and sends it; an unchanged state sends nothing (review CR-004).
    // A stop caused by death is local only: the server clears the body itself (EEKilled), and the
    // body may no longer carry the identity the ownership check needs.
    protected void NimbyAutorunSync(PlayerBase player, bool running, int movementIdx, string cause)
    {
        if (running == player.NimbyIsAutorunning() && movementIdx == player.NimbyGetAutorunIdx())
        {
            return;
        }
        player.NimbyAutorunApply(running, movementIdx);
        player.NimbyAutorunLogToggle(player.GetIdentity(), "client", cause);
        if (!running && cause == "dead")
        {
            return;
        }

        int angle = 0;
        if (running)
        {
            angle = 1;
        }
        auto request = new Param3<bool, int, int>(running, player.NimbyGetAutorunIdx(), angle);
        GetGame().RPCSingleParam(player, NimbyAutorunRpc.TOGGLE, request, true);
    }
}
