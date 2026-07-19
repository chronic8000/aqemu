/****************************************************************************
** Full-window VM session widget
****************************************************************************/
#include "VM_Session_Widget.h"

#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

#include "VM.h"
#include "QMP_Client.h"
#include "Embedded_Display/Spice_View.h"
#include "Utils.h"

#ifdef VNC_DISPLAY
#include "Embedded_Display/Machine_View.h"
#endif

VM_Session_Widget::VM_Session_Widget( QWidget *parent )
	: QWidget( parent )
	, VM( nullptr )
	, QMP( nullptr )
	, Toolbar( nullptr )
	, Stack( new QStackedWidget( this ) )
	, Placeholder( new QLabel( this ) )
	, Spice( new Spice_View( this ) )
#ifdef VNC_DISPLAY
	, Vnc( nullptr )
#endif
	, Spice_Port( 0 )
	, Vnc_Port( 0 )
{
	QVBoxLayout *lay = new QVBoxLayout( this );
	lay->setContentsMargins( 0, 0, 0, 0 );
	lay->setSpacing( 0 );

	Build_Toolbar();
	lay->addWidget( Toolbar );

	Placeholder->setAlignment( Qt::AlignCenter );
	Placeholder->setText( tr( "Starting embedded guest display…" ) );
	Stack->addWidget( Placeholder );
	Stack->addWidget( Spice );
#ifdef VNC_DISPLAY
	Vnc = new MachineView( this, nullptr );
	Stack->addWidget( Vnc );
#endif
	lay->addWidget( Stack, 1 );

	connect( Spice, SIGNAL(Connected()), this, SLOT(On_Display_Connected()) );
	connect( Spice, SIGNAL(Connection_Error(QString)), this, SLOT(On_Display_Error(QString)) );
}

VM_Session_Widget::~VM_Session_Widget()
{
	Detach();
}

void VM_Session_Widget::Build_Toolbar()
{
	Toolbar = new QToolBar( this );
	Toolbar->setMovable( false );
	Toolbar->setIconSize( QSize( 20, 20 ) );

	Toolbar->addAction( tr("CD/DVD…"), this, SLOT(On_Change_CD()) );
	Toolbar->addAction( tr("Eject CD"), this, SLOT(On_Eject_CD()) );
	Toolbar->addSeparator();
	Toolbar->addAction( tr("Floppy A…"), this, SLOT(On_Change_FD0()) );
	Toolbar->addAction( tr("Eject A"), this, SLOT(On_Eject_FD0()) );
	Toolbar->addAction( tr("Floppy B…"), this, SLOT(On_Change_FD1()) );
	Toolbar->addAction( tr("Eject B"), this, SLOT(On_Eject_FD1()) );
	Toolbar->addSeparator();
	Toolbar->addAction( tr("Ctrl+Alt+Del"), this, SLOT(On_CAD()) );
	Toolbar->addAction( tr("Fullscreen"), this, SLOT(On_Fullscreen()) );
	Toolbar->addSeparator();
	Toolbar->addAction( tr("Reset"), this, SIGNAL(Request_Reset()) );
	Toolbar->addAction( tr("Shut Down"), this, SIGNAL(Request_Shutdown()) );
	Toolbar->addAction( tr("Power Off"), this, SIGNAL(Request_Stop()) );
	Toolbar->addSeparator();
	Toolbar->addAction( tr("Exit View"), this, SLOT(On_Exit_View()) );
}

