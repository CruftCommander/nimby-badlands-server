// -----------------------------------------------------------------------------
// NimbyHandsActionData - what a transfer action saw when it started.
// Purpose : An interact action starts on the server after ActionBase.Can
//           (ActionManagerServer.c:168) and executes later on the animation event
//           (AnimatedActionBase.c:188-201) without re-checking its condition. The
//           server records the item and both identities at OnStartServer
//           (ActionBase.c:788) so the dispatch can refuse a stale request: the item
//           was replaced, or a player disconnected or was replaced (plan Phase 1).
// Vanilla : ActionBase.CreateActionData, ActionBase.c:240; ActionData, ActionBase.c:21.
// -----------------------------------------------------------------------------
class NimbyHandsActionData : ActionData
{
    EntityAI m_NimbyHandsItem;
    string m_NimbyHandsGiverId;
    string m_NimbyHandsReceiverId;

    static string IdentityId(Man player)
    {
        if (!player || !player.GetIdentity())
        {
            return "";
        }
        return player.GetIdentity().GetId();
    }

    void Capture(EntityAI item, Man giver, Man receiver)
    {
        m_NimbyHandsItem = item;
        m_NimbyHandsGiverId = IdentityId(giver);
        m_NimbyHandsReceiverId = IdentityId(receiver);
    }

    void ApplyTo(NimbyHandsMoveItemToPlayer move)
    {
        move.SetExpected(m_NimbyHandsItem, m_NimbyHandsGiverId, m_NimbyHandsReceiverId);
    }
}
