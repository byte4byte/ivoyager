#define WM_FSOCKET      (WM_USER+1)
#define WM_HOSTRESOLVED (WM_USER+2)
#define WM_QUERYDONE    (WM_USER+3)

#define VAR_STREAMS 0

#define STREAM_FILE 0
#define STREAM_HTTP 1

#define HTTP_CHUNK_LIST_VAR 1

typedef struct Stream Stream;

typedef int (far * stream_read)(Stream far *stream, char far *buff, int len);
typedef BOOL (far * stream_eof)(Stream far *stream);
typedef BOOL (far * stream_close)(Stream far *stream);
typedef BOOL (far * stream_hasData)(Stream far *stream);

typedef struct Stream {
        int type;
        stream_read read;
        stream_eof eof;
        stream_close close;
        stream_hasData hasData;
        ContentWindow far *window;
        Task far *task;
		URL_INFO far *url_info;
} Stream;

typedef struct SocketStream {
        SOCKET s;
        HANDLE hGetHostname;
        char far *getHostBuf; //[MAXGETHOSTSTRUCT];
        Stream far *stream;
} SocketStream;

typedef struct Stream_FILE {
        Stream stream; // must come first
#ifdef WIN3_1
        HFILE fp;
#else
        FILE *fp;
#endif
} Stream_FILE;

typedef struct Stream_http {
        SOCKET s;
        Task far *parseHttpTask;
        int redirectCount;
} Stream_http;

typedef struct Stream_HTTP {
        Stream stream; // must come first
        Task far *chunksTask;
        Stream_http far *http;
                SocketStream *ss;
} Stream_HTTP;

int far readStream_FILE(Stream far *stream, char far *buff, int len) {
#ifdef WIN3_1
        int ret = _lread(((Stream_FILE far *)stream)->fp, buff, len);
#else
        int ret = fread(buff, 1, len, ((Stream_FILE far *)stream)->fp);
#endif
        buff[ret] = '\0';
        WriteSource(stream->window->tab, buff, ret);
        return ret;
}

BOOL far EOFstream_FILE(Stream far *stream) {
#ifdef WIN3_1
        LONG end_pos, curr_pos;
        curr_pos = _llseek(((Stream_FILE far *)stream)->fp, 0, 1);
        end_pos = _llseek(((Stream_FILE far *)stream)->fp, 0, 2);
        if (curr_pos == end_pos) return TRUE;
        _llseek(((Stream_FILE far *)stream)->fp, curr_pos, 0);
        return FALSE;
#else  
        return feof(((Stream_FILE far *)stream)->fp);
#endif
}

BOOL far hasDataStream_FILE(Stream far *stream) {
        return TRUE; //!EOFstream_FILE(stream);
}

BOOL far closeStream_FILE(Stream far *stream) {
#ifdef WIN3_1
        return _lclose(((Stream_FILE far *)stream)->fp) == 0;

#else
        return fclose(((Stream_FILE far *)stream)->fp);
#endif
}



int far readStream_HTTP(Stream far *stream, char far *buff, int len) {
        LPARAM read_chunk_ptr;
        
        //DebugLog(NULL, "\nread1\n");

        if (! GetCustomTaskListData(((Stream_HTTP far *)stream)->chunksTask, HTTP_CHUNK_LIST_VAR, 0, (LPARAM far *)&read_chunk_ptr)) {
                //DebugLog(NULL, "\nread2\n");
                return 0;
        }
        
        //DebugLog(NULL, "\nread3\n");
        lstrcpy(buff, (LPSTR)read_chunk_ptr);
        
        GlobalFree((void far *)read_chunk_ptr);
        //DebugLog(NULL, "\nread4\n");
        RemoveCustomTaskListData(((Stream_HTTP far *)stream)->chunksTask, HTTP_CHUNK_LIST_VAR, 0);
        
        //DebugLog(NULL, "\nread5\n");
        
        return lstrlen(buff);
}

BOOL far hasDataStream_HTTP(Stream far *stream) {
        return GetNumCustomTaskListData(((Stream_HTTP far *)stream)->chunksTask, HTTP_CHUNK_LIST_VAR);
}

BOOL far EOFstream_HTTP(Stream far *stream) {
        LPARAM connect_state;
        GetCustomTaskVar(stream->task, RUN_TASK_VAR_CONNECT_STATE, &connect_state, NULL);
        return connect_state == CONNECT_STATE_CLOSED/* && GetNumCustomTaskListData(((Stream_HTTP FAR *)stream)->chunksTask, HTTP_CHUNK_LIST_VAR) == 0*/;
}

