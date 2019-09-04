#include "ivoyager.h"

#include <stdio.h>
#include <winsock.h>
#ifdef WIN3_1
//#include <wing.h>
#include <stdarg.h>
#endif
#ifndef WIN3_1
#include <windowsx.h>
#include <commctrl.h>
#include <richedit.h>
#endif

static Task far *g_tabTask = NULL, far *g_socketsTask = NULL;
HWND browserWin;

#define RUN_TASK_VAR_STATE              0
#define RUN_TASK_VAR_DATA                       1
#define RUN_TASK_VAR_CHUNKS             2
#define RUN_TASK_VAR_CONNECT_STATE      3

#define CONNECT_STATE_CONNECTING        0
#define CONNECT_STATE_ERROR                     1
#define CONNECT_STATE_SUCCESS           2
#define CONNECT_STATE_READY                     3
#define CONNECT_STATE_CLOSED            4

#define RUN_TASK_STATE_OPEN_STREAM 0
#define RUN_TASK_STATE_READ_STREAM 1
#define RUN_TASK_STATE_PARSE_DOM   2
#define RUN_TASK_STATE_CONNECTING  3

void WriteSource(Tab far *tab, LPSTR raw, int len) ;

#include "utils.c"
#include "url.c"
#include "stream.c"
#include "http.c"

static FILE *fpLog, *fpSource;

static HWND hAddressBar, hTopBrowserWnd;
static HINSTANCE g_hInstance;

static LPSTR g_szDefURL = "file://d:/index.htm";

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

