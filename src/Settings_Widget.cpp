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

#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
#include <QTabWidget>
#include <QBoxLayout>
#include <QApplication>
#include <QSplitter>
#include <QFontMetrics>
#include <QAbstractItemView>

#include "Settings_Widget.h"
#include "AQ_UI_Style.h"
#include "Utils.h"

#include <iostream>

My_List_Widget::My_List_Widget(QWidget* parent) : QListWidget(parent)
{
  
}

void My_List_Widget::wheelEvent(QWheelEvent* e)
{
    if ( e->angleDelta().y() > 0 )
    {
        if ( currentRow() == 0 )
            setCurrentRow(count()-1);
        else
            setCurrentRow(currentRow()-1);
        e->accept();
    }
    else if ( e->angleDelta().y() < 0 )
    {
        if ( currentRow() == count() -1 )
            setCurrentRow(0);
        else
            setCurrentRow(currentRow()+1);
        e->accept();
    }
}
                
Settings_Widget::Settings_Widget(QTabWidget* tab_widget, QBoxLayout::Direction dir, bool erase_margins, bool erase_parent_margins)
{
    QBoxLayout *l = nullptr;

    splitter = nullptr;
    stack = new QStackedWidget(this);
    list = new My_List_Widget(this);

    if ( dir == QBoxLayout::TopToBottom )
    {
        l = new QBoxLayout(QBoxLayout::LeftToRight);

        list->setFlow( QListView::TopToBottom );
        list->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
        list->setViewMode( QListView::ListMode );
        list->setMovement( QListView::Static );
        list->setWrapping( false );
    }
    else
    {
        l = new QBoxLayout(QBoxLayout::TopToBottom);

        // Icon beside text (like the VM list) — not tiny truncated IconMode captions.
        list->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
        list->setFlow( QListView::LeftToRight );
        list->setViewMode( QListView::ListMode );
        list->setMovement( QListView::Static );
        list->setWrapping( false );
        list->setHorizontalScrollMode( QAbstractItemView::ScrollPerPixel );

        stack->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Expanding );
    }

    splitter = new QSplitter(this);

    if ( dir != QBoxLayout::TopToBottom )
        splitter->setOrientation(Qt::Vertical);
    else
        splitter->setOrientation(Qt::Horizontal);

    splitter->addWidget(list);
    splitter->addWidget(stack);

    l->addWidget(splitter);

    if ( erase_margins ) 
        l->setContentsMargins(0,0,0,0);

    setLayout(l);

    int i = 0;
    QWidget* widget = nullptr;

    while( ( widget = tab_widget->widget(i) ) != 0 )
    {
        list->addItem( new QListWidgetItem( tab_widget->tabIcon(i), tab_widget->tabText(i) ) );
        stack->addWidget(widget);
    }


    connect(list, SIGNAL(currentRowChanged(int)), stack, SLOT(setCurrentIndex(int)));

    tab_widget->hide();

    tab_widget->parentWidget()->layout()->replaceWidget( tab_widget, this );

    if ( erase_margins && erase_parent_margins )
        tab_widget->parentWidget()->layout()->setContentsMargins(0,0,0,0);
    

    if ( dir != QBoxLayout::TopToBottom )
    {
        list->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        list->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
        list->setSpacing( AQ_Px( 4, this ) );
        const int pad = AQ_Px( 6, this );
        const int rad = AQ_Px( 6, this );
        list->setStyleSheet( QStringLiteral(
			"QListWidget {"
			"  background: transparent;"
			"  border: none;"
			"  border-bottom: 1px solid palette(mid);"
			"  padding: %1px %2px;"
			"  outline: 0;"
			"}"
			"QListWidget::item {"
			"  padding: %1px %3px;"
			"  margin-right: %2px;"
			"  border-radius: %4px;"
			"}"
			"QListWidget::item:selected {"
			"  background: palette(highlight);"
			"  color: palette(highlighted-text);"
			"}"
			"QListWidget::item:hover:!selected {"
			"  background: palette(alternate-base);"
			"}" ).arg( pad ).arg( AQ_Px( 2, this ) ).arg( AQ_Px( 10, this ) ).arg( rad ) );
        const int row = qMax( AQ_Nav_Icon_Size( this ).height(), QFontMetrics( list->font() ).height() )
			+ AQ_Px( 12, this );
        list->setMinimumHeight( row );
        list->setMaximumHeight( row + AQ_Px( 8, this ) );
    }
    else
    {
        if ( erase_margins )
            stack->setContentsMargins(0,0,0,0);
        list->setSpacing( 3 );
        list->setUniformItemSizes( true );
    }

    AQ_Cap_Content_Width( this );
}

void Settings_Widget::setCurrentIndex(int i)
{
    list->setCurrentRow(i);
}

QMap<QString,QList<Settings_Widget*>> Settings_Widget::groups = QMap<QString,QList<Settings_Widget*>>();

void Settings_Widget::addToGroup(QString g)
{
    if(g.isEmpty())
        return;

    my_Group = g;

    groups[g].append(this);
}

void Settings_Widget::syncGroupIconSizes(QString g)
{
    QList<Settings_Widget*> list = groups[g];

    QList<int> max_width;

    for( int i = 0; i < list.count(); i++ )
    {
        auto sw = list.at(i);
        const int row_h = qMax( sw->list->iconSize().height(),
			QFontMetrics( sw->list->font() ).height() ) + AQ_Px( 12, sw );

        for ( int j = 0; j < sw->list->count(); j++ )
        {
            QListWidgetItem *it = sw->list->item(j);
            const int icon_w = sw->list->iconSize().width() > 0
                ? sw->list->iconSize().width() + AQ_Px( 10, sw ) : AQ_Px( 28, sw );
            QFontMetrics fm( sw->list->font() );
            const int text_w = fm.horizontalAdvance( it->text() ) + AQ_Px( 24, sw );
            const int w = icon_w + text_w;

            if ( max_width.count() < j + 1 )
                max_width.append( w );
            else if ( w > max_width.at(j) )
                max_width.replace( j, w );

            Q_UNUSED( row_h );
        }
    }

    int min_total_list_width = AQ_Px( 20 );
    for ( int j = 0; j < max_width.count(); j++ )
        min_total_list_width += max_width.at(j) + AQ_Px( 8 );

    for( int i = 0; i < list.count(); i++ )
    {
        auto sw = list.at(i);
        const int row_h = qMax( sw->list->iconSize().height(),
			QFontMetrics( sw->list->font() ).height() ) + AQ_Px( 12, sw );

        for ( int j = 0; j < sw->list->count(); j++ )
            sw->list->item(j)->setSizeHint( QSize( max_width.at(j), row_h ) );

        sw->list->setMinimumHeight( row_h + AQ_Px( 10, sw ) );
        sw->list->setMaximumHeight( row_h + AQ_Px( 14, sw ) );
        sw->list->setMinimumWidth( qMin( min_total_list_width, AQ_Px( 1200, sw ) ) );
    }
}

void Settings_Widget::setIconSize(QSize s)
{
    list->setIconSize(s);
}

Settings_Widget::~Settings_Widget()
{
    delete list;
    delete stack;

    delete splitter;
}

