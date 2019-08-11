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
#define PARSE_DOM_CURR_PARSED_TAG				105
#define PARSE_DOM_PROBE_END						106
#define PARSE_DOM_IN_COMMENT					107
#define PARSE_DOM_IN_COMMENT_CNT				108

#define PARSE_CSS_VAR_STATE   					200
#define PARSE_CSS_IN_SQUOTE						201
#define PARSE_CSS_IN_DQUOTE						202
#define PARSE_CSS_CURR_SELECTOR					203
#define PARSE_CSS_CURR_PROP						204
#define PARSE_CSS_CURR_VALUE					205
#define PARSE_CSS_FIND_END_TASK					206
#define PARSE_CSS_IN_FIND_END_TASK				207
#define PARSE_CSS_FOUND_END_TASK				208
#define PARSE_CSS_IN_COMMENT					210
#define PARSE_CSS_LAST_STAR						211
#define PARSE_CSS_RESTORE_STATE					212
#define PARSE_CSS_STATE_FIND_SELECTOR			0 // parse until !isspace
#define PARSE_CSS_STATE_FIND_NEXT_SELECTOR		1
#define PARSE_CSS_STATE_IN_SELECTOR				2 // parse until , {
#define PARSE_CSS_STATE_FIND_BLOCK				3 // parse until {
#define PARSE_CSS_STATE_FIND_PROP				4// parse until !isspace
#define PARSE_CSS_STATE_IN_PROP					5 // parse until isspace }
#define PARSE_CSS_STATE_FIND_COLON				6 // parse until : }
#define PARSE_CSS_STATE_FIND_VALUE				7 // parse until !isspace }
#define PARSE_CSS_STATE_IN_VALUE				8 // (if inquote parse until '") ; }
#define PARSE_CSS_STATE_IN_MAYBEDONE			9

#define COMPILE_JS_VAR_STATE  					300

BOOL CompileJSChunk(ContentWindow far *window, Task far *task, LPARAM *state, DomNode far *scriptEl, char far **buff, int len, BOOL eof) {
	*state = PARSE_STATE_TAG_START;
	AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, *state);
	return TRUE;
}

BOOL CSSEnd(ContentWindow far *window, Task far *task, LPARAM far *state) {
	AddCustomTaskVar(task, PARSE_CSS_RESTORE_STATE, *state);
	Task far *findEndTask = AllocTempTask();
	*state = PARSE_CSS_STATE_IN_MAYBEDONE;
	AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, *state);
	AddCustomTaskVar(task, PARSE_CSS_FIND_END_TASK, (LPARAM)findEndTask);
	AddCustomTaskVar(findEndTask, PARSE_CSS_IN_FIND_END_TASK, (LPARAM)TRUE);
	return TRUE;
}

BOOL StripCSSComment(char far *ptr, LPARAM *bInComment) {
	char far *comment_start = NULL;
	BOOL inSlash = FALSE;
	BOOL inDblQuote = FALSE;
	BOOL inSgnlQuote = FALSE;
	BOOL inStar = FALSE;
	*bInComment = FALSE;
	
	while (*ptr) {
		if (*bInComment) {
			if (*ptr == '*') {
				inStar = TRUE;
			}
			else if (inStar && *ptr == '/') {
				inStar = FALSE;
				*bInComment = FALSE;
				lstrcpy(comment_start, ptr+1);
				comment_start = NULL;
			}
			else {
				inStar = FALSE;
			}
		}
		else {
			if (*ptr == '"') {
				inDblQuote = !inDblQuote;
			}
			else if (*ptr == '\'') {
				inSgnlQuote = !inSgnlQuote;
			}
			else if (! inDblQuote && ! inSgnlQuote) {
				if (*ptr == '/') {
					inSlash = TRUE;
				}
				else if (inSlash && *ptr == '*') {
					*bInComment = TRUE;
					inSlash = FALSE;
					comment_start = ptr-1;
				}
				else {
					inSlash = FALSE;
				}
			}
		}
		ptr++;
	}
	
	if (comment_start) lstrcpy(comment_start, "");
	
	return TRUE;
}

