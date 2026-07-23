/****************************************************************************
** AQEMU — QMP (QEMU Machine Protocol) JSON client
****************************************************************************/
#include "QMP_Client.h"

#include <QHostAddress>
#include <QAbstractSocket>
#include <QTcpServer>
#include <QJsonArray>
#include <QDir>

#include "Utils.h"

quint16 Find_Free_TCP_Port( quint16 start )
{
	for( quint16 p = start; p < start + 2000; ++p )
	{
		QTcpServer probe;
		if( probe.listen( QHostAddress::LocalHost, p ) )
		{
			quint16 ok = probe.serverPort();
			probe.close();
			return ok;
		}
	}
	return 0;
}

QMP_Client::QMP_Client( QObject *parent )
	: QObject( parent )
	, Socket( new QTcpSocket( this ) )
	, Port_( 0 )
	, Capabilities_Sent( false )
	, Next_Id( 1 )
{
	connect( Socket, SIGNAL(readyRead()), this, SLOT(On_Ready_Read()) );
	connect( Socket, SIGNAL(error(QAbstractSocket::SocketError)),
	         this, SLOT(On_Socket_Error(QAbstractSocket::SocketError)) );
	connect( Socket, SIGNAL(connected()), this, SLOT(On_Connected()) );
	connect( Socket, SIGNAL(disconnected()), this, SIGNAL(Disconnected()) );
}

bool QMP_Client::Connect_To( const QString &host, quint16 port )
{
	Disconnect();
	Port_ = port;
	Capabilities_Sent = false;
	Buffer.clear();
	Socket->connectToHost( host, port );
	return Socket->waitForConnected( 3000 );
}

void QMP_Client::Disconnect()
{
	if( Socket->state() != QAbstractSocket::UnconnectedState )
		Socket->disconnectFromHost();
	Capabilities_Sent = false;
	Buffer.clear();
	Pending_Commands.clear();
}

bool QMP_Client::Is_Connected() const
{
	return Socket->state() == QAbstractSocket::ConnectedState && Capabilities_Sent;
}

bool QMP_Client::Is_Connecting() const
{
	const QAbstractSocket::SocketState st = Socket->state();
	if( st == QAbstractSocket::HostLookupState || st == QAbstractSocket::ConnectingState )
		return true;
	return st == QAbstractSocket::ConnectedState && ! Capabilities_Sent;
}

void QMP_Client::On_Connected()
{
	AQDebug( "QMP_Client::On_Connected()", "TCP connected, waiting for greeting…" );
}

void QMP_Client::On_Socket_Error( QAbstractSocket::SocketError err )
{
	Q_UNUSED( err );
	emit Error( Socket->errorString() );
}

void QMP_Client::On_Ready_Read()
{
	Buffer.append( Socket->readAll() );
	int nl;
	while( ( nl = Buffer.indexOf( '\n' ) ) >= 0 )
	{
		QByteArray line = Buffer.left( nl ).trimmed();
		Buffer.remove( 0, nl + 1 );
		if( ! line.isEmpty() )
			Handle_Line( line );
	}
}

void QMP_Client::Handle_Line( const QByteArray &line )
{
	QJsonParseError pe;
	QJsonDocument doc = QJsonDocument::fromJson( line, &pe );
	if( pe.error != QJsonParseError::NoError || ! doc.isObject() )
	{
		AQWarning( "QMP_Client::Handle_Line()", "Bad JSON: " + QString::fromUtf8( line ) );
		return;
	}
	QJsonObject obj = doc.object();

	if( obj.contains( "QMP" ) && ! Capabilities_Sent )
	{
		Send_Command( "qmp_capabilities" );
		return;
	}

	if( obj.value( "return" ).isObject() || obj.contains( "return" ) )
	{
		if( ! Capabilities_Sent )
		{
			Capabilities_Sent = true;
			AQDebug( "QMP_Client::Handle_Line()", "qmp_capabilities OK" );
			emit Connected();
		}
		emit Reply_Received( obj );

		const int reply_id = obj.value( "id" ).toInt( -1 );
		const QString cmd = Pending_Commands.take( reply_id );
		if( obj.value( "return" ).isArray() )
		{
			const QJsonArray arr = obj.value( "return" ).toArray();
			if( cmd == "query-blockstats" )
				emit Block_Stats( arr );
			else
				emit Block_Info( arr );
		}
		return;
	}

	if( obj.contains( "event" ) )
	{
		emit Event_Received( obj );
		return;
	}

	if( obj.contains( "error" ) )
		emit Error( obj.value( "error" ).toObject().value( "desc" ).toString() );
}

