#ifndef lint
static char *RCSid() { return RCSid("$Id: wgraph.c,v 1.67.2.9 2010/02/16 07:02:45 mikulik Exp $"); }
#endif

/* GNUPLOT - win/wgraph.c */
/*[
 * Copyright 1992, 1993, 1998, 2004   Maurice Castro, Russell Lang
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
 *   Maurice Castro
 *   Russell Lang
 */

#define STRICT
#include <windows.h>
#include <windowsx.h>
#if WINVER >= 0x030a
#  include <commdlg.h>
#endif
#ifndef __MSC__
# include <mem.h>
#endif
#include <stdio.h>
#include <string.h>
#include "wgnuplib.h"
#include "wresourc.h"
#include "wcommon.h"
#include "term_api.h"         /* for enum JUSTIFY */
#ifdef USE_MOUSE
# include "gpexecute.h"
# include "mouse.h"
#endif
# include "color.h"
# include "getcolor.h"

#ifdef USE_MOUSE
/* Petr Mikulik, February 2001
 * Declarations similar to src/os2/gclient.c -- see section
 * "PM: Now variables for mouse" there in.
 */

/* If wgnuplot crashes during redrawing and mouse on, then this could help: */
/* static char lock_mouse = 1; */

/* Status of the ruler */
static struct Ruler {
    TBOOLEAN on;		/* ruler active ? */
    int x, y;			/* ruler position */
} ruler = {FALSE,0,0,};

/* Status of the line from ruler to cursor */
static struct RulerLineTo {
    TBOOLEAN on;		/* ruler line active ? */
    int x, y;			/* ruler line end position (previous cursor position) */
} ruler_lineto = {FALSE,0,0,};

/* Status of zoom box */
static struct Zoombox {
    TBOOLEAN on;		/* set to TRUE during zooming */
    POINT from, to;		/* corners of the zoom box */
    LPCSTR text1, text2;	/* texts in the corners (i.e. positions) */
} zoombox = { FALSE, {0,0}, {0,0}, NULL, NULL };

/* Pointer definitions */
HCURSOR hptrDefault, hptrCrossHair, hptrScaling, hptrRotating, hptrZooming, hptrCurrent;

/* Mouse support routines */
static void	Wnd_exec_event(LPGW lpgw, LPARAM lparam, char type, int par1);
static void	Wnd_refresh_zoombox(LPGW lpgw, LPARAM lParam);
static void	Wnd_refresh_ruler_lineto(LPGW lpgw, LPARAM lParam);
static void     GetMousePosViewport(LPGW lpgw, int *mx, int *my);
static void	Draw_XOR_Text(LPGW lpgw, const char *text, size_t length, int x, int y);
static void     DisplayStatusLine(LPGW lpgw);
static void     UpdateStatusLine(LPGW lpgw, const char text[]);
static void	DrawRuler(LPGW lpgw);
static void	DrawRulerLineTo(LPGW lpgw);
static void     DrawZoomBox(LPGW lpgw);
static void	LoadCursors(LPGW lpgw);
static void	DestroyCursors(LPGW lpgw);
#endif /* USE_MOUSE */

/* ================================== */

#define MAXSTR 255

#define WGDEFCOLOR 15
COLORREF wginitcolor[WGDEFCOLOR] =  {
	RGB(255,0,0),	/* red */
	RGB(0,255,0),	/* green */
	RGB(0,0,255),	/* blue */
	RGB(255,0,255), /* magenta */
	RGB(0,0,128),	/* dark blue */
	RGB(128,0,0),	/* dark red */
	RGB(0,128,128),	/* dark cyan */
	RGB(0,0,0),	/* black */
	RGB(128,128,128), /* grey */
	RGB(0,128,64),	/* very dark cyan */
	RGB(128,128,0), /* dark yellow */
	RGB(128,0,128),	/* dark magenta */
	RGB(192,192,192), /* light grey */
	RGB(0,255,255),	/* cyan */
	RGB(255,255,0),	/* yellow */
};
#define WGDEFSTYLE 5
int wginitstyle[WGDEFSTYLE] = {PS_SOLID, PS_DASH, PS_DOT, PS_DASHDOT, PS_DASHDOTDOT};

/* Maximum number of GWOPBLK arrays to be remembered. */
/* HBB 20010218: moved here from wgnuplib.h: other parts of the program don't
 * need to know about it */
#define GWOPMAX 4096


/* bitmaps for filled boxes (ULIG) */
/* zeros represent the foreground color and ones represent the background color */
/* FIXME HBB 20010916: *never* extern in a C source! */
/*  extern int filldensity; */
/*  extern int fillpattern; */

static unsigned char halftone_bitmaps[][16] ={
  { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },   /* no fill */
  { 0xEE, 0xEE, 0xBB, 0xBB, 0xEE, 0xEE, 0xBB, 0xBB,
    0xEE, 0xEE, 0xBB, 0xBB, 0xEE, 0xEE, 0xBB, 0xBB },   /* 25% pattern */
  { 0xAA, 0xAA, 0x55, 0x55, 0xAA, 0xAA, 0x55, 0x55,
    0xAA, 0xAA, 0x55, 0x55, 0xAA, 0xAA, 0x55, 0x55 },   /* 50% pattern */
  { 0x88, 0x88, 0x22, 0x22, 0x88, 0x88, 0x22, 0x22,
    0x88, 0x88, 0x22, 0x22, 0x88, 0x88, 0x22, 0x22 },   /* 75% pattern */
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }    /* solid pattern */
};
#define halftone_num (sizeof(halftone_bitmaps) / sizeof (*halftone_bitmaps))
static HBRUSH halftone_brush[halftone_num];
static BITMAP halftone_bitdata[halftone_num];
static HBITMAP halftone_bitmap[halftone_num];

static unsigned char pattern_bitmaps[][16] = {
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, /* no fill */
  {0xFE, 0xFE, 0x7D, 0x7D, 0xBB, 0xBB, 0xD7, 0xD7,
   0xEF, 0xEF, 0xD7, 0xD7, 0xBB, 0xBB, 0x7D, 0x7D}, /* cross-hatch (1) */
  {0x77, 0x77, 0xAA, 0xBB, 0xDD, 0xDD, 0xAA, 0xBB,
   0x77, 0x77, 0xAA, 0xBB, 0xDD, 0xDD, 0xAA, 0xBB}, /* double cross-hatch (2) */
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* solid fill (3) */
  {0x7F, 0x7F, 0xBF, 0xBF, 0xDF, 0xDF, 0xEF, 0xEF,
   0xF7, 0xF7, 0xFB, 0xFB, 0xFD, 0xFD, 0xFE, 0xFE}, /* diagonals (4) */
  {0xFE, 0xFE, 0xFD, 0xFD, 0xFB, 0xFB, 0xF7, 0xF7,
   0xEF, 0xEF, 0xDF, 0xDF, 0xBF, 0xBF, 0x7F, 0x7F}, /* diagonals (5) */
  {0x77, 0x77, 0x77, 0x77, 0xBB, 0xBB, 0xBB, 0xBB,
   0xDD, 0xDD, 0xDD, 0xDD, 0xEE, 0xEE, 0xEE, 0xEE}, /* steep diagonals (6) */
  {0xEE, 0xEE, 0xEE, 0xEE, 0xDD, 0xDD, 0xDD, 0xDD,
   0xBB, 0xBB, 0xBB, 0xBB, 0x77, 0x77, 0x77, 0x77}  /* steep diagonals (7) */
#if (0)
 ,{0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0x3F, 0x3F,
   0xFC, 0xFC, 0xF3, 0xF3, 0xCF, 0xCF, 0x3F, 0x3F}, /* shallow diagonals (old 5) */
  {0x3F, 0x3F, 0xCF, 0xCF, 0xF3, 0xF3, 0xFC, 0xFC,
   0x3F, 0x3F, 0xCF, 0xCF, 0xF3, 0xF3, 0xFC, 0xFC}  /* shallow diagonals (old 6) */
#endif
};

#define pattern_num (sizeof(pattern_bitmaps)/(sizeof(*pattern_bitmaps)))
static HBRUSH pattern_brush[pattern_num];
static BITMAP pattern_bitdata[pattern_num];
static HBITMAP pattern_bitmap[pattern_num];

static TBOOLEAN brushes_initialized = FALSE;


/* ================================== */

/* prototypes for module-local functions */

LRESULT CALLBACK WINEXPORT WndGraphProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WINEXPORT LineStyleDlgProc(HWND hdlg, UINT wmsg, WPARAM wparam, LPARAM lparam);

static void	DestroyBlocks(LPGW lpgw);
static BOOL	AddBlock(LPGW lpgw);
static void	StorePen(LPGW lpgw, int i, COLORREF ref, int colorstyle, int monostyle);
static void	MakePens(LPGW lpgw, HDC hdc);
static void	DestroyPens(LPGW lpgw);
static void	Wnd_GetTextSize(HDC hdc, LPCSTR str, size_t len, int *cx, int *cy);
static void	MakeFonts(LPGW lpgw, LPRECT lprect, HDC hdc);
static void	DestroyFonts(LPGW lpgw);
static void	SetFont(LPGW lpgw, HDC hdc);
static void	SelFont(LPGW lpgw);
static void	dot(HDC hdc, int xdash, int ydash);
static void	drawgraph(LPGW lpgw, HDC hdc, LPRECT rect);
static void	CopyClip(LPGW lpgw);
static void	SaveAsEMF(LPGW lpgw);
static void	CopyPrint(LPGW lpgw);
static void	WriteGraphIni(LPGW lpgw);
#if 0 /* shige */
static void	ReadGraphIni(LPGW lpgw);
#endif
static COLORREF	GetColor(HWND hwnd, COLORREF ref);
static void	UpdateColorSample(HWND hdlg);
static BOOL	LineStyle(LPGW lpgw);

/* ================================== */

/* Helper functions for GraphOp(): */

/* destroy memory blocks holding graph operations */
static void
DestroyBlocks(LPGW lpgw)
{
    struct GWOPBLK *this, *next;
    struct GWOP FAR *gwop;
    unsigned int i;

	this = lpgw->gwopblk_head;
	while (this != NULL) {
		next = this->next;
		if (!this->gwop) {
			this->gwop = (struct GWOP FAR *)GlobalLock(this->hblk);
		}
		if (this->gwop) {
			/* free all text strings within this block */
			gwop = this->gwop;
			for (i=0; i<GWOPMAX; i++) {
				if (gwop->htext)
					LocalFree(gwop->htext);
				gwop++;
			}
		}
		GlobalUnlock(this->hblk);
		GlobalFree(this->hblk);
		LocalFreePtr(this);
		this = next;
	}
	lpgw->gwopblk_head = NULL;
	lpgw->gwopblk_tail = NULL;
	lpgw->nGWOP = 0;
}


/* add a new memory block for graph operations */
/* returns TRUE if block allocated */
static BOOL
AddBlock(LPGW lpgw)
{
	HGLOBAL hblk;
	struct GWOPBLK *next, *this;

	/* create new block */
	next = (struct GWOPBLK *)LocalAllocPtr(LHND, sizeof(struct GWOPBLK) );
	if (next == NULL)
		return FALSE;
	hblk = GlobalAlloc(GHND, GWOPMAX*sizeof(struct GWOP));
	if (hblk == NULL)
		return FALSE;
	next->hblk = hblk;
	next->gwop = (struct GWOP FAR *)NULL;
	next->next = (struct GWOPBLK *)NULL;
	next->used = 0;

	/* attach it to list */
	this = lpgw->gwopblk_tail;
	if (this == NULL) {
		lpgw->gwopblk_head = next;
	} else {
		this->next = next;
		this->gwop = (struct GWOP FAR *)NULL;
		GlobalUnlock(this->hblk);
	}
	lpgw->gwopblk_tail = next;
	next->gwop = (struct GWOP FAR *)GlobalLock(next->hblk);
	if (next->gwop == (struct GWOP FAR *)NULL)
		return FALSE;

	return TRUE;
}


void WDPROC
GraphOp(LPGW lpgw, WORD op, WORD x, WORD y, LPCSTR str)
{
    if (str)
	GraphOpSize(lpgw, op, x, y, str, _fstrlen(str)+1);
    else
	GraphOpSize(lpgw, op, x, y, NULL, 0);
}


void WDPROC
GraphOpSize(LPGW lpgw, WORD op, WORD x, WORD y, LPCSTR str, DWORD size)
{
	struct GWOPBLK *this;
	struct GWOP FAR *gwop;
	char *npstr;

	this = lpgw->gwopblk_tail;
	if ( (this==NULL) || (this->used >= GWOPMAX) ) {
		/* not enough space so get new block */
		if (!AddBlock(lpgw))
			return;
		this = lpgw->gwopblk_tail;
	}
	gwop = &this->gwop[this->used];
	gwop->op = op;
	gwop->x = x;
	gwop->y = y;
	gwop->htext = 0;
	if (str) {
		gwop->htext = LocalAlloc(LHND, size);
		npstr = LocalLock(gwop->htext);
		if (gwop->htext && (npstr != (char *)NULL))
			memcpy(npstr, str, size);
		LocalUnlock(gwop->htext);
	}
	this->used++;
	lpgw->nGWOP++;
	return;
}

/* ================================== */

/* Prepare Graph window for being displayed by windows, read wgnuplot.ini, update
 * the window's menus and show it */
