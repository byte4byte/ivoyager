#include "ivoyager.h"
#include "dom.h"

BOOL ParseDOMChunk(ContentWindow *window, char *buff, int len) {
	buff[len-1] = '\0';
	MessageBox(window->hWnd, buff, "chunk", MB_OK);
	return TRUE;
}