#ifndef lint
static char *RCSid = "$Id: winmain.c,v 1.3.2.2 2000/10/22 13:50:51 joze Exp $";
#endif

/* GNUPLOT - win/winmain.c */
/*[
 * Copyright 1992, 1993, 1998   Maurice Castro, Russell Lang
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
 * Send your comments or suggestions to 
 *  info-gnuplot@dartmouth.edu.
 * This is a mailing list; to join it send a note to 
 *  majordomo@dartmouth.edu.  
 * Send bug reports to
 *  bug-gnuplot@dartmouth.edu.
 */

/* This file implements the initialization code for running gnuplot   */
/* under Microsoft Windows. The code currently compiles only with the */
/* Borland C++ 3.1 compiler. 
/*                                                                    */
/* The modifications to allow Gnuplot to run under Windows were made  */
/* by Maurice Castro. (maurice@bruce.cs.monash.edu.au)  3 Jul 1992    */
/* and Russell Lang (rjl@monu1.cc.monash.edu.au) 30 Nov 1992          */
/*                                                                    */
 
#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#ifdef __MSC__
#include <malloc.h>
#else
# ifdef __TURBOC__ /* HBB 981201: MinGW32 doesn't have this */
#include <alloc.h>
#endif
#endif
#include <io.h>
#include "plot.h"
#include "setshow.h"
#include "wgnuplib.h"
#include "wtext.h"

/* limits */
#define MAXSTR 255
#define MAXPRINTF 1024

/* globals */
TW textwin;
GW graphwin;
PW pausewin;
MW menuwin;
LPSTR szModuleName;
LPSTR winhelpname;
LPSTR szMenuName;
#define MENUNAME "wgnuplot.mnu"
#ifndef HELPFILE /* HBB 981203: makefile.win predefines this... */
#define HELPFILE "wgnuplot.hlp"
#endif

#if 0 /* HBB 990914: new names, and now decl'd in plot.h... */
extern char version[];
extern char patchlevel[];
extern char date[];
#endif /* 1/0 */
/*extern char *authors[];*/
char *authors[]={
                 "Colin Kelly",
                 "Thomas Williams"
                };
 
/* extern char gnuplot_copyright[]; */ /* HBB 990914: now decl'd in plot.h */
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

/* atexit procedure */
void
WinExit(void)
{
	term_reset();

#ifndef __MINGW32__ /* HBB 980809: FIXME: doesn't exist for MinGW32. So...? */
	fcloseall();
#endif
	if (graphwin.hWndGraph && IsWindow(graphwin.hWndGraph))
		GraphClose(&graphwin);
	TextMessage();	/* process messages */
 	WinHelp(textwin.hWndText,(LPSTR)winhelpname,HELP_QUIT,(DWORD)NULL);
	TextClose(&textwin);
	TextMessage();	/* process messages */
	return;
}

/* call back function from Text Window WM_CLOSE */
int CALLBACK WINEXPORT
ShutDown(void)
{
#if 0  /* HBB 19990505: try to avoid crash on clicking 'close' */
       /* Problem was that WinExit was called *twice*, once directly,
        * and again via 'atexit'. This caused problems by double-freeing
        * of GlobalAlloc-ed memory inside TextClose() */
       /* Caveat: relies on atexit() working properly */
	WinExit();
#endif
	exit(0);
	return 0;
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		LPSTR lpszCmdLine, int nCmdShow)
{
	/*WNDCLASS wndclass;*/
	LPSTR tail;
	
#ifdef __MSC__  /* MSC doesn't give us _argc and _argv[] so ...   */
#define MAXCMDTOKENS 128
	int     _argc=0;
	LPSTR   _argv[MAXCMDTOKENS];
	_argv[_argc] = "wgnuplot.exe";
	_argv[++_argc] = _fstrtok( lpszCmdLine, " ");
	while (_argv[_argc] != NULL)
		_argv[++_argc] = _fstrtok( NULL, " ");
#endif /* __MSC__ */

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
	textwin.IniFile = "wgnuplot.ini";
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
	graphwin.Title = "gnuplot graph";
	graphwin.lptw = &textwin;
	graphwin.IniFile = textwin.IniFile;
	graphwin.IniSection = textwin.IniSection;
	graphwin.color=TRUE;
	graphwin.fontsize = WINFONTSIZE;

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
			if (!stricmp(_argv[i],"-noend") || !stricmp(_argv[i],"/noend"))
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


	atexit(WinExit);

	gnu_main(_argc, _argv, environ);

	return 0;
}


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
		TextMessage();
		return ch;
	}
	return fputc(ch,file);
}

int
MyFPutS(char *str, FILE *file)
{
	if (isterm(file)) {
		TextPutS(&textwin, str);
		TextMessage();
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

int MyFPrintF(FILE *file, char *fmt, ...)
{
int count;
va_list args;
	va_start(args,fmt);
	if (isterm(file)) {
		char buf[MAXPRINTF];
		count = vsprintf(buf,fmt,args);
		TextPutS(&textwin,buf);
	}
	else
		count = vfprintf(file, fmt, args);
	va_end(args);
	return count;
}

int MyVFPrintF(FILE *file, char *fmt, va_list args)
{
	int count;

	if (isterm(file)) {
        	char buf[MAXPRINTF];
                count = vsprintf(buf,fmt,args);
                TextPutS(&textwin, buf);
        } else
        	count = vfprintf(file, fmt, args);
        return count;
}

int MyPrintF(char *fmt, ...)
{
int count;
char buf[MAXPRINTF];
va_list args;
	va_start(args,fmt);
	count = vsprintf(buf,fmt,args);
	TextPutS(&textwin,buf);
	va_end(args);
	return count;
}

size_t MyFWrite(const void *ptr, size_t size, size_t n, FILE *file)
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

size_t MyFRead(void *ptr, size_t size, size_t n, FILE *file)
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
screen_dump(void)
{
	GraphPrint(&graphwin);
}


