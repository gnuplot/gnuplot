/* GNUPLOT - qt_term.cpp */

/*[
 * Copyright 2009   Jérôme Lodewyck
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
 * Alternatively, the contents of this file may be used under the terms of the
 * GNU General Public License Version 2 or later (the "GPL"), in which case the
 * provisions of GPL are applicable instead of those above. If you wish to allow
 * use of your version of this file only under the terms of the GPL and not
 * to allow others to use your version of this file under the above gnuplot
 * license, indicate your decision by deleting the provisions above and replace
 * them with the notice and other provisions required by the GPL. If you do not
 * delete the provisions above, a recipient may use your version of this file
 * under either the GPL or the gnuplot license.
]*/

#include <QtCore>
#include <QtGui>
#include <QtNetwork>

extern "C" {
	#include "plot.h"      // for interactive
	#include "term_api.h"  // for stdfn.h, JUSTIFY, encoding, *term definition, color.h
	#include "mouse.h"     // for do_event declaration
	#include "getcolor.h"  // for rgb functions
	#include "command.h"   // for paused_for_mouse, PAUSE_BUTTON1 and friends
	#include "util.h"      // for int_error
	#include "alloc.h"     // for gp_alloc
	#include "parse.h"     // for real_expression
	#include "axis.h"
	#include <signal.h>
}

#include "qt_term.h"
#include "QtGnuplotApplication.h"
#include "QtGnuplotEvent.h"
#include "qt_conversion.cpp"

void qt_atexit();

/*-------------------------------------------------------
 * Terminal options with default values
 *-------------------------------------------------------*/

/// @todo per-window options

int  qt_optionWindowId = 0;
bool qt_optionEnhanced = false;
bool qt_optionPersist  = false;
bool qt_optionRaise    = true;
bool qt_optionCtrl     = false;
int  qt_optionWidth    = 640;
int  qt_optionHeight   = 480;
int  qt_optionFontSize = 9;
QString qt_optionFontName = "Sans";
QString qt_optionTitle;
QString qt_optionWidget;
static const int qt_oversampling = 10;
static const double qt_oversamplingF = double(qt_oversampling);

/*-------------------------------------------------------
 * State variables
 *-------------------------------------------------------*/

bool qt_initialized = false;
bool qt_setSize = true;
int  qt_setWidth = qt_optionWidth;
int  qt_setHeight = qt_optionHeight;
int  qt_currentFontSize;
QString qt_currentFontName;
QString qt_localServerName;
QTextCodec* qt_codec = QTextCodec::codecForLocale();

/* ------------------------------------------------------
 * Helpers
 * ------------------------------------------------------*/

// Convert gnuplot coordinates into floating point term coordinates
QPointF qt_termCoordF(unsigned int x, unsigned int y)
{
	return QPointF(double(x)/qt_oversamplingF, double(term->ymax - y)/qt_oversamplingF);
}

// The same, but with coordinates clipped to the nearest pixel
QPoint qt_termCoord(unsigned int x, unsigned int y)
{
	return QPoint(qRound(double(x)/qt_oversamplingF), qRound(double(term->ymax - y)/qt_oversamplingF));
}

/*-------------------------------------------------------
 * Communication gnuplot -> terminal
 *-------------------------------------------------------*/

// Events coming from gnuplot are processed by this file
// and send to the GUI by a QDataStream writing on a
// QByteArray sent trough a QLocalSocket (a cross-platform IPC protocol)
QLocalSocket qt_socket;
QByteArray   qt_outBuffer;
QDataStream  qt_out(&qt_outBuffer, QIODevice::WriteOnly);

void qt_flushOutBuffer()
{
	if (!qt_initialized)
		return;

	// Write the block size at the beginning of the bock
	QDataStream sizeStream(&qt_socket);
	sizeStream << (quint32)(qt_outBuffer.size());
	// Write the block to the QLocalSocket
	qt_socket.write(qt_outBuffer);
	// waitForBytesWritten(-1) is supposed implement this loop, but it does not seem to work !
	// update: seems to work with Qt 4.5
	while (qt_socket.bytesToWrite() > 0)
	{
		qt_socket.flush();
		qt_socket.waitForBytesWritten(-1);
	}
	// Reset the buffer
	qt_out.device()->seek(0);
	qt_outBuffer.clear();
}

