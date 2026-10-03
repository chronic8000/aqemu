/****************************************************************************
**
** Copyright (C) 2026 Chronic Engineering
**
** This file is part of AQEMU.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor,
** Boston, MA  02110-1301, USA.
**
****************************************************************************/

#include "Physical_Drive.h"

#include "Storage_Recovery.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>

#ifndef Q_OS_WIN
#include <unistd.h>
#endif

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winioctl.h>
#include <shellapi.h>
#endif

namespace {

struct Block_Node
{
	QString path;
	QString caption;
	bool whole = false;
	qint64 bytes = -1;
};

QString Format_Bytes( qint64 bytes )
{
	if( bytes < 0 )
		return QString();
	const double tb = 1000.0 * 1000.0 * 1000.0 * 1000.0;
	const double gb = 1000.0 * 1000.0 * 1000.0;
	const double mb = 1000.0 * 1000.0;
	if( bytes >= tb )
		return QString::number( bytes / tb, 'f', 2 ) + QLatin1String( " TB" );
	if( bytes >= gb )
		return QString::number( bytes / gb, 'f', 1 ) + QLatin1String( " GB" );
	if( bytes >= mb )
		return QString::number( bytes / mb, 'f', 0 ) + QLatin1String( " MB" );
	return QString::number( bytes ) + QLatin1String( " B" );
}

QString Join_Caption( const QString &path, const QString &model, qint64 bytes )
{
	QString text = path;
	const QString size = Format_Bytes( bytes );
	if( ! size.isEmpty() )
		text += QLatin1String( "  —  " ) + size;
	const QString trimmed = model.trimmed();
	if( ! trimmed.isEmpty() )
		text += QLatin1String( "  —  " ) + trimmed;
	return text;
}

#ifndef Q_OS_WIN
bool Name_Token_Ok( const QString &name )
{
	if( name.isEmpty() )
		return false;
	for( int i = 0; i < name.size(); ++i )
	{
		const QChar c = name.at( i );
		if( c.isLetterOrNumber() || c == QLatin1Char( '_' ) || c == QLatin1Char( '-' ) || c == QLatin1Char( '.' ) )
			continue;
		return false;
	}
	return true;
}
#endif

#ifdef Q_OS_WIN
bool Windows_Is_Elevated()
{
	HANDLE token = NULL;
	if( ! OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token ) )
		return false;
	TOKEN_ELEVATION elev;
	DWORD size = 0;
	const BOOL ok = GetTokenInformation( token, TokenElevation, &elev, sizeof( elev ), &size );
	CloseHandle( token );
	return ok && elev.TokenIsElevated;
}

QString Windows_Model( HANDLE handle )
{
	STORAGE_PROPERTY_QUERY query;
	ZeroMemory( &query, sizeof( query ) );
	query.PropertyId = StorageDeviceProperty;
	query.QueryType = PropertyStandardQuery;
	char buffer[1024];
	ZeroMemory( buffer, sizeof( buffer ) );
	DWORD bytes = 0;
	if( ! DeviceIoControl( handle, IOCTL_STORAGE_QUERY_PROPERTY,
			&query, sizeof( query ), buffer, sizeof( buffer ), &bytes, NULL ) )
		return QString();
	const STORAGE_DEVICE_DESCRIPTOR *desc =
		reinterpret_cast<const STORAGE_DEVICE_DESCRIPTOR *>( buffer );
	QString vendor;
	QString product;
	if( desc->VendorIdOffset > 0 && desc->VendorIdOffset < (DWORD)sizeof( buffer ) )
		vendor = QString::fromLatin1( buffer + desc->VendorIdOffset ).trimmed();
	if( desc->ProductIdOffset > 0 && desc->ProductIdOffset < (DWORD)sizeof( buffer ) )
		product = QString::fromLatin1( buffer + desc->ProductIdOffset ).trimmed();
	if( vendor.isEmpty() )
		return product;
	if( product.isEmpty() )
		return vendor;
	return vendor + QLatin1Char( ' ' ) + product;
}

