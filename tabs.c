#include <stddef.h>

static long g_tabcntr = 0;
static long genTabId() {
	return ++g_tabcntr;
}

static long g_currTabId = 0;

#define VAR_TABS	0

Tab far *AllocTab(long id) {
	Tab far *tab = (Tab far *)GlobalAlloc(GMEM_FIXED, sizeof(Tab));
	_fmemset(tab, 0, sizeof(Tab));
	tab->id = id;
	tab->TOP_WINDOW.tab = tab;
	AddCustomTaskListData(g_tabTask, VAR_TABS, (LPARAM)tab);
	SetStatusText(tab, "Ready...");
	SetTabTitle(tab, "New Tab");
	SetTabURL(tab, "");
	return tab;
}

BOOL FreeTab(Tab far *tab) {
	int idx = GetCustomTaskListIdxByData(g_tabTask, VAR_TABS, (LPARAM)tab);
	if (idx >= 0) {
		RemoveCustomTaskListData(g_tabTask, VAR_TABS, idx);
		return TRUE;
	}

	return FALSE;
}

BOOL SetTabTitle(Tab far *tab, LPSTR lpStr) {
	if (tab->szTitle) GlobalFree((HGLOBAL)tab->szTitle);
	tab->szTitle = (LPSTR)GlobalAlloc(GMEM_FIXED, lstrlen(lpStr)+1);
	lstrcpy(tab->szTitle, lpStr);
	return TRUE;
}

BOOL SetTabStatus(Tab far *tab, LPSTR szStatus) {
	if (tab->szStatus) GlobalFree((HGLOBAL)tab->szStatus);
	tab->szStatus = (LPSTR)GlobalAlloc(GMEM_FIXED, lstrlen(szStatus)+1);
	lstrcpy(tab->szStatus, szStatus);
	return TRUE;
}

BOOL SetTabURL(Tab far *tab, LPSTR szUrl) {
	if (tab->szUrl) GlobalFree((HGLOBAL)tab->szUrl);
	tab->szUrl = (LPSTR)GlobalAlloc(GMEM_FIXED, lstrlen(szUrl)+1);
	lstrcpy(tab->szUrl, szUrl);
	return TRUE;
}

int GetNumTabs() {
	return GetNumCustomTaskListData(g_tabTask, VAR_TABS);
}

Tab far *TabFromIdx(int idx) {
	LPARAM ret;
	GetCustomTaskListData(g_tabTask, VAR_TABS, idx, &ret);
	return (Tab far *)ret;
}

Tab far *TabFromId(long id) {
	LPARAM ret;
	GetCustomTaskListDataByPtrField(g_tabTask, VAR_TABS, (char far *)&id, offsetof(Tab, id), sizeof(long), &ret);
	return (Tab far *)ret;
}

void ResetDebugLogAttr(Tab far *tab) {
	DebugLogAttr(tab, FALSE, FALSE, RGB(0,0,0));
}

#ifdef WIN3_1
void DebugLogAttr(Tab far *tab, BOOL bold, BOOL italic, COLORREF color) {

}
#else
void DebugLogAttr(Tab far *tab, BOOL bold, BOOL italic, COLORREF color) {
	CHARFORMAT cf;

	memset(&cf,0,sizeof(cf));
	cf.cbSize = sizeof(cf);

	DWORD dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC;
	DWORD dwEffect = 0;
	if (bold) {
		dwEffect |= CFE_BOLD;
	}
	if (italic) {
		dwEffect |= CFE_ITALIC;
	}
	cf.crTextColor = color;
	cf.dwMask = dwMask;
	cf.dwEffects = dwEffect;

	SendMessage(tab->hSource,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);
}
#endif

void DebugLog(Tab far *tab, LPSTR format, ...) {
	int ndx;

	va_list args;
	int     len;
	char    *buffer;

	// retrieve the variable arguments
	va_start( args, format );

#ifndef WIN3_1
	len = _vscprintf( format, args ) // _vscprintf doesn't count
								+ 1; // terminating '\0'
#else
	len = 1024;
#endif

	buffer = (char*)malloc( len * sizeof(char) );

	wvsprintf( buffer, format, args ); // C4996

	ndx = GetWindowTextLength (tab->hSource);
	SetFocus(tab->hSource);
	#ifdef WIN32
	  SendMessage(tab->hSource, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
	#else
	  SendMessage(tab->hSource, EM_SETSEL, 0, MAKELONG (ndx, ndx));
	#endif
	SendMessage(tab->hSource, EM_REPLACESEL, 0, (LPARAM) ((LPSTR) buffer));

	free(buffer);
	va_end(args);
}

LPSTR lpszStatus =  NULL;
HWND hStatusBar = NULL;

void SetStatusText(Tab far *tab, LPSTR format, ...) {
	int ndx;

	va_list args;
	int     len;
	
	if (lpszStatus) GlobalFree((HGLOBAL)lpszStatus);

	// retrieve the variable arguments
	va_start( args, format );

#ifndef WIN3_1
	len = _vscprintf( format, args ) // _vscprintf doesn't count
								+ 1; // terminating '\0'
#else
	len = 1024;
#endif

	lpszStatus = (LPSTR)GlobalAlloc(GMEM_FIXED, len * sizeof(char) );

	wvsprintf( lpszStatus, format, args ); // C4996

	SetTabStatus(tab, lpszStatus);

	va_end(args);	
	
	if (hStatusBar) InvalidateRect(hStatusBar, NULL, TRUE);
}