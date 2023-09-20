#include "thWindow.h"

#include "thForm.h"
#include "thMDIClient.h"
#include "thMDIChild.h"

/* Defines */
#define MAX_NUMBER_OF_DIGITS_FOR_INDEX 6

/* Prototypes */
LRESULT CALLBACK WinProc( HWND, UINT, WPARAM, LPARAM);

thWindow::thWindow( thWindow * a_pParent, int a_posX, int a_posY)
    :
    m_pParent( nullptr),
    m_hWinHandle( NULL),
    PopupMenu( nullptr),
    Text( *this),
    Width( *this),
    Height( *this),
    X( *this),
    Y( *this),
    Font( *this),
    Resizable( *this)
{
    TH_ENTER_FUNCTION;

    m_sWindowArgs = { 0 };
    m_sWindowArgs.hMenu = reinterpret_cast< HMENU>( this->m_id);;
    m_sWindowArgs.nX = a_posX;
    m_sWindowArgs.nY = a_posY;
    m_sWindowArgs.nWidth = CW_USEDEFAULT;
    m_sWindowArgs.nHeight = CW_USEDEFAULT;

    if ( a_pParent)
    {
        m_pParent = a_pParent;
        m_sWindowArgs.hWndParent = m_pParent->m_hWinHandle;
        m_sWindowArgs.hInstance = m_pParent->m_sWindowArgs.hInstance;
        
        a_pParent->addChildrenWindow( this);
    }
    else
    {
        m_sWindowArgs.hInstance = GetModuleHandle( NULL);
        MSG_WARNING( TEXT( "Empty input pointer! Using module handle as parent instance"));
    }

    TH_LEAVE_FUNCTION;
}

thWindow::~thWindow()
{
}

void thWindow::create()
{
    TH_ENTER_OBJECT_FUNCTION;

    createDebugName();

    if ( NULL == m_hWinHandle)
    {
        if (!Resizable)
        {
            m_sWindowArgs.dwStyle &= ~WS_MINIMIZEBOX;
            m_sWindowArgs.dwStyle &= ~WS_MAXIMIZEBOX;
            m_sWindowArgs.dwStyle &= ~WS_THICKFRAME;
        }
        // Set the size, but not the position.
        RECT rcAdjWidth = {
                0,
                0,
                m_sWindowArgs.nWidth,
                m_sWindowArgs.nHeight
        };
        
        // Calculate window client area (no borders) and adjust the size.
        BOOL result = AdjustWindowRectEx(
            &rcAdjWidth,
            m_sWindowArgs.dwStyle,
            FALSE,
            m_sWindowArgs.dwExStyle
            );

        if ( FALSE == result)
        {
            MSG_ERROR( TEXT( "AdjustWindowRectEx failed with error = 0x%X"), GetLastError());
        }

        m_hWinHandle = CreateWindowEx(
            m_sWindowArgs.dwExStyle,
            m_sWindowArgs.lpClassName,
            m_sWindowArgs.lpWindowName,
            m_sWindowArgs.dwStyle,
            m_sWindowArgs.nX,
            m_sWindowArgs.nY,
            rcAdjWidth.right,
            rcAdjWidth.bottom,
            m_sWindowArgs.hWndParent,
            m_sWindowArgs.hMenu,
            m_sWindowArgs.hInstance,
            m_sWindowArgs.lpParam
            );

        if ( m_hWinHandle)
        {
            MSG_SUCCESS( TEXT( "Created new window: %s with ID=%d"), this->m_name.c_str(), this->m_id);

            // set defualt fond name and size. It must be done after successfull window creation
            this->Font.SetName( TH_DEF_APP_FONT_NAME);
            this->Font.SetSize( TH_DEF_APP_FONT_SIZE);

            const LONG_PTR const pUserData = GetWindowLongPtr( m_hWinHandle, GWLP_USERDATA);

            if ( NULL == pUserData)
            {
                // Set 'this' pointer to user data in Window system object so it can be accessed by WinProc.
                SetWindowLongPtr( m_hWinHandle, GWLP_USERDATA, reinterpret_cast< LONG_PTR>( this));
            }

            this->StoreCurrentRect();
        }
        else
        {
            MSG_ERROR( TEXT( "Failed to create window: %s, CreateWindowEx returned error: 0x%X"), this->m_name.c_str(), GetLastError());
        }
    }

    TH_LEAVE_OBJECT_FUNCTION;
}

LRESULT thWindow::onCreate()
{
    return 0;
}

// NCCreate must return 1! If not, CreateWindowEx won't create window and will return NULL
LRESULT thWindow::onNCCreate()
{
    return 1;
}

