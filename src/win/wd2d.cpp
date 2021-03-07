/*
Copyright (c) 2017 Bastian Maerkisch. All rights reserved.

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

// Enable to activate the Direct2D debug layer
//#define D2DDEBUG

// include iostream / cstdio _before_ syscfg.h in order
// to avoid re-definition by wtext.h/winmain.c routines
#include <iostream>
extern "C" {
# include "syscfg.h"
}

#include <windows.h>
#include <windowsx.h>

#include "wd2d.h"

#ifndef HAVE_D2D11
# include <d2d1.h>
# include <d2d1helper.h>
# include <dwrite.h>
#else
# include <d2d1_1.h>
# include <d2d1_1helper.h>
# include <d3d11_1.h>
# include <dwrite_1.h>
# include <d2d1effects.h>
# include <d2d1effecthelpers.h>
#ifdef HAVE_PRNTVPT
#  include <prntvpt.h>
#endif
# include <wincodec.h>
# include <xpsobjectmodel_1.h>
# include <documenttarget.h>
#endif
#include <tchar.h>
#include <wchar.h>

#include "wgnuplib.h"
#include "wcommon.h"

#ifndef __WATCOMC__
// MSVC and MinGW64 have the __uuidof() special function
# define HAVE_UUIDOF
#endif

// Note: This may or may not be declared in an enum in SDK headers.
//       Unfortunately, we cannot test for the SDK version here, so we always define it
//       ourselves.  MinGW headers currently do define that anyway.
# define D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT static_cast<D2D1_DRAW_TEXT_OPTIONS>(0x00000004)

#define MINMAX(a,val,b) (((val) <= (a)) ? (a) : ((val) <= (b) ? (val) : (b)))
const int pattern_num = 8;
const float textbox_width = 3000.f;

#ifndef HAVE_D2D11
static ID2D1Factory * g_pDirect2dFactory = NULL;
static IDWriteFactory * g_pDWriteFactory = NULL;
#else
static ID2D1Factory1 * g_pDirect2dFactory = NULL;
static IDWriteFactory1 * g_pDWriteFactory = NULL;
static ID3D11Device1 * g_pDirect3dDevice = NULL;
static IWICImagingFactory * g_wicFactory = NULL;
#endif
#ifdef DCRENDERER
// All graph windows share a common DC render target
static ID2D1DCRenderTarget * g_pRenderTarget = NULL;
#endif
static bool bHaveColorFonts = false;

static HRESULT d2dCreateStrokeStyle(D2D1_DASH_STYLE dashStyle, BOOL rounded, ID2D1StrokeStyle **ppStrokeStyle);
static HRESULT d2dCreateStrokeStyle(const FLOAT * dashes, UINT dashesCount, BOOL rounded, ID2D1StrokeStyle **ppStrokeStyle);
static HRESULT d2dCreateStrokeStyle(D2D1_DASH_STYLE dashStyle, const FLOAT * dashes, UINT dashesCount, BOOL rounded, ID2D1StrokeStyle **ppStrokeStyle);
static HRESULT d2dPolyline(ID2D1RenderTarget * pRenderTarget, ID2D1SolidColorBrush * pBrush, ID2D1StrokeStyle * pStrokeStyle, float width, D2D1_POINT_2F *points, int polyi, bool closed = false);
static HRESULT d2dFilledPolygon(ID2D1RenderTarget * pRenderTarget, ID2D1Brush * pBrush, D2D1_POINT_2F *points, int polyi);
static void d2dDot(ID2D1RenderTarget * pRenderTarget, ID2D1SolidColorBrush * pBrush, float x, float y);
static HRESULT d2dMeasureText(ID2D1RenderTarget * pRenderTarget, LPCWSTR text, IDWriteTextFormat * pWriteTextFormat, D2D1_SIZE_F * size);
static inline D2D1::ColorF D2DCOLORREF(COLORREF c, float a);
static HRESULT d2d_do_draw(LPGW lpgw, ID2D1RenderTarget * pRenderTarget, LPRECT rect, bool interactive);


/* Mingw64 currently does not provide the Print Ticket Header, so we put
our own definitions here, taken from MSDN */
#ifndef HAVE_PRNTVPT
extern "C" {
	typedef HANDLE HPTPROVIDER;
	typedef enum tagEPrintTicketScope {
		kPTPageScope,
		kPTDocumentScope,
		kPTJobScope
	} EPrintTicketScope;
	HRESULT WINAPI PTConvertDevModeToPrintTicket(HPTPROVIDER, ULONG, PDEVMODE, EPrintTicketScope, IStream *);
	HRESULT WINAPI PTCloseProvider(HPTPROVIDER);
	HRESULT WINAPI PTOpenProvider(PCWSTR, DWORD, HPTPROVIDER *);
}
#endif


/* Release COM pointers safely
*/
template<class Interface>
inline void SafeRelease(Interface **ppInterfaceToRelease)
{
	if (*ppInterfaceToRelease != NULL) {
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = NULL;
	}
}


/* Internal state of enhanced text processing.
*/
static struct {
	ID2D1RenderTarget * pRenderTarget;
	IDWriteTextFormat * pWriteTextFormat;
	ID2D1SolidColorBrush *pBrush;
} enhstate_d2d;


/* ****************  D2D initialization   ************************* */

#if defined(HAVE_D2D11) && !defined(DCRENDERER)
static HRESULT
d2dCreateD3dDevice(D3D_DRIVER_TYPE const type, ID3D11Device **device)
{
	// Set feature levels supported by our application
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// This flag adds support for surfaces with a different color channel ordering
	// than the API default. It is required for compatibility with Direct2D.
	UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef D2DDEBUG
	// Note: This will fail if the D3D debug layer is not installed!
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL returnedFeatureLevel;
	return D3D11CreateDevice(NULL, type, 0, flags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
						device, &returnedFeatureLevel, NULL);
}


static HRESULT
d2dCreateDeviceSwapChainBitmap(LPGW lpgw)
{
	// derived from code on "Katy's Code" Blog:
	// https://katyscode.wordpress.com/2013/01/23/migrating-existing-direct2d-applications-to-use-direct2d-1-1-functionality-in-windows-7/
	// and Kenny Kerr's MSDN Magazine Blog:
	// https://msdn.microsoft.com/en-us/magazine/dn198239.aspx
	HRESULT hr = S_OK;
	ID2D1DeviceContext * pDirect2dDeviceContext = lpgw->pRenderTarget;
	IDXGISwapChain1 * pDXGISwapChain = lpgw->pDXGISwapChain;

	if (pDirect2dDeviceContext == NULL || g_pDirect2dFactory == NULL)
		return hr;

	// do we already have a swap chain?
	if (pDXGISwapChain == NULL) {
		IDXGIDevice * dxgiDevice = NULL;
		hr = g_pDirect3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void **) &dxgiDevice);

		// Get the Display Adapter (virtual or GPU) we are using
		IDXGIAdapter *dxgiAdapter = NULL;
		if (SUCCEEDED(hr))
			hr = dxgiDevice->GetAdapter(&dxgiAdapter);

		// Get the DXGI factory instance
		IDXGIFactory2 *dxgiFactory = NULL;
		if (SUCCEEDED(hr))
			hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

		// Parameters for a Windows 7-compatible swap chain
		DXGI_SWAP_CHAIN_DESC1 props = { };
		props.Width = 0;
		props.Height = 0;
		props.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		props.Stereo = false;
		props.SampleDesc.Count = 1;
		props.SampleDesc.Quality = 0;
		props.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		props.BufferCount = 2;
		props.Scaling = DXGI_SCALING_STRETCH; // DXGI_SCALING_NONE
		props.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;  // DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
		props.Flags = 0;

		// Create DXGI swap chain targeting a window handle
		if (SUCCEEDED(hr))
			hr = dxgiFactory->CreateSwapChainForHwnd(g_pDirect3dDevice, lpgw->hGraph, &props, NULL, NULL, &pDXGISwapChain);
		if (SUCCEEDED(hr))
			lpgw->pDXGISwapChain = pDXGISwapChain;

		SafeRelease(&dxgiDevice);
		SafeRelease(&dxgiAdapter);
		SafeRelease(&dxgiFactory);
	}

	// Get the back-buffer as an IDXGISurface
	IDXGISurface * dxgiBackBuffer = NULL;
	if (SUCCEEDED(hr))
		hr = pDXGISwapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));

	// Get screen DPI
	FLOAT dpiX, dpiY;
	g_pDirect2dFactory->GetDesktopDpi(&dpiX, &dpiY);

	// Create a Direct2D surface (bitmap) linked to the Direct3D texture back buffer via the DXGI back buffer
	D2D1_BITMAP_PROPERTIES1 bitmapProperties =
		D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dpiX, dpiY);
	ID2D1Bitmap1 * pDirect2dBackBuffer = NULL;
	if (SUCCEEDED(hr))
		hr = pDirect2dDeviceContext->CreateBitmapFromDxgiSurface(dxgiBackBuffer, &bitmapProperties, &pDirect2dBackBuffer);
	if (SUCCEEDED(hr)) {
		pDirect2dDeviceContext->SetTarget(pDirect2dBackBuffer);
		pDirect2dDeviceContext->SetDpi(dpiX, dpiY);
		// Avoid rescaling units all the time
		pDirect2dDeviceContext->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
	}

	SafeRelease(&dxgiBackBuffer);
	SafeRelease(&pDirect2dBackBuffer);
	return hr;
}
#endif


HRESULT
d2dInit(LPGW lpgw)
{
	HRESULT hr = S_OK;

	// Create D2D factory
	if (g_pDirect2dFactory == NULL) {
		D2D1_FACTORY_OPTIONS options;
		ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));
#ifdef D2DDEBUG
		options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
		hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
#ifdef HAVE_UUIDOF
				__uuidof(g_pDirect2dFactory),
#else
				IID_ID2D1Factory,
