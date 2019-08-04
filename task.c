#include "task.h"

#ifndef NOTHREADS
static CRITICAL_SECTION g_TASK_CS;
#endif

static TaskQueue g_TASK_QUEUE;
static int g_CURR_TASK_INDEX = 0;

BOOL RemoveTask(Task *task) {

	TaskQueue *curr;
	TaskQueue *prev;
	int idx;

#ifndef NOTHREADS
	EnterCriticalSection(&g_TASK_CS);
#endif

	curr = &g_TASK_QUEUE;
	prev = curr;
	idx = 0;
	while (curr->next) {
		prev = curr;
		curr = curr->next;
		if (curr->task == task) {
			if (idx >= g_CURR_TASK_INDEX) g_CURR_TASK_INDEX--;

			GlobalFree((HGLOBAL)curr);
			prev->next = curr->next;

#ifndef NOTHREADS
			LeaveCriticalSection(&g_TASK_CS);
#endif

			return TRUE;
		}

		idx++;
	}

#ifndef NOTHREADS
	LeaveCriticalSection(&g_TASK_CS);
#endif
	
	return FALSE;
}

BOOL AddTask(Task *task) {
	TaskQueue *curr;

#ifndef NOTHREADS
	InitializeCriticalSection(&task->cs);
#endif

#ifndef NOTHREADS
	EnterCriticalSection(&g_TASK_CS);
#endif

	curr = &g_TASK_QUEUE;
	do {
		if (! curr->next) {
			curr->next = (TaskQueue *)GlobalAlloc(GMEM_FIXED, sizeof(TaskQueue));
			memset(curr->next, 0, sizeof(TaskQueue));
			curr = curr->next;
			curr->task = task;

#ifndef NOTHREADS
			LeaveCriticalSection(&g_TASK_CS);
#endif

			return TRUE;
		}
		curr = curr->next;
	} while(curr->next);

#ifndef NOTHREADS
	LeaveCriticalSection(&g_TASK_CS);
#endif
	
	return FALSE;	
}

TaskData *GetTaskData(Task *task, DWORD id) {
	TaskData *curr;

#ifndef NOTHREADS
	EnterCriticalSection(&task->cs);
#endif

	curr = task->taskData;
	while (curr) {
		if (curr->id == id) {
#ifndef NOTHREADS
			LeaveCriticalSection(&task->cs);
#endif
			return curr;
		}
		curr = curr->next;
	} while (curr);

#ifndef NOTHREADS
	LeaveCriticalSection(&task->cs);
#endif

	return NULL;
}

BOOL SetCustomTaskData(Task *task, DWORD id, LPARAM var, BYTE type) {
	TaskData *curr;

#ifndef NOTHREADS
	EnterCriticalSection(&task->cs);
#endif

	curr = GetTaskData(task, id);
	if (curr) { // id exists, just set var
		curr->type = type;
		curr->data = var;
#ifndef NOTHREADS
		LeaveCriticalSection(&task->cs);
#endif
		return TRUE;
	}

#ifndef NOTHREADS
	LeaveCriticalSection(&task->cs);
#endif
	return FALSE;
}

BOOL AddCustomTaskData(Task *task, DWORD id, LPARAM var, BYTE type) {
	TaskData *curr;

	if (SetCustomTaskData(task, id, var, type)) return TRUE;

#ifndef NOTHREADS
	EnterCriticalSection(&task->cs);
#endif

	curr = task->taskData;
	do {
		if (! curr || ! curr->next) {
			TaskData *node = (TaskData *)GlobalAlloc(GMEM_FIXED, sizeof(TaskData));
			memset(node, 0, sizeof(TaskData));
			node->id = id;
			node->type = type;
			node->data = var;
			if (! curr) task->taskData = node;
			else curr->next = node;
#ifndef NOTHREADS
			LeaveCriticalSection(&task->cs);
#endif
			return TRUE;
		}

		curr = curr->next;
	} while (1);

#ifndef NOTHREADS
	LeaveCriticalSection(&task->cs);
#endif
	return FALSE;
}

BOOL AddCustomTaskVar(Task *task, DWORD id, LPARAM var) {
	return AddCustomTaskData(task, id, var, TASK_DATA_VAR);
}

BOOL RemoveCustomTaskVar(Task *task, DWORD id) {
	TaskData *curr;
	TaskData *prev;

#ifndef NOTHREADS
	EnterCriticalSection(&task->cs);
#endif

	curr = task->taskData;
	prev = task->taskData;
	while (curr) {
		if (curr->id == id) {
			if (prev != task->taskData) {
				prev->next = curr->next;
				GlobalFree((HGLOBAL)curr);
			}
			else {
				task->taskData = curr->next;
				GlobalFree((HGLOBAL)curr);
			}
#ifndef NOTHREADS
			LeaveCriticalSection(&task->cs);
#endif
			return TRUE;
		}

		prev = curr;
		curr = curr->next;
	};

#ifndef NOTHREADS
	LeaveCriticalSection(&task->cs);
#endif
	return FALSE;
}

BOOL GetCustomTaskVar(Task *task, DWORD id, LPARAM *var, TaskData **parentTaskData) {
	TaskData *curr;

	*var = 0;

#ifndef NOTHREADS
	EnterCriticalSection(&task->cs);
#endif

	curr = task->taskData;
	while (curr) {
		if (curr->id == id) {
			*var = curr->data;
			if (parentTaskData) *parentTaskData = curr;
#ifndef NOTHREADS
			LeaveCriticalSection(&task->cs);
#endif
			return TRUE;
		}

		curr = curr->next;
	};

#ifndef NOTHREADS
	LeaveCriticalSection(&task->cs);
#endif
	return FALSE;
}

