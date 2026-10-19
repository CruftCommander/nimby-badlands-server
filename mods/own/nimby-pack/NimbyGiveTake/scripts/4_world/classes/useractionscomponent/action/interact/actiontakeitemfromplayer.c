class NimbyActionTakeItemBase extends ActionInteractBase
{
    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINone;
        m_ConditionTarget = new NimbyCCTManGiveOrTake( UAMaxDistances.DEFAULT, true, ENimbyGiveOrTake.GIVE );
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
        ItemBase tgt_entity = GetTargetEntity( target );
        if ( !tgt_entity )
        {
            return false;
        }

        // The item lands in the taker's inventory, never their hands (review CR-001).
        if ( !NimbyMoveItemToPlayer.ReceiverHasSpace( player, tgt_entity ) )
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

        NimbyGiveTakeActionData data = NimbyGiveTakeActionData.Cast( action_data );
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
            offering = tgt_player.NimbyGetGiveOrTakeState() == ENimbyGiveOrTake.GIVE;
        }

        // The taker receives; the player holding the item is the giver.
        NimbyMoveItemToPlayer move = new NimbyMoveItemToPlayer( action_data.m_Player, item, ClassName(), UAMaxDistances.DEFAULT );
        move.SetConsent( offering, "giver is no longer offering" );
        NimbyGiveTakeActionData data = NimbyGiveTakeActionData.Cast( action_data );
        if ( data )
        {
            data.ApplyTo( move );
        }
        move.Execute();
    }
}

class NimbyActionTakeItem extends NimbyActionTakeItemBase
{
    void NimbyActionTakeItem()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_PICKUP_INVENTORY;
        m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONFB_PICKUP_INVENTORY;
        m_Text = "#take";
    }
}

// NimbyActionTakeItemToHands was removed in the CR-001 rewrite: transfers never land in hands.
