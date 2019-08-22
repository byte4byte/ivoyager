#define STREAM_FILE 0
#define STREAM_HTTP 1

typedef struct Stream Stream;

typedef far int (* stream_read)(Stream far *stream, char far *buff, int len);
typedef far BOOL (* stream_eof)(Stream far *stream);
typedef far BOOL (* stream_close)(Stream far *stream);

typedef struct Stream {
	int type;
	stream_read read;
	stream_eof eof;
	stream_close close;
} Stream;

typedef struct Stream_FILE {
	Stream stream; // must come first
	FILE *fp;
} Stream_FILE;

typedef struct Stream_HTTP {
	Stream stream; // must come first
	FILE *fp;
} Stream_HTTP;

int far readStream_FILE(Stream far *stream, char far *buff, int len) {
	return fread(buff, 1, len, ((Stream_FILE far *)stream)->fp);
}

BOOL far EOFstream_FILE(Stream far *stream) {
	return feof(((Stream_FILE far *)stream)->fp);
}

BOOL far closeStream_FILE(Stream far *stream) {
	return fclose(((Stream_FILE far *)stream)->fp);
}

Stream far *openStream(URL_INFO far *url_info) {
	switch (url_info->protocol) {
		case HTTP_PROTOCOL: 
		{
			return NULL;
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
			return (Stream far *)ret;
		}		
	}
	
	return NULL;
}

BOOL closeStream(Stream far *stream) {
	return TRUE;
}