#include "Apple_SoC_Support.h"
#include "VM.h"
#include "Utils.h"
#include "WSL_Launch.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QObject>

bool AQ_Is_Apple_SoC_VM( const Virtual_Machine *vm )
{
	if( ! vm )
		return false;
	if( vm->Use_Apple_SoC_Profile() )
		return true;
	return vm->Get_Computer_Type().contains( QLatin1String( "applesoc" ), Qt::CaseInsensitive ) ||
	       vm->Get_Machine_Type().contains( QLatin1String( "t8030" ), Qt::CaseInsensitive ) ||
	       vm->Get_Machine_Type().contains( QLatin1String( "s8000" ), Qt::CaseInsensitive );
}

QString AQ_Apple_SoC_Default_Append()
{
	return QStringLiteral(
		"tlto_us=-1 mtxspin=-1 agm-genuine=1 agm-authentic=1 agm-trusted=1 "
		"serial=3 wdt=-1 -vm_compressor_wk_sw" );
}

QString AQ_Apple_SoC_WSL_Qemu_Binary()
{
	QSettings s;
	const QString override = s.value( QStringLiteral( "WSL_Launch/AppleSoC_Binary" ), QString() ).toString().trimmed();
	if( ! override.isEmpty() )
		return override;
	return QStringLiteral( "/usr/local/bin/qemu-system-applesoc" );
}

static bool Write_Sparse_Raw( const QString &path, qint64 size_bytes, QString *error_out )
{
	if( QFile::exists( path ) )
		return true;
	QFile f( path );
	if( ! f.open( QIODevice::WriteOnly ) )
	{
		if( error_out )
			*error_out = QObject::tr( "Cannot create %1" ).arg( path );
		return false;
	}
	if( ! f.resize( size_bytes ) )
	{
		if( error_out )
			*error_out = QObject::tr( "Cannot size %1" ).arg( path );
		f.close();
		return false;
	}
	f.close();
	return true;
}

bool AQ_Ensure_Apple_SoC_Disk_Images( const QString &vm_dir, QString *error_out )
{
	if( ! QDir().mkpath( vm_dir ) )
	{
		if( error_out )
			*error_out = QObject::tr( "Cannot create Apple SoC VM directory:\n%1" ).arg( vm_dir );
		return false;
	}
	struct Spec { const char *name; qint64 bytes; };
	// Sizes mirror common Inferno recipes (small SEP/NVRAM + larger root/firmware).
	const Spec specs[] = {
		{ "sep_nvram", 64 * 1024 },
		{ "sep_ssc", 64 * 1024 },
		{ "root", 32LL * 1024 * 1024 * 1024 },
		{ "firmware", 1LL * 1024 * 1024 * 1024 },
		{ "syscfg", 1 * 1024 * 1024 },
		{ "ctrl_bits", 1 * 1024 * 1024 },
		{ "nvram", 1 * 1024 * 1024 },
		{ "effaceable", 1 * 1024 * 1024 },
		{ "panic_log", 16 * 1024 * 1024 },
	};
	for( const Spec &s : specs )
	{
		const QString path = QDir( vm_dir ).filePath( QString::fromLatin1( s.name ) );
		if( ! Write_Sparse_Raw( path, s.bytes, error_out ) )
			return false;
	}
	return true;
}

static QString Quote_Drive( const QString &path, bool script )
{
	const QString n = AQ_Normalize_File_Path( path );
	if( script )
		return QStringLiteral( "\"%1\"" ).arg( n );
	return n;
}

