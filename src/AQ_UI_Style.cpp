/****************************************************************************
** Shared AQEMU UI look: cards, tabs, group boxes (app-wide).
****************************************************************************/

#include "AQ_UI_Style.h"

#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QStyle>
#include <QFontMetrics>
#include <QWidget>
#include <QLayout>
#include <QLayoutItem>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFrame>
#include <QSizePolicy>
#include <QGroupBox>
#include <QTabWidget>
#include <QListWidget>
#include <QtGlobal>
#include <QCoreApplication>
#include <QWindow>

void AQ_Enable_High_Dpi()
{
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
	QCoreApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
	QCoreApplication::setAttribute( Qt::AA_UseHighDpiPixmaps );
#if QT_VERSION >= QT_VERSION_CHECK( 5, 14, 0 )
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
		Qt::HighDpiScaleFactorRoundingPolicy::PassThrough );
#endif
#endif
}

static QScreen *AQ_Hint_Screen( const QWidget *hint )
{
	if( hint )
	{
		if( QScreen *s = hint->screen() )
			return s;
		if( QWidget *top = hint->window() )
		{
			if( QWindow *wh = top->windowHandle() )
			{
				if( QScreen *s = wh->screen() )
					return s;
			}
		}
	}
	return QGuiApplication::primaryScreen();
}

qreal AQ_Ui_Scale( const QWidget *hint )
{
	Q_UNUSED( hint );
	// With AA_EnableHighDpiScaling, widget sizes are already device-independent
	// pixels — multiplying by logicalDPI/96 double-scales (huge West tabs on 4K).
	// Keep scale at 1; rely on QStyle / font metrics for proportional chrome.
	return qreal( 1.0 );
}

int AQ_Px( int baseline_96dpi, const QWidget *hint )
{
	Q_UNUSED( hint );
	if( baseline_96dpi <= 0 )
		return 0;
	// Baselines are DIP design sizes; Qt High-DPI maps them to physical pixels.
	return baseline_96dpi;
}

static QSize AQ_Style_Icon( QStyle::PixelMetric metric, qreal bump, const QWidget *hint )
{
	QStyle *style = hint && hint->style()
		? hint->style()
		: QApplication::style();
	int px = style ? style->pixelMetric( metric, nullptr, hint ) : 16;
	if( px < 8 )
		px = 16;
	px = qMax( 8, qRound( qreal( px ) * bump ) );
	return QSize( px, px );
}

QSize AQ_Toolbar_Icon_Size( const QWidget *hint )
{
	return AQ_Style_Icon( QStyle::PM_ToolBarIconSize, 1.0, hint );
}

QSize AQ_Nav_Icon_Size( const QWidget *hint )
{
	return AQ_Style_Icon( QStyle::PM_ListViewIconSize, 1.0, hint );
}

QSize AQ_Vm_List_Icon_Size( const QWidget *hint )
{
	return AQ_Style_Icon( QStyle::PM_LargeIconSize, 1.0, hint );
}

int AQ_Content_Max_Width( const QWidget *hint )
{
	const QFont font = hint ? hint->font() : QApplication::font();
	const QFontMetrics fm( font );
	const int ch = qMax( 1, fm.averageCharWidth() );
	return ch * 72; // ~readable column, scales with font (and High-DPI)
}

