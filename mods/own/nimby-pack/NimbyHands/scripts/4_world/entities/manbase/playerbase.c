modded class PlayerBase
{
    protected ENimbyHandsState m_NimbyHandsState = ENimbyHandsState.NONE;
    // G3: off by default; a new character (respawn) starts closed because nothing is persisted.
    protected bool m_NimbyHandsPocketsOpen = false;
    // Server memory only, never persisted: when this player last toggled pockets (review item
    // 3). Starts one interval in the past so the first press of a mission is allowed.
    protected int m_NimbyHandsPocketToggleMs = -NimbyHandsConstants.POCKET_TOGGLE_MIN_MS;

    override void Init()
    {
        super.Init();

        RegisterNetSyncVariableInt( "m_NimbyHandsState", ENimbyHandsState.NONE, ENimbyHandsState.TAKE );
        RegisterNetSyncVariableBool( "m_NimbyHandsPocketsOpen" );
    }

    // Marks the player dirty only on a change: the offer and request loops call this on
    // every animation event (review item 7).
    void NimbyHandsSetState( ENimbyHandsState state )
    {
        if ( m_NimbyHandsState == state )
        {
            return;
        }

        m_NimbyHandsState = state;

        SetSynchDirty();
    }

    ENimbyHandsState NimbyHandsGetState()
    {
        return m_NimbyHandsState;
    }

    void NimbyHandsSetPocketsOpen( bool allows )
    {
        m_NimbyHandsPocketsOpen = allows;

        SetSynchDirty();
    }

    bool NimbyHandsGetPocketsOpen()
    {
        return m_NimbyHandsPocketsOpen;
    }

    // Server only. True, and records the press, when at least POCKET_TOGGLE_MIN_MS passed since
    // the last allowed toggle; false for a repeat inside the interval, which changes nothing.
    bool NimbyHandsPocketToggleAllowed( int nowMs )
    {
        if ( !NimbyHandsRules.PocketToggleAllowed( m_NimbyHandsPocketToggleMs, nowMs, NimbyHandsConstants.POCKET_TOGGLE_MIN_MS ) )
        {
            return false;
        }

        m_NimbyHandsPocketToggleMs = nowMs;
        return true;
    }

    // G4 and G3: the server closes open pockets and writes the forced-clear line. Closed
    // pockets stay closed silently, so the log carries only real changes.
    void NimbyHandsClosePockets( string reason )
    {
        if ( !GetGame().IsServer() || !m_NimbyHandsPocketsOpen )
        {
            return;
        }

        NimbyHandsSetPocketsOpen( false );
        NimbyHandsServer.LogPockets( this, false, reason );
    }

    // G4: PlayerBase.c:3756.
    override void OnUnconsciousStart()
    {
        super.OnUnconsciousStart();

        NimbyHandsClosePockets( "unconscious" );
    }

    // G4: PlayerBase.c:2206; the restrain actions call this on the server.
    override void SetRestrained( bool is_restrained )
    {
        super.SetRestrained( is_restrained );

        if ( is_restrained )
        {
            NimbyHandsClosePockets( "restrained" );
        }
    }

    // G3: missionServer.c:633-638 calls OnReconnect on the surviving character; OnConnect
    // (PlayerBase.c:7623) runs for every new session on a character.
    override void OnReconnect()
    {
        super.OnReconnect();

        NimbyHandsClosePockets( "reconnect" );
    }

    override void OnConnect()
    {
        super.OnConnect();

        NimbyHandsClosePockets( "reconnect" );
    }

    override void SetActions( out TInputActionMap InputActionMap )
    {
        super.SetActions( InputActionMap );

        AddAction( NimbyHandsActionOfferItem, InputActionMap );
        AddAction( NimbyHandsActionRequestItem, InputActionMap );
        AddAction( NimbyHandsActionTogglePockets, InputActionMap );
    }

    override void SetActionsRemoteTarget( out TInputActionMap InputActionMap )
    {
        // register new actions first to make take/give action appear before "check pulse"
        AddAction( NimbyHandsActionTakeItem, InputActionMap );
        AddAction( NimbyHandsActionGiveItem, InputActionMap );
        AddAction( NimbyHandsActionPutInPockets, InputActionMap );

        super.SetActionsRemoteTarget( InputActionMap );
    }
}
