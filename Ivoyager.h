#ifndef _IVOYAGER_H_
#define _IVOYAGER_H_


// detect borland turbo C win3.1
#if defined(__TURBOC__) && defined(__MSDOS__) && defined(_Windows)
#define WIN3_1
#elif defined(__WATCOMC__)
#define WIN3_1
#endif

#if defined(_WIN32) || defined(WIN3_1)
#define OS_WINDOWS
#include <windows.h>
#elif defined(OS_LINUX)
#include "Wincross.h"
#ifdef USE_NCURSES
#include <ncurses.h>
#endif
#endif

#ifdef WIN3_1
#include <string.h>
#ifndef NOTHREADS
#define NOTHREADS
#endif
#endif

#include <stddef.h>

void StackTrace();

#define BUFFER_SIZE 1024

#define NUM_THREADS 5

#define TEST_PATH_NORMAL
#define DEBUG_TASK_DATA_FUNCS

typedef struct Tab Tab;
typedef struct ContentWindow ContentWindow;
typedef struct Task Task;

#include "Task.h"
#include "Dom.h"
#include "Tabs.h"

#endif
