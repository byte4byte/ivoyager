#include "ivoyager.h"
#include "utils.c"
#include "task.h"
#include "url.c"
#include "dom.h"
#include <stdio.h>
#ifdef WIN3_1
#include <wing.h>
#endif

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

#define TASK_LOADURL 0

BOOL  RunOpenUrlTask(Task far *task) {
	typedef struct {
		URL_INFO far *url_info;
		FILE *fp;
	} OPEN_URL_DATA;
	
	typedef struct {
		char read_buff[128];
		int len;
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
			switch (url_info->protocol) {
				case FILE_PROTOCOL:
				{
					FILE *fp;
					MessageBox(g_TOP_WINDOW.hWnd, url_info->path, "", MB_OK);
					fp = _ffopen(url_info->path, "rb");
					if (! fp) { // 404
						MessageBox(g_TOP_WINDOW.hWnd, "404 ERROR", "", MB_OK);
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

			if (! GetCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, 0, &read_chunk_ptr)) {
				ret = TRUE;
				break;
			}
			read_chunk = (READ_CHUNK far *)read_chunk_ptr;
			
			if (ParseDOMChunk(&g_TOP_WINDOW, (char far *)read_chunk->read_buff, read_chunk->len)) {
				LocalFree((HLOCAL)read_chunk);
				RemoveCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, 0);
			}
			
			break;
		}
	}
	
	//task->dwNextRun = (GetTickCount() + 5000);
	
	if (ret) { // free data
		MessageBox(g_TOP_WINDOW.hWnd, "free", "", MB_OK);
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

static HWND hAddressBar, hTopBrowserWnd;
static HINSTANCE g_hInstance;

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

LRESULT  CALLBACK BrowserShellProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static int fontHeight = 25;

	switch (msg) {
		case WM_CREATE:
			{
				RECT rc;
				GetClientRect(hWnd, &rc);
				hAddressBar = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, -1, 0, rc.right+2, fontHeight, hWnd, NULL, g_hInstance, NULL);

				hTopBrowserWnd = CreateWindow("VOYAGER", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL, 0, fontHeight, rc.right, rc.bottom-fontHeight, hWnd, NULL, g_hInstance, NULL);

				g_TOP_WINDOW.hWnd = hTopBrowserWnd;

				OpenUrl(&g_TOP_WINDOW, g_szDefURL);
				SetWindowText(hAddressBar, (LPCSTR)g_szDefURL);
#ifdef WIN3_1a
				oldAddressBarProc = (FARPROC)SetWindowLong(hAddressBar, GWL_WNDPROC, (LONG)MakeProcInstance((FARPROC)AddressBarProc, g_hInstance));
#else
				oldAddressBarProc = (WNDPROC)SetWindowLongPtr(hAddressBar, GWLP_WNDPROC, (LONG_PTR)AddressBarProc);
#endif
			}
			return DefWindowProc(hWnd, msg, wParam, lParam);
		case WM_SIZE: {
			RECT rc;
			GetClientRect(hWnd, &rc);

			MoveWindow(hAddressBar, -1, -1, rc.right+2, fontHeight, TRUE);
			MoveWindow(hTopBrowserWnd, 0, fontHeight-1, rc.right, rc.bottom-fontHeight, TRUE);

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
	if (hShCore) {
		typedef HRESULT (*lpSetProcessDpiAwareness)(int value);
		enum PROCESS_DPI_AWARENESS {
		  PROCESS_DPI_UNAWARE,
		  PROCESS_SYSTEM_DPI_AWARE,
		  PROCESS_PER_MONITOR_DPI_AWARE
		};
		lpSetProcessDpiAwareness SetProcessDpiAwareness = (lpSetProcessDpiAwareness)GetProcAddress(hShCore, "SetProcessDpiAwareness");
		if (SetProcessDpiAwareness) {
			//#define PROCESS_PER_MONITOR_DPI_AWARE 0
			SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);	
		}
	}
#endif

	g_hInstance = hInstance;

	_fmemset(&g_TOP_WINDOW, 0, sizeof(ContentWindow));
	InitTaskSystem();

	#include "tests/path.c"

	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = NULL;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = (WNDPROC)BrowserProc;
	wc.lpszClassName = "VOYAGER";
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&wc);
	
#ifndef WIN3_1
	icon = LoadImage(hInstance, "ivoyager32.ico",  IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
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

	browserWin = CreateWindow("VOYAGER_SHELL", "Internet Voyager v1.0", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
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