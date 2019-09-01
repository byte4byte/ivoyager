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
#define HTTP_ENCODING_VAR		3
#define HTTP_CHUNK_STATE_VAR	4
#define HTTP_CHUNK_SIZE_VAR		5
#define HTTP_CHUNK_SIZE_INT_VAR	6

#define HTTP_ENCODING_NORMAL	0
#define HTTP_ENCODING_CHUNKED	1

#define CHUNK_STATE_SIZE		0
#define CHUNK_STATE_DATA		1

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
					
					
					{
						LPARAM location_header;
						GetCustomTaskListDataByStringField(((Stream_HTTP far *)stream)->http->parseHttpTask, HTTP_HEADERS_VAR, "Location", offsetof(HttpHeader, szName), TRUE, &location_header);
						if (location_header) {
							DebugLog(stream->stream.window->tab, "-------------------------------------\n");
							SetWindowText(stream->stream.window->tab->hSource, "");
							redirectStream((Stream_HTTP far *)stream, ((HttpHeader far *)location_header)->szValue);
							return;
						}
						else {
							AddCustomTaskVar(((Stream far *)stream)->task, RUN_TASK_VAR_CONNECT_STATE, CONNECT_STATE_READY);
						}
					}
					
					{
						LPARAM encoding_header;
						GetCustomTaskListDataByStringField(stream->http->parseHttpTask, HTTP_HEADERS_VAR, "Transfer-Encoding", offsetof(HttpHeader, szName), TRUE, &encoding_header);
						if (encoding_header && ! _stricmp(((HttpHeader far *)encoding_header)->szValue, "chunked")) {
							AddCustomTaskVar(stream->http->parseHttpTask, HTTP_ENCODING_VAR, (LPARAM)HTTP_ENCODING_CHUNKED);
						}
					}
					
					DebugLog(stream->stream.window->tab, "\n\n=============== /HEADERS ==================\n\n");
					break;
				}
			}
		}
		
		if (state == PARSE_HTTP_HEADER_STATE && lstrlen(header_chunk)) ConcatVar(stream->http->parseHttpTask, HTTP_HEADER_ACCUM_VAR, header_chunk);
	}
	
	if (state == PARSE_HTTP_DATA_STATE) {
		LPARAM encoding;
		GetCustomTaskVar(stream->http->parseHttpTask, HTTP_ENCODING_VAR, &encoding, NULL);
		if (encoding == HTTP_ENCODING_NORMAL) {
			LPSTR ptr = (LPSTR)GlobalAlloc(GMEM_FIXED, lstrlen(buff)+1);
			lstrcpy(ptr, buff);
			AddCustomTaskListData(stream->chunksTask, HTTP_CHUNK_LIST_VAR, (LPARAM)ptr);
		}
		else if (encoding == HTTP_ENCODING_CHUNKED) {
			LPARAM chunk_state;
			GetCustomTaskVar(stream->http->parseHttpTask, HTTP_CHUNK_STATE_VAR, &chunk_state, NULL);
			for (;;) {
				if (chunk_state == CHUNK_STATE_SIZE) {
					char far *p = buff;
					while (*p) {
						if (*p == '\n') {
							LPARAM chunk_size;
                                                        LPSTR fullsize;
							*p = '\0';
							//MessageBox(browserWin, "here", "", MB_OK);
							fullsize = Trim(ConcatVar(stream->http->parseHttpTask, HTTP_CHUNK_SIZE_VAR, buff), 1, 1);
							if (! lstrcmp(fullsize, "")) {
								*p = '\n';
								p++;
								continue;
							}
							AddCustomTaskVar(stream->http->parseHttpTask, HTTP_CHUNK_SIZE_VAR, (LPARAM)NULL);
							
							chunk_size = _fstrtoul(fullsize, NULL, 16);
							AddCustomTaskVar(stream->http->parseHttpTask, HTTP_CHUNK_SIZE_INT_VAR, (LPARAM)chunk_size);
							
							GlobalFree((HGLOBAL)fullsize);
							
							
							len -= ((p+1) - buff);
							buff = p+1;
							if (chunk_size > 0) {
								chunk_state = CHUNK_STATE_DATA;
								AddCustomTaskVar(stream->http->parseHttpTask, HTTP_CHUNK_STATE_VAR, (LPARAM)CHUNK_STATE_DATA);
								//DebugLog(stream->stream.window->tab, "Chunk size: %ld\n", chunk_size);
							}
							break;
						}
						p++;
					}
					if (chunk_state == CHUNK_STATE_SIZE) {
						ConcatVar(stream->http->parseHttpTask, HTTP_CHUNK_SIZE_VAR, buff);
						break;
					}
				}
				if (chunk_state == CHUNK_STATE_DATA) {
					LPARAM chunk_size;
					GetCustomTaskVar(stream->http->parseHttpTask, HTTP_CHUNK_SIZE_INT_VAR, &chunk_size, NULL);
					if (chunk_size >= len) {
						
						LPSTR ptr = (LPSTR)GlobalAlloc(GMEM_FIXED, lstrlen(buff)+1);
						lstrcpy(ptr, buff);
						AddCustomTaskListData(stream->chunksTask, HTTP_CHUNK_LIST_VAR, (LPARAM)ptr);
						
						chunk_size -= len;
						AddCustomTaskVar(stream->http->parseHttpTask, HTTP_CHUNK_SIZE_INT_VAR, (LPARAM)chunk_size);
						break;
					}
					else {
						char chRestore = buff[chunk_size];
						LPSTR ptr = (LPSTR)GlobalAlloc(GMEM_FIXED, chunk_size+1);
						buff[chunk_size] = '\0';
						lstrcpy(ptr, buff);
						AddCustomTaskListData(stream->chunksTask, HTTP_CHUNK_LIST_VAR, (LPARAM)ptr);
						buff[chunk_size] = chRestore;
						
						buff += (chunk_size+1);
						len -= (chunk_size+1);
						chunk_state = CHUNK_STATE_SIZE;
						AddCustomTaskVar(stream->http->parseHttpTask, HTTP_CHUNK_STATE_VAR, (LPARAM)chunk_state);
						chunk_size = 0;
						AddCustomTaskVar(stream->http->parseHttpTask, HTTP_CHUNK_SIZE_INT_VAR, (LPARAM)chunk_size);
					}
				}
			}
		}
		//DebugLog(stream->stream.window->tab, "%s", buff);
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