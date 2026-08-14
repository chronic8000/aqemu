/****************************************************************************
** Full-window VM session widget
****************************************************************************/
#include "VM_Session_Widget.h"

#include <QVBoxLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QSettings>
#include <QMouseEvent>
#include <QStyle>
#include <QSizePolicy>
#include <QJsonObject>
#include <QJsonValue>
#include <QMainWindow>
#include <QMenuBar>
#include <QApplication>
#include <QTcpSocket>
#include <QHostAddress>
#include <QMenu>
#include <QToolButton>
#include <QThread>
#include <QFontMetrics>
#include <QLabel>
#include <QDesktopServices>
#include <QUrl>
#include <QCursor>

#include "VM.h"
#include "QMP_Client.h"
#include "Migrate_Progress_Dialog.h"
#include "Migrate_URI_Dialog.h"
#include "Embedded_Display/Spice_View.h"
#include "Utils.h"
#include "System_Info.h"
#include "Apple_SoC_Support.h"
#include "Apple_SoC_Button_Pad.h"
#include "Apple_SoC_Device_Tools_Window.h"
#include "Serial_Console_Window.h"
#include "AQ_UI_Style.h"

#include <QDir>
#include <QEventLoop>
#include <QThread>
#ifdef VNC_DISPLAY
#include "Embedded_Display/Machine_View.h"
#endif

static const int kHotZoneDwellMs = 1000; // hold top-center ~1s to reveal (Chrome-like)
static const int kToolbarHideMs = 700;   // hide soon after leaving the toolbar
static const int kDrivePollMs = 200;

static int AQ_Hot_Zone_Height( const QWidget *w )
{
	return qMax( AQ_Px( 40, w ), AQ_Toolbar_Icon_Size( w ).height() + AQ_Px( 20, w ) );
}

static QString AQ_Toolbar_Style( const QWidget *w )
{
	const int pad = AQ_Px( 4, w );
	const int btn_pad_h = AQ_Px( 6, w );
	const int rad = AQ_Px( 3, w );
	return QStringLiteral(
		"QToolBar { background: #2d2d30; border: none; border-bottom: 1px solid #1a1a1a;"
		" spacing: %1px; padding: %1px; }"
		"QToolButton { color: #eee; padding: %1px %2px; border-radius: %3px; }"
		"QToolButton:hover { background: rgba(255,255,255,30); }"
		"QToolButton:pressed { background: rgba(255,255,255,50); }"
	).arg( pad ).arg( btn_pad_h ).arg( rad );
}

static QString AQ_Drive_Light_Style( const QWidget *w, const QString &bg, const QString &border )
{
	const int d = qMax( AQ_Px( 14, w ), QFontMetrics( w ? w->font() : QApplication::font() ).height() / 2 );
	const int rad = d / 2;
	const int margin = AQ_Px( 3, w );
	return QStringLiteral(
		"QLabel { min-width: %1px; max-width: %1px; min-height: %1px; max-height: %1px;"
		" border-radius: %2px; background: %3; border: 1px solid %4; margin: 0 %5px; }"
	).arg( d ).arg( rad ).arg( bg ).arg( border ).arg( margin );
}

VM_Session_Widget::VM_Session_Widget( QWidget *parent )
	: QWidget( parent )
	, VM( nullptr )
	, QMP( nullptr )
	, Toolbar( nullptr )
	, Main_Layout( nullptr )
	, Stack( new QStackedWidget( this ) )
	, Placeholder( new QLabel( this ) )
	, Spice( new Spice_View( this ) )
#ifdef VNC_DISPLAY
	, Vnc( nullptr )
#endif
	, Spice_Port( 0 )
	, Vnc_Port( 0 )
	, Act_Fullscreen( nullptr )
	, Act_Pause( nullptr )
	, Act_Insert_CD( nullptr )
	, Act_Eject_CD( nullptr )
	, Act_Insert_FD0( nullptr )
	, Act_Eject_FD0( nullptr )
	, Act_Insert_FD1( nullptr )
	, Act_Eject_FD1( nullptr )
	, Act_Restore_IPSW( nullptr )
	, Act_Grab_Mouse( nullptr )
	, Act_CAD( nullptr )
	, Act_Shift_F10( nullptr )
	, Act_Apple_Home( nullptr )
	, Act_Apple_Power( nullptr )
	, Act_Apple_Vol_Down( nullptr )
	, Act_Apple_Vol_Up( nullptr )
	, Act_Apple_SOS( nullptr )
	, Act_Apple_More( nullptr )
	, Act_Button_Pad( nullptr )
	, Act_Guest_Internet( nullptr )
	, TB_USB( nullptr )
	, Menu_USB( nullptr )
	, USB_Enum_Busy( false )
	, Serial_Win( nullptr )
	, Button_Pad( nullptr )
	, Light_FD0( nullptr )
	, Light_FD1( nullptr )
	, Light_CD( nullptr )
	, Light_HD( nullptr )
	, Toolbar_Hide_Timer( new QTimer( this ) )
	, Toolbar_Show_Timer( new QTimer( this ) )
	, Drive_Poll_Timer( new QTimer( this ) )
	, Display_Connect_Timer( new QTimer( this ) )
	, Display_Connect_Attempts( 0 )
	, Display_Connect_In_Progress( false )
	, Fullscreen_Active( false )
	, Fullscreen_Toolbar_Visible( false )
	, Main_Chrome_Hidden( false )
{
	Main_Layout = new QVBoxLayout( this );
	Main_Layout->setContentsMargins( 0, 0, 0, 0 );
	Main_Layout->setSpacing( 0 );

	Placeholder->setAlignment( Qt::AlignCenter );
	Placeholder->setText( tr( "Starting embedded guest display…" ) );
	Stack->addWidget( Placeholder );
	Stack->addWidget( Spice );
#ifdef VNC_DISPLAY
	Vnc = new MachineView( this, nullptr );
	Stack->addWidget( Vnc );
#endif

	Build_Toolbar();
	Main_Layout->addWidget( Toolbar );
	Main_Layout->addWidget( Stack, 1 );

	Display_Connect_Timer->setSingleShot( true );
	connect( Display_Connect_Timer, SIGNAL(timeout()), this, SLOT(Try_Connect_Display()) );

	Toolbar_Hide_Timer->setSingleShot( true );
	Toolbar_Hide_Timer->setInterval( kToolbarHideMs );
	connect( Toolbar_Hide_Timer, SIGNAL(timeout()), this, SLOT(On_Toolbar_Hide_Timeout()) );

	Toolbar_Show_Timer->setSingleShot( true );
	Toolbar_Show_Timer->setInterval( kHotZoneDwellMs );
	connect( Toolbar_Show_Timer, SIGNAL(timeout()), this, SLOT(On_Toolbar_Show_Timeout()) );

	Drive_Poll_Timer->setInterval( kDrivePollMs );
	connect( Drive_Poll_Timer, SIGNAL(timeout()), this, SLOT(On_Drive_Poll()) );

	connect( Spice, SIGNAL(Connected()), this, SLOT(On_Display_Connected()) );
	connect( Spice, SIGNAL(Disconnected()), this, SLOT(On_Display_Disconnected()) );
	connect( Spice, SIGNAL(Connection_Error(QString)), this, SLOT(On_Display_Error(QString)) );
}

VM_Session_Widget::~VM_Session_Widget()
{
	if( Fullscreen_Active )
	{
		qApp->removeEventFilter( this );
		Set_Main_Window_Chrome_Visible( true );
	}
	if( QWidget *w = window() )
		w->removeEventFilter( this );
	Detach();
}

QAction *VM_Session_Widget::Add_Toolbar_Action( const QIcon &icon, const QString &tip, const char *slot )
{
	QAction *a = Toolbar->addAction( icon, tip, this, slot );
	a->setToolTip( tip );
	return a;
}

