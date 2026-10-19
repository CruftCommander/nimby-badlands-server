// -----------------------------------------------------------------------------
// NimbyGiveTakeActionData - what a transfer action saw when it started.
// Purpose : An interact action starts on the server after ActionBase.Can
//           (ActionManagerServer.c:168) and executes later on the animation event
//           (AnimatedActionBase.c:188-201) without re-checking its condition. The
//           server records the item and both identities at OnStartServer
//           (ActionBase.c:788) so the dispatch can refuse a stale request: the item
//           was replaced, or a player disconnected or was replaced (plan Phase 1).
// Vanilla : ActionBase.CreateActionData, ActionBase.c:240; ActionData, ActionBase.c:21.
// -----------------------------------------------------------------------------
class NimbyGiveTakeActionData : ActionData
{
    EntityAI m_NimbyItem;
    string m_NimbyGiverId;
    string m_NimbyReceiverId;

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
        m_NimbyItem = item;
        m_NimbyGiverId = IdentityId(giver);
        m_NimbyReceiverId = IdentityId(receiver);
    }

    void ApplyTo(NimbyMoveItemToPlayer move)
    {
        move.SetExpected(m_NimbyItem, m_NimbyGiverId, m_NimbyReceiverId);
    }
}
