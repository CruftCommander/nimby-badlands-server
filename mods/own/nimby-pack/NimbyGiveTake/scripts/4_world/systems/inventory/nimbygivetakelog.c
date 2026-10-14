// -----------------------------------------------------------------------------
// NimbyGiveTakeLog - one structured server log line per player-to-player transfer.
// Purpose : Lets an admin answer "who gave what to whom, and did it work" from the
//           server script log (review CR-003). Expected refusals are logged here
//           instead of raising Error() with no context (review CR-006).
// Format  : [NIMBY][GiveTake] action=<class> result=<OK|REFUSED|FAILED|REVERTED|GROUND>
//           giver=<name> (<id>) receiver=<name> (<id>) item=<type> #<netid> detail=<text>
//           The id is the BI player id that the server .ADM log also uses.
// -----------------------------------------------------------------------------

class NimbyGiveTakeLog
{
    static string DescribePlayer(Man player)
    {
        if (!player)
        {
            return "none";
        }

        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
        {
            return "no-identity";
        }
        return identity.GetName() + " (" + identity.GetId() + ")";
    }

    static string DescribeItem(EntityAI item)
    {
        if (!item)
        {
            return "none";
        }
        return item.GetType() + " #" + item.GetNetworkIDString();
    }

    static void Transfer(string action, string result, Man giver, Man receiver, EntityAI item, string detail)
    {
        Print("[NIMBY][GiveTake] action=" + action + " result=" + result + " giver=" + DescribePlayer(giver) + " receiver=" + DescribePlayer(receiver) + " item=" + DescribeItem(item) + " detail=" + detail);
    }
}
