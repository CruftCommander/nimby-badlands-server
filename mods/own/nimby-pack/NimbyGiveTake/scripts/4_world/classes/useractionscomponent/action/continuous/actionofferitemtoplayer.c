class ActionOfferItemToPlayerCB : ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime( -1 );
    }
}

class ActionOfferItemToPlayer extends ActionContinuousBase
{
    void ActionOfferItemToPlayer()
    {
        m_CallbackClass = ActionOfferItemToPlayerCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_GIVER;
        m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONFB_GIVER;
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCIPresent;
        m_ConditionTarget = new CCTNone;
    }

    override typename GetInputType()
    {
        return ContinuousOfferOrRequestItemActionInput;
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

    override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
    {
        if ( player.GetItemInHands() != item )
        {
            return false;
        }

        if( !item.CanBeOffered() )
        {
            return false;
        }

        if ( !player.GetHumanInventory().CanRemoveEntityInHands() )
        {
            return false;
        }

        return true;
    }

    override void OnExecuteServer( ActionData action_data )
    {
        action_data.m_Player.SetGiveOrTakeState( eGiveOrTake.GIVE );
    }

    override void OnEndServer( ActionData action_data )
    {
        action_data.m_Player.SetGiveOrTakeState( eGiveOrTake.NONE );
    }
}