QStringList AQ_Build_Apple_SoC_Extra_Args( const Virtual_Machine *vm,
                                           bool for_script_mode,
                                           bool via_wsl,
                                           bool create_missing_images,
                                           QString *error_out )
{
	QStringList out;
	if( ! vm || ! AQ_Is_Apple_SoC_VM( vm ) )
		return out;

	QString vm_dir = QFileInfo( vm->Get_VM_XML_File_Path() ).absolutePath();
	if( vm_dir.isEmpty() )
		vm_dir = QDir::currentPath();

	if( create_missing_images )
	{
		QString err;
		if( ! AQ_Ensure_Apple_SoC_Disk_Images( vm_dir, &err ) )
		{
			if( error_out )
				*error_out = err;
			AQWarning( "AQ_Build_Apple_SoC_Extra_Args", err );
			return QStringList();
		}
	}

	auto p = [&]( const QString &name ) -> QString {
		const QString override = vm->Get_Apple_SoC_Image_Path( name );
		if( ! override.trimmed().isEmpty() )
			return AQ_Normalize_File_Path( override );
		return QDir( vm_dir ).filePath( name );
	};

	// SEP pflash pair
	out << QStringLiteral( "-drive" )
	    << QStringLiteral( "file=%1,if=pflash,format=raw" )
	           .arg( Quote_Drive( p( QStringLiteral( "sep_nvram" ) ), for_script_mode ) );
	out << QStringLiteral( "-drive" )
	    << QStringLiteral( "file=%1,if=pflash,format=raw" )
	           .arg( Quote_Drive( p( QStringLiteral( "sep_ssc" ) ), for_script_mode ) );

	struct Ns { const char *file; int nsid; int nstype; bool apple_nvram; };
	const Ns namespaces[] = {
		{ "root", 1, 1, false },
		{ "firmware", 2, 2, false },
		{ "syscfg", 3, 3, false },
		{ "ctrl_bits", 4, 4, false },
		{ "nvram", 5, 5, true },
		{ "effaceable", 6, 6, false },
		{ "panic_log", 7, 8, false },
	};

	for( const Ns &ns : namespaces )
	{
		const QString id = QString::fromLatin1( ns.file );
		const QString file = Quote_Drive( p( id ), for_script_mode );
		out << QStringLiteral( "-drive" )
		    << QStringLiteral( "file=%1,format=raw,if=none,id=%2" ).arg( file, id );
		if( ns.apple_nvram )
		{
			out << QStringLiteral( "-device" )
			    << QStringLiteral(
				       "apple-nvram,drive=%1,bus=nvme-bus.0,nsid=%2,nstype=%3,id=nvram,"
				       "logical_block_size=4096,physical_block_size=4096" )
			           .arg( id ).arg( ns.nsid ).arg( ns.nstype );
		}
		else
		{
			out << QStringLiteral( "-device" )
			    << QStringLiteral(
				       "nvme-ns,drive=%1,bus=nvme-bus.0,nsid=%2,nstype=%3,"
				       "logical_block_size=4096,physical_block_size=4096" )
			           .arg( id ).arg( ns.nsid ).arg( ns.nstype );
		}
	}

	Q_UNUSED( via_wsl );
	return out;
}

QString AQ_Build_Apple_SoC_Machine_Props( const Virtual_Machine *vm, bool via_wsl )
{
	if( ! vm )
		return QString();

	QString machine = vm->Get_Machine_Type().trimmed();
	if( machine.isEmpty() )
		machine = QStringLiteral( "t8030" );

	QStringList props;
	props << machine;

	auto add_path_prop = [&]( const char *key, const QString &path ) {
		const QString n = AQ_Normalize_File_Path( path );
		if( ! n.isEmpty() )
			props << QStringLiteral( "%1=%2" ).arg( QString::fromLatin1( key ), n );
	};

	add_path_prop( "trustcache", vm->Get_Apple_Trustcache_Path() );
	add_path_prop( "ticket", vm->Get_Apple_Ticket_Path() );
	add_path_prop( "sep-fw", vm->Get_Apple_SEP_FW_Path() );
	add_path_prop( "sep-rom", vm->Get_Apple_SEP_ROM_Path() );
	props << QStringLiteral( "kaslr-off=true" );

	// USB remote for companion / idevicerestore. Prefer TCP on Windows host native;
	// unix socket when launching under WSL (Linux Inferno).
	QString conn = vm->Get_Apple_USB_Conn_Type().trimmed().toLower();
	if( conn.isEmpty() )
	{
#ifdef Q_OS_WIN32
		conn = via_wsl ? QStringLiteral( "unix" ) : QStringLiteral( "ipv4" );
#else
		conn = QStringLiteral( "unix" );
#endif
	}
	props << QStringLiteral( "usb-conn-type=%1" ).arg( conn );
	if( conn == QLatin1String( "unix" ) )
	{
		QString addr = vm->Get_Apple_USB_Conn_Addr().trimmed();
		if( addr.isEmpty() )
			addr = QStringLiteral( "/tmp/InfernoUSBRemote" );
		props << QStringLiteral( "usb-conn-addr=%1" ).arg( addr );
	}
	else
	{
		QString addr = vm->Get_Apple_USB_Conn_Addr().trimmed();
		if( addr.isEmpty() )
			addr = QStringLiteral( "127.0.0.1" );
		props << QStringLiteral( "usb-conn-addr=%1" ).arg( addr );
		int port = vm->Get_Apple_USB_Conn_Port();
		if( port <= 0 )
			port = 8030;
		props << QStringLiteral( "usb-conn-port=%1" ).arg( port );
	}

	return props.join( QLatin1Char( ',' ) );
}