#endif
				&options, reinterpret_cast<void **>(&g_pDirect2dFactory));
	}

	// Create a DirectWrite factory
	if (SUCCEEDED(hr) && g_pDWriteFactory == NULL) {
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
#ifdef HAVE_UUIDOF
			__uuidof(g_pDWriteFactory),
#else
			IID_IDWriteFactory,
#endif
			reinterpret_cast<IUnknown **>(&g_pDWriteFactory)
		);
	}

	// Create a render target
	if (SUCCEEDED(hr) && lpgw->pRenderTarget == NULL) {
#ifdef DCRENDERER
		if (g_pRenderTarget == NULL) {
			D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
				D2D1_RENDER_TARGET_TYPE_DEFAULT,
				D2D1::PixelFormat(
					DXGI_FORMAT_B8G8R8A8_UNORM,
					D2D1_ALPHA_MODE_IGNORE),
				0,
				0,
				D2D1_RENDER_TARGET_USAGE_NONE,
				D2D1_FEATURE_LEVEL_DEFAULT
			);
			hr = g_pDirect2dFactory->CreateDCRenderTarget(&props, &g_pRenderTarget);
		}
		if (SUCCEEDED(hr)) {
			lpgw->pRenderTarget = g_pRenderTarget;
		}
#else
#ifndef HAVE_D2D11
		ID2D1HwndRenderTarget *pRenderTarget;
		D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
		rtProps.usage = D2D1_RENDER_TARGET_USAGE_NONE;

		HWND hwnd = lpgw->hGraph;
		RECT rect;
		GetClientRect(hwnd, &rect);
		D2D1_SIZE_U size = D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top);

		// Create a Hwnd render target.
		hr = g_pDirect2dFactory->CreateHwndRenderTarget(
			rtProps,
			D2D1::HwndRenderTargetProperties(hwnd, size),
			&pRenderTarget
		);
		if (SUCCEEDED(hr)) {
			// This makes pixels = DIPs, so we don't have to rescale positions etc.
			// it comes at the expense of loosing the correct threshold for anti-aliasing.
			pRenderTarget->SetDpi(96.f, 96.f);
			lpgw->pRenderTarget = pRenderTarget;
		}
#else
		// global objects (all windows)
		if (g_pDirect3dDevice == NULL) {
			// Create Direct3D device
			ID3D11Device * device = NULL;
			hr = d2dCreateD3dDevice(D3D_DRIVER_TYPE_HARDWARE, &device);
			if (hr == DXGI_ERROR_UNSUPPORTED)
				hr = d2dCreateD3dDevice(D3D_DRIVER_TYPE_WARP, &device);
			if (SUCCEEDED(hr))
				hr = device->QueryInterface(__uuidof(ID3D11Device1), (void **) &g_pDirect3dDevice);
			SafeRelease(&device);
		}

		// window objects
		if (SUCCEEDED(hr)) {
			IDXGIDevice * dxgiDevice = NULL;
			hr = g_pDirect3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void **) &dxgiDevice);

			// Create Direct2D device and context (our render target)
			ID2D1Device * pDirect2dDevice = NULL;
			ID2D1DeviceContext * pDirect2dDeviceContext = NULL;
			if (SUCCEEDED(hr) && lpgw->pDirect2dDevice == NULL)
				hr = g_pDirect2dFactory->CreateDevice(dxgiDevice, &pDirect2dDevice);
			if (SUCCEEDED(hr))
				lpgw->pDirect2dDevice = pDirect2dDevice;
			if (SUCCEEDED(hr))
				hr = pDirect2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &pDirect2dDeviceContext);
			if (SUCCEEDED(hr))
				lpgw->pRenderTarget = pDirect2dDeviceContext;

			SafeRelease(&dxgiDevice);
		}

		if (SUCCEEDED(hr))
			hr = d2dCreateDeviceSwapChainBitmap(lpgw);
#endif
#endif

		// Test for Windows 8.1 or later (version >= 6.3).
		// We don't use versionhelpers.h as this is not available for OpenWatcom.
		// Note: We should rather do a feature test here.
		OSVERSIONINFO versionInfo;
		ZeroMemory(&versionInfo, sizeof(OSVERSIONINFO));
		versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&versionInfo);
		if ((versionInfo.dwMajorVersion > 6) ||
			((versionInfo.dwMajorVersion == 6) && (versionInfo.dwMinorVersion == 3)))
			bHaveColorFonts = true;
	}
	return hr;
}


HRESULT
d2dResize(LPGW lpgw, RECT rect)
{
	HRESULT hr = S_OK;

	// do nothing if we don't have a target yet
	if (lpgw->pRenderTarget == NULL)
		return hr;

#ifndef DCRENDERER
#ifndef HAVE_D2D11
	// HwndRenderTarget
	ID2D1HwndRenderTarget * pRenderTarget = static_cast<ID2D1HwndRenderTarget *>(lpgw->pRenderTarget);
	D2D1_SIZE_U size = D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top);
	hr = pRenderTarget->Resize(size);
#else
	// DeviceContext
	lpgw->pRenderTarget->SetTarget(NULL);
	hr = lpgw->pDXGISwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	if (SUCCEEDED(hr))
		hr = d2dCreateDeviceSwapChainBitmap(lpgw);
	else
		d2dReleaseRenderTarget(lpgw);
	if (FAILED(hr))
		fprintf(stderr, "D2d: Unable to resize swap chain. hr = %0x\n", hr);
#endif
#endif

	return hr;
}


void
d2dReleaseRenderTarget(LPGW lpgw)
{
#ifndef DCRENDERER
#ifdef HAVE_D2D11
	if (lpgw->pRenderTarget != NULL)
		lpgw->pRenderTarget->SetTarget(NULL);
	SafeRelease(&(lpgw->pDXGISwapChain));
#endif
	SafeRelease(&(lpgw->pRenderTarget));
#ifdef HAVE_D2D11
	SafeRelease(&(lpgw->pDirect2dDevice));
#endif
#endif
}


void
d2dCleanup(void)
{
#ifdef DCRENDERER
	SafeRelease(&g_pRenderTarget);
#endif
	SafeRelease(&g_pDirect2dFactory);
	SafeRelease(&g_pDWriteFactory);
#ifdef HAVE_D2D11
	SafeRelease(&g_pDirect3dDevice);
	SafeRelease(&g_wicFactory);
#endif
}


/* ****************  D2D drawing functions   ************************* */


static HRESULT
d2dCreateStrokeStyle(D2D1_DASH_STYLE dashStyle, BOOL rounded, ID2D1StrokeStyle **ppStrokeStyle)
{
	const FLOAT dashstyles[4][6] = {
		{ 16.f, 8.f },	// dash
		{ 3.f, 3.f },	// dot
		{ 8.f, 5.f, 3.f, 5.f }, // dash dot
		{ 8.f, 4.f, 3.f, 4.f, 3.f, 4.f } // dash dot dot
	};
	const int dashstyle_len[4] = { 2, 2, 4, 6 };

	if (dashStyle == D2D1_DASH_STYLE_SOLID) {
		return d2dCreateStrokeStyle(dashStyle, NULL, 0, rounded, ppStrokeStyle);
	} else {
		unsigned style = (dashStyle % 5) - 1;
		return d2dCreateStrokeStyle(D2D1_DASH_STYLE_CUSTOM, dashstyles[style], dashstyle_len[style], rounded, ppStrokeStyle);
	}
}


static HRESULT
d2dCreateStrokeStyle(const FLOAT * dashes, UINT dashesCount, BOOL rounded, ID2D1StrokeStyle **ppStrokeStyle)
{
	return d2dCreateStrokeStyle(D2D1_DASH_STYLE_CUSTOM, dashes, dashesCount, rounded, ppStrokeStyle);
}


static HRESULT
d2dCreateStrokeStyle(D2D1_DASH_STYLE dashStyle, const FLOAT * dashes, UINT dashesCount, BOOL rounded, ID2D1StrokeStyle **ppStrokeStyle)
{
	if (*ppStrokeStyle != NULL) {
		if ((*ppStrokeStyle)->GetDashStyle() != dashStyle)
			SafeRelease(ppStrokeStyle);
		else
			return S_OK;
	}
	return g_pDirect2dFactory->CreateStrokeStyle(
		D2D1::StrokeStyleProperties(
			rounded ? D2D1_CAP_STYLE_ROUND : D2D1_CAP_STYLE_SQUARE,
			rounded ? D2D1_CAP_STYLE_ROUND : D2D1_CAP_STYLE_SQUARE,
			rounded ? D2D1_CAP_STYLE_ROUND : D2D1_CAP_STYLE_SQUARE,
			rounded ? D2D1_LINE_JOIN_ROUND : D2D1_LINE_JOIN_MITER,
			10.0f,
			dashStyle,
			0.0f),
		dashes, dashesCount,
		ppStrokeStyle);
}


static HRESULT
d2dPolyline(ID2D1RenderTarget * pRenderTarget, ID2D1SolidColorBrush * pBrush, ID2D1StrokeStyle * pStrokeStyle, float width, D2D1_POINT_2F *points, int polyi, bool closed)
{
	HRESULT hr = S_OK;

	bool all_vert_or_horz = true;
	for (int i = 1; i < polyi; i++)
		if (!((points[i - 1].x == points[i].x) ||
		      (points[i - 1].y == points[i].y)))
			all_vert_or_horz = false;

	// If all lines are horizontal or vertical we snap to nearest pixel
	// to avoid "blurry" lines.
	if (all_vert_or_horz) {
		for (int i = 0; i < polyi; i++) {
			points[i].x = trunc(points[i].x) + 0.5;
			points[i].y = trunc(points[i].y) + 0.5;
		}
	}

	if (SUCCEEDED(hr)) {
		if (polyi == 2) {
			pRenderTarget->DrawLine(points[0], points[1], pBrush, width, pStrokeStyle);
		} else {
			ID2D1PathGeometry *pPathGeometry = NULL;
			ID2D1GeometrySink *pSink = NULL;

			hr = g_pDirect2dFactory->CreatePathGeometry(&pPathGeometry);
			if (SUCCEEDED(hr))
				hr = pPathGeometry->Open(&pSink);

			if (SUCCEEDED(hr)) {
				pSink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_HOLLOW);
				for (int i = 1; i < polyi; i++) {
					pSink->AddLine(points[i]);
				}
				pSink->EndFigure(closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
				hr = pSink->Close();
				SafeRelease(&pSink);
			}

			if (SUCCEEDED(hr))
				pRenderTarget->DrawGeometry(pPathGeometry, pBrush, width, pStrokeStyle);

			SafeRelease(&pPathGeometry);
		}
	}
	return hr;
}