void WDPROC
GraphInit(LPGW lpgw)
{
	HMENU sysmenu;
	WNDCLASS wndclass;
	char buf[MAX_PATH];
	HDC hdc;
	TEXTMETRIC metric;

	if (!lpgw->hPrevInstance) {
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = WndGraphProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 2 * sizeof(void FAR *);
		wndclass.hInstance = lpgw->hInstance;
		wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = szGraphClass;
		RegisterClass(&wndclass);
	}

#if 0  /* shige */
	ReadGraphIni(lpgw);
#endif

	lpgw->hWndGraph = CreateWindow(szGraphClass, lpgw->Title,
		WS_OVERLAPPEDWINDOW,
		lpgw->Origin.x, lpgw->Origin.y,
		lpgw->Size.x, lpgw->Size.y,
		NULL, NULL, lpgw->hInstance, lpgw);

	/* determine height of status line */
	hdc = GetDC(lpgw->hWndGraph);
	GetTextMetrics(hdc, &metric);
	lpgw->statuslineheight = metric.tmHeight * 1.2;
	ReleaseDC(lpgw->hWndGraph, hdc);

	lpgw->hPopMenu = CreatePopupMenu();
	AppendMenu(lpgw->hPopMenu, MF_STRING | (lpgw->graphtotop ? MF_CHECKED : MF_UNCHECKED),
		M_GRAPH_TO_TOP, "Bring to &Top");
	AppendMenu(lpgw->hPopMenu, MF_STRING | (lpgw->color ? MF_CHECKED : MF_UNCHECKED),
		M_COLOR, "C&olor");
	AppendMenu(lpgw->hPopMenu, MF_STRING, M_COPY_CLIP, "&Copy to Clipboard (Ctrl+C)");
	AppendMenu(lpgw->hPopMenu, MF_STRING, M_SAVE_AS_EMF, "&Save as EMF... (Ctrl+S)");
#if WINVER >= 0x030a
	AppendMenu(lpgw->hPopMenu, MF_STRING, M_BACKGROUND, "&Background...");
	AppendMenu(lpgw->hPopMenu, MF_STRING, M_CHOOSE_FONT, "Choose &Font...");
	AppendMenu(lpgw->hPopMenu, MF_STRING, M_LINESTYLE, "&Line Styles...");
#endif
	AppendMenu(lpgw->hPopMenu, MF_STRING, M_PRINT, "&Print...");
	if (lpgw->IniFile != (LPSTR)NULL) {
		wsprintf(buf,"&Update %s",lpgw->IniFile);
		AppendMenu(lpgw->hPopMenu, MF_STRING, M_WRITEINI, (LPSTR)buf);
	}

	/* modify the system menu to have the new items we want */
	sysmenu = GetSystemMenu(lpgw->hWndGraph,0);
	AppendMenu(sysmenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(sysmenu, MF_POPUP, (UINT)lpgw->hPopMenu, "&Options");
	AppendMenu(sysmenu, MF_STRING, M_ABOUT, "&About");

	if (!IsWindowVisible(lpgw->lptw->hWndParent)) {
		AppendMenu(sysmenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(sysmenu, MF_STRING, M_COMMANDLINE, "C&ommand Line");
	}

	ShowWindow(lpgw->hWndGraph, SW_SHOWNORMAL);
}

/* close a graph window */
void WDPROC
GraphClose(LPGW lpgw)
{
	/* Pass it through mouse handling to check for "bind Close" */
	Wnd_exec_event(lpgw, (LPARAM)0, GE_reset, 0);
	/* close window */
	if (lpgw->hWndGraph)
		DestroyWindow(lpgw->hWndGraph);
#ifndef WGP_CONSOLE
	TextMessage();
#endif
	lpgw->hWndGraph = NULL;

	lpgw->locked = TRUE;
	DestroyBlocks(lpgw);
	lpgw->locked = FALSE;
}


void WDPROC
GraphStart(LPGW lpgw, double pointsize)
{
	lpgw->locked = TRUE;
	DestroyBlocks(lpgw);
        lpgw->org_pointsize = pointsize;
	if ( !lpgw->hWndGraph || !IsWindow(lpgw->hWndGraph) )
		GraphInit(lpgw);
	if (IsIconic(lpgw->hWndGraph))
		ShowWindow(lpgw->hWndGraph, SW_SHOWNORMAL);
	if (lpgw->graphtotop) {
		/* HBB NEW 20040221: avoid grabbing the keyboard focus
		 * unless mouse mode is on */
#ifdef USE_MOUSE
		if (mouse_setting.on) {
			BringWindowToTop(lpgw->hWndGraph);
			return;
		}
#endif /* USE_MOUSE */
		SetWindowPos(lpgw->hWndGraph,
			     HWND_TOP, 0,0,0,0,
			     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	}
}

void WDPROC
GraphEnd(LPGW lpgw)
{
	RECT rect;

	GetClientRect(lpgw->hWndGraph, &rect);
	InvalidateRect(lpgw->hWndGraph, (LPRECT) &rect, 1);
	lpgw->locked = FALSE;
	UpdateWindow(lpgw->hWndGraph);
#ifdef USE_MOUSE
	gp_exec_event(GE_plotdone, 0, 0, 0, 0, 0);	/* notify main program */
#endif
}

/* shige */
void WDPROC
GraphChangeTitle(LPGW lpgw)
{
	SetWindowText(lpgw->hWndGraph,lpgw->Title);
}

void WDPROC
GraphResume(LPGW lpgw)
{
	lpgw->locked = TRUE;
}

void WDPROC
GraphPrint(LPGW lpgw)
{
	if (lpgw->hWndGraph && IsWindow(lpgw->hWndGraph))
		SendMessage(lpgw->hWndGraph,WM_COMMAND,M_PRINT,0L);
}

void WDPROC
GraphRedraw(LPGW lpgw)
{
	if (lpgw->hWndGraph && IsWindow(lpgw->hWndGraph))
		SendMessage(lpgw->hWndGraph,WM_COMMAND,M_REBUILDTOOLS,0L);
}
/* ================================== */

/* Helper functions for bookkeeping of pens, brushes and fonts */

/* Set up LOGPEN structures based on information coming from wgnuplot.ini, via
 * ReadGraphIni() */
static void
StorePen(LPGW lpgw, int i, COLORREF ref, int colorstyle, int monostyle)
{
	LOGPEN FAR *plp;

	plp = &lpgw->colorpen[i];
	plp->lopnColor = ref;
	if (colorstyle < 0) {
		plp->lopnWidth.x = -colorstyle;
		plp->lopnStyle = 0;
	} else {
		plp->lopnWidth.x = 1;
		plp->lopnStyle = colorstyle % 5;
	}
	plp->lopnWidth.y = 0;

	plp = &lpgw->monopen[i];
	plp->lopnColor = RGB(0,0,0);
	if (monostyle < 0) {
		plp->lopnWidth.x = -monostyle;
		plp->lopnStyle = 0;
	} else {
		plp->lopnWidth.x = 1;
		plp->lopnStyle = monostyle % 5;
	}
	plp->lopnWidth.y = 0;
}

/* Prepare pens and brushes (--> colors) for use by the driver. Pens are (now) created
 * on-the-fly (--> DeleteObject(SelectObject(...)) idiom), but the brushes are still
 * all created statically, and kept until the window is closed */
static void
MakePens(LPGW lpgw, HDC hdc)
{
	int i;

	if ((GetDeviceCaps(hdc,NUMCOLORS) == 2) || !lpgw->color) {
		lpgw->hapen = CreatePenIndirect((LOGPEN FAR *)&lpgw->monopen[1]); 	/* axis */
		lpgw->hbrush = CreateSolidBrush(RGB(255,255,255));
		for (i=0; i<WGNUMPENS+2; i++)
			lpgw->colorbrush[i] = CreateSolidBrush(RGB(0,0,0));
	} else {
		lpgw->hapen = CreatePenIndirect((LOGPEN FAR *)&lpgw->colorpen[1]); 	/* axis */
		lpgw->hbrush = CreateSolidBrush(lpgw->background);
		for (i=0; i<WGNUMPENS+2; i++)
			lpgw->colorbrush[i] = CreateSolidBrush(lpgw->colorpen[i].lopnColor);
	}

	/* build pattern brushes for filled boxes (ULIG) */
	if( ! brushes_initialized ) {
		int i;

		for(i=0; i < halftone_num; i++) {
			halftone_bitdata[i].bmType       = 0;
			halftone_bitdata[i].bmWidth      = 16;
			halftone_bitdata[i].bmHeight     = 8;
			halftone_bitdata[i].bmWidthBytes = 2;
			halftone_bitdata[i].bmPlanes     = 1;
			halftone_bitdata[i].bmBitsPixel  = 1;
			halftone_bitdata[i].bmBits       = halftone_bitmaps[i];
			halftone_bitmap[i] = CreateBitmapIndirect(&halftone_bitdata[i]);
			halftone_brush[i] = CreatePatternBrush(halftone_bitmap[i]);
		}

		for(i=0; i < pattern_num; i++) {
			pattern_bitdata[i].bmType       = 0;
			pattern_bitdata[i].bmWidth      = 16;
			pattern_bitdata[i].bmHeight     = 8;
			pattern_bitdata[i].bmWidthBytes = 2;
			pattern_bitdata[i].bmPlanes     = 1;
			pattern_bitdata[i].bmBitsPixel  = 1;
			pattern_bitdata[i].bmBits       = pattern_bitmaps[i];
			pattern_bitmap[i] = CreateBitmapIndirect(&pattern_bitdata[i]);
			pattern_brush[i] = CreatePatternBrush(pattern_bitmap[i]);
		}

		brushes_initialized = TRUE;
	}
}

/* Undo effect of MakePens(). To be called just before the window is closed. */
static void
DestroyPens(LPGW lpgw)
{
	int i;

	DeleteObject(lpgw->hbrush);
	DeleteObject(lpgw->hapen);
	for (i=0; i<WGNUMPENS+2; i++)
		DeleteObject(lpgw->colorbrush[i]);

	/* delete brushes used for boxfilling (ULIG) */
	if( brushes_initialized ) {
		int i;

		for( i=0; i<halftone_num; i++ ) {
			DeleteObject(halftone_bitmap[i]);
			DeleteObject(halftone_brush[i]);
		}
		for( i=0; i<pattern_num; i++ ) {
			DeleteObject(pattern_bitmap[i]);
			DeleteObject(pattern_brush[i]);
		}
		brushes_initialized = FALSE;
	}
}

/* ================================== */

/* HBB 20010218: new function. An isolated snippet from MakeFont(), now also
 * used in Wnd_put_tmptext() to size the temporary bitmap. */
static void
Wnd_GetTextSize(HDC hdc, LPCSTR str, size_t len, int *cx, int *cy)
{
#ifdef WIN32
	SIZE size;

	GetTextExtentPoint(hdc, str, len, &size);
	*cx = size.cx;
	*cy = size.cy;
#else
	/* Hmm... if not for compatibility to Win 3.0 and earlier, we could
	 * use GetTextExtentPoint on Win16, too :-( */
	DWORD extent;

	extent = GetTextExtent(hdc, str, len);
	*cx = LOWORD(extent);
	*cy = HIWORD(extent);
#endif
}


void 
GetPlotRect(LPGW lpgw, LPRECT rect)
{
	GetClientRect(lpgw->hWndGraph, rect);
	rect->bottom -= lpgw->statuslineheight; /* leave some room for the status line */
	if (rect->bottom < rect->top) rect->bottom = rect->top;
}

static void 
GetPlotRectInMM(LPGW lpgw, LPRECT rect, HDC hdc)
{
	GetPlotRect (lpgw, rect);
	
	/* Taken from 
	http://msdn.microsoft.com/en-us/library/dd183519(VS.85).aspx
	 */
	
	// Determine the picture frame dimensions.  
	// iWidthMM is the display width in millimeters.  
	// iHeightMM is the display height in millimeters.  
	// iWidthPels is the display width in pixels.  
	// iHeightPels is the display height in pixels  
	
	int iWidthMM = GetDeviceCaps(hdc, HORZSIZE); 
	int iHeightMM = GetDeviceCaps(hdc, VERTSIZE); 
	int iWidthPels = GetDeviceCaps(hdc, HORZRES); 
	int iHeightPels = GetDeviceCaps(hdc, VERTRES); 
	
	// Convert client coordinates to .01-mm units.  
	// Use iWidthMM, iWidthPels, iHeightMM, and  
	// iHeightPels to determine the number of  
	// .01-millimeter units per pixel in the x-  
	//  and y-directions.  
	
	rect->left = (rect->left * iWidthMM * 100)/iWidthPels; 
	rect->top = (rect->top * iHeightMM * 100)/iHeightPels; 
	rect->right = (rect->right * iWidthMM * 100)/iWidthPels; 
	rect->bottom = (rect->bottom * iHeightMM * 100)/iHeightPels; 
}


static void
MakeFonts(LPGW lpgw, LPRECT lprect, HDC hdc)
{
	HFONT hfontold;
	TEXTMETRIC tm;
	int result;
	char FAR *p;
	int cx, cy;

	lpgw->rotate = FALSE;
	_fmemset(&(lpgw->lf), 0, sizeof(LOGFONT));
	_fstrncpy(lpgw->lf.lfFaceName,lpgw->fontname,LF_FACESIZE);
	lpgw->lf.lfHeight = -MulDiv(lpgw->fontsize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	lpgw->lf.lfCharSet = DEFAULT_CHARSET;
	if ( (p = _fstrstr(lpgw->fontname," Italic")) != (LPSTR)NULL ) {
		lpgw->lf.lfFaceName[ (unsigned int)(p-lpgw->fontname) ] = '\0';
		lpgw->lf.lfItalic = TRUE;
	}
	if ( (p = _fstrstr(lpgw->fontname," Bold")) != (LPSTR)NULL ) {
		lpgw->lf.lfFaceName[ (unsigned int)(p-lpgw->fontname) ] = '\0';
		lpgw->lf.lfWeight = FW_BOLD;
	}

	if (lpgw->hfonth == 0) {
		lpgw->hfonth = CreateFontIndirect((LOGFONT FAR *)&(lpgw->lf));
	}

	/* we do need a 90 degree font */
	if (lpgw->hfontv) 
		DeleteObject(lpgw->hfontv);
	lpgw->lf.lfEscapement = 900;
	lpgw->lf.lfOrientation = 900;
	lpgw->hfontv = CreateFontIndirect((LOGFONT FAR *)&(lpgw->lf));

	/* save text size */
	hfontold = SelectObject(hdc, lpgw->hfonth);
	Wnd_GetTextSize(hdc, "0123456789", 10, &cx, &cy);
	lpgw->vchar = MulDiv(cy,lpgw->ymax,lprect->bottom - lprect->top);
	lpgw->hchar = MulDiv(cx/10,lpgw->xmax,lprect->right - lprect->left);

	/* CMW: Base tick size on character size */
	lpgw->htic = lpgw->hchar / 2;
	cy = MulDiv(cx/20, GetDeviceCaps(hdc, LOGPIXELSY), GetDeviceCaps(hdc, LOGPIXELSX));
	lpgw->vtic = MulDiv(cy,lpgw->ymax,lprect->bottom - lprect->top);
	/* find out if we can rotate text 90deg */
	SelectObject(hdc, lpgw->hfontv);
	result = GetDeviceCaps(hdc, TEXTCAPS);
	if ((result & TC_CR_90) || (result & TC_CR_ANY))
		lpgw->rotate = TRUE;
	GetTextMetrics(hdc,(TEXTMETRIC FAR *)&tm);
	if (tm.tmPitchAndFamily & TMPF_VECTOR)
		lpgw->rotate = TRUE;	/* vector fonts can all be rotated */
#if WINVER >=0x030a
	if (tm.tmPitchAndFamily & TMPF_TRUETYPE)
		lpgw->rotate = TRUE;	/* truetype fonts can all be rotated */
#endif
	SelectObject(hdc, hfontold);
	return;
}

static void
DestroyFonts(LPGW lpgw)
{
	if (lpgw->hfonth) {
		DeleteObject(lpgw->hfonth);
		lpgw->hfonth = 0;
	}
	if (lpgw->hfontv) {
		DeleteObject(lpgw->hfontv);
		lpgw->hfontv = 0;
	}
	return;
}

static void
SetFont(LPGW lpgw, HDC hdc)
{
    if (lpgw->rotate && lpgw->angle) {
	if (lpgw->hfontv)
	    DeleteObject(lpgw->hfontv);
	lpgw->lf.lfEscapement = lpgw->lf.lfOrientation  = lpgw->angle * 10;
	lpgw->hfontv = CreateFontIndirect((LOGFONT FAR *)&(lpgw->lf));
       if (lpgw->hfontv)
	    SelectObject(hdc, lpgw->hfontv);
    } else {
	if (lpgw->hfonth)
	    SelectObject(hdc, lpgw->hfonth);
    }
    return;
}

static void
SelFont(LPGW lpgw)
{
#if WINVER >= 0x030a
	LOGFONT lf;
	CHOOSEFONT cf;
	HDC hdc;
	char lpszStyle[LF_FACESIZE];
	char FAR *p;

	/* Set all structure fields to zero. */
	_fmemset(&cf, 0, sizeof(CHOOSEFONT));
	_fmemset(&lf, 0, sizeof(LOGFONT));
	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner = lpgw->hWndGraph;
	_fstrncpy(lf.lfFaceName,lpgw->fontname,LF_FACESIZE);
	if ((p = _fstrstr(lpgw->fontname," Bold")) != (LPSTR)NULL) {
		_fstrncpy(lpszStyle,p+1,LF_FACESIZE);
		lf.lfFaceName[ (unsigned int)(p-lpgw->fontname) ] = '\0';
	} else if ((p = _fstrstr(lpgw->fontname," Italic")) != (LPSTR)NULL) {
		_fstrncpy(lpszStyle,p+1,LF_FACESIZE);
		lf.lfFaceName[ (unsigned int)(p-lpgw->fontname) ] = '\0';
	} else {
		_fstrcpy(lpszStyle,"Regular");
	}
	cf.lpszStyle = lpszStyle;
	hdc = GetDC(lpgw->hWndGraph);
	lf.lfHeight = -MulDiv(lpgw->fontsize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(lpgw->hWndGraph, hdc);
	cf.lpLogFont = &lf;
	cf.nFontType = SCREEN_FONTTYPE;
	cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_USESTYLE;
	if (ChooseFont(&cf)) {
		_fstrcpy(lpgw->fontname,lf.lfFaceName);
		lpgw->fontsize = cf.iPointSize / 10;
		if (cf.nFontType & BOLD_FONTTYPE)
			lstrcat(lpgw->fontname," Bold");
		if (cf.nFontType & ITALIC_FONTTYPE)
			lstrcat(lpgw->fontname," Italic");
		/* set current font as default font */
		strcpy(lpgw->deffontname,lpgw->fontname);
		lpgw->deffontsize = lpgw->fontsize;
		SendMessage(lpgw->hWndGraph,WM_COMMAND,M_REBUILDTOOLS,0L);
	}
#endif
}

#ifdef USE_MOUSE
/* ================================== */

static void
LoadCursors(LPGW lpgw)
{
	/* 3 of them are standard cursor shapes: */
	hptrDefault = LoadCursor(NULL, IDC_ARROW);
	hptrZooming = LoadCursor(NULL, IDC_SIZEALL);
	hptrCrossHair = LoadCursor( NULL, IDC_CROSS);
	/* the other 2 are kept in the resource file: */
	hptrScaling = LoadCursor( lpgw->hInstance, MAKEINTRESOURCE(IDC_SCALING));
	hptrRotating = LoadCursor( lpgw->hInstance, MAKEINTRESOURCE(IDC_ROTATING));

	hptrCurrent = hptrCrossHair;
}

static void
DestroyCursors(LPGW lpgw)
{
	/* No-op. Cursors from LoadCursor() don't need destroying */
	return;
}

#endif /* USE_MOUSE */

/* ================================== */


static void
dot(HDC hdc, int xdash, int ydash)
{
	MoveTo(hdc, xdash, ydash);
	LineTo(hdc, xdash, ydash+1);
}


/* This one is really the heart of this module: it executes the stored set of
 * commands, whenever it changed or a redraw is necessary */
static void
drawgraph(LPGW lpgw, HDC hdc, LPRECT rect)
{
    int xdash, ydash;			/* the transformed coordinates */
    int rr, rl, rt, rb;
    struct GWOP FAR *curptr;
    struct GWOPBLK *blkptr;
    int htic, vtic;
    int hshift, vshift;
    unsigned int lastop=-1;		/* used for plotting last point on a line */
    int pen;
    int polymax = 200;
    int polyi = 0;
    POINT *ppt;
    unsigned int ngwop=0;
    BOOL isColor;
    double line_width = 1.0;
    unsigned int fillstyle = 0.0;
    int idx;

    if (lpgw->locked)
	return;

    /* HBB 20010218: the GDI status query functions don't work on Metafile
     * handles, so can't know whether the screen is actually showing
     * color or not, if drawgraph() is being called from CopyClip().
     * Solve by defaulting isColor to 1 if hdc is a metafile. */
    isColor = (((GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc,BITSPIXEL))
		> 2)
	       || (GetDeviceCaps(hdc, TECHNOLOGY) == DT_METAFILE));

    if (lpgw->background != RGB(255,255,255) && lpgw->color && isColor) {
	SetBkColor(hdc,lpgw->background);
	FillRect(hdc, rect, lpgw->hbrush);
    }

    ppt = (POINT *)LocalAllocPtr(LHND, (polymax+1) * sizeof(POINT));

    rr = rect->right;
    rl = rect->left;
    rt = rect->top;
    rb = rect->bottom;

    htic = lpgw->org_pointsize*MulDiv(lpgw->htic, rr - rl, lpgw->xmax) + 1;
    vtic = lpgw->org_pointsize*MulDiv(lpgw->vtic, rb - rt, lpgw->ymax) + 1;

    lpgw->angle = 0;
    SetFont(lpgw, hdc);
    SetTextAlign(hdc, TA_LEFT|TA_BASELINE);
 
    /* calculate text shifting for horizontal text */
    hshift = 0;
    vshift = MulDiv(lpgw->vchar, rb - rt, lpgw->ymax)/2;
  
    pen = 2;
    SelectObject(hdc, lpgw->hapen);
    SelectObject(hdc, lpgw->colorbrush[pen]);

    /* do the drawing */
    blkptr = lpgw->gwopblk_head;
    curptr = NULL;
    if (blkptr) {
	if (!blkptr->gwop)
	    blkptr->gwop = (struct GWOP FAR *)GlobalLock(blkptr->hblk);
	if (!blkptr->gwop)
	    return;
	curptr = (struct GWOP FAR *)blkptr->gwop;
    }

    while(ngwop < lpgw->nGWOP) {
	/* transform the coordinates */
	xdash = MulDiv(curptr->x, rr-rl-1, lpgw->xmax) + rl;
	ydash = MulDiv(curptr->y, rt-rb+1, lpgw->ymax) + rb - 1;
	if ((lastop==W_vect) && (curptr->op!=W_vect)) {
	    if (polyi >= 2) {
		Polyline(hdc, ppt, polyi);
		/* EAM - why isn't this a move to ppt[polyi-1] ? */
		MoveTo(hdc, ppt[0].x, ppt[0].y);
	    }
	    /* EAM - I think this is not necessary */
	    else if (polyi == 1)
		LineTo(hdc, ppt[0].x, ppt[0].y);
	    polyi = 0;
	}
	switch (curptr->op) {
	case 0:	/* have run past last in this block */
	    break;
	case W_move:
	    ppt[0].x = xdash;
	    ppt[0].y = ydash;
	    polyi = 1;
	    break;
	case W_vect:
	    ppt[polyi].x = xdash;
	    ppt[polyi].y = ydash;
	    polyi++;
	    if (polyi >= polymax) {
		Polyline(hdc, ppt, polyi);
		MoveTo(hdc, xdash, ydash);
		ppt[0].x = xdash;
		ppt[0].y = ydash;
		polyi = 1;
	    }
	    break;
	case W_line_type:
	    {
		LOGBRUSH lb;
		LOGPEN cur_penstruct;
		short cur_pen = curptr->x;

		if (cur_pen > WGNUMPENS)
		    cur_pen = cur_pen % WGNUMPENS;
		if (cur_pen <= LT_BACKGROUND) {
		    cur_pen = 0;
		    cur_penstruct = lpgw->colorpen[0];
		    cur_penstruct.lopnColor = lpgw->background;
		} else {
		    cur_pen += 2;
		    cur_penstruct =  (lpgw->color && isColor) ?  lpgw->colorpen[cur_pen] : lpgw->monopen[cur_pen];
		}

		if (line_width != 1)
		    cur_penstruct.lopnWidth.x *= line_width;
	
		/* use ExtCreatePen instead of CreatePen/CreatePenIndirect
		 * to support dashed lines if line_width > 1 */
		lb.lbStyle = BS_SOLID;
		lb.lbColor = cur_penstruct.lopnColor;

		/* shige: work-around for Windows clipboard bug */
		if (line_width==1)
		  lpgw->hapen = CreatePenIndirect((LOGPEN FAR *) &cur_penstruct);
		else
		  lpgw->hapen = ExtCreatePen(
		        PS_GEOMETRIC | cur_penstruct.lopnStyle | PS_ENDCAP_FLAT | PS_JOIN_BEVEL, 
			cur_penstruct.lopnWidth.x, &lb, 0, 0);
		DeleteObject(SelectObject(hdc, lpgw->hapen));

		pen = cur_pen;
		SelectObject(hdc, lpgw->colorbrush[pen]);
		/* Text color is also used for pattern fill */
		SetTextColor(hdc, cur_penstruct.lopnColor);
	    }
	break;

	case W_put_text:
	    {
		char *str;
		str = LocalLock(curptr->htext);
		if (str) {
		    /* shift correctly for rotated text */
		    xdash += hshift;
		    ydash += vshift;

		    SetBkMode(hdc,TRANSPARENT);
		    TextOut(hdc,xdash,ydash,str,lstrlen(str));
		    SetBkMode(hdc,OPAQUE);
		}
		LocalUnlock(curptr->htext);
	    }
	break;

	case W_fillstyle:
	    /* HBB 20010916: new entry, needed to squeeze the many
	     * parameters of a filled box call through the bottlneck
	     * of the fixed number of parameters in GraphOp() and
	     * struct GWOP, respectively. */
	    fillstyle = curptr->x;
	    break;

	case W_boxfill:   /* ULIG */

	    assert (polyi == 1);

	    /* NOTE: the x and y passed with this call are the width and
	     * height of the box, actually. The left corner was stored into
	     * ppt[0] by a preceding W_move, and the style was memorized
	     * by a W_fillstyle call. */
	    switch(fillstyle & 0x0f) {
		case FS_SOLID:
		case FS_TRANSPARENT_SOLID:
		    /* style == 1 --> use halftone fill pattern
		     * according to filldensity. Density is from
		     * 0..100 percent: */
		    idx = ((fillstyle >> 4) * (halftone_num - 1) / 100 );
		    if (idx < 0)
			idx = 0;
		    if (idx > halftone_num - 1)
			idx = halftone_num - 1;
		    SelectObject(hdc, halftone_brush[idx]);
		    break;
		case FS_PATTERN:
		case FS_TRANSPARENT_PATTERN:
		    /* style == 2 --> use fill pattern according to
                     * fillpattern. Pattern number is enumerated */
		    idx = fillstyle >> 4;
		    if (idx < 0)
			idx = 0;
		    if (idx > pattern_num - 1)
			idx = idx % pattern_num;
		    SelectObject(hdc, pattern_brush[idx]);
		    break;
		case FS_DEFAULT:
		    /* Leave the current brush in place */
		    break;
		case FS_EMPTY:
		default:
		    /* fill with background color */
		    SelectObject(hdc, halftone_brush[0]);
		    break;
	    }

	    xdash -= rl;
	    ydash -= rb - 1;
	    PatBlt(hdc, ppt[0].x, ppt[0].y, xdash, ydash, PATCOPY);
	    polyi = 0;
	    break;
  	case W_text_angle:
 	    if (lpgw->angle != (short int)curptr->x) {
 		lpgw->angle = (short int)curptr->x;
 		/* correctly calculate shifting of rotated text */
 		hshift = sin(M_PI/180. * lpgw->angle) * MulDiv(lpgw->vchar, rr-rl, lpgw->xmax) / 2;
 		vshift = cos(M_PI/180. * lpgw->angle) * MulDiv(lpgw->vchar, rb-rt, lpgw->ymax) / 2;
 		SetFont(lpgw, hdc);
 	    }
	    break;
	case W_justify:
	    switch (curptr->x)
		{
		case LEFT:
		    SetTextAlign(hdc, TA_LEFT|TA_BASELINE);
		    break;
		case RIGHT:
		    SetTextAlign(hdc, TA_RIGHT|TA_BASELINE);
		    break;
		case CENTRE:
		    SetTextAlign(hdc, TA_CENTER|TA_BASELINE);
		    break;
		}
	    break;
	case W_font: {
		char *font;

		font = LocalLock(curptr->htext);
		if (font) {
		    GraphChangeFont(lpgw, font, curptr->x, hdc, *rect);
	            SetFont(lpgw, hdc);
		}
		LocalUnlock(curptr->htext);
	    }
	    break;
	case W_pointsize:
	    if (curptr->x != 0) {
		double pointsize = curptr->x / 100.0;
		htic = pointsize*MulDiv(lpgw->htic, rr-rl, lpgw->xmax) + 1;
		vtic = pointsize*MulDiv(lpgw->vtic, rb-rt, lpgw->ymax) + 1;
	    } else {
		char *str;
		str = LocalLock(curptr->htext);
		if (str) {
		    double pointsize;
		    sscanf(str, "%lg", &pointsize);
		    htic = lpgw->org_pointsize
			* MulDiv(lpgw->htic, rr-rl, lpgw->xmax) + 1;
		    vtic = lpgw->org_pointsize
			* MulDiv(lpgw->vtic, rb-rt, lpgw->ymax) + 1;
		}
		LocalUnlock(curptr->htext);
	    }
	    break;
	case W_line_width:
	    /* HBB 20000813: this may look strange, but it ensures
	     * that linewidth is exactly 1 iff it's in default
	     * state */
	    line_width = curptr->x == 100 ? 1 : (curptr->x / 100.0);
	    break;
	case W_pm3d_setcolor:
	    {
		static HBRUSH last_pm3d_brush = NULL;
		HBRUSH this_brush;
		COLORREF c;
		LOGPEN cur_penstruct;
		LOGBRUSH lb;

		/* distinguish gray values and RGB colors */
		if (curptr->y == 0) {			/* TC_FRAC */
		    rgb255_color rgb255;
		    rgb255maxcolors_from_gray(curptr->x / (double)WIN_PAL_COLORS, &rgb255);
		    c = RGB(rgb255.r, rgb255.g, rgb255.b);
		}
		else if (curptr->y == (TC_LT << 8)) {	/* TC_LT */
		    short pen = curptr->x;
		    if (pen > WGNUMPENS) pen = pen % WGNUMPENS;
		    if (pen <= LT_BACKGROUND) {
			pen = 1;
			c = lpgw->background;
		    } else {
			pen += 2;
			c = lpgw->colorpen[pen].lopnColor;
		    }
		}
		else {					/* TC_RGB */
		    c = RGB(curptr->y & 0xff, (curptr->x >> 8) & 0xff, curptr->x & 0xff);
		}

		/* Solid fill brush */
		this_brush = CreateSolidBrush(c);
		SelectObject(hdc, this_brush);
		if (last_pm3d_brush != NULL)
		    DeleteObject(last_pm3d_brush);
		last_pm3d_brush = this_brush;

		/* create new pen, too */
		cur_penstruct = (lpgw->color && isColor) ?  lpgw->colorpen[pen] : lpgw->monopen[pen];
		if (line_width != 1)
		    cur_penstruct.lopnWidth.x *= line_width;
		lb.lbStyle = BS_SOLID;
		lb.lbColor = c;
		/* shige: work-around for Windows clipboard bug */
		if (line_width == 1) {
		    cur_penstruct.lopnColor = c;
		    lpgw->hapen = CreatePenIndirect((LOGPEN FAR *) &cur_penstruct);
		} else
		    lpgw->hapen = ExtCreatePen(
			PS_GEOMETRIC | cur_penstruct.lopnStyle | PS_ENDCAP_FLAT | PS_JOIN_BEVEL, 
			cur_penstruct.lopnWidth.x, &lb, 0, 0);
		DeleteObject(SelectObject(hdc, lpgw->hapen));

		/* set text color, which is also used for pattern fill */
		SetTextColor(hdc, c);
	    }
	    break;
	case W_pm3d_filled_polygon_pt:
	    {
		/* a point of the polygon is coming */
		if (polyi >= polymax) {
		    polymax += 200;
		    ppt = (POINT *)LocalReAllocPtr(ppt, LHND, (polymax+1) * sizeof(POINT));
		}
		ppt[polyi].x = xdash;
		ppt[polyi].y = ydash;
		polyi++;
	    }
	    break;
	case W_pm3d_filled_polygon_draw:
	    {
		/* end of point series --> draw polygon now */
		Polygon(hdc, ppt, polyi);
		polyi = 0;
	    }
	    break;
	case W_image:
	    {
		/* Due to the structure of gwop in total 5 entries are needed.
		   These static variables help to collect all the needed information
		*/
		static int seq = 0;  /* sequence counter */
		static POINT corners[4];

		if (seq < 4) {
		    /* The first four OPs contain the `corner` array */
		    corners[seq].x = xdash;
		    corners[seq].y = ydash;
		} else {
		    /* The last OP contains the image and it's size */
		    BITMAPINFO bmi;
		    char *image;
		    unsigned int M, N;
		    int rc;

		    M = curptr->x;
		    N = curptr->y;
		    image = LocalLock(curptr->htext);
		    if (image) {

			/* rc = SetStretchBltMode(hdc, HALFTONE); */

			memset(&bmi, 0, sizeof(bmi));
			bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
			bmi.bmiHeader.biWidth = M;
			bmi.bmiHeader.biHeight = N;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 24;
    			bmi.bmiHeader.biCompression = BI_RGB;
			bmi.bmiHeader.biClrUsed = 0;

			rc = StretchDIBits(hdc, 
			    corners[0].x, corners[0].y, 
			    corners[1].x - corners[0].x, corners[1].y - corners[0].y,
			    0, 0,
			    M, N,
			    image, &bmi,
			    DIB_RGB_COLORS, SRCCOPY );
		    }
		    LocalUnlock(curptr->htext);
		}
		seq = (seq + 1) % 5;
	    }
	    break;
	case W_dot:
	    dot(hdc, xdash, ydash);
	    break;
	case W_plus: /* do plus */
	    MoveTo(hdc,xdash-htic,ydash);
	    LineTo(hdc,xdash+htic+1,ydash);
	    MoveTo(hdc,xdash,ydash-vtic);
	    LineTo(hdc,xdash,ydash+vtic+1);
	    break;
	case W_cross: /* do X */
	    MoveTo(hdc,xdash-htic,ydash-vtic);
	    LineTo(hdc,xdash+htic+1,ydash+vtic+1);
	    MoveTo(hdc,xdash-htic,ydash+vtic);
	    LineTo(hdc,xdash+htic+1,ydash-vtic-1);
	    break;
	case W_star: /* do star */
	    MoveTo(hdc,xdash-htic,ydash);
	    LineTo(hdc,xdash+htic+1,ydash);
	    MoveTo(hdc,xdash,ydash-vtic);
	    LineTo(hdc,xdash,ydash+vtic+1);
	    MoveTo(hdc,xdash-htic,ydash-vtic);
	    LineTo(hdc,xdash+htic+1,ydash+vtic+1);
	    MoveTo(hdc,xdash-htic,ydash+vtic);
	    LineTo(hdc,xdash+htic+1,ydash-vtic-1);
	    break;
	case W_circle: /* do open circle */
	    Arc(hdc, xdash-htic, ydash-vtic, xdash+htic+1, ydash+vtic+1,
		xdash, ydash+vtic+1, xdash, ydash+vtic+1);
	    dot(hdc, xdash, ydash);
	    break;
	case W_fcircle: /* do filled circle */
	    Ellipse(hdc, xdash-htic, ydash-vtic,
		    xdash+htic+1, ydash+vtic+1);
	    break;
	default:	/* potentially closed figure */
	    {
		POINT p[6];
		int i;
		int shape = 0;
		int filled = 0;
		static float pointshapes[5][10] = {
		    {-1, -1, +1, -1, +1, +1, -1, +1, 0, 0}, /* box */
		    { 0, +1, -1,  0,  0, -1, +1,  0, 0, 0}, /* diamond */
		    { 0, -4./3, -4./3, 2./3,
		      4./3,  2./3, 0, 0}, /* triangle */
		    { 0, 4./3, -4./3, -2./3,
		      4./3,  -2./3, 0, 0}, /* inverted triangle */
		    { 0, 1, 0.95106, 0.30902, 0.58779, -0.80902,
		      -0.58779, -0.80902, -0.95106, 0.30902} /* pentagon */
		};

		switch (curptr->op) {
		case W_box:
		    shape = 0;
		    break;
		case W_diamond:
		    shape = 1;
		    break;
		case W_itriangle:
		    shape = 2;
		    break;
		case W_triangle:
		    shape = 3;
		    break;
		case W_pentagon:
		    shape = 4;
		    break;
		case W_fbox:
		    shape = 0;
		    filled = 1;
		    break;
		case W_fdiamond:
		    shape = 1;
		    filled = 1;
		    break;
		case W_fitriangle:
		    shape = 2;
		    filled = 1;
		    break;
		case W_ftriangle:
		    shape = 3;
		    filled = 1;
		    break;
		case W_fpentagon:
		    shape = 4;
		    filled = 1;
		    break;
		}

		for (i = 0; i < 5; ++i) {
		    if (pointshapes[shape][i * 2 + 1] == 0
			&& pointshapes[shape][i * 2] == 0)
			break;
		    p[i].x = xdash + htic*pointshapes[shape][i*2] + 0.5;
		    p[i].y = ydash + vtic*pointshapes[shape][i*2+1] + 0.5;
		}
		if ( filled )
		    /* Filled polygon */
		    Polygon(hdc, p, i);
		else {
		    /* Outline polygon */
		    p[i].x = p[0].x;
		    p[i].y = p[0].y;
		    Polyline(hdc, p, i+1);
		    dot(hdc, xdash, ydash);
		}
	    } /* default case */
	} /* switch(opcode) */
	lastop = curptr->op;
	ngwop++;
	curptr++;
	if ((unsigned)(curptr - blkptr->gwop) >= GWOPMAX) {
	    GlobalUnlock(blkptr->hblk);
	    blkptr->gwop = (struct GWOP FAR *)NULL;
	    if ((blkptr = blkptr->next) == NULL)
		/* If exact multiple of GWOPMAX entries are queued,
		 * next will be NULL. Only the next GraphOp() call would
		 * have allocated a new block */
		return;
	    if (!blkptr->gwop)
		blkptr->gwop = (struct GWOP FAR *)GlobalLock(blkptr->hblk);
	    if (!blkptr->gwop)
		return;
	    curptr = (struct GWOP FAR *)blkptr->gwop;
	}
    }
    if (polyi >= 2)
	Polyline(hdc, ppt, polyi);
    LocalFreePtr(ppt);
}

/* ================================== */

/* save graph windows as enhanced metafile
 * The code in here is very similar to what CopyClip does...
 */
static void
SaveAsEMF(LPGW lpgw)
{
    static OPENFILENAME Ofn;
    
    static char lpstrCustomFilter[256] = { '\0' };
    static char lpstrFileName[MAX_PATH] = { '\0' };
    static char lpstrFileTitle[MAX_PATH] = { '\0' };
    
    HWND hwnd;
    
    hwnd = lpgw->hWndGraph;
    
    Ofn.lStructSize = sizeof(OPENFILENAME);
    Ofn.hwndOwner = hwnd;
    Ofn.lpstrInitialDir = (LPSTR)NULL;
    Ofn.lpstrFilter = (LPCTSTR) "Enhanced Metafile (*.EMF)\0*.EMF\0All Files (*.*)\0*.*\0";
    Ofn.lpstrCustomFilter = lpstrCustomFilter;
    Ofn.nMaxCustFilter = 255;
    Ofn.nFilterIndex = 1;   /* start with the *.emf filter */
    Ofn.lpstrFile = lpstrFileName;
    Ofn.nMaxFile = MAX_PATH;
    Ofn.lpstrFileTitle = lpstrFileTitle;
    Ofn.nMaxFileTitle = MAX_PATH;
    Ofn.lpstrInitialDir = (LPSTR)NULL;
    Ofn.lpstrTitle = (LPSTR)NULL;
    Ofn.Flags = OFN_OVERWRITEPROMPT;
    Ofn.lpstrDefExt = (LPSTR) "emf";
    
    /* save cwd as GetSaveFileName apparently changes it */
    char *cwd;
    cwd = _getcwd( NULL, 0 );
    
    if( GetSaveFileName(&Ofn) != 0 ) {
	RECT rect, mfrect;
	HDC hdc;
	HENHMETAFILE hemf;
	HDC hmf;
	
	/* get the context */
	hdc = GetDC(hwnd);
	GetPlotRect(lpgw, &rect);
	GetPlotRectInMM(lpgw, &mfrect, hdc);

	hmf = CreateEnhMetaFile(hdc, Ofn.lpstrFile, &mfrect, (LPCTSTR)NULL);
	drawgraph(lpgw, hmf, (LPRECT) &rect);
	hemf = CloseEnhMetaFile(hmf);

	DeleteEnhMetaFile(hemf);
	ReleaseDC(hwnd, hdc);
	
	/* restore cwd */
	if (cwd != NULL) 
	    _chdir( cwd );
    }
    
    /* free the cwd buffer allcoated by _getcwd */
    free(cwd);
}

/* ================================== */

/* copy graph window to clipboard --- note that the Metafile is drawn at the full
 * virtual resolution of the Windows terminal driver (24000 x 18000 pixels), to
 * preserve as much accuracy as remotely possible */
static void
CopyClip(LPGW lpgw)
{
	RECT rect, mfrect;
	HDC mem, hmf;
	HBITMAP bitmap;
	HENHMETAFILE hemf;
	HWND hwnd;
	HDC hdc;

	hwnd = lpgw->hWndGraph;

	/* view the window */
	if (IsIconic(hwnd))
		ShowWindow(hwnd, SW_SHOWNORMAL);
	BringWindowToTop(hwnd);
	UpdateWindow(hwnd);

	/* get the context */
	hdc = GetDC(hwnd);
	GetPlotRect(lpgw, &rect);

	/* make a bitmap and copy it there */
	mem = CreateCompatibleDC(hdc);
	bitmap = CreateCompatibleBitmap(hdc, rect.right - rect.left,
			rect.bottom - rect.top);
	if (bitmap) {
		/* there is enough memory and the bitmaps OK */
		SelectObject(mem, bitmap);
		BitBlt(mem,0,0,rect.right - rect.left,
			rect.bottom - rect.top, hdc, rect.left,
			rect.top, SRCCOPY);
	} else {
		MessageBeep(MB_ICONHAND);
		MessageBox(hwnd, "Insufficient Memory to Copy Clipboard",
			lpgw->Title, MB_ICONHAND | MB_OK);
	}
	DeleteDC(mem);

	/* OK, bitmap done, now create an enhanced Metafile context 
	 * and redraw the whole plot into that. 
	 */
	{
		/* make copy of window's main status struct for modification */
		GW gwclip = *lpgw;

		gwclip.hfonth = gwclip.hfontv = 0;
		MakePens(&gwclip, hdc);
		MakeFonts(&gwclip, &rect, hdc);

		GetPlotRectInMM(lpgw, &mfrect, hdc);

		hmf = CreateEnhMetaFile(hdc, (LPCTSTR)NULL, &mfrect, (LPCTSTR)NULL);
		drawgraph(&gwclip, hmf, (LPRECT) &rect);
		hemf = CloseEnhMetaFile(hmf);

		DestroyFonts(&gwclip);
		DestroyPens(&gwclip);
	}

	/* Now we have the Metafile and Bitmap prepared, post their contents to
	 * the Clipboard */

	OpenClipboard(hwnd);
	EmptyClipboard();
	SetClipboardData(CF_ENHMETAFILE,hemf);
	SetClipboardData(CF_BITMAP, bitmap);
	CloseClipboard();
	ReleaseDC(hwnd, hdc);
	DeleteEnhMetaFile(hemf);
	return;
}

/* copy graph window to printer */
static void
CopyPrint(LPGW lpgw)
{
#ifdef WIN32
	DOCINFO docInfo;
#endif

#if WINVER >= 0x030a	/* If Win 3.0, this whole function does nothing at all ... */
	HDC printer;
#ifndef WIN32
	DLGPROC lpfnAbortProc;
	DLGPROC lpfnPrintDlgProc;
#endif
	PAGESETUPDLG pg;
	DEVNAMES* pDevNames;
	DEVMODE* pDevMode;
	LPCTSTR szDriver, szDevice, szOutput;
	HWND hwnd;
	RECT rect;
	GP_PRINT pr;

	hwnd = lpgw->hWndGraph;


	/* See http://support.microsoft.com/kb/240082 */

	_fmemset (&pg, 0, sizeof pg);
	pg.lStructSize = sizeof pg;
	pg.hwndOwner = hwnd;

	if (!PageSetupDlg (&pg))
		return;

	pDevNames = (DEVNAMES*) GlobalLock (pg.hDevNames);
	pDevMode = (DEVMODE*) GlobalLock (pg.hDevMode);

	szDriver = (LPCTSTR)pDevNames + pDevNames->wDriverOffset;
	szDevice = (LPCTSTR)pDevNames + pDevNames->wDeviceOffset;
	szOutput = (LPCTSTR)pDevNames + pDevNames->wOutputOffset;

	printer = CreateDC (szDriver, szDevice, szOutput, pDevMode);

	GlobalUnlock (pg.hDevMode);
	GlobalUnlock (pg.hDevNames);

	GlobalFree (pg.hDevMode);
	GlobalFree (pg.hDevNames);

	if (NULL == printer)
		return;	/* abort */

	if (!PrintSize(printer, hwnd, &rect)) {
		DeleteDC(printer);
		return; /* abort */
	}

	pr.hdcPrn = printer;
	SetWindowLong(hwnd, 4, (LONG)((GP_LPPRINT)&pr));
#ifdef WIN32
	PrintRegister((GP_LPPRINT)&pr);
#endif

	EnableWindow(hwnd,FALSE);
	pr.bUserAbort = FALSE;
#ifdef WIN32
	pr.hDlgPrint = CreateDialogParam(hdllInstance,"CancelDlgBox",hwnd,PrintDlgProc,(LPARAM)lpgw->Title);
	SetAbortProc(printer,PrintAbortProc);

	memset(&docInfo, 0, sizeof(DOCINFO));
	docInfo.cbSize = sizeof(DOCINFO);
	docInfo.lpszDocName = lpgw->Title;

	if (StartDoc(printer, &docInfo) > 0) {
#else /* not WIN32 */
#  ifdef __DLL__
	lpfnPrintDlgProc = (DLGPROC)GetProcAddress(hdllInstance, "PrintDlgProc");
	lpfnAbortProc = (DLGPROC)GetProcAddress(hdllInstance, "PrintAbortProc");
#  else /* __DLL__ */
	lpfnPrintDlgProc = (DLGPROC)MakeProcInstance((FARPROC)PrintDlgProc, hdllInstance);
	lpfnAbortProc = (DLGPROC)MakeProcInstance((FARPROC)PrintAbortProc, hdllInstance);
#  endif /* __DLL__ */
	pr.hDlgPrint = CreateDialogParam(hdllInstance,"CancelDlgBox",hwnd,lpfnPrintDlgProc,(LPARAM)lpgw->Title);
	Escape(printer,SETABORTPROC,0,(LPSTR)lpfnAbortProc,NULL);
	if (Escape(printer, STARTDOC, lstrlen(lpgw->Title),lpgw->Title, NULL) > 0) {
#endif
		SetMapMode(printer, MM_TEXT);
		SetBkMode(printer,OPAQUE);
#ifdef WIN32
		StartPage(printer);
#endif
		DestroyFonts(lpgw);
		MakeFonts(lpgw, (RECT FAR *)&rect, printer);
		DestroyPens(lpgw);	/* rebuild pens */
		MakePens(lpgw, printer);
		drawgraph(lpgw, printer, (void *) &rect);
#ifdef WIN32
		if (EndPage(printer) > 0)
			EndDoc(printer);
# else /* WIN32 */
		if (Escape(printer,NEWFRAME,0,NULL,NULL) > 0)
			Escape(printer,ENDDOC,0,NULL,NULL);
# endif /* WIN32 */
	}
	if (!pr.bUserAbort) {
		EnableWindow(hwnd,TRUE);
		DestroyWindow(pr.hDlgPrint);
	}
#ifndef WIN32
#ifndef __DLL__
	FreeProcInstance((FARPROC)lpfnPrintDlgProc);
	FreeProcInstance((FARPROC)lpfnAbortProc);
# endif /* __DLL__ */
#endif /* WIN32 */
	DeleteDC(printer);
	SetWindowLong(hwnd, 4, (LONG)(0L));
#ifdef WIN32
	PrintUnregister((GP_LPPRINT)&pr);
#endif /* WIN32 */
	/* make certain that the screen pen set is restored */
	SendMessage(lpgw->hWndGraph,WM_COMMAND,M_REBUILDTOOLS,0L);
#endif /* WIN Version >= 3.1 */
	return;
}

/* ================================== */
/*  INI file stuff */
static void
WriteGraphIni(LPGW lpgw)
{
	RECT rect;
	int i;
	char entry[32];
	LPLOGPEN pc;
	LPLOGPEN pm;
	LPSTR file = lpgw->IniFile;
	LPSTR section = lpgw->IniSection;
	char profile[80];

	if ((file == (LPSTR)NULL) || (section == (LPSTR)NULL))
		return;
	if (IsIconic(lpgw->hWndGraph))
		ShowWindow(lpgw->hWndGraph, SW_SHOWNORMAL);
	GetWindowRect(lpgw->hWndGraph,&rect);
	wsprintf(profile, "%d %d", rect.left, rect.top);
	WritePrivateProfileString(section, "GraphOrigin", profile, file);
	wsprintf(profile, "%d %d", rect.right-rect.left, rect.bottom-rect.top);
	WritePrivateProfileString(section, "GraphSize", profile, file);
	wsprintf(profile, "%s,%d", lpgw->deffontname, lpgw->deffontsize);
	WritePrivateProfileString(section, "GraphFont", profile, file);
	wsprintf(profile, "%d", lpgw->color);
	WritePrivateProfileString(section, "GraphColor", profile, file);
	wsprintf(profile, "%d", lpgw->graphtotop);
	WritePrivateProfileString(section, "GraphToTop", profile, file);
	wsprintf(profile, "%d %d %d",GetRValue(lpgw->background),
			GetGValue(lpgw->background), GetBValue(lpgw->background));
	WritePrivateProfileString(section, "GraphBackground", profile, file);

	/* now save pens */
	for (i=0; i<WGNUMPENS+2; i++) {
		if (i==0)
			_fstrcpy(entry,"Border");
		else if (i==1)
			_fstrcpy(entry,"Axis");
		else
			 wsprintf(entry,"Line%d",i-1);
		pc = &lpgw->colorpen[i];
		pm = &lpgw->monopen[i];
		wsprintf(profile, "%d %d %d %d %d",GetRValue(pc->lopnColor),
			GetGValue(pc->lopnColor), GetBValue(pc->lopnColor),
			(pc->lopnWidth.x != 1) ? -pc->lopnWidth.x : pc->lopnStyle,
			(pm->lopnWidth.x != 1) ? -pm->lopnWidth.x : pm->lopnStyle);
		WritePrivateProfileString(section, entry, profile, file);
	}
	return;
}

void
ReadGraphIni(LPGW lpgw)
{
	LPSTR file = lpgw->IniFile;
	LPSTR section = lpgw->IniSection;
	char profile[81];
	char entry[32];
	LPSTR p;
	int i,r,g,b,colorstyle,monostyle;
	COLORREF ref;
	BOOL bOKINI;

	bOKINI = (file != (LPSTR)NULL) && (section != (LPSTR)NULL);
	if (!bOKINI)
		profile[0] = '\0';

	if (bOKINI)
	  GetPrivateProfileString(section, "GraphOrigin", "", profile, 80, file);
	if ( (p = GetInt(profile, (LPINT)&lpgw->Origin.x)) == NULL)
		lpgw->Origin.x = CW_USEDEFAULT;
	if ( (p = GetInt(p, (LPINT)&lpgw->Origin.y)) == NULL)
		lpgw->Origin.y = CW_USEDEFAULT;
	if (bOKINI)
	  GetPrivateProfileString(section, "GraphSize", "", profile, 80, file);
	if ( (p = GetInt(profile, (LPINT)&lpgw->Size.x)) == NULL)
		lpgw->Size.x = CW_USEDEFAULT;
	if ( (p = GetInt(p, (LPINT)&lpgw->Size.y)) == NULL)
		lpgw->Size.y = CW_USEDEFAULT;

	if (bOKINI)
	  GetPrivateProfileString(section, "GraphFont", "", profile, 80, file);
	{
		char FAR *size;
		size = _fstrchr(profile,',');
		if (size) {
			*size++ = '\0';
			if ( (p = GetInt(size, (LPINT)&lpgw->fontsize)) == NULL)
				lpgw->fontsize = WINFONTSIZE;
		}
		_fstrcpy(lpgw->fontname, profile);
		if (lpgw->fontsize == 0)
			lpgw->fontsize = WINFONTSIZE;
		if (!(*lpgw->fontname)) {
			if (LOWORD(GetVersion()) == 3)
				_fstrcpy(lpgw->fontname,WIN30FONT);
			else
				_fstrcpy(lpgw->fontname,WINFONT);
		}
		/* set current font as default font */
		_fstrcpy(lpgw->deffontname, lpgw->fontname);
		lpgw->deffontsize = lpgw->fontsize;
	}

	if (bOKINI)
	  GetPrivateProfileString(section, "GraphColor", "", profile, 80, file);
		if ( (p = GetInt(profile, (LPINT)&lpgw->color)) == NULL)
			lpgw->color = TRUE;

	if (bOKINI)
	  GetPrivateProfileString(section, "GraphToTop", "", profile, 80, file);
		if ( (p = GetInt(profile, (LPINT)&lpgw->graphtotop)) == NULL)
			lpgw->graphtotop = TRUE;

	lpgw->background = RGB(255,255,255);
	if (bOKINI)
	  GetPrivateProfileString(section, "GraphBackground", "", profile, 80, file);
	if ( ((p = GetInt(profile, (LPINT)&r)) != NULL) &&
	     ((p = GetInt(p, (LPINT)&g)) != NULL) &&
	     ((p = GetInt(p, (LPINT)&b)) != NULL) )
			lpgw->background = RGB(r,g,b);

	StorePen(lpgw, 0,RGB(0,0,0),PS_SOLID,PS_SOLID);
	if (bOKINI)
	  GetPrivateProfileString(section, "Border", "", profile, 80, file);
	if ( ((p = GetInt(profile, (LPINT)&r)) != NULL) &&
	     ((p = GetInt(p, (LPINT)&g)) != NULL) &&
	     ((p = GetInt(p, (LPINT)&b)) != NULL) &&
	     ((p = GetInt(p, (LPINT)&colorstyle)) != NULL) &&
	     ((p = GetInt(p, (LPINT)&monostyle)) != NULL) )
			StorePen(lpgw,0,RGB(r,g,b),colorstyle,monostyle);

	StorePen(lpgw, 1,RGB(192,192,192),PS_DOT,PS_DOT);
	if (bOKINI)
	  GetPrivateProfileString(section, "Axis", "", profile, 80, file);
	if ( ((p = GetInt(profile, (LPINT)&r)) != NULL) &&
	     ((p = GetInt(p, (LPINT)&g)) != NULL) &&
	     ((p = GetInt(p, (LPINT)&b)) != NULL) &&
	     ((p = GetInt(p, (LPINT)&colorstyle)) != NULL) &&
	     ((p = GetInt(p, (LPINT)&monostyle)) != NULL) )
			StorePen(lpgw,1,RGB(r,g,b),colorstyle,monostyle);

	for (i=0; i<WGNUMPENS; i++)
	{
		ref = wginitcolor[ i%WGDEFCOLOR ];
		colorstyle = wginitstyle[ (i/WGDEFCOLOR) % WGDEFSTYLE ];
		monostyle  = wginitstyle[ i%WGDEFSTYLE ];
		StorePen(lpgw, i+2,ref,colorstyle,monostyle);
		wsprintf(entry,"Line%d",i+1);
		if (bOKINI)
		  GetPrivateProfileString(section, entry, "", profile, 80, file);
		if ( ((p = GetInt(profile, (LPINT)&r)) != NULL) &&
		     ((p = GetInt(p, (LPINT)&g)) != NULL) &&
		     ((p = GetInt(p, (LPINT)&b)) != NULL) &&
		     ((p = GetInt(p, (LPINT)&colorstyle)) != NULL) &&
		     ((p = GetInt(p, (LPINT)&monostyle)) != NULL) )
				StorePen(lpgw,i+2,RGB(r,g,b),colorstyle,monostyle);
	}
}


/* ================================== */

/* the "Line Styles..." dialog and its support functions */
/* FIXME HBB 20010218: this might better be delegated to a separate source file */

#define LS_DEFLINE 2
typedef struct tagLS {
	int	widtype;
	int	wid;
	HWND	hwnd;
	int	pen;			/* current pen number */
	LOGPEN	colorpen[WGNUMPENS+2];	/* logical color pens */
	LOGPEN	monopen[WGNUMPENS+2];	/* logical mono pens */
} LS;
typedef LS FAR*  LPLS;


static COLORREF
GetColor(HWND hwnd, COLORREF ref)
{
	CHOOSECOLOR cc;
	COLORREF aclrCust[16];
	int i;

	for (i=0; i<16; i++) {
		aclrCust[i] = RGB(0,0,0);
	}
	_fmemset(&cc, 0, sizeof(CHOOSECOLOR));
	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = hwnd;
	cc.lpCustColors = aclrCust;
	cc.rgbResult = ref;
	cc.Flags = CC_RGBINIT;
	if (ChooseColor(&cc))
		return cc.rgbResult;
	return ref;
}


/* force update of owner draw button */
static void
UpdateColorSample(HWND hdlg)
{
	RECT rect;
	POINT ptul, ptlr;
	GetWindowRect( GetDlgItem(hdlg, LS_COLORSAMPLE), &rect);
	ptul.x = rect.left;
	ptul.y = rect.top;
	ptlr.x = rect.right;
	ptlr.y = rect.bottom;
	ScreenToClient(hdlg, &ptul);
	ScreenToClient(hdlg, &ptlr);
	rect.left   = ptul.x;
	rect.top    = ptul.y;
	rect.right  = ptlr.x;
	rect.bottom = ptlr.y;
	InvalidateRect(hdlg, &rect, TRUE);
	UpdateWindow(hdlg);
}

/* Window handler function for the "Line Styles" dialog */
BOOL CALLBACK WINEXPORT
LineStyleDlgProc(HWND hdlg, UINT wmsg, WPARAM wparam, LPARAM lparam)
{
	char buf[16];
	LPLS lpls;
	int i;
	UINT pen;
	LPLOGPEN plpm, plpc;
	lpls = (LPLS)GetWindowLong(GetParent(hdlg), 4);

	switch (wmsg) {
		case WM_INITDIALOG:
			pen = 2;
			for (i=0; i<WGNUMPENS+2; i++) {
				if (i==0)
					_fstrcpy(buf,"Border");
				else if (i==1)
					_fstrcpy(buf,"Axis");
				else
			 		wsprintf(buf,"Line%d",i-1);
				SendDlgItemMessage(hdlg, LS_LINENUM, LB_ADDSTRING, 0,
					(LPARAM)((LPSTR)buf));
			}
			SendDlgItemMessage(hdlg, LS_LINENUM, LB_SETCURSEL, pen, 0L);

			SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_ADDSTRING, 0,
				(LPARAM)((LPSTR)"Solid"));
			SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_ADDSTRING, 0,
				(LPARAM)((LPSTR)"Dash"));
			SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_ADDSTRING, 0,
				(LPARAM)((LPSTR)"Dot"));
			SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_ADDSTRING, 0,
				(LPARAM)((LPSTR)"DashDot"));
			SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_ADDSTRING, 0,
				(LPARAM)((LPSTR)"DashDotDot"));

			plpm = &lpls->monopen[pen];
			SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_SETCURSEL,
				plpm->lopnStyle, 0L);
			wsprintf(buf,"%d",plpm->lopnWidth.x);
			SetDlgItemText(hdlg, LS_MONOWIDTH, buf);

			SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_ADDSTRING, 0,
				(LPARAM)((LPSTR)"Solid"));
			SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_ADDSTRING, 0,
				(LPARAM)((LPSTR)"Dash"));
			SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_ADDSTRING, 0,
				(LPARAM)((LPSTR)"Dot"));
			SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_ADDSTRING, 0,
				(LPARAM)((LPSTR)"DashDot"));
			SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_ADDSTRING, 0,
				(LPARAM)((LPSTR)"DashDotDot"));

			plpc = &lpls->colorpen[pen];
			SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_SETCURSEL,
				plpc->lopnStyle, 0L);
			wsprintf(buf,"%d",plpc->lopnWidth.x);
			SetDlgItemText(hdlg, LS_COLORWIDTH, buf);

			return TRUE;
		case WM_COMMAND:
			pen = (UINT)SendDlgItemMessage(hdlg, LS_LINENUM, LB_GETCURSEL, 0, 0L);
			plpm = &lpls->monopen[pen];
			plpc = &lpls->colorpen[pen];
			switch (LOWORD(wparam)) {
				case LS_LINENUM:
					wsprintf(buf,"%d",plpm->lopnWidth.x);
					SetDlgItemText(hdlg, LS_MONOWIDTH, buf);
					SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_SETCURSEL,
						plpm->lopnStyle, 0L);
					wsprintf(buf,"%d",plpc->lopnWidth.x);
					SetDlgItemText(hdlg, LS_COLORWIDTH, buf);
					SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_SETCURSEL,
						plpc->lopnStyle, 0L);
					UpdateColorSample(hdlg);
					return FALSE;
				case LS_MONOSTYLE:
					plpm->lopnStyle =
						(UINT)SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_GETCURSEL, 0, 0L);
					if (plpm->lopnStyle != 0) {
						plpm->lopnWidth.x = 1;
						wsprintf(buf,"%d",plpm->lopnWidth.x);
						SetDlgItemText(hdlg, LS_MONOWIDTH, buf);
					}
					return FALSE;
				case LS_MONOWIDTH:
					GetDlgItemText(hdlg, LS_MONOWIDTH, buf, 15);
					GetInt(buf, (LPINT)&plpm->lopnWidth.x);
					if (plpm->lopnWidth.x != 1) {
						plpm->lopnStyle = 0;
						SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_SETCURSEL,
							plpm->lopnStyle, 0L);
					}
					return FALSE;
				case LS_CHOOSECOLOR:
					plpc->lopnColor = GetColor(hdlg, plpc->lopnColor);
					UpdateColorSample(hdlg);
					return FALSE;
				case LS_COLORSTYLE:
					plpc->lopnStyle =
						(UINT)SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_GETCURSEL, 0, 0L);
					if (plpc->lopnStyle != 0) {
						plpc->lopnWidth.x = 1;
						wsprintf(buf,"%d",plpc->lopnWidth.x);
						SetDlgItemText(hdlg, LS_COLORWIDTH, buf);
					}
					return FALSE;
				case LS_COLORWIDTH:
					GetDlgItemText(hdlg, LS_COLORWIDTH, buf, 15);
					GetInt(buf, (LPINT)&plpc->lopnWidth.x);
					if (plpc->lopnWidth.x != 1) {
						plpc->lopnStyle = 0;
						SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_SETCURSEL,
							plpc->lopnStyle, 0L);
					}
					return FALSE;
				case LS_DEFAULT:
					plpm = lpls->monopen;
					plpc = lpls->colorpen;
					/* border */
					plpc->lopnColor   = RGB(0,0,0);
					plpc->lopnStyle   = PS_SOLID;
					plpc->lopnWidth.x = 1;
					plpm->lopnStyle   = PS_SOLID;
					plpm->lopnWidth.x = 1;
					plpc++; plpm++;
					/* axis */
					plpc->lopnColor   = RGB(192,192,192);
					plpc->lopnStyle   = PS_DOT;
					plpc->lopnWidth.x = 1;
					plpm->lopnStyle   = PS_DOT;
					plpm->lopnWidth.x = 1;
					/* LineX */
					for (i=0; i<WGNUMPENS; i++) {
						plpc++; plpm++;
						plpc->lopnColor   = wginitcolor[ i%WGDEFCOLOR ];
						plpc->lopnStyle   = wginitstyle[ (i/WGDEFCOLOR) % WGDEFSTYLE ];
						plpc->lopnWidth.x = 1;
						plpm->lopnStyle   = wginitstyle[ i%WGDEFSTYLE ];
						plpm->lopnWidth.x = 1;
					}
					/* update window */
					plpm = &lpls->monopen[pen];
					plpc = &lpls->colorpen[pen];
					SendDlgItemMessage(hdlg, LS_LINENUM, LB_SETCURSEL, pen, 0L);
					wsprintf(buf,"%d",plpm->lopnWidth.x);
					SetDlgItemText(hdlg, LS_MONOWIDTH, buf);
					SendDlgItemMessage(hdlg, LS_MONOSTYLE, CB_SETCURSEL,
						plpm->lopnStyle, 0L);
					wsprintf(buf,"%d",plpc->lopnWidth.x);
					SetDlgItemText(hdlg, LS_COLORWIDTH, buf);
					SendDlgItemMessage(hdlg, LS_COLORSTYLE, CB_SETCURSEL,
						plpc->lopnStyle, 0L);
					UpdateColorSample(hdlg);
					return FALSE;
				case IDOK:
					EndDialog(hdlg, IDOK);
					return TRUE;
				case IDCANCEL:
					EndDialog(hdlg, IDCANCEL);
					return TRUE;
			}
			break;
		case WM_DRAWITEM:
			{
			HBRUSH hBrush;
			LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lparam;
			pen = (UINT)SendDlgItemMessage(hdlg, LS_LINENUM, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			plpc = &lpls->colorpen[pen];
			hBrush = CreateSolidBrush(plpc->lopnColor);
			FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
			FrameRect(lpdis->hDC, &lpdis->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
			DeleteObject(hBrush);
			}
			return FALSE;
	}
	return FALSE;
}



