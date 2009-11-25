#ifndef lint
static char *RCSid() { return RCSid("$Id: winmain.c,v 1.30 2009/10/30 22:15:39 mikulik Exp $"); }
#endif

/* GNUPLOT - win/winmain.c */
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
 *
 */

/* This file implements the initialization code for running gnuplot   */
/* under Microsoft Windows. The code currently compiles only with the */
/* Borland C++ 3.1 compiler.					      */
/* 								      */
/* The modifications to allow Gnuplot to run under Windows were made  */
/* by Maurice Castro. (maurice@bruce.cs.monash.edu.au)  3 Jul 1992    */
/* and Russell Lang (rjl@monu1.cc.monash.edu.au) 30 Nov 1992          */
/*								      */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <fcntl.h>
#ifdef __MSC__
# include <malloc.h>
#endif
#ifdef __TURBOC__ /* HBB 981201: MinGW32 doesn't have this */
# include <alloc.h>
#endif
#ifdef __WATCOMC__
# define mktemp _mktemp
#endif
#include <io.h>
#include "plot.h"
#include "setshow.h"
#include "version.h"
#include "wgnuplib.h"
#include "wtext.h"
#include "wcommon.h"

#ifdef WIN32
# ifndef _WIN32_IE
#  define _WIN32_IE 0x0400
# endif
# include <shlobj.h>
# include <shlwapi.h>
  /* workaround for old header files */
# ifndef CSIDL_APPDATA
#  define CSIDL_APPDATA (0x001a)
# endif
#endif

/* limits */
#define MAXSTR 255
#define MAXPRINTF 1024
  /* used if vsnprintf(NULL,0,...) returns zero (MingW 3.4) */

/* globals */
TW textwin;
GW graphwin;
PW pausewin;
MW menuwin;
LPSTR szModuleName;
LPSTR szPackageDir;
LPSTR winhelpname;
LPSTR szMenuName;
#define MENUNAME "wgnuplot.mnu"
#ifndef HELPFILE /* HBB 981203: makefile.win predefines this... */
#define HELPFILE "wgnuplot.hlp"
#endif

char *authors[]={
                 "Colin Kelley",
                 "Thomas Williams"
                };

void WinExit(void);
int gnu_main(int argc, char *argv[], char *env[]);

void
CheckMemory(LPSTR str)
{
	if (str == (LPSTR)NULL) {
		MessageBox(NULL, "out of memory", "gnuplot", MB_ICONSTOP | MB_OK);
		exit(1);
	}
}

int
Pause(LPSTR str)
{
	pausewin.Message = str;
	return (PauseBox(&pausewin) == IDOK);
}

void
kill_pending_Pause_dialog ()
{
	if (pausewin.bPause == FALSE) /* no Pause dialog displayed */
	    return;
	/* Pause dialog displayed, thus kill it */
	DestroyWindow(pausewin.hWndPause);
#ifndef WIN32
#ifndef __DLL__
	FreeProcInstance((FARPROC)pausewin.lpfnPauseButtonProc);
#endif
#endif
	pausewin.bPause = FALSE;
}

/* atexit procedure */
void
WinExit()
{
	term_reset();

#ifndef __MINGW32__ /* HBB 980809: FIXME: doesn't exist for MinGW32. So...? */
	fcloseall();
#endif
	if (graphwin.hWndGraph && IsWindow(graphwin.hWndGraph))
		GraphClose(&graphwin);
#ifndef WGP_CONSOLE
	TextMessage();	/* process messages */
#endif
 	WinHelp(textwin.hWndText,(LPSTR)winhelpname,HELP_QUIT,(DWORD)NULL);
#ifndef WGP_CONSOLE
	TextMessage();	/* process messages */
#endif
	return;
}

/* call back function from Text Window WM_CLOSE */
int CALLBACK WINEXPORT
ShutDown()
{
	exit(0);
	return 0;
}

#ifdef WIN32

/* This function can be used to retrieve version information from
 * Window's Shell and common control libraries such (Comctl32.dll,
 * Shell32.dll, and Shlwapi.dll) The code was copied from the MSDN
 * article "Shell and Common Controls Versions" */
