// -----------------------------------------------------------------------------
// NimbyAutorun - client side.
// Purpose : Reads the toggle and interrupt inputs, drives the local movement
//           override, and asks the server to mirror it.
// Origin  : Fork of SchanaModAutorun missiongameplay.c (Apache-2.0). See NOTICE.
// Behavior: Starts at sprint and drops to run when stamina runs out, as the
//           original did. Stamina drains normally; autorun is not a stamina cheat.
// -----------------------------------------------------------------------------

modded class MissionGameplay extends MissionBase
{
    ref array<string> m_NimbyAutorunInterruptInputs;

    // Last state sent to the server. NimbyAutorunGetUpdatedSpeed returns RUN every frame while
    // the player cannot sprint, so without this the client sent one guaranteed RPC per frame
    // (review CR-004). Only changes are sent now.
    protected bool m_NimbyAutorunLastSentRunning;
    protected int m_NimbyAutorunLastSentIdx = -1;
    // The player the last-sent state belongs to. A respawn creates a new player object, so the
    // remembered state is reset rather than suppressing the new player's first request.
    protected PlayerBase m_NimbyAutorunLastSentPlayer;

    override void OnInit()
    {
        super.OnInit();

        // Index 0 is the toggle; it both starts and interrupts.
        m_NimbyAutorunInterruptInputs = new array<string>;
        m_NimbyAutorunInterruptInputs.Insert("UANimbyAutorunToggle");
        m_NimbyAutorunInterruptInputs.Insert("UAMoveForward");
        m_NimbyAutorunInterruptInputs.Insert("UAMoveBack");
        m_NimbyAutorunInterruptInputs.Insert("UAMoveLeft");
        m_NimbyAutorunInterruptInputs.Insert("UAMoveRight");
        m_NimbyAutorunInterruptInputs.Insert("UAFire");
        m_NimbyAutorunInterruptInputs.Insert("UATempRaiseWeapon");
        m_NimbyAutorunInterruptInputs.Insert("UALeanLeft");
        m_NimbyAutorunInterruptInputs.Insert("UALeanRight");
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

    protected bool NimbyAutorunShouldInterrupt(PlayerBase player)
    {
        if (!player.NimbyIsAutorunning())
        {
            return false;
        }

        if (player.IsInVehicle() || player.IsUnconscious() || player.IsRestrained() || !player.IsAlive())
        {
            return true;
        }

        foreach (string inputName : m_NimbyAutorunInterruptInputs)
        {
            if (NimbyAutorunCheckInput(inputName))
            {
                return true;
            }
        }
        return false;
    }

    protected bool NimbyAutorunShouldStart(PlayerBase player)
    {
        if (player.NimbyIsAutorunning() || IsControlDisabled())
        {
            return false;
        }
        return NimbyAutorunCheckInput(m_NimbyAutorunInterruptInputs[0]);
    }

    // Returns MOVEMENTIDX_IDLE when the speed should not change this frame.
    protected int NimbyAutorunGetUpdatedSpeed(PlayerBase player)
    {
        if (!player.CanSprint() || player.GetStaminaHandler().GetStamina() <= 0)
        {
            return DayZPlayerConstants.MOVEMENTIDX_RUN;
        }
        if (NimbyAutorunCheckInput("UAWalkRunToggle"))
        {
            return DayZPlayerConstants.MOVEMENTIDX_WALK;
        }
        if (NimbyAutorunCheckInput("UAWalkRunTemp"))
        {
            return DayZPlayerConstants.MOVEMENTIDX_RUN;
        }
        if (NimbyAutorunCheckInput("UATurbo"))
        {
            return DayZPlayerConstants.MOVEMENTIDX_SPRINT;
        }
        return DayZPlayerConstants.MOVEMENTIDX_IDLE;
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
        {
            return;
        }

        if (NimbyAutorunShouldInterrupt(player))
        {
            NimbyAutorunSync(player, false, DayZPlayerConstants.MOVEMENTIDX_IDLE, 0);
        }
        else if (NimbyAutorunShouldStart(player))
        {
            NimbyAutorunSync(player, true, DayZPlayerConstants.MOVEMENTIDX_SPRINT, 1);
        }
        else if (player.NimbyIsAutorunning())
        {
            int speed = NimbyAutorunGetUpdatedSpeed(player);
            if (speed != DayZPlayerConstants.MOVEMENTIDX_IDLE)
            {
                NimbyAutorunSync(player, true, speed, 1);
            }
        }
    }

    protected void NimbyAutorunSync(PlayerBase player, bool isRunning, int movementIdx, int angle)
    {
        if (player == m_NimbyAutorunLastSentPlayer && isRunning == m_NimbyAutorunLastSentRunning && movementIdx == m_NimbyAutorunLastSentIdx)
        {
            return;
        }
        m_NimbyAutorunLastSentPlayer = player;
        m_NimbyAutorunLastSentRunning = isRunning;
        m_NimbyAutorunLastSentIdx = movementIdx;

        player.NimbySetAutorunning(isRunning);
        player.GetInputController().OverrideMovementSpeed(isRunning, movementIdx);
        player.GetInputController().OverrideMovementAngle(isRunning, angle);

        auto request = new Param3<bool, int, int>(isRunning, movementIdx, angle);
        GetGame().RPCSingleParam(player, NimbyAutorunRpc.TOGGLE, request, true);
    }
}