void qt_connectToServer()
{
	if (!qt_initialized)
		return;

	// Determine to which server we should connect
	QString server = qt_localServerName;
	if (!qt_optionWidget.isEmpty())
		server = qt_optionWidget;

	// Check if we are already connected
	if (qt_socket.serverName() == server)
		return;

	// Disconnect
	if (qt_socket.state() == QLocalSocket::ConnectedState)
	{
		qt_socket.disconnectFromServer();
		while (qt_socket.state() == QLocalSocket::ConnectedState)
			qt_socket.waitForDisconnected(1000);
	}

	// Connect to server, or local server if not available.
	qt_socket.connectToServer(server);
	if (!qt_socket.waitForConnected(3000))
		while (qt_socket.state() != QLocalSocket::ConnectedState)
			qt_socket.connectToServer(qt_localServerName);
}

/*-------------------------------------------------------
 * Communication terminal -> gnuplot
 *-------------------------------------------------------*/

bool qt_processTermEvent(gp_event_t* event)
{
	// Intercepts resize event
	if (event->type == GE_fontprops)
	{
		qt_setSize   = true;
		qt_setWidth  = event->mx;
		qt_setHeight = event->my;
	}
	// Scale mouse events
	else
	{
		event->mx *= qt_oversampling;
		event->my = (qt_setHeight - event->my)*qt_oversampling;
	}

	// Send the event to gnuplot core
	do_event(event);
	// Process pause_for_mouse
	if ((event->type == GE_buttonrelease) && (paused_for_mouse & PAUSE_CLICK))
	{
		int button = event->par1;
		if (((button == 1) && (paused_for_mouse & PAUSE_BUTTON1)) ||
		    ((button == 2) && (paused_for_mouse & PAUSE_BUTTON2)) ||
		    ((button == 3) && (paused_for_mouse & PAUSE_BUTTON3)))
			paused_for_mouse = 0;
		if (paused_for_mouse == 0)
			return true;
	}
	if ((event->type == GE_keypress) && (paused_for_mouse & PAUSE_KEYSTROKE) && (event->par1 > '\0'))
	{
		paused_for_mouse = 0;
		return true;
	}

	return false;
}

/* ------------------------------------------------------
 * Functions called by gnuplot
 * ------------------------------------------------------*/

// Called before first plot after a set term command.
void qt_init()
{
	// Start the QtGnuplotApplication if not already started
	if (qt_initialized)
		return;

	// Fork the GUI if necessary
	int argc = 0;
	pid_t pid = 0;
	pid = fork();
	if (pid < 0)
		fprintf(stderr, "Forking error\n");
	else if (pid == 0) // Child: start the GUI
	{
		signal(SIGINT, SIG_IGN); // Do not listen to SIGINT signals anymore
		QtGnuplotApplication application(argc, (char**)( NULL));

		// Load translations for the qt library
		QTranslator qtTranslator;
		qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
		application.installTranslator(&qtTranslator);

		// Load translations for the qt terminal
		QTranslator translator;
		translator.load("qtgnuplot_" + QLocale::system().name(), QTGNUPLOT_DATA_DIR);
		application.installTranslator(&translator);

		// Start
		application.exec();
		exit(0);
	}

	// Parent: create a QApplication without event loop for QObject's that need it
	QApplication* application = new QApplication(argc, (char**)( NULL));

	// Init state variables
	qt_localServerName = "qtgnuplot" + QString::number(pid);
	qt_out.setVersion(QDataStream::Qt_4_4);
	qt_initialized = true;
	GP_ATEXIT(qt_atexit);

	// The creation of a QApplication mangled our locale settings
#ifdef HAVE_LOCALE_H
	setlocale(LC_NUMERIC, "C");
	setlocale(LC_TIME, current_locale);
#endif
}

