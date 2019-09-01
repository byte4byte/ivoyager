#ifndef _IVOYAGER_H_
#define _IVOYAGER_H_

#include <windows.h>

// detect borland turbo C win3.1
#if defined(__TURBOC__) && defined(__MSDOS__) && defined(_Windows)
#define WIN3_1
#elif defined(_WIN16) && ! defined(_WIN32)
#define WIN3_1
#endif

//#define WIN3_1

#ifdef WIN3_1
#include <string.h>
#ifndef NOTHREADS
#define NOTHREADS
#endif
#endif

#include <stddef.h>

#define BUFFER_SIZE 1024

#define NUM_THREADS 5

#define TEST_PATH_NORMAL
#define DEBUG_TASK_DATA_FUNCS

typedef struct Tab Tab;
typedef struct ContentWindow ContentWindow;
typedef struct Task Task;

#include "task.h"
#include "dom.h"
#include "tabs.h"

#endif