bool QMP_Client::Send_Command( const QString &execute, const QJsonObject &arguments )
{
	if( Socket->state() != QAbstractSocket::ConnectedState )
		return false;

	const int id = Next_Id++;
	QJsonObject cmd;
	cmd.insert( "execute", execute );
	if( ! arguments.isEmpty() )
		cmd.insert( "arguments", arguments );
	cmd.insert( "id", id );
	Pending_Commands.insert( id, execute );

	QByteArray payload = QJsonDocument( cmd ).toJson( QJsonDocument::Compact );
	payload.append( '\n' );
	return Socket->write( payload ) == payload.size();
}

bool QMP_Client::Change_Medium( const QString &device_id, const QString &file_path )
{
	const QString path = QDir::fromNativeSeparators( file_path );

	// For "-drive …,id=aqemu-fd0" the block backend name is the QMP "device"
	// field. QOM "id" is a different path (e.g. /machine/unattached/device[N])
	// and fails with DeviceNotFound — which left us stuck on null-co forever.
	QJsonObject by_device;
	by_device.insert( "device", device_id );
	by_device.insert( "filename", path );
	by_device.insert( "format", "raw" );
	if( Send_Command( "blockdev-change-medium", by_device ) )
		return true;

	QJsonObject by_id;
	by_id.insert( "id", device_id );
	by_id.insert( "filename", path );
	by_id.insert( "format", "raw" );
	return Send_Command( "blockdev-change-medium", by_id );
}

bool QMP_Client::Eject_Medium( const QString &device_id, bool force )
{
	QJsonObject by_device;
	by_device.insert( "device", device_id );
	by_device.insert( "force", force );
	if( Send_Command( "eject", by_device ) )
		return true;

	QJsonObject by_id;
	by_id.insert( "id", device_id );
	by_id.insert( "force", force );
	return Send_Command( "eject", by_id );
}

bool QMP_Client::System_Powerdown()
{
	return Send_Command( "system_powerdown" );
}

bool QMP_Client::System_Reset()
{
	return Send_Command( "system_reset" );
}

bool QMP_Client::Quit_QEMU()
{
	return Send_Command( "quit" );
}

bool QMP_Client::Query_Block()
{
	return Send_Command( "query-block" );
}

bool QMP_Client::Query_Blockstats()
{
	return Send_Command( "query-blockstats" );
}

bool QMP_Client::Human_Monitor( const QString &command_line )
{
	QJsonObject args;
	args.insert( "command-line", command_line );
	return Send_Command( "human-monitor-command", args );
}

bool QMP_Client::Migrate( const QString &uri, bool blk, bool inc )
{
	if( uri.trimmed().isEmpty() )
		return false;
	QJsonObject args;
	args.insert( "uri", uri.trimmed() );
	if( blk )
		args.insert( "blk", true );
	if( inc )
		args.insert( "inc", true );
	return Send_Command( "migrate", args );
}

bool QMP_Client::Query_Migrate()
{
	return Send_Command( "query-migrate" );
}

bool QMP_Client::Migrate_Cancel()
{
	return Send_Command( "migrate_cancel" );
}

void QMP_Client::Connect_Async( const QString &host, quint16 port )
{
	if( Is_Connected() || Is_Connecting() )
		return;
	if( Socket->state() != QAbstractSocket::UnconnectedState )
		Disconnect();
	Port_ = port;
	Capabilities_Sent = false;
	Buffer.clear();
	Socket->connectToHost( host, port );
}
