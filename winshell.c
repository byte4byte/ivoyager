
#include "tabs.c"

#ifdef WIN3_1
static FARPROC oldAddressBarProc;
#else
static WNDPROC oldAddressBarProc;
#endif

BOOL CreateTabChildWindows(HWND parent, HWND *hPage, HWND *hSource, HWND *hConsole) {
	*hSource = CreateWindow("RICHEDIT50W", "",  WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parent, NULL, g_hInstance, NULL);
	if (! *hSource) {
		*hSource = CreateWindow("EDIT", "", WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parent, NULL, g_hInstance, NULL);
	}
	return TRUE;
}

BOOL SetActiveTab(Tab far *tab) {
	Tab far *currTab = TabFromId(g_currTabId);
	//RECT rc;
	HWND shell;
	if (currTab) {
		ShowWindow(currTab->hSource, SW_HIDE);
	}
	ShowWindow(tab->hSource, SW_SHOW);
	g_currTabId = tab->id;
	shell = GetParent(tab->hSource);
	SendMessage(shell, WM_SIZE, 0, 0L);
	//GetWindowRect(shell, &rc);
	//SetWindowPos(shell, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER);
}

COLORREF GetTabColor(BOOL selected, BOOL special, BOOL over) {
	if (over) return selected ? RGB(162, 62, 52) : RGB(22, 22, 192);
	return (selected ? RGB(182, 82, 72) : (special ? RGB(0, 0, 0) : RGB(62, 62, 62)));
}

COLORREF GetTabShineColor(BOOL selected, BOOL special, BOOL over) {
	return (selected ? RGB(255, 245, 245) : (special ? RGB(255, 255, 255) : RGB(255, 255, 255)));
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
				url = (LPSTR)GlobalAlloc(GMEM_FIXED, len+2);
				url[len] = '\0';
				GetWindowText(hWnd, url, len+1);
				tab = TabFromId(g_currTabId);
				OpenUrl(tab, &tab->TOP_WINDOW, url);
				GlobalFree((HGLOBAL)url);
				return FALSE;
			}
			break;
	}
	return CallWindowProc(oldAddressBarProc, hWnd, msg, wParam, lParam);
}

#ifndef WIN3_1
typedef UINT WINAPI (* lpGetDpiForWindow)(HWND hWnd);
lpGetDpiForWindow GetDpiForWindow = NULL;
#endif

#define DEF_FONT_HEIGHT 25
#define DEF_DPI_FONT_HEIGHT 14

