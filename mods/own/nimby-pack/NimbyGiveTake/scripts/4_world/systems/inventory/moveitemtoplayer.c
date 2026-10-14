// -----------------------------------------------------------------------------
// NimbyMoveItemToPlayer - moves an item from one player's hands into another
// player's inventory (cargo or an attachment slot, never hands).
//
// Rewritten October 14, 2026 (review CR-001). The original deleted the item's
// network state, moved it natively, and injected fake hand events on both
// players; one failure path left both players' hand state machines wrong while
// the item had already moved. This version uses the vanilla server path:
// Man.ServerTakeToDst on the player holding the item, which routes a
// HandEventMoveTo through that player's own hand state machine
// (HumanInventory.RedirectToHandEvent). Because the item never lands in the
// receiver's hands, no second hand state machine has to be updated - the same
// way vanilla moves an item from hands into a container.
//
// The hand event is processed later by the holder's inventory (it is deferred),
// so Execute only reports whether the request was accepted. The receiver checks
// where the item actually ended up after VERIFY_DELAY_MS and logs the outcome
// (PlayerBase.NimbyVerifyTransfer).
// -----------------------------------------------------------------------------

class NimbyMoveItemToPlayer
{
    // Milliseconds before the outcome is checked; long enough for a deferred hand event.
    static const int VERIFY_DELAY_MS = 1000;

    // Destinations a transfer may use. Hands are excluded on purpose (see header). A function
    // rather than a static const: vanilla has no example of a const built from enum flags.
    static int DestinationFlags()
    {
        return FindInventoryLocationType.ANY_CARGO | FindInventoryLocationType.ATTACHMENT;
    }

    protected PlayerBase m_DstPlayer;
    protected EntityAI m_Item;
    protected string m_ActionName;
    protected Man m_SrcPlayer;

    void NimbyMoveItemToPlayer( PlayerBase dst_player, EntityAI item, string action_name )
    {
        m_DstPlayer = dst_player;
        m_Item = item;
        m_ActionName = action_name;
        if ( item )
        {
            m_SrcPlayer = item.GetHierarchyRootPlayer();
        }
    }

    // True when the receiver has room for the item outside their hands. Used by action
    // conditions so the prompt only shows when the transfer can succeed.
    static bool ReceiverHasSpace( PlayerBase receiver, EntityAI item )
    {
        if ( !receiver || !item )
        {
            return false;
        }
        InventoryLocation dst = new InventoryLocation;
        return receiver.GetInventory().FindFreeLocationFor( item, DestinationFlags(), dst );
    }

    protected void LogTransfer( string result, string detail )
    {
        NimbyGiveTakeLog.Transfer( m_ActionName, result, m_SrcPlayer, m_DstPlayer, m_Item, detail );
    }

    // Requests the move. Returns true when the request was accepted; the final outcome is
    // logged by the receiver after VERIFY_DELAY_MS.
    bool Execute()
    {
        if ( GetGame().IsClient() )
        {
            // A client calling this is a programming error, not a player outcome.
            Error( "[NIMBY][GiveTake] NimbyMoveItemToPlayer can only run on the server" );
            return false;
        }

        // Missing receiver or item happens when two players act on one offer at once;
        // it is an expected outcome, so it is logged rather than raised.
        if ( !m_DstPlayer )
        {
            LogTransfer( "REFUSED", "no receiver" );
            return false;
        }
        if ( !m_Item )
        {
            LogTransfer( "REFUSED", "no item" );
            return false;
        }

        InventoryLocation src = new InventoryLocation;
        if ( !m_Item.GetInventory().GetCurrentInventoryLocation( src ) )
        {
            LogTransfer( "REFUSED", "item location unavailable" );
            return false;
        }
        if ( src.GetType() != InventoryLocationType.HANDS )
        {
            LogTransfer( "REFUSED", "item is not in a player's hands" );
            return false;
        }

        Man holder = Man.Cast( src.GetParent() );
        if ( !holder )
        {
            LogTransfer( "REFUSED", "item holder is not a player" );
            return false;
        }
        if ( holder == m_DstPlayer )
        {
            LogTransfer( "REFUSED", "giver and receiver are the same player" );
            return false;
        }

        InventoryLocation dst = new InventoryLocation;
        if ( !m_DstPlayer.GetInventory().FindFreeLocationFor( m_Item, DestinationFlags(), dst ) )
        {
            LogTransfer( "REFUSED", "no space in receiver inventory" );
            return false;
        }

        if ( !holder.ServerTakeToDst( src, dst ) )
        {
            LogTransfer( "FAILED", "server move rejected" );
            return false;
        }

        LogTransfer( "REQUESTED", "dst=" + InventoryLocation.DumpToStringNullSafe( dst ) );
        GetGame().GetCallQueue( CALL_CATEGORY_GAMEPLAY ).CallLater( m_DstPlayer.NimbyVerifyTransfer, VERIFY_DELAY_MS, false, m_ActionName, holder, m_Item );
        return true;
    }
}
