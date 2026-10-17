#include "Guest_File_Channel.h"

static QString AQ_Local_Socket_Name( const QString &qemu_path )
{
#ifdef Q_OS_WIN
	const QString prefix = QStringLiteral( "\\\\.\\pipe\\" );
	if( qemu_path.startsWith( prefix, Qt::CaseInsensitive ) )
		return qemu_path.mid( prefix.size() );
#endif
	return qemu_path;
}

Guest_File_Channel::Guest_File_Channel( QObject *parent )
	: QObject( parent )
{
	retry.setInterval( 2000 );
	connect( &retry, SIGNAL(timeout()), this, SLOT(Try_Connect()) );
	connect( &socket, SIGNAL(readyRead()), this, SLOT(On_Ready_Read()) );
	connect( &socket, SIGNAL(disconnected()), this, SLOT(On_Disconnected()) );
}

void Guest_File_Channel::Start( const QString &socket_path )
{
	Stop();
	path = socket_path;
	if( path.isEmpty() )
		return;
	retry.start();
	Try_Connect();
}

void Guest_File_Channel::Stop()
{
	retry.stop();
	path.clear();
	socket.abort();
	parser = AQ_Guest_Frame_Parser();
}

void Guest_File_Channel::Try_Connect()
{
	if( path.isEmpty() )
		return;
	if( socket.state() == QLocalSocket::ConnectedState || socket.state() == QLocalSocket::ConnectingState )
		return;
	socket.connectToServer( AQ_Local_Socket_Name( path ) );
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
	if( ! path.isEmpty() )
		retry.start();
}