/* GetWindowLong(hwnd, 4) must be available for use */
static BOOL
LineStyle(LPGW lpgw)
{
	BOOL status = FALSE;
	LS ls;
#ifndef WIN32
	DLGPROC lpfnLineStyleDlgProc;
#endif

	SetWindowLong(lpgw->hWndGraph, 4, (LONG)((LPLS)&ls));
	_fmemcpy(&ls.colorpen, &lpgw->colorpen, (WGNUMPENS + 2) * sizeof(LOGPEN));
	_fmemcpy(&ls.monopen, &lpgw->monopen, (WGNUMPENS + 2) * sizeof(LOGPEN));

#ifdef WIN32
	if (DialogBox (hdllInstance, "LineStyleDlgBox", lpgw->hWndGraph, LineStyleDlgProc)
#else
# ifdef __DLL__
	lpfnLineStyleDlgProc = (DLGPROC)GetProcAddress(hdllInstance, "LineStyleDlgProc");
# else
	lpfnLineStyleDlgProc = (DLGPROC)MakeProcInstance((FARPROC)LineStyleDlgProc, hdllInstance);
# endif
	if (DialogBox (hdllInstance, "LineStyleDlgBox", lpgw->hWndGraph, lpfnLineStyleDlgProc)
#endif
		== IDOK) {
		_fmemcpy(&lpgw->colorpen, &ls.colorpen, (WGNUMPENS + 2) * sizeof(LOGPEN));
		_fmemcpy(&lpgw->monopen, &ls.monopen, (WGNUMPENS + 2) * sizeof(LOGPEN));
		status = TRUE;
	}
#ifndef WIN32
# ifndef __DLL__
	FreeProcInstance((FARPROC)lpfnLineStyleDlgProc);
# endif
#endif
	SetWindowLong(lpgw->hWndGraph, 4, (LONG)(0L));
	return status;
}

#ifdef USE_MOUSE
/* ================================== */
/* HBB 20010207: new helper functions: wrapper around gp_exec_event
 * and DoZoombox. These may vanish again, as has the original idea I
 * invented them for... */

static void
Wnd_exec_event(LPGW lpgw, LPARAM lparam, char type, int par1)
{
    int mx, my;
    static unsigned long lastTimestamp = 0;
    unsigned long thisTimestamp = GetMessageTime();
    int par2 = thisTimestamp - lastTimestamp;

    if (type == GE_keypress)
	par2 = 0;

    GetMousePosViewport(lpgw, &mx, &my);
    gp_exec_event(type, mx, my, par1, par2, 0);
    lastTimestamp = thisTimestamp;
}

static void
Wnd_refresh_zoombox(LPGW lpgw, LPARAM lParam)
{
    int mx, my;

    GetMousePosViewport(lpgw, &mx, &my);
    DrawZoomBox(lpgw); /*  erase current zoom box */
    zoombox.to.x = mx; zoombox.to.y = my;
    DrawZoomBox(lpgw); /*  draw new zoom box */
}

static void
Wnd_refresh_ruler_lineto(LPGW lpgw, LPARAM lParam)
{
    int mx, my;

    GetMousePosViewport(lpgw, &mx, &my);
    DrawRulerLineTo(lpgw); /*  erase current line */
    ruler_lineto.x = mx; ruler_lineto.y = my;
    DrawRulerLineTo(lpgw); /*  draw new line box */
}
#endif /* USE_MOUSE */

/* ================================== */

/* The toplevel function of this module: Window handler function of the graph window */
LRESULT CALLBACK WINEXPORT
WndGraphProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	LPGW lpgw;
	HMENU sysmenu;
	int i;
#ifdef USE_MOUSE
	static unsigned int last_modifier_mask = -99;
#endif

	lpgw = (LPGW)GetWindowLong(hwnd, 0);

#ifdef USE_MOUSE
	/*  mouse events first */
	if (mouse_setting.on /* AND NOT mouse_lock */ ) {
		switch (message) {
			case WM_MOUSEMOVE:
#if 1
				SetCursor(hptrCurrent);
#else
				SetCursor(hptrCrossHair);
#endif
				if (zoombox.on) {
					Wnd_refresh_zoombox(lpgw, lParam);
				}
				if (ruler.on && ruler_lineto.on) {
					Wnd_refresh_ruler_lineto(lpgw, lParam);
				}
				/* track (show) mouse position -- send the event to gnuplot */
				Wnd_exec_event(lpgw, lParam,  GE_motion, wParam);
				return 0L; /* end of WM_MOUSEMOVE */

			case WM_LBUTTONDOWN:
				Wnd_exec_event(lpgw, lParam, GE_buttonpress, 1);
				return 0L;

			case WM_RBUTTONDOWN:
				/* FIXME HBB 20010207: this collides with the right-click
				 * context menu !!! */
				Wnd_exec_event(lpgw, lParam, GE_buttonpress,  3);
				return 0L;

			case WM_MBUTTONDOWN:
				Wnd_exec_event(lpgw, lParam, GE_buttonpress, 2);
				return 0L;

			case WM_LBUTTONDBLCLK:
				Wnd_exec_event(lpgw, lParam, GE_buttonrelease, 1);
				return 0L;

			case WM_RBUTTONDBLCLK:
				Wnd_exec_event(lpgw, lParam, GE_buttonrelease, 3);
				return 0L;

			case WM_MBUTTONDBLCLK:
				Wnd_exec_event(lpgw, lParam, GE_buttonrelease, 2);
				return 0L;

#if 1
			case WM_LBUTTONUP:
#else
			case WM_LBUTTONCLICK:
#endif
				Wnd_exec_event(lpgw, lParam, GE_buttonrelease, 1);
				return 0L;

#if 1
			case WM_RBUTTONUP:
#else
			case WM_RBUTTONCLICK:
#endif
				Wnd_exec_event(lpgw, lParam, GE_buttonrelease, 3);
				return 0L;

#if 1
			case WM_MBUTTONUP:
#else
			case WM_MBUTTONCLICK:
#endif
				Wnd_exec_event(lpgw, lParam, GE_buttonrelease, 2);
				return 0L;

		} /* switch over mouse events */
	}
#endif /* USE_MOUSE */



	switch(message)
	{
		case WM_SYSCOMMAND:
			switch(LOWORD(wParam))
			{
				case M_GRAPH_TO_TOP:
				case M_COLOR:
				case M_CHOOSE_FONT:
				case M_COPY_CLIP:
				case M_SAVE_AS_EMF:
				case M_LINESTYLE:
				case M_BACKGROUND:
				case M_PRINT:
				case M_WRITEINI:
				case M_REBUILDTOOLS:
					SendMessage(hwnd, WM_COMMAND, wParam, lParam);
					break;
				case M_ABOUT:
					if (lpgw->lptw)
						AboutBox(hwnd,lpgw->lptw->AboutText);
					return 0;
				case M_COMMANDLINE:
					sysmenu = GetSystemMenu(lpgw->hWndGraph,0);
					i = GetMenuItemCount (sysmenu);
					DeleteMenu (sysmenu, --i, MF_BYPOSITION);
					DeleteMenu (sysmenu, --i, MF_BYPOSITION);
					ShowWindow (lpgw->lptw->hWndParent, SW_SHOW);
					break;
			}
			break;
		case WM_CHAR:
			/* All 'normal' keys (letters, digits and the likes) end up
			 * here... */
#ifndef DISABLE_SPACE_RAISES_CONSOLE
			if (wParam == VK_SPACE) {
				/* HBB 20001023: implement the '<space> in graph returns to
				 * text window' --- feature already present in OS/2 and X11 */
				/* Make sure the text window is visible: */
				ShowWindow(lpgw->lptw->hWndParent, SW_SHOW);
				/* and activate it (--> Keyboard focus goes there */
				BringWindowToTop(lpgw->lptw->hWndParent);
				return 0;
			}
#endif /* DISABLE_SPACE_RAISES_CONSOLE */

#ifdef USE_MOUSE
			Wnd_exec_event(lpgw, lParam, GE_keypress, (TCHAR)wParam);
#endif
			return 0L;
#ifdef USE_MOUSE
		/* "special" keys have to be caught from WM_KEYDOWN, as they
		 * don't generate WM_CHAR messages. */
		/* NB: It may not be possible to catch Alt-keys, this way */
		case WM_KEYUP:
			{
				/* First, look for a change in modifier status */
				unsigned int modifier_mask = 0;
				modifier_mask = ((GetKeyState(VK_SHIFT) < 0) ? Mod_Shift : 0 )
					| ((GetKeyState(VK_CONTROL) < 0) ? Mod_Ctrl : 0)
					| ((GetKeyState(VK_MENU) < 0) ? Mod_Alt : 0);
				if (modifier_mask != last_modifier_mask) {
					Wnd_exec_event ( lpgw, lParam, GE_modifier, modifier_mask);
					last_modifier_mask = modifier_mask;
				}
			}
			/* Ignore Key-Up events other than those of modifier keys */
			break;
		case WM_KEYDOWN:
			{
			if (GetKeyState(VK_CONTROL) < 0) {
				switch(wParam) {
				case 'C':
					/* Ctrl-C: Copy to Clipboard */
					SendMessage(hwnd,WM_COMMAND,M_COPY_CLIP,0L);
					break;
				case 'S':
					/* Ctrl-S: Save As EMF */
					SendMessage(hwnd,WM_COMMAND,M_SAVE_AS_EMF,0L);
					break;
				} /* switch(wparam) */
			} /* if(Ctrl) */
			else {
				/* First, look for a change in modifier status */
				unsigned int modifier_mask = 0;
				modifier_mask = ((GetKeyState(VK_SHIFT) < 0) ? Mod_Shift : 0 )
					| ((GetKeyState(VK_CONTROL) < 0) ? Mod_Ctrl : 0)
					| ((GetKeyState(VK_MENU) < 0) ? Mod_Alt : 0);
				if (modifier_mask != last_modifier_mask) {
					Wnd_exec_event ( lpgw, lParam, GE_modifier, modifier_mask);
					last_modifier_mask = modifier_mask;
				}
			}
			}
			switch (wParam) {
			case VK_BACK:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_BackSpace);
				break;
			case VK_TAB:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Tab);
				break;
			case VK_RETURN:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Return);
				break;
			case VK_PAUSE:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Pause);
				break;
			case VK_SCROLL:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Scroll_Lock);
				break;
