/*
 * $Id: wxt_gui.cpp,v 1.26 2006/09/11 21:48:38 tlecomte Exp $
 */

/* GNUPLOT - wxt_gui.cpp */

/*[
 * Copyright 2005,2006   Timothee Lecomte
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
 *
 *
 * Alternatively, the contents of this file, apart from one portion
 * that originates from other gnuplot files and is designated as such,
 * may be used under the terms of the GNU General Public License
 * Version 2 or later (the "GPL"), in which case the provisions of GPL
 * are applicable instead of those above. If you wish to allow
 * use of your version of the appropriate portion of this file only
 * under the terms of the GPL and not to allow others to use your version
 * of this file under the above gnuplot license, indicate your decision
 * by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL. If you do not
 * delete the provisions above, a recipient may use your version of this file
 * under either the GPL or the gnuplot license.
]*/

/* -----------------------------------------------------
 * The following code uses the wxWidgets library, which is
 * distributed under its own licence (derivated from the LGPL).
 *
 * You can read it at the following address :
 * http://www.wxwidgets.org/licence.htm
 * -----------------------------------------------------*/

/* ------------------------------------------------------
 * This file implements in C++ the functions which are called by wxt.trm :
 *
 * It depends on the generic cairo functions,
 * declared in gp_cairo.h for all the drawing work.
 *
 * Here is the interactive part :
 * - rescaling according to the window's size,
 * - mouse support (cursor position, zoom, rotation, ruler, clipboard...),
 * - a toolbar to give additionnal capabilities (similar to the OS/2 terminal),
 * - multiple plot windows.
 *
 * ------------------------------------------------------*/

/* PORTING NOTES
 * Since it uses wxWidgets and Cairo routines, this code is mostly cross-platform.
 * However some details have to be implemented or tweaked for each platform :
 *
 * 1) A generic 'image' surface is implemented as the destination surface
 * for cairo drawing. But for optimal results, cairo should draw to a native
 * surface corresponding to the graphical system.
 * Examples :
 * - a gdkpixmap when compiling for wxGTK (currently disabled because of a bug in CAIRO_OPERATOR_SATURATE),
 * - a HDC when compiling for wxMSW
 * - [insert your contribution here !]
 *
 * 2) You have to be careful with the gui main loop.
 * As far as I understand :
 * Some platforms (Windows ?) require that it is in the main thread.
 * When compiling for Windows (wxMSW), the text window already implements it, so we
 * don't have to do it, but wxWidgets still have to be initialized correctly.
 * When compiling for Unix (wxGTK), we don't have one, so we launch it in a separate thread.
 * For new platforms, it is necessary to figure out what is necessary.
 */


/* define DEBUG here to have debugging messages in stderr */
#include "wxt_gui.h"

/* frame icon composed of three icons of different resolutions */
#include "bitmaps/xpm/icon16x16.xpm"
#include "bitmaps/xpm/icon32x32.xpm"
#include "bitmaps/xpm/icon64x64.xpm"
/* cursors */
#include "bitmaps/xpm/cross.xpm"
#include "bitmaps/xpm/right.xpm"
#include "bitmaps/xpm/rotate.xpm"
#include "bitmaps/xpm/size.xpm"
/* Toolbar icons
 * Those are embedded PNG icons previously converted to an array.
 * See bitmaps/png/README for details */
#include "bitmaps/png/clipboard_png.h"
#include "bitmaps/png/replot_png.h"
#include "bitmaps/png/grid_png.h"
#include "bitmaps/png/previouszoom_png.h"
#include "bitmaps/png/nextzoom_png.h"
#include "bitmaps/png/autoscale_png.h"
#include "bitmaps/png/config_png.h"
#include "bitmaps/png/help_png.h"

/* ---------------------------------------------------------------------------
 * event tables and other macros for wxWidgets
 * --------------------------------------------------------------------------*/

/* the event tables connect the wxWidgets events with the functions (event
 * handlers) which process them. It can be also done at run-time, but for the
 * simple menu events like this the static method is much simpler.
 */

BEGIN_EVENT_TABLE( wxtFrame, wxFrame )
	EVT_CLOSE( wxtFrame::OnClose )
	EVT_SIZE( wxtFrame::OnSize )
	EVT_TOOL( Toolbar_CopyToClipboard, wxtFrame::OnCopy )
#ifdef USE_MOUSE
	EVT_TOOL( Toolbar_Replot, wxtFrame::OnReplot )
	EVT_TOOL( Toolbar_ToggleGrid, wxtFrame::OnToggleGrid )
	EVT_TOOL( Toolbar_ZoomPrevious, wxtFrame::OnZoomPrevious )
	EVT_TOOL( Toolbar_ZoomNext, wxtFrame::OnZoomNext )
	EVT_TOOL( Toolbar_Autoscale, wxtFrame::OnAutoscale )
#endif /*USE_MOUSE*/
	EVT_TOOL( Toolbar_Config, wxtFrame::OnConfig )
	EVT_TOOL( Toolbar_Help, wxtFrame::OnHelp )
END_EVENT_TABLE()

BEGIN_EVENT_TABLE( wxtPanel, wxPanel )
	EVT_PAINT( wxtPanel::OnPaint )
	EVT_ERASE_BACKGROUND( wxtPanel::OnEraseBackground )
	EVT_SIZE( wxtPanel::OnSize )
#ifdef USE_MOUSE
	EVT_MOTION( wxtPanel::OnMotion )
	EVT_LEFT_DOWN( wxtPanel::OnLeftDown )
	EVT_LEFT_UP( wxtPanel::OnLeftUp )
	EVT_MIDDLE_DOWN( wxtPanel::OnMiddleDown )
	EVT_MIDDLE_UP( wxtPanel::OnMiddleUp )
	EVT_RIGHT_DOWN( wxtPanel::OnRightDown )
	EVT_RIGHT_UP( wxtPanel::OnRightUp )
	EVT_CHAR( wxtPanel::OnKeyDownChar )
#endif /*USE_MOUSE*/
END_EVENT_TABLE()

BEGIN_EVENT_TABLE( wxtConfigDialog, wxDialog )
	EVT_CLOSE( wxtConfigDialog::OnClose )
	EVT_CHOICE( Config_Rendering, wxtConfigDialog::OnRendering )
	EVT_COMMAND_RANGE( Config_OK, Config_CANCEL,
		wxEVT_COMMAND_BUTTON_CLICKED, wxtConfigDialog::OnButton )
END_EVENT_TABLE()


#ifdef __WXGTK__
/* ----------------------------------------------------------------------------
 *   gui thread
 * ----------------------------------------------------------------------------*/

/* What really happens in the thread
 * Just before it returns, wxEntry will call a whole bunch of wxWidgets-cleanup functions */
void *wxtThread::Entry()
{
	FPRINTF((stderr,"gui_thread_entry\n"));

	/* don't answer to SIGINT in this thread - avoids LONGJMP problems */
	sigset_t set[1] = {SIGINT};
	pthread_sigmask(SIG_BLOCK,set,NULL);

	/* gui loop */
	wxTheApp->OnRun();

	/* Workaround for a deadlock when the main thread will Wait() for this one.
	 * This issue comes from the fact that our gui main loop is not in the
	 * main thread as wxWidgets was written for. */
	wxt_MutexGuiLeave();

	FPRINTF((stderr,"gui_thread_entry finishing\n"));
	return NULL;
}
#elif defined (__WXMSW__)
/* nothing to do here */
#else
# error "Not implemented."
#endif



/* ----------------------------------------------------------------------------
 *  `Main program' equivalent: the program execution "starts" here
 * ----------------------------------------------------------------------------*/

/* Create a new application object */
IMPLEMENT_APP_NO_MAIN(wxtApp)

bool wxtApp::OnInit()
{
	/* Usually wxWidgets apps create their main window here.
	 * However, in the context of multiple plot windows, the same code is written in wxt_init().
	 * So, to avoid duplication of the code, we do only what is strictly necessary.*/

	/* initialize frames icons */
	icon.AddIcon(wxIcon(icon16x16_xpm));
	icon.AddIcon(wxIcon(icon32x32_xpm));
	icon.AddIcon(wxIcon(icon64x64_xpm));

	/* we load the image handlers, needed to copy the plot to clipboard, and to load icons */
	::wxInitAllImageHandlers();

#ifdef __WXMSW__
	/* allow the toolbar to display properly png icons with an alpha channel */
	wxSystemOptions::SetOption(wxT("msw.remap"), 0);
#endif /* __WXMSW__ */

	/* load toolbar icons */
	LoadPngIcon(clipboard_png, sizeof(clipboard_png), 0);
	LoadPngIcon(replot_png, sizeof(replot_png), 1);
	LoadPngIcon(grid_png, sizeof(grid_png), 2);
	LoadPngIcon(previouszoom_png, sizeof(previouszoom_png), 3);
	LoadPngIcon(nextzoom_png, sizeof(nextzoom_png), 4);
	LoadPngIcon(autoscale_png, sizeof(autoscale_png), 5);
	LoadPngIcon(config_png, sizeof(config_png), 6);
	LoadPngIcon(help_png, sizeof(help_png), 7);

	/* load cursors */
	LoadCursor(wxt_cursor_cross, cross);
	LoadCursor(wxt_cursor_right, right);
	LoadCursor(wxt_cursor_rotate, rotate);
	LoadCursor(wxt_cursor_size, size);

	/* Initialize the config object */
	/* application and vendor name are used by wxConfig to construct the name
	 * of the config file/registry key and must be set before the first call
	 * to Get() */
	SetVendorName(wxT("gnuplot"));
	SetAppName(wxT("gnuplot-wxt"));
	wxConfigBase *pConfig = wxConfigBase::Get();
	/* this will force writing back of the defaults for all values
	 * if they're not present in the config - this can give the user an idea
	 * of all possible settings */
	pConfig->SetRecordDefaults();

	return true; /* means that process must continue */
}

/* load an icon from a PNG file embedded as a C array */
void wxtApp::LoadPngIcon(const unsigned char *embedded_png, int length, int icon_number)
{
	wxMemoryInputStream pngstream(embedded_png, length);
	toolBarBitmaps[icon_number] = new wxBitmap(wxImage(pngstream, wxBITMAP_TYPE_PNG));
}

/* load a cursor */
void wxtApp::LoadCursor(wxCursor &cursor, char* xpm_bits[])
{
	int hotspot_x, hotspot_y;
	wxBitmap cursor_bitmap = wxBitmap(xpm_bits);
	wxImage cursor_image = cursor_bitmap.ConvertToImage();
	/* XPM spec : first string is :
	 * width height ncolors charperpixel hotspotx hotspoty */
	sscanf(xpm_bits[0], "%*d %*d %*d %*d %d %d", &hotspot_x, &hotspot_y);
	cursor_image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, hotspot_x);
	cursor_image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, hotspot_y);
	cursor = wxCursor(cursor_image);
}

/* cleanup on exit
 * In a pure wxWidgets app, the returned int is the exit status of the app.
 * Here it is not used. */
int wxtApp::OnExit()
{
	FPRINTF((stderr,"wxtApp::OnExit\n"));
	/* clean up: Set() returns the active config object as Get() does, but unlike
	 * Get() it doesn't try to create one if there is none (definitely not what
	 * we want here!) */
	delete wxConfigBase::Set((wxConfigBase *) NULL);
	return 0;
}

/* ---------------------------------------------------------------------------
 * Frame : the main windows (one for each plot)
 * ----------------------------------------------------------------------------*/

/* frame constructor*/
wxtFrame::wxtFrame( const wxString& title, wxWindowID id, int xpos, int ypos, int width, int height )
	: wxFrame((wxFrame *)NULL, id, title, wxPoint(xpos,ypos),
			wxSize(width,height), wxDEFAULT_FRAME_STYLE|wxWANTS_CHARS)
{
	FPRINTF((stderr,"wxtFrame constructor\n"));

	/* used to check for panel initialization */
	panel = NULL;

	/* initialize the state of the configuration dialog */
	config_displayed = false;

	/* set up the window icon, in several resolutions */
	SetIcons(icon);

	/* set up the status bar, and fill it with an empty
	 * string. It will be immediately overriden by gnuplot. */
	CreateStatusBar();
	SetStatusText( wxT("") );

	/* set up the toolbar */
	wxToolBar * toolbar = CreateToolBar();
	/* With wxMSW, default toolbar size is only 16x15. */
	toolbar->SetToolBitmapSize(wxSize(16,16));

	toolbar->AddTool(Toolbar_CopyToClipboard, wxT("Copy"),
				*(toolBarBitmaps[0]), wxT("Copy the plot to clipboard"));
#ifdef USE_MOUSE
	toolbar->AddSeparator();
	toolbar->AddTool(Toolbar_Replot, wxT("Replot"),
				*(toolBarBitmaps[1]), wxT("Replot"));
	toolbar->AddTool(Toolbar_ToggleGrid, wxT("Toggle grid"),
				*(toolBarBitmaps[2]),wxNullBitmap,wxITEM_NORMAL, wxT("Toggle grid"));
	toolbar->AddTool(Toolbar_ZoomPrevious, wxT("Previous zoom"),
				*(toolBarBitmaps[3]), wxT("Apply the previous zoom settings"));
	toolbar->AddTool(Toolbar_ZoomNext, wxT("Next zoom"),
				*(toolBarBitmaps[4]), wxT("Apply the next zoom settings"));
	toolbar->AddTool(Toolbar_Autoscale, wxT("Autoscale"),
				*(toolBarBitmaps[5]), wxT("Apply autoscale"));
#endif /*USE_MOUSE*/
	toolbar->AddSeparator();
	toolbar->AddTool(Toolbar_Config, wxT("Terminal configuration"),
				*(toolBarBitmaps[6]), wxT("Open configuration dialog"));
	toolbar->AddTool(Toolbar_Help, wxT("Help"),
				*(toolBarBitmaps[7]), wxT("Open help dialog"));
	toolbar->Realize();

	FPRINTF((stderr,"wxtFrame constructor 2\n"));

	/* build the panel, which will contain the visible device context */
	panel = new wxtPanel( this, this->GetId(), this->GetClientSize() );

	/* setting minimum height and width for the window */
	SetSizeHints(100, 100);

	FPRINTF((stderr,"wxtFrame constructor 3\n"));
}


