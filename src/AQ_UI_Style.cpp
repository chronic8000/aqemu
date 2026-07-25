/****************************************************************************
** Shared AQEMU UI look: cards, tabs, group boxes (app-wide).
****************************************************************************/

#include "AQ_UI_Style.h"

#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QStyle>
#include <QStylePainter>
#include <QStyleOptionTab>
#include <QPainter>
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
#include <QTabBar>
#include <QListWidget>
#include <QtGlobal>
#include <QCoreApplication>
#include <QWindow>
#include <QPaintEvent>

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

AQ_West_TabBar::AQ_West_TabBar( QWidget *parent )
	: QTabBar( parent )
{
	setExpanding( false );
	setUsesScrollButtons( true );
	QFont f = QApplication::font();
	f.setStyleHint( QFont::SansSerif, QFont::PreferAntialias );
	f.setStyleStrategy( QFont::PreferAntialias );
	f.setWeight( QFont::Medium );
	setFont( f );
	QStyle *st = style();
	const int icon = st ? st->pixelMetric( QStyle::PM_SmallIconSize, nullptr, this ) : 16;
	setIconSize( QSize( icon, icon ) );
}

QSize AQ_West_TabBar::tabSizeHint( int index ) const
{
	const QFontMetrics fm( font() );
	const QSize ic = iconSize();
	const int pad = qMax( 6, fm.height() / 3 );
	const int text_len = fm.horizontalAdvance( tabText( index ) );
	// West rail: width = thickness, height = length along the bar.
	const int thick = qMax( ic.width(), fm.height() ) + pad;
	const int along = qMax( text_len, ic.height() ) + pad * 2;
	return QSize( thick, along );
}

void AQ_West_TabBar::paintEvent( QPaintEvent *event )
{
	Q_UNUSED( event );
	QStylePainter painter( this );
	painter.setRenderHint( QPainter::Antialiasing, true );
	painter.setRenderHint( QPainter::TextAntialiasing, true );
	painter.setRenderHint( QPainter::SmoothPixmapTransform, true );

	for( int i = 0; i < count(); ++i )
	{
		QStyleOptionTab opt;
		initStyleOption( &opt, i );
		painter.drawControl( QStyle::CE_TabBarTabShape, opt );

		painter.save();
		QSize s = opt.rect.size();
		s.transpose();
		QRect r( QPoint(), s );
		r.moveCenter( opt.rect.center() );
		opt.shape = QTabBar::RoundedNorth;
		opt.rect = r;

		const QPoint c = tabRect( i ).center();
		painter.translate( c );
		painter.rotate( 90 );
		painter.translate( -c );
		painter.drawControl( QStyle::CE_TabBarTabLabel, opt );
		painter.restore();
	}
}

void AQ_Install_West_TabBar( QTabWidget *tabs )
{
	if( ! tabs )
		return;
	if( tabs->property( "aq_west_tabbar" ).toBool() )
		return;

	// setTabBar() is protected — access via a thin derived type.
	struct Tab_Access : public QTabWidget {
		using QTabWidget::setTabBar;
	};
	auto *bar = new AQ_West_TabBar( tabs );
	static_cast<Tab_Access *>( tabs )->setTabBar( bar );
	tabs->setTabPosition( QTabWidget::West );
	tabs->setProperty( "aq_west_tabbar", true );
}

