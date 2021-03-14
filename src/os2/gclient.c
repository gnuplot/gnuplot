/****************************************************************************

    PROGRAM: Gnupmdrv

    MODULE:  gclient.c

    This file contains the client window procedures for Gnupmdrv

****************************************************************************/

/* PM driver for GNUPLOT */

/*[
 * Copyright 1992, 1993, 1998, 2004, 2019-2021   Roger Fearick
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
 * AUTHOR
 *
 *   Gnuplot driver for OS/2:  Roger Fearick
 *
 *
 * CONTRIBUTIONS:
 *
 *   Petr Mikulik  (see PM labels)
 *       - menu item to keep aspect ratio on/off (October 1997)
 *       - mouse support (1998, 1999); changes made after gnuplot 3.7.0.5:
 *	    - use gnuplot's pid in the name of shared memory (11. 5. 1999)
 *	    - mouse in maps; distance in polar coords (14. 5. 1999)
 *	    - event semaphore for sending data via shared memory;
 *	      zooming completely revised;
 *	      remap keys in 'case WM_CHAR';
 *	      Ctrl-C for breaking long drawings;
 *	      new menu items under 'Mouse' and 'Utilities' (August 1999)
 *	- rewrite of mouse support for the new scheme common with X11
 *	      (October 1999 - January 2000)
 *      - pm3d stuff (since January 1999)
 *
 *   Franz Bakan
 *       - communication gnupmdrv -> gnuplot via shared memory (April 1999)
 *       - date and time on x axis (August 1999)
 *
 *   Bastian Maerkisch
 *       RGB colors, pattern fill, images, fixes (2004, 2005)
 *       dashed lines, bold/italic text, fill style for boxes and polygons, fixes (2019)
 *       line-to-ruler, zoom-coordinates, cleanup, fixes (2021)
 *
 */

#define INCL_PM
#define INCL_WIN
#define INCL_SPL
#define INCL_SPLDOSPRINT
#define INCL_WINSTDFONT
#define INCL_DOSMEMMGR
#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#define INCL_DOSFILEMGR
#define INCL_DOSNMPIPES
#define INCL_DOSSESMGR
#define INCL_DOSSEMAPHORES
#define INCL_DOSMISC
#ifdef HAVE_EXCEPTQ
# define INCL_DOSEXCEPTIONS
# define INCL_DOSMODULEMGR
#endif
#define INCL_DOSQUEUES
#define INCL_WINSWITCHLIST
#define INCL_GPIPRIMITIVES
#include <os2.h>
/* Warning: the OS/2 headers already define LT_DEFAULT
            which conflicts with our definition. */
#undef LT_DEFAULT

#include <string.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <unistd.h>
#include <process.h>
#include <signal.h>
#include <time.h>
#include "config.h"
#include "version.c" /* directly include version information */
#include "term_api.h"
#include "gnupmdrv.h"
#include "pm_msgs.h"
#include "mousecmn.h"
/* Finally, include the common mousing declarations and routines: */
#include "gpexecute.h"


/*==== m i s c e l l a n e o u s =============================================*/

/* February 2008, according to the gnuplot bugs report #1773922
   The standard font selection dialog is somehow broken after saving options.
   Use the font palette dialog instead.
*/
/* #define STANDARD_FONT_DIALOG 1 */
#undef STANDARD_FONT_DIALOG

/*==== d e b u g g i n g =====================================================*/

#ifdef HAVE_EXCEPTQ
# include <exceptq.h>
/* missing definition */
BOOL    LoadExceptq(EXCEPTIONREGISTRATIONRECORD* pExRegRec,
                    const char* pOpts, const char* pInfo);
#endif
#ifdef HAVE_PMPRINTF
# include <pmprintf.h>
#endif
#define DEBUG_IMAGE(a) // PmPrintf a
#define DEBUG_COLOR(a) // PmPrintf a
#define DEBUG_FONT(a) // PmPrintf a
#define DEBUG_LINES(a) // PmPrintf a
#define TEXT_DEBUG(x) // PmPrintf x

/*==== l o c a l    d a t a ==================================================*/

static long lLineTypes[7] = {
    LINETYPE_SOLID,
    LINETYPE_SHORTDASH,
    LINETYPE_DOT,
    LINETYPE_DASHDOT,
    LINETYPE_LONGDASH,
    LINETYPE_DOUBLEDOT,
    LINETYPE_DASHDOUBLEDOT
};

static long lCols[16] = {
    CLR_BLACK,
    CLR_DARKGRAY,
    CLR_RED,
    CLR_GREEN,
    CLR_BLUE,
    CLR_PINK,
    CLR_CYAN,
    CLR_BROWN,
    CLR_YELLOW,
    CLR_DARKBLUE,
    CLR_DARKRED,
    CLR_DARKGREEN,
    CLR_DARKCYAN,
    CLR_DARKPINK,
    CLR_PALEGRAY,
    CLR_WHITE
};

static BOOL bCols_init = FALSE;

/* Note that there are 16 colours used for drawing. The two more
   entries in this array were added by Ilya to avoid drawing white
   line on white background. */
#define nColors 16
static LONG alColourTable[nColors + 2];

static BOOL bPMPaletteMode = TRUE; /* do we use PM palette manager? */

/* RGB equivalents of all normal CLR_xxx colour constants
   Note: First entry corresponds to CLR_WHITE, which is negative (-2)!
*/
static LONG rgb_colors[18];
/* macro to facilitate translation */
#define RGB_TRANS(v) rgb_colors[(v)-CLR_WHITE]

/* A value of 0 indicates direct rgb colors without palette */
#define   RGB_PALETTE_SIZE 0    /* size of the 'virtual' palette used for
				   translation of index to RGB value */

/* GNUBUF is the size of the buffer for the data stream from gnuplot.
   Requests larger than this are split. By default, we use 128kB. */
#define   GNUBUF    131072      /* buffer for gnuplot commands */
#define   PIPEBUF   4096        /* size of pipe buffers */

#define   PAUSE_DLG 1           /* pause handled in dialog box */
#define   PAUSE_BTN 2           /* pause handled by menu item */
#define   PAUSE_GNU 3           /* pause handled by Gnuplot */

#define   DEFLW     20

static HDC      hdcScreen;
static HPS      hpsScreen;     /* screen pres. space */
static int      iSeg = 1;

static HSWITCH hSwitch = 0;    /* switching between windows */
static SWCNTRL swGnu;

static BOOL     bColours = TRUE;
static BOOL     bShellPos = FALSE;
static BOOL     bPopFront = TRUE;
static BOOL     bKeepRatio = TRUE;

static double   multLineHor  = 1.; /* Horizontal and vertical spacing shifts */
static double   multLineVert = 0.; /* for multiline prints.		    */

static int	codepage = 0;

static int 	ulMouseSprintfFormatItem = IDM_MOUSE_FORMAT_XcY;
static const  char  *SetDataStyles[] = {
    "boxes", "dots", "fsteps", "histeps", "impulses",
    "lines", "linespoints", "points", "steps"
};

static ULONG    ulShellPos[4];
static PAUSEDATA pausedata = {sizeof(PAUSEDATA), NULL, NULL};
static char     szInitialFontNameSize[FONTBUF] = INITIAL_FONT;
static int      iInitialFontSize;
static char     *szInitialFontName;
static char     szFontNameSize[FONTBUF];	/* default name and size, format: "10.Helvetica" */
static char	szCurrentFontNameSize[FONTBUF];	/* currently selected font */
static unsigned fontscale = 100;	/* scaling factor for font size in percent */
static PRQINFO3 infPrinter = { "" };
static QPRINT   qPrintData = {
    sizeof(QPRINT), 0.0, 0.0, 1.0, 1.0, 0,
    "", "", &infPrinter, 0, NULL
};
//static HEV      semStartSeq;   /* semaphore to start things in right sequence */
static HEV      semPause;
static HMTX     semHpsAccess;
static ULONG    ulPauseReply = 1;
static ULONG    ulPauseMode  = PAUSE_DLG;
static ULONG    ulPauseItem  = IDM_PAUSEDLG;

static HWND     hSysMenu;

            /* stuff for screen-draw thread control */

static TID     tidDraw, tidSpawn;

static BOOL    breakDrawing = FALSE;
            /* //PM flag for stopping (long) drawings */

            /* font data */

static int lSupOffset = 0;
static int lSubOffset = 0;
static int lBaseSupOffset = 0;
static int lBaseSubOffset = 0;
static int lCharWidth = 217;
static int lCharHeight = 465;

/* useMouse is set when user switches mousing on, e.g. the mouse is allowed */
static BOOL useMouse = FALSE;

/* gnuplot's PM terminal sends GR_MOUSECAPABLE message from its init routine, which
   sets the variable below. Then we are sure that we talk to a mouse-capable
   terminal and can read the mouse data from the pipe.
   Non-mouseable versions of PM terminal or non-gnuplot programs using
   gnupmdrv will let this variable unset, thus no mousing occurs.
*/
static BOOL mouseTerminal = FALSE;

/* Lock (hide) mouse when building the plot (redrawing screen).
   Otherwise gnupmdrv would crash when trying to display mouse position
   in a window not yet plotted.
*/
static BOOL lock_mouse = TRUE;

/* Status of zoom box */
static struct Zoombox {
    BOOL on;		/* set during zooming */
    POINTL from, to;		/* corners of the zoom box */
    char * text1, * text2;	/* texts in the corners (i.e. positions) */
} zoombox = { FALSE, {0,0}, {0,0}, NULL, NULL };

/* Structure for the ruler: on/off, position,... */
static struct {
   BOOL on;
   int x, y;  /* ruler position */
} ruler = { 0, 0, 0 };

/* Status of the line from ruler to cursor */
static struct {
    BOOL on;	/* ruler line active ? */
    int x, y;	/* ruler line end position (previous cursor position) */
} ruler_lineto = { FALSE, 0, 0 };

// Pointer definition
HWND hptrDefault, hptrCrossHair, hptrScaling,
    hptrRotating, hptrZooming, hptrCurrent;

// After passing gpPMmenu in PM_pipe and SET_GRAPHICS, it seems not to be possible
// to update the menu. Thus this flag is set on and menu is updated
// afterwards.
struct t_gpPMmenu gpPMmenu;
BOOL gpPMmenu_update_req = FALSE;

// colours of mouse-relating drawings (CLR_DEFAULT is not allowed!)
#define COLOR_MOUSE    CLR_BLACK    // mouse position
#define COLOR_ANNOTATE CLR_BLACK    // annotating strings (MB3)
#define COLOR_RULER    CLR_DARKPINK // colour of the ruler
#define COLOR_ERROR    CLR_RED      // colour of error messages

static enum JUSTIFY jmode;

static SIZEF sizBaseSubSup;
static SIZEF sizCurSubSup;
static SIZEF sizCurFont;
static long lVOffset = 0;
static SIZEF sizBaseFont;

static struct _ft {
    char *name;
    int   codepage;
    BOOL  bold;
    BOOL  italic;
    LONG  lcid;
} tabFont[256] = {
    {NULL, 0, FALSE, FALSE, 0L},
    {NULL}
};

typedef struct image_list_entry {
    PBITMAPINFO2 pbmi;
    PBYTE image;
    struct image_list_entry *next;
} image_list_entry;
static image_list_entry *image_list = NULL;


/*==== f u n c t i o n s =====================================================*/

int             DoPrint(HWND);
HBITMAP         CopyToBitmap(HPS);
HMF             CopyToMetaFile(HPS);
MRESULT         WmClientCmdProc(HWND , ULONG, MPARAM, MPARAM);
void            ChangeCheck(HWND, USHORT, USHORT);
BOOL            QueryIni(HAB);
static void     SaveIni(HWND);
static void     ThreadDraw(void*);
static void     DoPaint(HWND, HPS);
static void     SelectFont(HPS, char *);
static void     SwapFont(HPS, char *);
static void     CopyToClipBrd(HWND);
static void     ReadGnu(void*);
static void     SetFillStyle(HPS hps, int style);
static HPS      InitScreenPS(void);
static APIRET   BufRead(HFILE, void*, int, PULONG);
static int      GetNewFont(HWND, HPS);
void            SigHandler(int);
static void     FontExpand(char *);
#ifdef PM_KEEP_OLD_ENHANCED_TEXT
static char    *ParseText(HPS, char *, BOOL, char *, int, int, BOOL, BOOL);
static void     CharStringAt(HPS, int, int, int, char *);
static int      QueryTextBox(HPS, int, char *);
#endif
static void     LMove(HPS hps, POINTL *p);
static void     LLine(HPS hps, POINTL *p);
static int      LType(int iType);

/* Functions related to mouse processing */
static void     TextToClipboard(PCSZ);
static void     GetMousePosViewport(HWND hWnd, int *mx, int *my);
static void     MousePosToViewport(int *x, int *y, SHORT mx, SHORT my);
static void     gpPMmenu_update(void);
static void     ResetStatusLineText(void);
static void     DisplayStatusLine(HPS hps);
static void     UpdateStatusLine(HPS hps, char *text);
static void     DrawMouseText(HPS hps, PPOINTL pt, char * text, LONG len);
static void     DrawZoomBox(void);
static void     DrawRuler(void);
static void     DrawRulerLineTo(void);

#define IGNORE_MOUSE (!mouseTerminal || !useMouse || lock_mouse)
/*  don't react to mouse in the event handler, and avoid some crashes */

/* Drag'n'Drop support from PMEmacs */
struct drop {
    unsigned long cookie;
    unsigned len;
    char str[CCHMAXPATH];
};


/*==== c o d e ===============================================================*/

/* Our own versions of strlcpy and strlcat, required only for EMX. */
#ifndef __KLIBC__
#undef strlcpy
static size_t
strlcpy(char *dst, const char *src, size_t size)
{
    size_t len = strlen(src);
    if (size > 0) {
	strncpy(dst, src, size);
	if (len >= size)
	    dst[size - 1] = NUL;
    }
    return len;
}

#undef strlcat
size_t
strlcat(char *dst, const char *src, size_t size)
{
    size_t len = strlen(dst);
    return strlcpy(dst + len, src, size - len) + len;
}
#endif


/* An object is being dragged over the client window.  Check whether
   it can be dropped or not. */

void
report_error(HWND hWnd, char* s)
{
      HPS hps;
      POINTL pt;
      RECTL rc;

      hps = WinGetPS(hWnd);
      GpiSetMix(hps, FM_OVERPAINT);
      /* clear the rectangle below the text */
      /* GpiSetColor(hps, RGB_TRANS(CLR_CYAN)); */
      GpiSetColor(hps, RGB_TRANS(CLR_BACKGROUND));
      pt.x = 0;
      pt.y = 0;
      GpiMove(hps,&pt);
      /* clear the whole line */
      GpiQueryPageViewport(hpsScreen,&rc);
      pt.x = rc.xRight;
      pt.y = 16;
      GpiBox(hps, DRO_FILL, &pt, 0,0);
      /* now write the mouse position on the screen */
      GpiSetColor(hps, RGB_TRANS(COLOR_ERROR)); /* set text color */
      GpiSetCharMode(hps,CM_MODE1);

      pt.x = 2;
      pt.y = 2;
      GpiCharStringAt(hps,&pt,(long)strlen(s),s);
      WinReleasePS(hps);
}


MRESULT
drag_over(HWND hWnd, PDRAGINFO pDraginfo)
{
    PDRAGITEM pditem;
    USHORT indicator, operation;

/* Redefine this macro for debugging. */
#define DRAG_FAIL(x) report_error(hWnd,x)

    indicator = DOR_NODROPOP; operation = DO_COPY;
    if (!DrgAccessDraginfo(pDraginfo))
	DRAG_FAIL("DrgAccessDraginfo failed");
    else if (!(pDraginfo->usOperation == DO_DEFAULT
	       || pDraginfo->usOperation == DO_COPY))
	DRAG_FAIL("Invalid operation");
    else if (DrgQueryDragitemCount(pDraginfo) < 1)
	DRAG_FAIL("Invalid count");
    else {
	pditem = DrgQueryDragitemPtr(pDraginfo, 0);
	if (!(pditem->fsSupportedOps & DO_COPYABLE))
	    DRAG_FAIL("Not copyable");
	else if (!DrgVerifyRMF(pditem, "DRM_OS2FILE", NULL))
	    DRAG_FAIL("DrgVerifyRMF failed");
	else {
	    /* The object can be dropped (copied). */
	    indicator = DOR_DROP; operation = DO_COPY;
	}
    }
    DrgFreeDraginfo(pDraginfo);
    return (MRFROM2SHORT(indicator, operation));
}


/* An object is dropped on the client window. */

MRESULT
drag_drop(HWND hwnd, PDRAGINFO pDraginfo)
{
  PDRAGITEM pditem;
  POINTL ptl;
  char name[CCHMAXPATH];
  char path[CCHMAXPATH];
  char *p;
  int count, idx, len;

  DrgAccessDraginfo(pDraginfo);
  ptl.x = pDraginfo->xDrop; ptl.y = pDraginfo->yDrop;
  WinMapWindowPoints(HWND_DESKTOP, hwnd, &ptl, 1);
  count = DrgQueryDragitemCount(pDraginfo);
  for (idx = 0; idx < count; ++idx) {
      pditem = DrgQueryDragitemPtr(pDraginfo, idx);
      DrgQueryStrName(pditem->hstrContainerName, sizeof(path), path);
      DrgQueryStrName(pditem->hstrSourceName, sizeof(name), name);
      DrgSendTransferMsg(pditem->hwndItem, DM_ENDCONVERSATION,
			 MPFROMLONG(pditem->ulItemID),
			 MPFROMSHORT(DMFL_TARGETSUCCESSFUL));
      len = strlen(path);
      if (len >= 1 && strchr("\\/:", path[len-1]) == NULL)
	  path[len++] = '/';
      if (len + strlen(name) + 1 <= sizeof(path)) {
	  strcpy(path + len, name);
	  for (p = path; *p != 0; ++p)
	      if (*p == '\\')
		  *p = '/';
	  if (input_from_PM_Terminal) {
	      sprintf(input_from_PM_Terminal,
		      "set loadpath \"%.*s\"; load \"%s\"",
		      len, path, path);
	      gp_execute(0);
	  }
      } else
	  report_error(hwnd, "Dropped path name too long");
  } /* for (idx) */

  DrgDeleteDraginfoStrHandles(pDraginfo);
  DrgFreeDraginfo(pDraginfo);
  return(0);
}

#if 0
/* A color has been dropped on a frame (or the background color has
   been changed with WinSetPresParam). */

static void
drop_color(HWND hwnd)
{
    RGB rgb;
    char buf[3];

    if (WinQueryPresParam(hwnd, PP_BACKGROUNDCOLOR, 0, NULL,
			  sizeof(rgb), &rgb,
			  QPF_NOINHERIT | QPF_PURERGBCOLOR) == sizeof(rgb)
	) {
	buf[0] = rgb.bRed;
	buf[1] = rgb.bGreen;
	buf[2] = rgb.bBlue;
#if CAN_DROP_COLOR
	send_drop_color(buf, 3);
#endif
    }
}
#endif

