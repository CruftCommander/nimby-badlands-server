modded class PlayerBaseClient
{
    protected bool m_LastAllowsPutInPockets = false;

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();

        if ( !IsControlledPlayer() )
        {
            return;
        }

        if ( m_AllowsPutInPockets != m_LastAllowsPutInPockets )
        {
            m_LastAllowsPutInPockets = m_AllowsPutInPockets;

            if ( GetAllowsPutInPockets() )
            {
                NotificationSystem.AddNotificationExtended( GameConstants.POCKTS_OPEN_CLOSE_NOTIFICATION_TIME, "#STR_POCKETS_OPEN", "#STR_POCKETS_OPEN_DETAIL", "set:dayz_gui image:icon_lock" );
            }
            else
            {
                NotificationSystem.AddNotificationExtended( GameConstants.POCKTS_OPEN_CLOSE_NOTIFICATION_TIME, "#STR_POCKETS_CLOSED", "#STR_POCKETS_CLOSED_DETAIL", "set:dayz_gui image:icon_lock" );
            }
        }
    }
}