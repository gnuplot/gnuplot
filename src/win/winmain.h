/* GNUPLOT - win/winmain.h */

/*[
 * Copyright 2000, 2004   Hans-Bernhard Broeker
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
 *   Hans-Bernhard Broeker
 */

/* this file contains items defined by winmain.c that other parts of
 * the program need to access. Like the global data containers */

#ifndef GNUPLOT_WINMAIN_H
#define GNUPLOT_WINMAIN_H

#include <tchar.h>
#include <wchar.h>
#include "wgnuplib.h"

#ifdef __cplusplus
extern "C" {
#endif

extern TW textwin;
extern LPGW graphwin;
extern LPGW listgraphs;
extern PW pausewin;
extern MW menuwin;

extern HWND help_window;
extern LPTSTR winhelpname;
extern LPTSTR szMenuName;

int Pause (LPSTR str);
void screen_dump(void);
void kill_pending_Pause_dialog(void);
void win_sleep(DWORD dwMilliSeconds);
TBOOLEAN WinAnyWindowOpen(void);
void WinPersistTextClose(void);
void WinMessageLoop(void);
void WinOpenConsole(void);
BOOL WINAPI ConsoleHandler(DWORD dwType);
void WinRaiseConsole(void);
UINT WinGetCodepage(enum set_encoding_id encoding);
LPWSTR UnicodeText(LPCSTR str, enum set_encoding_id encoding);
LPSTR AnsiText(LPCWSTR strw,  enum set_encoding_id encoding);
void MultiByteAccumulate(BYTE ch, LPWSTR wstr, int * count);
LPSTR RelativePathToGnuplot(const char * path);

int ConsoleReadCh(void);
HANDLE init_pipe_input(void);
int next_pipe_input(void);

UINT GetDPI(void);

#ifdef __cplusplus
}
#endif

#endif /* GNUPLOT_WINMAIN_H */
