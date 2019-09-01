#include "ivoyager.h"
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
#define PARSE_DOM_LAST_WAS_HTML_OPEN			109
#define PARSE_DOM_LAST_SLASH					110

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
#define PARSE_CSS_MEDIA							213
#define PARSE_CSS_KEYFRAMES						214
#define PARSE_CSS_BLOCK_PARAMS					215
#define PARSE_CSS_BLOCKTYPE						216
#define PARSE_CSS_FOUND_SLASH					217
#define PARSE_CSS_LAST_ESCAPE					218

#define PARSE_CSS_STATE_FIND_SELECTOR			0
#define PARSE_CSS_STATE_FIND_NEXT_SELECTOR		1
#define PARSE_CSS_STATE_IN_SELECTOR				2
#define PARSE_CSS_STATE_FIND_BLOCK				3
#define PARSE_CSS_STATE_FIND_PROP				4
#define PARSE_CSS_STATE_IN_PROP					5
#define PARSE_CSS_STATE_FIND_COLON				6
#define PARSE_CSS_STATE_FIND_VALUE				7
#define PARSE_CSS_STATE_IN_VALUE				8
#define PARSE_CSS_STATE_IN_MAYBEDONE			9
#define PARSE_CSS_STATE_FIND_NEXT_BLOCK_PARAMS	10
#define PARSE_CSS_STATE_IN_BLOCK_PARAMS			11
#define PARSE_CSS_STATE_IN_UNKNOWN_BLOCK		12

#define COMPILE_JS_VAR_STATE  					300

BOOL CompileJSChunk(ContentWindow far *window, Task far *task, LPARAM *state, DomNode far *scriptEl, char far **buff, int len, BOOL eof) {
	*state = PARSE_STATE_TAG_START;
	AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, *state);
	return TRUE;
}

BOOL CSSEnd(ContentWindow far *window, Task far *task, LPARAM far *state) {
	Task far *findEndTask = AllocTempTask();
	AddCustomTaskVar(task, PARSE_CSS_RESTORE_STATE, *state);
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
    BOOL last_escape = FALSE;
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
				ptr = comment_start;
				comment_start = NULL;
			}
			else {
				inStar = FALSE;
			}
		}
		else {
			if (*ptr == '"') {
				if (last_escape) { /*lstrcpy(ptr-1, ptr);*/ }
				else inDblQuote = !inDblQuote;
			}
			else if (*ptr == '\'') {
				if (last_escape) { /*lstrcpy(ptr-1, ptr);*/ }
				else inSgnlQuote = !inSgnlQuote;
			}
			else if ((inSgnlQuote || inDblQuote) && *ptr == '\\') {
				last_escape = TRUE;
			}
			else if (! inDblQuote && ! inSgnlQuote) {
				last_escape = FALSE; 
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
			else {
				last_escape = FALSE;
			}
		}
		ptr++;
	}
	
	if (comment_start) lstrcpy(comment_start, "");
	
	return TRUE;
}

BOOL inBlock = FALSE;
BOOL inSelector = FALSE;
BOOL bLastSelector = FALSE;
BOOL bLastBlockParams = FALSE;
BOOL bLastProperty = FALSE;

BOOL CSSOpenBracket(ContentWindow far *window, Task far *task) {
	//bLastBlockParams = bLastSelector = FALSE;
	DebugLogAttr(window->tab, TRUE, FALSE, RGB(0,0,0));	
	DebugLog(window->tab, " {\r\n");
	ResetDebugLogAttr(window->tab);
	return TRUE;
}

BOOL CSSCloseBracket(ContentWindow far *window, Task far *task) {

	bLastBlockParams = bLastSelector = bLastProperty = FALSE;
	
	if (! inSelector && ! inBlock) return FALSE;

	if (inSelector) inSelector = FALSE;
	else if (inBlock) inBlock = FALSE;
	DebugLogAttr(window->tab, TRUE, FALSE, RGB(0,0,0));
	if (inBlock) DebugLog(window->tab, "\t}\r\n");
	else DebugLog(window->tab, "}\r\n");
	ResetDebugLogAttr(window->tab);
	return TRUE;
}

BOOL SelectorParsed(ContentWindow far *window, Task far *task, char far *ptr, LPARAM *bInComment, LPARAM *state) {
	LPSTR fullptr = ConcatVar(task, PARSE_CSS_CURR_SELECTOR, ptr);
		
	StripCSSComment(fullptr, bInComment);
	if (! IsWhitespace(fullptr)) {
		fullptr = Trim(fullptr, TRUE, TRUE);
		inSelector = TRUE;
		if (bLastSelector == TRUE) DebugLog(window->tab, ", ");
		else if (! bLastBlockParams) DebugLog(window->tab, "\r\n");
		if (inBlock && ! bLastSelector) DebugLog(window->tab, "\t");

		DebugLogAttr(window->tab, TRUE, FALSE, RGB(0,0,255));
		DebugLog(window->tab, "%s", Trim(fullptr, TRUE, TRUE));
		ResetDebugLogAttr(window->tab);

		bLastSelector = TRUE;
		bLastProperty = FALSE;
		//MessageBox(window->hWnd, fullptr, "CSS Selector", MB_OK);
	}
	else {
		*state = PARSE_CSS_STATE_FIND_SELECTOR;
		AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, *state);
	}
	
	GlobalFree((HGLOBAL)fullptr);
	
	AddCustomTaskVar(task, PARSE_CSS_CURR_SELECTOR, (LPARAM)NULL);
	
	return TRUE;
}

