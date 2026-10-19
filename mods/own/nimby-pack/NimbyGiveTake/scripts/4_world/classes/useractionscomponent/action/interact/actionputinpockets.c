class NimbyActionPutInPocketsCB extends ActionContinuousBaseCB
{
    static const float TIME = 2.0;

    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime( TIME );
    }
}

class NimbyActionPutInPockets extends ActionContinuousBase
{
    void NimbyActionPutInPockets()
    {
        m_CallbackClass = NimbyActionPutInPocketsCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
        m_FullBody = true;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT;
        m_Text = "#STR_NIMBY_GT_PUT_IN_POCKETS";
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCIPresent;
        m_ConditionTarget = new CCTMan( UAMaxDistances.SMALL );
    }

    override ActionData CreateActionData()
    {
        return new NimbyGiveTakeActionData;
    }

    override typename GetInputType()
    {
        return ContinuousInteractActionInput;
    }

    // Evaluated on the client for the prompt, on the server at start, and on every progress
    // tick through CanContinue (AnimatedActionBase.c:419), so closing pockets mid-action
    // interrupts it.
    override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
    {
        PlayerBase tgt_player = PlayerBase.Cast( target.GetObject() );
        if ( !tgt_player )
        {
            return false;
        }

        if ( !tgt_player.NimbyGetAllowsPocketAccess() )
        {
            return false;
        }

        // G4: no deposit into an unconscious or restrained player.
        if ( tgt_player.IsUnconscious() || tgt_player.IsRestrained() )
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

        if ( !item.NimbyCanBeGiven() )
        {
            return false;
        }

        return true;
    }

    // Records the item and both players for the dispatch-time staleness checks.
    override void OnStartServer( ActionData action_data )
    {
        super.OnStartServer( action_data );

        NimbyGiveTakeActionData data = NimbyGiveTakeActionData.Cast( action_data );
        PlayerBase tgt_player = PlayerBase.Cast( action_data.m_Target.GetObject() );
        if ( data )
        {
            data.Capture( action_data.m_MainItem, action_data.m_Player, tgt_player );
        }
    }

    override void OnFinishProgressServer( ActionData action_data )
    {
        PlayerBase tgt_player = PlayerBase.Cast( action_data.m_Target.GetObject() );
        bool open = false;
        if ( tgt_player )
        {
            open = tgt_player.NimbyGetAllowsPocketAccess();
        }

        NimbyMoveItemToPlayer move = new NimbyMoveItemToPlayer( tgt_player, action_data.m_Player.GetItemInHands(), ClassName(), UAMaxDistances.SMALL );
        move.SetConsent( open, "receiver's pockets are closed" );
        NimbyGiveTakeActionData data = NimbyGiveTakeActionData.Cast( action_data );
        if ( data )
        {
            data.ApplyTo( move );
        }
        move.Execute();
    }
}