BOOL RunOpenUrlTask(Task far *task) {
        typedef struct {
                URL_INFO far *url_info;
                Stream far *stream;
        } OPEN_URL_DATA;
        
        typedef struct {
                char read_buff[BUFFER_SIZE];  // should be >= FD_READ recv buff size
                int len;
                BOOL eof;
        } READ_CHUNK;
        
        
        OPEN_URL_DATA far *open_url_data;
        BOOL ret = FALSE;
        LPARAM state;
        Stream far *stream;
        
        GetCustomTaskVar(task, RUN_TASK_VAR_STATE, &state, NULL);
        GetCustomTaskVar(task, RUN_TASK_VAR_DATA, (LPARAM far *)&open_url_data, NULL);
        switch (state) {
                case RUN_TASK_STATE_OPEN_STREAM:
                {
                        URL_INFO  far *url_info = GetUrlInfo(((DownloadFileTaskParams far *)task->params)->url, NULL);
                        //MessageBox(NULL, "open stream", "", MB_OK);
                        if (! url_info) {
                                ret = TRUE;
                                break;
                        }
                        SetWindowText(((DownloadFileTaskParams far *)task->params)->window->tab->hSource, "");
                        SetWindowText(((DownloadFileTaskParams far *)task->params)->window->tab->hConsole, "");
                        
                        stream = openStream(task, ((DownloadFileTaskParams far *)task->params)->window, url_info);
                        if (! stream) {
                                SetStatusText(((DownloadFileTaskParams far *)task->params)->window->tab, "\"%s\" - 404 Error", url_info->path);
                                ret = TRUE;
                        }
                        else {

                                switch (url_info->protocol) {
                                        case HTTP_PROTOCOL: 
                                        {
                                                SetTabTitle(((DownloadFileTaskParams far *)task->params)->window->tab, url_info->domain);
                                                SetStatusText(((DownloadFileTaskParams far *)task->params)->window->tab, "Connecting to: \"%s\"", url_info->domain);
                                                
                                                
                                                open_url_data = (OPEN_URL_DATA far *)GlobalAlloc(GMEM_FIXED, sizeof(OPEN_URL_DATA));
                                                _fmemset(open_url_data, 0, sizeof(OPEN_URL_DATA));
                                                
                                                open_url_data->stream = stream;
                                                open_url_data->url_info = url_info;
                                                
                                                AddCustomTaskVar(task, RUN_TASK_VAR_STATE, RUN_TASK_STATE_CONNECTING);
                                                AddCustomTaskVar(task, RUN_TASK_VAR_DATA, (LPARAM)open_url_data);
                                                
                                                break;
                                        }
                                        case FILE_PROTOCOL:
                                        {
                                                //MessageBox(g_TOP_WINDOW.hWnd, url_info->path, "", MB_OK);
                                                SetStatusText(((DownloadFileTaskParams far *)task->params)->window->tab, "Opening: \"%s\"", url_info->path);
                                                SetTabTitle(((DownloadFileTaskParams far *)task->params)->window->tab, url_info->path);

                                                SetStatusText(((DownloadFileTaskParams far *)task->params)->window->tab, "Opened: \"%s\"", url_info->path);
                                                
                                                open_url_data = (OPEN_URL_DATA far *)GlobalAlloc(GMEM_FIXED, sizeof(OPEN_URL_DATA));
                                                _fmemset(open_url_data, 0, sizeof(OPEN_URL_DATA));
                                                
                                                open_url_data->stream = stream;
                                                open_url_data->url_info = url_info;
                                                
                                                AddCustomTaskVar(task, RUN_TASK_VAR_STATE, RUN_TASK_STATE_READ_STREAM);
                                                AddCustomTaskVar(task, RUN_TASK_VAR_DATA, (LPARAM)open_url_data);
                                                break;
                                        }
                                        default:
                                                ret = TRUE;
                                                break;
                                }
                        }
                        break;
                }
                case RUN_TASK_STATE_CONNECTING:
                {
                        LPARAM connect_state;
                        GetCustomTaskVar(task, RUN_TASK_VAR_CONNECT_STATE, &connect_state, NULL);
                        if (connect_state == CONNECT_STATE_CONNECTING) {
                                // check time limit here
                                break;
                        }
                        else if (connect_state == CONNECT_STATE_ERROR) {
                                ret = TRUE;
                                break;
                        }
                        else if (connect_state == CONNECT_STATE_SUCCESS) {
                                break;
                        }
                        else if (connect_state == CONNECT_STATE_READY) {
                                // let fall into read stream
                                AddCustomTaskVar(task, RUN_TASK_VAR_STATE, RUN_TASK_STATE_READ_STREAM);
                        }
                }
                case RUN_TASK_STATE_READ_STREAM:
                {
                        int addidx;
            READ_CHUNK far *read_chunk;
                        if (open_url_data->stream->hasData(open_url_data->stream)) {
                                //DebugLog(((DownloadFileTaskParams far *)task->params)->window->tab, "\nhere1\n");
                                read_chunk = (READ_CHUNK far *)GlobalAlloc(GMEM_FIXED, sizeof(READ_CHUNK));
                                //DebugLog(((DownloadFileTaskParams far *)task->params)->window->tab, "\nhere1.2\n");
                                read_chunk->len = open_url_data->stream->read(open_url_data->stream, read_chunk->read_buff, sizeof(read_chunk->read_buff)-1);
                                //DebugLog(((DownloadFileTaskParams far *)task->params)->window->tab, "\nhere1.3\n");
                                read_chunk->eof = FALSE;
                                //DebugLog(((DownloadFileTaskParams far *)task->params)->window->tab, "\nhere2\n");
                                //read_chunk->eof = open_url_data->stream->eof(open_url_data->stream);
                                if (read_chunk->len > 0) {      
                                        //DebugLog(((DownloadFileTaskParams far *)task->params)->window->tab, "\nhere3\n");
                                        SetStatusText(((DownloadFileTaskParams far *)task->params)->window->tab, "Reading: \"%s\"", open_url_data->url_info->path);
                                        //MessageBox(browserWin, open_url_data->url_info->path, "path", MB_OK);
                                        addidx = AddCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, (LPARAM)read_chunk);
                                }
                                else if (open_url_data->stream->eof(open_url_data->stream)) {
                                        //DebugLog(((DownloadFileTaskParams far *)task->params)->window->tab, "\nhere4\n");
                                        read_chunk->eof = TRUE;
                                        read_chunk->read_buff[0] = '\0';
                                        addidx = AddCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, (LPARAM)read_chunk);
                                        SetStatusText(((DownloadFileTaskParams far *)task->params)->window->tab, "Done: \"%s\"", open_url_data->url_info->path);
                                        //LocalFree((HGLOBAL)read_chunk);
                                        AddCustomTaskVar(task, RUN_TASK_VAR_STATE, RUN_TASK_STATE_PARSE_DOM);
                                        //MessageBox(browserWin, "EOF", "", MB_OK);
                                        DebugLog(((DownloadFileTaskParams far *)task->params)->window->tab, "\r\n--EOF--\r\n");
                                        GlobalFree((HGLOBAL)read_chunk);
                                }
                                else {
                                        //DebugLog(((DownloadFileTaskParams far *)task->params)->window->tab, "\nhere5\n");
                                          GlobalFree((HGLOBAL)read_chunk);
                                }
                        
                        }
                        //break;
                }
                case RUN_TASK_STATE_PARSE_DOM: {
                        LPARAM read_chunk_ptr;
                        READ_CHUNK far *read_chunk;
                        char far *ptr;
                        
                        //DebugLog(((DownloadFileTaskParams far *)task->params)->window->tab, "\nhere6\n");

                        if (! GetCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, 0, &read_chunk_ptr)) {
                                if (state == RUN_TASK_STATE_PARSE_DOM) ret = TRUE;
                                break;
                        }
                        read_chunk = (READ_CHUNK far *)read_chunk_ptr;
                        ptr = read_chunk->read_buff;
                        
                        //DebugLog(((DownloadFileTaskParams far *)task->params)->window->tab, "\nhere7\n");
                        
                        if (ParseDOMChunk(((DownloadFileTaskParams far *)task->params)->window, task, (char far * far *)&ptr, read_chunk->len, read_chunk->eof)) {
                                //DebugLog(((DownloadFileTaskParams far *)task->params)->window->tab, "\nhere8\n");
                                GlobalFree((HGLOBAL)read_chunk);
                                RemoveCustomTaskListData(task, RUN_TASK_VAR_CHUNKS, 0);
                        }
                        
                        break;
                }
        }
        
        //task->dwNextRun = (GetTickCount() + 5000);
        
        if (ret) { // free data
                //MessageBox(g_TOP_WINDOW.hWnd, "free", "", MB_OK);
                //DebugLog(((DownloadFileTaskParams far *)task->params)->window->tab, "\nhere9\n");
                if (open_url_data) {
                        if (open_url_data->stream) closeStream(open_url_data->stream);
                        FreeUrlInfo(open_url_data->url_info);
                        GlobalFree((HGLOBAL)open_url_data);
                }
                GlobalFree((HGLOBAL)((DownloadFileTaskParams far *)task->params)->url);
                GlobalFree((HGLOBAL)task->params);
                //DebugLog(((DownloadFileTaskParams far *)task->params)->window->tab, "\nhere10\n");
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

void OpenUrl(Tab far *tab, ContentWindow far *window, DomNode *element, LPSTR  url) {
        // TODO: Clear out window

        DownloadFileTaskParams far *params;
        Task far *loadUrlTask;
        
        fpLog = fopen("C:\\out.log", "w");
        fpSource = fopen("C:\\source.log", "w");
        
        //MessageBox(browserWin, url, "", MB_OK);
        
        loadUrlTask = (Task  far *)GlobalAlloc(GMEM_FIXED, sizeof(Task));
        _fmemset(loadUrlTask, 0, sizeof(Task));
        loadUrlTask->type = TASK_LOADURL;

        params = (DownloadFileTaskParams far *)GlobalAlloc(GMEM_FIXED, sizeof(DownloadFileTaskParams));
        params->url = (LPSTR)GlobalAlloc(GMEM_FIXED, lstrlen(url)+1);
        params->window = window;
        
        lstrcpy(params->url, url);

        loadUrlTask->params = (LPVOID)params;


        /* todo: find empty slot for web request in window else queue it */
        //if (! emptyslot) {} else
                {
                        AddTask(loadUrlTask);

                        SetTabURL(tab, url);
                }
                
        #include "tests/taskdata.c"
}

#include "winshell.c"

int pascal WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
        static WNDCLASS wc;
        static MSG msg;
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
                typedef HRESULT  (WINAPI *lpSetProcessDpiAwareness)(int value);
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
        
        g_tabTask = AllocTempTask();
        g_socketsTask = AllocTempTask();
        if (! WinsockStart()) {
                MessageBox(NULL, "Unable to start winsock!", "", MB_OK);
                return 1;
        }

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
