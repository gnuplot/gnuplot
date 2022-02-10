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
/* under Microsoft Windows.                                           */
/*                                                                    */
/* The modifications to allow Gnuplot to run under Windows were made  */
/* by Maurice Castro. (maurice@bruce.cs.monash.edu.au)  3 Jul 1992    */
/* and Russell Lang (rjl@monu1.cc.monash.edu.au) 30 Nov 1992          */
/*                                                                    */

#include "syscfg.h"
#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <htmlhelp.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <tchar.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include "alloc.h"
#include "plot.h"
#include "setshow.h"
#include "version.h"
#include "command.h"
#include "encoding.h"
#include "winmain.h"
#include "wtext.h"
#include "wcommon.h"
#ifdef HAVE_GDIPLUS
#include "wgdiplus.h"
#endif
#ifdef HAVE_D2D
#include "wd2d.h"
#endif
#ifdef WXWIDGETS
#include "wxterminal/wxt_term.h"
#endif
#ifdef HAVE_LIBCACA
# define TERM_PUBLIC_PROTO
# include "caca.trm"
# undef TERM_PUBLIC_PROTO
#endif


/* workaround for old header files */
#ifndef CSIDL_APPDATA
# define CSIDL_APPDATA (0x001a)
#endif

/* limits */
#define MAXSTR 255
#define MAXPRINTF 1024
  /* used if vsnprintf(NULL,0,...) returns zero (MingW 3.4) */

/* globals */
#ifndef WGP_CONSOLE
TW textwin;
MW menuwin;
#endif
LPGW graphwin; /* current graph window */
LPGW listgraphs; /* list of graph windows */
PW pausewin;
LPTSTR szModuleName;
LPTSTR szPackageDir;
LPTSTR winhelpname;
LPTSTR szMenuName;
static LPTSTR szLanguageCode = NULL;
HWND help_window = NULL;

char *authors[]={
                 "Colin Kelley",
                 "Thomas Williams"
                };

void WinExit(void);
static void WinCloseHelp(void);
int CALLBACK ShutDown(void);
#ifdef WGP_CONSOLE
static char * ConsoleGetS(char * str, unsigned int size);
static int ConsolePutS(const char *str);
static int ConsolePutCh(int ch);
#endif


static void
CheckMemory(LPTSTR str)
{
    if (str == NULL) {
	MessageBox(NULL, TEXT("out of memory"), TEXT("gnuplot"), MB_ICONSTOP | MB_OK);
	gp_exit(EXIT_FAILURE);
    }
}


int
Pause(LPSTR str)
{
    int rc;

    pausewin.Message = UnicodeText(str, encoding);
    rc = PauseBox(&pausewin) == IDOK;
    free(pausewin.Message);
    return rc;
}


void
kill_pending_Pause_dialog(void)
{
    if (!pausewin.bPause) /* no Pause dialog displayed */
	return;
    /* Pause dialog displayed, thus kill it */
    DestroyWindow(pausewin.hWndPause);
    pausewin.bPause = FALSE;
}


/* atexit procedure */
void
WinExit(void)
{
    LPGW lpgw;

    /* Last chance to close Windows help, call before anything else to avoid a crash. */
    WinCloseHelp();

    /* clean-up call for printing system */
    PrintingCleanup();

    term_reset();

    _fcloseall();

    /* Close all graph windows */
    for (lpgw = listgraphs; lpgw != NULL; lpgw = lpgw->next) {
	if (GraphHasWindow(lpgw))
	    GraphClose(lpgw);
    }

#ifndef WGP_CONSOLE
    TextMessage();  /* process messages */
# ifndef __WATCOMC__
    /* revert C++ stream redirection */
    RedirectOutputStreams(FALSE);
# endif
#endif
#ifdef HAVE_GDIPLUS
    gdiplusCleanup();
#endif
#ifdef HAVE_D2D
    d2dCleanup();
#endif
    CoUninitialize();
    return;
}


/* call back function from Text Window WM_CLOSE */
int CALLBACK
ShutDown(void)
{
    /* First chance for wgnuplot to close help system. */
    WinCloseHelp();
    gp_exit(EXIT_SUCCESS);
    return 0;
}


/* This function can be used to retrieve version information from
 * Window's Shell and common control libraries (Comctl32.dll,
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
	pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");

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


BOOL
IsWindowsXPorLater(void)
{
    OSVERSIONINFO versionInfo;

    /* get Windows version */
    ZeroMemory(&versionInfo, sizeof(OSVERSIONINFO));
    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&versionInfo);
    return ((versionInfo.dwMajorVersion > 5) ||
           ((versionInfo.dwMajorVersion == 5) && (versionInfo.dwMinorVersion >= 1)));
}


char *
appdata_directory(void)
{
    HMODULE hShell32;
    FARPROC pSHGetSpecialFolderPath;
    static char dir[MAX_PATH] = "";

    if (dir[0])
	return dir;

    /* FIMXE: "ANSI" Version, no Unicode support */

    /* Make sure that SHGetSpecialFolderPath is supported. */
    hShell32 = LoadLibrary(TEXT("shell32.dll"));
    if (hShell32) {
	pSHGetSpecialFolderPath =
	    GetProcAddress(hShell32, "SHGetSpecialFolderPathA");
	if (pSHGetSpecialFolderPath)
	    (*pSHGetSpecialFolderPath)(NULL, dir, CSIDL_APPDATA, FALSE);
	FreeModule(hShell32);
	return dir;
    }

    /* use APPDATA environment variable as fallback */
    if (dir[0] == NUL) {
	char *appdata = getenv("APPDATA");
	if (appdata) {
	    strcpy(dir, appdata);
	    return dir;
	}
    }

    return NULL;
}


