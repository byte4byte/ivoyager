#include "ivoyager.h"
#include "utils.c"
#include "task.h"
#include "url.c"
#include "dom.h"
#include <stdio.h>

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
	
	MessageBox(g_TOP_WINDOW.hWnd, "RUN 1", "", MB_OK);
	
	
	#define RUN_TASK_VAR_STATE   0
	#define RUN_TASK_VAR_DATA  	 1
	#define RUN_TASK_VAR_CHUNKS  2
	
	#define RUN_TASK_STATE_OPEN_STREAM 0
	#define RUN_TASK_STATE_READ_STREAM 1
	#define RUN_TASK_STATE_PARSE_DOM   2
	
	GetCustomTaskVar(task, RUN_TASK_VAR_STATE, &state, NULL);
	MessageBox(g_TOP_WINDOW.hWnd, "RUN 2", "", MB_OK);
	GetCustomTaskVar(task, RUN_TASK_VAR_DATA, (LPARAM far *)&open_url_data, NULL);
	MessageBox(g_TOP_WINDOW.hWnd, "RUN 3", "", MB_OK);
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
					
					MessageBox(g_TOP_WINDOW.hWnd, "OPENED", "", MB_OK);
					
					open_url_data = (OPEN_URL_DATA far *)GlobalAlloc(GMEM_FIXED, sizeof(OPEN_URL_DATA));
					//memset(open_url_data, 0, sizeof(OPEN_URL_DATA));
					
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
			static char buff[256];
			int addidx;
			READ_CHUNK near *read_chunk = (READ_CHUNK near *)LocalAlloc(LMEM_FIXED, sizeof(READ_CHUNK));
			READ_CHUNK far *read_chunk_far;
			LPARAM read_chunk_ptr;
			MessageBox(g_TOP_WINDOW.hWnd, "read", "", MB_OK);
			read_chunk->len = fread(read_chunk->read_buff, 1, sizeof(read_chunk->read_buff)-1, open_url_data->fp);
			MessageBox(g_TOP_WINDOW.hWnd, "read 2", "", MB_OK);
			if (read_chunk->len <= 0) {
				MessageBox(g_TOP_WINDOW.hWnd, "read 3", "", MB_OK);
				LocalFree((HGLOBAL)read_chunk);
				AddCustomTaskVar(task, RUN_TASK_VAR_STATE, RUN_TASK_STATE_PARSE_DOM);
				MessageBox(g_TOP_WINDOW.hWnd, "read 4", "", MB_OK);
			}
			else {	
				MessageBox(g_TOP_WINDOW.hWnd, "read 5", "", MB_OK);
				read_chunk_far = (READ_CHUNK far *)NearPtrToFar((const char near *)read_chunk, sizeof(READ_CHUNK));
				ParseDOMChunk(&g_TOP_WINDOW, (char far *)read_chunk_far->read_buff, read_chunk_far->len);
				addidx = AddCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, (LPARAM)read_chunk_far);
				read_chunk_ptr = (LPARAM)read_chunk_far;
				//if ((READ_CHUNK far *)read_chunk_ptr != read_chunk_far) {
					
					wsprintf(buff, "first %lX %lX %d", (READ_CHUNK far *)read_chunk_ptr, (READ_CHUNK far *)read_chunk_far, addidx);
					MessageBox(NULL, buff, "", MB_OK); // 2 390E
					//MessageBox(g_TOP_WINDOW.hWnd, "ERROR NO MATCH", "", MB_OK);
				//}
				if (! GetCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, 0, &read_chunk_ptr)) {
					MessageBox(NULL, "GET FAILED", "", MB_OK);
				}
				if ((READ_CHUNK far *)read_chunk_ptr != read_chunk_far) {
					
					wsprintf(buff, "%lX %lX", (READ_CHUNK far *)read_chunk_ptr, (READ_CHUNK far *)read_chunk_far);
					MessageBox(NULL, buff, "", MB_OK); // 2 390E
					MessageBox(g_TOP_WINDOW.hWnd, "ERROR NO MATCH", "", MB_OK);
				}
				MessageBox(g_TOP_WINDOW.hWnd, "read 5.1", "", MB_OK);
				LocalFree((HGLOBAL)read_chunk);
				MessageBox(g_TOP_WINDOW.hWnd, "read 6", "", MB_OK);
			}
			//break;
		}
		case RUN_TASK_STATE_PARSE_DOM: {
			LPARAM read_chunk_ptr;
			READ_CHUNK far *read_chunk;
			MessageBox(g_TOP_WINDOW.hWnd, "dom", "", MB_OK);
			if (! GetCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, 0, &read_chunk_ptr)) {
				MessageBox(g_TOP_WINDOW.hWnd, "dom1", "", MB_OK);
				ret = TRUE;
				break;
			}
			read_chunk = (READ_CHUNK far *)read_chunk_ptr;
			
			MessageBox(g_TOP_WINDOW.hWnd, "dom2", "", MB_OK);
			if (ParseDOMChunk(&g_TOP_WINDOW, (char far *)read_chunk->read_buff, read_chunk->len)) {
				MessageBox(g_TOP_WINDOW.hWnd, "dom3", "", MB_OK);
				GlobalFree((HGLOBAL)read_chunk);
				RemoveCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, 0);
				MessageBox(g_TOP_WINDOW.hWnd, "dom4", "", MB_OK);
			}
			
			MessageBox(g_TOP_WINDOW.hWnd, "dom5", "", MB_OK);
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

LRESULT  CALLBACK BrowserShellProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
	//MessageBox(NULL, "voyager", "", MB_OK);

	WNDCLASS wc;
	MSG msg;
	HWND browserWin;
	int i = 0;
	
#ifdef NOTHREADS
	Task far *currTask;
#else
	DWORD threadId;
#endif

	//static char buff[256];
	//wsprintf(buff, "%d %d %d %d", sizeof(char *), sizeof(char near *), sizeof(char far *), sizeof(LPARAM));
	//MessageBox(NULL, buff, "", MB_OK);

	g_hInstance = hInstance;

	_fmemset(&g_TOP_WINDOW, 0, sizeof(ContentWindow));
	InitTaskSystem();

	#include "tests/path.c"

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