LRESULT thWindow::onClose()
{
    return 0;
}

LRESULT thWindow::onDestroy()
{
    TH_ENTER_OBJECT_FUNCTION;

    LRESULT tResult = 0;

    if ( OnDestroy)
    {
        tResult = OnDestroy( this, {});

        if ( 0 != tResult)
        {
            PostQuitMessage( 0);
        }
    }

    TH_LEAVE_OBJECT_FUNCTION;

    return tResult;
}

thWindow * thWindow::GetParent() const
{
    return this->m_pParent;
}

// a_lParam is pointer to char* ended with \0
LRESULT thWindow::onSetText( LPARAM a_lParam)
{
    return 0;
}

// If an application processes this message, it should return zero.
// a_wParam - low-order word is new width
// a_lParam - high-order word is new height
LRESULT thWindow::onResize(HWND a_hwnd, WPARAM a_wParam, LPARAM a_lParam)
{
    LRESULT result = 0;

    // loop through this window children
    for ( const auto & i: m_children)
    {
        // skip thForm and thMDIChild windows
        if ( nullptr == dynamic_cast< thForm*>( i) && nullptr == dynamic_cast< thMDIChild*>( i))
        {
            RECT rcCurPos = { 0 };

            i->GetRect( rcCurPos);

            LONG newX = rcCurPos.left;
            LONG newY = rcCurPos.top;
            LONG newW = i->Width;
            LONG newH = i->Height;

            if ( false == i->Anchors.Left)
            {
                if ( rcCurPos.right > i->m_rcOldPosition.right)
                {
                    newX += ( rcCurPos.right - i->m_rcOldPosition.right);
                }
                else
                {
                    newX -= ( i->m_rcOldPosition.right - rcCurPos.right);
                }

                if ( i->m_pParent)
                {
                    rcCurPos.right = i->m_pParent->Width - (newX + static_cast< LONG>( i->Width));
                }
            }

            if ( false == i->Anchors.Top)
            {
                if ( rcCurPos.bottom > i->m_rcOldPosition.bottom)
                {
                    newY += ( rcCurPos.bottom - i->m_rcOldPosition.bottom);
                }
                else
                {
                    newY -= ( i->m_rcOldPosition.bottom - rcCurPos.bottom);
                }

                if ( i->m_pParent)
                {
                    rcCurPos.bottom = i->m_pParent->Height - ( newX + static_cast< LONG>( i->Height));
                }
            }

            if ( true == i->Anchors.Right)
            {
                if ( rcCurPos.right > i->m_rcOldPosition.right)
                {
                    newW += ( rcCurPos.right - i->m_rcOldPosition.right);
                }
                else
                {
                    newW -= ( i->m_rcOldPosition.right - rcCurPos.right);
                }
            }

            if ( true == i->Anchors.Bottom)
            {
                if ( rcCurPos.bottom > i->m_rcOldPosition.bottom)
                {
                    newH += ( rcCurPos.bottom - i->m_rcOldPosition.bottom);
                }
                else
                {
                    newH -= ( i->m_rcOldPosition.bottom - rcCurPos.bottom);
                }
            }

            BOOL fResult = SetWindowPos(
                i->m_hWinHandle,
                NULL,
                newX,
                newY,
                newW,
                newH,
                SWP_NOZORDER);

            if ( FALSE == fResult)
            {
                MSG_ERROR( TEXT( "SetWindowPos failed with error = 0x%X"), GetLastError());
            }

            result = 1; // this will resize all children manually, so dont send WM_SIZE to children
            
            thMDIClient * mdi = dynamic_cast< thMDIClient*>( i);

            if ( nullptr != mdi)
            { 
                return 1;
            }
        }
        else
        {
            result = 0;
        }
    }

    return result;
}


// a_wParam - handle to the window user right clicked. this can be a child
// a_lParam - low-order word is X, high-order word is Y
// return 0 will process this message further in Z-axis
// return 1 will stop processing this msg
LRESULT thWindow::onContextMenu( WPARAM a_wParam, LPARAM a_lParam)
{
    TH_ENTER_OBJECT_FUNCTION;

    LRESULT result = 1;
    POINT   point = { 0 };
    HWND    hwnd = 0;

    hwnd = reinterpret_cast< HWND>( a_wParam);
    point = { GET_X_LPARAM( a_lParam), GET_Y_LPARAM( a_lParam) };

    if (PopupMenu)
    {
        PopupMenu->Show( hwnd, point);
    }


    TH_LEAVE_OBJECT_FUNCTION;
    return result;
}

