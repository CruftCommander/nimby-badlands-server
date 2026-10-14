modded class PlayerBaseClient
{
    protected bool m_NimbyLastAllowsPocketAccess = false;

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();

        if ( !IsControlledPlayer() )
        {
            return;
        }

        if ( m_NimbyAllowsPocketAccess != m_NimbyLastAllowsPocketAccess )
        {
            m_NimbyLastAllowsPocketAccess = m_NimbyAllowsPocketAccess;

            if ( NimbyGetAllowsPocketAccess() )
            {
                NotificationSystem.AddNotificationExtended( NimbyGiveTakeConstants.POCKET_NOTIFICATION_TIME, "#STR_NIMBY_GT_POCKETS_OPEN", "#STR_NIMBY_GT_POCKETS_OPEN_DETAIL", "set:dayz_gui image:icon_lock" );
            }
            else
            {
                NotificationSystem.AddNotificationExtended( NimbyGiveTakeConstants.POCKET_NOTIFICATION_TIME, "#STR_NIMBY_GT_POCKETS_CLOSED", "#STR_NIMBY_GT_POCKETS_CLOSED_DETAIL", "set:dayz_gui image:icon_lock" );
            }
        }
    }
}