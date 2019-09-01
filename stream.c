#define WM_FSOCKET      (WM_USER+1)
#define WM_HOSTRESOLVED (WM_USER+2)
#define WM_QUERYDONE    (WM_USER+3)

#define VAR_STREAMS 0

#define STREAM_FILE 0
#define STREAM_HTTP 1

#define HTTP_CHUNK_LIST_VAR 1

typedef struct Stream Stream;

typedef far int (* stream_read)(Stream far *stream, char near *buff, int len);
typedef far BOOL (* stream_eof)(Stream far *stream);
typedef far BOOL (* stream_close)(Stream far *stream);

typedef struct Stream {
	int type;
	stream_read read;
	stream_eof eof;
	stream_close close;
	ContentWindow far *window;
	Task far *task;
} Stream;

typedef struct SocketStream {
	SOCKET s;
	HANDLE hGetHostname;
	char far *getHostBuf; //[MAXGETHOSTSTRUCT];
	Stream far *stream;
} SocketStream;

typedef struct Stream_FILE {
	Stream stream; // must come first
	FILE *fp;
} Stream_FILE;

typedef struct Stream_http {
	SOCKET s;
	URL_INFO far *url_info;
	Task far *parseHttpTask;
	int redirectCount;
} Stream_http;

typedef struct Stream_HTTP {
	Stream stream; // must come first
	Task far *chunksTask;
	Stream_http far *http;
} Stream_HTTP;

int far readStream_FILE(Stream far *stream, char near *buff, int len) {
	int ret = fread(buff, 1, len, ((Stream_FILE far *)stream)->fp);
	buff[ret] = '\0';
	WriteSource(stream->window->tab, buff, ret);
	return ret;
}

BOOL far EOFstream_FILE(Stream far *stream) {
	return feof(((Stream_FILE far *)stream)->fp);
}

BOOL far closeStream_FILE(Stream far *stream) {
	return fclose(((Stream_FILE far *)stream)->fp);
}

int far readStream_HTTP(Stream far *stream, char near *buff, int len) {
	LPARAM read_chunk_ptr;

	if (! GetCustomTaskListData(((Stream_HTTP far *)stream)->chunksTask, HTTP_CHUNK_LIST_VAR, 0, &read_chunk_ptr)) {
		return 0;
	}
	
	lstrcpy(buff, (LPSTR)read_chunk_ptr);
	
	GlobalFree((HGLOBAL)read_chunk_ptr);
	RemoveCustomTaskListData(((Stream_HTTP far *)stream)->chunksTask, HTTP_CHUNK_LIST_VAR, 0);
	
	return lstrlen(buff);
}

BOOL far EOFstream_HTTP(Stream far *stream) {
	LPARAM connect_state;
	GetCustomTaskVar(stream->task, RUN_TASK_VAR_CONNECT_STATE, &connect_state, NULL);
	return connect_state == CONNECT_STATE_CLOSED/* && GetNumCustomTaskListData(((Stream_HTTP FAR *)stream)->chunksTask, HTTP_CHUNK_LIST_VAR) == 0*/;
}

BOOL far closeStream_HTTP(Stream far *stream) {
	return TRUE;
}

