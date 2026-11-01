// -----------------------------------------------------------------------------
// NimbyAdminServer - Discord delivery (Plan 3 v0.8 N5, Phase 3 Discord list; recon 7; ruling 7;
// gate-session fix round, November 1, 2026).
// Purpose : A bounded queue (discordQueueMax), one POST in flight, pumped from MissionServer.OnUpdate
//           with tick-time comparisons (no CallLater, shared rules 7.1). A command never waits on
//           this queue and is never repeated because its notification failed (the audit file is
//           the record).
// Retry   : Gate session of November 1, 2026: Discord answers a webhook POST with 204 and an
//           empty body, which 164200 reported through OnError, so every delivered message was
//           re-sent (teleport 3 posted four times). Now (orchestrator ruling): the request path
//           carries wait=true, added at send time and never stored, so Discord answers 200 with a
//           body; a message is retried only after a timeout, and only while no other callback has
//           settled it; any other error is logged once per class per episode and the message is
//           dropped, because a POST that reached Discord cannot be told apart from one that did
//           not. A late success or error from an earlier attempt settles the message and cancels
//           a retry not yet sent. Residual: a timed-out POST that did land and is then re-sent
//           posts twice.
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
    int m_MessageId;
    int m_Sequence;

    void NimbyAdminServerDiscordCallback(int messageId, int sequence)
    {
        m_MessageId = messageId;
        m_Sequence = sequence;
    }

    override void OnSuccess(string data, int dataSize)
    {
        NimbyAdminServerDiscord.OnFinished(m_MessageId, m_Sequence, NimbyAdminServerDiscordQueue.KIND_OK, 0);
    }

    override void OnError(int errorCode)
    {
        NimbyAdminServerDiscord.OnFinished(m_MessageId, m_Sequence, NimbyAdminServerDiscord.ErrorClass(errorCode), errorCode);
    }

    override void OnTimeout()
    {
        NimbyAdminServerDiscord.OnFinished(m_MessageId, m_Sequence, NimbyAdminServerDiscordQueue.KIND_TIMEOUT, -1);
    }
}

// The delivery state machine, engine-free and self-tested (gate-session fix round). One message
// at the head is worked at a time; each POST of it is one attempt with its own sequence number.
class NimbyAdminServerDiscordQueue
{
    static const string KIND_OK = "ok";
    static const string KIND_TIMEOUT = "timeout";
    static const string DELIVERED = "delivered";
    static const string DROPPED = "dropped";
    static const string RETRY = "retry";
    static const string IGNORED = "ignored";

    protected ref array<string> m_Payloads = new array<string>;
    protected ref array<int> m_Ids = new array<int>;
    protected int m_NextId;
    protected int m_Sequence;
    protected int m_InFlightSequence;
    protected int m_Attempts;
    protected float m_NextAt;
    int m_Max = 200;
    // The payload size of the message the last DELIVERED or DROPPED outcome settled.
    int m_SettledBytes;

    // False when the queue is full (the message is not kept).
    bool Add(string payload)
    {
        if (m_Payloads.Count() >= m_Max)
        {
            return false;
        }
        m_NextId = m_NextId + 1;
        m_Payloads.Insert(payload);
        m_Ids.Insert(m_NextId);
        return true;
    }

    int Count()
    {
        return m_Payloads.Count();
    }

    bool InFlight()
    {
        return m_InFlightSequence != 0;
    }

    // True when the head message should be posted now; marks the attempt in flight.
    bool NextSend(float now, out int messageId, out int sequence, out string payload)
    {
        messageId = 0;
        sequence = 0;
        payload = "";
        if (m_InFlightSequence != 0 || m_Payloads.Count() == 0 || now < m_NextAt)
        {
            return false;
        }
        m_Sequence = m_Sequence + 1;
        m_InFlightSequence = m_Sequence;
        messageId = m_Ids.Get(0);
        sequence = m_Sequence;
        payload = m_Payloads.Get(0);
        return true;
    }