/*
**  Window proc for main window
**  -- passes most stuff to active child window via WmClientCmdProc
**  -- passes DDE messages to DDEProc
*/
MRESULT
EXPENTRY DisplayClientWndProc(HWND hWnd, ULONG message, MPARAM mp1, MPARAM mp2)
{
    static RECTL   rectlPaint = { 0, 0, 0, 0 };
    static int     iPaintCount = 0;
    static int     firstcall = 1;
    static int     prev_mx = 0, prev_my = 0;		/* previous mouse position */
    int            mx, my;				/* current mouse position  */

#if 1
    GetMousePosViewport(hWnd, &mx, &my);
#else
    /* The following cannot be used because `message' is not OK for
     * WM_CHAR */
    /* macro which gets mouse coords in pixels */
    mx = MOUSEMSG(&message)->x;
    my = MOUSEMSG(&message)->y;
    /* viewport units --- this trashes mx,my! */
    MousePosToViewport(&mx, &my, mx, my);
#endif

    /* Graphics part, SET_GRAPHICS, required to update menu */
    if (gpPMmenu_update_req)
	gpPMmenu_update();

    /* mouse events first */
    switch (message) {
    case WM_MOUSEMOVE:
	if (IGNORE_MOUSE) {
	    WinSetPointer(HWND_DESKTOP, hptrDefault); /* set default pointer */
	    return 0L;
	}
	/* check for valid mouse parameters */
	if ((mx <= 0) && (my <= 0))
	    return 0L;
	/* was the mouse moved? */
	if ((prev_mx != mx) || (prev_my != my)) {
	    WinSetPointer(HWND_DESKTOP, hptrCurrent);
	    if (zoombox.on) {
		DrawZoomBox(); /* erase zoom box */
		zoombox.to.x = mx; zoombox.to.y = my;
		DrawZoomBox(); /* draw new zoom box */
	    }
	    if (ruler.on && ruler_lineto.on) {
		DrawRulerLineTo(); /*  erase current line */
		ruler_lineto.x = mx; ruler_lineto.y = my;
		DrawRulerLineTo(); /*  draw new line */
	    }
	    /* track(show) mouse position -- send the event to gnuplot */
	    gp_exec_event(GE_motion, mx, my, 0, 0, 0);
	}
	prev_mx = mx;
	prev_my = my;
	return 0L; /* end of WM_MOUSEMOVE */

    case WM_BUTTON1DOWN:
	WinSetFocus(HWND_DESKTOP, hWnd);
	WinSetWindowPos(hwndFrame, HWND_TOP, 0,0,0,0, SWP_ACTIVATE|SWP_ZORDER);
	if (! IGNORE_MOUSE)
	    gp_exec_event(GE_buttonpress, mx, my, 1, 0, 0);
	return 0L;

    case WM_BUTTON2DOWN:
	WinSetFocus(HWND_DESKTOP, hWnd);
	WinSetWindowPos(hwndFrame, HWND_TOP, 0,0,0,0, SWP_ACTIVATE|SWP_ZORDER);
	if (!IGNORE_MOUSE)
	    gp_exec_event(GE_buttonpress, mx, my, 3, 0, 0);
	return 0L;

    case WM_BUTTON3DOWN:
	WinSetFocus(HWND_DESKTOP, hWnd);
	WinSetWindowPos(hwndFrame, HWND_TOP, 0,0,0,0, SWP_ACTIVATE|SWP_ZORDER);
	if (!IGNORE_MOUSE)
	    gp_exec_event(GE_buttonpress, mx, my, 2, 0, 0);
	return 0L;

    case WM_BUTTON1DBLCLK:
	if (!IGNORE_MOUSE)
	    gp_exec_event(GE_buttonrelease, mx, my, 1, 0, 0);
	return 0L;

    case WM_BUTTON2DBLCLK:
	if (!IGNORE_MOUSE)
	    /* Note: 9999 should be replaced by time! */
	    gp_exec_event(GE_buttonrelease, mx, my, 3, 9999, 0);
	return 0L;

    case WM_BUTTON3DBLCLK:
	if (!IGNORE_MOUSE)
	    gp_exec_event(GE_buttonrelease, mx, my, 2, 9999, 0);
	return 0L;

    case WM_BUTTON1UP:
	if (!IGNORE_MOUSE)
	    gp_exec_event(GE_buttonrelease, mx, my, 1, 9999, 0);
	return 0L;

    case WM_BUTTON2UP:
	if (!IGNORE_MOUSE)
	    gp_exec_event(GE_buttonrelease, mx, my, 3, 9999, 0);
	return 0L;

    case WM_BUTTON3UP:
	if (!IGNORE_MOUSE)
	    gp_exec_event(GE_buttonrelease, mx, my, 2, 9999, 0);
	return 0L;

    } /* switch over mouse events */


    switch (message) {
    case WM_CREATE:
    {
	SIZEL   sizlPage;
	LONG    NumColors;
	LONG    PalSupport;

	/* set initial values */
	ChangeCheck(hWnd, IDM_LINES_THICK, bWideLines?IDM_LINES_THICK:0);
	ChangeCheck(hWnd, IDM_COLOURS, bColours?IDM_COLOURS:0);
	ChangeCheck(hWnd, IDM_FRONT, bPopFront?IDM_FRONT:0);
	ChangeCheck(hWnd, IDM_KEEPRATIO, bKeepRatio?IDM_KEEPRATIO:0);
	ChangeCheck(hWnd, IDM_USEMOUSE, useMouse?IDM_USEMOUSE:0);
#if 0
	ChangeCheck(hWnd, IDM_MOUSE_POLAR_DISTANCE, mousePolarDistance?IDM_MOUSE_POLAR_DISTANCE:0);
#endif
	switch (ulPauseMode) {
	case PAUSE_DLG:
	    ChangeCheck(hWnd, ulPauseItem, IDM_PAUSEDLG);
	    ulPauseItem = IDM_PAUSEDLG;
	    break;
	case PAUSE_BTN:
	    ChangeCheck(hWnd, ulPauseItem, IDM_PAUSEBTN);
	    ulPauseItem = IDM_PAUSEBTN;
	    break;
	case PAUSE_GNU:
	    ChangeCheck(hWnd, ulPauseItem, IDM_PAUSEGNU);
	    ulPauseItem = IDM_PAUSEGNU;
	    break;
	}

	/* disable close from system menu(close only from gnuplot) */
	hApp = WinQueryWindow(hWnd, QW_PARENT); /* temporary assignment.. */
	hSysMenu = WinWindowFromID(hApp, FID_SYSMENU);

	/* setup semaphores */
	/* DosCreateEventSem(NULL, &semStartSeq, 0L, 0L); */
	DosCreateEventSem(NULL, &semPause, 0L, 0L);
	DosCreateMutexSem(NULL, &semHpsAccess, 0L, 1L);

	/* create a dc and hps to draw on the screen */
	hdcScreen = WinOpenWindowDC(hWnd);

	/* How many colors can be displayed ? */
	DevQueryCaps(hdcScreen, CAPS_COLORS, 1, &NumColors);
	/* Is Palette Manager supported at all? */
	DevQueryCaps(hdcScreen, CAPS_ADDITIONAL_GRAPHICS, 1, &PalSupport);

	/* Determine if PM Palette Manager should be used */
	bPMPaletteMode = (PalSupport & CAPS_PALETTE_MANAGER) &&
			 (NumColors <= 256);
	DEBUG_COLOR(( "WM_CREATE: colors = %i, pm = %i, rgb = %i",
		NumColors, PalSupport & CAPS_PALETTE_MANAGER, !bPMPaletteMode ));

	sizlPage.cx = 0; sizlPage.cy = 0;
	sizlPage.cx = GNUXPAGE; sizlPage.cy = GNUYPAGE;
	hpsScreen = GpiCreatePS(hab, hdcScreen, &sizlPage,
				PU_HIMETRIC|GPIT_NORMAL|GPIA_ASSOC);
	/* spawn server for GNUPLOT ... */
	tidSpawn = _beginthread(ReadGnu, NULL, 32768, NULL);

	/* initialize pointers */
	hptrDefault = WinQuerySysPointer(HWND_DESKTOP, SPTR_ARROW, FALSE);
	hptrCrossHair = WinLoadPointer(HWND_DESKTOP,(ULONG)0, IDP_CROSSHAIR);
	hptrScaling = WinLoadPointer(HWND_DESKTOP,(ULONG)0, IDP_SCALING);
	hptrRotating = WinLoadPointer(HWND_DESKTOP,(ULONG)0, IDP_ROTATING);
	hptrZooming = WinQuerySysPointer(HWND_DESKTOP, SPTR_MOVE, FALSE);
	hptrCurrent = hptrCrossHair;

	break;
    }

    case WM_GPSTART:
	/* Show the Mouse menu if connected to mouseable PM terminal */
	if (1 || mouseTerminal) /* PM: workaround for a bug---SEE "BUGS 2" IN README!!! */
	    /* if (mouseTerminal) */
	    WinEnableMenuItem(WinWindowFromID(
				  WinQueryWindow(hApp, QW_PARENT),
				  FID_MENU),
			      IDM_MOUSE, TRUE);
	if (!input_from_PM_Terminal) { /* no feedback */
#define NOFEEDBACK(X)							\
	    WinEnableMenuItem(WinWindowFromID(				\
				  WinQueryWindow(hApp, QW_PARENT),	\
				  FID_MENU),				\
			      X, FALSE)
	    NOFEEDBACK(IDM_SET_GRID);
	    NOFEEDBACK(IDM_SET_LINLOGY);
	    NOFEEDBACK(IDM_SET_AUTOSCALE);
	    NOFEEDBACK(IDM_DO_REPLOT);
	    NOFEEDBACK(IDM_DO_RELOAD);
	    NOFEEDBACK(IDM_DO_SENDCOMMAND);
	    NOFEEDBACK(IDM_SET);
#undef NOFEEDBACK
	}

	/* get details of command-line window */
	hSwitch = WinQuerySwitchHandle(0, ppidGnu);
	WinQuerySwitchEntry(hSwitch, &swGnu);

	if (firstcall) {
	    /* set size of this window */
            WinSetWindowPos(
		WinQueryWindow(hWnd, QW_PARENT),
		bPopFront?HWND_TOP:swGnu.hwnd,
		ulShellPos[0],
		ulShellPos[1],
		ulShellPos[2],
		ulShellPos[3],
		bShellPos
		? (bPopFront
		   ? SWP_SIZE|SWP_MOVE|SWP_SHOW|SWP_ACTIVATE
		   : SWP_SIZE|SWP_MOVE|SWP_SHOW|SWP_ZORDER)
		: (bPopFront
		   ? SWP_SHOW|SWP_ACTIVATE
		   : SWP_SHOW|SWP_ZORDER));
            signal(SIGTERM, SigHandler);
	    firstcall = 0;
	}
	if (!bPopFront)
	    WinSwitchToProgram(hSwitch);
	DosReleaseMutexSem(semHpsAccess);
	return 0;

    case WM_COMMAND:
	return WmClientCmdProc(hWnd, message, mp1, mp2);

    case WM_CHAR:
    {
	/* Note: doc for WM_CHAR is available in PMMSG.INF */
	USHORT usFlag = SHORT1FROMMP(mp1); /* keyboard control codes */
	SHORT  key = SHORT2FROMMP(mp2);    /* virtual key code */
	static int last_modifier_mask = -99;
	int modifier_mask =
	    ((usFlag & KC_SHIFT) ? Mod_Shift : 0)
	    | ((usFlag & KC_CTRL) ? Mod_Ctrl : 0)
	    | ((usFlag & KC_ALT) ? Mod_Alt : 0);

	if (modifier_mask != last_modifier_mask) {
	    gp_exec_event(GE_modifier, mx, my, modifier_mask, 0, 0);
	    last_modifier_mask = modifier_mask;
	}
#if 0
	if (!(usFlag & KC_SCANCODE))
	    return 0L; // only modifier mask has changed */
#endif
	if (usFlag & KC_KEYUP)
	    return 0L;   /* ignore key release events */

	switch (key) {
	case VK_SPACE: {
#ifndef DISABLE_SPACE_RAISES_CONSOLE
	    /* raise gnuplot's window */
	    HWND hw = WinQueryWindow(swGnu.hwnd, QW_BOTTOM);
	    WinSetFocus(HWND_DESKTOP, hw);
	    WinSendMsg(hw, message,
		       MPFROM2SHORT((USHORT)(KC_SCANCODE), 1),
		       0 /* MPFROMSHORT(key) */
		);
	    WinSwitchToProgram(hSwitch);
#else
	    key = ' ';
#endif /* DISABLE_SPACE_RAISES_CONSOLE */
	    break;
	}
	    /* remap virtual keys to gnuplot's codes: */
	case VK_BACKSPACE:
	    key = GP_BackSpace;
	    break;
	case VK_TAB:
	    key = GP_Tab;
	    break;
	case VK_NEWLINE:
	    key = GP_Return;
	    break;
	case VK_PAUSE:
	    key = GP_Pause;
	    break;
	case VK_SCRLLOCK:
	    key = GP_Scroll_Lock;
	    break;
	case VK_SYSRQ:
	    key = GP_Sys_Req;
	    break;
	case VK_ESC:
	    key = GP_Escape;
	    break;
	case VK_DELETE:
	    key = GP_Delete;
	    break;
	case VK_INSERT:
	    key = GP_KP_Insert;
	    break;
	case VK_HOME:
	    key = GP_Home;
	    break;
	case VK_LEFT:
	    key = GP_Left;
	    break;
	case VK_UP:
	    key = GP_Up;
	    break;
	case VK_RIGHT:
	    key = GP_Right;
	    break;
	case VK_DOWN:
	    key = GP_Down;
	    break;
	case VK_END:
	    key = GP_End;
	    break;
	case VK_PAGEUP:
	    key = GP_PageUp;
	    break;
	case VK_PAGEDOWN:
	    key = GP_PageDown;
	    break;
	case VK_F1:
	    key = GP_F1;
	    break;
	case VK_F2:
	    key = GP_F2;
	    break;
	case VK_F3:
	    key = GP_F3;
	    break;
	case VK_F4:
	    key = GP_F4;
	    break;
	case VK_F5:
	    key = GP_F5;
	    break;
	case VK_F6:
	    key = GP_F6;
	    break;
	case VK_F7:
	    key = GP_F7;
	    break;
	case VK_F8:
	    key = GP_F8;
	    break;
	case VK_F9:
	    key = GP_F9;
	    break;
	case VK_F10:
	    key = GP_F10;
	    break;
	case VK_F11:
	    key = GP_F11;
	    break;
	case VK_F12:
	    key = GP_F12;
	    break;
	case VK_SHIFT:
	case VK_CTRL:
	case VK_ALT:
	    gp_exec_event(GE_modifier, mx, my, modifier_mask, 0, 0);
	    return 0L;
	default:
	    key = SHORT1FROMMP(mp2); /* character key code */
	} /* switch(key) */

	if (key)
	    gp_exec_event(GE_keypress, mx, my, key, 0, 0);

	return 0L;
    } /*case(WM_CHAR) */

    case WM_DESTROY:
	if (WinSendMsg(hWnd, WM_USER_PRINT_QBUSY, 0L, 0L) != 0L) {
	    WinMessageBox(HWND_DESKTOP,
			  hWnd,
			  "Still printing - not closed",
			  APP_NAME,
			  0,
			  MB_OK | MB_ICONEXCLAMATION);
	    return 0L;
	}
	return(WinDefWindowProc(hWnd, message, mp1, mp2));

    case WM_PAINT:
    {
	ULONG ulCount;
	PID pid; TID tid;
	HPS hps_tmp;
	RECTL rectl_tmp;

	DosQueryMutexSem(semHpsAccess, &pid, &tid, &ulCount);
	if ((ulCount > 0) && (tid != tidDraw)) {
	    /* simple repaint while building plot or metafile */
	    /* use temporary PS                   */
	    lock_mouse = TRUE; /* PM: this may help against gnupmdrv crashes */
	    hps_tmp = WinBeginPaint(hWnd, 0, &rectl_tmp);
	    WinFillRect(hps_tmp, &rectl_tmp, CLR_BACKGROUND);
	    WinEndPaint(hps_tmp);
	    /* add dirty rectangle to saved rectangle     */
	    /* to be repainted when PS is available again */
	    WinUnionRect(hab, &rectlPaint, &rectl_tmp, &rectlPaint);
	    lock_mouse = FALSE;
	    iPaintCount ++;
	    break;
	}
	lock_mouse = TRUE;
	WinInvalidateRect(hWnd, &rectlPaint, TRUE);
	DoPaint(hWnd, hpsScreen);
	WinSetRectEmpty(hab, &rectlPaint);
	lock_mouse = FALSE;
	break;
    }

    case WM_FOCUSCHANGE:
	if (!SHORT1FROMMP(mp2)) /* losing focus */
	    gp_exec_event(GE_reset, 0, 0, 0, 0, 0);
	break;

    case WM_SIZE :
	WinInvalidateRect(hWnd, NULL, TRUE);
	break;

    case WM_PRESPARAMCHANGED: {
	char *pp = malloc(FONTBUF);
	ULONG ulID;

	if (WinQueryPresParam(hWnd,
			      PP_FONTNAMESIZE,
			      0,
			      &ulID,
			      FONTBUF,
			      pp,
			      QPF_NOINHERIT) != 0L) {
	    strlcpy(szFontNameSize, pp, FONTBUF);
	    DEBUG_FONT(("WM_PRESPARAMCHANGED: \"%s\"", szFontNameSize));
	    WinInvalidateRect(hWnd, NULL, TRUE);
	}
	free(pp);
#ifndef STANDARD_FONT_DIALOG
	gp_exec_event(GE_replot, 0, 0, 0, 0, 0);
#endif
	break;
#if 0
	if (LONGFROMMP(mp1) == PP_BACKGROUNDCOLOR)
	    drop_color(hWnd);
	return 0;
#endif
    }

    case WM_USER_PRINT_BEGIN:
    case WM_USER_PRINT_OK :
    case WM_USER_DEV_ERROR :
    case WM_USER_PRINT_ERROR :
    case WM_USER_PRINT_QBUSY :
	return(PrintCmdProc(hWnd, message, mp1, mp2));

    case WM_GNUPLOT:
	/* display the plot */
	lock_mouse = TRUE;
	if (bPopFront) {
	    SWP swp; /* pop to front only if the window is not minimized */

	    if ((WinQueryWindowPos(hwndFrame,(PSWP) &swp) == TRUE)
		&&((swp.fl & SWP_MINIMIZE) == 0))
		WinSetWindowPos(hwndFrame, HWND_TOP, 0,0,0,0, SWP_ACTIVATE|SWP_ZORDER);
	}
	if (iPaintCount > 0) { /* if outstanding paint messages, repaint */
	    WinInvalidateRect(hWnd, &rectlPaint, TRUE);
	    iPaintCount = 0;
	}
	lock_mouse = FALSE;
	return 0L;

    case WM_PAUSEPLOT:
    {
	SWP swp; /* restore the window if it has been minimized */

	if ((WinQueryWindowPos(hwndFrame, &swp) == TRUE)
	    && ((swp.fl & SWP_MINIMIZE) != 0))
	    WinSetWindowPos(hwndFrame, HWND_TOP, 0,0,0,0,
			    SWP_RESTORE|SWP_SHOW|SWP_ACTIVATE);
	/* put pause message on screen, or enable 'continue' button */
	if (ulPauseMode == PAUSE_DLG) {
	    pausedata.pszMessage = (char*) mp1;
	    WinLoadDlg(HWND_DESKTOP,
		       hWnd,
		       (PFNWP)PauseMsgDlgProc,
		       0L,
		       IDD_PAUSEBOX,
		       &pausedata);
	} else {
	    WinEnableMenuItem(WinWindowFromID(
				  WinQueryWindow(hWnd, QW_PARENT), FID_MENU),
			      IDM_CONTINUE, TRUE);
	}
	return 0L;
    }

    case WM_PAUSEEND:
	/* resume plotting */
	ulPauseReply = (ULONG) mp1;
	DosPostEventSem(semPause);
	return 0L;

    case DM_DRAGOVER:
	/* Determine whether the object can be dropped. */
	return drag_over(hWnd, (PDRAGINFO) mp1);

    case DM_DROP:
	/* Drop an object. */
	return drag_drop(hWnd, (PDRAGINFO) mp1);

    default:         /* Passes it on if unprocessed    */
	return WinDefWindowProc(hWnd, message, mp1, mp2);

    } /* switch(message) */

    return 0L;
}