// thForm need specialized version
LRESULT thWindow::onGetMinMax( LPARAM a_lParam)
{
    constexpr LRESULT message_not_proccessed{ 0U};
    constexpr LRESULT message_proccessed{ 1U};

    LRESULT      result = message_proccessed;
    MINMAXINFO * sInfo = reinterpret_cast< MINMAXINFO*>( a_lParam);

    if ( sInfo)
    {
        if ( this->Constraints.MaxWidth)
        {
            sInfo->ptMaxTrackSize.x = this->Constraints.MaxWidth;
            result = message_not_proccessed;
        }

        if ( this->Constraints.MaxHeight)
        {
            sInfo->ptMaxTrackSize.y = this->Constraints.MaxHeight;
            result = message_not_proccessed;
        }

        if ( this->Constraints.MinWidth)
        {
            sInfo->ptMinTrackSize.x = this->Constraints.MinWidth;
            result = message_not_proccessed;
        }

        if ( this->Constraints.MinHeight)
        {
            sInfo->ptMinTrackSize.y = this->Constraints.MinHeight;
            result = message_not_proccessed;
        }
    }

    return result;
}

// WPARAM is key code: https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx
// LPARAM is https://msdn.microsoft.com/en-us/library/windows/desktop/ms646280%28v=vs.85%29.aspx
// note: An application should return zero if it processes this message.
LRESULT thWindow::onKeyDown( WPARAM a_wParam, LPARAM a_lParam)
{
    constexpr LRESULT message_not_proccessed{ 0U};

    return message_not_proccessed;
}

LRESULT thWindow::processMessage( HWND a_hwnd, UINT a_uMsg, WPARAM a_wParam, LPARAM a_lParam)
{
    constexpr LRESULT message_not_proccessed{ 0U};

    LRESULT result = message_not_proccessed;

    switch ( a_uMsg)
    {
    case WM_NOTIFY:
        result = this->processNotifyMessage( a_hwnd, a_uMsg, a_wParam, a_lParam);
        break;
    case WM_COMMAND:
        {
            result = this->processCommandMessage( a_hwnd, a_uMsg, a_wParam, a_lParam);

            if ( message_not_proccessed == result)
            {
                result = this->processMenuCommandMessage( a_hwnd, a_uMsg, a_wParam, a_lParam);
            }
        }
        break;
#if 0
    case WM_MENUCOMMAND:
        tResult = this->processMenuCommandMessage(a_hwnd, a_uMsg, a_wParam, a_lParam);
        break;
#endif
    case WM_CONTEXTMENU:
        result = this->onContextMenu( a_wParam, a_lParam);
        break;
    case WM_GETMINMAXINFO:
        result = onGetMinMax( a_lParam);
        break;
    case WM_NCCREATE:
        result = this->onNCCreate();
        break;
    case WM_NCDESTROY:
        {
            if ( m_pParent)
            {
                m_pParent->removeChildrenWindow( this);
            }

            result = this->onDestroy();
        }
        break;
    case WM_CREATE:
        result = this->onCreate();
        break;
    case WM_CLOSE:
        result = this->onClose();
        break;
#if 0
    // not needed
    case WM_DESTROY:
        tResult = this->onDestroy();
        break;
#endif
    case WM_SIZE:
        result = this->onResize( a_hwnd, a_wParam, a_lParam);
        break;
    case WM_CTLCOLORSTATIC:
        //MSG_ERROR(TEXT("WM_CTLCOLORSTATIC - not supported atm"));
        break;
    case WM_SETTEXT:
        result = this->onSetText( a_lParam);
        break;
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ff468861%28v=vs.85%29.aspx
#if 0
    case WM_KEYUP:
        break;
    case WM_KEYDOWN:
        tResult = this->onKeyDown( a_wParam, a_lParam);
        break;
#endif
    }

    if (UserMessage.count(a_uMsg))
    {
        return UserMessage.at(a_uMsg)(this, {a_uMsg, a_wParam, a_lParam});
    }

    return result;
}

// Pass command message to children.
LRESULT thWindow::processCommandMessage( HWND a_hwnd, UINT a_uMsg, WPARAM a_wParam, LPARAM a_lParam)
{
    TH_ENTER_OBJECT_FUNCTION;

    LRESULT result = SendMessage( reinterpret_cast< HWND>( a_lParam), WM_COMMAND, a_wParam, a_lParam);

    TH_LEAVE_OBJECT_FUNCTION;
    return result;
}

