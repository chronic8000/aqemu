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

#include <QApplication>
#include <QFont>
#include <QPalette>

#include "Utils.h"
#include "AQ_UI_Style.h"
#include "Highlighted_Label.h"

Highlighted_Label::Highlighted_Label( QWidget *parent )
	: QLabel( parent )
{
	// Scale from the application font — no fixed pixel type size.
	QFont f = QApplication::font();
	if( f.pointSizeF() > 0 )
		f.setPointSizeF( f.pointSizeF() * 1.25 );
	else if( f.pixelSize() > 0 )
		f.setPixelSize( qRound( f.pixelSize() * 1.25 ) );
	f.setWeight( QFont::DemiBold );
	setFont( f );

	QPalette pal = palette();
	const QColor background_color = pal.color( QPalette::Window );
	const QColor link_color = pal.color( QPalette::Link );
	const bool use_link = calculateContrast( background_color, link_color ) > 3.0;

	const int pad_t = AQ_Px( 8, this );
	const int pad_b = AQ_Px( 5, this );
	const int rule = qMax( 1, AQ_Px( 2, this ) );

	setStyleSheet( QStringLiteral(
		"Highlighted_Label {"
		"  font-weight: 600;"
		"  color: %1;"
		"  padding: %2px %3px %4px %3px;"
		"  border-bottom: %5px solid palette(mid);"
		"  margin-top: %6px;"
		"  margin-bottom: %7px;"
		"  background: transparent;"
		"}"
	).arg( use_link ? QStringLiteral( "palette(link)" ) : QStringLiteral( "palette(window-text)" ) )
	 .arg( pad_t ).arg( AQ_Px( 2, this ) ).arg( pad_b )
	 .arg( rule ).arg( AQ_Px( 4, this ) ).arg( AQ_Px( 2, this ) ) );
}

Highlighted_Label::Highlighted_Label()
{
}
