class NimbyHandsActionPutInPocketsCB extends ActionContinuousBaseCB
{
    static const float TIME = 2.0;

    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime( TIME );
    }
}

class NimbyHandsActionPutInPockets extends ActionContinuousBase
{
    void NimbyHandsActionPutInPockets()
    {
        m_CallbackClass = NimbyHandsActionPutInPocketsCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
        m_FullBody = true;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT;
        m_Text = "#STR_NIMBY_HANDS_PUT_IN_POCKETS";
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCIPresent;
        m_ConditionTarget = new CCTMan( UAMaxDistances.SMALL );
    }

    override ActionData CreateActionData()
    {
        return new NimbyHandsActionData;
    }

    override typename GetInputType()
    {
        return ContinuousInteractActionInput;
    }

    // Evaluated on the client for the prompt, on the server at start, and on every progress
    // tick through CanContinue (AnimatedActionBase.c:419), so closing pockets mid-action
    // interrupts it, and so does a receiver who fills up during the animation.
    override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
    {
        PlayerBase tgt_player = PlayerBase.Cast( target.GetObject() );
        if ( !tgt_player )
        {
            return false;
        }

        if ( !tgt_player.NimbyHandsGetPocketsOpen() )
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

        if ( !item.NimbyHandsCanBeGiven() )
        {
            return false;
        }

        // G3: pockets open permits deposits into cargo only (review items 1 and 4).
        if ( !NimbyHandsMoveItemToPlayer.ReceiverHasSpace( tgt_player, item ) )
        {
            return false;
        }

        // P3 step 1 stages the item in the depositor's own cargo (cost path A).
        if ( !NimbyHandsMoveItemToPlayer.GiverHasSpace( player, item ) )
        {
            return false;
        }

        return true;
    }

    // Records the item and both players for the dispatch-time staleness checks.
    override void OnStartServer( ActionData action_data )
    {
        super.OnStartServer( action_data );

        NimbyHandsActionData data = NimbyHandsActionData.Cast( action_data );
        PlayerBase tgt_player = PlayerBase.Cast( action_data.m_Target.GetObject() );
        if ( data && tgt_player )
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
            open = tgt_player.NimbyHandsGetPocketsOpen();
        }

        // The depositor gives; the depositor is the requester of this transfer.
        NimbyHandsMoveItemToPlayer move = new NimbyHandsMoveItemToPlayer( action_data.m_Player, tgt_player, action_data.m_Player.GetItemInHands(), ClassName(), UAMaxDistances.SMALL );
        move.SetConsent( open, "receiver's pockets are closed" );
        NimbyHandsActionData data = NimbyHandsActionData.Cast( action_data );
        if ( data )
        {
            data.ApplyTo( move );
        }
        move.Execute();
    }
}
