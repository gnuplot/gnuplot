#ifndef lint
static char *RCSid = "$Id: wgraph.c%v 3.50.1.13 1993/08/19 03:21:26 woo Exp $";
#endif

/* GNUPLOT - win/wgraph.c */
/*
 * Copyright (C) 1992   Maurice Castro, Russell Lang
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
 *   Maurice Castro
 *   Russell Lang
 * 
 * Send your comments or suggestions to 
 *  info-gnuplot@dartmouth.edu.
 * This is a mailing list; to join it send a note to 
 *  info-gnuplot-request@dartmouth.edu.  
 * Send bug reports to
 *  bug-gnuplot@dartmouth.edu.
 */

#define STRICT
#include <windows.h>
#include <windowsx.h>
#if WINVER >= 0x030a
#include <commdlg.h>
#endif
#ifndef __MSC__
#include <mem.h>
#endif
#include <string.h>
#include "wgnuplib.h"
#include "wresourc.h"
#include "wcommon.h"

LRESULT CALLBACK _export WndGraphProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void ReadGraphIni(LPGW lpgw);

/* ================================== */

#define MAXSTR 255

#define WGDEFCOLOR 15
COLORREF wginitcolor[WGDEFCOLOR] =  {
	RGB(0,0,255),	/* blue */
	RGB(0,255,0),	/* green */
	RGB(255,0,0),	/* red */
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

/* ================================== */

/* destroy memory blocks holding graph operations */
void
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
BOOL 
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
	}
	else {
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
GraphOp(LPGW lpgw, WORD op, WORD x, WORD y, LPSTR str)
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
		gwop->htext = LocalAlloc(LHND, _fstrlen(str)+1);
		npstr = LocalLock(gwop->htext);
		if (gwop->htext && (npstr != (char *)NULL))
			lstrcpy(npstr, str);
		LocalUnlock(gwop->htext);
	}
	this->used++;
	lpgw->nGWOP++;
	return;
}

/* ================================== */

void WDPROC
GraphInit(LPGW lpgw)
{
	HMENU sysmenu;
	WNDCLASS wndclass;
	char buf[80];

	if (!lpgw->hPrevInstance) {
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = WndGraphProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 2 * sizeof(void FAR *);
		wndclass.hInstance = lpgw->hInstance;
		wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = GetStockBrush(WHITE_BRUSH);
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = szGraphClass;
		RegisterClass(&wndclass);
	}

	ReadGraphIni(lpgw);

	lpgw->hWndGraph = CreateWindow(szGraphClass, lpgw->Title,
		WS_OVERLAPPEDWINDOW,
		lpgw->Origin.x, lpgw->Origin.y,
		lpgw->Size.x, lpgw->Size.y,
		NULL, NULL, lpgw->hInstance, lpgw);

	lpgw->hPopMenu = CreatePopupMenu();
	AppendMenu(lpgw->hPopMenu, MF_STRING | (lpgw->graphtotop ? MF_CHECKED : MF_UNCHECKED), 
		M_GRAPH_TO_TOP, "Bring to &Top");
	AppendMenu(lpgw->hPopMenu, MF_STRING | (lpgw->color ? MF_CHECKED : MF_UNCHECKED), 
		M_COLOR, "C&olor");
	AppendMenu(lpgw->hPopMenu, MF_STRING, M_COPY_CLIP, "&Copy to Clipboard");
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

	ShowWindow(lpgw->hWndGraph, SW_SHOWNORMAL);
}

/* close a graph window */
void WDPROC
GraphClose(LPGW lpgw)
{
	/* close window */
	if (lpgw->hWndGraph)
		DestroyWindow(lpgw->hWndGraph);
	TextMessage();
	lpgw->hWndGraph = NULL;

	lpgw->locked = TRUE;
	DestroyBlocks(lpgw);
	lpgw->locked = FALSE;

}
	

void WDPROC
GraphStart(LPGW lpgw)
{
	lpgw->locked = TRUE;
	DestroyBlocks(lpgw);
	if ( !lpgw->hWndGraph || !IsWindow(lpgw->hWndGraph) )
		GraphInit(lpgw);
	if (IsIconic(lpgw->hWndGraph))
		ShowWindow(lpgw->hWndGraph, SW_SHOWNORMAL);
	if (lpgw->graphtotop)
		BringWindowToTop(lpgw->hWndGraph);
}
		