qint64 Windows_Length( HANDLE handle )
{
	GET_LENGTH_INFORMATION info;
	ZeroMemory( &info, sizeof( info ) );
	DWORD bytes = 0;
	if( ! DeviceIoControl( handle, IOCTL_DISK_GET_LENGTH_INFO,
			NULL, 0, &info, sizeof( info ), &bytes, NULL ) )
		return -1;
	return info.Length.QuadPart;
}

QString Quote_Windows_Arg( const QString &arg )
{
	if( ! arg.contains( QLatin1Char( ' ' ) ) && ! arg.contains( QLatin1Char( '\t' ) )
		&& ! arg.contains( QLatin1Char( '"' ) ) )
		return arg;
	QString out = QStringLiteral( "\"" );
	for( int i = 0; i < arg.size(); ++i )
	{
		if( arg.at( i ) == QLatin1Char( '"' ) )
			out += QLatin1String( "\\\"" );
		else
			out += arg.at( i );
	}
	out += QLatin1Char( '"' );
	return out;
}

bool Windows_Restart_Elevated( QWidget *parent )
{
	const QMessageBox::StandardButton answer = QMessageBox::question( parent,
		QObject::tr( "Administrator rights" ),
		QObject::tr( "Using a physical drive needs administrator rights.\n\n"
			"Windows will ask for permission, then AQEMU will restart. "
			"Running virtual machines will be stopped. Choose the drive again after AQEMU opens." ),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
	if( answer != QMessageBox::Yes )
		return false;

	QStringList args = QCoreApplication::arguments();
	if( ! args.isEmpty() )
		args.removeFirst();
	if( ! args.contains( QLatin1String( "--aqemu-elevated" ) ) )
		args << QLatin1String( "--aqemu-elevated" );
	QStringList quoted;
	for( int i = 0; i < args.size(); ++i )
		quoted << Quote_Windows_Arg( args.at( i ) );

	const QString file = QCoreApplication::applicationFilePath();
	const QString dir = QCoreApplication::applicationDirPath();
	const QString params = quoted.join( QLatin1Char( ' ' ) );

	SHELLEXECUTEINFOW info;
	ZeroMemory( &info, sizeof( info ) );
	info.cbSize = sizeof( info );
	info.fMask = SEE_MASK_NOCLOSEPROCESS;
	info.hwnd = parent ? reinterpret_cast<HWND>( parent->winId() ) : NULL;
	info.lpVerb = L"runas";
	info.lpFile = reinterpret_cast<LPCWSTR>( file.utf16() );
	info.lpParameters = params.isEmpty() ? NULL : reinterpret_cast<LPCWSTR>( params.utf16() );
	info.lpDirectory = reinterpret_cast<LPCWSTR>( dir.utf16() );
	info.nShow = SW_SHOWNORMAL;
	if( ! ShellExecuteExW( &info ) )
	{
		if( GetLastError() != ERROR_CANCELLED )
		{
			QMessageBox::warning( parent, QObject::tr( "Administrator rights" ),
				QObject::tr( "Windows did not start AQEMU with administrator rights." ) );
		}
		return false;
	}
	if( info.hProcess )
		CloseHandle( info.hProcess );
	QTimer::singleShot( 0, qApp, SLOT(quit()) );
	return true;
}
#endif

#ifndef Q_OS_WIN
bool Linux_Skip_Disk( const QString &name )
{
	return name.startsWith( QLatin1String( "loop" ) )
		|| name.startsWith( QLatin1String( "ram" ) )
		|| name.startsWith( QLatin1String( "dm-" ) )
		|| name.startsWith( QLatin1String( "md" ) )
		|| name.startsWith( QLatin1String( "sr" ) )
		|| name.startsWith( QLatin1String( "fd" ) )
		|| name.startsWith( QLatin1String( "zram" ) )
		|| name.startsWith( QLatin1String( "nbd" ) );
}

qint64 Linux_Bytes( const QString &sysfs )
{
	QFile file( sysfs + QStringLiteral( "/size" ) );
	if( ! file.open( QIODevice::ReadOnly ) )
		return -1;
	bool ok = false;
	const qint64 sectors = QString::fromLatin1( file.readAll().trimmed() ).toLongLong( &ok );
	if( ! ok || sectors < 0 )
		return -1;
	return sectors * 512;
}

QString Linux_Model( const QString &disk_name )
{
	QFile model( QStringLiteral( "/sys/block/" ) + disk_name + QStringLiteral( "/device/model" ) );
	QString text;
	if( model.open( QIODevice::ReadOnly ) )
		text = QString::fromLocal8Bit( model.readAll() ).trimmed();
	QFile vendor( QStringLiteral( "/sys/block/" ) + disk_name + QStringLiteral( "/device/vendor" ) );
	if( vendor.open( QIODevice::ReadOnly ) )
	{
		const QString v = QString::fromLocal8Bit( vendor.readAll() ).trimmed();
		if( ! v.isEmpty() )
			text = text.isEmpty() ? v : v + QLatin1Char( ' ' ) + text;
	}
	return text;
}

bool Linux_Device_Token( const QString &path )
{
	if( ! path.startsWith( QLatin1String( "/dev/" ) ) )
		return false;
	const QString name = path.mid( 5 );
	if( name.contains( QLatin1Char( '/' ) ) || ! Name_Token_Ok( name ) )
		return false;
	return QFileInfo( path ).exists();
}

QString Linux_User_Name()
{
	QString user = QString::fromLocal8Bit( qgetenv( "USER" ) );
	if( user.isEmpty() )
		user = QString::fromLocal8Bit( qgetenv( "LOGNAME" ) );
	if( ! Name_Token_Ok( user ) )
		return QString();
	return user;
}

bool Linux_Sudo_Passwordless()
{
	QProcess probe;
	probe.start( QStringLiteral( "sudo" ), QStringList() << QStringLiteral( "-n" ) << QStringLiteral( "true" ) );
	if( ! probe.waitForFinished( 3000 ) )
		return false;
	return probe.exitCode() == 0;
}

int Linux_Run_Sudo( const QString &password, const QStringList &args, QString *error_text )
{
	QProcess proc;
	QStringList full;
	full << QStringLiteral( "-S" ) << QStringLiteral( "-p" ) << QString() << QStringLiteral( "-k" );
	full << args;
	proc.start( QStringLiteral( "sudo" ), full );
	if( ! proc.waitForStarted( 5000 ) )
	{
		if( error_text )
			*error_text = QObject::tr( "sudo did not start." );
		return -1;
	}
	QByteArray secret = password.toUtf8();
	secret.append( '\n' );
	proc.write( secret );
	secret.fill( ' ' );
	proc.closeWriteChannel();
	proc.waitForFinished( 20000 );
	if( error_text )
		*error_text = QString::fromLocal8Bit( proc.readAllStandardError() );
	return proc.exitCode();
}

bool Linux_Grant( QWidget *parent, const QStringList &paths )
{
	if( paths.isEmpty() )
		return true;
	if( QStandardPaths::findExecutable( QStringLiteral( "sudo" ) ).isEmpty() )
	{
		QMessageBox::warning( parent, QObject::tr( "Administrator rights" ),
			QObject::tr( "sudo was not found. Run AQEMU as root to use a physical drive." ) );
		return false;
	}
	const QString user = Linux_User_Name();
	if( user.isEmpty() )
	{
		QMessageBox::warning( parent, QObject::tr( "Administrator rights" ),
			QObject::tr( "AQEMU could not determine the current user name." ) );
		return false;
	}

	QStringList safe;
	for( int i = 0; i < paths.size(); ++i )
	{
		if( Linux_Device_Token( paths.at( i ) ) )
			safe << paths.at( i );
	}
	if( safe.isEmpty() )
		return false;

	const bool passwordless = Linux_Sudo_Passwordless();
	QString password;
	if( ! passwordless )
	{
		QDialog ask( parent );
		ask.setWindowTitle( QObject::tr( "Administrator rights" ) );
		QVBoxLayout *lay = new QVBoxLayout( &ask );
		lay->addWidget( new QLabel( QObject::tr(
			"Using a physical drive needs administrator rights.\n\n"
			"Enter your password so AQEMU can open the drive. "
			"It is sent only to sudo and is not saved." ) ) );
		QLineEdit *edit = new QLineEdit();
		edit->setEchoMode( QLineEdit::Password );
		lay->addWidget( edit );
		QDialogButtonBox *box = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
		lay->addWidget( box );
		QObject::connect( box, SIGNAL(accepted()), &ask, SLOT(accept()) );
		QObject::connect( box, SIGNAL(rejected()), &ask, SLOT(reject()) );
		if( ask.exec() != QDialog::Accepted )
			return false;
		password = edit->text();
		edit->clear();
	}
	else
	{
		const QMessageBox::StandardButton answer = QMessageBox::question( parent,
			QObject::tr( "Administrator rights" ),
			QObject::tr( "Using a physical drive needs administrator rights. AQEMU will use sudo to open it." ),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
		if( answer != QMessageBox::Yes )
			return false;
	}

	QStringList quoted;
	for( int i = 0; i < safe.size(); ++i )
		quoted << QStringLiteral( "'" ) + safe.at( i ) + QStringLiteral( "'" );
	const QString joined = quoted.join( QLatin1Char( ' ' ) );
	const QString script = QStringLiteral( "setfacl -m u:%1:rw -- %2 || chmod a+rw -- %2" )
		.arg( user, joined );
	QString error_text;
	int code = 0;
	if( passwordless )
	{
		QProcess proc;
		proc.start( QStringLiteral( "sudo" ),
			QStringList() << QStringLiteral( "-n" ) << QStringLiteral( "sh" )
				<< QStringLiteral( "-c" ) << script );
		proc.waitForFinished( 20000 );
		code = proc.exitCode();
		error_text = QString::fromLocal8Bit( proc.readAllStandardError() );
	}
	else
	{
		code = Linux_Run_Sudo( password, QStringList() << QStringLiteral( "sh" )
			<< QStringLiteral( "-c" ) << script, &error_text );
	}
	password.fill( QLatin1Char( ' ' ) );
	password.clear();
	if( code != 0 )
	{
		QString detail = QObject::tr( "The password was not accepted, or sudo could not open the drive." );
		if( error_text.contains( QLatin1String( "Sorry" ), Qt::CaseInsensitive ) )
			detail = QObject::tr( "The password was not accepted." );
		QMessageBox::warning( parent, QObject::tr( "Administrator rights" ), detail );
		return false;
	}
	return true;
}
#endif

bool Node_Openable( const QString &path )
{
	QFile file( path );
	if( ! file.open( QIODevice::ReadOnly ) )
		return false;
	file.close();
	return true;
}

bool Skip_Because_Boot( const QString &path )
{
	return Path_Is_Host_Boot_Disk( path );
}

#ifdef Q_OS_WIN
void Windows_Append_Partitions( QList<Block_Node> *nodes, int disk, HANDLE handle, const QString &model )
{
	QByteArray buffer( 8192, 0 );
	DWORD bytes = 0;
	BOOL ok = FALSE;
	for( int attempt = 0; attempt < 4; ++attempt )
	{
		ok = DeviceIoControl( handle, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
			NULL, 0, buffer.data(), (DWORD)buffer.size(), &bytes, NULL );
		if( ok )
			break;
		if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
			return;
		buffer.resize( buffer.size() * 2 );
	}
	if( ! ok || buffer.size() < (int)sizeof( DRIVE_LAYOUT_INFORMATION_EX ) )
		return;
	const DRIVE_LAYOUT_INFORMATION_EX *layout =
		reinterpret_cast<const DRIVE_LAYOUT_INFORMATION_EX *>( buffer.constData() );
	const int count = (int)layout->PartitionCount;
	const char *base = buffer.constData();
	const int entry = (int)sizeof( PARTITION_INFORMATION_EX );
	const int first = (int)offsetof( DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry );
	for( int i = 0; i < count; ++i )
	{
		const int at = first + i * entry;
		if( at + entry > buffer.size() )
			break;
		const PARTITION_INFORMATION_EX *part =
			reinterpret_cast<const PARTITION_INFORMATION_EX *>( base + at );
		if( part->PartitionNumber == 0 || part->PartitionLength.QuadPart <= 0 )
			continue;
#if defined(NONAMELESSUNION)
		const PARTITION_INFORMATION_MBR &mbr = part->DUMMYUNIONNAME.Mbr;
		const PARTITION_INFORMATION_GPT &gpt = part->DUMMYUNIONNAME.Gpt;
#else
		const PARTITION_INFORMATION_MBR &mbr = part->Mbr;
		const PARTITION_INFORMATION_GPT &gpt = part->Gpt;
#endif
		if( part->PartitionStyle == PARTITION_STYLE_MBR )
		{
			const BYTE kind = mbr.PartitionType;
			if( kind == 0 || kind == 0x05 || kind == 0x0F )
				continue;
		}
		Block_Node node;
		node.whole = false;
		node.bytes = part->PartitionLength.QuadPart;
		node.path = QStringLiteral( "\\\\.\\Harddisk%1Partition%2" )
			.arg( disk )
			.arg( part->PartitionNumber );
		QString label = model;
		if( part->PartitionStyle == PARTITION_STYLE_GPT && gpt.Name[0] != 0 )
			label = QString::fromWCharArray( gpt.Name );
		if( Skip_Because_Boot( node.path ) )
			continue;
		node.caption = Join_Caption( node.path, label, node.bytes );
		nodes->append( node );
	}
}
#endif

QList<Block_Node> Enumerate_Nodes()
{
	QList<Block_Node> nodes;
#ifdef Q_OS_WIN
	for( int disk = 0; disk < 32; ++disk )
	{
		const QString path = QStringLiteral( "\\\\.\\PhysicalDrive%1" ).arg( disk );
		HANDLE handle = CreateFileW( reinterpret_cast<LPCWSTR>( path.utf16() ),
			GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL );
		if( handle == INVALID_HANDLE_VALUE )
			continue;
		const QString model = Windows_Model( handle );
		const qint64 bytes = Windows_Length( handle );
		if( ! Skip_Because_Boot( path ) )
		{
			Block_Node whole;
			whole.whole = true;
			whole.path = path;
			whole.bytes = bytes;
			whole.caption = Join_Caption( path, model, bytes );
			nodes.append( whole );
		}
		Windows_Append_Partitions( &nodes, disk, handle, model );
		CloseHandle( handle );
	}
#else
	QDir block( QStringLiteral( "/sys/block" ) );
	const QStringList names = block.entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );
	for( int i = 0; i < names.size(); ++i )
	{
		const QString name = names.at( i );
		if( Linux_Skip_Disk( name ) )
			continue;
		const QString path = QStringLiteral( "/dev/" ) + name;
		if( ! QFileInfo( path ).exists() )
			continue;
		const QString model = Linux_Model( name );
		const qint64 bytes = Linux_Bytes( QStringLiteral( "/sys/block/" ) + name );
		if( ! Skip_Because_Boot( path ) )
		{
			Block_Node whole;
			whole.whole = true;
			whole.path = path;
			whole.bytes = bytes;
			whole.caption = Join_Caption( path, model, bytes );
			nodes.append( whole );
		}
		QDir parts( QStringLiteral( "/sys/block/" ) + name );
		const QStringList children = parts.entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );
		for( int c = 0; c < children.size(); ++c )
		{
			const QString child = children.at( c );
			if( ! child.startsWith( name ) )
				continue;
			if( ! QFileInfo( parts.filePath( child ) + QStringLiteral( "/partition" ) ).exists() )
				continue;
			const QString part_path = QStringLiteral( "/dev/" ) + child;
			if( ! QFileInfo( part_path ).exists() || Skip_Because_Boot( part_path ) )
				continue;
			Block_Node part;
			part.whole = false;
			part.path = part_path;
			part.bytes = Linux_Bytes( QStringLiteral( "/sys/block/" ) + name + QLatin1Char( '/' ) + child );
			part.caption = Join_Caption( part_path, model, part.bytes );
			nodes.append( part );
		}
	}
#endif
	return nodes;
}