#if 0 /* HOW_IS_THIS_FOR_WINDOWS */
/* HBB 20010215: not at all, AFAICS... :-( */
			case VK_SYSRQ:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Sys_Req);
				break;
#endif
			case VK_ESCAPE:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Escape);
				break;
			case VK_DELETE:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Delete);
				break;
			case VK_INSERT:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_KP_Insert);
				break;
			case VK_HOME:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Home);
				break;
			case VK_LEFT:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Left);
				break;
			case VK_UP:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Up);
				break;
			case VK_RIGHT:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Right);
				break;
			case VK_DOWN:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_Down);
				break;
			case VK_END:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_End);
				break;
			case VK_PRIOR:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_PageUp);
				break;
			case VK_NEXT:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_PageDown);
				break;
			case VK_F1:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F1);
				break;
			case VK_F2:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F2);
				break;
			case VK_F3:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F3);
				break;
			case VK_F4:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F4);
				break;
			case VK_F5:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F5);
				break;
			case VK_F6:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F6);
				break;
			case VK_F7:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F7);
				break;
			case VK_F8:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F8);
				break;
			case VK_F9:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F9);
				break;
			case VK_F10:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F10);
				break;
			case VK_F11:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F11);
				break;
			case VK_F12:
				Wnd_exec_event(lpgw, lParam, GE_keypress, GP_F12);
				break;
			} /* switch (wParam) */

			return 0L;