void VM_Session_Widget::Build_Toolbar()
{
	Toolbar = new QToolBar( this );
	Toolbar->setMovable( false );
	Toolbar->setFloatable( false );
	Toolbar->setIconSize( AQ_Toolbar_Icon_Size( this ) );
	Toolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
	Toolbar->setStyleSheet( AQ_Toolbar_Style( this ) );

	// Runtime power controls (replaces left Tool_Bar_VM_Control in session mode)
	Act_Pause = Add_Toolbar_Action( QIcon( ":/pause.png" ), tr( "Pause / Resume" ), SLOT(On_Pause()) );
	Add_Toolbar_Action( QIcon( ":/save-state.png" ), tr( "Save VM state" ), SLOT(On_Save()) );
	Add_Toolbar_Action( QIcon( ":/preferences-system-network.png" ), tr( "Migrate to URI…" ), SLOT(On_Migrate()) );
	Add_Toolbar_Action( QIcon( ":/restart.png" ), tr( "Reset guest" ), SLOT(On_Reset()) );
	Add_Toolbar_Action( QIcon( ":/shutdown.png" ), tr( "ACPI shutdown" ), SLOT(On_Shutdown()) );
	Add_Toolbar_Action( QIcon( ":/stop.png" ), tr( "Power off" ), SLOT(On_Power_Off()) );
	Toolbar->addSeparator();

	// Removable media
	Act_Insert_CD = Add_Toolbar_Action( QIcon( ":/cdrom.png" ), tr( "Insert CD/DVD image…" ), SLOT(On_Change_CD()) );
	Act_Eject_CD = Add_Toolbar_Action( QIcon( ":/eject.png" ), tr( "Eject CD/DVD" ), SLOT(On_Eject_CD()) );
	Toolbar->addSeparator();
	Act_Insert_FD0 = Add_Toolbar_Action( QIcon( ":/fdd.png" ), tr( "Insert floppy A image…" ), SLOT(On_Change_FD0()) );
	Act_Eject_FD0 = Add_Toolbar_Action( QIcon( ":/eject.png" ), tr( "Eject floppy A" ), SLOT(On_Eject_FD0()) );
	Act_Insert_FD1 = Add_Toolbar_Action( QIcon( ":/fdd.png" ), tr( "Insert floppy B image…" ), SLOT(On_Change_FD1()) );
	Act_Eject_FD1 = Add_Toolbar_Action( QIcon( ":/eject.png" ), tr( "Eject floppy B" ), SLOT(On_Eject_FD1()) );
	Toolbar->addSeparator();

	Act_Restore_IPSW = Add_Toolbar_Action(
		QIcon( ":/default_mac.png" ),
		tr( "Restore IPSW (Inferno companion + idevicerestore)…" ),
		SLOT(On_Restore_IPSW()) );
	Act_Restore_IPSW->setVisible( false );

	// Inferno hardware buttons (ChefKiss F-keys) — shown only for Apple SoC
	QAction *sep_apple = Toolbar->addSeparator();
	Apple_Sep_Actions << sep_apple;
	Act_Apple_Vol_Down = Toolbar->addAction( tr( "Vol−" ), this, SLOT(On_Apple_Vol_Down()) );
	Act_Apple_Vol_Down->setToolTip( tr( "Volume down (Inferno F3)" ) );
	Act_Apple_Vol_Up = Toolbar->addAction( tr( "Vol+" ), this, SLOT(On_Apple_Vol_Up()) );
	Act_Apple_Vol_Up->setToolTip( tr( "Volume up (Inferno F4)" ) );
	Act_Apple_Home = Toolbar->addAction( tr( "Home" ), this, SLOT(On_Apple_Home()) );
	Act_Apple_Home->setToolTip( tr(
		"Home / Menu (Inferno F6)\n"
		"Click: Home screen  |  Use More… for App Switcher (double Home)" ) );
	Act_Apple_Power = Toolbar->addAction( tr( "Side" ), this, SLOT(On_Apple_Power()) );
	Act_Apple_Power->setToolTip( tr( "Power / Side button (Inferno F5). More… for hold." ) );
	Act_Apple_SOS = Toolbar->addAction( tr( "SOS" ), this, SLOT(On_Apple_SOS()) );
	Act_Apple_SOS->setToolTip( tr( "SOS / slide-to-power-off (Vol up, then Side)" ) );
	Act_Apple_More = Toolbar->addAction( tr( "More…" ), this, SLOT(On_Apple_More_Buttons()) );
	Act_Apple_More->setToolTip( tr( "App Switcher, Power hold, Ringer, Force shutdown…" ) );
	Act_Button_Pad = Toolbar->addAction( tr( "Pad" ), this, SLOT(On_Toggle_Button_Pad()) );
	Act_Button_Pad->setCheckable( true );
	Act_Button_Pad->setToolTip( tr( "Show / hide floating iOS button pad (not an iPhone bezel)" ) );
	Act_Guest_Internet = Toolbar->addAction( tr( "Net" ), this, SLOT(On_Guest_Internet()) );
	Act_Guest_Internet->setToolTip( tr(
		"Enable guest internet (companion reverse-tether)\n"
		"Not the AQEMU Network NIC tab — opens Device Tools → Internet" ) );
	for( QAction *a : { Act_Apple_Vol_Down, Act_Apple_Vol_Up, Act_Apple_Home, Act_Apple_Power,
	                    Act_Apple_SOS, Act_Apple_More, Act_Button_Pad, Act_Guest_Internet, sep_apple } )
	{
		if( a )
			a->setVisible( false );
	}

	Toolbar->addSeparator();

	// USB hotplug (VMware-style connect/disconnect menu)
	Menu_USB = new QMenu( this );
	TB_USB = new QToolButton( Toolbar );
	TB_USB->setIcon( QIcon( ":/usb.png" ) );
	TB_USB->setToolTip( tr( "USB devices — connect / disconnect host USB to this guest" ) );
	TB_USB->setPopupMode( QToolButton::InstantPopup );
	TB_USB->setMenu( Menu_USB );
	TB_USB->setAutoRaise( true );
	Toolbar->addWidget( TB_USB );
	connect( Menu_USB, SIGNAL(aboutToShow()), this, SLOT(On_USB_Menu_About_To_Show()) );
	Toolbar->addSeparator();

	Add_Toolbar_Action( QIcon( ":/key.png" ), tr( "Serial console (guest COM / ttyS0)" ),
	                    SLOT(On_Serial_Console()) );
	Toolbar->addSeparator();

	// Guest / display
	Act_CAD = Add_Toolbar_Action( QIcon( ":/key.png" ), tr( "Send Ctrl+Alt+Del" ), SLOT(On_CAD()) );
	Act_Shift_F10 = Add_Toolbar_Action( QIcon( ":/key.png" ),
	                    tr( "Send Shift+F10 (Windows Setup / OOBE command prompt)" ),
	                    SLOT(On_Shift_F10()) );
	Act_Grab_Mouse = Add_Toolbar_Action(
		QIcon( ":/input-mouse.png" ),
		tr( "Grab mouse into guest — or click the guest display; Esc / Ctrl+Alt releases" ),
		SLOT(On_Grab_Mouse()) );
	Act_Grab_Mouse->setCheckable( true );
	Act_Fullscreen = Add_Toolbar_Action( QIcon( ":/fullscreen.png" ),
	                                     tr( "Fullscreen — hold mouse at top center to show toolbar" ),
	                                     SLOT(On_Fullscreen()) );
	Toolbar->addSeparator();
	Add_Toolbar_Action( QIcon( ":/exit.png" ),
	                    tr( "Back to VM list (guest keeps running — use Connect to return)" ),
	                    SLOT(On_Exit_View()) );

	// Drive activity lights (PCem / 86Box style) — right side
	QWidget *spacer = new QWidget( Toolbar );
	spacer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
	Toolbar->addWidget( spacer );

	QLabel *lights_label = new QLabel( tr( "Drives" ), Toolbar );
	lights_label->setStyleSheet( QStringLiteral(
		"QLabel { color: #888; margin-right: %1px; }" ).arg( AQ_Px( 2, this ) ) );
	Toolbar->addWidget( lights_label );

	Light_FD0 = Make_Drive_Light( "A" );
	Light_FD1 = Make_Drive_Light( "B" );
	Light_CD = Make_Drive_Light( "CD" );
	Light_HD = Make_Drive_Light( "HD" );
	Toolbar->addWidget( Light_FD0 );
	Toolbar->addWidget( Light_FD1 );
	Toolbar->addWidget( Light_CD );
	Toolbar->addWidget( Light_HD );
}

QLabel *VM_Session_Widget::Make_Drive_Light( const QString &letter )
{
	QLabel *l = new QLabel( Toolbar );
	l->setStyleSheet( AQ_Drive_Light_Style( this, QStringLiteral( "#3a3a3a" ), QStringLiteral( "#222" ) ) );
	l->setToolTip( letter );
	l->setAlignment( Qt::AlignCenter );
	return l;
}

void VM_Session_Widget::Update_Pause_Action()
{
	if( ! Act_Pause )
		return;

	const bool paused = VM && VM->Get_State() == VM::VMS_Pause;
	Act_Pause->setIcon( QIcon( paused ? ":/play.png" : ":/pause.png" ) );
	Act_Pause->setToolTip( paused ? tr( "Resume" ) : tr( "Pause" ) );
	Act_Pause->setText( Act_Pause->toolTip() );
}

void VM_Session_Widget::Set_Toolbar_In_Layout()
{
	if( ! Toolbar || ! Main_Layout )
		return;

	Toolbar->setParent( this );
	if( Main_Layout->indexOf( Toolbar ) < 0 )
		Main_Layout->insertWidget( 0, Toolbar );
	Toolbar->show();
}

void VM_Session_Widget::Set_Toolbar_Fullscreen_Overlay()
{
	if( ! Toolbar || ! Main_Layout )
		return;

	if( Main_Layout->indexOf( Toolbar ) >= 0 )
		Main_Layout->removeWidget( Toolbar );

	Toolbar->setParent( this );
	Toolbar->setMouseTracking( true );
	Toolbar->raise();
}

void VM_Session_Widget::Show_Fullscreen_Toolbar()
{
	if( ! Fullscreen_Active || Fullscreen_Toolbar_Visible )
		return;

	Fullscreen_Toolbar_Visible = true;
	Toolbar_Show_Timer->stop();
	Toolbar_Hide_Timer->stop();

	const int th = qMax( Toolbar->sizeHint().height(),
		AQ_Toolbar_Icon_Size( this ).height() + AQ_Px( 16, this ) );
	QRect start( 0, -th, width(), th );
	QRect end( 0, 0, width(), th );

	Toolbar->setGeometry( start );
	Toolbar->show();
	Toolbar->raise();

	QPropertyAnimation *anim = new QPropertyAnimation( Toolbar, "geometry", this );
	anim->setDuration( 180 );
	anim->setStartValue( start );
	anim->setEndValue( end );
	anim->setEasingCurve( QEasingCurve::OutCubic );
	anim->start( QAbstractAnimation::DeleteWhenStopped );
}