BOOL SelectorParsed(ContentWindow far *window, Task far *task, char far *ptr, LPARAM *bInComment, LPARAM *state) {
	//LPSTR prevtag;
	LPSTR fullptr = ConcatVar(task, PARSE_CSS_CURR_SELECTOR, ptr);
	
	//GetCustomTaskVar(task, PARSE_DOM_CURR_PARSED_TAG, (LPARAM far *)&prevtag, NULL);
	//if (prevtag) GlobalFree((HGLOBAL)prevtag);
	
	StripCSSComment(fullptr, bInComment);
	if (! IsWhitespace(fullptr)) {
		MessageBox(window->hWnd, fullptr, "CSS Selector", MB_OK);
	}
	else {
		*state = PARSE_CSS_STATE_FIND_SELECTOR;
		AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, *state);
	}
	
	GlobalFree((HGLOBAL)fullptr);
	
	AddCustomTaskVar(task, PARSE_CSS_CURR_SELECTOR, (LPARAM)NULL);
	//AddCustomTaskVar(task, PARSE_DOM_CURR_PARSED_TAG, (LPARAM)fullptr);
	
	return TRUE;
}

BOOL CSSValueParsed(ContentWindow far *window, Task far *task, char far *ptr, LPARAM *bInComment) {
	//LPSTR prevtag;
	LPSTR fullptr = ConcatVar(task, PARSE_CSS_CURR_VALUE, ptr);
	
	//GetCustomTaskVar(task, PARSE_DOM_CURR_PARSED_TAG, (LPARAM far *)&prevtag, NULL);
	//if (prevtag) GlobalFree((HGLOBAL)prevtag);
	
	StripCSSComment(fullptr, bInComment);
	if (! IsWhitespace(fullptr)) {
		MessageBox(window->hWnd, fullptr, "CSSValue", MB_OK);
	}
	
	GlobalFree((HGLOBAL)fullptr);
	
	AddCustomTaskVar(task, PARSE_CSS_CURR_VALUE, (LPARAM)NULL);
	//AddCustomTaskVar(task, PARSE_DOM_CURR_PARSED_TAG, (LPARAM)fullptr);
	
	return TRUE;
}

BOOL CSSPropertyParsed(ContentWindow far *window, Task far *task, char far *ptr, LPARAM *bInComment) {
		//LPSTR prevtag;
	LPSTR fullptr = ConcatVar(task, PARSE_CSS_CURR_PROP, ptr);
	
	//GetCustomTaskVar(task, PARSE_DOM_CURR_PARSED_TAG, (LPARAM far *)&prevtag, NULL);
	//if (prevtag) GlobalFree((HGLOBAL)prevtag);
	
	StripCSSComment(fullptr, bInComment);
	if (! IsWhitespace(fullptr)) {	
		MessageBox(window->hWnd, fullptr, "CSS Property", MB_OK);
	}
	
	GlobalFree((HGLOBAL)fullptr);
	
	AddCustomTaskVar(task, PARSE_CSS_CURR_PROP, (LPARAM)NULL);
	//AddCustomTaskVar(task, PARSE_DOM_CURR_PARSED_TAG, (LPARAM)fullptr);
	
	return TRUE;
}