/* passing either n(n>=0) or f(when n==-1) */
void
SetMouseCoords(HWND hWnd, MPARAM mp1, int n, char *f)
{
    char s[100];

    ChangeCheck(hWnd, ulMouseSprintfFormatItem, SHORT1FROMMP(mp1));
    ulMouseSprintfFormatItem = SHORT1FROMMP(mp1);
    if (n >= 0)
	sprintf(s, "set mouse mouseformat %i", n);
    else
	sprintf(s, "set mouse mouseformat \"%s\"", f);
    gp_execute(s);
}


/*
**   Handle client window command(menu) messages
*/
MRESULT
WmClientCmdProc(HWND hWnd, ULONG message, MPARAM mp1, MPARAM mp2)
{
    int mx, my;

    GetMousePosViewport(hWnd, &mx, &my);

    switch ((USHORT) SHORT1FROMMP(mp1)) {
    case IDM_ABOUT :    /* show the 'About' box */
    {
	char label[256];
	MB2INFO mb;

	sprintf(label,"gnuplot display for OS/2\nVersion %s patchlevel %s",
		gnuplot_version, gnuplot_patchlevel);
	mb.cb         = sizeof(mb);
	mb.hIcon      = WinLoadPointer(HWND_DESKTOP, 0, 1);
	mb.cButtons   = 1;
	mb.flStyle    = MB_CUSTOMICON | MB_MOVEABLE;
	mb.hwndNotify = NULLHANDLE;
	strcpy(mb.mb2d[0].achText, "~OK");
	mb.mb2d[0].idButton = 1;
	mb.mb2d[0].flStyle  = BS_PUSHBUTTON | BS_DEFAULT;
	WinMessageBox2(HWND_DESKTOP, hwndFrame,
		label,"GnuplotPM", ID_ABOUT, &mb);
	break;
    }

    case IDM_GPLOTINF:  /* view gnuplot.inf */
    {
	const char cmd_prefix[] = "start view ";
	const char helpfile[] = "gnuplot.inf";
	char *cmd;
	char *gnuplot_path;
	unsigned cmd_length;

	cmd_length = strlen(cmd_prefix) + strlen(helpfile);
	gnuplot_path = getenv("GNUPLOT");
	if (gnuplot_path != NULL)
	    cmd_length += strlen(gnuplot_path) + 1;

	cmd = (char *)malloc( cmd_length );
	strcpy(cmd, cmd_prefix);
	if (gnuplot_path != NULL) {
	    strcat(cmd, gnuplot_path);
	    strcat(cmd, "\\");
	}
	strcat(cmd, helpfile);
	system(cmd);
	free(cmd);
	break;
    }

    case IDM_PRINT :    /* print plot */
	if (SetupPrinter(hWnd, &qPrintData)) {
	    WinPostMsg(hWnd,
		       WM_USER_PRINT_BEGIN,
		       (MPARAM) &qPrintData,
		       (MPARAM) hpsScreen);
	}
	break;

    case IDM_PRINTSETUP :    /* select printer */
	WinDlgBox(HWND_DESKTOP,
		  hWnd ,
		  (PFNWP) QPrintersDlgProc,
		  0L,
		  IDD_QUERYPRINT,
		  qPrintData.szPrinterName);
	break;

    case IDM_LINES_THICK:
	/* change line setting */
	bWideLines = !bWideLines;
	ChangeCheck(hWnd, IDM_LINES_THICK, bWideLines?IDM_LINES_THICK:0);
	WinInvalidateRect(hWnd, NULL, TRUE);
	break;

    case IDM_COLOURS:
	/* change colour setting */
	bColours = !bColours;
	ChangeCheck(hWnd, IDM_COLOURS, bColours?IDM_COLOURS:0);
	WinInvalidateRect(hWnd, NULL, TRUE);
	break;

    case IDM_FRONT:
	/* toggle z-order forcing */
	bPopFront = !bPopFront;
	ChangeCheck(hWnd, IDM_FRONT, bPopFront?IDM_FRONT:0);
	break;

    case IDM_KEEPRATIO:
	/* toggle keep aspect ratio */
	bKeepRatio = !bKeepRatio;
	ChangeCheck(hWnd, IDM_KEEPRATIO, bKeepRatio?IDM_KEEPRATIO:0);
	WinInvalidateRect(hWnd, NULL, TRUE); /* redraw screen */
	break;

    case IDM_FONTS:
	if (GetNewFont(hWnd, hpsScreen)) {
	    WinInvalidateRect(hWnd, NULL, TRUE);
	}
	break;

    case IDM_SAVE:
	SaveIni(hWnd);
	break;

    case IDM_COPY:
	/* copy to clipboard */
	if (WinOpenClipbrd(hab)) {
	    CopyToClipBrd(hWnd);
	} else {
	    WinMessageBox(HWND_DESKTOP,
			  hWnd,
			  "Can't open clipboard",
			  APP_NAME,
			  0,
			  MB_OK | MB_ICONEXCLAMATION);
	}
	break;

    case IDM_CLEARCLIP :         /* clear clipboard */
	if (WinOpenClipbrd(hab)) {
	    WinEmptyClipbrd(hab);
	    WinCloseClipbrd(hab);
	} else {
	    WinMessageBox(HWND_DESKTOP,
			  hWnd,
			  "Can't open clipboard",
			  APP_NAME,
			  0,
			  MB_OK | MB_ICONEXCLAMATION);
	}
	break;

    case IDM_COMMAND:       /* go back to GNUPLOT command window */
	WinSwitchToProgram(hSwitch);
	break;

    case IDM_CONTINUE:
	WinPostMsg(hWnd, WM_PAUSEEND, (MPARAM)1L, (MPARAM)0L);
	WinEnableMenuItem(WinWindowFromID(
			      WinQueryWindow(hWnd, QW_PARENT), FID_MENU),
			  IDM_CONTINUE,
			  FALSE);
	break;

    case IDM_PAUSEGNU:  /* gnuplot handles pause */
	ChangeCheck(hWnd, ulPauseItem, IDM_PAUSEGNU);
	ulPauseItem = IDM_PAUSEGNU;
	ulPauseMode = PAUSE_GNU;
	break;

    case IDM_PAUSEDLG:  /* pause message in dlg box */
	ChangeCheck(hWnd, ulPauseItem, IDM_PAUSEDLG);
	ulPauseItem = IDM_PAUSEDLG;
	ulPauseMode = PAUSE_DLG;
	break;

    case IDM_PAUSEBTN:  /* pause uses menu button, no message */
	ChangeCheck(hWnd, ulPauseItem, IDM_PAUSEBTN);
	ulPauseItem = IDM_PAUSEBTN;
	ulPauseMode = PAUSE_BTN;
	break;

    case IDM_HELPFORHELP:
	WinSendMsg(WinQueryHelpInstance(hWnd),
		   HM_DISPLAY_HELP, 0L, 0L);
	return 0L;

    case IDM_EXTENDEDHELP:
	WinSendMsg(WinQueryHelpInstance(hWnd),
		   HM_EXT_HELP, 0L, 0L);
	return 0L;

    case IDM_KEYSHELP:
	WinSendMsg(WinQueryHelpInstance(hWnd),
		   HM_KEYS_HELP, 0L, 0L);
	return 0L;

    case IDM_HELPINDEX:
	WinSendMsg(WinQueryHelpInstance(hWnd),
		   HM_HELP_INDEX, 0L, 0L);
	return 0L;

	/* Now new mousing stuff: */

    case IDM_USEMOUSE: /* toggle using/not using mouse cursor tracking */
	useMouse = !useMouse;
	ChangeCheck(hWnd, IDM_USEMOUSE, useMouse ? IDM_USEMOUSE : 0);
	gp_execute(useMouse ? "set mouse" : "unset mouse");
	return 0L;

    case IDM_MOUSE_HELP:
	gp_exec_event(GE_keypress, mx, my, 'h', 1, 0);
	return 0L;

    case IDM_MOUSE_FORMAT_pXcYp:
	SetMouseCoords(hWnd, mp1, -1, "[%g, %g]");
	return 0L;
    case IDM_MOUSE_FORMAT_XcY:
	SetMouseCoords(hWnd, mp1, 1, NULL);
	return 0L;
    case IDM_MOUSE_FORMAT_TIMEFMT:
	SetMouseCoords(hWnd, mp1, 3, NULL);
	return 0L;
    case IDM_MOUSE_FORMAT_DATE:
	SetMouseCoords(hWnd, mp1, 4, NULL);
	return 0L;
    case IDM_MOUSE_FORMAT_TIME:
	SetMouseCoords(hWnd, mp1, 5, NULL);
	return 0L;
    case IDM_MOUSE_FORMAT_DATETIME:
	SetMouseCoords(hWnd, mp1, 6, NULL);
	return 0L;
    case IDM_MOUSE_FORMAT_X_Y:
	SetMouseCoords(hWnd, mp1, -1, "%g %g");
	return 0L;

    case IDM_MOUSE_POLAR_DISTANCE:
	/* toggle using/not using polar coords of distance */
	gp_execute(gpPMmenu.polar_distance
		   ? "set mouse nopolardistance"
		   : "set mouse polardistance");
	return 0L;

    case IDM_MOUSE_ZOOMNEXT: /* zoom to next level */
	gp_exec_event(GE_keypress, mx, my, 'n', 1, 0);
	return 0L;

    case IDM_MOUSE_UNZOOM: /* unzoom one level back */
	gp_exec_event(GE_keypress, mx, my, 'p', 1, 0);
	return 0L;

    case IDM_MOUSE_UNZOOMALL: /* unzoom to the first level */
	gp_exec_event(GE_keypress, mx, my, 'u', 1, 0);
	return 0L;

    case IDM_MOUSE_RULER:
    {
	int mx, my;

	GetMousePosViewport(hWnd, &mx, &my);
	gp_exec_event(GE_keypress, mx, my, 'r', 1, 0);
	return 0L;
    }

    case IDM_BREAK_DRAWING:
	breakDrawing = TRUE;
	return 0L;

    case IDM_SET_GRID:
	gp_exec_event(GE_keypress, mx, my, 'g', 1, 0);
	return 0L;

    case IDM_SET_LINLOGY:
	gp_exec_event(GE_keypress, mx, my, 'l', 1, 0);
	return 0L;

    case IDM_SET_AUTOSCALE:
	gp_execute("set autoscale; replot");
	return 0L;

    case IDM_DO_REPLOT:
	gp_exec_event(GE_replot, 0, 0, 0, 0, 0);
	return 0L;

    case IDM_DO_RELOAD:
	gp_execute("history !load");
	return 0L;

    case IDM_DO_SENDCOMMAND:
	if (input_from_PM_Terminal) {
	    if (pausing)
		DosBeep(440,111);
	    else
		WinDlgBox(HWND_DESKTOP, hWnd, SendCommandDlgProc,
			  NULLHANDLE, IDM_DO_SENDCOMMAND,
			  input_from_PM_Terminal);
	}
	return (MRESULT) 0;

    case IDM_SET_D_S_BOXES:
    case IDM_SET_D_S_DOTS:
    case IDM_SET_D_S_FSTEPS:
    case IDM_SET_D_S_HISTEPS:
    case IDM_SET_D_S_IMPULSES:
    case IDM_SET_D_S_LINES:
    case IDM_SET_D_S_LINESPOINTS:
    case IDM_SET_D_S_POINTS:
    case IDM_SET_D_S_STEPS:
	if (input_from_PM_Terminal)
            sprintf(input_from_PM_Terminal, "set style data %s; replot",
		    SetDataStyles[(USHORT) SHORT1FROMMP(mp1) - IDM_SET_D_S_BOXES ]);
	gp_execute(0);
	return 0L;

    case IDM_SET_F_S_BOXES:
    case IDM_SET_F_S_DOTS:
    case IDM_SET_F_S_FSTEPS:
    case IDM_SET_F_S_HISTEPS:
    case IDM_SET_F_S_IMPULSES:
    case IDM_SET_F_S_LINES:
    case IDM_SET_F_S_LINESPOINTS:
    case IDM_SET_F_S_POINTS:
    case IDM_SET_F_S_STEPS:
	if (input_from_PM_Terminal)
            sprintf(input_from_PM_Terminal, "set style function %s; replot",
		    SetDataStyles[(USHORT) SHORT1FROMMP(mp1) - IDM_SET_F_S_BOXES ]);
	gp_execute(0);
	return 0L;

    default :
	return WinDefWindowProc(hWnd, message, mp1, mp2);
    } /* switch(message) */

    return(NULL);
}


/*
**  Utility function:
**
**  move check mark from menu item 1 to item 2
*/
void
ChangeCheck(HWND hWnd , USHORT wItem1 , USHORT wItem2)
{
    HWND hMenu = WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT),
				 FID_MENU);

    if (wItem1 != 0)
        WinSendMsg(hMenu,
		   MM_SETITEMATTR,
		   MPFROM2SHORT(wItem1, TRUE),
		   MPFROM2SHORT(MIA_CHECKED, 0));
    if (wItem2 != 0)
        WinSendMsg(hMenu,
		   MM_SETITEMATTR,
		   MPFROM2SHORT(wItem2, TRUE),
		   MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
}


/*
**  Copy window to clipboard as bitmap.
*/
static void
CopyToClipBrd(HWND hWnd)
{
    HAB hab;
    HBITMAP hbm;
    HMF     hmf;

    hab = WinQueryAnchorBlock(hWnd);
    WinEmptyClipbrd(hab);
    hbm = CopyToBitmap(hpsScreen);
    WinSetClipbrdData(hab,(ULONG) hbm, CF_BITMAP, CFI_HANDLE);
    hmf = CopyToMetaFile(hpsScreen);
    WinSetClipbrdData(hab,(ULONG) hmf, CF_METAFILE, CFI_HANDLE);
    WinCloseClipbrd(hab);
}


/*
**  Copy ps to a bitmap.
*/
HBITMAP CopyToBitmap(HPS hps)
{
    HPS     hpsMem;
    HWND    hwnd;
    HAB     hab;
    PSZ     psz[4] = {NULL, "Display", NULL, NULL};
    HDC     hdcMem, hdcScr;
    SIZEL   sizel;
    BITMAPINFOHEADER2 bmp;
    HBITMAP hbm;
    LONG    alData[2];
    RECTL   rectl;
    POINTL  aptl[6];

    hdcScr = GpiQueryDevice(hps);
    hwnd = WinWindowFromDC(hdcScr);
    hab = WinQueryAnchorBlock(hwnd);
    hdcMem = DevOpenDC(hab,
		       OD_MEMORY,
		       "*",
		       4L,
                       (PDEVOPENDATA) psz,
		       hdcScr);
    sizel.cx = 0/*GNUPAGE*/;
    sizel.cy = 0/*GNUPAGE*/;
    hpsMem = GpiCreatePS(hab, hdcMem, &sizel,
			 PU_PELS | GPIA_ASSOC | GPIT_MICRO);
    GpiQueryDeviceBitmapFormats(hpsMem, 2L, alData);
    WinQueryWindowRect(hwnd, &rectl);
    memset(&bmp, 0, sizeof(bmp));
    bmp.cbFix =(ULONG) sizeof(bmp);
    bmp.cx =(SHORT)(rectl.xRight - rectl.xLeft);
    bmp.cy =(SHORT)(rectl.yTop - rectl.yBottom);
    bmp.cPlanes = alData[0];
    bmp.cBitCount = alData[1];
    hbm = GpiCreateBitmap(hpsMem, &bmp, 0, NULL, NULL);
    GpiSetBitmap(hpsMem, hbm);
    aptl[0].x = 0;
    aptl[0].y = 0;
    aptl[1].x =(LONG) bmp.cx;
    aptl[1].y =(LONG) bmp.cy;
    aptl[2].x = 0;
    aptl[2].y = 0;
    GpiBitBlt(hpsMem, hps, 3L, aptl, ROP_SRCCOPY, BBO_IGNORE);
    GpiDestroyPS(hpsMem);
    DevCloseDC(hdcMem);
    return hbm;
}


/*
**  Copy ps to a metafile.
*/
HMF
CopyToMetaFile(HPS hps)
{
    HDC hdcMF, hdcOld;
    HAB hab;
    HWND hwnd;
    PSZ psz[4] = {NULL,"Display",NULL,NULL};
    HMF hmf;
    hdcOld = GpiQueryDevice(hps);
    hwnd = WinWindowFromDC(hdcOld);
    hab = WinQueryAnchorBlock(hwnd);
    hdcMF = DevOpenDC(hab, OD_METAFILE, "*", 4L, psz, hdcOld);

    DosRequestMutexSem(semHpsAccess,(ULONG) SEM_INDEFINITE_WAIT);
    GpiSetDrawingMode(hps, DM_DRAW);
    GpiAssociate(hps, 0);
    GpiAssociate(hps, hdcMF);
    ScalePS(hps);
    GpiDrawChain(hps);
    GpiAssociate(hps, 0);
    GpiAssociate(hps, hdcOld);
    DosReleaseMutexSem(semHpsAccess);
    hmf = DevCloseDC(hdcMF);
    return hmf;
}