DWORD
GetDllVersion(LPCTSTR lpszDllName)
{
    HINSTANCE hinstDll;
    DWORD dwVersion = 0;

    /* For security purposes, LoadLibrary should be provided with a
       fully-qualified path to the DLL. The lpszDllName variable should be
       tested to ensure that it is a fully qualified path before it is used. */
    hinstDll = LoadLibrary(lpszDllName);

    if (hinstDll) {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll,
                          "DllGetVersion");

        /* Because some DLLs might not implement this function, you
        must test for it explicitly. Depending on the particular
        DLL, the lack of a DllGetVersion function can be a useful
        indicator of the version. */
        if (pDllGetVersion) {
            DLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);
            hr = (*pDllGetVersion)(&dvi);
            if (SUCCEEDED(hr))
               dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
        }
        FreeLibrary(hinstDll);
    }
    return dwVersion;
}


char *
appdata_directory(void)
{
    HMODULE hShell32;
    FARPROC pSHGetSpecialFolderPath;
    static char dir[MAX_PATH] = "";

    if (dir[0])
	return dir;

    /* Make sure that SHGetSpecialFolderPath is supported. */
    hShell32 = LoadLibrary(TEXT("shell32.dll"));
    if (hShell32) {
	pSHGetSpecialFolderPath =
	    GetProcAddress(hShell32,
			   TEXT("SHGetSpecialFolderPathA"));
	if (pSHGetSpecialFolderPath)
	    (*pSHGetSpecialFolderPath)(NULL, dir, CSIDL_APPDATA, FALSE);
	FreeModule(hShell32);
	return dir;
    }

    /* use APPDATA environment variable as fallback */
    if (dir[0] == '\0') {
	char *appdata = getenv("APPDATA");
	if (appdata) {
	    strcpy(dir, appdata);
	    return dir;
	}
    }

    return NULL;
}

#endif /* WIN32 */

