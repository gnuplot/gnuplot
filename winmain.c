#ifndef lint
static char *RCSid = "$Id: winmain.c%v 3.50 1993/07/09 05:35:24 woo Exp $";
#endif

/* GNUPLOT - win/winmain.c */
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
 *  info-gnuplot-request@dartmouth.edu.  
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
#ifdef __MSC__
#include <malloc.h>
#else
#include <alloc.h>
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

extern char version[];
extern char patchlevel[];
extern char date[];
extern char *authors[];
extern char copyright[];
extern void close_printer();
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
	if (term && term_init)
		(*term_tbl[term].reset)();
	if ( !strcmp(outstr,"'PRN'") )
		close_printer();
	fcloseall();
	if (graphwin.hWndGraph && IsWindow(graphwin.hWndGraph))
		GraphClose(&graphwin);
	TextMessage();	/* process messages */
 	WinHelp(textwin.hWndText,(LPSTR)winhelpname,HELP_QUIT,(DWORD)NULL);
	TextClose(&textwin);
	TextMessage();	/* process messages */
	return;
}

/* call back function from Text Window WM_CLOSE */
int CALLBACK _export
ShutDown(void)
{
	WinExit();
	exit(0);
	return 0;
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		LPSTR lpszCmdLine, int nCmdShow)
{
	WNDCLASS wndclass;
	LPSTR tail;
	
#if defined(__MSC__)    /* MSC doesn't give us _argc and _argv[] so ...   */
#define MAXCMDTOKENS 128
	int     _argc=0;
	LPSTR   _argv[MAXCMDTOKENS];
	_argv[_argc] = "wgnuplot.exe";
	_argv[++_argc] = _fstrtok( lpszCmdLine, " ");
	while (_argv[_argc] != NULL)
		_argv[++_argc] = _fstrtok( NULL, " ");
#endif

  	szModuleName = (LPSTR)farmalloc(MAXSTR+1);
  	CheckMemory(szModuleName);
	szModuleName = (LPSTR)farmalloc(MAXSTR+1);
	CheckMemory(szModuleName);

	/* get path to EXE */
	GetModuleFileName(hInstance, (LPSTR) szModuleName, MAXSTR);
	if (CheckWGNUPLOTVersion(WGNUPLOTVERSION)) {
		MessageBox(NULL, "Wrong version of WGNUPLOT.DLL", szModuleName, MB_ICONSTOP | MB_OK);
		exit(1);
	}
	if ((tail = _fstrrchr(szModuleName,'\\')) != (LPSTR)NULL)
	{
		tail++;
		*tail = NULL;
	}
	szModuleName = farrealloc(szModuleName, _fstrlen(szModuleName)+1);
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
	sprintf(textwin.AboutText,"Version %s\nPatchlevel %s\nLast Modified %s\n%s\n%s, %s",
		version, patchlevel, date, copyright, authors[1], authors[0]);
	textwin.AboutText = farrealloc(textwin.AboutText, _fstrlen(textwin.AboutText)+1);
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
	textwin.hIcon = LoadIcon(hInstance, "texticon");
#ifdef WIN32
	SetClassLong(textwin.hWndParent, GCL_HICON, (DWORD)textwin.hIcon);
#else
	SetClassWord(textwin.hWndParent, GCW_HICON, (WORD)textwin.hIcon);
#endif
	if (_argc>1)
		ShowWindow(textwin.hWndParent,SW_SHOWMINIMIZED);
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

#ifdef __MSC__
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
		int i;
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
		int i;
		for (i=0; i<n; i++)
			((BYTE *)ptr)[i] = TextGetChE(&textwin);
		TextMessage();
		return n;
	}
	return fread(ptr, size, n, file);
}