void VM_Session_Widget::Hide_Fullscreen_Toolbar()
{
	if( ! Fullscreen_Active || ! Fullscreen_Toolbar_Visible )
		return;

	Fullscreen_Toolbar_Visible = false;
	Toolbar_Hide_Timer->stop();
	Toolbar_Show_Timer->stop();

	const int th = Toolbar->height() > 0 ? Toolbar->height() : Toolbar->sizeHint().height();
	QRect start( Toolbar->geometry() );
	QRect end( 0, -th, width(), th );

	QPropertyAnimation *anim = new QPropertyAnimation( Toolbar, "geometry", this );
	anim->setDuration( 160 );
	anim->setStartValue( start );
	anim->setEndValue( end );
	anim->setEasingCurve( QEasingCurve::InCubic );
	connect( anim, SIGNAL(finished()), Toolbar, SLOT(hide()) );
	anim->start( QAbstractAnimation::DeleteWhenStopped );
}

bool VM_Session_Widget::Hot_Zone_Contains_Global( const QPoint &global_pos ) const
{
	const QPoint local = mapFromGlobal( global_pos );
	if( local.y() < 0 || local.y() > AQ_Hot_Zone_Height( this ) )
		return false;
	// Top-center band (middle third), like Chrome's F11 exit chip.
	const int cx = width() / 2;
	const int half = qMax( 140, width() / 6 );
	return local.x() >= cx - half && local.x() <= cx + half;
}

bool VM_Session_Widget::Toolbar_Contains_Global( const QPoint &global_pos ) const
{
	if( ! Toolbar || ! Toolbar->isVisible() )
		return false;
	return Toolbar->geometry().contains( mapFromGlobal( global_pos ) );
}

void VM_Session_Widget::Handle_Fullscreen_Mouse( const QPoint &global_pos )
{
	if( ! Fullscreen_Active )
		return;

	const bool over_toolbar = Fullscreen_Toolbar_Visible && Toolbar_Contains_Global( global_pos );
	const bool in_hot = Hot_Zone_Contains_Global( global_pos );

	if( over_toolbar )
	{
		// Stay up while the pointer is on the session toolbar.
		Toolbar_Show_Timer->stop();
		Toolbar_Hide_Timer->stop();
		return;
	}

	if( in_hot )
	{
		Toolbar_Hide_Timer->stop();
		if( ! Fullscreen_Toolbar_Visible && ! Toolbar_Show_Timer->isActive() )
			Toolbar_Show_Timer->start();
		return;
	}

	// Left hot zone before dwell completed — cancel reveal.
	Toolbar_Show_Timer->stop();
	if( Fullscreen_Toolbar_Visible && ! Toolbar_Hide_Timer->isActive() )
		Toolbar_Hide_Timer->start();
}

void VM_Session_Widget::Set_Main_Window_Chrome_Visible( bool visible )
{
	QMainWindow *mw = qobject_cast<QMainWindow *>( window() );
	if( ! mw )
		return;

	if( QMenuBar *mb = mw->menuBar() )
		mb->setVisible( visible );

	Main_Chrome_Hidden = ! visible;
}

void VM_Session_Widget::Update_Fullscreen_State()
{
	QWidget *w = window();
	const bool fs = w && w->isFullScreen();

	if( fs == Fullscreen_Active )
		return;

	Fullscreen_Active = fs;
	Fullscreen_Toolbar_Visible = false;
	Toolbar_Show_Timer->stop();
	Toolbar_Hide_Timer->stop();

	if( Act_Fullscreen )
		Act_Fullscreen->setIcon( QIcon( Fullscreen_Active ? ":/preferences-desktop-display.png" : ":/fullscreen.png" ) );

	if( Fullscreen_Active )
	{
		// Hide File/VM/Help — session toolbar is the only chrome in FS.
		Set_Main_Window_Chrome_Visible( false );
		Set_Toolbar_Fullscreen_Overlay();
		Toolbar->hide();
		// App filter so moves over Spice/VNC children still hit the hot zone.
		qApp->installEventFilter( this );
		setMouseTracking( true );
	}
	else
	{
		qApp->removeEventFilter( this );
		Set_Main_Window_Chrome_Visible( true );
		Set_Toolbar_In_Layout();
	}
}

void VM_Session_Widget::resizeEvent( QResizeEvent *event )
{
	QWidget::resizeEvent( event );
	if( Fullscreen_Active && Fullscreen_Toolbar_Visible )
		Toolbar->setGeometry( 0, 0, width(), Toolbar->sizeHint().height() );
	Position_Button_Pad();
}

void VM_Session_Widget::changeEvent( QEvent *event )
{
	if( event->type() == QEvent::WindowStateChange )
		Update_Fullscreen_State();
	QWidget::changeEvent( event );
}

bool VM_Session_Widget::eventFilter( QObject *obj, QEvent *event )
{
	Q_UNUSED( obj );
	if( Fullscreen_Active && event->type() == QEvent::MouseMove )
		Handle_Fullscreen_Mouse( static_cast<QMouseEvent *>( event )->globalPos() );

	return QWidget::eventFilter( obj, event );
}

void VM_Session_Widget::On_Toolbar_Hide_Timeout()
{
	if( Fullscreen_Active )
		Hide_Fullscreen_Toolbar();
}

void VM_Session_Widget::On_Toolbar_Show_Timeout()
{
	if( Fullscreen_Active )
		Show_Fullscreen_Toolbar();
}

QString VM_Session_Widget::Pick_Backend( const QString &preferred ) const
{
	QString p = preferred.toLower();
	if( p.isEmpty() )
		p = QSettings().value( "Embedded_Display_Backend", "vnc" ).toString().toLower();

#ifdef Q_OS_WIN32
	// Windows: spice-client-glib channel teardown / ERROR_LINK retries have been
	// crashing the whole AQEMU process. Prefer LibVNC (QEMU already opens -vnc).
	if( p != QLatin1String( "spice" ) )
	{
#ifdef VNC_DISPLAY
		return QStringLiteral( "vnc" );
#endif
	}
	// Explicit "spice" still allowed for debugging, but fall through carefully.
#endif

	if( Spice && Spice->Spice_Available() &&
	    ( p == QLatin1String( "spice" ) ) )
		return QStringLiteral( "spice" );

#ifdef VNC_DISPLAY
	return QStringLiteral( "vnc" );
#else
	if( Spice && Spice->Spice_Available() )
		return QStringLiteral( "spice" );
	return QStringLiteral( "none" );
#endif
}

void VM_Session_Widget::Attach_VM( Virtual_Machine *vm, QMP_Client *qmp,
                                   const QString &display_host,
                                   int spice_port, int vnc_port,
                                   const QString &preferred_backend )
{
	VM = vm;
	QMP = qmp;
	Host = display_host;
	Spice_Port = spice_port;
	Vnc_Port = vnc_port;
	Backend = Pick_Backend( preferred_backend );
	Display_Connect_Attempts = 0;
	Display_Connect_In_Progress = false;
	Display_Connect_Timer->stop();

	if( QMP_Client *q = Active_QMP() )
	{
		connect( q, SIGNAL(Connected()), this, SLOT(On_QMP_Connected()), Qt::UniqueConnection );
		connect( q, SIGNAL(Block_Stats(QJsonArray)), this, SLOT(On_Block_Stats(QJsonArray)), Qt::UniqueConnection );
		if( q->Is_Connected() )
			On_QMP_Connected();
	}

	Placeholder->setText( tr( "Connecting %1 display on %2…" )
		.arg( Backend.toUpper() ).arg( Host ) );
	Stack->setCurrentWidget( Placeholder );

	// Preparing: session shell is up, QEMU has not published ports yet.
	if( ( Backend == "spice" && Spice_Port <= 0 ) ||
	    ( Backend == "vnc" && Vnc_Port <= 0 ) )
	{
		Placeholder->setText( tr( "Starting guest…" ) );
		Stack->setCurrentWidget( Placeholder );
		if( ! Fullscreen_Active )
			Set_Toolbar_In_Layout();
		Update_Pause_Action();
		Update_Media_Actions();
		return;
	}

	// Wait until QEMU is actually listening — early SPICE connect causes
	// "incomplete link header" and a broken VNC fallback popup.
	Placeholder->setText( tr( "Waiting for guest display…" ) );
	Stack->setCurrentWidget( Placeholder );
	// Give spice-server a moment after TCP accept before the first handshake.
	Schedule_Display_Connect( 600 );

	if( ! Fullscreen_Active )
		Set_Toolbar_In_Layout();
	Update_Pause_Action();
	Update_Media_Actions();
}

bool VM_Session_Widget::Tcp_Port_Is_Open( const QString &host, int port ) const
{
	if( port <= 0 )
		return false;
	QTcpSocket sock;
	sock.connectToHost( host, static_cast<quint16>( port ) );
	const bool ok = sock.waitForConnected( 200 );
	if( ok )
		sock.disconnectFromHost();
	return ok;
}

