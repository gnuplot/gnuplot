#ifdef INCRCSDATA
static char RCSid[]="$Id: print.c%v 3.50 1993/07/09 05:35:24 woo Exp $" ;
#endif

/****************************************************************************

    PROGRAM: gnupmdrv
    
    Outboard PM driver for GNUPLOT 3.3

    MODULE:  print.c -- support for printing graphics under OS/2 
        
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

#define INCL_SPLDOSPRINT
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_DEV
#define INCL_SPL
#define INCL_PM
#define INCL_WIN
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include "gnupmdrv.h"

typedef struct {            /* for print thread parameters */
    HWND  hwnd ;
    char  szPrintFile[256] ;    /* file for printer output if not to printer */
    PPRQINFO3 piPrinter ;       /* print queue info */
    } PRINTPARAMS ;

static struct { 
    long    lTech ;     // printer technology
    long    lVer ;      // driver version
    long    lWidth ;    // page width in pels
    long    lHeight ;   // page height in pels
    long    lWChars ;   // page width in chars    
    long    lHChars ;   // page height in chars    
    long    lHorRes ;   // horizontal resolution pels / metre
    long    lVertRes ;  // vertical resolution pels / metre
    } prCaps ;

static float   flXFrac = (float) 0.6 ;   /* print area fractions */
static float   flYFrac = (float) 0.5 ;

static DRIVDATA     driv = {sizeof( DRIVDATA) } ;
static PDRIVDATA    pdriv = NULL ;
static char         szPrintFile[CCHMAXPATHCOMP] = {0} ;
static DEVOPENSTRUC devop ;

void            ThreadPrintPage( PRINTPARAMS* ) ;