/* toolbar event : Copy to clipboard
 * We will copy the panel to a bitmap, using platform-independant wxWidgets functions */
void wxtFrame::OnCopy( wxCommandEvent& WXUNUSED( event ) )
{
	FPRINTF((stderr,"Copy to clipboard\n"));
	int width = panel->plot.device_xmax, height = panel->plot.device_ymax;
	wxBitmap cp_bitmap(width,height);
	wxMemoryDC cp_dc;
	wxClientDC dc(panel);

	cp_dc.SelectObject(cp_bitmap);
	cp_dc.Blit(0,0,width,height,&dc,0,0);
	cp_dc.SelectObject(wxNullBitmap);

	wxTheClipboard->UsePrimarySelection(false);
	/* SetData clears the clipboard */
	if ( wxTheClipboard->Open() ) {
		wxTheClipboard->SetData(new wxBitmapDataObject(cp_bitmap));
		wxTheClipboard->Close();
	}
	wxTheClipboard->Flush();
}

#ifdef USE_MOUSE
/* toolbar event : Replot */
void wxtFrame::OnReplot( wxCommandEvent& WXUNUSED( event ) )
{
	if ( this->GetId()==wxt_window_number )
		wxt_exec_event(GE_keypress, 0, 0, 'e' , 0, this->GetId());
}

/* toolbar event : Toggle Grid */
void wxtFrame::OnToggleGrid( wxCommandEvent& WXUNUSED( event ) )
{
	if ( this->GetId()==wxt_window_number )
		wxt_exec_event(GE_keypress, 0, 0, 'g', 0, this->GetId());
}

/* toolbar event : Previous Zoom in history */
void wxtFrame::OnZoomPrevious( wxCommandEvent& WXUNUSED( event ) )
{
	if ( this->GetId()==wxt_window_number )
		wxt_exec_event(GE_keypress, 0, 0, 'p', 0, this->GetId());
}

/* toolbar event : Next Zoom in history */
void wxtFrame::OnZoomNext( wxCommandEvent& WXUNUSED( event ) )
{
	if ( this->GetId()==wxt_window_number )
		wxt_exec_event(GE_keypress, 0, 0, 'n', 0, this->GetId());
}

/* toolbar event : Autoscale */
void wxtFrame::OnAutoscale( wxCommandEvent& WXUNUSED( event ) )
{
	if ( this->GetId()==wxt_window_number )
		wxt_exec_event(GE_keypress, 0, 0, 'a', 0, this->GetId());
}
#endif /*USE_MOUSE*/

/* toolbar event : Config */
void wxtFrame::OnConfig( wxCommandEvent& WXUNUSED( event ) )
{
	/* if we have already opened a dialog, just raise it */
	if (config_displayed) {
		config_dialog->Raise();
		return;
	}

	/* otherwise, open a dialog */
	config_displayed = true;
	config_dialog = new wxtConfigDialog(this);
	config_dialog->Show(true);
}


/* toolbar event : Help */
void wxtFrame::OnHelp( wxCommandEvent& WXUNUSED( event ) )
{
	wxMessageBox( wxString(wxT("You are using an interactive terminal "\
		"based on wxWidgets for the interface, Cairo "\
		"for the drawing facilities, and Pango for the text layouts.\n"\
		"Please note that toolbar icons in the terminal "\
		"don't reflect the whole range of mousing "\
		"possibilities in the terminal.\n"\
		"Hit 'h' in the plot window "\
		"and a help message for mouse commands "\
		"will appear in the gnuplot console.\n"\
		"See also 'help mouse'.\n")),
		wxT("wxWidgets terminal help"), wxOK | wxICON_INFORMATION, this );
}

/* called on Close() (menu or window manager) */
void wxtFrame::OnClose( wxCloseEvent& event )
{
	FPRINTF((stderr,"OnClose\n"));
	if ( event.CanVeto() ) {
		/* Default behaviour when Quit is clicked, or the window cross X */
		event.Veto();
		this->Hide();
	}
	else /* Should not happen, but just in case */
		this->Destroy();
}

/* when the window is resized,
 * resize the panel to fit in the frame.
 * Note : can't simply use "replot", as it doesn't work with multiplot mode */
void wxtFrame::OnSize( wxSizeEvent& event )
{
	FPRINTF((stderr,"frame OnSize\n"));

	/* Under Windows the frame receives an OnSize event before being completely initialized.
	 * So we must check for the panel to be properly initialized before.*/
	if (panel)
		panel->SetSize( this->GetClientSize() );
}

/* ---------------------------------------------------------------------------
 * Panel : the space used for the plot, between the toolbar and the statusbar
 * ----------------------------------------------------------------------------*/

/* panel constructor
 * Note : under Windows, wxDefaultPosition makes the panel hide the toolbar */
wxtPanel::wxtPanel( wxWindow *parent, wxWindowID id, const wxSize& size )
	: wxPanel( parent,  id,  wxPoint(0,0) /*wxDefaultPosition*/, size, wxWANTS_CHARS )
{
	FPRINTF((stderr,"panel constructor\n"));

	/* initialisations */
	gp_cairo_initialize_plot(&plot);
	GetSize(&(plot.device_xmax),&(plot.device_ymax));

	settings_queued = false;

#ifdef USE_MOUSE
	mouse_x = 0;
	mouse_y = 0;
	wxt_zoombox = false;
	zoom_x1 = 0;
	zoom_y1 = 0;
	zoom_string1 = wxT("");
	zoom_string2 = wxT("");

	wxt_ruler = false;
	wxt_ruler_x = 0;
	wxt_ruler_y = 0;

	modifier_mask = 0;
#endif /*USE_MOUSE*/

#if defined(GTK_SURFACE)
	gdkpixmap = NULL;
#elif defined(__WXMSW__)
	hdc = NULL;
	hbm = NULL;
#else /* IMAGE_SURFACE */
	cairo_bitmap = NULL;
	data32 = NULL;
#endif /* SURFACE */

	FPRINTF((stderr,"panel constructor4\n"));

	/* create the device context to be drawn */
	wxt_cairo_create_context();

	FPRINTF((stderr,"panel constructor5\n"));

#ifdef IMAGE_SURFACE
	wxt_cairo_create_bitmap();
#endif
	FPRINTF((stderr,"panel constructor6\n"));
}


/* destructor */
wxtPanel::~wxtPanel()
{
	FPRINTF((stderr,"panel destructor\n"));
	wxt_cairo_free_context();

	/* clear the command list, free the allocated memory */
	ClearCommandlist();
}

/* temporary store new settings values to be applied for the next plot */
void wxtPanel::wxt_settings_queue(TBOOLEAN antialiasing,
					TBOOLEAN oversampling,
					int hinting_setting)
{
	mutex_queued.Lock();
	settings_queued = true;
	antialiasing_queued = antialiasing;
	oversampling_queued = oversampling;
	hinting_queued = hinting_setting;
	mutex_queued.Unlock();
}

/* apply queued settings */
void wxtPanel::wxt_settings_apply()
{
	mutex_queued.Lock();
	if (settings_queued) {
		plot.antialiasing = antialiasing_queued;
		plot.oversampling = oversampling_queued;
		plot.hinting = hinting_queued;
		settings_queued = false;
	}
	mutex_queued.Unlock();
}

/* clear the command list, free the allocated memory */
void wxtPanel::ClearCommandlist()
{
	command_list_mutex.Lock();

	command_list_t::iterator iter; /*declare the iterator*/

	/* run through the list, and free allocated memory */
	for(iter = command_list.begin(); iter != command_list.end(); ++iter) {
		if (iter->command == command_enhanced_put_text ||
			iter->command == command_put_text ||
			iter->command == command_set_font)
			delete[] iter->string;
		if (iter->command == command_filled_polygon)
			delete[] iter->corners;
#ifdef WITH_IMAGE
		if (iter->command == command_image)
			delete[] iter->image;
#endif /* WITH_IMAGE */
	}

	command_list.clear();
	command_list_mutex.Unlock();
}


/* method called when the panel has to be painted
 * -> Refresh(), window dragged, dialogs over the window, etc. */
void wxtPanel::OnPaint( wxPaintEvent &WXUNUSED(event) )
{
	/* Constructor of the device context */
	wxPaintDC dc(this);
	DrawToDC(dc, GetUpdateRegion());
}

/* same as OnPaint, but can be directly called by a user function */
void wxtPanel::Draw()
{
	wxClientDC dc(this);
	wxBufferedDC buffered_dc(&dc, wxSize(plot.device_xmax, plot.device_ymax));
	wxRegion region(0, 0, plot.device_xmax, plot.device_ymax);
	DrawToDC(buffered_dc, region);
}

/* copy the plot to the panel, draw zoombow and ruler needed */
void wxtPanel::DrawToDC(wxDC &dc, wxRegion &region)
{
	dc.BeginDrawing();

	wxPen tmp_pen;

	/* TODO extend the region mechanism to surfaces other than GTK_SURFACE */
#ifdef GTK_SURFACE
	wxRegionIterator upd(region);
	int vX,vY,vW,vH; /* Dimensions of client area in pixels */

	while (upd) {
		vX = upd.GetX();
		vY = upd.GetY();
		vW = upd.GetW();
		vH = upd.GetH();
	
		FPRINTF((stderr,"OnPaint %d,%d,%d,%d\n",vX,vY,vW,vH));
		/* Repaint this rectangle */
		if (gdkpixmap)
			gdk_draw_drawable(dc.GetWindow(),
				dc.m_penGC,
				gdkpixmap,
				vX,vY,
				vX,vY,
				vW,vH);
		++upd;
	}
#elif defined(__WXMSW__)
	BitBlt((HDC) dc.GetHDC(), 0, 0, plot.device_xmax, plot.device_ymax, hdc, 0, 0, SRCCOPY);
#else
	dc.DrawBitmap(*cairo_bitmap, 0, 0, false);
#endif

	/* fill in gray when the aspect ratio conservation has let empty space in the panel */
	if (plot.device_xmax*plot.ymax > plot.device_ymax*plot.xmax) {
		dc.SetPen( *wxTRANSPARENT_PEN );
		dc.SetBrush( wxBrush( wxT("LIGHT GREY"), wxSOLID ) );
		dc.DrawRectangle(plot.xmax/plot.oversampling_scale*plot.xscale,
				0,
				plot.device_xmax - plot.xmax/plot.oversampling_scale*plot.xscale,
				plot.device_ymax);
	} else if (plot.device_xmax*plot.ymax < plot.device_ymax*plot.xmax) {
		dc.SetPen( *wxTRANSPARENT_PEN );
		dc.SetBrush( wxBrush( wxT("LIGHT GREY"), wxSOLID ) );
		dc.DrawRectangle(0,
				plot.ymax/plot.oversampling_scale*plot.yscale,
				plot.device_xmax,
				plot.device_ymax - plot.ymax/plot.oversampling_scale*plot.yscale);
	}

#ifdef USE_MOUSE
	if (wxt_zoombox) {
		tmp_pen = wxPen( wxT("BLACK") );
		tmp_pen.SetCap( wxCAP_ROUND );
		dc.SetPen( tmp_pen );
		dc.SetLogicalFunction( wxINVERT );
		dc.DrawLine( zoom_x1, zoom_y1, mouse_x, zoom_y1 );
		dc.DrawLine( mouse_x, zoom_y1, mouse_x, mouse_y );
		dc.DrawLine( mouse_x, mouse_y, zoom_x1, mouse_y );
		dc.DrawLine( zoom_x1, mouse_y, zoom_x1, zoom_y1 );
		dc.SetPen( *wxTRANSPARENT_PEN );
		dc.SetBrush( wxBrush( wxT("LIGHT BLUE"), wxSOLID ) );
		dc.SetLogicalFunction( wxAND );
		dc.DrawRectangle( zoom_x1, zoom_y1, mouse_x -zoom_x1, mouse_y -zoom_y1);
		dc.SetLogicalFunction( wxCOPY );

		dc.DrawText( zoom_string1.BeforeFirst(wxT('\r')),
			zoom_x1, zoom_y1 - term->v_char/plot.oversampling_scale);
		dc.DrawText( zoom_string1.AfterFirst(wxT('\r')),
			zoom_x1, zoom_y1);

		dc.DrawText( zoom_string2.BeforeFirst(wxT('\r')),
			mouse_x, mouse_y - term->v_char/plot.oversampling_scale);
		dc.DrawText( zoom_string2.AfterFirst(wxT('\r')),
			mouse_x, mouse_y);

		/* if we have to redraw the zoombox, it is with another size,
		 * so it will be issued later and we can disable it now */
		wxt_zoombox = false;
	}

	if (wxt_ruler) {
		tmp_pen = wxPen(wxT("black"), 1, wxSOLID);
		tmp_pen.SetCap(wxCAP_BUTT);
		dc.SetPen( tmp_pen );
		dc.SetLogicalFunction( wxINVERT );
		dc.CrossHair( (int)wxt_ruler_x, (int)wxt_ruler_y );
		dc.SetLogicalFunction( wxCOPY );
	}

	if (wxt_ruler && wxt_ruler_lineto) {
		tmp_pen = wxPen(wxT("black"), 1, wxSOLID);
		tmp_pen.SetCap(wxCAP_BUTT);
		dc.SetPen( tmp_pen );
		dc.SetLogicalFunction( wxINVERT );
		dc.DrawLine((int)wxt_ruler_x, (int)wxt_ruler_y, mouse_x, mouse_y);
		dc.SetLogicalFunction( wxCOPY );
	}
#endif /*USE_MOUSE*/

	dc.EndDrawing();
}

