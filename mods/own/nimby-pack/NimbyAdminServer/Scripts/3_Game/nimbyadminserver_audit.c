// -----------------------------------------------------------------------------
// NimbyAdminServer - the audit journal (Plan 3 v0.8, audit record table; recon 6).
// Purpose : One JSON object per line in $profile:NimbyAdmin\audit\audit-<YYYY-MM-DD>.log (UTC).
//           A command that reaches its handler writes "intent" first and runs only when that
//           append succeeded; "result" follows. A denied or invalid request writes one "result"
//           record (plan line 3). One OpenFile per record (APPEND, FPrintln, CloseFile), so a
//           crash loses at most the record being written.
// Verify  : FPrintln and CloseFile return nothing (EnSystem.c:481, :443), so after the close the
//           day file is opened again and its last line compared with the record just written;
//           Append is true only on a match (review finding 4, October 31, 2026). One extra read
//           per record, at admin-command rates.
// Limit   : the readback proves the record reached the file, not that it survives a crash or a
//           power cut (recon R5, kept as a stated limitation).
// Engine  : OpenFile with FileMode.APPEND and READ (1_Core/DayZ/proto/EnSystem.c:417, :382-386),
//           FPrintln (:481), FGets (:501), CloseFile (:443); GetYearMonthDayUTC and
//           GetHourMinuteSecondUTC (:100, :76).
// Style   : One statement per line.
// -----------------------------------------------------------------------------

// One record; ToJson is engine-free and self-tested. target, args and effects are JSON objects
// the caller built with NimbyAdminServerJsonObject ("" leaves the field out).
class NimbyAdminServerAuditRecord
{
    string id;
    string phase;
    string ts;
    string actorSteam64;
    string actorBiId;
    string actorName;
    string command;
    string target;
    string args;
    string result;
    string effects;
    string reason;

    string ToJson()
    {
        string json = "{\"id\":" + NimbyAdminServerRules.JsonString(id);
        json = json + ",\"phase\":" + NimbyAdminServerRules.JsonString(phase);
        json = json + ",\"ts\":" + NimbyAdminServerRules.JsonString(ts);
        json = json + ",\"actor\":{\"steam64\":" + NimbyAdminServerRules.JsonString(actorSteam64);
        json = json + ",\"biId\":" + NimbyAdminServerRules.JsonString(actorBiId);
        json = json + ",\"name\":" + NimbyAdminServerRules.JsonString(actorName) + "}";
        json = json + ",\"command\":" + NimbyAdminServerRules.JsonString(command);
        if (target != "")
        {
            json = json + ",\"target\":" + target;
        }
        if (args != "")
        {
            json = json + ",\"args\":" + args;
        }
        if (phase == "result")
        {
            json = json + ",\"result\":" + NimbyAdminServerRules.JsonString(result);
        }
        if (effects != "")
        {
            json = json + ",\"effects\":" + effects;
        }
        if (reason != "")
        {
            json = json + ",\"reason\":" + NimbyAdminServerRules.JsonString(reason);
        }
        return json + "}";
    }
}

// A flat JSON object builder for the target, args and effects fields.
class NimbyAdminServerJsonObject
{
    protected string m_Body = "";

    void Add(string key, string value)
    {
        Raw(key, NimbyAdminServerRules.JsonString(value));
    }

    void AddInt(string key, int value)
    {
        Raw(key, value.ToString());
    }

    void AddFloat(string key, float value)
    {
        Raw(key, value.ToString());
    }

    void AddVector(string key, vector value)
    {
        Raw(key, "[" + value[0].ToString() + "," + value[1].ToString() + "," + value[2].ToString() + "]");
    }

    void Raw(string key, string json)
    {
        if (m_Body != "")
        {
            m_Body = m_Body + ",";
        }
        m_Body = m_Body + NimbyAdminServerRules.JsonString(key) + ":" + json;
    }

    string ToJson()
    {
        return "{" + m_Body + "}";
    }
}

class NimbyAdminServerAudit
{
    protected static bool s_Failing;

    static string NowUtc()
    {
        int year;
        int month;
        int day;
        int hour;
        int minute;
        int second;
        GetYearMonthDayUTC(year, month, day);
        GetHourMinuteSecondUTC(hour, minute, second);
        return NimbyAdminServerRules.FormatIsoUtc(year, month, day, hour, minute, second);
    }

    static string PathFor(string isoUtc)
    {
        return NimbyAdminServerRules.DIR_AUDIT + "\\audit-" + isoUtc.Substring(0, 10) + ".log";
    }

    // False when the record could not be appended; the caller blocks the state change.
    static bool Append(NimbyAdminServerAuditRecord record)
    {
        if (record.ts == "")
        {
            record.ts = NowUtc();
        }
        string path = PathFor(record.ts);
        FileHandle handle = OpenFile(path, FileMode.APPEND);
        if (handle == 0)
        {
            if (!s_Failing)
            {
                s_Failing = true;
                NimbyAdminLog.Info("audit unavailable: cannot open " + path + " for append");
            }
            return false;
        }
        string json = record.ToJson();
        FPrintln(handle, json);
        CloseFile(handle);
        string last;
        if (!ReadLastLine(path, last) || !IsLastRecord(last, json))
        {
            if (!s_Failing)
            {
                s_Failing = true;
                NimbyAdminLog.Info("audit unavailable: the record just appended could not be read back from " + path);
            }
            return false;
        }
        if (s_Failing)
        {
            s_Failing = false;
            NimbyAdminLog.Info("audit available again: " + path);
        }
        return true;
    }

    // The last non-empty line of the file; false when it cannot be opened or holds none.
    protected static bool ReadLastLine(string path, out string last)
    {
        last = "";
        FileHandle handle = OpenFile(path, FileMode.READ);
        if (handle == 0)
        {
            return false;
        }
        bool any = false;
        string line = "";
        while (FGets(handle, line) >= 0)
        {
            string bare = StripLineEnd(line);
            if (bare != "")
            {
                last = bare;
                any = true;
            }
            line = "";
        }
        CloseFile(handle);
        return any;
    }

    // Engine-free (self-tested): the line without trailing CR and LF bytes.
    static string StripLineEnd(string line)
    {
        int end = line.Length();
        while (end > 0)
        {
            string ch = line.Get(end - 1);
            if (ch != "\r" && ch != "\n")
            {
                break;
            }
            end = end - 1;
        }
        return line.Substring(0, end);
    }

    // Engine-free (self-tested, finding 4): true only when the line read back is the record.
    static bool IsLastRecord(string lastLine, string expected)
    {
        if (expected == "")
        {
            return false;
        }
        return StripLineEnd(lastLine) == expected;
    }
}
