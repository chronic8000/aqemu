/****************************************************************************
** Full-window VM session widget
****************************************************************************/
#include "VM_Session_Widget.h"

#include <QVBoxLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
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

#include "VM.h"
#include "QMP_Client.h"
#include "Embedded_Display/Spice_View.h"
#include "Utils.h"

#ifdef VNC_DISPLAY
#include "Embedded_Display/Machine_View.h"
#endif

static const int kHotZoneHeight = 48;
static const int kHotZoneDwellMs = 1000; // hold top-center ~1s to reveal (Chrome-like)
static const int kToolbarHideMs = 700;   // hide soon after leaving the toolbar
static const int kDrivePollMs = 200;

static const char *kToolbarStyle =
	"QToolBar { background: #2d2d30; border: none; border-bottom: 1px solid #1a1a1a; spacing: 4px; padding: 4px; }"
	"QToolButton { color: #eee; padding: 4px 6px; border-radius: 3px; }"
	"QToolButton:hover { background: rgba(255,255,255,30); }"
	"QToolButton:pressed { background: rgba(255,255,255,50); }";

static const char *kLightOff =
	"QLabel { min-width: 12px; max-width: 12px; min-height: 12px; max-height: 12px;"
	" border-radius: 6px; background: #3a3a3a; border: 1px solid #222; margin: 0 3px; }";
static const char *kLightLoaded =
	"QLabel { min-width: 12px; max-width: 12px; min-height: 12px; max-height: 12px;"
	" border-radius: 6px; background: #1a6b2a; border: 1px solid #0d3d18; margin: 0 3px; }";
static const char *kLightActive =
	"QLabel { min-width: 12px; max-width: 12px; min-height: 12px; max-height: 12px;"
	" border-radius: 6px; background: #5dff6a; border: 1px solid #2a8a35; margin: 0 3px; }";

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
	Toolbar->setIconSize( QSize( 22, 22 ) );
	Toolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
	Toolbar->setStyleSheet( kToolbarStyle );

	// Runtime power controls (replaces left Tool_Bar_VM_Control in session mode)
	Act_Pause = Add_Toolbar_Action( QIcon( ":/pause.png" ), tr( "Pause / Resume" ), SLOT(On_Pause()) );
	Add_Toolbar_Action( QIcon( ":/save-state.png" ), tr( "Save VM state" ), SLOT(On_Save()) );
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

	// Guest / display
	Add_Toolbar_Action( QIcon( ":/key.png" ), tr( "Send Ctrl+Alt+Del" ), SLOT(On_CAD()) );
	Act_Fullscreen = Add_Toolbar_Action( QIcon( ":/fullscreen.png" ),
	                                     tr( "Fullscreen — hold mouse at top center to show toolbar" ),
	                                     SLOT(On_Fullscreen()) );
	Toolbar->addSeparator();
	Add_Toolbar_Action( QIcon( ":/exit.png" ), tr( "Exit session view (guest keeps running)" ), SLOT(On_Exit_View()) );

	// Drive activity lights (PCem / 86Box style) — right side
	QWidget *spacer = new QWidget( Toolbar );
	spacer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
	Toolbar->addWidget( spacer );

	QLabel *lights_label = new QLabel( tr( "Drives" ), Toolbar );
	lights_label->setStyleSheet( "QLabel { color: #888; font-size: 10px; margin-right: 2px; }" );
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
	l->setStyleSheet( kLightOff );
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

	const int th = qMax( Toolbar->sizeHint().height(), 36 );
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
	if( local.y() < 0 || local.y() > kHotZoneHeight )
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
		p = QSettings().value( "Embedded_Display_Backend", "spice" ).toString().toLower();

	// Prefer SPICE whenever the spice-client-glib viewer was compiled in.
	if( Spice && Spice->Spice_Available() && ( p.isEmpty() || p == "spice" || p == "auto" ) )
		return "spice";
	if( p == "spice" && Spice && Spice->Spice_Available() )
		return "spice";
#ifdef VNC_DISPLAY
	return "vnc";
#else
	if( p == "spice" )
		return "spice";
	return "none";
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
	Schedule_Display_Connect( 250 );

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
		++Display_Connect_Attempts;
		if( Display_Connect_Attempts < 40 )
		{
			Placeholder->setText( tr( "Waiting for guest display… (%1)" )
				.arg( Display_Connect_Attempts ) );
			Stack->setCurrentWidget( Placeholder );
			Schedule_Display_Connect( 400 );
			return;
		}
		Placeholder->setText( tr(
			"Guest display did not open on %1:%2.\n"
			"QEMU may still be starting, or SPICE/VNC failed to bind." )
			.arg( Host ).arg( port ) );
		Stack->setCurrentWidget( Placeholder );
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

void VM_Session_Widget::On_Display_Error( const QString &msg )
{
	AQWarning( "VM_Session_Widget", msg );

	// Retry SPICE — QEMU often accepts TCP before the SPICE handshake is ready.
	if( Backend == "spice" && Spice_Port > 0 && Spice && Spice->Spice_Available()
	    && Display_Connect_Attempts < 40 )
	{
		++Display_Connect_Attempts;
		if( Spice )
			Spice->Disconnect();
		Placeholder->setText( tr( "Retrying guest display… (%1)" )
			.arg( Display_Connect_Attempts ) );
		Stack->setCurrentWidget( Placeholder );
		Schedule_Display_Connect( 500 );
		return;
	}

	Display_Connect_In_Progress = false;
	Placeholder->setText( tr( "Display error: %1\nWaiting / retry from Start if the guest is still booting." )
		.arg( msg ) );
	Stack->setCurrentWidget( Placeholder );
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
		light->setStyleSheet( kLightActive );
	else if( loaded )
		light->setStyleSheet( kLightLoaded );
	else
		light->setStyleSheet( kLightOff );
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
		QString(), tr( "Images (*.iso *.img *.cdr);;All (*)" ) );
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
		QString(), tr( "Floppy (*.img *.ima *.vfd *.dsk);;All (*)" ) );
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
		QString(), tr( "Floppy (*.img *.ima *.vfd *.dsk);;All (*)" ) );
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

void VM_Session_Widget::On_CAD()
{
	Send_CAD_To_Guest();
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
