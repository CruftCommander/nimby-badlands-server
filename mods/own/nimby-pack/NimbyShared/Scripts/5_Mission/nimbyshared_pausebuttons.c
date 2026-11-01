// -----------------------------------------------------------------------------
// NimbyShared - pause-menu buttons (Plan 11 v0.2, C6 as built, R2).
// Purpose : Addons register an entry object at mission init; the one modded InGameMenu below
//           adds a button per visible entry to the vanilla pause menu (InGameMenu, layout
//           gui/layouts/day_z_ingamemenu.layout, InGameMenu.c:38) under the existing button
//           column (the "top" WrapSpacer), in registration order, from
//           NimbyShared/GUI/nimbyshared_pause_button.layout. That layout copies the shape of
//           optionsbtn, so the vanilla hover code (OnMouseEnter and OnMouseLeave, InGameMenu.c:
//           149-159; ButtonSetColor and ButtonSetTextColor find "<name>_panel" and "<name>_label",
//           :403-415) paints each button like the rest.
// Module  : The registry and every entry live in 5_Mission on purpose. A 3_Game static holding a
//           5_Mission object would outlive the mission module (NIMBY-T-052, shared rules 7.1).
//           Register replaces an entry with the same id, so the list stays right whether or not
//           5_Mission statics reset with the module.
// Origin  : nimbygreet_ingamemenu.c (Greet plan v0.4, G7), moved in Plan 11 Phase 1.
// Style   : One statement per line.
// -----------------------------------------------------------------------------

// A consumer subclasses this and registers an instance (Greet: NimbyGreetPauseEntry).
class NimbySharedPauseEntry
{
    string Id()
    {
        return "";
    }

    string LabelKey()
    {
        return "";
    }

    string LabelFallback()
    {
        return "";
    }

    // Read when the pause menu opens; Central Scrutinizer returns "the player has a role".
    bool IsVisible()
    {
        return true;
    }

    void OnClick()
    {
    }
}

class NimbySharedPauseButtons
{
    static const string LAYOUT = "NimbyShared/GUI/nimbyshared_pause_button.layout";

    protected static ref array<ref NimbySharedPauseEntry> s_Entries;

    protected static void Ensure()
    {
        if (!s_Entries)
        {
            s_Entries = new array<ref NimbySharedPauseEntry>;
        }
    }

    // An entry with the same id replaces the old one in place; an empty id is refused.
    static void Register(NimbySharedPauseEntry entry)
    {
        Ensure();
        if (!entry || entry.Id() == "")
        {
            return;
        }
        int at = IndexOf(entry.Id());
        if (at >= 0)
        {
            s_Entries.Set(at, entry);
            return;
        }
        s_Entries.Insert(entry);
    }

    static void Unregister(string id)
    {
        Ensure();
        int at = IndexOf(id);
        if (at >= 0)
        {
            s_Entries.Remove(at);
        }
    }

    static int Count()
    {
        Ensure();
        return s_Entries.Count();
    }

    static NimbySharedPauseEntry Get(int index)
    {
        Ensure();
        if (index < 0 || index >= s_Entries.Count())
        {
            return null;
        }
        return s_Entries.Get(index);
    }

    static int IndexOf(string id)
    {
        Ensure();
        int i = 0;
        while (i < s_Entries.Count())
        {
            NimbySharedPauseEntry entry = s_Entries.Get(i);
            if (entry && entry.Id() == id)
            {
                return i;
            }
            i = i + 1;
        }
        return -1;
    }
}

modded class InGameMenu extends UIScriptedMenu
{
    protected ref array<Widget> m_NimbySharedButtons;
    protected ref array<string> m_NimbySharedButtonIds;

    override Widget Init()
    {
        Widget root = super.Init();
        m_NimbySharedButtons = new array<Widget>;
        m_NimbySharedButtonIds = new array<string>;
        if (!root)
        {
            return root;
        }
        if (NimbySharedPauseButtons.Count() == 0)
        {
            return root;
        }
        Widget column = root.FindAnyWidget("top");
        if (!column)
        {
            NimbySharedLog.Info("pause menu: button column 'top' not found; no NIMBY pause buttons");
            return root;
        }
        int i = 0;
        while (i < NimbySharedPauseButtons.Count())
        {
            NimbySharedPauseEntry entry = NimbySharedPauseButtons.Get(i);
            i = i + 1;
            if (!entry || !entry.IsVisible())
            {
                continue;
            }
            Widget button = g_Game.GetWorkspace().CreateWidgets(NimbySharedPauseButtons.LAYOUT, column);
            if (!button)
            {
                NimbySharedLog.Info("pause menu: nimbyshared_pause_button.layout failed to load");
                return root;
            }
            TextWidget label = TextWidget.Cast(button.FindAnyWidget("nimbysharedbtn_label"));
            if (label)
            {
                label.SetText(NimbySharedWindow.Tr(entry.LabelKey(), entry.LabelFallback()));
            }
            m_NimbySharedButtons.Insert(button);
            m_NimbySharedButtonIds.Insert(entry.Id());
        }
        return root;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && m_NimbySharedButtons)
        {
            int at = m_NimbySharedButtons.Find(w);
            if (at >= 0)
            {
                int entryAt = NimbySharedPauseButtons.IndexOf(m_NimbySharedButtonIds.Get(at));
                NimbySharedPauseEntry entry = NimbySharedPauseButtons.Get(entryAt);
                if (entry)
                {
                    entry.OnClick();
                }
                return true;
            }
        }
        return super.OnClick(w, x, y, button);
    }
}
