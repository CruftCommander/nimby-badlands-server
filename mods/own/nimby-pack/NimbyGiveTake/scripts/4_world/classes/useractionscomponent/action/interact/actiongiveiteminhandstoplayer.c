class NimbyActionGiveItemInHands extends ActionInteractBase
{
    void NimbyActionGiveItemInHands()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_DROPITEM_HANDS;
        m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONMOD_DROPITEM_HANDS;
        m_Text = "#STR_NIMBY_GT_GIVE";
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINone;
        m_ConditionTarget = new NimbyCCTManGiveOrTake( UAMaxDistances.DEFAULT, true, ENimbyGiveOrTake.TAKE );
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

        if ( !src_entity.NimbyCanBeGiven() )
        {
            return false;
        }

        // The item lands in the receiver's inventory, never their hands (review CR-001).
        PlayerBase tgt_player = PlayerBase.Cast( target.GetObject() );
        if ( !NimbyMoveItemToPlayer.ReceiverHasSpace( tgt_player, src_entity ) )
        {
            return false;
        }

        return true;
    }

    override void OnExecuteServer( ActionData action_data )
    {
        PlayerBase tgt_player = PlayerBase.Cast( action_data.m_Target.GetObject() );

        // setup custom move item "lambda" and execute it
        NimbyMoveItemToPlayer move = new NimbyMoveItemToPlayer( tgt_player, action_data.m_Player.GetItemInHands(), ClassName() );
        if ( move.Execute() )
        {
#ifdef DIAG
            // debug purpose
            tgt_player.NimbySetGiveOrTakeState( ENimbyGiveOrTake.GIVE );
#endif
        }
    }
}