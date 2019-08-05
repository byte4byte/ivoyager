#ifndef _DOM_H
#define _DOM_H

typedef int CSSPropType;
typedef int CSSUnit;

typedef struct CSSValue {
	LPSTR szValue;
	int iValue;
	float fValue;
	CSSUnit unit;
} CSSValue;

/* Should be the index in AllCSSProps */
#define CSS_BACKGROUNDATTACHMENT	0
#define CSS_BACKGROUNDCLIP			1

typedef struct CSSProp {
	CSSPropType type;
	CSSValue value;
} CSSProp;

typedef struct AllCSSProps {
	CSSProp backgroundAttachment;
	CSSProp backgroundClip;
	CSSProp backgroundColor;
	CSSProp backgroundImage;
	CSSProp backgroundOrigin;
	CSSProp backgroundPositionX;
	CSSProp backgroundPositionY;
	CSSProp backgroundRepeatX;
	CSSProp backgroundRepeatY;
	CSSProp backgroundSize;
	CSSProp borderBottomLeftRadius;
	CSSProp borderBottomRightRadius;
	CSSProp borderTopLeftRadius;
	CSSProp borderTopRightRadius;
	CSSProp boxShadow;
	CSSProp boxSizing;
	CSSProp color;
	CSSProp cursor;
	CSSProp display;
	CSSProp fontFamily;
	CSSProp fontSize;
	CSSProp fontWeight;
	CSSProp height;
	CSSProp marginBottom;
	CSSProp marginLeft;
	CSSProp marginRight;
	CSSProp marginTop;
	CSSProp opacity;
	CSSProp overflowX;
	CSSProp overflowY;
	CSSProp position;
	CSSProp textAlign;
	CSSProp textShadow;
	CSSProp transform;
	CSSProp transitionDelay;
	CSSProp transitionDuration;
	CSSProp transitionProperty;
	CSSProp transitionTimingFunction;
	CSSProp userSelect;
	CSSProp visibility;
	CSSProp whiteSpace;
	CSSProp width;
	CSSProp zoom;
} AllCSSProps;

typedef struct DomNode DomNode;
typedef struct ContentWindow ContentWindow;
typedef struct ContentWindow {
	HWND hWnd;
	RECT rcClient;
	DomNode *rootNode;
	ContentWindow *parent;
	// Parsed CSS
} ContentWindow;

typedef struct DomNode {
	LPSTR szTagName;
	int type;
	ContentWindow contentWindow;
	AllCSSProps computedCSS;
	DomNode *parent;
	DomNode *firstChild;
	DomNode *next;
} DomNode;

BOOL  ParseDOMChunk(ContentWindow  *window, char  *buff, int len);

#endif