LRESULT CALLBACK BrowserShellToggleBar(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static HWND hSource = NULL, hPage = NULL, hConsole = NULL;
	static HFONT hToggleFont;
	static HFONT hToggleFontBold;
	switch (msg) {
		case WM_CREATE: {
			

			hSource = CreateWindow("BUTTON", "Source", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, (HMENU)1, g_hInstance, NULL);
			hPage = CreateWindow("BUTTON", "Page", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, (HMENU)2, g_hInstance, NULL);
			hConsole = CreateWindow("BUTTON", "Console", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, (HMENU)3, g_hInstance, NULL);
			
			//SendMessage(hSource, WM_SETFONT, (WPARAM)hFont, TRUE);
			//SendMessage(hPage, WM_SETFONT, (WPARAM)hFont, TRUE);
			//SendMessage(hConsole, WM_SETFONT, (WPARAM)hFont, TRUE);
			
			//SendMessage(hSource, BM_SETCHECK, BST_CHECKED, 0);
			
			return 0;
		}
		case WM_SETCURSOR:
#ifndef WIN3_1		
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
		
			if (wParam == 1) {
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
				case 1:
					SetTextColor(di->hDC, RGB(200, 200, 200));
					di->rcItem.left += 2;
					di->rcItem.top += 2;
					DrawText(di->hDC, "Source", 6, &di->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
					SetTextColor(di->hDC, RGB(0, 0, 0));
					di->rcItem.left -= 2;
					di->rcItem.top -= 2;
					DrawText(di->hDC, "Source", 6, &di->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE );	
					break;
				case 2:
					SetTextColor(di->hDC, RGB(44, 44, 44));
					DrawText(di->hDC, "Page", 4, &di->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
					break;					
				case 3:
					SetTextColor(di->hDC, RGB(44, 44, 44));
					DrawText(di->hDC, "Console", 7, &di->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
					break;					
			}
			SelectObject(di->hDC, hPrevFont);
			
			DeleteObject(hToggleFont);
			DeleteObject(hToggleFontBold);
			return TRUE;
		}
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
			
			if (! lpszStatus) return DefWindowProc(hWnd, msg, wParam, lParam);
			
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

	int l = rc->left;
	int tabSize = 165;
	int i;
	BOOL drawnSelected = FALSE;
	BOOL suboveradd;
	
	if (overtab) *overtab = -1;
	if (overx) *overx = FALSE;
	
	#define FULL_TAB_SIZE 230

	if (g_numTabs-1 > 0) {
		tabSize = (rc->right-l-45-FULL_TAB_SIZE) / (g_numTabs-1);
		//tabSize = tabSize + ((5-(tabSize % 5)) % 5);
		tabSize -= 3;
		tabSize = (tabSize > FULL_TAB_SIZE) ? FULL_TAB_SIZE : tabSize;
		tabSize = (tabSize < 15+TAB_X_SIZE) ? 15+TAB_X_SIZE : tabSize;
	}
	rcTab.left = l;

	for (i = 0; i < g_numTabs; i++) {
		int ts = tabSize;
		BOOL subover = FALSE;
		BOOL selected = (i+1==g_selectedTab);
		if (selected) ts = FULL_TAB_SIZE;
		rcTab.top = rc->top;
		rcTab.right = rcTab.left + ts;
		rcTab.bottom = bottom+1;

		if (! drawnSelected && rcTab.right-l > rc->right-45-FULL_TAB_SIZE && ! selected)	continue;
		else if (drawnSelected && rcTab.right-l > rc->right-45 && ! selected)	continue;
		
		if (selected) drawnSelected = TRUE;
		
#ifdef WIN3_1
//	rcTab.top -= 5;
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

		switch (i) {
			case 0:
				if (draw) drawTab(hWnd, hDC, &rcTab, "Internet Voyager", selected, FALSE, OverTab(&rcTab, *mousecoords), OverTabX(&rcTab, *mousecoords));
				break;
			default:
				if (draw) drawTab(hWnd, hDC, &rcTab, "New Tab", selected, FALSE, OverTab(&rcTab, *mousecoords), OverTabX(&rcTab, *mousecoords));
				break;
		}
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
				HWND hSource, hPage, hConsole;
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
				//	hTopBrowserWnd = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL, 0, fontHeight, rc.right, rc.bottom-fontHeight, hWnd, NULL, g_hInstance, NULL);
				//}
				//SendMessage(hTopBrowserWnd, EM_SETBKGNDCOLOR, 0, RGB( 0,0,0 ) );
				//g_TOP_WINDOW.hWnd = hTopBrowserWnd;

				//
#ifdef WIN3_1
				oldAddressBarProc = (FARPROC)SetWindowLong(hAddressBar, GWL_WNDPROC, (LONG)MakeProcInstance((FARPROC)AddressBarProc, g_hInstance));
#else
				oldAddressBarProc = (WNDPROC)SetWindowLongPtr(hAddressBar, GWLP_WNDPROC, (LONG_PTR)AddressBarProc);
#endif
				g_currTabId = genTabId();
				tab = AllocTab(g_currTabId);
				CreateTabChildWindows(hWnd, &hPage, &hSource, &hConsole);
				tab->hSource = hSource;
				SetActiveTab(tab);
				OpenUrl(tab, &tab->TOP_WINDOW, g_szDefURL);
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
#ifdef WIN3_1			
			hbr = CreatePen(PS_SOLID, 1, RGB(60, 60, 60));
#else
			hbr = CreatePen(PS_SOLID, 1, RGB(60, 60, 60));
#endif
			hPrevBrush = (HPEN)SelectObject(hDC, hbr);
			GetClientRect(hWnd, &rc);
			ptStart.x = 0;
			ptEnd.x = rc.right;
//#ifdef WIN3_1			
//			ptEnd.y = ptStart.y = fontHeight + fontHeight+4+9-1;
//			MoveTo(hDC, ptStart.x, ptStart.y);
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
//			ptEnd.y = ptStart.y = fontHeight + fontHeight+4+9-2;
//			MoveTo(hDC, ptStart.x, ptStart.y);
//#else
			ptEnd.y = ptStart.y = fontHeight+ fontHeight+(padding*2)-1+tabpadding;
			MoveToEx(hDC, ptStart.x, ptStart.y, NULL);
//#endif
			LineTo(hDC, ptEnd.x, ptEnd.y);
//#ifdef WIN3_1			
//			ptEnd.y = ptStart.y = fontHeight + fontHeight+4+9-3;
//			MoveTo(hDC, ptStart.x, ptStart.y);
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
			LineTo(hDC, ptEnd.x, ptEnd.y);
#ifdef WIN3_1			
			ptEnd.y = 2;
			MoveTo(hDC, ptStart.x, ptStart.y);
#else
			ptEnd.y = ptStart.y = 2;
			MoveToEx(hDC, ptStart.x, ptStart.y, NULL);
#endif
			LineTo(hDC, ptEnd.x, ptEnd.y);

			SelectObject(hDC, hPrevBrush);
			DeleteObject(hbr);

#ifdef WIN3_1			
			hbr = CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
#else
			hbr = CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
#endif
#ifdef WIN3_1			
			ptEnd.y = ptStart.y = 0;
			MoveTo(hDC, ptStart.x, ptStart.y);
#else
			ptEnd.y = ptStart.y = 0;
			MoveToEx(hDC, ptStart.x, ptStart.y, NULL);
#endif
			LineTo(hDC, ptEnd.x, ptEnd.y);

			SelectObject(hDC, hPrevBrush);
			DeleteObject(hbr);

			rcTabBar.left = padding+6;
//#ifdef WIN3_1			
//			rcTabBar.top = padding+3;
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

			EndPaint(hWnd, &ps);
			return 0;
		}	
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
				HWND hPage, hConsole, hSource;
				tab = AllocTab(genTabId());
				CreateTabChildWindows(hWnd, &hPage, &hSource, &hConsole);
				tab->hSource = hSource;
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
				InvalidateRect(hWnd, NULL, TRUE);
			}

#ifndef WIN3_1
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
			MoveWindow(hAddressBar, padding+3, t+5, rc.right-(padding*2)-6, fontHeight-5, TRUE);
#else
			MoveWindow(hAddressBar, padding+3, t+2, rc.right-(padding*2)-6, fontHeight-2, TRUE);
#endif
			t = fontHeight+ fontHeight+1+(padding*2)+tabpadding-1;
			h = rc.bottom - t - fontHeight;
//			MoveWindow(hTopBrowserWnd, 4, fontHeight, rc.right-8, rc.bottom-fontHeight-fontHeight-4, TRUE);					
			//MoveWindow(hToggleBar, 4, rc.bottom-fontHeight, rc.right-8, fontHeight-4, TRUE);
			t++;
			MoveWindow(currTab->hSource, 0, t, rc.right, h, TRUE);		

			t += h;
			h = fontHeight + 2;
			
			MoveWindow(hToggleBar, -1, t, rc.right+2, h, TRUE);
}
//#endif

			return 0;
		}
		//case WM_NCHITTEST:
		//	return HTNOWHERE;
		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK BrowserShellProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static HWND innerShell;
	switch (msg) {
		case WM_CREATE: {
			
			RECT rc;

			GetClientRect(hWnd, &rc);
			
			//if (TabFromId(g_currTabId)) MessageBox(hWnd, "found tab", "", MB_OK);

			innerShell = CreateWindow("VOYAGER_INNERSHELL", "",  WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_BORDER, 0, 0, rc.right, rc.bottom, hWnd, NULL, g_hInstance, NULL);

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