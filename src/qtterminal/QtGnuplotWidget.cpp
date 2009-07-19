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

#include <QtGui>
#include <QSvgGenerator>

int QtGnuplotWidget::m_widgetUid = 1;

QtGnuplotWidget::QtGnuplotWidget(int id, QtGnuplotEventHandler* eventHandler, QWidget* parent)
	: QWidget(parent)
{
	m_id = id;
	m_active = false;
	m_resizeToken = false;
	m_eventHandler = eventHandler;
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

	loadSettings();
}

void QtGnuplotWidget::setStatusText(const QString& status)
{
	emit statusTextChanged(status);
}

bool QtGnuplotWidget::isActive() const
{
	return m_active;
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
//		qDebug() << "1 Size request" << s << "W" << size() << "V" << m_view->size() << "P" << m_view->maximumViewportSize();
		m_lastSizeRequest = s;
		m_resizeToken = false;
		m_view->resize((s));// + m_view->size() - m_view->maximumViewportSize()));
//		qDebug() << "2 Size request" << s << "W" << size() << "V" << m_view->size() << "P" << m_view->maximumViewportSize();
//		resize(m_view->size());
//		qDebug() << "3 Size request" << s << "W" << size() << "V" << m_view->size() << "P" << m_view->maximumViewportSize();
		setViewMatrix();
		adjustSize();
//		setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

/*		if (oldSize.width() < sizeHint().width() || oldSize.height() < sizeHint().height())
		{
			setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
			updateGeometry();
		}
		else
		{
			setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
			updateGeometry();
		}*/
		//setFixedSize(m_view->size());
//		updateGeometry();
//		qDebug() << "4 Size request" << s << "W" << size() << "V" << m_view->size() << "P" << m_view->maximumViewportSize();
		layout()->update();
//		qDebug() << "5 Size request" << s << "W" << size() << "V" << m_view->size() << "P" << m_view->maximumViewportSize();
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

QSize QtGnuplotWidget::sizeHint() const
{
//	qDebug() << "SizeHint" <<  m_view->size() << size();
	return m_view->size();
}

void QtGnuplotWidget::resizeEvent(QResizeEvent* event)
{
//	qDebug() << "Resize" << sizePolicy().verticalPolicy() << event->size() << event->oldSize();
//	qDebug() << "1 Resize" << "W" << size() << "V" << m_view->size() << "P" << m_view->maximumViewportSize();
	if (m_resizeToken && (size() != m_lastSizeRequest))
	{
//		qDebug() << "2 Resize" << "W" << size() << "V" << m_view->size() << "P" << m_view->maximumViewportSize();
//		qDebug() << "Send size request";
		m_eventHandler->postTermEvent(GE_fontprops, m_view->size().width(),
		                               m_view->size().height(), 9, 9, 0); /// @todo m_id
		m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
	//	m_eventHandler->postTermEvent(GE_keypress, 0, 0, 'e', 0, 0); // ask for replot ?
//		qDebug() << "3 Resize" << "W" << size() << "V" << m_view->size() << "P" << m_view->maximumViewportSize();
//		QMainWindow* parent = dynamic_cast<QMainWindow*>(parentWidget());
//		if (parent)
//			parent->adjustSize();
	}

	m_resizeToken = true;
	QWidget::resizeEvent(event);
//	qDebug() << "4 Resize" << "W" << size() << "V" << m_view->size() << "P" << m_view->maximumViewportSize();
}

/////////////////////////////////////////////////
// Slots

QPixmap QtGnuplotWidget::createPixmap()
{
	QPixmap pixmap(m_scene->width(), m_scene->height());
	pixmap.fill();
	QPainter painter(&pixmap);
	painter.translate(0.5, 0.5);
	painter.setRenderHint(m_antialias ? QPainter::Antialiasing : QPainter::TextAntialiasing);
	m_scene->render(&painter);
	painter.end();
	return pixmap;
}

void QtGnuplotWidget::copyToClipboard()
{
	QApplication::clipboard()->setPixmap(createPixmap());
}

void QtGnuplotWidget::print()
{
	QPrinter printer;
	if (QPrintDialog(&printer).exec() == QDialog::Accepted)
	{
		QPainter painter(&printer);
		painter.setRenderHint(m_antialias ? QPainter::Antialiasing : QPainter::TextAntialiasing);
		m_scene->render(&painter);
	}
}

void QtGnuplotWidget::exportToPdf()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Export to PDF"), "", tr("PDF files (*.pdf)"));
	if (fileName.isEmpty())
		return;
	if (!fileName.endsWith(".pdf", Qt::CaseInsensitive))
			fileName += ".pdf";

	QPrinter printer;
	printer.setOutputFormat(QPrinter::PdfFormat);
	printer.setOutputFileName(fileName);
	QPainter painter(&printer);
	painter.setRenderHint(m_antialias ? QPainter::Antialiasing : QPainter::TextAntialiasing);
	m_scene->render(&painter);
}

