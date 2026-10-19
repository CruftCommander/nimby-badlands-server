class NimbyActionTogglePocketAccess extends ActionInstantBase
{
    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINone;
        m_ConditionTarget = new CCTNone;
    }

    override bool UseMainItem()
    {
        return false;
    }

    // G4: a restrained player cannot open (or close) their pockets; restraint closes them.
    override bool CanBeUsedInRestrain()
    {
        return false;
    }

    override bool CanBeUsedInVehicle()
    {
        return true;
    }

    override bool CanBeUsedOnBack()
    {
        return true;
    }

    override bool CanBeUsedSwimming()
    {
        return true;
    }

    override bool CanBeUsedOnLadder()
    {
        return true;
    }

    override bool CanBeUsedRaised()
    {
        return true;
    }

    override bool CanBeUsedThrowing()
    {
        return true;
    }

    override typename GetInputType()
    {
        return NimbyTogglePocketAccessActionInput;
    }

    override void OnStartServer( ActionData action_data )
    {
        PlayerBase player = action_data.m_Player;
        bool open = !player.NimbyGetAllowsPocketAccess();

        player.NimbySetAllowsPocketAccess( open );
        NimbyGiveTakeServer.LogPockets( player, open, "toggle" );
    }
}
