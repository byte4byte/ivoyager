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
typedef struct ContentWindow {
	HWND hWnd;
	RECT rcClient;
	DomNode far *document;
	ContentWindow *parent;
	Tab far *tab;
	// Parsed CSS
} ContentWindow;

typedef struct NodeList {
 DomNode *node;
} NodeList;

typedef struct DomNode {
	LPSTR szTagName;
	int type;
	ContentWindow far *contentWindow;
	AllCSSProps computedCSS;
	int childElementCount; //childElementCount: 2
	NodeList far *children; //children: HTMLCollection(2)
	NodeList far *childNodes; //childNodes: NodeList(3) [head, text, body]
	DomNode far *firstChild; //firstChild: head
	DomNode far *firstElementChild; //firstElementChild: head
	DomNode far *nextElementSibling; //nextElementSibling: null
	DomNode far *nextSibling; //nextSibling: null
	int nodeType; //nodeType: 1
	DomNode far *parentNode; //parentNode: document
	DomNode far *parentElement; //parentElement: null
	DomNode far *previousElementSibling; //previousElementSibling: null
	DomNode far *previousSibling; //previousSibling: null
	LPSTR tagName; 
	//calc on read: innerHTML: "<head><title>blah</title></head>↵<body bgcolor="#000000">↵<b><i>hi</i></b>↵<script>↵sdsd();↵alert("blah");↵</script>↵<script>↵console.log(document.childNodes);↵</script>↵</body>"
	//calc on read: innerText: "hi"
} DomNode;

BOOL ParseDOMChunk(ContentWindow far *window, Task far *task, char far **buff, int len, BOOL end);

#endif