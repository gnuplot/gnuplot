/* GNUPLOT - QtGnuplotItems.cpp */

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

#include "QtGnuplotItems.h"

#include <QtGui>

/////////////////////////////
// QtGnuplotEnhanced

void QtGnuplotEnhanced::addText(const QString& fontName, double fontSize, double base, bool widthFlag,
                                bool showFlag, int overprint, const QString& text, QColor color)
{
	if ((overprint == 1) && !(m_overprintMark)) // Underprint
	{
		m_overprintPos = m_currentPos.x();
		m_overprintMark = true;
	}
	if (overprint == 3)                         // Save position
		m_savedPos = m_currentPos;
	else if (overprint == 4)                    // Recall saved position
		m_currentPos = m_savedPos;

	QFont font(fontName, fontSize);
	QtGnuplotEnhancedFragment* item = new QtGnuplotEnhancedFragment(font, text, this);
	item->setPos(m_currentPos + QPointF(0., -base));
	if (showFlag)
		item->setPen(color);
	else
		item->setPen(Qt::NoPen);

	if (overprint == 2)                         // Overprint
	{
		item->setPos(QPointF((m_overprintPos + m_currentPos.x())/2. - (item->boundingRect().right() + item->boundingRect().left())/2., -base));
		m_overprintMark = false;
	}

	if (widthFlag && (overprint != 2))
		m_currentPos += QPointF(item->boundingRect().right(), 0.);
}

void QtGnuplotEnhanced::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(painter);
	Q_UNUSED(option);
	Q_UNUSED(widget);
}

QtGnuplotEnhancedFragment::QtGnuplotEnhancedFragment(const QFont& font, const QString& text, QGraphicsItem * parent)
	: QAbstractGraphicsShapeItem(parent)
{
	m_font = font;
	m_text = text;
}

QRectF QtGnuplotEnhancedFragment::boundingRect() const
{
	QFontMetricsF metrics(m_font);
	return metrics.boundingRect(m_text);
}

void QtGnuplotEnhancedFragment::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	painter->setPen(pen());
	painter->setFont(m_font);
	painter->drawText(QPointF(0.,0.), m_text);
}

/////////////////////////////
// QtGnuplotPoint

QtGnuplotPoint::QtGnuplotPoint(int style, int size, QColor color, QGraphicsItem * parent)
	: QGraphicsItem(parent)
{
	m_color = color;
	m_style = style;
	m_size = 3.*size;
}

QRectF QtGnuplotPoint::boundingRect() const
{
	return QRectF(QPointF(-m_size, -m_size), QPointF(m_size, m_size));
}

void QtGnuplotPoint::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	int style = m_style % 13;

	painter->setPen(m_color);
	if ((style % 2 == 0) && (style > 3)) // Filled points
		painter->setBrush(m_color);

	painter->drawPoint(0, 0);

	if ((style == 0) || (style == 2)) // plus or star
	{
		painter->drawLine(QPointF(0., -m_size), QPointF(0., m_size));
		painter->drawLine(QPointF(-m_size, 0.), QPointF(m_size, 0.));
	}
	if ((style == 1) || (style == 2)) // cross or star
	{
		painter->drawLine(QPointF(-m_size, -m_size), QPointF(m_size, m_size));
		painter->drawLine(QPointF(-m_size, m_size), QPointF(m_size, -m_size));
	}
	else if ((style == 3) || (style == 4)) // box
		painter->drawRect(boundingRect());
	else if ((style == 5) || (style == 6)) // circle
		painter->drawEllipse(boundingRect());
	else if ((style == 7) || (style == 8)) // triangle
	{
		const QPointF p[3] = { QPointF(0., -m_size),
		                       QPointF(.866*m_size, .5*m_size),
		                       QPointF(-.866*m_size, .5*m_size)};
		painter->drawPolygon(p, 3);
	}
	else if ((style == 9) || (style == 10)) // upside down triangle
	{
		const QPointF p[3] = { QPointF(0., m_size),
		                       QPointF(.866*m_size, -.5*m_size),
		                       QPointF(-.866*m_size, -.5*m_size)};
		painter->drawPolygon(p, 3);
	}
	else if ((style == 11) || (style == 12)) // diamond
	{
		const QPointF p[4] = { QPointF(0., m_size),
		                       QPointF(m_size, 0.),
		                       QPointF(0., -m_size),
		                       QPointF(-m_size, 0.)};
		painter->drawPolygon(p, 4);
	}
}