// Called just before a plot is going to be displayed.
void qt_graphics()
{
	qt_out << GEDesactivate;
	qt_flushOutBuffer();
	qt_connectToServer();

	// Set text encoding
	if (!(qt_codec = qt_encodingToCodec(encoding)))
		qt_codec = QTextCodec::codecForLocale();

	// Set font
	qt_currentFontSize = qt_optionFontSize;
	qt_currentFontName = qt_optionFontName;

	// Set plot metrics
	QFontMetrics metrics(QFont(qt_currentFontName, qt_currentFontSize));
	term->v_char = qt_oversampling*metrics.ascent() + 1;
	term->h_char = qt_oversampling*metrics.width("0123456789")/10;
	term->v_tic = (unsigned int) (term->v_char/2.5);
	term->h_tic = (unsigned int) (term->v_char/2.5);
	if (qt_setSize)
	{
		term->xmax = qt_oversampling*qt_setWidth;
		term->ymax = qt_oversampling*qt_setHeight;
		qt_setSize = false;
	}

	// Initialize window
	qt_out << GESetCurrentWindow << qt_optionWindowId;
	qt_out << GEInitWindow;
	qt_out << GEActivate;
	qt_out << GETitle << qt_optionTitle;
	qt_out << GESetCtrl << qt_optionCtrl;
	qt_out << GESetWidgetSize << QSize(term->xmax, term->ymax)/qt_oversampling;
	// Initialize the scene
	qt_out << GESetSceneSize << QSize(term->xmax, term->ymax)/qt_oversampling;
	qt_out << GEClear;
	// Initialize the font
	qt_out << GESetFont << qt_currentFontName << qt_currentFontSize;
}

// Called after plotting is done
void qt_text()
{
	if (qt_optionRaise)
		qt_out << GERaise;
	qt_out << GEDone;
	qt_flushOutBuffer();
}

void qt_text_wrapper()
{
	// Remember scale to update the status bar while the plot is inactive
	qt_out << GEScale;

	const int axis_order[4] = {FIRST_X_AXIS, FIRST_Y_AXIS, SECOND_X_AXIS, SECOND_Y_AXIS};

	for (int i = 0; i < 4; i++)
	{
		qt_out << (axis_array[axis_order[i]].ticmode != NO_TICS); // Axis active or not
		qt_out << axis_array[axis_order[i]].min;
		double lower = double(axis_array[axis_order[i]].term_lower);
		double scale = double(axis_array[axis_order[i]].term_scale);
		// Reverse the y axis
		if (i % 2)
		{
			lower = term->ymax - lower;
			scale *= -1;
		}
		qt_out << lower/qt_oversamplingF << scale/qt_oversamplingF;
		qt_out << (axis_array[axis_order[i]].log ? axis_array[axis_order[i]].log_base : 0.);
	}

	qt_text();
}

void qt_reset()
{
	/// @todo
}

void qt_move(unsigned int x, unsigned int y)
{
	qt_out << GEMove << qt_termCoordF(x, y);
}

void qt_vector(unsigned int x, unsigned int y)
{
	qt_out << GEVector << qt_termCoordF(x, y);
}

bool       qt_enhancedSymbol = false;
QString    qt_enhancedFontName;
double     qt_enhancedFontSize;
double     qt_enhancedBase;
bool       qt_enhancedWidthFlag;
bool       qt_enhancedShowFlag;
int        qt_enhancedOverprint;
QByteArray qt_enhancedText;

void qt_enhanced_flush()
{
	qt_out << GEEnhancedFlush << qt_enhancedFontName << qt_enhancedFontSize
	       << qt_enhancedBase << qt_enhancedWidthFlag << qt_enhancedShowFlag
	       << qt_enhancedOverprint << qt_codec->toUnicode(qt_enhancedText);
	qt_enhancedText.clear();
}

void qt_enhanced_writec(int c)
{
	if (qt_enhancedSymbol)
		qt_enhancedText.append(qt_codec->fromUnicode(qt_symbolToUnicode(c)));
	else
		qt_enhancedText.append(char(c));
}

