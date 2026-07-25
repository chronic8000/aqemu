/****************************************************************************
** Shared AQEMU UI look: cards, tabs, group boxes (app-wide).
****************************************************************************/
#ifndef AQ_UI_STYLE_H
#define AQ_UI_STYLE_H

#include <QString>
#include <QSize>
#include <QTabBar>

class QApplication;
class QWidget;
class QLayout;
class QTabWidget;

/** Must run before QApplication is constructed (Qt 5 High-DPI). */
void AQ_Enable_High_Dpi();

/**
 * Extra layout multiplier. With High-DPI enabled this stays 1.0 — Qt already
 * maps DIPs; multiplying by logicalDPI/96 double-scales chrome on 4K.
 */
qreal AQ_Ui_Scale( const QWidget *hint = nullptr );

/**
 * West (vertical) tab bar with a slim rail and antialiased rotated labels.
 * Replaces the stock bar on a QTabWidget (avoids fat/pixelated West tabs).
 */
class AQ_West_TabBar : public QTabBar
{
public:
	explicit AQ_West_TabBar( QWidget *parent = nullptr );
	QSize tabSizeHint( int index ) const override;

protected:
	void paintEvent( QPaintEvent *event ) override;
};

void AQ_Install_West_TabBar( QTabWidget *tabs );

/** Scale a size that was designed for 96 DPI into current logical pixels. */
int AQ_Px( int baseline_96dpi, const QWidget *hint = nullptr );

/** Toolbar / tab / list / VM-list icon sizes from QStyle + DPI (never hard-coded). */
QSize AQ_Toolbar_Icon_Size( const QWidget *hint = nullptr );
QSize AQ_Nav_Icon_Size( const QWidget *hint = nullptr );
QSize AQ_Vm_List_Icon_Size( const QWidget *hint = nullptr );

/** Readable content column width from font metrics (~72 average characters). */
int AQ_Content_Max_Width( const QWidget *hint = nullptr );

/** Install the global AQEMU chrome (group boxes, tabs, lists, inputs). */
void AQ_Apply_App_Style( QApplication *app );

/** Style a flat QWidget as a bordered content card (objectName required). */
void AQ_Style_Card( QWidget *w, int max_width = 0 );

/** Move an existing layout into a scrollable inner page (packs to top). */
void AQ_Make_Tab_Scrollable( QWidget *tab, const QString &inner_object_name = QString() );

/** Collapse large expanding vertical spacers to small fixed gaps. */
void AQ_Tighten_Layout_Spacers( QLayout *layout, int gap_px = -1 );

/** Cap readable content width on common panels under root. */
void AQ_Cap_Content_Width( QWidget *root, int max_width = -1 );

#endif
