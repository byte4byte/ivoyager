#ifndef _TASK_H
#define _TASK_H

#include "ivoyager.h"

typedef struct TaskDataList TaskDataList;
typedef struct TaskDataList {
	LPARAM data;
	TaskDataList far *next;
} TaskDataList;

#define TASK_DATA_VAR	0
#define TASK_DATA_LIST	1

typedef struct TaskData TaskData;
typedef struct TaskData { // important "data" and "next" come first as list allocates TaskDataList to save memory
	LPARAM data; // if type == TASK_DATA_LIST data is a ptr to TaskDataList
	TaskData far *next;
	DWORD id;
	BYTE type;
} TaskData;

typedef struct Task Task;
typedef struct Task {
	int type;
	DWORD totalTime;
	LPVOID params;
	BOOL locked; /* for multhreaded mode */
    DWORD dwNextRun;
	Task far *parentTask;
	TaskData  far *taskData;
#ifndef NOTHREADS
	CRITICAL_SECTION cs;
#endif
} Task;

typedef struct TaskQueue TaskQueue;
typedef struct TaskQueue {
	Task far *task;
	TaskQueue far *next;
} TaskQueue;

void  InitTaskSystem();
void  FreeTask(Task far *task);
BOOL  GetNextTask(Task far **task);
BOOL  AddTask(Task far *task);
BOOL  RemoveTask(Task far *task);

BOOL  RemoveAllCustomTaskData(Task far *task);

// arraylist var functions
BOOL  GetCustomTaskListData(Task far *task, DWORD id, int idx, LPARAM  far *out);
BOOL  RemoveCustomTaskListData(Task far *task, DWORD id, int idx);
int  AddCustomTaskListData(Task far *task, DWORD id, LPARAM data);
int  GetNumCustomTaskListData(Task far *task, DWORD id);

// scaler var functions
BOOL  GetCustomTaskVar(Task far *task, DWORD id, LPARAM far *var, TaskData far **parentTaskData);
BOOL  RemoveCustomTaskVar(Task far *task, DWORD id);
BOOL  AddCustomTaskVar(Task far *task, DWORD id, LPARAM var);

#endif