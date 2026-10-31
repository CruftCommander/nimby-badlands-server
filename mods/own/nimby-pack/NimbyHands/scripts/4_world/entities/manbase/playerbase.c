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

    // NIMBY-T-063 probe, October 30, 2026 (build 10): a waiting Hands request whose giver is
    // this player is polled and dispatched here, inside the giver's own CommandHandler, where
    // vanilla runs its server inventory work: DayZPlayerImplement.CommandHandler calls
    // HandleInventory and HumanInventory.Update (DayZPlayerImplement.c:2294-2295) and then this
    // hook (:2297). Nine server crashes on 1.30.164200 followed a step 1 hand event dispatched
    // from the global call queue; this build moves only the dispatch site. Server only; a
    // player with no waiting request costs one static call with an early return.
    override bool ModCommandHandlerBefore( float pDt, int pCurrentCommandID, bool pCurrentCommandFinished )
    {
        if ( super.ModCommandHandlerBefore( pDt, pCurrentCommandID, pCurrentCommandFinished ) )
        {
            return true;
        }

        if ( GetGame().IsServer() )
        {
            NimbyHandsServer.OnCommandHandler( this );
        }

        return false;
    }

    // Client-driven step 1 (NIMBY-T-063, October 31, 2026). STAGE arrives on the giver's client
    // (sent to its identity only); STAGE_REFUSED arrives on the server from that client. Ids in
    // NimbyHandsRpc; the same shape as NimbyGreet and NimbyAutorun.
    override void OnRPC( PlayerIdentity sender, int rpc_type, ParamsReadContext ctx )
    {
        if ( rpc_type == NimbyHandsRpc.STAGE )
        {
            if ( GetGame().IsClient() )
            {
                NimbyHandsClient.OnStageRequest( this, ctx );
            }
            return;
        }
        if ( rpc_type == NimbyHandsRpc.STAGE_REFUSED )
        {
            if ( GetGame().IsServer() )
            {
                NimbyHandsServer.OnClientStageRefused( sender, ctx );
            }
            return;
        }
        super.OnRPC( sender, rpc_type, ctx );
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