BOOL CSSValueParsed(ContentWindow far *window, Task far *task, char far *ptr, LPARAM *bInComment) {
	LPSTR fullptr = ConcatVar(task, PARSE_CSS_CURR_VALUE, ptr);
	
	StripCSSComment(fullptr, bInComment);
	if (! IsWhitespace(fullptr)) {
		bLastBlockParams = bLastSelector = FALSE;
		DebugLogAttr(window->tab, FALSE, FALSE, RGB(66, 66, 66));
		DebugLog(window->tab, "%s", Trim(fullptr, TRUE, TRUE));
		ResetDebugLogAttr(window->tab);
		DebugLog(window->tab, ";\r\n");
		bLastProperty = FALSE;
		//MessageBox(window->hWnd, Trim(fullptr, TRUE, TRUE), "CSSValue", MB_OK);
	}
	
	GlobalFree((HGLOBAL)fullptr);
	
	AddCustomTaskVar(task, PARSE_CSS_CURR_VALUE, (LPARAM)NULL);
	
	return TRUE;
}

BOOL BlockTypeParsed(ContentWindow far *window, Task far *task, char far *ptr, LPARAM *bInComment, LPARAM *state) {
	LPSTR fullptr = ConcatVar(task, PARSE_CSS_BLOCKTYPE, ptr);
	
	StripCSSComment(fullptr, bInComment);
	if (! IsWhitespace(fullptr)) {	
		bLastBlockParams = bLastSelector = FALSE;
		DebugLog(window->tab, "\r\n");
		DebugLogAttr(window->tab, TRUE, FALSE, RGB(0,0,100));
		DebugLog(window->tab, "@%s", Trim(fullptr, TRUE, TRUE));
		ResetDebugLogAttr(window->tab);
		DebugLog(window->tab, " ");
		bLastProperty = FALSE;
		//MessageBox(window->hWnd, Trim(fullptr, TRUE, TRUE), "CSS Block Type", MB_OK);
	}
	else {
		*state = PARSE_CSS_STATE_FIND_SELECTOR;
	}
	
	GlobalFree((HGLOBAL)fullptr);
	
	AddCustomTaskVar(task, PARSE_CSS_BLOCKTYPE, (LPARAM)NULL);
	
	return TRUE;
}



BOOL BlockParamsParsed(ContentWindow far *window, Task far *task, char far *ptr, LPARAM *bInComment, LPARAM *state) {
	LPSTR fullptr = ConcatVar(task, PARSE_CSS_BLOCK_PARAMS, ptr);
	
	StripCSSComment(fullptr, bInComment);
	if (! IsWhitespace(fullptr)) {	
		inBlock = TRUE;
		if (bLastBlockParams == TRUE) DebugLog(window->tab, ",");
		DebugLog(window->tab, " ");
		DebugLogAttr(window->tab, FALSE, FALSE, RGB(0,0,145));
		DebugLog(window->tab, "%s", Trim(fullptr, TRUE, TRUE));
		ResetDebugLogAttr(window->tab);
		DebugLog(window->tab, " ");
		//MessageBox(window->hWnd, Trim(fullptr, TRUE, TRUE), "CSS Block Params", MB_OK);
		bLastBlockParams = TRUE;
		bLastProperty = FALSE;
	}
	
	GlobalFree((HGLOBAL)fullptr);
	
	AddCustomTaskVar(task, PARSE_CSS_BLOCK_PARAMS, (LPARAM)NULL);

	return TRUE;
}

BOOL CSSPropertyParsed(ContentWindow far *window, Task far *task, char far *ptr, LPARAM *bInComment) {
	LPSTR fullptr = ConcatVar(task, PARSE_CSS_CURR_PROP, ptr);
	
	StripCSSComment(fullptr, bInComment);
	if (! IsWhitespace(fullptr)) {
		if (bLastProperty) DebugLog(window->tab, "\r\n");
		if (inBlock) DebugLog(window->tab, "\t\t");
		else DebugLog(window->tab, "\t");
		//MessageBox(window->hWnd, Trim(fullptr, TRUE, TRUE), "CSS Property", MB_OK);
		DebugLogAttr(window->tab, TRUE, FALSE, RGB(122,0,0));
		DebugLog(window->tab, "%s", Trim(fullptr, TRUE, TRUE));
		ResetDebugLogAttr(window->tab);
		DebugLog(window->tab, ": ");
		bLastProperty = TRUE;
	}
	
	GlobalFree((HGLOBAL)fullptr);
	
	AddCustomTaskVar(task, PARSE_CSS_CURR_PROP, (LPARAM)NULL);
	
	return TRUE;
}