BOOL far closeStream_HTTP(Stream far *stream) {
	#define HTTP_HEADERS_VAR                1
    
	Stream_HTTP far *http_stream = (Stream_HTTP far *)stream;
                
	if (http_stream->ss)
	{
		int idx = GetCustomTaskListIdxByData(g_socketsTask, VAR_STREAMS, (LPARAM)http_stream->ss);
		RemoveCustomTaskListData(g_socketsTask, VAR_STREAMS, idx);
		if (http_stream->ss->s != INVALID_SOCKET) closesocket(http_stream->ss->s);
		http_stream->ss->s = INVALID_SOCKET;
		GlobalFree((LPVOID)http_stream->ss);
	}
                
	if (http_stream->chunksTask) {
		FreeCustomTaskListData(http_stream->chunksTask, HTTP_CHUNK_LIST_VAR, GFREE);
		FreeTempTask(http_stream->chunksTask);
	}
	if (http_stream->http->parseHttpTask) {
		FreeCustomTaskListData(http_stream->http->parseHttpTask, HTTP_HEADERS_VAR, LFREE);
		FreeTempTask(http_stream->http->parseHttpTask);
	}
	FreeUrlInfo(http_stream->stream.url_info);	
	
	GlobalFree(http_stream->http);
	GlobalFree(http_stream);
	
	return TRUE;
}

BOOL redirectStream(Stream_HTTP far *ret, LPSTR szUrl) {
        URL_INFO far *new_url_info;
        SocketStream far *ss;
        
		AddCustomTaskVar(((Stream far *)ret)->task, RUN_TASK_VAR_STATE, RUN_TASK_STATE_CONNECTING);
        AddCustomTaskVar(((Stream far *)ret)->task, RUN_TASK_VAR_CONNECT_STATE, CONNECT_STATE_CONNECTING);
	        
		if (ret->ss) 
		{
			int idx = GetCustomTaskListIdxByData(g_socketsTask, VAR_STREAMS, (LPARAM)ret->ss);
			RemoveCustomTaskListData(g_socketsTask, VAR_STREAMS, idx);
			if (ret->ss->s != INVALID_SOCKET) closesocket(ret->ss->s);
			ret->ss->s = INVALID_SOCKET;
			GlobalFree((LPVOID)ret->ss);
			ret->ss = NULL;
		}
		
		if (ret) {
			FreeCustomTaskListData(ret->chunksTask, HTTP_CHUNK_LIST_VAR, GFREE);
			FreeTempTask(ret->chunksTask);
			ret->chunksTask = AllocTempTask();
		}
        
		
		new_url_info = GetUrlInfo(szUrl, ret->stream.url_info);
		
		FreeCustomTaskListData(ret->stream.task, RUN_TASK_VAR_CHUNKS, GFREE);
		               
        FreeCustomTaskListData(ret->http->parseHttpTask, HTTP_HEADERS_VAR, LFREE);
		FreeTempTask(ret->http->parseHttpTask);
		ret->http->parseHttpTask = NULL;
        FreeUrlInfo(ret->stream.url_info);
                
        GlobalFree((void far *)ret->http);

        ret->http = (Stream_http far *)GlobalAlloc(GMEM_FIXED, sizeof(Stream_http));
        _fmemset(ret->http, 0, sizeof(Stream_http));
        
        ret->http->s = (SOCKET)socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (ret->http->s == INVALID_SOCKET) {
			GlobalFree((void far *)ret);
			return FALSE;
        }
        
        ret->stream.url_info = new_url_info;
                
        ss = (SocketStream far *)GlobalAlloc(GMEM_FIXED, sizeof(SocketStream));
        ss->s = ret->http->s;
        ss->stream = (Stream far *)ret;
        ss->getHostBuf = (char far *)GlobalAlloc(GMEM_FIXED, MAXGETHOSTSTRUCT);
                
        ret->ss = ss;
                
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
                                GlobalFree((void far *)ret);
                                return NULL;
                        }
                        ret->stream.read = readStream_HTTP;
                        ret->stream.eof = EOFstream_HTTP;
                        ret->stream.close = closeStream_HTTP;
                        ret->stream.hasData = hasDataStream_HTTP;
                        ret->stream.window = window;
                        ret->stream.task = task;
                        ret->stream.url_info = url_info;
                        
                        ss = (SocketStream far *)GlobalAlloc(GMEM_FIXED, sizeof(SocketStream));
                        ss->s = ret->http->s;
                        ss->stream = (Stream far *)ret;
                        ss->getHostBuf = (char far *)GlobalAlloc(GMEM_FIXED, MAXGETHOSTSTRUCT);
                                                
                                                ret->ss = ss;
                                                
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
#ifdef WIN3_1
                        {
                              ret->fp = _lopen(url_info->path, OF_READ | OF_SHARE_DENY_NONE);
                              if (ret->fp == HFILE_ERROR) {
                                GlobalFree((void far *)ret);
                                return NULL;
                              }
                        }
#else
                        ret->fp = _ffopen(url_info->path, "rb");
                        if (! ret->fp) {
                                GlobalFree((void far *)ret);
                                return NULL;
                        }
						
#endif

						ret->stream.url_info = url_info;

                        ret->stream.read = readStream_FILE;
                        ret->stream.eof = EOFstream_FILE;
                        ret->stream.close = closeStream_FILE;
                        ret->stream.hasData = hasDataStream_FILE;
                        ret->stream.window = window;
                        ret->stream.task = task;
                        return (Stream far *)ret;
                }               
        }
        
        return NULL;
}

BOOL closeStream(Stream far *stream) {
    return stream->close(stream);
}