static HRESULT
d2dFilledPolygon(ID2D1RenderTarget * pRenderTarget, ID2D1Brush * pBrush, D2D1_POINT_2F *points, int polyi)
{
	HRESULT hr = S_OK;

	ID2D1PathGeometry *pPathGeometry = NULL;
	ID2D1GeometrySink *pSink = NULL;

	if (pBrush == NULL)
		return E_FAIL;

	hr = g_pDirect2dFactory->CreatePathGeometry(&pPathGeometry);
	if (SUCCEEDED(hr))
		hr = pPathGeometry->Open(&pSink);

	if (SUCCEEDED(hr)) {
		pSink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_FILLED);
		for (int i = 1; i < polyi; i++) {
			pSink->AddLine(points[i]);
		}
		pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
		hr = pSink->Close();
		SafeRelease(&pSink);
	}

	if (SUCCEEDED(hr))
		pRenderTarget->FillGeometry(pPathGeometry, pBrush);

	SafeRelease(&pPathGeometry);
	return hr;
}


static void
d2dDot(ID2D1RenderTarget * pRenderTarget, ID2D1SolidColorBrush * pBrush, float x, float y)
{
	// draw a rectangle with 1 pixel height and width
	pRenderTarget->FillRectangle(D2D1::RectF(x - 0.5f, y - 0.5f, x + 0.5f, y + 0.5f), pBrush);
}


static HRESULT
d2dMeasureText(ID2D1RenderTarget * pRenderTarget, LPCWSTR text, IDWriteTextFormat * pWriteTextFormat, D2D1_SIZE_F * size)
{
	HRESULT hr = S_OK;

	IDWriteTextLayout * pTextLayout;
	if (SUCCEEDED(hr)) {
		hr = g_pDWriteFactory->CreateTextLayout(
			text,
			static_cast<UINT32>(wcslen(text)),
			pWriteTextFormat,
			4000,
			200,
			&pTextLayout
		);
	}

	if (SUCCEEDED(hr)) {
		DWRITE_TEXT_METRICS textMetrics;
		pTextLayout->GetMetrics(&textMetrics);
		// Note: result is in DIPs
		size->width = textMetrics.widthIncludingTrailingWhitespace;
		size->height = textMetrics.height;
		SafeRelease(&pTextLayout);
	}
	return hr;
}


ID2D1BitmapBrush *
d2dCreatePatternBrush(LPGW lpgw, unsigned pattern, COLORREF color, bool transparent)
{
	HRESULT hr = S_OK;
	ID2D1RenderTarget * pMainRenderTarget = lpgw->pRenderTarget;
	ID2D1BitmapBrush * pBitmapBrush = NULL;

	// scale dash pattern with resolution
	FLOAT dpiX, dpiY;
#ifdef HAVE_D2D11
	pMainRenderTarget->GetDpi(&dpiX, &dpiY);
#else
	g_pDirect2dFactory->GetDesktopDpi(&dpiX, &dpiY);
#endif
	FLOAT scale = dpiX / 96.f;
	FLOAT size = 96.f * scale;

	if (pMainRenderTarget == NULL)
		return NULL;

	// TODO:  Would it not be more efficient to retain the target?
	// NOTE:  Using a bitmap brush is non-optimal for printing, but works if we enforce
	//        the bitmap format.
	ID2D1BitmapRenderTarget * pRenderTarget = NULL;
	if (SUCCEEDED(hr)) {
		// enforce the pixel size and the format of the bitmap
		hr = pMainRenderTarget->CreateCompatibleRenderTarget(
			D2D1::SizeF(size, size), D2D1::SizeU(size, size),
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			&pRenderTarget);
	}

	if (SUCCEEDED(hr)) {
		ID2D1SolidColorBrush * pSolidBrush = NULL;
		hr = pRenderTarget->CreateSolidColorBrush(D2DCOLORREF(color, 1.0f), &pSolidBrush);

		pRenderTarget->SetAntialiasMode(lpgw->antialiasing ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
		if (SUCCEEDED(hr)) {
			pRenderTarget->BeginDraw();
			// always clear the target
			pRenderTarget->Clear(D2DCOLORREF(lpgw->background, transparent ? 0.f : 1.f));
			switch (pattern) {
			case 0:  // empty
				break;
			case 1:  // diagonal cross
				for (float x = -size; x <= size; x += 6.f * scale)
					pRenderTarget->DrawLine(D2D1::Point2F(0.f, x), D2D1::Point2F(size, x + size), pSolidBrush);
				for (float x = 0.f; x <= 2*size; x += 6.f * scale)
					pRenderTarget->DrawLine(D2D1::Point2F(0.f, x), D2D1::Point2F(size, x - size), pSolidBrush);
				break;
			case 2:  // fine diagonal cross
				for (float x = -size; x <= size; x += 4.f * scale)
					pRenderTarget->DrawLine(D2D1::Point2F(0.f, x), D2D1::Point2F(size, x + size), pSolidBrush, 0.5f);
				for (float x = 0.f; x <= 2*size; x += 4.f * scale)
					pRenderTarget->DrawLine(D2D1::Point2F(0.f, x), D2D1::Point2F(size, x - size), pSolidBrush, 0.5f);
				break;
			case 3:  // solid
				pRenderTarget->FillRectangle(D2D1::RectF(0.f, 0.f, size, size), pSolidBrush);
				break;
			case 4:  // forward diagonals
				for (float x = -size; x <= size; x += 6.f * scale)
					pRenderTarget->DrawLine(D2D1::Point2F(0.f, x), D2D1::Point2F(size, x + size), pSolidBrush);
				break;
			case 5:  // backward diagonals
				for (float x = 0.f; x <= 2 * size; x += 6.f * scale)
					pRenderTarget->DrawLine(D2D1::Point2F(0.f, x), D2D1::Point2F(size, x - size), pSolidBrush);
				break;
			case 6:  // step forward diagonals
				for (float x = -size - size / 2.f; x <= size + size / 2.f; x += 4.f * scale)
					pRenderTarget->DrawLine(D2D1::Point2F(0.f, x), D2D1::Point2F(size, x + size + size / 2.f), pSolidBrush);
				break;
			case 7: // step backward diagonals
				for (float x = 0.f; x <= 2 * size + size / 2.f; x += 4.f * scale)
					pRenderTarget->DrawLine(D2D1::Point2F(0.f, x), D2D1::Point2F(size, x - size - size / 2.f), pSolidBrush);
				break;
			default:
				// should not happen: ignore
				break;
			}
			hr = pRenderTarget->EndDraw();

			ID2D1Bitmap * pBitmap = NULL;
			if (SUCCEEDED(hr))
				hr = pRenderTarget->GetBitmap(&pBitmap);

			if (SUCCEEDED(hr)) {
				D2D1_BITMAP_BRUSH_PROPERTIES brushProperties =
					D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP);

				hr = pMainRenderTarget->CreateBitmapBrush(pBitmap, brushProperties, &pBitmapBrush);
			}
			SafeRelease(&pBitmap);
		}
		SafeRelease(&pSolidBrush);
	}
	SafeRelease(&pRenderTarget);

	return pBitmapBrush;
}


static HRESULT
d2dSetFont(ID2D1RenderTarget * pRenderTarget, LPRECT rect, LPGW lpgw, LPTSTR fontname, int size, IDWriteTextFormat ** ppWriteTextFormat)
{
	HRESULT hr = S_OK;

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
	LPTSTR italic, bold;
	if ((italic = _tcsstr(fontname, TEXT(" Italic"))) == NULL)
		italic = _tcsstr(fontname, TEXT(":Italic"));
	if ((bold = _tcsstr(fontname, TEXT(" Bold"))) == NULL)
		bold = _tcsstr(fontname, TEXT(":Bold"));
	if (italic) *italic = 0;
	if (bold) *bold = 0;

	// NOTE: We implement our own fall-back
	if (_tcscmp(fontname, TEXT("Times")) == 0) {
		free(fontname);
		fontname = _tcsdup(TEXT("Times New Roman"));
	}
#ifdef UNICODE
	LPWSTR family = fontname;
#else
	LPWSTR family = UnicodeText(fontname, lpgw->encoding);
#endif

	if (*ppWriteTextFormat)
		SafeRelease(ppWriteTextFormat);

	// Take into account differences between the render target's reported
	// DPI setting and the system DPI.
	FLOAT fontSize = size * lpgw->dpi / 72.f;
	hr = g_pDWriteFactory->CreateTextFormat(
		family,
		NULL,
		bold != NULL ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
		italic != NULL ? DWRITE_FONT_STYLE_ITALIC: DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		fontSize,
		L"", //locale
		ppWriteTextFormat
	);

	IDWriteTextLayout * pTextLayout = NULL;
	if (SUCCEEDED(hr)) {
		(*ppWriteTextFormat)->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		(*ppWriteTextFormat)->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

		hr = g_pDWriteFactory->CreateTextLayout(
			L"0123456789",
			static_cast<UINT32>(10),
			*ppWriteTextFormat,
			4000,
			200,
			&pTextLayout
		);
	}

	// Deduce font metrics from text layout
	if (SUCCEEDED(hr)) {
		DWRITE_LINE_METRICS lineMetrics;
		UINT32 count;
		hr = pTextLayout->GetLineMetrics(&lineMetrics, 1, &count);

		if (SUCCEEDED(hr)) {
			// Note:  values are in DIPs
			lpgw->tmAscent = static_cast<FLOAT>(lineMetrics.baseline);
			lpgw->tmDescent = static_cast<FLOAT>(lineMetrics.height - lineMetrics.baseline);
			lpgw->tmHeight = static_cast<FLOAT>(lineMetrics.height);
		}
	}

	// Determine vchar, hchar, vtic, htic from text layout
	if (SUCCEEDED(hr)) {
		DWRITE_TEXT_METRICS textMetrics;
		hr = pTextLayout->GetMetrics(&textMetrics);

		if (SUCCEEDED(hr)) {
			FLOAT width = textMetrics.widthIncludingTrailingWhitespace;
			//FLOAT height = textMetrics.height * dpiY / 96.f;

			lpgw->vchar = MulDiv(lpgw->tmHeight, lpgw->ymax, rect->bottom - rect->top);
			lpgw->hchar = MulDiv(width, lpgw->xmax, 10 * (rect->right - rect->left));
			lpgw->htic = MulDiv(lpgw->hchar, 2, 5);
			unsigned cy = MulDiv(width, 2, 50);
			lpgw->vtic = MulDiv(cy, lpgw->ymax, rect->bottom - rect->top);
		}
	}

	// Only vector fonts allowed.
	lpgw->rotate = TRUE;

	SafeRelease(&pTextLayout);
	free(fontname);
	return hr;
}