#if 0 /* DO WE NEED THIS ??? */
		case WM_MOUSEMOVE:
			/* set default pointer: */
			SetCursor(hptrDefault);
			return 0L;
#endif
#endif /* USE_MOUSE */
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case M_GRAPH_TO_TOP:
					lpgw->graphtotop = !lpgw->graphtotop;
					SendMessage(hwnd,WM_COMMAND,M_REBUILDTOOLS,0L);
					return(0);
				case M_COLOR:
					lpgw->color = !lpgw->color;
					SendMessage(hwnd,WM_COMMAND,M_REBUILDTOOLS,0L);
					return(0);
				case M_CHOOSE_FONT:
					SelFont(lpgw);
					return 0;
				case M_COPY_CLIP:
					CopyClip(lpgw);
					return 0;
				case M_SAVE_AS_EMF:
					SaveAsEMF(lpgw);
					return 0;
				case M_LINESTYLE:
					if (LineStyle(lpgw))
						SendMessage(hwnd,WM_COMMAND,M_REBUILDTOOLS,0L);
					return 0;
				case M_BACKGROUND:
					lpgw->background = GetColor(hwnd, lpgw->background);
					SendMessage(hwnd,WM_COMMAND,M_REBUILDTOOLS,0L);
					return 0;
				case M_PRINT:
					CopyPrint(lpgw);
					return 0;
				case M_WRITEINI:
					WriteGraphIni(lpgw);
					if (lpgw->lptw)
						WriteTextIni(lpgw->lptw);
					return 0;
				case M_REBUILDTOOLS:
					lpgw->resized = TRUE;
					if (lpgw->color)
						CheckMenuItem(lpgw->hPopMenu, M_COLOR, MF_BYCOMMAND | MF_CHECKED);
					else
						CheckMenuItem(lpgw->hPopMenu, M_COLOR, MF_BYCOMMAND | MF_UNCHECKED);
					if (lpgw->graphtotop)
						CheckMenuItem(lpgw->hPopMenu, M_GRAPH_TO_TOP, MF_BYCOMMAND | MF_CHECKED);
					else
						CheckMenuItem(lpgw->hPopMenu, M_GRAPH_TO_TOP, MF_BYCOMMAND | MF_UNCHECKED);
					DestroyPens(lpgw);
					DestroyFonts(lpgw);
					hdc = GetDC(hwnd);
					MakePens(lpgw, hdc);
					GetPlotRect(lpgw, &rect);
					MakeFonts(lpgw, (LPRECT)&rect, hdc);
					ReleaseDC(hwnd, hdc);
					GetClientRect(hwnd, &rect);
					InvalidateRect(hwnd, (LPRECT) &rect, 1);
					UpdateWindow(hwnd);
					return 0;
			}
			return 0;
		case WM_RBUTTONDOWN:
			/* HBB 20010218: note that this only works in mouse-off
			 * mode, now. You'll need to go via the System menu to
			 * access this popup, instead */
			{
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			ClientToScreen(hwnd,&pt);
			TrackPopupMenu(lpgw->hPopMenu, TPM_LEFTALIGN,
				pt.x, pt.y, 0, hwnd, NULL);
			}
			return(0);
		case WM_CREATE:
			lpgw = ((CREATESTRUCT FAR *)lParam)->lpCreateParams;
			SetWindowLong(hwnd, 0, (LONG)lpgw);
			lpgw->hWndGraph = hwnd;
			hdc = GetDC(hwnd);
			MakePens(lpgw, hdc);
