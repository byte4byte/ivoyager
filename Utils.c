#ifndef _UTILS_C
#define _UTILS_C

#include <ctype.h>
#include <stdarg.h>

#ifdef WIN3_1

typedef unsigned long far * ULONG_PTR;

#define GlobalPtrHandle( x )        ((HGLOBAL)GlobalHandle( x ))
#define GlobalLockPtr( x )          ((BOOL)GlobalLock( GlobalPtrHandle( x ) ))
#define GlobalUnlockPtr( x )        GlobalUnlock( GlobalPtrHandle( x ) )
#define GlobalAllocPtr( p1, p2 )    GlobalLock( GlobalAlloc( p1, p2 ) )
#define GlobalReAllocPtr( x, p1, p2 ) \
    (GlobalUnlockPtr( x ), GlobalLock( GlobalReAlloc( GlobalPtrHandle( x ), p1, p2 ) ))
#define GlobalFreePtr( x ) \
    (GlobalUnlockPtr( x ), (BOOL)(ULONG_PTR)GlobalFree( GlobalPtrHandle( x ) ))

extern int g_cnt, lcntr;

static LPSTR GAlloc(DWORD len) {
        void far * ret = (void far *)GlobalAllocPtr(GMEM_FIXED, len);
		//void *ret = (void *)malloc(len);
        g_cnt++;
        if (! ret) {
            char buff[20];
            wsprintf(buff, "%d", g_cnt);
            OutputDebugString("GALLOC FAILED");
			StackTrace();
        }
        return ret;
/*        HANDLE hMem;
        void far *ret;
        hMem = GlobalAlloc(GMEM_FIXED, len);
        if (! hMem) MessageBox(NULL, "GlobalAlloc FAILED", len <= 0 ? "ZERO" : "nbOT", MB_OK);
        ret = (void far *)GlobalLock(hMem);
        if (! ret) MessageBox(NULL, "GlobalAlloc FAILED", len <= 0 ? "ZERO" : "nbOT", MB_OK);
        return ret;*/
}



static void far *LAlloc(int len) {
        void *ret = (void *)malloc(len);
           
        //void *ret = (void *)LocalAlloc(LMEM_FIXED, len);
        lcntr++;
        if (! ret) {
            char buff[20];
            wsprintf(buff, "%d", lcntr);
            OutputDebugString("localalloc failed");
            StackTrace();
        }
        return ret;
}

static void GFree(void far *ptr) {
        g_cnt--;
		//free(ptr);
        GlobalFreePtr((UINT)ptr);
        //) MessageBox(NULL, "global free failed", "", MB_OK);
//        if (GlobalFree(GlobalHandle((UINT)ptr)) != NULL) MessageBox(NULL, "global free failed", "", MB_OK);
}

static void LFree(void far * ptr) {
        lcntr--;
        free(ptr);
        //LocalFree(ptr);
}

#define GlobalAlloc(x, y) GAlloc(y)
#define LocalAlloc(x, y) LAlloc(y)
#define LocalFree(x) LFree(x)
#define GlobalFree(x) GFree(x)

#else
	
extern int g_cnt, lcntr;

static LPSTR GAlloc(DWORD len) {
        void far * ret = (void far *)GlobalAlloc(GMEM_FIXED, len);
		//void *ret = (void *)malloc(len);
        g_cnt++;
        if (! ret) {
            char buff[20];
            wsprintf(buff, "%d", g_cnt);
            OutputDebugString("GALLOC FAILED");
			StackTrace();
        }
        return ret;
/*        HANDLE hMem;
        void far *ret;
        hMem = GlobalAlloc(GMEM_FIXED, len);
        if (! hMem) MessageBox(NULL, "GlobalAlloc FAILED", len <= 0 ? "ZERO" : "nbOT", MB_OK);
        ret = (void far *)GlobalLock(hMem);
        if (! ret) MessageBox(NULL, "GlobalAlloc FAILED", len <= 0 ? "ZERO" : "nbOT", MB_OK);
        return ret;*/
}