BOOL ParseCSSChunk(ContentWindow far *window, Task far *task, LPARAM *dom_state, DomNode far *styleEl, char far **buff, int len, BOOL eof) {
	LPARAM state;
	
	LPSTR lpCurrSelectorStart = NULL;
	LPSTR lpCurrPropStart = NULL;
	LPSTR lpCurrValueStart = NULL;
	
	BOOL bDone = FALSE;
	
	LPARAM bInComment = FALSE;
	LPARAM bLastStar = FALSE;
	
	char far *ptr = *buff;
	char far *end = *buff + len;
	
	LPARAM bInSQuote = FALSE;
	LPARAM bInDQuote = FALSE;
		
	GetCustomTaskVar(task, PARSE_CSS_VAR_STATE, &state, NULL);
	GetCustomTaskVar(task, PARSE_CSS_IN_COMMENT, &bInComment, NULL);
	GetCustomTaskVar(task, PARSE_CSS_LAST_STAR, &bLastStar, NULL);

	switch (state) {
		case PARSE_CSS_STATE_IN_SELECTOR:
			GetCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, &bInSQuote, NULL);
			GetCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, &bInDQuote, NULL);
			lpCurrSelectorStart = *buff;
			break;
		case PARSE_CSS_STATE_IN_PROP:
			lpCurrPropStart = *buff;
			break;
		case PARSE_CSS_STATE_IN_VALUE:
			GetCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, &bInSQuote, NULL);
			GetCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, &bInDQuote, NULL);
			lpCurrValueStart = *buff;
			break;
	}
	
	for (;;) {
		BOOL bNoInc = FALSE;
		
		if (bInComment) {
			while (ptr < end) {
				
				if (*ptr == '<') {
					CSSEnd(window, task, &state);
					bNoInc = TRUE;
					break;
				}
				
				if (*ptr == '*') {
					bLastStar = TRUE;
				}
				else if (bLastStar && *ptr == '/') {
					bLastStar = FALSE;
					bInComment = FALSE;
					ptr++;
					break;
				}
				else {
					bLastStar = FALSE;
				}
				ptr++;
			}
		}
		
		if (bInComment && ! state != PARSE_CSS_STATE_IN_MAYBEDONE) break;
		
		switch (state) {
			case PARSE_CSS_STATE_FIND_SELECTOR:
				while (ptr < end) {
					if (*ptr == '<') {
						CSSEnd(window, task, &state);
						bNoInc = TRUE;
						break;
					}
					else if (! isspace(*ptr)) {
						lpCurrSelectorStart = ptr;
						
						state = PARSE_CSS_STATE_IN_SELECTOR;
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						bNoInc = TRUE;
						break;
					}
					ptr++;
				}
				break;
			case PARSE_CSS_STATE_FIND_NEXT_SELECTOR:
				while (ptr < end) {
					if (*ptr == '<') {
						CSSEnd(window, task, &state);
						bNoInc = TRUE;
						break;
					}
					else if (*ptr == '{') {
						state = PARSE_CSS_STATE_FIND_PROP;
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
						SelectorParsed(window, task, lpCurrSelectorStart, &bInComment, &state);
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						break;
					}
					else if (! isspace(*ptr)) {
						lpCurrSelectorStart = ptr;
						state = PARSE_CSS_STATE_IN_SELECTOR;
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						bNoInc = TRUE;
						break;
					}
					ptr++;
				}
				break;
			case PARSE_CSS_STATE_IN_SELECTOR:
				while (ptr < end) {
					if (*ptr == '<') {
						CSSEnd(window, task, &state);
						bNoInc = TRUE;
						break;
					}
					if (bInSQuote) {
						if (*ptr == '\'' || *ptr == '\n') {
							bInSQuote = FALSE;
						}
					}
					else if (bInDQuote) {
						if (*ptr == '\"' || *ptr == '\n') {
							bInDQuote = FALSE;
						}
					}
					else if (*ptr == '<') {
						CSSEnd(window, task, &state);
						bNoInc = TRUE;
						break;
					}
					else {
						if (*ptr == '\'') {
							bInSQuote = TRUE;
						}
						else if (*ptr == '\"') {
							bInDQuote = TRUE;
						}
						else if (*ptr == ',') {
							state = PARSE_CSS_STATE_FIND_NEXT_SELECTOR;
							*ptr = '\0';
							//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
							SelectorParsed(window, task, lpCurrSelectorStart, &bInComment, &state);
							AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);

							break;
						}
						else if (*ptr == '{') {
							state = PARSE_CSS_STATE_FIND_PROP;
							*ptr = '\0';
							//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
							SelectorParsed(window, task, lpCurrSelectorStart, &bInComment, &state);
							AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
							break;
						}
					}
					ptr++;
				}
				break;
			case PARSE_CSS_STATE_FIND_PROP:
				while (ptr < end) {
					if (*ptr == '<') {
						CSSEnd(window, task, &state);
						bNoInc = TRUE;
						break;
					}
					else if (*ptr == '}') {
						state = PARSE_CSS_STATE_FIND_SELECTOR;
						//*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
						//SelectorParsed(window, task, lpCurrSelectorStart);
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						break;
					}
					else if (! isspace(*ptr)) {
						lpCurrPropStart = ptr;
						state = PARSE_CSS_STATE_IN_PROP;
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						bNoInc = TRUE;
						bInSQuote = bInDQuote = FALSE;
						break;
					}
					ptr++;
				}
				break;
			case PARSE_CSS_STATE_IN_PROP:
				while (ptr < end) {
					if (*ptr == '<') {
						CSSEnd(window, task, &state);
						bNoInc = TRUE;
						break;
					}
					else if (*ptr == '}') {
						state = PARSE_CSS_STATE_FIND_SELECTOR;
						//*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
						//SelectorParsed(window, task, lpCurrSelectorStart);
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						break;
					}
					else {
						if (*ptr == ':') {
							state = PARSE_CSS_STATE_FIND_VALUE;
							*ptr = '\0';
							//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
							CSSPropertyParsed(window, task, lpCurrPropStart, &bInComment);
							AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);

							break;
						}
						else if (isspace(*ptr)) {
							state = PARSE_CSS_STATE_FIND_COLON;
							*ptr = '\0';
							//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
							CSSPropertyParsed(window, task, lpCurrPropStart, &bInComment);
							AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
							break;
						}
					}
					ptr++;
				}
				break;	
			case PARSE_CSS_STATE_FIND_COLON:
				while (ptr < end) {
					if (*ptr == '<') {
						CSSEnd(window, task, &state);
						bNoInc = TRUE;
						break;
					}
					else if (*ptr == '}') {
						state = PARSE_CSS_STATE_FIND_SELECTOR;
						//*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
						//SelectorParsed(window, task, lpCurrSelectorStart);
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						break;
					}
					else {
						if (*ptr == ':') {
							state = PARSE_CSS_STATE_FIND_VALUE;
							AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);

							break;
						}
						else if (*ptr == '\n') {
							state = PARSE_CSS_STATE_FIND_PROP;
							AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
							break;
						}
					}
					ptr++;
				}
				break;
			case PARSE_CSS_STATE_FIND_VALUE:
				while (ptr < end) {
					if (*ptr == '<') {
						CSSEnd(window, task, &state);
						bNoInc = TRUE;
						break;
					}
					else if (*ptr == '}') {
						state = PARSE_CSS_STATE_FIND_SELECTOR;
						//*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
						//SelectorParsed(window, task, lpCurrSelectorStart);
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						break;
					}
					else {
						if (*ptr == '\n') {
							state = PARSE_CSS_STATE_FIND_PROP;
							AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
							break;
						}
						else if (! isspace(*ptr)) {
							state = PARSE_CSS_STATE_IN_VALUE;
							lpCurrValueStart = ptr;
							AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
							bNoInc = TRUE;
							break;
						}
					}
					ptr++;
				}
				break;
			case PARSE_CSS_STATE_IN_VALUE:
				while (ptr < end) {
					if (*ptr == '<') {
						CSSEnd(window, task, &state);
						bNoInc = TRUE;
						break;
					}
					if (bInSQuote) {
						if (*ptr == '\'' || *ptr == '\n') {
							bInSQuote = FALSE;
						}
					}
					else if (bInDQuote) {
						if (*ptr == '\"' || *ptr == '\n') {
							bInDQuote = FALSE;
						}
					}
					else if (*ptr == '<') {
						CSSEnd(window, task, &state);
						bNoInc = TRUE;
						break;
					}
					else {
						if (*ptr == '\'') {
							bInSQuote = TRUE;
						}
						else if (*ptr == '\"') {
							bInDQuote = TRUE;
						}
						else if (*ptr == ';' || *ptr == '\n') {
							state = PARSE_CSS_STATE_FIND_PROP;
							*ptr = '\0';
							//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
							CSSValueParsed(window, task, lpCurrValueStart, &bInComment);
							AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);

							break;
						}
						else if (*ptr == '}') {
							state = PARSE_CSS_STATE_FIND_SELECTOR;
							*ptr = '\0';
							//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
							CSSValueParsed(window, task, lpCurrValueStart, &bInComment);
							AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
							break;
						}
					}
					ptr++;
				}
				break;
			case PARSE_CSS_STATE_IN_MAYBEDONE: {
				Task far *findEndTask;
				LPARAM bTaskComplete = FALSE;
				LPARAM bFoundEnd = FALSE;
				GetCustomTaskVar(task, PARSE_CSS_FIND_END_TASK, (LPARAM far *)&findEndTask, NULL);\
				//MessageBox(window->hWnd, ptr, "FIND END CSS", MB_OK);
				ParseDOMChunk(window, findEndTask, &ptr, len-(ptr-*buff), eof);
				GetCustomTaskVar(findEndTask, TASK_COMPLETE, (LPARAM far *)&bTaskComplete, NULL);
				GetCustomTaskVar(findEndTask, PARSE_CSS_FOUND_END_TASK, (LPARAM far *)&bFoundEnd, NULL);
				if (eof || bTaskComplete) {
					if (bFoundEnd) {
						bDone = TRUE;
						//MessageBox(window->hWnd, ptr, "found end", MB_OK);
					}
					else {
						//MessageBox(window->hWnd, ptr, "NO found end", MB_OK);
						//state = PARSE_CSS_STATE_FIND_SELECTOR;
						GetCustomTaskVar(task, PARSE_CSS_RESTORE_STATE, &state, NULL);
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						bNoInc = TRUE;
					}
					FreeTempTask(findEndTask);
				}
				else {
					//MessageBox(window->hWnd, ptr, "NOT FOUND", MB_OK);
				}
				break;
			}
		}
		
		if (bDone) break;
		
		if (! bNoInc) ptr++;
		if (ptr >= end) break;
	}
	
	
	if (! eof) {
		switch (state) {
			case PARSE_CSS_STATE_IN_SELECTOR:
				if (lpCurrSelectorStart) {
					AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
					AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
					ConcatVar(task, PARSE_CSS_CURR_SELECTOR, lpCurrSelectorStart);
				}
				break;
			case PARSE_CSS_STATE_IN_PROP:
				if (lpCurrPropStart) {
					ConcatVar(task, PARSE_CSS_CURR_PROP, lpCurrPropStart);
				}
				break;
			case PARSE_CSS_STATE_IN_VALUE:
				if (lpCurrValueStart) {
					AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
					AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
					ConcatVar(task, PARSE_CSS_CURR_VALUE, lpCurrValueStart);
				}
				break;
		}
	}
	
	AddCustomTaskVar(task, PARSE_CSS_IN_COMMENT, (LPARAM)bInComment);
	AddCustomTaskVar(task, PARSE_CSS_LAST_STAR, (LPARAM)bLastStar);
	
	if (eof) {
		GetCustomTaskVar(task, PARSE_CSS_CURR_SELECTOR, (LPARAM far *)&lpCurrSelectorStart, NULL);
		GetCustomTaskVar(task, PARSE_CSS_STATE_IN_PROP, (LPARAM far *)&lpCurrPropStart, NULL);
		GetCustomTaskVar(task, PARSE_CSS_CURR_VALUE, (LPARAM far *)&lpCurrValueStart, NULL);
		if (lpCurrSelectorStart) GlobalFree((HGLOBAL)lpCurrSelectorStart);
		if (lpCurrPropStart) GlobalFree((HGLOBAL)lpCurrPropStart);
		if (lpCurrValueStart) GlobalFree((HGLOBAL)lpCurrValueStart);
		
		AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, PARSE_CSS_STATE_FIND_SELECTOR);
	}
	
	if (bDone) {
		*dom_state = PARSE_STATE_TAG_START;
		AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, *dom_state);
	}
	
	*buff = ptr;
	
	return TRUE;
}