/* avoid flickering under win32 */
void wxtPanel::OnEraseBackground( wxEraseEvent &WXUNUSED(event) )
{
}

/* when the window is resized */
void wxtPanel::OnSize( wxSizeEvent& event )
{
	/* don't do anything if term variables are not initialized */
	if (plot.xmax == 0 || plot.ymax == 0)
		return;

	/* update window size, and scaling variables */
	GetSize(&(plot.device_xmax),&(plot.device_ymax));

	double new_xscale, new_yscale;
	
	new_xscale = ((double) plot.device_xmax)*plot.oversampling_scale/((double) plot.xmax);
	new_yscale = ((double) plot.device_ymax)*plot.oversampling_scale/((double) plot.ymax);

	/* We will keep the aspect ratio constant */
	if (new_yscale == new_xscale) {
		plot.xscale = new_xscale;
		plot.yscale = new_yscale;
	} else if (new_yscale < new_xscale) {
		plot.xscale = new_yscale;
		plot.yscale = new_yscale;
	} else if (new_yscale > new_xscale) {
		plot.xscale = new_xscale;
		plot.yscale = new_xscale;
	}
	FPRINTF((stderr,"panel OnSize %d %d %lf %lf\n",
		plot.device_xmax, plot.device_ymax, plot.xscale,plot.yscale));

	/* create a new cairo context of the good size */
	wxt_cairo_create_context();
	/* redraw the plot with the new scaling */
	wxt_cairo_refresh();
}

#ifdef USE_MOUSE
/* when the mouse is moved over the panel */
void wxtPanel::OnMotion( wxMouseEvent& event )
{
	/* Get and store mouse position for _put_tmp_text() and key events (ruler) */
	mouse_x = event.GetX();
	mouse_y = event.GetY();

	UpdateModifiers(event);

	/* update the ruler_lineto thing */
	if (wxt_ruler && wxt_ruler_lineto)
		Draw();

	/* informs gnuplot */
	if ( this->GetId()==wxt_window_number )
		wxt_exec_event(GE_motion,
			(int)gnuplot_x( &plot, mouse_x ),
			(int)gnuplot_y( &plot, mouse_y ),
			0, 0, this->GetId());
}

/* mouse "click" event */
void wxtPanel::OnLeftDown( wxMouseEvent& event )
{
	int x,y;
	x = (int) gnuplot_x( &plot, event.GetX() );
	y = (int) gnuplot_y( &plot, event.GetY() );

	UpdateModifiers(event);

	if ( this->GetId()==wxt_window_number )
		wxt_exec_event(GE_buttonpress, x, y, 1, 0, this->GetId());
}

/* mouse "click" event */
void wxtPanel::OnLeftUp( wxMouseEvent& event )
{
	int x,y;
	x = (int) gnuplot_x( &plot, event.GetX() );
	y = (int) gnuplot_y( &plot, event.GetY() );

	UpdateModifiers(event);

	if ( this->GetId()==wxt_window_number ) {
		wxt_exec_event(GE_buttonrelease, x, y, 1, (int) left_button_sw.Time(), this->GetId());
		/* start a watch to send the time elapsed between up and down */
		left_button_sw.Start();
	}
}

/* mouse "click" event */
void wxtPanel::OnMiddleDown( wxMouseEvent& event )
{
	int x,y;
	x = (int) gnuplot_x( &plot, event.GetX() );
	y = (int) gnuplot_y( &plot, event.GetY() );

	UpdateModifiers(event);

	if ( this->GetId()==wxt_window_number )
		wxt_exec_event(GE_buttonpress, x, y, 2, 0, this->GetId());
}

/* mouse "click" event */
void wxtPanel::OnMiddleUp( wxMouseEvent& event )
{
	int x,y;
	x = (int) gnuplot_x( &plot, event.GetX() );
	y = (int) gnuplot_y( &plot, event.GetY() );

	UpdateModifiers(event);

	if ( this->GetId()==wxt_window_number ) {
		wxt_exec_event(GE_buttonrelease, x, y, 2,
			(int) middle_button_sw.Time(), this->GetId());
		/* start a watch to send the time elapsed between up and down */
		middle_button_sw.Start();
	}
}

/* mouse "click" event */
void wxtPanel::OnRightDown( wxMouseEvent& event )
{
	int x,y;
	x = (int) gnuplot_x( &plot, event.GetX() );
	y = (int) gnuplot_y( &plot, event.GetY() );

	UpdateModifiers(event);

	if ( this->GetId()==wxt_window_number )
		wxt_exec_event(GE_buttonpress, x, y, 3, 0, this->GetId());
}

/* mouse "click" event */
void wxtPanel::OnRightUp( wxMouseEvent& event )
{
	int x,y;
	x = (int) gnuplot_x( &plot, event.GetX() );
	y = (int) gnuplot_y( &plot, event.GetY() );

	UpdateModifiers(event);

	if ( this->GetId()==wxt_window_number ) {
		wxt_exec_event(GE_buttonrelease, x, y, 3, (int) right_button_sw.Time(), this->GetId());
		/* start a watch to send the time elapsed between up and down */
		right_button_sw.Start();
	}
}

/* the state of the modifiers is checked each time a key is pressed instead of
 * tracking the press and release events of the modifiers keys, because the
 * window manager catches some combinations, like ctrl+F1, and thus we do not
 * receive a release event in this case */
void wxtPanel::UpdateModifiers( wxMouseEvent& event )
{
	int current_modifier_mask = 0;

	/* retrieve current modifier mask from the wxEvent */
	current_modifier_mask |= (event.AltDown() ? (1<<2) : 0);
	current_modifier_mask |= (event.ControlDown() ? (1<<1) : 0);
	current_modifier_mask |= (event.ShiftDown() ? (1) : 0);

	/* update if changed */
	if (modifier_mask != current_modifier_mask) {
		modifier_mask = current_modifier_mask;
		wxt_exec_event(GE_modifier, 0, 0, modifier_mask, 0, this->GetId());
	}
}

/* a key has been pressed, modifiers have already been handled.
 * We receive keycodes here, and we send corresponding events to gnuplot main thread */
void wxtPanel::OnKeyDownChar( wxKeyEvent &event )
{
	int keycode = event.GetKeyCode();
	int gp_keycode;

	/* this is the same code as in UpdateModifiers(), but the latter method cannot be
	 * used here because wxKeyEvent and wxMouseEvent are different classes, both of them
	 * derive from wxEvent, but wxEvent does not have the necessary AltDown() and friends */
	int current_modifier_mask = 0;

	/* retrieve current modifier mask from the wxEvent */
	current_modifier_mask |= (event.AltDown() ? (1<<2) : 0);
	current_modifier_mask |= (event.ControlDown() ? (1<<1) : 0);
	current_modifier_mask |= (event.ShiftDown() ? (1) : 0);

	/* update if changed */
	if (modifier_mask != current_modifier_mask) {
		modifier_mask = current_modifier_mask;
		wxt_exec_event(GE_modifier, 0, 0, modifier_mask, 0, this->GetId());
	}

#define WXK_GPKEYCODE(wxkey,kcode) case wxkey : gp_keycode=kcode; break;

	if (keycode<256) {
		switch (keycode) {
		case WXK_SPACE :
			if ((wxt_ctrl==yes && event.ControlDown())
				|| wxt_ctrl!=yes) {
				RaiseConsoleWindow();
				return;
			} else {
				gp_keycode = ' ';
				break;
			}
		case 'q' :
		/* ctrl+q does not send 113 but 17 */
		/* WARNING : may be the same for other combinations */
		case 17 :
			if ((wxt_ctrl==yes && event.ControlDown())
				|| wxt_ctrl!=yes) {
				/* closes terminal window */
				this->GetParent()->Close(false);
				return;
			} else {
				gp_keycode = 'q';
				break;
			}
		WXK_GPKEYCODE(WXK_BACK,GP_BackSpace);
		WXK_GPKEYCODE(WXK_TAB,GP_Tab);
		WXK_GPKEYCODE(WXK_RETURN,GP_Return);
		WXK_GPKEYCODE(WXK_ESCAPE,GP_Escape);
		WXK_GPKEYCODE(WXK_DELETE,GP_Delete);
		default : gp_keycode = keycode; break; /* exact solution */
		}
	} else {
		switch( keycode ) {
		WXK_GPKEYCODE(WXK_PAUSE,GP_Pause);
		WXK_GPKEYCODE(WXK_SCROLL,GP_Scroll_Lock);
		WXK_GPKEYCODE(WXK_INSERT,GP_Insert);
		WXK_GPKEYCODE(WXK_HOME,GP_Home);
		WXK_GPKEYCODE(WXK_LEFT,GP_Left);
		WXK_GPKEYCODE(WXK_UP,GP_Up);
		WXK_GPKEYCODE(WXK_RIGHT,GP_Right);
		WXK_GPKEYCODE(WXK_DOWN,GP_Down);
		WXK_GPKEYCODE(WXK_PAGEUP,GP_PageUp);
		WXK_GPKEYCODE(WXK_PAGEDOWN,GP_PageDown);
		WXK_GPKEYCODE(WXK_END,GP_End);
		WXK_GPKEYCODE(WXK_NUMPAD_SPACE,GP_KP_Space);
		WXK_GPKEYCODE(WXK_NUMPAD_TAB,GP_KP_Tab);
		WXK_GPKEYCODE(WXK_NUMPAD_ENTER,GP_KP_Enter);
		WXK_GPKEYCODE(WXK_NUMPAD_F1,GP_KP_F1);
		WXK_GPKEYCODE(WXK_NUMPAD_F2,GP_KP_F2);
		WXK_GPKEYCODE(WXK_NUMPAD_F3,GP_KP_F3);
		WXK_GPKEYCODE(WXK_NUMPAD_F4,GP_KP_F4);
		
		WXK_GPKEYCODE(WXK_NUMPAD_INSERT,GP_KP_Insert);
		WXK_GPKEYCODE(WXK_NUMPAD_END,GP_KP_End);
		WXK_GPKEYCODE(WXK_NUMPAD_DOWN,GP_KP_Down);
		WXK_GPKEYCODE(WXK_NUMPAD_PAGEDOWN,GP_KP_Page_Down);
		WXK_GPKEYCODE(WXK_NUMPAD_LEFT,GP_KP_Left);
		WXK_GPKEYCODE(WXK_NUMPAD_BEGIN,GP_KP_Begin);
		WXK_GPKEYCODE(WXK_NUMPAD_RIGHT,GP_KP_Right);
		WXK_GPKEYCODE(WXK_NUMPAD_HOME,GP_KP_Home);
		WXK_GPKEYCODE(WXK_NUMPAD_UP,GP_KP_Up);
		WXK_GPKEYCODE(WXK_NUMPAD_PAGEUP,GP_KP_Page_Up);
		
		WXK_GPKEYCODE(WXK_NUMPAD_DELETE,GP_KP_Delete);
		WXK_GPKEYCODE(WXK_NUMPAD_EQUAL,GP_KP_Equal);
		WXK_GPKEYCODE(WXK_NUMPAD_MULTIPLY,GP_KP_Multiply);
		WXK_GPKEYCODE(WXK_NUMPAD_ADD,GP_KP_Add);
		WXK_GPKEYCODE(WXK_NUMPAD_SEPARATOR,GP_KP_Separator);
		WXK_GPKEYCODE(WXK_NUMPAD_SUBTRACT,GP_KP_Subtract);
		WXK_GPKEYCODE(WXK_NUMPAD_DECIMAL,GP_KP_Decimal);
		WXK_GPKEYCODE(WXK_NUMPAD_DIVIDE,GP_KP_Divide);
		WXK_GPKEYCODE(WXK_NUMPAD0,GP_KP_0);
		WXK_GPKEYCODE(WXK_NUMPAD1,GP_KP_1);
		WXK_GPKEYCODE(WXK_NUMPAD2,GP_KP_2);
		WXK_GPKEYCODE(WXK_NUMPAD3,GP_KP_3);
		WXK_GPKEYCODE(WXK_NUMPAD4,GP_KP_4);
		WXK_GPKEYCODE(WXK_NUMPAD5,GP_KP_5);
		WXK_GPKEYCODE(WXK_NUMPAD6,GP_KP_6);
		WXK_GPKEYCODE(WXK_NUMPAD7,GP_KP_7);
		WXK_GPKEYCODE(WXK_NUMPAD8,GP_KP_8);
		WXK_GPKEYCODE(WXK_NUMPAD9,GP_KP_9);
		WXK_GPKEYCODE(WXK_F1,GP_F1);
		WXK_GPKEYCODE(WXK_F2,GP_F2);
		WXK_GPKEYCODE(WXK_F3,GP_F3);
		WXK_GPKEYCODE(WXK_F4,GP_F4);
		WXK_GPKEYCODE(WXK_F5,GP_F5);
		WXK_GPKEYCODE(WXK_F6,GP_F6);
		WXK_GPKEYCODE(WXK_F7,GP_F7);
		WXK_GPKEYCODE(WXK_F8,GP_F8);
		WXK_GPKEYCODE(WXK_F9,GP_F9);
		WXK_GPKEYCODE(WXK_F10,GP_F10);
		WXK_GPKEYCODE(WXK_F11,GP_F11);
		WXK_GPKEYCODE(WXK_F12,GP_F12);
		default : return; /* probably not ideal */
		}
	}

	/* only send char events to gnuplot if we are the active window */
	if ( this->GetId()==wxt_window_number ) {
		FPRINTF((stderr,"sending char event\n"));
		wxt_exec_event(GE_keypress, (int) gnuplot_x( &plot, mouse_x ),
			(int) gnuplot_y( &plot, mouse_y ), gp_keycode, 0, this->GetId());
	}

	/* The following wxWidgets keycodes are not mapped :
	 *	WXK_ALT, WXK_CONTROL, WXK_SHIFT,
	 *	WXK_LBUTTON, WXK_RBUTTON, WXK_CANCEL, WXK_MBUTTON,
	 *	WXK_CLEAR, WXK_MENU,
	 *	WXK_NUMPAD_PRIOR, WXK_NUMPAD_NEXT,
	 *	WXK_CAPITAL, WXK_PRIOR, WXK_NEXT, WXK_SELECT,
	 *	WXK_PRINT, WXK_EXECUTE, WXK_SNAPSHOT, WXK_HELP,
	 *	WXK_MULTIPLY, WXK_ADD, WXK_SEPARATOR, WXK_SUBTRACT,
	 *	WXK_DECIMAL, WXK_DIVIDE, WXK_NUMLOCK, WXK_WINDOWS_LEFT,
	 *	WXK_WINDOWS_RIGHT, WXK_WINDOWS_MENU, WXK_COMMAND
	 * The following gnuplot keycodes are not mapped :
	 *	GP_Linefeed, GP_Clear, GP_Sys_Req, GP_Begin
	 */
}
#endif /*USE_MOUSE*/

