#include "Guest_Agent.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocalSocket>

static const int kChunk = 48 * 1024;
static const int kMaxFile = 64 * 1024 * 1024;

AQ_Guest_Frame_Parser::AQ_Guest_Frame_Parser()
	: step( Magic )
	, files_left( 0 )
	, body_left( 0 )
{
}

void AQ_Guest_Frame_Parser::Fail( const QString &message )
{
	error = message;
	pending.clear();
	batch.clear();
	body.clear();
	step = Magic;
}

bool AQ_Guest_Frame_Parser::Take_Line( QByteArray &line )
{
	const int nl = pending.indexOf( '\n' );
	if( nl < 0 )
		return false;
	line = pending.left( nl );
	if( line.endsWith( '\r' ) )
		line.chop( 1 );
	pending.remove( 0, nl + 1 );
	return true;
}

void AQ_Guest_Frame_Parser::Add( const QByteArray &chunk )
{
	if( ! error.isEmpty() )
		return;
	pending += chunk;
	while( error.isEmpty() )
	{
		if( step == Body )
		{
			if( pending.size() < body_left )
				return;
			body += pending.left( body_left );
			pending.remove( 0, body_left );
			AQ_Guest_File file;
			file.name = current_name;
			file.data = body;
			batch << file;
			body.clear();
			body_left = 0;
			--files_left;
			step = files_left > 0 ? Name : Magic;
			if( files_left <= 0 )
			{
				done << batch;
				batch.clear();
			}
			continue;
		}
		QByteArray line;
		if( ! Take_Line( line ) )
			return;
		const QString text = QString::fromUtf8( line );
		if( step == Magic )
		{
			if( text != QLatin1String( "AQEMU1" ) )
			{
				Fail( QStringLiteral( "The guest helper sent an unknown message." ) );
				return;
			}
			step = Kind;
		}
		else if( step == Kind )
		{
			if( text != QLatin1String( "FROM-GUEST" ) )
			{
				Fail( QStringLiteral( "The guest helper sent an unknown message." ) );
				return;
			}
			step = Count;
		}
		else if( step == Count )
		{
			bool ok = false;
			files_left = text.toInt( &ok );
			if( ! ok || files_left < 1 || files_left > 64 )
			{
				Fail( QStringLiteral( "The guest helper sent a bad file count." ) );
				return;
			}
			step = Name;
		}
		else if( step == Name )
		{
			current_name = QFileInfo( text ).fileName();
			if( current_name.isEmpty() || current_name == QLatin1String( "." ) || current_name == QLatin1String( ".." ) )
			{
				Fail( QStringLiteral( "The guest helper sent a bad file name." ) );
				return;
			}
			step = Size;
		}
		else if( step == Size )
		{
			bool ok = false;
			body_left = text.toInt( &ok );
			if( ! ok || body_left < 0 || body_left > kMaxFile )
			{
				Fail( QStringLiteral( "The guest helper sent a file larger than 64 MB." ) );
				return;
			}
			body.clear();
			step = Body;
			if( body_left == 0 )
			{
				AQ_Guest_File file;
				file.name = current_name;
				batch << file;
				--files_left;
				step = files_left > 0 ? Name : Magic;
				if( files_left <= 0 )
				{
					done << batch;
					batch.clear();
				}
			}
		}
	}
}

QList<AQ_Guest_File> AQ_Guest_Frame_Parser::Take()
{
	QList<AQ_Guest_File> out;
	for( int i = 0; i < done.size(); ++i )
		out << done.at( i );
	done.clear();
	return out;
}

AQ_Guest_Agent::AQ_Guest_Agent()
	: socket( new QLocalSocket() )
	, cancel_flag( nullptr )
{
}

void AQ_Guest_Agent::Set_Cancel( QAtomicInt *flag )
{
	cancel_flag = flag;
}

