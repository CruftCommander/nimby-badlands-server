modded class ActionTargetsCursor
{
    static const float GIVE_AND_TAKE_MAX_DISTANCE = UAMaxDistances.DEFAULT * UAMaxDistances.DEFAULT;

    protected bool CanShowGiveInformation( out ItemBase item )
    {
        PlayerBase tgt_player;
        if ( !PlayerBase.CastTo( tgt_player, m_Target.GetObject() ) )
        {
            return false;
        }

        if ( tgt_player.GetGiveOrTakeState() != eGiveOrTake.GIVE )
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
        if ( distance >= GIVE_AND_TAKE_MAX_DISTANCE )
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
        if ( CanShowGiveInformation( item ) )
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
        if ( CanShowGiveInformation( item ) )
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
        if ( CanShowGiveInformation( item ) )
        {
            cargoCount = AttachmentsWithInventoryOrCargoCount( item.GetInventory() );
        }
        else
        {
            super.GetItemCargoCount(cargoCount);
        }
    }
}