/* ====license information====
 * The following code originates from other gnuplot files,
 * and is not subject to the alternative license statement.
 */


/* FIXME : this code should be deleted, and the feature removed or handled differently,
 * because it is highly platform-dependant, is not reliable because
 * of a lot of factors (WINDOWID not set, multiple tabs in gnome-terminal, mechanisms
 * to prevent focus stealing) and is inconsistent with global bindings mechanism ) */
void wxtPanel::RaiseConsoleWindow()
{
#ifdef USE_GTK
	char *window_env;
	unsigned long windowid = 0;
	/* retrieve XID of gnuplot window */
	window_env = getenv("WINDOWID");
	if (window_env)
		sscanf(window_env, "%lu", &windowid);
	
	char *ptr = getenv("KONSOLE_DCOP_SESSION"); /* Try KDE's Konsole first. */
	if (ptr) {
		/* We are in KDE's Konsole, or in a terminal window detached from a Konsole.
		* In order to active a tab:
		* 1. get environmental variable KONSOLE_DCOP_SESSION: it includes konsole id and session name
		* 2. if
		*	$WINDOWID is defined and it equals
		*	    `dcop konsole-3152 konsole-mainwindow#1 getWinID`
		*	(KDE 3.2) or when $WINDOWID is undefined (KDE 3.1), then run commands
		*    dcop konsole-3152 konsole activateSession session-2; \
		*    dcop konsole-3152 konsole-mainwindow#1 raise
		* Note: by $WINDOWID we mean gnuplot's text console WINDOWID.
		* Missing: focus is not transferred unless $WINDOWID is defined (should be fixed in KDE 3.2).
		*
		* Implementation and tests on KDE 3.1.4: Petr Mikulik.
		*/
		char *konsole_name = NULL;
		char *cmd = NULL;
		/* use 'while' to easily break out (aka catch exception) */
		while (1) {
			char *konsole_tab;
			unsigned long w;
			FILE *p;
			ptr = strchr(ptr, '(');
			/* the string for tab nb 4 looks like 'DCOPRef(konsole-2866, session-4)' */
			if (!ptr) break;
			konsole_name = strdup(ptr+1);
			konsole_tab = strchr(konsole_name, ',');
			if (!konsole_tab) break;
			*konsole_tab++ = 0;
			ptr = strchr(konsole_tab, ')');
			if (ptr) *ptr = 0;
			cmd = (char*) malloc(strlen(konsole_name) + strlen(konsole_tab) + 64);
			sprintf(cmd, "dcop %s konsole-mainwindow#1 getWinID 2>/dev/null", konsole_name);
			/* is  2>/dev/null  portable among various shells? */
			p = popen(cmd, "r");
			if (p) {
				fscanf(p, "%lu", &w);
				pclose(p);
			}
			if (windowid) { /* $WINDOWID is known */
			if (w != windowid) break;
		/* `dcop getWinID`==$WINDOWID thus we are running in a window detached from Konsole */
			} else {
				windowid = w;
				/* $WINDOWID has not been known (KDE 3.1), thus set it up */
			}
			sprintf(cmd, "dcop %s konsole activateSession %s", konsole_name, konsole_tab);
			system(cmd);
		}
		if (konsole_name) free(konsole_name);
		if (cmd) free(cmd);
	}
	/* now test for GNOME multitab console */
	/* ... if somebody bothers to implement it ... */
	/* we are not running in any known (implemented) multitab console */
	
	if (windowid) {
		gdk_window_raise(gdk_window_foreign_new(windowid));
		gdk_window_focus(gdk_window_foreign_new(windowid), GDK_CURRENT_TIME);
	}
#endif /* USE_GTK */

#ifdef _Windows
	/* Make sure the text window is visible: */
	ShowWindow(textwin.hWndParent, SW_SHOW);
	/* and activate it (--> Keyboard focus goes there */
	BringWindowToTop(textwin.hWndParent);
#endif /* _Windows */

#ifdef OS2
	/* we assume that the console window is managed by PM, not by a X server */
	HSWITCH hSwitch = 0;
	SWCNTRL swGnu;
	HWND hw;
	/* get details of command-line window */
	hSwitch = WinQuerySwitchHandle(0, getpid());
	WinQuerySwitchEntry(hSwitch, &swGnu);
	hw = WinQueryWindow(swGnu.hwnd, QW_BOTTOM);
	WinSetFocus(HWND_DESKTOP, hw);
	WinSwitchToProgram(hSwitch);
#endif /* OS2 */
}

/* ====license information====
 * End of the non-relicensable portion.
 */


/* ------------------------------------------------------
 * Configuration dialog
 * ------------------------------------------------------*/

/* configuration dialog : handler for a close event */
void wxtConfigDialog::OnClose( wxCloseEvent& WXUNUSED( event ) )
{
	wxtFrame *parent = (wxtFrame *) GetParent();
	parent->config_displayed = false;
	this->Destroy();
}

/* configuration dialog : handler for a button event */
void wxtConfigDialog::OnButton( wxCommandEvent& event )
{
	TBOOLEAN antialiasing;
	TBOOLEAN oversampling;

	wxConfigBase *pConfig = wxConfigBase::Get();
	Validate();
	TransferDataFromWindow();

	wxtFrame *parent = (wxtFrame *) GetParent();

	switch (event.GetId()) {
	case Config_OK :
		Close(true);
		/* continue */
	case Config_APPLY :
		/* changes are applied immediately */
		wxt_raise = raise_setting?yes:no;
		wxt_persist = persist_setting?yes:no;
		wxt_ctrl = ctrl_setting?yes:no;

		switch (rendering_setting) {
		case 0 :
			antialiasing = FALSE;
			oversampling = FALSE;
			break;
		case 1 :
			antialiasing = TRUE;
			oversampling = FALSE;
			break;
		case 2 :
		default :
			antialiasing = TRUE;
			oversampling = TRUE;
			break;
		}

		/* we cannot apply the new settings right away, because it would mess up
		 * the plot in case of a window resize.
		 * Instead, we queue the settings until the next plot. */
		parent->panel->wxt_settings_queue(antialiasing, oversampling, hinting_setting);

		if (!pConfig->Write(wxT("raise"), raise_setting))
			wxLogError(wxT("Cannot write raise"));
		if (!pConfig->Write(wxT("persist"), persist_setting))
			wxLogError(wxT("Cannot write persist"));
		if (!pConfig->Write(wxT("ctrl"), ctrl_setting))
			wxLogError(wxT("Cannot write ctrl"));
		if (!pConfig->Write(wxT("rendering"), rendering_setting))
			wxLogError(wxT("Cannot write rendering_setting"));
		if (!pConfig->Write(wxT("hinting"), hinting_setting))
			wxLogError(wxT("Cannot write hinting_setting"));
		break;
	case Config_CANCEL :
	default :
		Close(true);
		break;
	}
}

/* Configuration dialog constructor */
wxtConfigDialog::wxtConfigDialog(wxWindow* parent)
	: wxDialog(parent, -1, wxT("Terminal configuration"), wxDefaultPosition, wxDefaultSize,
                   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
/* 	wxStaticBox *sb = new wxStaticBox( this, wxID_ANY, _T("&Explanation"),
		wxDefaultPosition, wxDefaultSize );
	wxStaticBoxSizer *wrapping_sizer = new wxStaticBoxSizer( sb, wxVERTICAL );
	wxStaticText *text1 = new wxStaticText(this, wxID_ANY,
		wxT("Options remembered between sessions, ")
		wxT("overriden by `set term wxt <options>`.\n\n"),
		wxDefaultPosition, wxSize(300, wxDefaultCoord));
	wrapping_sizer->Add(text1,wxSizerFlags(0).Align(0).Expand().Border(wxALL) );*/

	wxConfigBase *pConfig = wxConfigBase::Get();
	pConfig->Read(wxT("raise"),&raise_setting);
	pConfig->Read(wxT("persist"),&persist_setting);
	pConfig->Read(wxT("ctrl"),&ctrl_setting);
	pConfig->Read(wxT("rendering"),&rendering_setting);
	pConfig->Read(wxT("hinting"),&hinting_setting);

	wxCheckBox *check1 = new wxCheckBox (this, wxID_ANY,
		_T("Put the window at the top of your desktop after each plot (raise)"),
		wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&raise_setting));
	wxCheckBox *check2 = new wxCheckBox (this, wxID_ANY,
		_T("Don't quit until all windows are closed (persist)"),
		wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&persist_setting));
	wxCheckBox *check3 = new wxCheckBox (this, wxID_ANY,
		_T("Replace 'q' by <ctrl>+'q' and <space> by <ctrl>+<space> (ctrl)"),
		wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&ctrl_setting));

	wxString choices[3];
	choices[0] = wxT("No antialiasing");
	choices[1] = wxT("Antialiasing");
	choices[2] = wxT("Antialiasing and oversampling");

	wxStaticBox *sb2 = new wxStaticBox( this, wxID_ANY,
		_T("Rendering options (applied to the next plot)"),
		wxDefaultPosition, wxDefaultSize );
	wxStaticBoxSizer *box_sizer2 = new wxStaticBoxSizer( sb2, wxVERTICAL );

	wxStaticText *text_rendering = new wxStaticText(this, wxID_ANY,
		wxT("Rendering method :"));
	wxChoice *box = new wxChoice (this, Config_Rendering, wxDefaultPosition, wxDefaultSize,
		3, choices, 0, wxGenericValidator(&rendering_setting));

	text_hinting = new wxStaticText(this, wxID_ANY,
		wxT("Hinting (100=full,0=none) :"));

	slider = new wxSlider(this, wxID_ANY, 0, 0, 100,
		wxDefaultPosition, wxDefaultSize,
		wxSL_HORIZONTAL|wxSL_LABELS,
		wxGenericValidator(&hinting_setting));

	if (rendering_setting != 2) {
		slider->Enable(false);
		text_hinting->Enable(false);
	}
	box_sizer2->Add(text_rendering,wxSizerFlags().Align(0).Border(wxALL));
	box_sizer2->Add(box,wxSizerFlags().Align(0).Border(wxALL));
	box_sizer2->Add(text_hinting,wxSizerFlags().Align(0).Expand().Border(wxALL));
	box_sizer2->Add(slider,wxSizerFlags().Align(0).Expand().Border(wxALL));

	wxBoxSizer *hsizer = new wxBoxSizer( wxHORIZONTAL );
	hsizer->Add( new wxButton(this, Config_OK, wxT("OK")),
		wxSizerFlags().Align(0).Expand().Border(wxALL));
	hsizer->Add( new wxButton(this, Config_APPLY, wxT("Apply")),
		wxSizerFlags().Align(0).Expand().Border(wxALL));
	hsizer->Add( new wxButton(this, Config_CANCEL, wxT("Cancel")),
		wxSizerFlags().Align(0).Expand().Border(wxALL));

	wxBoxSizer *vsizer = new wxBoxSizer( wxVERTICAL );
	vsizer->Add(check1,wxSizerFlags().Align(0).Expand().Border(wxALL));
	vsizer->Add(check2,wxSizerFlags().Align(0).Expand().Border(wxALL));
	vsizer->Add(check3,wxSizerFlags().Align(0).Expand().Border(wxALL));
	vsizer->Add(box_sizer2,wxSizerFlags().Align(0).Expand().Border(wxALL));
	/*vsizer->Add(CreateButtonSizer(wxOK|wxCANCEL),wxSizerFlags().Align(0).Expand().Border(wxALL));*/
	vsizer->Add(hsizer,wxSizerFlags().Align(0).Expand().Border(wxALL));

	/* use the sizer for layout */
	SetSizer( vsizer );
	/* set size hints to honour minimum size */
	vsizer->SetSizeHints( this );
}

/* enable or disable the hinting slider depending on the selection of the oversampling method */
void wxtConfigDialog::OnRendering( wxCommandEvent& event )
{
	if (event.GetInt() != 2) {
		slider->Enable(false);
		text_hinting->Enable(false);
	} else {
		slider->Enable(true);
		text_hinting->Enable(true);
	}
}

/* ------------------------------------------------------
 * functions that are called by gnuplot
 * ------------------------------------------------------*/

/* "Called once, when the device is first selected."
 * Is the 'main' function of the terminal. */
