#ifndef lint
static char *RCSid() { return RCSid("$Id: wmenu.c,v 1.4 2003/01/23 13:22:05 broeker Exp $"); }
#endif

/* GNUPLOT - win/wmenu.c */
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

#define STRICT
#include <windows.h>
#include <windowsx.h>
#if WINVER >= 0x030a
#include <commdlg.h>
#endif
#include <string.h>	/* only use far items */
#include "wgnuplib.h"
#include "wresourc.h"
#include "wcommon.h"

BOOL CALLBACK WINEXPORT InputBoxDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WINEXPORT MenuButtonProc(HWND, UINT, WPARAM, LPARAM);

/* limits */
#define MAXSTR 255
#define MACROLEN 10000
/* #define NUMMENU 256  defined in wresourc.h */
#define MENUDEPTH 3

/* menu tokens */
#define CMDMIN 129
#define INPUT 129
#define EOS 130
#define OPEN 131
#define SAVE 132
#define CMDMAX 132
char * keyword[] = {
	"[INPUT]", "[EOS]", "[OPEN]", "[SAVE]",
        "{ENTER}", "{ESC}", "{TAB}",
        "{^A}", "{^B}", "{^C}", "{^D}", "{^E}", "{^F}", "{^G}", "{^H}", 
	"{^I}", "{^J}", "{^K}", "{^L}", "{^M}", "{^N}", "{^O}", "{^P}", 
	"{^Q}", "{^R}", "{^S}", "{^T}", "{^U}", "{^V}", "{^W}", "{^X}", 
	"{^Y}", "{^Z}", "{^[}", "{^\\}", "{^]}", "{^^}", "{^_}",
	NULL};
BYTE keyeq[] = {
	INPUT, EOS, OPEN, SAVE,
        13, 27, 9,
        1, 2, 3, 4, 5, 6, 7, 8,
	9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 
	25, 26, 28, 29, 30, 31,
	0};


