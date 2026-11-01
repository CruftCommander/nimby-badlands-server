// -----------------------------------------------------------------------------
// NimbyAdmin - self-test, 3_Game half (shared rules 8). Engine-free cases for the wire definition:
// the envelope, the args codec at every bound (recon question 6), the push-kind seam (N9), the
// request-id counter, roles, and the log gate. The 5_Mission half (the page registry) is in
// nimbyadmin_missionbase.c. Lines: [NIMBY][Admin][SelfTest] PASS|FAIL <case>.
// Style : one statement per line.
// -----------------------------------------------------------------------------

class NimbyAdminSelfTest
{
    // Returns the number of failures; the caller prints one PASS|FAIL line per result.
    static int Run(out array<string> results)
    {
        results = new array<string>;
        int failures = 0;
        failures += StubCases(results);
        failures += EnvelopeCases(results);
        failures += ArgsCases(results);
        failures += SeamCases(results);
        return failures;
    }

    protected static int StubCases(array<string> results)
    {
        int failures = 0;
        failures += Case(results, "rules: 6550301 is a command id", NimbyAdminRules.IsCommandRpc(6550301));
        failures += Case(results, "rules: 6550350 is reserved for observation (N9)", NimbyAdminRules.IsCommandRpc(6550350) == false);
        failures += Case(results, "rules: 6550400 is outside the block", NimbyAdminRules.IsCommandRpc(6550400) == false);
        failures += Case(results, "rules: selftest flag lives under $profile:NimbyAdmin", NimbyAdminRules.FILE_SELFTEST.IndexOf(NimbyAdminRules.DIR) == 0);
        failures += Case(results, "rules: every assigned RPC id is a command id", NimbyAdminRules.IsCommandRpc(NimbyAdminRules.RPC_REQUEST) && NimbyAdminRules.IsCommandRpc(NimbyAdminRules.RPC_DONE));
        return failures;
    }

    protected static int EnvelopeCases(array<string> results)
    {
        int failures = 0;
        string args900 = NimbySharedChunks.FillBytes("a", 900);
        string args901 = NimbySharedChunks.FillBytes("a", 901);
        failures += Case(results, "envelope: a good request passes", NimbyAdminRules.CheckEnvelope(1, 7, "player.teleportTo", "target=x") == "");
        failures += Case(results, "envelope: protocol 2 refused", NimbyAdminRules.CheckEnvelope(2, 7, "session.hello", "") == "protocol");
        failures += Case(results, "envelope: protocol 0 refused", NimbyAdminRules.CheckEnvelope(0, 7, "session.hello", "") == "protocol");
        failures += Case(results, "envelope: request id 0 refused", NimbyAdminRules.CheckEnvelope(1, 0, "session.hello", "") == "request id");
        failures += Case(results, "envelope: negative request id refused", NimbyAdminRules.CheckEnvelope(1, -3, "session.hello", "") == "request id");
        failures += Case(results, "envelope: empty command refused", NimbyAdminRules.CheckEnvelope(1, 7, "", "") == "command");
        failures += Case(results, "envelope: command of 32 bytes passes", NimbyAdminRules.CheckEnvelope(1, 7, NimbySharedChunks.FillBytes("c", 32), "") == "");
        failures += Case(results, "envelope: command of 33 bytes refused", NimbyAdminRules.CheckEnvelope(1, 7, NimbySharedChunks.FillBytes("c", 33), "") == "command");
        failures += Case(results, "envelope: args of 900 bytes pass", NimbyAdminRules.CheckEnvelope(1, 7, "session.hello", args900) == "");
        failures += Case(results, "envelope: args of 901 bytes refused", NimbyAdminRules.CheckEnvelope(1, 7, "session.hello", args901) == "args too long");
        failures += Case(results, "roles: owner and admin known, others not", NimbyAdminRules.IsKnownRole("owner") && NimbyAdminRules.IsKnownRole("admin") && !NimbyAdminRules.IsKnownRole("Owner") && !NimbyAdminRules.IsKnownRole("none"));
        failures += Case(results, "rules: CutBytes keeps a short text", NimbyAdminRules.CutBytes("abc", 10) == "abc");
        failures += Case(results, "rules: CutBytes never splits a two-byte character", NimbyAdminRules.CutBytes("aд", 2) == "a");
        return failures;
    }

    // Parse returns true or false; why is compared when given.
    protected static bool ParseIs(string text, bool expected, string expectedWhy)
    {
        NimbyAdminArgs args;
        string why;
        bool ok = NimbyAdminArgs.Parse(text, args, why);
        if (ok != expected)
        {
            return false;
        }
        if (expectedWhy != "" && why != expectedWhy)
        {
            return false;
        }
        return true;
    }

