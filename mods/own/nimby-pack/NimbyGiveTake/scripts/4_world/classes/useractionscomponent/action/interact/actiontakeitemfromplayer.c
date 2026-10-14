class ActionTakeItemFromPlayerBase extends ActionInteractBase
{
    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINone;
        m_ConditionTarget = new CCTManGiveOrTake( UAMaxDistances.DEFAULT, true, eGiveOrTake.GIVE );
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

        InventoryLocation il = new InventoryLocation;
        if ( !player.GetHumanInventory().FindFreeLocationFor( tgt_entity, GetTargetLocations(), il ) )
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

    FindInventoryLocationType GetTargetLocations()
    {
        return FindInventoryLocationType.ANY;
    }

    override void OnExecuteServer( ActionData action_data )
    {
        PlayerBase tgt_player = PlayerBase.Cast( action_data.m_Target.GetObject() );

        // setup custom move item "lambda" and execute it
        MoveItemToPlayer move = new MoveItemToPlayer( action_data.m_Player, tgt_player.GetItemInHands(), GetTargetLocations() );
        if ( move.Execute() )
        {
#ifdef DIAG
            // debug purpose
            tgt_player.SetGiveOrTakeState( eGiveOrTake.TAKE );
#endif
        }
    }
}

class ActionTakeItemFromPlayer extends ActionTakeItemFromPlayerBase
{
    void ActionTakeItemFromPlayer()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_PICKUP_INVENTORY;
        m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONFB_PICKUP_INVENTORY;
        m_Text = "#take";
    }

    override FindInventoryLocationType GetTargetLocations()
    {
        return FindInventoryLocationType.ANY_CARGO | FindInventoryLocationType.ATTACHMENT;
    }
}

class ActionTakeItemFromPlayerToHands extends ActionTakeItemFromPlayerBase
{
    void ActionTakeItemFromPlayerToHands()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_PICKUP_HANDS;
        m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONMOD_PICKUP_HANDS;
        m_Text = "#take_to_hands";
    }

    override FindInventoryLocationType GetTargetLocations()
    {
        return FindInventoryLocationType.HANDS;
    }

    override typename GetInputType()
    {
        return ContinuousInteractActionInput;
    }
}