/* Send a macro to the text window */
void
SendMacro(LPTW lptw, UINT m)
{
BYTE FAR *s;
char *d;
char *buf;
BOOL flag=TRUE;
int i;
LPMW lpmw = lptw->lpmw;
#if WINVER >= 0x030a
OPENFILENAME ofn;
char *szTitle;
char *szFile;
char *szFilter;
#endif

	if ( (buf = LocalAllocPtr(LHND, MAXSTR+1)) == (char *)NULL )
		return;

	if (m>=lpmw->nCountMenu)
		return;
	s = lpmw->macro[m];
	d = buf;
	*d = '\0';
	while (s && *s && (d-buf < MAXSTR)) {
	    if (*s>=CMDMIN  && *s<=CMDMAX) {
		switch (*s) {
			case SAVE: /* [SAVE] - get a save filename from a file list box */
			case OPEN: /* [OPEN] - get a filename from a file list box */
#if WINVER >= 0x030a
				/* This uses COMMDLG.DLL from Windows 3.1 
				   COMMDLG.DLL is redistributable */
				{
				BOOL save;
				if ( (szTitle = LocalAllocPtr(LHND, MAXSTR+1)) == (char *)NULL )
					return;
				if ( (szFile = LocalAllocPtr(LHND, MAXSTR+1)) == (char *)NULL )
					return;
				if ( (szFilter = LocalAllocPtr(LHND, MAXSTR+1)) == (char *)NULL )
					return;

				save = (*s==SAVE);
				s++;
						for(i=0; (*s >= 32 && *s <= 126); i++)
							szTitle[i] = *s++;	/* get dialog box title */
				szTitle[i]='\0';
				s++;
						for(i=0; (*s >= 32 && *s <= 126); i++)
							szFile[i] = *s++;	/* temporary copy of filter */
				szFile[i++]='\0';
				lstrcpy(szFilter,"Default (");
				lstrcat(szFilter,szFile);
				lstrcat(szFilter,")");
				i=lstrlen(szFilter);
				i++;	/* move past NULL */
				lstrcpy(szFilter+i,szFile);
				i+=lstrlen(szFilter+i);
				i++;	/* move past NULL */
				lstrcpy(szFilter+i,"All Files (*.*)");
				i+=lstrlen(szFilter+i);
				i++;	/* move past NULL */
				lstrcpy(szFilter+i,"*.*");
				i+=lstrlen(szFilter+i);
				i++;	/* move past NULL */
				szFilter[i++]='\0';	/* add a second NULL */
				flag = 0;

				/* the Windows 3.1 implentation - MC */
				szFile[0] = '\0';
				/* clear the structrure */
				_fmemset(&ofn, 0, sizeof(OPENFILENAME));
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = lptw->hWndParent;
				ofn.lpstrFilter = szFilter;
				ofn.nFilterIndex = 1;
				ofn.lpstrFile = szFile;
				ofn.nMaxFile = MAXSTR;
				ofn.lpstrFileTitle = szFile;
				ofn.nMaxFileTitle = MAXSTR;
				ofn.lpstrTitle = szTitle;
				ofn.lpstrInitialDir = (LPSTR)NULL;
				ofn.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
				flag = (save ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn));
				if( flag ) {
					lpmw->nChar = lstrlen(ofn.lpstrFile);
					for (i=0; i<lpmw->nChar; i++)
						*d++=ofn.lpstrFile[i];
				}

				LocalFreePtr((void NEAR *)OFFSETOF(szTitle));
				LocalFreePtr((void NEAR *)OFFSETOF(szFilter));
				LocalFreePtr((void NEAR *)OFFSETOF(szFile));

				}
				break;
#else
				/* Use InputBox if you don't have COMMDLG.DLL */
				s++;	/* skip list box title */
				for(i=0; (*s >= 32 && *s <= 126); i++)
					s++;
#endif
			case INPUT: /* [INPUT] - input a string of characters */
				s++;
				for(i=0; (*s >= 32 && *s <= 126); i++)
					lpmw->szPrompt[i] = *s++;
				lpmw->szPrompt[i]='\0';
#ifdef WIN32
				flag = DialogBox( hdllInstance, "InputDlgBox", lptw->hWndParent, InputBoxDlgProc);
#else
#ifdef __DLL__
				lpmw->lpProcInput = (DLGPROC)GetProcAddress(hdllInstance, "InputBoxDlgProc");
#else
				lpmw->lpProcInput = (DLGPROC)MakeProcInstance((FARPROC)InputBoxDlgProc, hdllInstance);
#endif
				flag = DialogBox( hdllInstance, "InputDlgBox", lptw->hWndParent, lpmw->lpProcInput);
#endif
				if( flag ) {
					for (i=0; i<lpmw->nChar; i++)
						*d++=lpmw->szAnswer[i];
				}
#ifndef WIN32
#ifndef __DLL__
				FreeProcInstance((FARPROC)lpmw->lpProcInput);
#endif
#endif
				break;
		    case EOS: /* [EOS] - End Of String - do nothing */
				default:
				s++;
				break;
		}
		if (!flag) { /* abort */
			d = buf;
			s = (BYTE FAR *)"";
		}
	    }
	    else {
		*d++ = *s++;
	    }
	}
	*d = '\0';
	if (buf[0]!='\0') {
		d = buf;
		while (*d) {
			SendMessage(lptw->hWndText,WM_CHAR,*d,1L);
			d++;
		}
	}
}


#define GBUFSIZE 512
typedef struct tagGFILE {
	HFILE	hfile;
	char 	getbuf[GBUFSIZE];
	int	getnext;
	int	getleft;
} GFILE;

GFILE * Gfopen(LPSTR lpszFileName, int fnOpenMode)
{
GFILE *gfile;
	
	gfile = (GFILE *)LocalAllocPtr(LHND, sizeof(GFILE));
	if (!gfile)
		return NULL;

	gfile->hfile = _lopen(lpszFileName, fnOpenMode);
	if (gfile->hfile == HFILE_ERROR) {
		LocalFreePtr((void NEAR *)OFFSETOF(gfile));
		return NULL;
	}
	gfile->getleft = 0;
	gfile->getnext = 0;
	return gfile;
}

void Gfclose(GFILE * gfile)
{
	
	_lclose(gfile->hfile);
	LocalFreePtr((void NEAR *)OFFSETOF(gfile));
	return;
}