void WDPROC
GraphEnd(LPGW lpgw)
{
RECT rect;
	GetClientRect(lpgw->hWndGraph, &rect);
	InvalidateRect(lpgw->hWndGraph, (LPRECT) &rect, 1);
	lpgw->locked = FALSE;
	UpdateWindow(lpgw->hWndGraph);
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

void
StorePen(LPGW lpgw, int i, COLORREF ref, int colorstyle, int monostyle)
{
	LOGPEN FAR *plp;

	plp = &lpgw->colorpen[i];
	plp->lopnColor = ref;
	if (colorstyle < 0) {
		plp->lopnWidth.x = -colorstyle;
		plp->lopnStyle = 0;
	}
	else {
		plp->lopnWidth.x = 1;
		plp->lopnStyle = colorstyle % 5;
	}
	plp->lopnWidth.y = 0;

	plp = &lpgw->monopen[i];
	plp->lopnColor = RGB(0,0,0);
	if (monostyle < 0) {
		plp->lopnWidth.x = -monostyle;
			plp->lopnStyle = 0;
	}
	else {
		plp->lopnWidth.x = 1;
		plp->lopnStyle = monostyle % 5;
	}
	plp->lopnWidth.y = 0;
}

void
MakePens(LPGW lpgw, HDC hdc)
{
	int i;

	if ((GetDeviceCaps(hdc,NUMCOLORS) == 2) || !lpgw->color) {
		/* Monochrome Device */
		/* create border pens */
		lpgw->hbpen = CreatePenIndirect((LOGPEN FAR *)&lpgw->monopen[0]);	/* border */
		lpgw->hapen = CreatePenIndirect((LOGPEN FAR *)&lpgw->monopen[1]); 	/* axis */
		/* create drawing pens */
		for (i=0; i<WGNUMPENS; i++)
		{
			lpgw->hpen[i] = CreatePenIndirect((LOGPEN FAR *)&lpgw->monopen[i+2]);
			}
		/* find number of solid, unit width line styles */
		for (i=0; i<WGNUMPENS && lpgw->monopen[i+2].lopnStyle==PS_SOLID
			&& lpgw->monopen[i+2].lopnWidth.x==1; i++) ;
		lpgw->numsolid = i ? i : 1;	/* must be at least 1 */
		lpgw->hbrush = CreateSolidBrush(RGB(255,255,255));
	}
	else {
		/* Color Device */
		/* create border pens */
		lpgw->hbpen = CreatePenIndirect((LOGPEN FAR *)&lpgw->colorpen[0]);	/* border */
		lpgw->hapen = CreatePenIndirect((LOGPEN FAR *)&lpgw->colorpen[1]); 	/* axis */
		/* create drawing pens */
		for (i=0; i<WGNUMPENS; i++)
		{
			lpgw->hpen[i] = CreatePenIndirect((LOGPEN FAR *)&lpgw->colorpen[i+2]);
			}
		/* find number of solid, unit width line styles */
		for (i=0; i<WGNUMPENS && lpgw->colorpen[i+2].lopnStyle==PS_SOLID
			&& lpgw->colorpen[i+2].lopnWidth.x==1; i++) ;
		lpgw->numsolid = i ? i : 1;	/* must be at least 1 */
		lpgw->hbrush = CreateSolidBrush(lpgw->background);
	}
}

void
DestroyPens(LPGW lpgw)
{
	int i;

	DeleteBrush(lpgw->hbrush);
	DeletePen(lpgw->hbpen);
	DeletePen(lpgw->hapen);
	for (i=0; i<WGNUMPENS; i++)
		DeletePen(lpgw->hpen[i]);
}

/* ================================== */

void
MakeFonts(LPGW lpgw, LPRECT lprect, HDC hdc)
{
	LOGFONT lf;
	HFONT hfontold;
	TEXTMETRIC tm;
	int result;
	char FAR *p;
	int cx, cy;

	lpgw->rotate = FALSE;
	_fmemset(&lf, 0, sizeof(LOGFONT));
	_fstrncpy(lf.lfFaceName,lpgw->fontname,LF_FACESIZE);
	lf.lfHeight = -MulDiv(lpgw->fontsize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	lf.lfCharSet = DEFAULT_CHARSET;
	if ( (p = _fstrstr(lpgw->fontname," Italic")) != (LPSTR)NULL ) {
		lf.lfFaceName[ (unsigned int)(p-lpgw->fontname) ] = '\0';
		lf.lfItalic = TRUE;
	}
	if ( (p = _fstrstr(lpgw->fontname," Bold")) != (LPSTR)NULL ) {
		lf.lfFaceName[ (unsigned int)(p-lpgw->fontname) ] = '\0';
		lf.lfWeight = FW_BOLD;
	}

	if (lpgw->hfonth == 0) {
		lpgw->hfonth = CreateFontIndirect((LOGFONT FAR *)&lf);
	}

	if (lpgw->hfontv == 0) {
		lf.lfEscapement = 900;
		lf.lfOrientation = 900;
		lpgw->hfontv = CreateFontIndirect((LOGFONT FAR *)&lf);
	}

	/* save text size */
	hfontold = SelectFont(hdc, lpgw->hfonth);
#ifdef WIN32
	{
	SIZE size;
	GetTextExtentPoint(hdc,"0123456789",10, (LPSIZE)&size);
	cx = size.cx;
	cy = size.cy;
	}
#else
	{
	DWORD extent;
	extent = GetTextExtent(hdc,"0123456789",10);
	cx = LOWORD(extent);
	cy = HIWORD(extent);
	}
#endif
	lpgw->vchar = MulDiv(cy,lpgw->ymax,lprect->bottom - lprect->top);
	lpgw->hchar = MulDiv(cx/10,lpgw->xmax,lprect->right - lprect->left);
	/* find out if we can rotate text 90deg */
	SelectFont(hdc, lpgw->hfontv);
	result = GetDeviceCaps(hdc, TEXTCAPS);
	if ((result & TC_CR_90) || (result & TC_CR_ANY))
		lpgw->rotate = 1;
	GetTextMetrics(hdc,(TEXTMETRIC FAR *)&tm);
	if (tm.tmPitchAndFamily & TMPF_VECTOR)
		lpgw->rotate = 1;	/* vector fonts can all be rotated */
#if WINVER >=0x030a
	if (tm.tmPitchAndFamily & TMPF_TRUETYPE)
		lpgw->rotate = 1;	/* truetype fonts can all be rotated */
#endif
	SelectFont(hdc, hfontold);
	return;
}

void
DestroyFonts(LPGW lpgw)
{
	if (lpgw->hfonth) {
		DeleteFont(lpgw->hfonth);
		lpgw->hfonth = 0;
	}
	if (lpgw->hfontv) {
		DeleteFont(lpgw->hfontv);
		lpgw->hfontv = 0;
	}
	return;
}

void
SetFont(LPGW lpgw, HDC hdc)
{
	if (lpgw->rotate && lpgw->angle) {
		if (lpgw->hfontv)
			SelectFont(hdc, lpgw->hfontv);
	}
	else {
		if (lpgw->hfonth)
			SelectFont(hdc, lpgw->hfonth);
	}
	return;
}

void
SelFont(LPGW lpgw) {
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
	if ( (p = _fstrstr(lpgw->fontname," Bold")) != (LPSTR)NULL ) {
		_fstrncpy(lpszStyle,p+1,LF_FACESIZE);
		lf.lfFaceName[ (unsigned int)(p-lpgw->fontname) ] = '\0';
	}
	else if ( (p = _fstrstr(lpgw->fontname," Italic")) != (LPSTR)NULL ) {
		_fstrncpy(lpszStyle,p+1,LF_FACESIZE);
		lf.lfFaceName[ (unsigned int)(p-lpgw->fontname) ] = '\0';
	}
	else
		_fstrcpy(lpszStyle,"Regular");
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
		SendMessage(lpgw->hWndGraph,WM_COMMAND,M_REBUILDTOOLS,0L);
	}
#endif
}

/* ================================== */

void
drawgraph(LPGW lpgw, HDC hdc, LPRECT rect)
{
	int xdash, ydash;			/* the transformed coordinates */
	int rr, rl, rt, rb;
	struct GWOP FAR *curptr;
	struct GWOPBLK *blkptr;
	int htic, vtic, vshift;
	unsigned int lastop=-1;		/* used for plotting last point on a line */
	int pen, numsolid;
	int polymax = 200;
	int polyi = 0;
	POINT *ppt;
	unsigned int ngwop=0;
	BOOL isColor;

	if (lpgw->locked) 
		return;

 	isColor= GetDeviceCaps(hdc, NUMCOLORS) > 2;
	if (lpgw->background != RGB(255,255,255) && lpgw->color && isColor) {
		SetBkColor(hdc,lpgw->background);
		FillRect(hdc, rect, lpgw->hbrush);
	}

	ppt = (POINT *)LocalAllocPtr(LHND, (polymax+1) * sizeof(POINT));

	rr = rect->right;
	rl = rect->left;
	rt = rect->top;
	rb = rect->bottom;

	htic = MulDiv(lpgw->htic, rr-rl, lpgw->xmax) + 1;
	vtic = MulDiv(lpgw->vtic, rb-rt, lpgw->ymax) + 1;

	lpgw->angle = 0;
	SetFont(lpgw, hdc);
	SetTextAlign(hdc, TA_LEFT|TA_BOTTOM);
	vshift = MulDiv(lpgw->vchar, rb-rt, lpgw->ymax)/2;

	pen = 0;
	SelectPen(hdc, lpgw->hpen[pen]);
	numsolid = lpgw->numsolid;

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
	while(ngwop < lpgw->nGWOP)
   	{
		/* transform the coordinates */
		xdash = MulDiv(curptr->x, rr-rl-1, lpgw->xmax) + rl;
		ydash = MulDiv(curptr->y, rt-rb+1, lpgw->ymax) + rb - 1;
		if ((lastop==W_vect) && (curptr->op!=W_vect)) {
			if (polyi >= 2)
				Polyline(hdc, ppt, polyi);
			polyi = 0;
		}
		switch (curptr->op) {
			case 0:	/* have run past last in this block */
				break;
			case W_move:
				ppt[0].x = xdash;
				ppt[0].y = ydash;
				polyi = 1;;
				break;
			case W_vect:
				ppt[polyi].x = xdash;
				ppt[polyi].y = ydash;
				polyi++;
				if (polyi >= polymax) {
					Polyline(hdc, ppt, polyi);
					ppt[0].x = xdash;
					ppt[0].y = ydash;
					polyi = 1;;
				}
				break;
			case W_line_type:
				switch (curptr->x)
				{
				    case -2:		/* black 2 pixel wide */
					    SelectPen(hdc, lpgw->hbpen);
					    if (lpgw->color && isColor)
					        SetTextColor(hdc, lpgw->colorpen[0].lopnColor);
					    break;
				    case -1:		/* black 1 pixel wide doted */
					    SelectPen(hdc, lpgw->hapen);
					    if (lpgw->color && isColor)
					        SetTextColor(hdc, lpgw->colorpen[1].lopnColor);
					    break;
				    default:
					    SelectPen(hdc, lpgw->hpen[(curptr->x)%WGNUMPENS]);
					    if (lpgw->color && isColor)
					        SetTextColor(hdc, lpgw->colorpen[(curptr->x)%WGNUMPENS + 2].lopnColor);
				}
				pen = curptr->x;
				break;
			case W_put_text:
				{char *str;
				str = LocalLock(curptr->htext);
				if (str) {
					ydash += vshift;
					SetBkMode(hdc,TRANSPARENT);
					TextOut(hdc,xdash,ydash,str,lstrlen(str));
					SetBkMode(hdc,OPAQUE);
				}
				LocalUnlock(curptr->htext);
				}
				break;
			case W_text_angle:
				lpgw->angle = curptr->x;
				SetFont(lpgw,hdc);
				break;
			case W_justify:
				switch (curptr->x)
				{
					case LEFT:
						SetTextAlign(hdc, TA_LEFT|TA_BOTTOM);
						break;
					case RIGHT:
						SetTextAlign(hdc, TA_RIGHT|TA_BOTTOM);
						break;
					case CENTRE:
						SetTextAlign(hdc, TA_CENTER|TA_BOTTOM);
						break;
					}
				break;
			case W_dot:
				if (pen >= numsolid) {
					pen %= numsolid;	/* select solid pen */
					SelectPen(hdc, lpgw->hpen[pen]);
				}
				MoveTo(hdc,xdash,ydash);
				LineTo(hdc,xdash+1,ydash);
				break;
			case W_diamond: /* do diamond */ 
				if (pen >= numsolid) {
					pen %= numsolid;
					SelectPen(hdc, lpgw->hpen[pen]);
				}
				MoveTo(hdc,xdash-htic,ydash);
				LineTo(hdc,xdash,ydash-vtic);
				LineTo(hdc,xdash+htic,ydash);
				LineTo(hdc,xdash,ydash+vtic);
				LineTo(hdc,xdash-htic,ydash);
				MoveTo(hdc,xdash,ydash);
				LineTo(hdc,xdash+1,ydash);
				break;
			case W_plus: /* do plus */ 
				if (pen >= numsolid) {
					pen %= numsolid;
					SelectPen(hdc, lpgw->hpen[pen]);
				}
				MoveTo(hdc,xdash-htic,ydash);
				LineTo(hdc,xdash+htic+1,ydash);
				MoveTo(hdc,xdash,ydash-vtic);
				LineTo(hdc,xdash,ydash+vtic+1);
				break;
			case W_box: /* do box */ 
				if (pen >= numsolid) {
					pen %= numsolid;
					SelectPen(hdc, lpgw->hpen[pen]);
				}
				MoveTo(hdc,xdash-htic,ydash-vtic);
				LineTo(hdc,xdash+htic,ydash-vtic);
				LineTo(hdc,xdash+htic,ydash+vtic);
				LineTo(hdc,xdash-htic,ydash+vtic);
				LineTo(hdc,xdash-htic,ydash-vtic);
				MoveTo(hdc,xdash,ydash);
				LineTo(hdc,xdash+1,ydash);
				break;
			case W_cross: /* do X */ 
				if (pen >= numsolid) {
					pen %= numsolid;
					SelectPen(hdc, lpgw->hpen[pen]);
				}
				MoveTo(hdc,xdash-htic,ydash-vtic);
				LineTo(hdc,xdash+htic+1,ydash+vtic+1);
				MoveTo(hdc,xdash-htic,ydash+vtic);
				LineTo(hdc,xdash+htic+1,ydash-vtic-1);
				break;
			case W_triangle: /* do triangle */ 
				if (pen >= numsolid) {
					pen %= numsolid;
					SelectPen(hdc, lpgw->hpen[pen]);
				}
				MoveTo(hdc,xdash,ydash-(4*vtic/3));
				LineTo(hdc,xdash-(4*htic/3),ydash+(2*vtic/3));
				LineTo(hdc,xdash+(4*htic/3),ydash+(2*vtic/3));
				LineTo(hdc,xdash,ydash-(4*vtic/3));
				MoveTo(hdc,xdash,ydash);
				LineTo(hdc,xdash+1,ydash);
				break;
			case W_star: /* do star */ 
				if (pen >= numsolid) {
					pen %= numsolid;
					SelectPen(hdc, lpgw->hpen[pen]);
				}
				MoveTo(hdc,xdash-htic,ydash);
				LineTo(hdc,xdash+htic+1,ydash);
				MoveTo(hdc,xdash,ydash-vtic);
				LineTo(hdc,xdash,ydash+vtic+1);
				MoveTo(hdc,xdash-htic,ydash-vtic);
				LineTo(hdc,xdash+htic+1,ydash+vtic+1);
				MoveTo(hdc,xdash-htic,ydash+vtic);
				LineTo(hdc,xdash+htic+1,ydash-vtic-1);
				break;
		}
		lastop = curptr->op;
		ngwop++;
		curptr++;
		if ((unsigned)(curptr - blkptr->gwop) >= GWOPMAX) {
			GlobalUnlock(blkptr->hblk);
			blkptr->gwop = (struct GWOP FAR *)NULL;
			blkptr = blkptr->next;
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

/* copy graph window to clipboard */
void
CopyClip(LPGW lpgw)
{
	RECT rect;
	HDC mem;
	HBITMAP bitmap;
	HANDLE hmf;
	GLOBALHANDLE hGMem;
	LPMETAFILEPICT lpMFP;
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
	GetClientRect(hwnd, &rect);
	/* make a bitmap and copy it there */
	mem = CreateCompatibleDC(hdc);
	bitmap = CreateCompatibleBitmap(hdc, rect.right - rect.left,
			rect.bottom - rect.top);
	if (bitmap) {
		/* there is enough memory and the bitmaps OK */
		SelectBitmap(mem, bitmap);
		BitBlt(mem,0,0,rect.right - rect.left, 
			rect.bottom - rect.top, hdc, rect.left,
			rect.top, SRCCOPY);
	}
	else {
		MessageBeep(MB_ICONHAND);
		MessageBox(hwnd, "Insufficient Memory to Copy Clipboard", 
			lpgw->Title, MB_ICONHAND | MB_OK);
	}
	DeleteDC(mem);
	ReleaseDC(hwnd, hdc);

	hdc = CreateMetaFile((LPSTR)NULL);
	SetMapMode(hdc, MM_ANISOTROPIC);
#ifdef WIN32
	SetWindowExtEx(hdc, rect.right, rect.bottom, (LPSIZE)NULL);
#else
	SetWindowExt(hdc, rect.right, rect.bottom);
#endif
	drawgraph(lpgw, hdc, (void *) &rect);
	hmf = CloseMetaFile(hdc);

	hGMem = GlobalAlloc(GMEM_MOVEABLE, (DWORD)sizeof(METAFILEPICT));
	lpMFP = (LPMETAFILEPICT) GlobalLock(hGMem);
	hdc = GetDC(hwnd);	/* get window size */
	GetClientRect(hwnd, &rect);
	/* in MM_ANISOTROPIC, xExt & yExt give suggested size in 0.01mm units */
	lpMFP->mm = MM_ANISOTROPIC;
	lpMFP->xExt = MulDiv(rect.right-rect.left, 2540, GetDeviceCaps(hdc, LOGPIXELSX));
	lpMFP->yExt = MulDiv(rect.bottom-rect.top, 2540, GetDeviceCaps(hdc, LOGPIXELSX));
	lpMFP->hMF = hmf;
	ReleaseDC(hwnd, hdc);
	GlobalUnlock(hGMem);

	OpenClipboard(hwnd);
	EmptyClipboard();
	SetClipboardData(CF_METAFILEPICT,hGMem);
	SetClipboardData(CF_BITMAP, bitmap);
	CloseClipboard();
	return;
}

/* copy graph window to printer */
void
CopyPrint(LPGW lpgw)
{
#if WINVER >= 0x030a
	HDC printer;
	DLGPROC lpfnAbortProc;
	DLGPROC lpfnPrintDlgProc;
	PRINTDLG pd;
	HWND hwnd;
	RECT rect;
	PRINT pr;
	UINT widabort;

	hwnd = lpgw->hWndGraph;

	_fmemset(&pd, 0, sizeof(PRINTDLG));
	pd.lStructSize = sizeof(PRINTDLG);
	pd.hwndOwner = hwnd;
	pd.Flags = PD_PRINTSETUP | PD_RETURNDC;

	if (!PrintDlg(&pd))
		return;
	printer = pd.hDC;
	if (NULL == printer)
		return;	/* abort */

	if (!PrintSize(printer, hwnd, &rect)) {
		DeleteDC(printer);
		return; /* abort */
	}

	pr.hdcPrn = printer;
	SetWindowLong(hwnd, 4, (LONG)((LPPRINT)&pr));
	PrintRegister((LPPRINT)&pr);

	EnableWindow(hwnd,FALSE);
	pr.bUserAbort = FALSE;
#ifdef __DLL__
	lpfnPrintDlgProc = (DLGPROC)GetProcAddress(hdllInstance, "PrintDlgProc");
	lpfnAbortProc = (DLGPROC)GetProcAddress(hdllInstance, "PrintAbortProc");
#else
	lpfnPrintDlgProc = (DLGPROC)MakeProcInstance((FARPROC)PrintDlgProc, hdllInstance);
	lpfnAbortProc = (DLGPROC)MakeProcInstance((FARPROC)PrintAbortProc, hdllInstance);
#endif
	pr.hDlgPrint = CreateDialogParam(hdllInstance,"PrintDlgBox",hwnd,lpfnPrintDlgProc,(LPARAM)lpgw->Title);
	Escape(printer,SETABORTPROC,0,(LPSTR)lpfnAbortProc,NULL);  
	if (Escape(printer, STARTDOC, lstrlen(lpgw->Title),lpgw->Title, NULL) > 0) {
		SetMapMode(printer, MM_TEXT);
		SetBkMode(printer,OPAQUE);
		DestroyFonts(lpgw);
		MakeFonts(lpgw, (RECT FAR *)&rect, printer);
		DestroyPens(lpgw);	/* rebuild pens */
		MakePens(lpgw, printer);
		drawgraph(lpgw, printer, (void *) &rect);
		if (Escape(printer,NEWFRAME,0,NULL,NULL) > 0)
			Escape(printer,ENDDOC,0,NULL,NULL);
	}
	if (!pr.bUserAbort) {
		EnableWindow(hwnd,TRUE);
		DestroyWindow(pr.hDlgPrint);
	}
#ifndef __DLL__
	FreeProcInstance((FARPROC)lpfnPrintDlgProc);
	FreeProcInstance((FARPROC)lpfnAbortProc);
#endif
	DeleteDC(printer);
	SetWindowLong(hwnd, 4, (LONG)(0L));
	PrintUnregister((LPPRINT)&pr);
	/* make certain that the screen pen set is restored */
	SendMessage(lpgw->hWndGraph,WM_COMMAND,M_REBUILDTOOLS,0L);
#endif
	return;
}

/* ================================== */
/*  INI file stuff */
void
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
	wsprintf(profile, "%s,%d", lpgw->fontname, lpgw->fontsize);
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
	if ( (p = GetInt(profile, &lpgw->Origin.x)) == NULL)
		lpgw->Origin.x = CW_USEDEFAULT;
	if ( (p = GetInt(p, &lpgw->Origin.y)) == NULL)
		lpgw->Origin.y = CW_USEDEFAULT;
	if (bOKINI)
	  GetPrivateProfileString(section, "GraphSize", "", profile, 80, file);
	if ( (p = GetInt(profile, &lpgw->Size.x)) == NULL)
		lpgw->Size.x = CW_USEDEFAULT;
	if ( (p = GetInt(p, &lpgw->Size.y)) == NULL)
		lpgw->Size.y = CW_USEDEFAULT;

	if (bOKINI)
	  GetPrivateProfileString(section, "GraphFont", "", profile, 80, file);
	{
		char FAR *size;
		size = _fstrchr(profile,',');
		if (size) {
			*size++ = '\0';
			if ( (p = GetInt(size, &lpgw->fontsize)) == NULL)
				lpgw->fontsize = WINFONTSIZE;
		}
		_fstrcpy(lpgw->fontname, profile);
		if (lpgw->fontsize == 0)
			lpgw->fontsize = WINFONTSIZE;
		if (!(*lpgw->fontname))
			if (LOWORD(GetVersion()) == 3)
				_fstrcpy(lpgw->fontname,WIN30FONT);
			else
				_fstrcpy(lpgw->fontname,WINFONT);
	}

	if (bOKINI)
	  GetPrivateProfileString(section, "GraphColor", "", profile, 80, file);
		if ( (p = GetInt(profile, &lpgw->color)) == NULL)
			lpgw->color = TRUE;

	if (bOKINI)
	  GetPrivateProfileString(section, "GraphToTop", "", profile, 80, file);
		if ( (p = GetInt(profile, &lpgw->graphtotop)) == NULL)
			lpgw->graphtotop = TRUE;

	lpgw->background = RGB(255,255,255);
	if (bOKINI)
	  GetPrivateProfileString(section, "GraphBackground", "", profile, 80, file);
	if ( ((p = GetInt(profile, &r)) != NULL) &&
	     ((p = GetInt(p, &g)) != NULL) &&
	     ((p = GetInt(p, &b)) != NULL) )
			lpgw->background = RGB(r,g,b);

	StorePen(lpgw, 0,RGB(0,0,0),PS_SOLID,PS_SOLID);
	if (bOKINI)
	  GetPrivateProfileString(section, "Border", "", profile, 80, file);
	if ( ((p = GetInt(profile, &r)) != NULL) &&
	     ((p = GetInt(p, &g)) != NULL) &&
	     ((p = GetInt(p, &b)) != NULL) &&
	     ((p = GetInt(p, &colorstyle)) != NULL) &&
	     ((p = GetInt(p, &monostyle)) != NULL) )
			StorePen(lpgw,0,RGB(r,g,b),colorstyle,monostyle);

	StorePen(lpgw, 1,RGB(192,192,192),PS_DOT,PS_DOT);
	if (bOKINI)
	  GetPrivateProfileString(section, "Axis", "", profile, 80, file);
	if ( ((p = GetInt(profile, &r)) != NULL) &&
	     ((p = GetInt(p, &g)) != NULL) &&
	     ((p = GetInt(p, &b)) != NULL) &&
	     ((p = GetInt(p, &colorstyle)) != NULL) &&
	     ((p = GetInt(p, &monostyle)) != NULL) )
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
		if ( ((p = GetInt(profile, &r)) != NULL) &&
		     ((p = GetInt(p, &g)) != NULL) &&
		     ((p = GetInt(p, &b)) != NULL) &&
		     ((p = GetInt(p, &colorstyle)) != NULL) &&
		     ((p = GetInt(p, &monostyle)) != NULL) )
				StorePen(lpgw,i+2,RGB(r,g,b),colorstyle,monostyle);
	}
}


/* ================================== */

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
	

COLORREF
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
void
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

BOOL CALLBACK _export
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
					GetInt(buf, &plpm->lopnWidth.x);
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
					GetInt(buf, &plpc->lopnWidth.x);
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
			FrameRect(lpdis->hDC, &lpdis->rcItem, GetStockBrush(BLACK_BRUSH));
			DeleteBrush(hBrush);
			}
			return FALSE;
	}
	return FALSE;
}



