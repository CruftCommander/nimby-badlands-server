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

    override ActionData CreateActionData()
    {
        return new NimbyGiveTakeActionData;
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

    // Records the item in hands and both players for the dispatch-time staleness checks.
    override void OnStartServer( ActionData action_data )
    {
        super.OnStartServer( action_data );

        NimbyGiveTakeActionData data = NimbyGiveTakeActionData.Cast( action_data );
        PlayerBase tgt_player = PlayerBase.Cast( action_data.m_Target.GetObject() );
        if ( data )
        {
            data.Capture( action_data.m_Player.GetItemInHands(), action_data.m_Player, tgt_player );
        }
    }

    override void OnExecuteServer( ActionData action_data )
    {
        PlayerBase tgt_player = PlayerBase.Cast( action_data.m_Target.GetObject() );
        bool requesting = false;
        if ( tgt_player )
        {
            requesting = tgt_player.NimbyGetGiveOrTakeState() == ENimbyGiveOrTake.TAKE;
        }

        NimbyMoveItemToPlayer move = new NimbyMoveItemToPlayer( tgt_player, action_data.m_Player.GetItemInHands(), ClassName(), UAMaxDistances.DEFAULT );
        move.SetConsent( requesting, "receiver is no longer requesting" );
        NimbyGiveTakeActionData data = NimbyGiveTakeActionData.Cast( action_data );
        if ( data )
        {
            data.ApplyTo( move );
        }
        move.Execute();
    }
}
