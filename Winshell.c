HWND g_innerShell;
void RefreshShell() {
        InvalidateRect(g_innerShell, NULL, TRUE);
}

#define CONSOLE_BUTTON 0
#define SOURCE_BUTTON  1
#define PAGE_BUTTON    2

static int selected_status_button = CONSOLE_BUTTON;

#include "tabs.c"

#ifdef WIN3_1
void DebugLogAttr(Tab far *tab, BOOL bold, BOOL italic, COLORREF color) {

}
#else
void DebugLogAttr(Tab far *tab, BOOL bold, BOOL italic, COLORREF color) {
        CHARFORMAT cf;
        DWORD dwMask;
        DWORD dwEffect;

        

        memset(&cf,0,sizeof(cf));
        cf.cbSize = sizeof(cf);

        dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC;
        dwEffect = 0;
        if (bold) {
                dwEffect |= CFE_BOLD;
        }
        if (italic) {
                dwEffect |= CFE_ITALIC;
        }
        cf.crTextColor = color;
        cf.dwMask = dwMask;
        cf.dwEffects = dwEffect;

        SendMessage(tab->hConsole,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);
}
#endif

void WriteSource(Tab far *tab, LPSTR raw, int len) {
        int ndx;

       //       return;

        ndx = GetWindowTextLength (tab->hSource);
        SetFocus(tab->hSource);
        #ifdef WIN32
          SendMessage(tab->hSource, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
        #else
          SendMessage(tab->hSource, EM_SETSEL, 0, MAKELONG (ndx, ndx));
        #endif
        SendMessage(tab->hSource, EM_REPLACESEL, 0, (LPARAM) ((LPSTR) raw));
 
        {
        //char near *ptr = FarStrToNear(raw);
        
        /*fwrite(ptr, 1, len, fpSource);
        fflush(fpSource);
        fclose(fpSource);
        fpSource = fopen("C:\\source.log", "a");*/
        
        //LocalFree((HLOCAL)ptr);
        }
}



void DebugLog(Tab far *tab, LPSTR format, ...) {
        int ndx;

        va_list args;
        int     len;
        LPSTR buffer = format;

        //return;

        if (! tab) tab = TabFromId(g_currTabId);
		
//#if 0
//		return;

        // retrieve the variable arguments
        va_start( args, format );

//#ifndef WIN3_1
//      len = _vscprintf( format, args ) // _vscprintf doesn't count
                                                            //  + 1; // terminating '\0'
//#else
        //len = 1024*6;
        len = vprintf_buffer_size(format, args);
//#endif

        buffer = (LPSTR)LocalAlloc(GMEM_FIXED,  len * sizeof(char) );

{
        //const void FAR *wargs = (const void FAR *)((&format)+1);
       // lstrcpy(buffer, ".");
        wvsprintf( buffer, format, args ); // C4996
}

//buffer = format;


//#endif

        ndx = GetWindowTextLength (tab->hConsole);
        SetFocus(tab->hConsole);
        #ifdef WIN32
          SendMessage(tab->hConsole, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
        #else
          SendMessage(tab->hConsole, EM_SETSEL, 0, MAKELONG (ndx, ndx));
        #endif
        SendMessage(tab->hConsole, EM_REPLACESEL, 0, (LPARAM) ((LPSTR) buffer));

        
        /*fwrite(buffer, 1, strlen(buffer), fpLog);
        fflush(fpLog);
        fclose(fpLog);
        fpLog = fopen("C:\\out.log", "a");*/

        LocalFree((void *)buffer);
        va_end(args);
}

//LPSTR lpszStatus =  NULL;
HWND hStatusBar = NULL;

void SetStatusText(Tab far *tab, LPSTR format, ...) {
        //int ndx;

        va_list args;
        int     len;
                
                LPSTR lpszStatus;
				
		//		 SetTabStatus(tab, format);
		//if (hStatusBar) InvalidateRect(hStatusBar, NULL, TRUE);
		//return;
				
				
				//return;
        
       // if (lpszStatus) LocalFree((void *)lpszStatus);

        // retrieve the variable arguments
        va_start( args, format );

//#ifndef WIN3_1
//      len = _vscprintf( format, args ) // _vscprintf doesn't count
                                                               // + 1; // terminating '\0'
//#else
        //len = 1024*6;
        len = vprintf_buffer_size(format, args);
        
//#endif

        lpszStatus = (LPSTR)LocalAlloc(GMEM_FIXED, len * sizeof(char) );
{
    //const void FAR *wargs = (const void FAR *)((&format)+1);
        wvsprintf( lpszStatus, format, args ); // C4996
}

        SetTabStatus(tab, lpszStatus);

        va_end(args);   
                
                LocalFree(lpszStatus);
        
        if (hStatusBar) InvalidateRect(hStatusBar, NULL, TRUE);
}

#ifndef GWLP_WNDPROC
static FARPROC oldAddressBarProc;
#else
static WNDPROC oldAddressBarProc;
#endif

BOOL CreateTabChildWindows(HWND parent, Tab far *tab) {
        tab->hConsole = CreateWindow("RICHEDIT50W", "",  WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parent, NULL, g_hInstance, NULL);
        if (! tab->hConsole) {
                tab->hConsole = CreateWindow("EDIT", "", WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parent, NULL, g_hInstance, NULL);
        }
        tab->hSource = CreateWindow("RICHEDIT50W", "",  WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parent, NULL, g_hInstance, NULL);
        if (! tab->hSource) {
                tab->hSource = CreateWindow("EDIT", "", WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parent, NULL, g_hInstance, NULL);
        }
        return TRUE;
}

BOOL SetActiveTab(Tab far *tab) {
        Tab far *currTab = TabFromId(g_currTabId);
        //RECT rc;
        HWND shell;
        if (currTab) {
                int len = GetWindowTextLength(hAddressBar);
                LPSTR buff;
                len++;
                buff = (LPSTR)LocalAlloc(GMEM_FIXED, len+1);
                buff[len] = '\0';
                GetWindowText(hAddressBar, buff, len);
                SetTabURL(currTab, buff);
                LocalFree((void *)buff);
                ShowWindow(currTab->hSource, SW_HIDE);
                ShowWindow(currTab->hConsole, SW_HIDE);
        }
        if (selected_status_button == CONSOLE_BUTTON) ShowWindow(tab->hConsole, SW_SHOW);
        else if (selected_status_button == SOURCE_BUTTON) ShowWindow(tab->hSource, SW_SHOW);
        g_currTabId = tab->id;
        shell = GetParent(tab->hSource);
        SendMessage(shell, WM_SIZE, 0, 0L);


       // if (lpszStatus) LocalFree((void far *)lpszStatus);
        //lpszStatus = (LPSTR)LocalAlloc(GMEM_FIXED, lstrlen(tab->szStatus)+1);
        //lstrcpy(lpszStatus, tab->szStatus);
        if (hStatusBar) InvalidateRect(hStatusBar, NULL, TRUE);

        SetWindowText(hAddressBar, tab->szUrl);

        //GetWindowRect(shell, &rc);
        //SetWindowPos(shell, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER);

        return TRUE;
}

COLORREF GetTabColor(BOOL selected, BOOL special, BOOL over) {
        if (over) return selected ? RGB(162, 62, 52) : RGB(22, 22, 192);
        return (selected ? RGB(182, 82, 72) : (special ? RGB(0, 0, 0) : RGB(62, 62, 62)));
}

#define COLOR(x) x>255?255:x

COLORREF GetTabShineColor(BOOL selected, BOOL special, BOOL over) {
        int inc = 190;
        return (selected ? RGB(255, 255, 255) : (special ? RGB(255, 255, 255) : RGB(255, 255, 255)));
        if (over) return selected ? RGB(COLOR(162+inc), COLOR(62+inc), COLOR(52+inc)) : RGB(COLOR(22+inc), COLOR(22+inc), COLOR(192+inc));
        return (selected ? RGB(COLOR(182+inc), COLOR(82+inc), COLOR(72+inc)) : (special ? RGB(COLOR(0+inc), COLOR(0+inc), COLOR(0+inc)) : RGB(62+inc, 62+inc, 62+inc)));
}

COLORREF GetWinColor() {
        return RGB(239, 235, 235);
}

LRESULT CALLBACK AddressBarProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
                case WM_LBUTTONDOWN:
                        SetFocus(hWnd);
                        break;
                case WM_KEYDOWN:
                        if (wParam == VK_RETURN) {
                                LPSTR url;
                                Tab far *tab;
                                int len = GetWindowTextLength(hWnd);
                                url = (LPSTR)LocalAlloc(GMEM_FIXED, len+2);
                                url[len] = '\0';
                                GetWindowText(hWnd, url, len+1);
                                tab = TabFromId(g_currTabId);
                                OpenUrl(tab, &tab->TOP_WINDOW, tab->TOP_WINDOW.document, url);
                                LocalFree((void *)url);
                                return FALSE;
                        }
                        break;
        }
        return (LRESULT)CallWindowProc(oldAddressBarProc, hWnd, msg, wParam, lParam);
}

