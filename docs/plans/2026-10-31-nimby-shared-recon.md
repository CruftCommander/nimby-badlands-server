# Plan 11 recon - NimbyShared move list and method set

Phase 0 record for [Plan 11](2026-10-31-nimby-shared-plan.md) (v0.1), October 31, 2026. Seat: Claude Code in worktree `D:\GitHub\nimby-badlands-server-wt\nimby-shared`, branch `feat/nimby-shared` at `8d08d2b` (the kickoff merge, stub present). Read-only recon: no addon code changed. Line numbers are against `8d08d2b`.

Sources read: Plan 11 whole; shared rules v1.7 sections 2.2, 3, 4, 7, 7.1, 8, 8.1; Greet plan v0.4 G5, G7, G13, G16; the Greet Phase 0 record (G-P0-31, G-P0-32); `mods/own/nimby-pack/NimbyGreet/` whole; the NimbyShared stub; `pack.json`; Plan 3 v0.7 Architecture and Phase 0; the headers of `Build-NimbyPack.ps1`, `Start-NimbyTestServer.ps1`, `Test-NimbyRepo.ps1`.

Vanilla API evidence used below (all under `vendor/dayz-1.30-scripts/scripts/`):

| API | File:line | Used by |
| --- | --- | --- |
| `UIScriptedMenu` base (`Init`, `OnShow`, `OnHide`, `Update`, `OnClick`, `SetID`) | `3_Game/DayZ/tools/UIScriptedMenu.c:66,146,163,173,184,195,238` | `NimbySharedWindow` |
| `UIScriptedMenu.Update` does work only under `PLATFORM_CONSOLE` | `3_Game/DayZ/tools/UIScriptedMenu.c:195-228` | Ordering of the subclass frame hook is free on PC |
| `Mission.AddActiveInputExcludes`, `RemoveActiveInputExcludes` | `3_Game/DayZ/gameplay.c:798,800` | Shell `OnShow` and `OnHide` (NIMBY-T-039) |
| `UAInput.LocalPress`, `ForceDisable`, `UAInputAPI.GetInputByName` | `3_Game/DayZ/InputAPI/UAInput.c:60,94,218` | Shell input handling, zeroing guard (NIMBY-T-058) |
| `Widget.TranslateString` | `1_Core/DayZ/proto/EnWidgets.c:112` | `NimbySharedWindow.Tr` |
| `WorkspaceWidget.CreateWidgets(layout, parent)` | `1_Core/DayZ/proto/EnWidgets.c:181` | Shell layout, content layout, section and pause buttons |
| `CGame.RPCSingleParam(target, rpc, param, guaranteed, recipient)` | `3_Game/DayZ/Global/Game.c:1013` | `NimbySharedChunkSender` |
| `InGameMenu.Init` builds `day_z_ingamemenu.layout`; hover paints `<name>_panel` and `<name>_label` | `5_Mission/DayZ/GUI/InGameMenu.c:36-78,149-159,365-415` | Pause-menu registry |

## 1. Move table

### 1.1. Window shell: `NimbyGreetMenu` (`Scripts/5_Mission/nimbygreet_menu.c`) to `NimbySharedWindow` (`NimbyShared/Scripts/5_Mission/nimbyshared_window.c`)