/*
** Query INI file
*/
BOOL
QueryIni(HAB hab)
{
    BOOL         bPos, bData, bSwp ;
    ULONG        ulOpts[5];
    HINI         hini;
    ULONG        ulCB;
    char         *p;
    static SWP   pauseswp;

    /* read gnuplot ini file */

    hini = PrfOpenProfile(hab, szIniFile);
    ulCB = sizeof(ulShellPos);
    bPos = PrfQueryProfileData(hini, APP_NAME, INISHELLPOS, &ulShellPos, &ulCB);
    ulCB = sizeof(SWP);
    bSwp = PrfQueryProfileData(hini, APP_NAME, INIPAUSEPOS, &pauseswp, &ulCB);
    if (bSwp)
	pausedata.pswp = &pauseswp;
    ulCB = sizeof(ulOpts);
    bData = PrfQueryProfileData(hini, APP_NAME, INIOPTS, &ulOpts, &ulCB);
    if (bData) {
        bWideLines =(BOOL)ulOpts[1];
        bColours =(BOOL)ulOpts[2];
        ulPauseMode = ulOpts[3];
        bPopFront =(BOOL)ulOpts[4];
    } else {
        /*   bWideLines = FALSE; */
        bColours = TRUE;
        bPopFront = TRUE;
        ulPauseMode = 1;
    }
    ulCB = 4 * sizeof(float);
    PrfQueryProfileData(hini, APP_NAME, INIFRAC, &qPrintData.xsize, &ulCB);
    if (PrfQueryProfileSize(hini, APP_NAME, INIPRDRIV, &ulCB)) {
        PDRIVDATA pdriv =(PDRIVDATA) malloc(ulCB);
        if (pdriv != NULL) {
            PrfQueryProfileData(hini, APP_NAME, INIPRDRIV, pdriv, &ulCB);
            qPrintData.pdriv = pdriv;
            qPrintData.cbpdriv = ulCB;
	}
    }
    PrfQueryProfileString(hini, APP_NAME, INIPRPR, "",
			  qPrintData.szPrinterName,
                          (long) sizeof(qPrintData.szPrinterName));
    PrfQueryProfileString(hini, APP_NAME, INIFONT, INITIAL_FONT,
			  szFontNameSize, FONTBUF);
    // save for later use
    strcpy(szInitialFontNameSize, szFontNameSize);
    szInitialFontName = strchr(szInitialFontNameSize, '.') + 1;
    if (*szInitialFontName == NUL) {
	strlcat(szInitialFontNameSize,
	        strchr(INITIAL_FONT, '.') + 1, FONTBUF);
    }
    iInitialFontSize = atoi(szInitialFontNameSize);
    ulCB = sizeof(ulOpts);
    bData = PrfQueryProfileData(hini, APP_NAME, INICHAR, &ulOpts, &ulCB);
    if (bData) {
        lCharWidth = ulOpts[0];
        lCharHeight = ulOpts[1];
    } else {
        lCharWidth = 217;
        lCharHeight = 465;
    }
    ulCB = sizeof(bKeepRatio);
    bData = PrfQueryProfileData(hini, APP_NAME, INIKEEPRATIO, &ulOpts, &ulCB);
    if (bData) bKeepRatio =(BOOL)ulOpts[0];

    PrfCloseProfile(hini);

    if (qPrintData.szPrinterName[0] == '\0') {
	/* get default printer name     */
        PrfQueryProfileString(HINI_PROFILE,
			      "PM_SPOOLER",
			      "PRINTER",
			      ";",
			      qPrintData.szPrinterName,
                              (long) sizeof(qPrintData.szPrinterName));
        if ((p=strchr(qPrintData.szPrinterName, ';')) != NULL)
	    *p = '\0';
    }
    bShellPos = bPos;
    return bPos;
}


/*
** save data in ini file
*/
static void
SaveIni(HWND hWnd)
{
    SWP     swp;
    HINI    hini;
    ULONG   ulOpts[5];
    HAB     hab;
    ULONG   ulPlotPos[4];

    hab = WinQueryAnchorBlock(hWnd);
    hini = PrfOpenProfile(hab, szIniFile);
    if (hini != NULLHANDLE) {
	/* windows position */
        WinQueryWindowPos(hwndFrame, &swp);
        ulPlotPos[0] = swp.x;
        ulPlotPos[1] = swp.y;
        ulPlotPos[2] = swp.cx;
        ulPlotPos[3] = swp.cy;
        PrfWriteProfileData(hini, APP_NAME, INISHELLPOS, &ulPlotPos,
			    sizeof(ulPlotPos));

	/* position of pause dialog */
        if (pausedata.pswp != NULL)
            PrfWriteProfileData(hini, APP_NAME, INIPAUSEPOS,
				pausedata.pswp, sizeof(SWP));

	/* options */
        ulOpts[0] = (ULONG)TRUE;
        ulOpts[1] = (ULONG)bWideLines;
        ulOpts[2] = (ULONG)bColours;
        ulOpts[3] = ulPauseMode;
        ulOpts[4] = (ULONG)bPopFront;
        PrfWriteProfileData(hini, APP_NAME, INIOPTS, &ulOpts, sizeof(ulOpts));

	/* printer data */
        PrfWriteProfileData(hini, APP_NAME, INIFRAC, &qPrintData.xsize,
			    4 * sizeof(float));
        if (qPrintData.pdriv != NULL)
            PrfWriteProfileData(hini, APP_NAME, INIPRDRIV, qPrintData.pdriv,
				qPrintData.cbpdriv);
        PrfWriteProfileString(hini, APP_NAME, INIPRPR,
			      qPrintData.szPrinterName[0] == '\0'? NULL:
			      qPrintData.szPrinterName);

	/* default font name */
	{
	    char szFontName[FONTBUF];

	    /* make sure the default font includes a font name */
	    strlcpy(szFontName, szFontNameSize, FONTBUF);
	    if (szFontName[strlen(szFontName) - 1] == '.')
		strlcat(szFontName, szInitialFontName, FONTBUF);
	    PrfWriteProfileString(hini, APP_NAME, INIFONT, szFontName);
	}
        ulOpts[0] =(ULONG)lCharWidth;
        ulOpts[1] =(ULONG)lCharHeight;
        PrfWriteProfileData(hini, APP_NAME, INICHAR, &ulOpts, sizeof(ulOpts));

	PrfWriteProfileData(hini, APP_NAME, INIKEEPRATIO, &bKeepRatio,
			    sizeof(bKeepRatio));

	PrfCloseProfile(hini);
    } else {
	WinMessageBox(HWND_DESKTOP, HWND_DESKTOP,
		      "Can't write ini file",
		      APP_NAME,
		      0,
		      MB_OK | MB_ICONEXCLAMATION);
    }
}


/*
**  Paint the screen with current data
*/
static void
DoPaint(HWND hWnd, HPS hps)
{
    static RECTL rectl;

    if (tidDraw != 0) {
	/* already drawing - stop it; include the rectl now
	   being drawn in, in the update region; and return
	   without calling beginpaint so that the paint
	   message is resent */
	GpiSetStopDraw(hpsScreen, SDW_ON);
	DosSleep(1);
	WinInvalidateRect(hWnd, &rectl, TRUE);
	return;
    }
    /* winbeginpaint here, so paint message is
       not resent when we return, then spawn a
       thread to do the drawing */
    WinBeginPaint(hWnd, hps, &rectl);                 /*rl */
    tidDraw = _beginthread(ThreadDraw, NULL, 32768, NULL);
}


/*
**  Thread to draw plot on screen
*/
static void
ThreadDraw(void* arg)
{
    HAB hab;
#ifdef HAVE_EXCEPTQ
    EXCEPTIONREGISTRATIONRECORD exRegRec;

    LoadExceptq(&exRegRec, NULL, NULL);
#endif

    hab = WinInitialize(0);
    DosRequestMutexSem(semHpsAccess,(ULONG) SEM_INDEFINITE_WAIT);
    InitScreenPS();
    ScalePS(hpsScreen);
    GpiSetStopDraw(hpsScreen, SDW_OFF);
    GpiSetDrawingMode(hpsScreen, DM_DRAW);
    GpiDrawChain(hpsScreen);
    DrawRuler();
    DrawRulerLineTo();
    DisplayStatusLine(hpsScreen);
    WinEndPaint(hpsScreen);
    DosReleaseMutexSem(semHpsAccess);
    WinTerminate(hab);
    tidDraw = 0;

#ifdef HAVE_EXCEPTQ
    UninstallExceptq(&exRegRec);
#endif
#if 0
    /* This does not work here(why?!), thus moved to pm.trm: PM_text(); */
    gp_exec_event(GE_plotdone, mx, my, 0, 0, 0); /* enable again zoom and scale by mouse motions */
#endif
}


/*
** Initialise the screen ps for drawing
*/
HPS
InitScreenPS()
{
    RECTL   rectClient;
    int     i;

    GpiResetPS(hpsScreen, GRES_ATTRS);
    /* Always use white background */
    WinQueryWindowRect(hApp, (PRECTL) &rectClient);
    WinFillRect(hpsScreen, &rectClient, CLR_WHITE);
    if (bKeepRatio) {
	double ratio = 1.560;
	double xs = rectClient.xRight - rectClient.xLeft;
	double ys = rectClient.yTop - rectClient.yBottom;

	if (ys > xs/ratio) { /* reduce ys to fit */
	    rectClient.yTop = rectClient.yBottom +(int)(xs/ratio);
	} else if (ys < xs/ratio) { /* reduce xs to fit */
	    rectClient.xRight = rectClient.xLeft +(int)(ys*ratio);
	}
    }

    GpiSetPageViewport(hpsScreen, &rectClient);
    if (!bColours) {
	alColourTable[2 + CLR_WHITE] = 0xffffff;		/* -2 */
	alColourTable[2 + CLR_BLACK] = 0;			/* -1 */
	for (i = 0; i < nColors; i++)
	    alColourTable[2 + i] = 0;
    }

    if (bPMPaletteMode) {
	// FIXME:  shouldn't this be alColourTable+2??
	GpiCreateLogColorTable(hpsScreen, LCOL_RESET, LCOLF_CONSECRGB,
			       0, nColors, alColourTable);
	if (!bCols_init) { /* Ilya: avoid white line on white background */
	    LONG bkColor;

	    bCols_init = TRUE;
	    GpiQueryLogColorTable(hpsScreen, 0, 0, 16, alColourTable + 2);
	    alColourTable[2 + CLR_WHITE] = 0xffffff;		/* -2 */
	    alColourTable[2 + CLR_BLACK] = 0;			/* -1 */
	    bkColor = alColourTable[2 + CLR_BACKGROUND];
	    i = -1;
	    while (i++ < 16) {
		if (alColourTable[2 + lCols[i]] == bkColor) {
		    while (i++ < 16)
			lCols[i - 1] = lCols[i];
		    break;
		}
	    }
	}

	/* init rgb_colors: simple index translation only */
	for (i = 0; i < 18; i++)
	    rgb_colors[i] = i + CLR_WHITE;
    } else {
	if (!bCols_init) {
	    bCols_init = TRUE;
	    /* get RGB values of all CLR_xxx constants */
	    for (i = 0; i < 18; i++)
		rgb_colors[i] = GpiQueryRGBColor(hpsScreen, LCOLOPT_REALIZED, i + CLR_WHITE );
	}

	GpiCreateLogColorTable(hpsScreen, LCOL_RESET, LCOLF_RGB, 0, 0, 0);
    }
    return hpsScreen;
}


/*
**  Get a font to use
**  Scale the plot area to world coords for subsequent plotting
*/
short
ScalePS(HPS hps)
{
    SelectFont(hps, szFontNameSize);
    return 0;
}


/*
**  Select the default outline font.
**  The format of szFontNameSize is "<size>.<face name>{:Bold}{:Italic}".
**  <face name> may be empty.
*/
void
SelectFont(HPS hps, char *szFontNameSize)
{
    HDC    hdc;
    static FATTRS  fat;  // docs say that it may not be on the stack...
    FONTMETRICS fm;
    LONG   xDeviceRes, yDeviceRes;
    POINTL ptlFont;
    SIZEF  sizfx;
    LONG   lcid = 0L;
    char   szFontName[FONTBUF];
    short  shPointSize;
    char  *p, *q;
    BOOL   bBold, bItalic;
    POINTL aptl[TXTBOX_COUNT];
    GRADIENTL grdlold, grdl = {1, 0};

    DEBUG_FONT(("SelectFont: %s", szFontNameSize));

    shPointSize = (atoi(szFontNameSize) * fontscale) / 100;
    p = strchr(szFontNameSize, '.');
    szFontName[0] = NUL;
    if (p != NULL) {
	p++;
	if (*p == NUL) {
	    // no font name, no attributes
	    strlcpy(szFontName, szInitialFontName, FONTBUF);
	} else if (*p == ':') {
	    // just attributes, but no font name
	    strlcpy(szFontName, szInitialFontName, FONTBUF);
	    // append attributes
	    strlcat(szFontName, p, FONTBUF);
	} else {
	    // font name provided
	    strlcpy(szFontName, p, FONTBUF);
	}
    }
    DEBUG_FONT(("SelectFont: fontname \"%s\"", szFontName));

    // strip off attributes to obtain font family name
    p = strstr(szFontName, " Bold");
    if (p == NULL)
	p = strstr(szFontName, ":Bold");
    bBold = (p != NULL);
    q = strstr(szFontName, " Italic");
    if (q == NULL)
	q = strstr(szFontName, ":Italic");
    bItalic = (q != NULL);
    if (bBold) *p = NUL;
    if (bItalic) *q = NUL;

    fat.usRecordLength  = sizeof(fat);
    fat.fsSelection     = 0;
    fat.lMatch          = 0;
    fat.idRegistry      = 0;
    fat.usCodePage      = codepage; /*GpiQueryCp(hps); */
    fat.lMaxBaselineExt = 0;
    fat.lAveCharWidth   = 0;
    fat.fsType          = 0;
    fat.fsFontUse       = FATTR_FONTUSE_OUTLINE |
	FATTR_FONTUSE_TRANSFORMABLE;

    if (bBold)
	fat.fsSelection |= FATTR_SEL_BOLD;
    if (bItalic)
	fat.fsSelection |= FATTR_SEL_ITALIC;

    strlcpy(fat.szFacename, szFontName, FACESIZE);

    if (tabFont[0].name != NULL)
	free(tabFont[0].name);
    tabFont[0].name = strdup(szFontName);
    tabFont[0].codepage = codepage;
    tabFont[0].bold = bBold;
    tabFont[0].italic = bItalic;
    tabFont[0].lcid = 10L;

    lcid = GpiQueryCharSet(hps);
    if (lcid != 10L)
	lcid = 10L;
    else {
	GpiSetCharSet(hps, 0L);
	GpiDeleteSetId(hps, lcid);
    }
    GpiCreateLogFont(hps, NULL, lcid, &fat);
    GpiSetCharSet(hps, lcid);

    hdc = GpiQueryDevice(hps);

    DevQueryCaps(hdc, CAPS_HORIZONTAL_RESOLUTION, 1L, &xDeviceRes);
    DevQueryCaps(hdc, CAPS_VERTICAL_RESOLUTION,   1L, &yDeviceRes);

    /* Find desired font size in pixels */
    ptlFont.x = 2540L * (long)shPointSize / 72L;
    ptlFont.y = 2540L * (long)shPointSize / 72L;

    /* Set the character box */
    sizfx.cx = MAKEFIXED(ptlFont.x, 0);
    sizfx.cy = MAKEFIXED(ptlFont.y, 0);
    lVOffset = ptlFont.y;
    sizBaseFont = sizfx;
    GpiSetCharBox(hps, &sizfx);

    /* set up some useful globals */
    GpiQueryFontMetrics(hps, sizeof(FONTMETRICS), &fm);
    lBaseSubOffset = -fm.lSubscriptYOffset;
    lBaseSupOffset = fm.lSuperscriptYOffset;
    lSubOffset = lBaseSubOffset;
    lSupOffset = lBaseSupOffset;
    lCharHeight = fm.lMaxAscender * 1.2;
    lCharWidth  = fm.lAveCharWidth;
    sizBaseSubSup.cx = MAKEFIXED(ptlFont.x * 0.7, 0);
    sizBaseSubSup.cy = MAKEFIXED(ptlFont.y * 0.7, 0);

    /* Use average width of digits instead of info from font metrics */
    GpiQueryCharAngle(hps, &grdlold);
    GpiSetCharAngle(hps, &grdl);
    GpiQueryTextBox(hps, 10, "0123456789", TXTBOX_COUNT, aptl);
    GpiSetCharAngle(hps, &grdlold);
    lCharWidth = aptl[TXTBOX_CONCAT].x / 10;

    sizCurFont = sizBaseFont;
    sizCurSubSup = sizBaseSubSup;

    strlcpy(szCurrentFontNameSize, szFontNameSize, FONTBUF);
}


