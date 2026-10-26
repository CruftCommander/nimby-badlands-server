// -----------------------------------------------------------------------------
// NimbyTack - the server's player-facing settings on the player (T27).
// Purpose : Carries three values of the server config to each player's game as
//           net-synced variables: refusalNotices (the boxes the client decides on its
//           own obey the switch; nimbytack_actions.c ClientNotice), and areaCapCount
//           and areaCapRadiusMeters (the client's cap pre-check counts with the
//           server's numbers, not the built-in 20 within 50 m; otherwise a raised cap
//           would withdraw the prompt at 20 and a lowered one would show it past the
//           limit). Nothing is persisted (shared rules 6: net-synced variables are
//           allowed and are not saved).
// Vanilla : PlayerBase.Init registers the net-synced variables (PlayerBase.c:364,
//           RegisterNetSyncVariableBool at PlayerBase.c:582); OnConnect runs on the
//           server when the player's connection is established (PlayerBase.c:7623).
//           The initial value travels with the entity; SetSynchDirty pushes a change.
// -----------------------------------------------------------------------------
modded class PlayerBase
{
    protected bool m_NimbyTackNotices = true;
    protected int m_NimbyTackCapCount = 20;
    protected float m_NimbyTackCapRadius = 50.0;

    override void Init()
    {
        super.Init();
        RegisterNetSyncVariableBool("m_NimbyTackNotices");
        RegisterNetSyncVariableInt("m_NimbyTackCapCount", 0, 100000);
        RegisterNetSyncVariableFloat("m_NimbyTackCapRadius", 0.0, 500.0, 2);
        NimbyTackReadServerConfig(false);
    }

    override void OnConnect()
    {
        super.OnConnect();
        NimbyTackReadServerConfig(true);
    }

    // Server: copy the player-facing values from the loaded config; push them when they changed
    // after the entity was first sent.
    protected void NimbyTackReadServerConfig(bool push)
    {
        if (!GetGame().IsServer())
        {
            return;
        }
        NimbyTackConfig config = NimbyTackConfigLoader.Get();
        bool notices = config.refusalNotices != 0;
        int capCount = config.areaCapCount;
        float capRadius = config.areaCapRadiusMeters;
        bool changed = notices != m_NimbyTackNotices || capCount != m_NimbyTackCapCount || capRadius != m_NimbyTackCapRadius;
        m_NimbyTackNotices = notices;
        m_NimbyTackCapCount = capCount;
        m_NimbyTackCapRadius = capRadius;
        if (push && changed)
        {
            SetSynchDirty();
        }
    }

    // Whether this player's game may show Tack's refusal boxes (T26, T27).
    bool NimbyTackNoticesOn()
    {
        return m_NimbyTackNotices;
    }

    // The server's area cap, for the client's pre-check (T26, T27).
    int NimbyTackCapCount()
    {
        return m_NimbyTackCapCount;
    }

    float NimbyTackCapRadius()
    {
        return m_NimbyTackCapRadius;
    }
}