void qt_enhanced_open(char* fontname, double fontsize, double base, TBOOLEAN widthflag, TBOOLEAN showflag, int overprint)
{
	qt_enhancedFontName = fontname;
	if (qt_enhancedFontName.toLower() == "symbol")
	{
		qt_enhancedSymbol = true;
		qt_enhancedFontName = "Sans";
	}
	else
		qt_enhancedSymbol = false;

	qt_enhancedFontSize  = fontsize;
	qt_enhancedBase      = base;
	qt_enhancedWidthFlag = widthflag;
	qt_enhancedShowFlag  = showflag;
	qt_enhancedOverprint = overprint;
}

void qt_put_text(unsigned int x, unsigned int y, const char* string)
{
	// if ignore_enhanced_text is set, draw with the normal routine.
	// This is meant to avoid enhanced syntax when the enhanced mode is on
	if (!qt_optionEnhanced || ignore_enhanced_text)
	{
		/// @todo Symbol font to unicode
		/// @todo bold, italic
		qt_out << GEPutText << qt_termCoord(x, y) << qt_codec->toUnicode(string);
		return;
	}

	// Uses enhanced_recursion() to analyse the string to print.
	// enhanced_recursion() calls _enhanced_open() to initialize the text drawing,
	// then it calls _enhanced_writec() which buffers the characters to draw,
	// and finally _enhanced_flush() to draw the buffer with the correct justification.

	// set up the global variables needed by enhanced_recursion()
	enhanced_fontscale = 1.0;
	strncpy(enhanced_escape_format, "%c", sizeof(enhanced_escape_format));

	// Set the recursion going. We say to keep going until a closing brace, but
	// we don't really expect to find one.  If the return value is not the nul-
	// terminator of the string, that can only mean that we did find an unmatched
	// closing brace in the string. We increment past it (else we get stuck
	// in an infinite loop) and try again.
	while (*(string = enhanced_recursion((char*)string, TRUE, qt_currentFontName.toUtf8().data(),
			qt_currentFontSize, 0.0, TRUE, TRUE, 0)))
	{
		qt_enhanced_flush();
		enh_err_check(string); // we can only get here if *str == '}'
		if (!*++string)
			break; // end of string
		// else carry on and process the rest of the string
	}

	qt_out << GEEnhancedFinish << qt_termCoord(x, y);
}

void qt_linetype(int lt)
{
	if (lt <= LT_NODRAW)
		lt = LT_NODRAW; // background color

	if (lt == -1)
		qt_out << GEPenStyle << Qt::DashLine;
	else
		qt_out << GEPenStyle << Qt::SolidLine;

	qt_out << GEPenColor << qt_colorList[lt % 9 + 3];
}

int qt_set_font (const char* font)
{
	if (font && (*font))
	{
		QStringList list = QString(font).split(',');
		if (list.size() > 0)
			qt_currentFontName = list[0];
		if (list.size() > 1)
			qt_currentFontSize = list[1].toInt();
	}

	if (qt_currentFontName.isEmpty())
		qt_currentFontName = qt_optionFontName;

	if (qt_currentFontSize <= 0)
		qt_currentFontSize = qt_optionFontSize;

	qt_out << GESetFont << qt_currentFontName << qt_currentFontSize;

	return 1;
}

int qt_justify_text(enum JUSTIFY mode)
{
	if (mode == LEFT)
		qt_out << GETextAlignment << Qt::AlignLeft;
	else if (mode == RIGHT)
		qt_out << GETextAlignment << Qt::AlignRight;
	else if (mode == CENTRE)
		qt_out << GETextAlignment << Qt::AlignCenter;

	return 1; // We can justify
}

void qt_point(unsigned int x, unsigned int y, int pointstyle)
{
	qt_out << GEPoint << qt_termCoordF(x, y) << pointstyle;
}

void qt_pointsize(double ptsize)
{
	if (ptsize < 0.) ptsize = 1.; // same behaviour as x11 terminal
	qt_out << GEPointSize << ptsize;
}

void qt_linewidth(double lw)
{
	qt_out << GELineWidth << lw;
}

