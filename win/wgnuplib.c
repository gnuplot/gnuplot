#ifndef lint
static char *RCSid = "$Id: wgnuplib.c,v 1.4 1996/09/15 13:08:41 drd Exp $";
#endif

/* GNUPLOT - win/wgnuplib.c */
/*
 * Copyright (C) 1992   Russell Lang
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
 *   Russell Lang
 * 
 * Send your comments or suggestions to 
 *  info-gnuplot@dartmouth.edu.
 * This is a mailing list; to join it send a note to 
 *  majordomo@dartmouth.edu.  
 * Send bug reports to
 *  bug-gnuplot@dartmouth.edu.
 */

#define STRICT
#include <ctype.h>
#include <windows.h>
#include <windowsx.h>
#include "wgnuplib.h"
#include "wresourc.h"
#include "wcommon.h"

HINSTANCE hdllInstance;
LPSTR szParentClass = "wgnuplot_parent";
LPSTR szTextClass = "wgnuplot_text";
LPSTR szPauseClass = "wgnuplot_pause";
LPSTR szGraphClass = "wgnuplot_graph";

/* Window ID */
struct WID {
	BOOL       used;
	HWND       hwnd;
	void FAR * ptr;
};
struct WID *widptr = NULL;
unsigned int nwid = 0;
HLOCAL hwid = 0;

#ifdef __DLL__
int WDPROC
LibMain(HINSTANCE hInstance, WORD wDataSeg, WORD wHeapSize, LPSTR lpszCmdLine)
{
	hdllInstance = hInstance;
	return 1;
}

int WDPROC
WEP(int nParam)
{
	return 1;
}

BOOL WDPROC
CheckWGNUPLOTVersion(LPSTR str)
{
char mess[256];
LPSTR version;
	version = WGNUPLOTVERSION;
	if (lstrcmp(str,version)) {
		wsprintf(mess,"Incorrect DLL version\nExpected version   %s\nThis is version   %s",str,version);
		MessageBox(NULL, mess , "WGNUPLOT.DLL", MB_OK | MB_ICONSTOP | MB_TASKMODAL);
		return TRUE;
	}
	return FALSE;	/* Correct version */
}
#endif /* __DLL__ */

void NEAR *
LocalAllocPtr(UINT flags, UINT size)
{
HLOCAL hlocal;
	hlocal = LocalAlloc(flags, size+1);
	return (char *)LocalLock(hlocal);
}

void
LocalFreePtr(void NEAR *ptr)
{
HLOCAL hlocal;
	hlocal = LocalHandle(ptr);
	LocalUnlock(hlocal);
	LocalFree(hlocal);
	return;
}


/* ascii to int */
/* returns:
 *  A pointer to character past int if successful,
 *  otherwise NULL on failure.
 *  convert int is stored at pval.
 */
LPSTR
GetInt(LPSTR str, LPINT pval)
{
int val = 0;
BOOL negative = FALSE;
BOOL success = FALSE;
int ch;
	if (!str)
		return NULL;
	while ( (ch = *str)!=0 && isspace(ch) )
		str++;
	if (ch == '-') {
		negative = TRUE;
		str++;
	}
	while ( (ch = *str)!=0 && isdigit(ch) ) {
		success = TRUE;
		val = val * 10 + (ch - '0');
		str++;
	}
	if (success) {
		if (negative)
			val = -val;
		*pval = val;
		return str;
	}
	return NULL;
}

