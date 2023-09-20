#include "thEditBox.h"

/* Defines */
#define CLASS_NAME TEXT("thEditBox")
#define WIN32_CLASS_NAME TEXT("EDIT")
#define DEFAULT_TEXT TEXT("")

#define DEFAULT_WIDTH  75
#define DEFAULT_HEIGHT 20

/* Local Memory */
int thEditBox::m_indexPool = 1;

/* Prototypes */

thEditBox::thEditBox( thWindow * a_pParent, int a_posX, int a_posY)
    :
    thWindow( a_pParent, a_posX, a_posY)
{
    TH_ENTER_FUNCTION;

    this->m_name = CLASS_NAME;

    this->m_sWindowArgs.dwExStyle =     WS_EX_CLIENTEDGE;
    this->m_sWindowArgs.lpClassName =   WIN32_CLASS_NAME;
    this->m_sWindowArgs.lpWindowName =  DEFAULT_TEXT;
    this->m_sWindowArgs.dwStyle =       WS_TABSTOP | WS_VISIBLE | WS_CHILD | ES_LEFT; //ES_READONLY WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL
    this->m_sWindowArgs.nWidth =        DEFAULT_WIDTH;
    this->m_sWindowArgs.nHeight =       DEFAULT_HEIGHT;
    this->m_sWindowArgs.hMenu =         reinterpret_cast< HMENU>( this->m_id);;
    this->m_sWindowArgs.lpParam =       this;

    this->create();

    BOOL fResult = SetWindowSubclass( this->m_hWinHandle, ChildWindProc, 0, reinterpret_cast< DWORD_PTR>( this));

    if ( FALSE == fResult)
    {
        MSG_ERROR( TEXT( "SetWindowSubclass failed with error = 0x%X"), GetLastError());
    }

    TH_LEAVE_FUNCTION;
}

void thEditBox::SetCaretPosition( uint32_t a_u32Position)
{
    SendMessage( this->m_hWinHandle, EM_SETSEL, a_u32Position, a_u32Position);
}

int thEditBox::getDebugIndex()
{
    TH_ENTER_OBJECT_FUNCTION;

    this->m_indexPool++;

    TH_LEAVE_OBJECT_FUNCTION;
    return this->m_indexPool;
}

// WPARAM is key code: https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx
// LPARAM is https://msdn.microsoft.com/en-us/library/windows/desktop/ms646280%28v=vs.85%29.aspx
// note: An application should return zero if it processes this message.
LRESULT thEditBox::onKeyDown( WPARAM a_wParam, LPARAM a_lParam)
{
    LRESULT tResult = 0;

    MSG_LOG( TEXT( "WM_KEYDOWN"));

    if ( nullptr != OnKeyDown)
    {
        tResult = this->OnKeyDown( this, { 0, a_wParam, a_lParam });
    }
    else
    {
        tResult = 1;
    }

    return tResult;
}

LRESULT thEditBox::processCommandMessage( HWND a_hwnd, UINT a_uMsg, WPARAM a_wParam, LPARAM a_lParam)
{
    TH_ENTER_OBJECT_FUNCTION;
    LRESULT tResult = 0; // should return 1 if not used (no CB registered)

    TH_LEAVE_OBJECT_FUNCTION;
    return tResult;
}

LRESULT thEditBox::processNotifyMessage( HWND a_hwnd, UINT a_uMsg, WPARAM a_wParam, LPARAM a_lParam)
{
    //TH_ENTER_OBJECT_FUNCTION;
    LRESULT tResult = 0;
    NMHDR * pData = reinterpret_cast< NMHDR*>( a_lParam);

    if ( pData)
    {
        MSG_ERROR( TEXT( "WM_NOTIFY: hwndFrom=0x%X, idFrom=%d, code=0x%X"), pData->hwndFrom, pData->idFrom, pData->code);
    }

    //TH_LEAVE_OBJECT_FUNCTION;
    return tResult;
}