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

BOOL ParseDOMChunk(ContentWindow far *window, Task far *task, char far *buff, int len, BOOL eof) {
	DomNode far *currNode;
	LPARAM state;
	LPSTR currText = NULL;
	char far *last_node_ptr = buff;
	char far *ptr = buff;
	char far *end = buff + len;

	#define PARSE_DOM_VAR_STATE   100
	#define PARSE_DOM_CURR_TEXT   101
	
	GetCustomTaskVar(task, PARSE_DOM_VAR_STATE, &state, NULL);
	GetCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM far *)&currText, NULL);
	
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
						MessageBox(window->hWnd, currText, "text", MB_OK);
						GlobalFree((HGLOBAL)currText);
						currText = NULL;
						AddCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM)currText);
					}
					MessageBox(window->hWnd, ptr, "found tag", MB_OK);					
					state = PARSE_STATE_TAG_TAGNAME;
					AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
					ptr++;
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
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						last_node_ptr = ptr+1;
						break;
					}
					else if (isspace(*ptr)) {
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
						break;
					}
					else if (! isspace(*ptr)) {
						state = PARSE_STATE_ATTRIB_NAME;
						MessageBox(window->hWnd, ptr, "attrib name", MB_OK);
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
						break;
					}
					else if (*ptr == '=') {
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
						break;
					}
					else if (*ptr == '=') {
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
						break;
					}
					else if (! isspace(*ptr)) {
						if (*ptr == '"') state = PARSE_STATE_ATTRIB_VALUE_DBLQOUTE;
						else if (*ptr == '\'') state = PARSE_STATE_ATTRIB_VALUE_SGLQOUTE;
						else state = PARSE_STATE_ATTRIB_VALUE;
						
						MessageBox(window->hWnd, ptr, "attrib value", MB_OK);

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
						break;
					}
					else if (isspace(*ptr)) {
						state = PARSE_STATE_ATTRIB_FIND_NAME;
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
						break;
					}
					ptr++;
				}
				break;
			case PARSE_STATE_ATTRIB_VALUE_DBLQOUTE:
				while (ptr < end) {
					if (*ptr == '"') {
						state = PARSE_STATE_ATTRIB_FIND_NAME;
						break;
					}
					ptr++;
				}
				break;
		}

		ptr++;
		if (ptr >= end) break;
	}
	
	if (eof) {
		if (currText) {
			MessageBox(window->hWnd, currText, "ending text", MB_OK);
			GlobalFree((HGLOBAL)currText);
		}
	}
	
	buff[len] = '\0';
	MessageBox(window->hWnd, buff, "chunk", MB_OK);
	return TRUE;
}