BOOL TagParsed(ContentWindow far *window, Task far *task, char far *ptr) {
	LPSTR prevtag;
	LPSTR fullptr = ConcatVar(task, PARSE_DOM_CURR_TAG, ptr);
	
	GetCustomTaskVar(task, PARSE_DOM_CURR_PARSED_TAG, (LPARAM far *)&prevtag, NULL);
	if (prevtag) GlobalFree((HGLOBAL)prevtag);
	
	MessageBox(window->hWnd, fullptr, "tag name", MB_OK);
	
	AddCustomTaskVar(task, PARSE_DOM_CURR_TAG, (LPARAM)NULL);
	AddCustomTaskVar(task, PARSE_DOM_CURR_PARSED_TAG, (LPARAM)fullptr);
	
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
	if (! IsWhitespace(ptr)) MessageBox(window->hWnd, ptr, "text", MB_OK);
	
	return TRUE;
}

BOOL TagDone(ContentWindow far *window, Task far *task, LPARAM *state) {
	LPSTR tag;
	BOOL ret = TRUE;
	LPARAM bFindCssEnd = FALSE;
	GetCustomTaskVar(task, PARSE_DOM_CURR_PARSED_TAG, (LPARAM far *)&tag, NULL);
	GetCustomTaskVar(task, PARSE_CSS_IN_FIND_END_TASK, (LPARAM far *)&bFindCssEnd, NULL);
	if (bFindCssEnd) {
		if (tag) {
			if (! _fstricmp(tag, "/style")) {
				AddCustomTaskVar(task, PARSE_CSS_FOUND_END_TASK, (LPARAM)TRUE);
			}
			else {
				AddCustomTaskVar(task, PARSE_CSS_FOUND_END_TASK, (LPARAM)FALSE);
			}
		}
		else {
			AddCustomTaskVar(task, PARSE_CSS_FOUND_END_TASK, (LPARAM)FALSE);
		}
		AddCustomTaskVar(task, TASK_COMPLETE, (LPARAM)TRUE);
		return FALSE;
	}
	if (tag) {
		if (! _fstricmp(tag, "style")) {
			*state = PARSE_STATE_CSS_CODE;
			AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, *state);
		}
		GlobalFree((HGLOBAL)tag);
		AddCustomTaskVar(task, PARSE_DOM_CURR_PARSED_TAG, (LPARAM)NULL);
	}
	
	return ret;
}

