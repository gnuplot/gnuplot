#ifdef INCRCSDATA
static char RCSid[]="$Id: gclient.c%v 3.50 1993/07/09 05:35:24 woo Exp $" ;
#endif

/****************************************************************************

    PROGRAM: Gnupmdrv
    
    MODULE:  gclient.c
        
    This file contains the client window procedures for Gnupmdrv
    
****************************************************************************/

/*
 * PM driver for GNUPLOT
 * Copyright (C) 1992   Roger Fearick
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
 * AUTHOR
 * 
 *   Gnuplot driver for OS/2:  Roger Fearick
 * 
 * Send your comments or suggestions to 
 *  info-gnuplot@dartmouth.edu.
 * This is a mailing list; to join it send a note to 
 *  info-gnuplot-request@dartmouth.edu.  
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
#include <stdlib.h>
#include <unistd.h>
#include <process.h>
#include "gnupmdrv.h"

/*==== g l o b a l    d a t a ================================================*/

extern char szIPCName[] ;       /* name used in IPC with gnuplot */

/*==== l o c a l    d a t a ==================================================*/

#define   GNUBUF    1024        /* buffer for gnuplot commands */
#define   PIPEBUF   4096        /* size of pipe buffers */
#define   CMDALLOC  4096        /* command buffer allocation increment (ints) */
#define   ENVSIZE   2048        /* size of environment */ 
#define   GNUPAGE   4096        /* size of gnuplot page in pixels (driver dependent) */

#define   PAUSE_DLG 1           /* pause handled in dialog box */
#define   PAUSE_BTN 2           /* pause handled by menu item */
#define   PAUSE_GNU 3           /* pause handled by Gnuplot */

static HWND     hWndstart ;     /* used for errors in startup */
static ULONG    pidGnu=0L ;      /* gnuplot session id */
static ULONG    ppidGnu=0L ;      /* gnuplot pid */
static HPS      hpsScreen ;     /* screen pres. space */

static HSWITCH hSwitch = 0 ;    /* switching between windows */
static SWCNTRL swGnu ;

static BOOL     bLineTypes = FALSE ;
static BOOL     bLineThick = FALSE ;
static BOOL     bColours = TRUE ;
static BOOL     bShellPos = FALSE ;
static BOOL     bPlotPos = FALSE ;
static ULONG    ulPlotPos[4] ;
static ULONG    ulShellPos[4] ;
static char     szFontNameSize[FONTBUF] ;
static char     achPrinterName[128] = "" ;
static PRQINFO3 infPrinter = { "" } ;
static HMTX     semCommands ;
static HEV      semStartSeq ;   /* semaphore to start things in right sequence */
static HEV      semPause ;
static ULONG    ulPauseReply = 1 ;
static ULONG    ulPauseMode  = PAUSE_DLG ;

            /* commands from gnuplot come via this ... */
            
static HPIPE    hRead = 0L ; 
           
            /* stuff for screen-draw thread control */
            
static BOOL     bExist ; 
static BOOL     bStopDraw ; 
static HEV      semDrawDone ;
static HEV      semStartDraw ;

            /* command buffer */
            
static int ncalloc = 0 ;
static int ic = 0 ;
static volatile int *commands = NULL ;

/*==== f u n c t i o n s =====================================================*/

int             DoPrint( HWND ) ;
MRESULT         WmClientCmdProc( HWND , ULONG, MPARAM, MPARAM ) ; 
void            ChangeCheck( HWND, USHORT, USHORT ) ;
BOOL            QueryIni( HAB ) ;
static void     SaveIni( void ) ;
static void     ThreadDraw( void ) ;
static void     DoPaint( HWND, HPS ) ;
static void     Display( void ) ;
void            SelectFont( HPS, char *, short );
static void     ReadGnu( void ) ;
static void     WaitEnd( void ) ;
static void     AllocMore() ;
static int      BufRead( HFILE, void*, int, PULONG ) ;
int             GetNewFont( HWND, HPS ) ;


/*==== c o d e ===============================================================*/