/* returns number of characters read */
int
Gfgets(LPSTR lp, int size, GFILE *gfile)
{
int i;
int ch;
	for (i=0; i<size; i++) {
		if (gfile->getleft <= 0) {
			if ( (gfile->getleft = _lread(gfile->hfile, gfile->getbuf, GBUFSIZE)) == 0)
				break;
			gfile->getnext = 0;
		}
		ch = *lp++ = gfile->getbuf[gfile->getnext++];
		gfile->getleft --;
		if (ch == '\r') {
			i--;
			lp--;
		}
		if (ch == '\n') {
			i++;
			break;
		}
	}
	if (i<size)
		*lp++ = '\0';
	return i;
}

/* Get a line from the menu file */
/* Return number of lines read from file including comment lines */
int GetLine(char * buffer, int len, GFILE *gfile)
{
BOOL  status;
int nLine = 0;
    
   status = (Gfgets(buffer,len,gfile) != 0);
   nLine++;
   while( status && ( buffer[0] == 0 || buffer[0] == '\n' || buffer[0] == ';' ) ) {
      /* blank line or comment - ignore */ 
   	  status = (Gfgets(buffer,len,gfile) != 0);
      nLine++;
   }
   if (lstrlen(buffer)>0)
      buffer[lstrlen(buffer)-1] = '\0';	/* remove trailing \n */

   if (!status)
      nLine = 0;	/* zero lines if file error */
        
    return nLine;
}

/* Left justify string */
void LeftJustify(char *d, char *s)
{
	while ( *s && (*s==' ' || *s=='\t') )
		s++;	/* skip over space */
	do {
		*d++ = *s;
	} while (*s++);
}

/* Translate string to tokenized macro */
void TranslateMacro(char *string)
{
int i, len;
LPSTR ptr;

    for( i=0; keyword[i]!=(char *)NULL; i++ ) {
        if( (ptr = _fstrstr( string, keyword[i] )) != NULL ) {
            len = lstrlen( keyword[i] );
            *ptr = keyeq[i];
            lstrcpy( ptr+1, ptr+len );
            i--;       /* allows for more than one occurrence of keyword */
            }
        }
}

