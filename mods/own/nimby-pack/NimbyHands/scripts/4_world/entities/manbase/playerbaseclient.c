modded class PlayerBaseClient
{
    protected bool m_NimbyHandsLastPocketsOpen = false;

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();

        if ( !IsControlledPlayer() )
        {
            return;
        }

        if ( m_NimbyHandsPocketsOpen != m_NimbyHandsLastPocketsOpen )
        {
            m_NimbyHandsLastPocketsOpen = m_NimbyHandsPocketsOpen;

            if ( NimbyHandsGetPocketsOpen() )
            {
                NotificationSystem.AddNotificationExtended( NimbyHandsConstants.POCKET_NOTIFICATION_TIME, "#STR_NIMBY_HANDS_POCKETS_OPEN", NimbyHandsPocketDetail( "#STR_NIMBY_HANDS_POCKETS_OPEN_DETAIL" ), "set:dayz_gui image:icon_lock" );
            }
            else
            {
                NotificationSystem.AddNotificationExtended( NimbyHandsConstants.POCKET_NOTIFICATION_TIME, "#STR_NIMBY_HANDS_POCKETS_CLOSED", NimbyHandsPocketDetail( "#STR_NIMBY_HANDS_POCKETS_CLOSED_DETAIL" ), "set:dayz_gui image:icon_lock" );
            }
        }
    }

    // G2: the notification names the binding, because the toggle ships unbound and the player
    // has to find it in Options > Controls. InputUtils.GetButtonNameFromInput (InputUtils.c:17)
    // returns "" for an unbound input; Widget.TranslateString (EnWidgets.c:112) resolves the
    // string-table text so the two parts can be joined.
    protected string NimbyHandsPocketDetail( string detailKey )
    {
        string binding = "Controls > Others > \"Hands: open or close pockets\"";
        string key = InputUtils.GetButtonNameFromInput( "UANimbyHandsTogglePockets", EInputDeviceType.MOUSE_AND_KEYBOARD );
        if ( key != "" )
        {
            binding = key + " (" + binding + ")";
        }

        return Widget.TranslateString( detailKey ) + " " + binding;
    }
}