MRESULT EXPENTRY DisplayClientWndProc(HWND hWnd, ULONG message, MPARAM mp1, MPARAM mp2)
/*
**  Window proc for main window
**  -- passes most stuff to active child window via WmClientCmdProc
**  -- passes DDE messages to DDEProc
*/
{
    HDC     hdcScreen ;
    SIZEL   sizlPage ;
    TID     tidDraw, tidSpawn ;
    char    *pp ;
    ULONG   ulID ;
    ULONG   ulFlag ;
    char    szErrs[128] ;
    
    switch (message) {

        case WM_CREATE:

                // set initial values
            ChangeCheck( hWnd, IDM_LINES_THICK, bLineThick?IDM_LINES_THICK:0 ) ;
            ChangeCheck( hWnd, IDM_LINES_SOLID, bLineTypes?0:IDM_LINES_SOLID ) ;
            ChangeCheck( hWnd, IDM_COLOURS, bColours?IDM_COLOURS:0 ) ;
            hWndstart = hWnd ;  /* used in ReadGnu for errors */ 
                // disable close from system menu (close only from gnuplot)
            WinSendMsg( WinWindowFromID( WinQueryWindow( hWnd, QW_PARENT ), FID_SYSMENU ),
                        MM_SETITEMATTR,
                        MPFROM2SHORT(SC_CLOSE, TRUE ),
                        MPFROM2SHORT(MIA_DISABLED, MIA_DISABLED ) ) ;
                // setup semaphores
            DosCreateMutexSem( NULL, &semCommands, 0L, 0L ) ;
            DosCreateEventSem( NULL, &semStartDraw, 0L, 0L ) ;
            DosCreateEventSem( NULL, &semDrawDone, 0L, 0L ) ;
            DosCreateEventSem( NULL, &semStartSeq, 0L, 0L ) ;
            DosCreateEventSem( NULL, &semPause, 0L, 0L ) ;
            bStopDraw = FALSE ;
            bExist = TRUE ;
                // create a dc and hps to draw on the screen
            hdcScreen = WinOpenWindowDC( hWnd ) ;
            sizlPage.cx = GNUPAGE ; sizlPage.cy = GNUPAGE ;
            hpsScreen = GpiCreatePS( hab, hdcScreen, &sizlPage,
                           PU_ARBITRARY|GPIT_MICRO|GPIA_ASSOC) ;
                // spawn a thread to do the drawing
            DosCreateThread( &tidDraw, (PFNTHREAD)ThreadDraw, 0L, 0L, 8192L ) ;
                // then spawn server for GNUPLOT ...
            DosCreateThread( &tidSpawn, (PFNTHREAD)ReadGnu, 0L, 0L, 8192L ) ;
            DosWaitEventSem( semStartSeq, SEM_INDEFINITE_WAIT ) ;
                // get details of command-line window
            hSwitch = WinQuerySwitchHandle( 0, ppidGnu ) ;
            WinQuerySwitchEntry( hSwitch, &swGnu ) ;
                // set size of this window
            WinSetWindowPos( WinQueryWindow( hWnd, QW_PARENT ), 
                             HWND_TOP,
                             ulShellPos[0],
                             ulShellPos[1],
                             ulShellPos[2],
                             ulShellPos[3], 
                             bShellPos?SWP_SIZE|SWP_MOVE|SWP_SHOW|SWP_ACTIVATE:SWP_SHOW|SWP_ACTIVATE ) ;
                //  clear screen 
            DosPostEventSem( semDrawDone ) ;  
            break ;
            

        case WM_COMMAND:

            return WmClientCmdProc( hWnd , message , mp1 , mp2 ) ;

        case WM_CLOSE:

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

            DoPaint( hWnd, hpsScreen ) ;
            break ;     

        case WM_SIZE :
            
            WinInvalidateRect( hWnd, NULL, TRUE ) ;
            break ;

        case WM_PRESPARAMCHANGED:
        
            pp = malloc(FONTBUF) ;
            if( WinQueryPresParam( hWnd, 
                                   PP_FONTNAMESIZE, 
                                   0, 
                                   &ulID, 
                                   FONTBUF, 
                                   pp, 
                                   QPF_NOINHERIT ) != 0L ) {
                strcpy( szFontNameSize, pp ) ;
                WinInvalidateRect( hWnd, NULL, TRUE ) ;
                }
            free(pp) ; 
            break ;
            
        case WM_USER_PRINT_BEGIN:
        case WM_USER_PRINT_OK :
        case WM_USER_DEV_ERROR :
        case WM_USER_PRINT_ERROR :
        case WM_USER_PRINT_QBUSY :

            return( PrintCmdProc( hWnd, message, mp1, mp2 ) ) ;

        case WM_GNUPLOT:
                // display the plot         
            WinSetWindowPos( hwndFrame, HWND_TOP, 0,0,0,0, SWP_ACTIVATE|SWP_ZORDER ) ;
            WinInvalidateRect( hWnd, NULL, TRUE ) ;
            return 0L ;

        case WM_PAUSEPLOT:
                /* put pause message on screen, or enable 'continue' button */
            if( ulPauseMode == PAUSE_DLG ) {
                WinLoadDlg( HWND_DESKTOP,
                            hWnd,
                            (PFNWP)PauseMsgDlgProc,
                            0L,
                            IDD_PAUSEBOX,
                            (char*)mp1 ) ; 
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
    
	default:         /* Passes it on if unproccessed    */
	    return (WinDefWindowProc(hWnd, message, mp1, mp2));
    }
    return (NULL);
}

MRESULT WmClientCmdProc(HWND hWnd, ULONG message, MPARAM mp1, MPARAM mp2)
/*
**   Handle client window command (menu) messages
**   -- mostly passed on to active child window
**
*/
    {
    ULONG usDlg ;
    extern HWND hApp ;
    static ulPauseItem = IDM_PAUSEDLG ;

    switch( (USHORT) SHORT1FROMMP( mp1 ) ) {
                
        case IDM_ABOUT :    /* show the 'About' box */
             
            WinDlgBox( HWND_DESKTOP,
                       hWnd , 
                       (PFNWP)About ,
                       0L,
                       ID_ABOUT, 
                       NULL ) ;
            break ;


        case IDM_PRINT :    /* print plot */

            if( SetupPrinter( hWnd, achPrinterName, &infPrinter ) )
                WinPostMsg( hWnd, WM_USER_PRINT_BEGIN, (MPARAM)&infPrinter, 0L ) ; 
            break ;

        case IDM_PRINTSETUP :    /* select printer */

            WinDlgBox( HWND_DESKTOP,
                       hWnd ,
                       (PFNWP)QPrintersDlgProc,
                       0L,
                       IDD_QUERYPRINT, 
                       achPrinterName ) ;
            break ;

        case IDM_LINES_THICK:
                // change line setting
            bLineThick = !bLineThick ;
            ChangeCheck( hWnd, IDM_LINES_THICK, bLineThick?IDM_LINES_THICK:0 ) ;
            WinInvalidateRect( hWnd, NULL, TRUE ) ;
            break ;

        case IDM_LINES_SOLID:
                // change line setting
            bLineTypes = !bLineTypes ;
            ChangeCheck( hWnd, IDM_LINES_SOLID, bLineTypes?0:IDM_LINES_SOLID ) ;
            WinInvalidateRect( hWnd, NULL, TRUE ) ;
            break ;

        case IDM_COLOURS:
                // change colour setting
            bColours = !bColours ;        
            ChangeCheck( hWnd, IDM_COLOURS, bColours?IDM_COLOURS:0 ) ;
            WinInvalidateRect( hWnd, NULL, TRUE ) ;
            break ;

        case IDM_FONTS:
        
            if( GetNewFont( hWnd, hpsScreen ) ) 
                WinInvalidateRect( hWnd, NULL, TRUE ) ;
            break ;
            
        case IDM_SAVE :
            SaveIni() ;
            break ;
            
        case IDM_COMMAND:       /* go back to GNUPLOT command window */            
            WinSwitchToProgram( hSwitch ) ;
            break ;

        case IDM_CONTINUE:
            WinPostMsg( hWnd, WM_PAUSEEND, 1L, 0L ) ; 
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

BOOL QueryIni( HAB hab )
/*
** Query INI file
*/
    {
    BOOL         bPos, bData ;
    ULONG        ulOpts[4] ;
    HINI         hini ;
    ULONG        ulCB ;
    char         *p ;
    
            // get default printer name    
            
    PrfQueryProfileString( HINI_PROFILE,
                           "PM_SPOOLER",
                           "PRINTER",
                           ";",
                           achPrinterName,
                           (long) sizeof achPrinterName ) ;
    if( (p=strchr( achPrinterName, ';' )) != NULL ) *p = '\0' ;
    
            // read gnuplot ini file

    hini = PrfOpenProfile( hab, GNUINI ) ;
    ulCB = sizeof( ulShellPos ) ;
    bPos = PrfQueryProfileData( hini, APP_NAME, INISHELLPOS, &ulShellPos, &ulCB ) ;
    ulCB = sizeof( ulOpts ) ;
    bData = PrfQueryProfileData( hini, APP_NAME, INIOPTS, &ulOpts, &ulCB ) ;
    if( bData ) {
        bLineTypes = (BOOL)ulOpts[0] ;
        bLineThick = (BOOL)ulOpts[1] ;
        bColours = (BOOL)ulOpts[2] ;
        ulPauseMode = ulOpts[3] ;
        }
    else {
        bLineTypes = FALSE ;  /* default values */
        bLineThick = FALSE ;
        bColours = TRUE ;
        ulPauseMode = 1 ;
        }
    PrfQueryProfileString( hini, APP_NAME, INIFONT, INITIAL_FONT, 
                                 szFontNameSize, FONTBUF ) ;
    PrfCloseProfile( hini ) ;
    bShellPos = bPos ;
    return bPos ;
    }

static void SaveIni( )
/*
** save data in ini file
*/
    {
    SWP swp ;
    HINI  hini ;
    ULONG ulOpts[4] ;
    
    hini = PrfOpenProfile( hab, GNUINI ) ;
    WinQueryWindowPos( hwndFrame, &swp ) ;
    ulPlotPos[0] = swp.x ;
    ulPlotPos[1] = swp.y ;
    ulPlotPos[2] = swp.cx ;
    ulPlotPos[3] = swp.cy ;
    PrfWriteProfileData( hini, APP_NAME, INISHELLPOS, &ulPlotPos, sizeof(ulPlotPos) ) ;
/*
    WinQueryWindowPos( swGnu.hwnd, &swp ) ;
    ulPlotPos[0] = swp.x ;
    ulPlotPos[1] = swp.y ;
    ulPlotPos[2] = swp.cx ;
    ulPlotPos[3] = swp.cy ;
    PrfWriteProfileData( hini, APP_NAME, INIPLOTPOS, &ulPlotPos, sizeof(ulPlotPos) ) ;
*/
    ulOpts[0] = (ULONG)bLineTypes ;
    ulOpts[1] = (ULONG)bLineThick ; 
    ulOpts[2] = (ULONG)bColours ;
    ulOpts[3] = ulPauseMode ; 
    PrfWriteProfileData( hini, APP_NAME, INIOPTS, &ulOpts, sizeof(ulOpts) ) ;
    PrfWriteProfileString( hini, APP_NAME, INIFONT, szFontNameSize ) ;
    PrfCloseProfile( hini ) ;
    }

static void DoPaint( HWND hWnd, HPS hps  ) 
/*
**  Paint the screen with current data 
*/
    {
    RECTL rectClient ;
    ULONG ulCount ;    
    bStopDraw = TRUE ;    // stop any drawing in progress and wait for 
                          // thread to signal completion 
    DosWaitEventSem( semDrawDone, SEM_INDEFINITE_WAIT ) ;
    DosResetEventSem( semDrawDone, &ulCount ) ;
    WinBeginPaint( hWnd , hps, NULL ) ;
    DosPostEventSem( semStartDraw ) ;  // start drawing
    }

static void ThreadDraw( )
/*
**  Thread to draw plot
*/
    {
    HAB  hab ;
    RECTL rectClient ;
    ULONG ulCount ;

        /* initialize and wait until ready to draw */
    
    hab = WinInitialize( 0 ) ;

        /* ok - draw until window is destroyed */

    while( bExist )  {

                // indicate access to window 
        DosWaitEventSem( semStartDraw, SEM_INDEFINITE_WAIT ) ;
        DosResetEventSem( semStartDraw, &ulCount ) ;

                // will be set TRUE if we decide to stop in the middle, but now
        bStopDraw = FALSE ;         
        
        GpiResetPS( hpsScreen, GRES_ALL ) ;        
        WinQueryWindowRect( hApp, (PRECTL)&rectClient ) ;
        GpiSetPageViewport( hpsScreen, &rectClient ) ;

        WinFillRect( hpsScreen, &rectClient, CLR_WHITE ) ;

        ScalePS( hpsScreen, &rectClient, 0 ) ;

        PlotThings( hpsScreen, 0L ) ;
                // ok, say that we did it

        WinEndPaint( hpsScreen ) ;
        DosPostEventSem( semDrawDone ) ;
        }
   
    WinTerminate( hab ) ;
    }


enum JUSTIFY { LEFT, CENTRE, RIGHT } jmode;

void PlotThings( HPS hps, long lColour ) 
/*
**  Plot a spectrum and related graphs on the designated presentation space
**
**  Input:
**          HPS hps         -- presentation space handle of plot ps
**          long lColour    -- number of physical colours, used mainly by
**                             printer drivers to set black & white mode.
**                             If 0, assume screen display
**
**  Note: use semaphore to prevent access to command list while
**        pipe thread is reallocating the list.
*/
    {
    int i, lt, ta, col, sl, n, x, y, cx, cy, width ;
    int icnow ;
    int cmd ;
    char *str, *buf ;
    long sw ;
    POINTL ptl ;
    POINTL aptl[4] ;    
    FONTMETRICS fm ;
    HDC hdc ;
    long lVOffset ;
    long yDeviceRes ;
    long lCurCol ;
    long lOldLine = 0 ;
    BOOL bBW ;
    GRADIENTL grdl ;
    BOOL bHorz = TRUE ;
    SIZEF sizHor, sizVer ;
        /* sometime, make these user modifiable... */
    static long lLineTypes[7] = { LINETYPE_SOLID,
                                  LINETYPE_SHORTDASH,
                                  LINETYPE_DOT,
                                  LINETYPE_DASHDOT,
                                  LINETYPE_LONGDASH,
                                  LINETYPE_DOUBLEDOT,
                                  LINETYPE_DASHDOUBLEDOT } ;
    static long lCols[15] =     { CLR_BLACK,
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
                                  CLR_PALEGRAY } ;

    if( commands == NULL ) return ;

        /* check for colourless devices */

    if( lColour== 1 || lColour == 2 ) bBW = TRUE ;
    else bBW = FALSE ;

        /* get vertical offset for horizontal text strings */
        /* (0.5 em height, so string in vertically centered
           about plot position */

    GpiQueryFontMetrics( hps, sizeof( FONTMETRICS ), &fm ) ;
    lVOffset = fm.lEmHeight ;

    
   /* loop over accumulated commands from inboard driver */

    DosRequestMutexSem( semCommands, SEM_INDEFINITE_WAIT ) ;

    GpiSetLineWidth( hps, bLineThick?LINEWIDTH_THICK:LINEWIDTH_NORMAL ) ;
    for( i=0; bExist && i<ic; ) {

            if( bStopDraw ) break ;

            cmd = commands[i++];

      /*   PM_vector(x,y) - draw vector  */
            if (cmd == 'V') { 
                 ptl.x = (LONG)commands[i++] ; ptl.y = (LONG)commands[i++] ;
                 GpiLine( hps, &ptl ) ;
	         }

      /*   PM_move(x,y) - move  */
            else if (cmd == 'M')  {
                ptl.x = (LONG)commands[i++] ; ptl.y = (LONG)commands[i++] ;
                GpiMove( hps, &ptl ) ;
                }
        
      /*   PM_put_text(x,y,str) - draw text   */
            else if (cmd == 'T') { 
                x = commands[i++] ;
                y = commands[i++] ;
	        str = (char*)&commands[i] ; 
	        sl = strlen(str) ;
	        i += 1+sl/sizeof(int) ;
                lCurCol = GpiQueryColor( hps ) ;
                GpiSetColor( hps, CLR_BLACK ) ;
                GpiQueryTextBox( hps, (LONG)strlen( str ), str, 4L, aptl ) ;
                if( bHorz ) sw = aptl[3].x ;
                else sw = aptl[3].y ;
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
                GpiCharStringAt( hps, &ptl, (LONG) strlen( str ) , str ) ;
                GpiSetColor( hps, lCurCol ) ;
	        }

      /*   PM_justify_text(mode) - set text justification mode  */
            else if (cmd == 'J') 
    	        jmode = commands[i++] ;

      /*   PM_linetype(type) - set line type  */
      /* mapped to colour */
            else if (cmd == 'L') { 
    	        lt = commands[i++] ;
	    /* linetype = -2 axes, -1 border, 0 arrows, all to 0 */
	        col = lt ;
                if( lt == -1 ) GpiSetLineWidth( hps, LINEWIDTH_NORMAL ) ;
                else GpiSetLineWidth( hps, bLineThick?LINEWIDTH_THICK:LINEWIDTH_NORMAL ) ;
                if( lt < 0 ) lt = 0 ;
	        lt = (lt%8);
	        col = (col+2)%16 ;
                if( bLineTypes || bBW ) {
                    GpiSetLineType( hps, lLineTypes[lt] ) ;
                    }
                if( !bBW )  /* maintain some flexibility here in case we don't want
                           the model T option */ 
                    if( bColours ) GpiSetColor( hps, lCols[col] ) ;
                    else GpiSetColor( hps, CLR_BLACK ) ;
                }

            else if (cmd == 'D') { /* point/dot mode - may need colour change */
    	        lt = commands[i++] ;  /* 1: enter point mode, 0: exit */
                if( bLineTypes || bBW ) {
                    if( lt == 1 ) lOldLine = GpiSetLineType( hps, lLineTypes[0] ) ;
                    else GpiSetLineType( hps, lOldLine ) ;
                    }
                }

      /*   PM_text_angle(ang) - set text angle, 0 horz, 1 vert  */
            else if (cmd == 'A') { 
    	        ta = commands[i++] ;
                if( ta == 0 ) {
                    grdl.x = 0L ; grdl.y = 0L ;
                    GpiSetCharAngle( hps, &grdl ) ;
                    if( !bHorz ) {
                        GpiQueryCharBox( hps, &sizVer ) ;
                        sizHor.cx = sizVer.cy ; sizHor.cy = sizVer.cx ;
                        GpiSetCharBox( hps, &sizHor ) ;
                        bHorz = TRUE ;
                        }
                    }
                else if( ta == 1 ) {
                    grdl.x = 0L ; grdl.y = 1L ;
                    GpiSetCharAngle( hps, &grdl ) ;
                    if( bHorz ) {
                        GpiQueryCharBox( hps, &sizHor ) ;
                        sizVer.cx = sizHor.cy ; sizVer.cy = sizHor.cx ;
                        GpiSetCharBox( hps, &sizVer ) ;
                        bHorz = FALSE ;
                        }
                    }
                else continue ;            
                }
            }
        DosReleaseMutexSem( semCommands ) ;
    }
    
short ScalePS( HPS hps, PRECTL prect, USHORT usFlags )
/*
**  Get a font to use
**  Scale the plot area to world coords for subsequent plotting
*/
    {
    RECTL rectView, rectClient ;
    SIZEL sizePage ;
    static char *szFontName ;
    static short shFontSize ;
        
    rectClient = *prect ;
    sizePage.cx = GNUPAGE ; 
    sizePage.cy = GNUPAGE ; 

    sscanf( szFontNameSize, "%d", &shFontSize ) ;
    szFontName = strchr( szFontNameSize, '.' ) + 1 ;
    rectView.xLeft = 0L ;
    rectView.xRight = sizePage.cx ;
    rectView.yBottom = 0L ; rectView.yTop = sizePage.cy ;
    GpiSetPS( hps, &sizePage, PU_ARBITRARY ) ;
    GpiSetPageViewport( hps, &rectClient ) ;
    SelectFont( hps, szFontName, shFontSize ) ;
    GpiSetGraphicsField( hps, &rectView ) ;
    return 0 ;
    }

void SelectFont( HPS hps, char *szFont, short shPointSize )
/*
**  Select a named and sized outline (adobe) font
*/
    {
     HDC         hdc ;
     static FATTRS fat ;
     LONG   xDeviceRes, yDeviceRes ;
     POINTL ptlFont ;
     SIZEF  sizfx ;
     static LONG lcid = 0L ;

     fat.usRecordLength  = sizeof fat ;
     fat.fsSelection     = 0 ;
     fat.lMatch          = 0 ;
     fat.idRegistry      = 0 ;
     fat.usCodePage      = GpiQueryCp (hps) ;
     fat.lMaxBaselineExt = 0 ;
     fat.lAveCharWidth   = 0 ;
     fat.fsType          = 0 ;
     fat.fsFontUse       = FATTR_FONTUSE_OUTLINE |
                           FATTR_FONTUSE_TRANSFORMABLE ;

     strcpy (fat.szFacename, szFont) ;

     if( lcid == 0L ) lcid = 1L ;
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

     ptlFont.x = 254L * (long)shPointSize * xDeviceRes / 720000L ;
     ptlFont.y = 254L * (long)shPointSize * yDeviceRes / 720000L ;

                         // Convert to page units

     GpiConvert (hps, CVTC_DEVICE, CVTC_PAGE, 1L, &ptlFont) ;

                         // Set the character box

     sizfx.cx = MAKEFIXED (ptlFont.x, 0) ;
     sizfx.cy = MAKEFIXED (ptlFont.y, 0) ;

     GpiSetCharBox (hps, &sizfx) ;
    }

static void ReadGnu()
/*
** Thread to read plot commands from  GNUPLOT pm driver.
** Opens named pipe, then clears semaphore to allow GNUPLOT driver to proceed.
** Reads commands and builds a command list.
*/
    {
    char *szEnv ;
    char *szFileBuf ;
    ULONG rc; 
    USHORT usErr ;
    ULONG cbR ;
    STARTDATA start ;
    USHORT i ;
    PID ppid ;
    unsigned char buff[2] ;
    int len ;
    HEV hev ;
    static char *szPauseText = NULL ;
    ULONG ulPause ;
    char *pszPipeName, *pszSemName ;
    
#ifdef USEOWNALLOC
    DosAllocMem( &commands, 64*1024*1024, PAG_READ|PAG_WRITE ) ;
#endif

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

    if( DosConnectNPipe( hRead ) == 0L ) {

        /* store graphics commands */
        /* use semaphore to prevent problems with drawing while reallocating
           the command buffers */

        DosRead( hRead, &ppidGnu, 4, &cbR ) ;
        DosPostEventSem( semStartSeq ) ;         /* once we've got pidGnu */


        while (1) {
        
            usErr=BufRead(hRead,buff, 1, &cbR) ;
            if( usErr != 0 ) break ;
 
            switch( *buff ) {
                case 'G' :    /* enter graphics mode */
    	            /* wait for access to command list and lock it */
                    DosRequestMutexSem( semCommands, SEM_INDEFINITE_WAIT ) ;
                    DosEnterCritSec() ;
#ifdef USEOWNALLOC
                    if( ncalloc > 0 ) {
                        DosSetMem( commands, ncalloc*sizeof(int), PAG_DECOMMIT ) ;
#else
       	            if (commands!=NULL) {    // delete all old commands and prepare for new
    	                free(commands);
#endif
    	                }
                    ic = 0 ;
                    ncalloc = CMDALLOC ;
#ifdef USEOWNALLOC
                    DosSetMem( commands, ncalloc*sizeof(int), PAG_COMMIT|PAG_DEFAULT ) ;
#else
                    commands = (int*)malloc(ncalloc*sizeof(int)) ;
#endif
                    DosExitCritSec() ;
                    DosReleaseMutexSem( semCommands ) ;
                    break ;
                    
                case 'E' :     /* leave graphics mode (graph completed) */
                    Display() ; /* plot graph */
                    break ;
                    
                case 'R' :
                    /* gnuplot has reset drivers, we do nothing */
                    break ;

                case 'M' :   /* move */
                case 'V' :   /* draw vector */
                    commands[ ic++ ] = (int)*buff ; 
                    if( ic+2 >= ncalloc ) AllocMore() ;
                    BufRead(hRead,&commands[ic], 2*sizeof(int), &cbR) ;
                    ic+=2 ;
                    break ;
                  
                case 'P' :   /* pause */
                    BufRead(hRead,&len, sizeof(int), &cbR) ;
                    len = (len+sizeof(int)-1)/sizeof(int) ;
                    if( len > 0 ){  /* get pause text */
                        szPauseText = malloc( len*sizeof(int) ) ;
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
                    if( szPauseText != NULL ) free( szPauseText ) ;
                    szPauseText = NULL ;
                             /* reply to gnuplot so it can continue */
                    DosWrite( hRead, &ulPauseReply, sizeof(int), &cbR ) ;
                    break ; 
                       
                case 'T' :   /* write text */
                    commands[ ic++ ] = (int)*buff ; 
                    if( ic+1 >= ncalloc ) AllocMore() ;
                        /* read x, y, len */
                    BufRead(hRead,&commands[ic++], sizeof(int), &cbR) ;
                    BufRead(hRead,&commands[ic++], sizeof(int), &cbR) ;
                    BufRead(hRead,&len, sizeof(int), &cbR) ;
                    if( ic+1+((len+sizeof(int)-1)/sizeof(int)) >= ncalloc ) AllocMore() ;
                    BufRead(hRead,&commands[ic], len, &cbR) ;
                    if( len == 0 ) len = 1 ;
                    ic += (len+sizeof(int)-1)/sizeof(int) ;
                    break ;
                    
                case 'J' :   /* justify */
                case 'A' :   /* text angle */
                case 'L' :   /* line type */
                case 'D' :   /* points mode */
                
                    commands[ ic++ ] = (int)*buff ; 
                    if( ic+1 >= ncalloc ) AllocMore() ;
                    BufRead(hRead,&commands[ic++], sizeof(int), &cbR) ;
                    break ;

                 default :  /* should handle error */
                    break ;
                 }
             }
        }
    DosEnterCritSec() ;
    free( szFileBuf ) ;
    free( szEnv ) ;
    DosExitCritSec() ;
    pidGnu = 0 ; /* gnuplot has shut down (?) */
    WinPostMsg( hApp, WM_CLOSE, 0L, 0L ) ;    
    }

static int BufRead( HFILE hfile, void *buf, int nBytes, ULONG *pcbR )
/*
** pull next plot command out of buffer read from GNUPLOT
*/
    {
    ULONG ulR, ulRR ;
    static char buffer[GNUBUF] ;
    static char *pbuffer = buffer+GNUBUF, *ebuffer = buffer+GNUBUF ;
    
    for( ; nBytes > 0 ; nBytes-- ) {
        if( pbuffer >= ebuffer ) {
            ulR = GNUBUF ;
            DosRead( hfile, buffer, ulR, &ulRR ) ;
            pbuffer = buffer ;
            ebuffer = pbuffer+ulRR ;
            }
        *(char*)buf++ = *pbuffer++ ;
        }
    return 0L ;
    } 

static void AllocMore()
/*
**  Allocate more memory for plot commands
*/
    {
    DosRequestMutexSem( semCommands, SEM_INDEFINITE_WAIT ) ;
    DosEnterCritSec() ;
#ifdef USEOWNALLOC
    DosSetMem( commands+ncalloc, CMDALLOC*sizeof(int), PAG_COMMIT|PAG_DEFAULT ) ;
#endif
    ncalloc = ncalloc + CMDALLOC ;
#ifndef USEOWNALLOC
    commands = (int*)realloc(commands, ncalloc*sizeof(int)) ;
#endif
    DosExitCritSec() ;
    DosReleaseMutexSem( semCommands ) ;
    }

static void Display()
/*
**  Display gnuplot results 
**  -- must post message as this thread is not drawing thread
*/
    {
    WinPostMsg( hApp, WM_GNUPLOT, 0L, 0L ) ;    
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
        pfdFontdlg.fl = FNTS_HELPBUTTON | FNTS_CENTER | FNTS_VECTORONLY | FNTS_INITFROMFATTRS ; 
        pfdFontdlg.clrFore = CLR_BLACK;
        pfdFontdlg.clrBack = CLR_WHITE;
        pfdFontdlg.usWeight = 5 ;
        pfdFontdlg.fAttrs.usCodePage = 0;
        i1=0; 
        }
    sprintf( szPtList, "%d 8 10 12 14 18 24", iSize ) ;
    pfdFontdlg.pszPtSizeList = szPtList ;
    pfdFontdlg.fxPointSize = MAKEFIXED(iSize,0); 
    hwndFontDlg = WinFontDlg(HWND_DESKTOP, hwnd, &pfdFontdlg);
 
    if (hwndFontDlg && (pfdFontdlg.lReturn == DID_OK)) {
        iSize = FIXEDINT( pfdFontdlg.fxPointSize ) ;
        sprintf( szFontNameSize, "%d.%s", iSize, pfdFontdlg.fAttrs.szFacename ) ;
        return 1 ;
        }
    else return 0 ;
    }
