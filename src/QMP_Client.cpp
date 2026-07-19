/****************************************************************************
** AQEMU — QMP (QEMU Machine Protocol) JSON client
****************************************************************************/
#include "QMP_Client.h"

#include <QHostAddress>
#include <QAbstractSocket>
#include <QTcpServer>
#include <QJsonArray>

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
}

bool QMP_Client::Is_Connected() const
{
	return Socket->state() == QAbstractSocket::ConnectedState && Capabilities_Sent;
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
		if( obj.value( "return" ).isArray() )
			emit Block_Info( obj.value( "return" ).toArray() );
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

	QJsonObject cmd;
	cmd.insert( "execute", execute );
	if( ! arguments.isEmpty() )
		cmd.insert( "arguments", arguments );
	cmd.insert( "id", Next_Id++ );

	QByteArray payload = QJsonDocument( cmd ).toJson( QJsonDocument::Compact );
	payload.append( '\n' );
	return Socket->write( payload ) == payload.size();
}

bool QMP_Client::Change_Medium( const QString &device_id, const QString &file_path )
{
	QJsonObject args;
	args.insert( "device", device_id );
	args.insert( "filename", file_path );
	return Send_Command( "change", args );
}

bool QMP_Client::Eject_Medium( const QString &device_id, bool force )
{
	QJsonObject args;
	args.insert( "device", device_id );
	args.insert( "force", force );
	return Send_Command( "eject", args );
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
