/* gnushell header file */
/*
** static char RCSid[]="$Id: gnupmdrv.h%v 3.50 1993/07/09 05:35:24 woo Exp $" ;
*/

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

    /* include resource defines */
    
#ifndef DISPDEFS_H
/*#include "dispdefs.h"*/
#include "dialogs.h"
#endif

/*==== own window messages  =================================================*/

#define WM_GNUPLOT          (WM_USER+20)
#define WM_PAUSEPLOT        (WM_USER+21)
#define WM_PAUSEEND         (WM_USER+22)
#define WM_USER_SET_DATA    (WM_USER+90)
#define WM_USER_GET_DATA    (WM_USER+91)
#define WM_USER_CHGFONT     (WM_USER+10) 
#define WM_USER_PRINT_BEGIN (WM_USER+200)
#define WM_USER_PRINT_OK    (WM_USER+201)
#define WM_USER_PRINT_ERROR (WM_USER+202)
#define WM_USER_DEV_ERROR   (WM_USER+203)
#define WM_USER_PRINT_QBUSY (WM_USER+204)

/*==== various names ========================================================*/

#define GNUPIPE     "\\pipe\\gnuplot"       /* named pipe to gnuplot */
#define GNUQUEUE    "\\queues\\gnuplot"     /* queue for gnuplot termination */
#define GNUSEM      "\\sem32\\gnuplot.sem"  /* synch gnuplot and gnupmdrv */
#define GNUINI      "gnupmdrv.ini"          /* ini filename */
#define ENVGNUHELP  "GNUHELP"               /* gnuplot help envionment name */
#define ENVGNUPLOT  "GNUPLOT"               /* general gnuplot environment */
#define GNUEXEFILE  "gnuplot.exe"           /* exe file name */
#define GNUHELPFILE "gnuplot.gih"           /* help file name */
#define GNUTERMINIT "GNUTERM=pm"            /* terminal setup string */
#define INITIAL_FONT "12.Helvetica"         /* initial font for plots */
#define APP_NAME     "GnuplotPM"             /* application name */

        // profile (ini file) names 
#define INISHELLPOS  "PosShell"
#define INIPLOTPOS   "PosPlot"
#define INIFONT      "DefFont" 
#define INIOPTS      "DefOpts"      

/*==== global data  ==========================================================*/

HAB         hab ;                   // application anchor block handle 
HWND   	    hApp ;                  // application window handle 
HWND        hwndFrame ;             // frame window handle 
#define   FONTBUF   256         /* buffer for dropped font namesize */

/*==== stuff for querying printer capability =================================*/

typedef struct {  //query data for printer setup
    float xsize ;
    float ysize ;
    float xfrac ;
    float yfrac ;
    short caps ;
    char  szFilename[CCHMAXPATHCOMP] ;
    PPRQINFO3 piPrinter ;
    } QPRINT, *PQPRINT ;

#define QP_CAPS_NORMAL 0
#define QP_CAPS_FILE   1   /* can print to file */

/*==== function declarations =================================================*/

short            ScalePS( HPS, PRECTL, USHORT ) ;
void             PlotThings( HPS, long ) ;
int              SetupPrinter( HWND, char*, PPRQINFO3 ) ;
HDC              OpenPrinterDC( HAB, PPRQINFO3, LONG, char* ) ;
int              SetPrinterMode( HWND, PPRQINFO3 ) ;
MPARAM           PrintCmdProc( HWND, ULONG, MPARAM, MPARAM ) ;
MRESULT EXPENTRY PrintDlgProc( HWND, ULONG, MPARAM, MPARAM ) ;
MRESULT EXPENTRY PauseMsgDlgProc( HWND, ULONG, MPARAM, MPARAM ) ;
MRESULT EXPENTRY QFontDlgProc( HWND ,ULONG, MPARAM, MPARAM ) ;
MRESULT EXPENTRY QPrintDlgProc (HWND, ULONG, MPARAM, MPARAM) ;
MRESULT EXPENTRY QPrintersDlgProc ( HWND, ULONG, MPARAM, MPARAM ) ;
MRESULT EXPENTRY DisplayClientWndProc(HWND, ULONG, MPARAM, MPARAM);
MRESULT EXPENTRY NewFrameWndProc(HWND, ULONG, MPARAM, MPARAM) ;
MRESULT EXPENTRY About(HWND, ULONG, MPARAM, MPARAM);

        /* own window functions... */
void WinSetDlgItemFloat( HWND, USHORT, float ) ;
void WinSetDlgItemFloatF( HWND, USHORT, int, float ) ;
void WinQueryDlgItemFloat( HWND, USHORT, float* ) ;