| Greet piece (lines) | Destination | What Greet keeps or calls instead |
| --- | --- | --- |
| `ID_CLOSE` (46), `ID_SECTION_BASE` (51) | `NimbySharedWindow.ID_CLOSE` = 1, `ID_SECTION_BASE` = 100, new `ID_SECTION_END` = 200 | Greet's ids 2 to 5 and 200 to 399 are unchanged, so every `userID` in the layouts is unchanged |
| `SECTION_TEXT_SIZE` (59), `COLOR_*` (70-76) | Base constants, same values | Greet's body sizes (55-58, 60-65) stay |
| Fields `m_AddedExclude`, `m_ZeroingDisabled`, `m_Selected` (87-89), `m_Title`, `m_Tagline`, `m_Hint`, `m_SectionScroll`, `m_SectionList`, `m_BodyScroll` (91-96), `m_SectionButtons` (106) | Base fields; new `m_TextScale` (float, 1.0) and `m_Content` (the content slot) | Greet keeps `m_BodyContent`, `m_LinkScroll`, `m_LinkRows`, the five footer buttons, and every content field |
| `Init` (206-236) | Base `Init`: load the shell layout, look up the chrome, load the subclass content layout into `Content`, look up `BodyScroll` by name, call `OnWindowInit`, `BuildChrome`, `BuildSections`, `SelectSection(0)`, focus `InitialFocus()` | Greet `OnWindowInit` holds the lookups of 220-227; `InitialFocus` returns `m_BtnClose` (231-234) |
| `OnShow` (238-249) | Base, unchanged | - |
| `SetZeroingDisabled` (261-276) and its call in `~NimbyGreetMenu` (256) | Base, plus `~NimbySharedWindow` releasing it | `~NimbyGreetMenu` keeps only `StopClips()` (255) |
| `OnHide` (278-298) | Base: zeroing release and exclude removal (282-292), then virtual `OnWindowClosed()` | Greet `OnWindowClosed`: `StopClips()` (281) and `gameplay.NimbyGreetOnMenuClosed(this)` (293-297) |
| `Update` (300-346) | Base: back, tab left and right, page up and down; page input names from virtuals | Greet `OnWindowUpdate` holds the table relayout (307-311); `PageUpInputName` and `PageDownInputName` return Greet's input names (see question 1) |
| `BuildChrome` (350-375), title, tagline, hint parts (352-364) | Base helpers `SetTitle(title, tagline)` and `SetHint(text)` | Greet `BuildChrome` (override) calls them, then its own button labels and `Refresh*Label` |
| `Tr` (465-477) | Base static; the "unresolved key" test becomes `text == key without its #`, in place of `StartsWith(text, "STR_NimbyGreet_")` (472) | Identical result for every Greet key (both tests catch an unresolved `STR_NimbyGreet_*`; no Greet translation starts with that prefix). Greet calls `NimbySharedWindow.Tr` |
| `SetButtonLabel` (479-490) | Base, unchanged | - |
| `BuildSections` (494-532) | Base; count and label from `SectionCount()` and `SectionLabel(i)`; layout `NimbyShared/GUI/nimbyshared_section_button.layout` | Greet supplies `m_Language.sections.Count()` and `.heading` |
| `SelectSection` (534-563) | Base: wrap, set `m_Selected`, paint, virtual `OnSectionSelected(index)`, scroll `BodyScroll` to 0 | Greet `OnSectionSelected`: `BuildBody`, `BuildLinks`, and its own debug line of 562, text unchanged |
| `PaintSectionButton` (565-594) | Base, unchanged | - |
| `Scaled` (1155-1158) | Base, reads `m_TextScale` | Greet `Setup` sets `m_TextScale = prefs.textScale` |
| `ApplyScale` section-label part (1172-1179) | Base `ApplyScale` (virtual) | Greet override: `super.ApplyScale()`, then body texts (1162-1171), link rows (1180-1187), `BuildBody`, `BuildLinks` |
| `Pressed` (1194-1202) | Base; returns false for an empty name | - |
| `PageBody` (1204-1219) | Base, on `m_BodyScroll` | - |
| `OnClick` close (1231-1239) and section (1266-1270) branches | Base `OnClick`; returns true when handled | Greet override: `if (super.OnClick(...)) return true;` then its own ids |
| `CycleTextSize` next-step logic (1298-1310) | `NimbySharedRules.NextTextScale(float)` (engine-free, self-tested) and base `CycleTextSize()`: set, `ApplyScale()`, virtual `OnTextScaleChanged()` | Greet `OnTextScaleChanged`: `m_Prefs.textScale`, `SavePrefs`, `RefreshTextSizeLabel`, the `pref textScale=` Info line (1311-1314, text unchanged) |
| `TEXT_SCALE_NORMAL`, `_LARGE`, `_LARGER` (`nimbygreet_rules.c:100-102`) | `NimbySharedRules` | `nimbygreet_clientfiles.c:104-112` (`NormalizeScale`) and `RefreshTextSizeLabel` (396-403) read `NimbySharedRules.*` |
| `OnMouseEnter`, `OnMouseLeave`, `OnFocus`, `OnFocusLost` (1336-1358) | Base, unchanged | - |
| `Highlight` (1361-1398) | Base; the `LinkRow_text` exception (1381-1384) becomes virtual `NormalPanelColor(Widget w)`, default `COLOR_PANEL_BUTTON` | Greet override returns `COLOR_PANEL_NORMAL` for `LinkRow_text` |
| `PaintButtonNormal` (1400-1403) | Base, unchanged | - |
| `SetID(NimbyGreetRules.MENU_ID)` (124) | Stays in the subclass constructor | `MENU_ID` 65511 stays Greet's |

