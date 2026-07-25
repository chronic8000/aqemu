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
#include <QPalette>
#include <QColor>
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

	// Light chrome: white page background (Windows palette(window) is grey).
	QPalette pal = app->palette();
	pal.setColor( QPalette::Window, QColor( 255, 255, 255 ) );
	pal.setColor( QPalette::Base, QColor( 255, 255, 255 ) );
	pal.setColor( QPalette::AlternateBase, QColor( 248, 248, 248 ) );
	app->setPalette( pal );

	// CRITICAL (Qt 5 + Windows HiDPI): never put padding / min-height / border-radius
	// on QComboBox, QLineEdit, QSpinBox, QPushButton, QCheckBox, QRadioButton, or
	// QToolButton in a global stylesheet. Those rules shrink the content rect and
	// clip glyphs mid-letter. Keep chrome-only styling; leave form controls native.
	app->setStyleSheet( QStringLiteral( R"(
QMainWindow, QDialog {
	background-color: #ffffff;
}

QGroupBox {
	font-weight: 600;
	border: 1px solid #e0e0e0;
	border-radius: 6px;
	margin-top: 12px;
	padding-top: 8px;
	background-color: #ffffff;
}
QGroupBox::title {
	subcontrol-origin: margin;
	subcontrol-position: top left;
	left: 10px;
	padding: 0 6px;
	color: palette(window-text);
}

/* Never style QTabWidget::pane / QTabBar globally — blanks West panes on Qt 5. */

QListWidget, QTreeWidget, QTableWidget {
	border: 1px solid #e0e0e0;
	border-radius: 4px;
	background: #ffffff;
	outline: 0;
}
QListWidget::item:selected, QTreeWidget::item:selected {
	background: palette(highlight);
	color: palette(highlighted-text);
}

QScrollArea {
	border: none;
	background: #ffffff;
}
QSplitter::handle {
	background: #e8e8e8;
}

QStatusBar {
	border-top: 1px solid #e0e0e0;
	background: #ffffff;
}
QMenuBar {
	background: #ffffff;
	border-bottom: 1px solid #e0e0e0;
}
QMenuBar::item:selected {
	background: palette(highlight);
	color: palette(highlighted-text);
}
QMenu::item:selected {
	background: palette(highlight);
	color: palette(highlighted-text);
}

QToolTip {
	border: 1px solid #e0e0e0;
	background: #ffffff;
	color: palette(window-text);
}
)" ) );
}

void AQ_Style_Card( QWidget *w, int max_width )
{
	if( ! w || w->objectName().isEmpty() )
		return;
	// Avoid CSS margin/padding on QWidget — it breaks layout geometry on Qt 5.
	w->setStyleSheet(
		QStringLiteral(
			"QWidget#%1 {"
			"  background-color: palette(base);"
			"  border: 1px solid palette(mid);"
			"  border-radius: 6px;"
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
			// Only collapse *vertical* expanding spacers. Touching horizontal ones
			// (sizeHint height ~20) left Memory/Audio/Win11 bunched on the left.
			const QSizePolicy::Policy vp = sp->sizePolicy().verticalPolicy();
			const QSizePolicy::Policy hp = sp->sizePolicy().horizontalPolicy();
			if( ( vp == QSizePolicy::Expanding || vp == QSizePolicy::MinimumExpanding ) &&
			    hp != QSizePolicy::Expanding && hp != QSizePolicy::MinimumExpanding )
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
	// Minimum vertical: grow with content so the scroll area scrolls instead of crushing.
	inner->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );
	inner->setAutoFillBackground( true );

	QVBoxLayout *innerLay = new QVBoxLayout( inner );
	innerLay->setContentsMargins( 10, 8, 14, 12 );
	innerLay->setSpacing( 6 );
	innerLay->setSizeConstraint( QLayout::SetMinimumSize );

	// Drain the old layout. Must use addWidget/addLayout — addItem() does NOT
	// reparent widgets, which left them as invisible siblings of the scroll area
	// (blank Machine tab on Qt 5 West tabs).
	QList<QLayoutItem *> items;
	while( old->count() > 0 )
		items.append( old->takeAt( 0 ) );
	delete old;

	for( QLayoutItem *it : items )
	{
		if( ! it )
			continue;
		if( QWidget *w = it->widget() )
		{
			innerLay->addWidget( w ); // reparents onto inner
			delete it;
		}
		else if( QLayout *sub = it->layout() )
		{
			innerLay->addLayout( sub );
			delete it;
		}
		else
		{
			if( QSpacerItem *sp = it->spacerItem() )
				sp->changeSize( 20, 6, QSizePolicy::Minimum, QSizePolicy::Fixed );
			innerLay->addItem( it );
		}
	}

	innerLay->addStretch( 1 );

	QScrollArea *scroll = new QScrollArea( tab );
	scroll->setObjectName( QStringLiteral( "AQ_Tab_Scroll" ) );
	scroll->setWidgetResizable( true );
	scroll->setFrameShape( QFrame::NoFrame );
	scroll->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	scroll->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	scroll->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	scroll->setWidget( inner );

	QVBoxLayout *outer = new QVBoxLayout( tab );
	outer->setContentsMargins( 0, 0, 0, 0 );
	outer->setSpacing( 0 );
	outer->addWidget( scroll, 1 );

	inner->adjustSize();
	inner->show();
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
