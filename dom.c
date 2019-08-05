#include "ivoyager.h"
#include "dom.h"
#include "utils.c"

BOOL  ParseDOMChunk(ContentWindow  *window, char  *buff, int len) {
	buff[len] = '\0';
	MessageBox(window->hWnd, buff, "chunk", MB_OK);
	return TRUE;
}