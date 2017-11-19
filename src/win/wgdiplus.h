/*
Copyright (c) 2011 Bastian Maerkisch. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY Bastian Maerkisch ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <windows.h>
#include "wgnuplib.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void gdiplusInit(void);
extern void gdiplusCleanup(void);

extern void InitFont_gdiplus(LPGW lpgw, HDC hdc, LPRECT rect);

extern void drawgraph_gdiplus(LPGW lpgw, HDC hdc, LPRECT rect);
extern void metafile_gdiplus(LPGW lpgw, HDC hdc, LPRECT rect, LPWSTR name);
extern HENHMETAFILE clipboard_gdiplus(LPGW lpgw, HDC hdc, LPRECT rect);
extern void print_gdiplus(LPGW lpgw, HDC hdc, HANDLE printer, LPRECT rect);

extern void SaveAsBitmap(LPGW lpgw);
extern HBITMAP gdiplusLoadBitmap(LPWSTR file, int size);

#ifdef __cplusplus
}
#endif
