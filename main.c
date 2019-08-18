#include "ivoyager.h"
#include "utils.c"
#include "task.h"
#include "url.c"
#include "dom.h"
#include <stdio.h>
#ifdef WIN3_1
#include <wing.h>
#include <stdarg.h>
#endif
#ifndef WIN3_1
#include <windowsx.h>
#include <commctrl.h>
#include <richedit.h>
#endif

static HWND hAddressBar, hTopBrowserWnd;
static HINSTANCE g_hInstance;

void ResetDebugLogAttr() {
	DebugLogAttr(FALSE, FALSE, RGB(0,0,0));
}

#ifdef WIN3_1
void DebugLogAttr(BOOL bold, BOOL italic, COLORREF color) {

}
#else
void DebugLogAttr(BOOL bold, BOOL italic, COLORREF color) {
	CHARFORMAT cf;

	memset(&cf,0,sizeof(cf));
	cf.cbSize = sizeof(cf);

	DWORD dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC;
	DWORD dwEffect = 0;
	if (bold) {
		dwEffect |= CFE_BOLD;
	}
	if (italic) {
		dwEffect |= CFE_ITALIC;
	}
	cf.crTextColor = color;
	cf.dwMask = dwMask;
	cf.dwEffects = dwEffect;

	SendMessage(hTopBrowserWnd,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);
}
#endif

