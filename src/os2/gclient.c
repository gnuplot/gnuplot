#ifdef INCRCSDATA
static char RCSid[]="$Id: gclient.c,v 1.5 1999/06/22 11:55:53 lhecking Exp $" ;
#endif

/****************************************************************************

    PROGRAM: Gnupmdrv
    
    MODULE:  gclient.c
        
    This file contains the client window procedures for Gnupmdrv
    
****************************************************************************/

/* PM driver for GNUPLOT */

/*[
 * Copyright 1992, 1993, 1998   Roger Fearick
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
 *   Petr Mikulik  (see  //PM  labels)
 *       - menu item to keep aspect ratio on/off (October 1997)
 *       - mouse support (1998, 1999); changes made after gnuplot 3.7.0.5:
 *	    - use gnuplot's pid in the name of shared memory (11. 5. 1999) 
 *	    - mouse in maps; distance in polar coords (14. 5. 1999)
 *	    - event semaphore for sending data via shared memory; 
 *	      zooming completely revised;
 *	      remap keys in 'case WM_CHAR';
 *	      Ctrl-C for breaking long drawings;
 *	      new menu items under 'Mouse' and 'Utilities' (August 1999)
 *
 *   Franz Bakan  (see  //fraba  labels)
 *       - communication gnupmdrv -> gnuplot via shared memory (April 1999)
 *       - date and time on x axis (August 1999)
 *
 *
 * Send your comments or suggestions to 
 *  info-gnuplot@dartmouth.edu.
 * This is a mailing list; to join it send a note to 
 *  majordomo@dartmouth.edu.  
 * Send bug reports to
 *  bug-gnuplot@dartmouth.edu.
**/

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
#define INCL_DOSQUEUES
#define INCL_WINSWITCHLIST
#include <os2.h>
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
#include "gnupmdrv.h"

/*==== g l o b a l    d a t a ================================================*/

extern char szIPCName[] ;       /* name used in IPC with gnuplot */
extern char szIniFile[256] ;    /* full path of ini file */
extern int  bServer ;
extern int  bPersist ;
extern int  bEnhanced ;

/*==== l o c a l    d a t a ==================================================*/

static long lLineTypes[7] = { LINETYPE_SOLID,
                              LINETYPE_SHORTDASH,
                              LINETYPE_DOT,
                              LINETYPE_DASHDOT,
                              LINETYPE_LONGDASH,
                              LINETYPE_DOUBLEDOT,
                              LINETYPE_DASHDOUBLEDOT } ;
static long lCols[16] =     { CLR_BLACK,
                              CLR_DARKGRAY,
                              CLR_BLUE,
                              CLR_RED,
                              CLR_GREEN,
                              CLR_CYAN,
                              CLR_PINK,
                              CLR_YELLOW,
                              CLR_DARKBLUE,
                              CLR_DARKRED,
                              CLR_DARKGREEN,
                              CLR_DARKCYAN,
                              CLR_DARKPINK,
                              CLR_BROWN,
                              CLR_PALEGRAY,
                              CLR_WHITE } ;

static LONG alColourTable[ 16 ] ;

#define   GNUBUF    1024        /* buffer for gnuplot commands */
#define   PIPEBUF   4096        /* size of pipe buffers */
#define   CMDALLOC  4096        /* command buffer allocation increment (ints) */
#define   ENVSIZE   2048        /* size of environment */ 

#define   PAUSE_DLG 1           /* pause handled in dialog box */
#define   PAUSE_BTN 2           /* pause handled by menu item */
#define   PAUSE_GNU 3           /* pause handled by Gnuplot */

#define   MOUSE_COORDINATES_REAL     1 //PM which mouse coordinates
#define   MOUSE_COORDINATES_PIXELS   2
#define   MOUSE_COORDINATES_SCREEN   3
#define   MOUSE_COORDINATES_XDATE	4
#define   MOUSE_COORDINATES_XTIME	5
#define   MOUSE_COORDINATES_XDATETIME	6

#define   DEFLW     50

static ULONG    ppidGnu=0L ;      /* gnuplot pid */
static HPS      hpsScreen ;     /* screen pres. space */
static int      iSeg = 1 ;

static HSWITCH hSwitch = 0 ;    /* switching between windows */
static SWCNTRL swGnu ;

static BOOL     bLineTypes = FALSE ;    // true if use dashed linetypes
BOOL     bWideLines ;
static BOOL     bColours = TRUE ;
static BOOL     bShellPos = FALSE ;
static BOOL     bPlotPos = FALSE ;
static BOOL     bPopFront = TRUE ;
static BOOL     bKeepRatio = TRUE ;	//PM
static BOOL     bNewFont = FALSE ;
static BOOL     bHorz = TRUE ;

//PM:
static BOOL	bUseMouse = FALSE;
static int 	ulMouseSprintfFormatItem = IDM_MOUSE_FORMAT_XcY;
static BOOL     bSend2gp = FALSE ;
const  int	nMouseSprintfFormats = IDM_MOUSE_FORMAT_LABEL - IDM_MOUSE_FORMAT;
const  char  *( MouseSprintfFormats[ /*nMouseSprintfFormats*/ ] ) = {
		"%g %g","%g,%g","%g;%g",
		"%g,%g,","%g,%g;",
		"[%g:%g]","[%g,%g]","[%g;%g]",
		"set label \"\" at %g,%g"
		 };
const  char  *( SetDataStyles[] ) = {
		"boxes", "dots", "fsteps", "histeps", "impulses",
		"lines", "linespoints", "points", "steps"
		 };


static BOOL	bMousePolarDistance = FALSE;

#define NEW_TRACK_ZOOM
  //PM 23. 8. 1999
  // define the above for using new method for tracing the zooming rectangle,
  // undefine for using previous WinTracRect API (zooming in 1 quadrant only)

#ifdef NEW_TRACK_ZOOM
int zooming = 0;  // set to 1 during zooming
POINTL zoomrect_from, zoomrect_now;
//#define ZOOM_SETUP GpiSetLineType(hps,LINETYPE_SHORTDASH); GpiSetMix(hps,FM_XOR); GpiSetColor(hps,CLR_WHITE);
#define ZOOM_SETUP GpiSetLineType(hps,LINETYPE_SHORTDASH); GpiSetMix(hps,FM_INVERT);
#endif // NEW_TRACK_ZOOM

//fraba:
static BOOL bhave_grid = FALSE ; // for toggling grid by mouse
static BOOL bhave_log = FALSE ;  // for toggling lin/log by mouse

static ULONG    ulPlotPos[4] ;
static ULONG    ulShellPos[4] ;
static PAUSEDATA pausedata = {sizeof(PAUSEDATA), NULL, NULL} ;
static char     szFontNameSize[FONTBUF] ;
static PRQINFO3 infPrinter = { "" } ;
static QPRINT   qPrintData = { sizeof(QPRINT), 0.0, 0.0, 1.0, 1.0, 0, 
                               "", "", &infPrinter, 0, NULL } ;
//static HEV      semStartSeq ;   /* semaphore to start things in right sequence */
static HEV      semPause ;
static HMTX     semHpsAccess ;
static ULONG    ulPauseReply = 1 ;
static ULONG    ulPauseMode  = PAUSE_DLG ;

static HWND     hSysMenu ;
            /* stuff for screen-draw thread control */
            
static BOOL     bExist ; 
//static HEV      semDrawDone ;

            /* thread control */

static TID     tidDraw, tidSpawn ;

            /* //PM flag for stopping (long) drawings */

static int breakDrawing = 0;

            /* font data */

static int lSupOffset = 0 ;
static int lSubOffset = 0 ;
static int lBaseSupOffset = 0 ;
static int lBaseSubOffset = 0 ;
static int lCharWidth = 217 ;
static int lCharHeight = 465 ; 


//PM: Now variables for mouse

static ULONG mouse_mode = MOUSE_COORDINATES_REAL;

/* gnuplot's PM terminal sends 'm' message from its init routine, which
   sets the variable below to 1. Then we are sure that we talk to the
   mouseable terminal and can read the mouseable data from the pipe. 
   Non-mouseable versions of PM terminal or non-new-gnuplot programs 
   using gnupmdrv will let this variable set to 0, thus no mousing occurs.
*/
static char mouseTerminal = 0;

/* Lock (hide) mouse when building the plot (redrawing screen).
Otherwise gnupmdrv would crash when trying to display mouse position
   in a window not yet plotted.
*/
static char lock_mouse = 1;

/* Structure for mouse used for the recalculation of the mouse coordinates
in pixels into the true coordinates of the plot.
   Note: any change to this structure must be accompanied by the appropriate
pushes into PM_pipe in file pm.trm, routine PM_graphics() !
*/
static struct {
    int graph;
      /*
      What the mouse is moving over?
	0 ... cannot use mouse with this graph---multiplot, for instance.
	1 ... 2d polar graph
	2 ... 2d graph
	3 ... 3d graph (not implemented, thus pm.trm sends 0)
	// note: 3d picture plotted as a 2d map is treated as 2d graph
      */
    double xmin, ymin, xmax, ymax; // range of x1 and y1 axes of 2d plot
    int xleft, ybot, xright, ytop; // pixel positions of the above
    int /*TBOOLEAN*/ is_log_x, is_log_y; // are x and y axes log?
    double base_log_x, base_log_y; // bases of log
    double log_base_log_x, log_base_log_y; // log of bases
    } gp4mouse;

/* Zoom queue
*/

struct t_zoom {
  double xmin, ymin, xmax, ymax;
  struct t_zoom *prev, *next;
  };

struct t_zoom *zoom_head = NULL,
	      *zoom_now = NULL;

/* Structure for the ruler: on/off, position,...
*/
static struct {
   char on;
   double x, y;  // ruler position in real units of the graph
   long px, py;  // ruler position in the viewport units
   } ruler;

// pointer definition
HWND hptrDefault, hptrCrossHair;

// colours of mouse-relating drawings
#define COLOR_MOUSE    CLR_DEFAULT  // mouse position
#define COLOR_ANNOTATE CLR_DEFAULT  // annotating strings (MB3)
#define COLOR_RULER    CLR_DARKPINK // colour of the ruler

PVOID input_from_PM_Terminal; //fraba

HEV semInputReady = 0; //PM

int pausing = 0; // avoid passing data back to gnuplot in `pause' mode


/*==== f u n c t i o n s =====================================================*/

int             DoPrint( HWND ) ;
HBITMAP         CopyToBitmap( HPS ) ; 
HMF             CopyToMetaFile( HPS ) ; 
MRESULT         WmClientCmdProc( HWND , ULONG, MPARAM, MPARAM ) ; 
void            ChangeCheck( HWND, USHORT, USHORT ) ;
BOOL            QueryIni( HAB ) ;
static void     SaveIni( HWND ) ;
static void     ThreadDraw( void* ) ;
static void     DoPaint( HWND, HPS ) ;
static void     Display( HPS ) ;
void            SelectFont( HPS, char * );
void            SwapFont( HPS, char * );
static void     CopyToClipBrd( HWND ) ; 
static void     ReadGnu( void* ) ;
static void     EditLineTypes( HWND, HPS, BOOL ) ;
static void     EditCharCell( HPS, SIZEF* ) ;
static HPS      InitScreenPS( void ) ;
static int      BufRead( HFILE, void*, int, PULONG ) ;
int             GetNewFont( HWND, HPS ) ;
void            SigHandler( int ) ;
void            FontExpand( char * ) ;
static char    *ParseText(HPS, char *, BOOL, char *,
                       int, int, BOOL, BOOL ) ;
static void     CharStringAt(HPS, int, int, int, char *) ;
static int      QueryTextBox( HPS, int, char * ) ; 
static void      LMove( HPS hps, POINTL *p ) ;
static void      LLine( HPS hps, POINTL *p ) ;
static void      LType( int iType ) ;

/* //PM: new functions related to the mouse processing */
static void	TextToClipboard ( PCSZ );
static void	draw_ruler ( void );
static char*	xDateTimeFormat ( double x, char* b );
       void     gp_execute ();
static void     MousePosToGraphPos ( double *, double *,
			HWND, SHORT, SHORT, ULONG );
static void	apply_zoom ();
static void	do_zoom ( double xmin, double ymin, double xmax, double ymax );
static void	recalc_ruler_pos ();
#define IGNORE_MOUSE (!mouseTerminal || bUseMouse==FALSE || lock_mouse || !gp4mouse.graph)
  // don't react to mouse in the event handler
/* //PM: end of new functions related to the mouse processing */

/*==== c o d e ===============================================================*/

