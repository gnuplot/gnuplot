/*
 * $Id: wcommon.h,v 1.6 1996/09/15 13:08:41 drd Exp $
 */

/* GNUPLOT - wcommon.h */
/*
 * Copyright (C) 1992   Maurice Castro, Russell Lang
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
 *   Maurice Castro
 *   Russell Lang
 * 
 * Send your comments or suggestions to 
 *  info-gnuplot@dartmouth.edu.
 * This is a mailing list; to join it send a note to 
 *  majordomo@dartmouth.edu.  
 * Send bug reports to
 *  bug-gnuplot@dartmouth.edu.
 */

#if WINVER >= 0x030a
#include <shellapi.h>
#endif
/* this file contains items that are internal to wgnuplot.dll */


/* wgnuplib.c */
extern HINSTANCE hdllInstance;
extern LPSTR szParentClass;
extern LPSTR szTextClass;
extern LPSTR szPauseClass;
extern LPSTR szGraphClass;
extern LPSTR szAboutClass;

void NEAR * LocalAllocPtr(UINT flags, UINT size);
void LocalFreePtr(void NEAR *ptr);
LPSTR GetInt(LPSTR str, LPINT pval);

/* wtext.c */
void UpdateText(LPTW, int);
void CreateTextClass(LPTW lptw);
void NewLine(LPTW);
void TextPutStr(LPTW lptw, LPSTR str);
void WriteTextIni(LPTW lptw);
void ReadTextIni(LPTW lptw);
#if WINVER >= 0x030a
void DragFunc(LPTW lptw, HDROP hdrop);
#endif

/* wmenu.c - Menu */
void SendMacro(LPTW lptw, UINT m);
void LoadMacros(LPTW lptw);
void CloseMacros(LPTW lptw);

/* wprinter.c - Printer setup and dump */
BOOL PrintSize(HDC printer, HWND hwnd, LPRECT lprect);
void PrintRegister(LPPRINT lpr);
void PrintUnregister(LPPRINT lpr);
#if WINVER >= 0x030a
BOOL CALLBACK WINEXPORT PrintAbortProc(HDC hdcPrn, int code);
BOOL CALLBACK WINEXPORT PrintDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
#endif

/* wgraph.c */

