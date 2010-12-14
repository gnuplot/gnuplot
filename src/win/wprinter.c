#ifndef lint
static char *RCSid() { return RCSid("$Id: wprinter.c,v 1.5 2005/08/03 16:55:40 mikulik Exp $"); }
#endif

/* GNUPLOT - win/wprinter.c */
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

/* Dump a file to the printer */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define STRICT
#include <windows.h>
#include <windowsx.h>
#if WINVER >= 0x030a
#include <commdlg.h>
#endif
#ifdef WIN32
#include <stdio.h>
#include <stdlib.h>
#endif
#ifdef __MSC__
#include <memory.h>
#else
#include <mem.h>
#endif
#include "wgnuplib.h"
#include "wresourc.h"
#include "wcommon.h"

GP_LPPRINT prlist = NULL;

BOOL CALLBACK WINEXPORT PrintSizeDlgProc(HWND hdlg, UINT wmsg, WPARAM wparam, LPARAM lparam);

BOOL CALLBACK WINEXPORT
PrintSizeDlgProc(HWND hdlg, UINT wmsg, WPARAM wparam, LPARAM lparam)
{
    char buf[8];
    GP_LPPRINT lpr = (GP_LPPRINT)GetWindowLong(GetParent(hdlg), 4);

    switch (wmsg) {
    case WM_INITDIALOG:
	wsprintf(buf,"%d",lpr->pdef.x);
	SetDlgItemText(hdlg, PSIZE_DEFX, buf);
	wsprintf(buf,"%d",lpr->pdef.y);
	SetDlgItemText(hdlg, PSIZE_DEFY, buf);
	wsprintf(buf,"%d",lpr->poff.x);
	SetDlgItemText(hdlg, PSIZE_OFFX, buf);
	wsprintf(buf,"%d",lpr->poff.y);
	SetDlgItemText(hdlg, PSIZE_OFFY, buf);
	wsprintf(buf,"%d",lpr->psize.x);
	SetDlgItemText(hdlg, PSIZE_X, buf);
	wsprintf(buf,"%d",lpr->psize.y);
	SetDlgItemText(hdlg, PSIZE_Y, buf);
	CheckDlgButton(hdlg, PSIZE_DEF, TRUE);
	EnableWindow(GetDlgItem(hdlg, PSIZE_X), FALSE);
	EnableWindow(GetDlgItem(hdlg, PSIZE_Y), FALSE);
	return TRUE;
    case WM_COMMAND:
	switch (wparam) {
	case PSIZE_DEF:
	    EnableWindow(GetDlgItem(hdlg, PSIZE_X), FALSE);
	    EnableWindow(GetDlgItem(hdlg, PSIZE_Y), FALSE);
	    return FALSE;
	case PSIZE_OTHER:
	    EnableWindow(GetDlgItem(hdlg, PSIZE_X), TRUE);
	    EnableWindow(GetDlgItem(hdlg, PSIZE_Y), TRUE);
	    return FALSE;
	case IDOK:
	    if (SendDlgItemMessage(hdlg, PSIZE_OTHER, BM_GETCHECK, 0, 0L)) {
		SendDlgItemMessage(hdlg, PSIZE_X, WM_GETTEXT, 7,
				   (LPARAM) (LPSTR) buf);
		GetInt(buf, (LPINT)&lpr->psize.x);
		SendDlgItemMessage(hdlg, PSIZE_Y, WM_GETTEXT, 7,
				   (LPARAM) (LPSTR) buf);
		GetInt(buf, (LPINT)&lpr->psize.y);
	    } else {
		lpr->psize.x = lpr->pdef.x;
		lpr->psize.y = lpr->pdef.y;
	    }
	    SendDlgItemMessage(hdlg, PSIZE_OFFX, WM_GETTEXT, 7,
			       (LPARAM) (LPSTR) buf);
	    GetInt(buf, (LPINT)&lpr->poff.x);
	    SendDlgItemMessage(hdlg, PSIZE_OFFY, WM_GETTEXT, 7,
			       (LPARAM) (LPSTR) buf);
	    GetInt(buf, (LPINT)&lpr->poff.y);

	    if (lpr->psize.x <= 0)
		lpr->psize.x = lpr->pdef.x;
	    if (lpr->psize.y <= 0)
		lpr->psize.y = lpr->pdef.y;

	    EndDialog(hdlg, IDOK);
	    return TRUE;
	case IDCANCEL:
	    EndDialog(hdlg, IDCANCEL);
	    return TRUE;
	} /* switch(wparam) */
	break;
    } /* switch(msg) */
    return FALSE;
}



