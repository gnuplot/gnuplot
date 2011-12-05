#ifndef lint
static char *RCSid() { return RCSid("$Id: winmain.c,v 1.52 2011/11/14 21:03:38 markisch Exp $"); }
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
/* under Microsoft Windows.                                           */
/*                                                                    */
/* The modifications to allow Gnuplot to run under Windows were made  */
/* by Maurice Castro. (maurice@bruce.cs.monash.edu.au)  3 Jul 1992    */
/* and Russell Lang (rjl@monu1.cc.monash.edu.au) 30 Nov 1992          */
/*                                                                    */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#define STRICT
/* required for MinGW64 */
#define _WIN32_IE 0x0501
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#ifdef WITH_HTML_HELP
#include <htmlhelp.h>
#endif
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
#ifdef __WATCOMC__
# define mktemp _mktemp
#endif
#include <io.h>
#include <sys/stat.h>
#include "plot.h"
#include "setshow.h"
#include "version.h"
#include "command.h"
#include "winmain.h"
#include "wtext.h"
#include "wcommon.h"
#ifdef HAVE_GDIPLUS
#include "wgdiplus.h"
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
LPSTR szModuleName;
LPSTR szPackageDir;
LPSTR winhelpname;
LPSTR szMenuName;
static LPSTR szLanguageCode = NULL;
#if defined(WGP_CONSOLE) && defined(CONSOLE_SWITCH_CP)
BOOL cp_changed = FALSE;
UINT cp_input;  /* save previous codepage settings */
UINT cp_output;
#endif
#ifdef WITH_HTML_HELP
HWND help_window = NULL;
#endif

char *authors[]={
                 "Colin Kelley",
                 "Thomas Williams"
                };

void WinExit(void);
int gnu_main(int argc, char *argv[], char *env[]);
static void WinCloseHelp(void);
int CALLBACK ShutDown();


static void
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
        pausewin.bPause = FALSE;
}

/* atexit procedure */
void
WinExit(void)
{
	LPGW lpgw;

    /* Last chance, call before anything else to avoid a crash. */
    WinCloseHelp();

    term_reset();

#ifndef __MINGW32__ /* HBB 980809: FIXME: doesn't exist for MinGW32. So...? */
    fcloseall();
#endif

	/* Close all graph windows */
	for (lpgw = listgraphs; lpgw != NULL; lpgw = lpgw->next) {
		if (GraphHasWindow(lpgw))
			GraphClose(lpgw);
	}

#ifndef WGP_CONSOLE
    TextMessage();  /* process messages */
#ifndef WITH_HTML_HELP
    WinHelp(textwin.hWndText,(LPSTR)winhelpname,HELP_QUIT,(DWORD)NULL);
#endif
    TextMessage();  /* process messages */
#else
#ifndef WITH_HTML_HELP
    WinHelp(GetDesktopWindow(), (LPSTR)winhelpname, HELP_QUIT, (DWORD)NULL);
#endif
#ifdef CONSOLE_SWITCH_CP
    /* restore console codepages */
    if (cp_changed) {
		SetConsoleCP(cp_input);
		SetConsoleOutputCP(cp_output);
		/* file APIs are per process */
    }
#endif
#endif
#ifdef HAVE_GDIPLUS
    gdiplusCleanup();
#endif
    return;
}

/* call back function from Text Window WM_CLOSE */
int CALLBACK
ShutDown()
{
	/* First chance for wgnuplot to close help system. */
	WinCloseHelp();
	exit(0);
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


BOOL IsWindowsXPorLater(void)
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


static void
WinCloseHelp(void)
{
#ifdef WITH_HTML_HELP
	/* Due to a known bug in the HTML help system we have to
	 * call this as soon as possible before the end of the program.
	 * See e.g. http://helpware.net/FAR/far_faq.htm#HH_CLOSE_ALL
	 */
	if (IsWindow(help_window))
		SendMessage(help_window, WM_CLOSE, 0, 0);
	Sleep(0);
#endif
}


static char * 
GetLanguageCode()
{
	static char lang[6] = "";

	if (lang[0] == NUL) {
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME, lang, sizeof(lang));
		//strcpy(lang, "JPN"); //TEST
		/* language definition files for Japanese already use "ja" as abbreviation */
		if (strcmp(lang, "JPN") == 0)
			lang[1] = 'A';
		/* prefer lower case */
		lang[0] = tolower(lang[0]);
		lang[1] = tolower(lang[1]);
		/* only use two character sequence */
		lang[2] = NUL;
	}

	return lang;
}