void wxt_init()
{
	FPRINTF((stderr,"Init\n"));

	if ( wxt_abort_init ) {
		fprintf(stderr,"Previous attempt to initialize wxWidgets has failed. Not retrying.\n");
		return;
	}

	wxt_sigint_init();

	if ( wxt_status == STATUS_UNINITIALIZED ) {
		FPRINTF((stderr,"First Init\n"));

#ifdef __WXMSW__
		/* the following is done in wxEntry() with wxMSW only */
		wxSetInstance(GetModuleHandle(NULL));
		wxApp::m_nCmdShow = SW_SHOW;
#endif

		if (!wxInitialize()) {
			fprintf(stderr,"Failed to initialize wxWidgets.\n");
			wxt_abort_init = true;
			return;
		}

		/* app initialization */
		wxTheApp->CallOnInit();

#ifdef __WXGTK__
		/* Three commands to create the thread and run it.
		 * We do this at first init only.
		 * If the user sets another terminal and goes back to wxt,
		 * the gui thread is already in action. */
		thread = new wxtThread();
		thread->Create();
		thread->Run();
#elif defined (__WXMSW__)
/* nothing to do */
#else
# error "Not implemented."
#endif /*__WXMSW__*/


#ifdef USE_MOUSE
		/* initialize the gnuplot<->terminal event system state */
		wxt_change_thread_state(RUNNING);
#endif /*USE_MOUSE*/

 		FPRINTF((stderr,"First Init2\n"));
#ifdef HAVE_LOCALE_H
		/* when wxGTK is initialised, GTK+ also sets the locale of the program itself;
		 * we must revert it */
		setlocale(LC_NUMERIC, "C");
#endif /*have_locale_h*/
	}

	wxt_sigint_check();

	/* try to find the requested window in the list of existing ones */
	wxt_current_window = wxt_findwindowbyid(wxt_window_number);

	/* open a new plot window if it does not exist */
	if ( wxt_current_window == NULL ) {
		FPRINTF((stderr,"opening a new plot window\n"));

		wxt_MutexGuiEnter();
		wxt_window_t window;
		window.id = wxt_window_number;
		/* create a new plot window and show it */
		wxString title;
		if (strlen(wxt_title))
			/* NOTE : this assumes that the title is encoded in the locale charset.
			 * This is probably a good assumption, but it is not guaranteed !
			 * May be improved by using gnuplot encoding setting. */
			title << wxString(wxt_title, wxConvLocal);
		else
			title.Printf(wxT("Gnuplot (window id : %d)"), window.id);
		window.frame = new wxtFrame( title, window.id, 50, 50, 640, 480 );
		window.frame->Show(true);
		FPRINTF((stderr,"new plot window opened\n"));
		/* make the panel able to receive keyboard input */
		window.frame->panel->SetFocus();
		/* set the default crosshair cursor */
		window.frame->panel->SetCursor(wxt_cursor_cross);
		/* creating the true context (at initialization, it may be a fake one).
		 * Note : the frame must be shown for this to succeed */
		if (!window.frame->panel->plot.success)
			window.frame->panel->wxt_cairo_create_context();
		wxt_MutexGuiLeave();
		/* store the plot structure in the list and keep shortcuts */
		wxt_window_list.push_back(window);
		wxt_current_window = &(wxt_window_list.back());
	}

	/* initialize helper pointers */
	wxt_current_panel = wxt_current_window->frame->panel;
	wxt_current_plot = &(wxt_current_panel->plot);
	wxt_current_command_list = &(wxt_current_panel->command_list);

	wxt_sigint_check();

	bool raise_setting;
       bool persist_setting;
	bool ctrl_setting;
	int rendering_setting;
	int hinting_setting;

	/* if needed, restore the setting from the config file/registry keys.
	 * Unset values are set to default reasonable values. */
	wxConfigBase *pConfig = wxConfigBase::Get();

	if (!pConfig->Read(wxT("raise"), &raise_setting)) {
		pConfig->Write(wxT("raise"), true);
		raise_setting = true;
	}
	if (wxt_raise==UNSET)
		wxt_raise = raise_setting?yes:no;

	if (!pConfig->Read(wxT("persist"), &persist_setting))
		pConfig->Write(wxT("persist"), false);

	if (!pConfig->Read(wxT("ctrl"), &ctrl_setting)) {
		pConfig->Write(wxT("ctrl"), false);
		ctrl_setting = false;
	}
	if (wxt_ctrl==UNSET)
		wxt_ctrl = ctrl_setting?yes:no;

	if (!pConfig->Read(wxT("rendering"), &rendering_setting)) {
		pConfig->Write(wxT("rendering"), 2);
		rendering_setting = 2;
	}
	switch (rendering_setting) {
	case 0 :
		wxt_current_plot->antialiasing = FALSE;
		wxt_current_plot->oversampling = FALSE;
		break;
	case 1 :
		wxt_current_plot->antialiasing = TRUE;
		wxt_current_plot->oversampling = FALSE;
		break;
	case 2 :
	default :
		wxt_current_plot->antialiasing = TRUE;
		wxt_current_plot->oversampling = TRUE;
		break;
	}

	if (!pConfig->Read(wxT("hinting"), &hinting_setting)) {
		pConfig->Write(wxT("hinting"), 100);
		hinting_setting = 100;
	}
	wxt_current_plot->hinting = hinting_setting;

	/* accept the following commands from gnuplot */
	wxt_status = STATUS_OK;
	wxt_current_plot->interrupt = FALSE;

	wxt_sigint_check();
	wxt_sigint_restore();

	FPRINTF((stderr,"Init finished \n"));
}


/* "Called just before a plot is going to be displayed."
 * Should clear the terminal. */
void wxt_graphics()
{
	if (wxt_status != STATUS_OK)
		return;

#ifdef DEBUG
	/* performance watch - to be removed */
	sw.Start();
#endif

	/* The sequence of gnuplot commands is critical as it involves mutexes.
	 * We replace the original interrupt handler with a custom one. */
	wxt_sigint_init();

	/* update the window scale factor first, cairo needs it */
	wxt_current_plot->xscale = 1.0;
	wxt_current_plot->yscale = 1.0;

	/* apply the queued rendering settings */
	wxt_current_panel->wxt_settings_apply();

	FPRINTF((stderr,"Graphics1\n"));

	wxt_MutexGuiEnter();
	/* set the transformation matrix of the context, and other details */
	/* depends on plot->xscale and plot->yscale */
	gp_cairo_initialize_context(wxt_current_plot);

	/* set or refresh terminal size according to the window size */
	/* oversampling_scale is updated in gp_cairo_initialize_context */
	term->xmax = (unsigned int) wxt_current_plot->device_xmax*wxt_current_plot->oversampling_scale;
	term->ymax = (unsigned int) wxt_current_plot->device_ymax*wxt_current_plot->oversampling_scale;
	wxt_current_plot->xmax = term->xmax;
	wxt_current_plot->ymax = term->ymax;
	/* initialize encoding */
	wxt_current_plot->encoding = encoding;

	wxt_MutexGuiLeave();

	/* set font details (hchar, vchar, h_tic, v_tic) according to settings */
	wxt_set_font("");

	/* clear the command list, and free the allocated memory */
	wxt_current_panel->ClearCommandlist();

	wxt_sigint_check();
	wxt_sigint_restore();

	FPRINTF((stderr,"Graphics time %d xmax %d ymax %d v_char %d h_char %d\n",
		sw.Time(), term->xmax, term->ymax, term->v_char, term->h_char));
}

void wxt_text()
{
	if (wxt_status != STATUS_OK) {
#ifdef USE_MOUSE
		/* Inform gnuplot that we have finished plotting.
		 * This avoids to lose the mouse after a interrupt.
		 * Do this immediately, instead of posting an event which may not be processed. */
		event_plotdone();
#endif
		return;
	}

	wxt_sigint_init();

	FPRINTF((stderr,"Text0 %d\n", sw.Time())); /*performance watch*/

	/* translates the command list to a bitmap */
	wxt_MutexGuiEnter();
	wxt_current_panel->wxt_cairo_refresh();
	wxt_MutexGuiLeave();

	wxt_sigint_check();

	/* raise the window, conditionnaly to the user choice */
	wxt_MutexGuiEnter();
	wxt_raise_window(wxt_current_window,false);
	wxt_MutexGuiLeave();

	FPRINTF((stderr,"Text2 %d\n", sw.Time())); /*performance watch*/

#ifdef USE_MOUSE
	/* Inform gnuplot that we have finished plotting */
	wxt_exec_event(GE_plotdone, 0, 0, 0, 0, wxt_window_number );
#endif /*USE_MOUSE*/

	wxt_sigint_check();
	wxt_sigint_restore();

	FPRINTF((stderr,"Text finished %d\n", sw.Time())); /*performance watch*/
}

void wxt_reset()
{
	/* sent when gnuplot exits and when the terminal or the output change.*/
	FPRINTF((stderr,"wxt_reset\n"));

	if (wxt_status == STATUS_UNINITIALIZED)
		return;

#ifdef USE_MOUSE
	if (wxt_status == STATUS_INTERRUPT) {
		/* send "reset" event to restore the mouse system in a well-defined state.
		 * Send it directly, not with wxt_exec_event(), which would only enqueue it,
		 * but not process it. */
		FPRINTF((stderr,"send reset event to the mouse system\n"));
		event_reset((gp_event_t *)1);   /* cancel zoombox etc. */

		/* clear the event list */
		wxt_clear_event_list();
	}

	/* stop sending mouse events */
	FPRINTF((stderr,"change thread state\n"));
	wxt_change_thread_state(RUNNING);
#endif /*USE_MOUSE*/

	FPRINTF((stderr,"wxt_reset ends\n"));
}

void wxt_move(unsigned int x, unsigned int y)
{
	if (wxt_status != STATUS_OK)
		return;

	gp_command temp_command;

	temp_command.command = command_move;
	temp_command.x1 = x;
	temp_command.y1 = term->ymax - y;

	wxt_command_push(temp_command);
}

void wxt_vector(unsigned int x, unsigned int y)
{
	if (wxt_status != STATUS_OK)
		return;

	gp_command temp_command;

	temp_command.command = command_vector;
	temp_command.x1 = x;
	temp_command.y1 = term->ymax - y;

	wxt_command_push(temp_command);
}

void wxt_put_text(unsigned int x, unsigned int y, const char * string)
{
	if (wxt_status != STATUS_OK)
		return;

	gp_command temp_command;

	/* note : this test must be here, not when processing the command list,
	 * because the user may have toggled the terminal option between two window resizing.*/
	/* if ignore_enhanced_text is set, draw with the normal routine.
	 * This is meant to avoid enhanced syntax when the enhanced mode is on */
	if (wxt_enhanced_enabled && !ignore_enhanced_text)
		temp_command.command = command_enhanced_put_text;
	else
		temp_command.command = command_put_text;

	temp_command.x1 = x;
	temp_command.y1 = term->ymax - y;
	/* Note : we must take '\0' (EndOfLine) into account */
	temp_command.string = new char[strlen(string)+1];
	strcpy(temp_command.string, string);

	wxt_command_push(temp_command);
}

void wxt_linetype(int lt)
{
	if (wxt_status != STATUS_OK)
		return;

	gp_command temp_command;
	gp_command temp_command2;

	temp_command.command = command_color;
	temp_command.color = gp_cairo_linetype2color( lt );

	temp_command2.command = command_linestyle;
	if (lt == -1)
		temp_command2.integer_value = GP_CAIRO_DASH;
	else
		temp_command2.integer_value = GP_CAIRO_SOLID;

	wxt_command_push(temp_command);
	wxt_command_push(temp_command2);
}


/* - fonts are selected as strings "name,size".
 * - _set_font("") restores the terminal's default font.*/
int wxt_set_font (const char *font)
{
	if (wxt_status != STATUS_OK)
		return 1;

	char fontname[MAX_ID_LEN + 1] = "";
	gp_command temp_command;
	int fontsize = 0;

	temp_command.command = command_set_font;

	if (!font || !(*font)) {
		strncpy(fontname, "", sizeof(fontname));
		fontsize = 0;
	} else {
		int sep;

		sep = strcspn(font,",");
		if (sep > 0) {
			strncpy(fontname, font, sep);
			fontname[sep] = '\0';
		}
		if (font[sep] == ',')
			sscanf(&(font[sep+1]), "%d", &fontsize);
	}

	wxt_sigint_init();
	wxt_MutexGuiEnter();

	if ( strlen(fontname) == 0 ) {
		if ( strlen(wxt_set_fontname) == 0 )
			strncpy(fontname, "Sans", sizeof(fontname));
		else
			strncpy(fontname, wxt_set_fontname, sizeof(fontname));
	}

	if ( fontsize == 0 ) {
		if ( wxt_set_fontsize == 0 )
			fontsize = 10;
		else
			fontsize = wxt_set_fontsize;
	}


	/* Reset the term variables (hchar, vchar, h_tic, v_tic).
	 * They may be taken into account in next plot commands */
	gp_cairo_set_font(wxt_current_plot, fontname, fontsize);
	gp_cairo_set_termvar(wxt_current_plot);
	wxt_MutexGuiLeave();
	wxt_sigint_check();
	wxt_sigint_restore();

	/* Note : we must take '\0' (EndOfLine) into account */
	temp_command.string = new char[strlen(fontname)+1];
	strcpy(temp_command.string, fontname);
	temp_command.integer_value = fontsize;

	wxt_command_push(temp_command);
	/* the returned int is not used anywhere */
	return 1;
}
	

int wxt_justify_text(enum JUSTIFY mode)
{
	if (wxt_status != STATUS_OK)
		return 1;

	gp_command temp_command;

	temp_command.command = command_justify;
	temp_command.mode = mode;

	wxt_command_push(temp_command);
	return 1; /* we can justify */
}