/*
**  Select a named and sized outline font.
**  The format of szFNS is "<point size>.<face name>{:Bold}{:Italic}"
**  or NULL for the default font.  <face name> may be empty.
*/
static void
SwapFont(HPS hps, char *szFNS)
{
    HDC    hdc;
    static FATTRS  fat;  // docs say that it may not be on the stack...
    LONG   xDeviceRes, yDeviceRes;
    POINTL ptlFont;
    LONG   lcid = 0L;
    static int itab = 1;

    if (szFNS == NULL || szFNS[0] == NUL) {    /* restore base font */
	sizCurFont = sizBaseFont;
	sizCurSubSup = sizBaseSubSup;
	lSubOffset = lBaseSubOffset;
	lSupOffset = lBaseSupOffset;
	GpiSetCharSet(hps, 10);
	GpiSetCharBox(hps, &sizBaseFont);
    } else {
	char   szFontName[FONTBUF];
	short  shPointSize;
	FONTMETRICS fm;
	int    i;
	char  *p, *q;
	BOOL   bBold, bItalic;
	POINTL aptl[TXTBOX_COUNT];
	GRADIENTL grdlold, grdl = {1, 0};

	// we always expect a font size
	shPointSize = (atoi(szFNS) * fontscale) / 100;
	p = strchr(szFNS, '.');
	szFontName[0] = NUL;
	if (p != NULL) {
	    p++;
	    if (*p == NUL || *p == ':') {
		// no font name, but maybe attributes
		char * q = strchr(szFontNameSize, '.');
		if (q != NULL) {
		    q++;
		    if (*q == ':') {
			// default font does not include font name
			strlcpy(szFontName, szInitialFontName, FONTBUF);
			// append default attributes
			strlcat(szFontName, q, FONTBUF);
		    } else if (*q == NUL) {
			// no (proper) default font name
			strlcpy(szFontName, szInitialFontName, FONTBUF);
		    } else {
			// copy default font name
			strlcpy(szFontName, q, FONTBUF);
		    }
		    if (*p != NUL) {
			// append attributes
			strlcat(szFontName, p, FONTBUF);
		    }
		}
	    } else {
		// font name provided
		strlcpy(szFontName, p, FONTBUF);
	    }
	}
	DEBUG_FONT(("SwapFont: fontname \"%s\"", szFontName));

	// strip off attributes to obtain font family name
	p = strstr(szFontName, " Bold");
	if (p == NULL)
	    p = strstr(szFontName, ":Bold");
	bBold = (p != NULL);
	q = strstr(szFontName, " Italic");
	if (q == NULL)
	    q = strstr(szFontName, ":Italic");
	bItalic = (q != NULL);
	if (bBold) *p = NUL;
	if (bItalic) *q = NUL;

	/* search for a previous font with correct encoding and attributes */
	lcid = 0;
	for (i = 0; i < itab; i++) {
	    if ((strcmp(szFontName, tabFont[i].name) == 0) &&
	        (bBold == tabFont[i].bold) && (bItalic == tabFont[i].italic) &&
	        (codepage == tabFont[i].codepage)) {
		lcid = tabFont[i].lcid;
		break;
	    }
	}

	// no match found: create new logical font
	if (lcid == 0) {
	    fat.usRecordLength  = sizeof(fat);
	    fat.fsSelection     = 0;
	    fat.lMatch          = 0;
	    fat.idRegistry      = 0;
	    fat.usCodePage      = codepage;
	    fat.lMaxBaselineExt = 0;
	    fat.lAveCharWidth   = 0;
	    fat.fsType          = 0;
	    fat.fsFontUse       = FATTR_FONTUSE_OUTLINE |
		FATTR_FONTUSE_TRANSFORMABLE;

	    if (bBold)
		fat.fsSelection |= FATTR_SEL_BOLD;
	    if (bItalic)
		fat.fsSelection |= FATTR_SEL_ITALIC;

	    strlcpy(fat.szFacename, szFontName, FACESIZE);

	    // Use the default encoding for symbol fonts for all encodings but UTF-8
	    if (codepage != 1208) { // not UTF-8
		LONG lCount = 1;
		FONTMETRICS fm;

		// Query font metrics
		GpiQueryFonts(hps, QF_PUBLIC, fat.szFacename,
			      &lCount, (LONG) sizeof(FONTMETRICS), &fm);
		// Test for magic number of a symbol font with special encoding
		if (lCount > 0 && fm.usCodePage == 65400)
		    fat.usCodePage = 0;
	    }

	    DEBUG_FONT(("SwapFont: '%s' - '%s' - '%s', %x", szFNS, szFontName, fat.szFacename, fat.fsSelection));

	    tabFont[itab].name = strdup(szFontName);
	    tabFont[itab].codepage = codepage;
	    tabFont[itab].bold = bBold;
	    tabFont[itab].italic = bItalic;
	    lcid = itab + 10;
	    tabFont[itab].lcid = lcid;
	    ++itab;

	    GpiSetCharSet(hps, 0L);
	    GpiDeleteSetId(hps, lcid);
	    GpiCreateLogFont(hps, NULL, lcid, &fat);
	} /* if (lcid) */

	GpiSetCharSet(hps, lcid);
	hdc = GpiQueryDevice(hps);

	DevQueryCaps(hdc, CAPS_HORIZONTAL_RESOLUTION, 1L, &xDeviceRes);
	DevQueryCaps(hdc, CAPS_VERTICAL_RESOLUTION,   1L, &yDeviceRes);

	/* Find desired font size in pixels */
	ptlFont.x = 2540L * (long) shPointSize / 72L;
	ptlFont.y = 2540L * (long) shPointSize / 72L;

	/* Set the character box */
	sizCurFont.cx = MAKEFIXED(ptlFont.x, 0);
	sizCurFont.cy = MAKEFIXED(ptlFont.y, 0);
	/* lVOffset = ptlFont.y; */

	GpiSetCharBox(hps, &sizCurFont);
	sizCurSubSup.cx = MAKEFIXED(ptlFont.x*0.7, 0);
	sizCurSubSup.cy = MAKEFIXED(ptlFont.y*0.7, 0);

	/* set up some useful globals */
	GpiQueryFontMetrics(hps, sizeof(FONTMETRICS), &fm);
	lSubOffset = -fm.lSubscriptYOffset;
	lSupOffset = fm.lSuperscriptYOffset;
	lCharHeight = fm.lMaxAscender * 1.2;
	lCharWidth  = fm.lAveCharWidth;

	/* Use average width of digits instead of info from font metrics */
	GpiQueryCharAngle(hps, &grdlold);
	GpiSetCharAngle(hps, &grdl);
	GpiQueryTextBox(hps, 10, "0123456789", TXTBOX_COUNT, aptl);
	GpiSetCharAngle(hps, &grdlold);
	lCharWidth = aptl[TXTBOX_CONCAT].x / 10;
    }
}


#define FLUSHPATH(hps, where) FlushPath(hps, &bPath, linewidth, color, where)
static inline void
FlushPath(HPS hps, BOOL * bPath, int linewidth, LONG color, char * where)
{
    if (*bPath) {
	GpiEndPath(hps);
	GpiSetLineWidthGeom(hps, linewidth);
	GpiSetColor(hps, color);
	GpiStrokePath(hps, 1, 0);
	*bPath = FALSE;
	// PmPrintf("FlushPath() in %s", where);
	(void) where;
    }
}