void DebugLog(LPSTR format, ...) {
	int ndx;

	va_list args;
	int     len;
	char    *buffer;

	// retrieve the variable arguments
	va_start( args, format );

#ifndef WIN3_1
	len = _vscprintf( format, args ) // _vscprintf doesn't count
								+ 1; // terminating '\0'
#else
	len = 1024;
#endif

	buffer = (char*)malloc( len * sizeof(char) );

	wvsprintf( buffer, format, args ); // C4996

	ndx = GetWindowTextLength (hTopBrowserWnd);
	SetFocus(hTopBrowserWnd);
	#ifdef WIN32
	  SendMessage(hTopBrowserWnd, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
	#else
	  SendMessage(hTopBrowserWnd, EM_SETSEL, 0, MAKELONG (ndx, ndx));
	#endif
	SendMessage(hTopBrowserWnd, EM_REPLACESEL, 0, (LPARAM) ((LPSTR) buffer));

	free(buffer);
	va_end(args);
}

LPSTR lpszStatus =  NULL;
HWND hStatusBar = NULL;

void SetStatusText(LPSTR format, ...) {
	
	
	int ndx;

	va_list args;
	int     len;
	
	if (lpszStatus) GlobalFree((HGLOBAL)lpszStatus);

	// retrieve the variable arguments
	va_start( args, format );

#ifndef WIN3_1
	len = _vscprintf( format, args ) // _vscprintf doesn't count
								+ 1; // terminating '\0'
#else
	len = 1024;
#endif

	lpszStatus = (LPSTR)GlobalAlloc(GMEM_FIXED, len * sizeof(char) );

	wvsprintf( lpszStatus, format, args ); // C4996

	va_end(args);	
	
	if (hStatusBar) InvalidateRect(hStatusBar, NULL, TRUE);
}

ContentWindow g_TOP_WINDOW;
static LPSTR g_szDefURL = "ivoyager.online";

typedef struct DownloadFileTaskParams {
	LPSTR  url;
	ContentWindow far *window;
} DownloadFileTaskParams;

typedef struct ParseHtmlTaskParams {
	LPSTR  html_buff_start;
	int len;
} ParseHtmlTaskParams;

typedef struct ParseCSSTaskParams {
	LPSTR  css_buff_start;
	int len;
} ParseCSSTaskParams;

#define TASK_LOADURL 1

BOOL  RunOpenUrlTask(Task far *task) {
	typedef struct {
		URL_INFO far *url_info;
		FILE *fp;
	} OPEN_URL_DATA;
	
	typedef struct {
		char read_buff[1024];  // min 2
		int len;
		BOOL eof;
	} READ_CHUNK;
	
	
	OPEN_URL_DATA far *open_url_data;
	BOOL ret = FALSE;
	LPARAM state;
		
	#define RUN_TASK_VAR_STATE   0
	#define RUN_TASK_VAR_DATA  	 1
	#define RUN_TASK_VAR_CHUNKS  2
	
	#define RUN_TASK_STATE_OPEN_STREAM 0
	#define RUN_TASK_STATE_READ_STREAM 1
	#define RUN_TASK_STATE_PARSE_DOM   2
	
	GetCustomTaskVar(task, RUN_TASK_VAR_STATE, &state, NULL);
	GetCustomTaskVar(task, RUN_TASK_VAR_DATA, (LPARAM far *)&open_url_data, NULL);
	switch (state) {
		case RUN_TASK_STATE_OPEN_STREAM:
		{
			URL_INFO  far *url_info = GetUrlInfo(((DownloadFileTaskParams far *)task->params)->url);
			if (! url_info) {
				ret = TRUE;
				break;
			}
			SetWindowText(hTopBrowserWnd, "");
			switch (url_info->protocol) {
				case HTTP_PROTOCOL: 
				{
					SetStatusText("Connecting to: \"%s\"", url_info->domain);
					ret = TRUE;
					break;
				}
				case FILE_PROTOCOL:
				{
					FILE *fp;
					//MessageBox(g_TOP_WINDOW.hWnd, url_info->path, "", MB_OK);
					SetStatusText("Opening: \"%s\"", url_info->path);
					fp = _ffopen(url_info->path, "rb");
					if (! fp) { // 404
						//MessageBox(g_TOP_WINDOW.hWnd, "404 ERROR", "", MB_OK);
						SetStatusText("\"%s\" - 404 Error", url_info->path);
						ret = TRUE;
						break;
					}
					else {
						SetStatusText("Opened: \"%s\"", url_info->path);
					}
					
					open_url_data = (OPEN_URL_DATA far *)GlobalAlloc(GMEM_FIXED, sizeof(OPEN_URL_DATA));
					_fmemset(open_url_data, 0, sizeof(OPEN_URL_DATA));
					
					open_url_data->fp = fp;
					open_url_data->url_info = url_info;
					
					AddCustomTaskVar(task, RUN_TASK_VAR_STATE, RUN_TASK_STATE_READ_STREAM);
					AddCustomTaskVar(task, RUN_TASK_VAR_DATA, (LPARAM)open_url_data);
					break;
				}
				default:
					ret = TRUE;
					break;
			}
			break;
		}
		case RUN_TASK_STATE_READ_STREAM:
		{
			int addidx;
			READ_CHUNK near *read_chunk = (READ_CHUNK near *)LocalAlloc(LMEM_FIXED, sizeof(READ_CHUNK));
			read_chunk->len = fread(read_chunk->read_buff, 1, sizeof(read_chunk->read_buff)-1, open_url_data->fp);
			read_chunk->eof = feof(open_url_data->fp);
			if (read_chunk->len <= 0) {
				SetStatusText("Done: \"%s\"", open_url_data->url_info->path);
				LocalFree((HGLOBAL)read_chunk);
				AddCustomTaskVar(task, RUN_TASK_VAR_STATE, RUN_TASK_STATE_PARSE_DOM);
			}
			else {	
				SetStatusText("Reading: \"%s\"", open_url_data->url_info->path);
				addidx = AddCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, (LPARAM)read_chunk);
			}
			//break;
		}
		case RUN_TASK_STATE_PARSE_DOM: {
			LPARAM read_chunk_ptr;
			READ_CHUNK far *read_chunk;
			char far *ptr;

			if (! GetCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, 0, &read_chunk_ptr)) {
				ret = TRUE;
				break;
			}
			read_chunk = (READ_CHUNK far *)read_chunk_ptr;
			ptr = read_chunk->read_buff;
			
			if (ParseDOMChunk(&g_TOP_WINDOW, task, (char far **)&ptr, read_chunk->len, read_chunk->eof)) {
				
				LocalFree((HLOCAL)read_chunk);
				RemoveCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, 0);
			}
			
			break;
		}
	}
	
	//task->dwNextRun = (GetTickCount() + 5000);
	
	if (ret) { // free data
		//MessageBox(g_TOP_WINDOW.hWnd, "free", "", MB_OK);
		if (open_url_data) {
			if (open_url_data->fp) fclose(open_url_data->fp);
			FreeUrlInfo(open_url_data->url_info);
			GlobalFree((HGLOBAL)open_url_data);
		}
		GlobalFree((HGLOBAL)((DownloadFileTaskParams far *)task->params)->url);
		GlobalFree((HGLOBAL)task->params);
	}
	return ret;
}