void wxt_point(unsigned int x, unsigned int y, int pointstyle)
{
	if (wxt_status != STATUS_OK)
		return;

	gp_command temp_command;

	temp_command.command = command_point;
	temp_command.x1 = x;
	temp_command.y1 = term->ymax - y;
	temp_command.integer_value = pointstyle;

	wxt_command_push(temp_command);
}

void wxt_pointsize(double ptsize)
{
	if (wxt_status != STATUS_OK)
		return;

	/* same behaviour as x11 terminal */
	if (ptsize<0) ptsize = 1;

	gp_command temp_command;
	temp_command.command = command_pointsize;
	temp_command.double_value = ptsize;

	wxt_command_push(temp_command);
}

void wxt_linewidth(double lw)
{
	if (wxt_status != STATUS_OK)
		return;

	gp_command temp_command;

	temp_command.command = command_linewidth;
	temp_command.double_value = lw;

	wxt_command_push(temp_command);
}

int wxt_text_angle(int angle)
{
	if (wxt_status != STATUS_OK)
		return 1;

	gp_command temp_command;

	temp_command.command = command_text_angle;
	/* a double is needed to compute cos, sin, etc. */
	temp_command.double_value = (double) angle;

	wxt_command_push(temp_command);
	return 1; /* 1 means we can rotate */
}

void wxt_fillbox(int style, unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	if (wxt_status != STATUS_OK)
		return;

	gp_command temp_command;

	temp_command.command = command_fillbox;
	temp_command.x1 = x;
	temp_command.y1 = term->ymax - y;
	temp_command.x2 = width;
	temp_command.y2 = height;
	temp_command.integer_value = style;

	wxt_command_push(temp_command);
}

int wxt_make_palette(t_sm_palette * palette)
{
	/* we can do continuous colors */
	return 0;
}

void wxt_set_color(t_colorspec *colorspec)
{
	if (wxt_status != STATUS_OK)
		return;

	rgb_color rgb1;
	gp_command temp_command;

	if (colorspec->type == TC_LT) {
		wxt_linetype(colorspec->lt);
		return;
	} else if (colorspec->type == TC_FRAC)
		rgb1_from_gray( colorspec->value, &rgb1 );
	else if (colorspec->type == TC_RGB) {
		rgb1.r = (double) ((colorspec->lt >> 16) & 0xff)/255;
		rgb1.g = (double) ((colorspec->lt >> 8) & 0xff)/255;
		rgb1.b = (double) ((colorspec->lt) & 0xff)/255;
	} else return;

	temp_command.command = command_color;
	temp_command.color = rgb1;

	wxt_command_push(temp_command);
}


/* here we send the polygon command */
void wxt_filled_polygon(int n, gpiPoint *corners)
{
	if (wxt_status != STATUS_OK)
		return;

	gp_command temp_command;

	temp_command.command = command_filled_polygon;
	temp_command.integer_value = n;
	temp_command.corners = new gpiPoint[n];
	/* can't use memcpy() here, as we have to mirror the y axis */
	gpiPoint *corners_copy = temp_command.corners;
	while (corners_copy < (temp_command.corners + n)) {
		*corners_copy = *corners++;
		corners_copy->y = term->ymax - corners_copy->y;
		++corners_copy;
	}

	wxt_command_push(temp_command);
}

#ifdef WITH_IMAGE
void wxt_image(unsigned M, unsigned N, coordval * image, gpiPoint * corner, t_imagecolor color_mode)
{
	/* This routine is to plot a pixel-based image on the display device.
	'M' is the number of pixels along the y-dimension of the image and
	'N' is the number of pixels along the x-dimension of the image.  The
	coordval pointer 'image' is the pixel values normalized to the range
	[0:1].  These values should be scaled accordingly for the output
	device.  They 'image' data starts in the upper left corner and scans
	along rows finishing in the lower right corner.  If 'color_mode' is
	IC_PALETTE, the terminal is to use palette lookup to generate color
	information.  In this scenario the size of 'image' is M*N.  If
	'color_mode' is IC_RGB, the terminal is to use RGB components.  In
	this scenario the size of 'image' is 3*M*N.  The data appears in RGB
	tripples, i.e., image[0] = R(1,1), image[1] = G(1,1), image[2] =
	B(1,1), image[3] = R(1,2), image[4] = G(1,2), ..., image[3*M*N-1] =
	B(M,N).  The 'image' is actually an "input" image in the sense that
	it must also be properly resampled for the output device.  Many output
	mediums, e.g., PostScript, do this work via various driver functions.
	To determine the appropriate rescaling, the 'corner' information
	should be used.  There are four entries in the gpiPoint data array.
	'corner[0]' is the upper left corner (in terms of plot location) of
	the outer edge of the image.  Similarly, 'corner[1]' is the lower
	right corner of the outer edge of the image.  (Outer edge means the
	outer extent of the corner pixels, not the middle of the corner
	pixels.)  'corner[2]' is the upper left corner of the visible part
	of the image, and 'corner[3]' is the lower right corner of the visible
	part of the image.  The information is provided in this way because
	often it is necessary to clip a portion of the outer pixels of the
	image. */

	/* we will draw an image, scale and resize it, copy to bitmap,
	 * give a pointer to it in the list, and when processing the list we will use DrawBitmap */
	/* FIXME add palette support ??? */

	if (wxt_status != STATUS_OK)
		return;

	int imax;
	gp_command temp_command;

	temp_command.command = command_image;
	temp_command.x1 = corner[0].x;
	temp_command.y1 = term->ymax - corner[0].y;
	temp_command.x2 = corner[1].x;
	temp_command.y2 = term->ymax - corner[1].y;
	temp_command.x3 = corner[2].x;
	temp_command.y3 = term->ymax - corner[2].y;
	temp_command.x4 = corner[3].x;
	temp_command.y4 = term->ymax - corner[3].y;
	temp_command.integer_value = M;
	temp_command.integer_value2 = N;
	temp_command.color_mode = color_mode;	

	if (color_mode == IC_RGB)
		imax = 3*M*N;
	else
		imax = M*N;

	temp_command.image = new coordval[imax];
	memcpy(temp_command.image, image, imax*sizeof(coordval));

	wxt_command_push(temp_command);
}
#endif /*WITH_IMAGE*/

#ifdef USE_MOUSE
/* Display temporary text, after
 * erasing any temporary text displayed previously at this location.
 * The int determines where: 0=statusline, 1,2: at corners of zoom
 * box, with \r separating text above and below the point. */
void wxt_put_tmptext(int n, const char str[])
{
	if (wxt_status == STATUS_UNINITIALIZED)
		return;

	wxt_sigint_init();
	wxt_MutexGuiEnter();

	switch ( n ) {
	case 0:
		wxt_current_window->frame->SetStatusText( wxString(str, wxConvLocal) );
		break;
	case 1:
		wxt_current_panel->zoom_x1 = wxt_current_panel->mouse_x;
		wxt_current_panel->zoom_y1 = wxt_current_panel->mouse_y;
		wxt_current_panel->zoom_string1 =  wxString(str, wxConvLocal);
		break;
	case 2:
		if ( strlen(str)==0 )
			wxt_current_panel->wxt_zoombox = false;
		else {
			wxt_current_panel->wxt_zoombox = true;
			wxt_current_panel->zoom_string2 =  wxString(str, wxConvLocal);
		}
		wxt_current_panel->Draw();
		break;
	default :
		break;
	}

	wxt_MutexGuiLeave();
	wxt_sigint_check();
	wxt_sigint_restore();
}

/* c selects the action:
 * -4=don't draw (erase) line between ruler and current mouse position,
 * -3=draw line between ruler and current mouse position,
 * -2=warp the cursor to the given point,
 * -1=start zooming,
 * 0=standard cross-hair cursor,
 * 1=cursor during rotation,
 * 2=cursor during scaling,
 * 3=cursor during zooming. */
void wxt_set_cursor(int c, int x, int y)
{
	if (wxt_status == STATUS_UNINITIALIZED)
		return;

	wxt_sigint_init();
	wxt_MutexGuiEnter();

	switch ( c ) {
	case -4:
		wxt_current_panel->wxt_ruler_lineto = false;
		wxt_current_panel->Draw();
		break;
	case -3:
		wxt_current_panel->wxt_ruler_lineto = true;
		wxt_current_panel->Draw();
		break;
	case -2: /* warp the pointer to the given position */
		wxt_current_panel->WarpPointer(
				(int) device_x(wxt_current_plot, x),
				(int) device_y(wxt_current_plot, y) );
		break;
	case -1: /* start zooming */
		wxt_current_panel->SetCursor(wxt_cursor_right);
		break;
	case 0: /* cross-hair cursor, also cancel zoombox when Echap is pressed */
		wxt_current_panel->wxt_zoombox = false;
		wxt_current_panel->SetCursor(wxt_cursor_cross);
		wxt_current_panel->Draw();
		break;
	case 1: /* rotation */
		wxt_current_panel->SetCursor(wxt_cursor_rotate);
		break;
	case 2: /* scaling */
		wxt_current_panel->SetCursor(wxt_cursor_size);
		break;
	case 3: /* zooming */
		wxt_current_panel->SetCursor(wxt_cursor_right);
		break;
	default:
		wxt_current_panel->SetCursor(wxt_cursor_cross);
		break;
	}

	wxt_MutexGuiLeave();
	wxt_sigint_check();
	wxt_sigint_restore();
}


/* Draw a ruler (crosshairs) centered at the
 * indicated screen coordinates.  If x<0, switch ruler off. */
void wxt_set_ruler(int x, int y)
{
	if (wxt_status == STATUS_UNINITIALIZED)
		return;

	wxt_sigint_init();
	wxt_MutexGuiEnter();

	if (x<0) {
		wxt_current_panel->wxt_ruler = false;
		wxt_current_panel->Draw();
	} else {
		wxt_current_panel->wxt_ruler = true;
		wxt_current_panel->wxt_ruler_x = device_x(wxt_current_plot, x);
		wxt_current_panel->wxt_ruler_y = device_y(wxt_current_plot, y);
		wxt_current_panel->Draw();
	}

	wxt_MutexGuiLeave();
	wxt_sigint_check();
	wxt_sigint_restore();
}

/* Write a string to the clipboard */
void wxt_set_clipboard(const char s[])
{
	if (wxt_status == STATUS_UNINITIALIZED)
		return;

	wxt_sigint_init();
	wxt_MutexGuiEnter();

	if (wxTheClipboard->Open()) {
		wxTheClipboard->SetData( new wxTextDataObject(wxString(s, wxConvLocal)) );
		wxTheClipboard->Flush();
		wxTheClipboard->Close();
	}

	wxt_MutexGuiLeave();
	wxt_sigint_check();
	wxt_sigint_restore();
}
#endif /*USE_MOUSE*/


/* ===================================================================
 * Command list processing
 * =================================================================*/

/* push a command in the current commands list */
void wxt_command_push(gp_command command)
{
	wxt_sigint_init();
	wxt_current_panel->command_list_mutex.Lock();
	wxt_current_command_list->push_back(command);
	wxt_current_panel->command_list_mutex.Unlock();
	wxt_sigint_check();
	wxt_sigint_restore();
}

/* refresh the plot by (re)processing the plot commands list */
void wxtPanel::wxt_cairo_refresh()
{
	/* Clear background. */
	gp_cairo_clear(&plot);

	command_list_t::iterator wxt_iter; /*declare the iterator*/
	for(wxt_iter = command_list.begin(); wxt_iter != command_list.end(); ++wxt_iter) {
		if (wxt_status == STATUS_INTERRUPT_ON_NEXT_CHECK) {
			FPRINTF((stderr,"interrupt detected inside drawing loop\n"));
#ifdef IMAGE_SURFACE
			wxt_cairo_create_bitmap();
#endif /* IMAGE_SURFACE */
			/* draw the pixmap to the screen */
			Draw();
			return;
		}
		wxt_cairo_exec_command( *wxt_iter );
	}

	/* don't forget to stroke the last path if vector was the last command */
	gp_cairo_stroke(&plot);
	/* and don't forget to draw the polygons if draw_polygon was the last command */
	gp_cairo_end_polygon(&plot);

/* the following is a test for a bug in cairo when drawing to a gdkpixmap */
#if 0
	cairo_set_source_rgb(plot.cr,1,1,1);
	cairo_paint(plot.cr);

	cairo_matrix_t matrix;
	cairo_matrix_init(&matrix,
			plot.xscale,
			0,
			0,
			plot.yscale,
			0,
			0);
	cairo_set_matrix(plot.cr, &matrix);

	cairo_t *context;
	cairo_surface_t *surface;
	surface = cairo_surface_create_similar(cairo_get_target(plot.cr),
                                             CAIRO_CONTENT_COLOR_ALPHA,
                                             plot.device_xmax,
                                             plot.device_ymax);
	context = cairo_create(surface);
	cairo_set_operator(context,CAIRO_OPERATOR_SATURATE);

	cairo_move_to(context, 300, 200);
	cairo_rel_line_to(context, 100, 0);
	cairo_rel_line_to(context, 0, 100);
	cairo_close_path(context);
	cairo_set_source_rgb(context,0,0,0);
	cairo_fill(context);

	cairo_move_to(context, 250, 170);
	cairo_rel_line_to(context, 100, 0);
	cairo_rel_line_to(context, 0, 100);
	cairo_close_path(context);
	cairo_set_source_rgb(context,0,0,0.5);
	cairo_fill(context);

	cairo_move_to(context, 360, 200);
	cairo_rel_line_to(context, 30, 0);
	cairo_rel_line_to(context, 0, -40);
	cairo_close_path(context);
	cairo_set_source_rgb(context,1,0,0);
	cairo_fill(context);

	cairo_move_to(context, 400, 100);
	cairo_rel_line_to(context, 100, 0);
	cairo_rel_line_to(context, -100, 300);
	cairo_close_path(context);
	cairo_set_source_rgb(context,0,1,0);
	cairo_fill(context);

	cairo_move_to(context, 400, 300);
	cairo_rel_line_to(context, -80, -80);
	cairo_rel_line_to(context, 0, 100);
	cairo_close_path(context);
	cairo_set_source_rgb(context,0.6,0.4,0);
	cairo_fill(context);

	cairo_pattern_t *pattern = cairo_pattern_create_for_surface( surface );
	cairo_destroy( context );

	cairo_surface_destroy( surface );
	cairo_set_source( plot.cr, pattern );
	cairo_pattern_destroy( pattern );
	cairo_paint(plot.cr);

	cairo_matrix_init(&matrix,
			plot.xscale/plot.oversampling_scale,
			0,
			0,
			plot.yscale/plot.oversampling_scale,
			0,
			0);
	cairo_set_matrix(plot.cr, &matrix);
#endif

#ifdef IMAGE_SURFACE
	wxt_cairo_create_bitmap();
#endif /* !have_gtkcairo */

	/* draw the pixmap to the screen */
	Draw();
	FPRINTF((stderr,"commands done, number of commands %d\n", command_list.size()));
}