    protected static int ArgsCases(array<string> results)
    {
        int failures = 0;
        failures += Case(results, "args: empty text is zero pairs", ParseIs("", true, ""));
        NimbyAdminArgs one;
        string whyOne;
        bool okOne = NimbyAdminArgs.Parse("target=abc;session=4", one, whyOne);
        failures += Case(results, "args: two pairs parse in order", okOne && one.Count() == 2 && one.Get("target") == "abc" && one.Get("session") == "4");
        NimbyAdminArgs built = new NimbyAdminArgs();
        built.Set("target", "h8o5+/AB=");
        built.Set("note", "50%;off=yes");
        NimbyAdminArgs back;
        string whyBack;
        bool okBack = NimbyAdminArgs.Parse(built.Encode(), back, whyBack);
        failures += Case(results, "args: escaped values round trip", okBack && back.Get("target") == "h8o5+/AB=" && back.Get("note") == "50%;off=yes");
        failures += Case(results, "args: encode escapes %, ; and =", NimbyAdminArgs.Escape("%;=") == "%25%3B%3D");
        failures += Case(results, "args: an empty value is allowed", ParseIs("k=", true, ""));
        failures += Case(results, "args: a pair without = refused", ParseIs("target", false, "bad pair"));
        failures += Case(results, "args: an empty key refused", ParseIs("=x", false, "bad pair"));
        failures += Case(results, "args: a trailing ; refused", ParseIs("a=1;", false, "bad pair"));
        failures += Case(results, "args: an uppercase key refused", ParseIs("Target=1", false, "bad key"));
        failures += Case(results, "args: a key of 32 bytes passes", ParseIs(NimbySharedChunks.FillBytes("k", 32) + "=1", true, ""));
        failures += Case(results, "args: a key of 33 bytes refused", ParseIs(NimbySharedChunks.FillBytes("k", 33) + "=1", false, "bad key"));
        failures += Case(results, "args: a value of 200 bytes passes", ParseIs("v=" + NimbySharedChunks.FillBytes("x", 200), true, ""));
        failures += Case(results, "args: a value of 201 bytes refused", ParseIs("v=" + NimbySharedChunks.FillBytes("x", 201), false, "value too long"));
        failures += Case(results, "args: an unknown escape refused", ParseIs("v=%41", false, "bad escape"));
        failures += Case(results, "args: a cut escape refused", ParseIs("v=%3", false, "bad escape"));
        failures += Case(results, "args: a control byte refused", ParseIs("v=a\nb", false, "control byte"));
        failures += Case(results, "args: a duplicate key refused", ParseIs("a=1;a=2", false, "duplicate key"));
        string sixteen = "";
        string seventeen = "";
        int i = 0;
        while (i < 17)
        {
            string pair = "k" + i.ToString() + "=1";
            if (i < 16)
            {
                if (i > 0)
                {
                    sixteen = sixteen + ";";
                }
                sixteen = sixteen + pair;
            }
            if (i > 0)
            {
                seventeen = seventeen + ";";
            }
            seventeen = seventeen + pair;
            i = i + 1;
        }
        failures += Case(results, "args: 16 pairs pass", ParseIs(sixteen, true, ""));
        failures += Case(results, "args: 17 pairs refused", ParseIs(seventeen, false, "too many args"));
        string args900 = "a=" + NimbySharedChunks.FillBytes("x", 176) + ";b=" + NimbySharedChunks.FillBytes("x", 176) + ";c=" + NimbySharedChunks.FillBytes("x", 176) + ";d=" + NimbySharedChunks.FillBytes("x", 176) + ";e=" + NimbySharedChunks.FillBytes("x", 182);
        string args901 = "a=" + NimbySharedChunks.FillBytes("x", 176) + ";b=" + NimbySharedChunks.FillBytes("x", 176) + ";c=" + NimbySharedChunks.FillBytes("x", 176) + ";d=" + NimbySharedChunks.FillBytes("x", 176) + ";e=" + NimbySharedChunks.FillBytes("x", 183);
        failures += Case(results, "args: five pairs of 900 bytes pass", args900.Length() == 900 && ParseIs(args900, true, ""));
        failures += Case(results, "args: the same pairs at 901 bytes refused", args901.Length() == 901 && ParseIs(args901, false, "args too long"));
        NimbyAdminArgs refused = new NimbyAdminArgs();
        failures += Case(results, "args: Set refuses a bad key, a duplicate and a control byte", refused.Set("ok", "1") && !refused.Set("ok", "2") && !refused.Set("Bad", "1") && !refused.Set("c", "x\ty"));
        return failures;
    }

    protected static int SeamCases(array<string> results)
    {
        int failures = 0;
        failures += Case(results, "push: role is a known kind", NimbyAdminPushKinds.IsKnown(NimbyAdminRules.PUSH_ROLE));
        failures += Case(results, "push: an unregistered kind is unknown", NimbyAdminPushKinds.IsKnown(99) == false);
        NimbyAdminPushKinds.Register(99);
        failures += Case(results, "push: a fake kind registers (N9 seam)", NimbyAdminPushKinds.IsKnown(99));
        NimbyAdminPushKinds.Unregister(99);
        failures += Case(results, "push: the fake kind unregisters", NimbyAdminPushKinds.IsKnown(99) == false);
        NimbyAdminPushKinds.Unregister(NimbyAdminRules.PUSH_ROLE);
        failures += Case(results, "push: role cannot be unregistered", NimbyAdminPushKinds.IsKnown(NimbyAdminRules.PUSH_ROLE));
        int first = NimbyAdminRequestIds.Next();
        int second = NimbyAdminRequestIds.Next();
        failures += Case(results, "ids: request ids rise and stay positive", second == first + 1 && first > 0);
        failures += Case(results, "log: the gate holds a second line inside the window", NimbyAdminLog.GateAllows(100.0, 105.0, 10.0) == false);
        failures += Case(results, "log: the gate allows a line after the window", NimbyAdminLog.GateAllows(100.0, 110.0, 10.0));
        failures += Case(results, "log: names are escaped after both ids", NimbyAdminLog.WhoFrom("a\"b", "BI", "7656") == "player=(BI, 7656) \"a\\\"b\"");
        return failures;
    }

    static int Case(array<string> results, string name, bool passed)
    {
        if (passed)
        {
            results.Insert("PASS " + name);
            return 0;
        }
        results.Insert("FAIL " + name);
        return 1;
    }
}