BOOL  RunTask(Task far *task) {
	BOOL ret = TRUE;	
	// return true if task completed
	// return false to be scheduled to run again later for continous tasks
	switch (task->type) {
		case TASK_LOADURL:
			ret = RunOpenUrlTask(task);
			break;
	}
	if (ret) FreeTask(task);
	else task->locked = FALSE;
	return ret;
}

void  OpenUrl(ContentWindow  far *window, LPSTR  url) {
	// TODO: Clear out window

	DownloadFileTaskParams far *params;
	Task far *loadUrlTask;
	
	loadUrlTask = (Task  far *)GlobalAlloc(GMEM_FIXED, sizeof(Task));
	_fmemset(loadUrlTask, 0, sizeof(Task));
	loadUrlTask->type = TASK_LOADURL;

	params = (DownloadFileTaskParams far *)GlobalAlloc(GMEM_FIXED, sizeof(DownloadFileTaskParams));
	params->url = (LPSTR)GlobalAlloc(GMEM_FIXED, lstrlen(url)+1);
	params->window = window;
	
	lstrcpy(params->url, url);

	loadUrlTask->params = (LPVOID)params;

	AddTask(loadUrlTask);
	
	#include "tests/taskdata.c"
}

#ifdef WIN3_1
static FARPROC oldAddressBarProc;
#else
static WNDPROC oldAddressBarProc;
#endif