/* retrieve path relative to gnuplot executable */
LPSTR
RelativePathToGnuplot(const char * path)
{
#ifdef UNICODE
    LPSTR ansi_dir = AnsiText(szPackageDir, encoding);
    LPSTR rel_path = (char *) gp_realloc(ansi_dir, strlen(ansi_dir) + strlen(path) + 1, "RelativePathToGnuplot");
    if (rel_path == NULL) {
	free(ansi_dir);
	return (LPSTR) path;
    }
#else
    char * rel_path = (char * ) gp_alloc(strlen(szPackageDir) + strlen(path) + 1, "RelativePathToGnuplot");
    strcpy(rel_path, szPackageDir);
#endif
    /* szPackageDir is guaranteed to have a trailing backslash */
    strcat(rel_path, path);
    return rel_path;
}


static void
WinCloseHelp(void)
{
    /* Due to a known bug in the HTML help system we have to
     * call this as soon as possible before the end of the program.
     * See e.g. http://helpware.net/FAR/far_faq.htm#HH_CLOSE_ALL
     */
    if (IsWindow(help_window))
	SendMessage(help_window, WM_CLOSE, 0, 0);
    Sleep(0);
}


static LPTSTR
GetLanguageCode(void)
{
    static TCHAR lang[6] = TEXT("");

    if (lang[0] == NUL) {
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME, lang, sizeof(lang));
	//strcpy(lang, "JPN"); //TEST
	/* language definition files for Japanese already use "ja" as abbreviation */
	if (_tcscmp(lang, TEXT("JPN")) == 0)
	    lang[1] = 'A';
	/* prefer lower case */
	lang[0] = tolower((unsigned char)lang[0]);
	lang[1] = tolower((unsigned char)lang[1]);
	/* only use two character sequence */
	lang[2] = NUL;
    }

    return lang;
}


static LPTSTR
LocalisedFile(LPCTSTR name, LPCTSTR ext, LPCTSTR defaultname)
{
    LPTSTR lang;
    LPTSTR filename;

    /* Allow user to override language detection. */
    if (szLanguageCode)
	lang = szLanguageCode;
    else
	lang = GetLanguageCode();

    filename = (LPTSTR) malloc((_tcslen(szModuleName) + _tcslen(name) + _tcslen(lang) + _tcslen(ext) + 1) * sizeof(TCHAR));
    if (filename) {
	_tcscpy(filename, szModuleName);
	_tcscat(filename, name);
	_tcscat(filename, lang);
	_tcscat(filename, ext);
	if (!PathFileExists(filename)) {
	    _tcscpy(filename, szModuleName);
	    _tcscat(filename, defaultname);
	}
    }
    return filename;
}


static void
ReadMainIni(LPTSTR file, LPTSTR section)
{
    TCHAR profile[81] = TEXT("");
    const TCHAR hlpext[] = TEXT(".chm");
    const TCHAR name[] = TEXT("wgnuplot-");

    /* Language code override */
    GetPrivateProfileString(section, TEXT("Language"), TEXT(""), profile, 80, file);
    if (profile[0] != NUL)
	szLanguageCode = _tcsdup(profile);
    else
	szLanguageCode = NULL;

    /* help file name */
    GetPrivateProfileString(section, TEXT("HelpFile"), TEXT(""), profile, 80, file);
    if (profile[0] != NUL) {
	winhelpname = (LPTSTR) malloc((_tcslen(szModuleName) + _tcslen(profile) + 1) * sizeof(TCHAR));
	if (winhelpname) {
	    _tcscpy(winhelpname, szModuleName);
	    _tcscat(winhelpname, profile);
	}
    } else {
	/* default name is "wgnuplot-LL.chm" */
	winhelpname = LocalisedFile(name, hlpext, TEXT(HELPFILE));
    }

    /* menu file name */
    GetPrivateProfileString(section, TEXT("MenuFile"), TEXT(""), profile, 80, file);
    if (profile[0] != NUL) {
	szMenuName = (LPTSTR) malloc((_tcslen(szModuleName) + _tcslen(profile) + 1) * sizeof(TCHAR));
	if (szMenuName) {
	    _tcscpy(szMenuName, szModuleName);
	    _tcscat(szMenuName, profile);
	}
    } else {
	/* default name is "wgnuplot-LL.mnu" */
	szMenuName = LocalisedFile(name, TEXT(".mnu"), TEXT("wgnuplot.mnu"));
    }
}