#ifndef WIN3_1
typedef UINT  (WINAPI * lpGetDpiForWindow)(HWND hWnd);
lpGetDpiForWindow GetDpiForWindow = NULL;
#endif

#define DEF_FONT_HEIGHT 25
#define DEF_DPI_FONT_HEIGHT 14

static void drawStatusButtonText(HDC hDC, WPARAM wParam, LPSTR szText, LPRECT rc) {
        if (wParam == selected_status_button) {
                SetTextColor(hDC, RGB(200, 200, 200));
                rc->left += 2;
                rc->top += 2;
                DrawText(hDC, szText, lstrlen(szText), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
                SetTextColor(hDC, RGB(0, 0, 0));
                rc->left -= 2;
                rc->top -= 2;
                DrawText(hDC, szText, lstrlen(szText), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE );    
        }
        else {
                SetTextColor(hDC, RGB(44, 44, 44));
                DrawText(hDC, szText, lstrlen(szText), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
        }
}

LRESULT CALLBACK BrowserShellToggleBar(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        static HWND hSource = NULL, hPage = NULL, hConsole = NULL;
        static HFONT hToggleFont;
        static HFONT hToggleFontBold;
        switch (msg) {
                case WM_CREATE: {
                        

                        hSource = CreateWindow("BUTTON", "Source", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, (HMENU)SOURCE_BUTTON, g_hInstance, NULL);
                        hPage = CreateWindow("BUTTON", "Page", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, (HMENU)PAGE_BUTTON, g_hInstance, NULL);
                        hConsole = CreateWindow("BUTTON", "Console", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, (HMENU)CONSOLE_BUTTON, g_hInstance, NULL);
                        
                        //SendMessage(hSource, WM_SETFONT, (WPARAM)hFont, TRUE);
                        //SendMessage(hPage, WM_SETFONT, (WPARAM)hFont, TRUE);
                        //SendMessage(hConsole, WM_SETFONT, (WPARAM)hFont, TRUE);
                        
                        //SendMessage(hSource, BM_SETCHECK, BST_CHECKED, 0);
                        
                        return 0;
                }
                case WM_SETCURSOR:
#if defined(IDC_HAND)
                        if ((HWND)wParam == hSource || (HWND)wParam == hPage || (HWND)wParam == hConsole) {
                                SetCursor(LoadCursor(NULL, IDC_HAND));
                                return TRUE;
                        }
#endif
                        return (DefWindowProc(hWnd, msg, wParam, lParam));
                case WM_DRAWITEM: {
                        LPDRAWITEMSTRUCT di = (LPDRAWITEMSTRUCT)lParam;
                        HFONT hPrevFont;
                        HBRUSH hbr;
                        
#ifndef WIN3_1                  
                        if (GetDpiForWindow) {
                                UINT dpi = GetDpiForWindow(hWnd);
                                int fontHeight = (int)(((float)dpi / (float)5.5)); // 1/3.5 inch
                                //fontheight = -MulDiv(PointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
                                hToggleFont = CreateFont(fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
                                hToggleFontBold = CreateFont(fontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
                        }                       
                        else {
                                hToggleFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
                                hToggleFontBold = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
                        }
#else
                        hToggleFont = CreateFont(16, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
                        hToggleFontBold = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
#endif                  
                
                        if (wParam == selected_status_button) {
                                SetTextColor(di->hDC, RGB(0, 0, 0));
                                hPrevFont = (HFONT)SelectObject(di->hDC, hToggleFontBold);
                                FillRect(di->hDC, &di->rcItem, GetStockObject(WHITE_BRUSH));
                        }
                        else {
                                SetTextColor(di->hDC, RGB(33, 33, 33));
                                hPrevFont = (HFONT)SelectObject(di->hDC, hToggleFont);
//#ifdef WIN3_1                 
                                hbr = CreateSolidBrush(GetWinColor());
                                FillRect(di->hDC, &di->rcItem, hbr);
                                DeleteObject(hbr);
//#else
                                //FillRect(di->hDC, &di->rcItem, (HBRUSH)COLOR_WINDOW);
//#endif
                        }
                        
                        {
#ifdef WIN3_1                   
                        HPEN hbr = CreatePen(PS_SOLID, 3, RGB(255, 255, 255));
#else
                        HPEN hbr = CreatePen(PS_SOLID, 4, RGB(255, 255, 255));
#endif
                        HPEN hPrevBrush = (HPEN)SelectObject(di->hDC, hbr);
#ifdef WIN3_1
                        MoveTo(di->hDC, 0, 0);
#else
                        MoveToEx(di->hDC, 0, 0, NULL);
#endif
                        LineTo(di->hDC, di->rcItem.right, 0);


                        SelectObject(di->hDC, hPrevBrush);
                        DeleteObject(hbr);

#ifdef WIN3_1                   
                        hbr = CreatePen(PS_SOLID, 1, RGB(152, 152, 152));
#else
                        hbr = CreatePen(PS_SOLID, 1, RGB(152, 152, 152));
#endif
                        hPrevBrush = (HPEN)SelectObject(di->hDC, hbr);

#ifdef WIN3_1
                        MoveTo(di->hDC, 0, 0);
#else
                        MoveToEx(di->hDC, 0, 0, NULL);
#endif
                        LineTo(di->hDC, 0, di->rcItem.bottom-0);
                        SelectObject(di->hDC, hPrevBrush);
                        DeleteObject(hbr);

#ifdef WIN3_1                   
                        hbr = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
#else
                        hbr = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
#endif
                        hPrevBrush = (HPEN)SelectObject(di->hDC, hbr);

#ifdef WIN3_1
                        MoveTo(di->hDC, 1, 0);
#else
                        MoveToEx(di->hDC, 1, 0, NULL);
#endif
                        LineTo(di->hDC, 1, di->rcItem.bottom-0);


                        SelectObject(di->hDC, hPrevBrush);
                        DeleteObject(hbr);
                        }                       
                        
                        
                        SetBkMode(di->hDC, TRANSPARENT);
                        switch (wParam) {
                                case SOURCE_BUTTON:
                                        drawStatusButtonText(di->hDC, wParam, "Source", &di->rcItem); 
                                        break;
                                case PAGE_BUTTON:
                                        drawStatusButtonText(di->hDC, wParam, "Page", &di->rcItem); 
                                        break;                                  
                                case CONSOLE_BUTTON:
                                        drawStatusButtonText(di->hDC, wParam, "Console", &di->rcItem); 
                                        break;                                  
                        }
                        SelectObject(di->hDC, hPrevFont);
                        
                        DeleteObject(hToggleFont);
                        DeleteObject(hToggleFontBold);
                        return TRUE;
                }
                case WM_COMMAND:
                        switch (LOWORD(wParam)) {
                                case SOURCE_BUTTON:
                                        selected_status_button = SOURCE_BUTTON;
                                        InvalidateRect(hWnd, NULL, TRUE);
                                        SetActiveTab(TabFromId(g_currTabId));
                                        break;
                                case CONSOLE_BUTTON:
                                        selected_status_button = CONSOLE_BUTTON;
                                        InvalidateRect(hWnd, NULL, TRUE);
                                        SetActiveTab(TabFromId(g_currTabId));
                                        break;
                                case PAGE_BUTTON:
                                        selected_status_button = PAGE_BUTTON;
                                        InvalidateRect(hWnd, NULL, TRUE);
                                        SetActiveTab(TabFromId(g_currTabId));
                                        break;
                        }
                        return 0;
                case WM_SIZE: {
                        RECT rc, rcBtn;
                        int l, padding;
                        
                        GetClientRect(hWnd, &rc);
                        GetWindowRect(hConsole, &rcBtn);
                        l = rc.right;
                        padding = 0;
                        
                        rcBtn.left = 0;
#ifdef WIN3_1                   
                        rcBtn.right = 65;
#else
                        rcBtn.right = 115;
#endif
                        
                        MoveWindow(hConsole, l - (rcBtn.right - rcBtn.left) - padding, 0, (rcBtn.right - rcBtn.left) + padding, rc.bottom,  TRUE);
                        l -= ((rcBtn.right - rcBtn.left) - padding);
                        
                        GetWindowRect(hSource, &rcBtn);
                        
                        rcBtn.left = 0;
#ifdef WIN3_1                   
                        rcBtn.right = 65;
#else
                        rcBtn.right = 115;
#endif
                        
                        MoveWindow(hSource, l - (rcBtn.right - rcBtn.left) - padding, 0, (rcBtn.right - rcBtn.left) + padding, rc.bottom, TRUE);
                        l -= ((rcBtn.right - rcBtn.left) - padding);
                        
                        GetWindowRect(hPage, &rcBtn);
                        
                        rcBtn.left = 0;
#ifdef WIN3_1                   
                        rcBtn.right = 65;
#else
                        rcBtn.right = 115;
#endif
                        
                        MoveWindow(hPage, l - (rcBtn.right - rcBtn.left) - padding, 0, (rcBtn.right - rcBtn.left) + padding, rc.bottom, TRUE);
                        
                        return 0;
                }
                case WM_PAINT: {
                        
                        PAINTSTRUCT ps;
                        RECT rc;
                        HDC hDC;
                        static HFONT hFont = NULL;
                        HFONT hPrevFont;
                                                
                                                LPSTR lpszStatus;
                                                Tab far *currTab = TabFromId(g_currTabId);
                                                if (! currTab || ! currTab->szStatus)  return DefWindowProc(hWnd, msg, wParam, lParam);
                                                
                                                lpszStatus = currTab->szStatus;
                        
                        //if (! lpszStatus) return DefWindowProc(hWnd, msg, wParam, lParam);
                        
                        hDC = BeginPaint(hWnd, &ps);
#ifndef WIN3_1                  
                        if (GetDpiForWindow) {
                                UINT dpi = GetDpiForWindow(hWnd);
                                int fontHeight = (int)(((float)dpi / (float)5.0)); // 1/3.5 inch
                                //fontheight = -MulDiv(PointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
                                if (! hFont) hFont = CreateFont(fontHeight, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
                        }                       
                        else {
                                if (! hFont) hFont = CreateFont(16, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
                        }
#else
                        if (! hFont) hFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
#endif
                        
                        hPrevFont = (HFONT)SelectObject(hDC, hFont);
                        GetClientRect(hWnd, &rc);
#ifdef WIN3_1
                        rc.left += 4;
#else
                        rc.left += 6;
#endif
                        SetBkMode(hDC, TRANSPARENT);
                        
#ifdef WIN3_1                   
                        rc.left+=2;
                        rc.top+=2;
                        SetTextColor(hDC, RGB(200, 200, 200));
#else
                        rc.left+=1;
                        rc.top+=1;
                        //SetTextColor(hDC, RGB(255, 255, 255));
                        SetTextColor(hDC, RGB(255, 255, 255));
#endif
                        DrawText(hDC, lpszStatus, lstrlen(lpszStatus), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE );

#ifdef WIN3_1
                        rc.left-=2;
                        rc.top-=2;
                        SetTextColor(hDC, RGB(0, 0, 0));
#else
                        rc.left-=1;
                        rc.top-=1;
                        SetTextColor(hDC, RGB(66, 66, 66));
#endif
                        DrawText(hDC, lpszStatus, lstrlen(lpszStatus), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE );
                        SelectObject(hDC, hPrevFont);
                        
                        
                        {
#ifdef WIN3_1                   
                        HPEN hbr = CreatePen(PS_SOLID, 3, RGB(255, 255, 255));
#else
                        HPEN hbr = CreatePen(PS_SOLID, 4, RGB(255, 255, 255));
#endif
                        HPEN hPrevBrush = (HPEN)SelectObject(hDC, hbr);
                        //GetClientRect(hWnd, &rc);
#ifdef WIN3_1
                        MoveTo(hDC, 0, 0);
#else
                        MoveToEx(hDC, 0, 0, NULL);
#endif
                        LineTo(hDC, rc.right, 0);
                        SelectObject(hDC, hPrevBrush);
                        DeleteObject(hbr);
                        }
                        
                        
                        //DeleteObject(hFont);
                        EndPaint(hWnd, &ps);
                        return 0;
                }
        }
        return (DefWindowProc(hWnd, msg, wParam, lParam));
}

void drawTab(HWND hWnd, HDC hDC, LPRECT rc, LPSTR szText, BOOL selected, BOOL special, BOOL mouseover, BOOL overx) {
        HPEN hpen;
        HPEN hPrevBrush;
        static HFONT hToggleFontBold = NULL;
        static HFONT hToggleFont = NULL;
        HBRUSH hbr;
        
        if (overx) mouseover = FALSE;

                hbr = CreateSolidBrush(GetTabColor(selected, special, mouseover));


#ifndef WIN3_1                  
                        if (GetDpiForWindow) {
                                UINT dpi = GetDpiForWindow(hWnd);
                                int fontHeight = (int)(((float)dpi / (float)5.5)); // 1/3.5 inch
                                //fontheight = -MulDiv(PointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
                                if (! hToggleFont) hToggleFont = CreateFont(fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
                                if (! hToggleFontBold) hToggleFontBold = CreateFont(fontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
                        }                       
                        else {
                                if (! hToggleFont) hToggleFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
                                if (! hToggleFontBold) hToggleFontBold = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
                        }
#else
                        if (! hToggleFont) hToggleFont = CreateFont(16, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
                        if (! hToggleFontBold) hToggleFontBold = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
#endif


        //HBRUSH hPrevBrush = (HBRUSH)SelectObject(hDC, hbr);
rc->left+=1;
        FillRect(hDC, rc,hbr);
rc->left-=1;
        SetBkMode(hDC, TRANSPARENT);
        //SetTextColor(hDC, RGB(66, 66, 66));
        //DrawText(hDC, "Tab 1", 5, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE );

if (selected) SelectObject(hDC, hToggleFontBold);
else SelectObject(hDC, hToggleFont);

                                        SetTextColor(hDC, selected ? RGB(0, 0, 0) : RGB(100, 100, 255));
                                        rc->left += 2;
                                        rc->top += 2;
                                        rc->left += 2;
                                        rc->top += 2;
                                        if (selected) {
#ifdef WIN3_1
                                        DrawText(hDC, szText, lstrlen(szText), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
#else
                                        DrawText(hDC, szText, lstrlen(szText), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
#endif
                                        }
                                        SetTextColor(hDC, (selected || special)? RGB(255, 255, 255) : RGB(255, 255, 255));
                                        rc->left -= 2;
                                        rc->top -= 2;
#ifdef WIN3_1
                                        DrawText(hDC, szText, lstrlen(szText), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
#else
                                        DrawText(hDC, szText, lstrlen(szText), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
#endif
                                        rc->left -= 2;
                                        rc->top -= 2;
                                        
if (! special && (mouseover||overx)) {

                                                SetTextColor(hDC, selected ? RGB(0, 0, 0) : RGB(100, 100, 255));
                                        rc->right -= 8;
                                        rc->left += 2;
                                        rc->top += 2;
                                        rc->left += 2;
                                        rc->top += 2;
                                        //if (selected) {
#ifdef WIN3_1
                                        DrawText(hDC, " X ", 3, rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
#else
                                        DrawText(hDC, " X ", 3, rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
#endif
                                        //}
                                        if (overx) {
                                                SetBkColor(hDC, RGB(255, 255, 255));
                                                SetBkMode(hDC, OPAQUE);
                                                SetTextColor(hDC, (selected || special) ? RGB(255, 0, 0) : RGB(255, 0, 0));
                                                
                                        }
                                        else SetTextColor(hDC, (selected || special)? RGB(255, 255, 255) : RGB(255, 255, 255));
                                        rc->left -= 2;
                                        rc->top -= 2;
#ifdef WIN3_1
                                        DrawText(hDC, " X ", 3, rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
#else
                                        DrawText(hDC, " X ", 3, rc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
#endif
                                        SetBkMode(hDC, TRANSPARENT);
                                        rc->left -= 2;
                                        rc->top -= 2;
                                        rc->right += 8;
}       

        DeleteObject(hbr);


#ifdef WIN3_1                   
        hpen = CreatePen(PS_SOLID, 1, GetTabColor(selected, special, mouseover));
#else
        hpen = CreatePen(PS_SOLID, 1, GetTabColor(selected, special, mouseover));
#endif
        hPrevBrush = (HPEN)SelectObject(hDC, hpen);
#ifdef WIN3_1                   
        MoveTo(hDC, rc->left, rc->top+1);
#else
        MoveToEx(hDC, rc->left, rc->top+1, NULL);
#endif
        LineTo(hDC, rc->left, rc->bottom);
        SelectObject(hDC, hPrevBrush);
        DeleteObject(hpen);

        hpen = CreatePen(PS_SOLID, 1, GetTabShineColor(selected, special, mouseover));
        hPrevBrush = (HPEN)SelectObject(hDC, hpen);
#ifdef WIN3_1                   
        MoveTo(hDC, rc->left+2, rc->top+1);
#else
        MoveToEx(hDC, rc->left+2, rc->top+1, NULL);
#endif
        LineTo(hDC, rc->right-1, rc->top+1);
        SelectObject(hDC, hPrevBrush);
        DeleteObject(hpen);

        hpen = CreatePen(PS_SOLID, 1, GetTabShineColor(selected, special, mouseover));
        hPrevBrush = (HPEN)SelectObject(hDC, hpen);
#ifdef WIN3_1                   
        MoveTo(hDC, rc->left+1, rc->top+1);
#else
        MoveToEx(hDC, rc->left+1, rc->top+1, NULL);
#endif
        LineTo(hDC, rc->left+1, rc->bottom);
        SelectObject(hDC, hPrevBrush);
        DeleteObject(hpen);



#ifdef WIN3_1                   
        hpen = CreatePen(PS_SOLID, 1, GetTabColor(selected, special, mouseover));
#else
        hpen = CreatePen(PS_SOLID, 1, GetTabColor(selected, special, mouseover));
#endif
        hPrevBrush = (HPEN)SelectObject(hDC, hpen);
#ifdef WIN3_1                   
        MoveTo(hDC, rc->left+1, rc->top);
#else
        MoveToEx(hDC, rc->left+1, rc->top, NULL);
#endif
        LineTo(hDC, rc->right-1, rc->top);
        SelectObject(hDC, hPrevBrush);
        DeleteObject(hpen);

#ifdef WIN3_1                   
        hpen = CreatePen(PS_SOLID, 1, GetTabColor(selected, special, mouseover));
#else
        hpen = CreatePen(PS_SOLID, 1, GetTabColor(selected, special, mouseover));
#endif
        hPrevBrush = (HPEN)SelectObject(hDC, hpen);
#ifdef WIN3_1                   
        MoveTo(hDC, rc->right, rc->top+1);
#else
        MoveToEx(hDC, rc->right, rc->top+1, NULL);
#endif
        LineTo(hDC, rc->right, rc->bottom);
        SelectObject(hDC, hPrevBrush);
        DeleteObject(hpen);

}

BOOL OverTab(const RECT *lprc, POINT pt)
{
        return pt.x >= lprc->left && pt.y >= lprc->top && pt.x <= lprc->right && pt.y <= lprc->bottom;
}

#define TAB_X_SIZE 25

BOOL OverTabX(const RECT *lprc, POINT pt)
{
        return pt.x >= (lprc->right-TAB_X_SIZE) && pt.y >= lprc->top && pt.x <= lprc->right && pt.y <= lprc->bottom;
}

static int g_selectedTab = 1;
static int g_numTabs = 1;

void drawTabs(HWND hWnd, HDC hDC, LPRECT rc, LPPOINT mousecoords, BOOL draw, int *overtab, BOOL *overx) {
        RECT rcTab;
        
        int bottom = rc->top + rc->bottom;

        int numTabs = GetNumTabs();

        int l = rc->left;
        int tabSize = 165;
        int i;
        BOOL drawnSelected = FALSE;
        BOOL suboveradd;
        
        if (overtab) *overtab = -1;
        if (overx) *overx = FALSE;
        
        #define FULL_TAB_SIZE 230

        if (numTabs-1 > 0) {
                tabSize = (rc->right-l-45-FULL_TAB_SIZE) / (numTabs-1);
                //tabSize = tabSize + ((5-(tabSize % 5)) % 5);
                tabSize -= 3;
                tabSize = (tabSize > FULL_TAB_SIZE) ? FULL_TAB_SIZE : tabSize;
                tabSize = (tabSize < 15+TAB_X_SIZE) ? 15+TAB_X_SIZE : tabSize;
        }
        rcTab.left = l;

        for (i = 0; i < numTabs; i++) {
                Tab far *tab = TabFromIdx(i);
                int ts = tabSize;
                BOOL subover = FALSE;
                BOOL selected = (i+1==g_selectedTab);
                if (selected) ts = FULL_TAB_SIZE;
                rcTab.top = rc->top;
                rcTab.right = rcTab.left + ts;
                rcTab.bottom = bottom+1;

                if (! drawnSelected && rcTab.right-l > rc->right-45-FULL_TAB_SIZE && ! selected)        continue;
                else if (drawnSelected && rcTab.right-l > rc->right-45 && ! selected)   continue;
                
                if (selected) drawnSelected = TRUE;
                
#ifdef WIN3_1
//      rcTab.top -= 5;
#endif          

//#ifndef WIN3_1
                if (! selected) {
                        rcTab.top -= 4;
                        if (! OverTab(&rcTab, *mousecoords)) rcTab.top+=4;
                        else subover = TRUE;
                }
                else {
                        rcTab.top-=4;
                }
//#endif

                if (overtab && OverTab(&rcTab, *mousecoords)) *overtab = (i+1);
                if (overx && OverTabX(&rcTab, *mousecoords)) *overx = TRUE;

                //rcTab.bottom = (rc->top + rc->bottom) - rcTab.top;


                if (draw) drawTab(hWnd, hDC, &rcTab, tab->szTitle, selected, FALSE, OverTab(&rcTab, *mousecoords), OverTabX(&rcTab, *mousecoords));
#if 0
                switch (i) {
                        case 0:
                                if (draw) drawTab(hWnd, hDC, &rcTab, "Internet Voyager", selected, FALSE, OverTab(&rcTab, *mousecoords), OverTabX(&rcTab, *mousecoords));
                                break;
                        default:
                                if (draw) drawTab(hWnd, hDC, &rcTab, "New Tab", selected, FALSE, OverTab(&rcTab, *mousecoords), OverTabX(&rcTab, *mousecoords));
                                break;
                }
#endif
//#ifndef WIN3_1
                if (subover) {                  
                        rcTab.top+=4;
                }
                else if (selected) {
                        rcTab.top+=4;
                }
//#endif

#ifdef WIN3_1
        //rcTab.top += 5;
#endif  

                rcTab.left += ts + 3;
        }

        //rcTab.top += 5;
        //rcTab.left = rc->right - 36;
        //rcTab.left += 6;
        //rcTab.top-=4;
        rcTab.right = rcTab.left + 44 - 1;
        
        
        suboveradd = FALSE;
        rcTab.top -= 4;
        if (overtab && OverTab(&rcTab, *mousecoords)) {
                *overtab = 0;
                suboveradd = TRUE;
        }
        else if (! overtab && OverTab(&rcTab, *mousecoords)) {
                suboveradd = TRUE;
        }
        else {
                rcTab.top += 4;
                suboveradd = FALSE;
        }
        
        if (draw) drawTab(hWnd, hDC, &rcTab, "+", FALSE, TRUE, OverTab(&rcTab, *mousecoords), FALSE);

        if (suboveradd) rcTab.top += 4;

}

#define DBLBUFF 1

LRESULT  CALLBACK BrowserInnerShellProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        static int fontHeight = DEF_FONT_HEIGHT;
        static HFONT hAddrBarFont = NULL;
        static HWND hToggleBar;
        static int tabpadding = 5;
#ifdef WIN3_1   
        static int padding = 8;
#else
        static int padding = 12;
#endif
        static POINT mousecoords = { -1, -1 };
        static int overtab = -1;
        static BOOL overx = FALSE;

        switch (msg) {
                case WM_CREATE:
                        {
                                RECT rc;
                                Tab far *tab;

                                GetClientRect(hWnd, &rc);
#ifdef WIN3_1
                                hAddressBar = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE, -1, 0, rc.right+2, fontHeight, hWnd, NULL, g_hInstance, NULL);                    
#else   
                                hAddressBar = CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE, -1, 0, rc.right+2, fontHeight, hWnd, NULL, g_hInstance, NULL);                       
#endif

                                hStatusBar = hToggleBar = CreateWindow("VOYAGER_SHELL_TOGGLEBAR", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 0, 0, 1, 1, hWnd, NULL, g_hInstance, NULL);

                                //hTopBrowserWnd = CreateWindow("RICHEDIT50W", "",  WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL, 0, fontHeight, rc.right, rc.bottom-fontHeight, hWnd, NULL, g_hInstance, NULL);
                                //if (! hTopBrowserWnd) {
                                //      hTopBrowserWnd = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL, 0, fontHeight, rc.right, rc.bottom-fontHeight, hWnd, NULL, g_hInstance, NULL);
                                //}
                                //SendMessage(hTopBrowserWnd, EM_SETBKGNDCOLOR, 0, RGB( 0,0,0 ) );
                                //g_TOP_WINDOW.hWnd = hTopBrowserWnd;

                                //
#ifndef GWLP_WNDPROC
                                oldAddressBarProc = (FARPROC)SetWindowLong(hAddressBar, GWL_WNDPROC, (LONG)MakeProcInstance((FARPROC)AddressBarProc, g_hInstance));
#else
                                oldAddressBarProc = (WNDPROC)SetWindowLongPtr(hAddressBar, GWLP_WNDPROC, (LONG_PTR)AddressBarProc);
#endif
                                g_currTabId = genTabId();
                                tab = AllocTab(g_currTabId);
                                CreateTabChildWindows(hWnd, tab);
                                SetActiveTab(tab);
                                OpenUrl(tab, &tab->TOP_WINDOW, tab->TOP_WINDOW.document, g_szDefURL);
                                SetWindowText(hAddressBar, (LPCSTR)g_szDefURL);
                        }
                        return DefWindowProc(hWnd, msg, wParam, lParam);
                case WM_PAINT: {
                        PAINTSTRUCT ps;
                        RECT rc, rcTabBar;
                        HDC hDC = BeginPaint(hWnd, &ps);
                        POINT ptStart;
                        POINT ptEnd;
                        HPEN hbr, hPrevBrush;
#ifdef DBLBUFF
                        HDC backbuffDC;
                        HBITMAP backbuffer;
                        HDC hPrevDC;
                        int savedDC;
#endif
#ifdef WIN3_1                   
                        hbr = CreatePen(PS_SOLID, 1, RGB(60, 60, 60));
#else
                        hbr = CreatePen(PS_SOLID, 1, RGB(60, 60, 60));
#endif
                        hPrevBrush = (HPEN)SelectObject(hDC, hbr);
                        GetClientRect(hWnd, &rc);
#ifdef DBLBUFF
                        hPrevDC = hDC;
                        backbuffDC = CreateCompatibleDC(hDC);
                        backbuffer = CreateCompatibleBitmap(hDC, rc.right, rc.bottom);
                        savedDC = SaveDC(backbuffDC);
                        hDC = backbuffDC;
                        SelectObject(backbuffDC, backbuffer);
                        {
                                HBRUSH bgcol = CreateSolidBrush(GetWinColor());
                                FillRect(hDC, &rc, bgcol);
                                DeleteObject(bgcol);
                        }
#endif
                        ptStart.x = 0;
                        ptEnd.x = rc.right;
//#ifdef WIN3_1                 
//                      ptEnd.y = ptStart.y = fontHeight + fontHeight+4+9-1;
//                      MoveTo(hDC, ptStart.x, ptStart.y);
//#else
                        ptEnd.y = ptStart.y = fontHeight+ fontHeight+(padding*2)+tabpadding;
                        MoveToEx(hDC, ptStart.x, ptStart.y, NULL);
//#endif
                        LineTo(hDC, ptEnd.x, ptEnd.y);
                        SelectObject(hDC, hPrevBrush);
                        DeleteObject(hbr);


#ifdef WIN3_1                   
                        hbr = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
#else
                        hbr = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
#endif
                        hPrevBrush = (HPEN)SelectObject(hDC, hbr);
                        GetClientRect(hWnd, &rc);
                        ptStart.x = 0;
                        ptEnd.x = rc.right;
//#ifdef WIN3_1                 
//                      ptEnd.y = ptStart.y = fontHeight + fontHeight+4+9-2;
//                      MoveTo(hDC, ptStart.x, ptStart.y);
//#else
                        ptEnd.y = ptStart.y = fontHeight+ fontHeight+(padding*2)-1+tabpadding;
                        MoveToEx(hDC, ptStart.x, ptStart.y, NULL);
//#endif
                        LineTo(hDC, ptEnd.x, ptEnd.y);
//#ifdef WIN3_1                 
//                      ptEnd.y = ptStart.y = fontHeight + fontHeight+4+9-3;
//                      MoveTo(hDC, ptStart.x, ptStart.y);
//#else
                        ptEnd.y = ptStart.y = fontHeight+ fontHeight+(padding*2)-2+tabpadding;
                        MoveToEx(hDC, ptStart.x, ptStart.y, NULL);
//#endif
                        LineTo(hDC, ptEnd.x, ptEnd.y);





#ifdef WIN3_1                   
                        ptEnd.y = ptStart.y = 1;
                        MoveTo(hDC, ptStart.x, ptStart.y);
#else
                        ptEnd.y = ptStart.y = 1;
                        MoveToEx(hDC, ptStart.x, ptStart.y, NULL);
#endif
                        //LineTo(hDC, ptEnd.x, ptEnd.y);
#ifdef WIN3_1                   
                        ptEnd.y = 2;
                        MoveTo(hDC, ptStart.x, ptStart.y);
#else
                        ptEnd.y = ptStart.y = 2;
                        MoveToEx(hDC, ptStart.x, ptStart.y, NULL);
#endif
                        //LineTo(hDC, ptEnd.x, ptEnd.y);

                        SelectObject(hDC, hPrevBrush);
                        DeleteObject(hbr);

#ifdef WIN3_1                   
                        hbr = CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
#else
                        hbr = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
#endif
#ifdef WIN3_1                   
                        ptEnd.y = ptStart.y = 0;
                        MoveTo(hDC, ptStart.x, ptStart.y);
#else
                        ptEnd.y = ptStart.y = 0;
                        MoveToEx(hDC, ptStart.x, ptStart.y, NULL);
#endif
                        //LineTo(hDC, ptEnd.x, ptEnd.y);

                        SelectObject(hDC, hPrevBrush);
                        DeleteObject(hbr);

                        rcTabBar.left = padding+6;
//#ifdef WIN3_1                 
//                      rcTabBar.top = padding+3;
//#else
                        rcTabBar.top = padding+5;
//#endif
                        rcTabBar.right = rc.right - padding-6;
                        rcTabBar.bottom = padding + fontHeight - padding - 1 - 5 + tabpadding;

                        drawTabs(hWnd, hDC, &rcTabBar, &mousecoords, TRUE, NULL, NULL);
                        
                        {
                                HBRUSH hFrame;
                                RECT rcAddrBar;
                                hFrame = CreateSolidBrush(RGB(0,0,0)/*GetTabColor(TRUE, FALSE)*/);

                                rcAddrBar.left = padding;
                                rcAddrBar.right = rcAddrBar.left + rc.right-(padding*2);
                                rcAddrBar.top = fontHeight+padding+tabpadding;
                                rcAddrBar.bottom = rcAddrBar.top + fontHeight+2;
                                
                                FillRect(hDC, &rcAddrBar, hFrame);
                                rcAddrBar.left+=1;
                                rcAddrBar.top+=1;
                                rcAddrBar.right-=1;
                                rcAddrBar.bottom-=1;
                                FillRect(hDC, &rcAddrBar, GetStockObject(WHITE_BRUSH));
                        
                                DeleteObject(hFrame);
                        }       
                        

#ifdef DBLBUFF
                        BitBlt(hPrevDC, 0, 0, rc.right, rc.bottom, backbuffDC, 0, 0, SRCCOPY);
                        RestoreDC(backbuffDC, savedDC);
                        DeleteObject(backbuffer);
                        DeleteDC(backbuffDC);
#endif

                        EndPaint(hWnd, &ps);
                        return 0;
                }
                case WM_ERASEBKGND:
#ifdef DBLBUFF
                        return TRUE;
#endif
                        return DefWindowProc(hWnd, msg, wParam, lParam);
                case WM_LBUTTONDOWN: {
                        int currtab;
                        RECT rcTabBar, rc;
                        BOOL curroverx;
                        
#ifdef WIN3_1                   
                        mousecoords = MAKEPOINT(lParam);
#else
                        mousecoords.x = GET_X_LPARAM(lParam);
                        mousecoords.y = GET_Y_LPARAM(lParam);
#endif  
                        
                        GetClientRect(hWnd, &rc);
                        
                        rcTabBar.left = padding+6;
                        rcTabBar.top = padding+5;
                        rcTabBar.right = rc.right - padding-6;
                        rcTabBar.bottom = padding + fontHeight - padding - 1 - 5 + tabpadding;                  
                        drawTabs(hWnd, NULL, &rcTabBar, &mousecoords, FALSE, &currtab, &curroverx);
                        
                        if ((currtab != g_selectedTab || curroverx) && currtab > 0) {
                                if (curroverx) {
                                        //char buff[50];
                                        Tab far *tab = TabFromIdx(currtab-1);
                                        //wsprintf(buff, "%d", currtab);
                                        //MessageBox(hWnd, buff, "", MB_OK);
                                        if (tab) {
                                                g_currTabId = -1;
                                                DestroyWindow(tab->hSource);
                                                FreeTab(tab);
                                        }
                                        if (g_selectedTab > currtab) {
                                                g_selectedTab--;
                                        }
                                        if (g_selectedTab <= 0) g_selectedTab = 1;
                                        //DebugLog("removetab \r\n");
                                        g_numTabs--;
                                        if (g_selectedTab > g_numTabs) g_selectedTab = g_numTabs;
                                        if (g_numTabs <= 0) DestroyWindow(GetParent(hWnd));
                                }
                                else {
                                        g_selectedTab = currtab;
                                }
                                if (g_numTabs > 0) {
                                        Tab far *tab = TabFromIdx(g_selectedTab-1);
                                        if (tab) SetActiveTab(tab);
                                }
                                overtab = -1;
                                InvalidateRect(hWnd, NULL, TRUE);
                        }
                        else if (currtab == 0) {
                                //DebugLog("addtab \r\n");
                                Tab far *tab;
                                tab = AllocTab(genTabId());
                                CreateTabChildWindows(hWnd, tab);
                                SetActiveTab(tab);

                                g_numTabs++;
                                g_selectedTab = g_numTabs;
                                overtab = -1;
                                InvalidateRect(hWnd, NULL, TRUE);
                        }
                        
                        return 0;
                }
                case WM_MOUSEMOVE: {
                        
                        int currtab;
                        RECT rcTabBar, rc;
                        BOOL curroverx;
                        
#ifdef WIN3_1                   
                        mousecoords = MAKEPOINT(lParam);
#else
                        mousecoords.x = GET_X_LPARAM(lParam);
                        mousecoords.y = GET_Y_LPARAM(lParam);
#endif  
                        
                        GetClientRect(hWnd, &rc);
                        
                        rcTabBar.left = padding+6;
                        rcTabBar.top = padding+5;
                        rcTabBar.right = rc.right - padding-6;
                        rcTabBar.bottom = padding + fontHeight - padding - 1 - 5 + tabpadding;                  
                        drawTabs(hWnd, NULL, &rcTabBar, &mousecoords, FALSE, &currtab, &curroverx);
                        
                        if (currtab != overtab || curroverx != overx) {
                                overtab = currtab;
                                overx = curroverx;
                                rcTabBar.top -= 4;
                                rcTabBar.right += rcTabBar.left;
                                rcTabBar.bottom += rcTabBar.top + 5;
                                InvalidateRect(hWnd, &rcTabBar, TRUE);
                        }

#if defined(IDC_HAND)
                        if (overtab >= 0 || curroverx) {
                                SetCursor(LoadCursor(NULL, IDC_HAND));
                        }
                        else {
                                SetCursor(LoadCursor(NULL, IDC_ARROW));
                        }
#endif
                        return 0;
                }
                case WM_MOVE:
                case WM_SIZE: {
                        RECT rc;
                        Tab far *currTab = TabFromId(g_currTabId);
#ifndef WIN3_1                          
                        if (GetDpiForWindow) {
                                UINT dpi = GetDpiForWindow(hWnd);
                                fontHeight = (int)(((float)dpi / (float)3.5)); // 1/3.5 inch
                                if (hAddrBarFont) {
                                        DeleteObject(hAddrBarFont);
                                }
                                //fontheight = -MulDiv(PointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
                                hAddrBarFont = CreateFont(fontHeight-2, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                                OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                                DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
                                SendMessage(hAddressBar, WM_SETFONT, (WPARAM)hAddrBarFont, TRUE);
                                SendMessage(hToggleBar, WM_SETFONT, (WPARAM)hAddrBarFont, TRUE);

                                //GetClientRect(hAddressBar, &rc);
                                fontHeight += 2;
                        }
                        else {
                        if (hAddrBarFont) {
                                        DeleteObject(hAddrBarFont);
                                }
                hAddrBarFont = CreateFont(fontHeight-4, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                                OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                                DEFAULT_PITCH | FF_DONTCARE, "Arial");
                                SendMessage(hAddressBar, WM_SETFONT, (WPARAM)hAddrBarFont, TRUE);
                        }
#else
        if (hAddrBarFont) {
                                        DeleteObject(hAddrBarFont);
                                }
                hAddrBarFont = CreateFont(fontHeight-4, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                                OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                                DEFAULT_PITCH | FF_DONTCARE, "Arial");
                                SendMessage(hAddressBar, WM_SETFONT, (WPARAM)hAddrBarFont, TRUE);
#endif                  
                        GetClientRect(hWnd, &rc);
/*#ifdef WIN3_1
                        {       
                        int t = fontHeight+5;
                        MoveWindow(hAddressBar, 6, t+4, rc.right-12, fontHeight-4, TRUE);
                        t += fontHeight;
                        MoveWindow(hTopBrowserWnd, -1, t+9, rc.right+1, rc.bottom-fontHeight-t-9, TRUE);                                
                        MoveWindow(hToggleBar, -1, rc.bottom-fontHeight, rc.right+2, fontHeight+2, TRUE);
                        }
#else*/
{
                        int t, h;
                        t = fontHeight+padding+1+tabpadding;
#ifdef WIN3_1                   
{
                        MoveWindow(hAddressBar, padding+3, t+2, rc.right-(padding*2)-6, fontHeight-2, TRUE);
}
#else
                        MoveWindow(hAddressBar, padding+3, t+2, rc.right-(padding*2)-6, fontHeight-2, TRUE);
#endif
                        t = fontHeight+ fontHeight+1+(padding*2)+tabpadding-1;
                        h = rc.bottom - t - fontHeight;
//                      MoveWindow(hTopBrowserWnd, 4, fontHeight, rc.right-8, rc.bottom-fontHeight-fontHeight-4, TRUE);                                 
                        //MoveWindow(hToggleBar, 4, rc.bottom-fontHeight, rc.right-8, fontHeight-4, TRUE);
                        t++;
                        MoveWindow(currTab->hSource, 0, t, rc.right, h, TRUE);          
                        MoveWindow(currTab->hConsole, 0, t, rc.right, h, TRUE);         

                        t += h;
                        h = fontHeight + 2;
                        
                        MoveWindow(hToggleBar, -1, t, rc.right+2, h, TRUE);
}
//#endif

                        return 0;
                }
                //case WM_NCHITTEST:
                //      return HTNOWHERE;
                default:
                        return DefWindowProc(hWnd, msg, wParam, lParam);
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
}

int SetSocketBufferSize (SOCKET sd, int option, int size)
{                                          
 /*  int bsize = size, ssize = 0, sor = sizeof (int);

   getsockopt (sd, SOL_SOCKET, option, (LPSTR) & ssize, & sor);

   if (ssize >= size) {
      return ssize;
   } else sor = sizeof (int);
   
   if (setsockopt (sd, SOL_SOCKET, option, (LPSTR) & bsize, sor) != SOCKET_ERROR) {
      if (getsockopt (sd, SOL_SOCKET, option, (LPSTR) & ssize, & sor) != SOCKET_ERROR) {
         if (ssize >= size)
            return (ssize);
      }
   }*/

   return 0;
   //return (option == SO_SNDBUF ? WS_WRITE_SIZE : WS_READ_BUFFER);
} 

LRESULT CALLBACK BrowserShellProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        static HWND innerShell;
        switch (msg) {
                case WM_CREATE: {
                        
                        RECT rc;

                        GetClientRect(hWnd, &rc);
                        
                        //if (TabFromId(g_currTabId)) MessageBox(hWnd, "found tab", "", MB_OK);

                        g_innerShell = innerShell = CreateWindow("VOYAGER_INNERSHELL", "",  WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_BORDER, 0, 0, rc.right, rc.bottom, hWnd, NULL, g_hInstance, NULL);

                        return 0;
                }
                case WM_FSOCKET: {
                        LPARAM ret;
                        SocketStream far *ss;
#ifndef NOTHREADS
        EnterCriticalSection(&sockcs);
#endif
                        GetCustomTaskListDataByPtrField(g_socketsTask, VAR_STREAMS, (char far *)&wParam, offsetof(SocketStream, s), sizeof(SOCKET), &ret);
                        ss = (SocketStream far *)ret;
                        if (! ss) {
#ifndef NOTHREADS
        LeaveCriticalSection(&sockcs);
#endif
                                                        return 0;
                                                }

                        switch (WSAGETSELECTEVENT(lParam))
            {
            case FD_CONNECT:
                 //G_con = 1;
                                 // todo: set status on task to success
                                 SetSocketBufferSize(ss->s, SO_RCVBUF, BUFFER_SIZE-2);
                                 HttpGet((Stream_HTTP far *)ss->stream);
                                 AddCustomTaskVar(ss->stream->task, RUN_TASK_VAR_CONNECT_STATE, CONNECT_STATE_SUCCESS);
                 SetStatusText(ss->stream->window->tab, "Connection successful");
                 break;

            case FD_WRITE:
                 /*if (!G_con) return NULL;
                 MSG("Sendind data...")
                 sprintf(G_out,"%s\r\n",G_name);
                 len = strlen(G_out);
                 send(G_s, (char far *)G_out, len, SO_DONTROUTE);*/
                 break;
            case FD_READ: {
                                char far *buff = (char far *)LocalAlloc(GMEM_FIXED, BUFFER_SIZE); // should be <= READ_CHUNK size
                                //for (;;) {
                                        int ret = recv(ss->s, buff, BUFFER_SIZE-2, 0);
                                        if (ret == SOCKET_ERROR) break;
                                        if (ret > 0) {
                                                BOOL headersParsed = FALSE;
                                                buff[ret] = '\0';
                                                WriteSource(ss->stream->window->tab, buff, ret);
                                                if (! HttpGetChunk((Stream_HTTP far *)ss->stream, buff, ret, &headersParsed)) {
													LPARAM connect_state;
														 GetCustomTaskVar(ss->stream->task, RUN_TASK_VAR_CONNECT_STATE, &connect_state, NULL);
														SetStatusText(ss->stream->window->tab, "Connection closed");
														 if (connect_state != CONNECT_STATE_CONNECTING) {
															 //AddCustomTaskVar(ss->stream->task, RUN_TASK_VAR_CONNECT_STATE, CONNECT_STATE_CLOSED);
														 //MessageBox(browserWin, "closed", "", MB_OK);
																						
															if (ss->s != INVALID_SOCKET) closesocket(ss->s);
															ss->s = INVALID_SOCKET;
														 }
												}
                                                if (headersParsed) {
                                                        
                                                }
                                                //DebugLog(ss->stream->window->tab, "%s", buff);
                                        }
                                //}
                                LocalFree((void far *)buff);
                 /*if (!G_con) return NULL;
                 MSG("Receiving data...")
                 ret = recv(G_s, (char far *)G_query, QUERYLEN, 0);
                 if ( ret==SOCKET_ERROR &&
                      WSAGetLastError()!=WSAEWOULDBLOCK ) CANCEL
                 G_query[ret] = 0;
                 PostMessage(hWnd,WM_QUERYDONE,0,0);*/

                 break;
                        }

            case FD_CLOSE: {
                                 LPARAM connect_state;
                                 GetCustomTaskVar(ss->stream->task, RUN_TASK_VAR_CONNECT_STATE, &connect_state, NULL);
                 SetStatusText(ss->stream->window->tab, "Connection closed");
                                 if (connect_state != CONNECT_STATE_CONNECTING)  {
									 AddCustomTaskVar(ss->stream->task, RUN_TASK_VAR_CONNECT_STATE, CONNECT_STATE_CLOSED);
                                 //MessageBox(browserWin, "closed", "", MB_OK);
                                                                
									if (ss->s != INVALID_SOCKET) closesocket(ss->s);
									ss->s = INVALID_SOCKET;
								 }
                                                                 
                 //G_con = 0;
                 //closesocket(G_s);
                 break;
                        }
            }
                        
                        #ifndef NOTHREADS
                                LeaveCriticalSection(&sockcs);
                        #endif

                        
                        return 0;
                }
                case WM_HOSTRESOLVED: {
                        LPARAM ret;
                        SocketStream far *ss;
                                                
#ifndef NOTHREADS
        EnterCriticalSection(&sockcs);
#endif

                        GetCustomTaskListDataByPtrField(g_socketsTask, VAR_STREAMS, (char far *)&wParam, offsetof(SocketStream, hGetHostname), sizeof(HANDLE), &ret);
                        ss = (SocketStream far *)ret;
                        if (WSAGETASYNCERROR(lParam)) {
                                if (ss) {
                                        // todo: set status on task to failed
                                        GlobalFree((LPVOID)ss->getHostBuf);
                                        AddCustomTaskVar(ss->stream->task, RUN_TASK_VAR_CONNECT_STATE, CONNECT_STATE_ERROR);
                                        SetStatusText(ss->stream->window->tab, "Unable to resolve");
                                }
                                //MessageBox(hWnd, "Unable to resolve", "", MB_OK);
                        #ifndef NOTHREADS
                                LeaveCriticalSection(&sockcs);
                        #endif
                                return 0;
                        }
                        //MessageBox(hWnd, "Resolved", "", MB_OK);
                        if (ss &&  ss->getHostBuf) {
                                int ret;
                                struct hostent far *remote_host;
                                struct sockaddr_in remote_addr;
                                SetStatusText(ss->stream->window->tab, "Host Resolved");
                                remote_host = (struct hostent far *)ss->getHostBuf;
                                remote_addr.sin_family                = AF_INET;
                                remote_addr.sin_addr.S_un.S_un_b.s_b1 = remote_host->h_addr[0];
                                remote_addr.sin_addr.S_un.S_un_b.s_b2 = remote_host->h_addr[1];
                                remote_addr.sin_addr.S_un.S_un_b.s_b3 = remote_host->h_addr[2];
                                remote_addr.sin_addr.S_un.S_un_b.s_b4 = remote_host->h_addr[3];
                                remote_addr.sin_port                  = htons(((Stream_HTTP far *)ss->stream)->stream.url_info->port);
                                ret = connect(ss->s,(struct sockaddr far *) &(remote_addr), sizeof(struct sockaddr));
                                GlobalFree((LPVOID)ss->getHostBuf);
                                                                ss->getHostBuf = NULL;
                                if ( ret==SOCKET_ERROR && WSAGetLastError()!=WSAEWOULDBLOCK ) {
                                        // todo: set status on task to failed
                                        AddCustomTaskVar(ss->stream->task, RUN_TASK_VAR_CONNECT_STATE, CONNECT_STATE_ERROR);
                                        SetStatusText(ss->stream->window->tab, "Connect failed");
                        #ifndef NOTHREADS
                                LeaveCriticalSection(&sockcs);
                        #endif
                                        return 0;
                                }
                                SetStatusText(ss->stream->window->tab, "Connecting...");
                        }
                        #ifndef NOTHREADS
                                LeaveCriticalSection(&sockcs);
                        #endif
                        return 0;
                }
                case WM_SIZE: {
                        RECT rc;
                        GetClientRect(hWnd, &rc);
                        MoveWindow(innerShell, 0, 0, rc.right-0, rc.bottom-0, TRUE);
                        return 0;
                }
                case WM_DESTROY:
                        PostQuitMessage(0);
                        return 0;
                default:
                        return DefWindowProc(hWnd, msg, wParam, lParam);
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
}


LRESULT CALLBACK BrowserProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        return DefWindowProc(hWnd, msg, wParam, lParam);
}

#ifndef NOTHREADS
DWORD WINAPI workerProc(LPVOID unused) {
        Task far *currTask;

        for (;;) {
                if (GetNextTask(&currTask)) {
                        RunTask(currTask);
                }
                else {
                        Sleep(1);
                }
        }
        return 0;
}
#endif

int WinsockStart() {
        WSADATA  wsaData;
    if ( WSAStartup( 0x0101 , &wsaData ) ) { return FALSE; }
        return TRUE;
}