void VM_Session_Widget::Schedule_Display_Connect( int delay_ms )
{
	Display_Connect_Timer->stop();
	Display_Connect_Timer->start( qMax( 50, delay_ms ) );
}

void VM_Session_Widget::Try_Connect_Display()
{
	if( ! VM )
		return;

	const bool want_spice = ( Backend == "spice" && Spice_Port > 0 && Spice && Spice->Spice_Available() );
	const int port = want_spice ? Spice_Port : Vnc_Port;
	if( port <= 0 )
		return;

	if( ! Tcp_Port_Is_Open( Host, port ) )
	{
		// Check if the underlying VM process has already exited
		if( VM && VM->Get_State() == VM::VMS_Power_Off )
		{
			QString detail = VM->QEMU_Stderr_History.trimmed();
			if( detail.isEmpty() )
				detail = VM->QEMU_Stdout_History.trimmed();
			const QString err_msg = detail.isEmpty()
				? tr( "The virtual machine exited before the guest display was ready." )
				: tr( "The virtual machine exited before the guest display was ready:\n\n%1" )
					.arg( detail );
			Placeholder->setText( err_msg );
			Stack->setCurrentWidget( Placeholder );
			AQGraphic_Error( "VM_Session_Widget::Try_Connect_Display",
			                 tr( "Virtual machine failed" ), err_msg, false );
			return;
		}

		++Display_Connect_Attempts;
		const int max_attempts = ( VM && AQ_Is_Apple_SoC_VM( VM ) ) ? 450 : 15; // ~3 min vs ~6 s
		const int retry_ms = ( VM && AQ_Is_Apple_SoC_VM( VM ) ) ? 400 : 400;
		if( Display_Connect_Attempts < max_attempts )
		{
			Placeholder->setText( tr( "Waiting for guest display… (%1)" )
				.arg( Display_Connect_Attempts ) );
			Stack->setCurrentWidget( Placeholder );
			Schedule_Display_Connect( retry_ms );
			return;
		}
		const QString err_msg = ( VM && AQ_Is_Apple_SoC_VM( VM ) )
			? tr(
				"Guest display is still not open on %1:%2 after several minutes.\n\n"
				"Inferno often needs a long time before VNC listens. Leave the VM running "
				"and use Connect, or check:\n"
				"%3" )
				.arg( Host ).arg( port )
				.arg( QDir::toNativeSeparators( AQ_Apple_SoC_QEMU_Log_Path( VM ) ) )
			: tr(
				"Guest display failed to open on %1:%2 after 6 seconds.\n\n"
				"Please verify that QEMU binary path and machine settings are valid in VM settings." )
				.arg( Host ).arg( port );
		Placeholder->setText( err_msg );
		Stack->setCurrentWidget( Placeholder );
		if( VM && AQ_Is_Apple_SoC_VM( VM ) )
		{
			// Soft warning only — do not imply the guest failed; keep QEMU running.
			AQGraphic_Warning( tr( "Guest display still waiting" ), err_msg );
			Display_Connect_Attempts = 0;
			Schedule_Display_Connect( 2000 );
			return;
		}
		AQGraphic_Error( "VM_Session_Widget::Try_Connect_Display",
		                 tr( "Guest Display Failure" ), err_msg, false );
		return;
	}

	Display_Connect_In_Progress = true;

	if( want_spice )
	{
		Stack->setCurrentWidget( Spice );
		Spice->Connect_To( Host, Spice_Port );
		return;
	}

#ifdef VNC_DISPLAY
	if( Vnc && Vnc_Port > 0 )
	{
		Backend = "vnc";
		Placeholder->setText( tr( "Connecting VNC display on %1:%2…" )
			.arg( Host ).arg( Vnc_Port ) );
		Stack->setCurrentWidget( Vnc );
		Vnc->Set_VNC_URL( Host, Vnc_Port );
		// Scale to fit; deep-copied frames + nearest-neighbor paint handle text mode.
		Vnc->Set_Scaling( true );
		Vnc->initView();
		connect( Vnc, SIGNAL(Connected()), this, SLOT(On_Display_Connected()), Qt::UniqueConnection );
		return;
	}
#endif

	Placeholder->setText( tr(
		"QEMU is running headless (SPICE %1 / VNC %2).\n"
		"Build with LibVNC or spice-client-glib to embed the guest display." )
		.arg( Spice_Port ).arg( Vnc_Port ) );
	Stack->setCurrentWidget( Placeholder );
}

void VM_Session_Widget::Detach()
{
	Drive_Poll_Timer->stop();
	Display_Connect_Timer->stop();
	Display_Connect_In_Progress = false;
	Display_Connect_Attempts = 0;
	Last_Drive_IO.clear();
	Connected_USB_Ids.clear();
	Toolbar_Show_Timer->stop();
	Toolbar_Hide_Timer->stop();

	if( Fullscreen_Active )
	{
		qApp->removeEventFilter( this );
		Set_Main_Window_Chrome_Visible( true );
		if( QWidget *w = window() )
		{
			if( w->isFullScreen() )
				w->showNormal();
		}
		Fullscreen_Active = false;
		Fullscreen_Toolbar_Visible = false;
		Set_Toolbar_In_Layout();
	}

	if( QWidget *w = window() )
		w->removeEventFilter( this );

	if( Spice )
		Spice->Disconnect();
#ifdef VNC_DISPLAY
	if( Vnc )
		Vnc->disconnectVNC();
#endif
	if( Serial_Win )
	{
		Serial_Win->Detach();
		Serial_Win->hide();
	}
	if( Button_Pad )
	{
		Button_Pad->hide();
		if( Act_Button_Pad )
			Act_Button_Pad->setChecked( false );
	}
	if( QMP_Client *q = Active_QMP() )
	{
		disconnect( q, SIGNAL(Connected()), this, SLOT(On_QMP_Connected()) );
		disconnect( q, SIGNAL(Block_Stats(QJsonArray)), this, SLOT(On_Block_Stats(QJsonArray)) );
	}

	VM = nullptr;
	QMP = nullptr;
	Fullscreen_Active = false;
	Fullscreen_Toolbar_Visible = false;
	Set_Toolbar_In_Layout();
	Update_Media_Actions();
}

void VM_Session_Widget::On_QMP_Connected()
{
	AQDebug( "VM_Session_Widget::On_QMP_Connected()", "QMP ready for media / power commands" );
	if( ! Drive_Poll_Timer->isActive() )
		Drive_Poll_Timer->start();
	Update_Media_Actions();
}

void VM_Session_Widget::On_Drive_Poll()
{
	if( QMP_Client *q = Active_QMP() )
	{
		if( q->Is_Connected() )
			q->Query_Blockstats();
	}
}

void VM_Session_Widget::On_Block_Stats( const QJsonArray &stats )
{
	auto io_total = []( const QJsonObject &st ) -> qint64 {
		return st.value( "rd_bytes" ).toVariant().toLongLong() +
		       st.value( "wr_bytes" ).toVariant().toLongLong() +
		       st.value( "rd_operations" ).toVariant().toLongLong() +
		       st.value( "wr_operations" ).toVariant().toLongLong();
	};

	// Match only our stable drive ids / known HMP names — never broad
	// substrings like "hd"/"scsi" that false-trigger activity.
	auto match_key = []( const QString &device ) -> QString {
		const QString d = device.toLower();
		if( d == "aqemu-fd0" || d == "floppy0" || d == "fda" )
			return "fd0";
		if( d == "aqemu-fd1" || d == "floppy1" || d == "fdb" )
			return "fd1";
		if( d == "aqemu-cdrom" || d == "ide1-cd0" || d == "cdrom" )
			return "cd";
		if( d == "aqemu-hda" || d == "ide0-hd0" || d == "hda" ||
		    d.startsWith( "aqhd" ) || d == "virtio0" )
			return "hd";
		return QString();
	};

	QHash<QString, bool> active;
	for( int i = 0; i < stats.count(); ++i )
	{
		const QJsonObject entry = stats.at( i ).toObject();
		QString device = entry.value( "device" ).toString();
		if( device.isEmpty() )
			device = entry.value( "id" ).toString();
		const QString key = match_key( device );
		if( key.isEmpty() )
			continue;

		const QJsonObject st = entry.value( "stats" ).toObject();
		const qint64 total = io_total( st );
		if( ! Last_Drive_IO.contains( key ) )
		{
			// First sample = baseline only (avoids false "always on" flash)
			Last_Drive_IO.insert( key, total );
			continue;
		}
		const qint64 prev = Last_Drive_IO.value( key );
		Last_Drive_IO.insert( key, total );
		if( total > prev )
			active.insert( key, true );
	}

	const bool fd0_loaded = VM && VM->Get_FD0().Get_Enabled() && ! VM->Get_FD0().Get_File_Name().isEmpty();
	const bool fd1_loaded = VM && VM->Get_FD1().Get_Enabled() && ! VM->Get_FD1().Get_File_Name().isEmpty();
	const bool cd_loaded = VM && VM->Get_CD_ROM().Get_Enabled() && ! VM->Get_CD_ROM().Get_File_Name().isEmpty();
	const bool hd_loaded = VM && (
		( VM->Get_HDA().Get_Enabled() && ! VM->Get_HDA().Get_File_Name().isEmpty() ) ||
		( VM->Get_HDB().Get_Enabled() && ! VM->Get_HDB().Get_File_Name().isEmpty() ) ||
		( VM->Get_HDC().Get_Enabled() && ! VM->Get_HDC().Get_File_Name().isEmpty() ) ||
		( VM->Get_HDD().Get_Enabled() && ! VM->Get_HDD().Get_File_Name().isEmpty() ) );

	Set_Drive_Light( Light_FD0, fd0_loaded, active.value( "fd0" ),
		fd0_loaded ? tr( "A: %1" ).arg( Media_Base_Name( VM->Get_FD0().Get_File_Name() ) )
		           : tr( "A: empty" ) );
	Set_Drive_Light( Light_FD1, fd1_loaded, active.value( "fd1" ),
		fd1_loaded ? tr( "B: %1" ).arg( Media_Base_Name( VM->Get_FD1().Get_File_Name() ) )
		           : tr( "B: empty" ) );
	Set_Drive_Light( Light_CD, cd_loaded, active.value( "cd" ),
		cd_loaded ? tr( "CD: %1" ).arg( Media_Base_Name( VM->Get_CD_ROM().Get_File_Name() ) )
		          : tr( "CD: empty" ) );
	Set_Drive_Light( Light_HD, hd_loaded, active.value( "hd" ),
		hd_loaded ? tr( "HD: activity" ) : tr( "HD: none" ) );
}