Generic-looking but not generic, so it stays in Greet:

- The never-again control (`ID_NEVER`, `NeverLabel`, 377-390, `IsNeverChecked`): its meaning is Greet's G4 dismissal, written by `MissionGameplay.NimbyGreetOnMenuClosed` to `seen.json`.
- The language cycle (`CycleLanguage`, 1317-1334, `RefreshLanguageLabel`, `NimbyGreetLanguageName`): it needs `NimbyGreetDone.languages` and sends a Greet request.
- The close reason (`SetCloseReason`, `GetCloseReason`, `m_CloseReason`): its vocabulary (`player`, `hotkey`, `hit`, `unconscious`, `vanilla`, `death`) is Greet's bookkeeping. The base only passes a reason to the virtual `RequestClose(reason)`; Greet's override records it if none is set, then closes. Base default: `Close()`.
- The animations toggle and every clip method: media prefs, and `FlipClip` is the known 5_Mission `CallLater` use (shared rules 7.1) that must not enter NimbyShared.
- Link rows (G10) and the `LinkRow_text` panel colour: Greet's URL rules; the base keeps only a colour hook.
- The text-size label strings and `prefs.json`: the steps are the shell's, the persistence and labels are Greet's.
- The hint text: the base owns the widget; the string names the Greet key and stays a Greet string-table row.

### 1.2. Layouts

| Greet file (lines) | Destination | Note |
| --- | --- | --- |
| `GUI/nimbygreet_window.layout` 1-91 (Root, Dim, Panel, Title, Tagline, SectionScroll, SectionList) and 351-365 (Hint) | `NimbyShared/GUI/nimbyshared_window.layout`, plus one new `FrameWidgetClass Content` (position 0 0, size 1 1, relative) placed after Tagline and before SectionScroll | Later siblings sit above `Content`, so the section column keeps the pointer |
| `GUI/nimbygreet_window.layout` 92-350 (BodyBackground, BodyScroll, BodyContent, LinkScroll, LinkRows, BtnNever, BtnTextSize, BtnAnimations, BtnLanguage, BtnClose) | New Greet file `GUI/nimbygreet_content.layout`: one root `FrameWidgetClass` (position 0 0, size 1 1, relative) holding those widgets verbatim | The root frame is the size of `Panel`, so every child keeps its exact coordinates; this replaces the plan's WrapSpacer footer (refinement R1, section 5) |
| `GUI/nimbygreet_section_button.layout` | `NimbyShared/GUI/nimbyshared_section_button.layout`, verbatim | Widget names `SectionBtn*` unchanged |
| `GUI/nimbygreet_pause_button.layout` | `NimbyShared/GUI/nimbyshared_pause_button.layout`; widget names `nimbysharedbtn`, `_panel`, `_label`; label text `""` instead of `#STR_NimbyGreet_PauseButton` | Vanilla hover finds `<name>_panel` and `<name>_label` per button (`InGameMenu.c:403-415`), so one name serves every registered button. The label is set at `Init` from the registry, as Greet does today (`nimbygreet_ingamemenu.c:40`) |
| Block, rule, heading, media, table-row, link-row layouts | Stay | - |

### 1.3. String table

No row moves. Every row in `NimbyGreet/stringtable.csv` is Greet text, including `STR_NimbyGreet_PauseButton`, which Greet passes into the registry. NimbyShared shows no text of its own, so it ships no `stringtable.csv` (plan Phase 1 step 1 change, section 5 item 6).

### 1.4. Pause menu: `nimbygreet_ingamemenu.c` (whole file)