#ifdef USE_MOUSE
			LoadCursors(lpgw);
#endif
			GetPlotRect(lpgw, &rect);
			MakeFonts(lpgw, (LPRECT)&rect, hdc);
			ReleaseDC(hwnd, hdc);
#if WINVER >= 0x030a
			{
			WORD version = LOWORD(GetVersion());

			if ((LOBYTE(version)*100 + HIBYTE(version)) >= 310)
				if ( lpgw->lptw && (lpgw->lptw->DragPre!=(LPSTR)NULL) && (lpgw->lptw->DragPost!=(LPSTR)NULL) )
					DragAcceptFiles(hwnd, TRUE);
			}
#endif
			return(0);
		case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);
			SetMapMode(hdc, MM_TEXT);
			SetBkMode(hdc,OPAQUE);
			GetPlotRect(lpgw, &rect);
#ifdef WIN32
			SetViewportExtEx(hdc, rect.right, rect.bottom, NULL);
#else
			SetViewportExt(hdc, rect.right, rect.bottom);
#endif
			drawgraph(lpgw, hdc, &rect);
			EndPaint(hwnd, &ps);
#ifdef USE_MOUSE
			/* drawgraph() erases the plot window, so immediately after
			 * it has completed, all the 'real-time' stuff the gnuplot core
			 * doesn't know anything about has to be redrawn */
			DrawRuler(lpgw);
			DisplayStatusLine(lpgw);
			DrawRulerLineTo(lpgw);
			DrawZoomBox(lpgw);
