#include "ivoyager.h"
#include "dom.h"
#include "utils.c"
#include <ctype.h>

#define PARSE_STATE_TAG_NEW_TAG 				0
#define PARSE_STATE_TAG_START 					1
#define PARSE_STATE_TAG_TAGNAME					2
#define PARSE_STATE_ATTRIB_FIND_NAME			3
#define PARSE_STATE_ATTRIB_NAME					4
#define PARSE_STATE_ATTRIB_EQUALS				5
#define PARSE_STATE_ATTRIB_FIND_VALUE			6
#define PARSE_STATE_ATTRIB_VALUE				7
#define PARSE_STATE_ATTRIB_VALUE_DBLQOUTE 		8
#define PARSE_STATE_ATTRIB_VALUE_SGLQOUTE 		9
#define PARSE_STATE_JS_CODE						10
#define PARSE_STATE_CSS_CODE					11

#define PARSE_DOM_VAR_STATE   					100
#define PARSE_DOM_CURR_TEXT   					101
#define PARSE_DOM_CURR_TAG   					102
#define PARSE_DOM_CURR_ATTRIB  					103
#define PARSE_DOM_CURR_VALUE   					104

#define PARSE_CSS_VAR_STATE   					200

#define COMPILE_JS_VAR_STATE  					300

BOOL CompileJSChunk(ContentWindow far *window, Task far *task, DomNode far *scriptEl, char far **buff, int len, BOOL eof) {
	return TRUE;
}

BOOL ParseCSSChunk(ContentWindow far *window, Task far *task, DomNode far *styleEl, char far **buff, int len, BOOL eof) {
	return TRUE;
}

LPSTR ConcatVar(Task far *task, DWORD id, LPSTR str) {
	LPSTR prev_str;
	LPSTR new_str;
	GetCustomTaskVar(task, id, (LPARAM far *)&prev_str, NULL);
	if (prev_str) {
		int len = lstrlen(prev_str);
		len += lstrlen(str);
		new_str = (LPSTR)GlobalAlloc(GMEM_FIXED, len+1);
		lstrcpy(new_str, prev_str);
		strcat(new_str, str);
		AddCustomTaskVar(task, id, (LPARAM)new_str);
		GlobalFree((HGLOBAL)prev_str);
	}
	else {
		int len = lstrlen(str);
		new_str = (LPSTR)GlobalAlloc(GMEM_FIXED, len+1);
		lstrcpy(new_str, str);
		AddCustomTaskVar(task, id, (LPARAM)new_str);
	}
	return new_str;
}

BOOL TagParsed(ContentWindow far *window, Task far *task, char far *ptr) {
	LPSTR fullptr = ConcatVar(task, PARSE_DOM_CURR_TAG, ptr);
	MessageBox(window->hWnd, fullptr, "tag name", MB_OK);
	GlobalFree((HGLOBAL)fullptr);
	
	AddCustomTaskVar(task, PARSE_DOM_CURR_TAG, (LPARAM)NULL);
	return TRUE;
}

BOOL AttribNameParsed(ContentWindow far *window, Task far *task, char far *ptr) {
	LPSTR fullptr = ConcatVar(task, PARSE_DOM_CURR_ATTRIB, ptr);
	MessageBox(window->hWnd, fullptr, "attrib name", MB_OK);
	GlobalFree((HGLOBAL)fullptr);
	
	AddCustomTaskVar(task, PARSE_DOM_CURR_ATTRIB, (LPARAM)NULL);
	return TRUE;
}

BOOL AttribValueParsed(ContentWindow far *window, Task far *task, char far *ptr) {
	LPSTR fullptr = ConcatVar(task, PARSE_DOM_CURR_VALUE, ptr);
	MessageBox(window->hWnd, fullptr, "attrib value", MB_OK);
	GlobalFree((HGLOBAL)fullptr);
	
	AddCustomTaskVar(task, PARSE_DOM_CURR_VALUE, (LPARAM)NULL);
	return TRUE;
}

BOOL TextParsed(ContentWindow far *window, Task far *task, char far *ptr) {
	MessageBox(window->hWnd, ptr, "text", MB_OK);
	
	return TRUE;
}