BOOL RemoveAllCustomTaskData(Task *task) {
	TaskData *start;
	TaskData *prev;
	TaskData *node;

#ifndef NOTHREADS
	EnterCriticalSection(&task->cs);
#endif

	start = node = prev = task->taskData;
	while (node) {
		
		if (node->type == TASK_DATA_LIST) {
			TaskData *lstart;
			TaskData *lprev;
			TaskData *lnode;

			lstart = lnode = lprev = (TaskData *)node->data;
			while (lnode) {
				lprev = lnode;
				lnode = lnode->next;
				GlobalFree((HGLOBAL)lprev);
			}
			node->data = 0;
		}

		prev = node;
		node = node->next;

		GlobalFree((HGLOBAL)prev);
	}

	task->taskData = NULL;

#ifndef NOTHREADS
	LeaveCriticalSection(&task->cs);
#endif
	return TRUE;
}

int GetNumCustomTaskListData(Task *task, DWORD id) {
	TaskData *start;
	int idx = 0;

#ifndef NOTHREADS
	EnterCriticalSection(&task->cs);
#endif

	if (! GetCustomTaskVar(task, id, (LPVOID)&start, NULL)) {
#ifndef NOTHREADS
		LeaveCriticalSection(&task->cs);
#endif
		return -1;
	}

	while (start) {
		idx++;
		start = start->next;
	}

#ifndef NOTHREADS
	LeaveCriticalSection(&task->cs);
#endif
	return idx;
}


int AddCustomTaskListData(Task *task, DWORD id, LPARAM data) {
	TaskData *start;
	int idx = 0;

	TaskData *node = GlobalAlloc(GMEM_FIXED, sizeof(TaskData));
	memset(node, 0, sizeof(TaskData));
	node->data = data;

#ifndef NOTHREADS
	EnterCriticalSection(&task->cs);
#endif

	if (GetCustomTaskVar(task, id, (LPVOID)&start, NULL)) {
		while (start->next) {
			idx++;
			start = start->next;
		}
		idx++;
		start->next = node;
	}
	else {
		if (! AddCustomTaskData(task, id, (LPARAM)node, TASK_DATA_LIST)) {
#ifndef NOTHREADS
			LeaveCriticalSection(&task->cs);
#endif
			GlobalFree((HGLOBAL)node);
			return -1;
		}
	}

#ifndef NOTHREADS
	LeaveCriticalSection(&task->cs);
#endif
	return idx;
}

BOOL RemoveCustomTaskListData(Task *task, DWORD id, int idx) {
	TaskData *start;
	TaskData *node;
	TaskData *prev;
	TaskData *parent;
	int i = 0;

#ifndef NOTHREADS
	EnterCriticalSection(&task->cs);
#endif

	if (! GetCustomTaskVar(task, id, (LPVOID)&start, &parent)) {
#ifndef NOTHREADS
		LeaveCriticalSection(&task->cs);
#endif
		return FALSE;
	}

	node = start;
	prev = start;

	while (node) {
		if (i == idx) {
			if (node == start) {
				parent->data = (LPARAM)node->next;
				GlobalFree((HGLOBAL)node);
				if (! parent->data) RemoveCustomTaskVar(task, id);
			}
			else {
				prev->next = node->next;
				GlobalFree((HGLOBAL)node);
			}
#ifndef NOTHREADS
			LeaveCriticalSection(&task->cs);
#endif
			return TRUE;
		}
		i++;
		prev = node;
		node = node->next;
	}

#ifndef NOTHREADS
	LeaveCriticalSection(&task->cs);
#endif
	return TRUE;
}

BOOL GetCustomTaskListData(Task *task, DWORD id, int idx, LPARAM *out) {
	TaskData *start;
	int i = 0;

	*out = 0;

#ifndef NOTHREADS
	EnterCriticalSection(&task->cs);
#endif

	if (! GetCustomTaskVar(task, id, (LPVOID)&start, NULL)) {
#ifndef NOTHREADS
		LeaveCriticalSection(&task->cs);
#endif
		return FALSE;
	}

	while (start) {
		if (i == idx) {
			*out = start->data;
#ifndef NOTHREADS
			LeaveCriticalSection(&task->cs);
#endif
			return TRUE;
		}
		i++;
		start = start->next;
	}

#ifndef NOTHREADS
	LeaveCriticalSection(&task->cs);
#endif
	return FALSE;
}


BOOL GetNextTask(Task **task) {
	TaskQueue *curr;
	int idx;

#ifndef NOTHREADS
	EnterCriticalSection(&g_TASK_CS);
#endif

	curr = &g_TASK_QUEUE;
	idx = 0;
	while (curr->next) {
		curr = curr->next;
		if (! curr->task->locked && (curr->task->dwNextRun == 0 || GetTickCount() >= curr->task->dwNextRun)) {
			if (idx >= g_CURR_TASK_INDEX) {
				curr->task->locked = TRUE;
				g_CURR_TASK_INDEX++;
				*task = curr->task;

#ifndef NOTHREADS
				LeaveCriticalSection(&g_TASK_CS);
#endif

				return TRUE;
			}
		}

		idx++;
	}
	
	g_CURR_TASK_INDEX = 0;

#ifndef NOTHREADS
	LeaveCriticalSection(&g_TASK_CS);
#endif

	return FALSE;
}


void FreeTask(Task *task) {
	RemoveAllCustomTaskData(task);
	RemoveTask(task);
#ifndef NOTHREADS
	DeleteCriticalSection(&task->cs);
#endif
	GlobalFree((HGLOBAL)task);
}

void InitTaskSystem() {
	memset(&g_TASK_QUEUE, 0, sizeof(TaskQueue));
#ifndef NOTHREADS
	InitializeCriticalSection(&g_TASK_CS);
#endif
}