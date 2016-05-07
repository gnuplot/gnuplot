/*
 * $Id: wgnuplib.h,v 1.73 2016/05/07 09:26:36 markisch Exp $
 */

/* GNUPLOT - win/wgnuplib.h */

/*[
 * Copyright 1982 - 1993, 1998, 2004   Russell Lang
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
 */

#ifndef WGNUPLIB_H
#define WGNUPLIB_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "screenbuf.h"
#include "term_api.h"

/* ================================== */
/* symbols for the two icons          */
#define TEXTICON 123
#define GRPICON 124

/* ================================== */
/* For WIN32 API's */
/* #define DEFAULT_CHARSET ANSI_CHARSET */
# define MoveTo(hdc,x,y) MoveToEx(hdc,x,y,(LPPOINT)NULL);

/* ================================== */
/* wprinter.c - windows printer routines */
void DumpPrinter(HWND hwnd, LPSTR szAppName, LPSTR szFileName);


typedef struct tagPRINT {
	HDC		hdcPrn;
	HWND	hDlgPrint;
	BOOL	bUserAbort;
	POINT	pdef;
	POINT	psize;
	POINT	poff;
	struct tagPRINT *next;
} GP_PRINT;
typedef GP_PRINT *  GP_LPPRINT;


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
typedef PW *  LPPW;

TBOOLEAN MousableWindowOpened(void);
int PauseBox(LPPW lppw);

/* ================================== */
/* wmenu.c - menu structure */
#define BUTTONMAX 10
typedef struct tagMW
{
	LPSTR	szMenuName;		/* required */
	HMENU	hMenu;
	BYTE	**macro;
	BYTE	*macrobuf;
	int		nCountMenu;
	DLGPROC	lpProcInput;
	char	*szPrompt;
	char	*szAnswer;
	int		nChar;
	int		nButton;
	HWND	hToolbar;
	HWND	hButton[BUTTONMAX];
	int		hButtonID[BUTTONMAX];
} MW;
typedef MW * LPMW;


/* ================================== */
/* wtext.c text window structure */
/* If an optional item is not specified it must be zero */
#define MAXFONTNAME 80
typedef struct tagTW
{
	GP_LPPRINT	lpr;			/* must be first */
	HINSTANCE hInstance;		/* required */
	HINSTANCE hPrevInstance;	/* required */
	LPWSTR	Title;			/* required */
	LPMW	lpmw;			/* optional */
	POINT	ScreenSize;		/* optional */  /* size of the visible screen in characters */
	unsigned int KeyBufSize;	/* optional */
	LPSTR	IniFile;		/* optional */
	LPSTR	IniSection;		/* optional */
	LPWSTR	DragPre;		/* optional */
	LPWSTR	DragPost;		/* optional */
	int	nCmdShow;		/* optional */
	FARPROC shutdown;		/* optional */
	HICON	hIcon;			/* optional */
	LPSTR   AboutText;		/* optional */
	HMENU	hPopMenu;
	HWND	hWndText;
	HWND	hWndParent;
	HWND	hStatusbar;
	POINT	Origin;
	POINT	Size;
	SB	ScreenBuffer;
	BOOL	bWrap;			/* wrap long lines */
	BYTE	*KeyBuf;
	BYTE	*KeyBufIn;
	BYTE	*KeyBufOut;
	BYTE	Attr;
	BOOL	bFocus;
	BOOL	bGetCh;
	BOOL	bSysColors;
	HBRUSH	hbrBackground;
	char	fontname[MAXFONTNAME];	/* font name */
	int	fontsize;		/* font size in pts */
	HFONT	hfont;
	int	CharAscent;
	int	ButtonHeight;
	int	StatusHeight;
	int	CaretHeight;
	int	CursorFlag;		/* scroll to cursor after \n & \r */
	POINT	CursorPos;		/* cursor position on screen */
	POINT	ClientSize;		/* size of the client window in pixels */
	POINT	CharSize;
	POINT	ScrollPos;
	POINT	ScrollMax;
	POINT	MarkBegin;
	POINT	MarkEnd;
	BOOL	Marking;
} TW;
typedef TW *  LPTW;