BOOL ParseDOMChunk(ContentWindow far *window, Task far *task, char far **buff, int len, BOOL eof) {
	DomNode far *currNode;
	LPARAM state;
	LPARAM bInComment = FALSE;
	LPARAM bInCommentCnt = 0; // 3 == in comment
	LPSTR currText = NULL;
	LPSTR currTag = NULL;
	LPSTR currAttrib = NULL;
	LPSTR currValue = NULL;
	BOOL brk = FALSE;
	
	LPSTR lpCurrTagStart = NULL;
	LPSTR lpCurrAttribStart = NULL;
	LPSTR lpCurrValueStart = NULL;
	
	char far *last_node_ptr = *buff;
	char far *ptr = *buff;
	char far *end = *buff + len;
	
	(*buff)[len] = '\0';
	//MessageBox(window->hWnd, *buff, "chunk", MB_OK);
	
	GetCustomTaskVar(task, PARSE_DOM_VAR_STATE, &state, NULL);
	GetCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM far *)&currText, NULL);
	GetCustomTaskVar(task, PARSE_DOM_IN_COMMENT, &bInComment, NULL);
	GetCustomTaskVar(task, PARSE_DOM_IN_COMMENT_CNT, &bInCommentCnt, NULL);
	
	switch (state) {
		case PARSE_STATE_TAG_TAGNAME:
			lpCurrTagStart = *buff;
			break;
		case PARSE_STATE_ATTRIB_NAME:
			lpCurrAttribStart = *buff;
			break;
		case PARSE_STATE_ATTRIB_VALUE_SGLQOUTE:
		case PARSE_STATE_ATTRIB_VALUE_DBLQOUTE:
		case PARSE_STATE_ATTRIB_VALUE:
			lpCurrValueStart = *buff;
			break;
	}
	
	for (;;) {
		
		if (bInComment) {
			while (ptr < end) {
				if (bInCommentCnt == 3 && *ptr == '-') {
					bInCommentCnt--;
				}
				else if (bInCommentCnt == 2 && *ptr == '-') {
					bInCommentCnt--;
				}
				else if (bInCommentCnt == 1 && *ptr == '>') {
					bInComment = FALSE;
					bInCommentCnt = 0;
					ptr++;
					last_node_ptr = ptr;
					break;
				}
				else {
					bInCommentCnt = 3;
				}
				ptr++;
			}
		}
		
		if (bInComment) break;
		
		switch (state) {
			case PARSE_STATE_TAG_NEW_TAG:
				currNode = (DomNode far *)GlobalAlloc(GMEM_FIXED, sizeof(DomNode));
				_fmemset(currNode, 0, sizeof(DomNode));
			
				state = PARSE_STATE_TAG_START;
				AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
			
				continue;
			case PARSE_STATE_CSS_CODE:
				ParseCSSChunk(window, task, &state, NULL, &ptr, len-(ptr-*buff), eof);
				last_node_ptr = ptr;
				break;
			case PARSE_STATE_TAG_START:
			
				bInCommentCnt = 0;
			
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
						_fstrncpy(&newText[prev_len], last_node_ptr, ptr-last_node_ptr);
						GlobalFree((HGLOBAL)currText);
						currText = newText;
					}
				}
				else if (ptr-last_node_ptr > 0) { // New text
					currText = (char far *)GlobalAlloc(GMEM_FIXED, ptr-last_node_ptr+1);
					_fmemset(currText, 0, ptr-last_node_ptr+1);
					_fstrncpy(currText, last_node_ptr, ptr-last_node_ptr);
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
					//ptr++;
					lpCurrTagStart = ptr+1;
					break;
				}
				else if (currText) {
					AddCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM)currText);
				}

				break;
			case PARSE_STATE_TAG_TAGNAME:
				while (ptr < end) {
					if (bInCommentCnt == 0 &&  *ptr == '!') {
						bInCommentCnt=1;
					}
					else if (bInCommentCnt == 1 && *ptr == '-') {
						bInCommentCnt=2;
					}
					else if (bInCommentCnt == 2 && *ptr == '-') { // in a comment
						bInCommentCnt=3;
						bInComment = TRUE;
						state = PARSE_STATE_TAG_START;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						break;
					}
					else if (*ptr == '>') {
						char chRestore = *ptr;
						state = PARSE_STATE_TAG_START;
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
						TagParsed(window, task, lpCurrTagStart);
						*ptr = chRestore;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						last_node_ptr = ptr+1;
						if (! TagDone(window, task, &state)) { ptr++; brk = TRUE; eof = TRUE; }
						break;
					}
					else if (isspace(*ptr)) {
						char chRestore = *ptr;
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
						TagParsed(window, task, lpCurrTagStart);
						*ptr = chRestore;
						state = PARSE_STATE_ATTRIB_FIND_NAME;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						break;
					}
					else {
						bInCommentCnt = -1;
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
						if (! TagDone(window, task, &state)) { ptr++; brk = TRUE; eof = TRUE; }
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
						if (! TagDone(window, task, &state)) { ptr++; brk = TRUE; eof = TRUE; }
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
						if (! TagDone(window, task, &state)) { ptr++; brk = TRUE; eof = TRUE; }
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
						if (! TagDone(window, task, &state)) { ptr++; brk = TRUE; eof = TRUE; }
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
						if (! TagDone(window, task, &state))  { ptr++; brk = TRUE; eof = TRUE; }
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

		if (brk) break;

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
	
	AddCustomTaskVar(task, PARSE_DOM_IN_COMMENT, (LPARAM)bInComment);
	AddCustomTaskVar(task, PARSE_DOM_IN_COMMENT_CNT, bInCommentCnt);
	
	if (eof) {
		LPSTR prevtag;
		if (currText) {
			TextParsed(window, task, currText);
			GlobalFree((HGLOBAL)currText);
		}
		GetCustomTaskVar(task, PARSE_DOM_CURR_PARSED_TAG, (LPARAM far *)&prevtag, NULL);
		if (prevtag) GlobalFree((HGLOBAL)prevtag);
		GetCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM far *)&lpCurrTagStart, NULL);
		GetCustomTaskVar(task, PARSE_DOM_CURR_ATTRIB, (LPARAM far *)&lpCurrAttribStart, NULL);
		GetCustomTaskVar(task, PARSE_DOM_CURR_VALUE, (LPARAM far *)&lpCurrValueStart, NULL);
		if (lpCurrTagStart) GlobalFree((HGLOBAL)lpCurrTagStart);
		if (lpCurrAttribStart) GlobalFree((HGLOBAL)lpCurrAttribStart);
		if (lpCurrValueStart) GlobalFree((HGLOBAL)lpCurrValueStart);
	}
	
	*buff = ptr;
	
	return TRUE;
}