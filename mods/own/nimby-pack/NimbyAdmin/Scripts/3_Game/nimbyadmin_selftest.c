// -----------------------------------------------------------------------------
// NimbyAdmin - self-test, 3_Game half (shared rules 8). Engine-free cases for the wire definition:
// the envelope, the args codec at every bound (recon question 6), the push-kind seam (N9), the
// request-id counter, roles, the log gate, and the selection rule (review finding 2). The
// 5_Mission half (the page registry) is in nimbyadmin_missionbase.c. Lines: [NIMBY][Admin][SelfTest] PASS|FAIL <case>.
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
        failures += SelectionCases(results);
        return failures;
    }

    // Up to four bytes; -1 leaves a place empty.
    protected static array<int> B(int b0, int b1, int b2, int b3)
    {
        array<int> bytes = new array<int>;
        array<int> given = new array<int>;
        given.Insert(b0);
        given.Insert(b1);
        given.Insert(b2);
        given.Insert(b3);
        foreach (int value : given)
        {
            if (value >= 0)
            {
                bytes.Insert(value);
            }
        }
        return bytes;
    }

    // The units NextUnit finds, in order: "?" for one not kept, "k<length>" for one kept.
    protected static string Units(array<int> bytes)
    {
        string verdict = "";
        int i = 0;
        while (i < bytes.Count())
        {
            bool keep;
            int unit = NimbyAdminLog.NextUnit(bytes, i, keep);
            if (keep)
            {
                verdict = verdict + "k" + unit.ToString();
            }
            else
            {
                verdict = verdict + "?";
            }
            i = i + unit;
        }
        return verdict;
    }

    // Phase 1 A4: B then Units with a local in between, so no call sits in another's arguments.
    protected static string UnitsOf(int b0, int b1, int b2, int b3)
    {
        array<int> bytes = B(b0, b1, b2, b3);
        string verdict = Units(bytes);
        return verdict;
    }

    protected static NimbyAdminPlayerRow Row(string biId, int session)
    {
        NimbyAdminPlayerRow row = new NimbyAdminPlayerRow();
        row.biId = biId;
        row.session = session;
        row.name = biId;
        return row;
    }

    // Review finding 2: the selection survives a refresh only for the same BI id and session.
    protected static int SelectionCases(array<string> results)
    {
        int failures = 0;
        NimbyAdminPlayerList list = new NimbyAdminPlayerList();
        // Phase 1 A4: every value is a local before the call (the 164200 argument-slot rule).
        NimbyAdminPlayerRow listA = Row("A", 1);
        NimbyAdminPlayerRow listB = Row("B", 7);
        list.players.Insert(listA);
        list.players.Insert(listB);
        string notice;
        int kept = NimbyAdminSelection.Rebind(list, "B", 7, notice);
        failures += Case(results, "selection: the same BI id and session keep the selection", kept == 1 && notice == "");
        NimbyAdminPlayerList after = new NimbyAdminPlayerList();
        NimbyAdminPlayerRow afterA = Row("A", 1);
        NimbyAdminPlayerRow afterB = Row("B", 9);
        after.players.Insert(afterA);
        after.players.Insert(afterB);
        int moved = NimbyAdminSelection.Rebind(after, "B", 7, notice);
        failures += Case(results, "selection: a reconnected player (same BI id, new session) clears it", moved == -1 && notice == NimbyAdminSelection.NOTICE_RECONNECTED);
        NimbyAdminPlayerList gone = new NimbyAdminPlayerList();
        NimbyAdminPlayerRow goneA = Row("A", 1);
        gone.players.Insert(goneA);
        int left = NimbyAdminSelection.Rebind(gone, "B", 7, notice);
        failures += Case(results, "selection: a player who left clears it", left == -1 && notice == NimbyAdminSelection.NOTICE_LEFT);
        int none = NimbyAdminSelection.Rebind(list, "", 0, notice);
        failures += Case(results, "selection: no selection stays none, with no notice", none == -1 && notice == "");
        return failures;
    }

    protected static int StubCases(array<string> results)
    {
        int failures = 0;
        bool pass115 = NimbyAdminRules.IsCommandRpc(6550301);
        failures += Case(results, "rules: 6550301 is a command id", pass115);
        bool pass116 = NimbyAdminRules.IsCommandRpc(6550350) == false;
        failures += Case(results, "rules: 6550350 is reserved for observation (N9)", pass116);
        bool pass117 = NimbyAdminRules.IsCommandRpc(6550400) == false;
        failures += Case(results, "rules: 6550400 is outside the block", pass117);
        bool pass118 = NimbyAdminRules.FILE_SELFTEST.IndexOf(NimbyAdminRules.DIR) == 0;
        failures += Case(results, "rules: selftest flag lives under $profile:NimbyAdmin", pass118);
        bool pass119 = NimbyAdminRules.IsCommandRpc(NimbyAdminRules.RPC_REQUEST) && NimbyAdminRules.IsCommandRpc(NimbyAdminRules.RPC_DONE);
        failures += Case(results, "rules: every assigned RPC id is a command id", pass119);
        return failures;
    }

    protected static int EnvelopeCases(array<string> results)
    {
        int failures = 0;
        string args900 = NimbySharedChunks.FillBytes("a", 900);
        string args901 = NimbySharedChunks.FillBytes("a", 901);
        bool pass128 = NimbyAdminRules.CheckEnvelope(1, 7, "player.teleportTo", "target=x") == "";
        failures += Case(results, "envelope: a good request passes", pass128);
        bool pass129 = NimbyAdminRules.CheckEnvelope(2, 7, "session.hello", "") == "protocol";
        failures += Case(results, "envelope: protocol 2 refused", pass129);
        bool pass130 = NimbyAdminRules.CheckEnvelope(0, 7, "session.hello", "") == "protocol";
        failures += Case(results, "envelope: protocol 0 refused", pass130);
        bool pass131 = NimbyAdminRules.CheckEnvelope(1, 0, "session.hello", "") == "request id";
        failures += Case(results, "envelope: request id 0 refused", pass131);
        bool pass132 = NimbyAdminRules.CheckEnvelope(1, -3, "session.hello", "") == "request id";
        failures += Case(results, "envelope: negative request id refused", pass132);
        bool pass133 = NimbyAdminRules.CheckEnvelope(1, 7, "", "") == "command";
        failures += Case(results, "envelope: empty command refused", pass133);
        string command32 = NimbySharedChunks.FillBytes("c", 32);
        string command33 = NimbySharedChunks.FillBytes("c", 33);
        string envelope32 = NimbyAdminRules.CheckEnvelope(1, 7, command32, "");
        string envelope33 = NimbyAdminRules.CheckEnvelope(1, 7, command33, "");
        bool ok120 = envelope32 == "";
        bool ok121 = envelope33 == "command";
        failures += Case(results, "envelope: command of 32 bytes passes", ok120);
        failures += Case(results, "envelope: command of 33 bytes refused", ok121);
        bool pass142 = NimbyAdminRules.CheckEnvelope(1, 7, "session.hello", args900) == "";
        failures += Case(results, "envelope: args of 900 bytes pass", pass142);
        bool pass143 = NimbyAdminRules.CheckEnvelope(1, 7, "session.hello", args901) == "args too long";
        failures += Case(results, "envelope: args of 901 bytes refused", pass143);
        bool pass144 = NimbyAdminRules.IsKnownRole("owner") && NimbyAdminRules.IsKnownRole("admin") && !NimbyAdminRules.IsKnownRole("Owner") && !NimbyAdminRules.IsKnownRole("none");
        failures += Case(results, "roles: owner and admin known, others not", pass144);
        bool pass145 = NimbyAdminRules.CutBytes("abc", 10) == "abc";
        failures += Case(results, "rules: CutBytes keeps a short text", pass145);
        bool pass146 = NimbyAdminRules.CutBytes("aд", 2) == "a";
        failures += Case(results, "rules: CutBytes never splits a two-byte character", pass146);
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
        bool pass170 = ParseIs("", true, "");
        failures += Case(results, "args: empty text is zero pairs", pass170);
        NimbyAdminArgs one;
        string whyOne;
        bool okOne = NimbyAdminArgs.Parse("target=abc;session=4", one, whyOne);
        bool pass174 = okOne && one.Count() == 2 && one.Get("target") == "abc" && one.Get("session") == "4";
        failures += Case(results, "args: two pairs parse in order", pass174);
        NimbyAdminArgs built = new NimbyAdminArgs();
        built.Set("target", "h8o5+/AB=");
        built.Set("note", "50%;off=yes");
        NimbyAdminArgs back;
        string whyBack;
        string builtText = built.Encode();
        bool okBack = NimbyAdminArgs.Parse(builtText, back, whyBack);
        bool pass182 = okBack && back.Get("target") == "h8o5+/AB=" && back.Get("note") == "50%;off=yes";
        failures += Case(results, "args: escaped values round trip", pass182);
        bool pass183 = NimbyAdminArgs.Escape("%;=") == "%25%3B%3D";
        failures += Case(results, "args: encode escapes %, ; and =", pass183);
        bool pass184 = ParseIs("k=", true, "");
        failures += Case(results, "args: an empty value is allowed", pass184);
        bool pass185 = ParseIs("target", false, "bad pair");
        failures += Case(results, "args: a pair without = refused", pass185);
        bool pass186 = ParseIs("=x", false, "bad pair");
        failures += Case(results, "args: an empty key refused", pass186);
        bool pass187 = ParseIs("a=1;", false, "bad pair");
        failures += Case(results, "args: a trailing ; refused", pass187);
        bool pass188 = ParseIs("Target=1", false, "bad key");
        failures += Case(results, "args: an uppercase key refused", pass188);
        string key32 = NimbySharedChunks.FillBytes("k", 32) + "=1";
        string key33 = NimbySharedChunks.FillBytes("k", 33) + "=1";
        string value200 = "v=" + NimbySharedChunks.FillBytes("x", 200);
        string value201 = "v=" + NimbySharedChunks.FillBytes("x", 201);
        bool ok168 = ParseIs(key32, true, "");
        bool ok169 = ParseIs(key33, false, "bad key");
        bool ok170 = ParseIs(value200, true, "");
        bool ok171 = ParseIs(value201, false, "value too long");
        failures += Case(results, "args: a key of 32 bytes passes", ok168);
        failures += Case(results, "args: a key of 33 bytes refused", ok169);
        failures += Case(results, "args: a value of 200 bytes passes", ok170);
        failures += Case(results, "args: a value of 201 bytes refused", ok171);
        bool pass201 = ParseIs("v=%41", false, "bad escape");
        failures += Case(results, "args: an unknown escape refused", pass201);
        bool pass202 = ParseIs("v=%3", false, "bad escape");
        failures += Case(results, "args: a cut escape refused", pass202);
        bool pass203 = ParseIs("target=abc=;session=4", false, "unescaped =") && ParseIs("v==", false, "unescaped =");
        failures += Case(results, "args: a raw = inside a value refused (finding 11)", pass203);
        NimbyAdminArgs escapedEq;
        string whyEq;
        bool okEq = NimbyAdminArgs.Parse("target=abc%3D;session=4", escapedEq, whyEq);
        bool pass207 = okEq && escapedEq.Get("target") == "abc=" && escapedEq.Get("session") == "4";
        failures += Case(results, "args: %3D inside a value still decodes to =", pass207);
        bool pass208 = ParseIs("v=a\nb", false, "control byte");
        failures += Case(results, "args: a control byte refused", pass208);
        bool pass209 = ParseIs("a=1;a=2", false, "duplicate key");
        failures += Case(results, "args: a duplicate key refused", pass209);
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
        bool pass231 = ParseIs(sixteen, true, "");
        failures += Case(results, "args: 16 pairs pass", pass231);
        bool pass232 = ParseIs(seventeen, false, "too many args");
        failures += Case(results, "args: 17 pairs refused", pass232);
        string args900 = "a=" + NimbySharedChunks.FillBytes("x", 176) + ";b=" + NimbySharedChunks.FillBytes("x", 176) + ";c=" + NimbySharedChunks.FillBytes("x", 176) + ";d=" + NimbySharedChunks.FillBytes("x", 176) + ";e=" + NimbySharedChunks.FillBytes("x", 182);
        string args901 = "a=" + NimbySharedChunks.FillBytes("x", 176) + ";b=" + NimbySharedChunks.FillBytes("x", 176) + ";c=" + NimbySharedChunks.FillBytes("x", 176) + ";d=" + NimbySharedChunks.FillBytes("x", 176) + ";e=" + NimbySharedChunks.FillBytes("x", 183);
        bool pass235 = args900.Length() == 900 && ParseIs(args900, true, "");
        failures += Case(results, "args: five pairs of 900 bytes pass", pass235);
        bool pass236 = args901.Length() == 901 && ParseIs(args901, false, "args too long");
        failures += Case(results, "args: the same pairs at 901 bytes refused", pass236);
        NimbyAdminArgs refused = new NimbyAdminArgs();
        bool pass238 = refused.Set("ok", "1") && !refused.Set("ok", "2") && !refused.Set("Bad", "1") && !refused.Set("c", "x\ty");
        failures += Case(results, "args: Set refuses a bad key, a duplicate and a control byte", pass238);
        return failures;
    }

    protected static int SeamCases(array<string> results)
    {
        int failures = 0;
        bool pass245 = NimbyAdminPushKinds.IsKnown(NimbyAdminRules.PUSH_ROLE);
        failures += Case(results, "push: role is a known kind", pass245);
        bool pass246 = NimbyAdminPushKinds.IsKnown(99) == false;
        failures += Case(results, "push: an unregistered kind is unknown", pass246);
        NimbyAdminPushKinds.Register(99);
        bool pass248 = NimbyAdminPushKinds.IsKnown(99);
        failures += Case(results, "push: a fake kind registers (N9 seam)", pass248);
        NimbyAdminPushKinds.Unregister(99);
        bool pass250 = NimbyAdminPushKinds.IsKnown(99) == false;
        failures += Case(results, "push: the fake kind unregisters", pass250);
        NimbyAdminPushKinds.Unregister(NimbyAdminRules.PUSH_ROLE);
        bool pass252 = NimbyAdminPushKinds.IsKnown(NimbyAdminRules.PUSH_ROLE);
        failures += Case(results, "push: role cannot be unregistered", pass252);
        int first = NimbyAdminRequestIds.Next();
        int second = NimbyAdminRequestIds.Next();
        failures += Case(results, "ids: request ids rise and stay positive", second == first + 1 && first > 0);
        bool pass256 = NimbyAdminLog.GateAllows(100.0, 105.0, 10.0) == false;
        failures += Case(results, "log: the gate holds a second line inside the window", pass256);
        bool pass257 = NimbyAdminLog.GateAllows(100.0, 110.0, 10.0);
        failures += Case(results, "log: the gate allows a line after the window", pass257);
        bool pass258 = NimbyAdminLog.WhoFrom("a\"b", "BI", "7656") == "player=(BI, 7656) \"a\\\"b\"";
        failures += Case(results, "log: names are escaped after both ids", pass258);
        bool pass259 = NimbyAdminLog.Safe("a\nb\rc") == "a?b?c";
        failures += Case(results, "log: Safe replaces line breaks with ? (finding 7)", pass259);
        bool pass260 = NimbyAdminLog.IsUnsafeByte(0) && NimbyAdminLog.IsUnsafeByte(9) && NimbyAdminLog.IsUnsafeByte(31) && NimbyAdminLog.IsUnsafeByte(127) && !NimbyAdminLog.IsUnsafeByte(32) && !NimbyAdminLog.IsUnsafeByte(126);
        failures += Case(results, "log: Safe treats every byte below 0x20 and 0x7F as unsafe, and keeps 0x20 to 0x7E", pass260);
        string hundred = NimbySharedChunks.FillBytes("x", 100);
        string safeHundred = NimbyAdminLog.Safe(hundred);
        int safeHundredBytes = safeHundred.Length();
        string safeCyrillic = NimbyAdminLog.Safe("aд\nb");
        bool ok232 = safeHundredBytes == 64 && safeCyrillic == "aд?b";
        failures += Case(results, "log: Safe caps at 64 bytes and keeps a two-byte character whole", ok232);
        bool pass267 = NimbyAdminLog.ByteAt("д", 0) == 0xD0 && NimbyAdminLog.ByteAt("д", 1) == 0xB4;
        failures += Case(results, "log: ByteAt reads a byte above 0x7F as 128 to 255 (engine)", pass267);
        string u234a = UnitsOf(0x85, -1, -1, -1);
        string u234b = UnitsOf(0xC1, 0x85, -1, -1);
        string u234c = UnitsOf(0xE0, 0x80, 0x8A, -1);
        bool ok234 = u234a == "?" && u234b == "??" && u234c == "???";
        failures += Case(results, "log: a lone 0x85, an overlong C1 85 and an overlong E0 80 8A are all ?", ok234);
        string u235a = UnitsOf(0xC2, 0x85, -1, -1);
        string u235b = UnitsOf(0xD8, 0x9C, -1, -1);
        string u235c = UnitsOf(0xE2, 0x80, 0xA8, -1);
        string u235d = UnitsOf(0xE2, 0x80, 0xA9, -1);
        string u235e = UnitsOf(0xE2, 0x80, 0xAE, -1);
        string u235f = UnitsOf(0xE2, 0x81, 0xA6, -1);
        string u235g = UnitsOf(0xE2, 0x80, 0x8B, -1);
        string u235h = UnitsOf(0xEF, 0xBB, 0xBF, -1);
        bool ok235 = u235a == "?" && u235b == "?" && u235c == "?" && u235d == "?" && u235e == "?" && u235f == "?" && u235g == "?" && u235h == "?";
        failures += Case(results, "log: NEL, ALM (D8 9C), LS, PS, RLO, an isolate, ZWSP and BOM are ?", ok235);
        string u236a = UnitsOf(0xED, 0xA0, 0x80, -1);
        string u236b = UnitsOf(0xFF, -1, -1, -1);
        string u236c = UnitsOf(0xE2, 0x80, -1, -1);
        bool ok236 = u236a == "???" && u236b == "?" && u236c == "??";
        failures += Case(results, "log: a surrogate, a byte above 0xF4 and a sequence cut short are ?", ok236);
        string u237a = UnitsOf(0xD0, 0xB4, -1, -1);
        string u237b = UnitsOf(0xE2, 0x80, 0x99, -1);
        string u237c = UnitsOf(0xC2, 0xA0, -1, -1);
        string u237d = UnitsOf(0xF0, 0x9F, 0x98, 0x80);
        string u237e = UnitsOf(0x41, -1, -1, -1);
        bool ok237 = u237a == "k2" && u237b == "k3" && u237c == "k2" && u237d == "k4" && u237e == "k1";
        failures += Case(results, "log: a Cyrillic letter, a right quote, a no-break space, an emoji and ASCII are kept whole", ok237);
        bool pass295 = NimbyAdminLog.WhoFrom("x\n[NIMBY][Admin] fake", "BI\n", "7656").IndexOf("\n") < 0;
        failures += Case(results, "log: a forged name cannot add a line to WhoFrom", pass295);
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