/*
** Thread to read plot commands from GNUPLOT pm driver.
** Opens named pipe, then clears semaphore to allow GNUPLOT driver to proceed.
** Reads commands and builds a command list.
*/
static void
ReadGnu(void* arg)
{
    HPIPE hRead = 0L;
    POINTL ptl;
    long lOldLine = 0;
    BOOL bPath = FALSE;
    BOOL bDots = FALSE;
    ULONG rc;
    USHORT usErr;
    ULONG cbR;
    USHORT i;
    unsigned char buff[2];
    HEV hev;
    static char *szPauseText = NULL;
    ULONG ulPause;
    char *pszPipeName, *pszSemName;
    HPS hps;
    HAB hab;
    int linewidth = DEFLW;
    HPAL pm3d_hpal = 0;     /* palette used for make_palette() */
    HPAL pm3d_hpal_old = 0; /* default palette used before make_palette() */
    LONG pm3d_color = 0;    /* current colour (used if it is >0) */
    ULONG *rgbTable = NULL; /* current colour table (this is a 'virtual' palette) */
    LONG color = 0;
    int fillstyle = FS_SOLID | (100 << 4);
#ifdef HAVE_EXCEPTQ
    EXCEPTIONREGISTRATIONRECORD exRegRec;

    LoadExceptq(&exRegRec, NULL, NULL);
#endif

    hab = WinInitialize(0);
    DosEnterCritSec();
    pszPipeName = malloc(256);
    pszSemName  = malloc(256);
    DosExitCritSec();
    strcpy(pszPipeName, "\\pipe\\");
    strcpy(pszSemName, "\\sem32\\");
    strcat(pszPipeName, szIPCName);
    strcat(pszSemName, szIPCName);

    /* open a named pipe for communication with gnuplot */
    rc = DosCreateNPipe(pszPipeName,
			&hRead,
			NP_ACCESS_DUPLEX|NP_NOINHERIT|NP_NOWRITEBEHIND ,
			1|NP_WAIT|NP_READMODE_MESSAGE|NP_TYPE_MESSAGE,
			PIPEBUF,
			PIPEBUF,
			0xFFFFFFFF);
    hev = 0;       /* OK, gnuplot can try to open npipe ... */
    DosOpenEventSem(pszSemName, &hev);
    DosPostEventSem(hev);

    /* attach to gnuplot */
  server:

    if (DosConnectNPipe(hRead) == 0L) {
	WinPostMsg(hSysMenu,
		   MM_SETITEMATTR,
		   MPFROM2SHORT(SC_CLOSE, TRUE),
		   MPFROM2SHORT(MIA_DISABLED, MIA_DISABLED));

	/* store graphics commands */
	/* use semaphore to prevent problems with drawing while reallocating
	   the command buffers */

        DosRead(hRead, &ppidGnu, 4, &cbR);

	sprintf(mouseShareMemName, "\\SHAREMEM\\GP%i_Mouse_Input",(int)ppidGnu);
	if (DosGetNamedSharedMem(&input_from_PM_Terminal,
				 mouseShareMemName,
				 PAG_WRITE)) {
	    /*now: gray menu items; old code: DosBeep(1440L,1000L); // indicates error */
	    input_from_PM_Terminal = 0;
	}

	semInputReady = 0;
	/* semaphore 'semInputReady' must be open later in order to avoid problems */
	/* with the server mode; also 'bhave_*' init here because of server */

	/* DosPostEventSem(semStartSeq);         // once we've got pidGnu */
	WinPostMsg(hApp, WM_GPSTART, 0, 0);

	hps = hpsScreen;
	InitScreenPS();
	while (1) {
	    buff[0] = 0;
	    usErr = BufRead(hRead, buff, 1, &cbR);
	    if (usErr != 0)
		break;

	    if (breakDrawing) {
		/* PM: drawing has been stopped (by Ctrl-C) ... */
		hps = 0;	/*     ...thus drawings go to nowhere... */
		if (*buff == SET_TEXT) { /* ...unless 'plot finished' command */
		    POINTL p;
		    hps = hpsScreen;  /* drawings back to screen */
		    breakDrawing = FALSE;
		    GpiSetColor(hps, color = RGB_TRANS(CLR_RED)); /* cross the unfinished plot */
		    GpiBeginPath(hps, 1);
		    p.x = p.y = 0; GpiMove(hps, &p);
		    p.x = GNUXPAGE; p.y = GNUYPAGE; GpiLine(hps, &p);
		    p.x = 0; p.y = GNUYPAGE; GpiMove(hps, &p);
		    p.x = GNUXPAGE; p.y = 0; GpiLine(hps, &p);
		    GpiEndPath(hps);
		    GpiStrokePath(hps, 1, 0);
		}
	    }

            switch (*buff) {
	    case SET_GRAPHICS :    /* enter graphics mode */
	    {
		image_list_entry *ile;

		if (tidDraw != 0) {
		    /* already drawing - stop it */
		    GpiSetStopDraw(hpsScreen, SDW_ON);
		    while (tidDraw != 0) DosSleep(1);
		}
		/* wait for access to command list and lock it */
		/* DosEnterCritSec(); */
		DosRequestMutexSem(semHpsAccess,(ULONG) SEM_INDEFINITE_WAIT);
		InitScreenPS();
		ScalePS(hps);
		GpiSetDrawingMode(hps, DM_DRAWANDRETAIN);
		for (i = 1; i <= iSeg; i++)
		    GpiDeleteSegment(hps, i);
		iSeg = 1;
		GpiOpenSegment(hps, iSeg);
		/* DosExitCritSec(); */
		GpiSetLineEnd(hps, LINEEND_ROUND);
		GpiSetLineWidthGeom(hps, linewidth);
		GpiSetCharBox(hps, &sizBaseFont);

		/* free image buffers from previous plot, if any */
		while (image_list) {
		    DEBUG_IMAGE(("freeing image from last plot"));
		    ile = image_list;
		    image_list = ile->next;
		    free(ile->image);
		    free(ile->pbmi);
		    free(ile);
		}

		/* discard the old status line text */
		ResetStatusLineText();

		break;
	    }

	    case GR_QUERY :     /* query terminal info */
		/* mouseable gnupmdrv sends greetings to mouseable PM terminal */
		if (mouseTerminal) {
		    int i = 0xABCD;

		    DosWrite(hRead, &i, sizeof(int), &cbR);
		}
		DosWrite(hRead, &lCharWidth, sizeof(int), &cbR);
		DosWrite(hRead, &lCharHeight, sizeof(int), &cbR);
		break;

	    case SET_TEXT :     /* leave graphics mode(graph completed) */
		FLUSHPATH(hps, "SET_TEXT");
		GpiCloseSegment(hps);
		DrawRuler();
		DrawRulerLineTo();
		DisplayStatusLine(hps);
		DosReleaseMutexSem(semHpsAccess);
		WinPostMsg(hApp, WM_GNUPLOT, 0L, 0L);
		break;

	    case GR_RESET :
		/* gnuplot has reset drivers, allow user to kill this */
		WinPostMsg(hSysMenu,
			   MM_SETITEMATTR,
			   MPFROM2SHORT(SC_CLOSE, TRUE),
			   MPFROM2SHORT(MIA_DISABLED,(USHORT)0));

		/* if we are keeping us on the screen, wait for new connection */
		if (bServer || bPersist) {
		    DosDisConnectNPipe(hRead);
		    goto server;
		}
		break;

	    case GR_RESUME :
		/* resume after multiplot */
		DosRequestMutexSem(semHpsAccess, (ULONG) SEM_INDEFINITE_WAIT);
		iSeg++;
		GpiSetDrawingMode(hps, DM_DRAWANDRETAIN);
		GpiOpenSegment(hps, iSeg);
		break;

	    case 's' :
		/* suspend after multiplot */
		break;

	    case GR_MOVE :   /* move */
	    case GR_DRAW :   /* draw vector */
	    {
		LONG curr_color;

		if (pm3d_color >= 0) {
		    curr_color = GpiQueryColor(hps);
		    GpiSetColor(hps, pm3d_color);
		}
		if (*buff == GR_MOVE) {
		    FLUSHPATH(hps, "GR_MOVE");
		} else {
		    if (bWideLines && !bPath) {
			GpiBeginPath(hps, 1);
			bPath = TRUE;
		    }
		}
		BufRead(hRead, &ptl.x, 2 * sizeof(int), &cbR);
		if ((*buff == GR_DRAW) && bDots)
		    ptl.x += 5;
		else if ((*buff == GR_MOVE) && bDots)
		    ptl.x -= 5;
		if (*buff == GR_MOVE)
		    LMove(hps, &ptl);
		else
		    LLine(hps, &ptl);

		if (pm3d_color >= 0)
		    GpiSetColor(hps, curr_color);
	    }
	    break;

	    case GR_PAUSE  :   /* pause */
	    {
		int len;

		pausing = 1;
		BufRead(hRead, &len, sizeof(int), &cbR);
		len = (len + sizeof(int) - 1) / sizeof(int);
		if (len > 0){  /* get pause text */
		    DosEnterCritSec();
		    szPauseText = malloc(len*sizeof(int));
		    DosExitCritSec();
		    BufRead(hRead, szPauseText, len * sizeof(int), &cbR);
		}
		if (ulPauseMode != PAUSE_GNU) {
		    /* pause and wait for semaphore to be cleared */
		    DosResetEventSem(semPause, &ulPause);
		    WinPostMsg(hApp, WM_PAUSEPLOT, (MPARAM) szPauseText, 0L);
		    DosWaitEventSem(semPause, SEM_INDEFINITE_WAIT);
		} else {
		    /* gnuplot handles pause */
		    ulPauseReply = 2;
		}
		DosEnterCritSec();
		if (szPauseText != NULL)
		    free(szPauseText);
		szPauseText = NULL;
		DosExitCritSec();
		/* reply to gnuplot so it can continue */
		DosWrite(hRead, &ulPauseReply, sizeof(int), &cbR);
		pausing = 0;
		break;
	    }

	    case GR_ENH_TEXT :  /* write enhanced text */
		{
                    unsigned int x, y, len;
		    unsigned int mode;
		    int textwidth, textheight;
                    char *str;
                    POINTL aptl[TXTBOX_COUNT];

		    FLUSHPATH(hps, "GR_ENH_TEXT");

		    /* read x, y, mode, len */
		    BufRead(hRead, &x, sizeof(int), &cbR);
                    BufRead(hRead, &y, sizeof(int), &cbR);
                    BufRead(hRead, &mode, sizeof(int), &cbR);
                    BufRead(hRead, &len, sizeof(int), &cbR);

                    DosEnterCritSec();
		    len = (len + sizeof(int) - 1) / sizeof(int);
                    if (len == 0) len = 1; /*?? how about read */
                    str = malloc(len * sizeof(int));
		    *str = '\0';
                    DosExitCritSec();
                    BufRead(hRead, str, len * sizeof(int), &cbR);

                    GpiQueryTextBox(hps, strlen(str), str, TXTBOX_COUNT, aptl);
		    textwidth = aptl[TXTBOX_CONCAT].x;
		    textheight = aptl[TXTBOX_CONCAT].y;

		    /* only display text if requested */
		    if (mode & 0x01) {
			LONG curr_color;

			if (pm3d_color >= 0) {
			    curr_color = GpiQueryColor(hps);
			    GpiSetColor(hps, pm3d_color);
			}
			ptl.x = (LONG) (x + multLineVert * (lVOffset / 4));
			ptl.y = (LONG) (y - multLineHor * (lVOffset / 4));

			GpiSetBackMix(hps, BM_LEAVEALONE);
			GpiCharStringAt(hps, &ptl, strlen(str), str);

			if (pm3d_color >= 0)
			    GpiSetColor(hps, curr_color);
		    }

		    /* report back textwidth */
		    DosWrite(hRead, &textwidth, sizeof(textwidth), &cbR);
		    DosWrite(hRead, &textheight, sizeof(textheight), &cbR);

                    DosEnterCritSec();
                    free(str);
		    DosExitCritSec();
		    break;
		}

	    case GR_TEXT :   /* write text */
		/* read x, y, len */
		{
                    unsigned int x, y, len;
		    int sw;
                    char *str;
		    LONG curr_color;
#ifndef PM_KEEP_OLD_ENHANCED_TEXT
                    POINTL aptl[TXTBOX_COUNT];
#endif
		    FLUSHPATH(hps, "GR_TEXT");

		    BufRead(hRead, &x, sizeof(int), &cbR);
		    BufRead(hRead, &y, sizeof(int), &cbR);
		    BufRead(hRead, &len, sizeof(int), &cbR);

                    DosEnterCritSec();
                    len = (len + sizeof(int) - 1) / sizeof(int);
		    /* Data always includes a terminating NUL, hence len>0.
		       So this check is somewhat paranoid: */
                    if (len == 0) len = 1;
                    str = malloc(len * sizeof(int));
		    *str = '\0';
                    DosExitCritSec();
                    BufRead(hRead, str, len * sizeof(int), &cbR);
		    if (pm3d_color >= 0) {
                        curr_color = GpiQueryColor(hps);
			GpiSetColor(hps, pm3d_color);
		    }

#ifdef PM_KEEP_OLD_ENHANCED_TEXT
                    sw = QueryTextBox(hps, strlen(str), str);
#else
                    GpiQueryTextBox(hps, strlen(str), str, TXTBOX_COUNT, aptl);
		    sw = aptl[TXTBOX_BOTTOMRIGHT].x;
#endif

                    switch (jmode) {
		    case LEFT:
			sw = 0;
			break;
		    case CENTRE:
			sw /= -2;
			break;
		    case RIGHT:
			sw *= -1;
			break;
		    }

		    ptl.x = (LONG) (x + multLineHor * sw + multLineVert * (lVOffset / 4));
		    ptl.y = (LONG) (y + multLineVert * sw - multLineHor * (lVOffset / 4));

		    GpiSetBackMix(hps, BM_LEAVEALONE);

#ifdef PM_KEEP_OLD_ENHANCED_TEXT
                    if (bEnhanced)
                        CharStringAt(hps, ptl.x, ptl.y, strlen(str) , str);
                    else
#endif
			GpiCharStringAt(hps, &ptl, strlen(str), str);

		    if (pm3d_color >= 0)
                        GpiSetColor(hps, curr_color);

                    DosEnterCritSec();
                    free(str);
		    DosExitCritSec();

		    break;
		}

	    case SET_JUSTIFY :   /* justify */
		BufRead(hRead, &jmode, sizeof(int), &cbR);
		//printf( "SET_JUSTIFY: %i", jmode );
		break;

	    case SET_ANGLE :   /* text angle */
	    {
		int ta, t1;
		GRADIENTL grdl;

		FLUSHPATH(hps, "SET_ANGLE");

		BufRead(hRead, &ta, sizeof(int), &cbR);
		t1 = ta % 360;
		if (t1 < 0)
		    t1 += 360;
		switch (t1) {
		case   0:
		    grdl.x =  1L;
		    grdl.y =  0L;
		    multLineHor = 1;
		    multLineVert = 0;
		    break;
		case  90:
		    grdl.x =  0L;
		    grdl.y =  1L;
		    multLineHor = 0;
		    multLineVert = 1;
		    break;
		case 180:
		    grdl.x = -1L;
		    grdl.y =  0L;
		    multLineHor = -1;
		    multLineVert = 0;
		    break;
		case 270:
		    grdl.x =  0L;
		    grdl.y = -1L;
		    multLineHor = 0;
		    multLineVert = -1;
		    break;
		default:  {
		    double t = t1 * M_PI /180;

		    grdl.x = (LONG) (100 * cos(t));
		    grdl.y = (LONG) (100 * sin(t));
		    multLineHor = cos(t);
		    multLineVert = sin(t);
		} /* default case */
		} /* switch (t1) */

		GpiSetCharAngle(hps, &grdl);
		break;
	    }

	    case SET_LINE :   /* line type */
	    {
		int lt, col;

		FLUSHPATH(hps, "SET_LINE");

		BufRead(hRead, &lt, sizeof(int), &cbR);
		col = lt;
		GpiSetLineWidthGeom(hps, linewidth);
		if (lt > LT_NODRAW) {
		    col = (col + 2) % 16;
		} else if (lt == LT_NODRAW) {
		    col = 0; // black
		} else if (lt == LT_BACKGROUND) {
		    col = 15;  // white
		}
		LType(0);
		/* GpiSetLineType(hps, lLineTypes[lt]); */
		/* maintain some flexibility here in case we don't want
		 * the model T option */
		GpiSetColor(hps, color = (RGB_TRANS(bColours ? lCols[col] : CLR_NEUTRAL)));
		pm3d_color = -1; /* switch off using pm3d colours */
		break;
	    }

	    case GR_LTCOLOR:
	    {
		int lt, col;

		FLUSHPATH(hps, "SET_LTCOLOR");

		BufRead(hRead, &lt, sizeof(int), &cbR);
		col = lt;
		if (lt > LT_NODRAW) {
		    col = (col + 2) % 16;
		} else if (lt == LT_NODRAW) {
		    col = 0; // black
		} else if (lt == LT_BACKGROUND) {
		    col = 15;  // white
		}
		GpiSetColor(hps, color = (RGB_TRANS(bColours ? lCols[col] : CLR_NEUTRAL)));
		pm3d_color = -1; /* switch off using pm3d colours */
		break;
	    }

	    case SET_DASH :
	    {
		int dt = 0;

		FLUSHPATH(hps, "SET_DASH");

		BufRead(hRead, &dt, sizeof(int), &cbR);
		DEBUG_LINES(("SET_DASH: %d", dt));
		if (dt == DASHTYPE_AXIS) /* map axis dashpattern */
		    dt = 1;
		else if (dt == DASHTYPE_SOLID) /* map solid "pattern" */
		    dt = 0;
		if (dt >= 0)
		    LType(dt);
		break;
	    }

	    case SET_FILLBOX :   /* fill box */
	    {
		unsigned int x, y, w, h;
		POINTL pt;
		LONG curr_color;

		FLUSHPATH(hps, "SET_FILLBOX");

		BufRead(hRead, &fillstyle, sizeof(fillstyle), &cbR);
		BufRead(hRead, &x, sizeof(x), &cbR);
		BufRead(hRead, &y, sizeof(y), &cbR);
		BufRead(hRead, &w, sizeof(w), &cbR);
		BufRead(hRead, &h, sizeof(h), &cbR);
		pt.x = x;
		pt.y = y;
		GpiMove(hpsScreen, &pt);
		pt.x += w;
		pt.y += h;

		SetFillStyle(hps, fillstyle);

		/* apply PM3D color if applicable */
		if (pm3d_color >= 0) {
		    curr_color = GpiQueryColor(hps);
		    GpiSetColor(hps, pm3d_color);
		}
		GpiBox(hps, DRO_FILL, &pt, 0,0);
		/* revert fill style changes */
		fillstyle = FS_SOLID | (100 << 4);
		GpiSetBackMix(hps, BM_OVERPAINT);
		GpiSetPattern(hps, PATSYM_SOLID);
		if (pm3d_color >= 0)
		    GpiSetColor(hps, curr_color);
		break;
	    }

	    case SET_LINEWIDTH :   /* line width */
	    {
		int lw;

		FLUSHPATH(hps, "SET_LINEWIDTH");

		BufRead(hRead, &lw, sizeof(int), &cbR);
		linewidth = DEFLW * lw / 100;
		GpiSetLineWidthGeom(hps, linewidth);
		break;
	    }

	    case SET_POINTMODE :   /* points mode */
	    {
		int lt;

		BufRead(hRead, &lt, sizeof(int), &cbR);
		/* 1: enter point mode, 0: exit */
		if (lt == 1)
		    lOldLine = LType(0);
		else
		    LType(lOldLine);
		bDots = lt;
	    }
	    break;

	    case SET_FONT :   /* set font */
	    {
		int len, size;

		BufRead(hRead, &len, sizeof(int), &cbR);
		size = (len + sizeof(int) - 1) / sizeof(int);

		if (len == 0) {
		    SwapFont(hps, NULL);
		    strlcpy(szCurrentFontNameSize, szFontNameSize, FONTBUF);
		    DEBUG_FONT(("SET_FONT: default font \"%s\"", szFontNameSize));
		} else {
		    char font[FONTBUF];
		    char *p, *tmp, *str;

		    tmp = str = malloc(size * sizeof(int));
		    BufRead(hRead, str, size * sizeof(int), &cbR);
		    if (len == 1) { // empty string (only terminating NUL)
			DEBUG_FONT(("SET_FONT: empty font: \"%s\"", szFontNameSize));
			SwapFont(hps, NULL);
			strlcpy(szCurrentFontNameSize, szFontNameSize, FONTBUF);
		    } else {
			DEBUG_FONT(("SET_FONT: \"%s\"", str));
			p = strchr(str, ',');
			if (p == NULL) {
			    // default font size
			    strlcpy(font, szFontNameSize, FONTBUF);
			    p = strchr(font, '.');
			    *(++p) = NUL;
			} else {
			    *p = NUL;
			    strlcpy(font, p + 1, FONTBUF);
			    strlcat(font, ".", FONTBUF);
			}
			/* allow abbreviation of some well known font names */
			FontExpand(str);
			strlcat(font, str, FONTBUF);
			SwapFont(hps, font);
			strlcpy(szCurrentFontNameSize, font, FONTBUF);
		    }
		    free(tmp);
		}
		break;
	    }

	    case GR_QUERY_FONT : /* query current font */
	    {
		int namelen;

		namelen = strlen(szCurrentFontNameSize);
		DosWrite(hRead, &namelen, sizeof(int), &cbR);
		DosWrite(hRead, szCurrentFontNameSize, namelen, &cbR);
		DEBUG_FONT(("GR_QUERY_FONT \"%s\"", szCurrentFontNameSize));
		break;
	    }

	    case SET_OPTIONS :   /* set options */
	    {
		int len;
		char *str;

		BufRead(hRead,&len, sizeof(int), &cbR);
		len = (len + sizeof(int) - 1) / sizeof(int);
		bWideLines = FALSE; /* reset options */
#ifdef PM_KEEP_OLD_ENHANCED_TEXT
		bEnhanced = FALSE;
#endif
		if (len > 0) {
		    char *p;

		    p = str = malloc(len * sizeof(int));
		    BufRead(hRead, str, len * sizeof(int), &cbR);
		    while ((p=strchr(p,'-')) != NULL) {
			++p;
			if (*p == 'w')
			    bWideLines = TRUE;
#ifdef PM_KEEP_OLD_ENHANCED_TEXT
			if (*p == 'e')
			    bEnhanced = TRUE;
#endif
			++p;
		    }
		    free(str);
		}
		/* TODO: Update menu items */
		break;
	    }

	    case SET_SPECIAL :   /* set special options */
	    {
		char opt;
#ifdef PM_KEEP_OLD_ENHANCED_TEXT
		char param;
		static int prev_bEnhanced = 0;
#endif

		opt = 0;
		BufRead(hRead, &opt, 1, &cbR);
		switch (opt) {
#ifdef PM_KEEP_OLD_ENHANCED_TEXT
		case SET_SPECIAL_ENHANCED: /* enhanced mode on, off and restore */
		    param = -1;
		    BufRead(hRead, &param, 1, &cbR);
		    switch (param) {
		    case '0': prev_bEnhanced = bEnhanced;
			bEnhanced = 0;
			break;
		    case '1': prev_bEnhanced = bEnhanced;
			bEnhanced = 1;
			break;
		    case '2': bEnhanced = prev_bEnhanced;
			break;
		    }
		    break;
#endif
		case SET_SPECIAL_CODEPAGE: /* set codepage */
		{
		    int cp;

		    BufRead(hRead, &cp, sizeof(cp), &cbR);
		    if (codepage != cp) {
			codepage = cp;
			SelectFont(hps, szFontNameSize);
		    }
		    break;
		}
		case SET_SPECIAL_FONT: /* set default font */
		{
		    int len;

		    BufRead(hRead, &len, sizeof(int), &cbR);
		    len = (len + sizeof(int) - 1) / sizeof(int);

		    if (len != 0) {
			char font[FONTBUF];
			char *p, *tmp, *str;

			tmp = str = malloc(len * sizeof(int));
			BufRead(hRead, str, len * sizeof(int), &cbR);
			// deal with font size first
			p = strchr(str, ',');
			if (p == NULL) {
			    // fall back to initial font size
			    sprintf(font, "%i", iInitialFontSize);
			} else {
			    *p =  NUL;
			    strlcpy(font, p + 1, FONTBUF);
			}
			strlcat(font, ".", FONTBUF);
			/* allow abbreviation of some well known font names */
			FontExpand(str);
			strlcat(font, str, FONTBUF);
			free(tmp);
			strlcpy(szFontNameSize, font, FONTBUF);
			SelectFont(hps, font);
			DEBUG_FONT(("SET_SPECIAL_FONT: \"%s\"", font));
		    }
		    break;
		}
		case SET_SPECIAL_FONTSCALE:
		{
		    int newfontscale;

		    BufRead(hRead, &newfontscale, sizeof(int), &cbR);
		    if (fontscale != newfontscale) {
			fontscale = newfontscale;
			SelectFont(hps, szFontNameSize);
		    }
		    DEBUG_FONT(("SET_SPECIAL_FONTSCALE: %i\%", newfontscale));
		    break;
		}
		case SET_SPECIAL_RAISE: /* raise window */
		    WinSetWindowPos(hwndFrame, HWND_TOP, 0,0,0,0, SWP_RESTORE|SWP_SHOW|SWP_ACTIVATE|SWP_ZORDER);
		    WinSetFocus(HWND_DESKTOP, hApp);
		    break;
		case SET_SPECIAL_LOWER: /* lower window */
		    WinSetWindowPos(hwndFrame, HWND_BOTTOM, 0,0,0,0, SWP_ZORDER);
		    break;
		}
		break;
	    }

	    case PUT_TMPTEXT :
	    {  /* put_tmptext(int i, char c[]) term API */
		/* i = 0 at statusline,
		   1,2: at corners of zoom box, with \r separating text */
		int where, l;
		static char *text = NULL;
		static int text_alloc = -1;

		BufRead(hRead,&where, sizeof(int), &cbR);
		BufRead(hRead,&l, sizeof(int), &cbR);
		if (text_alloc < l)
		    text = realloc(text, text_alloc = l+10);
		BufRead(hRead, &text[0], l, &cbR);

		switch (where) {
		case 0:
		    UpdateStatusLine(hps,text);
		    break;
		case 1:
		    DrawZoomBox();
		    if (zoombox.text1)
			free(zoombox.text1);
		    zoombox.text1 = strdup(text);
		    DrawZoomBox();
		    break;
		case 2:
		    DrawZoomBox();
		    if (zoombox.text2)
			free(zoombox.text2);
		    zoombox.text2 = strdup(text);
		    DrawZoomBox();
		    break;
		}
		break;
	    }

	    /* Implementation problems(I haven't understood that from
	     * .INF doc): what is the difference between
	     * GpiCreateLogColorTable and GpiCreatePalette? */
	    case GR_MAKE_PALETTE :
	    {
		unsigned char c = 0;
		int smooth_colors;
		LONG lRetCount;

		/* read switch */
		BufRead(hRead, &c, sizeof(c), &cbR);
		if (c == 0) {
		    /* gnuplot asks for the number of colours in palette */
		    smooth_colors = (bPMPaletteMode ? (256 - nColors) : RGB_PALETTE_SIZE);
		    DosWrite(hRead, &smooth_colors, sizeof(int), &cbR);
		    DEBUG_COLOR(("GR_MAKE_PALETTE: max %i colours", smooth_colors));
		    break;
		}

		/* read the number of colours for the palette */
		BufRead(hRead, &smooth_colors, sizeof(int), &cbR);
		free(rgbTable);
		rgbTable = malloc(smooth_colors * sizeof(ULONG));

		/* append new RGB table after */
		DEBUG_COLOR(("GR_MAKE_PALETTE: reading palette with %i colours", smooth_colors));
		BufRead(hRead, &rgbTable[bPMPaletteMode ? nColors : 0],
			smooth_colors * sizeof(ULONG), &cbR);

		if (bPMPaletteMode) {
		    int i;
		    ULONG cclr;

		    /* preserve the first nColors entries of current palette */
		    /* retrieve the current table */
		    lRetCount = GpiQueryLogColorTable(hps, 0L, 0L, nColors, alColourTable);
		    if ((lRetCount > 0) && (lRetCount != nColors)) /* ring for developers! */
			DosBeep(880, 777);
		    for (i = 0; i < nColors; i++)
			rgbTable[i] = alColourTable[i];

		    if (pm3d_hpal != 0)
			GpiDeletePalette(pm3d_hpal);
		    pm3d_hpal = GpiCreatePalette(hab, 0L, LCOLF_CONSECRGB,
					     (long) (nColors + smooth_colors), rgbTable);
		    pm3d_hpal_old = GpiSelectPalette(hps, pm3d_hpal);

		    /* tell presentation manager to use the new palette */
		    WinRealizePalette(WinWindowFromDC(hdcScreen), hps, &cclr);
		}
		break;
	    }

	    case GR_RELEASE_PALETTE :
		/* no need to release the palette again */
		break;

	    case GR_SET_COLOR :
	    {
		/* Note: use of uchar limits the size of the 'virtual'
			  palette to 256 entries. (see also RGB_PALETTE_SIZE) */
		unsigned char c;

		FLUSHPATH(hps, "GR_SET_COLOR");

		BufRead(hRead, &c, sizeof(c), &cbR);
		// FIXME: need range checking here
		if (bPMPaletteMode)
		    pm3d_color = c + nColors;
		else
		    pm3d_color = rgbTable[c];
		color = pm3d_color;
		DEBUG_COLOR(("GR_SET_COLOR: %i -> 0x%x", (int)c, pm3d_color));
		break;
	    }

	    case GR_SET_RGBCOLOR :
	    {
		int rgb_color;

		FLUSHPATH(hps, "GR_SET_RGBCOLOR");

		BufRead(hRead, &rgb_color, sizeof(rgb_color), &cbR);
		/* ignore alpha value */
		rgb_color &= 0x00ffffff;

		// monochrome mode:  map to gray
		if (!bColours) {
		    int r = (rgb_color >> 16) & 0xff;
		    int g = (rgb_color >>  8) & 0xff;
		    int b = (rgb_color      ) & 0xff;
		    int luma = (r * 30 + g * 59 + b * 11) / 100;
		    rgb_color = (luma << 16) | (luma << 8) | luma;
		}

		/* Find an approximate color in the current palette */
		if (bPMPaletteMode)
		    pm3d_color = GpiQueryColorIndex(hps, 0, rgb_color);
		else
		    pm3d_color = rgb_color;
		color = pm3d_color;
#if 0
		{
		int real_rgb = GpiQueryRGBColor(hps, LCOLOPT_REALIZED, pm3d_color);
		DEBUG_COLOR(( "GR_SET_RGBCOLOR: req = %x  nearest = %x  index = %x",
		              rgb_color, real_rgb, pm3d_color ));
		}
#endif
		break;
	    }

	    case GR_FILLED_POLYGON :
		BufRead(hRead, &fillstyle, sizeof(fillstyle), &cbR);
		// fall-through

	    case 'f':  // old filled polygon - without fill style
	    {
		int points, x,y, i;
		LONG curr_color;
		POINTL p;

		FLUSHPATH(hps, "GR_FILLED_POLYGON");

		BufRead(hRead, &points, sizeof(points), &cbR);

		SetFillStyle(hps, fillstyle);
		/* using colours defined in the palette */
		if (pm3d_color >= 0) {
		    curr_color = GpiQueryColor(hps);
		    GpiSetColor(hps, pm3d_color);
		}

		GpiBeginArea(hps, BA_BOUNDARY | BA_ALTERNATE);
		for (i = 0; i < points; i++) {
		    BufRead(hRead, &x, sizeof(x), &cbR);
		    BufRead(hRead, &y, sizeof(y), &cbR);
		    p.x = x; p.y = y;
		    if (i)
			GpiLine(hps, &p);
		    else
			GpiMove(hps, &p);
		}
		GpiEndArea(hps);

		/* revert fill style changes */
		fillstyle = FS_SOLID | (100 << 4);
		GpiSetBackMix(hps, BM_OVERPAINT);
		GpiSetPattern(hps, PATSYM_SOLID);
		if (pm3d_color >= 0)
		    GpiSetColor(hps, curr_color);
		break;
	    }

	    case GR_RGB_IMAGE :
	    {
		unsigned int i, M, N, image_size;
		POINTL corner[4];
		PBYTE image;
		PBITMAPINFO2 pbmi;
		POINTL points[4];
		POINTL clip;
		LONG hits;
		image_list_entry *ile;

		FLUSHPATH(hps, "GR_RGBx_IMAGE");

		BufRead(hRead, &M, sizeof(M), &cbR);
		BufRead(hRead, &N, sizeof(N), &cbR);
		for (i = 0; i < 4; i++) {
		    BufRead(hRead, &(corner[i].x), sizeof(int), &cbR);
		    BufRead(hRead, &(corner[i].y), sizeof(int), &cbR);
		}
		usErr = BufRead(hRead, &image_size, sizeof(image_size), &cbR);
		DEBUG_IMAGE(("GR_IMAGE: M=%i, N=%i, size=%i, rc=%d", M, N, image_size, usErr));
		DEBUG_IMAGE(("GR_IMAGE: corner [0]=(%i,%i) [1]=(%i,%i)", corner[0].x, corner[0].y, corner[1].x, corner[1].y));
		image = (PBYTE) malloc(image_size);
		BufRead(hRead, image, image_size, &cbR);

		// monochrome mode:  map to gray
		if (!bColours) {
		    int x, y;
		    int pad_bytes = (4 - (3 * M) % 4) % 4; /* scan lines start on ULONG boundaries */
		    PBYTE pImage = image;
		    for (y = 0; y < N; y++) {
			for (x = 0; x < M; x++) {
			    RGB * pRGB = (RGB *) pImage;
			    int r = pRGB->bRed;
			    int g = pRGB->bGreen;
			    int b = pRGB->bBlue;
			    int luma = (r * 30 + g * 59 + b * 11) / 100;
			    pRGB->bRed = pRGB->bGreen = pRGB->bBlue = luma;
			    pImage += 3;
			}
			pImage += pad_bytes;
		    }
		}

		points[0].x = corner[0].x;
		points[0].y = corner[1].y;
		points[1].x = corner[1].x;
		points[1].y = corner[0].y;
		points[2].x = points[2].y = 0;
		points[3].x = M;
		points[3].y = N;

		// construct a clip path
		clip.x = corner[2].x;
		clip.y = corner[2].y;
		GpiMove(hps, &clip);
		GpiBeginPath(hps, 1);
		clip.x = corner[3].x;
		clip.y = corner[3].y;
		GpiBox(hps, DRO_OUTLINE, &clip, 0, 0);
		GpiEndPath(hps);
		GpiSetClipPath(hps, 1, SCP_AND);

		pbmi = (PBITMAPINFO2) calloc(sizeof(BITMAPINFOHEADER2), 1);
		pbmi->cbFix = sizeof(BITMAPINFOHEADER2);
		pbmi->cx = M;
		pbmi->cy = N;
		pbmi->cPlanes = 1;
		pbmi->cBitCount = 24;
		pbmi->ulCompression = BCA_UNCOMP;
		hits = GpiDrawBits(hps, image, pbmi, 4, points, ROP_SRCCOPY, BBO_IGNORE);
#if 0
		if (hits == GPI_ERROR) {
		    PERRINFO perriBlk = WinGetErrorInfo(hab);
		    if (perriBlk) {
			PSZ pszOffset, pszErrMsg;
			pszOffset = ((PSZ)perriBlk) + perriBlk->offaoffszMsg;
			pszErrMsg = ((PSZ)perriBlk) + *((PULONG)pszOffset);
			if (perriBlk->cDetailLevel >= 2)
				pszErrMsg = ((PSZ)perriBlk) + ((PULONG)pszOffset)[1];
			DEBUG_IMAGE(("GpiDrawBits code=%x msg=%s", perriBlk->idError, pszErrMsg));
			// DEBUG_IMAGE(("GpiDrawBits code=%x", perriErrorInfo->idError));
			WinFreeErrorInfo(perriBlk);
		    }
		}
#endif
		// reset clip region
		GpiSetClipPath(hps, 0, SCP_RESET);

		/* We have to keep the image data and header in memory since
		   we use retained graphics. */
		ile = (image_list_entry *) malloc(sizeof(image_list_entry));
		ile->next = image_list;
		ile->pbmi = pbmi;
		ile->image = image;
		image_list = ile;
		break;
	    }

	    case SET_RULER : /* set_ruler(int x, int y) term API: x<0 switches ruler off */
	    {
		int x, y;

		BufRead(hRead, &x, sizeof(x), &cbR);
		BufRead(hRead, &y, sizeof(y), &cbR);
		DrawRuler(); /* remove previous drawing, if any */
		DrawRulerLineTo();
		if (x < 0) {
		    ruler.on = FALSE;
		} else {
		    ruler.on = TRUE;
		    ruler.x = ruler_lineto.x = x;
		    ruler.y = ruler_lineto.x = y;
		    DrawRuler();
		    DrawRulerLineTo();
		}
		break;
	    }

	    case SET_CURSOR : /* set_cursor(int c, int x, int y) term API */
	    {
		int c, x, y;

		BufRead(hRead, &c, sizeof(c), &cbR);
		BufRead(hRead, &x, sizeof(x), &cbR);
		BufRead(hRead, &y, sizeof(y), &cbR);
		switch (c) {
		case -4: /* switch off line between ruler and mouse cursor */
		    DrawRulerLineTo();
		    ruler_lineto.on = FALSE;
		    break;
		case -3: /* switch on line between ruler and mouse cursor */
		    if (ruler.on && ruler_lineto.on)
			    break;
		    ruler_lineto.x = x;
		    ruler_lineto.y = y;
		    ruler_lineto.on = TRUE;
		    DrawRulerLineTo();
		    break;
		case -2: { /* move mouse to the given point */
		    RECTL rc;
		    POINTL pt;

		    GpiQueryPageViewport(hpsScreen, &rc);
		    /* only distance is important */
		    rc.xRight -= rc.xLeft;
		    rc.yTop -= rc.yBottom;
		    /* window => pixels coordinates */
		    pt.x = (long int) ((x * (double) rc.xRight) / (double) GNUXPAGE);
		    pt.y = (long int) ((x * (double) rc.yTop) / (double) GNUYPAGE);
		    WinMapWindowPoints(hApp, HWND_DESKTOP, &pt, 1);
		    WinSetPointerPos(HWND_DESKTOP, pt.x, pt.y);
		    break;
		}
		case -1: /* start zooming; zooming cursor */
		    zoombox.on = TRUE;
		    zoombox.from.x = zoombox.to.x = x;
		    zoombox.from.y = zoombox.to.y = y;
		    break;
		case 0:  /* standard cross-hair cursor */
		    WinSetPointer(HWND_DESKTOP,
				  hptrCurrent =
				  (useMouse ? hptrCrossHair:hptrDefault));
		    break;
		case 1:  /* cursor during rotation */
		    WinSetPointer(HWND_DESKTOP, hptrCurrent = hptrRotating);
		    break;
		case 2:  /* cursor during scaling */
		    WinSetPointer(HWND_DESKTOP, hptrCurrent = hptrScaling);
		    break;
		case 3:  /* cursor during zooming */
		    WinSetPointer(HWND_DESKTOP, hptrCurrent = hptrZooming);
		    break;
		}
		if (c >= 0 && zoombox.on) { /* erase zoom box */
		    DrawZoomBox();
		    zoombox.on = FALSE;
		}
		if (c >= 0 && ruler_lineto.on) { /* erase ruler line */
		    DrawRulerLineTo();
		    ruler_lineto.on = FALSE;
		}
		break;
	    }

	    case SET_CLIPBOARD :  /* set_clipboard(const char s[]) term API */
	    {
		int len;
		char *s;

		BufRead(hRead, &len, sizeof(int), &cbR);
		s = malloc(len + 1);
		BufRead(hRead,s, len+1, &cbR);
		TextToClipboard(s);
		free(s);
		break;
	    }

	    case SET_MENU :   /* update menu according to the gnuplot core
			  * settings, e.g.(un)checking menu items */
		if (mouseTerminal) {
		    /* we are connected to mouseable terminal */
		    BufRead(hRead, &gpPMmenu, sizeof(gpPMmenu), &cbR);
		    if (useMouse != gpPMmenu.use_mouse)
			WinSetPointer(HWND_DESKTOP,
				      hptrCurrent = (gpPMmenu.use_mouse
						     ?hptrCrossHair
						     :hptrDefault));
		    useMouse = gpPMmenu.use_mouse;
		    gpPMmenu_update_req = TRUE;
		}
		break;

	    case GR_MOUSECAPABLE :
		/* notification of being connected to a mouse-enabled terminal */
		mouseTerminal = TRUE;
		break;

	    default :  /* should handle error */
		break;
	    }
	}
    }
    DosDisConnectNPipe(hRead);
    WinPostMsg(hApp, WM_CLOSE, 0L, 0L);

#ifdef HAVE_EXCEPTQ
    UninstallExceptq(&exRegRec);
#endif
}


