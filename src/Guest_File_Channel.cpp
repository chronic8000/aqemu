#include "Guest_File_Channel.h"

#include <QHostAddress>

Guest_File_Channel::Guest_File_Channel( QObject *parent )
	: QObject( parent )
	, port( 0 )
{
	retry.setInterval( 2000 );
	connect( &retry, SIGNAL(timeout()), this, SLOT(Try_Connect()) );
	connect( &socket, SIGNAL(readyRead()), this, SLOT(On_Ready_Read()) );
	connect( &socket, SIGNAL(disconnected()), this, SLOT(On_Disconnected()) );
}

void Guest_File_Channel::Start( quint16 listen_port )
{
	Stop();
	port = listen_port;
	if( port == 0 )
		return;
	retry.start();
	Try_Connect();
}

void Guest_File_Channel::Stop()
{
	retry.stop();
	port = 0;
	socket.abort();
	parser = AQ_Guest_Frame_Parser();
}

void Guest_File_Channel::Try_Connect()
{
	if( port == 0 )
		return;
	if( socket.state() == QAbstractSocket::ConnectedState || socket.state() == QAbstractSocket::ConnectingState )
		return;
	socket.connectToHost( QHostAddress::LocalHost, port );
}

void Guest_File_Channel::On_Ready_Read()
{
	parser.Add( socket.readAll() );
	if( ! parser.Error().isEmpty() )
	{
		emit Channel_Error( parser.Error() );
		parser = AQ_Guest_Frame_Parser();
		return;
	}
	const QList<AQ_Guest_File> files = parser.Take();
	if( ! files.isEmpty() )
		emit Files_Arrived( files );
}

void Guest_File_Channel::On_Disconnected()
{
	if( port != 0 )
		retry.start();
}