// Pass command message to menu.
LRESULT thWindow::processMenuCommandMessage( HWND a_hwnd, UINT a_uMsg, WPARAM a_wParam, LPARAM a_lParam)
{
    TH_ENTER_OBJECT_FUNCTION;

    constexpr LRESULT message_not_proccessed{ 0U};
    LRESULT result = message_not_proccessed;

    if ( PopupMenu)
    {
        result = PopupMenu->processCommandMessage( a_hwnd, a_uMsg, a_wParam, a_lParam);
    }

    TH_LEAVE_OBJECT_FUNCTION;

    return result;
}

LRESULT thWindow::processNotifyMessage(HWND a_hwnd, UINT a_uMsg, WPARAM a_wParam, LPARAM a_lParam)
{
    constexpr LRESULT message_not_proccessed{ 0U};

    LRESULT result = message_not_proccessed;

    const NMHDR * const pData = reinterpret_cast< NMHDR*>( a_lParam);

    if ( pData)
    {
        result = SendMessage( pData->hwndFrom, WM_NOTIFY, a_wParam, a_lParam);
    }

    return result;
}

/*
    pre-condition: m_name should be already set by class deriving from thWindow
*/
void thWindow::createDebugName()
{
    TH_ENTER_OBJECT_FUNCTION;

    TCHAR   szNameIndex[MAX_NUMBER_OF_DIGITS_FOR_INDEX] = { 0 }; // up to 5 digtits
    int     index = this->getDebugIndex();

    if ( -1 == _stprintf_s( szNameIndex, MAX_NUMBER_OF_DIGITS_FOR_INDEX, TEXT( "%d"), index))
    {
        MSG_ERROR( TEXT( "Window index buffer overflow"));
    }
    else
    {
        this->m_name += thString( szNameIndex);
    }

    TH_LEAVE_OBJECT_FUNCTION;
}

void thWindow::addChildrenWindow(thWindow * a_pChildren)
{
    TH_ENTER_OBJECT_FUNCTION;

    if ( a_pChildren)
    {
        bool isDublicateFound = false;

        // check for dublicates
        for ( const auto & i: m_children)
        {
            if ( a_pChildren == i)
            {
                isDublicateFound = true;
                break;
            }
        }

        if ( false == isDublicateFound)
        {
            this->m_children.push_back( a_pChildren);
        }
    }

    TH_LEAVE_OBJECT_FUNCTION;
}

void thWindow::removeChildrenWindow( thWindow * a_pChildren)
{
    TH_ENTER_OBJECT_FUNCTION;

    if ( a_pChildren)
    {
        int position = 0;

        for ( const auto & i: m_children)
        {
            if ( a_pChildren == i)
            {
                m_children.erase( m_children.begin() + position);
                break;
            }

            ++position;
        }
    }

    TH_LEAVE_OBJECT_FUNCTION;
}

thWindow * thWindow::findChildrenByID( const WORD a_searchedId)
{
    TH_ENTER_OBJECT_FUNCTION;
    thWindow * pFoundChildren = nullptr;

    for ( const auto & i : m_children)
    {
        if ( a_searchedId == static_cast< WORD>( i->m_id))
        {
            pFoundChildren = i;
            break;
        }
    }
    TH_LEAVE_OBJECT_FUNCTION;

    return pFoundChildren;
}

// developer is responsible for de-allocating memory
#if 0
void thWindow::destroyChildren()
{
    TH_ENTER_OBJECT_FUNCTION;
    std::vector< thWindow*>::iterator i;

    i = this->m_children.begin();

    for (; i != m_children.end(); i++)
    {
        if ( NULL != ( *i))
        {
            delete ( *i);
            ( *i) = NULL;
        }
    }
    TH_LEAVE_OBJECT_FUNCTION;
}
#endif

void thWindow::Show(void)
{
    TH_ENTER_OBJECT_FUNCTION;

    ShowWindow( this->m_hWinHandle, SW_SHOWNORMAL);

    TH_LEAVE_OBJECT_FUNCTION;
}

void thWindow::Hide(void)
{
    TH_ENTER_OBJECT_FUNCTION;

    ShowWindow( this->m_hWinHandle, SW_HIDE);

    TH_LEAVE_OBJECT_FUNCTION;
}

void thWindow::Enable(void)
{
    TH_ENTER_OBJECT_FUNCTION;

    EnableWindow( this->m_hWinHandle, TRUE);

    TH_LEAVE_OBJECT_FUNCTION;
}

void thWindow::Disable(void)
{
    TH_ENTER_OBJECT_FUNCTION;

    EnableWindow( this->m_hWinHandle, FALSE);

    TH_LEAVE_OBJECT_FUNCTION;
}

