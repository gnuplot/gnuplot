/*
Copyright (c) 2011-2017 Bastian Maerkisch. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// include iostream / cstdio _before_ syscfg.h in order
// to avoid re-definition by wtext.h/winmain.c routines
#include <iostream>
#include <vector>
extern "C" {
# include "syscfg.h"
}
#include <windows.h>
#include <windowsx.h>
#define GDIPVER 0x0110
#include <gdiplus.h>
#include <tchar.h>
#include <wchar.h>
#ifdef __WATCOMC__
// swprintf_s is missing from <cwchar>
# define swprintf_s(s, c, f, ...) swprintf(s, c, f, __VA_ARGS__)
#endif

#include "wgdiplus.h"
#include "wgnuplib.h"
#include "winmain.h"
#include "wcommon.h"
using namespace Gdiplus;
// do not use namespace std: otherwise MSVC complains about
// ambiguous symbol bool
//using namespace std;

static bool gdiplusInitialized = false;
static ULONG_PTR gdiplusToken;

#define MINMAX(a,val,b) (((val) <= (a)) ? (a) : ((val) <= (b) ? (val) : (b)))
const int pattern_num = 8;

enum draw_target { DRAW_SCREEN, DRAW_PRINTER, DRAW_PLOTTER, DRAW_METAFILE };

static Color gdiplusCreateColor(COLORREF color, double alpha);
static void gdiplusSetDashStyle(Pen *pen, enum DashStyle style);
static void gdiplusPolyline(Graphics &graphics, Pen &pen, Point *points, int polyi);
Brush * gdiplusPatternBrush(int style, COLORREF color, double alpha, COLORREF backcolor, BOOL transparent);
static void gdiplusDot(Graphics &graphics, Brush &brush, int x, int y);
static Font * SetFont_gdiplus(Graphics &graphics, LPRECT rect, LPGW lpgw, LPTSTR fontname, int size);
static void do_draw_gdiplus(LPGW lpgw, Graphics &graphics, LPRECT rect, enum draw_target target);


/* Internal state of enhanced text processing.
*/

static struct {
	Graphics * graphics; /* graphics object */
	Font * font;
	SolidBrush * brush;
	StringFormat *stringformat;
} enhstate_gdiplus;


/* ****************  ....   ************************* */


void
gdiplusInit(void)
{
	if (!gdiplusInitialized) {
		gdiplusInitialized = true;
		GdiplusStartupInput gdiplusStartupInput;
		GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	}
}


void
gdiplusCleanup(void)
{
	if (gdiplusInitialized) {
		gdiplusInitialized = false;
		GdiplusShutdown(gdiplusToken);
	}
}


static Color
gdiplusCreateColor(COLORREF color, double alpha)
{
	ARGB argb = Color::MakeARGB(
		BYTE(255 * alpha),
		GetRValue(color), GetGValue(color), GetBValue(color));
	return Color(argb);
}


static void
gdiplusSetDashStyle(Pen *pen, enum DashStyle style)
{
	const REAL dashstyles[4][6] = {
		{ 16.f, 8.f },	// dash
		{ 3.f, 3.f },	// dot
		{ 8.f, 5.f, 3.f, 5.f }, // dash dot
		{ 8.f, 4.f, 3.f, 4.f, 3.f, 4.f } // dash dot dot
	};
	const int dashstyle_len[4] = { 2, 2, 4, 6 };

	style = static_cast<enum DashStyle>(style % 5);
	if (style == 0)
		pen->SetDashStyle(style);
	else
		pen->SetDashPattern(dashstyles[style - 1], dashstyle_len[style - 1]);
}


/* ****************  GDI+ only functions ********************************** */


static void
gdiplusPolyline(Graphics &graphics, Pen &pen, Point *points, int polyi)
{
	// Dash patterns get scaled with line width, in contrast to GDI.
	// Avoid smearing out caused by antialiasing for small line widths.
	SmoothingMode mode = graphics.GetSmoothingMode();

	if ((points[0].X != points[polyi - 1].X) ||
		(points[0].Y != points[polyi - 1].Y))
		graphics.DrawLines(&pen, points, polyi);
	else
		graphics.DrawPolygon(&pen, points, polyi - 1);

	/* restore */
	if (mode != SmoothingModeNone)
		graphics.SetSmoothingMode(mode);
}


static void
gdiplusPolyline(Graphics &graphics, Pen &pen, PointF *points, int polyi)
{
	// Dash patterns get scaled with line width, in contrast to GDI.
	// Avoid smearing out caused by antialiasing for small line widths.
	SmoothingMode mode = graphics.GetSmoothingMode();

	bool all_vert_or_horz = true;
	for (int i = 1; i < polyi; i++)
		if (!((points[i - 1].X == points[i].X) ||
		      (points[i - 1].Y == points[i].Y)))
			all_vert_or_horz = false;

	// if all lines are horizontal or vertical we snap to nearest pixel
	// to avoid "blurry" lines
	if (all_vert_or_horz) {
		for (int i = 0; i < polyi; i++) {
			points[i].X = INT(points[i].X + 0.5);
			points[i].Y = INT(points[i].Y + 0.5);
		}
	}

	if ((points[0].X != points[polyi - 1].X) ||
	    (points[0].Y != points[polyi - 1].Y))
		graphics.DrawLines(&pen, points, polyi);
	else
		graphics.DrawPolygon(&pen, points, polyi - 1);

	/* restore */
	if (mode != SmoothingModeNone)
		graphics.SetSmoothingMode(mode);
}


Brush *
gdiplusPatternBrush(int style, COLORREF color, double alpha, COLORREF backcolor, BOOL transparent)
{
	Color gdipColor = gdiplusCreateColor(color, alpha);
	Color gdipBackColor = gdiplusCreateColor(backcolor, transparent ? 0 : 1.);
	Brush * brush;
	style %= pattern_num;
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
	return brush;
}


static void
gdiplusDot(Graphics &graphics, Brush &brush, int x, int y)
{
	/* no antialiasing in order to avoid blurred pixel */
	SmoothingMode mode = graphics.GetSmoothingMode();
	graphics.SetSmoothingMode(SmoothingModeNone);
	graphics.FillRectangle(&brush, x, y, 1, 1);
	graphics.SetSmoothingMode(mode);
}


static Font *
SetFont_gdiplus(Graphics &graphics, LPRECT rect, LPGW lpgw, LPTSTR fontname, int size)
{
	if ((fontname == NULL) || (*fontname == 0))
		fontname = lpgw->deffontname;
	if (size == 0)
		size = lpgw->deffontsize;

	/* make a local copy */
	fontname = _tcsdup(fontname);

	/* save current font */
	_tcscpy(lpgw->fontname, fontname);
	lpgw->fontsize = size;

	// apply fontscale
	size *= lpgw->fontscale;

	/* set up font style */
	INT fontStyle = FontStyleRegular;
	LPTSTR italic, bold, underline, strikeout;
	if ((italic = _tcsstr(fontname, TEXT(" Italic"))) != NULL)
		fontStyle |= FontStyleItalic;
	else if ((italic = _tcsstr(fontname, TEXT(":Italic"))) != NULL)
		fontStyle |= FontStyleItalic;
	if ((bold = _tcsstr(fontname, TEXT(" Bold"))) != NULL)
		fontStyle |= FontStyleBold;
	else if ((bold = _tcsstr(fontname, TEXT(":Bold"))) != NULL)
		fontStyle |= FontStyleBold;
	if ((underline = _tcsstr(fontname, TEXT(" Underline"))) != NULL)
		fontStyle |= FontStyleUnderline;
	if ((strikeout = _tcsstr(fontname, TEXT(" Strikeout"))) != NULL)
		fontStyle |= FontStyleStrikeout;
	if (italic) *italic = 0;
	if (bold) *bold = 0;
	if (underline) *underline = 0;
	if (strikeout) *strikeout = 0;

#ifdef UNICODE
	const FontFamily * fontFamily = new FontFamily(fontname);
#else
	LPWSTR family = UnicodeText(fontname, lpgw->encoding);
	const FontFamily * fontFamily = new FontFamily(family);
	free(family);
#endif
	free(fontname);
	Font * font;
	int fontHeight;
	bool deleteFontFamily = true;
	if (fontFamily->GetLastStatus() != Ok) {
		delete fontFamily;
#if (!defined(__MINGW32__) || defined(__MINGW64_VERSION_MAJOR))
		// MinGW 4.8.1 does not have this
		fontFamily = FontFamily::GenericSansSerif();
		deleteFontFamily = false;
#else
#ifdef UNICODE
		fontFamily = new FontFamily(GraphDefaultFont());
#else
		family = UnicodeText(GraphDefaultFont(), S_ENC_DEFAULT); // should always be available
		fontFamily = new FontFamily(family);
		free(family);
#endif
#endif
	}
	font = new Font(fontFamily, size, fontStyle, UnitPoint);
	double scale = font->GetSize() / fontFamily->GetEmHeight(fontStyle) * graphics.GetDpiY() / 72.;
	/* store text metrics for later use */
	lpgw->tmHeight = fontHeight = scale * (fontFamily->GetCellAscent(fontStyle) + fontFamily->GetCellDescent(fontStyle));
	lpgw->tmAscent = scale * fontFamily->GetCellAscent(fontStyle);
	lpgw->tmDescent = scale * fontFamily->GetCellDescent(fontStyle);
	if (deleteFontFamily)
		delete fontFamily;

	RectF box;
	graphics.MeasureString(L"0123456789", -1, font, PointF(0, 0), StringFormat::GenericTypographic(), &box);
	lpgw->vchar = MulDiv(fontHeight, lpgw->ymax, rect->bottom - rect->top);
	lpgw->hchar = MulDiv(box.Width, lpgw->xmax, 10 * (rect->right - rect->left));
	lpgw->htic = MulDiv(lpgw->hchar, 2, 5);
	unsigned cy = MulDiv(box.Width, 2 * graphics.GetDpiY(), 50 * graphics.GetDpiX());
	lpgw->vtic = MulDiv(cy, lpgw->ymax, rect->bottom - rect->top);

	// Can always rotate text.
	lpgw->rotate = TRUE;

	return font;
}