MRESULT EXPENTRY DisplayClientWndProc(HWND hWnd, ULONG message, MPARAM mp1, MPARAM mp2)
/*
**  Window proc for main window
**  -- passes most stuff to active child window via WmClientCmdProc
**  -- passes DDE messages to DDEProc
*/
{
    static BOOL    bSw = FALSE ;
    static BOOL    bFirst = TRUE ; 
    static RECTL   rectlPaint = { 0, 0, 0, 0 } ;
    static int     iPaintCount = 0 ;
    static int     firstcall = 1 ;
    

    if (message == WM_MOUSEMOVE && !IGNORE_MOUSE) {
      // track (show) mouse position---code shared with zooming
      SHORT mx = MOUSEMSG(&message)->x;  // macro which gets mouse coord.
      SHORT my = MOUSEMSG(&message)->y;

      HPS hps;
      POINTL pt;
      RECTL rc;
      double x,y;
      char s[256];

      /* Position of the "table" of values resulting from mouse movement (and clicks).
	 Negative y value would position it at the top of the window---not implemented.
      */
      static long xMouseTableOrig = 0, yMouseTableOrig = 0;

      MousePosToGraphPos( &x, &y, hWnd, mx, my, mouse_mode ); // transform mouse->real graph coords

      if (mouse_mode==MOUSE_COORDINATES_XDATE ||
	  mouse_mode==MOUSE_COORDINATES_XTIME ||
	  mouse_mode==MOUSE_COORDINATES_XDATETIME) { // time is on the x axis
	    char buf[100];
	    sprintf(s, "[%s, %g]", xDateTimeFormat(x,buf), y);
	    }
	else {
	    sprintf(s,"[%g,%g]",x,y); // default presentation of x,y values
	    }
      /* For negative table position, shift it to the top of the window.
      This could be done like that: */
      if (yMouseTableOrig<0) {
	WinQueryWindowRect(hWnd,&rc);
	yMouseTableOrig = rc.yTop - 17;
	}
      hps = WinGetPS(hWnd);
      GpiSetMix(hps, FM_OVERPAINT);
      // clear the rectangle below the text
      //  GpiSetColor( hps, CLR_CYAN );
      GpiSetColor( hps, CLR_BACKGROUND );
      pt.x = xMouseTableOrig;
      pt.y = yMouseTableOrig;
      GpiMove(hps,&pt);
      if (mouse_mode == MOUSE_COORDINATES_XDATETIME) pt.x += 30;
      if (!ruler.on) pt.x += 220; // "ad-hoc" length for clearing the line
	else { RECTL rc; // clear the whole line
	       GpiQueryPageViewport(hpsScreen,&rc);
	       pt.x = xMouseTableOrig + rc.xRight;
	     }
      pt.y = yMouseTableOrig + 16;
      GpiBox(hps, DRO_FILL, &pt, 0,0);
      // now write the mouse position on the screen
      GpiSetColor( hps, COLOR_MOUSE ); // set text color
      GpiSetCharMode(hps,CM_MODE1);

      pt.x = xMouseTableOrig + 2;
      pt.y = yMouseTableOrig + 2;

      if (ruler.on) { // append this info coming from the ruler position
	char p[255];
	if (mouse_mode != MOUSE_COORDINATES_REAL)
	    sprintf(p,"  ruler: [%g,%g]", ruler.x,ruler.y); // distance makes no sense
	  else {
	    double dx, dy;
	    if (gp4mouse.is_log_x) // ratio for log, distance for linear
		dx = (ruler.x==0) ? 99999 : x / ruler.x;
	      else dx = x - ruler.x;
	    if (gp4mouse.is_log_y) dy = (ruler.y==0) ? 99999 : y / ruler.y;
	      else dy = y - ruler.y;
	    if (mouse_mode==MOUSE_COORDINATES_REAL)
	    sprintf(p,"  ruler: [%g,%g]  distance: %g;%g",ruler.x,ruler.y,dx,dy);
	    // polar coords of distance (axes cannot be logarithmic)
	    if (bMousePolarDistance && !gp4mouse.is_log_x && !gp4mouse.is_log_y) {
	      double rho = sqrt( (x-ruler.x)*(x-ruler.x) + (y-ruler.y)*(y-ruler.y) );
	      double phi = (180/M_PI) * atan2(y-ruler.y,x-ruler.x);
	      strcat(s,p);
	      sprintf(p," (%g;%.4gø)",rho,phi);
	      }
	    }
	strcat(s,p);
	}

      GpiCharStringAt(hps,&pt,(long)strlen(s),s);
      WinReleasePS(hps);
    } // end of WM_MOUSEMOVE


#ifdef NEW_TRACK_ZOOM
    if (zooming)
    switch (message) { // messages accepted during zooming

	case WM_MOUSEMOVE: // track the zoom rectangle
	    {
	    HPS hps = WinGetPS(hWnd);
	    ZOOM_SETUP
	    GpiMove( hps, &zoomrect_from ) ;
	    GpiBox( hps, DRO_OUTLINE, &zoomrect_now, 0,0 );
	    zoomrect_now.x = MOUSEMSG(&message)->x;
	    zoomrect_now.y = MOUSEMSG(&message)->y;
	    GpiBox( hps, DRO_OUTLINE, &zoomrect_now, 0,0 );
	    WinReleasePS(hps);
	    }
	    return 0;

	case WM_BUTTON1CLICK: // finish zooming
	    {
	    HPS hps = WinGetPS(hWnd);
	    POINTL pt1, pt2;
	    double x1,y1,x2,y2; // result---request this zooming
	    zooming = 0;
	    ZOOM_SETUP
	    GpiMove( hps, &zoomrect_from ) ;
	    GpiBox( hps, DRO_OUTLINE, &zoomrect_now, 0,0 );
	    zoomrect_now.x = MOUSEMSG(&message)->x;
	    zoomrect_now.y = MOUSEMSG(&message)->y;
	    WinReleasePS(hps);
	    // Return if the selected area is too small (8 pixels for canceling)
	    if (abs(zoomrect_from.x - zoomrect_now.x) <= 8 ||
		abs(zoomrect_from.y - zoomrect_now.y) <= 8)
	      return 0L;
	    // pt1 and pt2 are the left lower and right upper corner, respectively
	    if (zoomrect_from.x < zoomrect_now.x) {
		pt1.x = zoomrect_from.x; pt2.x = zoomrect_now.x; }
	      else {
		pt2.x = zoomrect_from.x; pt1.x = zoomrect_now.x; }
	    if (zoomrect_from.y < zoomrect_now.y) {
		pt1.y = zoomrect_from.y; pt2.y = zoomrect_now.y; }
	      else {
		pt2.y = zoomrect_from.y; pt1.y = zoomrect_now.y; }
	    // Transform mouse->real graph coordinates
	    MousePosToGraphPos( &x1, &y1, hWnd, pt1.x,pt1.y, MOUSE_COORDINATES_REAL );
	    MousePosToGraphPos( &x2, &y2, hWnd, pt2.x,pt2.y, MOUSE_COORDINATES_REAL );
	    #if 0 // Just for testing: write it to clipboard
	    { char s[255];
	    sprintf(s,"zoom from [%li,%li]..[%li,%li] => [%g,%g]..[%g,%g]", pt1.x,pt1.y,pt2.x,pt2.y, x1,y1,x2,y2);
	    TextToClipboard ( s ); }
	    #endif
	    // This situation can be unzoomed
	    WinEnableMenuItem(
	      WinWindowFromID( WinQueryWindow( hApp, QW_PARENT ), FID_MENU ),
	      IDM_MOUSE_UNZOOM, TRUE ) ;
	    WinEnableMenuItem(
	      WinWindowFromID( WinQueryWindow( hApp, QW_PARENT ), FID_MENU ),
	      IDM_MOUSE_UNZOOMALL, TRUE ) ;
	    // Make zoom of the box [x1,y1]..[x2,y1]
	    do_zoom( x1, y1, x2, y2);
	    }
	    return 0;

	case WM_CHAR: // hitting ESC cancels zooming
	    if ( CHAR1FROMMP(mp2) == 0x1B ) {
		HPS hps = WinGetPS(hWnd);
		ZOOM_SETUP
		GpiMove( hps, &zoomrect_from ) ;
		GpiBox( hps, DRO_OUTLINE, &zoomrect_now, 0,0 );
		WinReleasePS(hps);
		zooming = 0;
	    }
	    return 0;

	default:
	    return (WinDefWindowProc(hWnd, message, mp1, mp2));
    } // zooming messages
#endif // NEW_TRACK_ZOOM


    switch (message) {

        case WM_CREATE:
            {
            HDC     hdcScreen ;
            SIZEL   sizlPage ;
                // set initial values
            ChangeCheck( hWnd, IDM_LINES_THICK, bWideLines?IDM_LINES_THICK:0 ) ;
            ChangeCheck( hWnd, IDM_LINES_SOLID, bLineTypes?0:IDM_LINES_SOLID ) ;
            ChangeCheck( hWnd, IDM_COLOURS, bColours?IDM_COLOURS:0 ) ;
            ChangeCheck( hWnd, IDM_FRONT, bPopFront?IDM_FRONT:0 ) ;
	    ChangeCheck( hWnd, IDM_KEEPRATIO, bKeepRatio?IDM_KEEPRATIO:0 ) ; //PM
	    ChangeCheck( hWnd, IDM_USEMOUSE, bUseMouse?IDM_USEMOUSE:0 ) ;    //PM
	    ChangeCheck( hWnd, IDM_MOUSE_POLAR_DISTANCE, bMousePolarDistance?IDM_MOUSE_POLAR_DISTANCE:0 ) ;    //PM
                // disable close from system menu (close only from gnuplot)
            hApp = WinQueryWindow( hWnd, QW_PARENT ) ; /* temporary assignment.. */
            hSysMenu = WinWindowFromID( hApp, FID_SYSMENU ) ;
                // setup semaphores
//            DosCreateEventSem( NULL, &semDrawDone, 0L, 0L ) ;
//            DosCreateEventSem( NULL, &semStartSeq, 0L, 0L ) ;
            DosCreateEventSem( NULL, &semPause, 0L, 0L ) ;
            DosCreateMutexSem( NULL, &semHpsAccess, 0L, 1L ) ;

            bExist = TRUE ;
                // create a dc and hps to draw on the screen
            hdcScreen = WinOpenWindowDC( hWnd ) ;
            sizlPage.cx = 0 ; sizlPage.cy = 0 ;
            sizlPage.cx = 19500 ; sizlPage.cy = 12500 ;
            hpsScreen = GpiCreatePS( hab, hdcScreen, &sizlPage,
                           PU_HIMETRIC|GPIT_NORMAL|GPIA_ASSOC) ;
                // spawn server for GNUPLOT ...
            tidSpawn = _beginthread( ReadGnu, NULL, 32768, NULL ) ;
		// initialize pointers
	    hptrDefault = WinQuerySysPointer( HWND_DESKTOP, SPTR_ARROW, FALSE );
	    hptrCrossHair = WinLoadPointer( HWND_DESKTOP, (ULONG)0, IDP_CROSSHAIR );
            }
            break ;
            
        case WM_GPSTART:

		//PM: show the Mouse menu if connected to mouseable PM terminal
	   if (1 || mouseTerminal) //PM workaround for a bug---SEE "BUGS 2" IN README!!!
//	   if (mouseTerminal)
	     WinEnableMenuItem(
	       WinWindowFromID( WinQueryWindow( hApp, QW_PARENT ), FID_MENU ),
	       IDM_MOUSE, TRUE ) ;
                // get details of command-line window
            hSwitch = WinQuerySwitchHandle( 0, ppidGnu ) ;
            WinQuerySwitchEntry( hSwitch, &swGnu ) ;
            if( firstcall ) {
                // set size of this window
            WinSetWindowPos( WinQueryWindow( hWnd, QW_PARENT ), 
                             bPopFront?HWND_TOP:swGnu.hwnd,
                             ulShellPos[0],
                             ulShellPos[1],
                             ulShellPos[2],
                             ulShellPos[3], 
                             bShellPos?(bPopFront?SWP_SIZE|SWP_MOVE|SWP_SHOW|SWP_ACTIVATE
                                                 :SWP_SIZE|SWP_MOVE|SWP_SHOW|SWP_ZORDER)
                                      :(bPopFront?SWP_SHOW|SWP_ACTIVATE:SWP_SHOW|SWP_ZORDER) ) ;
            signal( SIGTERM, SigHandler ) ;
                firstcall = 0 ;
                }
            if( !bPopFront ) WinSwitchToProgram( hSwitch ) ;
//            DosPostEventSem( semDrawDone ) ;
            DosReleaseMutexSem( semHpsAccess ) ;
            return 0 ;
            
        case WM_COMMAND:

            return WmClientCmdProc( hWnd , message , mp1 , mp2 ) ;

        case WM_CHAR:    
                    /* If the user types a command in the driver window,
                       we route it to the gnuplot window and switch that
                       to the front. Doesn't work for full-screen
                       sessions, though (but does switch).  */
            {
            USHORT  usFlag = SHORT1FROMMP( mp1 ) ;
            SHORT   key = SHORT1FROMMP( mp2 ) ;
	    // { FILE *ff = fopen( "deb", "a" ); fprintf( ff, "key = %i\n", (int)key ); fclose(ff); }
	    switch (key) { // remap keys (PM scancodes to getc(stdin) codes)
		case 18656: case 18432: key = 72*256; break; // <Up Arrow> (the 2nd code comes from the key on the numerical keypad)
		case 20704: case 20480: key = 80*256; break; // <Down Arrow>
		case 19424: case 19200: key = 75*256; break; // <Left Arrow>
		case 19936: case 19712: key = 77*256; break; // <Right Arrow>
		case 18400: key = 71*256; break; // <Home>
		case 20448: key = 79*256; break; // <End>
		default: if (key >= 256) key = -1; // don't pass other special keys
		}
            if( !(usFlag & (KC_KEYUP|KC_ALT|KC_CTRL)) && key > 0 ) {
                    HWND hw = WinQueryWindow( swGnu.hwnd, QW_BOTTOM ) ;
                    WinSetFocus( HWND_DESKTOP, hw ) ; 
                WinSendMsg( hw, message, 
                            MPFROM2SHORT((USHORT)(KC_SCANCODE), 1), 
			    MPFROMSHORT(key) ) ;
                    WinSwitchToProgram( hSwitch ) ;
                    }
            }
            break ;        

        case WM_DESTROY:

            if( WinSendMsg( hWnd, WM_USER_PRINT_QBUSY, 0L, 0L ) != 0L ) {
                WinMessageBox( HWND_DESKTOP,
                               hWnd, 
                               "Still printing - not closed",
                               APP_NAME,
                               0,
                               MB_OK | MB_ICONEXCLAMATION ) ;
                return 0L ;
                }
      	    return (WinDefWindowProc(hWnd, message, mp1, mp2));

        case WM_PAINT:
            {
            ULONG ulCount ;
            PID pid; TID tid;
            HPS hps_tmp;
            RECTL rectl_tmp;
              DosQueryMutexSem( semHpsAccess, &pid, &tid, &ulCount ) ;
            if (( ulCount > 0 ) && (tid != tidDraw)) {
                /* simple repaint while building plot or metafile */
                /* use temporary PS                   */
              lock_mouse = 1; //PM (will this help against occassional gnupmdrv crashes?)
              hps_tmp = WinBeginPaint(hWnd,0,&rectl_tmp );
              WinFillRect(hps_tmp,&rectl_tmp,CLR_BACKGROUND);
              WinEndPaint(hps_tmp);
                /* add dirty rectangle to saved rectangle     */
                /* to be repainted when PS is available again */
              WinUnionRect(hab,&rectlPaint,&rectl_tmp,&rectlPaint);
              lock_mouse = 0; //PM
                iPaintCount ++ ;
                break ;
                } 
            lock_mouse = 1; //PM
            WinInvalidateRect( hWnd, &rectlPaint, TRUE ) ;
            DoPaint( hWnd, hpsScreen ) ;
            WinSetRectEmpty( hab, &rectlPaint ) ;
            lock_mouse = 0; //PM
            }
            break ;     

        case WM_SIZE :
            
            {
            WinInvalidateRect( hWnd, NULL, TRUE ) ;
            }
            break ;

        case WM_PRESPARAMCHANGED:
            {        
            char *pp ;
            ULONG ulID ;
            pp = malloc(FONTBUF) ;
            if( WinQueryPresParam( hWnd, 
                                   PP_FONTNAMESIZE, 
                                   0, 
                                   &ulID, 
                                   FONTBUF, 
                                   pp, 
                                   QPF_NOINHERIT ) != 0L ) {
                strcpy( szFontNameSize, pp ) ;
                bNewFont = TRUE ;
                WinInvalidateRect( hWnd, NULL, TRUE ) ;
                }
            free(pp) ;
            } 
            break ;
            
        case WM_USER_PRINT_BEGIN:
        case WM_USER_PRINT_OK :
        case WM_USER_DEV_ERROR :
        case WM_USER_PRINT_ERROR :
        case WM_USER_PRINT_QBUSY :

            return( PrintCmdProc( hWnd, message, mp1, mp2 ) ) ;

        case WM_GNUPLOT:
                // display the plot         
	    lock_mouse = 1; //PM
	    if( bPopFront ) {
		SWP swp; // pop to front only if the window is not minimized
		if (  (WinQueryWindowPos( hwndFrame, (PSWP)&swp) == TRUE)
		   && ((swp.fl & SWP_MINIMIZE) == 0) )
                WinSetWindowPos( hwndFrame, HWND_TOP, 0,0,0,0, SWP_ACTIVATE|SWP_ZORDER ) ;
	    }
            if( iPaintCount > 0 ) { /* if outstanding paint messages, repaint */
                WinInvalidateRect( hWnd, &rectlPaint, TRUE ) ;
                iPaintCount = 0 ;
                }
	    lock_mouse = 0; //PM
            return 0L ;

        case WM_PAUSEPLOT:
	    {
	    SWP swp; // restore the window if it has been minimized
	    if( (WinQueryWindowPos( hwndFrame, &swp) == TRUE)
		&& ((swp.fl & SWP_MINIMIZE) != 0) )
		WinSetWindowPos( hwndFrame, HWND_TOP, 0,0,0,0, SWP_RESTORE|SWP_SHOW|SWP_ACTIVATE ) ;
	    }
                /* put pause message on screen, or enable 'continue' button */
            if( ulPauseMode == PAUSE_DLG ) {
                pausedata.pszMessage = (char*)mp1 ;
                WinLoadDlg( HWND_DESKTOP,
                            hWnd,
                            (PFNWP)PauseMsgDlgProc,
                            0L,
                            IDD_PAUSEBOX,
                            &pausedata ) ; 
                }
            else { 
                WinEnableMenuItem( WinWindowFromID( 
                                   WinQueryWindow( hWnd, QW_PARENT ), FID_MENU ),
                                   IDM_CONTINUE,
                                   TRUE ) ;            
                }
            return 0L ;

        case WM_PAUSEEND:
            /* resume plotting */
            ulPauseReply = (ULONG) mp1 ;
            DosPostEventSem( semPause ) ;
            return 0L ;
    
/* //PM: new event handles for mouse processing */

case WM_MOUSEMOVE:
  if (IGNORE_MOUSE) {
    WinSetPointer( HWND_DESKTOP, hptrDefault ); // set default pointer
    return 0L;
    }
  WinSetPointer( HWND_DESKTOP, hptrCrossHair );
  return  0L;

case WM_BUTTON1DBLCLK:
  if (IGNORE_MOUSE) return 0L;
  // put the mouse coordinates to the clipboard
  {
  SHORT mx = MOUSEMSG(&message)->x;
  SHORT my = MOUSEMSG(&message)->y;
  double x,y;
  char s[256];
  int frm = ulMouseSprintfFormatItem-IDM_MOUSE_FORMAT_X_Y;
  if (frm<0 || frm>=nMouseSprintfFormats) frm=1;
  MousePosToGraphPos( &x, &y, hWnd, mx, my, mouse_mode ); // transform mouse->real graph coords
  sprintf( s, MouseSprintfFormats[frm], x,y );
  TextToClipboard ( s );
  /*
  Note:
  Another solution of getting mouse position (available at any
  method, not just in this handle event) is the following one:
  ok = WinQueryPointerPos(HWND_DESKTOP, &pt); // pt contains pos wrt desktop
  WinMapWindowPoints(HWND_DESKTOP, hWnd, &pt, 1); // pt contains pos wrt our hwnd window
  sprintf(s,"[%li,%li]",pt.x,pt.y);
  */

  }
  return 0L; // end of case WM_BUTTON1DBLCLK


case WM_BUTTON2CLICK: // WM_BUTTON2UP:
  if (IGNORE_MOUSE) return 0L;
  // make zoom
  {
#ifdef NEW_TRACK_ZOOM

  POINTL tmp;
  HPS hps = WinGetPS(hWnd);
  if (pausing) { // zoom is not allowed during pause
      DosBeep(440,111); break;
  }
  zoomrect_from.x = MOUSEMSG(&message)->x;
  zoomrect_from.y = MOUSEMSG(&message)->y;
  tmp.x = zoomrect_now.x = zoomrect_from.x + 50; // set opposite corner
  tmp.y = zoomrect_now.y = zoomrect_from.y + 50;
  ZOOM_SETUP
  GpiMove( hps, &zoomrect_from ) ;
  GpiBox(hps, DRO_OUTLINE, &zoomrect_now, 0,0 );
  WinMapWindowPoints( hWnd, HWND_DESKTOP, &tmp, 1); // move mouse to opposite corner
  WinSetPointerPos( HWND_DESKTOP, tmp.x, tmp.y);
  WinReleasePS(hps);
  zooming = 1;

#else

  TRACKINFO ti;
  // char s[256];
  double x1,y1,x2,y2; // result---request this zooming

  if (pausing) { // zoom is not allowed during pause
      DosBeep(440,111); break;
  }

  ti.cxBorder = 1;        // set border width
  ti.cyBorder = 1;
  ti.cxGrid = 0;          // set tracking grid
  ti.cyGrid = 0;
  ti.cxKeyboard = 4;      // set keyboard increment
  ti.cyKeyboard = 4;

  // set screen boundaries for tracking
  WinQueryWindowRect(hWnd,&ti.rclBoundary);

  // set minimum/maximum size for window = size of the present window
  ti.ptlMinTrackSize.x = ti.rclBoundary.xLeft;
  ti.ptlMinTrackSize.y = ti.rclBoundary.yBottom;
  ti.ptlMaxTrackSize.x = ti.rclBoundary.xRight;
  ti.ptlMaxTrackSize.y = ti.rclBoundary.yTop;

  // set initial initial tracking (zoomed) window
  ti.rclTrack.xRight = 50 + (ti.rclTrack.xLeft=MOUSEMSG(&message)->x);
  ti.rclTrack.yTop = 50 + (ti.rclTrack.yBottom=MOUSEMSG(&message)->y);

  // track the zooming rectangle, return when mouse is lifted
  ti.fs = TF_TOP | TF_RIGHT | TF_SETPOINTERPOS;
  if (!WinTrackRect(hWnd,NULLHANDLE,&ti)) break;

  // return if the selected area is too small (8 pixels for canceling)
  if (abs(ti.rclTrack.xRight-ti.rclTrack.xLeft)<=8 ||
      abs(ti.rclTrack.yTop-ti.rclTrack.yBottom)<=8)
      return 0L;

  // transform mouse->real graph coordinates
  MousePosToGraphPos( &x1, &y1, hWnd, ti.rclTrack.xLeft,ti.rclTrack.yBottom, MOUSE_COORDINATES_REAL );
  MousePosToGraphPos( &x2, &y2, hWnd, ti.rclTrack.xRight,ti.rclTrack.yTop, MOUSE_COORDINATES_REAL );

  // Just for testing: write it to clipboard
  // sprintf(s,"zoom from [%li,%li]..[%li,%li] => [%g,%g]..[%g,%g]",ti.rclTrack.xLeft,ti.rclTrack.yBottom,ti.rclTrack.xRight,ti.rclTrack.yTop,x1,y1,x2,y2);
  // TextToClipboard ( s );

  // Make an action with the resulting [x1,y1]..[x2,y1] zoom box.

  // this situation can be unzoomed
  WinEnableMenuItem(
    WinWindowFromID( WinQueryWindow( hApp, QW_PARENT ), FID_MENU ),
    IDM_MOUSE_UNZOOM, TRUE ) ;
  WinEnableMenuItem(
    WinWindowFromID( WinQueryWindow( hApp, QW_PARENT ), FID_MENU ),
    IDM_MOUSE_UNZOOMALL, TRUE ) ;

  do_zoom( x1, y1, x2, y2);

#endif // NEW_TRACK_ZOOM
  }
  return 0; // return from: case WM_BUTTON3DOWN (zoom)


case WM_BUTTON3UP: // WM_BUTTON3DBLCLK:
  // write mouse position to screen
  if (IGNORE_MOUSE) return 0L;
  {
  SHORT mx=MOUSEMSG(&message)->x; // mouse position
  SHORT my=MOUSEMSG(&message)->y;
  char s[256];
  POINTL pt;
  double x,y;
  HPS hps = WinGetPS(hWnd);

  MousePosToGraphPos( &x, &y, hWnd, mx, my, mouse_mode ); // transform mouse->real graph coords

  if (mouse_mode==MOUSE_COORDINATES_XDATE ||
      mouse_mode==MOUSE_COORDINATES_XTIME ||
      mouse_mode==MOUSE_COORDINATES_XDATETIME) { // time is on the x axis
	char buf[100];
	sprintf(s, "[%s, %g]", xDateTimeFormat(x,buf), y);
	}
    else {
        sprintf(s,"[%g,%g]",x,y); // usual x,y values
	}

  GpiSetColor( hps, COLOR_ANNOTATE );    // set color of the text
  GpiSetCharMode(hps,CM_MODE1);
  pt.x = mx; pt.y = my;
  GpiCharStringAt(hps,&pt,(long)strlen(s),s);
  WinReleasePS(hps);
  }
  return 0L; // end of case WM_BUTTON3...


  //PM: end of new event handles for mouse processing

	default:         /* Passes it on if unproccessed    */
	    return (WinDefWindowProc(hWnd, message, mp1, mp2));
    }
    return (NULL);
}

