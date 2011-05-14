/*
 * $Id: wgdiplus.h,v 1.2 2011/05/05 19:07:19 markisch Exp $
 */

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

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

extern void gdiplusInit(void);
extern void gdiplusCleanup(void);

extern void gdiplusLine(HDC hdc, POINT x, POINT y, const PLOGPEN logpen);
extern void gdiplusLineEx(HDC hdc, POINT x, POINT y, UINT style, float width, COLORREF color);

extern void gdiplusPolyline(HDC hdc, POINT *ppt, int polyi, const PLOGPEN logpen);
extern void gdiplusPolylineEx(HDC hdc, POINT *ppt, int polyi, UINT style, float width, COLORREF color);

extern void gdiplusSolidFilledPolygonEx(HDC hdc, POINT *ppt, int polyi, COLORREF color, double alpha);

extern void gdiplusCircleEx(HDC hdc, POINT *p, int radius, UINT style, float width, COLORREF color);

#ifdef __cplusplus
}
#endif
