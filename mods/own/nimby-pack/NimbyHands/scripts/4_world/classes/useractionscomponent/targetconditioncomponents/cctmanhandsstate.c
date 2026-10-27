// CCTMan with the target's Hands state. Enforce has no base-constructor call, so CCTMan's
// constructor runs with its own defaults; the two inherited fields (CCTMan.c:3-4, protected,
// set at CCTMan.c:8-9) are set here so the arguments reach them (review item 2).
class NimbyHandsCCTManState extends CCTMan
{
    protected ENimbyHandsState m_State;

    void NimbyHandsCCTManState( float maximal_target_distance = UAMaxDistances.DEFAULT, bool must_be_alive = true, ENimbyHandsState state = -1 )
    {
        m_MaximalActionDistanceSq = maximal_target_distance * maximal_target_distance;
        m_MustBeAlive = must_be_alive;
        m_State = state;
    }

    override bool Can( PlayerBase player, ActionTarget target )
    {
        if ( !super.Can( player, target ) )
        {
            return false;
        }

        PlayerBase tgt_player = PlayerBase.Cast( target.GetObject() );
        if ( !tgt_player )
        {
            return false;
        }

        return tgt_player.NimbyHandsGetState() == m_State && tgt_player.IsFacingTarget( player );
    }
}