void
InitFont_gdiplus(LPGW lpgw, HDC hdc, LPRECT rect)
{
	gdiplusInit();
	Graphics graphics(hdc);
	// call for the side effects:  set vchar/hchar and text metrics
	Font * font = SetFont_gdiplus(graphics, rect, lpgw, lpgw->fontname, lpgw->fontsize);
	// TODO:  save font object for later use
	delete font;
}


static void
EnhancedSetFont()
{
	if ((enhstate.lpgw->fontsize != enhstate.fontsize) ||
	    (_tcscmp(enhstate.lpgw->fontname, enhstate.fontname) != 0)) {
		if (enhstate_gdiplus.font)
			delete enhstate_gdiplus.font;
		enhstate_gdiplus.font = SetFont_gdiplus(*enhstate_gdiplus.graphics, enhstate.rect, enhstate.lpgw, enhstate.fontname, enhstate.fontsize);
	}
}


static unsigned
EnhancedTextLength(char * text)
{
	LPWSTR textw = UnicodeTextWithEscapes(enhanced_text, enhstate.lpgw->encoding);
	RectF box;
	enhstate_gdiplus.graphics->MeasureString(textw, -1, enhstate_gdiplus.font, PointF(0, 0), enhstate_gdiplus.stringformat, &box);
	free(textw);
	return ceil(box.Width);
}


static void
EnhancedPutText(int x, int y, char * text)
{
	LPWSTR textw = UnicodeTextWithEscapes(text, enhstate.lpgw->encoding);
	Graphics *g = enhstate_gdiplus.graphics;
	if (enhstate.lpgw->angle == 0) {
		PointF pointF(x, y + enhstate.lpgw->tmDescent);
		g->DrawString(textw, -1, enhstate_gdiplus.font, pointF, enhstate_gdiplus.stringformat, enhstate_gdiplus.brush);
	} else {
		/* shift rotated text correctly */
		g->TranslateTransform(x, y);
		g->RotateTransform(-enhstate.lpgw->angle);
		g->DrawString(textw, -1, enhstate_gdiplus.font, PointF(0, enhstate.lpgw->tmDescent), enhstate_gdiplus.stringformat, enhstate_gdiplus.brush);
		g->ResetTransform();
	}
	free(textw);
}


static void
EnhancedCleanup()
{
	delete enhstate_gdiplus.font;
	delete enhstate_gdiplus.stringformat;
}


static void
draw_enhanced_init(LPGW lpgw, Graphics &graphics, SolidBrush &brush, LPRECT rect)
{
	enhstate.set_font = &EnhancedSetFont;
	enhstate.text_length = &EnhancedTextLength;
	enhstate.put_text = &EnhancedPutText;
	enhstate.cleanup = &EnhancedCleanup;

	enhstate_gdiplus.graphics = &graphics;
	enhstate_gdiplus.font = SetFont_gdiplus(graphics, rect, lpgw, lpgw->fontname, lpgw->fontsize);
	enhstate_gdiplus.brush = &brush;
	enhstate.res_scale = graphics.GetDpiY() / 96.;

	enhstate_gdiplus.stringformat = new StringFormat(StringFormat::GenericTypographic());
	enhstate_gdiplus.stringformat->SetAlignment(StringAlignmentNear);
	enhstate_gdiplus.stringformat->SetLineAlignment(StringAlignmentFar);
	INT flags = enhstate_gdiplus.stringformat->GetFormatFlags();
	flags |= StringFormatFlagsMeasureTrailingSpaces;
	enhstate_gdiplus.stringformat->SetFormatFlags(flags);
}


void
drawgraph_gdiplus(LPGW lpgw, HDC hdc, LPRECT rect)
{
	gdiplusInit();
	Graphics graphics(hdc);
	do_draw_gdiplus(lpgw, graphics, rect, DRAW_SCREEN);
}


void
metafile_gdiplus(LPGW lpgw, HDC hdc, LPRECT lprect, LPWSTR name)
{
	gdiplusInit();
	Rect rect(lprect->left, lprect->top, lprect->right - lprect->left, lprect->bottom - lprect->top);
	Metafile metafile(name, hdc, rect, MetafileFrameUnitPixel, EmfTypeEmfPlusDual, NULL);
	Graphics graphics(&metafile);
	do_draw_gdiplus(lpgw, graphics, lprect, DRAW_METAFILE);
}


HENHMETAFILE
clipboard_gdiplus(LPGW lpgw, HDC hdc, LPRECT lprect)
{
	gdiplusInit();
	Rect rect(lprect->left, lprect->top, lprect->right - lprect->left, lprect->bottom - lprect->top);
	Metafile metafile(hdc, rect, MetafileFrameUnitPixel, EmfTypeEmfPlusDual, NULL);
	// Note: We can only get a valid handle once the graphics object has released
	// the metafile. Creating the graphics object on the heap seems the only way to
	// achieve that.
	Graphics * graphics = Graphics::FromImage(&metafile);
	do_draw_gdiplus(lpgw, *graphics, lprect, DRAW_METAFILE);
	delete graphics;
	return metafile.GetHENHMETAFILE();
}


void
print_gdiplus(LPGW lpgw, HDC hdc, HANDLE printer, LPRECT rect)
{
	gdiplusInit();

	// temporarily turn of antialiasing
	BOOL aa = lpgw->antialiasing;
	lpgw->antialiasing = FALSE;

	Graphics graphics(hdc, printer);
	graphics.SetPageUnit(UnitPixel);
	do_draw_gdiplus(lpgw, graphics, rect, DRAW_PRINTER);

	// restore settings
	lpgw->antialiasing = aa;
}


