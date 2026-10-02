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
#include <QEvent>
#include <QTimer>
#ifdef Q_OS_WIN
#include <QSettings>
#endif

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

static bool s_chrome_is_light = true;
static bool s_applying_style = false;
static bool s_theme_refresh_pending = false;

static bool AQ_Color_Is_Dark( const QColor &c )
{
	return c.isValid() && c.lightness() < 128;
}

#ifdef Q_OS_WIN
static bool AQ_Windows_Apps_Use_Dark()
{
	QSettings reg(
		QStringLiteral( "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize" ),
		QSettings::NativeFormat );
	if( ! reg.contains( QStringLiteral( "AppsUseLightTheme" ) ) )
		return false;
	return reg.value( QStringLiteral( "AppsUseLightTheme" ) ).toInt() == 0;
}
#endif

static bool AQ_Os_Prefers_Dark( const QApplication *app )
{
	if( ! app )
		return false;
#ifdef Q_OS_WIN
	// Qt 5 keeps a light palette on Windows even when Apps are in dark mode.
	if( AQ_Windows_Apps_Use_Dark() )
		return true;
#endif
	return AQ_Color_Is_Dark( app->palette().color( QPalette::Window ) );
}

static void AQ_Install_Dark_Palette( QApplication *app )
{
	QPalette pal = app->palette();
	const QColor window( 32, 32, 32 );
	const QColor base( 43, 43, 43 );
	const QColor text( 240, 240, 240 );
	const QColor disabled( 140, 140, 140 );
	QColor highlight = pal.color( QPalette::Highlight );
	if( highlight.lightness() > 180 || highlight.lightness() < 40 )
		highlight = QColor( 0, 120, 215 );

	pal.setColor( QPalette::Window, window );
	pal.setColor( QPalette::WindowText, text );
	pal.setColor( QPalette::Base, base );
	pal.setColor( QPalette::AlternateBase, QColor( 50, 50, 50 ) );
	pal.setColor( QPalette::ToolTipBase, window );
	pal.setColor( QPalette::ToolTipText, text );
	pal.setColor( QPalette::Text, text );
	pal.setColor( QPalette::Button, QColor( 50, 50, 50 ) );
	pal.setColor( QPalette::ButtonText, text );
	pal.setColor( QPalette::BrightText, Qt::white );
	pal.setColor( QPalette::Highlight, highlight );
	pal.setColor( QPalette::HighlightedText, Qt::white );
	pal.setColor( QPalette::Link, QColor( 100, 180, 255 ) );
	pal.setColor( QPalette::Mid, QColor( 90, 90, 90 ) );
	pal.setColor( QPalette::Dark, QColor( 20, 20, 20 ) );
	pal.setColor( QPalette::Shadow, Qt::black );
	pal.setColor( QPalette::Light, QColor( 70, 70, 70 ) );
	pal.setColor( QPalette::Midlight, QColor( 60, 60, 60 ) );
	pal.setColor( QPalette::Disabled, QPalette::Text, disabled );
	pal.setColor( QPalette::Disabled, QPalette::WindowText, disabled );
	pal.setColor( QPalette::Disabled, QPalette::ButtonText, disabled );
	app->setPalette( pal );
}

static void AQ_Refresh_Page_Surfaces()
{
	const QWidgetList tops = QApplication::topLevelWidgets();
	for( QWidget *top : tops )
	{
		if( ! top )
			continue;
		QList<QWidget *> pages = top->findChildren<QWidget *>();
		if( top->property( "aq_page_surface" ).toBool() )
			pages.prepend( top );
		for( QWidget *w : pages )
		{
			if( w && w->property( "aq_page_surface" ).toBool() )
				AQ_Apply_Page_Surface( w );
		}
	}
}

class AQ_Theme_Filter : public QObject
{
public:
	explicit AQ_Theme_Filter( QObject *parent )
		: QObject( parent )
	{}

protected:
	bool eventFilter( QObject *watched, QEvent *event ) override
	{
		Q_UNUSED( watched );
		if( s_applying_style || ! event )
			return false;
		const QEvent::Type type = event->type();
		if( type != QEvent::ApplicationPaletteChange && type != QEvent::ThemeChange )
			return false;
		if( s_theme_refresh_pending )
			return false;
		s_theme_refresh_pending = true;
		QTimer::singleShot( 0, this, []() {
			s_theme_refresh_pending = false;
			if( s_applying_style )
				return;
			QApplication *app = qobject_cast<QApplication *>( qApp );
			if( ! app )
				return;
			const bool want_light = ! AQ_Os_Prefers_Dark( app );
			const bool palette_is_dark = AQ_Color_Is_Dark( app->palette().color( QPalette::Window ) );
			if( want_light == s_chrome_is_light && want_light != palette_is_dark )
				return;
			AQ_Apply_App_Style( app );
		} );
		return false;
	}
};

bool AQ_Chrome_Is_Light()
{
	return s_chrome_is_light;
}

void AQ_Apply_Page_Surface( QWidget *w )
{
	if( ! w )
		return;
	w->setProperty( "aq_page_surface", true );
	w->setAutoFillBackground( true );
	QPalette p = QApplication::palette();
	if( s_chrome_is_light )
	{
		p.setColor( QPalette::Window, QColor( 255, 255, 255 ) );
		p.setColor( QPalette::Base, QColor( 255, 255, 255 ) );
	}
	w->setPalette( p );
}

