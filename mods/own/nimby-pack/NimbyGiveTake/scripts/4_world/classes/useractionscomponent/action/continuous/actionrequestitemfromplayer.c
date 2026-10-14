class NimbyActionRequestItemCB : ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime( -1 );
    }
}

class NimbyActionRequestItem extends ActionContinuousBase
{
    void NimbyActionRequestItem()
    {
        m_CallbackClass = NimbyActionRequestItemCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_GIVER;
        m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONFB_GIVER;
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINotPresent;
        m_ConditionTarget = new CCTNone;
    }

    override typename GetInputType()
    {
        return NimbyOfferOrRequestActionInput;
    }

    override bool InventoryReservation( ActionData action_data )
    {
        return true;
    }

    override bool UseMainItem()
    {
        return false;
    }

    override bool HasTarget()
    {
        return false;
    }

    override bool HasProgress()
    {
        return false;
    }

    override bool HasProneException()
    {
        return true;
    }

    override void OnExecuteServer( ActionData action_data )
    {
        action_data.m_Player.NimbySetGiveOrTakeState( ENimbyGiveOrTake.TAKE );
    }

    override void OnEndServer( ActionData action_data )
    {
        action_data.m_Player.NimbySetGiveOrTakeState( ENimbyGiveOrTake.NONE );
    }
}