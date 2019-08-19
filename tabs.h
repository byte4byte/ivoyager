#ifndef _TABS_H
#define _TABS_H

#define VIEW_PAGE 		0
#define VIEW_SOURCE		1
#define VIEW_CONSOLE	2

typedef struct Tab {
	long id;
	ContentWindow TOP_WINDOW;
	LPSTR szTitle;
	HWND hPage;
	HWND hSource;
	HWND hConsole;
	int view;
	LPSTR szStatus;
} Tab;

Tab far *AllocTab(long id);
BOOL FreeTab(Tab far *tab);
BOOL SetActiveTab(Tab far *tab);
int GetNumTabs();
BOOL SetTabTitle(Tab far *tab, LPSTR lpStr);
BOOL SetTabStatus(Tab far *tab, LPSTR szStatus);
BOOL SetTabURL(Tab far *tab, LPSTR szUrl);

void DebugLog(Tab far *tab, LPSTR format, ...);
void DebugLogAttr(Tab far *tab, BOOL bold, BOOL italic, COLORREF color);
void ResetDebugLogAttr(Tab far *tab);
void SetStatusText(Tab far *tab, LPSTR format, ...);

Tab far *TabFromIdx(int idx);
Tab far *TabFromId(long id);


#endif