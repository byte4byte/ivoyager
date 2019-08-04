typedef struct {
	int protocol;
	LPSTR path;
	LPSTR domain; // can be null if file
	LPSTR username; // can be null
	LPSTR getvars; // can be null
	int port;
} URL_INFO;

#define HTTP_PROTOCOL 	0
#define HTTPS_PROTOCOL 	1
#define FILE_PROTOCOL 	2

BOOL IsAbsolutePath(LPSTR url) {
	return (BOOL)(url[0] == '\\' || url[0] == '/' || (url[0] && url[1] == ':' && (url[2] == '\\' || url[2] == '/')));
}

// TODO: Wide chars & symlink support
LPSTR NormalizePath(LPSTR path) {
	LPSTR ret = (LPSTR)GlobalAlloc(GMEM_FIXED, lstrlen(path)+3);
	LPSTR ptr = ret, curr_dir_ptr = NULL, prev_dir_ptr = NULL;
	lstrcpy(ret, path);
	
	// lets simply remove dup path seps and normalize path sep
	ptr = ret;
	while (*ptr) {
		
		if (*ptr == '/' || *ptr == '\\') {
			curr_dir_ptr = ptr+1;
			if (*curr_dir_ptr == '/' || *curr_dir_ptr == '\\') {
				while (*curr_dir_ptr) {
					if (! (*(curr_dir_ptr+1) == '/' || *(curr_dir_ptr+1) == '\\')) {
						break;
					}
					curr_dir_ptr++;
				}
				lstrcpy(ptr, curr_dir_ptr);
			}
#if defined(_WIN32) || defined(WIN3_1)
			*ptr = '\\'; // here should be os specific sep	
#else			
			*ptr = '/'; // here should be os specific sep	
#endif
		}
		
		ptr++;
	}
	
	{ // remove a trailing slash
		int len = lstrlen(ret);
		if (len > 1 && (ret[len-1] == '/' || ret[len-1] == '\\')) ret[len-1] = '\0';
	}
	
	// now let's resolve . & ..
	curr_dir_ptr = NULL;
	ptr = ret;
	while (*ptr) {
		if (*ptr == '/' || *ptr == '\\') {
			if (curr_dir_ptr) {
				prev_dir_ptr = curr_dir_ptr;
				curr_dir_ptr = ptr;
			}
			else {
				curr_dir_ptr = ptr;
			}
			if (! _strnicmp(ptr+1, "../", 3) || ! _strnicmp(ptr+1, "..\\", 3) || ! _stricmp(ptr+1, "..") ) {
				
				char *back_ptr = ptr;
				if (! prev_dir_ptr) {
					lstrcpy(ptr, ptr+3);
					continue;
				}
				else {
					lstrcpy(prev_dir_ptr, ptr+3);
					ptr = prev_dir_ptr;
					curr_dir_ptr = NULL;
					back_ptr = ptr-1;
					prev_dir_ptr = NULL;
					for (;;) {
						if (back_ptr == ret) break;
						
						if (*back_ptr == '/' || *back_ptr == '\\') {
							prev_dir_ptr = back_ptr;
							break;
						}
						back_ptr--;
					}
					
					continue;
				}
			}
			else if (! _strnicmp(ptr+1, "./", 2) || ! _strnicmp(ptr+1, ".\\", 2) || ! _stricmp(ptr+1, ".")) {
				curr_dir_ptr = NULL;
				lstrcpy(ptr, ptr+2);
				continue;
			}
		}
		ptr++;
	}
	
#if defined(_WIN32) || defined(WIN3_1)
	if (ret[0] == '/' || ret[0] == '\\') {
		memmove(ret+2, ret, lstrlen(ret)+1);
		ret[0] = 'C';
		ret[1] = ':';
	}
#endif	
	
	return ret;
}

URL_INFO *GetUrlInfo(LPSTR url) {
	URL_INFO *url_info = (URL_INFO *)GlobalAlloc(GMEM_FIXED, sizeof(URL_INFO));
	memset(url_info, 0, sizeof(URL_INFO));
	if (! _strnicmp(url, "http://", 7)) {
		url_info->protocol = HTTP_PROTOCOL;
		url += 7;
	}
	else if (! _strnicmp(url, "https://", 8)) {
		url_info->protocol = HTTPS_PROTOCOL;
		url += 8;
	}
	else if (! _strnicmp(url, "file://", 7)) {
		url_info->protocol = FILE_PROTOCOL;
		url += 7;
	}
	else if (IsAbsolutePath(url)) {
		url_info->protocol = FILE_PROTOCOL;
	}
	
	if (url_info->protocol == FILE_PROTOCOL) {
		url_info->path = NormalizePath(url);
	}
	else {
		LPSTR tmp_url = (LPSTR)GlobalAlloc(GMEM_FIXED, lstrlen(url)+1);
		lstrcpy(tmp_url, url);
		LPSTR username = NULL;
		LPSTR domain = tmp_url;
		LPSTR path = NULL;
		LPSTR getvars = NULL;
		LPSTR ptrPort = NULL;
		int port = url_info->protocol == HTTPS_PROTOCOL ? 443 : 80;
		
		path = strchr(domain, '/');
		if (path) {
			*path = '\0';
			
			getvars = strchr(path+1, '?');
			if (getvars) {
				*getvars = '\0';
				getvars++;
			}
		}
		else {
			getvars = strchr(domain, '?');
			if (getvars) {
				*getvars = '\0';
				getvars++;
			}
		}
		
		username = strchr(tmp_url, '@');
		if (username) {
			*username = '\0';
			domain = username+1;
			
			username = tmp_url;
		}
		
		ptrPort = strchr(domain, ':');
		if (ptrPort) {
			*ptrPort = '\0';
			port = atoi(ptrPort+1);
		}
		
		if (path) {
			url_info->path = GlobalAlloc(GMEM_FIXED, lstrlen(path)+1+1);
			url_info->path[0] = '/';
			lstrcpy(&url_info->path[1], path+1);
		}
		else {
			url_info->path = GlobalAlloc(GMEM_FIXED, 2);
			url_info->path[0] = '/';
			url_info->path[1] = '\0';
		}
		if (username) {
			url_info->username = GlobalAlloc(GMEM_FIXED, lstrlen(username)+1);
			lstrcpy(url_info->username, username);
		}
		
		url_info->domain = GlobalAlloc(GMEM_FIXED, lstrlen(domain)+1);
		lstrcpy(url_info->domain, domain);
		
		if (getvars) {
			url_info->getvars = GlobalAlloc(GMEM_FIXED, lstrlen(getvars)+1);
			lstrcpy(url_info->getvars, getvars);
		}
		
		GlobalFree((HGLOBAL)tmp_url);
	}
	
	return url_info;
}

BOOL FreeUrlInfo(URL_INFO *url_info) {
	if (url_info) {
		if (url_info->path) {
			GlobalFree((HGLOBAL)url_info->path);
		}
		if (url_info->domain) {
			GlobalFree((HGLOBAL)url_info->domain);
		}
		if (url_info->username) {
			GlobalFree((HGLOBAL)url_info->username);
		}
		if (url_info->getvars) {
			GlobalFree((HGLOBAL)url_info->getvars);
		}
		GlobalFree((HGLOBAL)url_info);
		return TRUE;
	}
	return FALSE;
}
