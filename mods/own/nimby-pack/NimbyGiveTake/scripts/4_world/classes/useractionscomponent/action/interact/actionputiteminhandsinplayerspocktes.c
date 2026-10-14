class ActionPutItemInHandsInPlayersPocketsCB extends ActionContinuousBaseCB
{
    static const float TIME = 2.0;

    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime( TIME );
    }
}

class ActionPutItemInHandsInPlayersPockets extends ActionContinuousBase
{
    void ActionPutItemInHandsInPlayersPockets()
    {
        m_CallbackClass = ActionPutItemInHandsInPlayersPocketsCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
        m_FullBody = true;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT;
        m_Text = "#STR_PUT_IN_POCKETS";
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
        if ( !tgt_player.GetAllowsPutInPockets() )
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

        if ( !item.CanBeGiven() )
        {
            return false;
        }

        return true;
    }

    override void OnFinishProgressServer( ActionData action_data )
    {
        PlayerBase tgt_player = PlayerBase.Cast( action_data.m_Target.GetObject() );

        // setup custom move item "lambda" and execute it
        MoveItemToPlayer move = new MoveItemToPlayer( tgt_player, action_data.m_Player.GetItemInHands(), FindInventoryLocationType.ANY_CARGO | FindInventoryLocationType.ATTACHMENT );
        move.Execute();
    }
}