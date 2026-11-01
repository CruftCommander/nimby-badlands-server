// -----------------------------------------------------------------------------
// NimbyAdminServer - the audit journal (Plan 3 v0.8, audit record table; recon 6).
// Purpose : One JSON object per line, in two files per UTC day under $profile:NimbyAdmin\audit\
//           (Plan 3 v0.9, re-review finding 13, November 1, 2026). audit-<YYYY-MM-DD>.log holds
//           the intent and result records of commands that reached their handler (and the
//           system records of the bootstrap). audit-denied-<YYYY-MM-DD>.log holds the one result
//           record of a denied or invalid request (plan line 3) and the audit.suppressed
//           summaries. Same JSON shape in both. A command writes "intent" first and runs only
//           when that append succeeded; "result" follows. One OpenFile per record (APPEND,
//           FPrintln, CloseFile), so a crash loses at most the record being written.
// Verify  : FPrintln and CloseFile return nothing (EnSystem.c:481, :443), so after an intent
//           record the day file is opened again and its last line compared with the record just
//           written; Append is true only on a match (review finding 4, October 31, 2026). One
//           extra read per intent, at admin-command rates; result records are not read back
//           (they gate nothing). The readback reads the whole command file (1.30 has no seek or
//           size call), which only executed commands grow: denied and invalid records, which a
//           non-admin can drive, go to the denied file and never lengthen the readback.
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
    // Not serialized: a denied or invalid request's record, or a suppression summary, goes to the
    // denied file (StreamFor).
    bool denied;

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
    static const string STREAM_COMMAND = "command";
    static const string STREAM_DENIED = "denied";

    protected static bool s_FailingCommand;
    protected static bool s_FailingDenied;

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

    // Re-review finding 13: the file-selection rule. An intent record always goes to the command
    // file, so the readback only ever reads that file; a record marked denied goes to the denied
    // file; everything else (the result of an executed command, a bootstrap record) to the
    // command file.
    static string StreamFor(NimbyAdminServerAuditRecord record)
    {
        if (record.phase == "intent")
        {
            return STREAM_COMMAND;
        }
        if (record.denied)
        {
            return STREAM_DENIED;
        }
        return STREAM_COMMAND;
    }

    static string PathFor(string isoUtc, string stream)
    {
        string name = "\\audit-";
        if (stream == STREAM_DENIED)
        {
            name = "\\audit-denied-";
        }
        return NimbyAdminServerRules.DIR_AUDIT + name + isoUtc.Substring(0, 10) + ".log";
    }

    // False when the record could not be appended; the caller blocks the state change. Only an
    // intent record is read back (finding 4): it is the one append whose result gates a state
    // change, and only an authorized admin can cause one, at the sender rate. A result record's
    // return value gates nothing, so it is not read back; this keeps the readback, which reads
    // the whole day file (1.30 has no seek or size call, EnSystem.c:397-531), off the paths a
    // non-admin can drive (security review of 1180702).
    static bool Append(NimbyAdminServerAuditRecord record)
    {
        if (record.ts == "")
        {
            record.ts = NowUtc();
        }
        string stream = StreamFor(record);
        string path = PathFor(record.ts, stream);
        string why = WriteLine(path, record.ToJson(), record.phase == "intent");
        bool wasFailing = s_FailingCommand;
        if (stream == STREAM_DENIED)
        {
            wasFailing = s_FailingDenied;
        }
        if (why != "")
        {
            if (!wasFailing)
            {
                NimbyAdminLog.Info("audit unavailable: " + why);
            }
            SetFailing(stream, true);
            return false;
        }
        if (wasFailing)
        {
            NimbyAdminLog.Info("audit available again: " + path);
        }
        SetFailing(stream, false);
        return true;
    }

    // One unavailable / available-again line pair per file, so a failure of one file does not
    // hide or repeat the other's.
    protected static void SetFailing(string stream, bool failing)
    {
        if (stream == STREAM_DENIED)
        {
            s_FailingDenied = failing;
            return;
        }
        s_FailingCommand = failing;
    }

    // Appends one line; with verify, re-opens the file and requires its last line to be that
    // line. "" on success, else why (names the path, never the content). Also the self-test's
    // engine case, on a scratch file.
    static string WriteLine(string path, string json, bool verify)
    {
        FileHandle handle = OpenFile(path, FileMode.APPEND);
        if (handle == 0)
        {
            return "cannot open " + path + " for append";
        }
        FPrintln(handle, json);
        CloseFile(handle);
        if (!verify)
        {
            return "";
        }
        string last;
        if (!ReadLastLine(path, last) || !IsLastRecord(last, json))
        {
            return "the record just appended could not be read back from " + path;
        }
        return "";
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