#ifndef WGP_CONSOLE
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		LPSTR lpszCmdLine, int nCmdShow)
#else
int main(int argc, char **argv)
#endif
{
	/*WNDCLASS wndclass;*/
	LPSTR tail;

#ifdef WGP_CONSOLE
# define _argv argv
# define _argc argc
	HINSTANCE hInstance = GetModuleHandle(NULL), hPrevInstance = NULL;
	int nCmdShow = 0;
#else
#ifdef __MSC__  /* MSC doesn't give us _argc and _argv[] so ...   */
# ifdef WIN32    /* WIN32 has __argc and __argv */
#  define _argv __argv
#  define _argc __argc
# else
#  define MAXCMDTOKENS 128
	int     _argc=0;
	LPSTR   _argv[MAXCMDTOKENS];
	_argv[_argc] = "wgnuplot.exe";
	_argv[++_argc] = _fstrtok( lpszCmdLine, " ");
	while (_argv[_argc] != NULL)
		_argv[++_argc] = _fstrtok( NULL, " ");
# endif /* WIN32 */
#endif /* __MSC__ */
#endif /* WGP_CONSOLE */

#ifdef	__WATCOMC__
# define _argv __argv
# define _argc __argc
#endif

	szModuleName = (LPSTR)farmalloc(MAXSTR+1);
	CheckMemory(szModuleName);

	/* get path to EXE */
	GetModuleFileName(hInstance, (LPSTR) szModuleName, MAXSTR);
#ifndef WIN32
	if (CheckWGNUPLOTVersion(WGNUPLOTVERSION)) {
		MessageBox(NULL, "Wrong version of WGNUPLOT.DLL", szModuleName, MB_ICONSTOP | MB_OK);
		exit(1);
	}
#endif
	if ((tail = (LPSTR)_fstrrchr(szModuleName,'\\')) != (LPSTR)NULL)
	{
		tail++;
		*tail = 0;
	}
	szModuleName = (LPSTR)farrealloc(szModuleName, _fstrlen(szModuleName)+1);
	CheckMemory(szModuleName);

	if (_fstrlen(szModuleName) >= 5 && _fstrnicmp(&szModuleName[_fstrlen(szModuleName)-5], "\\bin\\", 5) == 0)
	{
		int len = _fstrlen(szModuleName)-4;
		szPackageDir = (LPSTR)farmalloc(len+1);
		CheckMemory(szPackageDir);
		_fstrncpy(szPackageDir, szModuleName, len);
		szPackageDir[len] = '\0';
	}
	else
		szPackageDir = szModuleName;

	winhelpname = (LPSTR)farmalloc(_fstrlen(szModuleName)+_fstrlen(HELPFILE)+1);
	CheckMemory(winhelpname);
	_fstrcpy(winhelpname,szModuleName);
	_fstrcat(winhelpname,HELPFILE);

	szMenuName = (LPSTR)farmalloc(_fstrlen(szModuleName)+_fstrlen(MENUNAME)+1);
	CheckMemory(szMenuName);
	_fstrcpy(szMenuName,szModuleName);
	_fstrcat(szMenuName,MENUNAME);

	textwin.hInstance = hInstance;
	textwin.hPrevInstance = hPrevInstance;
	textwin.nCmdShow = nCmdShow;
	textwin.Title = "gnuplot";

        get_user_env(); /* this hasn't been called yet */
        textwin.IniFile = gp_strdup("~\\wgnuplot.ini");
        gp_expand_tilde(&(textwin.IniFile));

	/* if tilde expansion fails use current directory as
	   default - that was the previous default behaviour */
	if (textwin.IniFile[0] == '~') {
	    free(textwin.IniFile);
	    textwin.IniFile = "wgnuplot.ini";
	}
	textwin.IniSection = "WGNUPLOT";
	textwin.DragPre = "load '";
	textwin.DragPost = "'\n";
	textwin.lpmw = &menuwin;
	textwin.ScreenSize.x = 80;
	textwin.ScreenSize.y = 80;
	textwin.KeyBufSize = 2048;
	textwin.CursorFlag = 1;	/* scroll to cursor after \n & \r */
	textwin.shutdown = MakeProcInstance((FARPROC)ShutDown, hInstance);
	textwin.AboutText = (LPSTR)farmalloc(1024);
	CheckMemory(textwin.AboutText);
	sprintf(textwin.AboutText,"Version %s\nPatchlevel %s\nLast Modified %s\n%s\n%s, %s and many others",
		gnuplot_version, gnuplot_patchlevel, gnuplot_date, gnuplot_copyright, authors[1], authors[0]);
	textwin.AboutText = (LPSTR)farrealloc(textwin.AboutText, _fstrlen(textwin.AboutText)+1);
	CheckMemory(textwin.AboutText);

	menuwin.szMenuName = szMenuName;

	pausewin.hInstance = hInstance;
	pausewin.hPrevInstance = hPrevInstance;
	pausewin.Title = "gnuplot pause";

	graphwin.hInstance = hInstance;
	graphwin.hPrevInstance = hPrevInstance;
	graphwin.Title = WINGRAPHTITLE;
	graphwin.lptw = &textwin;
	graphwin.IniFile = textwin.IniFile;
	graphwin.IniSection = textwin.IniSection;
	graphwin.color=TRUE;
	graphwin.fontsize = WINFONTSIZE;

#ifndef WGP_CONSOLE
	if (TextInit(&textwin))
		exit(1);
	textwin.hIcon = LoadIcon(hInstance, "TEXTICON");
#ifdef WIN32
	SetClassLong(textwin.hWndParent, GCL_HICON, (DWORD)textwin.hIcon);
#else
	SetClassWord(textwin.hWndParent, GCW_HICON, (WORD)textwin.hIcon);
#endif
	if (_argc>1) {
		int i,noend=FALSE;
		for (i=0; i<_argc; ++i)
			if (!stricmp(_argv[i],"-noend") || !stricmp(_argv[i],"/noend")
			    || !stricmp(_argv[i],"-persist"))
				noend = TRUE;
		if (noend)
			ShowWindow(textwin.hWndParent, textwin.nCmdShow);
	}
	else
		ShowWindow(textwin.hWndParent, textwin.nCmdShow);
	if (IsIconic(textwin.hWndParent)) { /* update icon */
		RECT rect;
		GetClientRect(textwin.hWndParent, (LPRECT) &rect);
		InvalidateRect(textwin.hWndParent, (LPRECT) &rect, 1);
		UpdateWindow(textwin.hWndParent);
	}
#endif


	atexit(WinExit);

	if (!isatty(fileno(stdin)))
		setmode(fileno(stdin), O_BINARY);

	gnu_main(_argc, _argv, environ);

	return 0;
}


#ifndef WGP_CONSOLE

/* replacement stdio routines that use Text Window for stdin/stdout */
/* WARNING: Do not write to stdout/stderr with functions not listed
   in win/wtext.h */

#undef kbhit
#undef getche
#undef getch
#undef putch

