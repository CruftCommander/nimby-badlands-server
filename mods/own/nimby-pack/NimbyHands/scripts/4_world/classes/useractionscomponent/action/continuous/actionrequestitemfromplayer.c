class NimbyHandsActionRequestItemCB : ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime( -1 );
    }
}

class NimbyHandsActionRequestItem extends ActionContinuousBase
{
    void NimbyHandsActionRequestItem()
    {
        m_CallbackClass = NimbyHandsActionRequestItemCB;
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
        return NimbyHandsOfferRequestActionInput;
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
        action_data.m_Player.NimbyHandsSetState( ENimbyHandsState.TAKE );
    }

    override void OnEndServer( ActionData action_data )
    {
        action_data.m_Player.NimbyHandsSetState( ENimbyHandsState.NONE );
    }
}