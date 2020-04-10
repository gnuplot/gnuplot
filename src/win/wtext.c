/* GNUPLOT - win/wtext.c */
/*[
 * Copyright 1992, 1993, 1998, 2004   Russell Lang
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

/* WARNING: Do not write to stdout/stderr with functions not listed
   in win/wtext.h */

#include "syscfg.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <dos.h>
#include <tchar.h>
#include <wchar.h>

#include "wgnuplib.h"
#include "winmain.h"
#include "wresourc.h"
#include "wcommon.h"
#include "stdfn.h"
#include "plot.h"
#include "util.h"

/* font stuff */
#define TEXTFONTSIZE 9

#ifndef WGP_CONSOLE

enum docked_layout
{
    DOCKED_LAYOUT_NONE, DOCKED_LAYOUT_HORIZONTAL, DOCKED_LAYOUT_VERTICAL
};

/* limits */
static POINT ScreenMinSize = {16,4};

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WndParentProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WndToolbarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WndSeparatorProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WndTextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void CreateTextClass(LPTW lptw);
static void TextToCursor(LPTW lptw);
static void NewLine(LPTW lptw);
static void UpdateScrollBars(LPTW lptw);
static void UpdateText(LPTW, int);
static void TextPutStr(LPTW lptw, LPSTR str);
static void LimitMark(LPTW lptw, POINT *lppt);
static void ClearMark(LPTW lptw, POINT pt);
static void DoLine(LPTW lptw, HDC hdc, int xpos, int ypos, int x, int y, int count);
static void DoMark(LPTW lptw, POINT pt, POINT end, BOOL mark);
static void UpdateMark(LPTW lptw, POINT pt);
static void TextCopyClip(LPTW lptw);
static void TextMakeFont(LPTW lptw);
static void TextSelectFont(LPTW lptw);
static int ReallocateKeyBuf(LPTW lptw);
static void UpdateCaretPos(LPTW lptw);
static LPTSTR GetUInt(LPTSTR str, uint *pval);
static enum docked_layout DockedLayout(LPTW lptw);
static unsigned NumberOfDockedWindows(LPTW lptw);
static void ApplyLayout(LPTW lptw, HWND hwnd, unsigned width, unsigned height);

static TCHAR szNoMemory[] = TEXT("out of memory");

static const COLORREF TextColorTable[16] = {
	RGB(0,0,0),		/* black */
	RGB(0,0,128),		/* dark blue */
	RGB(0,128,0),		/* dark green */
	RGB(0,128,128),		/* dark cyan */
	RGB(128,0,0),		/* dark red */
	RGB(128,0,128),		/* dark magenta */
	RGB(128,128,0),		/* dark yellow */
	RGB(128,128,128),	/* dark grey */
	RGB(192,192,192),	/* light grey */
	RGB(0,0,255),		/* blue */
	RGB(0,255,0),		/* green */
	RGB(0,255,255),		/* cyan */
	RGB(255,0,0),		/* red */
	RGB(255,0,255),		/* magenta */
	RGB(255,255,0),		/* yellow */
	RGB(255,255,255),	/* white */
};
#define NOTEXT 0xF0 /* black on white */
#define MARKFORE RGB(255,255,255)
#define MARKBACK RGB(0,0,128)
#define TextFore(attr) TextColorTable[(attr     ) & 0x0f]
#define TextBack(attr) TextColorTable[(attr >> 4) & 0x0f]


void
TextMessage()
{
    WinMessageLoop();
}


void
CreateTextClass(LPTW lptw)
{
    /* We deliberately call the "W" API variant in order to
       to receive UTF16 WM_CHAR messages. */
    WNDCLASSW wndclass;

    hdllInstance = lptw->hInstance;
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndTextProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 2 * sizeof(void *);
    wndclass.hInstance = lptw->hInstance;
    wndclass.hIcon = NULL;
    wndclass.hCursor = NULL;
    wndclass.hbrBackground = NULL;
    lptw->hbrBackground = CreateSolidBrush(lptw->bSysColors ?
					   GetSysColor(COLOR_WINDOW) : RGB(255,255,255));
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szTextClass;
    RegisterClassW(&wndclass);

    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndToolbarProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 2 * sizeof(void *);
    wndclass.hInstance = lptw->hInstance;
    wndclass.hIcon = NULL;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szToolbarClass;
    RegisterClassW(&wndclass);

    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndSeparatorProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 2 * sizeof(void *);
    wndclass.hInstance = lptw->hInstance;
    wndclass.hIcon = NULL;
    wndclass.hCursor = NULL;
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szSeparatorClass;
    RegisterClassW(&wndclass);

    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndParentProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 2 * sizeof(void *);
    wndclass.hInstance = lptw->hInstance;
    if (lptw->hIcon)
	wndclass.hIcon = lptw->hIcon;
    else
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szParentClass;
    RegisterClassW(&wndclass);
}