static void
SetFillStyle(HPS hps, int style)
{
    switch (style & 0x0f) {
    case FS_SOLID:
    case FS_TRANSPARENT_SOLID:
    {
	/* style == 1 --> fill with intensity according to filldensity */
	static const ULONG patternlist[] = {
	    PATSYM_NOSHADE, PATSYM_DENSE8, PATSYM_DENSE7,
	    PATSYM_DENSE6, PATSYM_DENSE5, PATSYM_DENSE4,
	    PATSYM_DENSE3, PATSYM_DENSE2, PATSYM_DENSE1,
	    PATSYM_SOLID
	};
	unsigned pattern;

	pattern = (unsigned) trunc(9 * ((style >> 4) / 100.0) + 0.5);
	if (pattern >= 10)
	    pattern = 9; /* only 10 patterns in list */
	GpiSetMix(hps, FM_OVERPAINT);
	if ((style & 0x0f) == FS_SOLID)
	    GpiSetBackMix(hps, BM_OVERPAINT);
	else
	    GpiSetBackMix(hps, BM_LEAVEALONE);
	GpiSetPattern(hps, patternlist[pattern]);
	break;
    }

    case FS_PATTERN:
    case FS_TRANSPARENT_PATTERN:
    {
	/* style == 2 --> fill with pattern according to fillpattern */
	/* the upper 3 nibbles of 'style' contain pattern number */
	static const ULONG patternlist[] = {
	    PATSYM_NOSHADE, PATSYM_DIAGHATCH,
	    PATSYM_HATCH, PATSYM_SOLID,
	    PATSYM_DIAG4, PATSYM_DIAG2,
	    PATSYM_DIAG3, PATSYM_DIAG1
	};
	unsigned pattern;

	pattern = (style >> 4) % 8;
	GpiSetMix(hps, FM_OVERPAINT);
	if ((style & 0x0f) == FS_PATTERN)
	    GpiSetBackMix(hps, BM_OVERPAINT);
	else
	    GpiSetBackMix(hps, BM_LEAVEALONE);
	GpiSetPattern(hps, patternlist[pattern]);
	break;
    }

    case FS_EMPTY:
    default:
	/* style == 0 or unknown --> fill with background color */
	GpiSetMix(hps, FM_OVERPAINT);
	GpiSetBackMix(hps, BM_OVERPAINT);
	GpiSetPattern(hps, PATSYM_SOLID);
	break;

    }
}


/*
** pull next plot command out of buffer read from GNUPLOT
*/
static APIRET
BufRead(HFILE hfile, void *buf, int nBytes, ULONG *pcbR)
{
    ULONG ulR, ulRR;
    int rc;
    static char buffer[GNUBUF];
    static char *pbuffer = buffer + GNUBUF;
    static char *ebuffer = buffer + GNUBUF;

    for (; nBytes > 0; nBytes--) {
	if (pbuffer >= ebuffer) {
	    ulR = GNUBUF;
	    rc = DosRead(hfile, buffer, ulR, &ulRR);
#ifdef HAVE_PMPRINTF
	    if ((rc != NO_ERROR && rc != ERROR_MORE_DATA) || (ulRR == 0))
		PmPrintf("BufRead error: %i bytes read: %li", rc, ulRR);
#endif
	    // ERROR_MORE_DATA signals that there's more data in the pipe
	    if (rc != NO_ERROR && rc != ERROR_MORE_DATA)
		return rc;
	    if (ulRR == 0)
		return ERROR_NO_DATA;
	    pbuffer = buffer;
	    ebuffer = pbuffer + ulRR;
	}
	*(char*)buf++ = *pbuffer++;
    }
    /* FIXME: pcbR is neither set nor used anywhere */
    return NO_ERROR;
}


#ifdef STANDARD_FONT_DIALOG

/*
** Get a new font using standard font dialog
*/
int
GetNewFont(HWND hwnd, HPS hps)
{
    static FONTDLG pfdFontdlg;      /* Font dialog info structure */
    static int i1 =1;
    static int iSize;
    char szPtList[64];
    HWND hwndFontDlg;     /* Font dialog window handle */
    char szFamilyName[FONTBUF];

    if (i1) {
        strcpy(pfdFontdlg.fAttrs.szFacename, strchr(szFontNameSize, '.') + 1);
        strcpy(szFamilyname, strchr(szFontNameSize, '.') + 1);
        sscanf(szFontNameSize, "%d", &iSize);
        memset(&pfdFontdlg, 0, sizeof(FONTDLG));

        pfdFontdlg.cbSize = sizeof(FONTDLG);
        pfdFontdlg.hpsScreen = hps;
	/*   szFamilyname[0] = 0; */
        pfdFontdlg.pszFamilyname = szFamilyname;
	pfdFontdlg.usFamilyBufLen = FONTBUF;
        pfdFontdlg.fl = FNTS_HELPBUTTON |
	    FNTS_CENTER | FNTS_VECTORONLY |
	    FNTS_OWNERDRAWPREVIEW;
        pfdFontdlg.clrFore = CLR_BLACK;
        pfdFontdlg.clrBack = CLR_WHITE;
        pfdFontdlg.usWeight = FWEIGHT_NORMAL;
        pfdFontdlg.fAttrs.usCodePage = codepage;
        pfdFontdlg.fAttrs.usRecordLength = sizeof(FATTRS);
    }
    sprintf(szPtList, "%d 8 10 12 14 18 24", iSize);
    pfdFontdlg.pszPtSizeList = szPtList;
    pfdFontdlg.fxPointSize = MAKEFIXED(iSize,0);
    hwndFontDlg = WinFontDlg(HWND_DESKTOP, hwnd, &pfdFontdlg);
    if (i1) {
        pfdFontdlg.fl = FNTS_HELPBUTTON |
	    FNTS_CENTER | FNTS_VECTORONLY |
	    FNTS_INITFROMFATTRS;
        i1=0;
    }
    if (hwndFontDlg &&(pfdFontdlg.lReturn == DID_OK)) {
        iSize = FIXEDINT(pfdFontdlg.fxPointSize);
        sprintf(szFontNameSize, "%d.%s", iSize, pfdFontdlg.fAttrs.szFacename);
        return 1;
    } else
	return 0;
}

#else

/*
** Get a new font using standard font palette
*/
int
GetNewFont(HWND hwnd, HPS hps)
{
    HOBJECT hObject;
    ULONG ulView = 0; /* OPEN_DEFAULT */
    BOOL fSuccess = FALSE;

    hObject = WinQueryObject("<WP_FNTPAL>");
    if (hObject != NULLHANDLE)
        fSuccess = WinOpenObject(hObject, ulView, TRUE);
    return fSuccess ? 1 : 0;
}

#endif /* STANDARD_FONT_DIALOG */


/*
**  Handle termination signal to free up resources before
**  termination.
*/
void SigHandler(int sig)
{
    if (sig == SIGTERM) {
        if (bPersist) {
            DosKillThread(tidSpawn);
            return;
	}
        DosEnterCritSec();
        DosKillThread(tidSpawn);
        DosKillThread(tidDraw);
        DosExitCritSec();
        exit(0);
    }
}


#ifdef PM_KEEP_OLD_ENHANCED_TEXT

/* process a bit of string, and return the last character used.
 * p is start of string
 * brace is TRUE to keep processing to }, FALSE for do one character
 * fontname & fontsize are obvious
 * base is the current baseline
 * widthflag is TRUE if the width of this should count,
 *              FALSE for zero width boxes
 * showflag is TRUE if this should be shown,
 *             FALSE if it should not be shown(like TeX \phantom)
 */

static char *starttext = NULL;
static int  textlen = 0;
static BOOL bText = FALSE;
static int  textwidth = 0;
static POINTL ptlText;