void VM_Session_Widget::On_Display_Connected()
{
	Display_Connect_In_Progress = false;
	Display_Connect_Attempts = 0;
	Placeholder->setText( tr( "Connected." ) );
}

void VM_Session_Widget::On_Display_Disconnected()
{
	Display_Connect_In_Progress = false;
	Placeholder->setText( tr(
		"Guest display closed.\n\n"
		"If Windows shut down, QEMU should stop shortly and return you to the VM list.\n"
		"Otherwise use the exit button (far right) to go back to the VM list — "
		"the guest keeps running and you can Connect again." ) );
	Stack->setCurrentWidget( Placeholder );

	// Guest ACPI power-off often drops SPICE before QEMU's process exit is
	// delivered. If the VM is already marked off, leave session immediately.
	if( VM && ( VM->Get_State() == VM::VMS_Power_Off ||
	            VM->Get_State() == VM::VMS_Saved ||
	            VM->Get_State() == VM::VMS_In_Error ) )
	{
		emit Exit_Session_View();
	}
}

void VM_Session_Widget::On_Display_Error( const QString &msg )
{
	AQWarning( "VM_Session_Widget", msg );

	// Retry SPICE — QEMU often accepts TCP before the SPICE handshake is ready.
	if( Backend == "spice" && Spice_Port > 0 && Spice && Spice->Spice_Available()
	    && Display_Connect_Attempts < 12 )
	{
		++Display_Connect_Attempts;
		Placeholder->setText( tr( "Retrying guest display… (%1)" )
			.arg( Display_Connect_Attempts ) );
		Stack->setCurrentWidget( Placeholder );
		// Defer disconnect one tick so we never tear down spice-glib while it
		// is still unwinding a channel-event (process crash on Windows).
		QTimer::singleShot( 0, this, [this]() {
			if( Spice )
				Spice->Disconnect();
			Schedule_Display_Connect( 700 );
		} );
		return;
	}

#ifdef VNC_DISPLAY
	// SPICE link errors are common on the first seconds of boot — fall back to
	// the VNC listener QEMU already opened for embedded sessions.
	if( Backend == "spice" && Vnc && Vnc_Port > 0 &&
	    Tcp_Port_Is_Open( Host, Vnc_Port ) )
	{
		AQWarning( "VM_Session_Widget",
		           QString( "SPICE failed (%1) — falling back to VNC :%2" )
		               .arg( msg ).arg( Vnc_Port ) );
		if( Spice )
			Spice->Disconnect();
		Backend = "vnc";
		Display_Connect_Attempts = 0;
		Placeholder->setText( tr( "Falling back to VNC display…" ) );
		Stack->setCurrentWidget( Placeholder );
		Schedule_Display_Connect( 300 );
		return;
	}
#endif

	Display_Connect_In_Progress = false;
	Placeholder->setText( tr( "Display error: %1\nWaiting / retry from Start if the guest is still booting." )
		.arg( msg ) );
	Stack->setCurrentWidget( Placeholder );
	AQGraphic_Error( "VM_Session_Widget::On_Display_Error",
	                 tr( "Guest Display Failure" ), msg, false );
}

QMP_Client *VM_Session_Widget::Active_QMP() const
{
	if( VM )
		return VM->Get_QMP();
	return QMP;
}

QString VM_Session_Widget::Hmp_Device_Name( const QString &block_id ) const
{
	if( block_id == "aqemu-fd0" ) return "floppy0";
	if( block_id == "aqemu-fd1" ) return "floppy1";
	if( block_id == "aqemu-cdrom" ) return "ide1-cd0";
	return block_id;
}

void VM_Session_Widget::Send_Monitor( const QString &cmd )
{
	if( ! VM )
		return;
	QString line = cmd;
	if( ! line.endsWith( '\n' ) )
		line += '\n';
	VM->Send_Emulator_Command( line );
}

QString VM_Session_Widget::USB_Instance_Key( const VM_USB &u, int index ) const
{
	if( ! u.Get_Bus().isEmpty() && ! u.Get_Addr().isEmpty() )
		return QStringLiteral( "bus:%1.%2" ).arg( u.Get_Bus(), u.Get_Addr() );
	if( ! u.Get_Serial_Number().isEmpty() )
		return QStringLiteral( "ser:%1:%2:%3" )
			.arg( u.Get_Vendor_ID().toLower(),
			      u.Get_Product_ID().toLower(),
			      u.Get_Serial_Number() );
	if( ! u.Get_DevPath().isEmpty() )
		return QStringLiteral( "path:%1" ).arg( u.Get_DevPath() );
	return QStringLiteral( "vp:%1:%2#%3" )
		.arg( u.Get_Vendor_ID().toLower(),
		      u.Get_Product_ID().toLower(),
		      QString::number( index ) );
}

QString VM_Session_Widget::USB_Qemu_Device_Id( const QString &instance_key ) const
{
	QString s = instance_key.toLower();
	s.replace( QRegularExpression( QStringLiteral( "[^a-z0-9_]+" ) ), QStringLiteral( "_" ) );
	while( s.contains( QStringLiteral( "__" ) ) )
		s.replace( QStringLiteral( "__" ), QStringLiteral( "_" ) );
	if( s.startsWith( QLatin1Char( '_' ) ) )
		s.remove( 0, 1 );
	if( s.isEmpty() )
		s = QStringLiteral( "dev" );
	return QStringLiteral( "aqusb_%1" ).arg( s.left( 48 ) );
}

QString VM_Session_Widget::USB_Device_Add_Command( const VM_USB &u, const QString &qemu_id ) const
{
	if( ! u.Get_Bus().isEmpty() && ! u.Get_Addr().isEmpty() )
	{
		bool ok_bus = false, ok_addr = false;
		const int bus = u.Get_Bus().toInt( &ok_bus );
		const int addr = u.Get_Addr().toInt( &ok_addr );
		if( ok_bus && ok_addr )
		{
			return QStringLiteral( "device_add usb-host,hostbus=%1,hostaddr=%2,id=%3" )
				.arg( bus ).arg( addr ).arg( qemu_id );
		}
	}
	return QStringLiteral( "device_add usb-host,vendorid=0x%1,productid=0x%2,id=%3" )
		.arg( u.Get_Vendor_ID().toLower(), u.Get_Product_ID().toLower(), qemu_id );
}

void VM_Session_Widget::Send_Hmp_Command( const QString &cmd )
{
	// Prefer QMP human-monitor-command; do not also write the legacy monitor (PR #10 / Qodo)
	if( QMP_Client *q = Active_QMP() )
	{
		if( q->Is_Connected() )
		{
			q->Human_Monitor( cmd );
			return;
		}
	}
	Send_Monitor( cmd );
}

void VM_Session_Widget::On_USB_Menu_About_To_Show()
{
	if( System_Info::Get_Cached_Host_USB().isEmpty() && ! USB_Enum_Busy )
		Start_USB_Host_Scan();
	else
		Rebuild_USB_Menu();
}

void VM_Session_Widget::Start_USB_Host_Scan()
{
	if( USB_Enum_Busy || ! Menu_USB )
		return;
	if( USB_Scan_Thread && USB_Scan_Thread->isRunning() )
		return;

	USB_Enum_Busy = true;
	Menu_USB->clear();
	QAction *loading = Menu_USB->addAction( tr( "Scanning USB devices…" ) );
	loading->setEnabled( false );

	// Scan into a heap list on a worker; publish to the cache only on the UI thread.
	auto *holder = new QList<VM_USB>();
	QThread *th = QThread::create( [holder]() {
		System_Info::Scan_Host_USB_Snapshot( *holder );
	} );
	th->setParent( this );
	USB_Scan_Thread = th;
	connect( th, &QThread::finished, this, [this, th, holder]() {
		if( USB_Scan_Thread == th )
			USB_Scan_Thread.clear();
		System_Info::Set_Cached_Host_USB( *holder );
		delete holder;
		USB_Enum_Busy = false;
		th->deleteLater();
		Rebuild_USB_Menu();
	} );
	th->start();
}