#ifndef WGP_CONSOLE
#ifndef __WATCOMC__
int WINAPI
_tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#else
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
#else
int
_tmain(int argc, TCHAR **argv)
#endif
{
    LPTSTR tail;
    int i;
#ifdef WGP_CONSOLE
    HINSTANCE hInstance = GetModuleHandle(NULL), hPrevInstance = NULL;
#endif

#ifndef _UNICODE
# ifndef WGP_CONSOLE
#  if defined(__MINGW32__) && !defined(_W64)
#   define argc _argc
#   define argv _argv
#  else /* MSVC, WATCOM, MINGW-W64 */
#   define argc __argc
#   define argv __argv
#  endif
# endif /* WGP_CONSOLE */
#else
#  define argc __argc
#  define argv argv_u8
    /* create an UTF-8 encoded copy of all arguments */
    char ** argv_u8 = calloc(__argc, sizeof(char *));
    for (i = 0; i < __argc; i++)
	argv_u8[i] = AnsiText(__wargv[i], S_ENC_UTF8);
#endif

    szModuleName = (LPTSTR) malloc((MAXSTR + 1) * sizeof(TCHAR));
    CheckMemory(szModuleName);

    /* get path to gnuplot executable  */
    GetModuleFileName(hInstance, szModuleName, MAXSTR);
    if ((tail = _tcsrchr(szModuleName, '\\')) != NULL) {
	tail++;
	*tail = 0;
    }
    szModuleName = (LPTSTR) realloc(szModuleName, (_tcslen(szModuleName) + 1) * sizeof(TCHAR));
    CheckMemory(szModuleName);

    if (_tcslen(szModuleName) >= 5 && _tcsnicmp(&szModuleName[_tcslen(szModuleName)-5], TEXT("\\bin\\"), 5) == 0) {
	size_t len = _tcslen(szModuleName) - 4;
	szPackageDir = (LPTSTR) malloc((len + 1) * sizeof(TCHAR));
	CheckMemory(szPackageDir);
	_tcsncpy(szPackageDir, szModuleName, len);
	szPackageDir[len] = NUL;
    } else {
	szPackageDir = szModuleName;
    }

#ifndef WGP_CONSOLE
    textwin.hInstance = hInstance;
    textwin.hPrevInstance = hPrevInstance;
    textwin.nCmdShow = nCmdShow;
    textwin.Title = L"gnuplot";
#endif

    /* create structure of first graph window */
    graphwin = (LPGW) calloc(1, sizeof(GW));
    listgraphs = graphwin;

    /* locate ini file */
    {
	char * inifile;
#ifdef UNICODE
	LPWSTR winifile;
#endif
	get_user_env(); /* this hasn't been called yet */
	inifile = gp_strdup("~\\wgnuplot.ini");
	gp_expand_tilde(&inifile);

	/* if tilde expansion fails use current directory as
	    default - that was the previous default behaviour */
	if (inifile[0] == '~') {
	    free(inifile);
	    inifile = "wgnuplot.ini";
	}
#ifdef UNICODE
	graphwin->IniFile = winifile = UnicodeText(inifile, S_ENC_DEFAULT);
#else
	graphwin->IniFile = inifile;
#endif
#ifndef WGP_CONSOLE
	textwin.IniFile = graphwin->IniFile;
#endif
	ReadMainIni(graphwin->IniFile, TEXT("WGNUPLOT"));
    }

#ifndef WGP_CONSOLE
    textwin.IniSection = TEXT("WGNUPLOT");
    textwin.DragPre = L"load '";
    textwin.DragPost = L"'\n";
    textwin.lpmw = &menuwin;
    textwin.ScreenSize.x = 80;
    textwin.ScreenSize.y = 80;
    textwin.KeyBufSize = 2048;
    textwin.CursorFlag = 1; /* scroll to cursor after \n & \r */
    textwin.shutdown = MakeProcInstance((FARPROC)ShutDown, hInstance);
    textwin.AboutText = (LPTSTR) malloc(1024 * sizeof(TCHAR));
    CheckMemory(textwin.AboutText);
    wsprintf(textwin.AboutText,
	TEXT("Version %hs patchlevel %hs\n") \
	TEXT("last modified %hs\n") \
	TEXT("%hs\n%hs, %hs and many others\n") \
	TEXT("gnuplot home:     http://www.gnuplot.info\n"),
	gnuplot_version, gnuplot_patchlevel,
	gnuplot_date,
	gnuplot_copyright, authors[1], authors[0]);
    textwin.AboutText = (LPTSTR) realloc(textwin.AboutText, (_tcslen(textwin.AboutText) + 1) * sizeof(TCHAR));
    CheckMemory(textwin.AboutText);

    menuwin.szMenuName = szMenuName;
#endif

    pausewin.hInstance = hInstance;
    pausewin.hPrevInstance = hPrevInstance;
    pausewin.Title = L"gnuplot pause";

    graphwin->hInstance = hInstance;
    graphwin->hPrevInstance = hPrevInstance;
#ifdef WGP_CONSOLE
    graphwin->lptw = NULL;
#else
    graphwin->lptw = &textwin;
#endif

    /* COM Initialization */
    if (!SUCCEEDED(CoInitialize(NULL))) {
	// FIXME: Need to abort
    }

    /* init common controls */
    {
	INITCOMMONCONTROLSEX initCtrls;
	initCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
	initCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&initCtrls);
    }

#ifndef WGP_CONSOLE
    if (TextInit(&textwin))
	gp_exit(EXIT_FAILURE);
    textwin.hIcon = LoadIcon(hInstance, TEXT("TEXTICON"));
    SetClassLongPtr(textwin.hWndParent, GCLP_HICON, (LONG_PTR)textwin.hIcon);

    /* Note: we want to know whether this is an interactive session so that we can
     * decide whether or not to write status information to stderr.  The old test
     * for this was to see if (argc > 1) but the addition of optional command line
     * switches broke this.  What we really wanted to know was whether any of the
     * command line arguments are file names or an explicit in-line "-e command".
     * (This is a copy of a code snippet from plot.c)
     */
    for (i = 1; i < argc; i++) {
	if (!_stricmp(argv[i], "/noend"))
	    continue;
	if ((argv[i][0] != '-') || (argv[i][1] == 'e')) {
	    interactive = FALSE;
	    break;
	}
    }
    if (interactive)
	ShowWindow(textwin.hWndParent, textwin.nCmdShow);
    if (IsIconic(textwin.hWndParent)) { /* update icon */
	RECT rect;
	GetClientRect(textwin.hWndParent, (LPRECT) &rect);
	InvalidateRect(textwin.hWndParent, (LPRECT) &rect, 1);
	UpdateWindow(textwin.hWndParent);
    }
# ifndef __WATCOMC__
    /* Finally, also redirect C++ standard output streams. */
    RedirectOutputStreams(TRUE);
