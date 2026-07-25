/****************************************************************************
**
** Copyright (C) 2016 Tobias Gläßer
**
** This file is part of AQEMU.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor,
** Boston, MA  02110-1301, USA.
**
****************************************************************************/

#include <QFont>
#include <QPalette>

#include "Utils.h"
#include "Highlighted_Label.h"

Highlighted_Label::Highlighted_Label( QWidget *parent )
	: QLabel( parent )
{
	QFont f = font();
	f.setPointSize( qMax( 12, f.pointSize() + 2 ) );
	f.setWeight( QFont::DemiBold );
	setFont( f );

	QPalette pal = palette();
	const QColor background_color = pal.color( QPalette::Window );
	const QColor link_color = pal.color( QPalette::Link );
	const bool use_link = calculateContrast( background_color, link_color ) > 3.0;

	// Modern section header — larger type, clear rule, no tiny blue scrap.
	setStyleSheet( QStringLiteral(
		"Highlighted_Label {"
		"  font-size: 15px;"
		"  font-weight: 600;"
		"  letter-spacing: 0.3px;"
		"  color: %1;"
		"  padding: 10px 4px 6px 2px;"
		"  border-bottom: 2px solid palette(mid);"
		"  margin-top: 4px;"
		"  margin-bottom: 2px;"
		"  background: transparent;"
		"}"
	).arg( use_link ? QStringLiteral( "palette(link)" ) : QStringLiteral( "palette(window-text)" ) ) );
}

Highlighted_Label::Highlighted_Label()
{
}
