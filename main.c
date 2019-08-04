#include "ivoyager.h"
#include "task.h"

ContentWindow g_TOP_WINDOW;

static LPSTR g_szDefURL = "/index.html";

#include "url_utils.c"

BOOL RunOpenUrlTask(Task *task) {
	URL_INFO *url_info = GetUrlInfo(((DownloadFileTaskParams *)task->params)->url);
	MessageBox(g_TOP_WINDOW.hWnd, url_info->path, "Opening URL", MB_OK);
	//MessageBox(g_TOP_WINDOW.hWnd, url_info->domain, "Opening URL", MB_OK);
	//MessageBox(g_TOP_WINDOW.hWnd, url_info->getvars, "Opening URL", MB_OK);
	//task->dwNextRun = (GetTickCount() + 5000);
	FreeUrlInfo(url_info);
	return TRUE;
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
	loadUrlTask->window = window;

	params = (DownloadFileTaskParams *)GlobalAlloc(GMEM_FIXED, sizeof(DownloadFileTaskParams));
	params->url = (LPSTR)GlobalAlloc(GMEM_FIXED, lstrlen(url)+1);
	
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