MPARAM PrintCmdProc( HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
/*
**  Handle messages for print commands for 1- and 2-d spectra
** (i.e for the appropriate 1-and 2-d child windows )
*/
    {
    static BYTE abStack[4096] ;  
    static PRINTPARAMS tp ;
    static char szBusy[] = "Busy - try again later" ;
    static char szStart[] = "Printing started" ;
    static HEV semPrint = 0L ;
    char szTemp[32] ;
    long lErr ;
    PBYTE pStack = abStack;
    TID tid ;
    char *pszMess, *szPrinter ;

    if( semPrint == 0L ) {
        DosCreateMutexSem( NULL, &semPrint, 0L, 0L ) ;
        }

    switch( msg ) {
        
        case WM_USER_PRINT_BEGIN:
        
            if( DosRequestMutexSem( semPrint, SEM_IMMEDIATE_RETURN ) != 0 ) {
                pszMess = szBusy ;
                WinMessageBox( HWND_DESKTOP,
                               hWnd, 
                               pszMess,
                               APP_NAME,
                               0,
                               MB_OK | MB_ICONEXCLAMATION ) ;
                }
            else {
                pszMess = szStart ;
                tp.hwnd = hWnd ;
                tp.piPrinter = (PPRQINFO3) mp1 ;
                strcpy( tp.szPrintFile, szPrintFile ) ;
                DosCreateThread( &tid, (PFNTHREAD)ThreadPrintPage, (ULONG)&tp, 0L, 8192L ) ;
                }
            break ;


        case WM_USER_PRINT_OK :

            WinMessageBox( HWND_DESKTOP,
                           hWnd, 
                           "Print done",
                           APP_NAME,
                           0,
                           MB_OK | MB_ICONEXCLAMATION ) ;
             DosReleaseMutexSem( semPrint ) ;
             break ;

        case WM_USER_DEV_ERROR :

            lErr = ERRORIDERROR( (ERRORID) mp1 ) ;
            sprintf( szTemp, "Dev error: %d %x", lErr, lErr ) ;
            WinMessageBox( HWND_DESKTOP,
                           hWnd, 
                           szTemp,
                           APP_NAME,
                           0,
                           MB_OK | MB_ICONEXCLAMATION ) ;
             DosReleaseMutexSem( semPrint ) ;
             break ;

        case WM_USER_PRINT_ERROR :
        
            lErr = ERRORIDERROR( (ERRORID) mp1 ) ;
            sprintf( szTemp, "Print error: %d %x", lErr, lErr ) ;
            WinMessageBox( HWND_DESKTOP,
                           hWnd, 
                           szTemp,
                           APP_NAME,
                           0,
                           MB_OK | MB_ICONEXCLAMATION ) ;
             DosReleaseMutexSem( semPrint ) ;
             break ;

        case WM_USER_PRINT_QBUSY :

            return( (MPARAM)DosRequestMutexSem( semPrint, SEM_IMMEDIATE_RETURN ) ) ;
            
        default : break ;
        }
        
    return 0L ;
    }

int SetupPrinter( HWND hwnd, char *szPrintername, PPRQINFO3 pinfPrinter )
/*
**  Set up the printer
**
*/
    {
    QPRINT qp ;
    
    HDC hdc ;
        /* check that printer is still around .. */
    if( FindPrinter( szPrintername, pinfPrinter ) != 0 ) return 0 ;
    qp.piPrinter =  pinfPrinter  ;
        /* get printer capabilities */
    if( (hdc = OpenPrinterDC( WinQueryAnchorBlock( hwnd ), pinfPrinter, OD_INFO, NULL )) != DEV_ERROR ) {
        DevQueryCaps( hdc, CAPS_TECHNOLOGY, (long)sizeof(prCaps)/sizeof(long), (PLONG)&prCaps ) ;
        DevCloseDC( hdc ) ;
        qp.xsize = (float)100.0* (float) prCaps.lWidth / (float) prCaps.lHorRes ; // in cm
        qp.ysize = (float)100.0* (float) prCaps.lHeight / (float) prCaps.lVertRes ; // in cm
        qp.xfrac = flXFrac ;
        qp.yfrac = flYFrac ;
        qp.szFilename[0] = 0 ;
        szPrintFile[0] = 0 ;
        qp.caps  = prCaps.lTech & (CAPS_TECH_VECTOR_PLOTTER|CAPS_TECH_POSTSCRIPT) ?
                   QP_CAPS_FILE : QP_CAPS_NORMAL ;        
        if( WinDlgBox( HWND_DESKTOP,
                      hwnd,
                      (PFNWP)QPrintDlgProc,
                      0L,
                      ID_QPRINT,
                      &qp ) == DID_OK ) {
            flXFrac = qp.xfrac ;
            flYFrac = qp.yfrac ;
            if( qp.caps & QP_CAPS_FILE ) {
                if( qp.szFilename[0] != 0 ) strcpy( szPrintFile, qp.szFilename ) ;
                }
            return 1 ;
            }    
        }
    return 0 ;
    }

int SetPrinterMode( HWND hwnd, PPRQINFO3 pinfo )
/*
**  call up printer driver's own setup dialog box
**
**  returns :  -1 if error
**              0 if no settable modes
**              1 if OK
*/
    {
    HAB hab ;
    LONG lBytes ;
    
    hab = WinQueryAnchorBlock( hwnd ) ;
    lBytes = DevPostDeviceModes( hab,
                                 NULL,
                                 devop.pszDriverName,
                                 driv.szDeviceName,
                                 NULL,
                                 DPDM_POSTJOBPROP ) ;
    if( lBytes > 0L ) {
            /* if we have old pdriv data, and if its for the same printer,
               keep it to retain user's current settings, else get new */
        if( pdriv != NULL
        && strcmp( pdriv->szDeviceName, pinfo->pDriverData->szDeviceName ) != 0 ) {
            free( pdriv ) ;
            pdriv = NULL ;
            }
        if( pdriv == NULL ) {
            if( lBytes < pinfo->pDriverData->cb ) lBytes = pinfo->pDriverData->cb ;
            pdriv = malloc( lBytes ) ;
            memcpy( pdriv, pinfo->pDriverData, pinfo->pDriverData->cb ) ;
            }
        pdriv->szDeviceName[0] = '\0' ;  /* to check if 'cancel' selected */
        lBytes = DevPostDeviceModes( hab,
                                     pdriv,
                                     devop.pszDriverName,
                                     driv.szDeviceName,
                                     NULL,
                                     DPDM_POSTJOBPROP ) ;
        if( pdriv->szDeviceName[0] == '\0' ) {  /* could be: 'cancel' selected */
            lBytes = 0 ;
            free(pdriv ) ;   /* is this right ???? */
            pdriv = NULL ;
            }
        }
    return ( (int) lBytes ) ;
    }

void ThreadPrintPage( PRINTPARAMS *ptp )
/*
**  thread to set up printer DC and print page 
**
**  Input: THREADPARAMS *ptp -- pointer to thread data passed by beginthread
**
*/
    {    
    HAB         hab ;       // thread anchor block nandle
    HDC         hdc ;       // printer device context handle
    HPS         hps ;       // presentation space handle
    SHORT       msgRet ;    // message posted prior to return (end of thread)
    SIZEL       sizPage ;   // size of page for creation of presentation space
    LONG        alPage[2] ; // actual size of printer page in pixels
    RECTL       rectPage ;  // viewport on page into which we draw
    LONG        lColors ;
    char        *szPrintFile ;
    
    hab = WinInitialize( 0 ) ;
    
    szPrintFile = ptp->szPrintFile[0] == '\0' ? NULL : ptp->szPrintFile ;
    
    if( (hdc = OpenPrinterDC( hab, ptp->piPrinter, 0L, szPrintFile )) != DEV_ERROR ) {
    
            // create presentation space for printer

       sizPage.cx = 0 ;
       sizPage.cy = 0 ;
       hps = GpiCreatePS( hab,
                          hdc,
                          &sizPage,
                          PU_ARBITRARY | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC ) ;

       DevQueryCaps( hdc, CAPS_WIDTH, 2L, alPage ) ;
       DevQueryCaps( hdc, CAPS_PHYS_COLORS, 1L, &lColors ) ;
       rectPage.xLeft = 0L ;
       rectPage.xRight = alPage[0]*flXFrac ;
       rectPage.yTop = alPage[1]*flYFrac ;//alPage[1]*(1.0-flYFrac) ;
       rectPage.yBottom = 0L ; //  = alPage[1] ;
       
            // start printing
                    
        if( DevEscape( hdc, 
                       DEVESC_STARTDOC,
                       7L,
                       APP_NAME,
                       NULL,
                       NULL ) != DEVESC_ERROR ) {

            ScalePS( hps, &rectPage, 0 ) ;
            PlotThings( hps, lColors ) ;            
            DevEscape( hdc, DEVESC_ENDDOC, 0L, NULL, NULL, NULL ) ;
            msgRet = WM_USER_PRINT_OK ;
            }
        else
            msgRet = WM_USER_PRINT_ERROR ;
        
        GpiDestroyPS( hps ) ;
        DevCloseDC( hdc ) ;
        }
    else
        msgRet = WM_USER_DEV_ERROR ;
        
    DosEnterCritSec() ;
    WinPostMsg( ptp->hwnd, msgRet, (MPARAM)WinGetLastError(hab), 0L ) ;
    WinTerminate( hab ) ;
    }

HDC OpenPrinterDC( HAB hab, PPRQINFO3 pqinf, LONG lMode, char *szPrintFile )
/*
** get printer info from os2.ini and set up DC
**
** Input:  HAB hab  -- handle of anchor block of printing thread
**         PPRQINFO3-- pointer to data of current selected printer
**         LONG lMode -- mode in which device context is opened = OD_QUEUED, OD_DIRECT, OD_INFO
**         char *szPrintFile -- name of file for printer output, NULL
**                  if to printer (only used for devices that support file
**                  output eg plotter, postscript)
**
** Return: HDC      -- handle of printer device context
**                   = DEV_ERROR (=0) if error
*/
    {
    CHAR   *pchDelimiter ;
    LONG   lType ;
    static CHAR   achPrinterData[256] ;

    if( pqinf == NULL ) return DEV_ERROR ;
        
    strcpy( achPrinterData, pqinf->pszDriverName ) ;
    achPrinterData[ strcspn(achPrinterData,".") ] = '\0' ;

    devop.pszDriverName = achPrinterData ;
    devop.pszLogAddress = pqinf->pszName ;

    if( (pdriv != NULL ) ) devop.pdriv = pdriv ;
    else devop.pdriv = pqinf->pDriverData ;

    if( szPrintFile != NULL )  devop.pszLogAddress = szPrintFile ;
        
            // set data type to RAW
            
    devop.pszDataType = "PM_Q_RAW" ;
    
            // open device context
    if( lMode != 0L ) 
        lType = lMode ;
    else
        lType = (szPrintFile == NULL) ? OD_QUEUED: OD_DIRECT ;

    return DevOpenDC( hab, //  WinQueryAnchorBlock( hwnd ),
                      lType,
                      "*",
                      4L,
                      (PDEVOPENDATA) &devop,
                      NULLHANDLE ) ;
    }

int FindPrinter( char *szName, PPRQINFO3 piPrinter )
/*
**  Find a valid printer
*/
    {
    PPRQINFO3 pprq ;
    int np ;
    
    if( *szName && (strcmp( szName, piPrinter->pszName ) == 0) ) return 0 ;
    if( GetPrinters( &pprq , &np ) == 0 ) return 1 ;
    for( --np; np>=0; np-- ) {
        if( strcmp( szName, pprq[np].pszName ) == 0 ) {
            memcpy( piPrinter, &pprq[np], sizeof( PRQINFO3 ) ) ;
            free( pprq ) ;
            return 0 ;
            }
        }
    memcpy( piPrinter, &pprq[0], sizeof( PRQINFO3 ) ) ;
    free( pprq ) ;
    return 0 ;
    }
