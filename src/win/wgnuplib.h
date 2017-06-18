/*
 * $Id: wgnuplib.h,v 1.89.2.2 2017/06/17 20:05:43 markisch Exp $
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

/* printf format for TCHAR arguments */ 
#ifdef UNICODE
# define TCHARFMT "%ls"
#else
# define TCHARFMT "%hs"
#endif

/* ================================== */
/* wprinter.c - windows printer routines */
void DumpPrinter(HWND hwnd, LPTSTR szAppName, LPTSTR szFileName);


typedef struct tagPRINT {
	HDC	hdcPrn;
	HWND	hDlgPrint;
	BOOL	bUserAbort;
	LPCTSTR	szTitle;
	POINT	pdef;
	POINT	psize;
	POINT	poff;
	BOOL	bDriverChanged;
	void *	services;
	struct tagPRINT *next;
} GP_PRINT;
typedef GP_PRINT *  GP_LPPRINT;


/* ================================== */
/* wpause.c - pause window structure */
typedef struct tagPW
{
	HINSTANCE	hInstance;	/* required */
	HINSTANCE	hPrevInstance;	/* required */
	LPWSTR	Title;			/* required */
	LPWSTR	Message;		/* required */
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
	LPTSTR	szMenuName;		/* required */
	HMENU	hMenu;
	BYTE	**macro;
	BYTE	*macrobuf;
	int	nCountMenu;
	DLGPROC	lpProcInput;
	LPWSTR	szPrompt;
	LPWSTR	szAnswer;
	int	nChar;
	int	nButton;
	HWND	hToolbar;
	HWND	hButton[BUTTONMAX];
	int	hButtonID[BUTTONMAX];
} MW;
typedef MW * LPMW;


