// -----------------------------------------------------------------------------
// NimbyGreet - server side (plan G3; Phase 0 items 2 and 6).
// Purpose : At mission start, reads $profile:NimbyGreet\greet.json with
//           JsonFileLoader<T>.LoadFile (3_Game/DayZ/tools/JsonFileLoader.c:7) and logs
//           the result: a missing file is written with a default (the G3 rule), a
//           malformed file logs one "greet.json invalid: <error>" line and the server
//           keeps running (item 6). Logs each ClientReady connect with both ids
//           (InvokeOnConnect, 5_Mission/DayZ/mission/missionServer.c:435, reached from
//           ClientReadyEventTypeID at :345-357), so the record can place the client's
//           request against the connect. The request itself is answered in PlayerBase.
// Probe   : the JSON shape is a two-field stand-in (version, text); the G13/G14 shape
//           lands in Phase 1. The file is only read and logged here, never served.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyGreetProbeText
{
    int version = 1;
    string text = "Welcome to NIMBY. Edit $profile:NimbyGreet\\greet.json on the server.";
}

modded class MissionServer extends MissionBase
{
    override void OnInit()
    {
        super.OnInit();
        NimbyGreetLoadText();
    }

    override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
    {
        super.InvokeOnConnect(player, identity);
        NimbyGreetLog.Probe("ClientReady: " + NimbyGreetLog.Who(identity) + " hasPlayer=" + NimbyGreetLog.Bool(player != null));
    }

    protected void NimbyGreetLoadText()
    {
        if (!FileExist(NimbyGreetRules.DIR))
        {
            MakeDirectory(NimbyGreetRules.DIR);
        }
        NimbyGreetProbeText data = new NimbyGreetProbeText();
        if (!FileExist(NimbyGreetRules.FILE_TEXT))
        {
            string saveError;
            bool saved = JsonFileLoader<NimbyGreetProbeText>.SaveFile(NimbyGreetRules.FILE_TEXT, data, saveError);
            NimbyGreetLog.Info("greet.json missing; default written=" + NimbyGreetLog.Bool(saved) + " error=" + saveError);
            return;
        }
        string error;
        bool loaded = JsonFileLoader<NimbyGreetProbeText>.LoadFile(NimbyGreetRules.FILE_TEXT, data, error);
        if (!loaded)
        {
            NimbyGreetLog.Info("greet.json invalid: " + error);
            return;
        }
        NimbyGreetLog.Info("greet.json loaded version=" + data.version.ToString() + " textBytes=" + data.text.Length().ToString());
    }
}
