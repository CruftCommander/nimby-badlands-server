class NimbyHandsActionTakeItemBase extends ActionInteractBase
{
    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINone;
        m_ConditionTarget = new NimbyHandsCCTManState( UAMaxDistances.DEFAULT, true, ENimbyHandsState.GIVE );
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
        ItemBase tgt_entity = GetTargetEntity( target );
        if ( !tgt_entity )
        {
            return false;
        }

        // The item lands in the taker's inventory, never their hands (review CR-001).
        if ( !NimbyHandsMoveItemToPlayer.ReceiverHasSpace( player, tgt_entity ) )
        {
            return false;
        }

        return true;
    }

    ItemBase GetTargetEntity( ActionTarget target )
    {
        PlayerBase player = PlayerBase.Cast( target.GetObject() );
        if ( !player )
        {
            return null;
        }

        return player.GetItemInHands();
    }

    override bool InventoryReservation( ActionData action_data )
    {
        // do not reserve any inventory throughout the action
        return true;
    }

    // Records the offered item and both players for the dispatch-time staleness checks.
    override void OnStartServer( ActionData action_data )
    {
        super.OnStartServer( action_data );

        NimbyHandsActionData data = NimbyHandsActionData.Cast( action_data );
        PlayerBase tgt_player = PlayerBase.Cast( action_data.m_Target.GetObject() );
        if ( data && tgt_player )
        {
            data.Capture( tgt_player.GetItemInHands(), tgt_player, action_data.m_Player );
        }
    }

    override void OnExecuteServer( ActionData action_data )
    {
        PlayerBase tgt_player = PlayerBase.Cast( action_data.m_Target.GetObject() );
        ItemBase item = null;
        bool offering = false;
        if ( tgt_player )
        {
            item = tgt_player.GetItemInHands();
            offering = tgt_player.NimbyHandsGetState() == ENimbyHandsState.GIVE;
        }

        // The taker receives; the player holding the item is the giver.
        NimbyHandsMoveItemToPlayer move = new NimbyHandsMoveItemToPlayer( action_data.m_Player, item, ClassName(), UAMaxDistances.DEFAULT );
        move.SetConsent( offering, "giver is no longer offering" );
        NimbyHandsActionData data = NimbyHandsActionData.Cast( action_data );
        if ( data )
        {
            data.ApplyTo( move );
        }
        move.Execute();
    }
}

class NimbyHandsActionTakeItem extends NimbyHandsActionTakeItemBase
{
    void NimbyHandsActionTakeItem()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_PICKUP_INVENTORY;
        m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONFB_PICKUP_INVENTORY;
        m_Text = "#take";
    }
}

// NimbyHandsActionTakeItemToHands was removed in the CR-001 rewrite: transfers never land in hands.
