#ifdef INCRCSDATA
static char RCSid[]="$Id: gclient.c,v 1.15 1998/03/22 22:34:21 drd Exp $" ;
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
#include <stdlib.h>
#include <unistd.h>
#include <process.h>
#include <signal.h>
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
static BOOL     bNewFont = FALSE ;
static BOOL     bHorz = TRUE ;

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

            /* font data */

static int lSupOffset = 0 ;
static int lSubOffset = 0 ;
static int lBaseSupOffset = 0 ;
static int lBaseSubOffset = 0 ;
static int lCharWidth = 217 ;
static int lCharHeight = 465 ; 


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
            }
            break ;
            
        case WM_GPSTART:

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
            USHORT  usFlag ;
            usFlag = SHORT1FROMMP( mp1 ) ;
            if(  !(usFlag & (KC_KEYUP|KC_ALT|KC_CTRL)) ) {
                USHORT uc = SHORT1FROMMP( mp2 ) ;
                    HWND hw = WinQueryWindow( swGnu.hwnd, QW_BOTTOM ) ;
                    WinSetFocus( HWND_DESKTOP, hw ) ; 
                WinSendMsg( hw, message, 
                            MPFROM2SHORT((USHORT)(KC_SCANCODE), 1), 
                            MPFROMSHORT(uc) ) ;
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
              hps_tmp = WinBeginPaint(hWnd,0,&rectl_tmp );
              WinFillRect(hps_tmp,&rectl_tmp,CLR_BACKGROUND);
              WinEndPaint(hps_tmp);
                /* add dirty rectangle to saved rectangle     */
                /* to be repainted when PS is available again */
              WinUnionRect(hab,&rectlPaint,&rectl_tmp,&rectlPaint);
                iPaintCount ++ ;
                break ;
                } 
            WinInvalidateRect( hWnd, &rectlPaint, TRUE ) ;
            DoPaint( hWnd, hpsScreen ) ;
            WinSetRectEmpty( hab, &rectlPaint ) ;
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
            if( bPopFront )
                WinSetWindowPos( hwndFrame, HWND_TOP, 0,0,0,0, SWP_ACTIVATE|SWP_ZORDER ) ;
            if( iPaintCount > 0 ) { /* if outstanding paint messages, repaint */
                WinInvalidateRect( hWnd, &rectlPaint, TRUE ) ;
                iPaintCount = 0 ;
                }
            return 0L ;

        case WM_PAUSEPLOT:
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


        case IDM_GPLOTINF:  /* view gnuplot.inf */
            {
	    /* should be bigger or dynamic */
            char path[256] ;
            strcpy( path, "start view " ) ;
            if( user_homedir != NULL ) {
                strcat( path, user_homedir ) ;
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
**  Copy ps to a mteafile.
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
//        DosPostEventSem( semStartSeq ) ;         /* once we've got pidGnu */
        WinPostMsg( hApp, WM_GPSTART, 0, 0 ) ;


        hps = hpsScreen ;
   InitScreenPS() ;
        while (1) {
        
            usErr=BufRead(hRead,buff, 1, &cbR) ;
            if( usErr != 0 ) break ;
 
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
                    }
                    break ;
                    
                case 'Q' :     /* query terminal info */
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

