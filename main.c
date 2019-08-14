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

ContentWindow g_TOP_WINDOW;
static LPSTR g_szDefURL = "d:/index.htm";

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
				case FILE_PROTOCOL:
				{
					FILE *fp;
					//MessageBox(g_TOP_WINDOW.hWnd, url_info->path, "", MB_OK);
					DebugLog("\"%s\" - Opening\r\n\r\n", url_info->path);
					fp = _ffopen(url_info->path, "rb");
					if (! fp) { // 404
						//MessageBox(g_TOP_WINDOW.hWnd, "404 ERROR", "", MB_OK);
						DebugLog("\"%s\" - 404 Error\r\n\r\n", url_info->path);
						ret = TRUE;
						break;
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
				LocalFree((HGLOBAL)read_chunk);
				AddCustomTaskVar(task, RUN_TASK_VAR_STATE, RUN_TASK_STATE_PARSE_DOM);
			}
			else {	
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

LRESULT CALLBACK AddressBarProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
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
typedef UINT (* lpGetDpiForWindow)(HWND hWnd);
lpGetDpiForWindow GetDpiForWindow = NULL;
#endif

#define DEF_FONT_HEIGHT 14

LRESULT  CALLBACK BrowserShellProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static int fontHeight = DEF_FONT_HEIGHT;
	static HFONT hAddrBarFont = NULL;

	switch (msg) {
		case WM_CREATE:
			{
				RECT rc;

				GetClientRect(hWnd, &rc);
#ifdef WIN3_1
				hAddressBar = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, -1, 0, rc.right+2, fontHeight, hWnd, NULL, g_hInstance, NULL);			
#else	
				hAddressBar = CreateWindowEx(WS_EX_STATICEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, -1, 0, rc.right+2, fontHeight, hWnd, NULL, g_hInstance, NULL);			
#endif

				hTopBrowserWnd = CreateWindow("RICHEDIT50W", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL | WS_BORDER, 0, fontHeight, rc.right, rc.bottom-fontHeight, hWnd, NULL, g_hInstance, NULL);
				if (! hTopBrowserWnd) {
					hTopBrowserWnd = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL | WS_BORDER, 0, fontHeight, rc.right, rc.bottom-fontHeight, hWnd, NULL, g_hInstance, NULL);
				}
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
		case WM_MOVE:
		case WM_SIZE: {
			RECT rc;
#ifndef WIN3_1				
			if (GetDpiForWindow) {
				UINT dpi = GetDpiForWindow(hWnd);
				fontHeight = (int)(((float)dpi / (float)4)); // 1/4 inch
				if (hAddrBarFont) {
					DeleteObject(hAddrBarFont);
				}
				//fontheight = -MulDiv(PointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
				hAddrBarFont = CreateFont(fontHeight-8, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE, ANSI_CHARSET, 
				OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
				DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
				SendMessage(hAddressBar, WM_SETFONT, (WPARAM)hAddrBarFont, TRUE);

				//GetClientRect(hAddressBar, &rc);
				fontHeight += 2;
			}
#endif			
			GetClientRect(hWnd, &rc);
#ifdef WIN3_1
			MoveWindow(hAddressBar, -1, -1, rc.right+2, fontHeight, TRUE);
			MoveWindow(hTopBrowserWnd, 0, fontHeight-1, rc.right, rc.bottom-fontHeight, TRUE);
#else
			MoveWindow(hAddressBar, -1, 0, rc.right+2, fontHeight, TRUE);
			MoveWindow(hTopBrowserWnd, 0, fontHeight, rc.right, rc.bottom-fontHeight, TRUE);
#endif

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
		typedef HRESULT (*lpSetProcessDpiAwareness)(int value);
		enum PROCESS_DPI_AWARENESS {
		  PROCESS_DPI_UNAWARE,
		  PROCESS_SYSTEM_DPI_AWARE,
		  PROCESS_PER_MONITOR_DPI_AWARE
		};
		lpSetProcessDpiAwareness SetProcessDpiAwareness = (lpSetProcessDpiAwareness)GetProcAddress(hShCore, "SetProcessDpiAwareness");
		if (SetProcessDpiAwareness) {
			SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);	
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
	wc.hbrBackground = GetStockObject(WHITE_BRUSH);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = icon;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = (WNDPROC)BrowserShellProc;
	wc.lpszClassName = "VOYAGER_SHELL";
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&wc);

	browserWin = CreateWindow("VOYAGER_SHELL", "Internet Voyager v1.0", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
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