int qt_text_angle(int angle)
{
	qt_out << GETextAngle << double(angle);
	return 1; // 1 means we can rotate
}

void qt_fillbox(int style, unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	qt_out << GEBrushStyle << style;
	qt_out << GEFillBox << QRect(qt_termCoord(x, y + height), QSize(width, height)/qt_oversampling);
}

int qt_make_palette(t_sm_palette* palette)
{
	return 0; // We can do continuous colors
}

void qt_set_color(t_colorspec* colorspec)
{
	if (colorspec->type == TC_LT)
		qt_linetype(colorspec->lt);
	else if (colorspec->type == TC_FRAC)
	{
		rgb_color rgb;
		rgb1maxcolors_from_gray(colorspec->value, &rgb);
		QColor color;
		color.setRgbF(rgb.r, rgb.g, rgb.b);
		qt_out << GEPenColor << color;
	}
	else if (colorspec->type == TC_RGB)
		qt_out << GEPenColor << QColor(QRgb(colorspec->lt));
}

void qt_filled_polygon(int n, gpiPoint *corners)
{
	QPolygonF polygon;
	for (int i = 0; i < n; i++)
		polygon << qt_termCoordF(corners[i].x, corners[i].y);

	qt_out << GEBrushStyle << corners->style;
	qt_out << GEFilledPolygon << polygon;
}

void qt_image(unsigned int M, unsigned int N, coordval* image, gpiPoint* corner, t_imagecolor color_mode)
{
	QImage qimage = qt_imageToQImage(M, N, image, color_mode);
	qt_out << GEImage;
	for (int i = 0; i < 4; i++)
		qt_out << qt_termCoord(corner[i].x, corner[i].y);
	qt_out << qimage;
}

#ifdef USE_MOUSE

// Display temporary text, after
// erasing any temporary text displayed previously at this location.
// The int determines where: 0=statusline, 1,2: at corners of zoom
// box, with \r separating text above and below the point.
void qt_put_tmptext(int n, const char str[])
{
	if (n == 0)
		qt_out << GEStatusText << QString(str);
	else if (n == 1)
		qt_out << GEZoomStart << QString(str);
	else if (n == 2)
		qt_out << GEZoomStop << QString(str);

	qt_flushOutBuffer();
}

void qt_set_cursor(int c, int x, int y)
{
	// Cancel zoombox when Echap is pressed
	if (c == 0)
		qt_out << GEZoomStop << QString();

	if (c == -4)
		qt_out << GELineTo << false;
	else if (c == -3)
		qt_out << GELineTo << true;
	else if (c == -2) // warp the pointer to the given position
		qt_out << GEWrapCursor << qt_termCoord(x, y);
	else if (c == -1) // start zooming
		qt_out << GECursor << Qt::SizeFDiagCursor;
	else if (c ==  1) // Rotation
		qt_out << GECursor << Qt::ClosedHandCursor;
	else if (c ==  2) // Rescale
		qt_out << GECursor << Qt::SizeAllCursor;
	else if (c ==  3) // Zoom
		qt_out << GECursor << Qt::SizeFDiagCursor;
	else
		qt_out << GECursor << Qt::CrossCursor;

	qt_flushOutBuffer();
}

void qt_set_ruler(int x, int y)
{
	qt_out << GERuler << qt_termCoord(x, y);
	qt_flushOutBuffer();
}

void qt_set_clipboard(const char s[])
{
	qt_out << GECopyClipboard << s;
	qt_flushOutBuffer();
}
#endif // USE_MOUSE