bool AQ_Guest_Agent::Cancelled( QString *error )
{
	if( ! cancel_flag || cancel_flag->loadRelaxed() == 0 )
		return false;
	socket->abort();
	if( error )
		*error = QStringLiteral( "The copy was cancelled." );
	return true;
}

void AQ_Guest_Agent::Close_Handle( int handle )
{
	if( handle < 0 )
		return;
	QJsonObject close_args;
	close_args.insert( QStringLiteral( "handle" ), handle );
	QString ignored;
	Exec( QStringLiteral( "guest-file-close" ), close_args, &ignored );
}

AQ_Guest_Agent::~AQ_Guest_Agent()
{
	socket->disconnectFromServer();
	delete socket;
}

QJsonValue AQ_Guest_Agent::Exec( const QString &command, const QJsonObject &arguments, QString *error )
{
	if( Cancelled( error ) )
		return QJsonValue();
	if( error )
		error->clear();
	QJsonObject cmd;
	cmd.insert( QStringLiteral( "execute" ), command );
	if( ! arguments.isEmpty() )
		cmd.insert( QStringLiteral( "arguments" ), arguments );
	QByteArray line = QJsonDocument( cmd ).toJson( QJsonDocument::Compact );
	line.append( '\n' );
	if( socket->write( line ) != line.size() || ! socket->waitForBytesWritten( 5000 ) )
	{
		if( error )
			*error = QStringLiteral( "Could not talk to the guest agent." );
		return QJsonValue();
	}
	for( int spins = 0; spins < 40 && ! incoming.contains( '\n' ); ++spins )
	{
		if( Cancelled( error ) )
			return QJsonValue();
		if( ! socket->waitForReadyRead( 500 ) )
			break;
		incoming += socket->readAll();
	}
	incoming += socket->readAll();
	const int nl = incoming.indexOf( '\n' );
	if( nl < 0 )
	{
		if( error )
			*error = QStringLiteral( "The guest agent did not answer. Install qemu-guest-agent in the guest and start it." );
		return QJsonValue();
	}
	const QByteArray one = incoming.left( nl );
	incoming.remove( 0, nl + 1 );
	QJsonParseError parse_error;
	const QJsonDocument doc = QJsonDocument::fromJson( one, &parse_error );
	if( parse_error.error != QJsonParseError::NoError || ! doc.isObject() )
	{
		if( error )
			*error = QStringLiteral( "The guest agent sent a reply AQEMU could not read." );
		return QJsonValue();
	}
	const QJsonObject obj = doc.object();
	if( obj.contains( QStringLiteral( "error" ) ) )
	{
		if( error )
			*error = obj.value( QStringLiteral( "error" ) ).toObject().value( QStringLiteral( "desc" ) ).toString();
		if( error && error->isEmpty() )
			*error = QStringLiteral( "The guest agent refused the request." );
		return QJsonValue();
	}
	return obj.value( QStringLiteral( "return" ) );
}

static QString AQ_Local_Socket_Name( const QString &qemu_path )
{
#ifdef Q_OS_WIN
	const QString prefix = QStringLiteral( "\\\\.\\pipe\\" );
	if( qemu_path.startsWith( prefix, Qt::CaseInsensitive ) )
		return qemu_path.mid( prefix.size() );
#else
	Q_UNUSED( qemu_path );
#endif
	return qemu_path;
}

bool AQ_Guest_Agent::Open( const QString &socket_path, QString *error )
{
	if( socket_path.isEmpty() )
	{
		if( error )
			*error = QStringLiteral( "This machine has no guest agent channel." );
		return false;
	}
	socket->connectToServer( AQ_Local_Socket_Name( socket_path ) );
	if( ! socket->waitForConnected( 2000 ) )
	{
		if( error )
			*error = QStringLiteral( "Could not connect to the guest agent." );
		return false;
	}
	const QJsonValue pong = Exec( QStringLiteral( "guest-ping" ), QJsonObject(), error );
	Q_UNUSED( pong );
	return error == nullptr || error->isEmpty();
}