void wxtPanel::wxt_cairo_exec_command(gp_command command)
{
	switch ( command.command ) {
	case command_color :
		gp_cairo_set_color(&plot,command.color);
		return;
	case command_filled_polygon :
		gp_cairo_draw_polygon(&plot, command.integer_value, command.corners);
		return;
	case command_move :
		gp_cairo_move(&plot, command.x1, command.y1);
		return;
	case command_vector :
		gp_cairo_vector(&plot, command.x1, command.y1);
		return;
	case command_linestyle :
		gp_cairo_set_linestyle(&plot, command.integer_value);
		return;
	case command_linetype :
		gp_cairo_set_linetype(&plot, command.integer_value);
		return;
	case command_pointsize :
		gp_cairo_set_pointsize(&plot, command.double_value);
		return;
	case command_point :
		gp_cairo_draw_point(&plot, command.x1, command.y1, command.integer_value);
		return;
	case command_justify :
		gp_cairo_set_justify(&plot,command.mode);
		return;
	case command_put_text :
		gp_cairo_draw_text(&plot, command.x1, command.y1, command.string);
		return;
	case command_enhanced_put_text :
		gp_cairo_draw_enhanced_text(&plot, command.x1, command.y1, command.string);
		return;
	case command_set_font :
		gp_cairo_set_font(&plot, command.string, command.integer_value);
		return;
	case command_linewidth :
		gp_cairo_set_linewidth(&plot, command.double_value);;
		return;
	case command_text_angle :
		gp_cairo_set_textangle(&plot, command.double_value);
		return;
	case command_fillbox :
		gp_cairo_draw_fillbox(&plot, command.x1, command.y1,
					command.x2, command.y2,
					command.integer_value);
		return;
#ifdef WITH_IMAGE
	case command_image :
		gp_cairo_draw_image(&plot, command.image,
				command.x1, command.y1,
				command.x2, command.y2,
				command.x3, command.y3,
				command.x4, command.y4,
				command.integer_value, command.integer_value2,
				command.color_mode);
		return;
#endif /*WITH_IMAGE*/
	}
}


/* given a plot number (id), return the associated plot structure */
wxt_window_t* wxt_findwindowbyid(wxWindowID id)
{
	size_t i;
	for(i=0;i<wxt_window_list.size();++i) {
		if (wxt_window_list[i].id == id)
			return &(wxt_window_list[i]);
	}
	return NULL;
}

/*----------------------------------------------------------------------------
 *   raise-lower functions
 *----------------------------------------------------------------------------*/

void wxt_raise_window(wxt_window_t* window, bool force)
{
	FPRINTF((stderr,"raise window\n"));

	window->frame->Show(true);

	if (wxt_raise != no||force) {
#ifdef USE_GTK
		/* Raise() in wxGTK call wxTopLevelGTK::Raise()
		 * which also gives the focus to the window.
		 * Refresh() also must be called, otherwise
		 * the raise won't happen immediately */
		window->frame->panel->Refresh(false);
		gdk_window_raise(window->frame->GetHandle()->window);
#else
		window->frame->Raise();
#endif /*USE_GTK */
	}
}


void wxt_lower_window(wxt_window_t* window)
{
#ifdef USE_GTK
	window->frame->panel->Refresh(false);
	gdk_window_lower(window->frame->GetHandle()->window);
#else
	window->frame->Lower();
#endif /* USE_GTK */
}


/* raise the plot with given number */
void wxt_raise_terminal_window(int number)
{
	wxt_window_t *window;

	if (wxt_status != STATUS_OK)
		return;

	wxt_sigint_init();

	wxt_MutexGuiEnter();
	if ((window = wxt_findwindowbyid(number))) {
		FPRINTF((stderr,"wxt : raise window %d\n",number));
		window->frame->Show(true);
		wxt_raise_window(window,true);
	}
	wxt_MutexGuiLeave();

	wxt_sigint_check();
	wxt_sigint_restore();
}

/* raise the plot the whole group */
void wxt_raise_terminal_group()
{
	/* declare the iterator */
	std::vector<wxt_window_t>::iterator wxt_iter;

	if (wxt_status != STATUS_OK)
		return;

	wxt_sigint_init();

	wxt_MutexGuiEnter();
	for(wxt_iter = wxt_window_list.begin(); wxt_iter != wxt_window_list.end(); wxt_iter++) {
		FPRINTF((stderr,"wxt : raise window %d\n",wxt_iter->id));
		wxt_iter->frame->Show(true);
		/* FIXME Why does wxt_iter doesn't work directly ? */
		wxt_raise_window(&(*wxt_iter),true);
	}
	wxt_MutexGuiLeave();

	wxt_sigint_check();
	wxt_sigint_restore();
}

/* lower the plot with given number */
void wxt_lower_terminal_window(int number)
{
	wxt_window_t *window;

	if (wxt_status != STATUS_OK)
		return;

	wxt_sigint_init();

	wxt_MutexGuiEnter();
	if ((window = wxt_findwindowbyid(number))) {
		FPRINTF((stderr,"wxt : lower window %d\n",number));
		wxt_lower_window(window);
	}
	wxt_MutexGuiLeave();

	wxt_sigint_check();
	wxt_sigint_restore();
}

/* lower the plot the whole group */
void wxt_lower_terminal_group()
{
	/* declare the iterator */
	std::vector<wxt_window_t>::iterator wxt_iter;

	if (wxt_status != STATUS_OK)
		return;

	wxt_sigint_init();

	wxt_MutexGuiEnter();
	for(wxt_iter = wxt_window_list.begin(); wxt_iter != wxt_window_list.end(); wxt_iter++) {
		FPRINTF((stderr,"wxt : lower window %d\n",wxt_iter->id));
		wxt_lower_window(&(*wxt_iter));
	}
	wxt_MutexGuiLeave();

	wxt_sigint_check();
	wxt_sigint_restore();
}

/* close the specified window */
void wxt_close_terminal_window(int number)
{
	wxt_window_t *window;

	if (wxt_status != STATUS_OK)
		return;

	wxt_sigint_init();

	wxt_MutexGuiEnter();
	if ((window = wxt_findwindowbyid(number))) {
		FPRINTF((stderr,"wxt : close window %d\n",number));
		window->frame->Close(false);
	}
	wxt_MutexGuiLeave();

	wxt_sigint_check();
	wxt_sigint_restore();
}

/* update the window title */
void wxt_update_title(int number)
{
	wxt_window_t *window;
	wxString title;

	if (wxt_status != STATUS_OK)
		return;

	wxt_sigint_init();

	wxt_MutexGuiEnter();

	if ((window = wxt_findwindowbyid(number))) {
		FPRINTF((stderr,"wxt : close window %d\n",number));
		if (strlen(wxt_title)) {
			/* NOTE : this assumes that the title is encoded in the locale charset.
				* This is probably a good assumption, but it is not guaranteed !
				* May be improved by using gnuplot encoding setting. */
			title << wxString(wxt_title, wxConvLocal);
		} else
			title.Printf(wxT("Gnuplot (window id : %d)"), window->id);

		window->frame->SetTitle(title);
	}

	wxt_MutexGuiLeave();

	wxt_sigint_check();
	wxt_sigint_restore();
}

/* --------------------------------------------------------
 * Cairo stuff
 * --------------------------------------------------------*/

void wxtPanel::wxt_cairo_create_context()
{
	cairo_surface_t *fake_surface;

	if ( plot.cr )
		cairo_destroy(plot.cr);

	if ( wxt_cairo_create_platform_context() ) {
		/* we are not able to create a true cairo context,
		 * but will create a fake one to give proper initialisation */
		FPRINTF((stderr,"creating temporary fake surface\n"));
		fake_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
		plot.cr = cairo_create( fake_surface );
		cairo_surface_destroy( fake_surface );
		/* this flag will make the program retry later */
		plot.success = FALSE;
	} else {
		plot.success = TRUE;
	}

	/* set the transformation matrix of the context, and other details */
	gp_cairo_initialize_context(&plot);
}

void wxtPanel::wxt_cairo_free_context()
{
	if (plot.cr)
		cairo_destroy(plot.cr);

	if (plot.success)
		wxt_cairo_free_platform_context();
}


#ifdef GTK_SURFACE
/* create a cairo context, where the plot will be drawn on
 * If there is an error, return 1, otherwise return 0 */
int wxtPanel::wxt_cairo_create_platform_context()
{
	cairo_surface_t *surface;
	wxClientDC dc(this);

	FPRINTF((stderr,"wxt_cairo_create_context\n"));

	/* free gdkpixmap */
	wxt_cairo_free_platform_context();
	
	/* GetWindow is a wxGTK specific wxDC method that returns
	 * the GdkWindow on which painting should be done */

	if ( !GDK_IS_DRAWABLE(dc.GetWindow()) )
		return 1;


	gdkpixmap = gdk_pixmap_new(dc.GetWindow(), plot.device_xmax, plot.device_ymax, -1);

	if ( !GDK_IS_DRAWABLE(gdkpixmap) )
		return 1;

	plot.cr = gdk_cairo_create(gdkpixmap);
	return 0;
}

void wxtPanel::wxt_cairo_free_platform_context()
{
	if (gdkpixmap)
		g_object_unref(gdkpixmap);
}

#elif defined(__WXMSW__)
/* create a cairo context, where the plot will be drawn on
 * If there is an error, return 1, otherwise return 0 */
int wxtPanel::wxt_cairo_create_platform_context()
{
	cairo_surface_t *surface;
	wxClientDC dc(this);

	FPRINTF((stderr,"wxt_cairo_create_context\n"));

	/* free hdc and hbm */
	wxt_cairo_free_platform_context();

	/* GetHDC is a wxMSW specific wxDC method that returns
	 * the HDC on which painting should be done */

	/* Create a compatible DC. */
	hdc = CreateCompatibleDC( (HDC) dc.GetHDC() );

	if (!hdc)
		return 1;

	/* Create a bitmap big enough for our client rectangle. */
	hbm = CreateCompatibleBitmap((HDC) dc.GetHDC(), plot.device_xmax, plot.device_ymax);

	if ( !hbm )
		return 1;

	/* Select the bitmap into the off-screen DC. */
	SelectObject(hdc, hbm);
	surface = cairo_win32_surface_create( hdc );
	plot.cr = cairo_create(surface);
	cairo_surface_destroy( surface );
	return 0;
}

void wxtPanel::wxt_cairo_free_platform_context()
{
	if (hdc)
		DeleteDC(hdc);
	if (hbm)
		DeleteObject(hbm);
}

#else /* generic image surface */
/* create a cairo context, where the plot will be drawn on
 * If there is an error, return 1, otherwise return 0 */
int wxtPanel::wxt_cairo_create_platform_context()
{
	int width, height;
	cairo_surface_t *surface;

	FPRINTF((stderr,"wxt_cairo_create_context\n"));

	if (data32)
		delete[] data32;

	width = plot.device_xmax;
	height = plot.device_ymax;

	if (width<1||height<1)
		return 1;

	data32 = new unsigned int[width*height];

	surface = cairo_image_surface_create_for_data((unsigned char*) data32,
			CAIRO_FORMAT_ARGB32,  width, height, 4*width);
	plot.cr = cairo_create(surface);
	cairo_surface_destroy( surface );
	return 0;
}


/* create a wxBitmap (to be painted to the screen) from the buffered cairo surface. */
void wxtPanel::wxt_cairo_create_bitmap()
{
	int width, height;
	unsigned char *data24;
	wxImage *image;

	if (!data32)
		return;

	width = plot.device_xmax;
	height = plot.device_ymax;

	data24 = new unsigned char[3*width*height];

	/* data32 is the cairo image buffer, upper bits are alpha, then r, g and b
	 * Depends on endianess !
	 * It is converted to RGBRGB... in data24 */
	for(int i=0;i<width*height;++i) {
		*(data24+3*i)=*(data32+i)>>16;
		*(data24+3*i+1)=*(data32+i)>>8;
		*(data24+3*i+2)=*(data32+i);
	}

	/* create a wxImage from data24 */
	image = new wxImage(width, height, data24, true);

	if (cairo_bitmap)
		delete cairo_bitmap;

	/* create a wxBitmap from the wxImage. */
	cairo_bitmap = new wxBitmap( *image );

	/* free memory */
	delete image;
	delete[] data24;
}