/* GetWindowLong(hwnd, 4) must be available for use */
BOOL
PrintSize(HDC printer, HWND hwnd, LPRECT lprect)
{
    HDC hdc;
#ifndef WIN32
    DLGPROC lpfnPrintSizeDlgProc;
#endif
    BOOL status = FALSE;
    GP_PRINT pr;

    SetWindowLong(hwnd, 4, (LONG)((GP_LPPRINT)&pr));
    pr.poff.x = 0;
    pr.poff.y = 0;
    pr.psize.x = GetDeviceCaps(printer, HORZSIZE);
    pr.psize.y = GetDeviceCaps(printer, VERTSIZE);
    hdc = GetDC(hwnd);
    GetClientRect(hwnd,lprect);
    pr.pdef.x = MulDiv(lprect->right-lprect->left, 254, 10*GetDeviceCaps(hdc, LOGPIXELSX));
    pr.pdef.y = MulDiv(lprect->bottom-lprect->top, 254, 10*GetDeviceCaps(hdc, LOGPIXELSX));
    ReleaseDC(hwnd,hdc);

#ifdef WIN32
    if (DialogBox (hdllInstance, "PrintSizeDlgBox", hwnd, PrintSizeDlgProc)
	== IDOK)
#else
# ifdef __DLL__
    lpfnPrintSizeDlgProc = (DLGPROC) GetProcAddress(hdllInstance,
						    "PrintSizeDlgProc");
# else
    lpfnPrintSizeDlgProc = (DLGPROC) MakeProcInstance(
	(FARPROC) PrintSizeDlgProc, hdllInstance);
# endif
    if (DialogBox (hdllInstance, "PrintSizeDlgBox", hwnd, lpfnPrintSizeDlgProc)
	== IDOK)
#endif
	{
	    lprect->left = MulDiv(pr.poff.x*10, GetDeviceCaps(printer, LOGPIXELSX), 254);
	    lprect->top = MulDiv(pr.poff.y*10, GetDeviceCaps(printer, LOGPIXELSY), 254);
	    lprect->right = lprect->left + MulDiv(pr.psize.x*10, GetDeviceCaps(printer, LOGPIXELSX), 254);
	    lprect->bottom = lprect->top + MulDiv(pr.psize.y*10, GetDeviceCaps(printer, LOGPIXELSY), 254);
	    status = TRUE;
	}
#ifndef WIN32
# ifndef __DLL__
    FreeProcInstance((FARPROC)lpfnPrintSizeDlgProc);
# endif
#endif
    SetWindowLong(hwnd, 4, (LONG)(0L));

    return status;
}

#ifdef WIN32
/* Win32 doesn't support OpenJob() etc. so we must use some old code
 * which attempts to sneak the output through a Windows printer driver */
void
PrintRegister(GP_LPPRINT lpr)
{
    GP_LPPRINT next;
    next = prlist;
    prlist = lpr;
    lpr->next = next;
}

GP_LPPRINT
PrintFind(HDC hdc)
{
    GP_LPPRINT this;
    this = prlist;
    while (this && (this->hdcPrn!=hdc)) {
	this = this->next;
    }
    return this;
}

void
PrintUnregister(GP_LPPRINT lpr)
{
    GP_LPPRINT this, prev;
    prev = (GP_LPPRINT)NULL;
    this = prlist;
    while (this && (this!=lpr)) {
	prev = this;
	this = this->next;
    }
    if (this && (this == lpr)) {
	/* unhook it */
	if (prev)
	    prev->next = this->next;
	else
	    prlist = this->next;
    }
}


/* GetWindowLong(GetParent(hDlg), 4) must be available for use */
BOOL CALLBACK WINEXPORT
PrintDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    GP_LPPRINT lpr;
    lpr = (GP_LPPRINT)GetWindowLong(GetParent(hDlg), 4);

    switch(message) {
    case WM_INITDIALOG:
	lpr->hDlgPrint = hDlg;
	SetWindowText(hDlg,(LPSTR)lParam);
	EnableMenuItem(GetSystemMenu(hDlg,FALSE),SC_CLOSE,MF_GRAYED);
	return TRUE;
    case WM_COMMAND:
	lpr->bUserAbort = TRUE;
	lpr->hDlgPrint = 0;
	EnableWindow(GetParent(hDlg),TRUE);
	EndDialog(hDlg, FALSE);
	return TRUE;
    }
    return FALSE;
}


BOOL CALLBACK WINEXPORT
PrintAbortProc(HDC hdcPrn, int code)
{
    MSG msg;
    GP_LPPRINT lpr;
    lpr = PrintFind(hdcPrn);

    while (!lpr->bUserAbort && PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		if (!lpr->hDlgPrint || !IsDialogMessage(lpr->hDlgPrint,&msg)) {
        	TranslateMessage(&msg);
        	DispatchMessage(&msg);
		}
    }
    return(!lpr->bUserAbort);
}