bool AQ_Guest_Agent::Copy_To_Guest( const QString &host_path, const QString &guest_path, QString *error )
{
	QFile in( host_path );
	if( ! in.open( QIODevice::ReadOnly ) )
	{
		if( error )
			*error = QStringLiteral( "Cannot read %1" ).arg( host_path );
		return false;
	}
	QJsonObject open_args;
	open_args.insert( QStringLiteral( "path" ), guest_path );
	open_args.insert( QStringLiteral( "mode" ), QStringLiteral( "wb" ) );
	const QJsonValue handle_v = Exec( QStringLiteral( "guest-file-open" ), open_args, error );
	if( error && ! error->isEmpty() )
		return false;
	const int handle = handle_v.toInt( -1 );
	if( handle < 0 )
	{
		if( error )
			*error = QStringLiteral( "The guest agent did not open %1" ).arg( guest_path );
		return false;
	}
	while( ! in.atEnd() )
	{
		const QByteArray chunk = in.read( kChunk );
		QJsonObject write_args;
		write_args.insert( QStringLiteral( "handle" ), handle );
		write_args.insert( QStringLiteral( "buf-b64" ), QString::fromLatin1( chunk.toBase64() ) );
		write_args.insert( QStringLiteral( "count" ), chunk.size() );
		const QJsonValue wrote = Exec( QStringLiteral( "guest-file-write" ), write_args, error );
		if( error && ! error->isEmpty() )
		{
			const QString write_error = *error;
			Close_Handle( handle );
			*error = write_error;
			return false;
		}
		if( wrote.toObject().value( QStringLiteral( "count" ) ).toInt() != chunk.size() )
		{
			Close_Handle( handle );
			if( error )
				*error = QStringLiteral( "The guest did not accept the whole file." );
			return false;
		}
	}
	QJsonObject close_args;
	close_args.insert( QStringLiteral( "handle" ), handle );
	Exec( QStringLiteral( "guest-file-close" ), close_args, error );
	return error == nullptr || error->isEmpty();
}

bool AQ_Guest_Agent::Copy_From_Guest( const QString &guest_path, const QString &host_path, QString *error )
{
	QJsonObject open_args;
	open_args.insert( QStringLiteral( "path" ), guest_path );
	open_args.insert( QStringLiteral( "mode" ), QStringLiteral( "rb" ) );
	const QJsonValue handle_v = Exec( QStringLiteral( "guest-file-open" ), open_args, error );
	if( error && ! error->isEmpty() )
		return false;
	const int handle = handle_v.toInt( -1 );
	if( handle < 0 )
	{
		if( error )
			*error = QStringLiteral( "The guest agent did not open %1" ).arg( guest_path );
		return false;
	}
	QTemporaryFile part( QFileInfo( host_path ).dir().filePath( QStringLiteral( ".aqemu-part-XXXXXX" ) ) );
	part.setAutoRemove( false );
	if( ! part.open() )
	{
		Close_Handle( handle );
		if( error )
			*error = QStringLiteral( "Cannot write %1" ).arg( host_path );
		return false;
	}
	const QString part_path = part.fileName();
	bool eof = false;
	qint64 total = 0;
	while( ! eof )
	{
		QJsonObject read_args;
		read_args.insert( QStringLiteral( "handle" ), handle );
		read_args.insert( QStringLiteral( "count" ), kChunk );
		const QJsonValue got = Exec( QStringLiteral( "guest-file-read" ), read_args, error );
		if( error && ! error->isEmpty() )
		{
			const QString read_error = *error;
			part.close();
			QFile::remove( part_path );
			Close_Handle( handle );
			*error = read_error;
			return false;
		}
		const QJsonObject obj = got.toObject();
		const QByteArray chunk = QByteArray::fromBase64( obj.value( QStringLiteral( "buf-b64" ) ).toString().toLatin1() );
		if( ! chunk.isEmpty() && part.write( chunk ) != chunk.size() )
		{
			part.close();
			QFile::remove( part_path );
			Close_Handle( handle );
			if( error )
				*error = QStringLiteral( "Could not finish writing %1" ).arg( host_path );
			return false;
		}
		total += chunk.size();
		if( total > qint64( 512 ) * 1024 * 1024 )
		{
			part.close();
			QFile::remove( part_path );
			Close_Handle( handle );
			if( error )
				*error = QStringLiteral( "That guest file is larger than 512 MB." );
			return false;
		}
		eof = obj.value( QStringLiteral( "eof" ) ).toBool() || chunk.isEmpty();
	}
	part.close();
	QJsonObject close_args;
	close_args.insert( QStringLiteral( "handle" ), handle );
	Exec( QStringLiteral( "guest-file-close" ), close_args, error );
	if( error && ! error->isEmpty() )
	{
		QFile::remove( part_path );
		return false;
	}
	if( QFileInfo::exists( host_path ) && ! QFile::remove( host_path ) )
	{
		QFile::remove( part_path );
		if( error )
			*error = QStringLiteral( "Cannot write %1" ).arg( host_path );
		return false;
	}
	if( ! QFile::rename( part_path, host_path ) )
	{
		QFile::remove( part_path );
		if( error )
			*error = QStringLiteral( "Could not finish writing %1" ).arg( host_path );
		return false;
	}
	return true;
}