# endif
#else  /* !WGP_CONSOLE */
# ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#  define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
# endif
    {
	/* Enable Windows 10 Console Virtual Terminal Sequences */
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD  mode;
	GetConsoleMode(handle, &mode);
	SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    // set console mode handler to catch "abort" signals
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
#endif

    gp_atexit(WinExit);

    if (!_isatty(_fileno(stdin)))
	_setmode(_fileno(stdin), O_BINARY);

    gnu_main(argc, argv);

    /* First chance to close help system for console gnuplot,
	second for wgnuplot */
    WinCloseHelp();
    gp_exit_cleanup();
    return 0;
}


void
MultiByteAccumulate(BYTE ch, LPWSTR wstr, int * count)
{
    static char mbstr[4] = "";
    static int mbwait = 0;
    static int mbcount = 0;

    *count = 0;

    /* try to re-sync on control characters */
    /* works for utf8 and sjis */
    if (ch < 32) {
	mbwait = mbcount = 0;
	mbstr[0] = NUL;
    }

    if (encoding == S_ENC_UTF8) { /* combine UTF8 byte sequences */
	if (mbwait == 0) {
	    /* first byte */
	    mbcount = 0;
	    mbstr[mbcount] = ch;
	    if ((ch & 0xE0) == 0xC0) {
		// expect one more byte
		mbwait = 1;
	    } else if ((ch & 0xF0) == 0xE0) {
		// expect two more bytes
		mbwait = 2;
	    } else if ((ch & 0xF8) == 0xF0) {
		// expect three more bytes
		mbwait = 3;
	    }
	} else {
	    /* subsequent byte */
	    /*assert((ch & 0xC0) == 0x80);*/
	    if ((ch & 0xC0) == 0x80) {
		mbcount++;
		mbwait--;
	    } else {
		/* invalid sequence */
		mbcount = 0;
		mbwait = 0;
	    }
	    mbstr[mbcount] = ch;
	}
	if (mbwait == 0) {
	    *count = MultiByteToWideChar(CP_UTF8, 0, mbstr, mbcount + 1, wstr, 2);
	}
    } else if (encoding == S_ENC_SJIS) { /* combine S-JIS sequences */
	if (mbwait == 0) {
	    /* first or single byte */
	    mbcount = 0;
	    mbstr[mbcount] = ch;
	    if (is_sjis_lead_byte(ch)) {
		/* first byte */
		mbwait = 1;
	    }
	} else {
	    if ((ch >= 0x40) && (ch <= 0xfc)) {
		/* valid */
		mbcount++;
	    } else {
		/* invalid */
		mbcount = 0;
	    }
	    mbwait = 0; /* max. double byte sequences */
	    mbstr[mbcount] = ch;
	}
	if (mbwait == 0) {
	    *count = MultiByteToWideChar(932, 0, mbstr, mbcount + 1, wstr, 2);
	}
    } else {
	mbcount = 0;
	mbwait = 0;
	mbstr[0] = (char) ch;
	*count = MultiByteToWideChar(WinGetCodepage(encoding), 0, mbstr, mbcount + 1, wstr, 2);
    }
}


/* replacement stdio routines that use
 *  -  Text Window for stdin/stdout (wgnuplot)
 *  -  Unicode console APIs to handle encodings (console gnuplot)
 * WARNING: Do not write to stdout/stderr with functions not listed
 * in win/wtext.h
*/

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

#ifndef WGP_CONSOLE
# define TEXTMESSAGE TextMessage()
# define GETCH() TextGetChE(&textwin)
# define PUTS(s) TextPutS(&textwin, (char*) s)
# define PUTCH(c) TextPutCh(&textwin, (BYTE) c)
# define isterm(f) (f==stdin || f==stdout || f==stderr)
#else
# define TEXTMESSAGE
# define GETCH() ConsoleReadCh()
# define PUTS(s) ConsolePutS(s)
# define PUTCH(c) ConsolePutCh(c)
# define isterm(f) _isatty(_fileno(f))
#endif

int
MyPutCh(int ch)
{
    return PUTCH(ch);
}

#ifndef WGP_CONSOLE
int
MyKBHit(void)
{
    return TextKBHit(&textwin);
}

int
MyGetCh(void)
{
    return TextGetCh(&textwin);
}

int
MyGetChE(void)
{
    return TextGetChE(&textwin);
}
#endif


int
MyFGetC(FILE *file)
{
    if (isterm(file))
	return GETCH();
    return fgetc(file);
}

char *
MyGetS(char *str)
{
    MyFGetS(str, 80, stdin);
    if (strlen(str) > 0 && str[strlen(str) - 1] == '\n')
	str[strlen(str) - 1] = '\0';
    return str;
}

char *
MyFGetS(char *str, unsigned int size, FILE *file)
{
    if (isterm(file)) {
#ifndef WGP_CONSOLE
	char * p = TextGetS(&textwin, str, size);
	if (p != NULL)
	    return str;
	return NULL;
#else
	return ConsoleGetS(str, size);
#endif
    }
    return fgets(str,size,file);
}

int
MyFPutC(int ch, FILE *file)
{
    if (isterm(file)) {
	PUTCH(ch);
	TEXTMESSAGE;
	return ch;
    }
    return fputc(ch,file);
}

int
MyFPutS(const char *str, FILE *file)
{
    if (isterm(file)) {
	PUTS(str);
	TEXTMESSAGE;
	return (*str);
    }
    return fputs(str,file);
}

int
MyPutS(const char *str)
{
    PUTS(str);
    PUTCH('\n');
    TEXTMESSAGE;
    return 0;
}

