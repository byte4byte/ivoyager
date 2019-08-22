#define WM_FSOCKET      (WM_USER+1)
#define WM_HOSTRESOLVED (WM_USER+2)
#define WM_QUERYDONE    (WM_USER+3)

#define VAR_STREAMS 0

#define STREAM_FILE 0
#define STREAM_HTTP 1

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

typedef struct Stream_HTTP {
	Stream stream; // must come first
	SOCKET s;
	URL_INFO far *url_info;
} Stream_HTTP;

int far readStream_FILE(Stream far *stream, char near *buff, int len) {
	return fread(buff, 1, len, ((Stream_FILE far *)stream)->fp);
}

BOOL far EOFstream_FILE(Stream far *stream) {
	return feof(((Stream_FILE far *)stream)->fp);
}

BOOL far closeStream_FILE(Stream far *stream) {
	return fclose(((Stream_FILE far *)stream)->fp);
}

int far readStream_HTTP(Stream far *stream, char near *buff, int len) {
	return 0;
}

BOOL far EOFstream_HTTP(Stream far *stream) {
	return TRUE;
}

BOOL far closeStream_HTTP(Stream far *stream) {
	return TRUE;
}

Stream far *openStream(Task far *task, ContentWindow far *window, URL_INFO far *url_info) {
	switch (url_info->protocol) {
		case HTTP_PROTOCOL: 
		{
			Stream_HTTP far *ret = (Stream_HTTP far *)GlobalAlloc(GMEM_FIXED, sizeof(Stream_HTTP));
			SocketStream far *ss;
			_fmemset(ret, 0, sizeof(Stream_HTTP));
			ret->s = (SOCKET)socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (ret->s == INVALID_SOCKET) {
				GlobalFree((HGLOBAL)ret);
				return NULL;
			}
			ret->stream.read = readStream_HTTP;
			ret->stream.eof = EOFstream_HTTP;
			ret->stream.close = closeStream_HTTP;
			ret->stream.window = window;
			ret->stream.task = task;
			ret->url_info = url_info;
			
			ss = (SocketStream far *)GlobalAlloc(GMEM_FIXED, sizeof(SocketStream));
			ss->s = ret->s;
			ss->stream = (Stream far *)ret;
			ss->getHostBuf = (char far *)GlobalAlloc(GMEM_FIXED, MAXGETHOSTSTRUCT);
			AddCustomTaskListData(g_socketsTask, VAR_STREAMS, (LPARAM)ss);
			
			WSAAsyncSelect(ret->s, browserWin, WM_FSOCKET,
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