/* Load Macros, and create Menu from Menu file */
void 
LoadMacros(LPTW lptw)
{
GFILE *menufile;
BYTE FAR *macroptr;
char *buf;
int nMenuLevel;
HMENU hMenu[MENUDEPTH+1];
LPMW lpmw;
int nLine = 1;
int nInc;
HGLOBAL hmacro, hmacrobuf;

int i;
HDC hdc;
TEXTMETRIC tm;
RECT rect;
int ButtonX, ButtonY;
char FAR *ButtonText[BUTTONMAX];

	lpmw = lptw->lpmw;

	/* mark all buffers and menu file as unused */
	buf = (char *)NULL;
	hmacro = 0;
	hmacrobuf = 0;
	lpmw->macro = (BYTE FAR * FAR *)NULL;
	lpmw->macrobuf = (BYTE FAR *)NULL;
	lpmw->szPrompt = (char *)NULL;
	lpmw->szAnswer = (char *)NULL;
	menufile = (GFILE *)NULL;

	/* open menu file */
	if ((menufile=Gfopen(lpmw->szMenuName,OF_READ)) == (GFILE *)NULL)
		goto errorcleanup;

	/* allocate buffers */
	if ((buf = LocalAllocPtr(LHND, MAXSTR)) == (char *)NULL)
		goto nomemory;
	hmacro = GlobalAlloc(GHND,(NUMMENU) * sizeof(BYTE FAR *));
	if ((lpmw->macro = (BYTE FAR * FAR *)GlobalLock(hmacro))  == (BYTE FAR * FAR *)NULL)
		goto nomemory;
	hmacrobuf = GlobalAlloc(GHND, MACROLEN);
	if ((lpmw->macrobuf = (BYTE FAR*)GlobalLock(hmacrobuf)) == (BYTE FAR *)NULL)
		goto nomemory;
	if ((lpmw->szPrompt = LocalAllocPtr(LHND, MAXSTR)) == (char *)NULL)
		goto nomemory;
	if ((lpmw->szAnswer = LocalAllocPtr(LHND, MAXSTR)) == (char *)NULL)
		goto nomemory;

	macroptr = lpmw->macrobuf;
	lpmw->nButton = 0;
	lpmw->nCountMenu = 0;
	lpmw->hMenu = hMenu[0] = CreateMenu();
	nMenuLevel = 0;

	while ((nInc = GetLine(buf,MAXSTR,menufile)) != 0) {
	  nLine += nInc;
	  LeftJustify(buf,buf);
	  if (buf[0]=='\0') {
		/* ignore blank lines */
	  }
	  else if (!lstrcmpi(buf,"[Menu]")) {
		/* new menu */
		if (!(nInc = GetLine(buf,MAXSTR,menufile))) {
			nLine += nInc;
			wsprintf(buf,"Problem on line %d of %s\n",nLine,lpmw->szMenuName);
            		MessageBox(lptw->hWndParent,(LPSTR) buf,lptw->Title, MB_ICONEXCLAMATION);
			goto errorcleanup;
		}
		LeftJustify(buf,buf);
		if (nMenuLevel<MENUDEPTH)
			nMenuLevel++;
		else {
			wsprintf(buf,"Menu is too deep at line %d of %s\n",nLine,lpmw->szMenuName);
            		MessageBox(lptw->hWndParent,(LPSTR) buf,lptw->Title, MB_ICONEXCLAMATION);
			goto errorcleanup;
		}
		hMenu[nMenuLevel] = CreateMenu();
		AppendMenu(hMenu[nMenuLevel > 0 ? nMenuLevel-1 : 0],
			MF_STRING | MF_POPUP, (UINT)hMenu[nMenuLevel], (LPCSTR)buf);
	  }
	  else if (!lstrcmpi(buf,"[EndMenu]")) {
		if (nMenuLevel > 0)
			nMenuLevel--;	/* back up one menu */
	  }
	  else if (!lstrcmpi(buf,"[Button]")) {
		/* button macro */
		if (lpmw->nButton >= BUTTONMAX) {
			wsprintf(buf,"Too many buttons at line %d of %s\n",nLine,lpmw->szMenuName);
           			MessageBox(lptw->hWndParent,(LPSTR) buf,lptw->Title, MB_ICONEXCLAMATION);
			goto errorcleanup;
		}
		if (!(nInc = GetLine(buf,MAXSTR,menufile))) {
			nLine += nInc;
			wsprintf(buf,"Problem on line %d of %s\n",nLine,lpmw->szMenuName);
            		MessageBox(lptw->hWndParent,(LPSTR) buf,lptw->Title, MB_ICONEXCLAMATION);
			goto errorcleanup;
		}
		LeftJustify(buf,buf);
		if (lstrlen(buf)+1 < MACROLEN - (macroptr-lpmw->macrobuf))
			lstrcpy((char FAR *)macroptr,buf);
		else {
			wsprintf(buf,"Out of space for storing menu macros\n at line %d of \n",nLine,lpmw->szMenuName);
           			MessageBox(lptw->hWndParent,(LPSTR) buf,lptw->Title, MB_ICONEXCLAMATION);
			goto errorcleanup;
		}
		ButtonText[lpmw->nButton] = (char FAR *)macroptr;
		macroptr += lstrlen((char FAR *)macroptr)+1;
		*macroptr = '\0';
		if (!(nInc = GetLine(buf,MAXSTR,menufile))) {
			nLine += nInc;
			wsprintf(buf,"Problem on line %d of %s\n",nLine,lpmw->szMenuName);
           			MessageBox(lptw->hWndParent,(LPSTR) buf,lptw->Title, MB_ICONEXCLAMATION);
			goto errorcleanup;
		}
		LeftJustify(buf,buf);
		TranslateMacro(buf);
		if (lstrlen(buf)+1 < MACROLEN - (macroptr - lpmw->macrobuf))
			lstrcpy((char FAR *)macroptr,buf);
		else {
			wsprintf(buf,"Out of space for storing menu macros\n at line %d of \n",nLine,lpmw->szMenuName);
           			MessageBox(lptw->hWndParent,(LPSTR) buf,lptw->Title, MB_ICONEXCLAMATION);
			goto errorcleanup;
		}
		lpmw->hButtonID[lpmw->nButton] = lpmw->nCountMenu;
		lpmw->macro[lpmw->nCountMenu] = macroptr;
		macroptr += lstrlen((char FAR *)macroptr)+1;
		*macroptr = '\0';
		lpmw->nCountMenu++;
		lpmw->nButton++;
	  }
	  else {
		/* menu item */
		if (lpmw->nCountMenu>=NUMMENU) {
			wsprintf(buf,"Too many menu items at line %d of %s\n",nLine,lpmw->szMenuName);
           			MessageBox(lptw->hWndParent,(LPSTR) buf,lptw->Title, MB_ICONEXCLAMATION);
			goto errorcleanup;
		}
		LeftJustify(buf,buf);
/* HBB 981202: added MF_SEPARATOR to the MF_MENU*BREAK items. This  is meant
 * to maybe avoid a CodeGuard warning about passing last argument zero
 * when item style is not SEPARATOR... Actually, a better solution would
 * have been to combine the '|' divider with the next menu item. */
		if (buf[0]=='-') {
		    if (nMenuLevel == 0)
				AppendMenu(hMenu[0], MF_SEPARATOR | MF_MENUBREAK, 0, (LPSTR)NULL);
		    else
			AppendMenu(hMenu[nMenuLevel], MF_SEPARATOR, 0, (LPSTR)NULL);
		}
		else if (buf[0]=='|') {
			AppendMenu(hMenu[nMenuLevel], MF_SEPARATOR | MF_MENUBARBREAK, 0, (LPSTR)NULL);
		}
		else {
			AppendMenu(hMenu[nMenuLevel],MF_STRING, lpmw->nCountMenu, (LPSTR)buf);
			if (!(nInc = GetLine(buf,MAXSTR,menufile))) {
				nLine += nInc;
				wsprintf(buf,"Problem on line %d of %s\n",nLine,lpmw->szMenuName);
            			MessageBox(lptw->hWndParent,(LPSTR) buf,lptw->Title, MB_ICONEXCLAMATION);
				goto errorcleanup;
			}
			LeftJustify(buf,buf);
			TranslateMacro(buf);
			if (lstrlen(buf)+1 < MACROLEN - (macroptr - lpmw->macrobuf))
				lstrcpy((char FAR *)macroptr,buf);
			else {
				wsprintf(buf,"Out of space for storing menu macros\n at line %d of %s\n",nLine,lpmw->szMenuName);
            			MessageBox(lptw->hWndParent,(LPSTR) buf,lptw->Title, MB_ICONEXCLAMATION);
				goto errorcleanup;
			}
			lpmw->macro[lpmw->nCountMenu] = macroptr;
			macroptr += lstrlen((char FAR *)macroptr)+1;
			*macroptr = '\0';
			lpmw->nCountMenu++;
		}
	  }
	}

	if ( (lpmw->nCountMenu - lpmw->nButton) > 0 ) {
		/* we have a menu bar so put it on the window */
		SetMenu(lptw->hWndParent,lpmw->hMenu);
		DrawMenuBar(lptw->hWndParent);
	}

	if (!lpmw->nButton)
		goto cleanup;		/* no buttons */

	/* calculate size of buttons */
	hdc = GetDC(lptw->hWndParent);
	SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
	GetTextMetrics(hdc, &tm);
	ButtonX = 8 * tm.tmAveCharWidth;
	ButtonY = 6 * (tm.tmHeight + tm.tmExternalLeading) / 4;
	ReleaseDC(lptw->hWndParent,hdc);

	/* move top of client text window down to allow space for buttons */
	lptw->ButtonHeight = ButtonY+1;
	GetClientRect(lptw->hWndParent, &rect);
	SetWindowPos(lptw->hWndText, (HWND)NULL, 0, lptw->ButtonHeight,
			rect.right, rect.bottom-lptw->ButtonHeight, 
			SWP_NOZORDER | SWP_NOACTIVATE);

	/* create the buttons */
#ifdef __DLL__
	lpmw->lpfnMenuButtonProc = (WNDPROC)GetProcAddress(hdllInstance, "MenuButtonProc");
#else
	lpmw->lpfnMenuButtonProc = (WNDPROC)MakeProcInstance((FARPROC)MenuButtonProc, hdllInstance);
#endif
	for (i=0; i<lpmw->nButton; i++) {
		lpmw->hButton[i] = CreateWindow("button", ButtonText[i],
			WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
				i * ButtonX, 0,
				ButtonX, ButtonY,
				lptw->hWndParent, (HMENU)i,
				lptw->hInstance, lptw);
		lpmw->lpfnButtonProc[i] = (WNDPROC) GetWindowLong(lpmw->hButton[i], GWL_WNDPROC);
		SetWindowLong(lpmw->hButton[i], GWL_WNDPROC, (LONG)lpmw->lpfnMenuButtonProc);
	}

	goto cleanup;


nomemory:
	MessageBox(lptw->hWndParent,"Out of memory",lptw->Title, MB_ICONEXCLAMATION);
errorcleanup:
	if (hmacro) {
		GlobalUnlock(hmacro);
		GlobalFree(hmacro);
	}
	if (hmacrobuf) {
		GlobalUnlock(hmacrobuf);
		GlobalFree(hmacrobuf);
	}
	if (lpmw->szPrompt != (char *)NULL)
		LocalFreePtr((void NEAR *)OFFSETOF(lpmw->szPrompt));
	if (lpmw->szAnswer != (char *)NULL)
		LocalFreePtr((void NEAR *)OFFSETOF(lpmw->szAnswer));

cleanup:
	if (buf != (char *)NULL)
		LocalFreePtr((void NEAR *)OFFSETOF(buf));
	if (menufile != (GFILE *)NULL)
		Gfclose(menufile);
	return;

}