static BOOL
FileExists(const char * filename)
{
	struct stat buffer;
	return stat(filename, &buffer) == 0;
}


static char *
LocalisedFile(const char * name, const char * ext, const char * defaultname)
{
	char * lang;
	char * filename;

	/* Allow user to override language detection. */
	if (szLanguageCode)
		lang = szLanguageCode;
	else
		lang = GetLanguageCode();

	filename = (LPSTR) malloc(strlen(szModuleName) + strlen(name) + strlen(lang) + strlen(ext) + 1);
	if (filename) {
		strcpy(filename, szModuleName);
		strcat(filename, name);
		strcat(filename, lang);
		strcat(filename, ext);
		if (!FileExists(filename)) {
			strcpy(filename, szModuleName);
			strcat(filename, defaultname);
		}
	}
	return filename;
}


static void
ReadMainIni(LPSTR file, LPSTR section)
{
    char profile[81] = "";
#ifdef WITH_HTML_HELP
	const char hlpext[] = ".chm";
#else
	const char hlpext[] = ".hlp";
#endif
	const char name[] = "wgnuplot-";

	/* Language code override */
	GetPrivateProfileString(section, "Language", "", profile, 80, file);
	if (profile[0] != NUL)
		szLanguageCode = strdup(profile);
	else
		szLanguageCode = NULL;

	/* help file name */
	GetPrivateProfileString(section, "HelpFile", "", profile, 80, file);
	if (profile[0] != NUL) {
		winhelpname = (LPSTR) malloc(strlen(szModuleName) + strlen(profile) + 1);
		if (winhelpname) {
			strcpy(winhelpname, szModuleName);
			strcat(winhelpname, profile);
		}
	} else {
		/* default name is "wgnuplot-LL.chm" */
		winhelpname = LocalisedFile(name, hlpext, HELPFILE);
	}

	/* menu file name */
	GetPrivateProfileString(section, "MenuFile", "", profile, 80, file);
	if (profile[0] != NUL) {
		szMenuName = (LPSTR) malloc(strlen(szModuleName) + strlen(profile) + 1);
		if (szMenuName) {
			strcpy(szMenuName, szModuleName);
			strcat(szMenuName, profile);
		}
	} else {
		/* default name is "wgnuplot-LL.mnu" */
		szMenuName = LocalisedFile(name, ".mnu", "wgnuplot.mnu");
	}
}


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
#else
#if defined(__MSC__) || defined(__WATCOMC__)
#  define _argv __argv
#  define _argc __argc
#endif
#endif /* WGP_CONSOLE */

        szModuleName = (LPSTR)malloc(MAXSTR+1);
        CheckMemory(szModuleName);

        /* get path to EXE */
        GetModuleFileName(hInstance, (LPSTR) szModuleName, MAXSTR);
        if ((tail = (LPSTR)_fstrrchr(szModuleName,'\\')) != (LPSTR)NULL)
        {
                tail++;
                *tail = 0;
        }
        szModuleName = (LPSTR)realloc(szModuleName, _fstrlen(szModuleName)+1);
        CheckMemory(szModuleName);

        if (_fstrlen(szModuleName) >= 5 && _fstrnicmp(&szModuleName[_fstrlen(szModuleName)-5], "\\bin\\", 5) == 0)
        {
                int len = _fstrlen(szModuleName)-4;
                szPackageDir = (LPSTR)malloc(len+1);
                CheckMemory(szPackageDir);
                _fstrncpy(szPackageDir, szModuleName, len);
                szPackageDir[len] = '\0';
        }
        else
                szPackageDir = szModuleName;

#ifndef WGP_CONSOLE
        textwin.hInstance = hInstance;
        textwin.hPrevInstance = hPrevInstance;
        textwin.nCmdShow = nCmdShow;
        textwin.Title = "gnuplot";
#endif

		/* create structure of first graph window */
		graphwin = calloc(1, sizeof(GW));
		listgraphs = graphwin;

		/* locate ini file */
		{
			char * inifile;
			get_user_env(); /* this hasn't been called yet */
			inifile = gp_strdup("~\\wgnuplot.ini");
			gp_expand_tilde(&inifile);

			/* if tilde expansion fails use current directory as
			   default - that was the previous default behaviour */
			if (inifile[0] == '~') {
				free(inifile);
				inifile = "wgnuplot.ini";
			}

#ifndef WGP_CONSOLE
			textwin.IniFile = inifile;
#endif
			graphwin->IniFile = inifile;

			ReadMainIni(inifile, "WGNUPLOT");
		}