| Greet piece (lines) | Destination |
| --- | --- |
| `modded class InGameMenu`, `Init` button creation (18-43) | `NimbyShared/Scripts/5_Mission/nimbyshared_ingamemenu.c`, looping over the registry; the two Info lines (28, 34) move to `[NIMBY][Shared]` |
| `OnClick` match (45-63) | Same file; finds the entry for the clicked widget and calls `entry.OnClick()` |
| The handler body (49-59: debug line, `mission.Continue()`, `NimbyGreetSendRequest(true, "menu")`) | New Greet file `Scripts/5_Mission/nimbygreet_pauseentry.c`, class `NimbyGreetPauseEntry`; the debug line keeps its text and Greet prefix |
| Registration | One new line in `NimbyGreet MissionGameplay.OnInit`: `NimbySharedPauseButtons.Register(new NimbyGreetPauseEntry())` |

`nimbygreet_ingamemenu.c` is deleted.

### 1.5. Codec

| Greet piece (lines) | Destination | What Greet calls instead |
| --- | --- | --- |
| `NimbyGreetRules.CHUNK_BYTES` (`nimbygreet_rules.c:46`) | `NimbySharedRules.CHUNK_BYTES` (already in the stub) | `nimbygreet_missionserver.c:501-503` (the done-size check) reads `NimbySharedRules.CHUNK_BYTES` |
| `ChunkCount` (144-152), `MaxChunkCount` (154-157), `IsContinuationByte` (160-185), `ChunkEnd` (188-210), `SplitChunks` (213-229), `FillBytes` (245-262) | `NimbySharedChunks` in `NimbyShared/Scripts/3_Game/nimbyshared_chunks.c`; `MaxChunkCount(int maxPayloadBytes)` takes the cap | `FillBytes` gets its own loop, so `Repeat` (232-242) stays in Greet, where ten non-chunk self-test cases use it |
| `NimbyGreetFetch` (`nimbygreet_chunks.c:59-294`) | `NimbySharedChunkReceiver` in `nimbyshared_chunks.c` (section 3) | Deleted from Greet. `MissionGameplay` holds a receiver plus two Greet fields, `m_NimbyGreetFetchManual` and `m_NimbyGreetFetchDone` (the old `m_Manual`, `m_Done`; `m_Language` is never read) |
| `NimbyGreetDone` (`nimbygreet_chunks.c:22-57`) | Stays (C7: the done record is the consumer's type) | - |
| Send loop and done (`nimbygreet_missionserver.c:490-504`); zero-chunk dones (476, 486) | `NimbySharedChunkSender.Send` and `.SendDone` | Greet sets `done.totalBytes` and `done.hash` before serializing (today 497-498, after the loop; the RPC order on the wire is unchanged: chunks, then done) |
| `PlayerBase.NimbyGreetSendChunk`, `NimbyGreetSendDone` (`nimbygreet_playerbase.c:163-174`) | Deleted (the sender calls `RPCSingleParam` on the player object itself) | The request send (100-105) and the `OnRPC` receive path stay; the header's "the only file that touches the wire" is reworded |

`MissionGameplay` lines that change to the receiver: 32, 60, 186-193, 203-214, 219-232, 239-258, 263-264. Every log line in them keeps its text.

### 1.6. Self-test cases

Move to `[NIMBY][Shared][SelfTest]`, same case names (38 cases):

- `Phase0Cases` (`nimbygreet_selftest.c:33-37, 39-55`): the five `chunks:` cases, the seven `utf8:` cases, the three `fill:` cases, the four `split:` cases.
- `FetchCases`: `fetch: 2500 cyrillic bytes are 3 chunks` (293-296) and every receiver case (322-350, 356), driven with `readable`, `totalBytes`, `hash` instead of a `NimbyGreetDone`.
- Helpers `Rejoin` (440-448) and `ChunksAreWholeChars` (450-460).

Stay in Greet: the four `cap:` cases of `Phase0Cases`, `done: json round trip`, `done: empty object parses to defaults`, and `fetch: payload cap is 164 chunks`, re-pointed to `NimbySharedChunks.MaxChunkCount(NimbyGreetRules.MAX_PAYLOAD_BYTES)` and renamed `fetch: Greet payload cap is 164 chunks`. Greet goes from 125 to 87 cases.

New NimbyShared cases (about 8): `MaxChunkCount` for two caps; `NextTextScale` for each step; receiver refuses an unreadable done; a fake pause entry registers, is listed once after a second `Register` with the same id, and unregisters. The registry cases are 5_Mission, so the 5_Mission hook (`nimbyshared_missionbase.c`) appends them to the 3_Game results before it prints the completion line. Expected NimbyShared total: 3 stub + 38 moved + about 8 new.

### 1.7. Log lines that change prefix

From `[NIMBY][Greet]` to `[NIMBY][Shared]`, text unchanged, debug lines now behind `$profile:NimbyShared\debug`:

| Line | Level | From |
| --- | --- | --- |
| `layout failed to load: <path>` | Info | `nimbygreet_menu.c:211` |
| `input exclude added: menu` | Debug | `:246` |
| `zeroing disabled=<bool>` | Debug | `:275` |
| `input exclude removed: menu` | Debug | `:289` |
| `section button layout failed to load` | Info | `:515` |
| `pause menu: button column 'top' not found; ...` | Info | `nimbygreet_ingamemenu.c:28` |
| `pause menu: <layout> failed to load` | Info | `nimbygreet_ingamemenu.c:34` |

Every other Greet line keeps its prefix and text, including `section selected` (562), `pref textScale` (1314), and every `MissionGameplay` and `MissionServer` line.

## 2. `NimbySharedWindow` method set

```c
class NimbySharedWindow extends UIScriptedMenu   // 5_Mission
{
    static const int ID_CLOSE = 1;
    static const int ID_SECTION_BASE = 100;
    static const int ID_SECTION_END = 200;      // sections beyond 100 are not built; one Info line
    // SECTION_TEXT_SIZE and the seven COLOR_* constants, values unchanged

    protected float m_TextScale = 1.0;
    protected int m_Selected = -1;
    protected Widget m_Content;                 // the slot the content layout is created in
    protected ScrollWidget m_BodyScroll;        // "BodyScroll" in the content layout, or null
    // m_Title, m_Tagline, m_Hint, m_SectionScroll, m_SectionList, m_SectionButtons,
    // m_AddedExclude, m_ZeroingDisabled

    // Supplied by the subclass
    protected string ContentLayout();                 // "" = no content
    protected void OnWindowInit();                    // look up the content widgets
    protected void BuildChrome();                     // calls SetTitle and SetHint
    protected int SectionCount();
    protected string SectionLabel(int index);
    protected void OnSectionSelected(int index);      // build the body
    protected Widget InitialFocus();
    protected string PageUpInputName();               // "" = none
    protected string PageDownInputName();
    protected void OnWindowUpdate(float timeslice);   // per frame, before input handling
    protected void OnTextScaleChanged();              // persist and relabel
    protected int NormalPanelColor(Widget w);         // default COLOR_PANEL_BUTTON
    protected void OnWindowClosed();                  // from OnHide, after the guards are released
    void RequestClose(string reason);                 // default Close()

    // Provided
    override Widget Init(); override void OnShow(); override void OnHide();
    override void Update(float timeslice); override bool OnClick(Widget w, int x, int y, int button);
    override bool OnMouseEnter(...); OnMouseLeave(...); OnFocus(...); OnFocusLost(...);
    void ~NimbySharedWindow();                        // releases the zeroing guard
    void SelectSection(int index);
    protected void BuildSections();
    protected void SetTitle(string title, string tagline);
    protected void SetHint(string text);
    protected void SetButtonLabel(Widget button, string text);
    protected void PaintButtonNormal(Widget button);
    protected void Highlight(Widget w, bool on);
    protected void PaintSectionButton(int index, bool hover);
    protected int Scaled(int base);
    protected void ApplyScale();                      // virtual; base rescales the section labels
    protected void CycleTextSize();
    protected void PageBody(int direction);
    protected bool Pressed(UAInputAPI api, string name);
    static string Tr(string key, string fallback);
}
```

The base queues no `CallLater`, `Call`, or `CallByName` (shared rules 7.1). It names no addon.

Greet against it (`NimbyGreetMenu extends NimbySharedWindow`): `ContentLayout()` returns `NimbyGreet/GUI/nimbygreet_content.layout`; `OnWindowInit` looks up `BodyContent`, `LinkScroll`, `LinkRows`, and the five buttons; `BuildChrome` calls `SetTitle(m_Language.title, m_Language.tagline)`, `SetHint(Tr("#STR_NimbyGreet_Hint", ...))`, then its own labels; `SectionCount` and `SectionLabel` read `m_Language.sections`; `OnSectionSelected` builds the body and links and logs line 562; `InitialFocus` returns `m_BtnClose`; the page input names are `UANimbyGreetPageUp` and `UANimbyGreetPageDown`; `OnWindowUpdate` runs the table relayout; `OnTextScaleChanged` saves `prefs.json`; `NormalPanelColor` handles `LinkRow_text`; `OnWindowClosed` stops the clips and calls `NimbyGreetOnMenuClosed`; `RequestClose` records the reason. `OnClick` calls `super.OnClick` first. Everything else in section 1.1's "stays" column is unchanged.

Central Scrutinizer without a base change: its `ContentLayout()` returns its own layout under the same `Panel`-sized frame, holding a list on the left, a right detail pane (a `ScrollWidget` named `BodyScroll` gets paging for free, or any name with its own scroll), and its own footer with a status `TextWidget` beside its buttons. `SetHint` is a second status line if it wants one. The page registry (Plan 3 N9c) lives behind `SectionCount`, `SectionLabel`, and `OnSectionSelected`.

## 3. Codec API

```c
class NimbySharedChunks   // 3_Game, engine-free except string.Hash (EnString.c:223)
{
    static int ChunkCount(int bytes);                     // at CHUNK_BYTES
    static int MaxChunkCount(int maxPayloadBytes);
    static bool IsContinuationByte(string text, int index);
    static int ChunkEnd(string text, int start, int maxBytes);
    static int SplitChunks(string text, int maxBytes, out array<string> chunks);
    static string FillBytes(string unit, int bytes);      // self-test helper
}

class NimbySharedChunkReceiver   // 3_Game, client side
{
    void NimbySharedChunkReceiver(int maxPayloadBytes);   // the only cap; chunk size is CHUNK_BYTES
    void Begin(int requestId, float now);                 // Begin(0, 0) clears
    bool IsActive();
    bool AcceptChunk(int requestId, int index, string text, float now, out string why);
    bool AcceptDone(int requestId, int chunkCount, bool readable, int totalBytes, int hash, float now, out string why);
    bool IsRefused(); bool IsComplete(); string Assemble(); string Verify(string assembled); int ElapsedMs();
    // read-only for log lines: m_RequestId, m_ChunkCount, m_Received, m_ReceivedBytes, m_DoneSeen, m_RefusedWhy
}

class NimbySharedChunkSender   // 3_Game, server side
{
    // Sends every chunk, then the done; returns the chunk count. Param3<int, int, string> both.
    static int Send(Object target, PlayerIdentity recipient, int chunkRpc, int doneRpc, int requestId, string payload, string doneJson);
    static void SendDone(Object target, PlayerIdentity recipient, int doneRpc, int requestId, int chunkCount, string doneJson);
}
```

Every `why` string stays word for word (`other request`, `chunk above CHUNK_BYTES`, `payload above MAX_PAYLOAD_BYTES`, and the rest), so Greet's debug lines and the moved case assertions do not change. The wire format is unchanged: chunk `Param3<requestId, index, text>`, done `Param3<requestId, chunkCount, doneJson>`.

The sender takes an `Object` target as well as the identity, because `RPCSingleParam` addresses an object (`Game.c:1013`); Greet passes the player, as today.

## 4. Pause-menu registry (C6, refined)

```c
class NimbySharedPauseEntry   // 5_Mission; a consumer subclasses it
{
    string Id();
    string LabelKey();
    string LabelFallback();
    bool IsVisible();        // default true; Central Scrutinizer returns "has a role"
    void OnClick();
}

class NimbySharedPauseButtons   // 5_Mission
{
    static void Register(NimbySharedPauseEntry entry);   // an entry with the same Id replaces the old one
    static void Unregister(string id);
    static int Count();
    static NimbySharedPauseEntry Get(int index);
}
```

`modded class InGameMenu` (NimbyShared) creates one button per visible entry, in registration order, under the vanilla `top` column, from `nimbyshared_pause_button.layout`, labelled with `NimbySharedWindow.Tr(LabelKey(), LabelFallback())`; `OnClick` maps the widget back to its entry. Greet's "Server welcome" is `NimbyGreetPauseEntry` (`Id` `greet`, `LabelKey` `#STR_NimbyGreet_PauseButton`, `OnClick` the body of `nimbygreet_ingamemenu.c:49-59`), registered in `MissionGameplay.OnInit`.

An entry object instead of the plan's `(id, labelKey, fallback, handler)`: Enforce Script has no closures, so the handler is an object either way, and `IsVisible` gives Central Scrutinizer its role gate. The registry and every entry live in 5_Mission on purpose: a 3_Game static holding a 5_Mission object would outlive the mission module, the NIMBY-T-052 class of crash. Replace-by-id keeps the list correct whether or not 5_Mission statics reset with the module.

## 5. Risks, refinements, and plan lines to change

Refinements within C5 to C8 (the seat's latitude), recorded here and in Plan 11 v0.2 at Phase 1:

- R1. The content slot replaces the plan's WrapSpacer footer. The Greet footer is five absolutely placed buttons of unequal width (0.30, 0.15, 0.15, 0.15, 0.17 of the panel, 0.01 gaps), and `BtnAnimations` is hidden in release 1 (no media), leaving its slot empty. A WrapSpacer collapses hidden children and spaces by pixel margin, so `BtnLanguage` would move left and the gaps would change: a visible break of C4. A `Panel`-sized content frame keeps every coordinate, and it also gives Central Scrutinizer its detail pane and status line with no base change.
- R2. The pause registry takes an entry object (section 4).
- R3. The receiver takes one cap (`maxPayloadBytes`); chunk size is the codec's `CHUNK_BYTES`.

Risks the plan does not list:

1. Destructor chaining. The zeroing release on a menu destroyed without `OnHide` (NIMBY-T-058) moves to `~NimbySharedWindow`, and Greet keeps `~NimbyGreetMenu` for the clips. This relies on Enforce Script running the base destructor after the subclass destructor; the 1.30 scripts give no direct proof (`~UIScriptedMenu` is empty, `UIScriptedMenu.c:141`). Mitigation: SH-06's destroyed-without-`OnHide` case with the `[NIMBY][Shared][Debug] zeroing disabled=false` line proves it; if it fails, Greet's destructor calls the base's idempotent release.
2. The server-side sender and shared rules 8.1. The rule lets a server-only PBO call "the codec's engine-free split function", and Plan 11's diagram says "SplitChunks only", but Plan 3 v0.7 Architecture puts `NimbySharedChunkSender` (which calls `RPCSingleParam`) in `NimbyAdminServer`. One of the texts needs to change before Central Scrutinizer Phase 0; shared rules are orchestrator-owned (item 2 below).
3. SH-13 may end UNKNOWN, not FAIL. With `NimbyShared` absent but named in Greet's `requiredAddons[]`, the engine can stop at config load on a missing required addon before any script compiles, and no `bootCheck.failurePatterns` entry matches that message; the boot check would then time out. The run is `Build-NimbyPack.ps1 -Addons NimbyAutorun,NimbyHands,NimbyEarplugs,NimbyNotes,NimbyTack,NimbyGreet` (pack.json untouched). If it times out, the evidence line is the engine's missing-addon line from the RPT, and adding that pattern is an orchestrator change.
4. The parity rows miss the buttons whose paint and click now pass through the base: footer hover and focus on Never, Text, Language, and Close, the transparent `LinkRow_text` normal state, link Open and Copy, and the never-again tick written at close. Proposed rows SH-15 and SH-16 (item 4 below).
5. Two consumers, one debug flag. The moved lines (section 1.7) do not say which window wrote them once Central Scrutinizer exists. Proposed: append ` menu=<GetID()>` to the four moved shell lines (item 5 below; a deviation from C4's "text unchanged", so it is Jeff's call).
6. Page keys and shared rules 5. Central Scrutinizer's window will want Page Up and Page Down too, and rule 5 forbids two mods sharing a default binding. That forces a choice now (question 1), because moving the inputs later means a second NimbyShared branch and a second binding reset.

Plan lines to change (numbered for the orchestrator):

1. Plan 11 Architecture table: the layout names read `nimbycommon_*`, left over from the NimbyCommon rename; they become `nimbyshared_*`, and the footer clause becomes the content slot (R1).
2. Shared rules 8.1 and Plan 11's diagram: "A server-only PBO MAY call the codec's engine-free split function" becomes "MAY call `NimbySharedChunks` and `NimbySharedChunkSender`", or Plan 3 drops the server sender. Orchestrator-owned file.
3. Plan 11 Tests, evidence text that does not match the code: SH-05 and SH-06 lines are `[NIMBY][Shared][Debug] ...` and need `$profile:NimbyShared\debug` on the client; SH-11's line is `[NIMBY][Greet][Debug] payload complete id=N language=russian ... chunks=74 bytes=73543 ...` (there is no "hash ok" text; a mismatch prints `payload rejected`); SH-12's lines are `request id=M replaces incomplete id=N` and `chunk dropped id=N ... why=other request`. SH-12 also has no trigger in the current code: the Phase 0 race mode is gone, and the server's request cooldown (5 s default, 1 s minimum) answers a fast second request with `reason=cooldown`. Proposed: SH-12 passes on the moved receiver cases plus one live attempt with `requestCooldownSeconds` 1 and two Insert presses inside the roughly 1.5 s transfer, recorded either way.
4. Plan 11 Tests, new rows: SH-15 footer and link-row paint (normal, hover, focus) against the release-1 screenshots; SH-16 never-again tick then close writes `seen.json` (`close reason=player neverShow=true ... written=true`), and link Open and Copy log as before.
5. Plan 11 C4: allow the ` menu=<id>` suffix on the four moved shell lines (risk 5), or keep strict text parity.
6. Plan 11 Phase 1 step 1: NimbyShared ships no `stringtable.csv` (section 1.3). If question 1 moves the page inputs, it ships `Data/Inputs.xml` and an `inputs` line in `CfgMods`.

## 6. Questions

Asked one at a time, each with a recommendation.

1. Page Up and Page Down for every NIMBY window: (A) move the two inputs to NimbyShared as `UANimbySharedPageUp` and `UANimbySharedPageDown`, Controls label "NIMBY: page the window up/down", read by whichever shell window is open; same default keys, but a player who rebound Greet's page keys since October 26 loses that rebinding, and the Controls label changes. (B) Move the declaration to NimbyShared but keep the names `UANimbyGreetPageUp/Down`: bindings survive, the names lie. (C) Keep them in Greet; Central Scrutinizer declares its own pair on other default keys (rule 5). Recommended: A.
2. Debug-line suffix (risk 5, plan line 5): recommended yes.
3. SH-12 evidence (plan line 3): recommended the self-test cases plus one recorded live attempt.

## Status, October 31, 2026, 21:20 (WRAPPING)

Done:

- Phase 1 code on `feat/nimby-shared` (`19672f7`): NimbyShared body, Greet refactored onto it, no copy of a moved function in Greet.
- Plan 11 v0.2, Greet plan v0.5, Greet `NOTICE` (`80bd577`, path fix after it); Phase 1 record `docs/testing/2026-10-31-nimby-shared-phase1.md` (`d26889a`).
- Boot check `SUMMARY PASS` on `build\@NIMBY-20261031-210751` and `build\@NIMBY-20261031-210953` (NimbyShared 53 of 53, Greet 87 of 87, total 645, 0 `SCRIPT (E)`); `Test-NimbyRepo.ps1` `SUMMARY PASS failures=0`.
- SH-01 PASS; SH-13 PASS (loud FAIL); SH-14 PASS on the file set, wording question open; SH-12 self-test part PASS.

Running:

- Test server on port 2302 from `build\@NIMBY-20261031-210953`, profile `profiles\nimby-shared-p1` (Russian SH-11 fixture, both debug flags), `DayZServer_x64` PID 22660. Its input is held open by a Git Bash `sleep` whose MSYS pid is in `logs\p1-holder.pid`. To stop it gracefully (RCON `#shutdown`, then the summary): in Git Bash from the worktree root, `kill $(cat logs/p1-holder.pid)`.

Next:

1. Jeff runs the client rows SH-02 to SH-12 and SH-15, SH-16 against `build\@NIMBY-20261031-210953` with `$profile:NimbyShared\debug` and `$profile:NimbyGreet\debug` in the client profile, and pastes the lines and screenshots.
2. The seat records each row with its `.rpt` line in the Phase 1 record, stops the server as above, and runs `Read-NimbyTestRun.ps1` on the run.
3. When every row is green: draft the PR description in a "PR description" section of this file; the PR is Jeff's call.

Blocked: SH-14 evidence wording (record, "Open for the orchestrator").
