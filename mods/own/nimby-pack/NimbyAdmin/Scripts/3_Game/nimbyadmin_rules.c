// -----------------------------------------------------------------------------
// NimbyAdmin - rules: the wire definition both halves read, and the engine-free decisions the
// self-test covers (shared rules 8). No engine call in this file except string methods.
// Plan  : docs/plans/2026-10-15-nimby-admin-plan.md v0.8: N1 (identifiers), N9 (push envelope,
//         reserved ids), the Phase 0 security contract; recon 2026-10-31-nimby-admin-phase0-recon.md
//         section 4 (accepted October 31, 2026). A constant change is a plan change first.
// Split : This client PBO also loads on the server (@NIMBY is a -mod= there), so NimbyAdminServer
//         reads the wire definition from here: one copy of the RPC ids, the args codec and the
//         payload classes. Every authorization and every other check lives in NimbyAdminServer
//         (shared rules 8.1); nothing here decides what a player may do.
// Engine: string.Length, Substring, Get, ToAscii, IndexOf (1_Core/DayZ/proto/EnString.c:199,113,
//         448,86,240); Length and Substring count bytes.
// Style : one statement per line.
// -----------------------------------------------------------------------------

class NimbyAdminRules
{
    // N1 and shared rules 4.1: the 65503xx block; 6550350-6550399 stay reserved (N9d).
    static const int RPC_BLOCK_FIRST = 6550300;
    static const int RPC_BLOCK_LAST = 6550399;
    static const int RPC_OBSERVE_FIRST = 6550350;
    static const int RPC_OBSERVE_LAST = 6550399;

    // Recon 4.1. Request Param4<int protocol, int requestId, string command, string args>; result
    // Param3<int requestId, int code, string detail>; push Param3<int kind, int sequence, string
    // payload>; chunk and done are the NimbyShared codec's Param3 records.
    static const int RPC_REQUEST = 6550301;
    static const int RPC_RESULT = 6550302;
    static const int RPC_PUSH = 6550303;
    static const int RPC_CHUNK = 6550304;
    static const int RPC_DONE = 6550305;

    static const int MENU_ID = 65503;

    // Recon 4.2: the request envelope.
    static const int PROTOCOL = 1;
    static const int MAX_COMMAND_BYTES = 32;
    static const int MAX_ARGS_BYTES = 900;
    static const int MAX_ARGS_PAIRS = 16;
    static const int MAX_KEY_BYTES = 32;
    static const int MAX_VALUE_BYTES = 200;
    static const int MAX_DETAIL_BYTES = 900;

    // Phase 0 commands.
    static const string CMD_HELLO = "session.hello";
    static const string CMD_PLAYERS = "players.list";
    static const string CMD_TELEPORT_TO = "player.teleportTo";

    // Recon 4.4: result codes.
    static const int CODE_OK = 0;
    static const int CODE_PARTIAL = 1;
    static const int CODE_DENIED = 2;
    static const int CODE_INVALID = 3;
    static const int CODE_FAILED = 4;

    // Recon 4.5: push kinds. ROLE is the only kind in this plan (N9a).
    static const int PUSH_ROLE = 1;

    static const string ROLE_OWNER = "owner";
    static const string ROLE_ADMIN = "admin";
    static const string ROLE_NONE = "none";

    // The client's cap on one codec response (recon 8.3).
    static const int MAX_LIST_BYTES = 65536;

    // Client: a request with no result after this long shows "no answer" (recon 8.5).
    static const float REQUEST_TIMEOUT_SECONDS = 10.0;

    // Shared rules 4.3 and 8: the flag files.
    static const string DIR = "$profile:NimbyAdmin";
    static const string FILE_DEBUG = "$profile:NimbyAdmin\\debug";
    static const string FILE_SELFTEST = "$profile:NimbyAdmin\\selftest";

    // A log line that a client can cause is written at most once per this many seconds per key.
    static const float LOG_GATE_SECONDS = 10.0;