/* GetWindowLong(hwnd, 4) must be available for use */
void WDPROC
DumpPrinter(HWND hwnd, LPSTR szAppName, LPSTR szFileName)
{
    HDC printer;
    PRINTDLG pd;
    GP_PRINT pr;
    DOCINFO di;
    char *buf;
    WORD *bufcount;
    int count;
    FILE *f;
    long lsize;
    long ldone;
    char pcdone[10];

    memset(&pd, 0, sizeof(PRINTDLG));
    pd.lStructSize = sizeof(PRINTDLG);
    pd.hwndOwner = hwnd;
    pd.Flags = PD_PRINTSETUP | PD_RETURNDC;

    if ((f = fopen(szFileName, "rb")) == (FILE *)NULL)
	return;
    fseek(f, 0L, SEEK_END);
    lsize = ftell(f);
    if (lsize <= 0)
	lsize = 1;
    fseek(f, 0L, SEEK_SET);
    ldone = 0;

    if (PrintDlg(&pd)) {
	printer = pd.hDC;
	if (printer != (HDC)NULL) {
	    pr.hdcPrn = printer;
	    SetWindowLong(hwnd, 4, (LONG)((GP_LPPRINT)&pr));
	    PrintRegister((GP_LPPRINT)&pr);
	    if ( (buf = malloc(4096+2)) != (char *)NULL ) {
	    	bufcount = (WORD *)buf;
		EnableWindow(hwnd,FALSE);
		pr.bUserAbort = FALSE;
/* is parent set correctly */
		pr.hDlgPrint = CreateDialogParam(hdllInstance,
						 "CancelDlgBox",
						 hwnd, PrintDlgProc,
						 (LPARAM) szAppName);
		SetAbortProc(printer, PrintAbortProc);
		di.cbSize = sizeof(DOCINFO);
		di.lpszDocName = szAppName;
		di.lpszOutput = NULL;
		if (StartDoc(printer, &di) > 0) {
		    while ( pr.hDlgPrint && !pr.bUserAbort
			    && (count = fread(buf+2, 1, 4096, f)) != 0 ) {
			*bufcount = count;
			Escape(printer, PASSTHROUGH, count+2, (LPSTR)buf, NULL);
	    		ldone += count;
	    		sprintf(pcdone, "%d%% done",
				(int) (ldone * 100 / lsize));
	    		SetWindowText(GetDlgItem(pr.hDlgPrint, CANCEL_PCDONE),
				      pcdone);
			if (pr.bUserAbort)
			    AbortDoc(printer);
			else
			    EndDoc(printer);
		    }
		    if (!pr.bUserAbort) {
			EnableWindow(hwnd,TRUE);
			DestroyWindow(pr.hDlgPrint);
		    }
		    free(buf);
		}
	    }
	    DeleteDC(printer);
	    SetWindowLong(hwnd, 4, (LONG)(0L));
	    PrintUnregister((GP_LPPRINT)&pr);
	}
    }
    fclose(f);
}

#else  /* !WIN32 */

/* documented in Device Driver Adaptation Guide */
/* Prototypes taken from print.h */
 DECLARE_HANDLE(HPJOB);

HPJOB   WINAPI OpenJob(LPSTR, LPSTR, HPJOB);
int     WINAPI StartSpoolPage(HPJOB);
int     WINAPI EndSpoolPage(HPJOB);
int     WINAPI WriteSpool(HPJOB, LPSTR, int);
int     WINAPI CloseJob(HPJOB);
int     WINAPI DeleteJob(HPJOB, int);
int     WINAPI WriteDialog(HPJOB, LPSTR, int);
int     WINAPI DeleteSpoolPage(HPJOB);

/* Modeless dialog box - Cancel printing */
BOOL CALLBACK WINEXPORT
CancelDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message) {
    case WM_INITDIALOG:
	SetWindowText(hDlg,(LPSTR)lParam);
	return TRUE;
    case WM_COMMAND:
	switch(LOWORD(wParam)) {
	case IDCANCEL:
	    DestroyWindow(hDlg);
	    return TRUE;
	}
    }
    return FALSE;
}