void AQ_Apply_App_Style( QApplication *app )
{
	if( ! app )
		return;

	app->setStyleSheet( QStringLiteral( R"(
/* —— Global chrome —— */
QMainWindow, QDialog {
	background-color: palette(window);
}

QGroupBox {
	font-weight: 600;
	border: 1px solid palette(mid);
	border-radius: 6px;
	margin-top: 12px;
	padding: 10px 8px 8px 8px;
	background-color: palette(base);
}
QGroupBox::title {
	subcontrol-origin: margin;
	subcontrol-position: top left;
	left: 10px;
	padding: 0 6px;
	color: palette(window-text);
}

/* Never style QTabWidget::pane / QTabBar globally — any pane rule breaks
   QTabWidget::West on Qt 5 (content area paints blank). Style North tabs
   on specific widgets only if needed. */

QListWidget, QTreeWidget, QTableWidget {
	border: 1px solid palette(mid);
	border-radius: 4px;
	background: palette(base);
	padding: 2px;
	outline: 0;
}
QListWidget::item, QTreeWidget::item {
	padding: 4px 6px;
	border-radius: 3px;
}
QListWidget::item:selected, QTreeWidget::item:selected {
	background: palette(highlight);
	color: palette(highlighted-text);
}

QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QTextEdit, QPlainTextEdit {
	padding: 3px 6px;
	border: 1px solid palette(mid);
	border-radius: 4px;
	background: palette(base);
	min-height: 22px;
	selection-background-color: palette(highlight);
}
QComboBox::drop-down {
	border: none;
	width: 20px;
}
QComboBox QAbstractItemView {
	border: 1px solid palette(mid);
	selection-background-color: palette(highlight);
}

QPushButton {
	padding: 5px 14px;
	border: 1px solid palette(mid);
	border-radius: 4px;
	background: palette(button);
	min-height: 22px;
}
QPushButton:hover {
	background: palette(light);
}
QPushButton:pressed {
	background: palette(midlight);
}
QPushButton:default {
	font-weight: 600;
	border-color: palette(highlight);
}
QToolButton {
	padding: 3px 8px;
	border-radius: 4px;
}

QCheckBox, QRadioButton {
	spacing: 6px;
	padding: 2px 0;
}

QSlider::groove:horizontal {
	height: 6px;
	border-radius: 3px;
	background: palette(mid);
}
QSlider::handle:horizontal {
	width: 14px;
	margin: -5px 0;
	border-radius: 7px;
	background: palette(highlight);
}

QScrollArea {
	border: none;
	background: transparent;
}
QScrollBar:vertical {
	width: 10px;
	background: transparent;
	margin: 0;
}
QScrollBar::handle:vertical {
	background: palette(mid);
	border-radius: 4px;
	min-height: 24px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
	height: 0;
}

QStatusBar {
	border-top: 1px solid palette(mid);
}
QMenuBar {
	background: palette(window);
	border-bottom: 1px solid palette(mid);
	padding: 2px;
}
QMenuBar::item:selected {
	background: palette(highlight);
	color: palette(highlighted-text);
	border-radius: 3px;
}
QSplitter::handle {
	background: palette(mid);
	width: 1px;
	height: 1px;
}

)" ) );
}

void AQ_Style_Card( QWidget *w, int max_width )
{
	if( ! w || w->objectName().isEmpty() )
		return;
	// Avoid CSS margin on QWidget — it breaks layout geometry on Qt 5.
	w->setStyleSheet(
		QStringLiteral(
			"QWidget#%1 {"
			"  background-color: palette(base);"
			"  border: 1px solid palette(mid);"
			"  border-radius: 6px;"
			"  padding: 4px;"
			"}"
		).arg( w->objectName() ) );
	if( max_width > 0 )
		w->setMaximumWidth( max_width );
}

void AQ_Tighten_Layout_Spacers( QLayout *layout, int gap_px )
{
	if( ! layout )
		return;
	if( gap_px < 0 )
		gap_px = AQ_Px( 6 );
	for( int i = 0; i < layout->count(); ++i )
	{
		QLayoutItem *it = layout->itemAt( i );
		if( ! it )
			continue;
		if( QSpacerItem *sp = it->spacerItem() )
		{
			QSizePolicy::Policy vp = sp->sizePolicy().verticalPolicy();
			if( vp == QSizePolicy::Expanding || vp == QSizePolicy::MinimumExpanding ||
			    sp->sizeHint().height() > gap_px + AQ_Px( 4 ) )
			{
				sp->changeSize( AQ_Px( 20 ), gap_px, QSizePolicy::Minimum, QSizePolicy::Fixed );
			}
		}
		else if( QLayout *sub = it->layout() )
		{
			AQ_Tighten_Layout_Spacers( sub, gap_px );
		}
	}
}

void AQ_Make_Tab_Scrollable( QWidget *tab, const QString &inner_object_name )
{
	if( ! tab )
		return;
	QLayout *old = tab->layout();
	if( ! old )
		return;
	// Already wrapped?
	if( tab->findChild<QScrollArea*>( QStringLiteral( "AQ_Tab_Scroll" ),
	                                  Qt::FindDirectChildrenOnly ) )
		return;

	QWidget *inner = new QWidget();
	inner->setObjectName( inner_object_name.isEmpty()
		? QStringLiteral( "AQ_Tab_Inner" ) : inner_object_name );

	QVBoxLayout *innerLay = new QVBoxLayout( inner );
	innerLay->setContentsMargins( 10, 8, 14, 12 );
	innerLay->setSpacing( 6 );

	if( QVBoxLayout *vold = qobject_cast<QVBoxLayout*>( old ) )
	{
		while( vold->count() > 0 )
		{
			QLayoutItem *it = vold->takeAt( 0 );
			if( ! it )
				continue;
			if( QSpacerItem *sp = it->spacerItem() )
				sp->changeSize( 20, 6, QSizePolicy::Minimum, QSizePolicy::Fixed );
			innerLay->addItem( it );
		}
		delete vold;
	}
	else if( QGridLayout *gold = qobject_cast<QGridLayout*>( old ) )
	{
		// Lift the single primary child (usually a nested tab widget) into the scroll page.
		QList<QLayoutItem*> items;
		while( gold->count() > 0 )
			items.append( gold->takeAt( 0 ) );
		delete gold;
		for( QLayoutItem *it : items )
		{
			if( ! it )
				continue;
			if( QSpacerItem *sp = it->spacerItem() )
				sp->changeSize( 20, 6, QSizePolicy::Minimum, QSizePolicy::Fixed );
			innerLay->addItem( it );
		}
	}
	else
	{
		delete inner;
		return;
	}

	innerLay->addStretch( 1 );

	QScrollArea *scroll = new QScrollArea( tab );
	scroll->setObjectName( QStringLiteral( "AQ_Tab_Scroll" ) );
	scroll->setWidgetResizable( true );
	scroll->setFrameShape( QFrame::NoFrame );
	scroll->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	scroll->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	scroll->setWidget( inner );

	QVBoxLayout *outer = new QVBoxLayout( tab );
	outer->setContentsMargins( 0, 0, 0, 0 );
	outer->setSpacing( 0 );
	outer->addWidget( scroll, 1 );
}

void AQ_Cap_Content_Width( QWidget *root, int max_width )
{
	if( ! root )
		return;
	if( max_width < 0 )
		max_width = AQ_Content_Max_Width( root );
	const auto boxes = root->findChildren<QGroupBox*>();
	for( QGroupBox *gb : boxes )
	{
		if( gb && gb->maximumWidth() > max_width )
			gb->setMaximumWidth( max_width );
	}
}