MRESULT WmClientCmdProc(HWND hWnd, ULONG message, MPARAM mp1, MPARAM mp2)
/*
**   Handle client window command (menu) messages
*/
    {
    extern HWND hApp ;
    static int ulPauseItem = IDM_PAUSEDLG ;
    static int ulMouseCoordItem = IDM_MOUSE_COORDINATES_REAL ;

    switch( (USHORT) SHORT1FROMMP( mp1 ) ) {
                
        case IDM_ABOUT :    /* show the 'About' box */
             
            WinDlgBox( HWND_DESKTOP,
                       hWnd , 
                       (PFNWP)About ,
                       0L,
                       ID_ABOUT, 
                       NULL ) ;
            break ;


        case IDM_GPLOTINF:  /* view gnuplot.inf */
            {
	    /* should be bigger or dynamic */
            char path[256] ;
	    char *p;
            strcpy( path, "start view " ) ;
	    if( (p=getenv("GNUPLOT")) != NULL ) {
		strcat( path, p ) ;
                strcat( path, "/" ) ;
                }    
            strcat( path, "gnuplot" ) ;
            system( path ) ;
            }
            break ;
            
        case IDM_PRINT :    /* print plot */

            if( SetupPrinter( hWnd, &qPrintData ) ) {
                            WinPostMsg( hWnd, 
                            WM_USER_PRINT_BEGIN, 
                            (MPARAM) &qPrintData, 
                            (MPARAM) hpsScreen ) ; 
                            }
            break ;

        case IDM_PRINTSETUP :    /* select printer */

            WinDlgBox( HWND_DESKTOP,
                       hWnd ,
                       (PFNWP)QPrintersDlgProc,
                       0L,
                       IDD_QUERYPRINT, 
                       qPrintData.szPrinterName ) ;
            break ;

        case IDM_LINES_THICK:
                // change line setting
            bWideLines = !bWideLines ;
            ChangeCheck( hWnd, IDM_LINES_THICK, bWideLines?IDM_LINES_THICK:0 ) ;
            WinInvalidateRect( hWnd, NULL, TRUE ) ;
            break ;

        case IDM_LINES_SOLID:
                // change line setting
            bLineTypes = !bLineTypes ;
            ChangeCheck( hWnd, IDM_LINES_SOLID, bLineTypes?0:IDM_LINES_SOLID ) ;
            EditLineTypes( hWnd, hpsScreen, bLineTypes ) ;
            WinInvalidateRect( hWnd, NULL, TRUE ) ;
            break ;

        case IDM_COLOURS:
                // change colour setting
            bColours = !bColours ;        
            ChangeCheck( hWnd, IDM_COLOURS, bColours?IDM_COLOURS:0 ) ;
            WinInvalidateRect( hWnd, NULL, TRUE ) ;
            break ;

        case IDM_FRONT:
                /* toggle z-order forcing */
            bPopFront = !bPopFront ;        
            ChangeCheck( hWnd, IDM_FRONT, bPopFront?IDM_FRONT:0 ) ;
            break ;

        case IDM_KEEPRATIO: //PM
		/* toggle keep aspect ratio */
	    bKeepRatio = !bKeepRatio ;
	    ChangeCheck( hWnd, IDM_KEEPRATIO, bKeepRatio?IDM_KEEPRATIO:0 ) ;
	    WinInvalidateRect( hWnd, NULL, TRUE ) ; // redraw screen
	    break ;

        case IDM_FONTS:
        
            if( GetNewFont( hWnd, hpsScreen ) ) {
                bNewFont = TRUE ;
                WinInvalidateRect( hWnd, NULL, TRUE ) ;
                }
            break ;
            
        case IDM_SAVE :
            SaveIni( hWnd ) ;
            break ;
            
        case IDM_COPY :         /* copy to clipboard */
            if( WinOpenClipbrd( hab ) ) {
                CopyToClipBrd( hWnd ) ; 
                }
            else {
                WinMessageBox( HWND_DESKTOP,
                               hWnd, 
                               "Can't open clipboard",
                               APP_NAME,
                               0,
                               MB_OK | MB_ICONEXCLAMATION ) ;
                }
            break ;

        case IDM_CLEARCLIP :         /* clear clipboard */
            if( WinOpenClipbrd( hab ) ) {
                WinEmptyClipbrd( hab ) ;
                WinCloseClipbrd( hab ) ;
                }
            else {
                WinMessageBox( HWND_DESKTOP,
                               hWnd, 
                               "Can't open clipboard",
                               APP_NAME,
                               0,
                               MB_OK | MB_ICONEXCLAMATION ) ;
                }
            break ;

        case IDM_COMMAND:       /* go back to GNUPLOT command window */
            WinSwitchToProgram( hSwitch ) ;
            break ;

        case IDM_CONTINUE:
            WinPostMsg( hWnd, WM_PAUSEEND, (MPARAM)1L, (MPARAM)0L ) ; 
            WinEnableMenuItem( WinWindowFromID( 
                               WinQueryWindow( hWnd, QW_PARENT ), FID_MENU ),
                               IDM_CONTINUE,
                               FALSE ) ;            
            break ;
            
        case IDM_PAUSEGNU:  /* gnuplot handles pause */
            ChangeCheck( hWnd, ulPauseItem, IDM_PAUSEGNU ) ;
            ulPauseItem = IDM_PAUSEGNU ;
            ulPauseMode = PAUSE_GNU ;
            break ;
            
        case IDM_PAUSEDLG:  /* pause message in dlg box */
            ChangeCheck( hWnd, ulPauseItem, IDM_PAUSEDLG ) ;
            ulPauseItem = IDM_PAUSEDLG ;
            ulPauseMode = PAUSE_DLG ;
            break ;
            
        case IDM_PAUSEBTN:  /* pause uses menu button, no message */
            ChangeCheck( hWnd, ulPauseItem, IDM_PAUSEBTN ) ;
            ulPauseItem = IDM_PAUSEBTN ;
            ulPauseMode = PAUSE_BTN ;
            break ;
          
        case IDM_HELPFORHELP:
            WinSendMsg(WinQueryHelpInstance(hWnd),
                       HM_DISPLAY_HELP, 0L, 0L ) ;
            return 0L ;

        case IDM_EXTENDEDHELP:
            WinSendMsg(WinQueryHelpInstance(hWnd),
                        HM_EXT_HELP, 0L, 0L);
            return 0L ;

        case IDM_KEYSHELP:
            WinSendMsg(WinQueryHelpInstance(hWnd),
                       HM_KEYS_HELP, 0L, 0L);
            return 0L ;

        case IDM_HELPINDEX:
            WinSendMsg(WinQueryHelpInstance(hWnd),
                       HM_HELP_INDEX, 0L, 0L);
            return 0L ;


	//PM Now new mousing stuff:

	case IDM_USEMOUSE: // toggle using/not using mouse cursor tracking
	    bUseMouse = !bUseMouse ;
	    ChangeCheck( hWnd, IDM_USEMOUSE, bUseMouse?IDM_USEMOUSE:0 ) ;
	    if(!bUseMouse) // redraw screen
	      WinInvalidateRect( hWnd, NULL, TRUE ) ;
	    return 0L;

	case IDM_MOUSE_COORDINATES_REAL:
	    ChangeCheck( hWnd, ulMouseCoordItem, IDM_MOUSE_COORDINATES_REAL ) ;
	    ulMouseCoordItem = IDM_MOUSE_COORDINATES_REAL;
	    mouse_mode = MOUSE_COORDINATES_REAL;
	    return 0L;

	case IDM_MOUSE_COORDINATES_PIXELS:
	    ChangeCheck( hWnd, ulMouseCoordItem, IDM_MOUSE_COORDINATES_PIXELS ) ;
	    ulMouseCoordItem = IDM_MOUSE_COORDINATES_PIXELS;
	    mouse_mode = MOUSE_COORDINATES_PIXELS;
	    return 0L;

	case IDM_MOUSE_COORDINATES_SCREEN:
	    ChangeCheck( hWnd, ulMouseCoordItem, IDM_MOUSE_COORDINATES_SCREEN ) ;
	    ulMouseCoordItem = IDM_MOUSE_COORDINATES_SCREEN;
	    mouse_mode = MOUSE_COORDINATES_SCREEN;
	    return 0L;

	case IDM_MOUSE_COORDINATES_XDATE:
	    ChangeCheck( hWnd, ulMouseCoordItem, IDM_MOUSE_COORDINATES_XDATE ) ;
	    ulMouseCoordItem = IDM_MOUSE_COORDINATES_XDATE;
	    mouse_mode = MOUSE_COORDINATES_XDATE;
	    return 0L;

	case IDM_MOUSE_COORDINATES_XTIME:
	    ChangeCheck( hWnd, ulMouseCoordItem, IDM_MOUSE_COORDINATES_XTIME ) ;
	    ulMouseCoordItem = IDM_MOUSE_COORDINATES_XTIME;
	    mouse_mode = MOUSE_COORDINATES_XTIME;
	    return 0L;

	case IDM_MOUSE_COORDINATES_XDATETIME:
	    ChangeCheck( hWnd, ulMouseCoordItem, IDM_MOUSE_COORDINATES_XDATETIME ) ;
	    ulMouseCoordItem = IDM_MOUSE_COORDINATES_XDATETIME;
	    mouse_mode = MOUSE_COORDINATES_XDATETIME;
	    return 0L;

        case IDM_MOUSE_CMDS2CLIP:
		// toggle copying the command sent to gnuplot to clipboard
	    bSend2gp = !bSend2gp ;
	    ChangeCheck( hWnd, IDM_MOUSE_CMDS2CLIP, bSend2gp?IDM_MOUSE_CMDS2CLIP:0 ) ;
	    break ;

	case IDM_MOUSE_FORMAT_X_Y:
	case IDM_MOUSE_FORMAT_XcY:
	case IDM_MOUSE_FORMAT_XsY:
	case IDM_MOUSE_FORMAT_XcYc:
	case IDM_MOUSE_FORMAT_XcYs:
	case IDM_MOUSE_FORMAT_pXdYp:
	case IDM_MOUSE_FORMAT_pXcYp:
	case IDM_MOUSE_FORMAT_pXsYp:
	case IDM_MOUSE_FORMAT_LABEL:
	    ChangeCheck( hWnd, ulMouseSprintfFormatItem, SHORT1FROMMP( mp1 ) ) ;
	    ulMouseSprintfFormatItem = SHORT1FROMMP( mp1 );
	    return 0L;

	case IDM_MOUSE_POLAR_DISTANCE: // toggle using/not using polar coords of distance
	    bMousePolarDistance = !bMousePolarDistance ;
	    ChangeCheck( hWnd, IDM_MOUSE_POLAR_DISTANCE, bMousePolarDistance?IDM_MOUSE_POLAR_DISTANCE:0 ) ;
	    return 0L;

	case IDM_MOUSE_ZOOMNEXT: // zoom to next level
	    {
	    char s[255];
	    if (zoom_now == NULL || zoom_now->next == NULL)
		DosBeep(440,111);
	    else
		apply_zoom( zoom_now->next );
	    }
	    return 0L;

	case IDM_MOUSE_UNZOOM: // unzoom one level back
	    {
	    char s[255];
	    if (zoom_now == NULL || zoom_now->prev == NULL)
		DosBeep(440,111);
	    else
		apply_zoom( zoom_now->prev );
	    }
	    return 0L;

	case IDM_MOUSE_UNZOOMALL: // unzoom to the first level
	    {
	    char s[255];
	    WinEnableMenuItem( // this situation cannot be unzoomed
	      WinWindowFromID( WinQueryWindow( hApp, QW_PARENT ), FID_MENU ),
	      IDM_MOUSE_UNZOOM, FALSE ) ;
	    if (zoom_head == NULL || zoom_now == zoom_head)
		DosBeep(440,111);
	    else
		apply_zoom( zoom_head );
	    }
	    return 0L;

	case IDM_MOUSE_RULER:
	    ruler.on = ! ruler.on;
	    ChangeCheck( hWnd, IDM_MOUSE_RULER, ruler.on?IDM_MOUSE_RULER:0 ) ;
	    if (ruler.on) {
		// remember the ruler position
		POINTL p;
		WinQueryPointerPos(HWND_DESKTOP, &p);
		    // this is position wrt desktop
		WinMapWindowPoints(HWND_DESKTOP, hWnd, &p, 1);
		    // pos. wrt our window in pixels
		MousePosToGraphPos( &ruler.x, &ruler.y, hWnd, // mouse -> screen relative
		  p.x, p.y, MOUSE_COORDINATES_SCREEN );
		ruler.px = ruler.x * 19500; // screen relative -> viewport coordinates
		ruler.py = ruler.y * 12500;
		MousePosToGraphPos( &ruler.x, &ruler.y, hWnd, // mouse->real graph coordinates
		  p.x, p.y, MOUSE_COORDINATES_REAL );
		// { char s[255];
		// sprintf(s,"ruler at %i,%i",ruler.px,ruler.py);
		// sprintf(s,"ruler at %g,%g",ruler.x,ruler.y);
		// TextToClipboard(s); }
		}
	    WinInvalidateRect( hWnd, NULL, TRUE ) ; // redraw screen
	    return 0L;

        case IDM_BREAK_DRAWING:
	    breakDrawing = 1;
            return 0L;

	case IDM_SET_GRID:
            if (bhave_grid) { //fraba
		bhave_grid = FALSE;
		strcpy(input_from_PM_Terminal,"set nogrid; replot");
            } else {
		bhave_grid = TRUE;
		#if 0 /* 0 ... main grid, 1 ... fine grid */
		strcpy(input_from_PM_Terminal,"set grid; replot");
		#else
		strcpy(input_from_PM_Terminal,"set mxtics 2; set mytics 2; set grid x y mx my; replot");
		#endif
            }
	    gp_execute();
	    return 0L;

	case IDM_SET_LINLOGY:
	    if (bhave_log) { //fraba
	       bhave_log = FALSE;
	       strcpy(input_from_PM_Terminal, "set nolog y; replot" );
	       gp_execute();
	    } else {
	       #if 0 // take care of positive y min
	       if (gp4mouse.ymin <= 0 || gp4mouse.ymax <= 0) {
		 // strcpy(input_from_PM_Terminal, "# Hey! y range must be greater than 0 for log scale.");
		 // gp_execute();
		 DosBeep(300,10);
		 return 0L;
	       }
	       #endif
	       bhave_log = TRUE;
	       strcpy(input_from_PM_Terminal, "set log y; replot" );
	       gp_execute();
            } /* endif */
	    return 0L;

	case IDM_SET_AUTOSCALE:
            strcpy(input_from_PM_Terminal,"set autoscale; replot");
	    gp_execute();
	    return 0L;

	case IDM_DO_REPLOT:
            strcpy(input_from_PM_Terminal,"replot");
	    gp_execute();
	    return 0L;

	case IDM_DO_RELOAD:
            strcpy(input_from_PM_Terminal,"history !load");
	    gp_execute();
	    return 0L;

        case IDM_DO_SENDCOMMAND:
	  if (pausing) DosBeep(440,111);
	    else WinDlgBox (HWND_DESKTOP, hWnd, SendCommandDlgProc,
			    NULLHANDLE, IDM_DO_SENDCOMMAND, input_from_PM_Terminal);
          return (MRESULT)0;

	case IDM_SET_D_S_BOXES:
	case IDM_SET_D_S_DOTS:
	case IDM_SET_D_S_FSTEPS:
	case IDM_SET_D_S_HISTEPS:
	case IDM_SET_D_S_IMPULSES:
	case IDM_SET_D_S_LINES:
	case IDM_SET_D_S_LINESPOINTS:
	case IDM_SET_D_S_POINTS:
	case IDM_SET_D_S_STEPS:
            sprintf(input_from_PM_Terminal, "set data style %s; replot",
		SetDataStyles[ (USHORT) SHORT1FROMMP( mp1 ) - IDM_SET_D_S_BOXES ] );
	    gp_execute();
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
            sprintf(input_from_PM_Terminal, "set function style %s; replot",
		SetDataStyles[ (USHORT) SHORT1FROMMP( mp1 ) - IDM_SET_F_S_BOXES ] );
	    gp_execute();
	    return 0L;

        default : 
    
            return WinDefWindowProc( hWnd, message, mp1, mp2 ) ;

        }  
    return( NULL ) ;
    }                      

