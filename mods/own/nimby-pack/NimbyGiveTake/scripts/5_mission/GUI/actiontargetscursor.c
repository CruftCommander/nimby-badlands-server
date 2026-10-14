modded class ActionTargetsCursor
{
    static const float NIMBY_GIVE_TAKE_MAX_DISTANCE_SQ = UAMaxDistances.DEFAULT * UAMaxDistances.DEFAULT;

    protected bool NimbyCanShowGiveInformation( out ItemBase item )
    {
        PlayerBase tgt_player;
        if ( !PlayerBase.CastTo( tgt_player, m_Target.GetObject() ) )
        {
            return false;
        }

        if ( tgt_player.NimbyGetGiveOrTakeState() != ENimbyGiveOrTake.GIVE )
        {
            return false;
        }

        if ( !tgt_player.IsFacingTarget( GetGame().GetPlayer() ) )
        {
            return false;
        }

        if ( !PlayerBase.Cast( GetGame().GetPlayer() ).IsFacingTarget( tgt_player ) )
        {
            return false;
        }

        float distance = vector.DistanceSq( tgt_player.GetPosition(), GetGame().GetPlayer().GetPosition() );
        if ( distance >= NIMBY_GIVE_TAKE_MAX_DISTANCE_SQ )
        {
            return false;
        }

        item = tgt_player.GetItemInHands();
        if ( !item )
        {
            return false;
        }

        return true;
    }

    // change UI to show item in hands name
    override string GetItemDesc( ActionBase action )
    {
        ItemBase item;
        if ( NimbyCanShowGiveInformation( item ) )
        {
            return item.GetDisplayName();
        }
        else
        {
            return super.GetItemDesc( action );
        }
    }

    // change UI to show item in hands health level
    override int GetItemHealth()
    {
        ItemBase item;
        if ( NimbyCanShowGiveInformation( item ) )
        {
            return item.GetHealthLevel();
        }
        else
        {
            return super.GetItemHealth();
        }
    }

    // change UI to show item in hands cargo count
    override void GetItemCargoCount( out int cargoCount )
    {
        ItemBase item;
        if ( NimbyCanShowGiveInformation( item ) )
        {
            cargoCount = AttachmentsWithInventoryOrCargoCount( item.GetInventory() );
        }
        else
        {
            super.GetItemCargoCount(cargoCount);
        }
    }
}
