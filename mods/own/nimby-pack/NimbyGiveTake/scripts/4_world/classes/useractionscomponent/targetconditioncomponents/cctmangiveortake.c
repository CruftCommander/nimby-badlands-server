class CCTManGiveOrTake extends CCTMan
{
    protected eGiveOrTake m_State;

    void CCTManGiveOrTake( float maximal_target_distance = UAMaxDistances.DEFAULT, bool must_be_alive = true, eGiveOrTake state = -1 )
    {
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

        return tgt_player.GetGiveOrTakeState() == m_State && tgt_player.IsFacingTarget( player );
    }
}