void
InitFont_d2d(LPGW lpgw, HDC hdc, LPRECT rect)
{
	HRESULT hr = d2dInit(lpgw);
	if (SUCCEEDED(hr)) {
#ifdef DCRENDERER
		hr = g_pRenderTarget->BindDC(hdc, rect);
#endif
		IDWriteTextFormat * pWriteTextFormat = NULL;  // must initialize
		d2dSetFont(lpgw->pRenderTarget, rect, lpgw, lpgw->fontname, lpgw->fontsize, &pWriteTextFormat);
		SafeRelease(&pWriteTextFormat);
	}
}


static void
EnhancedSetFont()
{
	if ((enhstate.lpgw->fontsize != enhstate.fontsize) ||
	    (_tcscmp(enhstate.lpgw->fontname, enhstate.fontname) != 0)) {
		d2dSetFont(enhstate_d2d.pRenderTarget, enhstate.rect, enhstate.lpgw, enhstate.fontname, enhstate.fontsize, &enhstate_d2d.pWriteTextFormat);
	}
}


static unsigned
EnhancedTextLength(char * text)
{
	LPWSTR textw = UnicodeTextWithEscapes(enhanced_text, enhstate.lpgw->encoding);
	D2D1_SIZE_F size;
	d2dMeasureText(enhstate_d2d.pRenderTarget, textw, enhstate_d2d.pWriteTextFormat, &size);
	free(textw);
	return ceil(size.width);
}


static void
EnhancedPutText(int x, int y, char * text)
{
	ID2D1RenderTarget * pRenderTarget = enhstate_d2d.pRenderTarget;
	LPWSTR textw = UnicodeTextWithEscapes(text, enhstate.lpgw->encoding);

	if (enhstate.lpgw->angle != 0)
		pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(-enhstate.lpgw->angle, D2D1::Point2F(x, y)));
	const float align_ofs = 0.;
	D2D1_DRAW_TEXT_OPTIONS options = D2D1_DRAW_TEXT_OPTIONS_NONE;
	if (bHaveColorFonts)
		options = D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT;
	pRenderTarget->DrawText(textw, static_cast<UINT32>(wcslen(textw)), enhstate_d2d.pWriteTextFormat,
							D2D1::RectF(x + align_ofs, y - enhstate.lpgw->tmAscent,
										x + align_ofs + textbox_width, 8888),
							enhstate_d2d.pBrush, options);
	if (enhstate.lpgw->angle != 0)
		pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

	free(textw);
}


static void
EnhancedCleanup()
{
	SafeRelease(&enhstate_d2d.pWriteTextFormat);
}


static void
draw_enhanced_init(LPGW lpgw, ID2D1RenderTarget * pRenderTarget, ID2D1SolidColorBrush *pBrush, LPRECT rect)
{
	enhstate.set_font = &EnhancedSetFont;
	enhstate.text_length = &EnhancedTextLength;
	enhstate.put_text = &EnhancedPutText;
	enhstate.cleanup = &EnhancedCleanup;

	IDWriteTextFormat * pWriteTextFormat = NULL;  // must initialize
	d2dSetFont(pRenderTarget, rect, lpgw, lpgw->fontname, lpgw->fontsize, &pWriteTextFormat);

	enhstate_d2d.pRenderTarget = pRenderTarget;
	enhstate_d2d.pBrush = pBrush;
	enhstate_d2d.pWriteTextFormat = pWriteTextFormat;

	// We cannot use pRenderTarget->GetDpi() since that might be set to 96;
	enhstate.res_scale = lpgw->dpi / 96.f;
}


static inline void
d2d_update_keybox(LPGW lpgw, ID2D1RenderTarget * pRenderTarget, unsigned plotno, FLOAT x, FLOAT y)
{
	draw_update_keybox(lpgw, plotno, x + 0.5f, y + 0.5f);
}


static inline D2D1::ColorF
D2DCOLORREF(COLORREF c, float a)
{
	return D2D1::ColorF(GetRValue(c) / 255.f, GetGValue(c) / 255.f, GetBValue(c) / 255.f, a);
}


void
#ifdef DCRENDERER
drawgraph_d2d(LPGW lpgw, HDC hdc, LPRECT rect)
#else
drawgraph_d2d(LPGW lpgw, HWND hwnd, LPRECT rect)
#endif
{
	HRESULT hr = S_OK;

	// create the D2D factory object and the render target
	hr = d2dInit(lpgw);
	if (FAILED(hr))
		return;

#ifdef DCRENDERER
	ID2D1DCRenderTarget * pRenderTarget = g_pRenderTarget;
	// new device context or size changed
	hr = pRenderTarget->BindDC(hdc, rect);
#else
#ifndef HAVE_D2D11
	ID2D1HwndRenderTarget * pRenderTarget = static_cast<ID2D1HwndRenderTarget *>(lpgw->pRenderTarget);
	// No need to draw to an occluded window
	if (pRenderTarget->CheckWindowState() == D2D1_WINDOW_STATE_OCCLUDED)
		return;
#else
	ID2D1DeviceContext * pRenderTarget = lpgw->pRenderTarget;
#endif
#endif

	// Get screen DPI
	FLOAT dpiX, dpiY;
	g_pDirect2dFactory->GetDesktopDpi(&dpiX, &dpiY);
	lpgw->dpi = dpiY;

	hr = d2d_do_draw(lpgw, pRenderTarget, rect, true);
}


#if defined(HAVE_D2D11) && !defined(DCRENDERER)

// Creates a print job ticket stream to define options for the next print job.
// Note: This is derived from an MSDN example
HRESULT
GetPrintTicketFromDevmode(PCTSTR printerName, PDEVMODE pDevMode, WORD devModesize, LPSTREAM * pPrintTicketStream)
{
	HRESULT hr = S_OK;
	HPTPROVIDER provider = NULL;

	*pPrintTicketStream = NULL;

	hr = CreateStreamOnHGlobal(NULL, TRUE, pPrintTicketStream);
	if (SUCCEEDED(hr)) {
		hr = PTOpenProvider(printerName, 1, &provider);
	}
	if (SUCCEEDED(hr)) {
		hr = PTConvertDevModeToPrintTicket(provider, devModesize, pDevMode, kPTJobScope, *pPrintTicketStream);
	}
	if (FAILED(hr) && pPrintTicketStream != NULL) {
		SafeRelease(pPrintTicketStream);
	}
	if (provider)
		PTCloseProvider(provider);

	return hr;
}


HRESULT
print_d2d(LPGW lpgw, DEVMODE * pDevMode, LPCTSTR szDevice, LPRECT rect)
{
	HRESULT hr = S_OK;
	ID2D1DeviceContext * pRenderTarget = lpgw->pRenderTarget;
	IStream * jobPrintTicketStream;
	IPrintDocumentPackageTarget * documentTarget;

	// Create a print ticket, init print subsystem
	if (SUCCEEDED(hr)) {
		hr = GetPrintTicketFromDevmode(szDevice, pDevMode,
				pDevMode->dmSize + pDevMode->dmDriverExtra, // size including private data
				&jobPrintTicketStream);
	}
	IPrintDocumentPackageTargetFactory * documentTargetFactory = NULL;
	if (SUCCEEDED(hr)) {
		hr = CoCreateInstance(__uuidof(PrintDocumentPackageTargetFactory),
				NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&documentTargetFactory));
	}
	if (SUCCEEDED(hr)) {
		hr = documentTargetFactory->CreateDocumentPackageTargetForPrintJob(
				szDevice, lpgw->Title,
				NULL, jobPrintTicketStream,
				&documentTarget);
	}

	// Make sure that the D2D factory object and the render target are created
	if (SUCCEEDED(hr)) {
		hr = d2dInit(lpgw);
	}

	// Create a command list to save commands to
	ID2D1CommandList * pCommandList;
	if (SUCCEEDED(hr)) {
		hr = pRenderTarget->CreateCommandList(&pCommandList);
	}

	// "Draw" to the command list
	if (SUCCEEDED(hr)) {
		ID2D1Image* originalTarget;
		pRenderTarget->GetTarget(&originalTarget);
		pRenderTarget->SetTarget(pCommandList);

		// temporarily turn anti-aliasing off
		BOOL save_aa = lpgw->antialiasing;
		lpgw->antialiasing = FALSE;

		// Pixel mode not supported for printing
		unsigned save_dpi = lpgw->dpi;
		lpgw->dpi = 96;
		pRenderTarget->SetUnitMode(D2D1_UNIT_MODE_DIPS);

		hr = d2d_do_draw(lpgw, pRenderTarget, rect, false);

		pRenderTarget->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
		lpgw->dpi = save_dpi;
		lpgw->antialiasing = save_aa;

		pRenderTarget->SetTarget(originalTarget);
		originalTarget->Release();
	}

	// Finalize the command list
	if (SUCCEEDED(hr)) {
		hr = pCommandList->Close();
	}

	// Create a WIC factory if it does not exist yet.
	if (SUCCEEDED(hr) && g_wicFactory == NULL) {
		// Create a WIC factory.
		hr = CoCreateInstance(CLSID_WICImagingFactory,
			NULL, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&g_wicFactory));
	}

	// Finally create the print control.  This is can be used for a single print job only.
	ID2D1PrintControl* printControl;
	if (SUCCEEDED(hr)) {
		D2D1_PRINT_CONTROL_PROPERTIES printControlProperties;
		printControlProperties.rasterDPI = 150.f;
		printControlProperties.fontSubset = D2D1_PRINT_FONT_SUBSET_MODE_DEFAULT;
		printControlProperties.colorSpace = D2D1_COLOR_SPACE_SRGB;
		hr = lpgw->pDirect2dDevice->CreatePrintControl(
			g_wicFactory,
			documentTarget,
			&printControlProperties,
			&printControl
		);
	}

	// Add a single page with the graph to the print control.
	if (SUCCEEDED(hr)) {
		FLOAT width, height;

		// special case document conversion
		if ((_tcscmp(TEXT("Microsoft Print to PDF"), szDevice) == 0) ||
		   (_tcscmp(TEXT("Microsoft XPS Document Writer"), szDevice) == 0)) {
			width = rect->right - rect->left;
			height = rect->bottom - rect->top;
		} else {
			if ((pDevMode->dmFields & DM_PAPERLENGTH) && (pDevMode->dmFields & DM_PAPERWIDTH)) {
				// paper size is given in 0.1 mm, convert to DIPS
				width = pDevMode->dmPaperWidth / 254.f * 96.f;
				height = pDevMode->dmPaperLength / 254.f * 96.f;
			} else {
				// TODO: do we need to check dmPaperSize?
				// default to A4
				width = 210.f / 25.4f * 96.f;
				height = 297.f / 25.4f * 96.f;
			}
		}

		hr = printControl->AddPage(pCommandList, D2D1::SizeF(width, height), NULL);
	}

	// Done.
	if (SUCCEEDED(hr)) {
		hr = printControl->Close();
	}

	// Clean-up
	SafeRelease(&pCommandList);
	SafeRelease(&printControl);
	SafeRelease(&jobPrintTicketStream);
	SafeRelease(&documentTargetFactory);

	if (hr == D2DERR_RECREATE_TARGET) {
		hr = S_OK;
		// discard device resources
		d2dReleaseRenderTarget(lpgw);
	}

	return hr;
}
#endif


