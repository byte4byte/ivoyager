#ifndef _UTILS_C
#define _UTILS_C

#ifdef WIN3_1

static void far *GAlloc(int len) {
	void far *ret = (void far *)GlobalLock(GlobalAlloc(GMEM_FIXED, len));
	if (! ret) MessageBox(NULL, "GlobalAlloc FAILED", "", MB_OK);
	return ret;
}


static void *LAlloc(int len) {
	void *ret = (void *)LocalAlloc(LMEM_FIXED, len);
	if (! ret) MessageBox(NULL, "LocalAlloc FAILED", "", MB_OK);
	return ret;
}

static void GFree(HGLOBAL ptr) {
	GlobalFree(GlobalHandle((UINT)ptr));
}

static void LFree(HLOCAL ptr) {
	LocalFree((HLOCAL)ptr);
}

#define GlobalAlloc(x, y) GAlloc(y)
#define LocalAlloc(x, y) LAlloc(y)
#define LocalFree(x) LFree(x)
#define GlobalFree(x) GFree(x)

#endif

static char near *FarStrToNear(const char far *str) {
	char near *l_str = (char near *)LocalAlloc(LMEM_FIXED, lstrlen(str)+1);
	char near *l_str_beg = l_str;
	while (*str) {
	*l_str = *str;
	l_str++;
	str++;
	}
	*l_str = '\0';
	return l_str_beg;
}

static char near *FarPtrToNear(const char far *str, int len) {
	char near *l_str = (char near *)LocalAlloc(LMEM_FIXED, len);
	int i;
	char near *l_str_beg = l_str;
	for (i = 0; i < len; i++) {
		*l_str = *str;
		l_str++;
		str++;
	}
	return l_str_beg;
}

static char far *NearPtrToFar(const char near *str, int len) {
	char far *l_str = (char far *)GlobalAlloc(GMEM_FIXED, len);
	int i;
	char far *l_str_beg = l_str;
	for (i = 0; i < len; i++) {
		*l_str = *str;
		l_str++;
		str++;
	}
	return l_str_beg;
}


static char far *NearStrToFar(const char near *str) {
	char far *l_str = (char far *)GlobalAlloc(GMEM_FIXED, strlen(str)+1);
	char far *l_str_beg = l_str;
	while (*str) {
	*l_str = *str;
	l_str++;
	str++;
	}
	*l_str = '\0';
	return l_str_beg;
}

#ifdef WIN3_1

#include <ctype.h>
#include <stdio.h>

static int
 _stricmp(const char  far *s1, const char  far *s2)
 {
   while (toupper(*s1) == toupper(*s2))
   {
     if (*s1 == 0)
       return 0;
     s1++;
     s2++;
   }
   return toupper(*(unsigned const char *)s1) - toupper(*(unsigned const char *)(s2));
 }

static int
_strnicmp(char const far *s1, char const far *s2, size_t len)
{
	unsigned char c1 = '\0';
	unsigned char c2 = '\0';
	if (len > 0) {
		do {
			c1 = *s1;
			c2 = *s2;
			s1++;
			s2++;
			if (!c1)
				break;
			if (!c2)
				break;
			if (c1 == c2)
				continue;
			c1 = tolower(c1);
			c2 = tolower(c2);
			if (c1 != c2)
				break;
		} while (--len);
	}
	return (int)c1 - (int)c2;
}


static FILE *_ffopen(const char far *filename, const char far *mode) {
	char *l_filename = FarStrToNear(filename);
	char *l_mode = FarStrToNear(mode);
	FILE *ret;
	
	ret = fopen(l_filename, l_mode);
	
	LocalFree((HLOCAL)l_filename);
	LocalFree((HLOCAL)l_mode);
	
	return ret;
}

#else
	
#define _fstricmp _stricmp
#define _fmemmove memmove
#define _fmemset memset
#define _fstrchr strchr
#define _ffopen fopen

#endif

#endif