/* make text window */
int
TextInit(LPTW lptw)
{
    RECT rect;
    HMENU sysmenu;
    HGLOBAL hglobal;
    LB lb;

    ReadTextIni(lptw);

    if (!lptw->hPrevInstance)
	CreateTextClass(lptw);

    if (lptw->KeyBufSize == 0)
	lptw->KeyBufSize = 256;

    if (lptw->ScreenSize.x < ScreenMinSize.x)
	lptw->ScreenSize.x = ScreenMinSize.x;
    if (lptw->ScreenSize.y < ScreenMinSize.y)
	lptw->ScreenSize.y = ScreenMinSize.y;

    lptw->CursorPos.x = 0;
    lptw->CursorPos.y = 0;
    lptw->bFocus = FALSE;
    lptw->bGetCh = FALSE;
    lptw->CaretHeight = 0;
    if (!lptw->nCmdShow)
	lptw->nCmdShow = SW_SHOWNORMAL;
    if (!lptw->Attr)
	lptw->Attr = NOTEXT;

    /* init ScreenBuffer, add empty line buffer,
       initial size has already been read from wgnuplot.ini
    */
    sb_init(&(lptw->ScreenBuffer), lptw->ScreenBuffer.size);
    lb_init(&lb);
    lb_set_attr(&lb, NOTEXT);
    sb_append(&(lptw->ScreenBuffer), &lb);

    hglobal = GlobalAlloc(LHND, lptw->KeyBufSize);
    lptw->KeyBuf = (BYTE *)GlobalLock(hglobal);
    if (lptw->KeyBuf == (BYTE *)NULL) {
	MessageBox(NULL, szNoMemory, NULL, MB_ICONHAND | MB_SYSTEMMODAL);
	return 1;
    }
    lptw->KeyBufIn = lptw->KeyBufOut = lptw->KeyBuf;

    lptw->hWndParent = CreateWindowW(szParentClass, lptw->Title,
				    WS_OVERLAPPEDWINDOW,
				    lptw->Origin.x, lptw->Origin.y,
				    lptw->Size.x, lptw->Size.y,
				    NULL, NULL, lptw->hInstance, lptw);
    if (lptw->hWndParent == NULL) {
	MessageBox(NULL, TEXT("Couldn't open parent text window"), NULL, MB_ICONHAND | MB_SYSTEMMODAL);
	return 1;
    }
    GetClientRect(lptw->hWndParent, &rect);

    lptw->hWndToolbar = CreateWindowW(szToolbarClass, L"gnuplot toolbar",
				      WS_CHILD,
				     0, 0,
				     rect.right, rect.bottom,
				     lptw->hWndParent, NULL, lptw->hInstance, lptw);
    if (lptw->hWndToolbar == NULL) {
	MessageBox(NULL, TEXT("Couldn't open toolbar window"), NULL, MB_ICONHAND | MB_SYSTEMMODAL);
	return 1;
    }

    lptw->hWndSeparator = CreateWindowW(szSeparatorClass, L"gnuplot separator",
				      WS_CHILD,
				      rect.right, rect.top,
				      rect.right, rect.top,
				      lptw->hWndParent, NULL, lptw->hInstance, lptw);
    if (lptw->hWndSeparator == NULL) {
	MessageBox(NULL, TEXT("Couldn't open separator window"), NULL, MB_ICONHAND | MB_SYSTEMMODAL);
	return 1;
    }

    lptw->hWndText = CreateWindowW(szTextClass, lptw->Title,
				  WS_CHILD | WS_VSCROLL | WS_HSCROLL,
				  0, lptw->ButtonHeight,
				  rect.right, rect.bottom - lptw->ButtonHeight,
				  lptw->hWndParent, NULL, lptw->hInstance, lptw);
    if (lptw->hWndText == NULL) {
	MessageBox(NULL, TEXT("Couldn't open text window"), NULL, MB_ICONHAND | MB_SYSTEMMODAL);
	return 1;
    }

    lptw->hStatusbar = CreateWindowEx(0, STATUSCLASSNAME, NULL,
				  WS_CHILD | SBARS_SIZEGRIP,
				  0, 0, 0, 0,
				  lptw->hWndParent, (HMENU)ID_TEXTSTATUS,
				  lptw->hInstance, lptw);
    if (lptw->hStatusbar) {
	RECT rect;
	/* reserve an extra slot for docked graph windows */
	int edges[2] = { 200, -1 };
	SendMessage(lptw->hStatusbar, SB_SETPARTS, (WPARAM)2, (LPARAM)&edges);

	/* auto-adjust size */
	SendMessage(lptw->hStatusbar, WM_SIZE, (WPARAM)0, (LPARAM)0);

	/* make room */
	GetClientRect(lptw->hStatusbar, &rect);
	lptw->StatusHeight = rect.bottom - rect.top;
	GetClientRect(lptw->hWndParent, &rect);
	SetWindowPos(lptw->hWndText, (HWND)NULL, 0, 0,
			rect.right, rect.bottom - lptw->StatusHeight,
			SWP_NOZORDER | SWP_NOACTIVATE);
	ShowWindow(lptw->hStatusbar, SW_SHOWNOACTIVATE);
    }

    lptw->hPopMenu = CreatePopupMenu();
    AppendMenu(lptw->hPopMenu, MF_STRING, M_COPY_CLIP, TEXT("&Copy to Clipboard\tCtrl-Ins"));
    AppendMenu(lptw->hPopMenu, MF_STRING, M_PASTE, TEXT("&Paste\tShift-Ins"));
    AppendMenu(lptw->hPopMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(lptw->hPopMenu, MF_STRING, M_CHOOSE_FONT, TEXT("Choose &Font..."));
    AppendMenu(lptw->hPopMenu, MF_STRING | (lptw->bSysColors ? MF_CHECKED : MF_UNCHECKED),
	       M_SYSCOLORS, TEXT("&System Colors"));
    AppendMenu(lptw->hPopMenu, MF_STRING | (lptw->bWrap ? MF_CHECKED : MF_UNCHECKED),
	       M_WRAP, TEXT("&Wrap long lines"));
    if (lptw->IniFile != NULL) {
	TCHAR buf[MAX_PATH+80];
	wsprintf(buf, TEXT("&Update %s"), lptw->IniFile);
	AppendMenu(lptw->hPopMenu, MF_STRING, M_WRITEINI, buf);
    }

    sysmenu = GetSystemMenu(lptw->hWndParent,0);	/* get the sysmenu */
    AppendMenu(sysmenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(sysmenu, MF_POPUP, (UINT_PTR)lptw->hPopMenu, TEXT("&Options"));
    AppendMenu(sysmenu, MF_STRING, M_ABOUT, TEXT("&About"));

    if (lptw->lpmw != NULL)
	LoadMacros(lptw);

    ShowWindow(lptw->hWndText, SW_SHOWNORMAL);
    ShowWindow(lptw->hWndToolbar, SW_SHOWNOACTIVATE);
    BringWindowToTop(lptw->hWndText);
    SetFocus(lptw->hWndText);
    TextMessage();
    return 0;
}


/* close a text window */
void
TextClose(LPTW lptw)
{
    HGLOBAL hglobal;

    /* close window */
    if (lptw->hWndParent)
	DestroyWindow(lptw->hWndParent);
    TextMessage();

    /* free the screen buffer */
    sb_free(&(lptw->ScreenBuffer));

    hglobal = (HGLOBAL)GlobalHandle(lptw->KeyBuf);
    if (hglobal) {
	GlobalUnlock(hglobal);
	GlobalFree(hglobal);
    }

    if (lptw->lpmw != NULL)
	CloseMacros(lptw);
    lptw->hWndParent = (HWND)NULL;
}


/* Bring the text window to front */
void
TextShow(LPTW lptw)
{
	ShowWindow(textwin.hWndParent, textwin.nCmdShow);
	ShowWindow(lptw->hWndText, SW_SHOWNORMAL);
	BringWindowToTop(lptw->hWndText);
	SetFocus(lptw->hWndText);
}


/* Bring Cursor into text window */
static void
TextToCursor(LPTW lptw)
{
    int nXinc=0;
    int nYinc=0;
    int cxCursor;
    int cyCursor;

    if (lptw->bWrap)
	cyCursor = (lptw->CursorPos.y + (lptw->CursorPos.x / lptw->ScreenBuffer.wrap_at)) * lptw->CharSize.y;
    else
	cyCursor = lptw->CursorPos.y * lptw->CharSize.y;
    if ((cyCursor + lptw->CharSize.y > lptw->ScrollPos.y + lptw->ClientSize.y)
	 || (cyCursor < lptw->ScrollPos.y)) {
	nYinc = max(0, cyCursor + lptw->CharSize.y - lptw->ClientSize.y) - lptw->ScrollPos.y;
	nYinc = min(nYinc, lptw->ScrollMax.y - lptw->ScrollPos.y);
    }
    if (lptw->bWrap)
	cxCursor = (lptw->CursorPos.x % lptw->ScreenBuffer.wrap_at) * lptw->CharSize.x;
    else
	cxCursor = lptw->CursorPos.x * lptw->CharSize.x;
    if ((cxCursor + lptw->CharSize.x > lptw->ScrollPos.x + lptw->ClientSize.x)
	 || (cxCursor < lptw->ScrollPos.x)) {
	nXinc = max(0, cxCursor + lptw->CharSize.x - lptw->ClientSize.x/2) - lptw->ScrollPos.x;
	nXinc = min(nXinc, lptw->ScrollMax.x - lptw->ScrollPos.x);
    }
    if (nYinc || nXinc) {
	lptw->ScrollPos.y += nYinc;
	lptw->ScrollPos.x += nXinc;
	ScrollWindow(lptw->hWndText, -nXinc, -nYinc, NULL, NULL);
	SetScrollPos(lptw->hWndText, SB_VERT, lptw->ScrollPos.y, TRUE);
	SetScrollPos(lptw->hWndText, SB_HORZ, lptw->ScrollPos.x, TRUE);
	UpdateWindow(lptw->hWndText);
    }
}


void
NewLine(LPTW lptw)
{
    LB lb;
    LPLB lplb;
    int ycorr;
    int last_lines;

    /* append an empty line buffer,
       dismiss previous lines if necessary */
    lplb = sb_get_last(&(lptw->ScreenBuffer));
    lb_set_attr(lplb, NOTEXT);
    lb_init(&lb);
    lb_set_attr(&lb, NOTEXT);
    /* return value is the number of lines which got dismissed */
    ycorr = sb_append(&(lptw->ScreenBuffer), &lb);

    last_lines = sb_lines(&(lptw->ScreenBuffer), lplb);
    lptw->CursorPos.x = 0;
    lptw->CursorPos.y += last_lines - ycorr;

    /* did we dismiss some lines ? */
    if (ycorr != 0) {
	if (ycorr > 1)
	    ycorr = ycorr +1 -1;
	/* make room for new last line */
	ScrollWindow(lptw->hWndText, 0, - last_lines * lptw->CharSize.y, NULL, NULL);
	lptw->ScrollPos.y -= (ycorr - last_lines) * lptw->CharSize.y;
	lptw->MarkBegin.y -= ycorr;
	lptw->MarkEnd.y -= ycorr;
	LimitMark(lptw, &lptw->MarkBegin);
	LimitMark(lptw, &lptw->MarkEnd);
	UpdateWindow(lptw->hWndText);
    }

    /* maximum line size may have changed, so update scroll bars */
    UpdateScrollBars(lptw);

    if (lptw->bFocus && lptw->bGetCh) {
	UpdateCaretPos(lptw);
	ShowCaret(lptw->hWndText);
    }

    if (lptw->CursorFlag)
	TextToCursor(lptw);
    TextMessage();
}


static void
UpdateScrollBars(LPTW lptw)
{
    signed int length;  /* this must be signed for this to work! */
    SCROLLINFO si;

    /* horizontal scroll bar */
    length = sb_max_line_length(&(lptw->ScreenBuffer)) + 1;
    if (length > lptw->ScreenSize.x) {
	/* maximum horizontal scroll position is given by maximum line length */
	lptw->ScrollMax.x = max(0, lptw->CharSize.x * length - lptw->ClientSize.x);
	lptw->ScrollPos.x = min(lptw->ScrollPos.x, lptw->ScrollMax.x);

	/* update scroll bar page size, range and position */
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
	si.nPage = lptw->ClientSize.x;
	si.nMin = 0;
	/* The maximum reported scroll position will be (nMax - (nPage - 1)),
	   so we need to set nMax to the full range. */
	si.nMax = lptw->CharSize.x * length;
	si.nPos = lptw->ScrollPos.x;
	SetScrollInfo(lptw->hWndText, SB_HORZ, &si, TRUE);
	ShowScrollBar(lptw->hWndText, SB_HORZ, TRUE);
    } else {
	lptw->ScrollMax.x = 0;
	lptw->ScrollPos.x = 0;
	ShowScrollBar(lptw->hWndText, SB_HORZ, FALSE);
    }

    /* vertical scroll bar */
    length = sb_length(&(lptw->ScreenBuffer));
    if (length >= lptw->ScreenSize.y) {
	lptw->ScrollMax.y = max(0, lptw->CharSize.y * length - lptw->ClientSize.y);
	lptw->ScrollPos.y = min(lptw->ScrollPos.y, lptw->ScrollMax.y);

	/* update scroll bar page size, range and position */
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
	si.nPage = lptw->ClientSize.y;
	si.nMin = 0;
	/* The maximum reported scroll position will be (nMax - (nPage - 1)),
	   so we need to set nMax to the full range. */
	si.nMax = lptw->CharSize.y * length;
	si.nPos = lptw->ScrollPos.y;
	SetScrollInfo(lptw->hWndText, SB_VERT, &si, TRUE);
	ShowScrollBar(lptw->hWndText, SB_VERT, TRUE);
    } else {
	lptw->ScrollMax.y = 0;
	lptw->ScrollPos.y = 0;
	ShowScrollBar(lptw->hWndText, SB_VERT, FALSE);
    }
}


/* Update count characters in window at cursor position */
/* Updates cursor position */
static void
UpdateText(LPTW lptw, int count)
{
    HDC hdc;

    if (lptw->bSuspend > 0) {
	/* track cursor position only */
	lptw->CursorPos.x += count;
	/* track maximum cursor position */
	if (lptw->CursorPos.x > lptw->MaxCursorPos)
	    lptw->MaxCursorPos = lptw->CursorPos.x;
	return;
    }

    if (lptw->CursorPos.x + count > lptw->ScreenSize.x)
	UpdateScrollBars(lptw);

    hdc = GetDC(lptw->hWndText);
    if (lptw->bSysColors) {
	SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
	SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
    } else {
	SetTextColor(hdc, TextFore(lptw->Attr));
	SetBkColor(hdc, TextBack(lptw->Attr));
    }
    SelectObject(hdc, lptw->hfont);

    if (lptw->bWrap) {
	int n, ypos, yofs, width;
	uint x = lptw->CursorPos.x;
	uint y = lptw->CursorPos.y;

	/* Always draw complete lines to avoid character overlap
	   when using Cleartype. */
	/* When we do not need to draw everything because we use suspend/resume,
	   we only would need to (re)draw the last one or two lines.
	 */
	width = lptw->ScreenBuffer.wrap_at;
	if (count == 0) { /* redraw all */
	    yofs = 0;
	    n = x / width + 1;
	} else {
	    yofs = x / width;
	    n    = (x + count - 1) / width + 1 - yofs; /* number of lines */
	}
	for (; n > 0; y++, n--) {
	    ypos = (y + yofs) * lptw->CharSize.y - lptw->ScrollPos.y;
	    DoLine(lptw, hdc, 0, ypos, 0, y + yofs, width);
	}
    } else {
	LPLB lb;
	LPWSTR wstr;
	int width, ypos;

	lb = sb_get_last(&(lptw->ScreenBuffer));
	width = lptw->ScreenSize.x + 1;
	wstr = lb_substr(lb, lptw->ScrollPos.x / lptw->CharSize.x, width);
	ypos = lptw->CursorPos.y * lptw->CharSize.y - lptw->ScrollPos.y;
	if (ypos > 0)
	    TextOutW(hdc, 0, ypos, wstr, width);
	free(wstr);
    }
    lptw->CursorPos.x += count;
    ReleaseDC(lptw->hWndText, hdc);
}


void
TextSuspend(LPTW lptw)
{
    lptw->bSuspend++;
}


void
TextResume(LPTW lptw)
{
    lptw->bSuspend--;

    if (lptw->bSuspend == 0) {
	int cursor = lptw->CursorPos.x;
	lptw->CursorPos.x = lptw->MaxCursorPos;
	UpdateText(lptw, 0);
	lptw->CursorPos.x = cursor;
	UpdateScrollBars(lptw);
	TextToCursor(lptw);
	lptw->MaxCursorPos = 0;
    }
}


int
TextPutCh(LPTW lptw, BYTE ch)
{
    WCHAR w[2];
    int count = 0;

    MultiByteAccumulate(ch, w, &count);
    if (count == 1) {
	/* FIXME: we only handle UCS-2: one double-byte only */
	TextPutChW(lptw, w[0]);
    }
    return ch;
}


int
TextPutChW(LPTW lptw, WCHAR ch)
{
    switch (ch) {
	case '\r':
	    lptw->CursorPos.x = 0;
	    if (lptw->CursorFlag)
		TextToCursor(lptw);
	    break;
	case '\n':
	    NewLine(lptw);
	    break;
	case 7:
	    MessageBeep(0xFFFFFFFF);
	    if (lptw->CursorFlag)
		TextToCursor(lptw);
	    break;
	case '\t': {
	    uint tab = 8 - (lptw->CursorPos.x  % 8);
	    lb_set_attr(sb_get_last(&(lptw->ScreenBuffer)), lptw->Attr);
	    sb_last_insert_str(&(lptw->ScreenBuffer), lptw->CursorPos.x, L"         ", tab);
	    UpdateText(lptw, tab);
	    if (lptw->bSuspend == 0) {
		UpdateScrollBars(lptw);
		TextToCursor(lptw);
	    }
	    break;
	}
	case 0x08:
	case 0x7f:
	    lptw->CursorPos.x--;
	    if (lptw->CursorPos.x < 0) {
		lptw->CursorPos.x = lptw->ScreenSize.x - 1;
		lptw->CursorPos.y--;
	    }
	    if (lptw->CursorPos.y < 0)
		lptw->CursorPos.y = 0;
	    break;
	default:
	    lb_set_attr(sb_get_last(&(lptw->ScreenBuffer)), lptw->Attr);
	    sb_last_insert_str(&(lptw->ScreenBuffer), lptw->CursorPos.x, &ch, 1);
	    UpdateText(lptw, 1);
	    if (lptw->bSuspend == 0) {
		/* maximum line size may have changed, so update scroll bars */
		UpdateScrollBars(lptw);
		TextToCursor(lptw);
	    }
    }
    return ch;
}


void
TextPutStr(LPTW lptw, LPSTR str)
{
    int count;
    uint n;
    uint idx;
    LPWSTR w, w_save;

    w_save = w = UnicodeText(str, encoding);
    while (*w != NUL) {
	idx = lptw->CursorPos.x;
	lb_set_attr(sb_get_last(&(lptw->ScreenBuffer)), lptw->Attr);
	for (count = 0, n = 0; (*w != NUL) && (iswprint(*w) || (*w == L'\t')); w++) {
	    if (*w == L'\t') {
		uint tab;

		tab = 8 - ((lptw->CursorPos.x + count + n) % 8);
		sb_last_insert_str(&(lptw->ScreenBuffer), idx, w - n, n);
		sb_last_insert_str(&(lptw->ScreenBuffer), idx + n, L"        ", tab);
		idx += n + tab;
		count += n + tab;
		n = 0;
	    } else {
		n++;
	    }
	}
	if (n != 0) {
	    sb_last_insert_str(&(lptw->ScreenBuffer), idx, w - n, n);
	    count += n;
	}

	if (count > 0)
	    UpdateText(lptw, count);
	if (*w == L'\n') {
	    NewLine(lptw);
	    w++;
	    n = 0;
	} else if (*w && !iswprint(*w) && (*w != L'\t')) {
	    TextPutChW(lptw, *w++);
	}
    }
    free(w_save);
    TextUpdateStatus(lptw);
}


static void
LimitMark(LPTW lptw, POINT *lppt)
{
    int length;

    if (lppt->x < 0)
	lppt->x = 0;
    if (lppt->y < 0) {
	lppt->x = 0;
	lppt->y = 0;
    }

    length = sb_max_line_length(&(lptw->ScreenBuffer));
    if (lppt->x > length)
	lppt->x = length;

    length = sb_length(&(lptw->ScreenBuffer));
    if (lppt->y >= length) {
	lppt->x = 0;
	lppt->y = length;
    }
}


static void
ClearMark(LPTW lptw, POINT pt)
{
    RECT rect1, rect2, rect3;
    int tmp;

    if ((lptw->MarkBegin.x != lptw->MarkEnd.x) ||
	(lptw->MarkBegin.y != lptw->MarkEnd.y) ) {
	if (lptw->MarkBegin.x > lptw->MarkEnd.x) {
	    tmp = lptw->MarkBegin.x;
	    lptw->MarkBegin.x = lptw->MarkEnd.x;
	    lptw->MarkEnd.x = tmp;
	}
	if (lptw->MarkBegin.y > lptw->MarkEnd.y) {
	    tmp = lptw->MarkBegin.y;
	    lptw->MarkBegin.y = lptw->MarkEnd.y;
	    lptw->MarkEnd.y = tmp;
	}
	/* calculate bounding rectangle in character coordinates */
	if (lptw->MarkBegin.y != lptw->MarkEnd.y) {
	    rect1.left = 0;
	    rect1.right = lptw->ScreenSize.x;
	} else {
	    rect1.left = lptw->MarkBegin.x;
	    rect1.right = lptw->MarkEnd.x + 1;
	}
	rect1.top = lptw->MarkBegin.y;
	rect1.bottom = lptw->MarkEnd.y + 1;
	/* now convert to client coordinates */
	rect1.left   = rect1.left   * lptw->CharSize.x - lptw->ScrollPos.x;
	rect1.right  = rect1.right  * lptw->CharSize.x - lptw->ScrollPos.x;
	rect1.top    = rect1.top    * lptw->CharSize.y - lptw->ScrollPos.y;
	rect1.bottom = rect1.bottom * lptw->CharSize.y - lptw->ScrollPos.y;
	/* get client rect and calculate intersection */
	GetClientRect(lptw->hWndText, &rect2);
	IntersectRect(&rect3, &rect1, &rect2);
	/* update window if necessary */
	if (!IsRectEmpty(&rect3)) {
	    InvalidateRect(lptw->hWndText, &rect3, TRUE);
	}
    }
    LimitMark(lptw, &pt);
    lptw->MarkBegin.x = lptw->MarkEnd.x = pt.x;
    lptw->MarkBegin.y = lptw->MarkEnd.y = pt.y;
    UpdateWindow(lptw->hWndText);
}


/* output a line including attribute changes as needed */
static void
DoLine(LPTW lptw, HDC hdc, int xpos, int ypos, int x, int y, int count)
{
    int idx, num;
    LPLB lb;
    LPWSTR w;
    PBYTE  a, pa;
    BYTE attr;

    idx = 0;
    if (y < sb_length(&(lptw->ScreenBuffer))) {
	lb = sb_get(&(lptw->ScreenBuffer), y);
	if (lb  == NULL)
	   return;
	w = lb_substr(lb, x + idx, count - idx);
	/* This sets the default color for "empty" cells. */
	lb_set_attr(lb, NOTEXT);
	a = lb_subattr(lb, x + idx, count - idx);
    } else {
	w = (LPWSTR) malloc(sizeof(WCHAR) * (count + 1));
	a = (PBYTE) malloc(sizeof(PBYTE) * (count + 1));
	wmemset(w, L' ', count);
	memset(a, NOTEXT, count);
	w[count] = NUL;
	a[count] = NUL;
    }

    num = count;
    pa = a;
    while (num > 0) {
	attr = *pa;
	while ((num > 0) && (attr == *pa)) {
	    /* skip over bytes with same attribute */
	    num--;
	    pa++;
	}

	if (lptw->bSysColors) {
	    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
	    SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
	} else {
	    SetTextColor(hdc, TextFore(attr));
	    SetBkColor(hdc, TextBack(attr));
	}
	TextOutW(hdc, xpos, ypos, w + idx, count - num - idx);

	xpos += lptw->CharSize.x * (count - num - idx);
	idx = count - num;
    }
    free(w);
    free(a);
    TextUpdateStatus(lptw);
}


static void
DoMark(LPTW lptw, POINT pt, POINT end, BOOL mark)
{
    int xpos, ypos;
    HDC hdc;
    int count;

    hdc = GetDC(lptw->hWndText);
    SelectObject(hdc, lptw->hfont);
    if (lptw->bSysColors) {
	SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
	SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
    } else {
	SetTextColor(hdc, MARKFORE);
	SetBkColor(hdc, MARKBACK);
    }
    while (pt.y < end.y) {
	/* multiple lines */
	xpos = pt.x * lptw->CharSize.x - lptw->ScrollPos.x;
	ypos = pt.y * lptw->CharSize.y - lptw->ScrollPos.y;
	count = max(lptw->ScreenSize.x - pt.x, 0);
	if (mark) {
	    LPLB lb = sb_get(&(lptw->ScreenBuffer), pt.y);
	    LPWSTR w = lb_substr(lb, pt.x, count);
	    TextOutW(hdc, xpos, ypos, w, count);
	    free(w);
	} else {
	    DoLine(lptw, hdc, xpos, ypos, pt.x, pt.y, count);

	    if (lptw->bSysColors) {
		SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
		SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
	    } else {
		SetTextColor(hdc, MARKFORE);
		SetBkColor(hdc, MARKBACK);
	    }
	}

	pt.y++;
	pt.x=0;
    }
    /* partial line */
    xpos = pt.x * lptw->CharSize.x - lptw->ScrollPos.x;
    ypos = pt.y * lptw->CharSize.y - lptw->ScrollPos.y;
    count = end.x - pt.x;
    if (count > 0) {
	if (mark) {
	    LPLB lb = sb_get(&(lptw->ScreenBuffer), pt.y);
	    LPWSTR w = lb_substr(lb, pt.x, count);
	    TextOutW(hdc, xpos, ypos, w, count);
	    free(w);
	} else {
	    DoLine(lptw, hdc, xpos, ypos, pt.x, pt.y, count);
	}
    }
    ReleaseDC(lptw->hWndText, hdc);
}


static void
UpdateMark(LPTW lptw, POINT pt)
{
    int begin, point, end;

    LimitMark(lptw, &pt);
    begin = lptw->ScreenSize.x * lptw->MarkBegin.y + lptw->MarkBegin.x;
    point = lptw->ScreenSize.x * pt.y + pt.x;
    end   = lptw->ScreenSize.x * lptw->MarkEnd.y + lptw->MarkEnd.x;

    if (begin <= end) {
	/* forward mark */
	if (point >= end) {
	    /* extend marked area */
	    DoMark(lptw, lptw->MarkEnd, pt, TRUE);
	} else if (point >= begin) {
	    /* retract marked area */
	    DoMark(lptw, pt, lptw->MarkEnd, FALSE);
	} else {
	    /* retract and reverse */
	    DoMark(lptw, lptw->MarkBegin, lptw->MarkEnd, FALSE);
	    DoMark(lptw, pt, lptw->MarkBegin, TRUE);
	}
    } else {
	/* reverse mark */
	if (point <= end) {
	    /* extend marked area */
	    DoMark(lptw, pt, lptw->MarkEnd, TRUE);
	} else if (point <= begin) {
	    /* retract marked area */
	    DoMark(lptw, lptw->MarkEnd, pt, FALSE);
	} else {
	    /* retract and reverse */
	    DoMark(lptw, lptw->MarkEnd, lptw->MarkBegin, FALSE);
	    DoMark(lptw, lptw->MarkBegin, pt, TRUE);
	}
    }
    lptw->MarkEnd.x = pt.x;
    lptw->MarkEnd.y = pt.y;
}


static void
TextCopyClip(LPTW lptw)
{
    size_t size, count;
    HGLOBAL hGMem;
    LPWSTR cbuf, cp;
    POINT pt, end;
    UINT type;
    LPLB lb;

    if ((lptw->MarkBegin.x == lptw->MarkEnd.x) &&
	(lptw->MarkBegin.y == lptw->MarkEnd.y) ) {
	/* copy user text */
	return;
    }

    /* calculate maximum total size */
    size = 1; /* end of string '\0' */
    for (pt.y = lptw->MarkBegin.y; pt.y <= lptw->MarkEnd.y; pt.y++) {
	LPLB line = sb_get(&(lptw->ScreenBuffer), pt.y);
	if (line) size += lb_length(line);
	size += 2;
    }

    hGMem = GlobalAlloc(GMEM_MOVEABLE, size * sizeof(WCHAR));
    cbuf = cp = (LPWSTR)GlobalLock(hGMem);
    if (cp == NULL)
	return;

    pt.x = lptw->MarkBegin.x;
    pt.y = lptw->MarkBegin.y;
    end.x = lptw->MarkEnd.x;
    end.y = lptw->MarkEnd.y;

    while (pt.y < end.y) {
	/* copy to global buffer */
	lb = sb_get(&(lptw->ScreenBuffer), pt.y);
	count = lb_length(lb) - pt.x;
	if (count > 0) {
	    wmemcpy(cp, lb->str + pt.x, count);
	    cp += count;
	}
	*(cp++) = L'\r';
	*(cp++) = L'\n';
	pt.y++;
	pt.x = 0;
    }
    /* partial line */
    count = end.x - pt.x;
    if (count > 0) {
	lb = sb_get(&(lptw->ScreenBuffer), pt.y);
	if (lb->len > pt.x) {
	    if (end.x > lb->len)
		count = lb->len - pt.x;
	    wmemcpy(cp, lb->str + pt.x, count);
	    cp += count;
	}
    }
    *cp = NUL;

    size = (wcslen(cbuf) + 1) * sizeof(WCHAR);
    GlobalUnlock(hGMem);
    hGMem = GlobalReAlloc(hGMem, size, GMEM_MOVEABLE);
    type = CF_UNICODETEXT;
    /* give buffer to clipboard */
    OpenClipboard(lptw->hWndParent);
    EmptyClipboard();
    SetClipboardData(type, hGMem);
    CloseClipboard();
}


static void
TextMakeFont(LPTW lptw)
{
    LOGFONT lf;
    TEXTMETRIC tm;
    LPTSTR p;
    HDC hdc;

    hdc = GetDC(lptw->hWndText);
    memset(&lf, 0, sizeof(LOGFONT));
    _tcsncpy(lf.lfFaceName, lptw->fontname, LF_FACESIZE);
    lf.lfHeight = -MulDiv(lptw->fontsize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    lf.lfPitchAndFamily = FIXED_PITCH;
    lf.lfOutPrecision = OUT_OUTLINE_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    /* ClearType quality is only supported on XP or later */
    lf.lfQuality = IsWindowsXPorLater() ? CLEARTYPE_QUALITY : PROOF_QUALITY;
    lf.lfCharSet = DEFAULT_CHARSET;
    if ((p = _tcsstr(lptw->fontname, TEXT(" Italic"))) != NULL) {
	lf.lfFaceName[(unsigned int)(p - lptw->fontname)] = NUL;
	lf.lfItalic = TRUE;
    }
    if ((p = _tcsstr(lptw->fontname, TEXT(" Bold"))) != NULL) {
	lf.lfFaceName[(unsigned int)(p - lptw->fontname)] = NUL;
	lf.lfWeight = FW_BOLD;
    }
    if (lptw->hfont != 0)
	DeleteObject(lptw->hfont);
    lptw->hfont = CreateFontIndirect((LOGFONT *)&lf);
    /* get text size */
    SelectObject(hdc, lptw->hfont);
    GetTextMetrics(hdc,(TEXTMETRIC *)&tm);
    lptw->CharSize.y = tm.tmHeight;
    lptw->CharSize.x = tm.tmAveCharWidth;
    lptw->CharAscent = tm.tmAscent;
    if (lptw->bFocus)
	CreateCaret(lptw->hWndText, 0, lptw->CharSize.x, 2+lptw->CaretHeight);
    ReleaseDC(lptw->hWndText, hdc);
    return;
}


static void
TextSelectFont(LPTW lptw)
{
    LOGFONT lf;
    CHOOSEFONT cf;
    HDC hdc;
    LPTSTR p;

    /* Set all structure fields to zero. */
    memset(&cf, 0, sizeof(CHOOSEFONT));
    memset(&lf, 0, sizeof(LOGFONT));
    cf.lStructSize = sizeof(CHOOSEFONT);
    cf.hwndOwner = lptw->hWndParent;
    _tcsncpy(lf.lfFaceName, lptw->fontname, LF_FACESIZE);
    if ((p = _tcsstr(lptw->fontname, TEXT(" Bold"))) != NULL) {
	lf.lfWeight = FW_BOLD;
	lf.lfFaceName[p - lptw->fontname] = NUL;
    } else {
	lf.lfWeight = FW_NORMAL;
    }
    if ((p = _tcsstr(lptw->fontname, TEXT(" Italic"))) != NULL) {
	lf.lfItalic = TRUE;
	lf.lfFaceName[p - lptw->fontname] = NUL;
    } else {
	lf.lfItalic = FALSE;
    }
    lf.lfCharSet = DEFAULT_CHARSET;
    hdc = GetDC(lptw->hWndText);
    lf.lfHeight = -MulDiv(lptw->fontsize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(lptw->hWndText, hdc);
    lf.lfPitchAndFamily = FIXED_PITCH;
    cf.lpLogFont = &lf;
    cf.nFontType = SCREEN_FONTTYPE;
    cf.Flags = CF_SCREENFONTS | CF_FIXEDPITCHONLY | CF_INITTOLOGFONTSTRUCT | CF_SCALABLEONLY;

    if (ChooseFont(&cf)) {
	RECT rect;

	_tcscpy(lptw->fontname, lf.lfFaceName);
	lptw->fontsize = cf.iPointSize / 10;
	if (cf.nFontType & BOLD_FONTTYPE)
	    _tcscat(lptw->fontname, TEXT(" Bold"));
	if (cf.nFontType & ITALIC_FONTTYPE)
	    _tcscat(lptw->fontname, TEXT(" Italic"));
	TextMakeFont(lptw);
	/* force a window update */
	GetClientRect(lptw->hWndText, &rect);
	SendMessage(lptw->hWndText, WM_SIZE, SIZE_RESTORED,
		    MAKELPARAM(rect.right - rect.left, rect.bottom - rect.top));
	GetClientRect(lptw->hWndText, &rect);
	InvalidateRect(lptw->hWndText, &rect, 1);
	UpdateWindow(lptw->hWndText);
    }
}


/*
 * Update the status bar
 */
void
TextUpdateStatus(LPTW lptw)
{
    static enum set_encoding_id enc = S_ENC_INVALID;
    if (enc != encoding) {  /* only update when changed */
	WCHAR buf[256];

	enc = encoding;
	swprintf_s(buf, sizeof(buf)/sizeof(WCHAR), L"encoding: %hs", encoding_names[enc]);
	SendMessageW(lptw->hStatusbar, SB_SETTEXTW, (WPARAM)0, (LPARAM)buf);
    }
}


/* parent overlapped window */
LRESULT CALLBACK
WndParentProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    LPTW lptw;

    lptw = (LPTW)GetWindowLongPtrW(hwnd, 0);

    switch (message) {
    case WM_SYSCOMMAND:
	switch (LOWORD(wParam)) {
	case M_COPY_CLIP:
	case M_PASTE:
	case M_CHOOSE_FONT:
	case M_SYSCOLORS:
	case M_WRAP:
	case M_WRITEINI:
	case M_ABOUT:
	    SendMessage(lptw->hWndText, WM_COMMAND, wParam, lParam);
	}
	break;
    case WM_SETFOCUS:
	if (IsWindow(lptw->hWndFocus)) {
	    SetFocus(lptw->hWndFocus);
	    return 0;
	} else if (IsWindow(lptw->hWndText)) {
	    SetFocus(lptw->hWndText);
	    return 0;
	}
	break;
    case WM_GETMINMAXINFO: {
	POINT * MMinfo = (POINT *)lParam;
	MMinfo[3].x = GetSystemMetrics(SM_CXVSCROLL) + 2 * GetSystemMetrics(SM_CXFRAME);
	MMinfo[3].y = GetSystemMetrics(SM_CYHSCROLL) + 2 * GetSystemMetrics(SM_CYFRAME)
	    + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU);
	if (lptw) {
	    MMinfo[3].x += ScreenMinSize.x * lptw->CharSize.x;
	    MMinfo[3].y += ScreenMinSize.y * lptw->CharSize.y;
	    MMinfo[3].y += lptw->ButtonHeight + lptw->StatusHeight;
	}
	return 0;
    }
    case WM_SIZE:
	/* Vista sets the window size to 0,0 when Windows-D is pressed */
	if (lParam > 0) {
	    unsigned width = LOWORD(lParam);
	    unsigned height = HIWORD(lParam) - lptw->StatusHeight;

	    ApplyLayout(lptw, hwnd, width, height);

	    // also resize status bar
	    SendMessage(lptw->hStatusbar, WM_SIZE, wParam, lParam);
	}
	return 0;
    case WM_COMMAND:
	SendMessage(lptw->hWndText, message, wParam, lParam); /* pass on menu commands */
	return 0;
    case WM_ERASEBKGND:
	return 1;
    case WM_PAINT: {
	/* clear empty space next to docked graph windows, if any */
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	int width, height;

	hdc = BeginPaint(hwnd, &ps);

	GetClientRect(hwnd, &rect);
	width = rect.right - rect.left;
	height = rect.bottom - rect.top - lptw->StatusHeight;

	if (lptw->nDocked > 0) {
	    enum docked_layout layout = DockedLayout(lptw);
	    unsigned m, rows, cols;

	    m = GPMAX(lptw->nDocked, lptw->nDockCols * lptw->nDockRows);
	    cols = lptw->nDockCols;
	    rows = (m + cols - 1) / cols;

	    SetDCBrushColor(hdc, lptw->SeparatorColor);
	    if (layout == DOCKED_LAYOUT_HORIZONTAL) {
		unsigned non_empty_rows = ((lptw->nDocked + (cols - 1)) / cols);
		unsigned xofs = MulDiv(width, lptw->HorzFracDock, 1000) + lptw->SeparatorWidth / 2;
		rect.left = xofs;
		rect.top    = MulDiv(height, non_empty_rows, rows);
		rect.bottom = height;
		if (non_empty_rows != rows)
		    FillRect(hdc, &rect, GetStockObject(DC_BRUSH));
		if (non_empty_rows > 0) {
		    unsigned empty_cols = non_empty_rows * cols - lptw->nDocked;
		    rect.left = xofs +	MulDiv(width - xofs, cols - empty_cols, cols);
		    rect.bottom = rect.top;
		    rect.top = MulDiv(height, non_empty_rows - 1, rows);
		    FillRect(hdc, &rect, GetStockObject(DC_BRUSH));
		}
	    } else {
		unsigned non_empty_rows = ((lptw->nDocked + (cols - 1)) / cols);
		unsigned yofs = MulDiv(height, lptw->VertFracDock, 1000) + lptw->SeparatorWidth / 2;
		unsigned gheight = height - yofs;
		rect.top = yofs + MulDiv(gheight, non_empty_rows, rows);
		rect.bottom = height;
		if (non_empty_rows != rows)
		    FillRect(hdc, &rect, GetStockObject(DC_BRUSH));
		if (non_empty_rows > 0) {
		    unsigned empty_cols = non_empty_rows * cols - lptw->nDocked;
		    rect.left = MulDiv(width, cols - empty_cols, cols);
		    rect.bottom = rect.top;
		    rect.top = yofs + MulDiv(gheight, non_empty_rows - 1, rows);
		    FillRect(hdc, &rect, GetStockObject(DC_BRUSH));
		}
	    }
	}

	EndPaint(hwnd, &ps);
	return 0;
    }
    case WM_DROPFILES:
	DragFunc(lptw, (HDROP)wParam);
	break;
    case WM_CONTEXTMENU:
	SendMessage(lptw->hWndText, WM_CONTEXTMENU, wParam, lParam);
	return 0;
    case WM_CREATE: {
	TEXTMETRIC tm;

	// store pointer to text window struct
	lptw = (LPTW) ((CREATESTRUCT *)lParam)->lpCreateParams;
	SetWindowLongPtrW(hwnd, 0, (LONG_PTR)lptw);
	lptw->hWndParent = hwnd;

	/* get character size */
	TextMakeFont(lptw);
	hdc = GetDC(hwnd);
	SelectObject(hdc, lptw->hfont);
	GetTextMetrics(hdc,(LPTEXTMETRIC)&tm);
	lptw->CharSize.y = tm.tmHeight;
	lptw->CharSize.x = tm.tmAveCharWidth;
	lptw->CharAscent = tm.tmAscent;
	ReleaseDC(hwnd, hdc);

	// init drag'n drop
	if ((lptw->DragPre != NULL) && (lptw->DragPost != NULL))
	    DragAcceptFiles(hwnd, TRUE);
	break;
    }
    case WM_DESTROY:
	DragAcceptFiles(hwnd, FALSE);
	DeleteObject(lptw->hfont);
	lptw->hfont = 0;
	break;
    case WM_CLOSE:
	if (lptw->shutdown) {
	    FARPROC lpShutDown = lptw->shutdown;
	    (*lpShutDown)();
	}
	break;
    } /* switch() */

    return DefWindowProcW(hwnd, message, wParam, lParam);
}


/* PM 20011218: Reallocate larger keyboard buffer */
static int
ReallocateKeyBuf(LPTW lptw)
{
    int newbufsize = lptw->KeyBufSize + 16*1024; /* new buffer size */
    HGLOBAL h_old = (HGLOBAL)GlobalHandle(lptw->KeyBuf);
    HGLOBAL h = GlobalAlloc(LHND, newbufsize);
    int pos_in = lptw->KeyBufIn - lptw->KeyBuf;
    int pos_out = lptw->KeyBufOut - lptw->KeyBuf;
    BYTE *NewKeyBuf = (BYTE *)GlobalLock(h);

    if (NewKeyBuf == NULL) {
	MessageBox((HWND)NULL, szNoMemory, NULL, MB_ICONHAND | MB_SYSTEMMODAL);
	return 1;
    }
    if (lptw->KeyBufIn > lptw->KeyBufOut) {
	/*  | Buf ... Out ... In | */
	memcpy(NewKeyBuf, lptw->KeyBufOut,
		  lptw->KeyBufIn - lptw->KeyBufOut);
	lptw->KeyBufIn = NewKeyBuf + (pos_in - pos_out);
    } else {
	/*  | Buf ... In ... Out ... | */
	memcpy(NewKeyBuf, lptw->KeyBufOut, lptw->KeyBufSize - pos_out);
	memcpy(NewKeyBuf, lptw->KeyBuf, pos_in);
	lptw->KeyBufIn = NewKeyBuf + (lptw->KeyBufSize - pos_out + pos_in);
    }
    if (h_old) {
	GlobalUnlock(h_old);
	GlobalFree(h_old);
    }
    lptw->KeyBufSize = newbufsize;
    lptw->KeyBufOut = lptw->KeyBuf = NewKeyBuf;
    return 0;
}


/* update the position of the cursor */
static void
UpdateCaretPos(LPTW lptw)
{
    HDC hdc;
    LPLB line = sb_get_last(&lptw->ScreenBuffer);
    SIZE size;
    int len, start;

    if (lptw->bWrap) {
	start = (lptw->CursorPos.x / lptw->ScreenBuffer.wrap_at) * lptw->ScreenBuffer.wrap_at;
	len = (lptw->CursorPos.x % lptw->ScreenBuffer.wrap_at);
    } else {
	start = 0;
	len = lptw->CursorPos.x;
    }

    if (encoding == S_ENC_UTF8 || encoding == S_ENC_SJIS) {
	/* determine actual text size */
	hdc = GetDC(lptw->hWndText);
	SelectObject(hdc, lptw->hfont);
	GetTextExtentPoint32W(hdc, line->str + start, len, &size);
	ReleaseDC(lptw->hWndText, hdc);
    } else {
	size.cx = len * lptw->CharSize.x;
    }

    if (lptw->bWrap) {
	SetCaretPos(size.cx - lptw->ScrollPos.x,
		    (lptw->CursorPos.y + (lptw->CursorPos.x / lptw->ScreenBuffer.wrap_at)) * lptw->CharSize.y + lptw->CharAscent
		    - lptw->CaretHeight - lptw->ScrollPos.y);
    } else {
	SetCaretPos(size.cx - lptw->ScrollPos.x,
		    lptw->CursorPos.y * lptw->CharSize.y + lptw->CharAscent
		    - lptw->CaretHeight - lptw->ScrollPos.y);
    }
}


// child toolbar window
LRESULT CALLBACK
WndSeparatorProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPTW lptw;

    lptw = (LPTW)GetWindowLongPtrW(hwnd, 0);
    switch (message) {
    case WM_CREATE:
	lptw = (LPTW)((CREATESTRUCT *)lParam)->lpCreateParams;
	SetWindowLongPtrW(hwnd, 0, (LONG_PTR)lptw);
	lptw->hWndText = hwnd;
	break;
    case WM_ERASEBKGND: {
	HDC hdc = GetDC(hwnd);
	RECT rect;
	SetDCBrushColor(hdc, GetSysColor(COLOR_WINDOWFRAME));
	GetClientRect(hwnd, &rect);
	FillRect(hdc, &rect, GetStockObject(DC_BRUSH));
	ReleaseDC(hwnd, hdc);
	return 1;
    }
    case WM_LBUTTONDOWN:
	if (wParam == MK_LBUTTON) {
	    WCHAR buf[100];

	    lptw->bFracChanging = TRUE;
	    SetCapture(hwnd);
	    swprintf_s(buf, sizeof(buf) / sizeof(WCHAR), L"fraction: %.1f %%",
		(DockedLayout(lptw) == DOCKED_LAYOUT_HORIZONTAL ? lptw->HorzFracDock : lptw->VertFracDock) / 10.);
	    SendMessageW(lptw->hStatusbar, SB_SETTEXTW, (WPARAM)1, (LPARAM)buf);
	}
	break;
    case WM_LBUTTONUP:
	if (lptw->bFracChanging) {
	    lptw->bFracChanging = FALSE;
	    ReleaseCapture();
	    SendMessageW(lptw->hStatusbar, SB_SETTEXTW, (WPARAM)1, (LPARAM)L"");
	}
	break;
    case WM_MOUSEMOVE: {
	TRACKMOUSEEVENT tme;

	if (lptw->bFracChanging) {
	    RECT rect;
	    POINT point;
	    WCHAR buf[100];
	    enum docked_layout layout;
	    unsigned width, height;

	    GetClientRect(lptw->hWndParent, &rect);
	    width = rect.right - rect.left;
	    height = rect.bottom - rect.top - lptw->StatusHeight;
	    GetWindowRect(hwnd, &rect);
	    point.x = (rect.left + rect.right) / 2;
	    point.y = (rect.top + rect.bottom) / 2;;
	    ScreenToClient(lptw->hWndParent, &point);
	    layout = DockedLayout(lptw);
	    if (layout == DOCKED_LAYOUT_HORIZONTAL) {
		int xPos = GET_X_LPARAM(lParam);
		lptw->HorzFracDock = MulDiv(xPos + point.x, 1000, width);
		if (lptw->HorzFracDock < 100) lptw->HorzFracDock = 100;
		if (lptw->HorzFracDock > 900) lptw->HorzFracDock = 900;
	    } else {
		int yPos = GET_Y_LPARAM(lParam);
		lptw->VertFracDock = MulDiv(yPos + point.y, 1000, height);
		if (lptw->VertFracDock < 100) lptw->VertFracDock = 100;
		if (lptw->VertFracDock > 900) lptw->VertFracDock = 900;
	    }
	    swprintf_s(buf, sizeof(buf) / sizeof(WCHAR), L"fraction: %.1f %%",
		(layout == DOCKED_LAYOUT_HORIZONTAL ? lptw->HorzFracDock : lptw->VertFracDock) / 10.);
	    SendMessageW(lptw->hStatusbar, SB_SETTEXTW, (WPARAM)1, (LPARAM)buf);
	    DockedUpdateLayout(lptw);
	    return 0;
	}

	// we want to receive mouse leave messages
	tme.cbSize = sizeof(TRACKMOUSEEVENT);
	tme.dwFlags = TME_LEAVE;
	tme.hwndTrack = hwnd;
	TrackMouseEvent(&tme);
	break;
    }
    case WM_SETCURSOR: {
	enum docked_layout layout;

	layout = DockedLayout(lptw);
	if (layout == DOCKED_LAYOUT_HORIZONTAL)
	    SetCursor(LoadCursor(NULL, IDC_SIZEWE));
	else
	    SetCursor(LoadCursor(NULL, IDC_SIZENS));
	return TRUE;
    }
    case WM_MOUSELEAVE:
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return 0;
    } /* switch(message) */

    return DefWindowProcW(hwnd, message, wParam, lParam);
}


/* child toolbar window */
LRESULT CALLBACK
WndToolbarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPTW lptw;

    lptw = (LPTW)GetWindowLongPtrW(hwnd, 0);

    switch (message) {
    case WM_CREATE:
	lptw = (LPTW)((CREATESTRUCT *)lParam)->lpCreateParams;
	SetWindowLongPtrW(hwnd, 0, (LONG_PTR)lptw);
	lptw->hWndText = hwnd;
	break;
    case WM_SIZE:
	SendMessage(lptw->lpmw->hToolbar, WM_SIZE, wParam, lParam);
	break;
    case WM_ERASEBKGND:
	return 1;
    case WM_COMMAND:
	if (IsWindow(lptw->hWndText))
	    SetFocus(lptw->hWndText);
	/* pass on menu commands */
	SendMessage(lptw->hWndText, message, wParam, lParam);
	return 0;
    case WM_NOTIFY:
	switch (((LPNMHDR)lParam)->code) {
	case TBN_DROPDOWN: {
	    RECT rc;
	    TPMPARAMS tpm;
	    LPNMTOOLBAR lpnmTB = (LPNMTOOLBAR)lParam;

	    SendMessage(lpnmTB->hdr.hwndFrom, TB_GETRECT, (WPARAM)lpnmTB->iItem, (LPARAM)&rc);
	    MapWindowPoints(lpnmTB->hdr.hwndFrom, HWND_DESKTOP, (LPPOINT)&rc, 2);
	    tpm.cbSize = sizeof(TPMPARAMS);
	    tpm.rcExclude = rc;
	    TrackPopupMenuEx(lptw->hPopMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
			     rc.left, rc.bottom, lptw->hWndText, &tpm);
	    return TBDDRET_DEFAULT;
	}
	default:
	    return FALSE;
	}
    } /* switch(message) */
    return DefWindowProcW(hwnd, message, wParam, lParam);
}