static void far *LAlloc(int len) {
        void *ret = (void *)malloc(len);
           
        //void *ret = (void *)LocalAlloc(LMEM_FIXED, len);
        lcntr++;
        if (! ret) {
            char buff[20];
            wsprintf(buff, "%d", lcntr);
            OutputDebugString("localalloc failed");
            StackTrace();
        }
        return ret;
}

static void GFree(void far *ptr) {
        g_cnt--;
		//free(ptr);
        GlobalFree(ptr);
        //) MessageBox(NULL, "global free failed", "", MB_OK);
//        if (GlobalFree(GlobalHandle((UINT)ptr)) != NULL) MessageBox(NULL, "global free failed", "", MB_OK);
}

static void LFree(void far * ptr) {
        lcntr--;
        free(ptr);
        //LocalFree(ptr);
}

#define GlobalAlloc(x, y) GAlloc(y)
#define LocalAlloc(x, y) LAlloc(y)
#define LocalFree(x) LFree(x)
#define GlobalFree(x) GFree(x)
        
#define _fstricmp _stricmp
#define _fmemmove memmove
#define _fmemset memset
#define _fstrchr strchr
#define _ffopen fopen
#define _fstrncpy strncpy
#define _fstrcat strcat

#endif

static LPSTR ConcatVar(Task far *task, DWORD id, LPSTR str) {
        LPSTR prev_str = NULL;
        LPSTR new_str;
		//OutputDebugString("concat var");
		//OutputDebugString(str);
        if (! str) {
            StackTrace();
            OutputDebugString("NULL STRING");
        }
        GetCustomTaskVar(task, id, (LPARAM far *)&prev_str, NULL);
        if (prev_str) {
                LPSTR p, p2;
				
                int len = lstrlen(prev_str);
                len += lstrlen(str);
				
				//OutputDebugString("prev var");
				//OutputDebugString(prev_str);
				
                new_str = (LPSTR)LocalAlloc(GMEM_FIXED, len+1);
                if (! new_str) OutputDebugString("alloc fail 1");
                lstrcpy(new_str, prev_str);
                lstrcat(new_str, str);
                /*p = new_str;
                while (*p) { p++; }
                p2 = str;
                while (*p2) {
                        *p = *p2;
                        p2++;
                }
                *p = '\0';*/
                //lstrcat(new_str, str);
                AddCustomTaskVar(task, id, (LPARAM)new_str);
                LocalFree((void far *)prev_str);
        }
        else {
                int len = lstrlen(str);
                new_str = (LPSTR)LocalAlloc(GMEM_FIXED, len+1);
                if (! new_str) OutputDebugString("alloc fail 2");
                lstrcpy(new_str, str);
                AddCustomTaskVar(task, id, (LPARAM)new_str);
        }
        return new_str;
}

static BOOL IsWhitespace(LPSTR str) {
        //if (! str) return TRUE;
        while (*str) {
                if (! isspace(*str)) return FALSE;
                str++;
        }
        
        return TRUE;
}

static LPSTR Trim(LPSTR str, BOOL left, BOOL right) {
        LPSTR ptr = str;
        if (left) {
                while (*ptr) {
                        if (! isspace(*ptr)) break;
                        ptr++;
                }
                if (ptr != str) lstrcpy(str, ptr);
                ptr = str;
        }
        if (right) {
                LPSTR lastChar = NULL;
                while (*ptr) {
                        if (! isspace(*ptr)) lastChar = ptr+1;
                        ptr++;
                }
                if (lastChar) lstrcpy(lastChar, "");
        }
        
        return str;
}

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

#define _stricmp() ERR()
#define _strnicmp() ERR()

#if 0
static int
 _fstricmp(const char  far *s1, const char  far *s2)
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
_fstrnicmp(char const far *s1, char const far *s2, size_t len)
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