BOOL ParseCSSChunk(ContentWindow far *window, Task far *task, LPARAM *dom_state, DomNode far *styleEl, char far **buff, int len, BOOL eof) {
	LPARAM state;
	
	LPSTR lpCurrSelectorStart = NULL;
	LPSTR lpCurrPropStart = NULL;
	LPSTR lpCurrValueStart = NULL;
	LPSTR blockType = NULL;
	LPSTR blockParams = NULL;
	
	BOOL bDone = FALSE;
	
	LPARAM bInComment = FALSE;
	LPARAM bLastStar = FALSE;
	
	char far *ptr = *buff;
	char far *end = *buff + len;
	
	LPARAM bInSQuote = FALSE;
	LPARAM bInDQuote = FALSE;
	
	LPARAM last_slash = FALSE;
	
	LPARAM last_escape = FALSE;
	
	GetCustomTaskVar(task, PARSE_CSS_FOUND_SLASH, &last_slash, NULL);
		
	GetCustomTaskVar(task, PARSE_CSS_VAR_STATE, &state, NULL);
	GetCustomTaskVar(task, PARSE_CSS_IN_COMMENT, &bInComment, NULL);
	GetCustomTaskVar(task, PARSE_CSS_LAST_STAR, &bLastStar, NULL);
	GetCustomTaskVar(task, PARSE_CSS_LAST_ESCAPE, &last_escape, NULL);

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
		case PARSE_CSS_STATE_IN_UNKNOWN_BLOCK:
			blockType = *buff;
			break;
		case PARSE_CSS_STATE_IN_BLOCK_PARAMS:
			GetCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, &bInSQuote, NULL);
			GetCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, &bInDQuote, NULL);
			blockParams = *buff;
			break;
			
	}
	
	//DebugLog(window->tab, "\nbegin state: %ld - \"%s\"\n", state, *buff);
	
	for (;;) {
		BOOL bNoInc = FALSE;
		
		if (bInComment && state != PARSE_CSS_STATE_IN_MAYBEDONE) {
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
		
		if (bInComment && state != PARSE_CSS_STATE_IN_MAYBEDONE) break;
		
		switch (state) {
			case PARSE_CSS_STATE_FIND_SELECTOR:
				while (ptr < end) {
					if (*ptr == '<') {
						last_slash = FALSE;
						CSSEnd(window, task, &state);
						bNoInc = TRUE;
						break;
					}
					else if (*ptr == '@') {
						last_slash = FALSE;
						state = PARSE_CSS_STATE_IN_UNKNOWN_BLOCK;
						//MessageBox(window->hWnd, "block found", "", MB_OK);
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						blockType = ptr+1;
						break;
					}
					else if (*ptr == '/') {
						last_slash = TRUE;
					}
					else if (*ptr == '*') {
						
						if (last_slash) {
							bInComment = TRUE;
							last_slash = FALSE;
							break;
						}
						
					}
					else if (*ptr == '}') {
						last_slash = FALSE;
						CSSCloseBracket(window, task);
						// todo close block
					}
					else if (! isspace(*ptr)) {
						last_slash = FALSE;
						lpCurrSelectorStart = ptr;
						
						state = PARSE_CSS_STATE_IN_SELECTOR;
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						bNoInc = TRUE;
						break;
					}
					ptr++;
				}
				break;
			case PARSE_CSS_STATE_IN_UNKNOWN_BLOCK: {
				while (ptr < end) {
					if (*ptr == '<') {
						CSSEnd(window, task, &state);
						bNoInc = TRUE;
						break;
					}
					else if (isspace(*ptr)) {
						*ptr = '\0';
						state = PARSE_CSS_STATE_IN_BLOCK_PARAMS;
						BlockTypeParsed(window, task, blockType, &bInComment, &state);
						blockParams = ptr+1;					
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						break;
					}
					else if (*ptr == '{') {
						*ptr = '\0';
						BlockTypeParsed(window, task, blockType, &bInComment, &state);
						state = PARSE_CSS_STATE_FIND_SELECTOR;
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						CSSOpenBracket(window, task);
						break;
					}
					ptr++;
				}
				break;
			}
			case PARSE_CSS_STATE_IN_BLOCK_PARAMS:
				while (ptr < end) {
					if (*ptr == '<') {
						CSSEnd(window, task, &state);
						*ptr = '\0';
						ConcatVar(task, PARSE_CSS_BLOCK_PARAMS, blockParams);
						*ptr = '<';
						bNoInc = TRUE;
						break;
					}
					if (bInSQuote) {
						if (*ptr == '\'' && last_escape) {
							last_escape = FALSE;
						}
						else if (*ptr == '\'' || *ptr == '\n') {
							last_escape = FALSE;
							bInSQuote = FALSE;
							AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
							AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
						}
						else if (*ptr == '\\') {
							last_escape = TRUE;
						}
						else {
							last_escape = FALSE;
						}
					}
					else if (bInDQuote) {
						if (*ptr == '\"' && last_escape) {
							last_escape = FALSE;
						}
						else if (*ptr == '\"' || *ptr == '\n') {
							last_escape = FALSE;
							bInDQuote = FALSE;
							AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
							AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
						}
						else if (*ptr == '\\') {
							last_escape = TRUE;
						}
						else {
							last_escape = FALSE;
						}
					}
					else {
						if (*ptr == '\'') {
							bInSQuote = TRUE;
							AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
							AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
						}
						else if (*ptr == '\"') {
							bInDQuote = TRUE;
							AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
							AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
						}
						else if (*ptr == ',') {
							state = PARSE_CSS_STATE_IN_BLOCK_PARAMS;
							*ptr = '\0';
							//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
							BlockParamsParsed(window, task, blockParams, &bInComment, &state);
							AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
							
							blockParams = ptr+1;

							break;
						}
						else if (*ptr == '{') {
							//state = PARSE_CSS_STATE_FIND_PROP;
							*ptr = '\0';
							//MessageBox(window->hWnd, lpCurrSelectorStart, "tag name", MB_OK);
							BlockParamsParsed(window, task, blockParams, &bInComment, &state);
							state = PARSE_CSS_STATE_FIND_SELECTOR;
							AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
							CSSOpenBracket(window, task);
							break;
						}
					}
					ptr++;
				}
				break;
			case PARSE_CSS_STATE_IN_SELECTOR:
				while (ptr < end) {
					if (*ptr == '<') {
						CSSEnd(window, task, &state);
						*ptr = '\0';
						ConcatVar(task, PARSE_CSS_CURR_SELECTOR, lpCurrSelectorStart);
						*ptr = '<';
						bNoInc = TRUE;
						break;
					}
					if (bInSQuote) {
						if (*ptr == '\'' && last_escape) {
							last_escape = FALSE;
						}
						else if (*ptr == '\'' || *ptr == '\n') {
							last_escape = FALSE;
							bInSQuote = FALSE;
							AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
							AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
						}
						else if (*ptr == '\\') {
							last_escape = TRUE;
						}
						else {
							last_escape = FALSE;
						}
					}
					else if (bInDQuote) {
						if (*ptr == '\"' && last_escape) {
							last_escape = FALSE;
						}
						else if (*ptr == '\"' || *ptr == '\n') {
							last_escape = FALSE;
							bInDQuote = FALSE;
							AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
							AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
						}
						else if (*ptr == '\\') {
							last_escape = TRUE;
						}
						else {
							last_escape = FALSE;
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
							AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
							AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
						}
						else if (*ptr == '\"') {
							bInDQuote = TRUE;
							AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
							AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
						}
						else if (*ptr == ',') {
							//state = PARSE_CSS_STATE_FIND_NEXT_SELECTOR;
							state = PARSE_CSS_STATE_IN_SELECTOR;
							*ptr = '\0';
							//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
							SelectorParsed(window, task, lpCurrSelectorStart, &bInComment, &state);
							lpCurrSelectorStart = ptr+1;
							AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);

							break;
						}
						else if (*ptr == '{') {
							//state = PARSE_CSS_STATE_FIND_PROP;
							*ptr = '\0';
							//MessageBox(window->hWnd, lpCurrSelectorStart, "tag name", MB_OK);
							state = PARSE_CSS_STATE_FIND_PROP;
							SelectorParsed(window, task, lpCurrSelectorStart, &bInComment, &state);
							
							//DebugLog(window->tab, "!!");
							
							//lpCurrSelectorStart = NULL;
							
							AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
							CSSOpenBracket(window, task);
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
						//MessageBox(window->hWnd, "closed bracket", "", MB_OK);
						//*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
						//SelectorParsed(window, task, lpCurrSelectorStart);
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						CSSCloseBracket(window, task);
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
						*ptr = '\0';
						ConcatVar(task, PARSE_CSS_CURR_PROP, lpCurrPropStart);
						*ptr = '<';
						break;
					}
					else if (*ptr == '}') {
						state = PARSE_CSS_STATE_FIND_SELECTOR;
						//*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrTagStart, "tag name", MB_OK);
						//SelectorParsed(window, task, lpCurrSelectorStart);
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						CSSCloseBracket(window, task);
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
						CSSCloseBracket(window, task);
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
						CSSCloseBracket(window, task);
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
						*ptr = '\0';
						ConcatVar(task, PARSE_CSS_CURR_VALUE, lpCurrValueStart);
						*ptr = '<';
						break;
					}
					if (bInSQuote) {
						if (*ptr == '\'' && last_escape) {
							last_escape = FALSE;
						}
						else if (*ptr == '\'' || *ptr == '\n') {
							last_escape = FALSE;
							bInSQuote = FALSE;
							AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
							AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
						}
						else if (*ptr == '\\') {
							last_escape = TRUE;
						}
						else {
							last_escape = FALSE;
						}
					}
					else if (bInDQuote) {
						if (*ptr == '\"' && last_escape) {
							last_escape = FALSE;
						}
						else if (*ptr == '\"' || *ptr == '\n') {
							last_escape = FALSE;
							bInDQuote = FALSE;
							AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
							AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
						}
						else if (*ptr == '\\') {
							last_escape = TRUE;
						}
						else {
							last_escape = FALSE;
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
							AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
							AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
						}
						else if (*ptr == '\"') {
							bInDQuote = TRUE;
							AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
							AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
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
							CSSCloseBracket(window, task);
							break;
						}
					}
					ptr++;
				}
				break;
			case PARSE_CSS_STATE_IN_MAYBEDONE: {
				Task far *findEndTask;
				char far *start_ptr;
				LPARAM bTaskComplete = FALSE;
				LPARAM bFoundEnd = FALSE;
				LPARAM prev_state;
				char chRestore;
				
				GetCustomTaskVar(task, PARSE_CSS_RESTORE_STATE, &prev_state, NULL);
				//MessageBox(window->hWnd, ConcatVar(task, PARSE_CSS_CURR_SELECTOR, ""), "sel", MB_OK);
				GetCustomTaskVar(task, PARSE_CSS_FIND_END_TASK, (LPARAM far *)&findEndTask, NULL);
				//MessageBox(window->hWnd, ptr, "FIND END CSS", MB_OK);
				start_ptr = ptr;
				ParseDOMChunk(NULL, findEndTask, &ptr, len-(ptr-*buff), eof);
				
				lpCurrSelectorStart = NULL;
				lpCurrValueStart = NULL;
				lpCurrValueStart = NULL;
				blockType = NULL;
				blockParams = NULL;
				
				GetCustomTaskVar(findEndTask, TASK_COMPLETE, (LPARAM far *)&bTaskComplete, NULL);
				GetCustomTaskVar(findEndTask, PARSE_CSS_FOUND_END_TASK, (LPARAM far *)&bFoundEnd, NULL);
				if (eof || bTaskComplete) {
					
					
					
					if (bFoundEnd) {
						bDone = TRUE;
						//MessageBox(window->hWnd, ptr, "found end", MB_OK);
					}
					else {
						
						BOOL bPast = ptr>=end;
						
						if (bPast) ptr--;
						chRestore = *ptr;
						*ptr = '\0';
						switch (prev_state) {
							case PARSE_CSS_STATE_IN_SELECTOR:
							//MessageBox(window->hWnd, start_ptr, "add 1", MB_OK);
								ConcatVar(task, PARSE_CSS_CURR_SELECTOR, start_ptr);
								lpCurrSelectorStart = ptr;
								GetCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, &bInSQuote, NULL);
								GetCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, &bInDQuote, NULL);
								break;
							case PARSE_CSS_STATE_IN_PROP:
								ConcatVar(task, PARSE_CSS_CURR_PROP, start_ptr);
								lpCurrPropStart = ptr;
								break;
							case PARSE_CSS_STATE_IN_VALUE:
								ConcatVar(task, PARSE_CSS_CURR_VALUE, start_ptr);
								lpCurrValueStart = ptr;
								GetCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, &bInSQuote, NULL);
								GetCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, &bInDQuote, NULL);
								break;
							case PARSE_CSS_STATE_IN_UNKNOWN_BLOCK:
								ConcatVar(task, PARSE_CSS_BLOCKTYPE, start_ptr);
								blockType = ptr;;
								break;
							case PARSE_CSS_STATE_IN_BLOCK_PARAMS:
								ConcatVar(task, PARSE_CSS_BLOCK_PARAMS, start_ptr);
								blockParams = ptr;
								GetCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, &bInSQuote, NULL);
								GetCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, &bInDQuote, NULL);
								break;								
						}
						*ptr = chRestore;
						
						if (bPast) ptr++;
						
						//MessageBox(window->hWnd, ptr, "cont", MB_OK);
						
						GetCustomTaskVar(task, PARSE_CSS_RESTORE_STATE, &state, NULL);
						AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
						bNoInc = TRUE;
					}
					FreeTempTask(findEndTask);
					AddCustomTaskVar(task, PARSE_CSS_FIND_END_TASK, (LPARAM)NULL);
				}
				else {
					BOOL bPast = ptr>=end;
						
					if (bPast) ptr--;
					chRestore = *ptr;
					*ptr = '\0';
					switch (prev_state) {
						case PARSE_CSS_STATE_IN_SELECTOR:
						//MessageBox(window->hWnd, start_ptr, "add 2", MB_OK);
							ConcatVar(task, PARSE_CSS_CURR_SELECTOR, start_ptr);
							lpCurrSelectorStart = NULL;
							break;
						case PARSE_CSS_STATE_IN_PROP:
							ConcatVar(task, PARSE_CSS_CURR_PROP, start_ptr);
							lpCurrPropStart = NULL;
							break;
						case PARSE_CSS_STATE_IN_VALUE:
							ConcatVar(task, PARSE_CSS_CURR_VALUE, start_ptr);
							lpCurrValueStart = NULL;
							break;
						case PARSE_CSS_STATE_IN_UNKNOWN_BLOCK:
							ConcatVar(task, PARSE_CSS_BLOCKTYPE, start_ptr);
							blockType = NULL;
							break;
						case PARSE_CSS_STATE_IN_BLOCK_PARAMS:
							ConcatVar(task, PARSE_CSS_BLOCK_PARAMS, start_ptr);
							blockParams = NULL;
							break;
					}
					*ptr = chRestore;	
					
					if (bPast) ptr++;

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
					//MessageBox(window->hWnd, lpCurrSelectorStart, "add 3", MB_OK);
					ConcatVar(task, PARSE_CSS_CURR_SELECTOR, lpCurrSelectorStart);
				}
				break;
			case PARSE_CSS_STATE_IN_PROP:
				if (lpCurrPropStart) {
					//DebugLog(window->tab, "saved in prop \"%s\"", lpCurrPropStart);
					ConcatVar(task, PARSE_CSS_CURR_PROP, lpCurrPropStart);
				}
				else {
					//DebugLog(window->tab, "saved in prop \"%NULL\"");
				}
				break;
			case PARSE_CSS_STATE_IN_VALUE:
				if (lpCurrValueStart) {
					AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
					AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
					ConcatVar(task, PARSE_CSS_CURR_VALUE, lpCurrValueStart);
				}
				break;
			case PARSE_CSS_STATE_IN_BLOCK_PARAMS:
				if (blockParams) {
					AddCustomTaskVar(task, PARSE_CSS_IN_SQUOTE, (LPARAM)bInSQuote);
					AddCustomTaskVar(task, PARSE_CSS_IN_DQUOTE, (LPARAM)bInDQuote);
					//MessageBox(window->hWnd, lpCurrSelectorStart, "add 3", MB_OK);
					ConcatVar(task, PARSE_CSS_BLOCK_PARAMS, blockParams);
				}
				break;
			case PARSE_CSS_STATE_IN_UNKNOWN_BLOCK:
				if (blockType) {
					ConcatVar(task, PARSE_CSS_BLOCKTYPE, blockType);
				}
				break;
		}
	}
	
	AddCustomTaskVar(task, PARSE_CSS_IN_COMMENT, (LPARAM)bInComment);
	AddCustomTaskVar(task, PARSE_CSS_LAST_STAR, (LPARAM)bLastStar);
	
	AddCustomTaskVar(task, PARSE_CSS_FOUND_SLASH, last_slash);
	
	AddCustomTaskVar(task, PARSE_CSS_LAST_ESCAPE, last_escape);
	
	if (eof || bDone) {
		Task far *findEndTask;
		//ptr--;
		state = 0;
		GetCustomTaskVar(task, PARSE_CSS_FIND_END_TASK, (LPARAM far *)&findEndTask, NULL);
		GetCustomTaskVar(task, PARSE_CSS_CURR_SELECTOR, (LPARAM far *)&lpCurrSelectorStart, NULL);
		GetCustomTaskVar(task, PARSE_CSS_STATE_IN_PROP, (LPARAM far *)&lpCurrPropStart, NULL);
		GetCustomTaskVar(task, PARSE_CSS_CURR_VALUE, (LPARAM far *)&lpCurrValueStart, NULL);
		GetCustomTaskVar(task, PARSE_CSS_STATE_IN_BLOCK_PARAMS, (LPARAM far *)&blockParams, NULL);
		GetCustomTaskVar(task, PARSE_CSS_BLOCKTYPE, (LPARAM far *)&blockType, NULL);
		if (lpCurrSelectorStart) { GlobalFree((HGLOBAL)lpCurrSelectorStart); AddCustomTaskVar(task, PARSE_CSS_CURR_SELECTOR, (LPARAM)NULL); }
		if (lpCurrPropStart) { GlobalFree((HGLOBAL)lpCurrPropStart); AddCustomTaskVar(task, PARSE_CSS_STATE_IN_PROP, (LPARAM)NULL); }
		if (lpCurrValueStart) { GlobalFree((HGLOBAL)lpCurrValueStart); AddCustomTaskVar(task, PARSE_CSS_CURR_VALUE, (LPARAM)NULL); }
		if (blockParams) { GlobalFree((HGLOBAL)blockParams); AddCustomTaskVar(task, PARSE_CSS_STATE_IN_BLOCK_PARAMS, (LPARAM)NULL); }
		if (blockType) { GlobalFree((HGLOBAL)blockType); AddCustomTaskVar(task, PARSE_CSS_BLOCKTYPE, (LPARAM)NULL); }

		if (findEndTask) { FreeTempTask(findEndTask); AddCustomTaskVar(task, PARSE_CSS_FIND_END_TASK, (LPARAM)NULL); }
		
		AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, PARSE_CSS_STATE_FIND_SELECTOR);
	}
	
	if (bDone) {
		*dom_state = PARSE_STATE_TAG_START;
		AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, *dom_state);
	}
	
	*buff = ptr;
	
	//DebugLog(window->tab, "\nend state: %ld - \"%s\"\n", state, ptr);
	AddCustomTaskVar(task, PARSE_CSS_VAR_STATE, state);
	
	return TRUE;
}

