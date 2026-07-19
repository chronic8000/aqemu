/****************************************************************************
** SPICE guest display
****************************************************************************/
#include "Spice_View.h"

#include <QVBoxLayout>
#include <QLabel>

#include "Utils.h"

#ifdef AQEMU_HAVE_SPICE_GTK
#include <glib-object.h>
#include <spice-client.h>
#endif

Spice_View::Spice_View( QWidget *parent )
	: Guest_Display_View( parent )
	, Layout( new QVBoxLayout( this ) )
	, Status( new QLabel( this ) )
	, Port( 0 )
	, Connected_Flag( false )
#ifdef AQEMU_HAVE_SPICE_GTK
	, Session( nullptr )
	, Spice_Container( nullptr )
#endif
{
	Layout->setContentsMargins( 0, 0, 0, 0 );
	Status->setAlignment( Qt::AlignCenter );
	Status->setWordWrap( true );
	Layout->addWidget( Status );

	if( Spice_GTK_Available() )
		Status->setText( tr( "SPICE client ready." ) );
	else
		Status->setText( tr(
			"SPICE client (spice-gtk) was not compiled into this AQEMU build.\n"
			"Embedded session uses VNC fallback with QEMU -display none.\n"
			"On Linux/Pi: cmake -DAQEMU_WITH_SPICE_GTK=ON …" ) );
}

Spice_View::~Spice_View()
{
	Disconnect();
}

bool Spice_View::Spice_GTK_Available() const
{
#ifdef AQEMU_HAVE_SPICE_GTK
	return true;
#else
	return false;
#endif
}

QString Spice_View::Backend_Name() const
{
	return Spice_GTK_Available() ? QStringLiteral( "spice" ) : QStringLiteral( "spice-unavailable" );
}

void Spice_View::Connect_To( const QString &host, int port )
{
	Host = host;
	Port = port;
	Disconnect();

#ifdef AQEMU_HAVE_SPICE_GTK
	Status->setText( tr( "SPICE: connecting to %1:%2 …" ).arg( host ).arg( port ) );

	Session = spice_session_new();
	g_object_set( Session,
	              "host", host.toUtf8().constData(),
	              "port", QByteArray::number( port ).constData(),
	              nullptr );

	if( ! spice_session_connect( Session ) )
	{
		Status->setText( tr( "SPICE session connect failed." ) );
		Connected_Flag = false;
		emit Connection_Error( tr( "spice_session_connect failed" ) );
		return;
	}

	// Pixel embedding of SpiceDisplay in Qt is not finished — keep the session
	// for tooling, but signal the session shell to use VNC for the guest view.
	Connected_Flag = false;
	Status->setText( tr( "SPICE server is up at %1:%2; using VNC for the embedded view." )
	                     .arg( host ).arg( port ) );
	emit Connection_Error( tr( "spice display widget not embedded; VNC fallback" ) );
#else
	Status->setText( tr( "No spice-gtk — session shell will use VNC on port %1." ).arg( port ) );
	Connected_Flag = false;
	emit Connection_Error( tr( "spice-gtk not available" ) );
#endif
}

void Spice_View::Disconnect()
{
#ifdef AQEMU_HAVE_SPICE_GTK
	if( Session )
	{
		spice_session_disconnect( Session );
		g_clear_object( &Session );
		Session = nullptr;
	}
#endif
	Connected_Flag = false;
	emit Disconnected();
}

bool Spice_View::Is_Connected() const
{
	return Connected_Flag;
}

void Spice_View::Send_CAD()
{
	AQDebug( "Spice_View::Send_CAD()", Backend_Name() );
}
