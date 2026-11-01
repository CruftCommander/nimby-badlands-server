// -----------------------------------------------------------------------------
// NimbyAdminServer - Discord delivery (Plan 3 v0.8 N5, Phase 3 Discord list; recon 7; ruling 7).
// Purpose : A bounded queue (discordQueueMax), one POST in flight, pumped from MissionServer.OnUpdate
//           with tick-time comparisons (no CallLater, shared rules 7.1). Every error class and
//           timeout retries the same message after 1 s, 5 s, 30 s, then drops it. One local line
//           per failure class per episode; a success ends the episode. A command never waits on
//           this queue and is never repeated because its notification failed (the audit file is
//           the record).
// Secret  : The webhook URL lives only in config.json and in this object's memory (N5). It is
//           never logged, never sent to a client, and RestApi.EnableDebug (RestApi.c:165) is
//           never called, since engine debug output could print it.
// Engine  : CreateRestApi, GetRestApi (3_Game/DayZ/Http/RestApi.c:181,183); RestApi.SetOption
//           with ERestOption (:175, :28); RestApi.GetRestContext (:155); RestContext.SetHeader
//           (:141, Content-Type only) and POST (:123); RestCallback.OnSuccess, OnError, OnTimeout
//           (:73, :55, :64; OnError may fire more than once per request, :53); ERestResultState (:8).
// Style   : One statement per line.
// -----------------------------------------------------------------------------

class NimbyAdminServerDiscordCallback extends RestCallback
{
    int m_Sequence;
    bool m_Fired;

    void NimbyAdminServerDiscordCallback(int sequence)
    {
        m_Sequence = sequence;
    }

    override void OnSuccess(string data, int dataSize)
    {
        if (m_Fired)
        {
            return;
        }
        m_Fired = true;
        NimbyAdminServerDiscord.OnFinished(m_Sequence, true, "ok");
    }

    override void OnError(int errorCode)
    {
        if (m_Fired)
        {
            return;
        }
        m_Fired = true;
        NimbyAdminServerDiscord.OnFinished(m_Sequence, false, NimbyAdminServerDiscord.ErrorClass(errorCode));
    }

    override void OnTimeout()
    {
        if (m_Fired)
        {
            return;
        }
        m_Fired = true;
        NimbyAdminServerDiscord.OnFinished(m_Sequence, false, "timeout");
    }
}

class NimbyAdminServerDiscord
{
    protected static bool s_Enabled;
    protected static string s_Base = "";
    protected static string s_Path = "";
    protected static int s_QueueMax = 200;
    protected static ref array<string> s_Queue;
    protected static ref NimbyAdminServerDiscordCallback s_InFlight;
    // Finished callbacks stay referenced for a while: OnFinished runs inside the callback, so it
    // must not drop the last reference to it, and OnError can fire again later (RestApi.c:53).
    protected static ref array<ref NimbyAdminServerDiscordCallback> s_Retired;
    protected static int s_Sequence;
    protected static int s_Attempts;
    protected static float s_NextAt;
    protected static bool s_Overflowing;
    protected static ref array<string> s_LoggedClasses;

    static void Init(NimbyAdminServerConfig config)
    {
        s_Queue = new array<string>;
        s_Retired = new array<ref NimbyAdminServerDiscordCallback>;
        s_LoggedClasses = new array<string>;
        s_QueueMax = config.discordQueueMax;
        s_Enabled = false;
        if (!config.HasWebhook())
        {
            return;
        }
        if (!NimbyAdminServerRules.SplitWebhook(config.webhookUrl, s_Base, s_Path))
        {
            return;
        }
        RestApi api = GetRestApi();
        if (!api)
        {
            api = CreateRestApi();
        }
        if (!api)
        {
            NimbyAdminLog.Info("discord off: RestApi unavailable");
            return;
        }
        api.SetOption(ERestOption.ERESTOPTION_READOPERATION, 10);
        api.SetOption(ERestOption.ERESTOPTION_CONNECTION, 10);
        s_Enabled = true;
    }

    static bool IsEnabled()
    {
        return s_Enabled;
    }

    static string ErrorClass(int errorCode)
    {
        if (errorCode == ERestResultState.EREST_ERROR_CLIENTERROR)
        {
            return "client";
        }
        if (errorCode == ERestResultState.EREST_ERROR_SERVERERROR)
        {
            return "server";
        }
        if (errorCode == ERestResultState.EREST_ERROR_TIMEOUT)
        {
            return "timeout";
        }
        return "other";
    }

    static void Enqueue(string text)
    {
        if (!s_Enabled)
        {
            return;
        }
        if (s_Queue.Count() >= s_QueueMax)
        {
            if (!s_Overflowing)
            {
                s_Overflowing = true;
                NimbyAdminLog.Info("discord queue full max=" + s_QueueMax.ToString() + "; newest messages dropped until it drains");
            }
            return;
        }
        s_Overflowing = false;
        s_Queue.Insert(NimbyAdminServerRules.DiscordPayload(text));
    }

    static void Tick(float now)
    {
        if (!s_Enabled || s_InFlight || s_Queue.Count() == 0)
        {
            return;
        }
        if (now < s_NextAt)
        {
            return;
        }
        RestApi api = GetRestApi();
        if (!api)
        {
            return;
        }
        RestContext context = api.GetRestContext(s_Base);
        if (!context)
        {
            return;
        }
        s_Sequence = s_Sequence + 1;
        s_InFlight = new NimbyAdminServerDiscordCallback(s_Sequence);
        context.SetHeader("application/json");
        context.POST(s_InFlight, s_Path, s_Queue.Get(0));
    }

    static void OnFinished(int sequence, bool ok, string errorClass)
    {
        if (!s_InFlight || sequence != s_InFlight.m_Sequence)
        {
            return;
        }
        s_Retired.Insert(s_InFlight);
        if (s_Retired.Count() > 8)
        {
            s_Retired.Remove(0);
        }
        s_InFlight = null;
        float now = GetGame().GetTickTime();
        if (ok)
        {
            int bytes = 0;
            if (s_Queue.Count() > 0)
            {
                bytes = s_Queue.Get(0).Length();
                s_Queue.Remove(0);
            }
            s_Attempts = 0;
            s_NextAt = now;
            s_LoggedClasses.Clear();
            NimbyAdminLog.Debug("discord delivered seq=" + sequence.ToString() + " bytes=" + bytes.ToString());
            return;
        }
        if (s_LoggedClasses.Find(errorClass) < 0)
        {
            s_LoggedClasses.Insert(errorClass);
            NimbyAdminLog.Info("discord failed class=" + errorClass + " queued=" + s_Queue.Count().ToString());
        }
        s_Attempts = s_Attempts + 1;
        float delay = NimbyAdminServerRules.RetryDelay(s_Attempts);
        if (delay < 0)
        {
            if (s_Queue.Count() > 0)
            {
                s_Queue.Remove(0);
            }
            s_Attempts = 0;
            s_NextAt = now;
            NimbyAdminLog.Debug("discord dropped seq=" + sequence.ToString() + " after retries class=" + errorClass);
            return;
        }
        s_NextAt = now + delay;
    }
}