#undef fgetc
#undef getchar
#undef getc
#undef fgets
#undef gets

#undef fputc
#undef putchar
#undef putc
#undef fputs
#undef puts

#undef fprintf
#undef printf
#undef vprintf
#undef vfprintf

#undef fwrite
#undef fread

#if defined(__MSC__)|| defined(WIN32)
#define isterm(f) (f==stdin || f==stdout || f==stderr)
#else
#define isterm(f) isatty(fileno(f))
#endif

int
MyPutCh(int ch)
{
    return TextPutCh(&textwin, (BYTE)ch);
}

int
MyKBHit()
{
    return TextKBHit(&textwin);
}

int
MyGetCh()
{
    return TextGetCh(&textwin);
}

int
MyGetChE()
{
    return TextGetChE(&textwin);
}

int
MyFGetC(FILE *file)
{
    if (isterm(file)) {
	return MyGetChE();
    }
    return fgetc(file);
}

char *
MyGetS(char *str)
{
    TextPutS(&textwin,"\nDANGER: gets() used\n");
    MyFGetS(str,80,stdin);
    if (strlen(str) > 0
	&& str[strlen(str)-1]=='\n')
	str[strlen(str)-1] = '\0';
    return str;
}

char *
MyFGetS(char *str, unsigned int size, FILE *file)
{
    char FAR *p;

    if (isterm(file)) {
	p = TextGetS(&textwin, str, size);
	if (p != (char FAR *)NULL)
	    return str;
	return (char *)NULL;
    }
    return fgets(str,size,file);
}

int
MyFPutC(int ch, FILE *file)
{
    if (isterm(file)) {
	MyPutCh((BYTE)ch);
#ifndef WGP_CONSOLE
	TextMessage();
#endif
	return ch;
    }
    return fputc(ch,file);
}

int
MyFPutS(const char *str, FILE *file)
{
    if (isterm(file)) {
	TextPutS(&textwin, (char*) str);
#ifndef WGP_CONSOLE
	TextMessage();
#endif
	return (*str);	/* different from Borland library */
    }
    return fputs(str,file);
}

int
MyPutS(char *str)
{
    TextPutS(&textwin, str);
    MyPutCh('\n');
    TextMessage();
    return 0;	/* different from Borland library */
}

int
MyFPrintF(FILE *file, const char *fmt, ...)
{
    int count;
    va_list args;

    va_start(args, fmt);
    if (isterm(file)) {
	char *buf;
#ifdef __MSC__
	count = _vscprintf(fmt, args) + 1;
#else
	count = vsnprintf(NULL,0,fmt,args) + 1;
	if (count == 0) count = MAXPRINTF;
#endif
	va_end(args);
	va_start(args, fmt);
	buf = (char *)malloc(count * sizeof(char));
	count = vsnprintf(buf, count, fmt, args);
	TextPutS(&textwin, buf);
	free(buf);
    } else
	count = vfprintf(file, fmt, args);
    va_end(args);
    return count;
}

int
MyVFPrintF(FILE *file, const char *fmt, va_list args)
{
    int count;

    if (isterm(file)) {
	char *buf;
	va_list args_copied;

	va_copy(args_copied, args);
#ifdef __MSC__
	count = _vscprintf(fmt, args_copied) + 1;
#else
	count = vsnprintf(NULL, 0U, fmt, args_copied) + 1;
	if (count == 0) count = MAXPRINTF;
#endif
	va_end(args_copied);
	buf = (char *)malloc(count * sizeof(char));
	count = vsnprintf(buf, count, fmt, args);
	TextPutS(&textwin, buf);
	free(buf);
    } else
	count = vfprintf(file, fmt, args);
    return count;
}

int
MyPrintF(const char *fmt, ...)
{
    int count;
    char *buf;
    va_list args;

    va_start(args, fmt);
#ifdef __MSC__
    count = _vscprintf(fmt, args) + 1;
#else
    count = vsnprintf(NULL, 0, fmt, args) + 1;
    if (count == 0) count = MAXPRINTF;
#endif
    va_end(args);
    va_start(args, fmt);
    buf = (char *)malloc(count * sizeof(char));
    count = vsnprintf(buf, count, fmt, args);
    TextPutS(&textwin, buf);
    free(buf);
    va_end(args);
    return count;
}