void ChangeCheck( HWND hWnd , USHORT wItem1 , USHORT wItem2 )
/*
**  Utility function:
**
**  move check mark from menu item 1 to item 2
*/
    {
    HWND hMenu ;

    hMenu = WinWindowFromID( WinQueryWindow( hWnd, QW_PARENT ), 
                             FID_MENU ) ;            
    if( wItem1 != 0 )
        WinSendMsg( hMenu,
                    MM_SETITEMATTR,
                    MPFROM2SHORT( wItem1, TRUE ),
                    MPFROM2SHORT( MIA_CHECKED, 0 ) ) ;
    if( wItem2 != 0 )
        WinSendMsg( hMenu,
                    MM_SETITEMATTR,
                    MPFROM2SHORT( wItem2, TRUE ),
                    MPFROM2SHORT( MIA_CHECKED, MIA_CHECKED ) ) ;
    }       

static void CopyToClipBrd( HWND hWnd ) 
/*
**  Copy window to clipboard as bitmap.
*/
    {
    HAB hab ;
    HBITMAP hbm ;
    HMF     hmf ;

    hab = WinQueryAnchorBlock( hWnd ) ;
    WinEmptyClipbrd( hab ) ;
    hbm = CopyToBitmap( hpsScreen ) ; 
    WinSetClipbrdData( hab, (ULONG) hbm, CF_BITMAP, CFI_HANDLE) ;
    hmf = CopyToMetaFile( hpsScreen ) ; 
    WinSetClipbrdData( hab, (ULONG) hmf, CF_METAFILE, CFI_HANDLE) ;
    WinCloseClipbrd( hab ) ;
    }

HBITMAP CopyToBitmap( HPS hps ) 
/*
**  Copy ps to a bitmap.
*/
    {
    HPS     hpsMem ;
    HWND    hwnd ;
    HAB     hab ;
    PSZ     psz[4] = {NULL,"Display",NULL,NULL} ;
    HDC     hdcMem, hdcScr ;
    SIZEL   sizel ;
    BITMAPINFOHEADER2 bmp ;
    PBITMAPINFO2 pbmi ;
    HBITMAP hbm ;
    BYTE    abBmp[80] ;
    LONG    alData[2] ;
    RECTL   rectl ;
    POINTL  aptl[6] ;
    HMF     hmf ;

    hdcScr = GpiQueryDevice( hps ) ;
    hwnd = WinWindowFromDC( hdcScr ) ;
    hab = WinQueryAnchorBlock( hwnd ) ;
    hdcMem = DevOpenDC( hab, 
                        OD_MEMORY, 
                        "*", 
                        4L, 
                        (PDEVOPENDATA) psz, 
                        hdcScr ) ;
    sizel.cx = 0/*GNUPAGE*/ ; sizel.cy = 0/*GNUPAGE*/ ;
    hpsMem = GpiCreatePS( hab, hdcMem, &sizel, PU_PELS|GPIA_ASSOC|GPIT_MICRO ) ;
    GpiQueryDeviceBitmapFormats( hpsMem, 2L, alData ) ;
    WinQueryWindowRect( hwnd, &rectl ) ;
    memset( &bmp, 0, sizeof(bmp) ) ;
    bmp.cbFix = (ULONG) sizeof( bmp ) ;
    bmp.cx = (SHORT) (rectl.xRight-rectl.xLeft) ;
    bmp.cy = (SHORT) (rectl.yTop-rectl.yBottom) ;
    bmp.cPlanes = alData[0] ;
    bmp.cBitCount = alData[1] ;
    hbm = GpiCreateBitmap( hpsMem, &bmp, 0, NULL, NULL ) ;
    GpiSetBitmap( hpsMem, hbm ) ;
    aptl[0].x = 0 ; aptl[0].y = 0 ;
    aptl[1].x = (LONG)bmp.cx ; aptl[1].y = (LONG)bmp.cy ;
    aptl[2].x = 0 ; aptl[2].y = 0 ;
    GpiBitBlt( hpsMem, hps, 3L, aptl, ROP_SRCCOPY, BBO_IGNORE ) ;
    GpiDestroyPS( hpsMem ) ;
    DevCloseDC( hdcMem ) ;
    return hbm ;
    }

HMF CopyToMetaFile( HPS hps ) 
/*
**  Copy ps to a metafile.
*/
    {
    HDC hdcMF, hdcOld ;
    HAB hab ;
    HWND hwnd ;    
    PSZ psz[4] = {NULL,"Display",NULL,NULL} ;
    HMF hmf ;
    hdcOld = GpiQueryDevice( hps ) ;
    hwnd = WinWindowFromDC( hdcOld ) ;
    hab = WinQueryAnchorBlock( hwnd ) ;
    hdcMF = DevOpenDC( hab, OD_METAFILE, "*", 4L, psz, hdcOld ) ;
    DosRequestMutexSem( semHpsAccess, (ULONG) SEM_INDEFINITE_WAIT ) ;
    GpiSetDrawingMode( hps, DM_DRAW ) ; 
    GpiAssociate( hps, 0 ) ; 
    GpiAssociate( hps, hdcMF ) ;
    ScalePS( hps ) ;    
    GpiDrawChain( hps ) ;
    GpiAssociate( hps, 0 ) ; 
    GpiAssociate( hps, hdcOld ) ; 
    DosReleaseMutexSem( semHpsAccess ) ;
    hmf = DevCloseDC( hdcMF ) ;
    return hmf ;
    }

BOOL QueryIni( HAB hab )
/*
** Query INI file
*/
    {
    BOOL         bPos, bData, bSwp  ;
    ULONG        ulOpts[5] ;
    HINI         hini ;
    ULONG        ulCB ;
    char         *p ;
    static SWP   pauseswp ;
    
            // read gnuplot ini file

    hini = PrfOpenProfile( hab, szIniFile ) ;
    ulCB = sizeof( ulShellPos ) ;
    bPos = PrfQueryProfileData( hini, APP_NAME, INISHELLPOS, &ulShellPos, &ulCB ) ;
    ulCB = sizeof( SWP ) ;
    bSwp = PrfQueryProfileData( hini, APP_NAME, INIPAUSEPOS, &pauseswp, &ulCB ) ;
    if( bSwp ) pausedata.pswp = &pauseswp ;
    ulCB = sizeof( ulOpts ) ;
    bData = PrfQueryProfileData( hini, APP_NAME, INIOPTS, &ulOpts, &ulCB ) ;
    if( bData ) {
        bLineTypes = (BOOL)ulOpts[0] ;
        bWideLines = (BOOL)ulOpts[1] ;
        bColours = (BOOL)ulOpts[2] ;
        ulPauseMode = ulOpts[3] ;
        bPopFront = (BOOL)ulOpts[4] ;
        }
    else {
        bLineTypes = FALSE ;  /* default values */
     /*   bWideLines = FALSE ; */
        bColours = TRUE ;
        bPopFront = TRUE ;
        ulPauseMode = 1 ;
        }
    ulCB = 4*sizeof(float) ;
    PrfQueryProfileData( hini, APP_NAME, INIFRAC, &qPrintData.xsize, &ulCB ) ;
    if( PrfQueryProfileSize( hini, APP_NAME, INIPRDRIV, &ulCB ) ) {
        PDRIVDATA pdriv = (PDRIVDATA) malloc( ulCB ) ;
        if( pdriv != NULL ) {
            PrfQueryProfileData( hini, APP_NAME, INIPRDRIV, pdriv, &ulCB ) ;
            qPrintData.pdriv = pdriv ;
            qPrintData.cbpdriv = ulCB ;
            }
        }
    PrfQueryProfileString( hini, APP_NAME, INIPRPR, "", 
                           qPrintData.szPrinterName,
                           (long) sizeof qPrintData.szPrinterName ) ;
    PrfQueryProfileString( hini, APP_NAME, INIFONT, INITIAL_FONT, 
                             szFontNameSize, FONTBUF ) ;
    ulCB = sizeof( ulOpts ) ;
    bData = PrfQueryProfileData( hini, APP_NAME, INICHAR, &ulOpts, &ulCB ) ;
    if( bData ) {
        lCharWidth = ulOpts[0] ;
        lCharHeight = ulOpts[1] ;
        }
    else {
        lCharWidth = 217 ;
        lCharHeight = 465 ;
        }
    ulCB = sizeof( bKeepRatio ) ; //PM --- keep ratio
    bData = PrfQueryProfileData( hini, APP_NAME, INIKEEPRATIO, &ulOpts, &ulCB ) ;
    if( bData ) bKeepRatio = (BOOL)ulOpts[0] ;
    //PM Mousing:
    /* ignore reading "Use mouse" --- no good idea to have mouse on by default.
       Maybe it was the reason of some crashes (mouse init before draw).
    ulCB = sizeof( bUseMouse ) ;
    bData = PrfQueryProfileData( hini, APP_NAME, INIUSEMOUSE, &ulOpts, &ulCB ) ;
    if( bData ) bUseMouse = (BOOL)ulOpts[0] ;
    */
    /* ignore reading mouse cursor (real, relative or pixels).
       Reason/bug: it does not switch the check mark in the menu, even
       though it works as expected.
    ulCB = sizeof( mouse_mode ) ;
    bData = PrfQueryProfileData( hini, APP_NAME, INIMOUSECOORD, &ulOpts, &ulCB ) ;
    if( bData ) mouse_mode = (ULONG)ulOpts[0] ;
    */

    PrfCloseProfile( hini ) ;

    if( qPrintData.szPrinterName[0] == '\0' ) {
            // get default printer name    
        PrfQueryProfileString( HINI_PROFILE,
                               "PM_SPOOLER",
                               "PRINTER",
                               ";",
                               qPrintData.szPrinterName,
                               (long) sizeof qPrintData.szPrinterName ) ;
        if( (p=strchr( qPrintData.szPrinterName, ';' )) != NULL ) *p = '\0' ;
        }
    bShellPos = bPos ;
    return bPos ;
    }

