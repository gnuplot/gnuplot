/*
 * $Id: wgnuplib.h,v 1.2.2.2 2000/10/22 13:50:51 joze Exp $
 */

/* GNUPLOT - win/wgnuplib.h */

/*[
 * Copyright 1982 - 1993, 1998   Russell Lang
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
]*/

/*
 * AUTHORS
 * 
 *   Russell Lang
 * 
 * Send your comments or suggestions to 
 *  info-gnuplot@dartmouth.edu.
 * This is a mailing list; to join it send a note to 
 *  majordomo@dartmouth.edu.  
 * Send bug reports to
 *  bug-gnuplot@dartmouth.edu.
 */

/* this file contains items to be visible outside wgnuplot.dll */

#ifdef _WINDOWS
#define _Windows
#endif

/* HBB 19990506: The following used to be #ifdef __DLL__.
 * But  _export is needed outside the DLL, as well. The long-standing
 * bug crashing 16bit wgnuplot on Alt-F4 or pressing the 'close' button
 * stemmed here.  */
#ifndef WIN32
#define WINEXPORT _export
#else
#define WINEXPORT
#endif

#define WDPROC WINAPI WINEXPORT

/* HBB: bumped version for pointsize, linewidth and PM3D implementation */
#define WGNUPLOTVERSION  "1.3   2000-08-13" 
BOOL WDPROC CheckWGNUPLOTVersion(LPSTR str);

/* ================================== */
/* symbols for the two icons          */
#define TEXTICON 123
#define GRPICON 124

/* ================================== */
/* For WIN32 API's */
#ifdef WIN32
/* #define DEFAULT_CHARSET ANSI_CHARSET */
#define OFFSETOF(x)  (x)
#define SELECTOROF(x)  (x)
#define MoveTo(hdc,x,y) MoveToEx(hdc,x,y,(LPPOINT)NULL);
# ifndef __TURBOC__ /* Borland C has these defines, already... */
#define farmalloc(x) malloc(x)
#define farrealloc(s,n) realloc(s,n)
#define farfree(s) free(s)
# endif /* __TURBOC__ */
#endif
 
#ifdef __MINGW32__
/* HBB 980809: MinGW32 doesn't define some of the more traditional
 * things gnuplot expects in every Windows C compiler, it seems: */
/* HBB 20000813: This has change, in the meantime. I'm taking some of these
 * out again: */
/* typedef LOGPEN *LPLOGPEN; */
/* typedef HGLOBAL GLOBALHANDLE; */
/* #define WINVER 0x0400 */
/* #define HFILE_ERROR ((HFILE)-1)*/

/* the far mem/string function family: */
#define _fstrstr(s1,s2) (strstr(s1,s2))
#define _fstrchr(s,c) (strchr(s,c))
#define _fstrrchr(s,c) (strrchr(s,c))
#define _fstrlen(s) (strlen(s))
#define _fstrcpy(d,s) (strcpy(d,s))
#define _fstrncpy(d,s,n) (strncpy(d,s,n))
#define _fstrcat(s1,s2) (strcat(s1,s2))
/* #define _fmemset(s,c,n) (memset(s,c,n)) */
/* #define _fmemmove(d,s,n) (memmove(d,s,n)) */

#endif /* __MINGW32__ */
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
#define W_circle 17
#define W_fcircle 18
#define W_fbox 19
#define W_fdiamond 20
#define W_fitriangle 21
#define W_itriangle 22
#define W_move 30
#define W_vect 31
#define W_line_type 32
#define W_put_text 33
#define W_justify 34
#define W_text_angle 35
#define W_pointsize 36
#define W_line_width 37
#define W_pm3d_setcolor 38
#define W_pm3d_filled_polygon 39

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
#if 1 /* HBB 980118: new try ... */
        HPEN    hsolidpen[WGNUMPENS];           /* solid pens (for point symbols) */
#endif
	LOGPEN	colorpen[WGNUMPENS+2];	/* logical color pens */
	LOGPEN	monopen[WGNUMPENS+2];	/* logical mono pens */
	COLORREF background;			/* background color */
	HBRUSH   hbrush;				/* background brush */
	HBRUSH   colorbrush[WGNUMPENS+2];   /* brushes to fill points */
	struct GWOPBLK *gwopblk_head;
	struct GWOPBLK *gwopblk_tail;
	unsigned int nGWOP;
	BOOL	locked;				/* locked if being written */
	double  org_pointsize;		/* Original Pointsize */
} GW;
typedef GW FAR*  LPGW;

#define WINFONTSIZE 10
#define WIN30FONT "Courier"
#define WINFONT "Arial"

#if 0
enum JUSTIFY {
	LEFT, CENTRE, RIGHT
};
#endif

void WDPROC GraphInit(LPGW lpgw);
void WDPROC GraphClose(LPGW lpgw);
void WDPROC GraphStart(LPGW lpgw, double pointsize);
void WDPROC GraphEnd(LPGW lpgw);
void WDPROC GraphResume(LPGW lpgw);
void WDPROC GraphOp(LPGW lpgw, WORD op, WORD x, WORD y, LPSTR str);
void WDPROC GraphPrint(LPGW lpgw);
void WDPROC GraphRedraw(LPGW lpgw);

/* ================================== */
