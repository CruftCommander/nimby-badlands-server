modded class PlayerBase
{
    protected ENimbyGiveOrTake m_NimbyGiveOrTakeState = ENimbyGiveOrTake.NONE;
    protected bool m_NimbyAllowsPocketAccess = false;

    override void Init()
    {
        super.Init();

        RegisterNetSyncVariableInt( "m_NimbyGiveOrTakeState", ENimbyGiveOrTake.NONE, ENimbyGiveOrTake.TAKE );
        RegisterNetSyncVariableBool( "m_NimbyAllowsPocketAccess" );
    }

    void NimbySetGiveOrTakeState( ENimbyGiveOrTake state )
    {
        m_NimbyGiveOrTakeState = state;

        SetSynchDirty();
    }

    ENimbyGiveOrTake NimbyGetGiveOrTakeState()
    {
        return m_NimbyGiveOrTakeState;
    }

    void NimbySetAllowsPocketAccess( bool allows )
    {
        m_NimbyAllowsPocketAccess = allows;

        SetSynchDirty();
    }

    bool NimbyGetAllowsPocketAccess()
    {
        return m_NimbyAllowsPocketAccess;
    }

    override void SetActions( out TInputActionMap InputActionMap )
    {
        super.SetActions( InputActionMap );

        AddAction( NimbyActionOfferItem, InputActionMap );
        AddAction( NimbyActionRequestItem, InputActionMap );
        AddAction( NimbyActionTogglePocketAccess, InputActionMap );
    }

    override void SetActionsRemoteTarget( out TInputActionMap InputActionMap )
    {
        // register new actions first to make take/give action appear before "check pulse"
        AddAction( NimbyActionTakeItem, InputActionMap );
        AddAction( NimbyActionGiveItemInHands, InputActionMap );
        AddAction( NimbyActionPutInPockets, InputActionMap );

        super.SetActionsRemoteTarget( InputActionMap );
    }

    // Called on the receiver VERIFY_DELAY_MS after a transfer request (NimbyMoveItemToPlayer).
    // Logs where the item actually ended up, because the hand event is processed later.
    void NimbyVerifyTransfer( string action, Man giver, EntityAI item )
    {
        if ( !item )
        {
            NimbyGiveTakeLog.Transfer( action, "FAILED", giver, this, item, "item no longer exists" );
            return;
        }

        Man holder = item.GetHierarchyRootPlayer();
        if ( holder == this )
        {
            NimbyGiveTakeLog.Transfer( action, "OK", giver, this, item, "item is in receiver inventory" );
        }
        else if ( holder == giver )
        {
            NimbyGiveTakeLog.Transfer( action, "FAILED", giver, this, item, "item still with giver; the giver's inventory refused the move" );
        }
        else
        {
            NimbyGiveTakeLog.Transfer( action, "FAILED", giver, this, item, "item ended up elsewhere" );
        }
    }
}