#endif
			return 0;
		case WM_SIZE:
			/* update font sizes if graph resized */
			if ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)) {
				RECT rect;
				SendMessage(hwnd,WM_SYSCOMMAND,M_REBUILDTOOLS,0L);
				GetWindowRect(hwnd,&rect);
				lpgw->Size.x = rect.right-rect.left;
				lpgw->Size.y = rect.bottom-rect.top;
			}
			break;
#if WINVER >= 0x030a
		case WM_DROPFILES:
			{
			WORD version = LOWORD(GetVersion());
			if ((LOBYTE(version)*100 + HIBYTE(version)) >= 310)
				if (lpgw->lptw)
					DragFunc(lpgw->lptw, (HDROP)wParam);
			}
			break;
#endif
		case WM_DESTROY:
			DestroyPens(lpgw);
			DestroyFonts(lpgw);
#ifdef USE_MOUSE
			DestroyCursors(lpgw);
#endif
#if __TURBOC__ >= 0x410    /* Borland C++ 3.1 or later */
			{
			WORD version = LOWORD(GetVersion());
			if ((LOBYTE(version)*100 + HIBYTE(version)) >= 310)
				DragAcceptFiles(hwnd, FALSE);
			}
#endif
			if (lpgw->lptw && !IsWindowVisible(lpgw->lptw->hWndParent)) {
				PostMessage (lpgw->lptw->hWndParent, WM_CLOSE, 0, 0);
			}
			return 0;
		case WM_CLOSE:
			GraphClose(lpgw);
			return 0;
		}
	return DefWindowProc(hwnd, message, wParam, lParam);
}


void WDPROC
GraphChangeFont(LPGW lpgw, LPCSTR font, int fontsize, HDC hdc, RECT rect)
{
    int newfontsize;
    bool remakefonts = FALSE;

    newfontsize = (fontsize != 0) ? fontsize : lpgw->deffontsize;
    if (font != NULL) {
	remakefonts = (strcmp(lpgw->fontname, font) != 0) || (newfontsize != lpgw->fontsize);
    } else {
	remakefonts = (strcmp(lpgw->fontname, lpgw->deffontname) != 0) || (newfontsize != lpgw->fontsize);
    }

    if (remakefonts) {
        lpgw->fontsize = newfontsize;
        strcpy(lpgw->fontname, (font) ? font : lpgw->deffontname);

        DestroyFonts(lpgw);
        MakeFonts(lpgw, &rect, hdc);
    }
}

/* close the terminal window */
void WDPROC
win_close_terminal_window(LPGW lpgw)
{
   if (lpgw->hWndGraph && IsWindow(lpgw->hWndGraph))
	SendMessage( lpgw->hWndGraph, WM_CLOSE, 0L, 0L );
}

#if 0
int WDPROC
GraphGetFontScaling(LPGW lpgw, LPCSTR font, int fontsize)
{
    HDC hdc;
    RECT rect;
    HGDIOBJ hprevfont;
    OUTLINETEXTMETRIC otm;
    int shift = 35;

    hdc = GetDC(lpgw->hWndGraph);
	GetPlotRect(lpgw, &rect);
    GraphChangeFont(lpgw, font, fontsize, hdc, rect);
    hprevfont = SelectObject(hdc, lpgw->hfonth);
    if (GetOutlineTextMetrics(hdc, sizeof(otm), &otm) != 0) {
        shift = otm.otmptSuperscriptOffset.y - otm.otmptSubscriptOffset.y;
        shift = MulDiv(shift, GetDeviceCaps(hdc, LOGPIXELSY), 2 * 72);
        printf( "shift: %i (%i %i)\n", shift*8, otm.otmptSuperscriptOffset.y, otm.otmptSubscriptOffset.y);
    }
    SelectObject(hdc, hprevfont);

    return shift * 8;
}
#endif


