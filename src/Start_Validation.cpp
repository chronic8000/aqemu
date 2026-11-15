/****************************************************************************
** Pre-start display and port checks (issue #107). Behaviour inspired by
** TrueNAS pylibvirt; clean-room, no pylibvirt code.
****************************************************************************/
#include "Start_Validation.h"

#include "VM.h"
#include "VM_Devices.h"
#include "Utils.h"

#include <QHostAddress>
#include <QMessageBox>
#include <QPair>
#include <QTcpServer>

static bool Port_In_Range( int port )
{
	return port >= 1 && port <= 65535;
}

static bool Port_Bindable( int port )
{
	if( ! Port_In_Range( port ) )
		return false;
	QTcpServer probe;
	return probe.listen( QHostAddress::LocalHost, static_cast<quint16>( port ) );
}

static void Check_Vnc_Password_Length( const Virtual_Machine *vm, QList<AQ_Start_Issue> &out )
{
	if( ! vm || ! vm->Use_VNC() || ! vm->Use_VNC_Password() )
		return;
	// Password is entered after Start_impl today; UI caps at 8. Warn so users know the limit.
	AQ_Start_Issue issue;
	issue.kind = AQ_Start_Issue::Warning;
	issue.text = QObject::tr(
		"VNC passwords are limited to 8 characters by the VNC protocol. "
		"The password dialog enforces that length." );
	out << issue;
}

static void Check_Spice_Password_Length( const Virtual_Machine *vm, QList<AQ_Start_Issue> &out )
{
	if( ! vm )
		return;
	const VM_SPICE &spice = vm->Get_SPICE();
	if( ! spice.Use_SPICE() || ! spice.Use_Password() )
		return;
	if( spice.Get_Password().size() > 8 )
	{
		AQ_Start_Issue issue;
		issue.kind = AQ_Start_Issue::Error;
		issue.text = QObject::tr(
			"The SPICE password is %1 characters. Keep display passwords to 8 characters or fewer." )
			.arg( spice.Get_Password().size() );
		out << issue;
	}
}

static void Check_Display_Ports( const Virtual_Machine *vm, QList<AQ_Start_Issue> &out )
{
	if( ! vm )
		return;

	QList<QPair<QString, int> > ports;

	if( vm->Use_VNC() && ! vm->Get_VNC_Socket_Mode() )
	{
		const int port = 5900 + vm->Get_VNC_Display_Number();
		ports << qMakePair( QObject::tr( "VNC" ), port );
	}

	const VM_SPICE &spice = vm->Get_SPICE();
	if( spice.Use_SPICE() )
	{
		ports << qMakePair( QObject::tr( "SPICE" ), static_cast<int>( spice.Get_Port() ) );
		if( spice.Use_SPort() )
			ports << qMakePair( QObject::tr( "SPICE TLS (SPort)" ), static_cast<int>( spice.Get_SPort() ) );
	}

	for( int i = 0; i < ports.count(); ++i )
	{
		const QString &label = ports.at( i ).first;
		const int port = ports.at( i ).second;
		if( ! Port_In_Range( port ) )
		{
			AQ_Start_Issue issue;
			issue.kind = AQ_Start_Issue::Error;
			issue.text = QObject::tr( "%1 port %2 is outside 1–65535." ).arg( label ).arg( port );
			out << issue;
			continue;
		}
		for( int j = 0; j < i; ++j )
		{
			if( ports.at( j ).second == port )
			{
				AQ_Start_Issue issue;
				issue.kind = AQ_Start_Issue::Error;
				issue.text = QObject::tr( "%1 and %2 both use port %3." )
					.arg( ports.at( j ).first, label ).arg( port );
				out << issue;
			}
		}
		if( ! Port_Bindable( port ) )
		{
			AQ_Start_Issue issue;
			issue.kind = AQ_Start_Issue::Error;
			issue.text = QObject::tr( "%1 port %2 is already in use on this host." )
				.arg( label ).arg( port );
			out << issue;
		}
	}
}

static void Check_Display_Device( const Virtual_Machine *vm, QList<AQ_Start_Issue> &out )
{
	if( ! vm )
		return;
	const QString video = vm->Get_Video_Card().trimmed().toLower();
	const QString machine = vm->Get_Computer_Type().trimmed().toLower();
	if( video.isEmpty() || video == QLatin1String( "none" ) )
		return;
	if( machine.contains( QLatin1String( "aarch64" ) ) || machine.contains( QLatin1String( "arm" ) ) )
	{
		if( video == QLatin1String( "vmware" ) || video == QLatin1String( "qxl" )
		    || video == QLatin1String( "cirrus" ) || video == QLatin1String( "std" ) )
		{
			AQ_Start_Issue issue;
			issue.kind = AQ_Start_Issue::Warning;
			issue.text = QObject::tr(
				"Video device \"%1\" is unusual on ARM guests. Prefer virtio-gpu or ramfb." )
				.arg( vm->Get_Video_Card() );
			out << issue;
		}
	}
}

QList<AQ_Start_Issue> AQ_Collect_Start_Issues( const Virtual_Machine *vm )
{
	QList<AQ_Start_Issue> out;
	Check_Vnc_Password_Length( vm, out );
	Check_Spice_Password_Length( vm, out );
	Check_Display_Ports( vm, out );
	Check_Display_Device( vm, out );
	return out;
}

bool AQ_Run_Start_Validation( Virtual_Machine *vm )
{
	const QList<AQ_Start_Issue> issues = AQ_Collect_Start_Issues( vm );
	if( issues.isEmpty() )
		return true;

	QStringList errors;
	QStringList warnings;
	for( int i = 0; i < issues.count(); ++i )
	{
		if( issues.at( i ).kind == AQ_Start_Issue::Error )
			errors << issues.at( i ).text;
		else
			warnings << issues.at( i ).text;
	}

	QString body;
	if( ! errors.isEmpty() )
	{
		body += QObject::tr( "Start was blocked:" ) + QStringLiteral( "\n\n• " )
			+ errors.join( QStringLiteral( "\n• " ) );
	}
	if( ! warnings.isEmpty() )
	{
		if( ! body.isEmpty() )
			body += QStringLiteral( "\n\n" );
		body += QObject::tr( "Warnings:" ) + QStringLiteral( "\n\n• " )
			+ warnings.join( QStringLiteral( "\n• " ) );
	}

	if( ! errors.isEmpty() )
	{
		AQGraphic_Error( "AQ_Run_Start_Validation", QObject::tr( "Cannot start" ), body, false );
		return false;
	}

	const int answer = QMessageBox::warning( nullptr, QObject::tr( "Start checks" ),
		body + QStringLiteral( "\n\n" ) + QObject::tr( "Continue starting this virtual machine?" ),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
	return answer == QMessageBox::Yes;
}