/* GetWindowLong(hwnd, 4) must be available for use */
BOOL
LineStyle(LPGW lpgw)
{
DLGPROC lpfnLineStyleDlgProc ;
BOOL status = FALSE;
LS ls;
	
	SetWindowLong(lpgw->hWndGraph, 4, (LONG)((LPLS)&ls));
	_fmemcpy(&ls.colorpen, &lpgw->colorpen, (WGNUMPENS + 2) * sizeof(LOGPEN));
	_fmemcpy(&ls.monopen, &lpgw->monopen, (WGNUMPENS + 2) * sizeof(LOGPEN));

#ifdef __DLL__
	lpfnLineStyleDlgProc = (DLGPROC)GetProcAddress(hdllInstance, "LineStyleDlgProc");
#else
	lpfnLineStyleDlgProc = (DLGPROC)MakeProcInstance((FARPROC)LineStyleDlgProc, hdllInstance);
#endif
	if (DialogBox (hdllInstance, "LineStyleDlgBox", lpgw->hWndGraph, lpfnLineStyleDlgProc)
		== IDOK) {
		_fmemcpy(&lpgw->colorpen, &ls.colorpen, (WGNUMPENS + 2) * sizeof(LOGPEN));
		_fmemcpy(&lpgw->monopen, &ls.monopen, (WGNUMPENS + 2) * sizeof(LOGPEN));
		status = TRUE;
	}
#ifndef __DLL__
	FreeProcInstance((FARPROC)lpfnLineStyleDlgProc);
#endif
	SetWindowLong(lpgw->hWndGraph, 4, (LONG)(0L));
	return status;
}