#ifndef WGP_CONSOLE
/* ================================== */
/* wtext.c - Text Window */
void TextMessage(void);
int TextInit(LPTW lptw);
void TextClose(LPTW lptw);
int TextKBHit(LPTW);
int TextGetCh(LPTW);
int TextGetChE(LPTW);
LPSTR TextGetS(LPTW lptw, LPSTR str, unsigned int size);
int TextPutCh(LPTW, BYTE);
int TextPutChW(LPTW lptw, WCHAR ch);
int TextPutS(LPTW lptw, LPSTR str);
void TextStartEditing(LPTW lptw);
void TextStopEditing(LPTW lptw);
#if 0
/* The new screen buffer currently does not support these */
void TextGotoXY(LPTW lptw, int x, int y);
int  TextWhereX(LPTW lptw);
int  TextWhereY(LPTW lptw);
void TextCursorHeight(LPTW lptw, int height);
void TextClearEOL(LPTW lptw);
void TextClearEOS(LPTW lptw);
void TextInsertLine(LPTW lptw);
void TextDeleteLine(LPTW lptw);
void TextScrollReverse(LPTW lptw);
#endif
void TextAttr(LPTW lptw, BYTE attr);
#endif /* WGP_CONSOLE */

void AboutBox(HWND hwnd, LPSTR str);

/* ================================== */
/* wgraph.c - graphics window */

/* windows data */

/* number of different 'basic' pens supported (the ones you can modify
 * by the 'Line styles...' dialog, and save to/from wgnuplot.ini). */
#define WGNUMPENS 15

/* maximum number of different colors per palette, used to be hardcoded (256) */
#define WIN_PAL_COLORS 4096

/* hypertext structure
*/
struct tooltips {
	LPWSTR text;
	RECT rect;
};

/* Information about one graphical operation to be stored by the
 * driver for the sake of redraws. Array of GWOP kept in global block */
struct GWOP {
	UINT op;
	UINT x, y;
	HLOCAL htext;
};

/* memory block for graph operations */
struct GWOPBLK {			/* kept in local memory */
	struct GWOPBLK *next;
	HGLOBAL hblk;			/* handle to a global block */
	struct GWOP *gwop;	/* pointer to global block if locked */
	UINT used;				/* number of GWOP's used */
};

/* ops */
#define W_endoflist 0

#define WIN_POINT_TYPES 15	/* required by win.trm */
#define W_dot 10
#define W_plus 11
#define W_cross 12
#define W_star 13
#define W_box 14
#define W_fbox 15
#define W_circle 16
#define W_fcircle 17
#define W_itriangle 18
#define W_fitriangle 19
#define W_triangle 20
#define W_ftriangle 21
#define W_diamond 22
#define W_fdiamond 23
#define W_pentagon 24
#define W_fpentagon 25

#define W_move 30
#define W_vect 31
#define W_line_type 32
#define W_put_text 33
#define W_justify 34
#define W_text_angle 35
#define W_pointsize 36
#define W_line_width 37
#define W_setcolor 38
#define W_filled_polygon_pt   39
#define W_filled_polygon_draw 40
#define W_boxfill 41
#define W_fillstyle 42
#define W_font 43
#define W_enhanced_text 44
#define W_image 45
#define W_layer 46
#define W_text_encoding 47
#define W_hypertext 48
#define W_boxedtext 49
#define W_dash_type 50