int
MyFPrintF(FILE *file, const char *fmt, ...)
{
    int count;
    va_list args;

    va_start(args, fmt);
    if (isterm(file)) {
	char *buf;

	count = vsnprintf(NULL, 0, fmt, args) + 1;
	if (count == 0)
	    count = MAXPRINTF;
	va_end(args);
	va_start(args, fmt);
	buf = (char *) malloc(count * sizeof(char));
	count = vsnprintf(buf, count, fmt, args);
	PUTS(buf);
	free(buf);
    } else {
	count = vfprintf(file, fmt, args);
    }
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
	count = vsnprintf(NULL, 0U, fmt, args) + 1;
	if (count == 0)
	    count = MAXPRINTF;
	va_end(args_copied);
	buf = (char *) malloc(count * sizeof(char));
	count = vsnprintf(buf, count, fmt, args);
	PUTS(buf);
	free(buf);
    } else {
	count = vfprintf(file, fmt, args);
    }
    return count;
}

int
MyPrintF(const char *fmt, ...)
{
    int count;
    char *buf;
    va_list args;

    va_start(args, fmt);
    count = vsnprintf(NULL, 0, fmt, args) + 1;
    if (count == 0)
	count = MAXPRINTF;
    va_end(args);
    va_start(args, fmt);
    buf = (char *) malloc(count * sizeof(char));
    count = vsnprintf(buf, count, fmt, args);
    PUTS(buf);
    free(buf);
    va_end(args);
    return count;
}

size_t
MyFWrite(const void *ptr, size_t size, size_t n, FILE *file)
{
    if (isterm(file)) {
	size_t i;
	for (i = 0; i < n; i++)
	    PUTCH(((BYTE *)ptr)[i]);
	TEXTMESSAGE;
	return n;
    }
    return fwrite(ptr, size, n, file);
}

size_t
MyFRead(void *ptr, size_t size, size_t n, FILE *file)
{
    if (isterm(file)) {
	size_t i;

	for (i = 0; i < n; i++)
	    ((BYTE *)ptr)[i] = GETCH();
	TEXTMESSAGE;
	return n;
    }
    return fread(ptr, size, n, file);
}


#ifdef USE_FAKEPIPES

static char pipe_type = NUL;
static char * pipe_filename = NULL;
static char * pipe_command = NULL;

FILE *
fake_popen(const char * command, const char * type)
{
    FILE * f = NULL;
    char tmppath[MAX_PATH];
    char tmpfile[MAX_PATH];
    DWORD ret;

    if (type == NULL)
	return NULL;

    pipe_type = NUL;
    if (pipe_filename != NULL)
	free(pipe_filename);

    /* Random temp file name in %TEMP% */
    ret = GetTempPathA(sizeof(tmppath), tmppath);
    if ((ret == 0) || (ret > sizeof(tmppath)))
	return NULL;
    ret = GetTempFileNameA(tmppath, "gpp", 0, tmpfile);
    if (ret == 0)
	return NULL;
    pipe_filename = gp_strdup(tmpfile);

    if (*type == 'r') {
	char * cmd;
	int rc;
	LPWSTR wcmd;
	pipe_type = *type;
	/* Execute command with redirection of stdout to temporary file. */
#ifndef HAVE_BROKEN_WSYSTEM
	cmd = (char *) malloc(strlen(command) + strlen(pipe_filename) + 5);
	sprintf(cmd, "%s > %s", command, pipe_filename);
	wcmd = UnicodeText(cmd, encoding);
	rc = _wsystem(wcmd);
	free(wcmd);
#else
	cmd = (char *) malloc(strlen(command) + strlen(pipe_filename) + 15);
	sprintf(cmd, "cmd /c %s > %s", command, pipe_filename);
	rc = system(cmd);
#endif
	free(cmd);
	/* Now open temporary file. */
	/* system() returns 1 if the command could not be executed. */
	if (rc != 1) {
	    f = fopen(pipe_filename, "r");
	} else {
	    remove(pipe_filename);
	    free(pipe_filename);
	    pipe_filename = NULL;
	    errno = EINVAL;
	}
    } else if (*type == 'w') {
	pipe_type = *type;
	/* Write output to temporary file and handle the rest in fake_pclose. */
	if (type[1] == 'b')
	    int_error(NO_CARET, "Could not execute pipe '%s'. Writing to binary pipes is not supported.", command);
	else
	    f = fopen(pipe_filename, "w");
	pipe_command = gp_strdup(command);
    }

    return f;
}


int
fake_pclose(FILE *stream)
{
    int rc = 0;
    if (!stream)
	return ECHILD;

    /* Close temporary file */
    fclose(stream);

    /* Finally, execute command with redirected stdin. */
    if (pipe_type == 'w') {
	char * cmd;
	LPWSTR wcmd;

#ifndef HAVE_BROKEN_WSYSTEM
	cmd = (char *) gp_alloc(strlen(pipe_command) + strlen(pipe_filename) + 10, "fake_pclose");
	/* FIXME: this won't work for binary data. We need a proper `cat` replacement. */
	sprintf(cmd, "type %s | %s", pipe_filename, pipe_command);
	wcmd = UnicodeText(cmd, encoding);
	rc = _wsystem(wcmd);
	free(wcmd);
#else
	cmd = (char *) gp_alloc(strlen(pipe_command) + strlen(pipe_filename) + 20, "fake_pclose");
	sprintf(cmd, "cmd/c type %s | %s", pipe_filename, pipe_command);
	rc = system(cmd);
#endif
	free(cmd);
    }

    /* Delete temp file again. */
    if (pipe_filename) {
	remove(pipe_filename);
	errno = 0;
	free(pipe_filename);
	pipe_filename = NULL;
    }

    if (pipe_command) {
	/* system() returns 255 if the command could not be executed.
	    The real popen would have returned an error already. */
	if (rc == 255)
	    int_error(NO_CARET, "Could not execute pipe '%s'.", pipe_command);
	free(pipe_command);
    }

    return rc;
}
#endif


#ifdef WGP_CONSOLE