bool AQ_Machine_Supports_Guest_Channels( const QString &machine )
{
	const QString m = machine.trimmed().toLower();
	if( m.isEmpty() || m == QLatin1String( "none" ) || m.startsWith( QLatin1String( "none-" ) ) )
		return false;
	if( m.contains( QLatin1String( "next" ) ) || m.contains( QLatin1String( "isapc" ) )
		|| m.contains( QLatin1String( "microvm" ) ) || m.contains( QLatin1String( "raspi" ) )
		|| m.startsWith( QLatin1String( "s390" ) ) )
		return false;
	if( m == QLatin1String( "virt" ) || m.startsWith( QLatin1String( "virt-" ) )
		|| m == QLatin1String( "pc" ) || m.startsWith( QLatin1String( "pc-" ) )
		|| m == QLatin1String( "q35" ) || m.startsWith( QLatin1String( "q35" ) )
		|| m.startsWith( QLatin1String( "pseries" ) )
		|| m == QLatin1String( "mac99" ) || m.startsWith( QLatin1String( "g3beige" ) )
		|| m == QLatin1String( "pegasos2" ) || m.startsWith( QLatin1String( "ppce500" ) )
		|| m == QLatin1String( "mpc8544ds" ) || m == QLatin1String( "sam460ex" )
		|| m == QLatin1String( "40p" ) || m == QLatin1String( "fuloong2e" )
		|| m == QLatin1String( "fulong2e" ) )
		return true;
	return false;
}

void AQ_Append_Guest_Channels( QStringList &args, const QString &agent_path, const QString &files_path )
{
	if( agent_path.isEmpty() || files_path.isEmpty() || agent_path == files_path )
		return;

	args << QStringLiteral( "-chardev" )
	     << QStringLiteral( "socket,id=aqga,path=%1,server=on,wait=off" ).arg( agent_path );
	args << QStringLiteral( "-chardev" )
	     << QStringLiteral( "socket,id=aqfiles,path=%1,server=on,wait=off" ).arg( files_path );
	args << QStringLiteral( "-device" ) << QStringLiteral( "virtio-serial-pci,id=aqemu_vserial" );
	args << QStringLiteral( "-device" )
	     << QStringLiteral( "virtserialport,bus=aqemu_vserial.0,nr=1,chardev=aqga,name=org.qemu.guest_agent.0" );
	args << QStringLiteral( "-device" )
	     << QStringLiteral( "virtserialport,bus=aqemu_vserial.0,nr=2,chardev=aqfiles,name=org.aqemu.files" );
}