/* child text window */
LRESULT CALLBACK
WndTextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    int nYinc, nXinc;
    LPTW lptw;

    lptw = (LPTW)GetWindowLongPtrW(hwnd, 0);

    switch(message) {
    case WM_SETFOCUS:
	lptw->bFocus = TRUE;
	lptw->hWndFocus = hwnd;
	CreateCaret(hwnd, 0, lptw->CharSize.x, 2+lptw->CaretHeight);
	UpdateCaretPos(lptw);
	if (lptw->bGetCh)
	    ShowCaret(hwnd);
	break;
    case WM_KILLFOCUS:
	DestroyCaret();
	lptw->bFocus = FALSE;
	break;
    case WM_SIZE: {
	bool caret_visible;
	int new_wrap;
	int new_screensize_y = HIWORD(lParam);
	int new_screensize_x = LOWORD(lParam);

	new_wrap = lptw->bWrap ? (new_screensize_x / lptw->CharSize.x) : 0;

	/* is caret visible? */
	caret_visible = ((lptw->ScrollPos.y < lptw->CursorPos.y * lptw->CharSize.y) &&
	    ((lptw->ScrollPos.y + lptw->ClientSize.y) >= (lptw->CursorPos.y * lptw->CharSize.y)));

	/* update scroll bar position */
	if (!caret_visible) {
	    uint new_x, new_y;

	    /* keep upper left corner in place */
	    sb_find_new_pos(&(lptw->ScreenBuffer),
		lptw->ScrollPos.x / lptw->CharSize.x, lptw->ScrollPos.y / lptw->CharSize.y,
		new_wrap, & new_x, & new_y);
	    lptw->ScrollPos.x = lptw->CharSize.x * new_x + lptw->ScrollPos.x % lptw->CharSize.x;
	    lptw->ScrollPos.y = lptw->CharSize.y * new_y + lptw->ScrollPos.y % lptw->CharSize.y;
	} else {
	    int xold, yold;
	    int deltax, deltay;
	    uint xnew, ynew;

	    /* keep cursor in place */
	    xold = lptw->CursorPos.x;
	    yold = lptw->CursorPos.y;
	    if (lptw->ScreenBuffer.wrap_at) {
		xold %= lptw->ScreenBuffer.wrap_at;
		yold += lptw->CursorPos.x / lptw->ScreenBuffer.wrap_at;
	    }
	    deltay = GPMAX(lptw->ScrollPos.y + lptw->ClientSize.y - (yold - 1) * lptw->CharSize.y, 0);
	    deltax = xold * lptw->CharSize.x - lptw->ScrollPos.x;

	    sb_find_new_pos(&(lptw->ScreenBuffer),
		xold, yold, new_wrap, &xnew, &ynew);

	    lptw->ScrollPos.x = GPMAX((xnew * lptw->CharSize.x) - deltax, 0);
	    if ((ynew + 1)* lptw->CharSize.y > new_screensize_y)
		lptw->ScrollPos.y = GPMAX((ynew * lptw->CharSize.y) + deltay - new_screensize_y, 0);
	    else
 		lptw->ScrollPos.y = 0;
	}

	lptw->ClientSize.y = HIWORD(lParam);
	lptw->ClientSize.x = LOWORD(lParam);
	lptw->ScreenSize.y = lptw->ClientSize.y / lptw->CharSize.y + 1;
	lptw->ScreenSize.x = lptw->ClientSize.x / lptw->CharSize.x + 1;

	if (lptw->bWrap) {
	    uint len;
	    LPLB lb;

	    /* update markers, if necessary */
	    if ((lptw->MarkBegin.x != lptw->MarkEnd.x) ||
		(lptw->MarkBegin.y != lptw->MarkEnd.y) ) {
		uint new_x, new_y;

		sb_find_new_pos(&(lptw->ScreenBuffer), lptw->MarkBegin.x, lptw->MarkBegin.y,
		    lptw->ScreenSize.x - 1, & new_x, & new_y);
		lptw->MarkBegin.x = new_x;
		lptw->MarkBegin.y = new_y;
		sb_find_new_pos(&(lptw->ScreenBuffer), lptw->MarkEnd.x, lptw->MarkEnd.y,
		    lptw->ScreenSize.x - 1, & new_x, & new_y);
		lptw->MarkEnd.x = new_x;
		lptw->MarkEnd.y = new_y;
	    }
	    /* set new wrapping: the character at ScreenSize.x is only partially
	       visible, so we wrap one character before */
	    sb_wrap(&(lptw->ScreenBuffer), new_wrap);

	    /* update y-position of cursor, x-position is adjusted automatically;
	       hint: the cursor is _always_ on the last (logical) line */
	    len = sb_length(&(lptw->ScreenBuffer));
	    lb  = sb_get_last(&(lptw->ScreenBuffer));
	    lptw->CursorPos.y = len - sb_lines(&(lptw->ScreenBuffer), lb);
	    if (lptw->CursorPos.y < 0) lptw->CursorPos.y = 0;
	}

	UpdateScrollBars(lptw);

	if (lptw->bFocus && lptw->bGetCh) {
	    UpdateCaretPos(lptw);
	    ShowCaret(hwnd);
	}
	return 0;
	}
    case WM_VSCROLL:
	switch(LOWORD(wParam)) {
	case SB_TOP:
	    nYinc = -lptw->ScrollPos.y;
	    break;
	case SB_BOTTOM:
	    nYinc = lptw->ScrollMax.y - lptw->ScrollPos.y;
	    break;
	case SB_LINEUP:
	    nYinc = -lptw->CharSize.y;
	    break;
	case SB_LINEDOWN:
	    nYinc = lptw->CharSize.y;
	    break;
	case SB_PAGEUP:
	    nYinc = min(-1, -lptw->ClientSize.y);
	    break;
	case SB_PAGEDOWN:
	    nYinc = max(1, lptw->ClientSize.y);
	    break;
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
	    nYinc = HIWORD(wParam) - lptw->ScrollPos.y;
	    break;
	default:
	    nYinc = 0;
	} /* switch(loword(wparam)) */

	if ((nYinc = max(-lptw->ScrollPos.y,
		  min(nYinc, lptw->ScrollMax.y - lptw->ScrollPos.y)))
	     != 0 ) {
	    lptw->ScrollPos.y += nYinc;
	    ScrollWindow(hwnd, 0, -nYinc, NULL, NULL);
	    SetScrollPos(hwnd, SB_VERT, lptw->ScrollPos.y, TRUE);
	    UpdateWindow(hwnd);
	}
	return 0;
    case WM_HSCROLL:
	switch(LOWORD(wParam)) {
	case SB_LINEUP:
	    nXinc = -lptw->CharSize.x;
	    break;
	case SB_LINEDOWN:
	    nXinc = lptw->CharSize.x;
	    break;
	case SB_PAGEUP:
	    nXinc = min(-1, -lptw->ClientSize.x);
	    break;
	case SB_PAGEDOWN:
	    nXinc = max(1, lptw->ClientSize.x);
	    break;
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
	    nXinc = HIWORD(wParam) - lptw->ScrollPos.x;
	    break;
	default:
	    nXinc = 0;
	} /* switch(loword(wparam)) */

	if ((nXinc = max(-lptw->ScrollPos.x,
			  min(nXinc, lptw->ScrollMax.x - lptw->ScrollPos.x)))
	     != 0 ) {
	    lptw->ScrollPos.x += nXinc;
	    ScrollWindow(hwnd, -nXinc, 0, NULL, NULL);
	    SetScrollPos(hwnd, SB_HORZ, lptw->ScrollPos.x, TRUE);
	    UpdateWindow(hwnd);
	}
	return 0;
    case WM_KEYDOWN:
	if (GetKeyState(VK_SHIFT) < 0) {
	    switch(wParam) {
	    case VK_HOME:
		SendMessage(hwnd, WM_VSCROLL, SB_TOP, (LPARAM)0);
		break;
	    case VK_END:
		SendMessage(hwnd, WM_VSCROLL, SB_BOTTOM, (LPARAM)0);
		break;
	    case VK_PRIOR:
		SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, (LPARAM)0);
		break;
	    case VK_NEXT:
		SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, (LPARAM)0);
		break;
	    case VK_UP:
		SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, (LPARAM)0);
		break;
	    case VK_DOWN:
		SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, (LPARAM)0);
		break;
	    case VK_LEFT:
		SendMessage(hwnd, WM_HSCROLL, SB_LINEUP, (LPARAM)0);
		break;
	    case VK_RIGHT:
		SendMessage(hwnd, WM_HSCROLL, SB_LINEDOWN, (LPARAM)0);
		break;
	    } /* switch(wparam) */
	} else {		/* if(shift) */
	    switch(wParam) {
	    case VK_HOME:
	    case VK_END:
	    case VK_PRIOR:
	    case VK_NEXT:
	    case VK_UP:
	    case VK_DOWN:
	    case VK_LEFT:
	    case VK_RIGHT:
	    case VK_DELETE: { /* store key in circular buffer */
		long count = lptw->KeyBufIn - lptw->KeyBufOut;

		if (count < 0)
		    count += lptw->KeyBufSize;
		if (count < lptw->KeyBufSize-2) {
		    *lptw->KeyBufIn++ = 0;
		    if (lptw->KeyBufIn - lptw->KeyBuf >= lptw->KeyBufSize)
			lptw->KeyBufIn = lptw->KeyBuf;	/* wrap around */
		    *lptw->KeyBufIn++ = HIWORD(lParam) & 0xff;
		    if (lptw->KeyBufIn - lptw->KeyBuf >= lptw->KeyBufSize)
			lptw->KeyBufIn = lptw->KeyBuf;	/* wrap around */
		}
	    }
	    break;
	    case VK_CANCEL:
		ctrlc_flag = TRUE;
		break;
	    } /* switch(wparam) */
	} /* else(shift) */
	break;
    case WM_KEYUP:
	if (GetKeyState(VK_SHIFT) < 0) {
	    switch(wParam) {
	    case VK_INSERT:
		/* Shift-Insert: paste clipboard */
		SendMessage(lptw->hWndText, WM_COMMAND, M_PASTE, (LPARAM)0);
		break;
	    }
	} /* if(shift) */
	if (GetKeyState(VK_CONTROL) < 0) {
	    switch(wParam) {
	    case VK_INSERT:
		/* Ctrl-Insert: copy to clipboard */
		SendMessage(lptw->hWndText, WM_COMMAND, M_COPY_CLIP, (LPARAM)0);
		break;
	    case 'C':
		/* Ctrl-C: copy to clipboard, if there's selected text,
		           otherwise indicate the Ctrl-C (break) flag */
		if ((lptw->MarkBegin.x != lptw->MarkEnd.x) ||
		    (lptw->MarkBegin.y != lptw->MarkEnd.y))
		    SendMessage(lptw->hWndText, WM_COMMAND, M_COPY_CLIP, (LPARAM)0);
		else
		    ctrlc_flag = TRUE;
		break;
	    case 'V':
		/* Ctrl-V: paste clipboard */
		SendMessage(lptw->hWndText, WM_COMMAND, M_PASTE, (LPARAM)0);
		break;
	    } /* switch(wparam) */
	} /* if(Ctrl) */
	break;
    case WM_CONTEXTMENU: {
	POINT pt;
	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	if (pt.x == -1) { /* keyboard activation */
	    pt.x = pt.y = 0;
	    ClientToScreen(hwnd, &pt);
	}
	TrackPopupMenu(lptw->hPopMenu, TPM_LEFTALIGN,
		pt.x, pt.y, 0, hwnd, NULL);
	return 0;
    }
    case WM_LBUTTONDOWN: {
	/* start marking text */
	POINT pt;

	/* grab input focus if there are docked graph windows */
	if (lptw->nDocked > 0)
	    SetFocus(hwnd);

	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	pt.x = (pt.x + lptw->ScrollPos.x)/lptw->CharSize.x;
	pt.y = (pt.y + lptw->ScrollPos.y)/lptw->CharSize.y;
	ClearMark(lptw, pt);
	SetCapture(hwnd);	/* track the mouse */
	lptw->Marking = TRUE;
	break;
    }
    case WM_LBUTTONUP: {
	/* finish marking text */
	/* ensure begin mark is before end mark */
	ReleaseCapture();
	lptw->Marking = FALSE;
	if ((lptw->ScreenSize.x*lptw->MarkBegin.y + lptw->MarkBegin.x) >
	    (lptw->ScreenSize.x*lptw->MarkEnd.y   + lptw->MarkEnd.x)) {
	    POINT tmp;
	    tmp.x = lptw->MarkBegin.x;
	    tmp.y = lptw->MarkBegin.y;
	    lptw->MarkBegin.x = lptw->MarkEnd.x;
	    lptw->MarkBegin.y = lptw->MarkEnd.y;
	    lptw->MarkEnd.x   = tmp.x;
	    lptw->MarkEnd.y   = tmp.y;
	}
	break;
    }
    case WM_MOUSEMOVE:
	if ((wParam & MK_LBUTTON) && lptw->Marking) {
	    RECT rect;
	    POINT pt;

	    SetCursor(LoadCursor(NULL, IDC_IBEAM));
	    pt.x = LOWORD(lParam);
	    pt.y = HIWORD(lParam);
	    GetClientRect(hwnd, &rect);
	    if (PtInRect(&rect, pt)) {
		pt.x = (pt.x + lptw->ScrollPos.x)/lptw->CharSize.x;
		pt.y = (pt.y + lptw->ScrollPos.y)/lptw->CharSize.y;
		UpdateMark(lptw, pt);
	    } else {
		int nXinc, nYinc;

		do {
		    nXinc = 0;
		    nYinc = 0;
		    if (pt.x > rect.right) {
			nXinc = lptw->CharSize.x * 4;
			pt.x = (rect.right + lptw->ScrollPos.x)
			    / lptw->CharSize.x + 2;
		    } else if (pt.x < rect.left) {
			nXinc = -lptw->CharSize.x * 4;
			pt.x = (rect.left + lptw->ScrollPos.x)
			    / lptw->CharSize.x - 2;
		    } else
			pt.x = (pt.x + lptw->ScrollPos.x)
			    /lptw->CharSize.x;
		    if (pt.y > rect.bottom) {
			nYinc = lptw->CharSize.y;
			pt.y = (rect.bottom + lptw->ScrollPos.y)
			    / lptw->CharSize.y + 1;
		    } else if (pt.y < rect.top) {
			nYinc = -lptw->CharSize.y;
			pt.y = (rect.top + lptw->ScrollPos.y)
			    / lptw->CharSize.y - 1;
		    } else
			pt.y = (pt.y + lptw->ScrollPos.y)
			    / lptw->CharSize.y;

		    LimitMark(lptw, &pt);
		    nXinc = max(nXinc, -lptw->ScrollPos.x);
		    nYinc = max(nYinc, -lptw->ScrollPos.y);
		    nYinc = min(nYinc, lptw->ScrollMax.y - lptw->ScrollPos.y);
		    nXinc = min(nXinc, lptw->ScrollMax.x - lptw->ScrollPos.x);
		    if (nYinc || nXinc) {
			lptw->ScrollPos.y += nYinc;
			lptw->ScrollPos.x += nXinc;
			ScrollWindow(lptw->hWndText, -nXinc, -nYinc,
				     NULL, NULL);
			SetScrollPos(lptw->hWndText, SB_VERT,
				     lptw->ScrollPos.y, TRUE);
			SetScrollPos(lptw->hWndText, SB_HORZ,
				     lptw->ScrollPos.x, TRUE);
			UpdateWindow(lptw->hWndText);
		    }
		    UpdateMark(lptw, pt);
		    GetCursorPos(&pt);
		    ScreenToClient(hwnd, &pt);
		}
		while((nYinc || nXinc) && !PtInRect(&rect, pt)
		      && (GetAsyncKeyState(VK_LBUTTON) < 0));
	    } /* moved inside viewport */
	} /* if(dragging) */
	else {
	    SetCursor(LoadCursor(NULL, IDC_ARROW));
	}
	break;
    case WM_MOUSEWHEEL: {
	    WORD fwKeys;
	    short int zDelta;

	    fwKeys = LOWORD(wParam);
	    zDelta = HIWORD(wParam);
	    switch (fwKeys) {
	    case 0:
		if (zDelta < 0)
		    SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, (LPARAM)0);
		else
		    SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, (LPARAM)0);
		return 0;
	    case MK_SHIFT:
		if (zDelta < 0)
		    SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, (LPARAM)0);
		else
		    SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, (LPARAM)0);
		return 0;
	    case MK_CONTROL:
		if (zDelta < 0)
		    SendMessage(hwnd, WM_CHAR, 0x0e, (LPARAM)0); // CTRL-N
		else
		    SendMessage(hwnd, WM_CHAR, 0x10, (LPARAM)0); // CTRL-P
		return 0;
	    }
	}
	break;
    case WM_CHAR: {
	long count;
	char char_mb[8];
	int count_mb;
	WCHAR char_utf16[2];
	WPARAM key = wParam;

	/* handle only UCS-2, not full UTF16 */
	if ((key >= 0xd800) && (key < 0xE000))
	    return 0;

	/* remap Shift-Tab to FS */
	if ((GetKeyState(VK_SHIFT) < 0) && (key == VK_TAB))
	    key = 034;

	/* convert UTF16 code to current encoding which may be a multi-byte encoding like utf8 or sjis */
	char_utf16[0] = key;
	char_utf16[1] = 0;
	count_mb = WideCharToMultiByte(WinGetCodepage(encoding), 0, char_utf16, 1, char_mb, sizeof(char_mb), NULL, NULL);

	/* store sequence in circular buffer */
	count  = lptw->KeyBufIn - lptw->KeyBufOut;
	if (count < 0)
	    count += lptw->KeyBufSize;
	if (count == lptw->KeyBufSize - count_mb) {
	    /* Keyboard buffer is full, so reallocate a larger one. */
	    if (ReallocateKeyBuf(lptw))
		return 0; /* not enough memory */
	}
	if (count < lptw->KeyBufSize-count_mb) {
	    int index;

	    for (index = 0; index < count_mb; index++) {
		*lptw->KeyBufIn++ = char_mb[index];
		if (lptw->KeyBufIn - lptw->KeyBuf >= lptw->KeyBufSize)
		    lptw->KeyBufIn = lptw->KeyBuf;    /* wrap around */
	    }
	}
	return 0;
    }
    case WM_COMMAND:
	if (LOWORD(wParam) < NUMMENU)
	    SendMacro(lptw, LOWORD(wParam));
	else
	    switch(LOWORD(wParam)) {
	    case M_COPY_CLIP:
		TextCopyClip(lptw);
		return 0;
	    case M_PASTE:
	    {
		HGLOBAL hGMem;
		LPWSTR cbuf;
		UINT type;

		type = CF_UNICODETEXT;
		/* now get it from clipboard */
		OpenClipboard(hwnd);
		hGMem = GetClipboardData(type);
		if (hGMem) {
		    cbuf = (LPWSTR) GlobalLock(hGMem);
		    while (*cbuf) {
			WCHAR c = *cbuf;
			if (c == 011) /* escape tab characters by sending ^V */
			    SendMessageW(lptw->hWndText, WM_CHAR, 026, 1L);
			if (*cbuf != L'\n')
			    SendMessageW(lptw->hWndText, WM_CHAR, c, 1L);
			cbuf++;
		    }
		    GlobalUnlock(hGMem);
		} /* if(hGmem) */
		CloseClipboard();
		return 0;
	    }
	    case M_CHOOSE_FONT:
		TextSelectFont(lptw);
		return 0;
	    case M_SYSCOLORS:
		lptw->bSysColors = !lptw->bSysColors;
		if (lptw->bSysColors)
		    CheckMenuItem(lptw->hPopMenu, M_SYSCOLORS, MF_BYCOMMAND | MF_CHECKED);
		else
		    CheckMenuItem(lptw->hPopMenu, M_SYSCOLORS, MF_BYCOMMAND | MF_UNCHECKED);
		SendMessage(hwnd, WM_SYSCOLORCHANGE, (WPARAM)0, (LPARAM)0);
		InvalidateRect(hwnd, (LPRECT)NULL, 1);
		UpdateWindow(hwnd);
		return 0;
	    case M_WRAP: {
		LPLB lb;
		uint len;
		uint new_wrap;
		bool caret_visible;

		lptw->bWrap = !lptw->bWrap;
		if (lptw->bWrap)
		    CheckMenuItem(lptw->hPopMenu, M_WRAP, MF_BYCOMMAND | MF_CHECKED);
		else
		    CheckMenuItem(lptw->hPopMenu, M_WRAP, MF_BYCOMMAND | MF_UNCHECKED);

		new_wrap = lptw->bWrap ? lptw->ScreenSize.x - 1 : 0;

		/* update markers, if necessary */
		if ((lptw->MarkBegin.x != lptw->MarkEnd.x) ||
		    (lptw->MarkBegin.y != lptw->MarkEnd.y) ) {
		    uint new_x, new_y;

		    sb_find_new_pos(&(lptw->ScreenBuffer), lptw->MarkBegin.x, lptw->MarkBegin.y,
			new_wrap, & new_x, & new_y);
		    lptw->MarkBegin.x = new_x;
		    lptw->MarkBegin.y = new_y;
		    sb_find_new_pos(&(lptw->ScreenBuffer), lptw->MarkEnd.x, lptw->MarkEnd.y,
			new_wrap, & new_x, & new_y);
		    lptw->MarkEnd.x = new_x;
		    lptw->MarkEnd.y = new_y;
		}

		/* is caret visible? */
		caret_visible = ((lptw->ScrollPos.y < lptw->CursorPos.y * lptw->CharSize.y) &&
		    ((lptw->ScrollPos.y + lptw->ClientSize.y) >= (lptw->CursorPos.y * lptw->CharSize.y)));

		/* update scroll bar position */
		if (!caret_visible) {
		    uint new_x, new_y;

		    /* keep upper left corner in place */
		    sb_find_new_pos(&(lptw->ScreenBuffer),
			lptw->ScrollPos.x / lptw->CharSize.x, lptw->ScrollPos.y / lptw->CharSize.y,
			new_wrap, & new_x, & new_y);
		    lptw->ScrollPos.x = lptw->CharSize.x * new_x;
		    lptw->ScrollPos.y = lptw->CharSize.y * new_y;
		} else {
		    int xold, yold;
		    int deltax, deltay;
		    uint xnew, ynew;

		    /* keep cursor in place */
		    xold = lptw->CursorPos.x;
		    yold = lptw->CursorPos.y;
		    if (lptw->ScreenBuffer.wrap_at) {
			xold %= lptw->ScreenBuffer.wrap_at;
			yold += lptw->CursorPos.x / lptw->ScreenBuffer.wrap_at;
		    }
		    deltay = GPMAX(lptw->ScrollPos.y + lptw->ClientSize.y - (yold - 1) * lptw->CharSize.y, 0);
		    deltax = xold * lptw->CharSize.x - lptw->ScrollPos.x;

		    sb_find_new_pos(&(lptw->ScreenBuffer),
			xold, yold, new_wrap, &xnew, &ynew);

		    lptw->ScrollPos.x = GPMAX((xnew * lptw->CharSize.x) - deltax, 0);
		    if ((ynew + 1)* lptw->CharSize.y > lptw->ScreenSize.y)
			lptw->ScrollPos.y = GPMAX((ynew * lptw->CharSize.y) + deltay - lptw->ScreenSize.y, 0);
		    else
			lptw->ScrollPos.y = 0;
		    lptw->ScrollPos.x = (xnew * lptw->CharSize.x) - deltax;
		}

		/* now switch wrapping */
		sb_wrap(&(lptw->ScreenBuffer), new_wrap);

		/* update y-position of cursor, x-position is adjusted automatically */
		len = sb_length(&(lptw->ScreenBuffer));
		lb  = sb_get_last(&(lptw->ScreenBuffer));
		lptw->CursorPos.y = len - sb_lines(&(lptw->ScreenBuffer), lb);

		UpdateScrollBars(lptw);
		if (lptw->bFocus && lptw->bGetCh) {
		    UpdateCaretPos(lptw);
		    ShowCaret(hwnd);
		}

		InvalidateRect(hwnd, (LPRECT)NULL, 1);
		UpdateWindow(hwnd);
		return 0;
	    }
	    case M_WRITEINI:
		WriteTextIni(lptw);
		return 0;
	    case M_ABOUT:
		AboutBox(hwnd, lptw->AboutText);
		return 0;
	    } /* switch(loword(wparam)) */
	return 0;
    case WM_SYSCOLORCHANGE:
	DeleteObject(lptw->hbrBackground);
	lptw->hbrBackground = CreateSolidBrush(lptw->bSysColors ?
					       GetSysColor(COLOR_WINDOW) : RGB(255,255,255));
	return 0;
    case WM_ERASEBKGND:
	return 1; /* not necessary */
    case WM_PAINT: {
	POINT source, width, dest;
	POINT MarkBegin, MarkEnd;

	/* check update region */
	if (!GetUpdateRect(hwnd, NULL, FALSE))
	    return 0;

	hdc = BeginPaint(hwnd, &ps);

	SelectObject(hdc, lptw->hfont);
	SetMapMode(hdc, MM_TEXT);
	SetBkMode(hdc, OPAQUE);
	GetClientRect(hwnd, &rect);

	/* source */
	source.x = (rect.left + lptw->ScrollPos.x) / lptw->CharSize.x;
	source.y = (rect.top + lptw->ScrollPos.y) / lptw->CharSize.y;

	/* destination */
	dest.x = source.x * lptw->CharSize.x - lptw->ScrollPos.x;
	dest.y = source.y * lptw->CharSize.y - lptw->ScrollPos.y;

	width.x = ((rect.right  + lptw->ScrollPos.x + lptw->CharSize.x - 1)
		   / lptw->CharSize.x) - source.x;
	width.y = ((rect.bottom + lptw->ScrollPos.y + lptw->CharSize.y - 1)
		   / lptw->CharSize.y) - source.y;
	if (source.x < 0)
	    source.x = 0;
	if (source.y < 0)
	    source.y = 0;

	/* ensure begin mark is before end mark */
	if ((lptw->ScreenSize.x * lptw->MarkBegin.y + lptw->MarkBegin.x) >
	    (lptw->ScreenSize.x * lptw->MarkEnd.y   + lptw->MarkEnd.x)) {
	    MarkBegin.x = lptw->MarkEnd.x;
	    MarkBegin.y = lptw->MarkEnd.y;
	    MarkEnd.x   = lptw->MarkBegin.x;
	    MarkEnd.y   = lptw->MarkBegin.y;
	} else {
	    MarkBegin.x = lptw->MarkBegin.x;
	    MarkBegin.y = lptw->MarkBegin.y;
	    MarkEnd.x   = lptw->MarkEnd.x;
	    MarkEnd.y   = lptw->MarkEnd.y;
	}

	/* for each line */
	while (width.y > 0) {
	    if ((source.y >= MarkBegin.y) && (source.y <= MarkEnd.y)) {
		int start, end;
		int count, offset;

		if (source.y == MarkBegin.y)
		    start = MarkBegin.x;
		else
		    start = 0;
		if (source.y == MarkEnd.y)
		    end = MarkEnd.x;
		else
		    end = lptw->ScreenSize.x;
		/* do stuff before marked text */
		offset = 0;
		count = start - source.x;
		if (count > 0)
		    DoLine(lptw, hdc, dest.x, dest.y, source.x, source.y, count);
		/* then the marked text */
		offset += count;
		count = end - start;
		if ((count > 0) && (offset < width.x)){
		    LPLB lb;
		    LPWSTR w;

		    if (lptw->bSysColors) {
			SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
			SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
		    } else {
			SetTextColor(hdc, MARKFORE);
			SetBkColor(hdc, MARKBACK);
		    }

		    lb = sb_get(&(lptw->ScreenBuffer), source.y);
		    w = lb_substr(lb, source.x + offset, count);
		    TextOutW(hdc, dest.x + lptw->CharSize.x * offset, dest.y, w, count);
		    free(w);
		}
		/* then stuff after marked text */
		offset += count;
		count = width.x + source.x - end;
		if ((count > 0) && (offset < width.x))
		    DoLine(lptw, hdc, dest.x + lptw->CharSize.x * offset, dest.y,
			   source.x + offset, source.y, count);
	    } else {
		DoLine(lptw, hdc, dest.x, dest.y, source.x, source.y, width.x);
	    }
	    dest.y += lptw->CharSize.y;
	    source.y++;
	    width.y--;
	}
	EndPaint(hwnd, &ps);
	return 0;
    }
    case WM_CREATE:
	lptw = (LPTW) ((CREATESTRUCT *)lParam)->lpCreateParams;
	SetWindowLongPtrW(hwnd, 0, (LONG_PTR)lptw);
	lptw->hWndText = hwnd;
	break;
    case WM_DESTROY:
	DeleteObject(lptw->hbrBackground);
	break;
    } /* switch(message) */
    return DefWindowProcW(hwnd, message, wParam, lParam);
}


