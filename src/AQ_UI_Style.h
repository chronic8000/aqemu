/****************************************************************************
** Shared AQEMU UI look: cards, tabs, group boxes (app-wide).
****************************************************************************/
#ifndef AQ_UI_STYLE_H
#define AQ_UI_STYLE_H

#include <QString>
#include <QSize>

class QApplication;
class QWidget;
class QLayout;

/** Must run before QApplication is constructed (Qt 5 High-DPI). */
void AQ_Enable_High_Dpi();

/**
 * Host UI scale vs a 96-DPI baseline (logicalDotsPerInch / 96, floored at 1).
 * Prefer this over hard-coded pixel sizes anywhere in the UI.
 */
qreal AQ_Ui_Scale( const QWidget *hint = nullptr );

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
