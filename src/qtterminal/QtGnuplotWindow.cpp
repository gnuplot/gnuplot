/* GNUPLOT - QtGnuplotWindow.cpp */

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

#include "QtGnuplotWindow.h"
#include "QtGnuplotWidget.h"
#include "QtGnuplotEvent.h"

#include <QtGui>

QtGnuplotWindow::QtGnuplotWindow(int id, QtGnuplotEventHandler* eventHandler, QWidget* parent)
	: QMainWindow(parent)
{
	m_ctrl = false;
	m_eventHandler = eventHandler;
	m_id = id;
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowIcon(QIcon(":/images/gnuplot"));

	// Register as the main event receiver if not already created
	if (m_eventHandler == 0)
		m_eventHandler = new QtGnuplotEventHandler(this,
		                 "qtgnuplot" + QString::number(QCoreApplication::applicationPid()));

	// Central widget
	m_widget = new QtGnuplotWidget(m_id, m_eventHandler, this);
	connect(m_widget, SIGNAL(statusTextChanged(const QString&)), this, SLOT(on_setStatusText(const QString&)));
	setCentralWidget(m_widget);

	// Bars
	m_toolBar = new QToolBar(this);
	addToolBar(m_toolBar);
	statusBar()->showMessage(tr("Qt frontend for gnuplot"));

	// Actions
	QAction* copyToClipboardAction = new QAction(QIcon(":/images/clipboard"   ), tr("Copy to clipboard"), this);
	QAction* printAction           = new QAction(QIcon(":/images/print"       ), tr("Print"            ), this);
	QAction* exportAction          = new QAction(QIcon(":/images/export"      ), tr("Export"           ), this);
	QAction* exportPdfAction       = new QAction(QIcon(":/images/exportPDF"   ), tr("Export to PDF"    ), this);
	QAction* exportEpsAction       = new QAction(QIcon(":/images/exportVector"), tr("Export to EPS"    ), this);
	QAction* exportSvgAction       = new QAction(QIcon(":/images/exportVector"), tr("Export to SVG"    ), this);
	QAction* exportPngAction       = new QAction(QIcon(":/images/exportRaster"), tr("Export to image"  ), this);
	QAction* settingsAction        = new QAction(QIcon(":/images/settings"    ), tr("Settings"         ), this);
	connect(copyToClipboardAction, SIGNAL(triggered()), m_widget, SLOT(copyToClipboard()));
	connect(printAction,           SIGNAL(triggered()), m_widget, SLOT(print()));
	connect(exportPdfAction,       SIGNAL(triggered()), m_widget, SLOT(exportToPdf()));
	connect(exportEpsAction,       SIGNAL(triggered()), m_widget, SLOT(exportToEps()));
	connect(exportSvgAction,       SIGNAL(triggered()), m_widget, SLOT(exportToSvg()));
	connect(exportPngAction,       SIGNAL(triggered()), m_widget, SLOT(exportToImage()));
	connect(settingsAction,        SIGNAL(triggered()), m_widget, SLOT(showSettingsDialog()));
	QMenu* exportMenu = new QMenu(this);
	exportMenu->addAction(copyToClipboardAction);
	exportMenu->addAction(printAction);
	exportMenu->addAction(exportPdfAction);
//	exportMenu->addAction(exportEpsAction);
	exportMenu->addAction(exportSvgAction);
	exportMenu->addAction(exportPngAction);
	exportAction->setMenu(exportMenu);
	m_toolBar->addAction(exportAction);
	createAction(tr("Replot")       , 'e', ":/images/replot");
	createAction(tr("Show grid")    , 'g', ":/images/grid");
	createAction(tr("Previous zoom"), 'p', ":/images/zoomPrevious");
	createAction(tr("Next zoom")    , 'n', ":/images/zoomNext");
	createAction(tr("Autoscale")    , 'a', ":/images/autoscale");
	m_toolBar->addAction(settingsAction);
}

void QtGnuplotWindow::createAction(const QString& name, int key, const QString& icon)
{
	QAction* action = new QAction(QIcon(icon), name, this);
	connect(action, SIGNAL(triggered()), this, SLOT(on_keyAction()));
	action->setData(key);
	m_toolBar->addAction(action);
}

void QtGnuplotWindow::on_setStatusText(const QString& status)
{
	statusBar()->showMessage(status);
}

void QtGnuplotWindow::on_keyAction()
{
	QAction* action = qobject_cast<QAction *>(sender());
	m_eventHandler->postTermEvent(GE_keypress, 0, 0, action->data().toInt(), 0, m_id);
}

void QtGnuplotWindow::processEvent(QtGnuplotEventType type, QDataStream& in)
{
	if (type == GETitle)
	{
		QString title;
		in >> title;
		if (title.isEmpty())
			title = tr("Gnuplot window ") + QString::number(m_id);
		setWindowTitle(title);
	}
	else if (type == GERaise)
		raise();
	else if (type == GESetCtrl)
		in >> m_ctrl;
	else
		m_widget->processEvent(type, in);
}

void QtGnuplotWindow::keyPressEvent(QKeyEvent* event)
{
	if ((event->key() == 'Q') && ( !m_ctrl || (QApplication::keyboardModifiers() & Qt::ControlModifier) ))
		close();

	QMainWindow::keyPressEvent(event);
}