static enum docked_layout
DockedLayout(LPTW lptw)
{
    RECT rect;
    unsigned width, height;

    if (lptw->nDocked == 0)
	return DOCKED_LAYOUT_NONE;

    GetClientRect(lptw->hWndParent, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top - lptw->StatusHeight;
    if (width >= height)
	return DOCKED_LAYOUT_HORIZONTAL;
    else
	return DOCKED_LAYOUT_VERTICAL;
}


/* redraw text window by triggering a resize event */
void
DockedUpdateLayout(LPTW lptw)
{
    HWND hwnd = lptw->hWndParent;
    RECT rect;

    GetClientRect(hwnd, &rect);
    SendMessage(hwnd, WM_SIZE, SIZE_RESTORED,
		MAKELPARAM(rect.right - rect.left, rect.bottom - rect.top));
    GetClientRect(hwnd, &rect);
    InvalidateRect(hwnd, &rect, 1);
    UpdateWindow(hwnd);
}


static unsigned
NumberOfDockedWindows(LPTW lptw)
{
    LPGW lpgw = listgraphs;
    unsigned n = 0;
    while (lpgw != NULL) {
	if (lpgw->bDocked && GraphHasWindow(lpgw))
	    n++;
	lpgw = lpgw->next;
    }
    return n;
}


void
DockedGraphSize(LPTW lptw, SIZE *size, BOOL newwindow)
{
    enum docked_layout layout;
    unsigned width, height;
    unsigned m, cols, rows;
    RECT rect;

    GetClientRect(lptw->hWndParent, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top - lptw->StatusHeight;

    // Are we about to create a new window?
    if (newwindow)
	lptw->nDocked++;

    // Only determine the (future) layout now
    layout = DockedLayout(lptw);

    m = GPMAX(lptw->nDocked, lptw->nDockCols * lptw->nDockRows);
    cols = lptw->nDockCols;
    rows = (m + cols - 1) / cols;

    if (layout == DOCKED_LAYOUT_HORIZONTAL) {
	size->cx = (MulDiv(width, 1000 - lptw->HorzFracDock, 1000) - lptw->SeparatorWidth / 2) / cols;
	size->cy = height / rows;
    } else {
	size->cx = width / cols;
	size->cy = (MulDiv(height, 1000 - lptw->VertFracDock, 1000) - lptw->SeparatorWidth / 2) / rows;
    }
}


static void
ApplyLayout(LPTW lptw, HWND hwnd, unsigned width, unsigned height)
{
    LPGW lpgw;
    enum docked_layout layout;

    // count actual number of docked graph windows
    lptw->nDocked = NumberOfDockedWindows(lptw);

    layout = DockedLayout(lptw);
    if (layout == DOCKED_LAYOUT_NONE) {
	// no docked graph windows:  resize text and toolbar windows
	SetWindowPos(lptw->hWndText, NULL,
	    0, lptw->ButtonHeight,
	    width, height - lptw->ButtonHeight,
	    SWP_NOZORDER | SWP_NOACTIVATE);
	if (lptw->lpmw != NULL)
	    SetWindowPos(lptw->hWndToolbar, NULL,
		0, 0,
		width, lptw->ButtonHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
	SetWindowPos(lptw->hWndSeparator, NULL,
	    width, 0,
	    width, 0,
	    SWP_NOZORDER | SWP_NOACTIVATE);
	ShowWindow(lptw->hWndSeparator, SW_HIDE);
    } else {
	unsigned n, m;
	unsigned cols, rows;
	SIZE size;
	RECT rect;

	// first resize text and toolbar windows
	if (layout == DOCKED_LAYOUT_HORIZONTAL) {
	    // split window horizontally
	    SetWindowPos(lptw->hWndText, NULL,
		0, lptw->ButtonHeight,
		MulDiv(width, lptw->HorzFracDock, 1000) - lptw->SeparatorWidth / 2,
		height - lptw->ButtonHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
	    if (lptw->lpmw != NULL)
		SetWindowPos(lptw->hWndToolbar, NULL,
		    0, 0,
		    MulDiv(width, lptw->HorzFracDock, 1000) - lptw->SeparatorWidth / 2, lptw->ButtonHeight,
		    SWP_NOZORDER | SWP_NOACTIVATE);
	    SetWindowPos(lptw->hWndSeparator, NULL,
		MulDiv(width, lptw->HorzFracDock, 1000) - lptw->SeparatorWidth / 2, 0,
		lptw->SeparatorWidth, height,
		SWP_NOZORDER | SWP_NOACTIVATE);
	} else {
	    // split window vertically
	    SetWindowPos(lptw->hWndText, NULL,
		0, lptw->ButtonHeight,
		width,
		MulDiv(height, lptw->VertFracDock, 1000) - lptw->SeparatorWidth / 2 - lptw->ButtonHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
	    if (lptw->lpmw != NULL)
		SetWindowPos(lptw->hWndToolbar, NULL,
		    0, 0,
		    width, lptw->ButtonHeight,
		    SWP_NOZORDER | SWP_NOACTIVATE);
	    SetWindowPos(lptw->hWndSeparator, NULL,
		0, MulDiv(height, lptw->VertFracDock, 1000) - lptw->SeparatorWidth / 2,
		width, lptw->SeparatorWidth,
		SWP_NOZORDER | SWP_NOACTIVATE);
	}
	ShowWindow(lptw->hWndSeparator, SW_SHOWNOACTIVATE);

	// Number of slots to reserve might be larger than number of actual plots
	m = GPMAX(lptw->nDocked, lptw->nDockCols * lptw->nDockRows);
	cols = lptw->nDockCols;
	rows = (m + cols - 1) / cols;

	// Resize graph windows
	n = 0;
	lpgw = listgraphs;
	DockedGraphSize(lptw, &size, FALSE);
	while (lpgw != NULL) {
	    if (lpgw->bDocked && GraphHasWindow(lpgw)) {
		if (layout == DOCKED_LAYOUT_HORIZONTAL) {
		    // all plot windows in the right part of the window in cols columns
		    lpgw->Origin.x  = MulDiv(width, lptw->HorzFracDock, 1000) + lptw->SeparatorWidth / 2;
		    lpgw->Origin.x += size.cx * (n % cols);
		    lpgw->Origin.y  = size.cy * (n / cols);
		} else {
		    // all plot windows in the lower part of the window in cols columns
		    lpgw->Origin.x  = size.cx * (n % cols);
		    lpgw->Origin.y  = MulDiv(height, lptw->VertFracDock, 1000) + lptw->SeparatorWidth / 2;
		    lpgw->Origin.y += size.cy * (n / cols);
		}
		SetWindowPos(lpgw->hWndGraph, NULL,
		    lpgw->Origin.x, lpgw->Origin.y,
		    size.cx, size.cy,
		    SWP_NOZORDER | SWP_NOACTIVATE);
		n++;
	    }
	    lpgw = lpgw->next;
	}

	GetClientRect(hwnd, &rect);
	InvalidateRect(hwnd, &rect, 0);
	UpdateWindow(hwnd);
    }
}


void
TextStartEditing(LPTW lptw)
{
    TextToCursor(lptw);
    if (lptw->bFocus && !lptw->bGetCh) {
	UpdateCaretPos(lptw);
	ShowCaret(lptw->hWndText);
    }
    lptw->bGetCh = TRUE;
}


void
TextStopEditing(LPTW lptw)
{
    if (lptw->bFocus && lptw->bGetCh)
	HideCaret(lptw->hWndText);
    lptw->bGetCh = FALSE;
}

/* ================================== */
/* replacement stdio routines */

/* TRUE if key hit, FALSE if no key */
int
TextKBHit(LPTW lptw)
{
    return (lptw->KeyBufIn != lptw->KeyBufOut);
}


/* get character from keyboard, no echo */
/* need to add extended codes */
int
TextGetCh(LPTW lptw)
{
    int ch;

    TextStartEditing(lptw);
    while (!TextKBHit(lptw)) {
	/* CMW: can't use TextMessage here as it does not idle properly */
	MSG msg;
	GetMessage(&msg, 0, 0, 0);
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }
    ch = *lptw->KeyBufOut++;
    if (ch == '\r')
	ch = '\n';
    if (lptw->KeyBufOut - lptw->KeyBuf >= lptw->KeyBufSize)
	lptw->KeyBufOut = lptw->KeyBuf;	/* wrap around */
    TextStopEditing(lptw);
    return ch;
}


/* get character from keyboard, with echo */
int
TextGetChE(LPTW lptw)
{
    int ch;

    ch = TextGetCh(lptw);
    TextPutCh(lptw, (BYTE)ch);
    return ch;
}


LPSTR
TextGetS(LPTW lptw, LPSTR str, unsigned int size)
{
    LPSTR next = str;

    while (--size > 0) {
	switch (*next = TextGetChE(lptw)) {
	case EOF:
	    *next = 0;
	    if (next == str)
		return NULL;
	    return str;
	case '\n':
	    *(next+1) = 0;
	    return str;
	case 0x08:
	case 0x7f:
	    if (next > str)
		--next;
	    break;
	default:
	    ++next;
	}
    }
    *next = 0;
    return str;
}


int
TextPutS(LPTW lptw, LPSTR str)
{
    TextPutStr(lptw, str);
    return str[strlen(str) - 1];
}


void
TextAttr(LPTW lptw, BYTE attr)
{
    lptw->Attr = attr;
}

#endif /* WGP_CONSOLE */


void
DragFunc(LPTW lptw, HDROP hdrop)
{
    int i, cFiles;
    LPWSTR p;
    LPWSTR w;
    struct _stat buf;

    if ((lptw->DragPre == NULL) || (lptw->DragPost == NULL))
	return;
    cFiles = DragQueryFileW(hdrop, (UINT) -1, NULL, 0);
    for (i = 0; i < cFiles; i++) {
	WCHAR szFile[MAX_PATH];

	DragQueryFileW(hdrop, i, szFile, MAX_PATH);
	_wstat(szFile, &buf);
	if (buf.st_mode & S_IFDIR) {
	    for (p = L"cd '"; *p; p++)
		SendMessageW(lptw->hWndText, WM_CHAR, *p, 1L);
	} else {
	    for (p = lptw->DragPre; *p; p++)
		SendMessageW(lptw->hWndText, WM_CHAR, *p, 1L);
	}
	for (w = szFile; *w; w++)
	    SendMessageW(lptw->hWndText, WM_CHAR, *w, 1L);
	for (p = lptw->DragPost; *p; p++)
	    SendMessageW(lptw->hWndText, WM_CHAR, *p, 1L);
    }
    DragFinish(hdrop);
}


void
WriteTextIni(LPTW lptw)
{
    RECT rect;
    LPTSTR file = lptw->IniFile;
    LPTSTR section = lptw->IniSection;
    TCHAR profile[80];
    BOOL iconic, zoomed;
    UINT dpi;

    if ((file == NULL) || (section == NULL))
	return;

    iconic = IsIconic(lptw->hWndParent);
    zoomed = IsZoomed(lptw->hWndParent);
    if (iconic || zoomed)
	ShowWindow(lptw->hWndParent, SW_SHOWNORMAL);
    /* Rescale window size to 96dpi. */
    GetWindowRect(lptw->hWndParent, &rect);
    dpi = GetDPI();
    wsprintf(profile, TEXT("%d %d"), MulDiv(rect.left, 96, dpi), MulDiv(rect.top, 96, dpi));
    WritePrivateProfileString(section, TEXT("TextOrigin"), profile, file);
    wsprintf(profile, TEXT("%d %d"), MulDiv(rect.right-rect.left, 96, dpi), MulDiv(rect.bottom-rect.top, 96, dpi));
    WritePrivateProfileString(section, TEXT("TextSize"), profile, file);
    wsprintf(profile, TEXT("%d"), iconic);
    WritePrivateProfileString(section, TEXT("TextMinimized"), profile, file);
    wsprintf(profile, TEXT("%d"), zoomed);
    WritePrivateProfileString(section, TEXT("TextMaximized"), profile, file);
    wsprintf(profile, TEXT("%s,%d"), lptw->fontname, lptw->fontsize);
    WritePrivateProfileString(section, TEXT("TextFont"), profile, file);
    wsprintf(profile, TEXT("%d"), lptw->bWrap);
    WritePrivateProfileString(section, TEXT("TextWrap"), profile, file);
    wsprintf(profile, TEXT("%d"), lptw->ScreenBuffer.size - 1);
    WritePrivateProfileString(section, TEXT("TextLines"), profile, file);
    wsprintf(profile, TEXT("%d"), lptw->bSysColors);
    WritePrivateProfileString(section, TEXT("SysColors"), profile, file);
    wsprintf(profile, TEXT("%d"), lptw->VertFracDock);
    WritePrivateProfileString(section, TEXT("DockVerticalTextFrac"), profile, file);
    wsprintf(profile, TEXT("%d"), lptw->HorzFracDock);
    WritePrivateProfileString(section, TEXT("DockHorizontalTextFrac"), profile, file);
    if (iconic)
	ShowWindow(lptw->hWndParent, SW_SHOWMINIMIZED);
    if (zoomed)
	ShowWindow(lptw->hWndParent, SW_SHOWMAXIMIZED);
    return;
}


/* Helper function to avoid signedness conflict --- windows delivers an INT, we want an uint */
static LPTSTR
GetUInt(LPTSTR str, uint *pval)
{
    INT val_fromGetInt;

    str = GetInt(str, &val_fromGetInt);
    *pval = (uint)val_fromGetInt;
    return str;
}


void
ReadTextIni(LPTW lptw)
{
    LPTSTR file = lptw->IniFile;
    LPTSTR section = lptw->IniSection;
    TCHAR profile[81];
    LPTSTR p;
    BOOL bOKINI;
    UINT dpi;

    bOKINI = (file != NULL) && (section != NULL);
    profile[0] = NUL;

    if (bOKINI)
	GetPrivateProfileString(section, TEXT("TextOrigin"), TEXT(""), profile, 80, file);
    if ((p = GetInt(profile, (LPINT)&lptw->Origin.x)) == NULL)
	lptw->Origin.x = CW_USEDEFAULT;
    if ((p = GetInt(p, (LPINT)&lptw->Origin.y)) == NULL)
	lptw->Origin.y = CW_USEDEFAULT;
    if (bOKINI)
	GetPrivateProfileString(section, TEXT("TextSize"), TEXT(""), profile, 80, file);
    if ((p = GetInt(profile, (LPINT)&lptw->Size.x)) == NULL)
	lptw->Size.x = CW_USEDEFAULT;
    if ((p = GetInt(p, (LPINT)&lptw->Size.y)) == NULL)
	lptw->Size.y = CW_USEDEFAULT;
    /* Saved size and position are normalised to 96dpi. */
    dpi = GetDPI();
    if (lptw->Origin.x != CW_USEDEFAULT)
	lptw->Origin.x = MulDiv(lptw->Origin.x, dpi, 96);
    if (lptw->Origin.y != CW_USEDEFAULT)
	lptw->Origin.y = MulDiv(lptw->Origin.y, dpi, 96);
    if (lptw->Size.x != CW_USEDEFAULT)
	lptw->Size.x = MulDiv(lptw->Size.x, dpi, 96);
    if (lptw->Size.y != CW_USEDEFAULT)
	lptw->Size.y = MulDiv(lptw->Size.y, dpi, 96);

    if (bOKINI)
	GetPrivateProfileString(section, TEXT("TextFont"), TEXT(""), profile, 80, file);
    {
	LPTSTR size = _tcschr(profile, ',');
	if (size) {
	    *size++ = NUL;
	    if ( (p = GetInt(size, (LPINT) &lptw->fontsize)) == NULL)
		lptw->fontsize = TEXTFONTSIZE;
	}
	if (lptw->fontsize == 0)
	    lptw->fontsize = TEXTFONTSIZE;

	_tcscpy(lptw->fontname, profile);
	if (!(*lptw->fontname)) {
	    if (GetACP() == 932) /* Japanese Shift-JIS */
		_tcscpy(lptw->fontname, TEXT("MS Gothic"));
	    else {
		/* select a default type face depending on the OS version */
		OSVERSIONINFO versionInfo;
		ZeroMemory(&versionInfo, sizeof(OSVERSIONINFO));
		versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&versionInfo);
		if (versionInfo.dwMajorVersion >= 6) /* Vista or later */
		    _tcscpy(lptw->fontname, TEXT("Consolas"));
		else if ((versionInfo.dwMajorVersion == 5) && (versionInfo.dwMinorVersion >= 1)) /* Windows XP */
		    _tcscpy(lptw->fontname, TEXT("Lucida Console"));
		else /* Windows 2000 or earlier */
		    _tcscpy(lptw->fontname, TEXT("Courier New"));
	    }
	}
    }

    if (bOKINI) {
	int iconic;
	GetPrivateProfileString(section, TEXT("TextMinimized"), TEXT(""), profile, 80, file);
	if ((p = GetInt(profile, &iconic)) == NULL)
	    iconic = 0;
	if (iconic)
	    lptw->nCmdShow = SW_SHOWMINIMIZED;
    }

    if (bOKINI) {
	int maximize;
	GetPrivateProfileString(section, TEXT("TextMaximized"), TEXT(""), profile, 80, file);
	if ((p = GetInt(profile, &maximize)) == NULL)
	    maximize = 0;
	if (maximize)
	    lptw->nCmdShow = SW_SHOWMAXIMIZED;
    }

    lptw->bSysColors = FALSE;
    GetPrivateProfileString(section, TEXT("SysColors"), TEXT(""), profile, 80, file);
    if ((p = GetInt(profile, &lptw->bSysColors)) == NULL)
	lptw->bSysColors = 0;

    /*  autowrapping is activated by default */
    GetPrivateProfileString(section, TEXT("TextWrap"), TEXT(""), profile, 80, file);
    if ((p = GetInt(profile, &lptw->bWrap)) == NULL)
	lptw->bWrap = TRUE;
    sb_wrap(&(lptw->ScreenBuffer), lptw->bWrap ? 80 : 0);

    /* length of screen buffer (unwrapped lines) */
    GetPrivateProfileString(section, TEXT("TextLines"), TEXT(""), profile, 80, file);
    if ((p = GetUInt(profile, &lptw->ScreenBuffer.size)) == NULL)
	lptw->ScreenBuffer.size = 400;

    /* control variables for docked graphs */
    /* TODO: an additional "Docked" switch would be nice */
#if 0
    // Disabled these since they are controlled by "set term win"
    GetPrivateProfileString(section, TEXT("DockCols"), TEXT(""), profile, 80, file);
    if ((p = GetUInt(profile, &lptw->nDockCols)) == NULL)
	lptw->nDockCols = 1;
    GetPrivateProfileString(section, TEXT("DockRows"), TEXT(""), profile, 80, file);
    if ((p = GetUInt(profile, &lptw->nDockRows)) == NULL)
	lptw->nDockRows = 1;
#else
    lptw->nDockCols = 1;
    lptw->nDockRows = 1;
#endif
    GetPrivateProfileString(section, TEXT("DockVerticalTextFrac"), TEXT(""), profile, 80, file);
    if ((p = GetUInt(profile, &lptw->VertFracDock)) == NULL)
	lptw->VertFracDock = 350;
    GetPrivateProfileString(section, TEXT("DockHorizontalTextFrac"), TEXT(""), profile, 80, file);
    if ((p = GetUInt(profile, &lptw->HorzFracDock)) == NULL)
	lptw->HorzFracDock = 400;
    GetPrivateProfileString(section, TEXT("DockSeparatorWidth"), TEXT(""), profile, 80, file);
    if ((p = GetUInt(profile, &lptw->SeparatorWidth)) == NULL)
	lptw->SeparatorWidth = 6;
    lptw->SeparatorWidth = MulDiv(lptw->SeparatorWidth, dpi, 96);
    /* TODO: Add wgnuplot.ini setting for this */
    lptw->SeparatorColor = RGB(240, 240, 240);
}


/* About Box */
INT_PTR CALLBACK
AboutDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg) {
    case WM_INITDIALOG:
    {
	TCHAR buf[80];

	GetWindowText(GetParent(hDlg), buf, 80);
	SetDlgItemText(hDlg, AB_TEXT1, buf);
	SetDlgItemText(hDlg, AB_TEXT2, (LPCTSTR) lParam);
	return TRUE;
    }
    case WM_DRAWITEM:
    {
	LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
	DrawIcon(lpdis->hDC, 0, 0, (HICON)GetClassLongPtr(GetParent(hDlg), GCLP_HICON));
	return FALSE;
    }
    case WM_COMMAND:
	switch (LOWORD(wParam)) {
	case IDCANCEL:
	case IDOK:
	    EndDialog(hDlg, LOWORD(wParam));
	    return TRUE;
	}
	break;
    } /* switch(message) */
    return FALSE;
}


void
AboutBox(HWND hwnd, LPTSTR str)
{
    DialogBoxParam(hdllInstance, TEXT("AboutDlgBox"), hwnd,
                   AboutDlgProc, (LPARAM)str);
}
