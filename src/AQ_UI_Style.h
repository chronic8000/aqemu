/****************************************************************************
** Shared AQEMU UI look: cards, tabs, group boxes (app-wide).
****************************************************************************/
#ifndef AQ_UI_STYLE_H
#define AQ_UI_STYLE_H

#include <QString>

class QApplication;
class QWidget;
class QLayout;

/** Install the global AQEMU chrome (group boxes, tabs, lists, inputs). */
void AQ_Apply_App_Style( QApplication *app );

/** Style a flat QWidget as a bordered content card (objectName required). */
void AQ_Style_Card( QWidget *w, int max_width = 0 );

/** Move an existing layout into a scrollable inner page (packs to top). */
void AQ_Make_Tab_Scrollable( QWidget *tab, const QString &inner_object_name = QString() );

/** Collapse large expanding vertical spacers to small fixed gaps. */
void AQ_Tighten_Layout_Spacers( QLayout *layout, int gap_px = 6 );

/** Cap readable content width on common panels under root. */
void AQ_Cap_Content_Width( QWidget *root, int max_width = 960 );

#endif