COLORREF GetTabColor(BOOL selected, BOOL special, BOOL over) {
	if (over) return selected ? RGB(162, 62, 52) : RGB(72, 82, 192);
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
				int len = GetWindowTextLength(hWnd);
				url = (LPSTR)GlobalAlloc(GMEM_FIXED, len+2);
				url[len] = '\0';
				GetWindowText(hWnd, url, len+1);
				OpenUrl(&g_TOP_WINDOW, url);
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
	static HWND hSource, hPage, hConsole;
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

void drawTab(HWND hWnd, HDC hDC, LPRECT rc, LPSTR szText, BOOL selected, BOOL special, BOOL mouseover) {
	HPEN hpen;
	HPEN hPrevBrush;
	static HFONT hToggleFontBold = NULL;
	static HFONT hToggleFont = NULL;
	HBRUSH hbr = CreateSolidBrush(GetTabColor(selected, special, mouseover));


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

static int g_selectedTab = 1;

void drawTabs(HWND hWnd, HDC hDC, LPRECT rc, LPPOINT mousecoords, BOOL draw, int *overtab) {
	RECT rcTab;

	#define NUM_TABS 4

	int bottom = rc->top + rc->bottom;

	int l = rc->left;
	int tabSize = 165;
	int i;
	
	if (overtab) *overtab = -1;
	
	tabSize = (rc->right-50) / (NUM_TABS+1);
	tabSize = (tabSize > 230) ? 230 : tabSize;
	tabSize = (tabSize < 15) ? 15 : tabSize;
	rcTab.left = l;

	for (i = 0; i < NUM_TABS; i++) {
		BOOL selected = (i+1==g_selectedTab);
		rcTab.top = rc->top;
		rcTab.right = rcTab.left + tabSize;
		rcTab.bottom = bottom+1;

		if (rcTab.right > rc->right-50)	break;
		
#ifdef WIN3_1
//	rcTab.top -= 5;
#endif		

//#ifndef WIN3_1
		if (! selected) {
			//rcTab.top-=5;
		}
		else {
			rcTab.top-=4;
		}
//#endif

		if (overtab && OverTab(&rcTab, *mousecoords)) *overtab = (i+1);

		//rcTab.bottom = (rc->top + rc->bottom) - rcTab.top;

		switch (i) {
			case 0:
				if (draw) drawTab(hWnd, hDC, &rcTab, "Internet Voyager", selected, FALSE, OverTab(&rcTab, *mousecoords));
				break;
			case 1:
				if (draw) drawTab(hWnd, hDC, &rcTab, "MSN.com", selected, FALSE, OverTab(&rcTab, *mousecoords));
				break;
			case 2:
				if (draw) drawTab(hWnd, hDC, &rcTab, "Google", selected, FALSE, OverTab(&rcTab, *mousecoords));
				break;
			case 3:
				if (draw) drawTab(hWnd, hDC, &rcTab, "Nintendo", selected, FALSE, OverTab(&rcTab, *mousecoords));
				break;
			default:
				if (draw) drawTab(hWnd, hDC, &rcTab, "Tab", selected, FALSE, OverTab(&rcTab, *mousecoords));
				break;
		}
//#ifndef WIN3_1
		if (! selected) {			
			//rcTab.top+=5;
		}
		else {
			rcTab.top+=4;
		}
//#endif

#ifdef WIN3_1
	//rcTab.top += 5;
#endif	

		rcTab.left += tabSize + 3;
	}

	//rcTab.top += 5;
	//rcTab.left = rc->right - 36;
	//rcTab.left += 6;
	//rcTab.top-=4;
	rcTab.right = rcTab.left + 44 - 1;
	
	if (overtab && OverTab(&rcTab, *mousecoords)) *overtab = 0;

	if (draw) drawTab(hWnd, hDC, &rcTab, "+", FALSE, TRUE, OverTab(&rcTab, *mousecoords));

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

	switch (msg) {
		case WM_CREATE:
			{
				RECT rc;

				GetClientRect(hWnd, &rc);
#ifdef WIN3_1
				hAddressBar = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE, -1, 0, rc.right+2, fontHeight, hWnd, NULL, g_hInstance, NULL);			
#else	
				hAddressBar = CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE, -1, 0, rc.right+2, fontHeight, hWnd, NULL, g_hInstance, NULL);			
#endif

				hStatusBar = hToggleBar = CreateWindow("VOYAGER_SHELL_TOGGLEBAR", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 0, 0, 1, 1, hWnd, NULL, g_hInstance, NULL);

				hTopBrowserWnd = CreateWindow("RICHEDIT50W", "",  WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL, 0, fontHeight, rc.right, rc.bottom-fontHeight, hWnd, NULL, g_hInstance, NULL);
				if (! hTopBrowserWnd) {
					hTopBrowserWnd = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL, 0, fontHeight, rc.right, rc.bottom-fontHeight, hWnd, NULL, g_hInstance, NULL);
				}
				//SendMessage(hTopBrowserWnd, EM_SETBKGNDCOLOR, 0, RGB( 0,0,0 ) );
				g_TOP_WINDOW.hWnd = hTopBrowserWnd;

				OpenUrl(&g_TOP_WINDOW, g_szDefURL);
				SetWindowText(hAddressBar, (LPCSTR)g_szDefURL);
#ifdef WIN3_1
				oldAddressBarProc = (FARPROC)SetWindowLong(hAddressBar, GWL_WNDPROC, (LONG)MakeProcInstance((FARPROC)AddressBarProc, g_hInstance));
#else
				oldAddressBarProc = (WNDPROC)SetWindowLongPtr(hAddressBar, GWLP_WNDPROC, (LONG_PTR)AddressBarProc);
#endif
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

			drawTabs(hWnd, hDC, &rcTabBar, &mousecoords, TRUE, NULL);
			
			{
				HBRUSH hFrame;
				RECT rcAddrBar;
				hFrame = CreateSolidBrush(RGB(0,0,0)/*GetTabColor(TRUE, FALSE)*/);

/*#ifdef WIN3_1
				rcAddrBar.left = 4;
				rcAddrBar.right = rcAddrBar.left + rc.right-8;
				rcAddrBar.top = fontHeight+4;
				rcAddrBar.bottom = rcAddrBar.top + fontHeight+2;
#else			*/
				rcAddrBar.left = padding;
				rcAddrBar.right = rcAddrBar.left + rc.right-(padding*2);
				rcAddrBar.top = fontHeight+padding+tabpadding;
				rcAddrBar.bottom = rcAddrBar.top + fontHeight+2;
//#endif
				
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
			drawTabs(hWnd, NULL, &rcTabBar, &mousecoords, FALSE, &currtab);
			
			if (currtab != g_selectedTab && currtab > 0) {
				g_selectedTab = currtab;
				overtab = -1;
				InvalidateRect(hWnd, NULL, TRUE);
			}
			
			return 0;
		}
		case WM_MOUSEMOVE: {
			
			int currtab;
			RECT rcTabBar, rc;
			
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
			drawTabs(hWnd, NULL, &rcTabBar, &mousecoords, FALSE, &currtab);
			
			if (currtab != overtab) {
				overtab = currtab;
				InvalidateRect(hWnd, NULL, FALSE);
			}
#ifndef WIN3_1
			if (overtab >= 0) {
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
			MoveWindow(hTopBrowserWnd, 0, t, rc.right, h, TRUE);		

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

int pascal WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	static WNDCLASS wc;
	static MSG msg;
	HWND browserWin;
	int i = 0;
	HICON icon = LoadIcon(hInstance, IDI_APPLICATION);
	
#ifdef NOTHREADS
	Task far *currTask;
#else
	DWORD threadId;
#endif

#ifndef WIN3_1
HMODULE hShCore = LoadLibrary("shcore.dll");
	HMODULE user32dll = GetModuleHandle(TEXT("user32.dll"));
	GetDpiForWindow = (lpGetDpiForWindow)GetProcAddress(user32dll, "GetDpiForWindow");
	if (hShCore) {
		typedef HRESULT WINAPI (*lpSetProcessDpiAwareness)(int value);
		enum PROCESS_DPI_AWARENESS {
		  PROCESS_DPI_UNAWARE,
		  PROCESS_SYSTEM_DPI_AWARE,
		  PROCESS_PER_MONITOR_DPI_AWARE
		};
		lpSetProcessDpiAwareness SetProcessDpiAwareness = (lpSetProcessDpiAwareness)GetProcAddress(hShCore, "SetProcessDpiAwareness");
		if (SetProcessDpiAwareness) {
			SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);	
		}
	}
#endif

	g_hInstance = hInstance;

	_fmemset(&g_TOP_WINDOW, 0, sizeof(ContentWindow));
	InitTaskSystem();
#ifndef WIN3_1
	LoadLibrary(TEXT("Msftedit.dll"));
	InitCommonControls();
#endif

	#include "tests/path.c"

	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground =  GetStockObject(WHITE_BRUSH);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = NULL;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = (WNDPROC)BrowserProc;
	wc.lpszClassName = "VOYAGER";
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&wc);
	
#ifndef WIN3_1
	icon = LoadImage(hInstance, "ivoyager32.ico",  IMAGE_ICON, 64, 64, LR_LOADFROMFILE);
#endif

	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = icon;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = (WNDPROC)BrowserShellProc;
	wc.lpszClassName = "VOYAGER_SHELL";
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&wc);
	
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
#ifdef WIN3_1	
	wc.hbrBackground = (HBRUSH)CreateSolidBrush(GetWinColor()); //CreateHatchBrush(HS_BDIAGONAL, RGB(235,235,235)); //GetStockObject(WHITE_BRUSH);
#else
	wc.hbrBackground = (HBRUSH)CreateSolidBrush(GetWinColor()); //CreateHatchBrush(HS_BDIAGONAL, RGB(235,235,235)); //GetStockObject(WHITE_BRUSH);
#endif
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = icon;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = (WNDPROC)BrowserInnerShellProc;
	wc.lpszClassName = "VOYAGER_INNERSHELL";
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&wc);
	
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
#ifdef WIN3_1	
	wc.hbrBackground = (HBRUSH)CreateSolidBrush(GetWinColor()); //GetStockObject(LTGRAY_BRUSH);
