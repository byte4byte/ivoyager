
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
	if (tab->szTitle) GlobalFree((void far *)tab->szTitle);
	tab->szTitle = (LPSTR)GlobalAlloc(GMEM_FIXED, lstrlen(lpStr)+1);
	lstrcpy(tab->szTitle, lpStr);
	RefreshShell();
	return TRUE;
}

BOOL SetTabStatus(Tab far *tab, LPSTR szStatus) {
	if (tab->szStatus) GlobalFree((void far *)tab->szStatus);
	tab->szStatus = (LPSTR)GlobalAlloc(GMEM_FIXED, lstrlen(szStatus)+1);
	lstrcpy(tab->szStatus, szStatus);
	return TRUE;
}

BOOL SetTabURL(Tab far *tab, LPSTR szUrl) {
	if (tab->szUrl) GlobalFree((void far *)tab->szUrl);
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
	GetCustomTaskListDataByPtrField(g_tabTask, VAR_TABS, (char far *)&id, 0/*offsetof(Tab, id)*/, sizeof(long), &ret);
	return (Tab far *)ret;
}

void ResetDebugLogAttr(Tab far *tab) {
	DebugLogAttr(tab, FALSE, FALSE, RGB(0,0,0));
}

