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

typedef struct HttpHeader {
	LPSTR szName;
	LPSTR szValue;
} HttpHeader;

#define PARSE_HTTP_HEADER_STATE	0
#define PARSE_HTTP_DATA_STATE	1

#define HTTP_STATE_VAR			0
#define HTTP_HEADERS_VAR		1
#define HTTP_HEADER_ACCUM_VAR	2

HttpHeader far *ParseHttpHeader(LPSTR szHeader) {
	LPSTR colon = szHeader;
	HttpHeader far *header;
	while (*colon) {
		if (*colon == ':') {
			*colon = '\0';
			colon++;
			break;
		}
		colon++;
	}
	header = (HttpHeader far *)GlobalAlloc(GMEM_FIXED, sizeof(HttpHeader));
	header->szName = Trim(szHeader, 1, 1);
	header->szValue = Trim(colon, 1, 1);
	return header;
}

void HttpGetChunk(Stream_HTTP far *stream, char far *buff, int len, BOOL *headersParsed) {
	LPARAM state;
	int i = 0;
	GetCustomTaskVar(stream->http->parseHttpTask, HTTP_STATE_VAR, &state, NULL);
	if (state == PARSE_HTTP_HEADER_STATE) {
		LPSTR header_chunk = buff;
		for (i = 0; i < len; i++) {
			if (buff[i] == '\n') {
				LPSTR header;
				BOOL end_header = TRUE;
				
				buff[i] = '\0';
				header_chunk = ConcatVar(stream->http->parseHttpTask, HTTP_HEADER_ACCUM_VAR, header_chunk);
				buff[i] = '\n';
				AddCustomTaskVar(stream->http->parseHttpTask, HTTP_HEADER_ACCUM_VAR, (LPARAM)NULL);
				header = header_chunk;
				while (*header) {
					if (*header != '\r' && *header != '\n') {
						end_header = FALSE;
						break;
					}
					header++;
				}
				if (! end_header) {
					HttpHeader far *http_header = ParseHttpHeader(header_chunk);
					AddCustomTaskListData(stream->http->parseHttpTask, HTTP_HEADERS_VAR, (LPARAM)http_header);
					DebugLog(stream->stream.window->tab, "\"%s\": \"%s\"\n", http_header->szName, http_header->szValue);
					header_chunk = &buff[i+1];
				}
				else {
					*headersParsed = TRUE;
					AddCustomTaskVar(stream->http->parseHttpTask, HTTP_STATE_VAR, (LPARAM)PARSE_HTTP_DATA_STATE);
					state = PARSE_HTTP_DATA_STATE;
					len -= (&buff[i+1] - buff);
					buff = &buff[i+1];
					DebugLog(stream->stream.window->tab, "\n\n=============== /HEADERS ==================\n\n");
					break;
				}
			}
		}
		
		if (state == PARSE_HTTP_HEADER_STATE && lstrlen(header_chunk)) ConcatVar(stream->http->parseHttpTask, HTTP_HEADER_ACCUM_VAR, header_chunk);
	}
	
	if (state == PARSE_HTTP_DATA_STATE) {
		DebugLog(stream->stream.window->tab, "%s", buff);
	}
}

void HttpGet(Stream_HTTP far *stream) {
	LPSTR buff = (LPSTR)GlobalAlloc(GMEM_FIXED, 2048);
	wsprintf(buff, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", stream->http->url_info->path, stream->http->url_info->domain);
	//LPSTR buf = "GET / HTTP/1.1\r\nHost: ivoyager.online\r\n\r\n";

	send(stream->http->s, buff, lstrlen(buff), 0);
	GlobalFree((HGLOBAL)buff);
	stream->http->parseHttpTask = AllocTempTask();
	//Task far *task = GetAvailableWebRequestTask(window);
}

void HttpPost(Stream_HTTP far *stream, unsigned char far *body, int len) {
	//Task far *task = GetAvailableWebRequestTask(window);
}