    // One callback of any attempt. kind is KIND_OK, KIND_TIMEOUT, or an error class.
    string OnCallback(int messageId, int sequence, string kind, float now)
    {
        bool current = sequence == m_InFlightSequence;
        if (current)
        {
            m_InFlightSequence = 0;
        }
        if (m_Ids.Count() == 0 || m_Ids.Get(0) != messageId)
        {
            // A message already settled (a late or repeated callback).
            return IGNORED;
        }
        if (!current)
        {
            // An earlier attempt of the head message, after a timeout. A success or an error
            // settles it and cancels a retry not yet sent; another timeout changes nothing.
            if (kind == KIND_TIMEOUT)
            {
                return IGNORED;
            }
            return Settle(kind == KIND_OK, now);
        }
        if (kind == KIND_TIMEOUT)
        {
            m_Attempts = m_Attempts + 1;
            float delay = NimbyAdminServerRules.RetryDelay(m_Attempts);
            if (delay >= 0)
            {
                m_NextAt = now + delay;
                return RETRY;
            }
            return Settle(false, now);
        }
        return Settle(kind == KIND_OK, now);
    }

    protected string Settle(bool ok, float now)
    {
        m_SettledBytes = m_Payloads.Get(0).Length();
        m_Payloads.RemoveOrdered(0);
        m_Ids.RemoveOrdered(0);
        m_Attempts = 0;
        m_NextAt = now;
        if (ok)
        {
            return DELIVERED;
        }
        return DROPPED;
    }
}

class NimbyAdminServerDiscord
{
    protected static bool s_Enabled;
    protected static string s_Base = "";
    protected static string s_Path = "";
    protected static ref NimbyAdminServerDiscordQueue s_Queue;
    // Callbacks stay referenced while the engine may still call them (OnError can fire again
    // later, RestApi.c:53); the newest 16 are kept.
    protected static ref array<ref NimbyAdminServerDiscordCallback> s_Callbacks;
    protected static bool s_Overflowing;
    protected static ref array<string> s_LoggedClasses;

    static void Init(NimbyAdminServerConfig config)
    {
        s_Queue = new NimbyAdminServerDiscordQueue();
        s_Queue.m_Max = config.discordQueueMax;
        s_Callbacks = new array<ref NimbyAdminServerDiscordCallback>;
        s_LoggedClasses = new array<string>;
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
            return NimbyAdminServerDiscordQueue.KIND_TIMEOUT;
        }
        return "other";
    }

    static void Enqueue(string text)
    {
        if (!s_Enabled)
        {
            return;
        }
        if (!s_Queue.Add(NimbyAdminServerRules.DiscordPayload(text)))
        {
            if (!s_Overflowing)
            {
                s_Overflowing = true;
                NimbyAdminLog.Info("discord queue full max=" + s_Queue.m_Max.ToString() + "; newest messages dropped until it drains");
            }
            return;
        }
        s_Overflowing = false;
    }

    static void Tick(float now)
    {
        if (!s_Enabled)
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
        int messageId;
        int sequence;
        string payload;
        if (!s_Queue.NextSend(now, messageId, sequence, payload))
        {
            return;
        }
        NimbyAdminServerDiscordCallback callback = new NimbyAdminServerDiscordCallback(messageId, sequence);
        s_Callbacks.Insert(callback);
        if (s_Callbacks.Count() > 16)
        {
            s_Callbacks.RemoveOrdered(0);
        }
        context.SetHeader("application/json");
        context.POST(callback, NimbyAdminServerRules.WebhookRequestPath(s_Path), payload);
    }

    // Runs inside the engine callback; the callback object stays referenced in s_Callbacks.
    static void OnFinished(int messageId, int sequence, string kind, int errorCode)
    {
        if (!s_Queue)
        {
            return;
        }
        float now = GetGame().GetTickTime();
        string outcome = s_Queue.OnCallback(messageId, sequence, kind, now);
        if (outcome == NimbyAdminServerDiscordQueue.IGNORED)
        {
            NimbyAdminLog.Debug("discord late callback ignored seq=" + sequence.ToString() + " kind=" + kind);
            return;
        }
        if (outcome == NimbyAdminServerDiscordQueue.DELIVERED)
        {
            s_LoggedClasses.Clear();
            NimbyAdminLog.Debug("discord delivered seq=" + sequence.ToString() + " bytes=" + s_Queue.m_SettledBytes.ToString());
            return;
        }
        string line = NimbyAdminServerRules.DiscordFailedLine(kind, errorCode, outcome, s_Queue.Count());
        if (s_LoggedClasses.Find(kind) < 0)
        {
            s_LoggedClasses.Insert(kind);
            NimbyAdminLog.Info(line);
            return;
        }
        NimbyAdminLog.Debug(line);
    }
}