void VM_Session_Widget::Rebuild_USB_Menu()
{
	if( ! Menu_USB )
		return;
	if( USB_Enum_Busy )
		return;

	Menu_USB->clear();

	QAction *hint = Menu_USB->addAction( tr( "Host USB devices (toggle to connect)" ) );
	hint->setEnabled( false );
	Menu_USB->addSeparator();

	QAction *refresh = Menu_USB->addAction( QIcon( ":/update.png" ), tr( "Refresh list" ) );
	connect( refresh, &QAction::triggered, this, [this]() { Start_USB_Host_Scan(); } );
	Menu_USB->addSeparator();

	const QList<VM_USB> host = System_Info::Get_Cached_Host_USB();
	if( host.isEmpty() )
	{
		QAction *empty = Menu_USB->addAction( tr( "(No cached USB list — click Refresh)" ) );
		empty->setEnabled( false );
		return;
	}

	for( int i = 0; i < host.count(); ++i )
	{
		const VM_USB &u = host[i];
		const QString vidpid = u.Get_ID_Line().toLower();
		if( vidpid.isEmpty() || ! vidpid.contains( QLatin1Char( ':' ) ) )
			continue;
		const QString key = USB_Instance_Key( u, i );
		const QString qemu_id = USB_Qemu_Device_Id( key );

		QString label = u.Get_Product_Name().trimmed();
		if( label.isEmpty() )
			label = vidpid;
		else
			label = QStringLiteral( "%1  (%2)" ).arg( label, vidpid );
		if( ! u.Get_Bus().isEmpty() && ! u.Get_Addr().isEmpty() )
			label += QStringLiteral( "  [%1.%2]" ).arg( u.Get_Bus(), u.Get_Addr() );

		QAction *act = Menu_USB->addAction( label );
		act->setCheckable( true );
		QVariantMap data;
		data.insert( QStringLiteral( "key" ), key );
		data.insert( QStringLiteral( "qemu_id" ), qemu_id );
		data.insert( QStringLiteral( "vid" ), u.Get_Vendor_ID().toLower() );
		data.insert( QStringLiteral( "pid" ), u.Get_Product_ID().toLower() );
		data.insert( QStringLiteral( "bus" ), u.Get_Bus() );
		data.insert( QStringLiteral( "addr" ), u.Get_Addr() );
		act->setData( data );
		act->blockSignals( true );
		act->setChecked( Connected_USB_Ids.contains( key ) );
		act->blockSignals( false );
		connect( act, SIGNAL(toggled(bool)), this, SLOT(On_USB_Device_Toggled(bool)) );
	}

	if( ! Connected_USB_Ids.isEmpty() )
	{
		Menu_USB->addSeparator();
		QAction *disc_all = Menu_USB->addAction( tr( "Disconnect all from guest" ) );
		connect( disc_all, &QAction::triggered, this, [this]() {
			const QStringList ids = Connected_USB_Ids.values();
			for( int i = 0; i < ids.count(); ++i )
			{
				if( ! ids[i].isEmpty() )
					Send_Hmp_Command( QStringLiteral( "device_del %1" ).arg( ids[i] ) );
			}
			Connected_USB_Ids.clear();
		} );
	}
}

void VM_Session_Widget::On_USB_Device_Toggled( bool checked )
{
	QAction *act = qobject_cast<QAction *>( sender() );
	if( ! act )
		return;
	const QVariantMap data = act->data().toMap();
	const QString key = data.value( QStringLiteral( "key" ) ).toString();
	QString qemu_id = data.value( QStringLiteral( "qemu_id" ) ).toString();
	if( key.isEmpty() )
		return;
	if( qemu_id.isEmpty() )
		qemu_id = USB_Qemu_Device_Id( key );

	if( checked )
	{
		VM_USB tmp;
		tmp.Set_Vendor_ID( data.value( QStringLiteral( "vid" ) ).toString() );
		tmp.Set_Product_ID( data.value( QStringLiteral( "pid" ) ).toString() );
		tmp.Set_Bus( data.value( QStringLiteral( "bus" ) ).toString() );
		tmp.Set_Addr( data.value( QStringLiteral( "addr" ) ).toString() );
		Send_Hmp_Command( USB_Device_Add_Command( tmp, qemu_id ) );
		Connected_USB_Ids.insert( key, qemu_id );
	}
	else
	{
		Send_Hmp_Command( QStringLiteral( "device_del %1" ).arg( qemu_id ) );
		Connected_USB_Ids.remove( key );
	}
}

bool VM_Session_Widget::Change_Medium_Id( const QString &block_id, const QString &path )
{
	const QString unix_path = QDir::fromNativeSeparators( path );

	// QMP (device= backend name). Fire-and-forget — also always send HMP,
	// which is what actually works for if=floppy drives on current QEMU.
	if( QMP_Client *q = Active_QMP() )
	{
		if( q->Is_Connected() )
			q->Change_Medium( block_id, unix_path );
	}

	Send_Monitor( QString( "change %1 \"%2\"" ).arg( block_id, unix_path ) );
	const QString hmp = Hmp_Device_Name( block_id );
	if( hmp != block_id )
		Send_Monitor( QString( "change %1 \"%2\"" ).arg( hmp, unix_path ) );
	return true;
}

void VM_Session_Widget::Enable_Boot_Device( VM::Boot_Device type )
{
	if( ! VM ) return;
	QList<VM::Boot_Order> list = VM->Get_Boot_Order_List();
	for( int i = 0; i < list.count(); ++i )
	{
		if( list[i].Type == type )
			list[i].Enabled = true;
	}
	VM->Set_Boot_Order_List( list );
}

void VM_Session_Widget::Apply_Runtime_Boot_Order()
{
	if( ! VM )
		return;

	const QString letters = VM->Get_X86_Boot_Order_Letters();
	if( letters.isEmpty() )
		return;

	// Update SeaBIOS boot list without relaunching QEMU (fixes insert-then-reset).
	const QString cmd = QString( "boot_set %1" ).arg( letters );
	if( QMP_Client *q = Active_QMP() )
	{
		if( q->Is_Connected() )
			q->Human_Monitor( cmd );
	}
	Send_Monitor( cmd );
	AQDebug( "VM_Session_Widget::Apply_Runtime_Boot_Order()", cmd );
}

QString VM_Session_Widget::Media_Base_Name( const QString &path )
{
	if( path.isEmpty() )
		return QString();
	return QFileInfo( path ).fileName();
}

void VM_Session_Widget::Set_Drive_Light( QLabel *light, bool loaded, bool active, const QString &tip )
{
	if( ! light )
		return;
	light->setToolTip( tip );
	if( active && loaded )
		light->setStyleSheet( AQ_Drive_Light_Style( this, QStringLiteral( "#5dff6a" ), QStringLiteral( "#2a8a35" ) ) );
	else if( loaded )
		light->setStyleSheet( AQ_Drive_Light_Style( this, QStringLiteral( "#1a6b2a" ), QStringLiteral( "#0d3d18" ) ) );
	else
		light->setStyleSheet( AQ_Drive_Light_Style( this, QStringLiteral( "#3a3a3a" ), QStringLiteral( "#222" ) ) );
}

