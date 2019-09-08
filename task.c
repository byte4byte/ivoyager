#include "ivoyager.h"
#include "task.h"
#include "utils.c"

#ifndef NOTHREADS
static CRITICAL_SECTION g_TASK_CS;
#endif

static TaskQueue g_TASK_QUEUE;
static int g_CURR_TASK_INDEX = 0;

BOOL  RemoveTask(Task  far *task) {

        TaskQueue far *curr;
        TaskQueue far *prev;
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

                        prev->next = curr->next;

                        GlobalFree((void far *)curr);

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

Task far *AllocTempTask() {
        Task far *task = (Task far *)GlobalAlloc(GMEM_FIXED, sizeof(Task));
        _fmemset(task, 0, sizeof(Task));

        #ifndef NOTHREADS
        InitializeCriticalSection(&task->cs);
        #endif
        
        return task;
}

BOOL FreeTempTask(Task far *task) {
        RemoveAllCustomTaskData(task);
#ifndef NOTHREADS
        DeleteCriticalSection(&task->cs);
#endif
        GlobalFree((void far *)task);
        return TRUE;
}

BOOL  AddTask(Task far *task) {
        TaskQueue far *curr;

#ifndef NOTHREADS
        InitializeCriticalSection(&task->cs);
#endif

#ifndef NOTHREADS
        EnterCriticalSection(&g_TASK_CS);
#endif

        curr = &g_TASK_QUEUE;
        do {
                if (! curr->next) {
                        curr->next = (TaskQueue far *)GlobalAlloc(GMEM_FIXED, sizeof(TaskQueue));
                        _fmemset(curr->next, 0, sizeof(TaskQueue));
                        curr = curr->next;
                        curr->task = task;      
                        
#ifndef NOTHREADS
                        LeaveCriticalSection(&g_TASK_CS);
#endif

                        return TRUE;
                }
                curr = curr->next;
        } while(1);

#ifndef NOTHREADS
        LeaveCriticalSection(&g_TASK_CS);
#endif
        
        return FALSE;   
}

static TaskData far * GetTaskData(Task far *task, DWORD id) {
        TaskData far *curr;

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
        }

#ifndef NOTHREADS
        LeaveCriticalSection(&task->cs);
#endif

        return NULL;
}