#endif
static FILE *_ffopen(const char far *filename, const char far *mode) {
        char *l_filename = FarStrToNear(filename);
        char *l_mode = FarStrToNear(mode);
        FILE *ret;
        
        ret = fopen(l_filename, l_mode);
        
        LocalFree((HLOCAL)l_filename);
        LocalFree((HLOCAL)l_mode);
        
        return ret;
}


#endif

#ifndef ULONG_MAX
#define ULONG_MAX       ((unsigned long)(~0L))          /* 0xFFFFFFFF */
#endif

/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
static unsigned long
_fstrtoul(const char far *nptr, char far * far *endptr, register int base)
{
        register const char far *s = nptr;
        register unsigned long acc;
        register int c;
        register unsigned long cutoff;
        register int neg = 0, any, cutlim;

        /*
         * See strtol for comments as to the logic used.
         */
        do {
                c = *s++;
        } while (isspace(c));
        if (c == '-') {
                neg = 1;
                c = *s++;
        } else if (c == '+')
                c = *s++;
        if ((base == 0 || base == 16) &&
            c == '0' && (*s == 'x' || *s == 'X')) {
                c = s[1];
                s += 2;
                base = 16;
        }
        if (base == 0)
                base = c == '0' ? 8 : 10;
        cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
        cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;
        for (acc = 0, any = 0;; c = *s++) {
                if (isdigit(c))
                        c -= '0';
                else if (isalpha(c))
                        c -= isupper(c) ? 'A' - 10 : 'a' - 10;
                else
                        break;
                if (c >= base)
                        break;
                if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
                        any = -1;
                else {
                        any = 1;
                        acc *= base;
                        acc += c;
                }
        }
        if (any < 0) {
                acc = ULONG_MAX;
            //  errno = ERANGE;
        } else if (neg)
                acc = -acc;
        if (endptr != 0)
                *endptr = (char far *) (any ? s - 1 : nptr);
        return (acc);
}

#define PTR void far *

static int
vprintf_buffer_size (const char far *format, va_list args)
{
  const char far *p = format;
  /* Add one to make sure that it is never zero, which might cause malloc
     to return NULL.  */
  int total_width = lstrlen (format) + 1;
  va_list ap;

#ifdef va_copy
  va_copy (ap, args);
#else
  _fmemcpy ((PTR) &ap, (PTR) &args, sizeof (va_list));
#endif

  while (*p != '\0')
    {
      if (*p++ == '%')
        {
          while (_fstrchr ("-+ #0", *p))
            ++p;
          if (*p == '*')
            {
              ++p;
              total_width += abs (va_arg (ap, int));
            }
          else
            total_width += _fstrtoul (p, (char far * far *) &p, 10);
          if (*p == '.')
            {
              ++p;
              if (*p == '*')
                {
                  ++p;
                  total_width += abs (va_arg (ap, int));
                }
              else
              total_width += _fstrtoul (p, (char far * far *) &p, 10);
            }
          while (_fstrchr ("hlL", *p))
            ++p;
          /* Should be big enough for any format specifier except %s and floats.  */
          total_width += 30;
          switch (*p)
            {
            case 'd':
            case 'i':
            case 'o':
            case 'u':
            case 'x':
            case 'X':
            case 'c':
              (void) va_arg (ap, int);
              break;
            case 'f':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
              (void) va_arg (ap, double);
              /* Since an ieee double can have an exponent of 307, we'll
                 make the buffer wide enough to cover the gross case. */
              total_width += 307;
              break;
            case 's':
              total_width += lstrlen (va_arg (ap, char far *));
              break;
            case 'p':
            case 'n':
              (void) va_arg (ap, char far *);
              break;
            }
          p++;
        }
    }
#ifdef va_copy
  va_end (ap);
#endif
  return total_width;
}


#endif