BOOL ParseDOMChunk(ContentWindow far *window, Task far *task, char far *buff, int len, BOOL eof) {
	DomNode far *currNode;
	LPARAM state;
	LPSTR currText = NULL;
	LPSTR currTag = NULL;
	LPSTR currAttrib = NULL;
	LPSTR currValue = NULL;
	
	LPSTR lpCurrTagStart = NULL;
	LPSTR lpCurrAttribStart = NULL;
	LPSTR lpCurrValueStart = NULL;
	
	char far *last_node_ptr = buff;
	char far *ptr = buff;
	char far *end = buff + len;
	
	buff[len] = '\0';
	MessageBox(window->hWnd, buff, "chunk", MB_OK);
	
	GetCustomTaskVar(task, PARSE_DOM_VAR_STATE, &state, NULL);
	GetCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM far *)&currText, NULL);
	switch (state) {
		case PARSE_STATE_TAG_TAGNAME:
			lpCurrTagStart = buff;
			break;
		case PARSE_STATE_ATTRIB_NAME:
			lpCurrAttribStart = buff;
			break;
		case PARSE_STATE_ATTRIB_VALUE_SGLQOUTE:
		case PARSE_STATE_ATTRIB_VALUE_DBLQOUTE:
		case PARSE_STATE_ATTRIB_VALUE:
			lpCurrValueStart = buff;
			break;
	}
	
	for (;;) {
		switch (state) {
			case PARSE_STATE_TAG_NEW_TAG:
				currNode = (DomNode far *)GlobalAlloc(GMEM_FIXED, sizeof(DomNode));
				_fmemset(currNode, 0, sizeof(DomNode));
			
				state = PARSE_STATE_TAG_START;
				AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
			
				continue;
			case PARSE_STATE_TAG_START:
				
				while (ptr < end) {
					if (*ptr == '<') {
						break;
					}
					ptr++;
				}
				
				if (currText) {
					if (ptr-last_node_ptr > 0) { // We have some text append to prev text
						int prev_len = lstrlen(currText);
						LPSTR newText = (char far *)GlobalAlloc(GMEM_FIXED, prev_len+ptr-last_node_ptr+1);
						_fmemset(newText, 0, prev_len+ptr-last_node_ptr+1);
						lstrcpy(newText, currText);
						strncpy(&newText[prev_len], last_node_ptr, ptr-last_node_ptr);
						GlobalFree((HGLOBAL)currText);
						currText = newText;
					}
				}
				else if (ptr-last_node_ptr > 0) { // New text
					currText = (char far *)GlobalAlloc(GMEM_FIXED, ptr-last_node_ptr+1);
					_fmemset(currText, 0, ptr-last_node_ptr+1);
					strncpy(currText, last_node_ptr, ptr-last_node_ptr);
				}				
				
				if (*ptr == '<') {
					if (currText) {
						//MessageBox(window->hWnd, currText, "text", MB_OK);
						TextParsed(window, task, currText);
						GlobalFree((HGLOBAL)currText);
						currText = NULL;
						AddCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM)currText);
					}					
					state = PARSE_STATE_TAG_TAGNAME;
					AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
					ptr++;
					lpCurrTagStart = ptr;
					break;
				}
				else if (currText) {
					AddCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM)currText);
				}

				break;
			case PARSE_STATE_TAG_TAGNAME:
				while (ptr < end) {
					if (*ptr == '>') {
						state = PARSE_STATE_TAG_START;
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
						TagParsed(window, task, lpCurrTagStart);
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						last_node_ptr = ptr+1;
						break;
					}
					else if (isspace(*ptr)) {
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
						TagParsed(window, task, lpCurrTagStart);
						state = PARSE_STATE_ATTRIB_FIND_NAME;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						break;
					}
					ptr++;
				}
				break;
			case PARSE_STATE_ATTRIB_FIND_NAME:
				while (ptr < end) {
					if (*ptr == '>') {
						state = PARSE_STATE_TAG_START;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						last_node_ptr = ptr+1;
						lpCurrTagStart = lpCurrValueStart = lpCurrAttribStart = NULL;
						break;
					}
					else if (! isspace(*ptr)) {
						state = PARSE_STATE_ATTRIB_NAME;
						lpCurrAttribStart = ptr;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						break;
					}
					ptr++;
				}
				break;
			case PARSE_STATE_ATTRIB_NAME:
				while (ptr < end) {
					if (*ptr == '>') {
						state = PARSE_STATE_TAG_START;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						last_node_ptr = ptr+1;
						lpCurrTagStart = lpCurrValueStart = lpCurrAttribStart = NULL;
						break;
					}
					else if (*ptr == '=') {
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrAttribStart, "attrib name", MB_OK);
						AttribNameParsed(window, task, lpCurrAttribStart);
						state = PARSE_STATE_ATTRIB_FIND_VALUE;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						break;
					}
					else if (isspace(*ptr)) {
						state = PARSE_STATE_ATTRIB_EQUALS;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						break;
					}
					ptr++;
				}
				break;
			case PARSE_STATE_ATTRIB_EQUALS:
				while (ptr < end) {
					if (*ptr == '>') {
						state = PARSE_STATE_TAG_START;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						last_node_ptr = ptr+1;
						lpCurrTagStart = lpCurrValueStart = lpCurrAttribStart = NULL;
						break;
					}
					else if (*ptr == '=') {
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrAttribStart, "attrib name", MB_OK);
						AttribNameParsed(window, task, lpCurrAttribStart);
						state = PARSE_STATE_ATTRIB_FIND_VALUE;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						break;
					}
					ptr++;
				}
				break;
			case PARSE_STATE_ATTRIB_FIND_VALUE:
				while (ptr < end) {
					if (*ptr == '>') {
						state = PARSE_STATE_TAG_START;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						last_node_ptr = ptr+1;
						lpCurrTagStart = lpCurrValueStart = lpCurrAttribStart = NULL;
						break;
					}
					else if (! isspace(*ptr)) {
						if (*ptr == '"') {
							state = PARSE_STATE_ATTRIB_VALUE_DBLQOUTE;
							lpCurrValueStart = ptr+1;
						}
						else if (*ptr == '\'') {
							state = PARSE_STATE_ATTRIB_VALUE_SGLQOUTE;
							lpCurrValueStart = ptr+1;
						}
						else {
							state = PARSE_STATE_ATTRIB_VALUE;
							lpCurrValueStart = ptr;
						}

						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						break;
					}
					ptr++;
				}
				if (state == PARSE_STATE_ATTRIB_VALUE) continue;
				break;
			case PARSE_STATE_ATTRIB_VALUE:
				while (ptr < end) {
					if (*ptr == '>') {
						state = PARSE_STATE_TAG_START;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						last_node_ptr = ptr+1;
						lpCurrTagStart = lpCurrValueStart = lpCurrAttribStart = NULL;
						break;
					}
					else if (isspace(*ptr)) {
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrValueStart, "attrib value", MB_OK);
						AttribValueParsed(window, task, lpCurrValueStart);
						state = PARSE_STATE_ATTRIB_FIND_NAME;
						lpCurrValueStart = lpCurrAttribStart = NULL;
						break;
					}
					ptr++;
				}
				if (state == PARSE_STATE_ATTRIB_FIND_NAME) continue;
				break;
			case PARSE_STATE_ATTRIB_VALUE_SGLQOUTE:
				while (ptr < end) {
					if (*ptr == '\'') {
						state = PARSE_STATE_ATTRIB_FIND_NAME;
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrValueStart, "attrib value", MB_OK);
						AttribValueParsed(window, task, lpCurrValueStart);
						lpCurrValueStart = lpCurrAttribStart = NULL;
						break;
					}
					ptr++;
				}
				break;
			case PARSE_STATE_ATTRIB_VALUE_DBLQOUTE:
				while (ptr < end) {
					if (*ptr == '"') {
						state = PARSE_STATE_ATTRIB_FIND_NAME;
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrValueStart, "attrib value", MB_OK);
						AttribValueParsed(window, task, lpCurrValueStart);
						lpCurrValueStart = lpCurrAttribStart = NULL;
						break;
					}
					ptr++;
				}
				break;
		}

		ptr++;
		if (ptr >= end) break;
	}
	
	if (! eof) {
		switch (state) {
			case PARSE_STATE_TAG_TAGNAME:
				if (lpCurrTagStart) {
					ConcatVar(task, PARSE_DOM_CURR_TAG, lpCurrTagStart);
				}
				break;
			case PARSE_STATE_ATTRIB_NAME:
				if (lpCurrAttribStart) {
					ConcatVar(task, PARSE_DOM_CURR_ATTRIB, lpCurrAttribStart);
				}
				break;
			case PARSE_STATE_ATTRIB_VALUE_SGLQOUTE:
			case PARSE_STATE_ATTRIB_VALUE_DBLQOUTE:
			case PARSE_STATE_ATTRIB_VALUE:
				if (lpCurrValueStart) {
					ConcatVar(task, PARSE_DOM_CURR_VALUE, lpCurrValueStart);
				}
				break;
		}
	}
	
	if (eof) {
		if (currText) {
			TextParsed(window, task, currText);
			GlobalFree((HGLOBAL)currText);
		}
		GetCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM far *)&lpCurrTagStart, NULL);
		GetCustomTaskVar(task, PARSE_DOM_CURR_ATTRIB, (LPARAM far *)&lpCurrAttribStart, NULL);
		GetCustomTaskVar(task, PARSE_DOM_CURR_VALUE, (LPARAM far *)&lpCurrValueStart, NULL);
		if (lpCurrAttribStart) GlobalFree((HGLOBAL)lpCurrTagStart);
		if (lpCurrAttribStart) GlobalFree((HGLOBAL)lpCurrAttribStart);
		if (lpCurrValueStart) GlobalFree((HGLOBAL)lpCurrValueStart);
	}
	
	return TRUE;
}