size_t
MyFWrite(const void *ptr, size_t size, size_t n, FILE *file)
{
    if (isterm(file)) {
	size_t i;
	for (i=0; i<n; i++)
	    TextPutCh(&textwin, ((BYTE *)ptr)[i]);
	TextMessage();
	return n;
    }
    return fwrite(ptr, size, n, file);
}

size_t
MyFRead(void *ptr, size_t size, size_t n, FILE *file)
{
    if (isterm(file)) {
	size_t i;

	for (i=0; i<n; i++)
	    ((BYTE *)ptr)[i] = TextGetChE(&textwin);
	TextMessage();
	return n;
    }
    return fread(ptr, size, n, file);
}

#else /* WGP_CONSOLE */

DWORD WINAPI stdin_pipe_reader(LPVOID param)
{
#if 0
    HANDLE h = (HANDLE)_get_osfhandle(fileno(stdin));
    char c;
    DWORD cRead;

    if (ReadFile(h, &c, 1, &cRead, NULL))
        return c;
#else
    unsigned char c;
    if (fread(&c, 1, 1, stdin) == 1)
        return (DWORD)c;
    return EOF;
#endif
}

int ConsoleGetch()
{
    int fd = fileno(stdin);
    HANDLE h;
    DWORD waitResult;

    if (!isatty(fd))
        h = CreateThread(NULL, 0, stdin_pipe_reader, NULL, 0, NULL);
    else
        h = (HANDLE)_get_osfhandle(fd);

    do
    {
        waitResult = MsgWaitForMultipleObjects(1, &h, FALSE, INFINITE, QS_ALLINPUT);
        if (waitResult == WAIT_OBJECT_0)
        {
            if (isatty(fd))
            {
                INPUT_RECORD rec;
                DWORD recRead;

                ReadConsoleInput(h, &rec, 1, &recRead);
                if (recRead == 1 && rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown &&
                        (rec.Event.KeyEvent.wVirtualKeyCode < VK_SHIFT ||
                         rec.Event.KeyEvent.wVirtualKeyCode > VK_MENU))
                {
                    if (rec.Event.KeyEvent.uChar.AsciiChar)
                        return rec.Event.KeyEvent.uChar.AsciiChar;
                    else
                        switch (rec.Event.KeyEvent.wVirtualKeyCode)
                        {
                            case VK_UP: return 020;
                            case VK_DOWN: return 016;
                            case VK_LEFT: return 002;
                            case VK_RIGHT: return 006;
                            case VK_HOME: return 001;
                            case VK_END: return 005;
                            case VK_DELETE: return 004;
                        }
                }
            }
            else
            {
                DWORD c;
                GetExitCodeThread(h, &c);
		CloseHandle(h);
                return c;
            }
        }
        else if (waitResult == WAIT_OBJECT_0+1)
        {
            MSG msg;

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
            break;
    } while (1);
}

#endif /* WGP_CONSOLE */

/* public interface to printer routines : Windows PRN emulation
 * (formerly in win.trm)
 */

#define MAX_PRT_LEN 256
static char win_prntmp[MAX_PRT_LEN+1];

FILE *
open_printer()
{
    char *temp;

    if ((temp = getenv("TEMP")) == (char *)NULL)
	*win_prntmp='\0';
    else  {
	strncpy(win_prntmp,temp,MAX_PRT_LEN);
	/* stop X's in path being converted by mktemp */
	for (temp=win_prntmp; *temp; temp++)
	    *temp = tolower(*temp);
	if ( strlen(win_prntmp) && (win_prntmp[strlen(win_prntmp)-1]!='\\') )
	    strcat(win_prntmp,"\\");
    }
    strncat(win_prntmp, "_gptmp",MAX_PRT_LEN-strlen(win_prntmp));
    strncat(win_prntmp, "XXXXXX",MAX_PRT_LEN-strlen(win_prntmp));
    mktemp(win_prntmp);
    return fopen(win_prntmp, "w");
}

void
close_printer(FILE *outfile)
{
    fclose(outfile);
    DumpPrinter(graphwin.hWndGraph, graphwin.Title, win_prntmp);
}

void
screen_dump()
{
    GraphPrint(&graphwin);
}

void
win_raise_terminal_window()
{
    ShowWindow(graphwin.hWndGraph, SW_SHOWNORMAL);
    BringWindowToTop(graphwin.hWndGraph);
}

void
win_lower_terminal_window()
{
    SetWindowPos(graphwin.hWndGraph, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
}