BOOL  SetCustomTaskData(Task far *task, DWORD id, LPARAM var, BYTE type) {
        TaskData far *curr;

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

void printTaskData(Task far *task) {
        static char buff[1024];
        static char tmpbuff[100];
        int len = 0;
        TaskData far *curr;
        
        strcpy(buff, "");
        
        curr = task->taskData;
        while (curr) {
                wsprintf(tmpbuff, "var: %ld = %ld\n", curr->id, curr->data);
                strcat(buff, tmpbuff);
                len++;
                curr = curr->next;
        } 
        wsprintf(tmpbuff, "len = %d", len);
        strcat(buff, tmpbuff);
        
        MessageBox(NULL, buff, "", MB_OK);
}

BOOL  AddCustomTaskData(Task far *task, DWORD id, LPARAM var, BYTE type) {
        TaskData far *curr;
        
        //if (id == 100) {
        //      DebugLog(NULL, "\nmode change: %ld\n", var);
        //}

        if (SetCustomTaskData(task, id, var, type)) return TRUE;

#ifndef NOTHREADS
        EnterCriticalSection(&task->cs);
#endif

        curr = task->taskData;
        do {
                if (! curr || ! curr->next) {
                        TaskData far *node = (TaskData far *)GlobalAlloc(GMEM_FIXED, sizeof(TaskData));
                        _fmemset(node, 0, sizeof(TaskData));
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

BOOL  AddCustomTaskVar(Task far *task, DWORD id, LPARAM var) {
        return AddCustomTaskData(task, id, var, TASK_DATA_VAR);
}

BOOL  RemoveCustomTaskVar(Task far *task, DWORD id) {
        TaskData far *curr;
        TaskData far *prev;

#ifndef NOTHREADS
        EnterCriticalSection(&task->cs);
#endif

        curr = task->taskData;
        prev = task->taskData;
        while (curr) {
                if (curr->id == id) {
                        if (prev != task->taskData) {
                                prev->next = curr->next;
                                GlobalFree((void far *)curr);
                        }
                        else {
                                task->taskData = curr->next;
                                GlobalFree((void far *)curr);
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

BOOL  GetCustomTaskVar(Task far *task, DWORD id, LPARAM far *var, TaskData far * far *parentTaskData) {
        TaskData far *curr;

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

BOOL  FreeCustomTaskListData(Task far *task, DWORD id, int freemode) {
    TaskData far *taskdata;
    LPARAM var;
	
	OutputDebugString("free");

#ifndef NOTHREADS
        EnterCriticalSection(&task->cs);
#endif
    
    if (! GetCustomTaskVar(task, id, &var, &taskdata)) {
#ifndef NOTHREADS
        LeaveCriticalSection(&task->cs);
#endif        
        return FALSE;
    }

    if (taskdata->type == TASK_DATA_LIST) {
            TaskData far *lstart;
            TaskData far *lprev;
            TaskData far *lnode;

            lstart = lnode = lprev = (TaskData far *)taskdata->data;
            while (lnode) {
                if (lnode->data) {
                    if (freemode == GFREE) GlobalFree((void far *)lnode->data);
                    else if (freemode == LFREE) LocalFree((void far *)lnode->data);
                    lnode->data = 0;
                    
                }
                lprev = lnode;
                lnode = lnode->next;
				GlobalFree((void far *)lprev);
            }
			
			taskdata->data = 0;
    }
    else {
        if (taskdata->data) {
            if (freemode == GFREE) GlobalFree((void far *)taskdata->data);
            else if (freemode == LFREE) LocalFree((void far *)taskdata->data);
            taskdata->data = 0;
        }
    }

#ifndef NOTHREADS
        LeaveCriticalSection(&task->cs);
#endif

    return RemoveCustomTaskVar(task, id);
}

BOOL  RemoveAllCustomTaskData(Task far *task) {
        TaskData far *start;
        TaskData far *prev;
        TaskData far *node;

#ifndef NOTHREADS
        EnterCriticalSection(&task->cs);
#endif

        start = node = prev = task->taskData;
        while (node) {
                
                if (node->type == TASK_DATA_LIST) {
                        TaskData far *lstart;
                        TaskData far *lprev;
                        TaskData far *lnode;

                        lstart = lnode = lprev = (TaskData far *)node->data;
                        while (lnode) {
                                lprev = lnode;
                                lnode = lnode->next;
                                GlobalFree((void far *)lprev);
                        }
                        node->data = 0;
                }

                prev = node;
                node = node->next;

                GlobalFree((void far *)prev);
        }

        task->taskData = NULL;

#ifndef NOTHREADS
        LeaveCriticalSection(&task->cs);
#endif
        return TRUE;
}

int  GetNumCustomTaskListData(Task far *task, DWORD id) {
        TaskData far *start;
        int idx = 0;

#ifndef NOTHREADS
        EnterCriticalSection(&task->cs);
#endif

        if (! GetCustomTaskVar(task, id, (LPARAM far *)&start, NULL)) {
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


int  AddCustomTaskListData(Task far *task, DWORD id, LPARAM data) {
        TaskData far *start;
        int idx = 0;

        TaskData far *node = (TaskData far *)GlobalAlloc(GMEM_FIXED, sizeof(TaskData));
        _fmemset(node, 0, sizeof(TaskData));
        node->data = data;

#ifndef NOTHREADS
        EnterCriticalSection(&task->cs);
#endif

        if (GetCustomTaskVar(task, id, (LPARAM far *)&start, NULL) && start) {
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
                        GlobalFree((void far *)node);
                        return -1;
                }
        }

#ifndef NOTHREADS
        LeaveCriticalSection(&task->cs);
#endif
        return idx;
}

BOOL  RemoveCustomTaskListData(Task far *task, DWORD id, int idx) {
        TaskData far *start;
        TaskData far *node;
        TaskData far *prev;
        TaskData far *parent;
        int i = 0;

#ifndef NOTHREADS
        EnterCriticalSection(&task->cs);
#endif

        if (! GetCustomTaskVar(task, id, (LPARAM far *)&start, (TaskData far * far *)&parent)) {
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
                                GlobalFree((void far *)node);
                                if (! parent->data) RemoveCustomTaskVar(task, id);
                        }
                        else {
                                prev->next = node->next;
                                GlobalFree((void far *)node);
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

int  GetCustomTaskListIdxByData(Task  far *task, DWORD id, LPARAM data) {
        TaskData far *start;

        int ret = 0;

#ifndef NOTHREADS
        EnterCriticalSection(&task->cs);
#endif

        if (! GetCustomTaskVar(task, id, (LPVOID )&start, NULL)) {
#ifndef NOTHREADS
                LeaveCriticalSection(&task->cs);
#endif
                return FALSE;
        }

        while (start) {
                if (start->data == data) {
#ifndef NOTHREADS
                        LeaveCriticalSection(&task->cs);
#endif
                        return ret;
                }

                ret++;
                start = start->next;
        }

#ifndef NOTHREADS
        LeaveCriticalSection(&task->cs);
#endif
        return -1;
}

BOOL  GetCustomTaskListDataByStringField(Task  far *task, DWORD id, char far *string, int ptrOffset, BOOL caseInsensitive, LPARAM  far *out) {
        TaskData far *start;

        *out = 0;

#ifndef NOTHREADS
        EnterCriticalSection(&task->cs);
#endif

		//OutputDebugString("GetCustomTaskListDataByStringField");
		//OutputDebugString(string);

        if (! GetCustomTaskVar(task, id, (LPVOID )&start, NULL)) {
#ifndef NOTHREADS
                LeaveCriticalSection(&task->cs);
#endif
                return FALSE;
        }

        while (start) {
                char far * far *ptr;
				BOOL found;
				ptr = (char far * far *)start->data + ptrOffset;
				//OutputDebugString("here1");
				//OutputDebugString(*ptr);
                found = (caseInsensitive ? (lstrcmpi(string, *ptr) == 0) : (lstrcmp(string, *ptr) == 0));
                //MessageBox(NULL, *ptr, "", MB_OK);
				//OutputDebugString("here2");
                if (found) {
                        *out = start->data;
#ifndef NOTHREADS
                        LeaveCriticalSection(&task->cs);
#endif
                        return TRUE;
                }
                start = start->next;
        }

#ifndef NOTHREADS
        LeaveCriticalSection(&task->cs);
#endif
        return FALSE;
}


BOOL  GetCustomTaskListDataByPtrField(Task  far *task, DWORD id, char far *val, int ptrOffset, int numbytes, LPARAM  far *out) {
        TaskData far *start;

        *out = 0;

#ifndef NOTHREADS
        EnterCriticalSection(&task->cs);
#endif

        if (! GetCustomTaskVar(task, id, (LPVOID )&start, NULL)) {
#ifndef NOTHREADS
                LeaveCriticalSection(&task->cs);
#endif
                return FALSE;
        }

        while (start) {
                char far *ptr = (char far *)start->data + ptrOffset;
                int c;
                BOOL found = TRUE;
                for (c = 0; c< numbytes; c++) {
                        if (ptr[c] != val[c]) {
                                found = FALSE;
                                break;
                        }
                }
                if (found) {
                        *out = start->data;
#ifndef NOTHREADS
                        LeaveCriticalSection(&task->cs);
#endif
                        return TRUE;
                }
                start = start->next;
        }

#ifndef NOTHREADS
        LeaveCriticalSection(&task->cs);
#endif
        return FALSE;
}

BOOL  GetCustomTaskListData(Task  far *task, DWORD id, int idx, LPARAM  far *out) {
        TaskData far *start;
        int i = 0;

        *out = 0;

#ifndef NOTHREADS
        EnterCriticalSection(&task->cs);
#endif

        if (! GetCustomTaskVar(task, id, (LPVOID )&start, NULL)) {
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


BOOL  GetNextTask(Task far * far *task) {
        TaskQueue far *curr;
        int idx;

#ifndef NOTHREADS
        EnterCriticalSection(&g_TASK_CS);
#endif

        curr = &g_TASK_QUEUE;
        idx = 0;
        while (curr->next) {
                
                //if (idx > 0) MessageBox(NULL, "found", "", MB_OK);
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


void  FreeTask(Task far *task) {
        RemoveAllCustomTaskData(task);
        RemoveTask(task);
#ifndef NOTHREADS
        DeleteCriticalSection(&task->cs);
#endif
        GlobalFree((void far *)task);
}

void  InitTaskSystem() {
        _fmemset(&g_TASK_QUEUE, 0, sizeof(TaskQueue));
#ifndef NOTHREADS
        InitializeCriticalSection(&g_TASK_CS);
#endif
}