QStringList Paths_Needing_Access( const QList<Block_Node> &nodes )
{
	QStringList need;
	for( int i = 0; i < nodes.size(); ++i )
	{
		if( ! Node_Openable( nodes.at( i ).path ) )
			need << nodes.at( i ).path;
	}
	return need;
}

class Drive_Picker : public QDialog
{
public:
	Drive_Picker( QWidget *parent, bool allow_many, const QList<Block_Node> &nodes )
		: QDialog( parent ), many( allow_many ), all( nodes )
	{
		setWindowTitle( tr( "Physical drive" ) );
		resize( 560, 420 );
		QVBoxLayout *lay = new QVBoxLayout( this );
		pages = new QStackedWidget();
		lay->addWidget( pages, 1 );

		QWidget *step = new QWidget();
		QVBoxLayout *step_lay = new QVBoxLayout( step );
		step_lay->addWidget( new QLabel( tr( "Choose what to give the virtual machine." ) ) );
		whole_radio = new QRadioButton( tr( "Use the entire physical drive" ) );
		part_radio = new QRadioButton( many
			? tr( "Use partitions" )
			: tr( "Use one partition" ) );
		whole_radio->setChecked( true );
		step_lay->addWidget( whole_radio );
		step_lay->addWidget( part_radio );
		step_lay->addWidget( new QLabel( tr(
			"The computer's own startup disk is not listed. "
			"The guest can read and write the drive you add, so the host must not use it at the same time." ) ) );
		step_lay->addStretch( 1 );
		pages->addWidget( step );

		QWidget *list_page = new QWidget();
		QVBoxLayout *list_lay = new QVBoxLayout( list_page );
		list_note = new QLabel();
		list_note->setWordWrap( true );
		list_lay->addWidget( list_note );
		list = new QListWidget();
		list->setSelectionMode( many
			? QAbstractItemView::ExtendedSelection
			: QAbstractItemView::SingleSelection );
		list_lay->addWidget( list, 1 );
		pages->addWidget( list_page );

		QHBoxLayout *row = new QHBoxLayout();
		back_button = new QPushButton( tr( "Back" ) );
		next_button = new QPushButton( tr( "Next" ) );
		add_button = new QPushButton( tr( "Add" ) );
		QPushButton *cancel = new QPushButton( tr( "Cancel" ) );
		back_button->setVisible( false );
		add_button->setVisible( false );
		row->addStretch( 1 );
		row->addWidget( back_button );
		row->addWidget( next_button );
		row->addWidget( add_button );
		row->addWidget( cancel );
		lay->addLayout( row );
		connect( cancel, &QPushButton::clicked, this, &QDialog::reject );
		connect( next_button, &QPushButton::clicked, this, [this]() { Show_List(); } );
		connect( back_button, &QPushButton::clicked, this, [this]() { Go_Back(); } );
		connect( add_button, &QPushButton::clicked, this, [this]() { Take_Selection(); } );
	}

