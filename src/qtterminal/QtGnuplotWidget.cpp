/* GNUPLOT - QtGnuplotWidget.cpp */

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

#include "QtGnuplotWidget.h"
#include "QtGnuplotEvent.h"
#include "QtGnuplotScene.h"
#include "QtGnuplotItems.h"

extern "C" {
#include "../mousecmn.h"
}

#include <QtGui>
#include <QSvgGenerator>

int QtGnuplotWidget::m_widgetUid = 1;

QtGnuplotWidget::QtGnuplotWidget(QWidget* parent)
	: QWidget(parent)
	, m_id(0)
{
	m_eventHandler = 0;
	init();
}

QtGnuplotWidget::QtGnuplotWidget(int id, QtGnuplotEventHandler* eventHandler, QWidget* parent)
	: QWidget(parent)
	, m_id(id)
{
	m_eventHandler = eventHandler;
	init();
}

void QtGnuplotWidget::init()
{
	m_active = false;
	m_lastSizeRequest = QSize(-1, -1);
	m_rounded = true;
	m_backgroundColor = Qt::white;
	m_antialias = true;
	m_replotOnResize = true;

	// Register as the main event receiver if not already created
	if (m_eventHandler == 0)
		m_eventHandler = new QtGnuplotEventHandler(this,"qtgnuplot" +
		                 QString::number(QCoreApplication::applicationPid()) + "-" +
		                 QString::number(m_widgetUid++));

	// Construct GUI elements
	m_scene = new QtGnuplotScene(m_eventHandler, this);
	m_view = new QGraphicsView(m_scene);
	m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	QVBoxLayout* layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_view);
	setLayout(layout);
	setViewMatrix();
}

void QtGnuplotWidget::setStatusText(const QString& status)
{
	emit statusTextChanged(status);
}

bool QtGnuplotWidget::isActive() const
{
	return m_active;
}

QSize QtGnuplotWidget::plotAreaSize() const
{
	return m_view->viewport()->size();
}

void QtGnuplotWidget::setViewMatrix()
{
	m_view->resetMatrix();
}

void QtGnuplotWidget::processEvent(QtGnuplotEventType type, QDataStream& in)
{
	if (type == GESetWidgetSize)
	{
		QSize s;
		in >> s;
//		qDebug() << "Size request" << s << " / viewport" << m_view->maximumViewportSize();
		m_lastSizeRequest = s;
		m_view->resetMatrix();
		QWidget* viewport = m_view->viewport();

		if (s != viewport->size())
		{
//			qDebug() << " -> resizing";
			QMainWindow* parent = dynamic_cast<QMainWindow*>(parentWidget());
			if (parent)
				parent->resize(s + parent->size() - viewport->size());
			viewport->resize(s);
		}
	}
	else if (type == GEStatusText)
	{
		QString status;
		in >> status;
		setStatusText(status);
	}
	else if (type == GECopyClipboard)
	{
		QString text;
		in >> text;
		QApplication::clipboard()->setText(text);
	}
	else if (type == GECursor)
	{
		int cursorType;
		in >> cursorType;
		m_view->setCursor(Qt::CursorShape(cursorType));
	}
	else if (type == GEWrapCursor)
	{
		QPoint point; in >> point;
		QCursor::setPos(mapToGlobal(point));
	}
	else if (type == GEActivate)
		m_active = true;
	else if (type == GEDesactivate)
		m_active = false;
	// Forward the event to the scene
	else
		m_scene->processEvent(type, in);
}

void QtGnuplotWidget::resizeEvent(QResizeEvent* event)
{
	QWidget* viewport = m_view->viewport();
//	qDebug() << "QtGnuplotWidget::resizeEvent" << "W" << size() << "V" << m_view->size() << "P" << viewport->size();

	// We only inform gnuplot of a new size, and not of the first resize event
	if ((viewport->size() != m_lastSizeRequest) && (m_lastSizeRequest != QSize(-1, -1)))
	{
		m_eventHandler->postTermEvent(GE_fontprops,viewport->size().width(),
		                               viewport->size().height(), 0, 0, 0); /// @todo m_id
		if (m_replotOnResize && m_active)
			m_eventHandler->postTermEvent(GE_keypress, 0, 0, 'e', 0, 0); // ask for replot
		else
			m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
	}

	QWidget::resizeEvent(event);
}

QPainter::RenderHints QtGnuplotWidget::renderHints() const
{
	QPainter::RenderHints hint = QPainter::TextAntialiasing;
	if (m_antialias)
		hint |= QPainter::Antialiasing;

	return hint;
}

/////////////////////////////////////////////////
// Slots

QPixmap QtGnuplotWidget::createPixmap()
{
	QPixmap pixmap(m_scene->width(), m_scene->height());
	pixmap.fill();
	QPainter painter(&pixmap);
	painter.translate(0.5, 0.5);
	painter.setRenderHints(renderHints());
	m_scene->render(&painter);
	painter.end();
	return pixmap;
}

void QtGnuplotWidget::copyToClipboard()
{
	QApplication::clipboard()->setPixmap(createPixmap());
}

void QtGnuplotWidget::print(QPrinter& printer)
{
	QPainter painter(&printer);
	painter.setRenderHints(renderHints());
	m_scene->render(&painter);
}

void QtGnuplotWidget::exportToPdf(const QString& fileName)
{
	QPrinter printer;
	printer.setOutputFormat(QPrinter::PdfFormat);
	printer.setOutputFileName(fileName);
	printer.setPaperSize(QSizeF(m_scene->width(), m_scene->height()), QPrinter::Point);
	printer.setPageMargins(0, 0, 0, 0, QPrinter::Point);
	QPainter painter(&printer);
	painter.setRenderHints(renderHints());
	m_scene->render(&painter);
}

void QtGnuplotWidget::exportToEps()
{
	/// @todo
}

void QtGnuplotWidget::exportToImage(const QString& fileName)
{
	createPixmap().save(fileName);
}

void QtGnuplotWidget::exportToSvg(const QString& fileName)
{
	QSvgGenerator svg;
	svg.setFileName(fileName);
	svg.setSize(QSize(m_view->width(), m_view->height()));
	svg.setViewBox(QRect(0, 0, m_view->width(), m_view->height()));
	QPainter painter(&svg);
	m_scene->render(&painter);
	painter.end();
}

/////////////////////////////////////////////////
// Settings

void QtGnuplotWidget::loadSettings(const QSettings& settings)
{
	setAntialias(settings.value("antialias", true).toBool());
	setRounded(settings.value("rounded", true).toBool());
	setBackgroundColor(settings.value("backgroundColor", QColor(Qt::white)).value<QColor>());
	setReplotOnResize(settings.value("replotOnResize", true).toBool());
}

void QtGnuplotWidget::saveSettings(QSettings& settings) const
{
	settings.setValue("antialias", m_antialias);
	settings.setValue("rounded", m_rounded);
	settings.setValue("backgroundColor", m_backgroundColor);
	settings.setValue("replotOnResize", m_replotOnResize);
}

void QtGnuplotWidget::setAntialias(bool value)
{
	m_antialias = value;
	m_view->setRenderHints(renderHints());
}

void QtGnuplotWidget::setRounded(bool value)
{
	m_rounded = value;
}

void QtGnuplotWidget::setReplotOnResize(bool value)
{
	m_replotOnResize = value;
}

void QtGnuplotWidget::setBackgroundColor(const QColor& color)
{
	m_backgroundColor = color;
	m_view->setBackgroundBrush(m_backgroundColor);
}