BOOL redirectStream(Stream_HTTP far *ret, LPSTR szUrl) {
	URL_INFO far *new_url_info;
	SocketStream far *ss;
	
	AddCustomTaskVar(((Stream far *)ret)->task, RUN_TASK_VAR_CONNECT_STATE, CONNECT_STATE_CONNECTING);
	
	closesocket(ret->http->s);
	new_url_info = GetUrlInfo(szUrl, ret->http->url_info);
	GlobalFree((HGLOBAL)ret->http);
	
	ret->http = (Stream_http far *)GlobalAlloc(GMEM_FIXED, sizeof(Stream_http));
	_fmemset(ret->http, 0, sizeof(Stream_http));
	
	ret->http->s = (SOCKET)socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ret->http->s == INVALID_SOCKET) {
		GlobalFree((HGLOBAL)ret);
		return FALSE;
	}
	//ret->stream.read = readStream_HTTP;
	//ret->stream.eof = EOFstream_HTTP;
	//ret->stream.close = closeStream_HTTP;
	//ret->stream.window = window;
	//ret->stream.task = task;
	ret->http->url_info = new_url_info;
		
	ss = (SocketStream far *)GlobalAlloc(GMEM_FIXED, sizeof(SocketStream));
	ss->s = ret->http->s;
	ss->stream = (Stream far *)ret;
	ss->getHostBuf = (char far *)GlobalAlloc(GMEM_FIXED, MAXGETHOSTSTRUCT);
	AddCustomTaskListData(g_socketsTask, VAR_STREAMS, (LPARAM)ss);
	
	WSAAsyncSelect(ret->http->s, browserWin, WM_FSOCKET,
					  FD_CONNECT|FD_CLOSE|FD_READ|FD_WRITE);
					  
	ss->hGetHostname = WSAAsyncGetHostByName(browserWin, WM_HOSTRESOLVED,
							 new_url_info->domain,
							 ss->getHostBuf, MAXGETHOSTSTRUCT);	
	
	return TRUE;
}

Stream far *openStream(Task far *task, ContentWindow far *window, URL_INFO far *url_info) {
	switch (url_info->protocol) {
		case HTTP_PROTOCOL: 
		{
			//Stream_http *http;
			Stream_HTTP far *ret = (Stream_HTTP far *)GlobalAlloc(GMEM_FIXED, sizeof(Stream_HTTP));
			SocketStream far *ss;
			_fmemset(ret, 0, sizeof(Stream_HTTP));
			ret->chunksTask = AllocTempTask();
			ret->http = (Stream_http far *)GlobalAlloc(GMEM_FIXED, sizeof(Stream_http));
			_fmemset(ret->http, 0, sizeof(Stream_http));
			
			ret->http->s = (SOCKET)socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (ret->http->s == INVALID_SOCKET) {
				GlobalFree((HGLOBAL)ret);
				return NULL;
			}
			ret->stream.read = readStream_HTTP;
			ret->stream.eof = EOFstream_HTTP;
			ret->stream.close = closeStream_HTTP;
			ret->stream.window = window;
			ret->stream.task = task;
			ret->http->url_info = url_info;
			
			ss = (SocketStream far *)GlobalAlloc(GMEM_FIXED, sizeof(SocketStream));
			ss->s = ret->http->s;
			ss->stream = (Stream far *)ret;
			ss->getHostBuf = (char far *)GlobalAlloc(GMEM_FIXED, MAXGETHOSTSTRUCT);
			AddCustomTaskListData(g_socketsTask, VAR_STREAMS, (LPARAM)ss);
			
			WSAAsyncSelect(ret->http->s, browserWin, WM_FSOCKET,
                              FD_CONNECT|FD_CLOSE|FD_READ|FD_WRITE);
							  
			ss->hGetHostname = WSAAsyncGetHostByName(browserWin, WM_HOSTRESOLVED,
                                     url_info->domain,
                                     ss->getHostBuf, MAXGETHOSTSTRUCT);							  
			
			return (Stream far *)ret;
		}
		case FILE_PROTOCOL:
		{
			Stream_FILE far *ret = (Stream_FILE far *)GlobalAlloc(GMEM_FIXED, sizeof(Stream_FILE));
			_fmemset(ret, 0, sizeof(Stream_FILE));
			ret->fp = _ffopen(url_info->path, "rb");
			if (! ret->fp) {
				GlobalFree((HGLOBAL)ret);
				return NULL;
			}
			ret->stream.read = readStream_FILE;
			ret->stream.eof = EOFstream_FILE;
			ret->stream.close = closeStream_FILE;
			ret->stream.window = window;
			ret->stream.task = task;
			return (Stream far *)ret;
		}		
	}
	
	return NULL;
}

BOOL closeStream(Stream far *stream) {
	return TRUE;
}