	QStringList chosen;

private:
	void Show_List()
	{
		list->clear();
		const bool want_whole = whole_radio->isChecked();
		for( int i = 0; i < all.size(); ++i )
		{
			if( all.at( i ).whole != want_whole )
				continue;
			QListWidgetItem *item = new QListWidgetItem( all.at( i ).caption, list );
			item->setData( Qt::UserRole, all.at( i ).path );
		}
		list_note->setText( list->count() == 0
			? tr( "No drives were found. The host startup disk is not listed." )
			: tr( "Select a drive to add to AQEMU." ) );
		back_button->setVisible( true );
		next_button->setVisible( false );
		add_button->setVisible( true );
		pages->setCurrentIndex( 1 );
	}

	void Go_Back()
	{
		back_button->setVisible( false );
		next_button->setVisible( true );
		add_button->setVisible( false );
		pages->setCurrentIndex( 0 );
	}

	void Take_Selection()
	{
		const QList<QListWidgetItem *> items = list->selectedItems();
		chosen.clear();
		for( int i = 0; i < items.size(); ++i )
			chosen << items.at( i )->data( Qt::UserRole ).toString();
		if( chosen.isEmpty() )
		{
			QMessageBox::information( this, tr( "Physical drive" ),
				tr( "Select a drive first." ) );
			return;
		}
		accept();
	}

private:
	bool many;
	QList<Block_Node> all;
	QStackedWidget *pages;
	QRadioButton *whole_radio;
	QRadioButton *part_radio;
	QLabel *list_note;
	QListWidget *list;
	QPushButton *back_button;
	QPushButton *next_button;
	QPushButton *add_button;
};

}