void VM_Session_Widget::Update_Media_Actions()
{
	const QString cd = VM ? VM->Get_CD_ROM().Get_File_Name() : QString();
	const QString fd0 = VM ? VM->Get_FD0().Get_File_Name() : QString();
	const QString fd1 = VM ? VM->Get_FD1().Get_File_Name() : QString();
	const bool cd_on = VM && VM->Get_CD_ROM().Get_Enabled() && ! cd.isEmpty();
	const bool fd0_on = VM && VM->Get_FD0().Get_Enabled() && ! fd0.isEmpty();
	const bool fd1_on = VM && VM->Get_FD1().Get_Enabled() && ! fd1.isEmpty();
	const bool hd_on = VM && (
		( VM->Get_HDA().Get_Enabled() && ! VM->Get_HDA().Get_File_Name().isEmpty() ) ||
		( VM->Get_HDB().Get_Enabled() && ! VM->Get_HDB().Get_File_Name().isEmpty() ) ||
		( VM->Get_HDC().Get_Enabled() && ! VM->Get_HDC().Get_File_Name().isEmpty() ) ||
		( VM->Get_HDD().Get_Enabled() && ! VM->Get_HDD().Get_File_Name().isEmpty() ) );

	if( Act_Insert_CD )
		Act_Insert_CD->setToolTip( cd_on
			? tr( "Change CD/DVD (loaded: %1)" ).arg( Media_Base_Name( cd ) )
			: tr( "Insert CD/DVD image…" ) );
	if( Act_Eject_CD )
	{
		Act_Eject_CD->setToolTip( cd_on
			? tr( "Eject CD: (%1)" ).arg( Media_Base_Name( cd ) )
			: tr( "Eject CD/DVD (empty)" ) );
		Act_Eject_CD->setEnabled( cd_on );
	}

	if( Act_Insert_FD0 )
		Act_Insert_FD0->setToolTip( fd0_on
			? tr( "Change floppy A (loaded: %1)" ).arg( Media_Base_Name( fd0 ) )
			: tr( "Insert floppy A image…" ) );
	if( Act_Eject_FD0 )
	{
		Act_Eject_FD0->setToolTip( fd0_on
			? tr( "Eject A: (%1)" ).arg( Media_Base_Name( fd0 ) )
			: tr( "Eject floppy A (empty)" ) );
		Act_Eject_FD0->setEnabled( fd0_on );
	}

	if( Act_Insert_FD1 )
		Act_Insert_FD1->setToolTip( fd1_on
			? tr( "Change floppy B (loaded: %1)" ).arg( Media_Base_Name( fd1 ) )
			: tr( "Insert floppy B image…" ) );
	if( Act_Eject_FD1 )
	{
		Act_Eject_FD1->setToolTip( fd1_on
			? tr( "Eject B: (%1)" ).arg( Media_Base_Name( fd1 ) )
			: tr( "Eject floppy B (empty)" ) );
		Act_Eject_FD1->setEnabled( fd1_on );
	}

	if( Act_Restore_IPSW )
		Act_Restore_IPSW->setVisible( VM && AQ_Is_Apple_SoC_VM( VM ) );

	Update_Apple_Controls_Visibility();

	Set_Drive_Light( Light_FD0, fd0_on, false,
		fd0_on ? tr( "A: %1" ).arg( Media_Base_Name( fd0 ) ) : tr( "A: empty" ) );
	Set_Drive_Light( Light_FD1, fd1_on, false,
		fd1_on ? tr( "B: %1" ).arg( Media_Base_Name( fd1 ) ) : tr( "B: empty" ) );
	Set_Drive_Light( Light_CD, cd_on, false,
		cd_on ? tr( "CD: %1" ).arg( Media_Base_Name( cd ) ) : tr( "CD: empty" ) );
	Set_Drive_Light( Light_HD, hd_on, false,
		hd_on ? tr( "HD: ready" ) : tr( "HD: none" ) );
}

bool VM_Session_Widget::Eject_Medium_Id( const QString &block_id )
{
	if( QMP_Client *q = Active_QMP() )
	{
		if( q->Is_Connected() )
			q->Eject_Medium( block_id, true );
	}

	Send_Monitor( QString( "eject -f %1" ).arg( block_id ) );
	const QString hmp = Hmp_Device_Name( block_id );
	if( hmp != block_id )
		Send_Monitor( QString( "eject -f %1" ).arg( hmp ) );
	return true;
}

void VM_Session_Widget::Send_CAD_To_Guest()
{
	if( Spice && Backend == "spice" && Spice->Is_Connected() )
	{
		Spice->Send_CAD();
		return;
	}
	Send_Monitor( "sendkey ctrl-alt-delete" );
}

void VM_Session_Widget::Persist_Media_Config()
{
	if( ! VM )
		return;
	if( ! VM->Save_VM() )
	{
		AQWarning( "VM_Session_Widget::Persist_Media_Config()",
			   "Failed to save media paths to " + VM->Get_VM_XML_File_Path() );
	}
}

void VM_Session_Widget::On_Change_CD()
{
	if( ! VM ) return;
	QString file = QFileDialog::getOpenFileName( this, tr( "Select CD/DVD image" ),
		QString(), Disk_Image_File_Filter( true, false ) );
	if( file.isEmpty() ) return;
	file = QDir::toNativeSeparators( file );
	if( ! Change_Medium_Id( "aqemu-cdrom", file ) )
	{
		QMessageBox::warning( this, tr( "Media" ), tr( "Could not change CD/DVD." ) );
		return;
	}
	VM->Set_CD_ROM( VM_Storage_Device( true, file ) );
	Enable_Boot_Device( VM::Boot_From_CDROM );
	Apply_Runtime_Boot_Order();
	Persist_Media_Config();
	Update_Media_Actions();
}

void VM_Session_Widget::On_Eject_CD()
{
	if( ! VM ) return;
	Eject_Medium_Id( "aqemu-cdrom" );
	VM->Set_CD_ROM( VM_Storage_Device( false, QString() ) );
	Apply_Runtime_Boot_Order();
	Persist_Media_Config();
	Update_Media_Actions();
}

void VM_Session_Widget::On_Change_FD0()
{
	if( ! VM ) return;
	QString file = QFileDialog::getOpenFileName( this, tr( "Select floppy A image" ),
		QString(), Disk_Image_File_Filter( false, true ) );
	if( file.isEmpty() ) return;
	file = QDir::toNativeSeparators( file );
	if( ! Change_Medium_Id( "aqemu-fd0", file ) )
	{
		QMessageBox::warning( this, tr( "Media" ), tr( "Could not change floppy A." ) );
		return;
	}
	VM->Set_FD0( VM_Storage_Device( true, file ) );
	Enable_Boot_Device( VM::Boot_From_FDA );
	Last_Drive_IO.remove( "fd0" );
	Apply_Runtime_Boot_Order();
	Persist_Media_Config();
	Update_Media_Actions();
	AQDebug( "VM_Session_Widget::On_Change_FD0()",
	         "Inserted A: " + file + " — boot_set " + VM->Get_X86_Boot_Order_Letters() );
}

void VM_Session_Widget::On_Eject_FD0()
{
	if( ! VM ) return;
	Eject_Medium_Id( "aqemu-fd0" );
	VM->Set_FD0( VM_Storage_Device( false, QString() ) );
	Apply_Runtime_Boot_Order();
	Persist_Media_Config();
	Update_Media_Actions();
}

void VM_Session_Widget::On_Change_FD1()
{
	if( ! VM ) return;
	QString file = QFileDialog::getOpenFileName( this, tr( "Select floppy B image" ),
		QString(), Disk_Image_File_Filter( false, true ) );
	if( file.isEmpty() ) return;
	file = QDir::toNativeSeparators( file );
	if( ! Change_Medium_Id( "aqemu-fd1", file ) )
	{
		QMessageBox::warning( this, tr( "Media" ), tr( "Could not change floppy B." ) );
		return;
	}
	VM->Set_FD1( VM_Storage_Device( true, file ) );
	Enable_Boot_Device( VM::Boot_From_FDB );
	Apply_Runtime_Boot_Order();
	Persist_Media_Config();
	Update_Media_Actions();
}

void VM_Session_Widget::On_Eject_FD1()
{
	if( ! VM ) return;
	Eject_Medium_Id( "aqemu-fd1" );
	VM->Set_FD1( VM_Storage_Device( false, QString() ) );
	Apply_Runtime_Boot_Order();
	Persist_Media_Config();
	Update_Media_Actions();
}

void VM_Session_Widget::On_Serial_Console()
{
	if( ! Serial_Win )
		Serial_Win = new Serial_Console_Window( this );
	Serial_Win->Attach( VM );
	Serial_Win->show();
	Serial_Win->raise();
	Serial_Win->activateWindow();
}

void VM_Session_Widget::On_Restore_IPSW()
{
	emit Request_Restore_IPSW();
}

void VM_Session_Widget::On_CAD()
{
	Send_CAD_To_Guest();
}

void VM_Session_Widget::On_Grab_Mouse()
{
#ifdef VNC_DISPLAY
	if( Backend == "vnc" && Vnc )
	{
		Vnc->captureAllMouseEvents();
		Update_Grab_Mouse_Action();
		return;
	}
#endif
	if( Spice && Backend == "spice" )
	{
		Spice->setFocus( Qt::MouseFocusReason );
		AQGraphic_Warning( tr( "Mouse grab" ),
			tr( "SPICE: click inside the guest to capture (relative mouse), Esc to release.\n"
			    "For seamless tablet mode, change Mouse Type to a relative device and restart." ) );
	}
}

void VM_Session_Widget::Update_Grab_Mouse_Action()
{
	if( ! Act_Grab_Mouse )
		return;
	bool grabbed = false;
#ifdef VNC_DISPLAY
	if( Backend == "vnc" && Vnc )
		grabbed = Vnc->isMouseGrabbed();
#endif
	Act_Grab_Mouse->setChecked( grabbed );
	Act_Grab_Mouse->setToolTip( grabbed
		? tr( "Mouse captured — press Esc or Ctrl+Alt to release" )
		: tr( "Grab mouse into guest — or click the guest display; Esc / Ctrl+Alt releases" ) );
}

void VM_Session_Widget::On_Shift_F10()
{
	if( Spice && Backend == "spice" && Spice->Is_Connected() )
	{
		Spice->Send_Shift_F10();
		return;
	}
	// HMP fallback (VNC / no SPICE inputs channel)
	Send_Monitor( "sendkey shift-f10" );
}

void VM_Session_Widget::Send_Apple_Key( const QString &key_name, int hold_ms )
{
	if( ! VM || key_name.isEmpty() )
		return;
	// ChefKiss Inferno maps F-keys to device buttons via QEMU keyboard.
	if( hold_ms <= 0 )
	{
		Send_Monitor( QStringLiteral( "sendkey %1" ).arg( key_name ) );
		return;
	}
	// QEMU HMP: sendkey keys [hold_ms] — one down/up with duration, not two taps.
	Send_Monitor( QStringLiteral( "sendkey %1 %2" ).arg( key_name ).arg( hold_ms ) );
}

