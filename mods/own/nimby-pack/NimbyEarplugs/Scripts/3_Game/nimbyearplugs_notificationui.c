// -----------------------------------------------------------------------------
// NimbyEarplugs - vanilla NotificationUI leak fix.
// Purpose : NotificationUI.RemoveNotification (3_Game/DayZ/Client/Notifications/
//           NotificationUI.c:79) parks an expiring toast in m_WidgetTimers under the
//           key count + show_time. Two toasts with the same show_time that expire while
//           an earlier one is still fading share a key; map.Insert refuses the duplicate
//           and that widget is never faded or deleted, so it stays on screen for the
//           rest of the game session (seen on October 17, 2026 with the Ears toggle,
//           which fires 3 s toasts in quick succession). A serial key cannot collide.
// Scope   : Client UI only. The class exists in server scripts but is never created
//           there. Voice notifications keep their own player-name keys.
// Moves to a shared NIMBY base addon when a second NIMBY mod needs it.
// Style   : One statement per line (Enforce rejects continuation lines).
// Upkeep  : The body below is a copy of NotificationUI.RemoveNotification
//           (NotificationUI.c:74-83) with only the map key changed, and it does not
//           call super. Re-diff it against that method on every game update.
//           m_WidgetTimers is map<string, Widget> (NotificationUI.c:16), so the
//           string key is the vanilla type.
// -----------------------------------------------------------------------------

modded class NotificationUI
{
    protected int m_NimbyToastSerial;

    override void RemoveNotification(NotificationRuntimeData data)
    {
        if (!m_Notifications.Contains(data))
        {
            return;
        }
        Widget notification = m_Notifications.Get(data);
        m_NimbyToastSerial = m_NimbyToastSerial + 1;
        string key = "nimby toast #" + m_NimbyToastSerial.ToString();
        m_WidgetTimers.Insert(key, notification);
        m_Notifications.Remove(data);
        UpdateTargetHeight();
    }
}