// FIXME: these do not get destroyed properly
HANDLE input_thread = NULL;
HANDLE input_event = NULL;
HANDLE input_cont = NULL;
int nextchar = EOF;

DWORD WINAPI
stdin_pipe_reader(LPVOID param)
{
    do {
	unsigned char c;
	size_t n = fread(&c, 1, 1, stdin);
	WaitForSingleObject(input_cont, INFINITE);
	if (n == 1)
	    nextchar = c;
	else if (feof(stdin))
	    nextchar = EOF;
	SetEvent(input_event);
	Sleep(0);
    } while (TRUE);
    return EOF;
}


HANDLE
init_pipe_input(void)
{
    if (input_event == NULL)
	input_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (input_cont == NULL)
	input_cont = CreateEvent(NULL, FALSE, TRUE, NULL);
    if (input_thread == NULL)
	input_thread = CreateThread(NULL, 0, stdin_pipe_reader, NULL, 0, NULL);
    return input_event;
}


int
next_pipe_input(void)
{
    int c = nextchar;
    SetEvent(input_cont);
    return c;
}


int
ConsoleGetch(void)
{
    int fd = _fileno(stdin);
    HANDLE h;
    DWORD waitResult;

    if (!_isatty(fd)) {
	h = init_pipe_input();
    } else {
	h = (HANDLE)_get_osfhandle(fd);
    }

    do {
	waitResult = MsgWaitForMultipleObjects(1, &h, FALSE, INFINITE, QS_ALLINPUT);
	if (waitResult == WAIT_OBJECT_0) {
	    if (_isatty(fd)) {
		DWORD c = ConsoleReadCh();
		if (c != NUL)
		    return c;
	    } else {
		return next_pipe_input();
	    }
	} else if (waitResult == WAIT_OBJECT_0+1) {
	    WinMessageLoop();
	    if (ctrlc_flag)
		return '\r';
	} else
		break;
    } while (1);

    return '\r';
}

#endif /* WGP_CONSOLE */


int
ConsoleReadCh(void)
{
    const int max_input = 8;
    static char console_input[8];
    static int first_input_char = 0;
    static int last_input_char = 0;
    INPUT_RECORD rec;
    DWORD recRead;
    HANDLE h;

    if (first_input_char != last_input_char) {
	int c = console_input[first_input_char];
	first_input_char++;
	first_input_char %= max_input;
	return c;
    }

    h = GetStdHandle(STD_INPUT_HANDLE);
    if (h == NULL)
	return NUL;

    ReadConsoleInputW(h, &rec, 1, &recRead);
    /* FIXME: We should handle rec.Event.KeyEvent.wRepeatCount > 1, too. */
    if (recRead == 1 && rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown &&
       (rec.Event.KeyEvent.wVirtualKeyCode < VK_SHIFT ||
	rec.Event.KeyEvent.wVirtualKeyCode > VK_MENU)) {
	    if (rec.Event.KeyEvent.uChar.UnicodeChar) {
		if ((rec.Event.KeyEvent.dwControlKeyState == SHIFT_PRESSED) && (rec.Event.KeyEvent.wVirtualKeyCode == VK_TAB)) {
		    return 034; /* remap Shift-Tab */
		} else {
		    int i, count;
		    char mbchar[8];
		    count = WideCharToMultiByte(WinGetCodepage(encoding), 0,
				&rec.Event.KeyEvent.uChar.UnicodeChar, 1,
				mbchar, sizeof(mbchar),
				NULL, NULL);
		    for (i = 1; i < count; i++) {
			console_input[last_input_char] = mbchar[i];
			last_input_char++;
			last_input_char %= max_input;
		    }
		    return mbchar[0];
		}
	    } else {
		switch (rec.Event.KeyEvent.wVirtualKeyCode) {
		case VK_UP: return 020;
		case VK_DOWN: return 016;
		case VK_LEFT: return 002;
		case VK_RIGHT: return 006;
		case VK_HOME: return 001;
		case VK_END: return 005;
		case VK_DELETE: return 0117;
		}
	    }
    }

    /* Error reading event or, key up or, one of the following event records:
	MOUSE_EVENT_RECORD, WINDOW_BUFFER_SIZE_RECORD, MENU_EVENT_RECORD, FOCUS_EVENT_RECORD */
    return NUL;
}

#ifdef WGP_CONSOLE

static char *
ConsoleGetS(char * str, unsigned int size)
{
    char * next = str;

    while (--size > 0) {
	switch (*next = ConsoleGetch()) {
	case EOF:
	    *next = 0;
	    if (next == str)
		return NULL;
	    return str;
	case '\r':
	    *next = '\n';
	    /* intentionally fall through */
	case '\n':
	    ConsolePutCh(*next);
	    *(next + 1) = 0;
	    return str;
	case 0x08:
	case 0x7f:
	    ConsolePutCh(*next);
	    if (next > str)
		--next;
	    break;
	default:
	    ConsolePutCh(*next);
	    ++next;
	}
    }
    *next = 0;
    return str;
}


static int
ConsolePutS(const char *str)
{
    LPWSTR wstr = UnicodeText(str, encoding);
    // Word-wrapping on Windows 10 (now) works anyway.
    if (isatty(fileno(stdout))) {
	// Using standard (wide) file IO screws up UTF-8
	// output, so use console IO instead. No idea why
	// it does so, though.
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleW(h, wstr, wcslen(wstr), NULL, NULL);
    } else {
	// Use standard file IO instead of Console API
	// to allow redirection of stdout/stderr.
	fputws(wstr, stdout);
    }
    free(wstr);
    return 0;
}