void VM_Session_Widget::Send_Apple_Key_Double( const QString &key_name )
{
	Send_Apple_Key( key_name );
	QTimer::singleShot( 180, this, [this, key_name]() {
		Send_Apple_Key( key_name );
	} );
}

void VM_Session_Widget::Send_Apple_SOS_Combo()
{
	// ChefKiss: hold volume up, then side (not both at once).
	Send_Apple_Key( QStringLiteral( "f4" ), 2000 );
	QTimer::singleShot( 500, this, [this]() {
		Send_Apple_Key( QStringLiteral( "f5" ), 1500 );
	} );
}

void VM_Session_Widget::Update_Apple_Controls_Visibility()
{
	const bool apple = VM && AQ_Is_Apple_SoC_VM( VM );
	for( QAction *a : { Act_Apple_Vol_Down, Act_Apple_Vol_Up, Act_Apple_Home, Act_Apple_Power,
	                    Act_Apple_SOS, Act_Apple_More, Act_Button_Pad, Act_Guest_Internet } )
	{
		if( a )
			a->setVisible( apple );
	}
	for( QAction *a : Apple_Sep_Actions )
	{
		if( a )
			a->setVisible( apple );
	}
	// Hide PC-centric keys / floppies for iPhone sessions
	if( Act_CAD )
		Act_CAD->setVisible( ! apple );
	if( Act_Shift_F10 )
		Act_Shift_F10->setVisible( ! apple );
	if( Act_Insert_FD0 )
		Act_Insert_FD0->setVisible( ! apple );
	if( Act_Eject_FD0 )
		Act_Eject_FD0->setVisible( ! apple );
	if( Act_Insert_FD1 )
		Act_Insert_FD1->setVisible( ! apple );
	if( Act_Eject_FD1 )
		Act_Eject_FD1->setVisible( ! apple );
	if( Light_FD0 )
		Light_FD0->setVisible( ! apple );
	if( Light_FD1 )
		Light_FD1->setVisible( ! apple );

	if( ! apple && Button_Pad )
	{
		Button_Pad->hide();
		if( Act_Button_Pad )
			Act_Button_Pad->setChecked( false );
	}
}

void VM_Session_Widget::Ensure_Button_Pad()
{
	if( Button_Pad )
		return;
	Button_Pad = new Apple_SoC_Button_Pad( window() ? window() : this );
	connect( Button_Pad, &Apple_SoC_Button_Pad::Home_Clicked,
	         this, &VM_Session_Widget::On_Apple_Home );
	connect( Button_Pad, &Apple_SoC_Button_Pad::Home_Double_Clicked,
	         this, &VM_Session_Widget::On_Apple_Home_Double );
	connect( Button_Pad, &Apple_SoC_Button_Pad::Power_Clicked,
	         this, &VM_Session_Widget::On_Apple_Power );
	connect( Button_Pad, &Apple_SoC_Button_Pad::Power_Hold,
	         this, &VM_Session_Widget::On_Apple_Power_Hold );
	connect( Button_Pad, &Apple_SoC_Button_Pad::Vol_Down,
	         this, &VM_Session_Widget::On_Apple_Vol_Down );
	connect( Button_Pad, &Apple_SoC_Button_Pad::Vol_Up,
	         this, &VM_Session_Widget::On_Apple_Vol_Up );
	connect( Button_Pad, &Apple_SoC_Button_Pad::SOS_Triggered,
	         this, &VM_Session_Widget::On_Apple_SOS );
}

void VM_Session_Widget::Position_Button_Pad()
{
	if( ! Button_Pad || ! Button_Pad->isVisible() )
		return;
	QWidget *host = window() ? window() : this;
	const QPoint g = host->mapToGlobal( QPoint(
		qMax( 8, ( host->width() - Button_Pad->width() ) / 2 ),
		qMax( 8, host->height() - Button_Pad->height() - 48 ) ) );
	Button_Pad->move( g );
}

void VM_Session_Widget::On_Apple_Home()
{
	Send_Apple_Key( QStringLiteral( "f6" ) );
}

void VM_Session_Widget::On_Apple_Home_Double()
{
	Send_Apple_Key_Double( QStringLiteral( "f6" ) );
}

void VM_Session_Widget::On_Apple_Power()
{
	Send_Apple_Key( QStringLiteral( "f5" ) );
}

void VM_Session_Widget::On_Apple_Power_Hold()
{
	Send_Apple_Key( QStringLiteral( "f5" ), 2000 );
}

void VM_Session_Widget::On_Apple_Vol_Down()
{
	Send_Apple_Key( QStringLiteral( "f3" ) );
}

void VM_Session_Widget::On_Apple_Vol_Up()
{
	Send_Apple_Key( QStringLiteral( "f4" ) );
}

void VM_Session_Widget::On_Apple_SOS()
{
	Send_Apple_SOS_Combo();
}

void VM_Session_Widget::On_Apple_Ringer()
{
	Send_Apple_Key( QStringLiteral( "f2" ) );
}

void VM_Session_Widget::On_Toggle_Button_Pad()
{
	if( ! VM || ! AQ_Is_Apple_SoC_VM( VM ) )
		return;
	Ensure_Button_Pad();
	const bool show = Act_Button_Pad && Act_Button_Pad->isChecked();
	if( show )
	{
		Position_Button_Pad();
		Button_Pad->show();
		Button_Pad->raise();
	}
	else if( Button_Pad )
		Button_Pad->hide();
}

void VM_Session_Widget::On_Guest_Internet()
{
	if( ! VM || ! AQ_Is_Apple_SoC_VM( VM ) )
		return;
	AQ_Show_Apple_SoC_Device_Tools_Window( VM, window() ? window() : this, 0 );
}

void VM_Session_Widget::On_Apple_More_Buttons()
{
	QMenu menu( this );
	menu.addAction( tr( "App Switcher (double Home)" ), this, SLOT(On_Apple_Home_Double()) );
	menu.addAction( tr( "Power hold (~2s)" ), this, SLOT(On_Apple_Power_Hold()) );
	menu.addAction( tr( "Toggle ringer (F2)" ), this, SLOT(On_Apple_Ringer()) );
	menu.addAction( tr( "Force shutdown (F1)" ), this, [this]() {
		Send_Apple_Key( QStringLiteral( "f1" ) );
	} );
	menu.addSeparator();
	menu.addAction( tr( "Show floating button pad" ), this, [this]() {
		if( Act_Button_Pad )
		{
			Act_Button_Pad->setChecked( true );
			On_Toggle_Button_Pad();
		}
	} );
	menu.addSeparator();
	menu.addAction( tr( "ChefKiss button guide…" ), this, []() {
		QDesktopServices::openUrl( QUrl( QStringLiteral(
			"https://chefkiss.dev/guides/inferno/device-buttons/" ) ) );
	} );
	if( Act_Apple_More )
	{
		QWidget *w = Toolbar ? Toolbar->widgetForAction( Act_Apple_More ) : nullptr;
		menu.exec( w ? w->mapToGlobal( QPoint( 0, w->height() ) ) : QCursor::pos() );
	}
	else
		menu.exec( QCursor::pos() );
}

void VM_Session_Widget::On_Fullscreen()
{
	QWidget *w = window();
	if( ! w ) return;

	if( w->isFullScreen() )
		w->showNormal();
	else
		w->showFullScreen();

	Update_Fullscreen_State();
}

void VM_Session_Widget::On_Pause()
{
	if( VM )
		VM->Pause();
	else
		emit Request_Pause();
	Update_Pause_Action();
}

void VM_Session_Widget::On_Migrate()
{
	QMP_Client *q = Active_QMP();
	if( ! q || ! q->Is_Connected() )
	{
		QMessageBox::warning( this, tr( "Migrate" ),
		                      tr( "QMP is not connected." ) );
		return;
	}

	Migrate_URI_Dialog pick( this );
	if( pick.exec() != QDialog::Accepted )
		return;
	const QString uri = pick.URI().trimmed();
	if( uri.isEmpty() )
		return;

	Migrate_Progress_Dialog dlg( q, uri, this, pick.Copy_Storage() );
	dlg.exec();
}

void VM_Session_Widget::On_Save()
{
	if( ! VM )
	{
		emit Request_Save();
		return;
	}

	if( VM->Use_Snapshot_Mode() )
	{
		QMessageBox::warning( this, tr( "Warning" ),
			tr( "QEMU is running in snapshot mode. The VM cannot be saved." ) );
		return;
	}

	if( QMessageBox::question( this, tr( "Save VM state?" ),
		tr( "Save the current state of \"%1\" and stop the guest?" )
			.arg( VM->Get_Machine_Name() ),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes ) != QMessageBox::Yes )
	{
		return;
	}

	VM->Save_VM_State();
}

void VM_Session_Widget::On_Power_Off()
{
	// Always go through Main_Window / service so Stop runs once (no double-quit).
	emit Request_Stop();
}

void VM_Session_Widget::On_Shutdown()
{
	emit Request_Shutdown();
}

void VM_Session_Widget::On_Reset()
{
	emit Request_Reset();
}

void VM_Session_Widget::On_Exit_View()
{
	if( QWidget *w = window() )
	{
		if( w->isFullScreen() )
			w->showNormal();
	}
	Update_Fullscreen_State();
	emit Exit_Session_View();
}
