class NimbyHandsActionGiveItem extends ActionInteractBase
{
    void NimbyHandsActionGiveItem()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_DROPITEM_HANDS;
        m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONMOD_DROPITEM_HANDS;
        m_Text = "#STR_NIMBY_HANDS_GIVE";
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINone;
        m_ConditionTarget = new NimbyHandsCCTManState( UAMaxDistances.DEFAULT, true, ENimbyHandsState.TAKE );
    }

    override ActionData CreateActionData()
    {
        return new NimbyHandsActionData;
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

        if ( !src_entity.NimbyHandsCanBeGiven() )
        {
            return false;
        }

        // The item lands in the receiver's cargo, never their hands (review CR-001) and never a
        // worn slot (Jeff, October 21, 2026).
        PlayerBase tgt_player = PlayerBase.Cast( target.GetObject() );
        if ( !NimbyHandsMoveItemToPlayer.ReceiverHasSpace( tgt_player, src_entity ) )
        {
            return false;
        }

        // P3 step 1 stages the item in the giver's own cargo (cost path A).
        if ( !NimbyHandsMoveItemToPlayer.GiverHasSpace( player, src_entity ) )
        {
            return false;
        }

        return true;
    }

    // Records the item in hands and both players for the dispatch-time staleness checks.
    override void OnStartServer( ActionData action_data )
    {
        super.OnStartServer( action_data );

        NimbyHandsActionData data = NimbyHandsActionData.Cast( action_data );
        PlayerBase tgt_player = PlayerBase.Cast( action_data.m_Target.GetObject() );
        if ( data && tgt_player )
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
            requesting = tgt_player.NimbyHandsGetState() == ENimbyHandsState.TAKE;
        }

        // The holder gives; the holder is the requester of this transfer.
        NimbyHandsMoveItemToPlayer move = new NimbyHandsMoveItemToPlayer( action_data.m_Player, tgt_player, action_data.m_Player.GetItemInHands(), ClassName(), UAMaxDistances.DEFAULT );
        move.SetConsent( requesting, "receiver is no longer requesting" );
        NimbyHandsActionData data = NimbyHandsActionData.Cast( action_data );
        if ( data )
        {
            data.ApplyTo( move );
        }
        move.Execute();
    }
}
