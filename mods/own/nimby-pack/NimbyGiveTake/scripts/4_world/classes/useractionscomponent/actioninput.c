class NimbyOfferOrRequestActionInput extends ContinuousDefaultActionInput
{
    void NimbyOfferOrRequestActionInput( PlayerBase player )
    {
        SetInput( "UANimbyOfferRequestItem" );
        m_InputType = ActionInputType.AIT_CLICKCONTINUOUS;
        m_DetectFromItem = false;
    }
}

class NimbyTogglePocketAccessActionInput extends StandardActionInput
{
    void NimbyTogglePocketAccessActionInput( PlayerBase player )
    {
        SetInput( "UANimbyTogglePocketAccess" );
        m_InputType = ActionInputType.AIT_SINGLE;
        m_DetectFromItem = false;
    }
}