/* ================================== */
/* wtext.c text window structure */
/* If an optional item is not specified it must be zero */
#define MAXFONTNAME 80
typedef struct tagTW
{
	GP_LPPRINT	lpr;		/* must be first */
	HINSTANCE hInstance;		/* required */
	HINSTANCE hPrevInstance;	/* required */
	LPWSTR	Title;			/* required */
	LPMW	lpmw;			/* optional */
	POINT	ScreenSize;		/* optional */  /* size of the visible screen in characters */
	unsigned int KeyBufSize;	/* optional */
	LPTSTR	IniFile;		/* optional */
	LPTSTR	IniSection;		/* optional */
	LPWSTR	DragPre;		/* optional */
	LPWSTR	DragPost;		/* optional */
	int	nCmdShow;		/* optional */
	FARPROC shutdown;		/* optional */
	HICON	hIcon;			/* optional */
	LPTSTR	AboutText;		/* optional */
	HMENU	hPopMenu;
	HWND	hWndText;
	HWND	hWndParent;
	HWND	hWndToolbar;
	HWND	hStatusbar;
	HWND	hWndSeparator;
	HWND	hWndFocus;		/* window with input focus */
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
	TCHAR	fontname[MAXFONTNAME];	/* font name */
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
	int	bSuspend;
	int	MaxCursorPos;
	/* variables for docked graphs */
	UINT	nDocked;
	UINT	VertFracDock;
	UINT	HorzFracDock;
	UINT	nDockCols;
	UINT	nDockRows;
	UINT	SeparatorWidth;
	COLORREF	SeparatorColor;
	BOOL	bFracChanging;
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

void AboutBox(HWND hwnd, LPTSTR str);

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

/* point types */
enum win_pointtypes {
	W_invalid_pointtype = 0,
	W_dot = 10,
	W_plus, W_cross, W_star, 
	W_box, W_fbox,
	W_circle, W_fcircle, 
	W_itriangle, W_fitriangle, 
	W_triangle, W_ftriangle,
	W_diamond, W_fdiamond,
	W_pentagon, W_fpentagon,
	W_last_pointtype = W_fpentagon
};
// The dot is reserved for pt 0, number of (remaining) point types:
#define WIN_POINT_TYPES (W_last_pointtype - W_plus + 1)

/* ops */
enum win_draw_commands {
	W_endoflist = 0,
	W_point = 9, 
	W_pointsize = 30,
	W_setcolor,
	W_polyline, W_line_type, W_dash_type, W_line_width,
	W_put_text, W_enhanced_text, W_boxedtext,
	 W_text_encoding, W_font, W_justify, W_text_angle,
	W_filled_polygon_draw, W_filled_polygon_pt,
	W_fillstyle,
	W_move, W_boxfill,
	W_image,
	W_layer,
	W_hypertext
};


typedef struct tagGW {
	GP_LPPRINT	lpr;	/* must be first */
	HINSTANCE hInstance;	/* required */
	HINSTANCE hPrevInstance;/* required */
	int	Id;		/* plot number */
	LPTSTR	Title;		/* required */
	int	xmax;		/* required */
	int	ymax;		/* required */
	LPTW	lptw;		/* optional */  /* associated text window */
	BOOL	bDocked;	/* is the graph docked to the text window? */
	POINT	Origin;		/* optional */	/* origin of graph window */
	POINT	Size;		/* optional */	/* size of graph window */
	LPTSTR	IniFile;	/* optional */
	LPTSTR	IniSection;	/* optional */
	HWND	hWndGraph;	/* window handle */
	HWND	hStatusbar;	/* window handle of status bar */
	int	StatusHeight;	/* height of status line area */
	HWND	hToolbar;
	int	ToolbarHeight;
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
	BOOL	oversample;	/* oversampling? */
	BOOL	gdiplus;	/* Use GDI+ only backend? */
	BOOL	d2d;
	BOOL	antialiasing;	/* anti-aliasing? */
	BOOL	polyaa;		/* anti-aliasing for polygons ? */
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

	int	htic;		/* horizontal size of point symbol (xmax units) */
	int 	vtic;		/* vertical size of point symbol (ymax units)*/
	int	hchar;		/* horizontal size of character (xmax units) */
	int	vchar;		/* vertical size of character (ymax units)*/

	TCHAR	fontname[MAXFONTNAME];	/* current font name */
	int	fontsize;	/* current font size in pts */
	TCHAR	deffontname[MAXFONTNAME]; /* default font name */
	int	deffontsize;	/* default font size */
	int	angle;		/* text angle */
	BOOL	rotate;		/* can text be rotated 90 degrees ? */
	int	justify;	/* text justification */
	HFONT	hfonth;		/* horizonal font */
	HFONT	hfontv;		/* rotated font (arbitrary angle) */
	LOGFONT	lf;			/* cached to speed up rotated fonts */
	double	org_pointsize;	/* Original Pointsize */
	int	encoding_error; /* last unknown encoding */
	double	fontscale;	/* scale factor for font sizes */
	double	pointscale;	/* scale factor for point sizes */
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
	COLORREF background;	/* background color */
	HBRUSH	hbrush;		/* background brush */
	HBRUSH	hcolorbrush;	/* */

	struct tagGW * next;	/* pointer to next window */
} GW;
typedef GW *  LPGW;


typedef struct {
	LPGW lpgw;           /* graph window */
	LPRECT rect;         /* rect to update */
	BOOL opened_string;  /* started processing of substring? */
	BOOL show;           /* print this substring? */
	int overprint;       /* overprint flag */
	BOOL widthflag;      /* FALSE for zero width boxes */
	BOOL sizeonly;       /* only measure length of substring? */
	double base;         /* current baseline position (above initial baseline) */
	int xsave, ysave;    /* save text position for overprinted text */
	int x, y;            /* current text position */
	TCHAR fontname[MAXFONTNAME]; /* current font name */
	double fontsize;     /* current font size */
	int totalwidth;      /* total width of printed text */
	int totalasc;        /* total height above center line */
	int totaldesc;       /* total height below center line */
	double res_scale;    /* scaling due to different resolution (printers) */
	int shift;           /* baseline alignment */
	void (* set_font)();
	unsigned (* text_length)(char *);
	void (* put_text)(int , int, char *);
	void (* cleanup)();
} enhstate_struct;
extern enhstate_struct enhstate;


/* No TEXT Macro required for fonts */
#define WINFONTSIZE 10
#ifndef WINFONT
# define WINFONT "Tahoma"
#endif
#ifndef WINJPFONT
# define WINJPFONT "MS PGothic"
#endif

#define WINGRAPHTITLE TEXT("gnuplot graph")

extern termentry * WIN_term;
extern TCHAR WIN_inifontname[MAXFONTNAME];
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
LPTSTR GraphDefaultFont(void);

#ifdef USE_MOUSE
void Graph_set_cursor(LPGW lpgw, int c, int x, int y);
void Graph_set_ruler(LPGW lpgw, int x, int y);
void Graph_put_tmptext(LPGW lpgw, int i, LPCSTR str);
void Graph_set_clipboard(LPGW lpgw, LPCSTR s);
#endif

/* BM: callback functions for enhanced text */
void GraphEnhancedOpen(char *fontname, double fontsize, double base,
    TBOOLEAN widthflag, TBOOLEAN showflag, int overprint);
void GraphEnhancedFlush(void);

void WIN_update_options __PROTO((void));


/* ================================== */

#ifdef __cplusplus
}
#endif

#endif