BOOL TagParsed(ContentWindow far *window, Task far *task, char far *ptr) {
	LPSTR prevtag;
	LPSTR fullptr = ConcatVar(task, PARSE_DOM_CURR_TAG, ptr);
	
	GetCustomTaskVar(task, PARSE_DOM_CURR_PARSED_TAG, (LPARAM far *)&prevtag, NULL);
	if (prevtag) GlobalFree((HGLOBAL)prevtag);
	
	if (window && ! IsWhitespace(fullptr)) {
		DebugLogAttr(window->tab, TRUE, FALSE, RGB(0,0,0));
		DebugLog(window->tab, "<");
		DebugLogAttr(window->tab, TRUE, fullptr[0] == '/' ? TRUE : FALSE, RGB(138,0,0));
		DebugLog(window->tab, "%s", fullptr);
		ResetDebugLogAttr(window->tab);
	}
	
	AddCustomTaskVar(task, PARSE_DOM_CURR_TAG, (LPARAM)NULL);
	AddCustomTaskVar(task, PARSE_DOM_CURR_PARSED_TAG, (LPARAM)fullptr);
	
	return TRUE;
}

BOOL AttribNameParsed(ContentWindow far *window, Task far *task, char far *ptr) {
	LPSTR fullptr = ConcatVar(task, PARSE_DOM_CURR_ATTRIB, ptr);
	if (window && ! IsWhitespace(fullptr)) {
		DebugLog(window->tab, " ");
		DebugLogAttr(window->tab, FALSE, FALSE, RGB(0,138,0));
		DebugLog(window->tab, "%s", fullptr);
		ResetDebugLogAttr(window->tab);
	}
	GlobalFree((HGLOBAL)fullptr);
	
	AddCustomTaskVar(task, PARSE_DOM_CURR_ATTRIB, (LPARAM)NULL);
	return TRUE;
}