void AQ_Apply_App_Style( QApplication *app )
{
	if( ! app )
		return;

	// Prefer font metrics over hard-coded min-heights — QSS min-height + padding
	// clips QComboBox/QLineEdit text and top-aligns QPushButton labels on HiDPI.
	const QFontMetrics fm( app->font() );
	const int pad_v = qMax( 4, fm.height() / 5 );
	const int pad_h = qMax( 8, fm.averageCharWidth() );
	const int btn_pad_v = qMax( 5, fm.height() / 4 );
	const int btn_pad_h = qMax( 12, fm.averageCharWidth() * 2 );
	const int drop_w = qMax( 18, fm.height() );

	app->setStyleSheet( QStringLiteral(
"/* —— Global chrome —— */\n"
"QMainWindow, QDialog {\n"
"	background-color: palette(window);\n"
"}\n"
"\n"
"QGroupBox {\n"
"	font-weight: 600;\n"
"	border: 1px solid palette(mid);\n"
"	border-radius: 6px;\n"
"	margin-top: 12px;\n"
"	padding: 10px 8px 8px 8px;\n"
"	background-color: palette(base);\n"
"}\n"
"QGroupBox::title {\n"
"	subcontrol-origin: margin;\n"
"	subcontrol-position: top left;\n"
"	left: 10px;\n"
"	padding: 0 6px;\n"
"	color: palette(window-text);\n"
"}\n"
"\n"
"/* Never style QTabWidget::pane / QTabBar globally — any pane rule breaks\n"
"   QTabWidget::West on Qt 5 (content area paints blank). Style North tabs\n"
"   on specific widgets only if needed. */\n"
"\n"
"QListWidget, QTreeWidget, QTableWidget {\n"
"	border: 1px solid palette(mid);\n"
"	border-radius: 4px;\n"
"	background: palette(base);\n"
"	padding: 2px;\n"
"	outline: 0;\n"
"}\n"
"QListWidget::item, QTreeWidget::item {\n"
"	padding: 4px 6px;\n"
"	border-radius: 3px;\n"
"}\n"
"QListWidget::item:selected, QTreeWidget::item:selected {\n"
"	background: palette(highlight);\n"
"	color: palette(highlighted-text);\n"
"}\n"
"\n"
"QLineEdit, QSpinBox, QDoubleSpinBox, QTextEdit, QPlainTextEdit {\n"
"	padding: %1px %2px;\n"
"	border: 1px solid palette(mid);\n"
"	border-radius: 4px;\n"
"	background: palette(base);\n"
"	selection-background-color: palette(highlight);\n"
"}\n"
"QComboBox {\n"
"	padding: %1px %2px;\n"
"	padding-right: %3px;\n"
"	border: 1px solid palette(mid);\n"
"	border-radius: 4px;\n"
"	background: palette(base);\n"
"	selection-background-color: palette(highlight);\n"
"}\n"
"QComboBox:editable {\n"
"	padding: 0px;\n"
"}\n"
"QComboBox:editable QLineEdit {\n"
"	padding: %1px %2px;\n"
"	border: none;\n"
"	background: transparent;\n"
"}\n"
"QComboBox::drop-down {\n"
"	border: none;\n"
"	width: %3px;\n"
"}\n"
"QComboBox QAbstractItemView {\n"
"	border: 1px solid palette(mid);\n"
"	selection-background-color: palette(highlight);\n"
"	padding: 2px;\n"
"}\n"
"\n"
"QPushButton {\n"
"	padding: %4px %5px;\n"
"	border: 1px solid palette(mid);\n"
"	border-radius: 4px;\n"
"	background: palette(button);\n"
"}\n"
"QPushButton:hover {\n"
"	background: palette(light);\n"
"}\n"
"QPushButton:pressed {\n"
"	background: palette(midlight);\n"
"}\n"
"QPushButton:default {\n"
"	font-weight: 600;\n"
"	border-color: palette(highlight);\n"
"}\n"
"QToolButton {\n"
"	padding: %1px %2px;\n"
"	border-radius: 4px;\n"
"}\n"
"\n"
"QCheckBox, QRadioButton {\n"
"	spacing: 6px;\n"
"	padding: 2px 0;\n"
"}\n"
	).arg( pad_v ).arg( pad_h ).arg( drop_w ).arg( btn_pad_v ).arg( btn_pad_h )
	+ QStringLiteral( R"(
QScrollBar:vertical {
	width: 12px;
	margin: 0;
}
QScrollBar:horizontal {
	height: 12px;
	margin: 0;
}

QSlider::groove:horizontal {
	height: 6px;
	background: palette(mid);
	border-radius: 3px;
}
QSlider::handle:horizontal {
	width: 14px;
	margin: -5px 0;
	background: palette(highlight);
	border-radius: 7px;
}

QStatusBar {
	min-height: 24px;
}

QToolTip {
	padding: 4px 8px;
	border: 1px solid palette(mid);
	background: palette(base);
	color: palette(window-text);
}

QHeaderView::section {
	padding: 4px 8px;
	border: none;
	border-right: 1px solid palette(mid);
	border-bottom: 1px solid palette(mid);
	background: palette(button);
}

QProgressBar {
	border: 1px solid palette(mid);
	border-radius: 4px;
	text-align: center;
	min-height: 16px;
}
QProgressBar::chunk {
	background: palette(highlight);
	border-radius: 3px;
}

QMenuBar {
	padding: 2px;
	background: palette(window);
}
QMenuBar::item {
	padding: 4px 10px;
	border-radius: 3px;
}
QMenuBar::item:selected {
	background: palette(highlight);
	color: palette(highlighted-text);
}
QMenu {
	border: 1px solid palette(mid);
	background: palette(base);
	padding: 4px;
}
QMenu::item {
	padding: 5px 28px 5px 24px;
	border-radius: 3px;
}
QMenu::item:selected {
	background: palette(highlight);
	color: palette(highlighted-text);
}
QMenu::separator {
	height: 1px;
	background: palette(mid);
	margin: 4px 8px;
}

QScrollArea {
	border: none;
	background: transparent;
}
QScrollBar::handle:vertical {
	background: palette(mid);
	border-radius: 4px;
	min-height: 24px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
	height: 0;
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