/* Dialog box to select printer port */
BOOL CALLBACK WINEXPORT
SpoolDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPSTR entry;

    switch(message) {
    case WM_INITDIALOG:
	entry = (LPSTR)lParam;
	while (*entry) {
	    SendDlgItemMessage(hDlg, SPOOL_PORT, LB_ADDSTRING, 0, (LPARAM)entry);
	    entry += lstrlen(entry)+1;
	}
	SendDlgItemMessage(hDlg, SPOOL_PORT, LB_SETCURSEL, 0, (LPARAM)0);
	return TRUE;
    case WM_COMMAND:
	switch(LOWORD(wParam)) {
	case SPOOL_PORT:
	    if (HIWORD(lParam) == LBN_DBLCLK)
		PostMessage(hDlg, WM_COMMAND, IDOK, 0L);
	    return FALSE;
	case IDOK:
	    EndDialog(hDlg, 1+(int)SendDlgItemMessage(hDlg, SPOOL_PORT, LB_GETCURSEL, 0, 0L));
	    return TRUE;
	case IDCANCEL:
	    EndDialog(hDlg, 0);
	    return TRUE;
	}
    }
    return FALSE;
}

/* Print File to port */
void WDPROC
DumpPrinter(HWND hwnd, LPSTR szAppName, LPSTR szFileName)
{
#define PRINT_BUF_SIZE 4096
    char *buffer;
    char *portname;
    int i, iport;
    DLGPROC lpfnSpoolProc;
    HPJOB hJob;
    UINT count;
    HFILE hf;
    int error = FALSE;
    DLGPROC lpfnCancelProc;
    long lsize;
    long ldone;
    char pcdone[10];
    MSG msg;
    HWND hDlgModeless;

    if ((buffer = LocalAllocPtr(LHND, PRINT_BUF_SIZE)) == (char *)NULL)
	return;
    /* get list of ports */
    GetProfileString("ports", NULL, "", buffer, PRINT_BUF_SIZE);
    /* select a port */
    lpfnSpoolProc = (DLGPROC)MakeProcInstance((FARPROC)SpoolDlgProc, hdllInstance);
    iport = DialogBoxParam(hdllInstance, "SpoolDlgBox", hwnd, lpfnSpoolProc, (LPARAM)buffer);
    FreeProcInstance((FARPROC)lpfnSpoolProc);
    if (!iport) {
	LocalFreePtr((void NEAR *)buffer);
	return;
    }
    portname = buffer;
    for (i=1; i<iport && lstrlen(portname)!=0; i++)
	portname += lstrlen(portname)+1;

    /* open file and get length */
    hf = _lopen(szFileName, OF_READ);
    if (hf == HFILE_ERROR) {
	LocalFreePtr((void NEAR *)buffer);
	return;
    }
    lsize = _llseek(hf, 0L, 2);
    (void) _llseek(hf, 0L, 0);
    if (lsize <= 0)
	lsize = 1;

    hJob = OpenJob(portname, szFileName, (HDC)NULL);
    switch ((int)hJob) {
    case SP_APPABORT:
    case SP_ERROR:
    case SP_OUTOFDISK:
    case SP_OUTOFMEMORY:
    case SP_USERABORT:
	_lclose(hf);
	LocalFreePtr((void NEAR *)buffer);
	return;
    }
    if (StartSpoolPage(hJob) < 0)
	error = TRUE;

    ldone = 0;
    lpfnCancelProc = (DLGPROC)MakeProcInstance((FARPROC)CancelDlgProc, hdllInstance);
    hDlgModeless = CreateDialogParam(hdllInstance, "CancelDlgBox", hwnd, lpfnCancelProc, (LPARAM)szAppName);

    while (!error && hDlgModeless && IsWindow(hDlgModeless)
	   && ((count = _lread(hf, buffer, PRINT_BUF_SIZE))!= 0) ) {
	wsprintf(pcdone, "%d%% done", (int)(ldone * 100 / lsize));
	SetWindowText(GetDlgItem(hDlgModeless, CANCEL_PCDONE), pcdone);
	if (WriteSpool(hJob, buffer, count) < 0)
	    error = TRUE;
	ldone += count;
	while (IsWindow(hDlgModeless) && PeekMessage(&msg, hDlgModeless, 0, 0, PM_REMOVE)) {
	    if (!IsDialogMessage(hDlgModeless, &msg)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	    }
	}
    }
    LocalFreePtr((void NEAR *)buffer);
    _lclose(hf);

    if (!hDlgModeless || !IsWindow(hDlgModeless))
	error=TRUE;
    if (IsWindow(hDlgModeless))
	DestroyWindow(hDlgModeless);
    hDlgModeless = 0;
    FreeProcInstance((FARPROC)lpfnCancelProc);
    EndSpoolPage(hJob);
    if (error)
	DeleteJob(hJob, 0);
    else
	CloseJob(hJob);
}
#endif /* !WIN32 */