typedef struct tagGW {
	GP_LPPRINT	lpr;		/* must be first */
	HINSTANCE hInstance;	/* required */
	HINSTANCE hPrevInstance;	/* required */
	int		Id;	/* plot number */
	LPSTR	Title;		/* required */
	int		xmax;		/* required */
	int		ymax;		/* required */
	LPTW	lptw;		/* optional */  /* associated text window */
	POINT	Origin;		/* optional */	/* origin of graph window */
	POINT	Size;		/* optional */	/* size of graph window */
	LPSTR	IniFile;	/* optional */
	LPSTR	IniSection;	/* optional */
	HWND	hWndGraph;	/* window handle */
	HWND	hStatusbar;	/* window handle of status bar */
	int		StatusHeight;	/* height of status line area */
	HWND	hToolbar;
	int		ToolbarHeight;
	HMENU	hPopMenu;	/* popup menu */
	HBITMAP	hBitmap;	/* bitmap of current graph */
	BOOL	buffervalid;	/* indicates if hBitmap is valid */
	BOOL	rotating;	/* are we currently rotating the graph? */
	POINT	Canvas;		/* requested size of the canvas */
	POINT	Decoration;	/* extent of the window decorations */

	struct GWOPBLK *gwopblk_head;
	struct GWOPBLK *gwopblk_tail;
	unsigned int nGWOP;
	BOOL	locked;		/* locked if being written */

	BOOL	initialized;	/* already initialized? */
	BOOL	graphtotop;	/* bring graph window to top after every plot? */
	BOOL	color;		/* color pens? */
	BOOL	dashed;		/* dashed lines? */
	BOOL	rounded;	/* rounded line caps and joins? */
	BOOL	doublebuffer;	/* double buffering? */
	BOOL	oversample;	/* oversampling? */
	BOOL	gdiplus;	/* Use GDI+ only backend? */
	BOOL	antialiasing;	/* anti-aliasing? */
	BOOL	polyaa;		/* anti-aliasing for polygons ? */
	BOOL	patternaa;	/* anti-aliasing for polygons ? */
	BOOL	fastrotation;	/* rotate without anti-aliasing? */

	BOOL	*hideplot;
	unsigned int maxhideplots;
	BOOL	hidegrid;
	unsigned int numplots;
	BOOL	hasgrid;
	LPRECT	keyboxes;
	unsigned int maxkeyboxes;

	HWND	hTooltip;	/* tooltip windows for hypertext */
	struct tooltips * tooltips;
	unsigned int maxtooltips;
	unsigned int numtooltips;

	int		htic;		/* horizontal size of point symbol (xmax units) */
	int 	vtic;		/* vertical size of point symbol (ymax units)*/
	int		hchar;		/* horizontal size of character (xmax units) */
	int		vchar;		/* vertical size of character (ymax units)*/

	char	fontname[MAXFONTNAME];	/* current font name */
	int		fontsize;	/* current font size in pts */
	char	deffontname[MAXFONTNAME]; /* default font name */
	int		deffontsize;	/* default font size */
	int		angle;		/* text angle */
	BOOL	rotate;		/* can text be rotated 90 degrees ? */
	int		justify;	/* text justification */
	HFONT	hfonth;		/* horizonal font */
	HFONT	hfontv;		/* rotated font (arbitrary angle) */
	LOGFONT	lf;			/* cached to speed up rotated fonts */
	double	org_pointsize;	/* Original Pointsize */
	int		encoding_error; /* last unknown encoding */
	double	fontscale;	/* scale factor for font sizes */
	enum set_encoding_id encoding;	/* text encoding */
	LONG	tmHeight;	/* text metric of current font */
	LONG	tmAscent;
	LONG	tmDescent;

	HPEN	hapen;		/* stored current pen */
	HPEN	hsolid;		/* solid sibling of current pen */
	HPEN	hnull;		/* empty null pen */
	LOGPEN	colorpen[WGNUMPENS+2];	/* color pen definitions */
	LOGPEN	monopen[WGNUMPENS+2];	/* mono pen definitions */
	double	linewidth;	/* scale factor for linewidth */

	HBRUSH	colorbrush[WGNUMPENS+2];   /* brushes to fill points */
	COLORREF background;		/* background color */
	HBRUSH	hbrush;				/* background brush */
	HBRUSH	hcolorbrush;		/* */
	int		sampling;			/* current sampling factor */

	struct tagGW * next;		/* pointer to next window */
} GW;
typedef GW *  LPGW;

#define WINFONTSIZE 10
#define WINFONT "Tahoma"
#ifndef WINJPFONT
#define WINJPFONT "MS PGothic"
#endif
#define WINGRAPHTITLE "gnuplot graph"

extern termentry * WIN_term;
extern char WIN_inifontname[MAXFONTNAME];
extern int WIN_inifontsize;

void GraphInitStruct(LPGW lpgw);
void GraphInit(LPGW lpgw);
void GraphUpdateWindowPosSize(LPGW lpgw);
void GraphClose(LPGW lpgw);
void GraphStart(LPGW lpgw, double pointsize);
void GraphEnd(LPGW lpgw);
void GraphChangeTitle(LPGW lpgw);
void GraphResume(LPGW lpgw);
void GraphOp(LPGW lpgw, UINT op, UINT x, UINT y, LPCSTR str);
void GraphOpSize(LPGW lpgw, UINT op, UINT x, UINT y, LPCSTR str, DWORD size);
void GraphPrint(LPGW lpgw);
void GraphRedraw(LPGW lpgw);
void GraphModifyPlots(LPGW lpgw, unsigned int operations, int plotno);
void win_close_terminal_window(LPGW lpgw);
TBOOLEAN GraphHasWindow(LPGW lpgw);
char * GraphDefaultFont(void);

#ifdef USE_MOUSE
void Graph_set_cursor(LPGW lpgw, int c, int x, int y);
void Graph_set_ruler(LPGW lpgw, int x, int y);
void Graph_put_tmptext(LPGW lpgw, int i, LPCSTR str);
void Graph_set_clipboard(LPGW lpgw, LPCSTR s);
#endif

/* BM: callback functions for enhanced text */
void GraphEnhancedOpen(char *fontname, double fontsize, double base,
    BOOL widthflag, BOOL showflag, int overprint);
void GraphEnhancedFlush(void);

void WIN_update_options __PROTO((void));


/* ================================== */

#ifdef __cplusplus
}
#endif

#endif