QString VM_Session_Widget::Pick_Backend( const QString &preferred ) const
{
	QString p = preferred.toLower();
	if( p.isEmpty() )
		p = QSettings().value( "Embedded_Display_Backend", "spice" ).toString().toLower();

	if( p == "spice" && Spice && Spice->Spice_GTK_Available() )
		return "spice";
	// Prefer VNC when spice-gtk missing (still headless QEMU)
#ifdef VNC_DISPLAY
	return "vnc";
#else
	if( p == "spice" )
		return "spice"; // server-only; show status
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

	Placeholder->setText( tr( "Connecting %1 display on %2…" )
		.arg( Backend.toUpper() ).arg( Host ) );
	Stack->setCurrentWidget( Placeholder );

	if( Backend == "spice" && Spice_Port > 0 && Spice && Spice->Spice_GTK_Available() )
	{
		Stack->setCurrentWidget( Spice );
		Spice->Connect_To( Host, Spice_Port );
		// On Connection_Error, On_Display_Error switches to VNC
	}
	else
	{
#ifdef VNC_DISPLAY
		Backend = "vnc";
		if( Vnc && Vnc_Port > 0 )
		{
			Placeholder->setText( tr( "Connecting VNC display on %1:%2…" )
				.arg( Host ).arg( Vnc_Port ) );
			Stack->setCurrentWidget( Vnc );
			Vnc->Set_VNC_URL( Host, Vnc_Port );
			Vnc->initView();
			connect( Vnc, SIGNAL(Connected()), this, SLOT(On_Display_Connected()), Qt::UniqueConnection );
		}
		else
#endif
		{
			Placeholder->setText( tr(
				"QEMU is running headless (SPICE %1 / VNC %2).\n"
				"Build with LibVNC (drop WITHOUT_EMBEDDED_DISPLAY) or spice-gtk "
				"(-DAQEMU_WITH_SPICE_GTK=ON) to embed the guest display.\n"
				"Use Exit View to return to the VM list; the guest keeps running." )
				.arg( Spice_Port ).arg( Vnc_Port ) );
			Stack->setCurrentWidget( Placeholder );
		}
	}
}

void VM_Session_Widget::Detach()
{
	if( Spice )
		Spice->Disconnect();
#ifdef VNC_DISPLAY
	if( Vnc )
		Vnc->disconnectVNC();
#endif
	VM = nullptr;
	QMP = nullptr;
}

void VM_Session_Widget::On_Display_Connected()
{
	Placeholder->setText( tr( "Connected." ) );
}

void VM_Session_Widget::On_Display_Error( const QString &msg )
{
	AQWarning( "VM_Session_Widget", msg );
#ifdef VNC_DISPLAY
	if( Backend == "spice" && Vnc_Port > 0 && Vnc )
	{
		Backend = "vnc";
		Stack->setCurrentWidget( Vnc );
		Vnc->Set_VNC_URL( Host, Vnc_Port );
		Vnc->initView();
		return;
	}
#endif
	Placeholder->setText( tr( "Display error: %1" ).arg( msg ) );
	Stack->setCurrentWidget( Placeholder );
}

void VM_Session_Widget::On_Change_CD()
{
	if( ! QMP ) return;
	QString file = QFileDialog::getOpenFileName( this, tr("Select CD/DVD image"),
		QString(), tr("Images (*.iso *.img *.cdr);;All (*)") );
	if( file.isEmpty() ) return;
	if( ! QMP->Change_Medium( "aqemu-cdrom", file ) )
		QMessageBox::warning( this, tr("Media"), tr("QMP change CD failed.") );
}

void VM_Session_Widget::On_Eject_CD()
{
	if( QMP ) QMP->Eject_Medium( "aqemu-cdrom", true );
}

void VM_Session_Widget::On_Change_FD0()
{
	if( ! QMP ) return;
	QString file = QFileDialog::getOpenFileName( this, tr("Select Floppy A image"),
		QString(), tr("Floppy (*.img *.ima *.vfd);;All (*)") );
	if( file.isEmpty() ) return;
	QMP->Change_Medium( "aqemu-fd0", file );
}

void VM_Session_Widget::On_Eject_FD0()
{
	if( QMP ) QMP->Eject_Medium( "aqemu-fd0", true );
}

void VM_Session_Widget::On_Change_FD1()
{
	if( ! QMP ) return;
	QString file = QFileDialog::getOpenFileName( this, tr("Select Floppy B image"),
		QString(), tr("Floppy (*.img *.ima *.vfd);;All (*)") );
	if( file.isEmpty() ) return;
	QMP->Change_Medium( "aqemu-fd1", file );
}

void VM_Session_Widget::On_Eject_FD1()
{
	if( QMP ) QMP->Eject_Medium( "aqemu-fd1", true );
}

void VM_Session_Widget::On_CAD()
{
	if( Spice && Backend == "spice" )
		Spice->Send_CAD();
}

void VM_Session_Widget::On_Fullscreen()
{
	QWidget *w = window();
	if( ! w ) return;
	if( w->isFullScreen() )
		w->showNormal();
	else
		w->showFullScreen();
}

void VM_Session_Widget::On_Exit_View()
{
	emit Exit_Session_View();
}
