/*
 * $Id: wgdiplus.cpp,v 1.4 2011/05/14 15:40:37 markisch Exp $
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

#include <windows.h>
#include <gdiplus.h>
#include "wgdiplus.h"
using namespace Gdiplus;

static bool gdiplusInitialized = false;
static ULONG_PTR gdiplusToken;


void gdiplusInit(void)
{
	if (!gdiplusInitialized) {
		gdiplusInitialized = true;
		GdiplusStartupInput gdiplusStartupInput;
		GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	}
};


void gdiplusCleanup(void)
{
	if (gdiplusInitialized) {
		gdiplusInitialized = false;
		GdiplusShutdown(gdiplusToken);
	}
};


static Color gdiplusCreateColor(COLORREF color, double alpha)
{
	ARGB argb = Color::MakeARGB(
		BYTE(255 * alpha),
		GetRValue(color), GetGValue(color), GetBValue(color));
	return Color(argb);
}


static Pen * gdiplusCreatePen(UINT style, float width, COLORREF color)
{
	// create GDI+ pen
	Color gdipColor(0, 0, 0, 255);
	gdipColor.SetFromCOLORREF(color);
	Pen * pen = new Pen(gdipColor, width > 1 ? width : 1);
	if (style <= PS_DASHDOTDOT)
		// cast is save since GDI and GDI+ use same numbers
		pen->SetDashStyle(static_cast<DashStyle>(style));
	pen->SetLineCap(LineCapFlat, LineCapFlat, DashCapFlat);

	return pen;
}


void gdiplusLine(HDC hdc, POINT x, POINT y, const PLOGPEN logpen)
{
	gdiplusLineEx(hdc, x, y, logpen->lopnStyle, (float)logpen->lopnWidth.x, logpen->lopnColor);
}


void gdiplusLineEx(HDC hdc, POINT x, POINT y, UINT style, float width, COLORREF color)
{
	gdiplusInit();
	Graphics graphics(hdc);

	// Dash patterns get scaled with line width, in contrast to GDI.
	// Avoid smearing out caused by antialiasing for small line widths.
	if ((style == PS_SOLID) || (width >= 2.))
		graphics.SetSmoothingMode(SmoothingModeAntiAlias);

	Pen * pen = gdiplusCreatePen(style, width, color);
	graphics.DrawLine(pen, (INT)x.x, (INT)x.y, (INT)y.x, (INT)y.y);
	delete(pen);
}


void gdiplusPolyline(HDC hdc, POINT *ppt, int polyi, const PLOGPEN logpen)
{
	gdiplusPolylineEx(hdc, ppt, polyi, logpen->lopnStyle, (float)logpen->lopnWidth.x, logpen->lopnColor);
}


void gdiplusPolylineEx(HDC hdc, POINT *ppt, int polyi, UINT style, float width, COLORREF color)
{
	gdiplusInit();
	Graphics graphics(hdc);

	// Dash patterns get scaled with line width, in contrast to GDI.
	// Avoid smearing out caused by antialiasing for small line widths.
	if ((style == PS_SOLID) || (width >= 2.))
		graphics.SetSmoothingMode(SmoothingModeAntiAlias);

	Pen * pen = gdiplusCreatePen(style, width, color);
	Point * points = new Point[polyi];
	for (int i = 0; i < polyi; i++) {
		points[i].X = ppt[i].x;
		points[i].Y = ppt[i].y;
	}
	graphics.DrawLines(pen, points, polyi);
	delete(pen);
	delete(points);
}


void gdiplusSolidFilledPolygonEx(HDC hdc, POINT *ppt, int polyi, COLORREF color, double alpha)
{
	gdiplusInit();
	Graphics graphics(hdc);
	graphics.SetSmoothingMode(SmoothingModeAntiAlias);

	Color gdipColor = gdiplusCreateColor(color, alpha);
	Point * points = new Point[polyi];
	for (int i = 0; i < polyi; i++) {
		points[i].X = ppt[i].x;
		points[i].Y = ppt[i].y;
	}
	SolidBrush brush(gdipColor);
	graphics.FillPolygon(&brush, points, polyi);
	delete points;
}


void gdiplusPatternFilledPolygonEx(HDC hdc, POINT *ppt, int polyi, COLORREF color, double alpha, COLORREF backcolor, BOOL transparent, int style)
{
	gdiplusInit();
	Graphics graphics(hdc);
	graphics.SetSmoothingMode(SmoothingModeAntiAlias);

	Color gdipColor = gdiplusCreateColor(color, alpha);
	Color gdipBackColor = gdiplusCreateColor(backcolor, transparent ? 0 : 1.);
	Brush * brush;
	style %= 8;
	const HatchStyle styles[] = { HatchStyleTotal, HatchStyleDiagonalCross,
		HatchStyleZigZag, HatchStyleTotal,
		HatchStyleForwardDiagonal, HatchStyleBackwardDiagonal,
		HatchStyleLightDownwardDiagonal, HatchStyleDarkUpwardDiagonal };
	switch (style) {
		case 0:
			brush = new SolidBrush(gdipBackColor);
			break;
		case 3:
			brush = new SolidBrush(gdipColor);
			break;
		default:
			brush = new HatchBrush(styles[style], gdipColor, gdipBackColor);
	}
	Point * points = new Point[polyi];
	for (int i = 0; i < polyi; i++) {
		points[i].X = ppt[i].x;
		points[i].Y = ppt[i].y;
	}
	graphics.FillPolygon(brush, points, polyi);
	delete(points);
	delete(brush);
}


void gdiplusCircleEx(HDC hdc, POINT * p, int radius, UINT style, float width, COLORREF color)
{
	gdiplusInit();
	Graphics graphics(hdc);
	graphics.SetSmoothingMode(SmoothingModeAntiAlias);

	Pen * pen = gdiplusCreatePen(style, width, color);
	graphics.DrawEllipse(pen, p->x - radius, p->y - radius, 2*radius, 2*radius);
	delete(pen);
}