unsigned int WDPROC
GraphGetTextLength(LPGW lpgw, LPCSTR text, LPCSTR fontname, int fontsize)
{
    HDC hdc;
    RECT rect;
    SIZE size;
    HGDIOBJ hprevfont;

    hdc = GetDC(lpgw->hWndGraph);
	GetPlotRect(lpgw, &rect);

    GraphChangeFont(lpgw, fontname, fontsize, hdc, rect);

    hprevfont = SelectObject(hdc, lpgw->hfonth);
    GetTextExtentPoint(hdc, text, strlen(text), &size);
    SelectObject(hdc, hprevfont);
    
    size.cx = MulDiv(size.cx + GetTextCharacterExtra(hdc), lpgw->xmax, rect.right-rect.left-1);
    return size.cx;
}


#ifdef USE_MOUSE
/* Implemented by Petr Mikulik, February 2001 --- the best Windows solutions
 * come from OS/2 :-))
 */

/* ================================================================= */

/* Firstly: terminal calls from win.trm */

/* Note that these all take lpgw as their first argument. It's an OO-type
 * 'this' pointer, sort of: it stores all the status information of the graph
 * window that we need, in a single large structure. */

void WDPROC
Graph_set_cursor (LPGW lpgw, int c, int x, int y )
{
	switch (c) {
	case -4: /* switch off line between ruler and mouse cursor */
		DrawRulerLineTo(lpgw);
		ruler_lineto.on = FALSE;
		break;
	case -3: /* switch on line between ruler and mouse cursor */
		if (ruler.on && ruler_lineto.on)
		    break;
		ruler_lineto.x = x;
		ruler_lineto.y = y;
		ruler_lineto.on = TRUE;
		DrawRulerLineTo(lpgw);
		break;
	case -2:
		{ /* move mouse to the given point */
			RECT rc;
			POINT pt;

			GetPlotRect(lpgw, &rc);
			pt.x = MulDiv(x, rc.right - rc.left, lpgw->xmax);
			pt.y = rc.bottom - MulDiv(y, rc.bottom - rc.top, lpgw->ymax);

			MapWindowPoints(lpgw->hWndGraph, HWND_DESKTOP, &pt, 1);
			SetCursorPos(pt.x, pt.y);
		}
		break;
	case -1: /* start zooming; zooming cursor */
		zoombox.on = TRUE;
		zoombox.from.x = zoombox.to.x = x;
		zoombox.from.y = zoombox.to.y = y;
		break;
	case 0:  /* standard cross-hair cursor */
		SetCursor( (hptrCurrent = mouse_setting.on ? hptrCrossHair : hptrDefault) );
		break;
	case 1:  /* cursor during rotation */
		SetCursor(hptrCurrent = hptrRotating);
		break;
	case 2:  /* cursor during scaling */
		SetCursor(hptrCurrent = hptrScaling);
		break;
	case 3:  /* cursor during zooming */
		SetCursor(hptrCurrent = hptrZooming);
		break;
	}
	if (c>=0 && zoombox.on) { /* erase zoom box */
		DrawZoomBox(lpgw);
		zoombox.on = FALSE;
	}
	if (c>=0 && ruler_lineto.on) { /* erase ruler line */
		DrawRulerLineTo(lpgw);
		ruler_lineto.on = FALSE;
	}
}

/* set_ruler(int x, int y) term API: x<0 switches ruler off. */
void WDPROC
Graph_set_ruler (LPGW lpgw, int x, int y )
{
	DrawRuler(lpgw); /* remove previous drawing, if any */
	DrawRulerLineTo(lpgw);
	if (x < 0) {
		ruler.on = FALSE;
		return;
	}
	ruler.on = TRUE;
	ruler.x = x; ruler.y = y;
	DrawRuler(lpgw); /* draw ruler at new positions */
	DrawRulerLineTo(lpgw);
}

/* put_tmptext(int i, char c[]) term API
 * 	i: 0..at statusline
 *	1, 2: at corners of zoom box with \r separating text
 */
void WDPROC
Graph_put_tmptext (LPGW lpgw, int where, LPCSTR text )
{
    /* Position of the annotation string (mouse movement) or zoom box
     * text or whatever temporary text added...
     */
    switch (where) {
	case 0:
		UpdateStatusLine(lpgw, text);
		break;
	case 1:
		DrawZoomBox(lpgw);
		if (zoombox.text1) {
			free((char*)zoombox.text1);
		}
		zoombox.text1 = strdup(text);
		DrawZoomBox(lpgw);
		break;
	case 2:
		DrawZoomBox(lpgw);
		if (zoombox.text2) {
			free((char*)zoombox.text2);
		}
		zoombox.text2 = strdup(text);
		DrawZoomBox(lpgw);
		break;
	default:
		; /* should NEVER happen */
    }
}

void WDPROC
Graph_set_clipboard (LPGW lpgw, LPCSTR s)
{
	size_t length;
	HGLOBAL memHandle;
	LPSTR clipText;

	/* check: no string --> nothing to do */
	if (!s || !s[0])
		return;

	/* Transport the string into a system-global storage area. In case
	 * of (unlikely) allocation failures, fail silently */
	length = strlen(s);
	if ( (memHandle = GlobalAlloc(GHND, length + 1)) == NULL)
		return;
	if ( (clipText = GlobalLock(memHandle)) == NULL) {
		GlobalFree(memHandle);
		return;
	}
	strcpy(clipText, s);
	GlobalUnlock(memHandle);

	/* Now post that memory area to the Clipboard */
	OpenClipboard(lpgw->hWndGraph);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, memHandle);
	CloseClipboard();
}

/* ================================================================= */

/* Secondly, support routines that implement direct mouse reactions. */

/* This routine gets the mouse/pointer position in the current window and
 * recalculates it to the viewport coordinates. */
static void
GetMousePosViewport (LPGW lpgw, int *mx, int *my)
{
	POINT pt;
	RECT rc;

	GetPlotRect(lpgw, &rc);

	/* HBB: has to be done this way. The simpler method by taking apart LPARM
	 * only works for mouse, but not for keypress events. */
	GetCursorPos(&pt);
	ScreenToClient(lpgw->hWndGraph,&pt);
	*mx = pt.x;
	*my = pt.y;

	/* px=px(mx); mouse=>gnuplot driver coordinates */
	/* FIXME: classically, this would use MulDiv() call, and no floating point */
	*mx = (int)((*mx - rc.left) * lpgw->xmax / (rc.right - rc.left) + 0.5);
	*my = (int)((rc.bottom - *my) * lpgw->ymax / (rc.bottom -rc.top) + 0.5);
}

/* HBB 20010218: Newly separated function: Draw text string in XOR mode.
 * That is surprisingly difficult using the Windows API: have to draw text
 * into a background bitmap, first, and then blit that onto the screen.
 *
 * x and y give the bottom-left corner of the bounding box into which the
 * text will be drawn */
static void
Draw_XOR_Text(LPGW lpgw, const char *text, size_t length, int x, int y)
{
	HDC hdc, tempDC;
	HBITMAP bitmap;
	int cx, cy;

	if (!text || !text[0])
	       return; /* no text to be displayed */

	hdc = GetDC(lpgw->hWndGraph);

	/* Prepare background image buffer of the necessary size */
	Wnd_GetTextSize(hdc, text, length, &cx, &cy);
	bitmap = CreateCompatibleBitmap(hdc, cx, cy);
	/* ... and a DeviceContext to access it by */
	tempDC = CreateCompatibleDC(hdc);
	DeleteObject(SelectObject(tempDC, bitmap));

	/* Print inverted text, so the second inversion done by SRCINVERT ends
	 * up printing the right way round... */
	/* FIXME HBB 20010218: find out the real ROP3 code for operation
	 * "target = target XOR (NOT source)" and use that, instead. It's the
	 * one with MSByte = 0x99, but without VC++ or MSDN, I can't seem to
	 * find out what the full code is. */
	SetTextColor(tempDC, GetBkColor(hdc));
	SetBkColor(tempDC, GetTextColor(hdc));
	TextOut(tempDC, 0, 0, text, length);

	/* Copy printed string to the screen window by XORing, so the
	 * repetition of this same operation will delete it again */
	BitBlt(hdc, x, y - cy, cx, cy, tempDC, 0, 0, SRCINVERT);

	/* Clean up behind ourselves */
	DeleteDC(tempDC);
	DeleteObject(bitmap);
	ReleaseDC(lpgw->hWndGraph, hdc);
}


/* ================================== */

/* Status line routines. */

/* Saved text currently contained in status line */
static char *sl_curr_text = NULL;

/* Display the status line text */
static void
DisplayStatusLine(LPGW lpgw)
{
	RECT rc;
	HDC hdc;

	if (!sl_curr_text || !sl_curr_text[0])
	       return; /* no text to be displayed */

	hdc = GetDC(lpgw->hWndGraph);
	SetBkMode(hdc, OPAQUE);
	GetClientRect(lpgw->hWndGraph, &rc);
	TextOut(hdc,  0, rc.bottom - lpgw->statuslineheight, sl_curr_text, strlen(sl_curr_text));
	ReleaseDC(lpgw->hWndGraph, hdc);
}

/*
 * Update the status line by the text; erase the previous text
 */
static void
UpdateStatusLine (LPGW lpgw, const char text[])
{
	RECT rc;
	HDC hdc;
	SIZE size, size2;

	hdc = GetDC(lpgw->hWndGraph);
	GetClientRect(lpgw->hWndGraph, &rc);

	/* determine length of previous text */
	size.cx = 0;
	if (sl_curr_text) {
		GetTextExtentPoint(hdc, sl_curr_text, strlen(sl_curr_text), &size);
		free(sl_curr_text);
	}

	/* determine length of new text */
	if (!text || !*text) {
		sl_curr_text = 0;
		size2.cx = 0;
	} else {
		sl_curr_text = strdup(text);
		GetTextExtentPoint(hdc, sl_curr_text, strlen(sl_curr_text), &size2);
		/* overwrite previous text */
		SetBkMode(hdc, OPAQUE);
		TextOut(hdc,  0, rc.bottom - lpgw->statuslineheight, sl_curr_text, strlen(sl_curr_text));
	}

	/* erase the rest */
	if (size.cx > size2.cx) {
		rc.left = size2.cx;
		rc.right = size.cx;
		rc.top  = rc.bottom - lpgw->statuslineheight;
		FillRect(hdc, &rc, (HBRUSH) (COLOR_WINDOW+1));
	}

	ReleaseDC(lpgw->hWndGraph, hdc);
}

/* Draw the ruler.
 */
static void
DrawRuler (LPGW lpgw)
{
	HDC hdc;
	int iOldRop;
	RECT rc;
	long rx, ry;

	if (!ruler.on || ruler.x < 0)
		return;

	hdc = GetDC(lpgw->hWndGraph);
	GetPlotRect(lpgw, &rc);

	rx = MulDiv(ruler.x, rc.right - rc.left, lpgw->xmax);
	ry = rc.bottom - MulDiv(ruler.y, rc.bottom - rc.top, lpgw->ymax);

	iOldRop = SetROP2(hdc, R2_NOT);
	MoveTo(hdc, rc.left, ry);
	LineTo(hdc, rc.right, ry);
	MoveTo(hdc, rx, rc.top);
	LineTo(hdc, rx, rc.bottom);
	SetROP2(hdc, iOldRop);
	ReleaseDC(lpgw->hWndGraph, hdc);
}

/* Draw the ruler line to cursor position.
 */
static void
DrawRulerLineTo (LPGW lpgw)
{
	HDC hdc;
	int iOldRop;
	RECT rc;
	long rx, ry, rlx, rly;

	if (!ruler.on || !ruler_lineto.on || ruler.x < 0 || ruler_lineto.x < 0)
		return;

	hdc = GetDC(lpgw->hWndGraph);
	GetPlotRect(lpgw, &rc);

	rx  = MulDiv(ruler.x, rc.right - rc.left, lpgw->xmax);
	ry  = rc.bottom - MulDiv(ruler.y, rc.bottom - rc.top, lpgw->ymax);
	rlx = MulDiv(ruler_lineto.x, rc.right - rc.left, lpgw->xmax);
	rly = rc.bottom - MulDiv(ruler_lineto.y, rc.bottom - rc.top, lpgw->ymax);

	iOldRop = SetROP2(hdc, R2_NOT);
	MoveTo(hdc, rx, ry);
	LineTo(hdc, rlx, rly);
	SetROP2(hdc, iOldRop);
	ReleaseDC(lpgw->hWndGraph, hdc);
}

/* Draw the zoom box.
 */
static void
DrawZoomBox (LPGW lpgw)
{
	HDC hdc;
	long fx, fy, tx, ty, text_y;
	int OldROP2;
	RECT rc;
	HPEN OldPen;

	if (!zoombox.on)
		return;

	hdc = GetDC(lpgw->hWndGraph);
	GetPlotRect(lpgw, &rc);

	fx = MulDiv(zoombox.from.x, rc.right - rc.left, lpgw->xmax);
	fy = rc.bottom - MulDiv(zoombox.from.y, rc.bottom - rc.top, lpgw->ymax);
	tx = MulDiv(zoombox.to.x, rc.right - rc.left, lpgw->xmax);
	ty = rc.bottom - MulDiv(zoombox.to.y, rc.bottom - rc.top, lpgw->ymax);
	text_y = MulDiv(lpgw->vchar, rc.bottom - rc.top, lpgw->ymax);

	OldROP2 = SetROP2(hdc, R2_NOTXORPEN);
	OldPen = SelectObject(hdc, CreatePenIndirect(
			(lpgw->color ? lpgw->colorpen : lpgw->monopen) + 1));
	Rectangle(hdc, fx, fy, tx, ty);
	DeleteObject(SelectObject(hdc, OldPen));
	SetROP2(hdc, OldROP2);

	ReleaseDC(lpgw->hWndGraph, hdc);

	if (zoombox.text1) {
		char *separator = strchr(zoombox.text1, '\r');

		if (separator) {
			Draw_XOR_Text(lpgw, zoombox.text1, separator - zoombox.text1, fx, fy);
			Draw_XOR_Text(lpgw, separator + 1, strlen(separator + 1), fx, fy + text_y);
		} else {
			Draw_XOR_Text(lpgw, zoombox.text1, strlen(zoombox.text1), fx, fy + lpgw->vchar / 2);
		}
	}
	if (zoombox.text2) {
		char *separator = strchr(zoombox.text2, '\r');

		if (separator) {
			Draw_XOR_Text(lpgw, zoombox.text2, separator - zoombox.text2, tx, ty);
			Draw_XOR_Text(lpgw, separator + 1, strlen(separator + 1), tx, ty + text_y);
		} else  {
			Draw_XOR_Text(lpgw, zoombox.text2, strlen(zoombox.text2), tx, ty + lpgw->vchar / 2);
		}
	}
}

#endif /* USE_MOUSE */

/* eof wgraph.c */

