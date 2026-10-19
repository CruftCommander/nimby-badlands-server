modded class PlayerBase
{
    protected ENimbyGiveOrTake m_NimbyGiveOrTakeState = ENimbyGiveOrTake.NONE;
    // G3: off by default; a new character (respawn) starts closed because nothing is persisted.
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

    // G4 and G3: the server closes open pockets and writes the forced-clear line. Closed
    // pockets stay closed silently, so the log carries only real changes.
    void NimbyClosePockets( string reason )
    {
        if ( !GetGame().IsServer() || !m_NimbyAllowsPocketAccess )
        {
            return;
        }

        NimbySetAllowsPocketAccess( false );
        NimbyGiveTakeServer.LogPockets( this, false, reason );
    }

    // G4: PlayerBase.c:3756.
    override void OnUnconsciousStart()
    {
        super.OnUnconsciousStart();

        NimbyClosePockets( "unconscious" );
    }

    // G4: PlayerBase.c:2206; the restrain actions call this on the server.
    override void SetRestrained( bool is_restrained )
    {
        super.SetRestrained( is_restrained );

        if ( is_restrained )
        {
            NimbyClosePockets( "restrained" );
        }
    }

    // G3: missionServer.c:633-638 calls OnReconnect on the surviving character; OnConnect
    // (PlayerBase.c:7623) runs for every new session on a character.
    override void OnReconnect()
    {
        super.OnReconnect();

        NimbyClosePockets( "reconnect" );
    }

    override void OnConnect()
    {
        super.OnConnect();

        NimbyClosePockets( "reconnect" );
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
}