    // True for an RPC id a command MAY use: inside the block and outside the reserved range.
    static bool IsCommandRpc(int id)
    {
        if (id < RPC_BLOCK_FIRST)
        {
            return false;
        }
        if (id > RPC_BLOCK_LAST)
        {
            return false;
        }
        if (id >= RPC_OBSERVE_FIRST && id <= RPC_OBSERVE_LAST)
        {
            return false;
        }
        return true;
    }

    // "" when the envelope fields are acceptable; otherwise the reason (recon 4.3 step 5).
    static string CheckEnvelope(int protocol, int requestId, string command, string args)
    {
        if (protocol != PROTOCOL)
        {
            return "protocol";
        }
        if (requestId <= 0)
        {
            return "request id";
        }
        if (command.Length() == 0)
        {
            return "command";
        }
        if (command.Length() > MAX_COMMAND_BYTES)
        {
            return "command";
        }
        if (args.Length() > MAX_ARGS_BYTES)
        {
            return "args too long";
        }
        return "";
    }

    static bool IsKnownRole(string role)
    {
        if (role == ROLE_OWNER)
        {
            return true;
        }
        if (role == ROLE_ADMIN)
        {
            return true;
        }
        return false;
    }

    static string CodeName(int code)
    {
        if (code == CODE_OK)
        {
            return "ok";
        }
        if (code == CODE_PARTIAL)
        {
            return "partial";
        }
        if (code == CODE_DENIED)
        {
            return "denied";
        }
        if (code == CODE_INVALID)
        {
            return "invalid";
        }
        return "failed";
    }

    // Cut text to at most maxBytes on a UTF-8 character boundary.
    static string CutBytes(string text, int maxBytes)
    {
        if (text.Length() <= maxBytes)
        {
            return text;
        }
        int end = NimbySharedChunks.ChunkEnd(text, 0, maxBytes);
        return text.Substring(0, end);
    }
}

// Recon 4.2, ruled October 31, 2026 (question 6): the request args are key=value pairs separated
// by ';'. In a value '%', ';' and '=' are written %25, %3B and %3D; any other '%' sequence, a raw
// '=' (review finding 11), and any byte below 32 is refused. Keys are [a-z0-9.], 1 to 32 bytes.
// Never JSON (recon R8).
class NimbyAdminArgs
{
    protected ref map<string, string> m_Values;
    protected ref array<string> m_Keys;

    void NimbyAdminArgs()
    {
        m_Values = new map<string, string>;
        m_Keys = new array<string>;
    }

    int Count()
    {
        return m_Keys.Count();
    }

    bool Has(string key)
    {
        return m_Values.Contains(key);
    }

    string Get(string key)
    {
        string value;
        if (m_Values.Find(key, value))
        {
            return value;
        }
        return "";
    }

    // Insert for the encoder; false on a bad key, a duplicate, or a refused value.
    bool Set(string key, string value)
    {
        if (!IsKey(key))
        {
            return false;
        }
        if (m_Values.Contains(key))
        {
            return false;
        }
        if (HasControlByte(value))
        {
            return false;
        }
        m_Values.Insert(key, value);
        m_Keys.Insert(key);
        return true;
    }

    // The wire text, in insertion order.
    string Encode()
    {
        string text = "";
        int i = 0;
        while (i < m_Keys.Count())
        {
            string key = m_Keys.Get(i);
            if (i > 0)
            {
                text = text + ";";
            }
            text = text + key + "=" + Escape(m_Values.Get(key));
            i = i + 1;
        }
        return text;
    }