static void SaveIni( HWND hWnd )
/*
** save data in ini file
*/
    {
    SWP     swp ;
    HINI    hini ;
    ULONG   ulOpts[5] ;
    HFILE   hfile ;
    ULONG   ulAct ;
    ERRORID errid ;
    char    achErr[64] ;
    HAB     hab ;

    hab = WinQueryAnchorBlock( hWnd ) ;    
    hini = PrfOpenProfile( hab, szIniFile ) ;
    if( hini != NULLHANDLE ) {
        WinQueryWindowPos( hwndFrame, &swp ) ;
        ulPlotPos[0] = swp.x ;
        ulPlotPos[1] = swp.y ;
        ulPlotPos[2] = swp.cx ;
        ulPlotPos[3] = swp.cy ;
        PrfWriteProfileData( hini, APP_NAME, INISHELLPOS, &ulPlotPos, sizeof(ulPlotPos) ) ;
        if( pausedata.pswp != NULL )
            PrfWriteProfileData( hini, APP_NAME, INIPAUSEPOS, 
                                 pausedata.pswp, sizeof(SWP) ) ;
        ulOpts[0] = (ULONG)bLineTypes ;
        ulOpts[1] = (ULONG)bWideLines ; 
        ulOpts[2] = (ULONG)bColours ;
        ulOpts[3] = ulPauseMode ; 
        ulOpts[4] = (ULONG)bPopFront ; 
        PrfWriteProfileData( hini, APP_NAME, INIOPTS, &ulOpts, sizeof(ulOpts) ) ;
        PrfWriteProfileData( hini, APP_NAME, INIFRAC, &qPrintData.xsize, 4*sizeof(float) ) ;
        if( qPrintData.pdriv != NULL )
            PrfWriteProfileData( hini, APP_NAME, INIPRDRIV, qPrintData.pdriv, 
                                 qPrintData.cbpdriv ) ;
        PrfWriteProfileString( hini, APP_NAME, INIPRPR, 
                               qPrintData.szPrinterName[0] == '\0'? NULL:
                               qPrintData.szPrinterName ) ;
        PrfWriteProfileString( hini, APP_NAME, INIFONT, szFontNameSize ) ;
        ulOpts[0] = (ULONG)lCharWidth ;
        ulOpts[1] = (ULONG)lCharHeight ;
        PrfWriteProfileData( hini, APP_NAME, INICHAR, &ulOpts, sizeof(ulOpts) ) ;
	PrfWriteProfileData( hini, APP_NAME, INIKEEPRATIO, &bKeepRatio, sizeof(bKeepRatio) ) ; //PM
	//PM mouse stuff
	/* ignore reading "Use mouse" --- no good idea to have mouse on by default.
	   Maybe it was the reason of some crashes (mouse init before draw).
	PrfWriteProfileData( hini, APP_NAME, INIUSEMOUSE, &bUseMouse, sizeof(bUseMouse) ) ;
	*/
	/* Do not write the mouse coord. mode.
	PrfWriteProfileData( hini, APP_NAME, INIMOUSECOORD, &mouse_mode, sizeof(mouse_mode) ) ;
	*/
        PrfCloseProfile( hini ) ;
        }
    else {
        WinMessageBox( HWND_DESKTOP,
                       HWND_DESKTOP,
                       "Can't write ini file",
                       APP_NAME,
                       0,
                       MB_OK | MB_ICONEXCLAMATION ) ;
        }    
    }

static void DoPaint( HWND hWnd, HPS hps  ) 
/*
**  Paint the screen with current data 
*/
    {
    ULONG ulCount ;

    static RECTL rectl ;    
    if( tidDraw != 0 ) {
            /* already drawing - stop it; include the rectl now
               being drawn in, in the update region; and return
               without calling beginpaint so that the paint 
               message is resent */
        GpiSetStopDraw( hpsScreen, SDW_ON ) ;
        DosSleep(1) ;
        WinInvalidateRect( hWnd, &rectl, TRUE ) ;
        return ;
        }
            /* winbeginpaint here, so paint message is
               not resent when we return, then spawn a 
               thread to do the drawing */
    WinBeginPaint( hWnd, hps, &rectl ) ;                 //rl
    tidDraw = _beginthread( ThreadDraw, NULL, 32768, NULL ) ;
    }

static void ThreadDraw( void* arg )
/*
**  Thread to draw plot on screen
*/
    {
    HAB     hab ;

    hab = WinInitialize( 0 ) ;

    InitScreenPS() ;        

    DosRequestMutexSem( semHpsAccess, (ULONG) SEM_INDEFINITE_WAIT ) ;
    ScalePS( hpsScreen ) ;
    GpiSetStopDraw( hpsScreen, SDW_OFF ) ;  
    GpiSetDrawingMode( hpsScreen, DM_DRAW ) ;
    GpiDrawChain( hpsScreen ) ;
    draw_ruler(); //PM
    WinEndPaint( hpsScreen ) ;
    DosReleaseMutexSem( semHpsAccess ) ;
    WinTerminate( hab ) ;
    tidDraw = 0 ;
    }

HPS InitScreenPS()
/*
** Initialise the screen ps for drawing
*/
    {
    RECTL   rectClient ;
    int     nColour = 0 ;        

    GpiResetPS( hpsScreen, GRES_ATTRS ) ;
    GpiErase(hpsScreen);
 
    WinQueryWindowRect( hApp, (PRECTL)&rectClient ) ;
    if (bKeepRatio) //PM
    {
    double ratio = 1.560 ;
    double xs = rectClient.xRight - rectClient.xLeft ;
    double ys = rectClient.yTop - rectClient.yBottom ;
    if( ys > xs/ratio ) { /* reduce ys to fit */
        rectClient.yTop = rectClient.yBottom + (int)(xs/ratio) ; 
        }
    else if( ys < xs/ratio ) { /* reduce xs to fit */
        rectClient.xRight = rectClient.xLeft + (int)(ys*ratio) ;
        }
    }
    else rectClient.xRight -= 10; //PM
	/* //PM: why this -10? Otherwise the right axis is too close to
	 the right border. However, this -10 should be taken into account 
	 for mousing! Or can it be inside a transformation?
	 */
       
    GpiSetPageViewport( hpsScreen, &rectClient ) ;
    if( !bColours ) {
        int i ;
        for( i=0; i<8; i++ ) alColourTable[i] = 0 ;
        for( i=8; i<16; i++ ) alColourTable[i] = 0 ;
        alColourTable[0] = 0xFFFFFF ;
        nColour = 16 ;
        }
    GpiCreateLogColorTable( hpsScreen,
                            LCOL_RESET,
                            LCOLF_CONSECRGB,
                            0, nColour, alColourTable ) ;
    return hpsScreen ;
    }

enum JUSTIFY { LEFT, CENTRE, RIGHT } jmode;


short ScalePS( HPS hps )
/*
**  Get a font to use
**  Scale the plot area to world coords for subsequent plotting
*/
    {
    RECTL rectView ;
        
    SelectFont( hps, szFontNameSize ) ;
    return 0 ;
    }

static SIZEF sizBaseSubSup ;
static SIZEF sizCurSubSup ;
static SIZEF sizCurFont ;
static long lVOffset = 0 ;
static SIZEF sizBaseFont ;
static struct _ft {
    char *name ;
    LONG  lcid ; 
    } tabFont[256] = {{NULL,0L}, {NULL}};
    
void SelectFont( HPS hps, char *szFontNameSize )
/*
**  Select a named and sized outline (adobe) font
*/
    {
     HDC    hdc ;
     FATTRS  fat ;
     LONG   xDeviceRes, yDeviceRes ;
     POINTL ptlFont ;
     SIZEF  sizfx ;
     static LONG lcid = 0L ;
     static char *szFontName ;
     static short shPointSize ;

     sscanf( szFontNameSize, "%hd", &shPointSize ) ;
     szFontName = strchr( szFontNameSize, '.' ) + 1 ;

     fat.usRecordLength  = sizeof fat ;
     fat.fsSelection     = 0 ;
     fat.lMatch          = 0 ;
     fat.idRegistry      = 0 ;
     fat.usCodePage      = 0 ; //GpiQueryCp (hps) ;
     fat.lMaxBaselineExt = 0 ;
     fat.lAveCharWidth   = 0 ;
     fat.fsType          = 0 ;
     fat.fsFontUse       = FATTR_FONTUSE_OUTLINE |
                           FATTR_FONTUSE_TRANSFORMABLE ;

     strcpy (fat.szFacename, szFontName) ;

     if(tabFont[0].name !=NULL) free( tabFont[0].name ) ;
     tabFont[0].name = strdup( szFontName ) ;
     tabFont[0].lcid = 10L ;
     
     lcid = GpiQueryCharSet( hps ) ;
     if( lcid != 10L ) lcid = 10L ;
     else {
        GpiSetCharSet( hps, 0L) ;
        GpiDeleteSetId( hps, lcid ) ;
        }
     GpiCreateLogFont (hps, NULL, lcid, &fat) ;
     GpiSetCharSet( hps, lcid ) ;

     hdc = GpiQueryDevice (hps) ;

     DevQueryCaps (hdc, CAPS_HORIZONTAL_RESOLUTION, 1L, &xDeviceRes) ;
     DevQueryCaps (hdc, CAPS_VERTICAL_RESOLUTION,   1L, &yDeviceRes) ;

                         // Find desired font size in pixels

     ptlFont.x = 2540L * (long)shPointSize / 72L ;
     ptlFont.y = 2540L * (long)shPointSize / 72L ;

                         // Set the character box

     sizfx.cx = MAKEFIXED (ptlFont.x, 0) ;
     sizfx.cy = MAKEFIXED (ptlFont.y, 0) ;
     lVOffset = ptlFont.y ;

     sizBaseFont = sizfx ;
     GpiSetCharBox (hps, &sizfx) ;

                        // set up some useful globals
     {
     FONTMETRICS fm ;
     GpiQueryFontMetrics( hps, sizeof(FONTMETRICS), &fm ) ;
     lBaseSubOffset = -fm.lSubscriptYOffset ;
     lBaseSupOffset = fm.lSuperscriptYOffset ;
     lSubOffset = lBaseSubOffset ;
     lSupOffset = lBaseSupOffset ;
     lCharHeight = fm.lMaxAscender*1.2 ;
     lCharWidth  = fm.lAveCharWidth ;
     sizBaseSubSup.cx = MAKEFIXED( ptlFont.x*0.7, 0 ) ; 
     sizBaseSubSup.cy = MAKEFIXED( ptlFont.y*0.7, 0 ) ; 
     }
     sizCurFont = sizBaseFont ;
     sizCurSubSup = sizBaseSubSup ;
    if( bNewFont ) {
//        EditCharCell( hps, &sizfx ) ;
        bNewFont = FALSE ;
        }
    }

void SwapFont( HPS hps, char *szFNS )
/*
**  Select a named and sized outline (adobe) font
*/
    {
     HDC    hdc ;
     FATTRS  fat ;
     LONG   xDeviceRes, yDeviceRes ;
     POINTL ptlFont ;
     SIZEF  sizfx ;
     static LONG lcid = 0L ;
     static int itab = 1 ;
     static char *szFontName ;
     static short shPointSize ;

     if( szFNS == NULL ) {    /* restore base font */
         sizCurFont = sizBaseFont ;
         sizCurSubSup = sizBaseSubSup ;
         lSubOffset = lBaseSubOffset ;
         lSupOffset = lBaseSupOffset ;
         GpiSetCharSet( hps, 10 ) ;
         GpiSetCharBox (hps, &sizBaseFont) ;
        }
     else {
        sscanf( szFNS, "%hd", &shPointSize ) ;
        szFontName = strchr( szFNS, '.' ) + 1 ;

        {
            int i ;
            lcid = 0 ;
            for(i=0;i<itab;i++) {
                if( strcmp( szFontName, tabFont[i].name ) == 0 ) {
                    lcid = tabFont[i].lcid ;
                    break ;
                    }
                }
        }
        if( lcid == 0 ) {   

        fat.usRecordLength  = sizeof fat ;
        fat.fsSelection     = 0 ;
        fat.lMatch          = 0 ;
        fat.idRegistry      = 0 ;
        fat.usCodePage      = 0 ; //GpiQueryCp (hps) ;
        fat.lMaxBaselineExt = 0 ;
        fat.lAveCharWidth   = 0 ;
        fat.fsType          = 0 ;
        fat.fsFontUse       = FATTR_FONTUSE_OUTLINE |
                               FATTR_FONTUSE_TRANSFORMABLE ;

        strcpy (fat.szFacename, szFontName) ;


        tabFont[itab].name = strdup( szFontName ) ;
        lcid = itab+10 ;
        tabFont[itab].lcid = lcid ;
        ++itab ;

//        lcid = 11L ;
        GpiSetCharSet( hps, 0L) ;
        GpiDeleteSetId( hps, lcid ) ;
        GpiCreateLogFont (hps, NULL, lcid, &fat) ;
        }
        GpiSetCharSet( hps, lcid ) ;
     hdc = GpiQueryDevice (hps) ;

     DevQueryCaps (hdc, CAPS_HORIZONTAL_RESOLUTION, 1L, &xDeviceRes) ;
     DevQueryCaps (hdc, CAPS_VERTICAL_RESOLUTION,   1L, &yDeviceRes) ;

                         // Find desired font size in pixels

     ptlFont.x = 2540L * (long)shPointSize / 72L ;
     ptlFont.y = 2540L * (long)shPointSize / 72L ;

                         // Set the character box

     sizCurFont.cx = MAKEFIXED (ptlFont.x, 0) ;
     sizCurFont.cy = MAKEFIXED (ptlFont.y, 0) ;
//     lVOffset = ptlFont.y ;

     GpiSetCharBox (hps, &sizCurFont) ;
     sizCurSubSup.cx = MAKEFIXED( ptlFont.x*0.7, 0 ) ; 
     sizCurSubSup.cy = MAKEFIXED( ptlFont.y*0.7, 0 ) ; 

                        // set up some useful globals
     {
     FONTMETRICS fm ;
     GpiQueryFontMetrics( hps, sizeof(FONTMETRICS), &fm ) ;
     lSubOffset = -fm.lSubscriptYOffset ;
     lSupOffset = fm.lSuperscriptYOffset ;
     }
        }
        
     }