static int
ConsolePutCh(int ch)
{
    WCHAR w[4];
    int count;

    MultiByteAccumulate(ch, w, &count);
    if (count > 0) {
	w[count] = 0;
	if (isatty(fileno(stdout))) {
	    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	    WriteConsoleW(h, w, count, NULL, NULL);
	} else {
	    fputws(w, stdout);
	}
    }
    return ch;
}
#endif


/* This is called by the system to signal various events.
   Note that it is executed in a separate thread.  */
BOOL WINAPI
ConsoleHandler(DWORD dwType)
{
    switch (dwType) {
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT: {
#ifdef WGP_CONSOLE
	HANDLE h;
	INPUT_RECORD rec;
	DWORD written;
#endif

	// NOTE: returning from this handler terminates the application.
	// Instead, we signal the main thread to clean up and exit and
	// then idle by sleeping.
#ifndef WGP_CONSOLE
	// close the main window to exit gnuplot
	PostMessage(textwin.hWndParent, WM_CLOSE, 0, 0);
#else
	terminate_flag = TRUE;
	// send ^D to main thread input queue
	h = GetStdHandle(STD_INPUT_HANDLE);
	ZeroMemory(&rec, sizeof(rec));
	rec.EventType = KEY_EVENT;
	rec.Event.KeyEvent.bKeyDown = TRUE;
	rec.Event.KeyEvent.wRepeatCount = 1;
	rec.Event.KeyEvent.uChar.AsciiChar = 004;
	WriteConsoleInput(h, &rec, 1, &written);
#endif
	// give the main thread time to exit
	Sleep(10000);
	return TRUE;
    }
    default:
	break;
    }
    return FALSE;
}


/* public interface to printer routines : Windows PRN emulation
 * (formerly in win.trm)
 */

#define MAX_PRT_LEN 256
static char win_prntmp[MAX_PRT_LEN+1];

FILE *
open_printer(void)
{
    char *temp;

    if ((temp = getenv("TEMP")) == NULL)
	*win_prntmp = '\0';
    else  {
	safe_strncpy(win_prntmp, temp, MAX_PRT_LEN);
	/* stop X's in path being converted by _mktemp */
	for (temp = win_prntmp; *temp != NUL; temp++)
	    *temp = tolower((unsigned char)*temp);
	if ((strlen(win_prntmp) > 0) && (win_prntmp[strlen(win_prntmp) - 1] != '\\'))
	    strcat(win_prntmp, "\\");
    }
    strncat(win_prntmp, "_gptmp", MAX_PRT_LEN - strlen(win_prntmp));
    strncat(win_prntmp, "XXXXXX", MAX_PRT_LEN - strlen(win_prntmp));
    _mktemp(win_prntmp);
    return fopen(win_prntmp, "wb");
}


void
close_printer(FILE *outfile)
{
    LPTSTR fname;
    HWND hwnd;
    TCHAR title[100];

#ifdef UNICODE
    fname = UnicodeText(win_prntmp, S_ENC_DEFAULT);
#else
    fname = win_prntmp;
#endif
    fclose(outfile);

#ifndef WGP_CONSOLE
    hwnd = textwin.hWndParent;
#else
    hwnd = GetDesktopWindow();
#endif
    if (term->name != NULL)
	wsprintf(title, TEXT("gnuplot graph (%hs)"), term->name);
    else
	_tcscpy(title, TEXT("gnuplot graph"));
    DumpPrinter(hwnd, title, fname);

#ifdef UNICODE
    free(fname);
#endif
}


void
screen_dump(void)
{
    if (term == NULL) {
	int_error(c_token, "");
    }
    if (strcmp(term->name, "windows") == 0)
	GraphPrint(graphwin);
#ifdef WXWIDGETS
    else if (strcmp(term->name, "wxt") == 0)
	wxt_screen_dump();
#endif
#ifdef QTTERM
    //else if (strcmp(term->name, "qt") == 0)
#endif
    else
	int_error(c_token, "screendump not supported for terminal `%s`", term->name);
}


void
win_raise_terminal_window(int id)
{
    LPGW lpgw = listgraphs;
    while ((lpgw != NULL) && (lpgw->Id != id))
	lpgw = lpgw->next;
    if (lpgw != NULL) {
	if (IsIconic(lpgw->hWndGraph))
	    ShowWindow(lpgw->hWndGraph, SW_SHOWNORMAL);
	BringWindowToTop(lpgw->hWndGraph);
    }
}


void
win_raise_terminal_group(void)
{
    LPGW lpgw = listgraphs;
    while (lpgw != NULL) {
	if (IsIconic(lpgw->hWndGraph))
	    ShowWindow(lpgw->hWndGraph, SW_SHOWNORMAL);
	BringWindowToTop(lpgw->hWndGraph);
	lpgw = lpgw->next;
    }
}