static char
*ParseText(
    HPS hps,
    char *p,
    BOOL brace,
    char *fontname,
    int fontsize, int base,
    BOOL widthflag, BOOL showflag)
{
    POINTL aptl[TXTBOX_COUNT];
    BOOL bChangeFont = FALSE;
    TEXT_DEBUG(("RECURSE WITH [%p] %s, %d %s %d %d %d %d\n",
		p, p, brace, fontname, fontsize, base,
		widthflag, showflag));

    /* Start each recursion with a clean string */
    if (textlen > 0) {
	GpiQueryTextBox(hps, textlen, starttext, TXTBOX_COUNT, aptl);
	textwidth += aptl[TXTBOX_BOTTOMRIGHT].x * multLineHor;
	textwidth += aptl[TXTBOX_BOTTOMRIGHT].y * multLineVert;
    }

    if (bText) {
	if (textlen > 0) {
	    GpiCharStringAt(hps, &ptlText, textlen, starttext);
	    ptlText.x += aptl[TXTBOX_CONCAT].x - multLineVert * base;
	    ptlText.y += aptl[TXTBOX_CONCAT].y + multLineHor * base;
	} else {
	    ptlText.x -= multLineVert * base;
	    ptlText.y += multLineHor * base;
	}
    }
    textlen = 0;
    starttext = p;

    if (fontname != NULL) {
	char szFont[FONTBUF];

	snprintf(szFont, FONTBUF, "%d.%s", fontsize, fontname);
	SwapFont(hps, szFont);
	bChangeFont = TRUE;
    }
    if (base != 0)
	GpiSetCharBox(hps, &sizCurSubSup);

    for (; *p; ++p) {
	int shift;

	switch (*p) {
	case '}':
	    /*{{{  deal with it*/
	    if (brace) {
		brace = 0;
	    }
	    break;
	    /*}}}*/

	case '_':
	case '^':
	    /*{{{  deal with super/sub script*/
	    shift = ((*p == '^') ? lSupOffset : lSubOffset);
	    p = ParseText(hps, p+1, FALSE,
			  NULL/*fontname*/, fontsize*0.8,
			  base+shift, widthflag, showflag);
	    break;
	    /*}}}*/

	case '{':
	{
	    char *savepos=NULL, save=0;
	    char *localfontname=fontname, ch;
	    char localfontbuf[FONTBUF];
	    int recode=1;
	    int f=fontsize;
	    char *q=localfontbuf;

	    /*{{{  recurse(possibly with a new font) */

	    TEXT_DEBUG(("Dealing with {\n"));
	    if (*++p == '/') {
		/* then parse a fontname, optional fontsize */
		while (*++p == ' ')
		    ;		/* do nothing */
		if (*p=='-') {
		    recode=0;
		    while (*++p == ' ')
			;	/* do nothing */
		}
		localfontname = p;
		while ((ch = *p) > ' ' && ch != '=') {
		    localfontname=localfontbuf;
		    if (*p=='_')
			*q=' ';
		    else
			*q=*p;
		    ++p;++q;
		}
		*q = '\0';
		FontExpand(localfontbuf);
		save = *(savepos=p);
		if (ch == '=') {
		    *p++ = '\0';
		    /*{{{  get optional font size*/
		    TEXT_DEBUG(("Calling strtod(%s) ...", p));
		    f = strtod(p, &p);
		    TEXT_DEBUG(("Returned %.1f and %s\n", f, p));

		    if (!f)
			f = fontsize;

		    TEXT_DEBUG(("Font size %d\n", f));
		    /*}}}*/
		} else {
		    *p++ = '\0';
		    f = fontsize;
		}

		while (*p == ' ')
		    ++p;

		if (!(*localfontname)) {
		    localfontname = fontname;
		    if (f != fontsize)
			localfontname = strchr(szFontNameSize, '.') + 1;
		}
	    } /* if ('/') */
	    /*}}}*/

	    TEXT_DEBUG(("Before recursing, we are at [%p] %s\n", p, p));

	    p = ParseText(hps,p, TRUE, localfontname, f, base, widthflag, showflag);

	    TEXT_DEBUG(("BACK WITH %s\n", p));
	    if (savepos)
		/* restore overwritten character */
		*savepos = save;

	    break;
	}

	case '@' :
	    /*{{{  phantom box - prints next 'char', then restores currentpoint */

	    p = ParseText(hps, ++p, FALSE, NULL/*fontname*/, fontsize,
			  base, FALSE, showflag);

	    break;
	    /*}}}*/

	case '&' :
	    /*{{{  character skip - skips space equal to length of character(s) */

	    p = ParseText(hps, ++p, FALSE, NULL/*fontname*/, fontsize,
			  base, widthflag, FALSE);

	    break;
	    /*}}}*/

	case '\\':
	{
	    char buffer[4]; /* should need only one char.. */
	    char *q = buffer;

	    *q = '\0';
	    ParseText(hps,q, FALSE, NULL, fontsize, base, widthflag, showflag);

	    /*{{{  is it an escape */
	    /* special cases */
	    if (p[1]=='\\' || p[1]=='{' || p[1]=='}') {
		*q++=p[1];
		++p;
	    }
#if 0
	    else if (p[1] >= '0' && p[1] <= '7') {
		/* up to 3 octal digits */
		int c = 0;

		c+=p[1];
		++p;
		if (p[1] >= '0' && p[1] <= '7') {
		    c = c * 8 + p[1];
		    ++p;
		    if (p[1] >= '0' && p[1] <= '7') {
			c = c * 8 + p[1];
			++p;
		    }
		}
		*q++ = c;
		break;
	    }
#endif /* 0 */

	    *q = '\0';
	    textlen = 1;
	    starttext=buffer;
	    ParseText(hps,q, FALSE, NULL/*fontname*/, fontsize,
		      base, widthflag, showflag);
	    starttext=p+1;
	    textlen = 0;
	    /*}}}*/
	    break;
	}

	default:
	    ++textlen;

	    /*}}}*/
	} /* switch(character) */

	/* like TeX, we only do one character in a recursion, unless it's
	 * in braces
	 */
	if (!brace)
	    break;
    }

    if (textlen > 0) {
	GpiQueryTextBox(hps, textlen, starttext, TXTBOX_COUNT, aptl);
	if (widthflag) {
	    textwidth += aptl[TXTBOX_BOTTOMRIGHT].x * multLineHor;
	    textwidth += aptl[TXTBOX_BOTTOMRIGHT].y * multLineVert;
	}
    }
    if (bText) {
	if (textlen > 0) {
	    if (showflag)
		GpiCharStringAt(hps, &ptlText, textlen, starttext);
	    if (widthflag) {
		ptlText.x += aptl[TXTBOX_CONCAT].x;
		ptlText.y += aptl[TXTBOX_CONCAT].y;
	    }
	}
	if (base != 0) {
	    ptlText.x += multLineVert * base;
	    ptlText.y -= multLineHor * base;
	}
    }
    if (bChangeFont) {
	SwapFont(hps, NULL);
	bChangeFont = FALSE;
    }
    if (base != 0)
	GpiSetCharBox(hps, &sizBaseFont);

    textlen = 0;
    starttext = p+1;
    return p;
}


static void
CharStringAt(HPS hps, int x, int y, int len, char *str)
{
    /* flush any pending graphics (all the XShow routines do this...) */
    char *fontname;
    int fontsize;

    if (!strlen(str))
        return;

    /* set up the globals */
    ptlText.x = x;
    ptlText.y = y;
    bText = TRUE;
    starttext = NULL;
    textlen = 0;
    textwidth = 0;
    sscanf(szFontNameSize, "%d", &fontsize);
    fontname = strchr(szFontNameSize, '.') + 1;

    while (*(str = ParseText(hps, str, TRUE, NULL,
			     fontsize,
			     0.0, TRUE, TRUE)))
	; /* do nothing */
}


static int
QueryTextBox(HPS hps, int len, char *str)
{
    char *fontname;
    int fontsize;

    if (!strlen(str))
        return 0;

    /* set up the globals */
    bText = FALSE;
    starttext = NULL;
    textlen = 0;
    textwidth = 0;
    sscanf(szFontNameSize, "%d", &fontsize);
    fontname = strchr(szFontNameSize, '.') + 1;

    while (*(str = ParseText(hps, str, TRUE, NULL,
			     fontsize,
			     0.0, TRUE, TRUE)))
	; /* do nothing */

    return textwidth;
}

#endif


void
FontExpand(char *name)
{
    char * p, *attr;

    // make a temporary copy of the font attributes
    p = strchr(name, ':');
    if (p != NULL) {
	attr = strdup(p);
	*p = NUL;
    }

    if (strcmp(name, "S") == 0)
	strcpy(name, "Symbol Set");
    if (strcmp(name, "Symbol") == 0)
	strcpy(name, "Symbol Set");
    else if (strcmp(name, "H") == 0)
	strcpy(name, "Helvetica");
    else if (strcmp(name, "sans") == 0)
	strcpy(name, "Helvetica");
    else if (strcmp(name, "T") == 0)
	strcpy(name, "Times New Roman");
    else if (strcmp(name, "Times") == 0)
	strcpy(name, "Times New Roman");
    else if (strcmp(name, "serif") == 0)
	strcpy(name, "Times New Roman");
    else if (strcmp(name, "C") == 0)
	strcpy(name, "Courier");

    // append them again to the font name
    if (p != NULL) {
	strcat(name, attr);
	free(attr);
    }
}


/*=======================================*/

static POINTL pCur;
static int iLinebegin = 1;
static int iLtype = 0;
static int iState = 0;
static double togo = 0.0;
static int iPatt[8][9] = {
    {   0,   0,   0,   0,   0,   0,   0,   0,  0 },
    { 300, 200,  -1,   0,   0,   0,   0,   0,  0 },
    { 150, 150,  -1,   0,   0,   0,   0,   0,  0 },
    { 300, 200, 150, 200,  -1,   0,   0,   0,  0 },
    { 500, 200,  -1,   0,   0,   0,   0,   0,  0 },
    { 300, 200, 150, 200, 150, 200,  -1,   0,  0 },
    { 300, 200, 150, 200, 150, 200, 150, 200, -1 },
    { 500, 200, 150, 200,  -1,   0,   0,   0,  0 }
};


void
LMove(HPS hps, POINTL *p)
{
    double ds, dx, dy;

    if (iLinebegin) {
        pCur = *p;
        GpiMove(hps, p);
    } else if (iLtype == 0)
	GpiMove(hps, p);
    else {
        dx = p->x - pCur.x;
        dy = p->y - pCur.y;
        ds = sqrt(dx*dx + dy*dy);
        dx /= ds; dy /= ds;
        while (ds > 0.0) {
            if (ds < togo) {
                togo -= ds;
                ds = 0.0;
                GpiMove(hps, p);
                pCur = *p;
	    } else {
                POINTL pn;

                pn.x = pCur.x + togo * dx;
                pn.y = pCur.y + togo * dy;
                GpiMove(hps, &pn);
                pCur = pn;
                ds -= togo;
                iState++;
                if (iPatt[iLtype][iState] < 0) {
                    togo = iPatt[iLtype][0];
                    iState = 0;
		} else
		    togo = iPatt[iLtype][iState];
	    }
	}
    }
}


void
LLine(HPS hps, POINTL *p)
{
    double ds, dx, dy;

    if (iLinebegin)
	iLinebegin = 0;

    if (iLtype == 0)
	GpiLine(hps, p);
    else {
        dx = p->x - pCur.x;
        dy = p->y - pCur.y;
        ds = sqrt(dx*dx + dy*dy);
        dx /= ds; dy /= ds;
        while (ds > 0.0) {
            if (ds < togo) {
                togo -= ds;
                ds = 0.0;
                if (iState & 1)
		    GpiMove(hps, p);
                else
		    GpiLine(hps, p);
                pCur = *p;
	    } else {
                POINTL pn;

                pn.x = pCur.x + togo * dx;
                pn.y = pCur.y + togo * dy;
                if (iState & 1)
		    GpiMove(hps, &pn);
                else
		    GpiLine(hps, &pn);
                pCur = pn;
                ds -= togo;
                iState++;
                if (iPatt[iLtype][iState] < 0) {
                    togo = iPatt[iLtype][0];
		    iState = 0;
		} else
		    togo = iPatt[iLtype][iState];
	    }
	}
    }
}


int
LType(int iType)
{
    int prevLtype = iLtype;
    iLinebegin = 1;
    if (iType > 7)
	iType = 0;
    iLtype = iType;
    iState = 0;
    togo = iPatt[iLtype][0];
    return prevLtype;
}


/* Now routines for mouse etc. */

static void
TextToClipboard(PCSZ szTextIn)
{
    /* copy string given by szTextIn to the clipboard */
    PSZ szTextOut = NULL;
    ULONG ulRC = DosAllocSharedMem(
	(PVOID*) &szTextOut, NULL,
	strlen(szTextIn) + 1,
	PAG_READ | PAG_WRITE | PAG_COMMIT | OBJ_GIVEABLE);

    if (!ulRC) {
	HAB hab = 0;

	strcpy(szTextOut, szTextIn);
	WinOpenClipbrd(hab);
	WinEmptyClipbrd(hab);
	WinSetClipbrdData(hab,(ULONG) szTextOut, CF_TEXT, CFI_POINTER);
	WinCloseClipbrd(hab);
    }
}


/* Draw the ruler
*/
static void
DrawRuler()
{
    POINTL p;

    if (!ruler.on || ruler.x < 0)
	return;

    GpiSetColor(hpsScreen, RGB_TRANS(CLR_RED));
    GpiSetLineWidth(hpsScreen, LINEWIDTH_NORMAL);
    GpiSetMix(hpsScreen, FM_INVERT);

    p.x = 0;
    p.y = ruler.y;
    GpiMove(hpsScreen, &p);

    p.x = GNUXPAGE;
    GpiLine(hpsScreen, &p);

    p.x = ruler.x;
    p.y = 0;
    GpiMove(hpsScreen, &p);

    p.y = GNUYPAGE;
    GpiLine(hpsScreen, &p);

    GpiSetMix(hpsScreen, FM_DEFAULT);
}


/* Draw the ruler line to cursor position.
 */
static void
DrawRulerLineTo()
{
    POINTL p;

    if (!ruler.on || !ruler_lineto.on || ruler.x < 0 || ruler_lineto.x < 0)
	return;

    GpiSetColor(hpsScreen, RGB_TRANS(CLR_RED));
    GpiSetLineWidth(hpsScreen, LINEWIDTH_NORMAL);
    GpiSetMix(hpsScreen, FM_INVERT);

    p.x = ruler.x;
    p.y = ruler.y;
    GpiMove(hpsScreen, &p);

    p.x = ruler_lineto.x;
    p.y = ruler_lineto.y;
    GpiLine(hpsScreen, &p);

    GpiSetMix(hpsScreen, FM_DEFAULT);
}


/*
 * This routine recalculates mouse/pointer position [mx,my] in [in pixels]
 * current window to the viewport coordinates
 */
static void
MousePosToViewport(int *x, int *y, SHORT mx, SHORT my)
{
    RECTL rc;

    /* Rectangle where we are moving: viewport, not the full window! */
    if (GpiQueryPageViewport(hpsScreen, &rc)) {
	rc.xRight -= rc.xLeft;
	rc.yTop -= rc.yBottom; /* only distance is important */

	/* px=px(mx); mouse=>gnuplot driver coordinates */
	*x = (int)(mx * (double) GNUXPAGE / rc.xRight + 0.5);
	*y = (int)(my * (double) GNUYPAGE / rc.yTop + 0.5);
    } else {
	/* call was unsuccessful */
	*x = *y = 0;
    }
}


/*
 * This routine gets the mouse/pointer position in the current window and
 * recalculates it to the viewport coordinates
 */
static void
GetMousePosViewport(HWND hWnd, int *mx, int *my)
{
    POINTL p;

    WinQueryPointerPos(HWND_DESKTOP, &p); /* this is position wrt desktop */
    WinMapWindowPoints(HWND_DESKTOP, hWnd, &p, 1); /* pos. wrt our window in pixels */
    MousePosToViewport(mx, my, p.x, p.y);
}


/*
 * Status line previous and current text:
 */
static char *sl_curr_text = NULL;


/*
 * Discard the status line text (because we start a new graph)
 */
static void
ResetStatusLineText(void)
{
    free(sl_curr_text);
    sl_curr_text = NULL;
}


/*
 * Display the status line by the text
 */
static void
DisplayStatusLine(HPS hps)
{
    POINTL pt;

    if (!sl_curr_text)
	return;

    pt.x = 2;
    pt.y = 2;
    DrawMouseText(hps, &pt, sl_curr_text, strlen(sl_curr_text));
}


/*
 * Update the status line by the text; firstly erase the previous text
 */
static void
UpdateStatusLine(HPS hps, char *text)
{
    if (gpPMmenu_update_req)
	gpPMmenu_update(); /* check for updated menu */
    if (sl_curr_text) {
	/* erase the previous text */
	DisplayStatusLine(hps);
	free(sl_curr_text);
    }
    if (!text || !*text)
	sl_curr_text = 0;
    else {
	/* display new text */
	sl_curr_text = strdup(text);
	DisplayStatusLine(hps);
    }
}


/*
 * Graphics part SET_GRAPHICS or anything else required to update menu according to
 * the items in gpPMmenu
 */
static void
gpPMmenu_update()
{
#if 0
    if (!gpPMmenu_update_req)
	return;
#endif

    ChangeCheck(hApp, IDM_USEMOUSE, useMouse ? IDM_USEMOUSE:0);
    ChangeCheck(hApp, IDM_LINES_THICK, bWideLines ? IDM_LINES_THICK:0);
    WinEnableMenuItem(/* can this situation be unzoomed back? */
	WinWindowFromID(WinQueryWindow(hApp, QW_PARENT), FID_MENU),
	IDM_MOUSE_UNZOOM,(gpPMmenu.where_zoom_queue & 1) ? TRUE : FALSE);
    WinEnableMenuItem(/* can this situation be unzoomed to the beginning? */
	WinWindowFromID(WinQueryWindow(hApp, QW_PARENT), FID_MENU),
	IDM_MOUSE_UNZOOMALL,(gpPMmenu.where_zoom_queue & 2) ? TRUE : FALSE);
    if (
	WinEnableMenuItem(/* can this situation be zoomed next? */
	    WinWindowFromID(WinQueryWindow(hApp, QW_PARENT), FID_MENU),
	    IDM_MOUSE_ZOOMNEXT,(gpPMmenu.where_zoom_queue & 4) ? TRUE : FALSE)
	== TRUE)
	gpPMmenu_update_req = FALSE;
    ChangeCheck(hApp, IDM_MOUSE_POLAR_DISTANCE, gpPMmenu.polar_distance?IDM_MOUSE_POLAR_DISTANCE:0);
}


static void
DrawMouseText(HPS hps, PPOINTL pt, char * text, LONG len)
{
    GpiSetColor(hps, RGB_TRANS(COLOR_MOUSE));
    GpiSetCharMode(hps, CM_MODE1);
    GpiSetMix(hps, FM_INVERT);
    GpiCharStringAt(hps, pt, len, text);

    GpiSetMix(hps, FM_DEFAULT);
}


static void
DrawZoomBox()
{
    HPS hps = hpsScreen;
    POINTL pt;

    if (!zoombox.on)
	return;

    GpiSetLineWidth(hpsScreen, LINEWIDTH_NORMAL);
    GpiSetLineType(hpsScreen, LINETYPE_SHORTDASH);
    GpiSetMix(hpsScreen, FM_INVERT);
    GpiMove(hpsScreen, &zoombox.from);
    GpiBox(hpsScreen, DRO_OUTLINE, &zoombox.to, 0, 0);

    GpiSetLineType(hpsScreen, LINETYPE_DEFAULT);
    GpiSetMix(hpsScreen, FM_DEFAULT);

    if (zoombox.text1) {
	char *separator = strchr(zoombox.text1, '\r');
	if (separator) {
	    pt = zoombox.from;
	    pt.y -= lCharHeight;
	    DrawMouseText(hps, &zoombox.from, zoombox.text1, separator - zoombox.text1);
	    DrawMouseText(hps, &pt, separator + 1, strlen(separator + 1));
	} else {
	    pt = zoombox.from;
	    pt.y -= lCharHeight / 2;
	    DrawMouseText(hps, &zoombox.from, zoombox.text1, strlen(zoombox.text1));
	}
    }
    if (zoombox.text2) {
	char *separator = strchr(zoombox.text2, '\r');
	if (separator) {
	    pt = zoombox.to;
	    pt.y -= lCharHeight;
	    DrawMouseText(hps, &zoombox.to, zoombox.text2, separator - zoombox.text2);
	    DrawMouseText(hps, &pt, separator + 1, strlen(separator + 1));
	} else {
	    pt = zoombox.to;
	    pt.y -= lCharHeight / 2;
	    DrawMouseText(hps, &pt, zoombox.text2, strlen(zoombox.text2));
	}
    }
}

/* eof gclient.c */