static void ReadGnu( void* arg )
/*
** Thread to read plot commands from  GNUPLOT pm driver.
** Opens named pipe, then clears semaphore to allow GNUPLOT driver to proceed.
** Reads commands and builds a command list.
*/
    {
    HPIPE    hRead = 0L ; 
    POINTL ptl ;
    POINTL aptl[4] ;    
    long lCurCol ;
    long lOldLine = 0 ;
    BOOL bBW = FALSE ; /* passed frpm print.c ?? */
    BOOL bPath = FALSE ;
    BOOL bDots = FALSE ;
    char *szEnv ;
    char *szFileBuf ;
    ULONG rc; 
    USHORT usErr ;
    ULONG cbR ;
    USHORT i ;
    PID ppid ;
    unsigned char buff[2] ;
    HEV hev ;
    static char *szPauseText = NULL ;
    ULONG ulPause ;
    char *pszPipeName, *pszSemName ;
    char  mouseShareMemName[40]; //PM
    LONG  commands[4] ;
    HPS hps ;    
    HAB hab ;
    int linewidth = DEFLW ;

    hab = WinInitialize( 0 ) ;
    DosEnterCritSec() ;
    pszPipeName = malloc( 256 ) ;
    pszSemName  = malloc( 256 ) ;
    DosExitCritSec() ;
    strcpy( pszPipeName, "\\pipe\\" ) ;
    strcpy( pszSemName, "\\sem32\\" ) ;
    strcat( pszPipeName, szIPCName ) ;
    strcat( pszSemName, szIPCName ) ;

            /* open a named pipe for communication with gnuplot */
 
    rc = DosCreateNPipe( pszPipeName,
                         &hRead, 
                         NP_ACCESS_DUPLEX|NP_NOINHERIT|NP_NOWRITEBEHIND ,
                         1|NP_WAIT|NP_READMODE_MESSAGE|NP_TYPE_MESSAGE,
                         PIPEBUF,
                         PIPEBUF,
                         0xFFFFFFFF) ;
    hev = 0 ;       /* OK, gnuplot can try to open npipe ... */
    DosOpenEventSem( pszSemName, &hev ) ;
    DosPostEventSem( hev ) ;
    

    
        /* attach to gnuplot */

server:

    if( DosConnectNPipe( hRead ) == 0L ) {

        WinPostMsg( hSysMenu,
                    MM_SETITEMATTR,
                    MPFROM2SHORT(SC_CLOSE, TRUE ),
                    MPFROM2SHORT(MIA_DISABLED, MIA_DISABLED) ) ;

        /* store graphics commands */
        /* use semaphore to prevent problems with drawing while reallocating
           the command buffers */

        DosRead( hRead, &ppidGnu, 4, &cbR ) ;

	sprintf( mouseShareMemName, "\\SHAREMEM\\GP%i_Mouse_Input", (int)ppidGnu ); //PM 11.5.1999
	if (DosGetNamedSharedMem(&input_from_PM_Terminal, //fraba
		mouseShareMemName,
		PAG_WRITE))
	   DosBeep(1440L,1000L); /* indicates error */

	semInputReady = 0; //PM
	bhave_grid = FALSE ;
	bhave_log = FALSE ;
	  // semaphore 'semInputReady' must be open later in order to avoid problems
	  // with the server mode; also 'bhave_*' init here because of server

//        DosPostEventSem( semStartSeq ) ;         /* once we've got pidGnu */
        WinPostMsg( hApp, WM_GPSTART, 0, 0 ) ;


        hps = hpsScreen ;
   InitScreenPS() ;
        while (1) {
        
            usErr=BufRead(hRead,buff, 1, &cbR) ;
            if( usErr != 0 ) break ;
 
	    if (breakDrawing) { //PM: drawing has been stopped (by Ctrl-C)...
		hps = 0;	//    ...thus drawings go to nowhere...
		if (*buff == 'E') { //...unless 'plot finished' command
		    POINTL p;
		    hps = hpsScreen;  // drawings back to screen
		    breakDrawing = 0;
		    GpiSetColor(hps, CLR_RED); // cross the unfinished plot
		    GpiBeginPath( hps, 1 ) ;
		    p.x = p.y = 0; GpiMove( hps, &p );
		    p.x = 19500; p.y = 12500; GpiLine( hps, &p );
		    p.x = 0; p.y = 12500; GpiMove( hps, &p );
		    p.x = 19500; p.y = 0; GpiLine( hps, &p );
		    GpiEndPath( hps ) ;
		    GpiStrokePath( hps, 1, 0 ) ;
		}
	    }

            switch( *buff ) {
                case 'G' :    /* enter graphics mode */
                    {
                    ULONG ulCount ;    
                    
                    if( tidDraw != 0 ) {
                      /* already drawing - stop it */
                        GpiSetStopDraw( hpsScreen, SDW_ON ) ;
                        while(tidDraw != 0) DosSleep(1) ;
                        }
    	            /* wait for access to command list and lock it */
//                    DosWaitEventSem( semDrawDone, SEM_INDEFINITE_WAIT ) ;                    
//                    DosEnterCritSec() ;
                    DosRequestMutexSem( semHpsAccess, (ULONG) SEM_INDEFINITE_WAIT ) ;
                    InitScreenPS() ;
                    ScalePS( hps ) ;
//                    DosResetEventSem( semDrawDone, &ulCount ) ;
                    GpiSetDrawingMode( hps, DM_DRAWANDRETAIN ) ;
                    for(i=1;i<=iSeg;i++)
                        GpiDeleteSegment( hps, i ) ;
                    iSeg = 1 ;
                    GpiOpenSegment( hps, iSeg ) ;
//                    DosExitCritSec() ;
                    GpiSetLineEnd( hps, LINEEND_ROUND ) ;
                    GpiSetLineWidthGeom( hps, linewidth ) ;
                    GpiSetCharBox (hps, &sizBaseFont) ;
		    //PM get some other information about real plot coord.:
		    if (mouseTerminal) { // we are connected to mouseable terminal
		      BufRead(hRead,&gp4mouse, sizeof(gp4mouse), &cbR);
		      if (ruler.on) recalc_ruler_pos();
		      }
                    }
                    break ;
                    
                case '#' :    /* do_3dplot() and boundary3d() changed [xleft..ytop] */
		    BufRead(hRead,&gp4mouse.xleft, 4*sizeof(gp4mouse.xleft), &cbR);
		    if (ruler.on) recalc_ruler_pos();
                    break ;

                case 'Q' :     /* query terminal info */
		    //PM mouseable gnupmdrv sends greetings to mouseable PM terminal
		    if (mouseTerminal) {
		      int i=0xABCD;
		      DosWrite( hRead, &i, sizeof(int), &cbR ) ;
		      }
                    DosWrite( hRead, &lCharWidth, sizeof(int), &cbR ) ;
                    DosWrite( hRead, &lCharHeight, sizeof(int), &cbR ) ;
                    break ;

                case 'E' :     /* leave graphics mode (graph completed) */
                    if( bPath ) {
                        GpiEndPath( hps ) ;
                        GpiStrokePath( hps, 1, 0 ) ;
                        bPath = FALSE ;
                        }
                    GpiCloseSegment( hps ) ;
		    draw_ruler(); //PM
//                    DosPostEventSem( semDrawDone ) ;
                    DosReleaseMutexSem( semHpsAccess ) ;
                    WinPostMsg( hApp, WM_GNUPLOT, 0L, 0L ) ;
                    break ;
                    
                case 'R' :
                    /* gnuplot has reset drivers, allow user to kill this */
                    WinPostMsg( hSysMenu,
                                MM_SETITEMATTR,
                                MPFROM2SHORT(SC_CLOSE, TRUE ),
                                MPFROM2SHORT(MIA_DISABLED, (USHORT)0 ) ) ;
                    /* if we are keeping us on the screen, wait for new connection */
                    if( bServer||bPersist ) {
                        DosDisConnectNPipe( hRead ) ;
                        goto server ;
                        }    
                    break ;
                
                case 'r' :
                    /* resume after multiplot */
                    {
                    ULONG ulCount ;    
                    DosRequestMutexSem( semHpsAccess, (ULONG) SEM_INDEFINITE_WAIT ) ;
//                    DosWaitEventSem( semDrawDone, SEM_INDEFINITE_WAIT ) ;
                    iSeg++ ;
//                    DosResetEventSem( semDrawDone, &ulCount ) ;
                    GpiSetDrawingMode( hps, DM_DRAWANDRETAIN ) ;
                    GpiOpenSegment( hps, iSeg ) ;
                    }
                    break ;
                    
                case 's' :
                    /* suspend after multiplot */
                    break ;

                case 'M' :   /* move */
                case 'V' :   /* draw vector */
                    if( *buff=='M' ) {
                        if( bPath ) {
                            GpiEndPath( hps ) ;
                            GpiStrokePath( hps, 1, 0 ) ;
                            bPath = FALSE ;
                            }
                        }
                    else {
                        if( bWideLines/*bWideLines*/ && !bPath ) {
                            GpiBeginPath( hps, 1 ) ;
                            bPath = TRUE ;
                            }
                        }
                    BufRead(hRead,&ptl.x, 2*sizeof(int), &cbR) ;
                    if( (*buff=='V') && bDots ) ptl.x += 5 ;
                    else if( (*buff=='M') && bDots ) ptl.x -= 5 ;
                    if( *buff == 'M' ) LMove( hps, &ptl ) ;
                    else LLine( hps, &ptl ) ;
                    break ;
                  
                case 'P' :   /* pause */
                    {
                    int len ;
		    pausing = 1;
                    BufRead(hRead,&len, sizeof(int), &cbR) ;
                    len = (len+sizeof(int)-1)/sizeof(int) ;
                    if( len > 0 ){  /* get pause text */
                        DosEnterCritSec() ;
                        szPauseText = malloc( len*sizeof(int) ) ;
                        DosExitCritSec() ;
                        BufRead(hRead,szPauseText, len*sizeof(int), &cbR) ;
                        }
                    if( ulPauseMode != PAUSE_GNU ) {
                             /* pause and wait for semaphore to be cleared */
                        DosResetEventSem( semPause, &ulPause ) ;
                        WinPostMsg( hApp, WM_PAUSEPLOT, (MPARAM) szPauseText, 0L ) ;
                        DosWaitEventSem( semPause, SEM_INDEFINITE_WAIT ) ;
                        }
                    else { /* gnuplot handles pause */
                        ulPauseReply = 2 ;
                        }
                    DosEnterCritSec() ;
                    if( szPauseText != NULL ) free( szPauseText ) ;
                    szPauseText = NULL ;
                    DosExitCritSec() ;
                             /* reply to gnuplot so it can continue */
                    DosWrite( hRead, &ulPauseReply, sizeof(int), &cbR ) ;
		    pausing = 0;
                    }
                    break ; 
                       
                case 'T' :   /* write text */
                        /* read x, y, len */
                    if( bPath ) {
                        GpiEndPath( hps ) ;
                        GpiStrokePath( hps, 1, 0 ) ;
                        bPath = FALSE ;
                        }
                    {
                    int x, y, len, sw ;
                    char *str ;
                    BufRead(hRead,&x, sizeof(int), &cbR) ;
                    BufRead(hRead,&y, sizeof(int), &cbR) ;
                    BufRead(hRead,&len, sizeof(int), &cbR) ;

                    DosEnterCritSec() ;
                    len = (len+sizeof(int)-1)/sizeof(int) ;
                    if( len == 0 ) len = 1 ; //?? how about read
                    str = malloc( len*sizeof(int) ) ;
                    *str = '\0' ;
                    DosExitCritSec() ;
                    BufRead(hRead, str, len*sizeof(int), &cbR) ;
                    lCurCol = GpiQueryColor( hps ) ;
                    GpiSetColor( hps, CLR_BLACK ) ;
                    sw = QueryTextBox( hps, strlen(str), str ) ; 
                    switch(jmode) {
	                case LEFT:   sw = 0;     break;
	                case CENTRE: sw = -sw/2; break;
	                case RIGHT:  sw = -sw;   break;
                        }
                    if( bHorz ) {
                        ptl.x = (LONG)(x+sw) ; ptl.y = (LONG)(y-lVOffset/4) ;
                        }
                    else {
                        ptl.x = (LONG)x ; ptl.y = (LONG)(y+sw) ;
                        }
                    if(bEnhanced)
                        CharStringAt( hps, ptl.x, ptl.y, strlen( str ) , str ) ;
                    else
                        GpiCharStringAt( hps, &ptl, strlen( str ), str ) ;
                    GpiSetColor( hps, lCurCol ) ;
                    DosEnterCritSec() ;
                    free(str) ;
                    DosExitCritSec() ;
                    }
                    break ;
                    
                case 'J' :   /* justify */
                    BufRead(hRead,&jmode, sizeof(int), &cbR) ;
                    break ;

                case 'A' :   /* text angle */
                    {
                    int ta ;    
                    GRADIENTL grdl ;
                    SIZEF sizHor, sizVer ;
                    if( bPath ) {
                        GpiEndPath( hps ) ;
                        GpiStrokePath( hps, 1, 0 ) ;
                        bPath = FALSE ;
                        }
                    BufRead(hRead,&ta, sizeof(int), &cbR) ;
                    if( ta == 0 ) {
                        grdl.x = 0L ; grdl.y = 0L ;
                        GpiSetCharAngle( hps, &grdl ) ;
                        if( !bHorz ) {
                            bHorz = TRUE ;
                            }
                        }
                    else if( ta == 1 ) {
                        grdl.x = 0L ; grdl.y = 1L ;
                        GpiSetCharAngle( hps, &grdl ) ;
                        if( bHorz ) {
                            bHorz = FALSE ;
                            }
                        }
                    }
                    break ;

                case 'L' :   /* line type */
                    {
                    int lt, col ;
                    if( bPath ) {
                        GpiEndPath( hps ) ;
                        GpiStrokePath( hps, 1, 0 ) ;
                        bPath = FALSE ;
                        }
                    BufRead(hRead,&lt, sizeof(int), &cbR) ;
	            /* linetype = -2 axes, -1 border, 0 arrows, all to 0 */
	            col = lt ;
                    if( lt == -2 )     GpiSetLineWidthGeom( hps, DEFLW*0.85 ) ;
                    else if( lt == -1 ) GpiSetLineWidthGeom( hps, DEFLW*0.6 ) ;
                    else GpiSetLineWidthGeom( hps, linewidth ) ;
                    if( lt < 0 ) lt = 0 ;
	            lt = (lt%8);
	            col = (col+2)%16 ;
                    GpiLabel( hps, lLineTypes[lt] ) ;
lOldLine=lt ;
                    LType( (bLineTypes||bBW)?lt:0 ) ;
//                    GpiSetLineType( hps, (bLineTypes||bBW)?lLineTypes[lt]:lLineTypes[0] ) ;
                    if( !bBW ) { /* maintain some flexibility here in case we don't want
                           the model T option */ 
                        if( bColours ) GpiSetColor( hps, lCols[col] ) ;
                        else GpiSetColor( hps, CLR_BLACK ) ;
                        }
                    }
                    break ;
                    
                case 'W' :   /* line width */
                    {
                    int lw ;
                    if( bPath ) {
                        GpiEndPath( hps ) ;
                        GpiStrokePath( hps, 1, 0 ) ;
                        bPath = FALSE ;
                        }
                    BufRead(hRead,&lw, sizeof(int), &cbR) ;
                    GpiSetLineWidthGeom( hps, DEFLW*lw/100 ) ;
                    linewidth = DEFLW*lw/100 ;
                    }
                    break ;
                    
                 case 'D' :   /* points mode */
                    {
                    int lt ;
                    BufRead(hRead,&lt, sizeof(int), &cbR) ;
    	              /* 1: enter point mode, 0: exit */
                    if( bLineTypes || bBW ) {
                        if( lt==1) LType(0) ;
                        else LType( lOldLine ) ;
//                        if( lt == 1 ) lOldLine = GpiSetLineType( hps, lLineTypes[0] ) ;
//                        else GpiSetLineType( hps, lOldLine ) ;
                        }
//                    if( lt == 1 ) GpiSetLineWidthGeom( hps, 20 ) ;
//                    else GpiSetLineWidthGeom( hps, 50 ) ;
                    bDots = lt ;
                    }
                    break ;
                
                case 'F' :   /* set font */

                    {
                    int len ;
                    char *str ;
                    char font[FONTBUF] ;
                    BufRead(hRead,&len, sizeof(int), &cbR) ;
                    len = (len+sizeof(int)-1)/sizeof(int) ;
                    if( len == 0 ) {
                        SwapFont( hps, NULL ) ;
                        }
                    else {
                        char *p ;
                        str = malloc( len*sizeof(int) ) ;
                        BufRead(hRead, str, len*sizeof(int), &cbR) ;
                        p = strchr(str, ',') ;
                        if( p==NULL ) strcpy( font, "14" ) ;
                        else {
                            *p = '\0' ;
                            strcpy( font, p+1 ) ;
                            }
                        strcat( font,"." ) ;
                        strcat( font, str ) ;
                        free( str ) ;
                        SwapFont( hps, font ) ;
                        } 
                    }
                    break ;
                    
                case 'O' :   /* set options */

                    {
                    int len ;
                    char *str ;
                    BufRead(hRead,&len, sizeof(int), &cbR) ;
                    len = (len+sizeof(int)-1)/sizeof(int) ;
                    bWideLines = FALSE ; /* reset options */
                    bEnhanced = FALSE ;
                    
                    if( len > 0 ) {
                        char *p ;
                        p = str = malloc( len*sizeof(int) ) ;
                        BufRead(hRead, str, len*sizeof(int), &cbR) ;
                        while( (p=strchr(p,'-')) != NULL ) {
                            ++p ;
                            if( *p == 'w' ) bWideLines = TRUE ;
                            if( *p == 'e' ) bEnhanced = TRUE ;
                            ++p ;
                            }
                        free( str ) ;
                        } 
                    }
                    break ;
                
		case 'm' :
		    //PM notification of being connected to a mouse-enabled terminal
		    mouseTerminal = 1;
		    break ;

                 default :  /* should handle error */
                    break ;
                 }
             }
        }
exitserver:
    DosDisConnectNPipe( hRead ) ;
    WinPostMsg( hApp, WM_CLOSE, 0L, 0L ) ;    
    }