void QtGnuplotWidget::exportToEps()
{
}

void QtGnuplotWidget::exportToImage()
{
	/// @todo other image formats supported by Qt
	QString fileName = QFileDialog::getSaveFileName(this, tr("Export to Image"), "",
	                       tr("Image files (*.png *.bmp)"));
	if (fileName.isEmpty())
		return;
	if (!fileName.endsWith(".png", Qt::CaseInsensitive) &&
	    !fileName.endsWith(".bmp", Qt::CaseInsensitive))
			fileName += ".png";

	createPixmap().save(fileName);
}

void QtGnuplotWidget::exportToSvg()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Export to SVG"), "", tr("SVG files (*.svg)"));
	if (fileName.isEmpty())
		return;
	if (!fileName.endsWith(".svg", Qt::CaseInsensitive))
			fileName += ".svg";

	QSvgGenerator svg;
	svg.setFileName(fileName);
	svg.setSize(QSize(m_scene->width(), m_scene->height()));
	QPainter painter(&svg);
	m_scene->render(&painter);
	painter.end();
}

/////////////////////////////////////////////////
// Settings

void QtGnuplotWidget::loadSettings()
{
	QSettings settings("gnuplot", "qtterminal");
	m_antialias = settings.value("view/antialias", true).toBool();
	m_backgroundColor = settings.value("view/backgroundColor", Qt::red).value<QColor>();
	applySettings();
}

void QtGnuplotWidget::applySettings()
{
	m_view->setRenderHints(m_antialias ? QPainter::Antialiasing : QPainter::TextAntialiasing);
	m_view->setBackgroundBrush(m_backgroundColor);
}

void QtGnuplotWidget::saveSettings()
{
	QSettings settings("gnuplot", "qtterminal");
	settings.setValue("view/antialias", m_antialias);
	settings.setValue("view/backgroundColor", m_backgroundColor);
}

#include "ui_QtGnuplotSettings.h"

void QtGnuplotWidget::showSettingsDialog()
{
	QDialog* settingsDialog = new QDialog(this);
	m_ui = new Ui_settingsDialog();
	m_ui->setupUi(settingsDialog);
	m_ui->antialiasCheckBox->setCheckState(m_antialias ? Qt::Checked : Qt::Unchecked);
	QPixmap samplePixmap(m_ui->sampleColorLabel->size());
	samplePixmap.fill(m_backgroundColor);
	m_ui->sampleColorLabel->setPixmap(samplePixmap);
	m_chosenBackgroundColor = m_backgroundColor;
	connect(m_ui->backgroundButton, SIGNAL(clicked()), this, SLOT(settingsSelectBackgroundColor()));
	settingsDialog->exec();

	if (settingsDialog->result() == QDialog::Accepted)
	{
		m_antialias = (m_ui->antialiasCheckBox->checkState() == Qt::Checked);
		m_backgroundColor = m_chosenBackgroundColor;
		applySettings();
		saveSettings();
	}
}

void QtGnuplotWidget::settingsSelectBackgroundColor()
{
	m_chosenBackgroundColor = QColorDialog::getColor(m_chosenBackgroundColor, this);
	QPixmap samplePixmap(m_ui->sampleColorLabel->size());
	samplePixmap.fill(m_chosenBackgroundColor);
	m_ui->sampleColorLabel->setPixmap(samplePixmap);
}

#include "moc_QtGnuplotWidget.cpp"