static void
do_draw_gdiplus(LPGW lpgw, Graphics &graphics, LPRECT rect, enum draw_target target)
{
	/* draw ops */
	unsigned int ngwop = 0;
	struct GWOP *curptr;
	struct GWOPBLK *blkptr;

	/* layers and hypertext */
	unsigned plotno = 0;
	bool gridline = false;
	bool skipplot = false;
	bool keysample = false;
	bool interactive;
	LPWSTR hypertext = NULL;
	int hypertype = 0;

	/* colors */
	bool isColor = true;		/* use colors? */
	COLORREF last_color = 0;	/* currently selected color */
	double alpha_c = 1.;		/* alpha for transparency */

	/* text */
	Font * font;

	/* lines */
	double line_width = lpgw->linewidth;	/* current line width */
	double lw_scale = 1.;
	LOGPEN cur_penstruct;		/* current pen settings */

	/* polylines and polygons */
	int polymax = 200;			/* size of ppt */
	int polyi = 0;				/* number of points in ppt */
	std::vector<PointF> ppt(polymax);	/* storage of polyline/polygon-points */
	int last_polyi = 0;			/* number of points in last_poly */
	std::vector<PointF> last_poly(0);	/* storage of last filled polygon */
	unsigned int lastop = -1;	/* used for plotting last point on a line */

	/* filled polygons and boxes */
	int last_fillstyle = -1;
	COLORREF last_fillcolor = 0;
	double last_fill_alpha = 1.;
	bool transparent = false;	/* transparent fill? */
	Brush * pattern_brush = NULL;
	Brush * fill_brush = NULL;
	Bitmap * poly_bitmap = NULL;
	Graphics * poly_graphics = NULL;
	float poly_scale = 2.f;

	/* images */
	POINT corners[4];			/* image corners */
	int color_mode = 0;			/* image color mode */

#ifdef EAM_BOXED_TEXT
	struct s_boxedtext {
		TBOOLEAN boxing;
		t_textbox_options option;
		POINT margin;
		POINT start;
		RECT  box;
		int   angle;
	} boxedtext;
#endif

	/* point symbols */
	enum win_pointtypes last_symbol = W_invalid_pointtype;
	CachedBitmap *cb = NULL;
	POINT cb_ofs;
	bool ps_caching = false;

	/* coordinates and lengths */
	float xdash, ydash;			/* the transformed coordinates */
	int rr, rl, rt, rb;			/* coordinates of drawing area */
	int htic, vtic;				/* tic sizes */
	float hshift, vshift;			/* correction of text position */

	/* indices */
	int seq = 0;				/* sequence counter for W_image and W_boxedtext */

	if (lpgw->locked) return;

	/* clear hypertexts only in display sessions */
	interactive = (target == DRAW_SCREEN);
	if (interactive)
		clear_tooltips(lpgw);

	/* Need to scale line widths for raster printers so they are the same
	   as on screen */
	if (target == DRAW_PRINTER) {
		HDC hdc = graphics.GetHDC();
		HDC hdc_screen = GetDC(NULL);
		lw_scale = (double) GetDeviceCaps(hdc, LOGPIXELSX) /
		           (double) GetDeviceCaps(hdc_screen, LOGPIXELSY);
		line_width *= lw_scale;
		ReleaseDC(NULL, hdc_screen);
		graphics.ReleaseHDC(hdc);
	}

	// only cache point symbols when drawing to a screen
	ps_caching = (target == DRAW_SCREEN);

	rr = rect->right;
	rl = rect->left;
	rt = rect->top;
	rb = rect->bottom;

	if (lpgw->antialiasing) {
		graphics.SetSmoothingMode(SmoothingModeAntiAlias8x8);
		graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
	}
	graphics.SetInterpolationMode(InterpolationModeNearestNeighbor);

	/* background fill */
	if (isColor)
		graphics.Clear(gdiplusCreateColor(lpgw->background, 1.));
	else
		graphics.Clear(Color(255, 255, 255));

	/* Init brush and pens: need to be created after the Graphics object. */
	SolidBrush solid_brush(gdiplusCreateColor(lpgw->background, 1.));
	SolidBrush solid_fill_brush(gdiplusCreateColor(lpgw->background, 1.));
	cur_penstruct = (lpgw->color && isColor) ?  lpgw->colorpen[2] : lpgw->monopen[2];
	last_color = cur_penstruct.lopnColor;

	Pen pen(Color(0, 0, 0));
	Pen solid_pen(Color(0, 0, 0));
	pen.SetColor(gdiplusCreateColor(last_color, 1.));
	pen.SetLineCap(lpgw->rounded ? LineCapRound : LineCapSquare,
					lpgw->rounded ? LineCapRound : LineCapSquare,
					DashCapFlat);
	pen.SetLineJoin(lpgw->rounded ? LineJoinRound : LineJoinMiter);
	solid_pen.SetColor(gdiplusCreateColor(last_color, 1.));
	solid_pen.SetLineCap(lpgw->rounded ? LineCapRound : LineCapSquare,
					lpgw->rounded ? LineCapRound : LineCapSquare,
					DashCapFlat);
	solid_pen.SetLineJoin(lpgw->rounded ? LineJoinRound : LineJoinMiter);

	htic = (lpgw->org_pointsize * MulDiv(lpgw->htic, rr - rl, lpgw->xmax) + 1);
	vtic = (lpgw->org_pointsize * MulDiv(lpgw->vtic, rb - rt, lpgw->ymax) + 1);

	lpgw->angle = 0;
	lpgw->justify = LEFT;
	StringFormat stringFormat(StringFormat::GenericTypographic());
	stringFormat.SetAlignment(StringAlignmentNear);
	stringFormat.SetLineAlignment(StringAlignmentNear);
	INT flags = stringFormat.GetFormatFlags();
	flags |= StringFormatFlagsMeasureTrailingSpaces;
	stringFormat.SetFormatFlags(flags);
	font = SetFont_gdiplus(graphics, rect, lpgw, NULL, 0);

	/* calculate text shifting for horizontal text */
	hshift = 0.0;
	vshift = - lpgw->tmHeight / 2;

	/* init layer variables */
	lpgw->numplots = 0;
	lpgw->hasgrid = FALSE;
	for (unsigned i = 0; i < lpgw->maxkeyboxes; i++) {
		lpgw->keyboxes[i].left = INT_MAX;
		lpgw->keyboxes[i].right = 0;
		lpgw->keyboxes[i].bottom = INT_MAX;
		lpgw->keyboxes[i].top = 0;
	}

#ifdef EAM_BOXED_TEXT
	boxedtext.boxing = FALSE;
#endif

	/* do the drawing */
	blkptr = lpgw->gwopblk_head;
	curptr = NULL;
	if (blkptr != NULL) {
		if (!blkptr->gwop)
			blkptr->gwop = (struct GWOP *) GlobalLock(blkptr->hblk);
		if (!blkptr->gwop)
			return;
		curptr = (struct GWOP *)blkptr->gwop;
	}
	if (curptr == NULL)
		return;

	while (ngwop < lpgw->nGWOP) {
		// transform the coordinates
		if (lpgw->oversample) {
			xdash = float(curptr->x) * (rr - rl - 1) / float(lpgw->xmax) + rl;
			ydash = float(rb) - float(curptr->y) * (rb - rt - 1) / float(lpgw->ymax) + rt - 1;
		} else {
			xdash = MulDiv(curptr->x, rr - rl - 1, lpgw->xmax) + rl;
			ydash = rb - MulDiv(curptr->y, rb - rt - 1, lpgw->ymax) + rt - 1;
		}

		/* finish last filled polygon */
		if ((!last_poly.empty()) &&
			(((lastop == W_filled_polygon_draw) && (curptr->op != W_fillstyle)) ||
			 ((curptr->op == W_fillstyle) && (curptr->x != unsigned(last_fillstyle))))) {
			if (poly_graphics == NULL) {
				// changing smoothing mode is necessary in case of new/unknown code paths
				SmoothingMode mode = graphics.GetSmoothingMode();
				if (lpgw->antialiasing && !lpgw->polyaa)
					graphics.SetSmoothingMode(SmoothingModeNone);
				graphics.FillPolygon(fill_brush, last_poly.data(), last_polyi);
				graphics.SetSmoothingMode(mode);
			} else {
				poly_graphics->FillPolygon(fill_brush, last_poly.data(), last_polyi);
			}
			last_polyi = 0;
			last_poly.clear();
		}

		/* handle layer commands first */
		if (curptr->op == W_layer) {
			t_termlayer layer = (t_termlayer) curptr->x;
			switch (layer) {
				case TERM_LAYER_BEFORE_PLOT:
					plotno++;
					lpgw->numplots = plotno;
					if (plotno >= lpgw->maxhideplots) {
						unsigned int idx;
						lpgw->maxhideplots += 10;
						lpgw->hideplot = (BOOL *) realloc(lpgw->hideplot, lpgw->maxhideplots * sizeof(BOOL));
						for (idx = plotno; idx < lpgw->maxhideplots; idx++)
							lpgw->hideplot[idx] = FALSE;
					}
					if (plotno <= lpgw->maxhideplots)
						skipplot = (lpgw->hideplot[plotno - 1] > 0);
					break;
				case TERM_LAYER_AFTER_PLOT:
					skipplot = false;
					break;
#if 0
				case TERM_LAYER_BACKTEXT:
				case TERM_LAYER_FRONTTEXT
				case TERM_LAYER_END_TEXT:
					break;
#endif
				case TERM_LAYER_BEGIN_GRID:
					gridline = true;
					lpgw->hasgrid = TRUE;
					break;
				case TERM_LAYER_END_GRID:
					gridline = false;
					break;
				case TERM_LAYER_BEGIN_KEYSAMPLE:
					keysample = true;
					break;
				case TERM_LAYER_END_KEYSAMPLE:
					/* grey out keysample if graph is hidden */
					if ((plotno <= lpgw->maxhideplots) && lpgw->hideplot[plotno - 1]) {
						ARGB argb = Color::MakeARGB(128, 192, 192, 192);
						Color transparentgrey(argb);
						SolidBrush greybrush(transparentgrey);
						LPRECT bb = lpgw->keyboxes + plotno - 1;
						graphics.FillRectangle(&greybrush, INT(bb->left), INT(bb->bottom),
						                       bb->right - bb->left, bb->top - bb->bottom);
					}
					keysample = false;
					break;
				case TERM_LAYER_RESET:
				case TERM_LAYER_RESET_PLOTNO:
					plotno = 0;
					break;
				case TERM_LAYER_BEGIN_PM3D_MAP:
				case TERM_LAYER_BEGIN_PM3D_FLUSH:
					// Antialiasing of pm3d polygons is obtained by drawing to a
					// bitmap four times as large and copying it back with interpolation
					if (lpgw->antialiasing && lpgw->polyaa) {
						poly_bitmap = new Bitmap(poly_scale * (rr - rl), poly_scale * (rb - rt), &graphics);
						poly_graphics = Graphics::FromImage(poly_bitmap);
						poly_graphics->SetSmoothingMode(SmoothingModeNone);
						Matrix transform(poly_scale, 0.0f, 0.0f, poly_scale, 0.0f, 0.0f);
						poly_graphics->SetTransform(&transform);
					}
					break;
				case TERM_LAYER_END_PM3D_MAP:
				case TERM_LAYER_END_PM3D_FLUSH:
					if (poly_graphics != NULL) {
						delete poly_graphics;
						poly_graphics = NULL;
						graphics.SetInterpolationMode(InterpolationModeHighQualityBilinear);
						graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
						graphics.DrawImage(poly_bitmap, 0, 0, rr - rl, rb - rt);
						graphics.SetInterpolationMode(InterpolationModeNearestNeighbor);
						graphics.SetPixelOffsetMode(PixelOffsetModeNone);
						delete poly_bitmap;
					}
					break;
				case TERM_LAYER_BEGIN_IMAGE:
				case TERM_LAYER_BEGIN_COLORBOX:
					// Turn of antialiasing for failsafe/pixel images and color boxes
					// to avoid seams.
					if (lpgw->antialiasing)
						graphics.SetSmoothingMode(SmoothingModeNone);
					break;
				case TERM_LAYER_END_IMAGE:
				case TERM_LAYER_END_COLORBOX:
					if (lpgw->antialiasing)
						graphics.SetSmoothingMode(SmoothingModeAntiAlias8x8);
					break;
				default:
					break;
			};
		}

		/* Hide this layer? Do not skip commands which could affect key samples: */
		if (!(skipplot || (gridline && lpgw->hidegrid)) ||
			keysample || (curptr->op == W_line_type) || (curptr->op == W_setcolor)
			          || (curptr->op == W_pointsize) || (curptr->op == W_line_width)
			          || (curptr->op == W_dash_type)) {

		/* special case hypertexts */
		if ((hypertext != NULL) && (hypertype == TERM_HYPERTEXT_TOOLTIP)) {
			/* point symbols */
			if ((curptr->op >= W_dot) && (curptr->op <= W_dot + WIN_POINT_TYPES)) {
				RECT rect;
				rect.left = xdash - htic - 0.5;
				rect.right = xdash + htic + 0.5;
				rect.top = ydash - vtic - 0.5;
				rect.bottom = ydash + vtic + 0.5;
				add_tooltip(lpgw, &rect, hypertext);
				hypertext = NULL;
			}
		}

		switch (curptr->op) {
		case 0:	/* have run past last in this block */
			break;

		case W_layer: /* already handled above */
			break;

		case W_polyline: {
			POINTL * poly = (POINTL *) LocalLock(curptr->htext);
			if (poly == NULL) break; // memory allocation failed
			polyi = curptr->x;
			PointF * points = new PointF[polyi];
			for (int i = 0; i < polyi; i++) {
				// transform the coordinates
				if (lpgw->oversample) {
					points[i].X = float(poly[i].x) * (rr - rl - 1) / float(lpgw->xmax) + rl;
					points[i].Y = float(rb) - float(poly[i].y) * (rb - rt - 1) / float(lpgw->ymax) + rt - 1;
				} else {
					points[i].X = MulDiv(poly[i].x, rr - rl - 1, lpgw->xmax) + rl;
					points[i].Y = rb - MulDiv(poly[i].y, rb - rt - 1, lpgw->ymax) + rt - 1;
				}
			}
			LocalUnlock(poly);
			if (poly_graphics == NULL)
				gdiplusPolyline(graphics, pen, points, polyi);
			else
				gdiplusPolyline(*poly_graphics, pen, points, polyi);
			if (keysample) {
				draw_update_keybox(lpgw, plotno, points[0].X, points[0].Y);
				draw_update_keybox(lpgw, plotno, points[polyi - 1].X, points[polyi - 1].Y);
			}
			delete [] points;
			break;
		}

		case W_line_type: {
			int cur_pen = (int)curptr->x % WGNUMPENS;

			/* create new pens */
			if (cur_pen > LT_NODRAW) {
				cur_pen += 2;
				cur_penstruct.lopnWidth =
					(lpgw->color && isColor) ? lpgw->colorpen[cur_pen].lopnWidth : lpgw->monopen[cur_pen].lopnWidth;
				cur_penstruct.lopnColor = lpgw->colorpen[cur_pen].lopnColor;
				if (!lpgw->color || !isColor) {
					COLORREF color = cur_penstruct.lopnColor;
					unsigned luma = luma_from_color(GetRValue(color), GetGValue(color), GetBValue(color));
					cur_penstruct.lopnColor = RGB(luma, luma, luma);
				}
				cur_penstruct.lopnStyle =
					lpgw->dashed ? lpgw->monopen[cur_pen].lopnStyle : lpgw->colorpen[cur_pen].lopnStyle;
			} else if (cur_pen == LT_NODRAW) {
				cur_pen = WGNUMPENS;
				cur_penstruct.lopnStyle = PS_NULL;
				cur_penstruct.lopnColor = 0;
				cur_penstruct.lopnWidth.x = 1;
			} else { /* <= LT_BACKGROUND */
				cur_pen = WGNUMPENS;
				cur_penstruct.lopnStyle = PS_SOLID;
				cur_penstruct.lopnColor = lpgw->background;
				cur_penstruct.lopnWidth.x = 1;
			}
			cur_penstruct.lopnWidth.x *= line_width;

			Color color = gdiplusCreateColor(cur_penstruct.lopnColor, 1.);
			solid_brush.SetColor(color);

			solid_pen.SetColor(color);
			solid_pen.SetWidth(cur_penstruct.lopnWidth.x);

			pen.SetColor(color);
			pen.SetWidth(cur_penstruct.lopnWidth.x);
			if (cur_penstruct.lopnStyle <= PS_DASHDOTDOT)
				// cast is safe since GDI and GDI+ use the same numbers
				gdiplusSetDashStyle(&pen, static_cast<DashStyle>(cur_penstruct.lopnStyle));
			else
				pen.SetDashStyle(DashStyleSolid);

			/* remember this color */
			last_color = cur_penstruct.lopnColor;
			alpha_c = 1.;
			break;
		}

		case W_dash_type: {
			int dt = static_cast<int>(curptr->x);

			if (dt >= 0) {
				dt %= WGNUMPENS;
				dt += 2;
				cur_penstruct.lopnStyle = lpgw->monopen[dt].lopnStyle;
				gdiplusSetDashStyle(&pen, static_cast<DashStyle>(cur_penstruct.lopnStyle));
			} else if (dt == DASHTYPE_SOLID) {
				cur_penstruct.lopnStyle = PS_SOLID;
				gdiplusSetDashStyle(&pen, static_cast<DashStyle>(cur_penstruct.lopnStyle));
			} else if (dt == DASHTYPE_AXIS) {
				dt = 1;
				cur_penstruct.lopnStyle =
					lpgw->dashed ? lpgw->monopen[dt].lopnStyle : lpgw->colorpen[dt].lopnStyle;
				gdiplusSetDashStyle(&pen, static_cast<DashStyle>(cur_penstruct.lopnStyle));
			} else if (dt == DASHTYPE_CUSTOM) {
				t_dashtype * dash = static_cast<t_dashtype *>(LocalLock(curptr->htext));
				if (dash == NULL) break; // memory allocation failed
				INT count = 0;
				while ((dash->pattern[count] != 0.) && (count < DASHPATTERN_LENGTH)) count++;
				pen.SetDashPattern(dash->pattern, count);
				LocalUnlock(curptr->htext);
			}
			break;
		}

		case W_text_encoding:
			lpgw->encoding = (set_encoding_id) curptr->x;
			break;

		case W_put_text: {
			char * str;
			str = (char *) LocalLock(curptr->htext);
			if (str) {
				LPWSTR textw = UnicodeText(str, lpgw->encoding);
				if (textw) {
					if (lpgw->angle == 0) {
						PointF pointF(xdash + hshift, ydash + vshift);
						graphics.DrawString(textw, -1, font, pointF, &stringFormat, &solid_brush);
					} else {
						/* shift rotated text correctly */
						graphics.TranslateTransform(xdash + hshift, ydash + vshift);
						graphics.RotateTransform(-lpgw->angle);
						graphics.DrawString(textw, -1, font, PointF(0,0), &stringFormat, &solid_brush);
						graphics.ResetTransform();
					}
					RectF size;
					int dxl, dxr;
#ifndef EAM_BOXED_TEXT
					if (keysample) {
#else
					if (keysample || boxedtext.boxing) {
#endif
						graphics.MeasureString(textw, -1, font, PointF(0,0), &stringFormat, &size);
						if (lpgw->justify == LEFT) {
							dxl = 0;
							dxr = size.Width + 0.5;
						} else if (lpgw->justify == CENTRE) {
							dxl = dxr = size.Width / 2;
						} else {
							dxl = size.Width + 0.5;
							dxr = 0;
						}
					}
					if (keysample) {
						draw_update_keybox(lpgw, plotno, xdash - dxl, ydash - size.Height / 2);
						draw_update_keybox(lpgw, plotno, xdash + dxr, ydash + size.Height / 2);
					}
#ifdef EAM_BOXED_TEXT
					if (boxedtext.boxing) {
						if (boxedtext.box.left > (xdash - boxedtext.start.x - dxl))
							boxedtext.box.left = xdash - boxedtext.start.x - dxl;
						if (boxedtext.box.right < (xdash - boxedtext.start.x + dxr))
							boxedtext.box.right = xdash - boxedtext.start.x + dxr;
						if (boxedtext.box.top > (ydash - boxedtext.start.y - size.Height / 2))
							boxedtext.box.top = ydash - boxedtext.start.y - size.Height / 2;
						if (boxedtext.box.bottom < (ydash - boxedtext.start.y + size.Height / 2))
							boxedtext.box.bottom = ydash - boxedtext.start.y + size.Height / 2;
						/* We have to remember the text angle as well. */
						boxedtext.angle = lpgw->angle;
					}
#endif
					free(textw);
				}
			}
			LocalUnlock(curptr->htext);
			break;
		}

		case W_enhanced_text: {
			char * str = (char *) LocalLock(curptr->htext);
			if (str) {
				RECT extend;
				draw_enhanced_init(lpgw, graphics, solid_brush, rect);
				draw_enhanced_text(lpgw, rect, xdash, ydash, str);
				draw_get_enhanced_text_extend(&extend);

				if (keysample) {
					draw_update_keybox(lpgw, plotno, xdash - extend.left, ydash - extend.top);
					draw_update_keybox(lpgw, plotno, xdash + extend.right, ydash + extend.bottom);
				}
#ifdef EAM_BOXED_TEXT
				if (boxedtext.boxing) {
					if (boxedtext.box.left > (boxedtext.start.x - xdash - extend.left))
						boxedtext.box.left = boxedtext.start.x - xdash - extend.left;
					if (boxedtext.box.right < (boxedtext.start.x - xdash + extend.right))
						boxedtext.box.right = boxedtext.start.x - xdash + extend.right;
					if (boxedtext.box.top > (boxedtext.start.y - ydash - extend.top))
						boxedtext.box.top = boxedtext.start.y - ydash - extend.top;
					if (boxedtext.box.bottom < (boxedtext.start.y - ydash + extend.bottom))
						boxedtext.box.bottom = boxedtext.start.y - ydash + extend.bottom;
					/* We have to store the text angle as well. */
					boxedtext.angle = lpgw->angle;
				}
#endif
			}
			LocalUnlock(curptr->htext);
			break;
		}

		case W_hypertext:
			if (interactive) {
				/* Make a copy for future reference */
				char * str = (char *) LocalLock(curptr->htext);
				free(hypertext);
				hypertext = UnicodeText(str, lpgw->encoding);
				hypertype = curptr->x;
				LocalUnlock(curptr->htext);
			}
			break;

#ifdef EAM_BOXED_TEXT
		case W_boxedtext:
			if (seq == 0) {
				boxedtext.option = (t_textbox_options) curptr->x;
				seq++;
				break;
			}
			seq = 0;
			switch (boxedtext.option) {
			case TEXTBOX_INIT:
				/* initialise bounding box */
				boxedtext.box.left   = boxedtext.box.right = 0;
				boxedtext.box.bottom = boxedtext.box.top   = 0;
				boxedtext.start.x = xdash;
				boxedtext.start.y = ydash;
				/* Note: initialising the text angle here would be best IMHO,
				   but current core code does not set this until the actual
				   print-out is done. */
				boxedtext.angle = lpgw->angle;
				boxedtext.boxing = TRUE;
				break;
			case TEXTBOX_OUTLINE:
			case TEXTBOX_BACKGROUNDFILL: {
				/* draw rectangle */
				int dx = boxedtext.margin.x;
				int dy = boxedtext.margin.y;
				if ((boxedtext.angle % 90) == 0) {
					Rect rect;

					switch (boxedtext.angle) {
					case 0:
						rect.X      = + boxedtext.box.left;
						rect.Y      = + boxedtext.box.top;
						rect.Width  = boxedtext.box.right - boxedtext.box.left;
						rect.Height = boxedtext.box.bottom - boxedtext.box.top;
						rect.Inflate(dx, dy);
						break;
					case 90:
						rect.X      = + boxedtext.box.top;
						rect.Y      = - boxedtext.box.right;
						rect.Height = boxedtext.box.right - boxedtext.box.left;
						rect.Width  = boxedtext.box.bottom - boxedtext.box.top;
						rect.Inflate(dy, dx);
						break;
					case 180:
						rect.X      = - boxedtext.box.right;
						rect.Y      = - boxedtext.box.bottom;
						rect.Width  = boxedtext.box.right - boxedtext.box.left;
						rect.Height = boxedtext.box.bottom - boxedtext.box.top;
						rect.Inflate(dx, dy);
						break;
					case 270:
						rect.X      = - boxedtext.box.bottom;
						rect.Y      = + boxedtext.box.left;
						rect.Height = boxedtext.box.right - boxedtext.box.left;
						rect.Width  = boxedtext.box.bottom - boxedtext.box.top;
						rect.Inflate(dy, dx);
						break;
					}
					rect.Offset(boxedtext.start.x, boxedtext.start.y);
					if (boxedtext.option == TEXTBOX_OUTLINE) {
						graphics.DrawRectangle(&pen, rect);
					} else {
						/* Fill bounding box with current color. */
						graphics.FillRectangle(&solid_brush, rect);
					}
				} else {
					float theta = boxedtext.angle * M_PI / 180.;
					float sin_theta = sin(theta);
					float cos_theta = cos(theta);
					PointF rect[5];

					rect[0].X =  (boxedtext.box.left   - dx) * cos_theta +
								 (boxedtext.box.top    - dy) * sin_theta;
					rect[0].Y = -(boxedtext.box.left   - dx) * sin_theta +
								 (boxedtext.box.top    - dy) * cos_theta;
					rect[1].X =  (boxedtext.box.left   - dx) * cos_theta +
								 (boxedtext.box.bottom + dy) * sin_theta;
					rect[1].Y = -(boxedtext.box.left   - dx) * sin_theta +
								 (boxedtext.box.bottom + dy) * cos_theta;
					rect[2].X =  (boxedtext.box.right  + dx) * cos_theta +
								 (boxedtext.box.bottom + dy) * sin_theta;
					rect[2].Y = -(boxedtext.box.right  + dx) * sin_theta +
								 (boxedtext.box.bottom + dy) * cos_theta;
					rect[3].X =  (boxedtext.box.right  + dx) * cos_theta +
								 (boxedtext.box.top    - dy) * sin_theta;
					rect[3].Y = -(boxedtext.box.right  + dx) * sin_theta +
								 (boxedtext.box.top    - dy) * cos_theta;
					for (int i = 0; i < 4; i++) {
						rect[i].X += boxedtext.start.x;
						rect[i].Y += boxedtext.start.y;
					}
					if (boxedtext.option == TEXTBOX_OUTLINE) {
						rect[4].X = rect[0].X;
						rect[4].Y = rect[0].Y;
						gdiplusPolyline(graphics, pen, rect, 5);
					} else {
						graphics.FillPolygon(fill_brush, rect, 4);
					}
				}
				boxedtext.boxing = FALSE;
				break;
			}
			case TEXTBOX_MARGINS:
				/* Adjust size of whitespace around text: default is 1/2 char height + 2 char widths. */
				boxedtext.margin.x = MulDiv(curptr->x * lpgw->hchar, rr - rl, 1000 * lpgw->xmax);
				boxedtext.margin.y = MulDiv(curptr->y * lpgw->hchar, rr - rl, 1000 * lpgw->xmax);
				break;
			default:
				break;
			}
			break;
#endif

		case W_fillstyle: {
			/* HBB 20010916: new entry, needed to squeeze the many
			 * parameters of a filled box call through the bottleneck
			 * of the fixed number of parameters in GraphOp() and
			 * struct GWOP, respectively. */
			/* FIXME: resetting polyi here should not be necessary */
			polyi = 0; /* start new sequence */
			int fillstyle = curptr->x;

			/* Eliminate duplicate fillstyle requests. */
			if ((fillstyle == last_fillstyle) &&
				(last_color == last_fillcolor) &&
				(last_fill_alpha == alpha_c))
				break;

			transparent = false;
			switch (fillstyle & 0x0f) {
				case FS_TRANSPARENT_SOLID: {
					double alpha = (fillstyle >> 4) / 100.;
					solid_fill_brush.SetColor(gdiplusCreateColor(last_color, alpha));
					fill_brush = &solid_fill_brush;
					break;
				}
				case FS_SOLID: {
					if (alpha_c < 1.) {
						solid_fill_brush.SetColor(gdiplusCreateColor(last_color, alpha_c));
					} else if ((int)(fillstyle >> 4) == 100) {
						/* special case this common choice */
						solid_fill_brush.SetColor(gdiplusCreateColor(last_color, 1.));
					} else {
						double density = MINMAX(0, (int)(fillstyle >> 4), 100) * 0.01;
						COLORREF color =
							RGB(255 - density * (255 - GetRValue(last_color)),
								255 - density * (255 - GetGValue(last_color)),
								255 - density * (255 - GetBValue(last_color)));
						solid_fill_brush.SetColor(gdiplusCreateColor(color, 1.));
					}
					fill_brush = &solid_fill_brush;
					break;
				}
				case FS_TRANSPARENT_PATTERN:
					transparent = true;
					/* intentionally fall through */
				case FS_PATTERN: {
					/* style == 2 --> use fill pattern according to
							 * fillpattern. Pattern number is enumerated */
					int pattern = GPMAX(fillstyle >> 4, 0) % pattern_num;
					if (pattern_brush)
						delete pattern_brush;
					pattern_brush = gdiplusPatternBrush(pattern,
									last_color, 1., lpgw->background, transparent);
					fill_brush = pattern_brush;
					break;
				}
				case FS_EMPTY:
					/* FIXME: Instead of filling with background color, we should not fill at all in this case! */
					/* fill with background color */
					solid_fill_brush.SetColor(gdiplusCreateColor(lpgw->background, 1.));
					fill_brush = &solid_fill_brush;
					break;
				case FS_DEFAULT:
				default:
					/* Leave the current brush and color in place */
					solid_fill_brush.SetColor(gdiplusCreateColor(last_color, 1.));
					fill_brush = &solid_fill_brush;
					break;
			}
			last_fillstyle = fillstyle;
			last_fillcolor = last_color;
			last_fill_alpha = alpha_c;
			break;
		}

		case W_move:
			// This is used for filled boxes only.
			ppt[0].X = xdash;
			ppt[0].Y = ydash;
			break;

		case W_boxfill: {
			/* NOTE: the x and y passed with this call are the coordinates of the
			 * lower right corner of the box. The upper left corner was stored into
			 * ppt[0] by a preceding W_move, and the style was set
			 * by a W_fillstyle call. */
			// snap to pixel grid
			Point p1(xdash + 0.5, ydash + 0.5);
			Point p2(ppt[0].X + 0.5, ppt[0].Y + 0.5);
			Point p;
			int height, width;

			p.X = GPMIN(p1.X, p2.X);
			p.Y = GPMIN(p1.Y, p2.Y);
			width = abs(p2.X - p1.X);
			height = abs(p1.Y - p2.Y);

			SmoothingMode mode = graphics.GetSmoothingMode();
			graphics.SetSmoothingMode(SmoothingModeNone);
			graphics.FillRectangle(fill_brush, p.X, p.Y, width, height);
			graphics.SetSmoothingMode(mode);
			if (keysample)
				draw_update_keybox(lpgw, plotno, xdash + 1, ydash);
			polyi = 0;
			break;
		}

		case W_text_angle:
			if (lpgw->angle != (int)curptr->x) {
				lpgw->angle = (int)curptr->x;
				/* recalculate shifting of rotated text */
				hshift = - sin(M_PI / 180. * lpgw->angle) * lpgw->tmHeight / 2.;
				vshift = - cos(M_PI / 180. * lpgw->angle) * lpgw->tmHeight / 2.;
				if (lpgw->antialiasing) {
					// Cleartype is only applied to non-rotated text
					if ((lpgw->angle % 180) != 0)
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
					else
						graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
				}
			}
			break;

		case W_justify:
			switch (curptr->x) {
				case LEFT:
					stringFormat.SetAlignment(StringAlignmentNear);
					break;
				case RIGHT:
					stringFormat.SetAlignment(StringAlignmentFar);
					break;
				case CENTRE:
					stringFormat.SetAlignment(StringAlignmentCenter);
					break;
			}
			lpgw->justify = curptr->x;
			break;

		case W_font: {
			int size = curptr->x;
			char * fontname = (char *) LocalLock(curptr->htext);
			delete font;
#ifdef UNICODE
			/* FIXME: Maybe this should be in win.trm instead. */
			LPWSTR wfontname = UnicodeText(fontname, lpgw->encoding);
			font = SetFont_gdiplus(graphics, rect, lpgw, wfontname, size);
			free(wfontname);
#else
			font = SetFont_gdiplus(graphics, rect, lpgw, fontname, size);
#endif
			LocalUnlock(curptr->htext);
			/* recalculate shifting of rotated text */
			hshift = - sin(M_PI / 180. * lpgw->angle) * lpgw->tmHeight / 2.;
			vshift = - cos(M_PI / 180. * lpgw->angle) * lpgw->tmHeight / 2.;
			break;
		}

		case W_pointsize:
			if (curptr->x > 0) {
				double pointsize = curptr->x / 100.0;
				htic = MulDiv(pointsize * lpgw->pointscale * lpgw->htic, rr - rl, lpgw->xmax) + 1;
				vtic = MulDiv(pointsize * lpgw->pointscale * lpgw->vtic, rb - rt, lpgw->ymax) + 1;
			} else {
				htic = vtic = 0;
			}
			/* invalidate point symbol cache */
			last_symbol = W_invalid_pointtype;
			break;

		case W_line_width:
			/* HBB 20000813: this may look strange, but it ensures
			 * that linewidth is exactly 1 iff it's in default
			 * state */
			line_width = curptr->x == 100 ? 1 : (curptr->x / 100.0);
			line_width *= lpgw->linewidth * lw_scale;
			if (poly_graphics != NULL)
				line_width *= poly_scale;
			solid_pen.SetWidth(line_width);
			pen.SetWidth(line_width);
			/* invalidate point symbol cache */
			last_symbol = W_invalid_pointtype;
			break;

		case W_setcolor: {
			COLORREF color;

			/* distinguish gray values and RGB colors */
			if (curptr->htext != NULL) {	/* TC_LT */
				int pen = (int)curptr->x % WGNUMPENS;
				color = (pen <= LT_NODRAW) ? lpgw->background : lpgw->colorpen[pen + 2].lopnColor;
				if (!lpgw->color || !isColor) {
					unsigned luma = luma_from_color(GetRValue(color), GetGValue(color), GetBValue(color));
					color = RGB(luma, luma, luma);
				}
				alpha_c = 1.;
			} else {					/* TC_RGB */
				rgb255_color rgb255;
				rgb255.r = (curptr->y & 0xff);
				rgb255.g = (curptr->x >> 8);
				rgb255.b = (curptr->x & 0xff);
				alpha_c = 1. - ((curptr->y >> 8) & 0xff) / 255.;

				if (lpgw->color || ((rgb255.r == rgb255.g) && (rgb255.r == rgb255.b))) {
					/* Use colors or this is already gray scale */
					color = RGB(rgb255.r, rgb255.g, rgb255.b);
				} else {
					/* convert to gray */
					unsigned luma = luma_from_color(rgb255.r, rgb255.g, rgb255.b);
					color = RGB(luma, luma, luma);
				}
			}

			/* update brushes and pens */
			Color pcolor = gdiplusCreateColor(color, alpha_c);
			solid_brush.SetColor(pcolor);
			pen.SetColor(pcolor);
			solid_pen.SetColor(pcolor);

			/* invalidate point symbol cache */
			if (last_color != color)
				last_symbol = W_invalid_pointtype;

			/* remember this color */
			cur_penstruct.lopnColor = color;
			last_color = color;
			break;
		}

		case W_filled_polygon_pt: {
			/* a point of the polygon is coming */
			if (polyi >= polymax) {
				polymax += 200;
				ppt.reserve(polymax + 1);
			}
			ppt[polyi].X = xdash;
			ppt[polyi].Y = ydash;
			polyi++;
			break;
		}

		case W_filled_polygon_draw: {
			bool found = false;
			int i, k;
			//bool same_rot = true;

			// Test if successive polygons share a common edge:
			if ((!last_poly.empty()) && (polyi > 2)) {
				// Check for a common edge with previous filled polygon.
				for (i = 0; (i < polyi) && !found; i++) {
					for (k = 0; (k < last_polyi) && !found; k++) {
						if ((ppt[i].X == last_poly[k].X) && (ppt[i].Y == last_poly[k].Y)) {
							if ((ppt[(i + 1) % polyi].X == last_poly[(k + 1) % last_polyi].X) &&
							    (ppt[(i + 1) % polyi].Y == last_poly[(k + 1) % last_polyi].Y)) {
								//found = true;
								//same_rot = true;
							}
							// This is the dominant case for filling between curves,
							// see fillbetween.dem and polar.dem.
							if ((ppt[(i + 1) % polyi].X == last_poly[(k + last_polyi - 1) % last_polyi].X) &&
							    (ppt[(i + 1) % polyi].Y == last_poly[(k + last_polyi - 1) % last_polyi].Y)) {
								found = true;
								//same_rot = false;
							}
						}
					}
				}
			}

			if (found) { // merge polygons
				// rewind
				i--; k--;

				int extra = polyi - 2;
				// extend buffer to make room for extra points
				last_poly.reserve(last_polyi + extra + 1);
				/* TODO: should use memmove instead */
				for (int n = last_polyi - 1; n >= k; n--) {
					last_poly[n + extra].X = last_poly[n].X;
					last_poly[n + extra].Y = last_poly[n].Y;
				}
				// copy new points
				for (int n = 0; n < extra; n++) {
					last_poly[k + n].X = ppt[(i + 2 + n) % polyi].X;
					last_poly[k + n].Y = ppt[(i + 2 + n) % polyi].Y;
				}
				last_polyi += extra;
			} else {
				if (!last_poly.empty()) {
					if (poly_graphics == NULL) {
						// changing smoothing mode is still necessary in case of new/unknown code paths
						SmoothingMode mode = graphics.GetSmoothingMode();
						if (lpgw->antialiasing && !lpgw->polyaa)
							graphics.SetSmoothingMode(SmoothingModeNone);
						graphics.FillPolygon(fill_brush, last_poly.data(), last_polyi);
						graphics.SetSmoothingMode(mode);
					} else {
						poly_graphics->FillPolygon(fill_brush, last_poly.data(), last_polyi);
					}
				}
				// save the current polygon
				last_poly = ppt;
				last_polyi = polyi;
			}

			polyi = 0;
			break;
		}

		case W_image:	{
			/* Due to the structure of gwop 6 entries are needed in total. */
			if (seq == 0) {
				/* First OP contains only the color mode */
				color_mode = curptr->x;
			} else if (seq < 5) {
				/* Next four OPs contain the `corner` array */
				corners[seq - 1].x = xdash + 0.5;
				corners[seq - 1].y = ydash + 0.5;
			} else {
				/* The last OP contains the image and it's size */
				char * image = (char *) LocalLock(curptr->htext);
				if (image == NULL) break; // memory allocation failed
				unsigned int width = curptr->x;
				unsigned int height = curptr->y;
				if (image) {
					Bitmap * bitmap;

					graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);

					/* create clip region */
					Rect clipRect(
						(INT) GPMIN(corners[2].x, corners[3].x), (INT) GPMIN(corners[2].y, corners[3].y),
						abs(corners[2].x - corners[3].x), abs(corners[2].y - corners[3].y));
					graphics.SetClip(clipRect);

					if (color_mode != IC_RGBA) {
						int pad_bytes = (4 - (3 * width) % 4) % 4; /* scan lines start on ULONG boundaries */
						int stride = width * 3 + pad_bytes;
						bitmap = new Bitmap(width, height, stride, PixelFormat24bppRGB, (BYTE *) image);
					} else {
						int stride = width * 4;
						bitmap = new Bitmap(width, height, stride, PixelFormat32bppARGB, (BYTE *) image);
					}

					if (bitmap) {
						/* image is upside-down */
						bitmap->RotateFlip(RotateNoneFlipY);
						if (lpgw->color) {
							graphics.DrawImage(bitmap,
								(INT) GPMIN(corners[0].x, corners[1].x),
								(INT) GPMIN(corners[0].y, corners[1].y),
								abs(corners[1].x - corners[0].x),
								abs(corners[1].y - corners[0].y));
						} else {
							/* convert to grayscale */
							ColorMatrix cm = {{{0.30f, 0.30f, 0.30f, 0, 0},
											   {0.59f, 0.59f, 0.59f, 0, 0},
											   {0.11f, 0.11f, 0.11f, 0, 0},
											   {0, 0, 0, 1, 0},
											   {0, 0, 0, 0, 1}
											 }};
							ImageAttributes ia;
							ia.SetColorMatrix(&cm, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
							graphics.DrawImage(bitmap,
								RectF((INT) GPMIN(corners[0].x, corners[1].x),
									(INT) GPMIN(corners[0].y, corners[1].y),
									abs(corners[1].x - corners[0].x),
									abs(corners[1].y - corners[0].y)),
								0, 0, width, height,
								UnitPixel, &ia);
						}
						delete bitmap;
					}
					graphics.ResetClip();
					graphics.SetPixelOffsetMode(PixelOffsetModeNone);
				}
				LocalUnlock(curptr->htext);
			}
			seq = (seq + 1) % 6;
			break;
		}

		default: {
			enum win_pointtypes symbol = (enum win_pointtypes) curptr->op;

			/* This covers only point symbols. All other codes should be
			   handled in the switch statement. */
			if ((symbol < W_dot) || (symbol > W_last_pointtype))
				break;

			// draw cached point symbol
			if (ps_caching && (last_symbol == symbol) && (cb != NULL)) {
				// always draw point symbols on integer (pixel) positions
				if (lpgw->oversample)
					graphics.DrawCachedBitmap(cb, INT(xdash + 0.5) - cb_ofs.x, INT(ydash + 0.5) - cb_ofs.y);
				else
					graphics.DrawCachedBitmap(cb, xdash - cb_ofs.x, ydash - cb_ofs.y);
				break;
			} else {
				if (cb != NULL) {
					delete cb;
					cb = NULL;
				}
			}

			Bitmap *b = 0;
			Graphics *g = 0;
			int xofs;
			int yofs;

			// Switch between cached and direct drawing
			if (ps_caching) {
				// Create a compatible bitmap
				b = new Bitmap(2 * htic + 3, 2 * vtic + 3, &graphics);
				g = Graphics::FromImage(b);
				if (lpgw->antialiasing)
					g->SetSmoothingMode(SmoothingModeAntiAlias8x8);
				cb_ofs.x = xofs = htic + 1;
				cb_ofs.y = yofs = vtic + 1;
				last_symbol = symbol;
			} else {
				g = &graphics;
				// snap to pixel
				if (lpgw->oversample) {
					xofs = xdash + 0.5;
					yofs = ydash + 0.5;
				} else {
					xofs = xdash;
					yofs = ydash;
				}
			}

			switch (symbol) {
			case W_dot:
				gdiplusDot(*g, solid_brush, xofs, yofs);
				break;
			case W_plus: /* do plus */
			case W_star: /* do star: first plus, then cross */
				g->DrawLine(&solid_pen, xofs - htic, yofs, xofs + htic, yofs);
				g->DrawLine(&solid_pen, xofs, yofs - vtic, xofs, yofs + vtic);
				if (symbol == W_plus)
					break;
			case W_cross: /* do X */
				g->DrawLine(&solid_pen, xofs - htic, yofs - vtic, xofs + htic - 1, yofs + vtic);
				g->DrawLine(&solid_pen, xofs - htic, yofs + vtic, xofs + htic - 1, yofs - vtic);
				break;
			case W_circle: /* do open circle */
				g->DrawEllipse(&solid_pen, xofs - htic, yofs - htic, 2 * htic, 2 * htic);
				break;
			case W_fcircle: /* do filled circle */
				g->FillEllipse(&solid_brush, xofs - htic, yofs - htic, 2 * htic, 2 * htic);
				break;
			default: {	/* potentially closed figure */
				Point p[6];
				int i;
				int shape = 0;
				int filled = 0;
				int index = 0;
				const float pointshapes[6][10] = {
					{-1, -1, +1, -1, +1, +1, -1, +1, 0, 0}, /* box */
					{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* dummy, circle */
					{ 0, -4./3, -4./3, 2./3,
						4./3,  2./3, 0, 0}, /* triangle */
					{ 0, 4./3, -4./3, -2./3,
						4./3,  -2./3, 0, 0}, /* inverted triangle */
					{ 0, +1, -1,  0,  0, -1, +1,  0, 0, 0}, /* diamond */
					{ 0, 1, 0.95106, 0.30902, 0.58779, -0.80902,
						-0.58779, -0.80902, -0.95106, 0.30902} /* pentagon */
				};

				// This should never happen since all other codes should be
				// handled in the switch statement.
				if ((symbol < W_box) || (symbol > W_last_pointtype))
					break;

				// Calculate index, instead of an ugly long switch statement;
				// Depends on definition of commands in wgnuplib.h.
				index = (symbol - W_box);
				shape = index / 2;
				filled = (index % 2) > 0;

				for (i = 0; i < 5; ++i) {
					if (pointshapes[shape][i * 2 + 1] == 0
						&& pointshapes[shape][i * 2] == 0)
						break;
					p[i].X = xofs + htic * pointshapes[shape][i * 2] + 0.5;
					p[i].Y = yofs + vtic * pointshapes[shape][i * 2 + 1] + 0.5;
				}
				if (filled) {
					/* filled polygon with border */
					g->FillPolygon(&solid_brush, p, i);
				} else {
					/* Outline polygon */
					p[i].X = p[0].X;
					p[i].Y = p[0].Y;
					gdiplusPolyline(*g, solid_pen, p, i + 1);
					gdiplusDot(*g, solid_brush, xofs, yofs);
				}
			} /* default case */
			} /* switch (point symbol) */

			if (b != NULL) {
				// create a cached bitmap for faster redrawing
				cb = new CachedBitmap(b, &graphics);
				// display point symbol snapped to pixel
				if (lpgw->oversample)
					graphics.DrawCachedBitmap(cb, INT(xdash + 0.5) - xofs, INT(ydash + 0.5) - yofs);
				else
					graphics.DrawCachedBitmap(cb, xdash - xofs, ydash - yofs);
				delete b;
				delete g;
			}

			if (keysample) {
				draw_update_keybox(lpgw, plotno, xdash + htic, ydash + vtic);
				draw_update_keybox(lpgw, plotno, xdash - htic, ydash - vtic);
			}
			break;
			} /* default case */
		} /* switch(opcode) */
		} /* hide layer? */

		lastop = curptr->op;
		ngwop++;
		curptr++;
		if ((unsigned)(curptr - blkptr->gwop) >= GWOPMAX) {
			GlobalUnlock(blkptr->hblk);
			blkptr->gwop = (struct GWOP *)NULL;
			if ((blkptr = blkptr->next) == NULL)
				/* If exact multiple of GWOPMAX entries are queued,
				 * next will be NULL. Only the next GraphOp() call would
				 * have allocated a new block */
				break;
			if (!blkptr->gwop)
				blkptr->gwop = (struct GWOP *)GlobalLock(blkptr->hblk);
			if (!blkptr->gwop)
				break;
			curptr = (struct GWOP *)blkptr->gwop;
		}
	} /* while (ngwop < lpgw->nGWOP) */

	/* clean-up */
	if (pattern_brush)
		delete pattern_brush;
	if (cb)
		delete cb;
	if (font)
		delete font;
	ppt.clear();
}


UINT nImageCodecs = 0; // number of image encoders
ImageCodecInfo * pImageCodecInfo = NULL;  // list of image encoders


static int
gdiplusGetEncoders()
{
	UINT num = 0;
	UINT size = 0;

	GetImageEncodersSize(&nImageCodecs, &size);
	if (size == 0)
		return -1;
	pImageCodecInfo = (ImageCodecInfo *) malloc(size);
	if (pImageCodecInfo == NULL)
		return -1;
	GetImageEncoders(nImageCodecs, size, pImageCodecInfo);
	return num;
}


void
SaveAsBitmap(LPGW lpgw)
{
	static OPENFILENAMEW Ofn;
	static WCHAR lpstrCustomFilter[256] = { '\0' };
	static WCHAR lpstrFileName[MAX_PATH] = { '\0' };
	static WCHAR lpstrFileTitle[MAX_PATH] = { '\0' };
	UINT i;

	// make sure GDI+ is initialized
	gdiplusInit();

	// ask GDI+ about supported encoders
	if (pImageCodecInfo == NULL)
		if (gdiplusGetEncoders() < 0)
			std::cerr << "Error:  GDI+ could not retrieve the list of encoders" << std::endl;
#if 0
	for (i = 0; i < nImageCodecs; i++) {
		char * descr = AnsiText(pImageCodecInfo[i].FormatDescription, S_ENC_DEFAULT);
		char * ext = AnsiText(pImageCodecInfo[i].FilenameExtension, S_ENC_DEFAULT);
		std::cerr << i << ": " << descr << " " << ext << std::endl;
		free(descr);
		free(ext);
	}
#endif

	// assemble filter list
	UINT npng = 1;
	size_t len;
	for (i = 0, len = 1; i < nImageCodecs; i++) {
		len += wcslen(pImageCodecInfo[i].FormatDescription) + wcslen(pImageCodecInfo[i].FilenameExtension) + 2;
		// make PNG the default selection
		if (wcsnicmp(pImageCodecInfo[i].FormatDescription, L"PNG", 3) == 0)
			npng = i + 1;
	}
	LPWSTR filter = (LPWSTR) malloc(len * sizeof(WCHAR));
	swprintf_s(filter, len, L"%ls\t%ls\t", pImageCodecInfo[0].FormatDescription, pImageCodecInfo[0].FilenameExtension);
	for (i = 1; i < nImageCodecs; i++) {
		size_t len2 = wcslen(pImageCodecInfo[i].FormatDescription) + wcslen(pImageCodecInfo[i].FilenameExtension) + 3;
		LPWSTR type = (LPWSTR) malloc(len2 * sizeof(WCHAR));
		swprintf_s(type, len2, L"%ls\t%ls\t", pImageCodecInfo[i].FormatDescription, pImageCodecInfo[i].FilenameExtension);
		wcscat(filter, type);
		free(type);
	}
	for (i = 1; i < len; i++) {
		if (filter[i] == TEXT('\t'))
			filter[i] = TEXT('\0');
	}

	// init save file dialog parameters
	Ofn.lStructSize = sizeof(OPENFILENAME);
	Ofn.hwndOwner = lpgw->hWndGraph;
	Ofn.lpstrInitialDir = NULL;
	Ofn.lpstrFilter = filter;
	Ofn.lpstrCustomFilter = lpstrCustomFilter;
	Ofn.nMaxCustFilter = 255;
	Ofn.nFilterIndex = npng;
	Ofn.lpstrFile = lpstrFileName;
	Ofn.nMaxFile = MAX_PATH;
	Ofn.lpstrFileTitle = lpstrFileTitle;
	Ofn.nMaxFileTitle = MAX_PATH;
	Ofn.lpstrInitialDir = NULL;
	Ofn.lpstrTitle = NULL;
	Ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN;
	Ofn.lpstrDefExt = L"png";

	if (GetSaveFileNameW(&Ofn) != 0) {
		// Note that there might be a copy in lpgw->hBitmap., but documentation
		// says we may not use that (although it seems to work).
		// So we get a new copy of the screen:
		HBITMAP hBitmap = GraphGetBitmap(lpgw);
		Bitmap * bitmap = Bitmap::FromHBITMAP(hBitmap, 0);
		UINT ntype = Ofn.nFilterIndex - 1;
#if 0
		LPWSTR wtype = pImageCodecInfo[ntype].FormatDescription;
		char * type = AnsiText(wtype, S_ENC_DEFAULT);
		SizeF size;
		bitmap->GetPhysicalDimension(&size);
		std::cerr << std::endl << "Saving bitmap: size: " << size.Width << " x " << size.Height << "  type: " << type << " ..." << std::endl;
		free(type);
#endif
		bitmap->Save(Ofn.lpstrFile, &(pImageCodecInfo[ntype].Clsid), NULL);
		delete bitmap;
		DeleteObject(hBitmap);
	}
	free(filter);
}


static Bitmap *
ResizeBitmap(Bitmap &bmp, INT width, INT height)
{
	unsigned iheight = bmp.GetHeight();
	unsigned iwidth = bmp.GetWidth();
	if (width != height) {
		double aspect = (double)iwidth / iheight;
		if (iwidth > iheight)
			height = static_cast<unsigned>(width / aspect);
		else
			width = static_cast<unsigned>(height * aspect);
	}
	Bitmap * nbmp = new Bitmap(width, height, bmp.GetPixelFormat());
	Graphics graphics(nbmp);
	graphics.DrawImage(&bmp, 0, 0, width - 1, height - 1);
	return nbmp;
}


HBITMAP
gdiplusLoadBitmap(LPWSTR file, int size)
{
	gdiplusInit();
	Bitmap ibmp(file);
	Bitmap * bmp = ResizeBitmap(ibmp, size, size);
	HBITMAP hbitmap;
	Color color(0, 0, 0);
	bmp->GetHBITMAP(color, &hbitmap);
	delete bmp;
	return hbitmap;
}
