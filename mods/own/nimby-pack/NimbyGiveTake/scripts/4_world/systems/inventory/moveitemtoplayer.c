class MoveItemToPlayer
{
    protected DayZPlayerImplement m_DstPlayer;
    protected EntityAI m_Item;
    protected FindInventoryLocationType m_InventoryLocationFindType;

    protected ref InventoryLocation m_SrcLocation;
    protected ref InventoryLocation m_DstLocation;

    void MoveItemToPlayer( DayZPlayerImplement dst_player, EntityAI item, FindInventoryLocationType find_type = FindInventoryLocationType.ANY )
    {
        m_DstPlayer = dst_player;
        m_Item = item;
        m_InventoryLocationFindType = find_type;
    }

    protected bool PrepareExecution()
    {
        // also works in singleplayer!
        if ( GetGame().IsClient() )
        {
            Error( "[inv] MoveItemToPlayer : Can not be executed on client!" );
            return false;
        }

        if ( !m_DstPlayer )
        {
            Error( "[inv] MoveItemToPlayer : No target player!" );
            return false;
        }

        if ( !m_Item )
        {
            Error( "[inv] MoveItemToPlayer : No source item!" );
            return false;
        }

        // get current item location
        m_SrcLocation = new InventoryLocation;
        if ( !m_Item.GetInventory().GetCurrentInventoryLocation( m_SrcLocation ) )
        {
            Error( "[inv] MoveItemToPlayer : Unable to retrieve current item location!" );
            return false;
        }
#ifdef DIAG
        Print( "[inv] MoveItemToPlayer : src = " + InventoryLocation.DumpToStringNullSafe( m_SrcLocation ) );
#endif

        // find and check target location
        m_DstLocation = new InventoryLocation;
        if ( !m_DstPlayer.GetInventory().FindFreeLocationFor( m_Item, m_InventoryLocationFindType, m_DstLocation ) )
        {
            Error( "[inv] MoveItemToPlayer : Unable to move entity to new player. No space in designated location!" );
            return false;
        }
#ifdef DIAG
        Print( "[inv] MoveItemToPlayer : dst = " + InventoryLocation.DumpToStringNullSafe( m_DstLocation ) );
#endif

        return true;
    }

    protected void CheckAndCancleHandEvent( DayZPlayerImplement player, HandEventID id )
    {
        DayZPlayerInventory inv = player.GetDayZPlayerInventory();
        if ( inv )
        {
            DeferredHandEvent e = DeferredHandEvent.Cast( inv.m_DeferredEvent );
            if ( e && e.m_event.GetEventID() == id )
            {
                // dequeue the event
                inv.m_DeferredEvent = null;
            }
        }
    }

    bool Execute()
    {
        if ( !PrepareExecution() )
        {
            return false;
        }

        /*
         * We have to be a bit tricky here!
         * It is not possible to "just move" the entity directly without potentially causing desync issues
         * depending on where the item is coming from or going to.
         *
         * If the src location or dst location is in a players hands additional syncronization means have to
         * be fullfiled.
         */
        bool released_on_src = false;
        bool received_on_dst = true;

        // delete network information to move entity without clients noticing
        GetGame().RemoteObjectTreeDelete( m_Item );

        // check if entity can be moved
        if ( GameInventory.LocationCanMoveEntity( m_SrcLocation, m_DstLocation ) )
        {
            if ( GameInventory.LocationSyncMoveEntity( m_SrcLocation, m_DstLocation ) )
            {
                DayZPlayerImplement src_player = null;

                // if item is moved out of a players hands we have to take care of hand synchronization
                released_on_src = m_SrcLocation.GetType() != InventoryLocationType.HANDS;
                if ( !released_on_src )
                {
                    src_player = DayZPlayerImplement.Cast ( m_SrcLocation.GetParent().GetHierarchyRootPlayer() );
                    if ( src_player )
                    {
                        // simulate deletion. necessary to keep server in sync
                        released_on_src = src_player.GetHumanInventory().HandEvent( InventoryMode.LOCAL, new HandEventDestroyed( src_player ) );
                    }
                }

                received_on_dst = m_DstLocation.GetType() != InventoryLocationType.HANDS;
                if ( released_on_src && !received_on_dst )
                {
                    // simulate creation. necessary to keep server in sync
                    received_on_dst = m_DstPlayer.GetHumanInventory().HandEvent( InventoryMode.LOCAL, new HandEventCreated( m_DstPlayer, m_DstLocation ) );
                }

                if ( released_on_src && !received_on_dst )
                {
                    Error( "[inv] MoveItemToPlayer : Unable to receive item on player. Reverting!" );

                    // first try to move item back to its original location
                    if ( GameInventory.LocationSyncMoveEntity( m_DstLocation, m_SrcLocation ) )
                    {
                        // cancle already set up HandEventDestroyed event
                        if ( src_player )
                        {
                            CheckAndCancleHandEvent( src_player, HandEventID.DESTROYED );
                        }
                    }
                    else
                    {
                        Error( "[inv] MoveItemToPlayer : Unable to revert. Try to place on ground instead!" );
                        InventoryLocation gnd_loc = new InventoryLocation;
                        if ( m_SrcLocation.GetParent() )
                        {
                            GameInventory.SetGroundPosByOwner( m_SrcLocation.GetParent(), m_Item, gnd_loc );
                        }
                        else
                        {
                            GameInventory.SetGroundPosByOwner( m_DstPlayer, m_Item, gnd_loc );
                        }
                        GameInventory.LocationSyncMoveEntity( m_DstLocation, gnd_loc );
                    }
                }
            }
        }

        // recreate network information after move is complete
        GetGame().RemoteObjectTreeCreate( m_Item );

        if ( released_on_src && received_on_dst )
        {
            OnMoveSuccessful();
        }
        else
        {
            OnMoveFailed();
        }

        return released_on_src && received_on_dst;
    }

    protected void OnMoveSuccessful()
    {
    }

    protected void OnMoveFailed()
    {
    }
}