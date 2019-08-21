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

void HttpGet(ContentWindow far *window, URL_INFO *urlinfo) {
	Task far *task = GetAvailableWebRequestTask(window);
}

void HttpPost(ContentWindow far *window, URL_INFO *urlinfo, unsigned char far *body, int len) {
	Task far *task = GetAvailableWebRequestTask(window);
}