/* ================================== */

LRESULT CALLBACK _export
WndGraphProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	LPGW lpgw;

	lpgw = (LPGW)GetWindowLong(hwnd, 0);

	switch(message)
	{
		case WM_SYSCOMMAND:
			switch(LOWORD(wParam))
			{
				case M_GRAPH_TO_TOP:
				case M_COLOR:
				case M_CHOOSE_FONT:
				case M_COPY_CLIP:
				case M_LINESTYLE:
				case M_PRINT:
				case M_WRITEINI:
				case M_REBUILDTOOLS:
					SendMessage(hwnd, WM_COMMAND, wParam, lParam);
					break;
				case M_ABOUT:
					if (lpgw->lptw)
						AboutBox(hwnd,lpgw->lptw->AboutText);
					return 0;
			}
			break;
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
					GetClientRect(hwnd, &rect);
					MakeFonts(lpgw, (LPRECT)&rect, hdc);
					ReleaseDC(hwnd, hdc);
					GetClientRect(hwnd, &rect);
					InvalidateRect(hwnd, (LPRECT) &rect, 1);
					UpdateWindow(hwnd);
					return 0;
			}
			return 0;
		case WM_RBUTTONDOWN:
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
			GetClientRect(hwnd, &rect);
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
			GetClientRect(hwnd, &rect);
			SetViewportExt(hdc, rect.right, rect.bottom);
			drawgraph(lpgw, hdc, (void *) &rect);
			EndPaint(hwnd, &ps);
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
#if __TURBOC__ >= 0x410    /* Borland C++ 3.1 or later */
			{
			WORD version = LOWORD(GetVersion());
			if ((LOBYTE(version)*100 + HIBYTE(version)) >= 310)
				DragAcceptFiles(hwnd, FALSE);
			}
#endif
			return 0;
		case WM_CLOSE:
			GraphClose(lpgw);
			return 0;
		}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