int qt_waitforinput(void)
{
#ifdef USE_MOUSE
	fd_set read_fds;
	int stdin_fd  = fileno(stdin);
	int socket_fd = qt_socket.socketDescriptor();

	if (!qt_initialized || (socket_fd < 0) || (qt_socket.state() != QLocalSocket::ConnectedState))
		return getchar();

	// Gnuplot event loop
	do
	{
		// Watch file descriptors
		FD_ZERO(&read_fds);
		FD_SET(socket_fd, &read_fds);
		if (!paused_for_mouse)
			FD_SET(stdin_fd, &read_fds);

		// Wait for input
		if (select(socket_fd+1, &read_fds, NULL, NULL, NULL) < 0)
		{
			fprintf(stderr, "Qt terminal communication error: select() error\n");
			break;
		}

		// Terminal event coming
		if (FD_ISSET(socket_fd, &read_fds))
		{
			qt_socket.waitForReadyRead(-1);
			// Temporary event for mouse move events. If several consecutive move events
			// are received, only transmit the last one.
			gp_event_t tempEvent;
			tempEvent.type = -1;
			while (qt_socket.bytesAvailable() >= sizeof(gp_event_t))
			{
				struct gp_event_t event;
				qt_socket.read((char*) &event, sizeof(gp_event_t));
				// Delay move events
				if (event.type == GE_motion)
					tempEvent = event;
				// Other events. Replay the last move event if present
				else
				{
					if (tempEvent.type == GE_motion)
					{
						qt_processTermEvent(&tempEvent);
						tempEvent.type = -1;
					}
					if (qt_processTermEvent(&event))
						return '\0'; // exit from paused_for_mouse
				}
			}
			// Replay move event
			if (tempEvent.type == GE_motion)
				qt_processTermEvent(&tempEvent);
		}
	} while (paused_for_mouse || (!paused_for_mouse && !FD_ISSET(stdin_fd, &read_fds)));
#endif
	return getchar();
}

/*-------------------------------------------------------
 * Misc
 *-------------------------------------------------------*/

void qt_atexit()
{
	if (qt_optionPersist || persist_cl)
		qt_out << GEPersist;
	else
		qt_out << GEExit;
	qt_flushOutBuffer();
	qt_initialized = false;
}

/*-------------------------------------------------------
 * Term options
 *-------------------------------------------------------*/

enum QT_id {
	QT_WIDGET,
	QT_FONT,
	QT_ENHANCED,
	QT_NOENHANCED,
	QT_SIZE,
	QT_PERSIST,
	QT_NOPERSIST,
	QT_RAISE,
	QT_NORAISE,
	QT_CTRL,
	QT_NOCTRL,
	QT_TITLE,
	QT_CLOSE,
	QT_OTHER
};

static struct gen_table qt_opts[] = {
	{"$widget",     QT_WIDGET},
	{"font",        QT_FONT},
	{"enh$anced",   QT_ENHANCED},
	{"noenh$anced", QT_NOENHANCED},
	{"s$ize",       QT_SIZE},
	{"per$sist",    QT_PERSIST},
	{"noper$sist",  QT_NOPERSIST},
	{"rai$se",      QT_RAISE},
	{"norai$se",    QT_NORAISE},
	{"ct$rlq",      QT_CTRL},
	{"noct$rlq",    QT_NOCTRL},
	{"ti$tle",      QT_TITLE},
	{"cl$ose",      QT_CLOSE},
	{NULL,          QT_OTHER}
};