    static bool IsKey(string key)
    {
        int length = key.Length();
        if (length < 1 || length > NimbyAdminRules.MAX_KEY_BYTES)
        {
            return false;
        }
        int i = 0;
        while (i < length)
        {
            int c = key.Get(i).ToAscii();
            bool lower = c >= 97 && c <= 122;
            bool digit = c >= 48 && c <= 57;
            bool dot = c == 46;
            if (!lower && !digit && !dot)
            {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    static bool HasControlByte(string text)
    {
        int i = 0;
        while (i < text.Length())
        {
            int c = text.Get(i).ToAscii();
            if (c >= 0 && c < 32)
            {
                return true;
            }
            i = i + 1;
        }
        return false;
    }

    static string Escape(string value)
    {
        string result = "";
        int i = 0;
        while (i < value.Length())
        {
            string ch = value.Get(i);
            if (ch == "%")
            {
                result = result + "%25";
            }
            else if (ch == ";")
            {
                result = result + "%3B";
            }
            else if (ch == "=")
            {
                result = result + "%3D";
            }
            else
            {
                result = result + ch;
            }
            i = i + 1;
        }
        return result;
    }

    // False with why set when text is not a valid escaped value.
    static bool Unescape(string text, out string value, out string why)
    {
        value = "";
        why = "";
        int i = 0;
        int length = text.Length();
        while (i < length)
        {
            string ch = text.Get(i);
            if (ch == "=")
            {
                why = "unescaped =";
                return false;
            }
            if (ch != "%")
            {
                value = value + ch;
                i = i + 1;
                continue;
            }
            if (i + 3 > length)
            {
                why = "bad escape";
                return false;
            }
            string code = text.Substring(i, 3);
            if (code == "%25")
            {
                value = value + "%";
            }
            else if (code == "%3B")
            {
                value = value + ";";
            }
            else if (code == "%3D")
            {
                value = value + "=";
            }
            else
            {
                why = "bad escape";
                return false;
            }
            i = i + 3;
        }
        return true;
    }

    // Parses the wire text. False with why set on the first rule broken; args is then empty.
    static bool Parse(string text, out NimbyAdminArgs args, out string why)
    {
        args = new NimbyAdminArgs();
        why = "";
        if (text.Length() > NimbyAdminRules.MAX_ARGS_BYTES)
        {
            why = "args too long";
            return false;
        }
        if (text.Length() == 0)
        {
            return true;
        }
        int start = 0;
        int length = text.Length();
        int pairs = 0;
        while (start <= length)
        {
            int end = text.IndexOfFrom(start, ";");
            if (end < 0)
            {
                end = length;
            }
            string pair = text.Substring(start, end - start);
            pairs = pairs + 1;
            if (pairs > NimbyAdminRules.MAX_ARGS_PAIRS)
            {
                why = "too many args";
                args = new NimbyAdminArgs();
                return false;
            }
            int eq = pair.IndexOf("=");
            if (eq <= 0)
            {
                why = "bad pair";
                args = new NimbyAdminArgs();
                return false;
            }
            string key = pair.Substring(0, eq);
            string raw = pair.Substring(eq + 1, pair.Length() - eq - 1);
            if (!IsKey(key))
            {
                why = "bad key";
                args = new NimbyAdminArgs();
                return false;
            }
            if (raw.Length() > NimbyAdminRules.MAX_VALUE_BYTES * 3)
            {
                why = "value too long";
                args = new NimbyAdminArgs();
                return false;
            }
            string value;
            string escapeWhy;
            if (!Unescape(raw, value, escapeWhy))
            {
                why = escapeWhy;
                args = new NimbyAdminArgs();
                return false;
            }
            if (value.Length() > NimbyAdminRules.MAX_VALUE_BYTES)
            {
                why = "value too long";
                args = new NimbyAdminArgs();
                return false;
            }
            if (HasControlByte(value))
            {
                why = "control byte";
                args = new NimbyAdminArgs();
                return false;
            }
            if (args.Has(key))
            {
                why = "duplicate key";
                args = new NimbyAdminArgs();
                return false;
            }
            args.Set(key, value);
            if (end >= length)
            {
                break;
            }
            start = end + 1;
        }
        return true;
    }
}

// N9a: the push kinds the client renders. Unknown kinds are dropped. The self-test registers a
// fake kind to prove the seam (N9, "one fake push kind").
class NimbyAdminPushKinds
{
    protected static ref array<int> s_Kinds;

    protected static void Ensure()
    {
        if (!s_Kinds)
        {
            s_Kinds = new array<int>;
            s_Kinds.Insert(NimbyAdminRules.PUSH_ROLE);
        }
    }

    static void Register(int kind)
    {
        Ensure();
        if (s_Kinds.Find(kind) < 0)
        {
            s_Kinds.Insert(kind);
        }
    }

    static void Unregister(int kind)
    {
        Ensure();
        if (kind == NimbyAdminRules.PUSH_ROLE)
        {
            return;
        }
        int at = s_Kinds.Find(kind);
        if (at >= 0)
        {
            s_Kinds.Remove(at);
        }
    }

    static bool IsKnown(int kind)
    {
        Ensure();
        return s_Kinds.Find(kind) >= 0;
    }
}

// Recon 8.3: one row of the Players refresh. The server fills and serializes it (JsonSerializer.
// WriteToString, 3_Game/DayZ/gameplay.c:68); the client parses what its own server sent.
class NimbyAdminPlayerRow
{
    int session;
    string biId;
    string steam64;
    string name;
    bool alive;
    float distance;
}

class NimbyAdminPlayerList
{
    ref array<ref NimbyAdminPlayerRow> players = new array<ref NimbyAdminPlayerRow>;
}

// Review finding 2 (October 31, 2026): a refresh keeps the selection only when a row carries both
// the selected BI id and the selected session number, so a selection never moves to a
// reconnected player's new session on its own (security contract, "targets resolve against
// current sessions"). Engine-free; the menu applies it in SetPlayers.
class NimbyAdminSelection
{
    static const string NOTICE_RECONNECTED = "selection cleared: player reconnected";
    static const string NOTICE_LEFT = "selection cleared: player left";

    // The row that keeps the selection, or -1. notice is "" when nothing was selected or the
    // selection was kept; otherwise why it was cleared.
    static int Rebind(NimbyAdminPlayerList list, string biId, int session, out string notice)
    {
        notice = "";
        if (biId == "" || !list)
        {
            return -1;
        }
        bool sameBi = false;
        int i = 0;
        while (i < list.players.Count())
        {
            NimbyAdminPlayerRow row = list.players.Get(i);
            if (row && row.biId == biId)
            {
                if (row.session == session)
                {
                    return i;
                }
                sameBi = true;
            }
            i = i + 1;
        }
        if (sameBi)
        {
            notice = NOTICE_RECONNECTED;
        }
        else
        {
            notice = NOTICE_LEFT;
        }
        return -1;
    }
}

// The codec's done record (Plan 11 C7: the consumer's own type).
class NimbyAdminDone
{
    int bytes;
    int hash;
    string kind;
}

// Review finding 1 (October 31, 2026): the highest request id the server accepted on each
// connection, by BI id, so the dev harness can forge above it (orchestrator ruling: highest plus
// 1024). Only NimbyAdminServer writes it, on the server; on a client nothing writes it and Get
// returns 0. No decision reads it: the replay check lives in NimbyAdminServerSeen.
class NimbyAdminHighWater
{
    protected static ref map<string, int> s_Highest;

    static void Publish(string biId, int highest)
    {
        if (!s_Highest)
        {
            s_Highest = new map<string, int>;
        }
        s_Highest.Set(biId, highest);
    }

    static void Clear(string biId)
    {
        if (s_Highest)
        {
            s_Highest.Remove(biId);
        }
    }

    static int Get(string biId)
    {
        int highest;
        if (s_Highest && s_Highest.Find(biId, highest))
        {
            return highest;
        }
        return 0;
    }
}

// The client's request-id counter: one fresh id per request for the whole client process.
class NimbyAdminRequestIds
{
    protected static int s_Next = 1;

    static int Next()
    {
        int id = s_Next;
        s_Next = s_Next + 1;
        return id;
    }
}