bool AQ_Is_Physical_Disk_Path( const QString &path )
{
	if( path.startsWith( QLatin1String( "\\\\.\\PhysicalDrive" ), Qt::CaseInsensitive ) )
		return true;
	if( path.startsWith( QLatin1String( "\\\\.\\Harddisk" ), Qt::CaseInsensitive )
		&& path.contains( QLatin1String( "Partition" ), Qt::CaseInsensitive ) )
		return true;
#ifndef Q_OS_WIN
	if( ! path.startsWith( QLatin1String( "/dev/" ) ) )
		return false;
	const QString name = path.mid( 5 );
	if( name.contains( QLatin1Char( '/' ) ) || Linux_Skip_Disk( name ) )
		return false;
	if( ! Name_Token_Ok( name ) )
		return false;
	return QFileInfo( QStringLiteral( "/sys/class/block/" ) + name ).exists();
#else
	return false;
#endif
}

bool AQ_Storage_Path_Present( const QString &path )
{
	if( path.isEmpty() )
		return false;
	if( QFile::exists( path ) )
		return true;
#ifdef Q_OS_WIN
	return AQ_Is_Physical_Disk_Path( path );
#else
	return false;
#endif
}

bool AQ_Require_Physical_Disk_Privilege( QWidget *parent, const QStringList &paths )
{
#ifdef Q_OS_WIN
	if( ! Windows_Is_Elevated() )
	{
		Windows_Restart_Elevated( parent );
		return false;
	}
#endif
	if( Host_Boot_Disk_Path().isEmpty() )
	{
		QMessageBox::warning( parent, QObject::tr( "Physical drive" ),
			QObject::tr( "Physical drives were not listed because the computer's startup disk could not be identified." ) );
		return false;
	}

	for( int i = 0; i < paths.size(); ++i )
	{
		if( ! AQ_Is_Physical_Disk_Path( paths.at( i ) ) )
		{
			QMessageBox::warning( parent, QObject::tr( "Physical drive" ),
				QObject::tr( "This is not a physical drive path." ) );
			return false;
		}
		if( Path_Is_Host_Boot_Disk( paths.at( i ) ) )
		{
			QMessageBox::warning( parent, QObject::tr( "Physical drive" ),
				QObject::tr( "The computer's startup disk cannot be given to a virtual machine." ) );
			return false;
		}
	}

#ifdef Q_OS_WIN
	for( int i = 0; i < paths.size(); ++i )
	{
		if( ! Node_Openable( paths.at( i ) ) )
		{
			QMessageBox::warning( parent, QObject::tr( "Physical drive" ),
				QObject::tr( "AQEMU cannot open this drive." ) );
			return false;
		}
	}
	return true;
#else
	if( geteuid() == 0 )
	{
		for( int i = 0; i < paths.size(); ++i )
		{
			if( ! Node_Openable( paths.at( i ) ) )
			{
				QMessageBox::warning( parent, QObject::tr( "Physical drive" ),
					QObject::tr( "AQEMU cannot open this drive." ) );
				return false;
			}
		}
		return true;
	}

	QStringList need;
	if( paths.isEmpty() )
		need = Paths_Needing_Access( Enumerate_Nodes() );
	else
	{
		for( int i = 0; i < paths.size(); ++i )
		{
			if( ! Node_Openable( paths.at( i ) ) )
				need << paths.at( i );
		}
	}
	if( need.isEmpty() )
		return true;
	if( ! Linux_Grant( parent, need ) )
		return false;
	for( int i = 0; i < need.size(); ++i )
	{
		if( ! Node_Openable( need.at( i ) ) )
		{
			QMessageBox::warning( parent, QObject::tr( "Physical drive" ),
				QObject::tr( "AQEMU still cannot open this drive." ) );
			return false;
		}
	}
	return true;
#endif
}

QStringList AQ_Pick_Physical_Drives( QWidget *parent, bool allow_many )
{
	if( ! AQ_Require_Physical_Disk_Privilege( parent, QStringList() ) )
		return QStringList();
	const QList<Block_Node> nodes = Enumerate_Nodes();
	Drive_Picker picker( parent, allow_many, nodes );
	if( picker.exec() != QDialog::Accepted )
		return QStringList();
	if( ! AQ_Require_Physical_Disk_Privilege( parent, picker.chosen ) )
		return QStringList();
	return picker.chosen;
}
