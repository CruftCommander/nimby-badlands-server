class NimbyActionOfferItemCB : ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime( -1 );
    }
}

class NimbyActionOfferItem extends ActionContinuousBase
{
    void NimbyActionOfferItem()
    {
        m_CallbackClass = NimbyActionOfferItemCB;
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
        return NimbyOfferOrRequestActionInput;
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

        if( !item.NimbyCanBeOffered() )
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
        action_data.m_Player.NimbySetGiveOrTakeState( ENimbyGiveOrTake.GIVE );
    }

    override void OnEndServer( ActionData action_data )
    {
        action_data.m_Player.NimbySetGiveOrTakeState( ENimbyGiveOrTake.NONE );
    }
}