class ActionGiveItemInHandsToPlayer extends ActionInteractBase
{
    void ActionGiveItemInHandsToPlayer()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_DROPITEM_HANDS;
        m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONMOD_DROPITEM_HANDS;
        m_Text = "#STR_GIVE";
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINone;
        m_ConditionTarget = new CCTManGiveOrTake( UAMaxDistances.DEFAULT, true, eGiveOrTake.TAKE );
    }

    override bool HasProneException()
    {
        return true;
    }

    override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
    {
        ItemBase src_entity = player.GetItemInHands();
        if ( !src_entity )
        {
            return false;
        }

        if ( !src_entity.CanBeGiven() )
        {
            return false;
        }

        return true;
    }

    override void OnExecuteServer( ActionData action_data )
    {
        PlayerBase tgt_player = PlayerBase.Cast( action_data.m_Target.GetObject() );

        // setup custom move item "lambda" and execute it
        MoveItemToPlayer move = new MoveItemToPlayer( tgt_player, action_data.m_Player.GetItemInHands(), FindInventoryLocationType.HANDS );
        if ( move.Execute() )
        {
#ifdef DIAG
            // debug purpose
            tgt_player.SetGiveOrTakeState( eGiveOrTake.GIVE );
#endif
        }
    }
}