#else
	wc.hbrBackground = (HBRUSH)CreateSolidBrush(GetWinColor()); //COLOR_WINDOW;
#endif
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = icon;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = (WNDPROC)BrowserShellToggleBar;
	wc.lpszClassName = "VOYAGER_SHELL_TOGGLEBAR";
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&wc);
	
	SetStatusText("Ready...");

	browserWin = CreateWindow("VOYAGER_SHELL", "Internet Voyager v1.0", WS_THICKFRAME | WS_OVERLAPPEDWINDOW  | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	UpdateWindow(browserWin);

#ifdef NOTHREADS
	#ifdef WIN3_1 // no Sleep function
		SetTimer(browserWin, 1, 0, NULL);
		while(GetMessage(&msg, NULL, 0, 0)) {
			if (msg.message == WM_TIMER && msg.wParam == 1) {
				DWORD start = GetTickCount();
				for (i = 0; i < NUM_THREADS; i++) {
					if (GetNextTask((Task far **)&currTask)) {
						RunTask(currTask);
						if ((GetTickCount() - start) > 3000) break;
					} else break;
				}
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	#else
		for (;;) {
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if(msg.message == WM_QUIT)
					break;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else if (GetNextTask(&currTask)) {
				RunTask(currTask);
			}
			else {
				Sleep(1);
			}
		}
	#endif
#else
	for (i = 0; i < NUM_THREADS; i++) {
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)workerProc, NULL, 0, &threadId);
	}

	while(GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#endif

	return msg.wParam;
}