void
CloseMacros(LPTW lptw)
{
HGLOBAL hglobal;
LPMW lpmw;
	lpmw = lptw->lpmw;

#ifndef WIN32
#ifndef __DLL__
	if (lpmw->lpfnMenuButtonProc)
		FreeProcInstance((FARPROC)lpmw->lpfnMenuButtonProc);
#endif
#endif
	hglobal = (HGLOBAL)GlobalHandle( SELECTOROF(lpmw->macro) );
	if (hglobal) {
		GlobalUnlock(hglobal);
		GlobalFree(hglobal);
	}
	hglobal = (HGLOBAL)GlobalHandle( SELECTOROF(lpmw->macrobuf) );
	if (hglobal) {
		GlobalUnlock(hglobal);
		GlobalFree(hglobal);
	}
	if (lpmw->szPrompt != (char *)NULL)
		LocalFreePtr((void NEAR *)OFFSETOF(lpmw->szPrompt));
	if (lpmw->szAnswer != (char *)NULL)
		LocalFreePtr((void NEAR *)OFFSETOF(lpmw->szAnswer));
}


/***********************************************************************/
/* InputBoxDlgProc() -  Message handling routine for Input dialog box         */
/***********************************************************************/

BOOL CALLBACK WINEXPORT
InputBoxDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
LPTW lptw;
LPMW lpmw;
    lptw = (LPTW)GetWindowLong(GetParent(hDlg), 0);
    lpmw = lptw->lpmw;

    switch( message) {
        case WM_INITDIALOG:
            SetDlgItemText( hDlg, ID_PROMPT, lpmw->szPrompt);
            return( TRUE);

        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case ID_ANSWER:
                    return( TRUE);

                case IDOK:
                    lpmw->nChar = GetDlgItemText( hDlg, ID_ANSWER, lpmw->szAnswer, MAXSTR);
                    EndDialog( hDlg, TRUE);
                    return( TRUE);

                case IDCANCEL:
                    lpmw->szAnswer[0] = 0;
                    EndDialog( hDlg, FALSE);
                    return( TRUE);

                default:
                    return( FALSE);
                }
        default:
            return( FALSE);
        }
    }


LRESULT CALLBACK WINEXPORT
MenuButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
LPTW lptw;
LPMW lpmw;
#ifdef WIN32
LONG n = GetWindowLong(hwnd, GWL_ID);
#else
WORD n = GetWindowWord(hwnd, GWW_ID);
#endif
	lptw = (LPTW)GetWindowLong(GetParent(hwnd), 0);
	lpmw = lptw->lpmw;

	switch(message) {
		case WM_LBUTTONUP:
			{
			RECT rect;
			POINT pt;
			GetWindowRect(hwnd, &rect);
			GetCursorPos(&pt);
			if (PtInRect(&rect, pt))
				SendMessage(lptw->hWndText, WM_COMMAND, lpmw->hButtonID[n], 0L);
			SetFocus(lptw->hWndText);
			}
			break;
	}
	return CallWindowProc((lpmw->lpfnButtonProc[n]), hwnd, message, wParam, lParam);
}