#ifndef WGP_CONSOLE
        textwin.IniSection = "WGNUPLOT";
        textwin.DragPre = "load '";
        textwin.DragPost = "'\n";
        textwin.lpmw = &menuwin;
        textwin.ScreenSize.x = 80;
        textwin.ScreenSize.y = 80;
        textwin.KeyBufSize = 2048;
        textwin.CursorFlag = 1; /* scroll to cursor after \n & \r */
        textwin.shutdown = MakeProcInstance((FARPROC)ShutDown, hInstance);
        textwin.AboutText = (LPSTR)malloc(1024);
        CheckMemory(textwin.AboutText);
        sprintf(textwin.AboutText,
	    "Version %s patchlevel %s\n" \
	    "last modified %s\n" \
	    "%s\n%s, %s and many others\n" \
	    "gnuplot home:     http://www.gnuplot.info\n",
            gnuplot_version, gnuplot_patchlevel,
	    gnuplot_date,
	    gnuplot_copyright, authors[1], authors[0]);
        textwin.AboutText = (LPSTR)realloc(textwin.AboutText, _fstrlen(textwin.AboutText)+1);
        CheckMemory(textwin.AboutText);

        menuwin.szMenuName = szMenuName;
#endif

        pausewin.hInstance = hInstance;
        pausewin.hPrevInstance = hPrevInstance;
        pausewin.Title = "gnuplot pause";

        graphwin->hInstance = hInstance;
        graphwin->hPrevInstance = hPrevInstance;
#ifdef WGP_CONSOLE
        graphwin->lptw = NULL;
#else
        graphwin->lptw = &textwin;
#endif

		/* init common controls */
	{
	    INITCOMMONCONTROLSEX initCtrls;
	    initCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
	    initCtrls.dwICC = ICC_WIN95_CLASSES;
	    InitCommonControlsEx(&initCtrls);
	}

#ifndef WGP_CONSOLE
        if (TextInit(&textwin))
                exit(1);
        textwin.hIcon = LoadIcon(hInstance, "TEXTICON");
        SetClassLong(textwin.hWndParent, GCL_HICON, (DWORD)textwin.hIcon);
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
#else /* WGP_CONSOLE */
#ifdef CONSOLE_SWITCH_CP
        /* Change codepage of console to match that of the graph window.
           WinExit() will revert this.
           Attention: display of characters does not work correctly with
           "Terminal" font! Users will have to use "Lucida Console" or similar.
        */
        cp_input = GetConsoleCP();
        cp_output = GetConsoleOutputCP();
        if (cp_input != GetACP()) {
            cp_changed = TRUE;
            SetConsoleCP(GetACP()); /* keyboard input */
            SetConsoleOutputCP(GetACP()); /* screen output */
            SetFileApisToANSI(); /* file names etc. */
        }
#endif
#endif

        atexit(WinExit);

        if (!isatty(fileno(stdin)))
            setmode(fileno(stdin), O_BINARY);

        gnu_main(_argc, _argv, environ);

        /* First chance to close help system for console gnuplot,
        second for wgnuplot */
        WinCloseHelp();
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

#define isterm(f) (f==stdin || f==stdout || f==stderr)

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
    char *p;

    if (isterm(file)) {
        p = TextGetS(&textwin, str, size);
        if (p != (char *)NULL)
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
        return (*str);  /* different from Borland library */
    }
    return fputs(str,file);
}

int
MyPutS(char *str)
{
    TextPutS(&textwin, str);
    MyPutCh('\n');
    TextMessage();
    return 0;   /* different from Borland library */
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

#ifdef __MSC__
        count = _vscprintf(fmt, args) + 1;
#else
        va_list args_copied;
        va_copy(args_copied, args);
        count = vsnprintf(NULL, 0U, fmt, args_copied) + 1;
        if (count == 0) count = MAXPRINTF;
        va_end(args_copied);
#endif
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
                            case VK_DELETE: return 0117;
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
    DumpPrinter(graphwin->hWndGraph, graphwin->Title, win_prntmp);
}

void
screen_dump()
{
    GraphPrint(graphwin);
}


void
win_raise_terminal_window(int id)
{
	LPGW lpgw = listgraphs;
	while ((lpgw != NULL) && (lpgw->Id != id))
		lpgw = lpgw->next;
	if (lpgw != NULL) {
		ShowWindow(lpgw->hWndGraph, SW_SHOWNORMAL);
		BringWindowToTop(lpgw->hWndGraph);
	}
}

void
win_raise_terminal_group(void)
{
	LPGW lpgw = listgraphs;
	while (lpgw != NULL) {
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

