/* GNUPLOT - wcommon.h */

/*[
 * Copyright 1992 - 1993, 1998, 2004 Maurice Castro, Russell Lang
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

#ifndef GNUPLOT_WCOMMON_H
#define GNUPLOT_WCOMMON_H

#include "winmain.h"

#ifndef CLEARTYPE_QUALITY
#define CLEARTYPE_QUALITY       5
#endif

/* maximum number of plots which can be enabled/disabled via toolbar */
#define MAXPLOTSHIDE 10

#ifdef __cplusplus
extern "C" {
#endif

/* winmain.c */
# define PACKVERSION(major,minor) MAKELONG(minor,major)
extern DWORD GetDllVersion(LPCTSTR lpszDllName);
extern BOOL IsWindowsXPorLater(void);
extern char *appdata_directory(void);
extern FILE *open_printer(void);
extern void close_printer(FILE *outfile);
extern BOOL cp_changed;
extern UINT cp_input;
extern UINT cp_output;

/* wgnuplib.c */
extern HINSTANCE hdllInstance;
extern LPWSTR szParentClass;
extern LPWSTR szTextClass;
extern LPWSTR szToolbarClass;
extern LPWSTR szSeparatorClass;
extern LPWSTR szPauseClass;
extern LPTSTR szAboutClass;

void * LocalAllocPtr(UINT flags, UINT size);
void * LocalReAllocPtr(void * ptr, UINT flags, UINT size);
void LocalFreePtr(void *ptr);
LPTSTR GetInt(LPTSTR str, LPINT pval);

/* wtext.c */
#ifndef WGP_CONSOLE
void WriteTextIni(LPTW lptw);
void ReadTextIni(LPTW lptw);
void DragFunc(LPTW lptw, HDROP hdrop);
void TextShow(LPTW lptw);
void TextUpdateStatus(LPTW lptw);
void TextSuspend(LPTW lptw);
void TextResume(LPTW lptw);
void DockedUpdateLayout(LPTW lptw);
void DockedGraphSize(LPTW lptw, SIZE *size, BOOL newwindow);

/* wmenu.c - Menu */
void SendMacro(LPTW lptw, UINT m);
void LoadMacros(LPTW lptw);
void CloseMacros(LPTW lptw);
#endif

/* wprinter.c - Printer setup and dump */
extern HGLOBAL hDevNames;
extern HGLOBAL hDevMode;

void PrintingCleanup(void);
void * PrintingCallbackCreate(GP_LPPRINT lpr);
void PrintingCallbackFree(void * callback);
void PrintRegister(GP_LPPRINT lpr);
void PrintUnregister(GP_LPPRINT lpr);
BOOL CALLBACK PrintAbortProc(HDC hdcPrn, int code);
INT_PTR CALLBACK PrintDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PrintSizeDlgProc(HWND hdlg, UINT wmsg, WPARAM wparam, LPARAM lparam);

/* wgraph.c */
unsigned luma_from_color(unsigned red, unsigned green, unsigned blue);
void add_tooltip(LPGW lpgw, PRECT rect, LPWSTR text);
void clear_tooltips(LPGW lpgw);
void draw_update_keybox(LPGW lpgw, unsigned plotno, unsigned x, unsigned y);
int draw_enhanced_text(LPGW lpgw, LPRECT rect, int x, int y, const char * str);
void draw_get_enhanced_text_extend(PRECT extend);
HBITMAP GraphGetBitmap(LPGW lpgw);

#ifdef __cplusplus
}
#endif

#endif /* GNUPLOT_WCOMMON_H */
