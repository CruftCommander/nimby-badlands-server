modded class PlayerBase
{
    protected eGiveOrTake m_GiveOrTakeState = eGiveOrTake.NONE;
    protected bool m_AllowsPutInPockets = false;

    override void Init()
    {
        super.Init();

        RegisterNetSyncVariableInt( "m_GiveOrTakeState", eGiveOrTake.NONE, eGiveOrTake.TAKE );
        RegisterNetSyncVariableBool( "m_AllowsPutInPockets" );
    }

    void SetGiveOrTakeState( eGiveOrTake state )
    {
        m_GiveOrTakeState = state;

        SetSynchDirty();
    }

    eGiveOrTake GetGiveOrTakeState()
    {
        return m_GiveOrTakeState;
    }

    void SetAllowsPutInPockets( bool allows )
    {
        m_AllowsPutInPockets = allows;

        SetSynchDirty();
    }

    bool GetAllowsPutInPockets()
    {
        return m_AllowsPutInPockets;
    }

    override void SetActions( out TInputActionMap InputActionMap )
    {
        super.SetActions( InputActionMap );

        AddAction( ActionOfferItemToPlayer, InputActionMap );
        AddAction( ActionRequestItemFromPlayer, InputActionMap );
        AddAction( ActionToggleAllowPutInPockets, InputActionMap );
    }

    override void SetActionsRemoteTarget( out TInputActionMap InputActionMap )
    {
        // register new actions first to make take/give action appear before "check pulse"
        AddAction( ActionTakeItemFromPlayer, InputActionMap );
        AddAction( ActionTakeItemFromPlayerToHands, InputActionMap );
        AddAction( ActionGiveItemInHandsToPlayer, InputActionMap );
        AddAction( ActionPutItemInHandsInPlayersPockets, InputActionMap );

        super.SetActionsRemoteTarget( InputActionMap );
    }
}