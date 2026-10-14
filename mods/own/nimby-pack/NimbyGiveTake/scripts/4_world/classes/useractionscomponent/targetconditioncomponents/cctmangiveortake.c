class NimbyCCTManGiveOrTake extends CCTMan
{
    protected ENimbyGiveOrTake m_State;

    void NimbyCCTManGiveOrTake( float maximal_target_distance = UAMaxDistances.DEFAULT, bool must_be_alive = true, ENimbyGiveOrTake state = -1 )
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

        return tgt_player.NimbyGetGiveOrTakeState() == m_State && tgt_player.IsFacingTarget( player );
    }
}