void AQ_Apply_App_Style( QApplication *app )
{
	if( ! app || s_applying_style )
		return;

	s_applying_style = true;

	const bool want_light = ! AQ_Os_Prefers_Dark( app );
	s_chrome_is_light = want_light;

	if( want_light )
	{
		// Light chrome: white page background (Windows palette(window) is grey).
		QPalette pal = app->palette();
		const QColor white( 255, 255, 255 );
		if( pal.color( QPalette::Window ) != white || pal.color( QPalette::Base ) != white )
		{
			pal.setColor( QPalette::Window, white );
			pal.setColor( QPalette::Base, white );
			pal.setColor( QPalette::AlternateBase, QColor( 248, 248, 248 ) );
			app->setPalette( pal );
		}
	}
	else if( ! AQ_Color_Is_Dark( app->palette().color( QPalette::Window ) ) )
	{
		// OS is dark but Qt still handed us a light palette (Windows).
		AQ_Install_Dark_Palette( app );
	}

	// CRITICAL (Qt 5 + Windows HiDPI): never put padding / min-height / border-radius
	// on QComboBox, QLineEdit, QSpinBox, QPushButton, QCheckBox, QRadioButton, or
	// QToolButton in a global stylesheet. Those rules shrink the content rect and
	// clip glyphs mid-letter. Keep chrome-only styling; leave form controls native.
	//
	// QMenuBar::item must set background and border. Styling QMenuBar alone makes
	// Qt draw each title (File, VM, Help) as a bordered button on every platform.
	app->setStyleSheet( QStringLiteral( R"(
QMainWindow, QDialog {
	background-color: palette(window);
	color: palette(window-text);
}

QGroupBox {
	font-weight: 600;
	border: 1px solid palette(mid);
	border-radius: 6px;
	margin-top: 12px;
	padding-top: 8px;
	background-color: palette(base);
	color: palette(window-text);
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
	border: 1px solid palette(mid);
	border-radius: 4px;
	background: palette(base);
	color: palette(text);
	outline: 0;
}
QListWidget::item:selected, QTreeWidget::item:selected, QTableWidget::item:selected {
	background: palette(highlight);
	color: palette(highlighted-text);
}

QScrollArea {
	border: none;
	background: palette(window);
}
QSplitter::handle {
	background: palette(mid);
}

QStatusBar {
	border-top: 1px solid palette(mid);
	background: palette(window);
	color: palette(window-text);
}
QMenuBar {
	background: palette(window);
	color: palette(window-text);
	border: none;
	border-bottom: 1px solid palette(mid);
}
QMenuBar::item {
	background: transparent;
	color: palette(window-text);
	padding: 4px 10px;
	margin: 1px 0;
	border: none;
	border-radius: 3px;
}
QMenuBar::item:selected, QMenuBar::item:pressed {
	background: palette(highlight);
	color: palette(highlighted-text);
	border: none;
}
QMenuBar::item:disabled {
	background: transparent;
	color: palette(disabled, window-text);
	border: none;
}
QMenu {
	background: palette(base);
	color: palette(text);
	border: 1px solid palette(mid);
}
QMenu::item {
	background: transparent;
	color: palette(text);
	padding: 5px 28px 5px 16px;
	border: none;
}
QMenu::item:selected {
	background: palette(highlight);
	color: palette(highlighted-text);
	border: none;
}
QMenu::item:disabled {
	background: transparent;
	color: palette(disabled, text);
	border: none;
}
QMenu::separator {
	height: 1px;
	background: palette(mid);
	margin: 4px 8px;
}

QToolTip {
	border: 1px solid palette(mid);
	background: palette(tool-tip-base);
	color: palette(tool-tip-text);
}
)" ) );

	static bool filter_installed = false;
	if( ! filter_installed )
	{
		filter_installed = true;
		app->installEventFilter( new AQ_Theme_Filter( app ) );
	}

	AQ_Refresh_Page_Surfaces();
	s_applying_style = false;
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

void AQ_Intelligently_Size_Dialog( QWidget *dialog, int preferred_w, int preferred_h )
{
	if( ! dialog )
		return;

	QScreen *scr = dialog->screen();
	if( ! scr && dialog->parentWidget() )
		scr = dialog->parentWidget()->screen();
	if( ! scr )
		scr = QGuiApplication::primaryScreen();

	const QRect avail = scr ? scr->availableGeometry() : QRect( 0, 0, 1024, 768 );

	// Leave margin for desktop taskbars, docks, and OS window decorations
	const int max_w = qMax( 360, avail.width() - 32 );
	const int max_h = qMax( 280, avail.height() - 48 );

	// Scaled preferred target bounds
	const int target_w = qBound( 380, qMin( AQ_Px( preferred_w, dialog ), static_cast<int>( avail.width() * 0.94 ) ), max_w );
	const int target_h = qBound( 280, qMin( AQ_Px( preferred_h, dialog ), static_cast<int>( avail.height() * 0.88 ) ), max_h );

	dialog->resize( target_w, target_h );

	// Ensure dialog geometry stays inside available screen bounds
	QRect rect = dialog->geometry();
	rect.setSize( QSize( target_w, target_h ) );
	if( ! avail.contains( rect ) )
	{
		rect.moveCenter( avail.center() );
		if( rect.top() < avail.top() )
			rect.moveTop( avail.top() );
		if( rect.left() < avail.left() )
			rect.moveLeft( avail.left() );
		dialog->setGeometry( rect );
	}
}