void
win_lower_terminal_window(int id)
{
    LPGW lpgw = listgraphs;
    while ((lpgw != NULL) && (lpgw->Id != id))
	lpgw = lpgw->next;
    if (lpgw != NULL)
	SetWindowPos(lpgw->hWndGraph, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
}


void
win_lower_terminal_group(void)
{
    LPGW lpgw = listgraphs;
    while (lpgw != NULL) {
	SetWindowPos(lpgw->hWndGraph, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
	lpgw = lpgw->next;
    }
}


/* returns true if there are any graph windows open (win terminal) */
static TBOOLEAN
WinWindowOpened(void)
{
    LPGW lpgw;

    lpgw = listgraphs;
    while (lpgw != NULL) {
	if (GraphHasWindow(lpgw))
	    return TRUE;
	lpgw = lpgw->next;
    }
    return FALSE;
}


/* returns true if there are any graph windows open (wxt/caca/win terminals) */
/* Note: This routine is used to handle "persist". Do not test for qt windows here
         since they run in a separate process */
TBOOLEAN
WinAnyWindowOpen(void)
{
    TBOOLEAN window_opened = WinWindowOpened();
#ifdef WXWIDGETS
    window_opened |= wxt_window_opened();
#endif
#ifdef HAVE_LIBCACA
    window_opened |= CACA_window_opened();
#endif
    return window_opened;
}


#ifndef WGP_CONSOLE
void
WinPersistTextClose(void)
{
    if (!WinAnyWindowOpen() &&
	(textwin.hWndParent != NULL) && !IsWindowVisible(textwin.hWndParent))
	PostMessage(textwin.hWndParent, WM_CLOSE, 0, 0);
}
#endif


void
WinMessageLoop(void)
{
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
	/* HBB 19990505: Petzold says we should check this: */
	if (msg.message == WM_QUIT)
	    return;
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }
}


#ifndef WGP_CONSOLE
void
WinOpenConsole(void)
{
    /* Try to attach to an existing console window. */
    if (AttachConsole(ATTACH_PARENT_PROCESS) == 0) {
	if (GetLastError() != ERROR_ACCESS_DENIED) {
	    /* Open new console if we are are not attached to one already.
	       Note that closing this console window will end wgnuplot, too. */
	    AllocConsole();
	}
    }
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
}
#endif


void
WinRaiseConsole(void)
{
    HWND console = NULL;
#ifndef WGP_CONSOLE
    console = textwin.hWndParent;
    if (pausewin.bPause && IsWindow(pausewin.hWndPause))
	console = pausewin.hWndPause;
#else
    console = GetConsoleWindow();
#endif
    if (console != NULL) {
	if (IsIconic(console))
	    ShowWindow(console, SW_SHOWNORMAL);
	BringWindowToTop(console);
    }
}


/* WinGetCodepage:
    Map gnuplot's internal character encoding to Windows codepage codes.
*/
UINT
WinGetCodepage(enum set_encoding_id encoding)
{
    UINT codepage;

    /* For a list of code page identifiers see
	http://msdn.microsoft.com/en-us/library/dd317756%28v=vs.85%29.aspx
    */
    switch (encoding) {
	case S_ENC_DEFAULT:    codepage = CP_ACP; break;
	case S_ENC_ISO8859_1:  codepage = 28591; break;
	case S_ENC_ISO8859_2:  codepage = 28592; break;
	case S_ENC_ISO8859_9:  codepage = 28599; break;
	case S_ENC_ISO8859_15: codepage = 28605; break;
	case S_ENC_CP437:      codepage =   437; break;
	case S_ENC_CP850:      codepage =   850; break;
	case S_ENC_CP852:      codepage =   852; break;
	case S_ENC_CP950:      codepage =   950; break;
	case S_ENC_CP1250:     codepage =  1250; break;
	case S_ENC_CP1251:     codepage =  1251; break;
	case S_ENC_CP1252:     codepage =  1252; break;
	case S_ENC_CP1254:     codepage =  1254; break;
	case S_ENC_KOI8_R:     codepage = 20866; break;
	case S_ENC_KOI8_U:     codepage = 21866; break;
	case S_ENC_SJIS:       codepage =   932; break;
	case S_ENC_UTF8:       codepage = CP_UTF8; break;
	default: {
	    /* unknown encoding, fall back to default "ANSI" codepage */
	    codepage = CP_ACP;
	    FPRINTF((stderr, "unknown encoding: %i\n", encoding));
	}
    }
    return codepage;
}


LPWSTR
UnicodeText(LPCSTR str, enum set_encoding_id encoding)
{
    LPWSTR strw = NULL;
    UINT codepage = WinGetCodepage(encoding);
    int length;

    /* sanity check */
    if (str == NULL)
	return NULL;

    /* get length of converted string */
    length = MultiByteToWideChar(codepage, 0, str, -1, NULL, 0);
    strw = (LPWSTR) malloc(sizeof(WCHAR) * length);

    /* convert string to UTF-16 */
    length = MultiByteToWideChar(codepage, 0, str, -1, strw, length);

    return strw;
}


LPSTR
AnsiText(LPCWSTR strw,  enum set_encoding_id encoding)
{
    LPSTR str = NULL;
    UINT codepage = WinGetCodepage(encoding);
    int length;

    /* get length of converted string */
    length = WideCharToMultiByte(codepage, 0, strw, -1, NULL, 0, NULL, 0);
    str = (LPSTR) malloc(sizeof(char) * length);

    /* convert string to "Ansi" */
    length = WideCharToMultiByte(codepage, 0, strw, -1, str, length, NULL, 0);

    return str;
}


FILE *
win_fopen(const char *filename, const char *mode)
{
    FILE * file;
    LPWSTR wfilename = UnicodeText(filename, encoding);
    LPWSTR wmode = UnicodeText(mode, encoding);
    file = _wfopen(wfilename, wmode);
    if (file == NULL) {
	/* "encoding" didn't work, try UTF-8 instead */
	free(wfilename);
	wfilename = UnicodeText(filename, S_ENC_UTF8);
	file = _wfopen(wfilename, wmode);
    }
    free(wfilename);
    free(wmode);
    return file;
}


#ifndef USE_FAKEPIPES
FILE *
win_popen(const char *filename, const char *mode)
{
    FILE * file;
    LPWSTR wfilename = UnicodeText(filename, encoding);
    LPWSTR wmode = UnicodeText(mode, encoding);
    file = _wpopen(wfilename, wmode);
    free(wfilename);
    free(wmode);
    return file;
}
#endif


UINT
GetDPI(void)
{
    HDC hdc_screen = GetDC(NULL);
    if (hdc_screen) {
	UINT dpi = GetDeviceCaps(hdc_screen, LOGPIXELSX);
	ReleaseDC(NULL, hdc_screen);
	return dpi;
    } else {
	return 96;
    }
}
