#ifdef INCRCSDATA
static char RCSid[]="$Id: gnupmdrv.c%v 3.50 1993/07/09 05:35:24 woo Exp $" ;
#endif

/****************************************************************************

    PROGRAM: gnupmdrv
    
    Outboard PM driver for GNUPLOT 3.3

    MODULE:  gnupmdrv.c
        
    This file contains the startup procedures for gnupmdrv
       
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
#define INCL_DOSMEMMGR
#define INCL_DOSPROCESS
#define INCL_DOSFILEMGR
#include <os2.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gnupmdrv.h"

/*==== g l o b a l    d a t a ================================================*/

char szIPCName[256] ;
#define IPCDEFAULT "gnuplot"

/*==== l o c a l    d a t a ==================================================*/

            /* class names for window registration */

static char szChildName []     = "Gnuchild" ;

/*==== f u n c t i o n s =====================================================*/

BOOL             QueryIni( HAB ) ;
int              main( int, char** ) ;
static HWND      InitHelp( HAB, HWND ) ;

/*==== c o d e ===============================================================*/

int main ( int argc, char **argv )
/*
** args:  argv[1] : name to be used for IPC (pipes/semaphores) with gnuplot 
** 
** Standard PM initialisation:
** -- set up message processing loop
** -- register all window classes
** -- start up main window
** -- subclass main window for help and dde message trapping to frame window
** -- init help system
** -- check command line and open any filename found there
**
*/
    {
    static ULONG flFrameFlags = (FCF_ACCELTABLE|FCF_STANDARD);//&(~FCF_TASKLIST) ;
    static ULONG flClientFlags = WS_VISIBLE ;
    HMQ          hmq ;
    QMSG         qmsg ;
    PFNWP        pfnOldFrameWndProc ;
    HWND         hwndHelp ;        
    BOOL         bPos ;

    if( argc <= 1 ) strcpy( szIPCName, IPCDEFAULT ) ;
    else            strcpy( szIPCName, argv[1] ) ; 

    hab = WinInitialize( 0 ) ;    
    hmq = WinCreateMsgQueue( hab, 50 ) ;

                // get defaults from gnupmdrv.ini

    bPos = QueryIni( hab ) ;
                
                // register window and child window classes

    if( ! WinRegisterClass( hab,        /* Exit if can't register */
                            APP_NAME,
                            (PFNWP)DisplayClientWndProc,
                            CS_SIZEREDRAW,
                            0 ) 
                            ) return 0L ;

                // create main window

    hwndFrame = WinCreateStdWindow (
                    HWND_DESKTOP,
                    0,//WS_VISIBLE,
                    &flFrameFlags,
                    APP_NAME,
                    NULL,
                    flClientFlags,
                    0L,
                    1,
                    &hApp) ;

    if ( ! hwndFrame ) return NULL ;

                // subclass window for help & DDE trapping

    pfnOldFrameWndProc = WinSubclassWindow( hwndFrame, (PFNWP)NewFrameWndProc ) ;
    WinSetWindowULong( hwndFrame, QWL_USER, (ULONG) pfnOldFrameWndProc ) ;

                // init the help manager

    hwndHelp = InitHelp( hab, hwndFrame ) ;        

                // set window title and make it active

    WinSetWindowText( hwndFrame, APP_NAME ) ;
        
                // process window messages 
      
    while (WinGetMsg (hab, &qmsg, NULL, 0, 0))
         WinDispatchMsg (hab, &qmsg) ;

                // shut down

    WinDestroyHelpInstance( hwndHelp ) ;
    WinDestroyWindow (hwndFrame) ;
    WinDestroyMsgQueue (hmq) ;
    WinTerminate (hab) ;

    return 0 ;
    }

static HWND InitHelp( HAB hab, HWND hwnd )
/*
**  initialise the help system
*/
    {
    static HELPINIT helpinit = { sizeof(HELPINIT),
                                 0L,
                                 NULL,
                                 (PHELPTABLE)MAKELONG(1, 0xFFFF),
                                 0L,
                                 0L,
                                 0,
                                 0,
                                 "GnuplotPM Help",
                                 CMIC_HIDE_PANEL_ID,
                                 "gnupmdrv.hlp" } ;
    HWND hwndHelp ;
    
    hwndHelp = WinCreateHelpInstance( hab, &helpinit ) ;
    WinAssociateHelpInstance( hwndHelp, hwnd ) ;
    return hwndHelp ;
    }

MRESULT EXPENTRY NewFrameWndProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
/*
**  Subclasses top-level frame window to trap help & dde messages
*/
    {
    PFNWP       pfnOldFrameWndProc ;
    
    pfnOldFrameWndProc = (PFNWP) WinQueryWindowULong( hwnd, QWL_USER ) ; 
    switch( msg ) {
        default: 
            break ;

        case HM_QUERY_KEYS_HELP:
            return (MRESULT) IDH_KEYS ;            
        }
    return (*pfnOldFrameWndProc)(hwnd, msg, mp1, mp2) ;    
    }


MRESULT EXPENTRY About( HWND hDlg, ULONG message, MPARAM mp1, MPARAM mp2)
/*
** 'About' box dialog function
*/
    {
  /*  switch (message) {

        case WM_COMMAND:
            if (SHORT1FROMMP(mp1) == DID_OK) {
                WinDismissDlg( hDlg, DID_OK );
                return 0L;
                }
            break;
        }*/
    return WinDefDlgProc( hDlg, message, mp1, mp2 ) ;
    }