void wxtPanel::wxt_cairo_free_platform_context()
{
	if (data32)
		delete[] data32;
	if (cairo_bitmap)
		delete cairo_bitmap;
}
#endif /* IMAGE_SURFACE */

/* --------------------------------------------------------
 * events handling
 * --------------------------------------------------------*/

/* Debugging events and _waitforinput adds a lot of lines of output
 * (~4 lines for an input character, and a few lines for each mouse move)
 * To debug it, define DEBUG and WXTDEBUGINPUT */

#ifdef WXTDEBUGINPUT
# define FPRINTF2(a) FPRINTF(a)
#else
# define FPRINTF2(a)
#endif

#ifdef USE_MOUSE
/* protected check for the state of the event list */
bool wxt_check_eventlist_empty()
{
	bool result;
	mutexProtectingEventList.Lock();
	result = EventList.empty();
	mutexProtectingEventList.Unlock();
	return result;
}

/* protected check for the state of the main thread (running or waiting for input) */
wxt_thread_state_t wxt_check_thread_state()
{
	wxt_thread_state_t result;
	mutexProtectingThreadState.Lock();
	result = wxt_thread_state;
	mutexProtectingThreadState.Unlock();
	return result;
}

/* multithread safe method to change thread state */
void wxt_change_thread_state(wxt_thread_state_t state)
{
	wxt_sigint_init();
	mutexProtectingThreadState.Lock();
	wxt_thread_state = state;
	mutexProtectingThreadState.Unlock();
	wxt_sigint_check();
	wxt_sigint_restore();
}

/* Similar to gp_exec_event(),
 * put the event sent by the terminal in a list,
 * to be processed by the main thread. */
void wxt_exec_event(int type, int mx, int my, int par1, int par2, wxWindowID id)
{
	struct gp_event_t event;

	/* add the event to the event list */
	event.type = type;
	event.mx = mx;
	event.my = my;
	event.par1 = par1;
	event.par2 = par2;
	event.winid = id;

#ifndef _Windows
	if (wxt_check_thread_state() == WAITING_FOR_STDIN)
#endif /* ! _Windows */
	{
		FPRINTF2((stderr,"Gui thread adds an event to the list\n"));
		mutexProtectingEventList.Lock();
		EventList.push_back(event);
		mutexProtectingEventList.Unlock();
	}
}


/* clear the event list, caring for the mutex */
void wxt_clear_event_list()
{
	mutexProtectingEventList.Lock();
	EventList.clear();
	mutexProtectingEventList.Unlock();
}


/* wxt_process_events will process the contents of the event list
 * until it is empty.
 * It will return 1 if one event ends the pause */
int wxt_process_events()
{
	struct gp_event_t wxt_event;
	int button;

	while ( !wxt_check_eventlist_empty() ) {
		FPRINTF2((stderr,"Processing event\n"));
		mutexProtectingEventList.Lock();
		wxt_event = EventList.front();
		EventList.pop_front();
		mutexProtectingEventList.Unlock();
		do_event( &wxt_event );
		FPRINTF2((stderr,"Event processed\n"));
		if (wxt_event.type == GE_buttonrelease && (paused_for_mouse & PAUSE_CLICK)) {
			button = wxt_event.par1;
			if (button == 1 && (paused_for_mouse & PAUSE_BUTTON1))
				paused_for_mouse = 0;
			if (button == 2 && (paused_for_mouse & PAUSE_BUTTON2))
				paused_for_mouse = 0;
			if (button == 3 && (paused_for_mouse & PAUSE_BUTTON3))
				paused_for_mouse = 0;
			if (paused_for_mouse == 0)
				return 1;
		}
		if (wxt_event.type == GE_keypress && (paused_for_mouse & PAUSE_KEYSTROKE)) {
			/* Ignore NULL keycode */
			if (wxt_event.par1 > '\0') {
				paused_for_mouse = 0;
				return 1;
			}
		}
	}
	return 0;
	
}

#ifdef __WXMSW__
/* Implements waitforinput used in wxt.trm
 * To avoid code duplication, the terminal events are directly
 * processed in TextGetCh(), which implements getch()
 * under Windows */
int wxt_waitforinput()
{
	return getch();
}

/* handles mouse events while the pause dialog is on,
 * if the wxWidgets is initialized and in use.
 * Used in wpause.c (PauseBox and win_sleep) .
 * It will return 1 if one event ends the pause */
int wxt_terminal_events()
{
	int result = 0;
	
	if (!strcmp(term->name,"wxt") && wxt_status == STATUS_OK)
		result = wxt_process_events();
		
	return 0;
}

#elif defined(__WXGTK__)

/* Implements waitforinput used in wxt.trm
 * Returns the next input charachter, meanwhile treats terminal events */
int wxt_waitforinput()
{
	/* wxt_waitforinput *is* launched immediately after the wxWidgets terminal
	 * is set using 'set term wxt' whereas wxt_init has not been called.
	 * So we must ensure that the library has been initialized
	 * before using any wxwidgets functions.
	 * When we just come back from SIGINT,
	 * we must process window events, so the check is not
	 * wxt_status != STATUS_OK */
	if (wxt_status == STATUS_UNINITIALIZED)
		return getc(stdin);

	int ierr;
	int fd = fileno(stdin);
	struct timeval timeout;
	fd_set fds;

	wxt_change_thread_state(WAITING_FOR_STDIN);

	do {
		if (wxt_process_events()) {
			wxt_change_thread_state(RUNNING);
			return '\0';
		}

		timeout.tv_sec = 0;
		timeout.tv_usec = 10000;
		FD_ZERO(&fds);
		FD_SET(0/*fd*/,&fds);

		ierr = select(1, &fds, NULL, NULL, &timeout);

		/* check for error on select and return immediately if any */
		if (ierr<0) {
			wxt_change_thread_state(RUNNING);
			return '\0';
		}
	} while (!FD_ISSET(fd,&fds));

	/* if we are paused_for_mouse, we should continue to wait for a mouse click */
	if (paused_for_mouse)
		while (true) {
			if (wxt_process_events()) {
				wxt_change_thread_state(RUNNING);
				return '\0';
			}
			/* wait 10 microseconds */
			wxMicroSleep(10);
		}

	wxt_change_thread_state(RUNNING);
	return getchar();
}
#else  /* !__WXMSW__ && !__WXGTK__ */
#error "Not implemented"
#endif

#endif /*USE_MOUSE*/

/* --------------------------------------------------------
 * 'persist' option handling
 * --------------------------------------------------------*/

/* returns true if at least one plot window is opened.
 * Used to handle 'persist' */
bool wxt_window_opened()
{
	std::vector<wxt_window_t>::iterator wxt_iter; /*declare the iterator*/

	wxt_MutexGuiEnter();
	for(wxt_iter = wxt_window_list.begin(); wxt_iter != wxt_window_list.end(); wxt_iter++) {
		if ( wxt_iter->frame->IsShown() ) {
			wxt_MutexGuiLeave();
			return true;
		}
	}
	wxt_MutexGuiLeave();
	return false;
}

/* Called when gnuplot exits.
 * Handle the 'persist' setting, ie will continue
 * to handle events and will return when
 * all the plot windows are closed. */
void wxt_atexit(TBOOLEAN persist)
{
	int i;
	int persist_setting;
#ifdef _Windows
	MSG msg;
#endif /*_Windows*/

	if (wxt_status == STATUS_UNINITIALIZED)
		return;

	/* first look for command_line setting */
	if (wxt_persist==UNSET && persist)
		wxt_persist = TRUE;

	wxConfigBase *pConfig = wxConfigBase::Get();

	/* then look for persistent configuration setting */
	if (wxt_persist==UNSET) {
		if (pConfig->Read(wxT("persist"),&persist_setting))
			wxt_persist = persist_setting?yes:no;
	}

	/* and let's go ! */
	if (wxt_persist==UNSET|| wxt_persist==no) {
		wxt_cleanup();
		return;
	}

	/* if the user hits ctrl-c and quits again, really quit */
	wxt_persist = no;

	FPRINTF((stderr,"wxWidgets terminal handles 'persist' setting\n"));

#ifdef USE_MOUSE
	wxt_change_thread_state(WAITING_FOR_STDIN);
#endif /*USE_MOUSE*/

	/* protect the following from interrupt */
	wxt_sigint_init();

	while (wxt_window_opened()) {
#ifdef USE_MOUSE
		if (!strcmp(term->name,"wxt"))
			wxt_process_events();
#endif /*USE_MOUSE*/
#ifdef _Windows
		/* continue to process gui messages */
		GetMessage(&msg, 0, 0, 0);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
#endif /*_Windows*/
		/* wait 10 microseconds and repeat */
		wxMicroSleep(10);
		wxt_sigint_check();
	}

	wxt_sigint_restore();

#ifdef USE_MOUSE
	wxt_change_thread_state(RUNNING);
#endif /*USE_MOUSE*/

	/* cleanup and quit */
	wxt_cleanup();
}


/* destroy everything related to wxWidgets */
void wxt_cleanup()
{
	std::vector<wxt_window_t>::iterator wxt_iter; /*declare the iterator*/

	if (wxt_status == STATUS_UNINITIALIZED)
		return;

	FPRINTF((stderr,"cleanup before exit\n"));

	/* prevent wxt_reset (for example) from doing anything bad after that */
	wxt_status = STATUS_UNINITIALIZED;

	/* protect the following from interrupt */
	wxt_sigint_init();

	/* Close all open terminal windows, this will make OnRun exit, and so will the gui thread */
	wxt_MutexGuiEnter();
	for(wxt_iter = wxt_window_list.begin(); wxt_iter != wxt_window_list.end(); wxt_iter++)
		delete wxt_iter->frame;
	wxt_MutexGuiLeave();

#ifndef __WXMSW__
	FPRINTF((stderr,"waiting for gui thread to exit\n"));
	FPRINTF((stderr,"gui thread status %d %d %d\n",
			thread->IsDetached(),
			thread->IsAlive(),
			thread->IsRunning() ));

	thread->Wait();
	delete thread;
	FPRINTF((stderr,"gui thread exited\n"));
#endif /* !__WXMSW__*/

	wxTheApp->OnExit();
	wxUninitialize();

	/* handle eventual interrupt, and restore original sigint handler */
	wxt_sigint_check();
	wxt_sigint_restore();

	FPRINTF((stderr,"wxWidgets terminal properly cleaned-up\n"));
}

/* -------------------------------------
 * GUI Mutex helper functions for porting
 * ----------------------------------------*/

void wxt_MutexGuiEnter()
{
	FPRINTF2((stderr,"locking gui mutex\n"));
#ifdef __WXGTK__
	wxMutexGuiEnter();
#elif defined(__WXMSW__)
#else
# error "No implementation"
#endif
}

void wxt_MutexGuiLeave()
{
	FPRINTF2((stderr,"unlocking gui mutex\n"));
#ifdef __WXGTK__
	wxMutexGuiLeave();
#elif defined(__WXMSW__)
#else
# error "No implementation"
#endif
}

/* ---------------------------------------------------
 * SIGINT handling : as the terminal is multithreaded, it needs several mutexes.
 * To avoid inconsistencies and deadlock when the user hits ctrl-c,
 * each critical set of instructions (implying mutexes for example) should be written :
 *	wxt_sigint_init();
 *	< critical instructions >
 *	wxt_sigint_check();
 *	wxt_sigint_restore();
 * Or, if the critical instructions are in a loop, wxt_sigint_check() should be
 * called regularly in the loop.
 * ---------------------------------------------------*/


/* our custom SIGINT handler, that just sets a flag */
void wxt_sigint_handler(int WXUNUSED(sig))
{
	FPRINTF((stderr,"custom interrupt handler called\n"));
	signal(SIGINT, wxt_sigint_handler);
	/* routines must check regularly for wxt_status, 
	 * and abort cleanly on STATUS_INTERRUPT_ON_NEXT_CHECK */
	wxt_status = STATUS_INTERRUPT_ON_NEXT_CHECK;
	if (wxt_current_plot)
		wxt_current_plot->interrupt = TRUE;
}

/* To be called when the function has finished cleaning after facing STATUS_INTERRUPT_ON_NEXT_CHECK */
/* Provided for flexibility, but use wxt_sigint_check instead directly */
void wxt_sigint_return()
{
	FPRINTF((stderr,"calling original interrupt handler\n"));
	wxt_status = STATUS_INTERRUPT;
	wxt_sigint_counter = 0;
	/* call the original sigint handler */
	/* this will not return !! */
	(*original_siginthandler)(SIGINT);
}

/* A critical function should call this from a safe zone (no locked mutex, objects destroyed).
 * If the interrupt is asked, this fonction will not return (longjmp) */
void wxt_sigint_check()
{
	FPRINTF2((stderr,"checking interrupt status\n"));
	if (wxt_status == STATUS_INTERRUPT_ON_NEXT_CHECK)
		wxt_sigint_return();
}

/* initialize our custom SIGINT handler */
/* this uses a usage counter, so that it can be encapsulated without problem */
void wxt_sigint_init()
{
	/* put our custom sigint handler, store the original one */
	if (wxt_sigint_counter == 0)
		original_siginthandler = signal(SIGINT, wxt_sigint_handler);
	++wxt_sigint_counter;
	FPRINTF2((stderr,"initialize custom interrupt handler %d\n",wxt_sigint_counter));
}

/* restore the original SIGINT handler */
void wxt_sigint_restore()
{
	if (wxt_sigint_counter==1)
		signal(SIGINT, *original_siginthandler);
	--wxt_sigint_counter;
	FPRINTF2((stderr,"restore custom interrupt handler %d\n",wxt_sigint_counter));
	if (wxt_sigint_counter<0)
		fprintf(stderr,"sigint counter < 0 : error !\n");
}