static void EditLineTypes( HWND hwnd, HPS hps, BOOL bDashed )
/*
*/
    {
    int i, rc ;
    char buf[64] ;
    GpiSetDrawingMode( hps, DM_RETAIN ) ;
    GpiOpenSegment( hps, iSeg ) ;
    GpiSetEditMode( hps, SEGEM_REPLACE ) ;
    for( i=0; i<7; i++ ) {
        while( GpiSetElementPointerAtLabel( hps, lLineTypes[i] ) ) {
            GpiOffsetElementPointer( hps, 1 ) ;
            GpiSetLineType( hps, bDashed?lLineTypes[i]:lLineTypes[0] ) ;
            }
        GpiSetElementPointer( hps, 0 ) ;
        }    
    GpiSetEditMode( hps, SEGEM_INSERT ) ;
    GpiCloseSegment( hps ) ;    
    }

static void EditCharCell( HPS hps, SIZEF *psize )
/*
** Edit segment to change char cell (font size)
*/
    {
    int i ;
    LONG rl, rc ;
    SIZEF sizH, sizV ;
    char buf[64] ;
    int iVert = 0 ;
    
    sizH = *psize ;
    sizV.cx = sizH.cy ;
    sizV.cy = sizH.cx ;
    GpiSetDrawingMode( hps, DM_RETAIN ) ;
    GpiOpenSegment( hps, iSeg ) ;
    GpiSetEditMode( hps, SEGEM_REPLACE ) ;
    i=0 ;
    while( GpiSetElementPointer( hps, i ) ) {
        rc = GpiQueryElementPointer( hps) ;
        if( rc != i ) break ;
        rl = GpiQueryElementType( hps, &rc, 0, NULL ) ;
        if( rc == 0x34 || rc == 0x74 ) {
            LONG gdata ;
            GpiQueryElement( hps, 5, 4, (PBYTE)&gdata ) ;
            if( gdata == 0 ) iVert = 0 ;
            else iVert = 1 ; 
            }
        else if( rc==0x33 || rc==0x03 ) GpiSetCharBox(hps, iVert?&sizV:&sizH ) ;
        ++i ;
        }
    GpiSetEditMode( hps, SEGEM_INSERT ) ;
    GpiCloseSegment( hps ) ;    
    }

static int BufRead( HFILE hfile, void *buf, int nBytes, ULONG *pcbR )
/*
** pull next plot command out of buffer read from GNUPLOT
*/
    {
    ULONG ulR, ulRR ;
    int rc ;
    static char buffer[GNUBUF] ;
    static char *pbuffer = buffer+GNUBUF, *ebuffer = buffer+GNUBUF ;
    
    for( ; nBytes > 0 ; nBytes-- ) {
        if( pbuffer >= ebuffer ) {
            ulR = GNUBUF ;
            rc = DosRead( hfile, buffer, ulR, &ulRR ) ;
            if( rc != 0 ) return rc ;
            if( ulRR == 0 ) return 1 ;
            pbuffer = buffer ;
            ebuffer = pbuffer+ulRR ;
            }
        *(char*)buf++ = *pbuffer++ ;
        }
    return 0L ;
    } 

 
int GetNewFont( HWND hwnd, HPS hps ) 
/*
** Get a new font using standard font dialog
*/
    {
    static FONTDLG pfdFontdlg;      /* Font dialog info structure */
    static int i1 =1 ;
    static int     iSize ;
    char szPtList[64] ;
    HWND    hwndFontDlg;     /* Font dialog window handle */
    char    *p ; 
    char szFamilyname[FACESIZE];
 
    if( i1 ) {
        strcpy( pfdFontdlg.fAttrs.szFacename, strchr( szFontNameSize, '.' ) + 1 ) ;
        strcpy( szFamilyname, strchr( szFontNameSize, '.' ) + 1 ) ;
        sscanf( szFontNameSize, "%d", &iSize ) ;
        memset(&pfdFontdlg, 0, sizeof(FONTDLG));
 
        pfdFontdlg.cbSize = sizeof(FONTDLG);
        pfdFontdlg.hpsScreen = hps;
 /*   szFamilyname[0] = 0;*/
        pfdFontdlg.pszFamilyname = szFamilyname;
        pfdFontdlg.usFamilyBufLen = FACESIZE;
        pfdFontdlg.fl = FNTS_HELPBUTTON | 
                        FNTS_CENTER | FNTS_VECTORONLY | 
                        FNTS_OWNERDRAWPREVIEW ; 
        pfdFontdlg.clrFore = CLR_BLACK;
        pfdFontdlg.clrBack = CLR_WHITE;
        pfdFontdlg.usWeight = FWEIGHT_NORMAL ; //5 ;
        pfdFontdlg.fAttrs.usCodePage = 0;
        pfdFontdlg.fAttrs.usRecordLength = sizeof(FATTRS) ;
        }
    sprintf( szPtList, "%d 8 10 12 14 18 24", iSize ) ;
    pfdFontdlg.pszPtSizeList = szPtList ;
    pfdFontdlg.fxPointSize = MAKEFIXED(iSize,0); 
    hwndFontDlg = WinFontDlg(HWND_DESKTOP, hwnd, &pfdFontdlg);
    if( i1 ) {
        pfdFontdlg.fl = FNTS_HELPBUTTON | 
                        FNTS_CENTER | FNTS_VECTORONLY | 
                        FNTS_INITFROMFATTRS ; 
        i1=0; 
        } 
    if (hwndFontDlg && (pfdFontdlg.lReturn == DID_OK)) {
        iSize = FIXEDINT( pfdFontdlg.fxPointSize ) ;
        sprintf( szFontNameSize, "%d.%s", iSize, pfdFontdlg.fAttrs.szFacename ) ;
        return 1 ;
        }
    else return 0 ;
    }
 
void SigHandler( int sig )
/*
**  Handle termination signal to free up resources before
**  termination.
*/
    {
    if( sig == SIGTERM ) {
        if( bPersist ) {
            DosKillThread( tidSpawn ) ;
            signal( SIGTERM, SIG_ACK ) ;
            return ;
            }
        DosEnterCritSec() ;
        DosKillThread( tidSpawn ) ;
        DosKillThread( tidDraw ) ;
        DosExitCritSec() ;
        exit(0) ;
        }
    }

/* disable debugging info */
#define TEXT_DEBUG(x) /* fprintf x; */

/* used in determining height of processed text */

//static float max_height, min_height;

/* process a bit of string, and return the last character used.
 * p is start of string
 * brace is TRUE to keep processing to }, FALSE for do one character
 * fontname & fontsize are obvious
 * base is the current baseline
 * widthflag is TRUE if the width of this should count,
 *              FALSE for zero width boxes
 * showflag is TRUE if this should be shown,
 *             FALSE if it should not be shown (like TeX \phantom)
 */

static char *starttext = NULL ;
static int  textlen = 0 ;
static BOOL bText = FALSE ;
static int  textwidth = 0 ;
static POINTL ptlText ;
static FILE *ff ;
static char *ParseText(HPS hps, char *p, BOOL brace, char *fontname,
                       int fontsize, int base, BOOL widthflag, BOOL showflag)
{
    POINTL aptl[TXTBOX_COUNT] ;
    BOOL bChangeFont = FALSE ;
	TEXT_DEBUG((ff,"RECURSE WITH [%p] %s, %d %s %.1f %.1f %d %d\n", p, p, brace, fontname, fontsize, base, widthflag, showflag))

    /* Start each recursion with a clean string */

//{FILE *ff;int i=textlen;ff=fopen("deb","a");
//for(i=0;i<textlen;i++)fputc(starttext[i], ff);fputc('\n',ff) ; fclose(ff);}
        if( textlen > 0 ) {
            GpiQueryTextBox( hps, textlen, starttext, TXTBOX_COUNT, aptl ) ;
            if( bHorz ) textwidth += aptl[TXTBOX_BOTTOMRIGHT].x ;
            else        textwidth += aptl[TXTBOX_BOTTOMRIGHT].y ;
            }
        if( bText ) {
            if(textlen > 0 ) {
                GpiCharStringAt( hps, &ptlText, textlen, starttext ) ;
                ptlText.x += aptl[TXTBOX_CONCAT].x + (bHorz?0:(-base)) ;
                ptlText.y += aptl[TXTBOX_CONCAT].y + (bHorz?base:0) ;
                }
            else {
                ptlText.x += (bHorz?0:(-base)) ;
                ptlText.y += (bHorz?base:0) ;
                }
            }
        textlen = 0 ;
        starttext = p ;
        if( fontname != NULL ) {
            char szFont[FONTBUF] ;
            sprintf(szFont, "%d.%s", fontsize, fontname ) ;
            SwapFont( hps, szFont ) ;
            bChangeFont = TRUE ;
            }
    if( base != 0 ) GpiSetCharBox( hps, &sizCurSubSup ) ;        

    for ( ; *p; ++p)
    {    int shift;

        switch (*p)
        {
            case '}'  :
                /*{{{  deal with it*/
                if (brace) {
                    brace = 0 ;
                    break ;
                    }
                
                break;
                /*}}}*/
        
            case '_'  :
            case '^'  :
                /*{{{  deal with super/sub script*/
                
                shift = (*p == '^') ? lSupOffset : lSubOffset;
                p = ParseText(hps, p+1, FALSE, NULL/*fontname*/, fontsize*0.7, base+shift, widthflag, showflag);                
                break;
                /*}}}*/
        
            case '{'  :
            {
                char *savepos=NULL, save=0;
                char *localfontname=fontname, ch;
                char localfontbuf[FONTBUF] ;
                int recode=1;
                int f=fontsize;
                BOOL bChangeFont = FALSE ;
                char *q=localfontbuf ;

                /*{{{  recurse (possibly with a new font) */
                
                TEXT_DEBUG((ff,"Dealing with {\n"))
                
                if (*++p == '/')
                {    /* then parse a fontname, optional fontsize */
                    while (*++p == ' ');
                    if (*p=='-')
                    {
                        recode=0;
                        while (*++p == ' ');
                    }
                    localfontname = p;
                    while ((ch = *p) > ' ' && ch != '=') {
                            localfontname=localfontbuf ;
                            if(*p=='_') *q=' ' ;
                            else *q=*p ;
                        ++p;++q;
                        }
                    *q = '\0' ;
                    FontExpand( localfontbuf ) ;
                    save = *(savepos=p);
                    if (ch == '=')
                    {
                        *p++ = '\0';                
                        /*{{{  get optional font size*/
                        TEXT_DEBUG((ff,"Calling strtod(%s) ...", p))
                        f = strtod(p, &p);
                        TEXT_DEBUG((ff,"Retured %.1f and %s\n", f, p))
                        
                        if (!f) f = fontsize;
                        
                        TEXT_DEBUG((ff,"Font size %.1f\n", f))
                        /*}}}*/
                    }
                    else
                    {
                        *p++ = '\0';
                        f = fontsize;
                    }                
                
                    while (*p == ' ')
                        ++p;
                    if (! (*localfontname)) {
                        localfontname = fontname;
                        if( f != fontsize ) 
                            localfontname = strchr( szFontNameSize, '.' ) + 1 ;
                        }
                }
                /*}}}*/
                
                TEXT_DEBUG((ff,"Before recursing, we are at [%p] %s\n", p, p))
        
                p = ParseText(hps,p, TRUE, localfontname, f, base, widthflag, showflag);
                
                TEXT_DEBUG((ff,"BACK WITH %s\n", p));
                if (savepos)
                    /* restore overwritten character */
                    *savepos = save;
        
                break;
            }
                
            case '@' :
                /*{{{  phantom box - prints next 'char', then restores currentpoint */
                
                p = ParseText(hps,++p, FALSE, NULL/*fontname*/, fontsize, base, FALSE, showflag);
                    
                break;
                /*}}}*/
        
            case '&' :
                /*{{{  character skip - skips space equal to length of character(s) */
                
                p = ParseText(hps,++p, FALSE, NULL/*fontname*/, fontsize, base, widthflag, FALSE);
                    
                break;
                /*}}}*/

            case '\\'  :
                {
                char buffer[4] ; /* should need only one char.. */
                char *q = buffer ;
                *q = '\0' ;
                ParseText(hps,q, FALSE, NULL, fontsize, base, widthflag, showflag);
                /*{{{  is it an escape */
                /* special cases */
                
                if (p[1]=='\\' || p[1]=='{' || p[1]=='}')
                {
                    *q++=p[1] ;
                    ++p ;
                }
#if 0           
                else if (p[1] >= '0' && p[1] <= '7')
                {
                    /* up to 3 octal digits */
                    int c = 0 ;
                    c+=p[1];
                    ++p;
                    if (p[1] >= '0' && p[1] <= '7')
                    {
                        c*=8; c+=p[1];
                        ++p;
                        if (p[1] >= '0' && p[1] <= '7')
                        {
                            c*=8; c+=p[1];
                            ++p;
                        }
                    }
                    *q++ = c ;
                    break;
                }
#endif          
                *q = '\0' ;
                textlen = 1 ;
                starttext=buffer ;
                ParseText(hps,q, FALSE, NULL/*fontname*/, fontsize, base, widthflag, showflag);
                starttext=p+1 ;
                textlen = 0 ;
                /*}}}*/
                }
                break ;

            default: 
                ++textlen ;                               
    
            /*}}}*/

        }

        /* like TeX, we only do one character in a recursion, unless it's
         * in braces
         */

        if (!brace) break ;
        }
        if( textlen > 0 ) {
            GpiQueryTextBox( hps, textlen, starttext, TXTBOX_COUNT, aptl ) ;
            if( widthflag ) {
                if( bHorz ) textwidth += aptl[TXTBOX_BOTTOMRIGHT].x ;
                else        textwidth += aptl[TXTBOX_BOTTOMRIGHT].y ;
                }
            }
        if( bText ) {
            if( textlen > 0 ) {
                if( showflag)
                    GpiCharStringAt( hps, &ptlText, textlen, starttext ) ;
                if( widthflag ) {
                    ptlText.x += aptl[TXTBOX_CONCAT].x ;
                    ptlText.y += aptl[TXTBOX_CONCAT].y ;
                    }
                }
            if( base != 0 ) {
                ptlText.x -= (bHorz?0:(-base)) ;
                ptlText.y -= (bHorz?base:0) ;
                }
            }
        if( bChangeFont ) {
            SwapFont( hps, NULL ) ;
            bChangeFont = FALSE ;
            }
    if( base != 0 ) GpiSetCharBox( hps, &sizBaseFont ) ;        
                

        textlen = 0 ;
        starttext = p+1 ;
    return p;
} 

