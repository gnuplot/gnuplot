/*
 * $Id: wgnuplib.h%v 3.50.1.13 1993/08/19 03:21:26 woo Exp $
 */

/* GNUPLOT - win/wgnuplib.h */
/*
 * Copyright (C) 1992   Russell Lang
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and 
 * that both that copyright notice and this permission notice appear 
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed 
 * as patches to released version.
 *  
 * This software is provided "as is" without express or implied warranty.
 * 
 *
 * AUTHORS
 * 
 *   Russell Lang
 * 
 * Send your comments or suggestions to 
 *  info-gnuplot@dartmouth.edu.
 * This is a mailing list; to join it send a note to 
 *  info-gnuplot-request@dartmouth.edu.  
 * Send bug reports to
 *  bug-gnuplot@dartmouth.edu.
 */

/* this file contains items to be visible outside wgnuplot.dll */

#ifdef _WINDOWS
#define _Windows
#endif

#ifdef __DLL__
#define WDPROC WINAPI _export
#else
#define WDPROC WINAPI
#endif

#define WGNUPLOTVERSION  "1.1   1993-08-14"
BOOL WDPROC CheckWGNUPLOTVersion(LPSTR str);

/* ================================== */
/* For WIN32 API's 
#ifdef WIN32
#define DEFAULT_CHARSET ANSI_CHARSET
#define OFFSETOF(x)  (x)
#define SELECTOROF(x)  (x)
#define MoveTo(hdc,x,y) MoveToEx(hdc,x,y,(LPPOINT)NULL);
#endif
 
/* ================================== */
/* wprinter.c - windows printer routines */
void WDPROC DumpPrinter(HWND hwnd, LPSTR szAppName, LPSTR szFileName);

typedef struct tagPRINT {
	HDC		hdcPrn;
	HWND	hDlgPrint;
	BOOL	bUserAbort;
	POINT	pdef;
	POINT	psize;
	POINT	poff;
	struct tagPRINT FAR *next;
} PRINT;
typedef PRINT FAR*  LPPRINT;

/* ================================== */
/* wpause.c - pause window structure */
typedef struct tagPW
{
	HINSTANCE	hInstance;		/* required */
	HINSTANCE	hPrevInstance;	/* required */
	LPSTR	Title;			/* required */
	LPSTR	Message;		/* required */
	POINT	Origin;			/* optional */
	HWND	hWndParent;		/* optional */
	HWND	hWndPause;
	HWND	hOK;
	HWND	hCancel;
	BOOL	bPause;
	BOOL	bPauseCancel;
	BOOL	bDefOK;
	WNDPROC	lpfnOK;
	WNDPROC	lpfnCancel;
	WNDPROC	lpfnPauseButtonProc;
} PW;
typedef PW FAR*  LPPW;

int WDPROC PauseBox(LPPW lppw);

/* ================================== */
/* wmenu.c - menu structure */
#define BUTTONMAX 10
typedef struct tagMW
{
	LPSTR	szMenuName;		/* required */
	HMENU	hMenu;
	BYTE FAR * FAR *macro;
	BYTE FAR *macrobuf;
	int		nCountMenu;
	DLGPROC	lpProcInput;
	char	*szPrompt;
	char	*szAnswer;
	int		nChar;
	int		nButton;
	HWND	hButton[BUTTONMAX];
	int		hButtonID[BUTTONMAX];
	WNDPROC	lpfnMenuButtonProc;
	WNDPROC	lpfnButtonProc[BUTTONMAX];
} MW;
typedef MW FAR * LPMW;

/* ================================== */
/* wtext.c text window structure */
/* If an optional item is not specified it must be zero */
#define MAXFONTNAME 80
typedef struct tagTW
{
	LPPRINT	lpr;			/* must be first */
	HINSTANCE hInstance;		/* required */
	HINSTANCE hPrevInstance;	/* required */
	LPSTR	Title;			/* required */
	LPMW	lpmw;			/* optional */
	POINT	ScreenSize;		/* optional */
	unsigned int KeyBufSize;	/* optional */
	LPSTR	IniFile;		/* optional */
	LPSTR	IniSection;		/* optional */
	LPSTR	DragPre;		/* optional */
	LPSTR	DragPost;		/* optional */
	int		nCmdShow;		/* optional */
	FARPROC shutdown;		/* optional */
	HICON	hIcon;			/* optional */
	LPSTR   AboutText;		/* optional */
	HMENU	hPopMenu;
	HWND	hWndText;
	HWND	hWndParent;
	POINT	Origin;
	POINT	Size;
	BYTE FAR *ScreenBuffer;
	BYTE FAR *AttrBuffer;
	BYTE FAR *KeyBuf;
	BYTE FAR *KeyBufIn;
	BYTE FAR *KeyBufOut;
	BYTE	Attr;
	BOOL	bFocus;
	BOOL	bGetCh;
	BOOL	bSysColors;
	HBRUSH	hbrBackground;
	char	fontname[MAXFONTNAME];	/* font name */
	int		fontsize;				/* font size in pts */
	HFONT	hfont;
	int		CharAscent;
	int		ButtonHeight;
	int		CaretHeight;
	int		CursorFlag;
	POINT	CursorPos;
	POINT	ClientSize;
	POINT	CharSize;
	POINT	ScrollPos;
	POINT	ScrollMax;
	POINT	MarkBegin;
	POINT	MarkEnd;
	BOOL	Marking;
} TW;
typedef TW FAR*  LPTW;


