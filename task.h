#ifndef _TASK_H
#define _TASK_H

#include "ivoyager.h"

typedef struct TaskDataList TaskDataList;
typedef struct TaskDataList {
	LPARAM data;
	TaskDataList *next;
} TaskDataList;

#define TASK_DATA_VAR	0
#define TASK_DATA_LIST	1

typedef struct TaskData TaskData;
typedef struct TaskData { // important "data" and "next" come first as list allocates TaskDataList to save memory
	LPARAM data; // if type == TASK_DATA_LIST data is a ptr to TaskDataList
	TaskData *next;
	DWORD id;
	BYTE type;
} TaskData;

typedef struct DownloadFileTaskParams {
	LPSTR url;
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

typedef struct Task Task;
typedef struct Task {
	int type;
	DWORD totalTime;
	ContentWindow *window;
	LPVOID params;
	BOOL locked; /* for multhreaded mode */
    DWORD dwNextRun;
	Task *parentTask;
	TaskData *taskData;
#ifndef NOTHREADS
	CRITICAL_SECTION cs;
#endif
} Task;

typedef struct TaskQueue TaskQueue;
typedef struct TaskQueue {
	Task *task;
	TaskQueue *next;
} TaskQueue;

void InitTaskSystem();
void FreeTask(Task *task);
BOOL GetNextTask(Task **task);
BOOL AddTask(Task *task);
BOOL RemoveTask(Task *task);

BOOL RemoveAllCustomTaskData(Task *task);

// arraylist var functions
BOOL GetCustomTaskListData(Task *task, DWORD id, int idx, LPARAM *out);
BOOL RemoveCustomTaskListData(Task *task, DWORD id, int idx);
int AddCustomTaskListData(Task *task, DWORD id, LPARAM data);
int GetNumCustomTaskListData(Task *task, DWORD id);

// scaler var functions
BOOL GetCustomTaskVar(Task *task, DWORD id, LPARAM *var, TaskData **parentTaskData);
BOOL RemoveCustomTaskVar(Task *task, DWORD id);
BOOL AddCustomTaskVar(Task *task, DWORD id, LPARAM var);

#endif