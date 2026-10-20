// -----------------------------------------------------------------------------
// NimbyAutorun - Legs. Client side.
// Purpose : Reads the toggle and the stop inputs, drives the local movement
//           override, and asks the server to mirror it, sending only changes.
// Origin  : Fork of SchanaModAutorun missiongameplay.c (Apache-2.0). See NOTICE.
// Behavior: A double tap of Left Shift starts autorun at sprint, or at run when
//           the player cannot sprint; sprint drops to run when stamina runs out
//           and returns once vanilla would let a held Shift sprint again, unless
//           the player chose walk or run with Left Ctrl (A6). A double tap while
//           the weapon is raised does nothing (A7). Stamina drains normally;
//           autorun is not a stamina cheat. A5: while
//           running, any Shift press, WASD, Space, fire, raise, lean (A2), or the
//           toggle stops it, and so do focus loss, a dialog, any menu but chat,
//           gestures, and the radial quickbar, disabled controls, a ladder, and
//           water while the server's stopOnSwim is true (NIMBY-T-006 experiment;
//           with it false, autorun carries on into the water, and the vanilla
//           swimming exclude group does not count as disabled controls). Nothing
//           resumes on its own; the player double-taps again.
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
    // A6: true once the player picks walk or run with Left Ctrl during this run; cleared on the
    // next start, so the choice holds until then and run never returns to sprint by itself.
    protected bool m_NimbyAutorunSpeedChosen;
    // The swim command as seen last frame (NimbyAutorunTrackSwim).
    protected bool m_NimbyAutorunSwimming;

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
        // A2, decided October 19, 2026: lean stays a stop input. The solo block showed a lean press
        // stops autorun at sprint and at run on 1.30, although vanilla locks both lean inputs on
        // sprint start (DayZPlayerImplement.c:3013-3020).
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
        m_NimbyAutorunSpeedChosen = false;
        m_NimbyAutorunSwimming = false;
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

    // The stamina half of vanilla's sprint gate for a held Shift (PlayerBase.c:3485, :4799-4810;
    // consumer thresholds StaminaHandler.c:65-103, :876-879): false once stamina is depleted, and
    // true again only above the sprint threshold, so sprint does not flicker at zero (A6).
    protected bool NimbyAutorunHasStamina(PlayerBase player)
    {
        return player.CanConsumeStamina(EStaminaConsumers.SPRINT);
    }

    // Vanilla IsControlDisabled (missionGameplay.c:1119-1131) is true while any input exclude
    // group is active, and every swim start adds "swimming" (PlayerBase.c:4248), which excludes
    // only the weapon raise (bin/specific.xml:407-410). Round 3 stopped on it with
    // cause=control-disabled although stopOnSwim was false, so the groups are read here and
    // NimbyAutorunRules.ControlDisabledForAutorun decides (NIMBY-T-006).
    protected bool NimbyAutorunControlDisabled(PlayerBase player)
    {
        int excludeCount = 0;
        bool swimmingActive = false;
        if (m_ActiveInputExcludeGroups)
        {
            excludeCount = m_ActiveInputExcludeGroups.Count();
            swimmingActive = m_ActiveInputExcludeGroups.Find("swimming") != -1;
        }
        int restrictionCount = 0;
        if (m_ActiveInputRestrictions)
        {
            restrictionCount = m_ActiveInputRestrictions.Count();
        }
        bool legacyDisabled = m_ControlDisabledMode >= INPUT_EXCLUDE_ALL;
        return NimbyAutorunRules.ControlDisabledForAutorun(excludeCount, swimmingActive, restrictionCount, legacyDisabled, player.NimbyAutorunStopOnSwim());
    }

    // Focus, dialogs, menus, and disabled controls (NimbyAutorunRules.MenuStopCause). Runs every
    // frame so the pass-through grace window is tracked while autorun is off as well.
    protected string NimbyAutorunMenuCause(PlayerBase player, float now)
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
        return NimbyAutorunRules.MenuStopCause(GetGame().IsAppActive(), ui.IsDialogVisible(), hasMenu, menuId, NimbyAutorunControlDisabled(player), now, m_NimbyAutorunPassThroughAt);
    }

    // A ladder hands control back to vanilla movement; water does only while the server's
    // stopOnSwim is true (human.c:1512, :1520; NimbyAutorunRules.MovementCause, NIMBY-T-006).
    protected string NimbyAutorunMovementCause(PlayerBase player)
    {
        bool onLadder = player.GetCommand_Ladder() != null;
        bool swimming = player.GetCommand_Swim() != null;
        return NimbyAutorunRules.MovementCause(onLadder, swimming, player.NimbyAutorunStopOnSwim());
    }

    // NIMBY-T-006 evidence: one line when the swim command starts or ends while autorun is on, so
    // the record can tell whether the override drove the swimmer. Tracked every frame, so a
    // transition while autorun is off is not reported later.
    protected void NimbyAutorunTrackSwim(PlayerBase player)
    {
        bool swimming = player.GetCommand_Swim() != null;
        if (swimming == m_NimbyAutorunSwimming)
        {
            return;
        }
        m_NimbyAutorunSwimming = swimming;
        if (!player.NimbyIsAutorunning())
        {
            return;
        }
        string line = "Swim " + NimbyAutorunLog.Who(player.GetIdentity()) + " entered=" + NimbyAutorunLog.Bool(swimming);
        line = line + " running=" + NimbyAutorunLog.Bool(player.NimbyIsAutorunning()) + " idx=" + player.NimbyGetAutorunIdx().ToString();
        NimbyAutorunLog.Debug(line);
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

    // Reads the facts NimbyAutorunRules.StartRefusal orders. A7: IsRaised is the movement state
    // (DayZPlayerImplement.c:4005, updated each CommandHandler at :2321).
    protected string NimbyAutorunStartRefusal(PlayerBase player, string menuCause, float now)
    {
        string blocked = player.NimbyAutorunBlockedReason();
        string movement = NimbyAutorunMovementCause(player);
        bool guardActive = NimbyAutorunRules.SwallowStart(now, m_NimbyAutorunTurboStopAt);
        bool controlDisabled = NimbyAutorunControlDisabled(player);
        return NimbyAutorunRules.StartRefusal(blocked, movement, menuCause, controlDisabled, player.IsRaised(), guardActive);
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
        // Before the stop check, so with stopOnSwim true the Swim line precedes cause=swim.
        NimbyAutorunTrackSwim(player);
        string menuCause = NimbyAutorunMenuCause(player, now);

        if (player.NimbyIsAutorunning())
        {
            string stopCause = NimbyAutorunStopCause(player, menuCause, now);
            if (stopCause != "")
            {
                NimbyAutorunSync(player, false, DayZPlayerConstants.MOVEMENTIDX_IDLE, stopCause);
                return;
            }
            bool walkToggle = NimbyAutorunCheckInput("UAWalkRunToggle");
            bool walkTemp = NimbyAutorunCheckInput("UAWalkRunTemp");
            if (walkToggle || walkTemp)
            {
                m_NimbyAutorunSpeedChosen = true;
            }
            int current = player.NimbyGetAutorunIdx();
            int speed = NimbyAutorunRules.ClientSpeed(current, player.CanSprint(), NimbyAutorunHasStamina(player), m_NimbyAutorunSpeedChosen, walkToggle, walkTemp);
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
        m_NimbyAutorunSpeedChosen = false;
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