static void CharStringAt(HPS hps, int x, int y, int len, char *str)
{
    /* flush any pending graphics (all the XShow routines do this...) */

     char *fontname ;
     int fontsize ;

    if (!strlen(str))
        return;

    /* set up the globals */
    
        ptlText.x = x ;
        ptlText.y = y ;
        bText = TRUE ;
        starttext = NULL ;
        textlen = 0 ;
        textwidth = 0 ;
        sscanf( szFontNameSize, "%d", &fontsize ) ;
        fontname = strchr( szFontNameSize, '.' ) + 1 ;
        
    while (*(str = ParseText(hps, str, TRUE, NULL,
                     fontsize,
                     0.0, TRUE, TRUE)));

}

static int QueryTextBox( HPS hps, int len, char *str ) 
{
     char *fontname ;
     int fontsize ;
    if (!strlen(str))
        return 0 ;


    /* set up the globals */
    
        bText = FALSE ;
        starttext = NULL ;
        textlen = 0 ;
        textwidth = 0 ;
        sscanf( szFontNameSize, "%d", &fontsize ) ;
        fontname = strchr( szFontNameSize, '.' ) + 1 ;
        
    while (*(str = ParseText(hps, str, TRUE, NULL,
                     fontsize,
                     0.0, TRUE, TRUE)));

    return textwidth ;    
}

void FontExpand( char *name )
    {
    if     ( strcmp(name,"S")==0 ) strcpy( name, "Symbol Set" ) ;
    else if( strcmp(name,"H")==0 ) strcpy( name, "Helvetica" ) ;
    else if( strcmp(name,"T")==0 ) strcpy( name, "Times New Roman" ) ;
    else if( strcmp(name,"C")==0 ) strcpy( name, "Courier" ) ;
    }

/*=======================================*/
static POINTL pCur ;
static int iLinebegin = 1 ;
static int iLtype = 0 ;
static int iState = 0 ;
static double togo = 0.0 ;
static int iPatt[8][9]
  = {
    {   0,   0,   0,   0,   0,   0,   0,   0,  0 },
    { 300, 200,  -1,   0,   0,   0,   0,   0,  0 },
    { 150, 150,  -1,   0,   0,   0,   0,   0,  0 },
    { 300, 200, 150, 200,  -1,   0,   0,   0,  0 },
    { 500, 200,  -1,   0,   0,   0,   0,   0,  0 },
    { 300, 200, 150, 200, 150, 200,  -1,   0,  0 },
    { 300, 200, 150, 200, 150, 200, 150, 200, -1 },
    { 500, 200, 150, 200,  -1,   0,   0,   0,  0 }
    } ;

void LMove( HPS hps, POINTL *p )
    {
    double ds, dx, dy ;
    if( iLinebegin ) {
        pCur = *p ;
        GpiMove( hps, p ) ;
        }
    else if( iLtype == 0 ) GpiMove( hps, p ) ;
    else {
        dx = p->x - pCur.x ;
        dy = p->y - pCur.y ;
        ds = sqrt( dx*dx + dy*dy ) ;
        dx /= ds ; dy /= ds ;
        while( ds > 0.0 ) {
            if( ds < togo ) {
                togo -= ds ;
                ds = 0.0 ;
                GpiMove( hps, p ) ;
                pCur = *p ;
                }
            else {
                POINTL pn ;
                pn.x = pCur.x + togo * dx ;
                pn.y = pCur.y + togo * dy ;
                GpiMove( hps, &pn ) ;
                pCur = pn ;
                ds -= togo ;
                iState++ ;
                if( iPatt[iLtype][iState] < 0 ) {
                    togo = iPatt[iLtype][0] ;
                    iState = 0 ;
        }
                else togo = iPatt[iLtype][iState] ;
    }
            }
        }
    } 

void LLine( HPS hps, POINTL *p )
    {
    double ds, dx, dy ;
    if( iLinebegin ) iLinebegin = 0 ;

    if( iLtype == 0 ) GpiLine( hps, p ) ;
    else {
        dx = p->x - pCur.x ;
        dy = p->y - pCur.y ;
        ds = sqrt( dx*dx + dy*dy ) ;
        dx /= ds ; dy /= ds ;
        while( ds > 0.0 ) {
            if( ds < togo ) {
                togo -= ds ;
                ds = 0.0 ;
                if( iState&1 ) GpiMove( hps, p ) ;
                else GpiLine( hps, p ) ;
                pCur = *p ;
                }
            else {
                POINTL pn ;
                pn.x = pCur.x + togo * dx ;
                pn.y = pCur.y + togo * dy ;
                if( iState&1 ) GpiMove( hps, &pn ) ;
                else GpiLine( hps, &pn ) ;
                pCur = pn ;
                ds -= togo ;
                iState++ ;
                if( iPatt[iLtype][iState] < 0 ) {
                    togo = iPatt[iLtype][0] ;
                    iState = 0 ;
                    }
                else togo = iPatt[iLtype][iState] ;
                }
            }
        }
    } 

void LType( int iType )
    {
    iLinebegin = 1 ;
    if( iType > 7 ) iType = 0 ;
    iLtype = iType ;
    iState = 0 ;
    togo = iPatt[iLtype][0] ;
    }


//PM Now routines for mouse etc.

static void TextToClipboard ( PCSZ szTextIn ) {
// copy string given by szTextIn to the clipboard
PSZ szTextOut = NULL;
ULONG ulRC = DosAllocSharedMem( (PVOID*)&szTextOut, NULL,
		       strlen( szTextIn ) + 1,
		       PAG_WRITE | PAG_COMMIT | OBJ_GIVEABLE );
if( !ulRC ) {
  HAB hab;
  strcpy( szTextOut, szTextIn );
  WinOpenClipbrd(hab);
  WinEmptyClipbrd(hab);
  WinSetClipbrdData( hab, (ULONG) szTextOut, CF_TEXT, CFI_POINTER );
  WinCloseClipbrd(hab);
  }
}


/* Draw the ruler
*/
static void draw_ruler ( void )
{
POINTL p;
if (!ruler.on) return; 
GpiSetColor( hpsScreen, COLOR_RULER );
//GpiBeginPath( hpsScreen, 1 ) ; // will this help? I don't know, but makes thic cross
p.x = 0;	p.y = ruler.py; GpiMove( hpsScreen, &p ) ;
p.x = 19500;			GpiLine( hpsScreen, &p ) ;
p.x = ruler.px;	p.y = 0;	GpiMove( hpsScreen, &p ) ;
p.y = 12500;			GpiLine( hpsScreen, &p ) ;
//GpiEndPath( hpsScreen ) ;
//GpiStrokePath( hpsScreen, 1, 0 ) ;
}


/* Format x according to the date/time mouse mode. Uses and returns b as 
   a buffer
*/
static char* xDateTimeFormat ( double x, char* b )
{
#define SEC_OFFS_SYS 946684800
time_t xtime_position = SEC_OFFS_SYS + x;
struct tm *pxtime_position = gmtime(&xtime_position);
switch (mouse_mode) {
  case MOUSE_COORDINATES_XDATE:
	sprintf(b,"%d. %d. %04d",
		pxtime_position->tm_mday,
		(pxtime_position->tm_mon)+1,
#if 1
		(pxtime_position->tm_year) +
		  ((pxtime_position->tm_year <= 68) ? 2000 : 1900)
#else
		((pxtime_position->tm_year)<100) ? 
		  (pxtime_position->tm_year) : (pxtime_position->tm_year)-100
//              (pxtime_position->tm_year)+1900
#endif
		);
	break;
  case MOUSE_COORDINATES_XTIME:
	sprintf(b,"%d:%02d", pxtime_position->tm_hour, pxtime_position->tm_min);
	break;
  case MOUSE_COORDINATES_XDATETIME:
	sprintf(b,"%d. %d. %04d %d:%02d",
		pxtime_position->tm_mday,
		(pxtime_position->tm_mon)+1,
#if 1
		(pxtime_position->tm_year) +
		  ((pxtime_position->tm_year <= 68) ? 2000 : 1900),
#else
		((pxtime_position->tm_year)<100) ? 
		  (pxtime_position->tm_year) : (pxtime_position->tm_year)-100,
//              (pxtime_position->tm_year)+1900,
#endif
		pxtime_position->tm_hour,
		pxtime_position->tm_min
		);
	break;
  default: sprintf(b, "%g", x);
  }
return b;
}


/* Let the command in the shared memory to be executed by gnuplot.
	If this routine is called during a 'pause', then the command is
   ignored (shared memory is cleared). Needed for actions launched by a
   hotkey.
	Firstly, the command is copied from shared memory to clipboard
   if this option is set on.
	Secondly, gnuplot is informed that shared memory contains a command
   by posting semInputReady event semaphore. 
*/
void gp_execute ( void )
{
    APIRET rc;
    if (input_from_PM_Terminal==NULL || ((char*)input_from_PM_Terminal)[0]==0)
	return;
    if (pausing) { // no communication during pause
	DosBeep(440,111);
	((char*)input_from_PM_Terminal)[0] = 0;
	return;
    }
    // write the command to clipboard
    if (bSend2gp == TRUE)
	TextToClipboard ( input_from_PM_Terminal );
    // let the command in the shared memory be executed...
    if (semInputReady == 0) { // but it must be open for the first time
	char semInputReadyName[40]; 
	sprintf( semInputReadyName, "\\SEM32\\GP%i_Input_Ready", (int)ppidGnu );
	DosOpenEventSem( semInputReadyName,  &semInputReady);
    }
    rc = DosPostEventSem(semInputReady);
}


/* This routine recalculates mouse/pointer position [mx,my] in [in pixels]
current window to the real/true [x,y] coordinates of the plotted graph.
*/
void MousePosToGraphPos ( double *x, double *y,
	HWND hWnd, SHORT mx, SHORT my, ULONG mouse_mode ) {
RECTL rc;

if (mouse_mode==MOUSE_COORDINATES_PIXELS) {
  *x = mx; *y = my;
  return;
  }

// Rectangle where we are moving: viewport, not the full window!
GpiQueryPageViewport(hpsScreen,&rc);

rc.xRight -= rc.xLeft; rc.yTop -= rc.yBottom; // only distance is important

if (mouse_mode==MOUSE_COORDINATES_SCREEN) {
  *x = (double)mx/rc.xRight; *y = (double)my/rc.yTop;
  return;
  }

*x = mx * 19500.0/rc.xRight; // px=px(mx); mouse=>gnuplot driver coordinates
*y = my * 12500.0/rc.yTop;

if (gp4mouse.xright==gp4mouse.xleft) *x = 1e38; else // protection
*x = gp4mouse.xmin + (*x-gp4mouse.xleft)/(gp4mouse.xright-gp4mouse.xleft)
	      * (gp4mouse.xmax-gp4mouse.xmin);
if (gp4mouse.ytop==gp4mouse.ybot) *y = 1e38; else // protection
*y = gp4mouse.ymin + (*y-gp4mouse.ybot)/(gp4mouse.ytop-gp4mouse.ybot)
	      * (gp4mouse.ymax-gp4mouse.ymin);
// Note: there is xleft+0.5 in "#define map_x" in graphics.c, which
// makes no major impact here. It seems that the mistake of the real
// coordinate is at about 0.5%, which corresponds to the screen resolution.
// It would be better to round the distance to this resolution, and thus
// *x = gp4mouse.xmin + rounded-to-screen-resolution (xdistance)

// Now take into account possible log scales of x and y axes
if  (gp4mouse.is_log_x) *x = exp( *x * gp4mouse.log_base_log_x );
if  (gp4mouse.is_log_y) *y = exp( *y * gp4mouse.log_base_log_y );

}


/* Ruler is on, thus recalc its (px,py) from (x,y) for
   the current zoom and log axes
*/
void recalc_ruler_pos()
 {
			double P;
			if (gp4mouse.is_log_x && ruler.x<0)
			    ruler.px = -1;
			  else {
			    P = gp4mouse.is_log_x ?
				  log(ruler.x) / gp4mouse.log_base_log_x
				  : ruler.x;
			    P = (P-gp4mouse.xmin) / (gp4mouse.xmax-gp4mouse.xmin);
			    P *= gp4mouse.xright-gp4mouse.xleft;
			    ruler.px = (long)( gp4mouse.xleft + P );
			    }
			if (gp4mouse.is_log_y && ruler.y<0)
			    ruler.py = -1;
			  else {
			    P = gp4mouse.is_log_y ?
				  log(ruler.y) / gp4mouse.log_base_log_y
				  : ruler.y;
			    P = (P-gp4mouse.ymin) / (gp4mouse.ymax-gp4mouse.ymin);
			    P *= gp4mouse.ytop-gp4mouse.ybot;
			    ruler.py = (long)( gp4mouse.ybot + P );
			    }
			}


void do_zoom ( double xmin, double ymin, double xmax, double ymax )
{
struct t_zoom *z;
if (zoom_head == NULL) { // queue not yet created, thus make its head
    zoom_head = malloc( sizeof(struct t_zoom) );
    zoom_head->prev = NULL;
    zoom_head->next = NULL;
    }
if (zoom_now == NULL) zoom_now = zoom_head;
if (zoom_now->next == NULL) { // allocate new item
    z = malloc( sizeof(struct t_zoom) );
    z->prev = zoom_now;
    z->next = NULL; 
    zoom_now->next = z;
    z->prev = zoom_now;
} else // overwrite next item
    z = zoom_now->next;
z->xmin = xmin; z->ymin = ymin;
z->xmax = xmax; z->ymax = ymax;
apply_zoom( z );
}


/* Applies the zoom rectangle of  z  by sending the appropriate command
   to gnuplot
*/
static void apply_zoom ( struct t_zoom *z )
{
char s[256];
if (zoom_now != NULL) { // remember the current zoom
  zoom_now->xmin = (!gp4mouse.is_log_x) ? gp4mouse.xmin : exp( gp4mouse.xmin * gp4mouse.log_base_log_x );
  zoom_now->ymin = (!gp4mouse.is_log_y) ? gp4mouse.ymin : exp( gp4mouse.ymin * gp4mouse.log_base_log_y );
  zoom_now->xmax = (!gp4mouse.is_log_x) ? gp4mouse.xmax : exp( gp4mouse.xmax * gp4mouse.log_base_log_x );
  zoom_now->ymax = (!gp4mouse.is_log_y) ? gp4mouse.ymax : exp( gp4mouse.ymax * gp4mouse.log_base_log_y );
}
zoom_now = z;
if (zoom_now == NULL) {
    DosBeep(444,111);
    return;
}
WinEnableMenuItem( // can this situation be zoomed back?
  WinWindowFromID( WinQueryWindow( hApp, QW_PARENT ), FID_MENU ),
  IDM_MOUSE_ZOOMNEXT, (zoom_now->next == NULL) ? FALSE : TRUE ) ;
WinEnableMenuItem( // can this situation be unzoomed back?
  WinWindowFromID( WinQueryWindow( hApp, QW_PARENT ), FID_MENU ),
  IDM_MOUSE_UNZOOM, (zoom_now->prev == NULL) ? FALSE : TRUE ) ;
WinEnableMenuItem( // can this situation be unzoomed to the beginning?
  WinWindowFromID( WinQueryWindow( hApp, QW_PARENT ), FID_MENU ),
  IDM_MOUSE_UNZOOMALL, (zoom_now == zoom_head) ? FALSE : TRUE ) ;
sprintf(s,"set xr[%g:%g]; set yr[%g:%g]; replot", // the command for gnuplot
	zoom_now->xmin, zoom_now->xmax, zoom_now->ymin, zoom_now->ymax);
sprintf(input_from_PM_Terminal,"%s",s); // send the command to shared memory...
gp_execute();				// and let gnuplot know about it
}


/* eof gclient.c */
