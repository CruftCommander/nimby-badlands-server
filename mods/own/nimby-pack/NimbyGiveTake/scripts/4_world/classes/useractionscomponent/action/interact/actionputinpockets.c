class NimbyActionPutInPocketsCB extends ActionContinuousBaseCB
{
    static const float TIME = 2.0;

    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime( TIME );
    }
}

class NimbyActionPutInPockets extends ActionContinuousBase
{
    void NimbyActionPutInPockets()
    {
        m_CallbackClass = NimbyActionPutInPocketsCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
        m_FullBody = true;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT;
        m_Text = "#STR_NIMBY_GT_PUT_IN_POCKETS";
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCIPresent;
        m_ConditionTarget = new CCTMan( UAMaxDistances.SMALL );
    }

    override typename GetInputType()
    {
        return ContinuousInteractActionInput;
    }

    override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
    {
        PlayerBase tgt_player = PlayerBase.Cast( target.GetObject() );
        if ( !tgt_player.NimbyGetAllowsPocketAccess() )
        {
            return false;
        }

        if ( tgt_player.IsSwimming() )
        {
            return false;
        }

        if ( tgt_player.IsInVehicle() )
        {
            return false;
        }

        if ( !item.NimbyCanBeGiven() )
        {
            return false;
        }

        return true;
    }

    override void OnFinishProgressServer( ActionData action_data )
    {
        PlayerBase tgt_player = PlayerBase.Cast( action_data.m_Target.GetObject() );

        // setup custom move item "lambda" and execute it
        NimbyMoveItemToPlayer move = new NimbyMoveItemToPlayer( tgt_player, action_data.m_Player.GetItemInHands(), FindInventoryLocationType.ANY_CARGO | FindInventoryLocationType.ATTACHMENT, ClassName() );
        move.Execute();
    }
}