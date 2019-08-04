#include "ivoyager.h"
#include "task.h"
#include "url_utils.c"
#include "dom.h"
#include <stdio.h>

ContentWindow g_TOP_WINDOW;
static LPSTR g_szDefURL = "d:/index.html";

typedef struct DownloadFileTaskParams {
	LPSTR url;
	ContentWindow *window;
} DownloadFileTaskParams;

typedef struct ParseHtmlTaskParams {
	LPSTR html_buff_start;
	int len;
} ParseHtmlTaskParams;

typedef struct ParseCSSTaskParams {
	LPSTR css_buff_start;
	int len;
} ParseCSSTaskParams;

#define TASK_LOADURL 0

BOOL RunOpenUrlTask(Task *task) {
	typedef struct {
		URL_INFO *url_info;
		FILE *fp;
	} OPEN_URL_DATA;
	
	typedef struct {
		char read_buff[1024];
		int len;
	} READ_CHUNK;
	
	OPEN_URL_DATA *open_url_data;
	BOOL ret = FALSE;
	LPARAM state;
	
	#define RUN_TASK_VAR_STATE   0
	#define RUN_TASK_VAR_DATA  	 1
	#define RUN_TASK_VAR_CHUNKS  2
	
	#define RUN_TASK_STATE_OPEN_STREAM 0
	#define RUN_TASK_STATE_READ_STREAM 1
	#define RUN_TASK_STATE_PARSE_DOM   2
	
	GetCustomTaskVar(task, RUN_TASK_VAR_STATE, (LPARAM *)&state, NULL);
	GetCustomTaskVar(task, RUN_TASK_VAR_DATA, (LPARAM *)&open_url_data, NULL);
	switch (state) {
		case RUN_TASK_STATE_OPEN_STREAM:
		{
			URL_INFO *url_info = GetUrlInfo(((DownloadFileTaskParams *)task->params)->url);
			if (! url_info) {
				ret = TRUE;
				break;
			}
			switch (url_info->protocol) {
				case FILE_PROTOCOL:
				{
					FILE *fp = fopen(url_info->path, "rb");
					if (! fp) { // 404
						ret = TRUE;
						break;
					}
					
					open_url_data = (OPEN_URL_DATA *)GlobalAlloc(GMEM_FIXED, sizeof(OPEN_URL_DATA));
					memset(open_url_data, 0, sizeof(OPEN_URL_DATA));
					
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
			READ_CHUNK *read_chunk = GlobalAlloc(GMEM_FIXED, sizeof(READ_CHUNK));
			read_chunk->len = fread(read_chunk->read_buff, 1, sizeof(read_chunk->read_buff), open_url_data->fp);
			if (read_chunk->len <= 0) {
				GlobalFree((HGLOBAL)read_chunk);
				AddCustomTaskVar(task, RUN_TASK_VAR_STATE, RUN_TASK_STATE_PARSE_DOM);
			}
			else {
				AddCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, (LPARAM)read_chunk);
			}
			//break;
		}
		case RUN_TASK_STATE_PARSE_DOM: {
			READ_CHUNK *read_chunk;
			if (! GetCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, 0, (LPARAM *)&read_chunk)) {
				ret = TRUE;
				break;
			}
			
			if (ParseDOMChunk(&g_TOP_WINDOW, read_chunk->read_buff, read_chunk->len)) {
				GlobalFree((HGLOBAL)read_chunk);
				RemoveCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, 0);
			}
			break;
		}
	}
	
	//task->dwNextRun = (GetTickCount() + 5000);
	
	if (ret) { // free data
		MessageBoxA(g_TOP_WINDOW.hWnd, "free", "", MB_OK);
		if (open_url_data) {
			if (open_url_data->fp) fclose(open_url_data->fp);
			FreeUrlInfo(open_url_data->url_info);
			GlobalFree((HGLOBAL)open_url_data);
		}
		GlobalFree((HGLOBAL)((DownloadFileTaskParams *)task->params)->url);
		GlobalFree((HGLOBAL)task->params);
	}
	return ret;
}


BOOL RunTask(Task *task) {
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

void OpenUrl(ContentWindow *window, LPSTR url) {
	// TODO: Clear out window

	DownloadFileTaskParams *params;
	Task *loadUrlTask;
	
	loadUrlTask = (Task *)GlobalAlloc(GMEM_FIXED, sizeof(Task));
	memset(loadUrlTask, 0, sizeof(Task));
	loadUrlTask->type = TASK_LOADURL;

	params = (DownloadFileTaskParams *)GlobalAlloc(GMEM_FIXED, sizeof(DownloadFileTaskParams));
	params->url = (LPSTR)GlobalAlloc(GMEM_FIXED, lstrlen(url)+1);
	params->window = window;
	
	lstrcpy(params->url, url);

	loadUrlTask->params = params;

	AddTask(loadUrlTask);
	
	#include "tests/task_data_unittest.c"
}

static HWND hAddressBar, hTopBrowserWnd;
static HINSTANCE g_hInstance;

LRESULT CALLBACK BrowserShellProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static int fontHeight = 25;

	switch (msg) {
		case WM_CREATE:
			{
				RECT rc;
				GetClientRect(hWnd, &rc);
				hAddressBar = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 0, 0, rc.right, fontHeight, hWnd, NULL, g_hInstance, NULL);

				hTopBrowserWnd = CreateWindow("VOYAGER", "", WS_CHILD | WS_VISIBLE, 0, fontHeight, rc.right, rc.bottom-fontHeight, hWnd, NULL, g_hInstance, NULL);

				g_TOP_WINDOW.hWnd = hTopBrowserWnd;

				OpenUrl(&g_TOP_WINDOW, g_szDefURL);
				SetWindowText(hAddressBar, (LPCSTR)g_szDefURL);
			}
			return DefWindowProc(hWnd, msg, wParam, lParam);
		case WM_SIZE: {
			RECT rc;
			GetClientRect(hWnd, &rc);

			MoveWindow(hAddressBar, 0, 0, rc.right, fontHeight, TRUE);
			MoveWindow(hTopBrowserWnd, 0, fontHeight, rc.right, rc.bottom-fontHeight, TRUE);

			return 0;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK BrowserProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

#ifndef NOTHREADS
DWORD WINAPI workerProc(LPVOID unused) {
	Task *currTask;

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
	//MessageBox(NULL, "voyager", "", MB_OK);

	WNDCLASS wc;
	MSG msg;
	HWND browserWin;
	int i = 0;
	
#ifdef NOTHREADS
	Task *currTask;
#else
	DWORD threadId;
#endif

	g_hInstance = hInstance;

	memset(&g_TOP_WINDOW, 0, sizeof(ContentWindow));
	InitTaskSystem();

	#include "tests/path_norm_unittest.c"

	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = GetStockObject(WHITE_BRUSH);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = NULL;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = (WNDPROC)BrowserProc;
	wc.lpszClassName = "VOYAGER";
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&wc);

	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = GetStockObject(WHITE_BRUSH);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = NULL;
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
					if (GetNextTask(&currTask)) {
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