/* ================================== */
/* wtext.c - Text Window */
void WDPROC TextMessage(void);
int WDPROC TextInit(LPTW lptw);
void WDPROC TextClose(LPTW lptw);
void WDPROC TextToCursor(LPTW lptw);
int WDPROC  TextKBHit(LPTW);
int WDPROC TextGetCh(LPTW);
int WDPROC TextGetChE(LPTW);
LPSTR WDPROC TextGetS(LPTW lptw, LPSTR str, unsigned int size);
int WDPROC TextPutCh(LPTW, BYTE);
int WDPROC TextPutS(LPTW lptw, LPSTR str);
void WDPROC TextGotoXY(LPTW lptw, int x, int y);
int  WDPROC TextWhereX(LPTW lptw);
int  WDPROC TextWhereY(LPTW lptw);
void WDPROC TextCursorHeight(LPTW lptw, int height);
void WDPROC TextClearEOL(LPTW lptw);
void WDPROC TextClearEOS(LPTW lptw);
void WDPROC TextInsertLine(LPTW lptw);
void WDPROC TextDeleteLine(LPTW lptw);
void WDPROC TextScrollReverse(LPTW lptw);
void WDPROC TextAttr(LPTW lptw, BYTE attr);
void WDPROC AboutBox(HWND hwnd, LPSTR str);

/* ================================== */
/* wgraph.c - graphics window */

/* windows data */
#define WGNUMPENS 15

#define GWOPMAX 4096
/* GWOP is 8 bytes long. Array of GWOP kept in global block */
struct GWOP {
	WORD op;
	WORD x, y; 
	HLOCAL htext;
};

/* memory block for graph operations */
struct GWOPBLK {			/* kept in local memory */
	struct GWOPBLK *next;
	HGLOBAL hblk;			/* handle to a global block */
	struct GWOP FAR *gwop;	/* pointer to global block if locked */
	UINT used;				/* number of GWOP's used */
};

/* ops */
#define W_endoflist 0
#define W_dot 10
#define W_diamond 11
#define W_plus 12
#define W_box 13
#define W_cross 14
#define W_triangle 15
#define W_star 16
#define W_move 20
#define W_vect 21
#define W_line_type 22
#define W_put_text 23
#define W_justify 24
#define W_text_angle 25

typedef struct tagGW {
	LPPRINT	lpr;			/* must be first */
	HINSTANCE	hInstance;		/* required */
	HINSTANCE	hPrevInstance;	/* required */
	LPSTR	Title;			/* required */
	int		xmax;			/* required */
	int		ymax;			/* required */
	LPTW	lptw;		/* optional */  /* associated text window */
	POINT	Origin;		/* optional */	/* origin of graph window */
	POINT	Size;		/* optional */	/* size of graph window */
	LPSTR	IniFile;	/* optional */
	LPSTR	IniSection;	/* optional */
	HWND	hWndGraph;	/* window handle */
	HMENU	hPopMenu;	/* popup menu */
	int		numsolid;	/* number of solid pen styles */
	int		pen;		/* current pen number */
	int		htic;		/* horizontal size of point symbol (xmax units) */
	int 	vtic;		/* vertical size of point symbol (ymax units)*/
	int		hchar;		/* horizontal size of character (xmax units) */
	int		vchar;		/* vertical size of character (ymax units)*/
	int		angle;		/* text angle */
	BOOL	rotate;		/* can text be rotated 90 degrees ? */
	char	fontname[MAXFONTNAME];	/* font name */
	int		fontsize;	/* font size in pts */
	HFONT	hfonth;		/* horizonal font */
	HFONT	hfontv;		/* vertical font */
	BOOL	resized;	/* has graph window been resized? */
	BOOL	graphtotop;	/* bring graph window to top after every plot? */
	BOOL	color;					/* color pens? */
	HPEN	hbpen;					/* border pen */
	HPEN	hapen;					/* axis pen */
	HPEN	hpen[WGNUMPENS];		/* pens */
	LOGPEN	colorpen[WGNUMPENS+2];	/* logical color pens */
	LOGPEN	monopen[WGNUMPENS+2];	/* logical mono pens */
	COLORREF background;			/* background color */
	HBRUSH   hbrush;				/* background brush */
	struct GWOPBLK *gwopblk_head;
	struct GWOPBLK *gwopblk_tail;
	unsigned int nGWOP;
	BOOL	locked;				/* locked if being written */
} GW;
typedef GW FAR*  LPGW;

#define WINFONTSIZE 10
#define WIN30FONT "Courier"
#define WINFONT "Arial"

#ifndef LEFT
#define LEFT 0
#endif
#ifndef CENTRE
#define CENTRE 1
#endif
#ifndef RIGHT
#define RIGHT 2
#endif

void WDPROC GraphInit(LPGW lpgw);
void WDPROC GraphClose(LPGW lpgw);
void WDPROC GraphStart(LPGW lpgw);
void WDPROC GraphEnd(LPGW lpgw);
void WDPROC GraphOp(LPGW lpgw, WORD op, WORD x, WORD y, LPSTR str);
void WDPROC GraphPrint(LPGW lpgw);
void WDPROC GraphRedraw(LPGW lpgw);

/* ================================== */
