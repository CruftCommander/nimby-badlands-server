// -----------------------------------------------------------------------------
// NimbyHandsClient - the giver's client side of step 1 (NIMBY-T-063, October 31, 2026).
// Purpose : Ten server crashes on 1.30.164200 (October 28 to 30) each followed a server-driven
//           hand event on a connected giver (Man.ServerTakeToDst from the call queue, and on
//           build 10 from the giver's own CommandHandler), and the same step 2 with no hand
//           event confirmed between two connected players (S6-D, testserver-20261031-153005).
//           So the giver's own client moves the item from hands into its own cargo with
//           Man.PredictiveTakeToDst (Man.c:738-750), the call vanilla's inventory screen makes
//           when a player drags the item in hands into cargo (HandsContainer.c:196 via
//           PredictiveTakeEntityToInventory). The engine carries it to the server as an ordinary
//           player hand event and applies it inside the giver's CommandHandler; the server's
//           signal 1 (ItemBase.EEItemLocationChanged) writes STAGED exactly as before.
// Trigger : PlayerBase.OnRPC with NimbyHandsRpc.STAGE, sent by NimbyHandsServer.RequestClientStage
//           to the giver's identity only, so it arrives on the controlled player.
// Refusal : Every check that fails answers NimbyHandsRpc.STAGE_REFUSED with the reason
//           (NimbyHandsRules.ClientStageRefusal) and the server writes REFUSED; the bounded
//           timeout covers a client that answers nothing. The staging cell is the same
//           FindStagingLocation the server computed, read on the client's own inventory view.
// Logs    : Two Info lines per request on the client ([NIMBY][Hands] client stage ...), so the
//           client script log shows the request and what was posted without the debug flag.
// -----------------------------------------------------------------------------
class NimbyHandsClient
{
    static void OnStageRequest(PlayerBase player, ParamsReadContext ctx)
    {
        Param3<int, int, int> request = new Param3<int, int, int>(0, 0, 0);
        if (!ctx.Read(request))
        {
            NimbyHandsLog.Info("client stage: unreadable request");
            return;
        }
        int id = request.param1;
        if (!player || !player.IsControlledPlayer())
        {
            // The RPC is addressed to the giver's identity, so this is a routing fault, not a
            // player outcome; the server's timeout ends the request.
            NimbyHandsLog.Info("client stage id=" + id.ToString() + " arrived on a player that is not the controlled player; ignored");
            return;
        }
        EntityAI item = EntityAI.Cast(GetGame().GetObjectByNetworkId(request.param2, request.param3));
        bool itemFound = item != null;
        bool inHands = itemFound && player.GetHumanInventory().GetEntityInHands() == item;
        bool canStoreInput = ScriptInputUserData.CanStoreInputUserData();
        InventoryLocation stage = new InventoryLocation;
        bool hasRoom = inHands && NimbyHandsMoveItemToPlayer.FindStagingLocation(player, item, stage);
        NimbyHandsLog.Info("client stage id=" + id.ToString() + " item=" + NimbyHandsServer.DescribeItem(item) + " found=" + NimbyHandsLog.Bool(itemFound) + " inHands=" + NimbyHandsLog.Bool(inHands) + " canStoreInput=" + NimbyHandsLog.Bool(canStoreInput) + " hasRoom=" + NimbyHandsLog.Bool(hasRoom));
        string refusal = NimbyHandsRules.ClientStageRefusal(itemFound, inHands, canStoreInput, hasRoom);
        if (refusal != "")
        {
            Refuse(player, id, refusal);
            return;
        }
        InventoryLocation src = new InventoryLocation;
        if (!item.GetInventory().GetCurrentInventoryLocation(src))
        {
            Refuse(player, id, "item location unknown on the client");
            return;
        }
        // For a HANDS source the return value only says the hand event was posted
        // (HumanInventory.RedirectToHandEvent, HumanInventory.c:200-229); the landing is read by
        // the server.
        bool posted = player.PredictiveTakeToDst(src, stage);
        NimbyHandsLog.Info("client stage id=" + id.ToString() + " posted=" + NimbyHandsLog.Bool(posted) + " src=" + NimbyHandsServer.DescribeLocation(src) + " stage=" + NimbyHandsServer.DescribeLocation(stage));
        if (!posted)
        {
            Refuse(player, id, "PredictiveTakeToDst returned false");
        }
    }

    protected static void Refuse(PlayerBase player, int id, string reason)
    {
        Param2<int, string> reply = new Param2<int, string>(id, reason);
        GetGame().RPCSingleParam(player, NimbyHandsRpc.STAGE_REFUSED, reply, true);
        NimbyHandsLog.Info("client stage id=" + id.ToString() + " refused: " + reason);
    }
}
