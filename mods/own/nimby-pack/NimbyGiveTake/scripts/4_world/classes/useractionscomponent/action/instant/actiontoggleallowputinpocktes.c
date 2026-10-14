class ActionToggleAllowPutInPockets extends ActionInstantBase
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

    override bool CanBeUsedInRestrain()
    {
        return true;
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
        return ToggleAllowPutInPocketsActionInput;
    }

    override void OnStartServer( ActionData action_data )
    {
        PlayerBase player = action_data.m_Player;

        player.SetAllowsPutInPockets( !player.GetAllowsPutInPockets() );
    }
}