void thWindow::Destroy(void)
{
    TH_ENTER_OBJECT_FUNCTION;

    if ( FALSE == DestroyWindow( this->m_hWinHandle))
    {
        MSG_ERROR( TEXT( "DestroyWindow failed with error = 0x%X"), GetLastError());
    }

    TH_LEAVE_OBJECT_FUNCTION;
}

void thWindow::SetFocus(void)
{
    TH_ENTER_OBJECT_FUNCTION;

    if ( NULL == ::SetFocus(this->m_hWinHandle))
    {
        MSG_ERROR( TEXT( "SetFocus failed with error = 0x%X"), GetLastError());
    }

    TH_LEAVE_OBJECT_FUNCTION;
}

// Store current position in m_rcOldPosition. m_rcOldPosition is used when resizing
void thWindow::StoreCurrentRect(void)
{
    TH_ENTER_OBJECT_FUNCTION;

    this->GetRect( this->m_rcOldPosition);

    TH_LEAVE_OBJECT_FUNCTION;
}

// store current window position in an argument
void thWindow::GetRect( RECT & a_rcOutput)
{
    TH_ENTER_OBJECT_FUNCTION;

    if ( m_pParent)
    {
        a_rcOutput.left = this->X;
        a_rcOutput.top = this->Y;
        a_rcOutput.bottom = m_pParent->Height - (this->Y + static_cast<LONG>( this->Height));
        a_rcOutput.right = m_pParent->Width - (this->X + static_cast<LONG>( this->Width));
#if 0
        MSG_SUCCESS( TEXT( "Window %s position: L=%d, T=%d, B=%d, R=%d"), this->m_name.c_str(),
            a_rcOutput.left, a_rcOutput.top, a_rcOutput.bottom, a_rcOutput.right);
#endif
    }

    TH_LEAVE_OBJECT_FUNCTION;
}

const HWND thWindow::GetHandle() const
{
     return this->m_hWinHandle; 
}

// Parent Window Procedure
LRESULT CALLBACK WinProc( HWND a_hwnd, UINT a_uMsg, WPARAM a_wParam, LPARAM a_lParam)
{
    constexpr LRESULT message_not_proccessed{ 0U};

    LRESULT result = message_not_proccessed;

    // Retrieve 'this' pointer from passed by CreateWindowEx.
    const LONG_PTR      pUserData = GetWindowLongPtr( a_hwnd, GWLP_USERDATA);
    thWindow * const    pWindow = reinterpret_cast< thWindow *>( pUserData);

    if ( pWindow)
    {
        result = pWindow->processMessage(a_hwnd, a_uMsg, a_wParam, a_lParam);

        if ( message_not_proccessed == result)
        {
            // Messages passed to MDIClient must be passed to DefFrameProc.
            const thForm * const pForm = dynamic_cast< thForm*>( pWindow);

            if ( pForm && pForm->m_hMDIClient)
            {
                result = DefFrameProc( a_hwnd, pForm->m_hMDIClient, a_uMsg, a_wParam, a_lParam);
            }
            else
            {
                result = DefWindowProc( a_hwnd, a_uMsg, a_wParam, a_lParam);
            }
        }
    }
    else
    {
        result = DefWindowProc( a_hwnd, a_uMsg, a_wParam, a_lParam);
    }

    return result;
}

// Children Window Procedure
LRESULT CALLBACK ChildWindProc(
    HWND        a_hWnd,
    UINT        a_uMsg,
    WPARAM      a_wParam,
    LPARAM      a_lParam,
    UINT_PTR    a_uIdSubclass,
    DWORD_PTR   a_dwRefData
)
{
    constexpr LRESULT message_not_proccessed{ 0U};

    LRESULT result = message_not_proccessed;

    if ( WM_NCDESTROY == a_uMsg)
    {
        BOOL fResult = RemoveWindowSubclass( a_hWnd, ChildWindProc, a_uIdSubclass);

        if ( FALSE == fResult)
        {
            MSG_ERROR( TEXT( "RemoveWindowSubclass failed"));
        }
    }

    thWindow * const pWindow = reinterpret_cast<thWindow*>(a_dwRefData);

    if ( pWindow)
    {
        result = pWindow->processMessage( a_hWnd, a_uMsg, a_wParam, a_lParam);

        if ( message_not_proccessed == result)
        {
            result = DefSubclassProc( a_hWnd, a_uMsg, a_wParam, a_lParam);
        }
    }
    else
    {
        result = DefSubclassProc( a_hWnd, a_uMsg, a_wParam, a_lParam);
    }

    return result;
}