// Called when terminal type is selected.
// This procedure should parse options on the command line.
// A list of the currently selected options should be stored in term_options[],
// in a form suitable for use with the set term command.
// term_options[] is used by the save command.  Use options_null() if no options are available." *
void qt_options()
{
	char *s = NULL;
	QString fontSettings;
	bool duplication = false;
	bool set_enhanced = false, set_font = false;
	bool set_persist = false, set_number = false;
	bool set_raise = false, set_ctrl = false;
	bool set_title = false, set_close = false;
	bool set_capjoin = false, set_size = false;
	bool set_widget = false;

#define SETCHECKDUP(x) { c_token++; if (x) duplication = true; x = true; }

	while (!END_OF_COMMAND)
	{
		FPRINTF((stderr, "processing token\n"));
		switch (lookup_table(&qt_opts[0], c_token)) {
		case QT_WIDGET:
			SETCHECKDUP(set_widget);
			if (!(s = try_to_get_string()))
				int_error(c_token, "widget: expecting string");
			if (*s)
				qt_optionWidget = QString(s);
			free(s);
			break;
		case QT_FONT:
			SETCHECKDUP(set_font);
			if (!(s = try_to_get_string()))
				int_error(c_token, "font: expecting string");
			if (*s)
			{
				fontSettings = QString(s);
				QStringList list = fontSettings.split(',');
				if ((list.size() > 0) && !list[0].isEmpty())
					qt_optionFontName = list[0];
				if ((list.size() > 1) && (list[1].toInt() > 0))
					qt_optionFontSize = list[1].toInt();
			}
			free(s);
			break;
		case QT_ENHANCED:
			SETCHECKDUP(set_enhanced);
			qt_optionEnhanced = true;
			term->flags |= TERM_ENHANCED_TEXT;
			break;
		case QT_NOENHANCED:
			SETCHECKDUP(set_enhanced);
			qt_optionEnhanced = false;
			term->flags &= ~TERM_ENHANCED_TEXT;
			break;
		case QT_SIZE:
			SETCHECKDUP(set_size);
			if (END_OF_COMMAND)
				int_error(c_token, "size requires 'width,heigth'");
			qt_optionWidth = real_expression();
			if (!equals(c_token++, ","))
				int_error(c_token, "size requires 'width,heigth'");
			qt_optionHeight = real_expression();
			if (qt_optionWidth < 1 || qt_optionHeight < 1)
				int_error(c_token, "size is out of range");
			break;
		case QT_PERSIST:
			SETCHECKDUP(set_persist);
			qt_optionPersist = true;
			break;
		case QT_NOPERSIST:
			SETCHECKDUP(set_persist);
			qt_optionPersist = false;
			break;
		case QT_RAISE:
			SETCHECKDUP(set_raise);
			qt_optionRaise = true;
			break;
		case QT_NORAISE:
			SETCHECKDUP(set_raise);
			qt_optionRaise = false;
			break;
		case QT_CTRL:
			SETCHECKDUP(set_ctrl);
			qt_optionCtrl = true;
			break;
		case QT_NOCTRL:
			SETCHECKDUP(set_ctrl);
			qt_optionCtrl = false;
			break;
		case QT_TITLE:
			SETCHECKDUP(set_title);
			if (!(s = try_to_get_string()))
				int_error(c_token, "title: expecting string");
			if (*s)
				qt_optionTitle = qt_codec->toUnicode(s);
			free(s);
			break;
		case QT_CLOSE:
			SETCHECKDUP(set_close);
			break;
		case QT_OTHER:
		default:
			qt_optionWindowId = int_expression();
			qt_optionWidget = "";
			if (set_number) duplication = true;
			set_number = true;
			break;
		}

		if (duplication)
			int_error(c_token-1, "Duplicated or contradicting arguments in qt term options.");
	}

	// Save options back into options string in normalized format
	QString termOptions = QString::number(qt_optionWindowId);

	if (set_title)
	{
		termOptions += " title \"" + qt_optionTitle + '"';
		if (qt_initialized)
			qt_out << GETitle << qt_optionTitle;
	}

	if (set_size)
	{
		termOptions += " size " + QString::number(qt_optionWidth) + ", "
		                        + QString::number(qt_optionHeight);
		qt_setSize   = true;
		qt_setWidth  = qt_optionWidth;
		qt_setHeight = qt_optionHeight;
	}

	if (set_enhanced) termOptions +=  qt_optionEnhanced ? " enhanced" : " noenhanced";
	if (set_font)     termOptions += " font \"" + fontSettings + '"';
	if (set_widget)   termOptions += " widget \"" + qt_optionWidget + '"';
	if (set_persist)  termOptions += qt_optionPersist ? " persist" : " nopersist";
	if (set_raise)    termOptions += qt_optionRaise ? " raise" : " noraise";
	if (set_ctrl)     termOptions += qt_optionCtrl ? " ctrl" : " noctrl";

	if (set_close && qt_initialized) qt_out << GECloseWindow << qt_optionWindowId;

	/// @bug change Utf8 to local encoding
	strncpy(term_options, termOptions.toUtf8().data(), MAX_LINE_LEN);
}
