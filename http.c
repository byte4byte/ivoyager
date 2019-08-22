Task far *GetAvailableWebRequestTask(ContentWindow far *window) {
	return NULL;
}

int FindAvailableWebRequest(ContentWindow far *window) {
	int i;
	for (i = 0; i < NUM_CONCURRENT_REQUESTS; i++) {
		if (window->activeRequests[i] == NULL) return i;
	}
	return -1;
}

BOOL FreeWebRequest(ContentWindow far *window, Task far *task) {
	int i;
	for (i = 0; i < NUM_CONCURRENT_REQUESTS; i++) {
		if (window->activeRequests[i] == task) {
			window->activeRequests[i] = NULL;
			return TRUE;
		}
	}
	return FALSE;
}

void HttpGet(Stream_HTTP far *stream) {
	LPSTR buff = (LPSTR)GlobalAlloc(GMEM_FIXED, 2048);
	wsprintf(buff, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", stream->url_info->path, stream->url_info->domain);
	//LPSTR buf = "GET / HTTP/1.1\r\nHost: ivoyager.online\r\n\r\n";

	send(stream->s, buff, lstrlen(buff), 0);
	GlobalFree((HGLOBAL)buff);
	//Task far *task = GetAvailableWebRequestTask(window);
}

void HttpPost(Stream_HTTP far *stream, unsigned char far *body, int len) {
	//Task far *task = GetAvailableWebRequestTask(window);
}