BOOL AttribValueParsed(ContentWindow far *window, Task far *task, char far *ptr) {
	LPSTR fullptr = ConcatVar(task, PARSE_DOM_CURR_VALUE, ptr);
	if (window && ! IsWhitespace(fullptr)) {
		DebugLog(window->tab, "=\"");
		DebugLogAttr(window->tab, FALSE, FALSE, RGB(0,0,138));
		DebugLog(window->tab, "%s", fullptr);
		ResetDebugLogAttr(window->tab);
		DebugLog(window->tab, "\"");
	}
	GlobalFree((HGLOBAL)fullptr);
	
	AddCustomTaskVar(task, PARSE_DOM_CURR_VALUE, (LPARAM)NULL);
	return TRUE;
}

BOOL TextParsed(ContentWindow far *window, Task far *task, char far *ptr) {
	///*if (! IsWhitespace(ptr)) */MessageBox(window->hWnd, ptr, "text", MB_OK);

	//DebugLog("\"%s\" - Text parsed", ptr);
	if (window) {
		DebugLogAttr(window->tab, FALSE, TRUE, RGB(0,0,0));
		DebugLog(window->tab, "%s", ptr);
		ResetDebugLogAttr(window->tab);
	}
	
	return TRUE;
}

BOOL TagDone(ContentWindow far *window, Task far *task, LPARAM *state, LPARAM autoclose) {
	LPSTR tag;
	BOOL ret = TRUE;
	LPARAM bFindCssEnd = FALSE;
	GetCustomTaskVar(task, PARSE_DOM_CURR_PARSED_TAG, (LPARAM far *)&tag, NULL);
	GetCustomTaskVar(task, PARSE_CSS_IN_FIND_END_TASK, (LPARAM far *)&bFindCssEnd, NULL);

	if (bFindCssEnd) {
		if (tag) {
			if (! _fstricmp(tag, "/style")) {
				AddCustomTaskVar(task, PARSE_CSS_FOUND_END_TASK, (LPARAM)TRUE);
				if (window) {
					DebugLogAttr(window->tab, TRUE, FALSE, RGB(0,0,0));
					DebugLog(window->tab, "\r\n<");
					DebugLogAttr(window->tab, TRUE, TRUE, RGB(138,0,0));
					DebugLog(window->tab, "/style");
					
					DebugLogAttr(window->tab, TRUE, FALSE, RGB(0,0,0));
					DebugLog(window->tab, ">");
					ResetDebugLogAttr(window->tab);
				}
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
		if (window) {
			DebugLogAttr(window->tab, TRUE, FALSE, RGB(0,0,0));
			if (autoclose) DebugLog(window->tab, " />");
			else DebugLog(window->tab, ">");
			ResetDebugLogAttr(window->tab);
		}
		if (! _fstricmp(tag, "style")) {
			if (window) DebugLog(window->tab, "\r\n");
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
	LPARAM bLastWasOpen = FALSE;

	LPSTR currText = NULL;
	LPSTR currTag = NULL;
	LPSTR currAttrib = NULL;
	LPSTR currValue = NULL;
	BOOL brk = FALSE;
	LPARAM last_slash = FALSE;
	
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
	GetCustomTaskVar(task, PARSE_DOM_LAST_WAS_HTML_OPEN, &bLastWasOpen, NULL);
	GetCustomTaskVar(task, PARSE_DOM_LAST_SLASH, &last_slash, NULL);
	
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
		
		BOOL noInc = FALSE;
		
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
				//currNode = (DomNode far *)GlobalAlloc(GMEM_FIXED, sizeof(DomNode));
				//_fmemset(currNode, 0, sizeof(DomNode));
			
				state = PARSE_STATE_TAG_START;
				AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
			
				continue;
			case PARSE_STATE_CSS_CODE:
				ParseCSSChunk(window, task, &state, NULL, &ptr, len-(ptr-*buff), eof);
				last_node_ptr = ptr;
				//if (ptr < end) DebugLog(window->tab, "done css: \"%s\"\n", ptr);
				//if (ptr < end && state != PARSE_STATE_CSS_CODE) continue;
				//AddCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM)NULL);	
				if (state == PARSE_STATE_CSS_CODE) break;
			case PARSE_STATE_TAG_START: {
				BOOL inTag = FALSE;
				char chRestore;
				char far *ptrLastPos = NULL;
				
				BOOL prevTag = bLastWasOpen;
			
				bInCommentCnt = 0;				
			
				while (ptr < end) {
					if (*ptr == '<') {
						bLastWasOpen = TRUE;
					}
					else if (bLastWasOpen && ! isspace(*ptr)) {
						inTag = TRUE;
						bLastWasOpen = FALSE;
						prevTag = FALSE;
						break;
					}
					else {
						bLastWasOpen = FALSE;
						ptrLastPos = ptr+1;
					}
					ptr++;
				}
				
				//if (inTag) ptr--;
				
				if (prevTag && ! inTag) ConcatVar(task, PARSE_DOM_CURR_TEXT, "<");
				if (ptrLastPos) {
					chRestore = *ptrLastPos;
					*ptrLastPos = '\0';
					ConcatVar(task, PARSE_DOM_CURR_TEXT, last_node_ptr);
					//MessageBoxA(window->hWnd, last_node_ptr, "add text", MB_OK);
					*ptrLastPos = chRestore;
				}
				
				if (bLastWasOpen) break; 

				GetCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM far *)&currText, NULL);				
				
				if (inTag) {
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
					lpCurrTagStart = ptr;
					noInc = TRUE;
					break;
				}
				//else if (currText) {
				//	AddCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM)currText);
				//}

				break;
			}
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
						if (! TagDone(window, task, &state, last_slash)) { ptr++; brk = TRUE; eof = TRUE; }
						last_slash = FALSE;
						break;
					}
					else if (bInCommentCnt != 0 && *ptr == '/') {
						last_slash = TRUE;
						lstrcpy(ptr, ptr+1);
						end--;
						continue;
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
						if (! TagDone(window, task, &state, last_slash)) { ptr++; brk = TRUE; eof = TRUE; }
						last_slash = FALSE;
						break;
					}
					else if (*ptr == '/') { // might be autoclose (useful in xhtml)
						last_slash = TRUE;
						lstrcpy(ptr, ptr+1);
						end--;
						continue;
					}
					else if (! isspace(*ptr)) {
						state = PARSE_STATE_ATTRIB_NAME;
						lpCurrAttribStart = ptr;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						last_slash = FALSE;
						break;
					}
					ptr++;
				}
				break;
			case PARSE_STATE_ATTRIB_NAME:
				while (ptr < end) {
					if (*ptr == '>') {
						*ptr = '\0';
						AttribNameParsed(window, task, lpCurrAttribStart);
						state = PARSE_STATE_TAG_START;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						last_node_ptr = ptr+1;
						lpCurrTagStart = lpCurrValueStart = lpCurrAttribStart = NULL;
						if (! TagDone(window, task, &state, last_slash)) { ptr++; brk = TRUE; eof = TRUE; }
						last_slash = FALSE;
						break;
					}
					else if (*ptr == '/') { // might be autoclose (useful in xhtml)
						last_slash = TRUE;
						lstrcpy(ptr, ptr+1);
						end--;
						continue;
					}
					else if (*ptr == '=') {
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrAttribStart, "attrib name", MB_OK);
						AttribNameParsed(window, task, lpCurrAttribStart);
						state = PARSE_STATE_ATTRIB_FIND_VALUE;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						last_slash = FALSE;
						break;
					}
					else if (isspace(*ptr)) {
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrAttribStart, "attrib name", MB_OK);
						AttribNameParsed(window, task, lpCurrAttribStart);
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
						if (! TagDone(window, task, &state, last_slash)) { ptr++; brk = TRUE; eof = TRUE; }
						last_slash = FALSE;
						break;
					}
					else if (*ptr == '/') { // might be autoclose (useful in xhtml)
						last_slash = TRUE;
						lstrcpy(ptr, ptr+1);
						end--;
						continue;
					}
					else if (*ptr == '=') {
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrAttribStart, "attrib name", MB_OK);
						AttribNameParsed(window, task, lpCurrAttribStart);
						state = PARSE_STATE_ATTRIB_FIND_VALUE;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						last_slash = FALSE;
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
						if (! TagDone(window, task, &state, last_slash)) { ptr++; brk = TRUE; eof = TRUE; }
						last_slash = FALSE;
						break;
					}
					else if (*ptr == '/') { // might be autoclose (useful in xhtml)
						last_slash = TRUE;
						lstrcpy(ptr, ptr+1);
						end--;
						continue;
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
						
						last_slash  = FALSE;

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
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrValueStart, "attrib value", MB_OK);
						AttribValueParsed(window, task, lpCurrValueStart);
						state = PARSE_STATE_TAG_START;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						last_node_ptr = ptr+1;
						lpCurrTagStart = lpCurrValueStart = lpCurrAttribStart = NULL;
						if (! TagDone(window, task, &state, last_slash))  { ptr++; brk = TRUE; eof = TRUE; }
						last_slash = FALSE;
						break;
					}
					else if (*ptr == '/') { // might be autoclose (useful in xhtml)
						last_slash = TRUE;
						lstrcpy(ptr, ptr+1);
						end--;
						continue;
					}
					else if (isspace(*ptr)) {
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrValueStart, "attrib value", MB_OK);
						AttribValueParsed(window, task, lpCurrValueStart);
						state = PARSE_STATE_ATTRIB_FIND_NAME;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						lpCurrValueStart = lpCurrAttribStart = NULL;
						noInc = TRUE;
						break;
					}
					ptr++;
				}
				if (state == PARSE_STATE_ATTRIB_FIND_NAME) continue;
				break;
			case PARSE_STATE_ATTRIB_VALUE_SGLQOUTE:
				while (ptr < end) {
					if (*ptr == '\'') {
						char chRestore = *ptr;
						state = PARSE_STATE_ATTRIB_FIND_NAME;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrValueStart, "attrib value", MB_OK);
						AttribValueParsed(window, task, lpCurrValueStart);
						*ptr = chRestore;
						lpCurrValueStart = lpCurrAttribStart = NULL;
						break;
					}
					ptr++;
				}
				break;
			case PARSE_STATE_ATTRIB_VALUE_DBLQOUTE:
				while (ptr < end) {
					if (*ptr == '"') {
						char chRestore = *ptr;
						state = PARSE_STATE_ATTRIB_FIND_NAME;
						AddCustomTaskVar(task, PARSE_DOM_VAR_STATE, state);
						*ptr = '\0';
						//MessageBox(window->hWnd, lpCurrValueStart, "attrib value", MB_OK);
						AttribValueParsed(window, task, lpCurrValueStart);
						*ptr = chRestore;
						lpCurrValueStart = lpCurrAttribStart = NULL;
						break;
					}
					ptr++;
				}
				break;
		}

		if (brk) break;

		if (! noInc) ptr++;
		if (ptr >= end) break;
	}
	
	if (! eof) {
		GetCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM far *)&currText, NULL);	
		if (currText) {
			TextParsed(window, task, currText);
			GlobalFree((HGLOBAL)currText);
			AddCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM)NULL);
		}
		switch (state) {
			case PARSE_STATE_TAG_TAGNAME:
				if (lpCurrTagStart) {
					ConcatVar(task, PARSE_DOM_CURR_TAG, lpCurrTagStart);
				}
				break;
			case PARSE_STATE_ATTRIB_FIND_NAME:
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
			default:
				AddCustomTaskVar(task, PARSE_DOM_CURR_TAG, (LPARAM)NULL);
				AddCustomTaskVar(task, PARSE_DOM_CURR_ATTRIB, (LPARAM)NULL);
				AddCustomTaskVar(task, PARSE_DOM_CURR_VALUE, (LPARAM)NULL);
				break;
		}
	}
	
	AddCustomTaskVar(task, PARSE_DOM_IN_COMMENT, (LPARAM)bInComment);
	AddCustomTaskVar(task, PARSE_DOM_IN_COMMENT_CNT, bInCommentCnt);
	AddCustomTaskVar(task, PARSE_DOM_LAST_WAS_HTML_OPEN, bLastWasOpen);
	AddCustomTaskVar(task, PARSE_DOM_LAST_SLASH, last_slash);
	
	if (eof) {
		LPSTR prevtag;
		GetCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM far *)&currText, NULL);	
		if (currText) {
			TextParsed(window, task, currText);
			GlobalFree((HGLOBAL)currText);
			AddCustomTaskVar(task, PARSE_DOM_CURR_TEXT, (LPARAM)NULL);
		}
		GetCustomTaskVar(task, PARSE_DOM_CURR_PARSED_TAG, (LPARAM far *)&prevtag, NULL);
		if (prevtag) GlobalFree((HGLOBAL)prevtag);
		GetCustomTaskVar(task, PARSE_DOM_CURR_TAG, (LPARAM far *)&lpCurrTagStart, NULL);
		GetCustomTaskVar(task, PARSE_DOM_CURR_ATTRIB, (LPARAM far *)&lpCurrAttribStart, NULL);
		GetCustomTaskVar(task, PARSE_DOM_CURR_VALUE, (LPARAM far *)&lpCurrValueStart, NULL);
		if (lpCurrTagStart) GlobalFree((HGLOBAL)lpCurrTagStart);
		if (lpCurrAttribStart) GlobalFree((HGLOBAL)lpCurrAttribStart);
		if (lpCurrValueStart) GlobalFree((HGLOBAL)lpCurrValueStart);
	}
	
	*buff = ptr;
	
	return TRUE;
}