static HRESULT
d2d_do_draw(LPGW lpgw, ID2D1RenderTarget * pRenderTarget, LPRECT rect, bool interactive)
{
	// COM return code
	HRESULT hr = S_OK;
	FLOAT dpiX, dpiY;

	/* draw ops */
	unsigned int ngwop = 0;
	struct GWOP *curptr;
	struct GWOPBLK *blkptr;

	/* layers and hypertext */
	unsigned plotno = 0;
	bool gridline = false;
	bool skipplot = false;
	bool keysample = false;
	LPWSTR hypertext = NULL;
	int hypertype = 0;

	/* colors */
	bool isColor = true;		/* use colors? */
	COLORREF last_color = 0;	/* currently selected color */
	double alpha_c = 1.;		/* alpha for transparency */
	ID2D1SolidColorBrush * pSolidBrush = NULL;

	/* text */
	float align_ofs = 0.f;

	/* lines */
	double line_width = lpgw->linewidth;	/* current line width */
	double lw_scale = 1.;
	LOGPEN cur_penstruct;		/* current pen settings */
	ID2D1StrokeStyle * pSolidStrokeStyle = NULL;
	ID2D1StrokeStyle * pStrokeStyle = NULL;

	/* polylines and polygons */
	int polymax = 200;			/* size of ppt */
	int polyi = 0;				/* number of points in ppt */
	D2D1_POINT_2F * ppt;		/* storage of polyline/polygon-points */
	int last_polyi = 0;			/* number of points in last_poly */
	D2D1_POINT_2F * last_poly = NULL;	/* storage of last filled polygon */
	unsigned int lastop = -1;	/* used for plotting last point on a line */

	/* filled polygons and boxes */
	int last_fillstyle = -1;
	COLORREF last_fillcolor = 0;
	double last_fill_alpha = 1.;
	bool transparent = false;	/* transparent fill? */
	ID2D1Brush * pFillBrush = NULL;
	ID2D1SolidColorBrush * pSolidFillBrush = NULL;
	ID2D1BitmapBrush * pPatternFillBrush = NULL;
	ID2D1SolidColorBrush * pGrayBrush = NULL;
	ID2D1BitmapRenderTarget * pPolygonRenderTarget = NULL;

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
#if 0
	CachedBitmap *cb = NULL;
	POINT cb_ofs;
#endif
	bool ps_caching = false;

	/* coordinates and lengths */
	float xdash, ydash;			/* the transformed coordinates */
	int rr, rl, rt, rb;			/* coordinates of drawing area */
	int htic, vtic;				/* tic sizes */
	float hshift, vshift;		/* correction of text position */

	/* indices */
	int seq = 0;				/* sequence counter for W_image and W_boxedtext */

	// check lock
	if (lpgw->locked)
		return hr;

	if (interactive)
		clear_tooltips(lpgw);

	rr = rect->right;
	rl = rect->left;
	rt = rect->top;
	rb = rect->bottom;

	// TODO: not implemented yet
	ps_caching = false;

	if (lpgw->antialiasing)
		pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	else
		pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

	// Note that this will always be 96 for a DC Render Target.
	pRenderTarget->GetDpi(&dpiX, &dpiY);
	//printf("dpiX = %.1f, DPI = %u\n", dpiX, GetDPI());

	/* background fill */
	pRenderTarget->BeginDraw();
	if (isColor)
		pRenderTarget->Clear(D2DCOLORREF(lpgw->background, 1.f));
	else
		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	// init brushes
	hr = pRenderTarget->CreateSolidColorBrush(D2DCOLORREF(lpgw->background, 1.), &pSolidBrush);
	hr = pRenderTarget->CreateSolidColorBrush(D2DCOLORREF(lpgw->background, 1.), &pSolidFillBrush);
	hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.75f, 0.75f, 0.75f, 0.5f), &pGrayBrush);
	cur_penstruct = (lpgw->color && isColor) ?  lpgw->colorpen[2] : lpgw->monopen[2];
	last_color = cur_penstruct.lopnColor;

	d2dCreateStrokeStyle(D2D1_DASH_STYLE_SOLID, lpgw->rounded, &pStrokeStyle);
	d2dCreateStrokeStyle(D2D1_DASH_STYLE_SOLID, lpgw->rounded, &pSolidStrokeStyle);

	ppt = (D2D1_POINT_2F *) malloc((polymax + 1) * sizeof(D2D1_POINT_2F));

	htic = (lpgw->org_pointsize * MulDiv(lpgw->htic, rr - rl, lpgw->xmax) + 1);
	vtic = (lpgw->org_pointsize * MulDiv(lpgw->vtic, rb - rt, lpgw->ymax) + 1);

	lpgw->angle = 0;
	lpgw->justify = LEFT;

	// Create a DirectWrite text format object.
	IDWriteTextFormat * pWriteTextFormat = NULL;
	hr = d2dSetFont(pRenderTarget, rect, lpgw, NULL, 0, &pWriteTextFormat);

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
			return hr;
		curptr = (struct GWOP *)blkptr->gwop;
	}
	if (curptr == NULL)
		return hr;

	while (ngwop < lpgw->nGWOP) {
		// transform the coordinates
		if (lpgw->oversample) {
			xdash = float(curptr->x) * (rr - rl - 1) / float(lpgw->xmax) + 0.5;
			ydash = float(rb) - float(curptr->y) * (rb - rt - 1) / float(lpgw->ymax) + rt - 0.5;
		} else {
			xdash = MulDiv(curptr->x, rr - rl - 1, lpgw->xmax) + rl + 0.5;
			ydash = rb - MulDiv(curptr->y, rb - rt - 1, lpgw->ymax) + rt - 0.5;
		}

		/* finish last filled polygon */
		if ((last_poly != NULL) &&
			(((lastop == W_filled_polygon_draw) && (curptr->op != W_fillstyle)) ||
			 ((curptr->op == W_fillstyle) && (curptr->x != unsigned(last_fillstyle))))) {
			D2D1_ANTIALIAS_MODE mode = pRenderTarget->GetAntialiasMode();
			if (lpgw->antialiasing && !lpgw->polyaa)
				pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
			if (pFillBrush != NULL) {
				if (pPolygonRenderTarget == NULL)
					hr = d2dFilledPolygon(pRenderTarget, pFillBrush, last_poly, last_polyi);
				else
					hr = d2dFilledPolygon(pPolygonRenderTarget, pFillBrush, last_poly, last_polyi);
			}
			pRenderTarget->SetAntialiasMode(mode);
			last_polyi = 0;
			free(last_poly);
			last_poly = NULL;
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
						LPRECT bb = lpgw->keyboxes + plotno - 1;
						D2D1_RECT_F rect = D2D1::RectF(bb->left, bb->top, bb->right, bb->bottom);
						pRenderTarget->FillRectangle(rect, pGrayBrush);
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
						D2D1_SIZE_F size = D2D1::SizeF(2 * (rr - rl), 2 * (rb - rt));
						D2D1_SIZE_U pixelSize = D2D1::SizeU(2 * (rr - rl), 2 * (rb - rt));
						if (SUCCEEDED(hr))
							hr = pRenderTarget->CreateCompatibleRenderTarget(size, pixelSize, &pPolygonRenderTarget);

						if (SUCCEEDED(hr)) {
							pPolygonRenderTarget->BeginDraw();
							pPolygonRenderTarget->Clear(D2D1::ColorF(1.0, 1.0, 1.0, 0.0));
							pPolygonRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
							pPolygonRenderTarget->SetTransform(D2D1::Matrix3x2F::Scale(2.f, 2.f, D2D1::Point2F(0, 0)));
						}
					}
					break;
				case TERM_LAYER_END_PM3D_MAP:
				case TERM_LAYER_END_PM3D_FLUSH:
					if (pPolygonRenderTarget != NULL) {
						ID2D1Bitmap * pBitmap;
						if (SUCCEEDED(hr))
							hr = pPolygonRenderTarget->EndDraw();
						if (SUCCEEDED(hr))
							hr = pPolygonRenderTarget->GetBitmap(&pBitmap);
						if (SUCCEEDED(hr)) {
							pRenderTarget->DrawBitmap(
								pBitmap,
								D2D1::RectF(rl, rt, rr, rb),
								1.0f,
								D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
								D2D1::RectF(0, 0, 2 * (rr - rl), 2 * (rb - rt))
							);
							SafeRelease(&pBitmap);
						}
						SafeRelease(&pPolygonRenderTarget);
					}
					break;
				case TERM_LAYER_BEGIN_IMAGE:
				case TERM_LAYER_BEGIN_COLORBOX:
					// Turn of antialiasing for failsafe/pixel images and color boxes
					// and pm3d polygons to avoid seams.
					if (lpgw->antialiasing)
						pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
					break;
				case TERM_LAYER_END_IMAGE:
				case TERM_LAYER_END_COLORBOX:
					if (lpgw->antialiasing)
						pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
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
			if ((curptr->op >= W_dot) && (curptr->op <= W_last_pointtype)) {
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
			POINTL * poly = reinterpret_cast<POINTL *>(LocalLock(curptr->htext));
			if (poly == NULL) break; // memory allocation failed
			polyi = curptr->x;
			// FIXME: Test for zero-length segments
			if (polyi == 2 && poly[0].x == poly[1].x && poly[0].y == poly[1].y) {
				LocalUnlock(poly);
				break;
			}
			// FIXME: Test for double polylines (same start and end point)
			if (polyi == 3 && poly[0].x == poly[2].x && poly[0].y == poly[2].y) {
				polyi = 2;
			}
			D2D1_POINT_2F * points = new D2D1_POINT_2F[polyi];
			for (int i = 0; i < polyi; i++) {
				// transform the coordinates
				if (lpgw->oversample) {
					points[i].x = float(poly[i].x) * (rr - rl - 1) / float(lpgw->xmax) + rl + 0.5;
					points[i].y = float(rb) - float(poly[i].y) * (rb - rt - 1) / float(lpgw->ymax) + rt - 0.5;
				} else {
					points[i].x = MulDiv(poly[i].x, rr - rl - 1, lpgw->xmax) + rl + 0.5;
					points[i].y = rb - MulDiv(poly[i].y, rb - rt - 1, lpgw->ymax) + rt - 0.5;
				}
			}
			LocalUnlock(poly);

			if (pPolygonRenderTarget == NULL)
				hr = d2dPolyline(pRenderTarget, pSolidBrush, pStrokeStyle, line_width, points, polyi);
			else
				hr = d2dPolyline(pPolygonRenderTarget, pSolidBrush, pStrokeStyle, line_width, points, polyi);
			if (keysample) {
				d2d_update_keybox(lpgw, pRenderTarget, plotno, points[0].x, points[0].y);
				d2d_update_keybox(lpgw, pRenderTarget, plotno, points[polyi - 1].x, points[polyi - 1].y);
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

			pSolidBrush->SetColor(D2DCOLORREF(cur_penstruct.lopnColor, 1.));

			if (cur_penstruct.lopnStyle <= PS_DASHDOTDOT)
				// cast is safe since GDI, GDI+ and Direct2D use the same numbers
				hr = d2dCreateStrokeStyle(static_cast<D2D1_DASH_STYLE>(cur_penstruct.lopnStyle), lpgw->rounded, &pStrokeStyle);
			else
				hr = d2dCreateStrokeStyle(D2D1_DASH_STYLE_SOLID, lpgw->rounded, &pStrokeStyle);

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
				hr = d2dCreateStrokeStyle(static_cast<D2D1_DASH_STYLE>(cur_penstruct.lopnStyle), lpgw->rounded, &pStrokeStyle);
			} else if (dt == DASHTYPE_SOLID) {
				cur_penstruct.lopnStyle = PS_SOLID;
				hr = d2dCreateStrokeStyle(static_cast<D2D1_DASH_STYLE>(cur_penstruct.lopnStyle), lpgw->rounded, &pStrokeStyle);
			} else if (dt == DASHTYPE_AXIS) {
				dt = 1;
				cur_penstruct.lopnStyle =
					lpgw->dashed ? lpgw->monopen[dt].lopnStyle : lpgw->colorpen[dt].lopnStyle;
				hr = d2dCreateStrokeStyle(static_cast<D2D1_DASH_STYLE>(cur_penstruct.lopnStyle), lpgw->rounded, &pStrokeStyle);
			} else if (dt == DASHTYPE_CUSTOM) {
				t_dashtype * dash = reinterpret_cast<t_dashtype *>(LocalLock(curptr->htext));
				if (dash == NULL) break;
				INT count = 0;
				while ((dash->pattern[count] != 0.) && (count < DASHPATTERN_LENGTH)) count++;
				hr = d2dCreateStrokeStyle(dash->pattern, count, lpgw->rounded, &pStrokeStyle);
				LocalUnlock(curptr->htext);
			}
			break;
		}

		case W_text_encoding:
			lpgw->encoding = static_cast<set_encoding_id>(curptr->x);
			break;

		case W_put_text: {
			char * str;
			str = reinterpret_cast<char *>(LocalLock(curptr->htext));
			if (str) {
				LPWSTR textw = UnicodeText(str, lpgw->encoding);
				if (textw) {
					if (lpgw->angle != 0)
						pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(-lpgw->angle, D2D1::Point2F(xdash + hshift, ydash + vshift)));
					D2D1_DRAW_TEXT_OPTIONS options = D2D1_DRAW_TEXT_OPTIONS_NONE;
					if (bHaveColorFonts)
						options = D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT;
					pRenderTarget->DrawText(textw, static_cast<UINT32>(wcslen(textw)), pWriteTextFormat,
											D2D1::RectF(xdash + hshift + align_ofs, ydash + vshift,
														xdash + hshift + align_ofs + textbox_width, 8888),
											pSolidBrush, options);
					if (lpgw->angle != 0)
						pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

					D2D1_SIZE_F size;
					int dxl, dxr;

#ifndef EAM_BOXED_TEXT
					if (keysample) {
#else
					if (keysample || boxedtext.boxing) {
#endif
						d2dMeasureText(pRenderTarget, textw, pWriteTextFormat, &size);
						size.height = lpgw->tmHeight;
						if (lpgw->justify == LEFT) {
							dxl = 0;
							dxr = size.width + 0.5;
						} else if (lpgw->justify == CENTRE) {
							dxl = dxr = size.width / 2;
						} else {
							dxl = size.width + 0.5;
							dxr = 0;
						}
					}
					if (keysample) {
						d2d_update_keybox(lpgw, pRenderTarget, plotno, xdash - dxl, ydash - size.height / 2);
						d2d_update_keybox(lpgw, pRenderTarget, plotno, xdash + dxr, ydash + size.height / 2);
					}
#ifdef EAM_BOXED_TEXT
					if (boxedtext.boxing) {
						if (boxedtext.box.left > (xdash - boxedtext.start.x - dxl))
							boxedtext.box.left = xdash - boxedtext.start.x - dxl;
						if (boxedtext.box.right < (xdash - boxedtext.start.x + dxr))
							boxedtext.box.right = xdash - boxedtext.start.x + dxr;
						if (boxedtext.box.top > (ydash - boxedtext.start.y - size.height / 2))
							boxedtext.box.top = ydash - boxedtext.start.y - size.height / 2;
						if (boxedtext.box.bottom < (ydash - boxedtext.start.y + size.height / 2))
							boxedtext.box.bottom = ydash - boxedtext.start.y + size.height / 2;
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
			char * str = reinterpret_cast<char *>(LocalLock(curptr->htext));
			if (str) {
				RECT extend;
				draw_enhanced_init(lpgw, pRenderTarget, pSolidBrush, rect);
				draw_enhanced_text(lpgw, rect, xdash, ydash, str);
				draw_get_enhanced_text_extend(&extend);

				if (keysample) {
					d2d_update_keybox(lpgw, pRenderTarget, plotno, xdash - extend.left, ydash - extend.top);
					d2d_update_keybox(lpgw, pRenderTarget, plotno, xdash + extend.right, ydash + extend.bottom);
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
				char * str = reinterpret_cast<char *>(LocalLock(curptr->htext));
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
					D2D1_RECT_F rect;

					switch (boxedtext.angle) {
					case 0:
						rect.left   = + boxedtext.box.left   - dx;
						rect.right  = + boxedtext.box.right  + dx;
						rect.top    = + boxedtext.box.top    - dy;
						rect.bottom = + boxedtext.box.bottom + dy;
						break;
					case 90:
						rect.left   = + boxedtext.box.top    - dy;
						rect.right  = + boxedtext.box.bottom + dy;
						rect.top    = - boxedtext.box.right  - dx;
						rect.bottom = - boxedtext.box.left   + dx;
						break;
					case 180:
						rect.left   = - boxedtext.box.right  - dx;
						rect.right  = - boxedtext.box.left   + dx;
						rect.top    = - boxedtext.box.bottom - dy;
						rect.bottom = - boxedtext.box.top    + dy;
						break;
					case 270:
						rect.left   = - boxedtext.box.bottom - dy;
						rect.right  = - boxedtext.box.top    + dy;
						rect.top    = + boxedtext.box.left   - dx;
						rect.bottom = + boxedtext.box.right  + dx;
						break;
					}
					// snap to grid
					rect.left   += trunc(boxedtext.start.x) + 0.5;
					rect.right  += trunc(boxedtext.start.x) + 0.5;
					rect.top    += trunc(boxedtext.start.y) + 0.5;
					rect.bottom += trunc(boxedtext.start.y) + 0.5;
					if (boxedtext.option == TEXTBOX_OUTLINE) {
						pRenderTarget->DrawRectangle(rect, pSolidBrush, line_width, pStrokeStyle);
					} else {
						/* Fill bounding box with current color. */
						pRenderTarget->FillRectangle(rect, pSolidBrush);
					}
				} else {
					float theta = boxedtext.angle * M_PI / 180.;
					float sin_theta = sin(theta);
					float cos_theta = cos(theta);
					D2D1_POINT_2F rect[4];

					rect[0].x =  (boxedtext.box.left   - dx) * cos_theta +
								 (boxedtext.box.top    - dy) * sin_theta;
					rect[0].y = -(boxedtext.box.left   - dx) * sin_theta +
								 (boxedtext.box.top    - dy) * cos_theta;
					rect[1].x =  (boxedtext.box.left   - dx) * cos_theta +
								 (boxedtext.box.bottom + dy) * sin_theta;
					rect[1].y = -(boxedtext.box.left   - dx) * sin_theta +
								 (boxedtext.box.bottom + dy) * cos_theta;
					rect[2].x =  (boxedtext.box.right  + dx) * cos_theta +
								 (boxedtext.box.bottom + dy) * sin_theta;
					rect[2].y = -(boxedtext.box.right  + dx) * sin_theta +
								 (boxedtext.box.bottom + dy) * cos_theta;
					rect[3].x =  (boxedtext.box.right  + dx) * cos_theta +
								 (boxedtext.box.top    - dy) * sin_theta;
					rect[3].y = -(boxedtext.box.right  + dx) * sin_theta +
								 (boxedtext.box.top    - dy) * cos_theta;
					for (int i = 0; i < 4; i++) {
						rect[i].x += boxedtext.start.x;
						rect[i].y += boxedtext.start.y;
					}
					if (boxedtext.option == TEXTBOX_OUTLINE) {
						hr = d2dPolyline(pRenderTarget, pSolidBrush, pStrokeStyle, line_width, rect, 4, true);
					} else {
						if (pFillBrush != NULL) {
							hr = d2dFilledPolygon(pRenderTarget, pFillBrush, rect, 4);
						}
					}
				}
				boxedtext.boxing = FALSE;
				break;
			}
			case TEXTBOX_MARGINS:
				/* Adjust size of whitespace around text. */
				boxedtext.margin.x = MulDiv(curptr->x * lpgw->hchar, rr - rl, 1000 * lpgw->xmax);
				boxedtext.margin.y = MulDiv(curptr->y * lpgw->hchar, rr - rl, 1000 * lpgw->xmax);
				break;
			default:
				break;
			}
			break;
#endif

		case W_fillstyle: {
			// FIXME: resetting polyi here should not be necessary
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
					float alpha = (fillstyle >> 4) / 100.f;
					pSolidFillBrush->SetColor(D2DCOLORREF(last_color, alpha));
					pFillBrush = pSolidFillBrush;
					break;
				}
				case FS_SOLID: {
					if (alpha_c < 1.) {
						pSolidFillBrush->SetColor(D2DCOLORREF(last_color, alpha_c));
					} else if ((int)(fillstyle >> 4) == 100) {
						/* special case this common choice */
						pSolidFillBrush->SetColor(D2DCOLORREF(last_color, 1.f));
					} else {
						float density = MINMAX(0, (int)(fillstyle >> 4), 100) * 0.01;
						D2D1::ColorF color(
								1.f - density * (1.f - GetRValue(last_color) / 255.f),
								1.f - density * (1.f - GetGValue(last_color) / 255.f),
								1.f - density * (1.f - GetBValue(last_color) / 255.f),
								1.f
						);
						pSolidFillBrush->SetColor(color);
					}
					pFillBrush = pSolidFillBrush;
					break;
				}
				case FS_TRANSPARENT_PATTERN:
					transparent = true;
					/* intentionally fall through */
				case FS_PATTERN: {
					/* style == 2 --> use fill pattern according to
							 * fillpattern. Pattern number is enumerated */
					int pattern = GPMAX(fillstyle >> 4, 0) % pattern_num;
					SafeRelease(&pPatternFillBrush);
					pFillBrush = pPatternFillBrush = d2dCreatePatternBrush(lpgw, pattern, last_color, transparent);
					break;
				}
				case FS_EMPTY:
					/* FIXME: Instead of filling with background color, we should not fill at all in this case! */
					/* fill with background color */
					pSolidFillBrush->SetColor(D2DCOLORREF(lpgw->background, 1.));
					pFillBrush = pSolidFillBrush;
					break;
				case FS_DEFAULT:
				default:
					/* Leave the current brush and color in place */
					pSolidFillBrush->SetColor(D2DCOLORREF(last_color, 1.));
					pFillBrush = pSolidFillBrush;
					break;
			}
			last_fillstyle = fillstyle;
			last_fillcolor = last_color;
			last_fill_alpha = alpha_c;
			break;
		}

		case W_move:
			// This is used for filled boxes only.
			ppt[0].x = xdash;
			ppt[0].y = ydash;
			break;

		case W_boxfill: {
			/* NOTE: the x and y passed with this call are the coordinates of the
			 * lower right corner of the box. The upper left corner was stored into
			 * ppt[0] by a preceding W_move, and the style was set
			 * by a W_fillstyle call. */
			// snap to pixel grid
			D2D1_RECT_F rect;
			rect.left = trunc(xdash);
			rect.top = trunc(ydash);
			rect.right = trunc(ppt[0].x);
			rect.bottom = trunc(ppt[0].y);

			D2D1_ANTIALIAS_MODE mode = pRenderTarget->GetAntialiasMode();
			pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
			if (pFillBrush != NULL) {
				pRenderTarget->FillRectangle(rect, pFillBrush);
			}
			pRenderTarget->SetAntialiasMode(mode);

			if (keysample)
				d2d_update_keybox(lpgw, pRenderTarget, plotno, xdash + 1, ydash);
			polyi = 0;
			break;
		}

		case W_text_angle:
			if (lpgw->angle != (int)curptr->x) {
				lpgw->angle = (int)curptr->x;
				/* recalculate shifting of rotated text */
				hshift = - sin(M_PI / 180. * lpgw->angle) * lpgw->tmHeight / 2.;
				vshift = - cos(M_PI / 180. * lpgw->angle) * lpgw->tmHeight / 2.;
			}
			break;

		case W_justify:
			switch (curptr->x) {
				case LEFT:
					pWriteTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
					align_ofs = 0.f;
					break;
				case RIGHT:
					pWriteTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
					align_ofs = -textbox_width;
					break;
				case CENTRE:
					pWriteTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
					align_ofs = -textbox_width / 2.f;
					break;
			}
			lpgw->justify = curptr->x;
			break;

		case W_font: {
			int size = curptr->x;
			char * fontname = reinterpret_cast<char *>(LocalLock(curptr->htext));
#ifdef UNICODE
			/* FIXME: Maybe this should be in win.trm instead. */
			LPWSTR wfontname = UnicodeText(fontname, lpgw->encoding);
			hr = d2dSetFont(pRenderTarget, rect, lpgw, wfontname, size, &pWriteTextFormat);
			free(wfontname);
#else
			hr = d2dSetFont(pRenderTarget, rect, lpgw, fontname, size, &pWriteTextFormat);
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
			// Minimum line width is 1 pixel.
			line_width = GPMAX(1, line_width);
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

			// update brushes
			pSolidBrush->SetColor(D2DCOLORREF(color, alpha_c));

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
				ppt = (D2D1_POINT_2F *) realloc(ppt, (polymax + 1) * sizeof(D2D1_POINT_2F));
			}
			ppt[polyi].x = xdash;
			ppt[polyi].y = ydash;
			polyi++;
			break;
		}

		case W_filled_polygon_draw: {
			bool found = false;
			int i, k;
			//bool same_rot = true;

			// Test if successive polygons share a common edge:
			if ((last_poly != NULL) && (polyi > 2)) {
				// Check for a common edge with previous filled polygon.
				for (i = 0; (i < polyi) && !found; i++) {
					for (k = 0; (k < last_polyi) && !found; k++) {
						if ((ppt[i].x == last_poly[k].x) && (ppt[i].y == last_poly[k].y)) {
							if ((ppt[(i + 1) % polyi].x == last_poly[(k + 1) % last_polyi].x) &&
							    (ppt[(i + 1) % polyi].y == last_poly[(k + 1) % last_polyi].y)) {
								//found = true;
								//same_rot = true;
							}
							// This is the dominant case for filling between curves,
							// see fillbetween.dem and polar.dem.
							if ((ppt[(i + 1) % polyi].x == last_poly[(k + last_polyi - 1) % last_polyi].x) &&
							    (ppt[(i + 1) % polyi].y == last_poly[(k + last_polyi - 1) % last_polyi].y)) {
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
				last_poly = (D2D1_POINT_2F *) realloc(last_poly, (last_polyi + extra + 1) * sizeof(D2D1_POINT_2F));
				/* TODO: should use memmove instead */
				for (int n = last_polyi - 1; n >= k; n--) {
					last_poly[n + extra].x = last_poly[n].x;
					last_poly[n + extra].y = last_poly[n].y;
				}
				// copy new points
				for (int n = 0; n < extra; n++) {
					last_poly[k + n].x = ppt[(i + 2 + n) % polyi].x;
					last_poly[k + n].y = ppt[(i + 2 + n) % polyi].y;
				}
				last_polyi += extra;
			} else {
				if (last_poly != NULL) {
					D2D1_ANTIALIAS_MODE mode = pRenderTarget->GetAntialiasMode();
					if (lpgw->antialiasing && !lpgw->polyaa)
						pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
					if (pFillBrush != NULL) {
						if (pPolygonRenderTarget == NULL)
							hr = d2dFilledPolygon(pRenderTarget, pFillBrush, last_poly, last_polyi);
						else
							hr = d2dFilledPolygon(pPolygonRenderTarget, pFillBrush, last_poly, last_polyi);
					}
					pRenderTarget->SetAntialiasMode(mode);
					free(last_poly);
					last_poly = NULL;
				}
				// save the current polygon
				last_poly = (D2D1_POINT_2F *) malloc(sizeof(D2D1_POINT_2F) * (polyi + 1));
				memcpy(last_poly, ppt, sizeof(D2D1_POINT_2F) * (polyi + 1));
				last_polyi = polyi;
			}

			polyi = 0;
			break;
		}

		case W_image: {
			/* Due to the structure of gwop, 6 entries are needed in total. */
			if (seq == 0) {
				/* First OP contains only the color mode */
				color_mode = curptr->x;
			} else if (seq < 5) {
				/* Next four OPs contain the `corner` array */
				// FIXME
				corners[seq - 1].x = xdash + 0.5;
				corners[seq - 1].y = ydash + 0.5;
			} else {
				/* The last OP contains the image and it's size */
				BYTE * image = reinterpret_cast<BYTE *>(LocalLock(curptr->htext));
				if (image == NULL) break; // memory allocation failed
				UINT32 width = curptr->x;
				UINT32 height = curptr->y;
				D2D1_SIZE_U size = D2D1::SizeU(width, height);
				ID2D1Bitmap *bitmap;

				// Note these are the only supported pixel formats for a D2D1DCRenderTarget. See:
				// https://msdn.microsoft.com/en-us/library/windows/desktop/dd756766.aspx
				if (color_mode != IC_RGBA) {
					UINT32 pad_bytes = (4 - (3 * width) % 4) % 4; // scan lines start on UINT32 boundaries
					UINT32 * argb_image = new UINT32[width * height];

					// RGB24 is not a supported bitmap format; convert to RGB32
					for (unsigned y = 0; y < height; y++) {
						for (unsigned x = 0; x < width; x++) {
							BYTE * p = (BYTE *) image + y * (3 * width + pad_bytes) + x * 3;
							if (lpgw->color) {
								argb_image[y * width + x] = RGB(p[0], p[1], p[2]);
							} else {
								unsigned luma = luma_from_color(p[2], p[1], p[0]);
								argb_image[y * width + x] = RGB(luma, luma, luma);
							}
						}
					}
					UINT32 stride = width * 4;
					hr = pRenderTarget->CreateBitmap(size, argb_image, stride,
													 D2D1::BitmapProperties(
														D2D1::PixelFormat(
															DXGI_FORMAT_B8G8R8A8_UNORM,
															D2D1_ALPHA_MODE_IGNORE),
														dpiX, dpiY),
													&bitmap);
					delete[] argb_image;
				} else {
					UINT32 stride = width * 4;
					UINT32 * argb_image = reinterpret_cast<UINT32 *>(image);

					// convert to gray-scale
					if (!lpgw->color) {
						argb_image = new UINT32[width * height];
						for (unsigned y = 0; y < height; y++) {
							for (unsigned x = 0; x < width; x++) {
								UINT32 v = reinterpret_cast<UINT32 *>(image)[y * width + x];
								unsigned luma = luma_from_color(GetRValue(v), GetGValue(v), GetBValue(v));
								argb_image[y * width + x] = (v & 0xff000000) | RGB(luma, luma, luma);
							}
						}
					}
					hr = pRenderTarget->CreateBitmap(size, argb_image, stride,
													 D2D1::BitmapProperties(
														 D2D1::PixelFormat(
															 DXGI_FORMAT_B8G8R8A8_UNORM,
															 D2D1_ALPHA_MODE_PREMULTIPLIED),
														 dpiX, dpiY),
													 &bitmap);
					if (!lpgw->color)
						delete[] argb_image;
				}
				if (SUCCEEDED(hr)) {
					// Note: if we would have a device context instead of a render target, we could use a ColorMatrix
					//       (or GrayScale) effect instead of the manual conversion above, which would use the GPU.
					D2D1_RECT_F dest = D2D1::RectF(
						(INT)GPMIN(corners[0].x, corners[1].x),
						(INT)GPMIN(corners[0].y, corners[1].y),
						(INT)GPMAX(corners[0].x, corners[1].x),
						(INT)GPMAX(corners[0].y, corners[1].y));
					D2D1_RECT_F clip = D2D1::RectF(
						(INT)GPMIN(corners[2].x, corners[3].x),
						(INT)GPMIN(corners[2].y, corners[3].y),
						(INT)GPMAX(corners[2].x, corners[3].x),
						(INT)GPMAX(corners[2].y, corners[3].y));
					// Flip Y axis
					D2D1_MATRIX_3X2_F m;
					m._11 = 1.f; m._12 =  0.f;
					m._21 = 0.f; m._22 = -1.f;
					m._31 = 0.f; m._32 = dest.top + dest.bottom;
					pRenderTarget->PushAxisAlignedClip(clip, D2D1_ANTIALIAS_MODE_ALIASED);
					pRenderTarget->SetTransform(m);
					pRenderTarget->DrawBitmap(bitmap, dest, 1.0, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
					pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
					pRenderTarget->PopAxisAlignedClip();
					SafeRelease(&bitmap);
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

			(void) last_symbol;
#if 0
			// draw cached point symbol
			if (ps_caching && (last_symbol == symbol) && (cb != NULL)) {
				// always draw point symbols on integer (pixel) positions
				GETHDC
				if (SUCCEEDED(hr)) {
					Graphics * graphics = gdiplusGraphics(lpgw, hdc);
					if (lpgw->oversample)
						graphics->DrawCachedBitmap(cb, INT(xdash + 0.5) - cb_ofs.x, INT(ydash + 0.5) - cb_ofs.y);
					else
						graphics->DrawCachedBitmap(cb, xdash - cb_ofs.x, ydash - cb_ofs.y);
					delete graphics;
					RELEASEHDC
				}
				break;
			} else {
				if (cb != NULL) {
					delete cb;
					cb = NULL;
				}
			}

			Bitmap *b = 0;
			Graphics *g = 0;
#endif
			float xofs;
			float yofs;

			// Switch between cached and direct drawing
			if (ps_caching) {
#if 0
				// TODO: symbol caching not implemented yet. Should it?
				// Create a compatible bitmap
				b = new Bitmap(2 * htic + 3, 2 * vtic + 3);
				g = Graphics::FromImage(b);
				if (lpgw->antialiasing)
					g->SetSmoothingMode(SmoothingModeAntiAlias8x8);
				cb_ofs.x = xofs = htic + 1;
				cb_ofs.y = yofs = vtic + 1;
				last_symbol = symbol;
#endif
			} else {
				// snap to pixel
				if (lpgw->oversample) {
					xofs = trunc(xdash) + 0.5;
					yofs = trunc(ydash) + 0.5;
				} else {
					xofs = xdash;
					yofs = ydash;
				}
			}

			switch (symbol) {
			case W_dot:
				d2dDot(pRenderTarget, pSolidBrush, xofs, yofs);
				break;
			case W_plus: /* do plus */
			case W_star: /* do star: first plus, then cross */
				pRenderTarget->DrawLine(D2D1::Point2F(xofs - htic, yofs), D2D1::Point2F(xofs + htic, yofs), pSolidBrush, line_width);
				pRenderTarget->DrawLine(D2D1::Point2F(xofs, yofs - vtic), D2D1::Point2F(xofs, yofs + vtic), pSolidBrush, line_width);
				if (symbol == W_plus)
					break;
			case W_cross: /* do X */
				pRenderTarget->DrawLine(D2D1::Point2F(xofs - htic, yofs - vtic), D2D1::Point2F(xofs + htic, yofs + vtic), pSolidBrush, line_width);
				pRenderTarget->DrawLine(D2D1::Point2F(xofs - htic, yofs + vtic), D2D1::Point2F(xofs + htic, yofs - vtic), pSolidBrush, line_width);
				break;
			case W_fcircle: /* do filled circle */
				pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(xofs, yofs), htic, htic), pSolidBrush);
				// fall through
			case W_circle: /* do open circle */
				pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(xofs, yofs), htic, htic), pSolidBrush, line_width, pSolidStrokeStyle);
				break;
			default: {	/* potentially closed figure */
				D2D1_POINT_2F p[6];
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
					p[i].x = xofs + htic * pointshapes[shape][i * 2];
					p[i].y = yofs + vtic * pointshapes[shape][i * 2 + 1];
				}
				if (filled) {
					/* filled polygon with border */
					d2dFilledPolygon(pRenderTarget, pSolidBrush, p, i);
					d2dPolyline(pRenderTarget, pSolidBrush, pSolidStrokeStyle, line_width, p, i, true);
				} else {
					/* Outline polygon */
					d2dPolyline(pRenderTarget, pSolidBrush, pSolidStrokeStyle, line_width, p, i, true);
					d2dDot(pRenderTarget, pSolidBrush, xofs, yofs);
				}
			} /* default case */
			} /* switch (point symbol) */
#if 0
			if (b != NULL) {
				GETHDC
				Graphics * graphics = gdiplusGraphics(lpgw, hdc);
				// create a cached bitmap for faster redrawing
				cb = new CachedBitmap(b, graphics);
				// display point symbol snapped to pixel
				if (lpgw->oversample)
					graphics->DrawCachedBitmap(cb, INT(xdash + 0.5) - xofs, INT(ydash + 0.5) - yofs);
				else
					graphics->DrawCachedBitmap(cb, xdash - xofs, ydash - yofs);
				delete graphics;
				RELEASEHDC
				delete b;
				delete g;
			}
#endif
			if (keysample) {
				d2d_update_keybox(lpgw, pRenderTarget, plotno, xdash + htic, ydash + vtic);
				d2d_update_keybox(lpgw, pRenderTarget, plotno, xdash - htic, ydash - vtic);
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

		if (FAILED(hr)) {
			fprintf(stderr, "Warning: Direct2D back-end error %x; last command: %i\n", hr, lastop);
		}
	} /* while (ngwop < lpgw->nGWOP) */

	/* clean-up */
#if 0
	if (cb)
		delete cb;
#endif
	free(ppt);

	hr = pRenderTarget->EndDraw();

#if !defined(DCRENDERER) && defined(HAVE_D2D11)
	if (interactive) {
		// Present (new for Direct2D 1.1)
		DXGI_PRESENT_PARAMETERS parameters = { 0 };
		parameters.DirtyRectsCount = 0;
		parameters.pDirtyRects = NULL;
		parameters.pScrollRect = NULL;
		parameters.pScrollOffset = NULL;

		hr = lpgw->pDXGISwapChain->Present1(1, 0, &parameters);
	}
#endif

	// TODO: some of these resources are device independent and could be preserved
	SafeRelease(&pSolidBrush);
	SafeRelease(&pSolidFillBrush);
	SafeRelease(&pPatternFillBrush);
	SafeRelease(&pGrayBrush);
	SafeRelease(&pSolidStrokeStyle);
	SafeRelease(&pStrokeStyle);
	SafeRelease(&pWriteTextFormat);

	if (hr == D2DERR_RECREATE_TARGET) {
		hr = S_OK;
		// discard device resources
		d2dReleaseRenderTarget(lpgw);
	}
	return hr;
}
