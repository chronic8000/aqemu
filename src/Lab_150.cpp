/****************************************************************************
** AQEMU 1.5.0 lab tools. Stock QEMU niches, studios, and BYO-firmware wraps.
****************************************************************************/
#ifndef Q_OS_WIN
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#include <unistd.h>
#endif
#include "Lab_150.h"

#include "VM.h"
#include "Utils.h"
#include "QMP_Client.h"
#include "QEMU_Probe_Catalog.h"
#include "WSL_Launch.h"
#include "VM_Wizard_Window.h"
#include "Service.h"
#include "Snapshots_Window.h"

#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextStream>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUuid>
#include <QDateTime>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>
#include <QHostInfo>
#include <QInputDialog>
#include <QCoreApplication>
#include <cstring>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincred.h>
#include <aclapi.h>
#endif

static QHash<const Virtual_Machine*, QList<QProcess*> > g_companions;
static bool g_note_launch = false;

static QString Lab_Blob_Of( const Virtual_Machine *vm )
{
	return vm ? vm->Get_Lab_Options() : QString();
}

static QString Lab_Escape( const QString &value )
{
	QString out;
	for( int i = 0; i < value.size(); ++i )
	{
		const QChar c = value.at( i );
		if( c == QLatin1Char( '\\' ) )
			out += QLatin1String( "\\\\" );
		else if( c == QLatin1Char( '\n' ) )
			out += QLatin1String( "\\n" );
		else
			out += c;
	}
	return out;
}

static QString Lab_Unescape( const QString &value )
{
	QString out;
	for( int i = 0; i < value.size(); ++i )
	{
		if( value.at( i ) == QLatin1Char( '\\' ) && i + 1 < value.size() )
		{
			const QChar next = value.at( i + 1 );
			if( next == QLatin1Char( 'n' ) )
			{
				out += QLatin1Char( '\n' );
				++i;
				continue;
			}
			if( next == QLatin1Char( '\\' ) )
			{
				out += QLatin1Char( '\\' );
				++i;
				continue;
			}
		}
		out += value.at( i );
	}
	return out;
}

QMap<QString, QString> AQ_Lab_Map( const QString &blob )
{
	QMap<QString, QString> opt;
	const QStringList lines = blob.split( QLatin1Char( '\n' ), QString::SkipEmptyParts );
	for( int i = 0; i < lines.count(); ++i )
	{
		const QString line = lines.at( i ).trimmed();
		const int eq = line.indexOf( QLatin1Char( '=' ) );
		if( eq <= 0 )
			continue;
		opt.insert( line.left( eq ).trimmed(), Lab_Unescape( line.mid( eq + 1 ) ) );
	}
	return opt;
}

QString AQ_Lab_Blob( const QMap<QString, QString> &opt )
{
	QStringList lines;
	for( QMap<QString, QString>::const_iterator it = opt.constBegin(); it != opt.constEnd(); ++it )
	{
		if( it.key().trimmed().isEmpty() )
			continue;
		lines << ( it.key().trimmed() + QLatin1Char( '=' ) + Lab_Escape( it.value() ) );
	}
	return lines.join( QLatin1Char( '\n' ) );
}

QString AQ_Lab_Get( const Virtual_Machine *vm, const QString &key )
{
	return AQ_Lab_Map( Lab_Blob_Of( vm ) ).value( key );
}

void AQ_Lab_Set( Virtual_Machine *vm, const QString &key, const QString &value )
{
	if( ! vm )
		return;
	QMap<QString, QString> opt = AQ_Lab_Map( vm->Get_Lab_Options() );
	if( value.isEmpty() )
		opt.remove( key );
	else
		opt.insert( key, value );
	vm->Set_Lab_Options( AQ_Lab_Blob( opt ) );
}

static QString Find_Qemu_Img( Virtual_Machine *vm )
{
	QString bin;
	if( vm )
		bin = vm->Get_Current_Emulator_Binary_Path( vm->Get_Computer_Type() );
	QStringList candidates;
	if( ! bin.isEmpty() )
	{
		QDir dir = QFileInfo( bin ).dir();
		candidates << dir.filePath( QStringLiteral( "qemu-img" ) );
#ifdef Q_OS_WIN
		candidates << dir.filePath( QStringLiteral( "qemu-img.exe" ) );
#endif
	}
	candidates << QStringLiteral( "qemu-img" );
	for( int i = 0; i < candidates.count(); ++i )
	{
		if( QFileInfo( candidates.at( i ) ).exists() || candidates.at( i ) == QLatin1String( "qemu-img" ) )
			return candidates.at( i );
	}
	return QStringLiteral( "qemu-img" );
}

static QString Run_Tool( const QString &program, const QStringList &args, int ms, int *code )
{
	QProcess proc;
	proc.start( program, args );
	if( ! proc.waitForFinished( ms ) )
	{
		proc.kill();
		if( code )
			*code = -1;
		return proc.readAllStandardError();
	}
	if( code )
		*code = proc.exitCode();
	const QString out = QString::fromLocal8Bit( proc.readAllStandardOutput() );
	const QString err = QString::fromLocal8Bit( proc.readAllStandardError() );
	return out.isEmpty() ? err : out;
}

static bool Args_Have( const QStringList &args, const QString &flag )
{
	return args.contains( flag );
}

void AQ_Lab_Append_Args( QStringList &args, Virtual_Machine *vm )
{
	if( ! vm )
		return;
	const QMap<QString, QString> opt = AQ_Lab_Map( vm->Get_Lab_Options() );

	if( opt.value( QStringLiteral( "gdb" ) ) == QLatin1String( "on" ) )
	{
		const QString port = opt.value( QStringLiteral( "gdb_port" ), QStringLiteral( "1234" ) );
		if( ! Args_Have( args, QStringLiteral( "-gdb" ) ) )
			args << QStringLiteral( "-gdb" ) << ( QStringLiteral( "tcp:127.0.0.1:" ) + port );
		if( opt.value( QStringLiteral( "gdb_wait" ) ) == QLatin1String( "on" ) && ! Args_Have( args, QStringLiteral( "-S" ) ) )
			args << QStringLiteral( "-S" );
	}

	const QString plugin = opt.value( QStringLiteral( "tcg_plugin" ) ).trimmed();
	if( ! plugin.isEmpty() && QFileInfo( plugin ).exists() )
		args << QStringLiteral( "-plugin" ) << plugin;

	if( opt.value( QStringLiteral( "aarch64_off" ) ) == QLatin1String( "on" ) &&
	    vm->Get_Computer_Type().contains( QLatin1String( "aarch64" ), Qt::CaseInsensitive ) )
	{
		const int cpu = args.indexOf( QStringLiteral( "-cpu" ) );
		if( cpu >= 0 && cpu + 1 < args.count() )
		{
			if( ! args[ cpu + 1 ].contains( QLatin1String( "aarch64=" ) ) )
				args[ cpu + 1 ] += QStringLiteral( ",aarch64=off" );
		}
		else
			args << QStringLiteral( "-cpu" ) << QStringLiteral( "max,aarch64=off" );
	}

	const QString cache = opt.value( QStringLiteral( "cache_mode" ) );
	const QString aio = opt.value( QStringLiteral( "aio" ) );
	if( ! cache.isEmpty() || ! aio.isEmpty() )
	{
		for( int i = 0; i < args.count(); ++i )
		{
			if( args.at( i ) != QLatin1String( "-drive" ) || i + 1 >= args.count() )
				continue;
			QString &drive = args[ i + 1 ];
			if( ! cache.isEmpty() && ! drive.contains( QLatin1String( "cache=" ) ) )
				drive += QStringLiteral( ",cache=" ) + cache;
			if( ! aio.isEmpty() && ! drive.contains( QLatin1String( "aio=" ) ) )
				drive += QStringLiteral( ",aio=" ) + aio;
		}
	}

	const QString iops = opt.value( QStringLiteral( "iops" ) );
	const QString bps = opt.value( QStringLiteral( "bps" ) );
	if( ! iops.isEmpty() || ! bps.isEmpty() )
	{
		for( int i = 0; i < args.count(); ++i )
		{
			if( args.at( i ) != QLatin1String( "-drive" ) || i + 1 >= args.count() )
				continue;
			if( ! iops.isEmpty() && ! args.at( i + 1 ).contains( QLatin1String( "throttling.iops-total=" ) ) )
				args[ i + 1 ] += QStringLiteral( ",throttling.iops-total=" ) + iops;
			if( ! bps.isEmpty() && ! args.at( i + 1 ).contains( QLatin1String( "throttling.bps-total=" ) ) )
				args[ i + 1 ] += QStringLiteral( ",throttling.bps-total=" ) + bps;
		}
	}

	const QString heads = opt.value( QStringLiteral( "heads" ) );
	if( ! heads.isEmpty() && heads != QLatin1String( "1" ) )
	{
		for( int i = 0; i < args.count(); ++i )
		{
			if( args.at( i ) == QLatin1String( "-device" ) && i + 1 < args.count() &&
			    ( args.at( i + 1 ).startsWith( QLatin1String( "virtio-gpu" ) ) ||
			      args.at( i + 1 ).startsWith( QLatin1String( "virtio-vga" ) ) ||
			      args.at( i + 1 ).startsWith( QLatin1String( "qxl" ) ) ) &&
			    ! args.at( i + 1 ).contains( QLatin1String( "max_outputs=" ) ) )
			{
				args[ i + 1 ] += QStringLiteral( ",max_outputs=" ) + heads;
				break;
			}
		}
	}

#ifndef Q_OS_WIN
	if( opt.value( QStringLiteral( "virgl" ) ) == QLatin1String( "on" ) )
	{
		for( int i = 0; i < args.count(); ++i )
		{
			if( args.at( i ) == QLatin1String( "-vga" ) && i + 1 < args.count() && args.at( i + 1 ) == QLatin1String( "virtio" ) )
				args[ i + 1 ] = QStringLiteral( "virtio" );
			if( args.at( i ) == QLatin1String( "-device" ) && i + 1 < args.count() )
			{
				if( args.at( i + 1 ).startsWith( QLatin1String( "virtio-vga" ) ) &&
				    ! args.at( i + 1 ).contains( QLatin1String( "virtio-vga-gl" ) ) )
					args[ i + 1 ].replace( QLatin1String( "virtio-vga" ), QLatin1String( "virtio-vga-gl" ) );
				else if( args.at( i + 1 ).startsWith( QLatin1String( "virtio-gpu-pci" ) ) &&
				         ! args.at( i + 1 ).contains( QLatin1String( "virtio-gpu-gl-pci" ) ) )
					args[ i + 1 ].replace( QLatin1String( "virtio-gpu-pci" ), QLatin1String( "virtio-gpu-gl-pci" ) );
			}
		}
	}
#endif

	if( opt.value( QStringLiteral( "ramfb" ) ) == QLatin1String( "on" ) )
		args << QStringLiteral( "-device" ) << QStringLiteral( "virtio-ramfb" );

	const QString vfio = opt.value( QStringLiteral( "vfio" ) ).trimmed();
	if( ! vfio.isEmpty() )
	{
		QString dev = QStringLiteral( "vfio-pci,host=" ) + vfio;
		const QString rom = opt.value( QStringLiteral( "vfio_rom" ) ).trimmed();
		if( ! rom.isEmpty() )
			dev += QStringLiteral( ",romfile=" ) + rom;
		args << QStringLiteral( "-device" ) << dev;
	}

	if( opt.value( QStringLiteral( "keyboard" ) ) == QLatin1String( "usb" ) )
		args << QStringLiteral( "-device" ) << QStringLiteral( "usb-kbd" );

	if( opt.value( QStringLiteral( "guest_agent" ) ) == QLatin1String( "on" ) )
	{
		const QString sock = QDir( AQEMU_User_Data_Dir() ).filePath(
			QStringLiteral( "qga-" ) + vm->Get_Machine_Name() + QStringLiteral( ".sock" ) );
		args << QStringLiteral( "-chardev" )
		     << ( QStringLiteral( "socket,id=qga0,path=" ) + sock + QStringLiteral( ",server=on,wait=off" ) );
		args << QStringLiteral( "-device" ) << QStringLiteral( "virtio-serial" );
		args << QStringLiteral( "-device" )
		     << QStringLiteral( "virtserialport,chardev=qga0,name=org.qemu.guest_agent.0" );
	}

	if( opt.value( QStringLiteral( "smartcard" ) ) == QLatin1String( "emulated" ) )
		args << QStringLiteral( "-device" ) << QStringLiteral( "usb-ccid" )
		     << QStringLiteral( "-device" ) << QStringLiteral( "ccid-card-emulated" );
	else if( opt.value( QStringLiteral( "smartcard" ) ) == QLatin1String( "passthru" ) )
		args << QStringLiteral( "-device" ) << QStringLiteral( "usb-ccid" )
		     << QStringLiteral( "-device" ) << QStringLiteral( "ccid-card-passthru" );

	const QString floppy = opt.value( QStringLiteral( "floppy" ) );
	if( ! floppy.isEmpty() )
		args << QStringLiteral( "-drive" ) << ( QStringLiteral( "if=floppy,format=raw,file=" ) + floppy );
	const QString pflash = opt.value( QStringLiteral( "pflash" ) );
	if( ! pflash.isEmpty() )
		args << QStringLiteral( "-drive" ) << ( QStringLiteral( "if=pflash,format=raw,readonly=on,file=" ) + pflash );

	const QStringList chardevs = opt.value( QStringLiteral( "chardevs" ) ).split( QLatin1Char( '\n' ), QString::SkipEmptyParts );
	for( int i = 0; i < chardevs.count(); ++i )
	{
		const QString line = chardevs.at( i ).trimmed();
		if( ! line.isEmpty() )
			args << QStringLiteral( "-chardev" ) << line;
	}
	const QString serial = opt.value( QStringLiteral( "serial_chardev" ) ).trimmed();
	if( ! serial.isEmpty() )
		args << QStringLiteral( "-serial" ) << ( QStringLiteral( "chardev:" ) + serial );

	const QStringList usb = opt.value( QStringLiteral( "usb_rules" ) ).split( QLatin1Char( '\n' ), QString::SkipEmptyParts );
	for( int i = 0; i < usb.count(); ++i )
	{
		const QStringList parts = usb.at( i ).split( QLatin1Char( ':' ) );
		if( parts.count() < 2 )
			continue;
		const QString cls = parts.count() > 2 ? parts.at( 2 ).trimmed() : QString();
		if( cls == QLatin1String( "03" ) || cls == QLatin1String( "0x03" ) )
			continue;
		QString dev = QStringLiteral( "usb-host,vendorid=" ) + parts.at( 0 ).trimmed()
			+ QStringLiteral( ",productid=" ) + parts.at( 1 ).trimmed();
		args << QStringLiteral( "-device" ) << dev;
	}

	if( opt.value( QStringLiteral( "mem_prealloc" ) ) == QLatin1String( "on" ) && ! Args_Have( args, QStringLiteral( "-mem-prealloc" ) ) )
		args << QStringLiteral( "-mem-prealloc" );

	const QString secret_file = opt.value( QStringLiteral( "secret_file" ) );
	if( ! secret_file.isEmpty() )
		args << QStringLiteral( "-object" )
		     << ( QStringLiteral( "secret,id=" ) + opt.value( QStringLiteral( "secret_id" ), QStringLiteral( "sec0" ) )
		          + QStringLiteral( ",file=" ) + secret_file );

	const QString luks = opt.value( QStringLiteral( "luks_image" ) );
	if( ! luks.isEmpty() )
		args << QStringLiteral( "-drive" )
		     << ( QStringLiteral( "file=" ) + luks + QStringLiteral( ",format=qcow2,key-secret=" )
		          + opt.value( QStringLiteral( "secret_id" ), QStringLiteral( "sec0" ) ) );

	if( opt.value( QStringLiteral( "sev" ) ) == QLatin1String( "snp" ) )
		args << QStringLiteral( "-object" )
		     << ( QStringLiteral( "sev-snp-guest,id=sev0,cbitpos=" ) + opt.value( QStringLiteral( "cbitpos" ), QStringLiteral( "51" ) )
		          + QStringLiteral( ",reduced-phys-bits=" ) + opt.value( QStringLiteral( "reduced_bits" ), QStringLiteral( "1" ) ) );
	if( opt.value( QStringLiteral( "tdx" ) ) == QLatin1String( "on" ) )
		args << QStringLiteral( "-object" )
		     << ( QStringLiteral( "tdx-guest,id=tdx0" ) );

	const QString eif = opt.value( QStringLiteral( "nitro_eif" ) );
	if( ! eif.isEmpty() && ! Args_Have( args, QStringLiteral( "-kernel" ) ) )
		args << QStringLiteral( "-kernel" ) << eif;
	const QString vsock = opt.value( QStringLiteral( "vsock" ) );
	if( ! vsock.isEmpty() )
	{
		args << QStringLiteral( "-chardev" ) << ( QStringLiteral( "socket,id=vsockc,path=" ) + vsock );
		args << QStringLiteral( "-device" ) << QStringLiteral( "vhost-user-vsock-pci,chardev=vsockc" );
	}

	const QString spice_note = opt.value( QStringLiteral( "spice_agent" ) );
	Q_UNUSED( spice_note );

	if( g_note_launch )
	{
		QSettings settings;
		settings.setValue( QStringLiteral( "Lab/Last_Command/" ) + vm->Get_UID(), args.join( QStringLiteral( "\n" ) ) );
		g_note_launch = false;
	}
}

static bool Device_Listed( Virtual_Machine *vm, const QString &name )
{
	if( ! vm || name.isEmpty() )
		return false;
	const QString bin = vm->Get_Current_Emulator_Binary_Path( vm->Get_Computer_Type() );
	if( bin.isEmpty() )
		return false;
	int code = 0;
	const QString help = Run_Tool( bin, QStringList() << QStringLiteral( "-device" ) << QStringLiteral( "help" ), 2500, &code );
	return help.contains( name );
}

void AQ_Lab_Adjust_Launch( Virtual_Machine *vm, QString &bin_path, QStringList *args )
{
	if( ! vm )
		return;
	const QString bin = AQ_Lab_Get( vm, QStringLiteral( "binary" ) ).trimmed();
	if( ! bin.isEmpty() && QFileInfo( bin ).exists() )
		bin_path = bin;
	if( ! args )
		return;
	const QString wrap = AQ_Lab_Get( vm, QStringLiteral( "wrap" ) );
	if( wrap == QLatin1String( "xemu" ) )
	{
		const QString toml = AQ_Lab_Get( vm, QStringLiteral( "xemu_toml" ) );
		if( ! toml.isEmpty() )
			*args = QStringList() << QStringLiteral( "-config" ) << toml;
	}
	else if( wrap == QLatin1String( "android" ) )
	{
		const QString avd = AQ_Lab_Get( vm, QStringLiteral( "android_avd" ) ).trimmed();
		if( ! avd.isEmpty() && ! bin_path.isEmpty() )
			*args = QStringList() << QStringLiteral( "-avd" ) << avd;
	}
}

static void Start_Companion( Virtual_Machine *vm, const QString &program, const QStringList &cargs )
{
	QProcess *proc = new QProcess();
	proc->start( program, cargs );
	g_companions[ vm ] << proc;
}

bool AQ_Lab_Prepare_Start( Virtual_Machine *vm )
{
	if( ! vm )
		return true;
	g_note_launch = true;
	const QMap<QString, QString> opt = AQ_Lab_Map( vm->Get_Lab_Options() );

	if( opt.value( QStringLiteral( "check_chain" ) ) == QLatin1String( "on" ) )
	{
		const QString disk = vm->Get_HDA().Get_File_Name();
		if( ! disk.isEmpty() && QFileInfo( disk ).exists() )
		{
			int code = 0;
			const QString info = Run_Tool( Find_Qemu_Img( vm ),
				QStringList() << QStringLiteral( "info" ) << QStringLiteral( "--backing-chain" ) << disk,
				8000, &code );
			if( code != 0 )
			{
				AQGraphic_Warning( QObject::tr( "Chain Studio" ),
					QObject::tr( "The disk backing chain did not open, so this VM was not started.\n\n%1" ).arg( info.left( 800 ) ) );
				g_note_launch = false;
				return false;
			}
		}
	}

	QSettings settings;
#ifdef Q_OS_WIN
	const bool whpx_bad = settings.value( QStringLiteral( "Lab/Whpx_Probe_Failed" ), false ).toBool();
#else
	const bool whpx_bad = false;
#endif
	if( whpx_bad && ! vm->Use_Force_TCG() && opt.value( QStringLiteral( "accel_policy" ) ) != QLatin1String( "tcg" ) )
	{
		const int answer = QMessageBox::warning( nullptr, QObject::tr( "WHPX health" ),
			QObject::tr( "The last Hyper-V / WHPX probe failed. Start with Force TCG, or start anyway." ),
			QObject::tr( "Force TCG" ), QObject::tr( "Start anyway" ), QString(), 0, 0 );
		if( answer == 0 )
		{
			vm->Use_Force_TCG( true );
			vm->Set_Machine_Accelerator( VM::TCG );
		}
	}

	const QString policy = opt.value( QStringLiteral( "accel_policy" ) );
	const QString owned = opt.value( QStringLiteral( "accel_forced" ) );
	if( policy == QLatin1String( "auto" ) || policy == QLatin1String( "hw" ) || policy.isEmpty() )
	{
		if( owned == QLatin1String( "tcg" ) )
			vm->Use_Force_TCG( false );
		if( owned == QLatin1String( "wsl" ) )
			vm->Use_Launch_Via_WSL( false );
		if( ! owned.isEmpty() )
			AQ_Lab_Set( vm, QStringLiteral( "accel_forced" ), QString() );
	}
	if( policy == QLatin1String( "tcg" ) )
	{
		vm->Use_Force_TCG( true );
		vm->Set_Machine_Accelerator( VM::TCG );
		AQ_Lab_Set( vm, QStringLiteral( "accel_forced" ), QStringLiteral( "tcg" ) );
	}
	else if( policy == QLatin1String( "hw" ) )
	{
		const bool x86 = vm->Get_Computer_Type().contains( QLatin1String( "x86" ), Qt::CaseInsensitive ) ||
		                 vm->Get_Computer_Type().contains( QLatin1String( "i386" ), Qt::CaseInsensitive );
#ifdef Q_OS_WIN
		if( ! x86 )
		{
			vm->Use_Force_TCG( true );
			vm->Set_Machine_Accelerator( VM::TCG );
			AQ_Lab_Set( vm, QStringLiteral( "accel_forced" ), QStringLiteral( "tcg" ) );
		}
		else
			vm->Set_Machine_Accelerator( VM::KVM );
#else
		Q_UNUSED( x86 );
		vm->Set_Machine_Accelerator( VM::KVM );
#endif
	}
	else if( policy == QLatin1String( "wsl" ) )
	{
		vm->Use_Launch_Via_WSL( true );
		AQ_Lab_Set( vm, QStringLiteral( "accel_forced" ), QStringLiteral( "wsl" ) );
	}

	const QList<VM_Shared_Folder> shares = vm->Get_Shared_Folders_List();
	for( int i = 0; i < shares.count(); ++i )
	{
		if( shares.at( i ).Get_Share_Kind() != QLatin1String( "virtiofs" ) )
			continue;
		const QString vfd = QStandardPaths::findExecutable( QStringLiteral( "virtiofsd" ) );
		if( vfd.isEmpty() )
			continue;
		const QString sock = QDir( AQEMU_User_Data_Dir() ).filePath(
			QStringLiteral( "virtiofs-" ) + QString::number( i ) + QStringLiteral( ".sock" ) );
		QFile::remove( sock );
		QStringList vargs;
		vargs << QStringLiteral( "--socket-path" ) << sock
		      << QStringLiteral( "--shared-dir" ) << shares.at( i ).Get_Folder();
		if( shares.at( i ).Get_Read_Only() )
			vargs << QStringLiteral( "--readonly" );
		Start_Companion( vm, vfd, vargs );
	}

	if( vm->Get_TPM_Type().trimmed().compare( QLatin1String( "emulator" ), Qt::CaseInsensitive ) == 0 )
	{
		QString tpm_sock = vm->Get_TPM_Path().trimmed();
		if( tpm_sock.isEmpty() )
			tpm_sock = QStringLiteral( "/tmp/aqemu-swtpm.sock" );
		const QString swtpm = QStandardPaths::findExecutable( QStringLiteral( "swtpm" ) );
		if( ! swtpm.isEmpty() )
		{
			QFile::remove( tpm_sock );
			const QFileInfo sock_info( tpm_sock );
			const QString state = sock_info.absolutePath() + QLatin1Char( '/' )
				+ sock_info.completeBaseName() + QStringLiteral( "-state" );
			QDir().mkpath( state );
			Start_Companion( vm, swtpm, QStringList()
				<< QStringLiteral( "socket" )
				<< QStringLiteral( "--tpm2" )
				<< QStringLiteral( "--ctrl" ) << ( QStringLiteral( "type=unixio,path=" ) + tpm_sock )
				<< QStringLiteral( "--tpmstate" ) << ( QStringLiteral( "dir=" ) + state ) );
		}
	}

	const QString vsock_helper = opt.value( QStringLiteral( "vsock_helper" ) );
	if( ! vsock_helper.isEmpty() && QFileInfo( vsock_helper ).exists() )
		Start_Companion( vm, vsock_helper, QStringList() << opt.value( QStringLiteral( "vsock" ) ) );

	return true;
}

void AQ_Lab_After_Start( Virtual_Machine *vm, QProcess *proc )
{
	if( ! vm || ! proc )
		return;
	const QString pin = AQ_Lab_Get( vm, QStringLiteral( "cpu_pin" ) ).trimmed();
	if( pin.isEmpty() )
		return;
	const QStringList parts = pin.split( QLatin1Char( ',' ), QString::SkipEmptyParts );
	if( parts.isEmpty() )
		return;
#ifdef Q_OS_WIN
	DWORD_PTR mask = 0;
	for( int i = 0; i < parts.count(); ++i )
	{
		bool ok = false;
		const int cpu = parts.at( i ).trimmed().toInt( &ok );
		if( ok && cpu >= 0 && cpu < 63 )
			mask |= ( DWORD_PTR( 1 ) << cpu );
	}
	if( mask && proc->processId() )
	{
		HANDLE handle = OpenProcess( PROCESS_SET_INFORMATION, FALSE, DWORD( proc->processId() ) );
		if( handle )
		{
			SetProcessAffinityMask( handle, mask );
			CloseHandle( handle );
		}
	}
#else
	cpu_set_t set;
	CPU_ZERO( &set );
	for( int i = 0; i < parts.count(); ++i )
	{
		bool ok = false;
		const int cpu = parts.at( i ).trimmed().toInt( &ok );
		if( ok && cpu >= 0 )
			CPU_SET( cpu, &set );
	}
	if( proc->processId() )
		sched_setaffinity( pid_t( proc->processId() ), sizeof( set ), &set );
#endif
}

void AQ_Lab_Stop_Companions( Virtual_Machine *vm )
{
	const QList<QProcess*> list = g_companions.take( vm );
	for( int i = 0; i < list.count(); ++i )
	{
		if( ! list.at( i ) )
			continue;
		list.at( i )->terminate();
		list.at( i )->waitForFinished( 1500 );
		list.at( i )->deleteLater();
	}
}

QString AQ_Lab_Effective_Accel( const Virtual_Machine *vm )
{
	if( ! vm )
		return QString();
	const QString policy = AQ_Lab_Get( vm, QStringLiteral( "accel_policy" ) );
	const bool x86 = vm->Get_Computer_Type().contains( QLatin1String( "x86" ), Qt::CaseInsensitive ) ||
	                 vm->Get_Computer_Type().contains( QLatin1String( "i386" ), Qt::CaseInsensitive );
	if( policy == QLatin1String( "tcg" ) || vm->Use_Force_TCG() )
		return QObject::tr( "TCG" );
	if( policy == QLatin1String( "wsl" ) )
		return QObject::tr( "WSL KVM" );
	if( policy == QLatin1String( "hw" ) )
	{
#ifdef Q_OS_WIN
		if( ! x86 )
			return QObject::tr( "TCG (this host cannot offer WHPX for this guest)" );
		return QObject::tr( "WHPX, then TCG" );
#else
		return QObject::tr( "KVM, then TCG" );
#endif
	}
	if( vm->Get_Machine_Accelerator() == VM::TCG )
		return QObject::tr( "TCG" );
#ifdef Q_OS_WIN
	if( ! x86 )
		return QObject::tr( "TCG" );
	return QObject::tr( "WHPX, then TCG" );
#else
	return QObject::tr( "KVM, then TCG" );
#endif
}

void AQ_Lab_Apply_Guest_Profile( Virtual_Machine *vm, const QString &os_name )
{
	if( ! vm )
		return;
	const QString os = os_name;
	if( os == QLatin1String( "Original Xbox" ) )
	{
		vm->Set_Machine_Type( QStringLiteral( "xbox" ) );
		vm->Use_Force_TCG( true );
		vm->Set_Machine_Accelerator( VM::TCG );
		AQ_Lab_Set( vm, QStringLiteral( "wrap" ), QStringLiteral( "xemu" ) );
	}
	else if( os == QLatin1String( "Android (AOSP Emulator)" ) )
		AQ_Lab_Set( vm, QStringLiteral( "wrap" ), QStringLiteral( "android" ) );
	else if( os == QLatin1String( "Nitro Enclave" ) )
	{
		vm->Set_Machine_Type( QStringLiteral( "nitro-enclave" ) );
		AQ_Lab_Set( vm, QStringLiteral( "wrap" ), QStringLiteral( "nitro" ) );
	}
	else if( os == QLatin1String( "SEV-SNP Guest" ) )
		AQ_Lab_Set( vm, QStringLiteral( "sev" ), QStringLiteral( "snp" ) );
	else if( os == QLatin1String( "TDX Guest" ) )
		AQ_Lab_Set( vm, QStringLiteral( "tdx" ), QStringLiteral( "on" ) );
	else if( os == QLatin1String( "Darwin research shell" ) )
	{
		vm->Use_Force_TCG( true );
		vm->Set_Machine_Accelerator( VM::TCG );
		AQ_Lab_Set( vm, QStringLiteral( "wrap" ), QStringLiteral( "darwin" ) );
	}
	else if( os == QLatin1String( "macOS (OpenCore)" ) )
	{
		vm->Use_Intel_MacOS_Profile( true );
		AQ_Lab_Set( vm, QStringLiteral( "wrap" ), QStringLiteral( "opencore" ) );
	}
	else if( os == QLatin1String( "PowerNV" ) || os == QLatin1String( "s390x Debian" ) ||
	         os == QLatin1String( "RISC-V virt lab" ) || os == QLatin1String( "LoongArch virt lab" ) )
		AQ_Lab_Set( vm, QStringLiteral( "gallery" ), os );
	else if( os == QLatin1String( "WoA native Linux" ) )
		AQ_Lab_Set( vm, QStringLiteral( "woa" ), QStringLiteral( "native" ) );
	else if( os == QLatin1String( "WoA x86 via TCG" ) )
	{
		vm->Use_Force_TCG( true );
		vm->Set_Machine_Accelerator( VM::TCG );
		AQ_Lab_Set( vm, QStringLiteral( "woa" ), QStringLiteral( "tcg" ) );
	}
	else if( os == QLatin1String( "OS/2" ) || os == QLatin1String( "MS-DOS" ) ||
	         os == QLatin1String( "Windows 98" ) || os == QLatin1String( "Mac OS 9" ) ||
	         os == QLatin1String( "Mac OS 6" ) || os == QLatin1String( "Mac OS 7" ) ||
	         os == QLatin1String( "Mac OS 8" ) )
	{
		vm->Use_Force_TCG( true );
		vm->Set_Machine_Accelerator( VM::TCG );
		vm->Set_Video_Card( QStringLiteral( "cirrus" ) );
		VM::Sound_Cards sound;
		sound.Audio_sb16 = true;
		vm->Set_Audio_Cards( sound );
	}
}

static QString Secret_Store_Path( const QString &vm_name )
{
	const QString path = QDir( AQEMU_User_Data_Dir() ).filePath( QStringLiteral( "secrets" ) );
	QDir().mkpath( path );
	return QDir( path ).filePath( vm_name + QStringLiteral( ".secret" ) );
}

static void Store_Secret( const QString &vm_name, const QString &secret )
{
#ifdef Q_OS_WIN
	const QString target = QStringLiteral( "AQEMU/" ) + vm_name;
	CREDENTIALW cred;
	memset( &cred, 0, sizeof( cred ) );
	cred.Type = CRED_TYPE_GENERIC;
	cred.TargetName = (LPWSTR)target.utf16();
	cred.CredentialBlobSize = DWORD( secret.size() * int( sizeof( ushort ) ) );
	cred.CredentialBlob = (LPBYTE)secret.utf16();
	cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
	CredWriteW( &cred, 0 );
	Q_UNUSED( vm_name );
#else
	QFile file( Secret_Store_Path( vm_name ) );
	if( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
		file.write( secret.toUtf8() );
		file.close();
		file.setPermissions( QFileDevice::ReadOwner | QFileDevice::WriteOwner );
	}
#endif
}

#ifdef Q_OS_WIN
static void Lock_Secret_File( const QString &path )
{
	HANDLE token = 0;
	if( ! OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token ) )
		return;
	DWORD len = 0;
	GetTokenInformation( token, TokenUser, 0, 0, &len );
	QByteArray buf( int( len ), 0 );
	if( len == 0 || ! GetTokenInformation( token, TokenUser, buf.data(), len, &len ) )
	{
		CloseHandle( token );
		return;
	}
	CloseHandle( token );
	TOKEN_USER *user = reinterpret_cast<TOKEN_USER *>( buf.data() );
	EXPLICIT_ACCESS_W access;
	memset( &access, 0, sizeof( access ) );
	access.grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
	access.grfAccessMode = SET_ACCESS;
	access.grfInheritance = NO_INHERITANCE;
	access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	access.Trustee.TrusteeType = TRUSTEE_IS_USER;
	access.Trustee.ptstrName = (LPWSTR)user->User.Sid;
	PACL acl = 0;
	if( SetEntriesInAclW( 1, &access, 0, &acl ) != ERROR_SUCCESS )
		return;
	SetNamedSecurityInfoW( (LPWSTR)path.utf16(), SE_FILE_OBJECT,
		DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
		0, 0, acl, 0 );
	LocalFree( acl );
}
#endif

static QString Load_Secret_File( const QString &vm_name )
{
#ifdef Q_OS_WIN
	const QString target = QStringLiteral( "AQEMU/" ) + vm_name;
	PCREDENTIALW cred = nullptr;
	if( ! CredReadW( (LPCWSTR)target.utf16(), CRED_TYPE_GENERIC, 0, &cred ) || ! cred )
		return QString();
	const QString secret = QString::fromUtf16( (const ushort *)cred->CredentialBlob,
		int( cred->CredentialBlobSize / sizeof( ushort ) ) );
	CredFree( cred );
	const QString path = Secret_Store_Path( vm_name );
	QFile file( path );
	if( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
		file.write( secret.toUtf8() );
		file.close();
		Lock_Secret_File( path );
	}
	return path;
#else
	const QString path = Secret_Store_Path( vm_name );
	return QFileInfo( path ).exists() ? path : QString();
#endif
}

static QWidget *Row( QWidget *parent, QWidget *left, QWidget *right )
{
	QWidget *w = new QWidget( parent );
	QHBoxLayout *lay = new QHBoxLayout( w );
	lay->setContentsMargins( 0, 0, 0, 0 );
	lay->addWidget( left );
	if( right )
		lay->addWidget( right );
	return w;
}

static bool Parse_Hostfwd_Rule( const QString &raw, QString &proto, QString &host, QString &guest_ip, QString &guest )
{
	QString rule = raw.trimmed();
	if( rule.startsWith( QLatin1String( "hostfwd=" ) ) )
		rule = rule.mid( 8 );
	const int colon = rule.indexOf( QLatin1Char( ':' ) );
	const int dash = rule.indexOf( QLatin1Char( '-' ) );
	if( colon <= 0 || dash < 0 || dash < colon )
		return false;
	proto = rule.left( colon );
	host = rule.mid( colon + 1, dash - colon - 1 );
	if( host.startsWith( QLatin1Char( ':' ) ) )
		host = host.mid( 1 );
	const QString guest_side = rule.mid( dash + 1 );
	const int guest_colon = guest_side.lastIndexOf( QLatin1Char( ':' ) );
	if( guest_colon >= 0 )
	{
		guest_ip = guest_side.left( guest_colon );
		if( guest_ip.isEmpty() )
			guest_ip = QStringLiteral( "10.0.2.15" );
		guest = guest_side.mid( guest_colon + 1 );
	}
	else
	{
		guest_ip = QStringLiteral( "10.0.2.15" );
		guest = guest_side;
	}
	return ! host.isEmpty() && ! guest.isEmpty();
}

static void Add_Port_Row( QTableWidget *table, const QString &proto, const QString &host, const QString &guest_ip, const QString &guest )
{
	const int row = table->rowCount();
	table->insertRow( row );
	table->setItem( row, 0, new QTableWidgetItem( proto ) );
	table->setItem( row, 1, new QTableWidgetItem( host ) );
	table->setItem( row, 2, new QTableWidgetItem( guest_ip ) );
	table->setItem( row, 3, new QTableWidgetItem( guest ) );
}

void AQ_Lab_Open_VM_Window( Virtual_Machine *vm, const QList<Virtual_Machine*> &others, QWidget *parent )
{
	if( ! vm )
		return;
	QDialog dlg( parent );
	dlg.setWindowTitle( QObject::tr( "Lab for %1" ).arg( vm->Get_Machine_Name() ) );
	dlg.resize( 760, 560 );
	QVBoxLayout *root = new QVBoxLayout( &dlg );
	QTabWidget *tabs = new QTabWidget( &dlg );
	root->addWidget( tabs );

	QMap<QString, QString> opt = AQ_Lab_Map( vm->Get_Lab_Options() );

	QWidget *net = new QWidget();
	QVBoxLayout *net_lay = new QVBoxLayout( net );
	QTableWidget *ports = new QTableWidget( 0, 4, net );
	ports->setHorizontalHeaderLabels( QStringList() << QObject::tr( "Proto" ) << QObject::tr( "Host port" )
		<< QObject::tr( "Guest IP" ) << QObject::tr( "Guest port" ) );
	ports->horizontalHeader()->setStretchLastSection( true );
	const QString existing = vm->Get_Network_Cards_Nativ().isEmpty() ? QString()
		: vm->Get_Network_Cards_Nativ().first().Get_HostFwd();
	const QStringList rules = existing.split( QStringLiteral( ",hostfwd=" ), QString::SkipEmptyParts );
	for( int i = 0; i < rules.count(); ++i )
	{
		QString proto, host, guest_ip, guest;
		if( Parse_Hostfwd_Rule( rules.at( i ), proto, host, guest_ip, guest ) )
			Add_Port_Row( ports, proto, host, guest_ip, guest );
	}
	net_lay->addWidget( ports );
	QHBoxLayout *presets = new QHBoxLayout();
	QPushButton *ssh = new QPushButton( QObject::tr( "SSH 22" ), net );
	QPushButton *rdp = new QPushButton( QObject::tr( "RDP 3389" ), net );
	QPushButton *http = new QPushButton( QObject::tr( "HTTP 80" ), net );
	QPushButton *add = new QPushButton( QObject::tr( "Add row" ), net );
	presets->addWidget( ssh );
	presets->addWidget( rdp );
	presets->addWidget( http );
	presets->addWidget( add );
	net_lay->addLayout( presets );
	QObject::connect( ssh, &QPushButton::clicked, &dlg, [ports]() { Add_Port_Row( ports, QStringLiteral( "tcp" ), QStringLiteral( "2222" ), QStringLiteral( "10.0.2.15" ), QStringLiteral( "22" ) ); } );
	QObject::connect( rdp, &QPushButton::clicked, &dlg, [ports]() { Add_Port_Row( ports, QStringLiteral( "tcp" ), QStringLiteral( "3389" ), QStringLiteral( "10.0.2.15" ), QStringLiteral( "3389" ) ); } );
	QObject::connect( http, &QPushButton::clicked, &dlg, [ports]() { Add_Port_Row( ports, QStringLiteral( "tcp" ), QStringLiteral( "8080" ), QStringLiteral( "10.0.2.15" ), QStringLiteral( "80" ) ); } );
	QObject::connect( add, &QPushButton::clicked, &dlg, [ports]() { Add_Port_Row( ports, QStringLiteral( "tcp" ), QString(), QStringLiteral( "10.0.2.15" ), QString() ); } );
	QCheckBox *air = new QCheckBox( QObject::tr( "Air-gap this guest (user-net restrict=on). SMB stays on the network card." ), net );
	air->setChecked( opt.value( QStringLiteral( "airgap" ) ) == QLatin1String( "on" ) );
	net_lay->addWidget( air );
	QPushButton *nic_socket = new QPushButton( QObject::tr( "Add isolated socket NIC" ), net );
	QPushButton *nic_mcast = new QPushButton( QObject::tr( "Add multicast LAN NIC" ), net );
	QPushButton *nic_tap = new QPushButton( QObject::tr( "TAP / bridge assistant" ), net );
	net_lay->addWidget( nic_socket );
	net_lay->addWidget( nic_mcast );
	net_lay->addWidget( nic_tap );
	QObject::connect( nic_socket, &QPushButton::clicked, &dlg, [vm]() {
		QList<VM_Net_Card_Native> cards = vm->Get_Network_Cards_Nativ();
		VM_Net_Card_Native extra;
		extra.Set_Network_Type( VM::Net_Mode_Native_Socket );
		extra.Set_Card_Model( QStringLiteral( "virtio-net-pci" ) );
		extra.Use_Listen( true );
		extra.Set_Listen( QStringLiteral( "127.0.0.1:1234" ) );
		cards << extra;
		vm->Set_Network_Cards_Nativ( cards );
	} );
	QObject::connect( nic_mcast, &QPushButton::clicked, &dlg, [vm]() {
		QList<VM_Net_Card_Native> cards = vm->Get_Network_Cards_Nativ();
		VM_Net_Card_Native extra;
		extra.Set_Network_Type( VM::Net_Mode_Native_MulticastSocket );
		extra.Set_Card_Model( QStringLiteral( "virtio-net-pci" ) );
		extra.Use_MCast( true );
		extra.Set_MCast( QStringLiteral( "230.0.0.1:1234" ) );
		cards << extra;
		vm->Set_Network_Cards_Nativ( cards );
	} );
	QObject::connect( nic_tap, &QPushButton::clicked, &dlg, [vm, &dlg]() {
		QMessageBox::warning( &dlg, QObject::tr( "TAP / bridge" ),
			QObject::tr( "Creating a TAP or bridge needs host privileges. AQEMU does not ask for a password.\n"
			             "Linux: ip tuntap add mode tap (then name the interface below).\n"
			             "Windows: an OpenVPN TAP adapter is used only when it already exists." ) );
#ifdef Q_OS_WIN
		int code = 0;
		const QString adapters = Run_Tool( QStringLiteral( "ipconfig" ), QStringList() << QStringLiteral( "/all" ), 8000, &code );
		if( ! adapters.contains( QLatin1String( "TAP" ), Qt::CaseInsensitive ) )
		{
			AQGraphic_Warning( QObject::tr( "TAP" ), QObject::tr( "No OpenVPN TAP adapter was found, so no TAP card was added." ) );
			return;
		}
#endif
		bool ok = false;
		const QString iface = QInputDialog::getText( &dlg, QObject::tr( "TAP" ), QObject::tr( "Existing interface name" ), QLineEdit::Normal, QStringLiteral( "tap0" ), &ok );
		if( ! ok || iface.trimmed().isEmpty() )
			return;
		QList<VM_Net_Card_Native> cards = vm->Get_Network_Cards_Nativ();
		VM_Net_Card_Native extra;
		extra.Set_Network_Type( VM::Net_Mode_Native_TAP );
		extra.Set_Card_Model( QStringLiteral( "virtio-net-pci" ) );
		extra.Use_Interface_Name( true );
		extra.Set_Interface_Name( iface.trimmed() );
		cards << extra;
		vm->Set_Network_Cards_Nativ( cards );
	} );
	QLabel *conflict = new QLabel( net );
	conflict->setWordWrap( true );
	net_lay->addWidget( conflict );
	QComboBox *accel = new QComboBox( net );
	accel->addItem( QObject::tr( "Auto" ), QStringLiteral( "auto" ) );
	accel->addItem( QObject::tr( "Prefer hardware" ), QStringLiteral( "hw" ) );
	accel->addItem( QObject::tr( "Force TCG" ), QStringLiteral( "tcg" ) );
	accel->addItem( QObject::tr( "Prefer WSL KVM" ), QStringLiteral( "wsl" ) );
	const int accel_idx = accel->findData( opt.value( QStringLiteral( "accel_policy" ), QStringLiteral( "auto" ) ) );
	if( accel_idx >= 0 )
		accel->setCurrentIndex( accel_idx );
	QLabel *accel_now = new QLabel( net );
	auto refresh_accel = [vm, accel, accel_now]() {
		QMap<QString, QString> tmp = AQ_Lab_Map( vm->Get_Lab_Options() );
		tmp.insert( QStringLiteral( "accel_policy" ), accel->currentData().toString() );
		Virtual_Machine copy( *vm );
		copy.Set_Lab_Options( AQ_Lab_Blob( tmp ) );
		accel_now->setText( QObject::tr( "Effective accelerator: %1" ).arg( AQ_Lab_Effective_Accel( &copy ) ) );
	};
	QObject::connect( accel, static_cast<void (QComboBox::*)(int)>( &QComboBox::currentIndexChanged ), &dlg, refresh_accel );
	refresh_accel();
	net_lay->addWidget( accel );
	net_lay->addWidget( accel_now );
	tabs->addTab( net, QObject::tr( "Network" ) );

	QWidget *dbg = new QWidget();
	QFormLayout *dbg_form = new QFormLayout( dbg );
	QCheckBox *gdb = new QCheckBox( QObject::tr( "Enable GDB stub" ), dbg );
	gdb->setChecked( opt.value( QStringLiteral( "gdb" ) ) == QLatin1String( "on" ) );
	QSpinBox *gdb_port = new QSpinBox( dbg );
	gdb_port->setRange( 1, 65535 );
	gdb_port->setValue( opt.value( QStringLiteral( "gdb_port" ), QStringLiteral( "1234" ) ).toInt() );
	QCheckBox *gdb_wait = new QCheckBox( QObject::tr( "Wait for GDB (-S)" ), dbg );
	gdb_wait->setChecked( opt.value( QStringLiteral( "gdb_wait" ) ) == QLatin1String( "on" ) );
	QPushButton *copy_gdb = new QPushButton( QObject::tr( "Copy target remote" ), dbg );
	QObject::connect( copy_gdb, &QPushButton::clicked, &dlg, [gdb_port]() {
		QGuiApplication::clipboard()->setText( QStringLiteral( "target remote 127.0.0.1:" ) + QString::number( gdb_port->value() ) );
	} );
	dbg_form->addRow( gdb );
	dbg_form->addRow( QObject::tr( "Port" ), gdb_port );
	dbg_form->addRow( gdb_wait );
	dbg_form->addRow( copy_gdb );
	QPushButton *dump = new QPushButton( QObject::tr( "Dump guest memory (ELF)" ), dbg );
	QObject::connect( dump, &QPushButton::clicked, &dlg, [vm, &dlg]() {
		if( ! vm->Get_QMP() || ! vm->Get_QMP()->Is_Connected() )
		{
			AQGraphic_Warning( QObject::tr( "Memory dump" ), QObject::tr( "Start the VM and wait for QMP." ) );
			return;
		}
		const QString path = QFileDialog::getSaveFileName( &dlg, QObject::tr( "Guest memory" ),
			QDir( AQEMU_User_Data_Dir() ).filePath( vm->Get_Machine_Name() + QStringLiteral( ".elf" ) ) );
		if( path.isEmpty() )
			return;
		QJsonObject args;
		args.insert( QStringLiteral( "protocol" ), QStringLiteral( "file:" ) + path );
		args.insert( QStringLiteral( "paging" ), false );
		args.insert( QStringLiteral( "format" ), QStringLiteral( "elf" ) );
		vm->Get_QMP()->Send_Command( QStringLiteral( "dump-guest-memory" ), args );
	} );
	dbg_form->addRow( dump );
	QComboBox *rr = new QComboBox( dbg );
	rr->addItem( QObject::tr( "Off" ), QString() );
	rr->addItem( QObject::tr( "Record" ), QStringLiteral( "record" ) );
	rr->addItem( QObject::tr( "Replay" ), QStringLiteral( "replay" ) );
	const int rr_idx = rr->findData( opt.value( QStringLiteral( "rr" ) ) );
	if( rr_idx >= 0 )
		rr->setCurrentIndex( rr_idx );
	QLineEdit *rrfile = new QLineEdit( opt.value( QStringLiteral( "rrfile" ) ), dbg );
	QSpinBox *shift = new QSpinBox( dbg );
	shift->setRange( 0, 12 );
	shift->setValue( opt.value( QStringLiteral( "icount_shift" ), QStringLiteral( "7" ) ).toInt() );
	dbg_form->addRow( QObject::tr( "Record / replay" ), rr );
	dbg_form->addRow( QObject::tr( "rrfile" ), rrfile );
	dbg_form->addRow( QObject::tr( "icount shift" ), shift );
	QPlainTextEdit *qmp = new QPlainTextEdit( dbg );
	qmp->setPlaceholderText( QObject::tr( "QMP execute name, or JSON object of arguments" ) );
	QLineEdit *qmp_name = new QLineEdit( QStringLiteral( "query-status" ), dbg );
	QPushButton *qmp_send = new QPushButton( QObject::tr( "Send QMP" ), dbg );
	QObject::connect( qmp_send, &QPushButton::clicked, &dlg, [vm, qmp_name, qmp]() {
		if( ! vm->Get_QMP() || ! vm->Get_QMP()->Is_Connected() )
		{
			qmp->appendPlainText( QObject::tr( "QMP is down." ) );
			return;
		}
		QJsonObject args;
		const QString body = qmp->toPlainText().trimmed();
		if( body.startsWith( QLatin1Char( '{' ) ) )
			args = QJsonDocument::fromJson( body.toUtf8() ).object();
		const QString cmd = qmp_name->text().trimmed();
		if( cmd == QLatin1String( "guest-exec" ) && AQ_Lab_Get( vm, QStringLiteral( "guest_exec" ) ) != QLatin1String( "on" ) )
		{
			qmp->appendPlainText( QObject::tr( "guest-exec stays off until you enable it on the Devices tab." ) );
			return;
		}
		vm->Get_QMP()->Send_Command( cmd, args );
		qmp->appendPlainText( QObject::tr( "Sent %1" ).arg( qmp_name->text().trimmed() ) );
	} );
	dbg_form->addRow( QObject::tr( "QMP command" ), qmp_name );
	dbg_form->addRow( qmp_send );
	dbg_form->addRow( qmp );
	QLineEdit *devadd = new QLineEdit( dbg );
	devadd->setPlaceholderText( QObject::tr( "driver=usb-tablet or device id to delete" ) );
	QPushButton *dev_add_btn = new QPushButton( QObject::tr( "device_add" ), dbg );
	QPushButton *dev_del_btn = new QPushButton( QObject::tr( "device_del" ), dbg );
	QObject::connect( dev_add_btn, &QPushButton::clicked, &dlg, [vm, devadd]() {
		if( vm->Get_QMP() && vm->Get_QMP()->Is_Connected() )
		{
			QJsonObject args;
			const QStringList bits = devadd->text().split( QLatin1Char( ',' ), QString::SkipEmptyParts );
			for( int i = 0; i < bits.count(); ++i )
			{
				const QStringList kv = bits.at( i ).split( QLatin1Char( '=' ) );
				if( kv.count() == 2 )
					args.insert( kv.at( 0 ).trimmed(), kv.at( 1 ).trimmed() );
			}
			vm->Get_QMP()->Send_Command( QStringLiteral( "device_add" ), args );
		}
	} );
	QObject::connect( dev_del_btn, &QPushButton::clicked, &dlg, [vm, devadd]() {
		if( vm->Get_QMP() && vm->Get_QMP()->Is_Connected() )
		{
			QJsonObject args;
			args.insert( QStringLiteral( "id" ), devadd->text().trimmed() );
			vm->Get_QMP()->Send_Command( QStringLiteral( "device_del" ), args );
		}
	} );
	dbg_form->addRow( devadd, Row( dbg, dev_add_btn, dev_del_btn ) );
	tabs->addTab( dbg, QObject::tr( "Debug" ) );

	QWidget *disk = new QWidget();
	QFormLayout *disk_form = new QFormLayout( disk );
	QComboBox *cache = new QComboBox( disk );
	cache->addItem( QObject::tr( "Leave disk default" ), QString() );
	cache->addItem( QObject::tr( "Safe (writethrough)" ), QStringLiteral( "writethrough" ) );
	cache->addItem( QObject::tr( "Fast (writeback)" ), QStringLiteral( "writeback" ) );
	cache->addItem( QObject::tr( "Install (unsafe)" ), QStringLiteral( "unsafe" ) );
	cache->addItem( QObject::tr( "Bench (none)" ), QStringLiteral( "none" ) );
	const int cache_idx = cache->findData( opt.value( QStringLiteral( "cache_mode" ) ) );
	if( cache_idx >= 0 )
		cache->setCurrentIndex( cache_idx );
	QComboBox *aio = new QComboBox( disk );
	aio->addItem( QObject::tr( "default" ), QString() );
	aio->addItem( QStringLiteral( "threads" ), QStringLiteral( "threads" ) );
	aio->addItem( QStringLiteral( "native" ), QStringLiteral( "native" ) );
#ifndef Q_OS_WIN
	aio->addItem( QStringLiteral( "io_uring" ), QStringLiteral( "io_uring" ) );
#endif
	const int aio_idx = aio->findData( opt.value( QStringLiteral( "aio" ) ) );
	if( aio_idx >= 0 )
		aio->setCurrentIndex( aio_idx );
	QLabel *cache_preview = new QLabel( disk );
	auto refresh_cache = [cache, aio, cache_preview]() {
		cache_preview->setText( QObject::tr( "Generated: cache=%1 aio=%2" )
			.arg( cache->currentData().toString().isEmpty() ? QObject::tr( "omit" ) : cache->currentData().toString() )
			.arg( aio->currentData().toString().isEmpty() ? QObject::tr( "omit" ) : aio->currentData().toString() ) );
	};
	QObject::connect( cache, static_cast<void (QComboBox::*)(int)>( &QComboBox::currentIndexChanged ), &dlg, refresh_cache );
	QObject::connect( aio, static_cast<void (QComboBox::*)(int)>( &QComboBox::currentIndexChanged ), &dlg, refresh_cache );
	refresh_cache();
	QLineEdit *iops = new QLineEdit( opt.value( QStringLiteral( "iops" ) ), disk );
	QLineEdit *bps = new QLineEdit( opt.value( QStringLiteral( "bps" ) ), disk );
	QLineEdit *floppy = new QLineEdit( opt.value( QStringLiteral( "floppy" ) ), disk );
	QLineEdit *sd = new QLineEdit( vm->Get_SecureDigital_File(), disk );
	QLineEdit *mtd = new QLineEdit( vm->Get_MTDBlock_File(), disk );
	QLineEdit *pflash = new QLineEdit( opt.value( QStringLiteral( "pflash" ) ), disk );
	QLineEdit *plugin = new QLineEdit( opt.value( QStringLiteral( "tcg_plugin" ) ), disk );
	disk_form->addRow( QObject::tr( "Cache preset" ), cache );
	disk_form->addRow( QObject::tr( "AIO" ), aio );
	disk_form->addRow( cache_preview );
	disk_form->addRow( QObject::tr( "IOPS" ), iops );
	disk_form->addRow( QObject::tr( "BPS" ), bps );
	disk_form->addRow( QObject::tr( "Floppy image" ), floppy );
	disk_form->addRow( QObject::tr( "SD image" ), sd );
	disk_form->addRow( QObject::tr( "MTD image" ), mtd );
	disk_form->addRow( QObject::tr( "pflash image" ), pflash );
	QPushButton *plugin_browse = new QPushButton( QObject::tr( "Browse" ), disk );
	QObject::connect( plugin_browse, &QPushButton::clicked, &dlg, [plugin, &dlg]() {
		const QString path = QFileDialog::getOpenFileName( &dlg, QObject::tr( "TCG plugin" ) );
		if( ! path.isEmpty() )
			plugin->setText( path );
	} );
	disk_form->addRow( QObject::tr( "TCG plugin .so" ), Row( disk, plugin, plugin_browse ) );
	tabs->addTab( disk, QObject::tr( "Disks" ) );

	QWidget *boot = new QWidget();
	QFormLayout *boot_form = new QFormLayout( boot );
	QLineEdit *kernel = new QLineEdit( vm->Get_App_Kernel_Path(), boot );
	QLineEdit *initrd = new QLineEdit( vm->Get_Initrd_Path(), boot );
	QLineEdit *append = new QLineEdit( vm->Get_Kernel_ComLine(), boot );
	QLineEdit *dtb = new QLineEdit( vm->Get_DeviceTree_Path(), boot );
	QComboBox *rtc = new QComboBox( boot );
	rtc->addItems( QStringList() << QStringLiteral( "host" ) << QStringLiteral( "vm" ) << QStringLiteral( "rt" ) );
	const int rtc_idx = rtc->findText( vm->Get_RTC_Clock() );
	if( rtc_idx >= 0 )
		rtc->setCurrentIndex( rtc_idx );
	QCheckBox *drift = new QCheckBox( QObject::tr( "driftfix=slew" ), boot );
	drift->setChecked( vm->Use_RTC_TD_Hack() );
	QCheckBox *arm32 = new QCheckBox( QObject::tr( "32-bit ARM EL1 (aarch64=off)" ), boot );
	arm32->setChecked( opt.value( QStringLiteral( "aarch64_off" ) ) == QLatin1String( "on" ) );
	QTableWidget *fwcfg = new QTableWidget( 0, 3, boot );
	fwcfg->setHorizontalHeaderLabels( QStringList() << QObject::tr( "Name" ) << QObject::tr( "Kind" ) << QObject::tr( "File or string" ) );
	fwcfg->horizontalHeader()->setStretchLastSection( true );
	fwcfg->setMinimumHeight( 120 );
	const QStringList fw_lines = vm->Get_FW_CFG_Lines().split( QLatin1Char( '\n' ), QString::SkipEmptyParts );
	for( int i = 0; i < fw_lines.count(); ++i )
	{
		const QString line = fw_lines.at( i ).trimmed();
		const QString name = line.section( QLatin1Char( '=' ), 1 ).section( QLatin1Char( ',' ), 0, 0 );
		QString kind = QStringLiteral( "file" );
		QString value;
		if( line.contains( QLatin1String( ",string=" ) ) )
		{
			kind = QStringLiteral( "string" );
			value = line.section( QLatin1String( ",string=" ), 1 );
		}
		else if( line.contains( QLatin1String( ",file=" ) ) )
			value = line.section( QLatin1String( ",file=" ), 1 );
		const int row = fwcfg->rowCount();
		fwcfg->insertRow( row );
		fwcfg->setItem( row, 0, new QTableWidgetItem( name ) );
		fwcfg->setItem( row, 1, new QTableWidgetItem( kind ) );
		fwcfg->setItem( row, 2, new QTableWidgetItem( value ) );
	}
	QPushButton *fw_add = new QPushButton( QObject::tr( "Add fw_cfg row" ), boot );
	QObject::connect( fw_add, &QPushButton::clicked, &dlg, [fwcfg]() {
		const int row = fwcfg->rowCount();
		fwcfg->insertRow( row );
		fwcfg->setItem( row, 0, new QTableWidgetItem( QStringLiteral( "opt/com.example" ) ) );
		fwcfg->setItem( row, 1, new QTableWidgetItem( QStringLiteral( "file" ) ) );
		fwcfg->setItem( row, 2, new QTableWidgetItem() );
	} );
	QComboBox *sandbox = new QComboBox( boot );
	sandbox->addItem( QObject::tr( "Off" ), QString() );
	sandbox->addItem( QObject::tr( "Recommended" ), QStringLiteral( "on" ) );
	sandbox->addItem( QObject::tr( "Paranoid" ), QStringLiteral( "on,obsolete=deny,elevateprivileges=deny,spawn=deny,resourcecontrol=deny" ) );
	const int sb_idx = sandbox->findData( vm->Get_Sandbox() );
	if( sb_idx >= 0 )
		sandbox->setCurrentIndex( sb_idx );
	QLabel *sb_note = new QLabel( QObject::tr( "Sandbox presets apply on Linux. Windows QEMU ignores -sandbox." ), boot );
	sb_note->setWordWrap( true );
	boot_form->addRow( QObject::tr( "Kernel" ), kernel );
	boot_form->addRow( QObject::tr( "initrd" ), initrd );
	boot_form->addRow( QObject::tr( "append" ), append );
	boot_form->addRow( QObject::tr( "dtb" ), dtb );
	boot_form->addRow( QObject::tr( "RTC clock" ), rtc );
	boot_form->addRow( drift );
	boot_form->addRow( arm32 );
	boot_form->addRow( QObject::tr( "fw_cfg" ), fwcfg );
	boot_form->addRow( fw_add );
	boot_form->addRow( QObject::tr( "Sandbox" ), sandbox );
	boot_form->addRow( sb_note );
	tabs->addTab( boot, QObject::tr( "Boot" ) );

	QWidget *dev = new QWidget();
	QFormLayout *dev_form = new QFormLayout( dev );
	QCheckBox *tablet = new QCheckBox( QObject::tr( "USB tablet (absolute pointer)" ), dev );
	tablet->setChecked( vm->Get_Mouse_Type() == QLatin1String( "usb-tablet" ) );
	QCheckBox *usb_kbd = new QCheckBox( QObject::tr( "USB keyboard" ), dev );
	usb_kbd->setChecked( opt.value( QStringLiteral( "keyboard" ) ) == QLatin1String( "usb" ) );
	QComboBox *card = new QComboBox( dev );
	card->addItem( QObject::tr( "No smartcard" ), QString() );
	card->addItem( QObject::tr( "Emulated" ), QStringLiteral( "emulated" ) );
	card->addItem( QObject::tr( "Passthrough" ), QStringLiteral( "passthru" ) );
	const int card_idx = card->findData( opt.value( QStringLiteral( "smartcard" ) ) );
	if( card_idx >= 0 )
		card->setCurrentIndex( card_idx );
	QCheckBox *ga = new QCheckBox( QObject::tr( "QEMU guest agent serial port" ), dev );
	ga->setChecked( opt.value( QStringLiteral( "guest_agent" ) ) == QLatin1String( "on" ) );
	QCheckBox *ga_exec = new QCheckBox( QObject::tr( "Allow guest-exec from the QMP scratchpad" ), dev );
	ga_exec->setChecked( opt.value( QStringLiteral( "guest_exec" ) ) == QLatin1String( "on" ) );
	QLabel *ga_badge = new QLabel( dev );
	const QString qga_sock = QDir( AQEMU_User_Data_Dir() ).filePath( QStringLiteral( "qga-" ) + vm->Get_Machine_Name() + QStringLiteral( ".sock" ) );
	ga_badge->setText( QFileInfo( qga_sock ).exists() && vm->Get_State() == VM::VMS_Running
		? QObject::tr( "Guest agent: socket present while the VM is running" )
		: QObject::tr( "Guest agent: down until the guest runs qemu-ga" ) );
	QComboBox *period = new QComboBox( dev );
	period->addItem( QObject::tr( "Leave guest audio and video" ), QString() );
	period->addItem( QObject::tr( "DOS: SB16 + Cirrus" ), QStringLiteral( "dos" ) );
	period->addItem( QObject::tr( "Win9x: SB16 + Cirrus" ), QStringLiteral( "win9x" ) );
	period->addItem( QObject::tr( "XP: AC97 + std VGA" ), QStringLiteral( "ac97" ) );
	QSpinBox *heads = new QSpinBox( dev );
	heads->setRange( 1, 4 );
	heads->setValue( opt.value( QStringLiteral( "heads" ), QStringLiteral( "1" ) ).toInt() );
	QCheckBox *virgl = new QCheckBox( QObject::tr( "VirGL / virtio-gpu-gl" ), dev );
	virgl->setChecked( opt.value( QStringLiteral( "virgl" ) ) == QLatin1String( "on" ) );
#ifdef Q_OS_WIN
	virgl->setEnabled( false );
	virgl->setText( QObject::tr( "VirGL is unsupported on Windows" ) );
#endif
	QLineEdit *vfio = new QLineEdit( opt.value( QStringLiteral( "vfio" ) ), dev );
#ifdef Q_OS_WIN
	vfio->setEnabled( false );
	vfio->setPlaceholderText( QObject::tr( "PCI VFIO is a Linux host feature" ) );
#else
	vfio->setPlaceholderText( QObject::tr( "BB:DD.F" ) );
#endif
	QString usb_text = opt.value( QStringLiteral( "usb_rules" ) );
	usb_text.replace( QLatin1Char( '\n' ), QLatin1Char( ';' ) );
	QLineEdit *usb_rules = new QLineEdit( usb_text, dev );
	usb_rules->setPlaceholderText( QObject::tr( "vid:pid or vid:pid:class, semicolon separated. Class 03 (HID) is refused." ) );
	QLineEdit *cpu_pin = new QLineEdit( opt.value( QStringLiteral( "cpu_pin" ) ), dev );
	QCheckBox *prealloc = new QCheckBox( QObject::tr( "mem-prealloc" ), dev );
	prealloc->setChecked( opt.value( QStringLiteral( "mem_prealloc" ) ) == QLatin1String( "on" ) || vm->Use_Mem_Prealloc() );
	QString chardev_text = opt.value( QStringLiteral( "chardevs" ) );
	chardev_text.replace( QLatin1Char( '\n' ), QLatin1Char( ';' ) );
	QLineEdit *chardevs = new QLineEdit( chardev_text, dev );
	QLineEdit *serial = new QLineEdit( opt.value( QStringLiteral( "serial_chardev" ) ), dev );
	QCheckBox *tpm = new QCheckBox( QObject::tr( "TPM 2.0 emulator (swtpm socket)" ), dev );
	tpm->setChecked( vm->Get_TPM_Type() == QLatin1String( "emulator" ) );
	QLineEdit *tpm_path = new QLineEdit( vm->Get_TPM_Path(), dev );
	QCheckBox *secure = new QCheckBox( QObject::tr( "Secure Boot: keep a private OVMF VARS file" ), dev );
	secure->setChecked( vm->Use_UEFI() );
	QLineEdit *secret = new QLineEdit( dev );
	secret->setEchoMode( QLineEdit::Password );
	secret->setPlaceholderText( QObject::tr( "LUKS passphrase. Stored in the OS credential store, not the VM file." ) );
	QLineEdit *luks = new QLineEdit( opt.value( QStringLiteral( "luks_image" ) ), dev );
	QCheckBox *ramfb = new QCheckBox( QObject::tr( "virtio-ramfb when this QEMU lists it" ), dev );
	ramfb->setChecked( opt.value( QStringLiteral( "ramfb" ) ) == QLatin1String( "on" ) );
	dev_form->addRow( tablet );
	dev_form->addRow( usb_kbd );
	dev_form->addRow( QObject::tr( "Smartcard" ), card );
	dev_form->addRow( ga );
	dev_form->addRow( ga_exec );
	dev_form->addRow( ga_badge );
	dev_form->addRow( QObject::tr( "Period preset" ), period );
	dev_form->addRow( QObject::tr( "Display heads" ), heads );
	dev_form->addRow( virgl );
	dev_form->addRow( QObject::tr( "VFIO host" ), vfio );
	dev_form->addRow( QObject::tr( "USB redirect" ), usb_rules );
	dev_form->addRow( QObject::tr( "CPU pin" ), cpu_pin );
	dev_form->addRow( prealloc );
	dev_form->addRow( QObject::tr( "Chardevs" ), chardevs );
	dev_form->addRow( QObject::tr( "Serial id" ), serial );
	dev_form->addRow( tpm );
	dev_form->addRow( QObject::tr( "swtpm socket" ), tpm_path );
	dev_form->addRow( secure );
	dev_form->addRow( QObject::tr( "LUKS secret" ), secret );
	dev_form->addRow( QObject::tr( "LUKS image" ), luks );
	dev_form->addRow( ramfb );
	QLabel *spice = new QLabel( QObject::tr( "SPICE agent: clipboard, drag-and-drop, and file transfer need spice-vdagent inside the guest. AQEMU only shows the channel; it does not install the guest agent." ), dev );
	spice->setWordWrap( true );
	dev_form->addRow( spice );
	tabs->addTab( dev, QObject::tr( "Devices" ) );

	QWidget *wrap = new QWidget();
	QFormLayout *wrap_form = new QFormLayout( wrap );
	QLineEdit *binary = new QLineEdit( opt.value( QStringLiteral( "binary" ) ), wrap );
	QLineEdit *mcpx = new QLineEdit( opt.value( QStringLiteral( "xemu_mcpx" ) ), wrap );
	QLineEdit *bios = new QLineEdit( opt.value( QStringLiteral( "xemu_bios" ) ), wrap );
	QLineEdit *xhdd = new QLineEdit( opt.value( QStringLiteral( "xemu_hdd" ) ), wrap );
	QLineEdit *dvd = new QLineEdit( opt.value( QStringLiteral( "xemu_dvd" ) ), wrap );
	QLineEdit *eeprom = new QLineEdit( opt.value( QStringLiteral( "xemu_eeprom" ) ), wrap );
	QLineEdit *sdk = new QLineEdit( opt.value( QStringLiteral( "android_sdk" ) ), wrap );
	QLineEdit *avd = new QLineEdit( opt.value( QStringLiteral( "android_avd" ) ), wrap );
	QLineEdit *oc = new QLineEdit( vm->Get_OpenCore_Boot_Path(), wrap );
	QLineEdit *eif = new QLineEdit( opt.value( QStringLiteral( "nitro_eif" ) ), wrap );
	QLineEdit *vsock = new QLineEdit( opt.value( QStringLiteral( "vsock" ) ), wrap );
	QLineEdit *darwin = new QLineEdit( opt.value( QStringLiteral( "darwin_dir" ) ), wrap );
	QCheckBox *sev = new QCheckBox( QObject::tr( "SEV-SNP when /dev/sev exists" ), wrap );
	sev->setChecked( opt.value( QStringLiteral( "sev" ) ) == QLatin1String( "snp" ) );
	QCheckBox *tdx = new QCheckBox( QObject::tr( "TDX when the host firmware is present" ), wrap );
	tdx->setChecked( opt.value( QStringLiteral( "tdx" ) ) == QLatin1String( "on" ) );
#ifdef Q_OS_WIN
	sev->setEnabled( false );
	tdx->setEnabled( false );
	sev->setText( QObject::tr( "SEV-SNP is a Linux host feature" ) );
	tdx->setText( QObject::tr( "TDX is a Linux host feature" ) );
#endif
	QLabel *byo = new QLabel( QObject::tr( "Bring your own firmware. AQEMU does not ship MCPX, BIOS, IPSW, or macOS media. Darwin research shell is not Inferno and does not boot SpringBoard." ), wrap );
	byo->setWordWrap( true );
#ifdef AQEMU_STORE_BUILD
	eif->setEnabled( false );
	vsock->setEnabled( false );
	wrap_form->addRow( new QLabel( QObject::tr( "Nitro, SEV, and TDX stay off the Store build." ), wrap ) );
#endif
	wrap_form->addRow( byo );
	wrap_form->addRow( QObject::tr( "Emulator binary" ), binary );
	wrap_form->addRow( QObject::tr( "MCPX" ), mcpx );
	wrap_form->addRow( QObject::tr( "Flash BIOS" ), bios );
	wrap_form->addRow( QObject::tr( "Xbox HDD" ), xhdd );
	wrap_form->addRow( QObject::tr( "DVD / ISO" ), dvd );
	wrap_form->addRow( QObject::tr( "EEPROM" ), eeprom );
	wrap_form->addRow( QObject::tr( "Android SDK" ), sdk );
	wrap_form->addRow( QObject::tr( "AVD name" ), avd );
	wrap_form->addRow( QObject::tr( "OpenCore image" ), oc );
	wrap_form->addRow( QObject::tr( "Nitro EIF" ), eif );
	wrap_form->addRow( QObject::tr( "vsock socket" ), vsock );
	wrap_form->addRow( QObject::tr( "Darwin firmware dir" ), darwin );
	wrap_form->addRow( sev );
	wrap_form->addRow( tdx );
	QPushButton *adb = new QPushButton( QObject::tr( "Show adb logcat" ), wrap );
	QPlainTextEdit *logcat = new QPlainTextEdit( wrap );
	QObject::connect( adb, &QPushButton::clicked, &dlg, [sdk, logcat]() {
		QString adb_bin = QDir( sdk->text() ).filePath( QStringLiteral( "platform-tools/adb" ) );
#ifdef Q_OS_WIN
		adb_bin += QStringLiteral( ".exe" );
#endif
		if( ! QFileInfo( adb_bin ).exists() )
			adb_bin = QStandardPaths::findExecutable( QStringLiteral( "adb" ) );
		int code = 0;
		logcat->setPlainText( Run_Tool( adb_bin, QStringList() << QStringLiteral( "logcat" ) << QStringLiteral( "-d" ) << QStringLiteral( "-t" ) << QStringLiteral( "80" ), 8000, &code ) );
	} );
	wrap_form->addRow( adb );
	wrap_form->addRow( logcat );
	tabs->addTab( wrap, QObject::tr( "Wraps" ) );

	QListWidget *shares = new QListWidget();
	const QList<VM_Shared_Folder> share_list = vm->Get_Shared_Folders_List();
	for( int i = 0; i < share_list.count(); ++i )
		shares->addItem( share_list.at( i ).Get_Folder() + QStringLiteral( " [" ) + share_list.at( i ).Get_Security_Model() + QStringLiteral( "]" ) );
	QComboBox *sec = new QComboBox();
	sec->addItems( QStringList() << QStringLiteral( "none" ) << QStringLiteral( "mapped-xattr" ) << QStringLiteral( "passthrough" ) );
	QLineEdit *tag = new QLineEdit();
	QCheckBox *ro = new QCheckBox( QObject::tr( "Read only" ) );
	QComboBox *kind = new QComboBox();
	kind->addItem( QStringLiteral( "9p" ) );
	kind->addItem( QStringLiteral( "virtiofs" ) );
#ifdef Q_OS_WIN
	kind->setCurrentIndex( 0 );
	kind->setEnabled( false );
#endif
	QLabel *mount_note = new QLabel( QObject::tr( "Linux guest: mount -t 9p -o trans=virtio TAG /mnt. virtiofs: mount -t virtiofs TAG /mnt. Windows guests do not get a virtiofsd helper from this host." ) );
	mount_note->setWordWrap( true );
	QWidget *share_tab = new QWidget();
	QFormLayout *share_form = new QFormLayout( share_tab );
	share_form->addRow( shares );
	share_form->addRow( QObject::tr( "Security model" ), sec );
	share_form->addRow( QObject::tr( "Mount tag" ), tag );
	share_form->addRow( ro );
	share_form->addRow( QObject::tr( "Kind" ), kind );
	share_form->addRow( mount_note );
	tabs->addTab( share_tab, QObject::tr( "Shares" ) );

	QDialogButtonBox *buttons = new QDialogButtonBox( QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dlg );
	root->addWidget( buttons );
	QObject::connect( buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject );
	QObject::connect( buttons, &QDialogButtonBox::accepted, &dlg, [&]() {
		QStringList hostfwd;
		QStringList used;
		for( int row = 0; row < ports->rowCount(); ++row )
		{
			const QString proto = ports->item( row, 0 ) ? ports->item( row, 0 )->text().trimmed() : QStringLiteral( "tcp" );
			const QString host = ports->item( row, 1 ) ? ports->item( row, 1 )->text().trimmed() : QString();
			const QString gip = ports->item( row, 2 ) ? ports->item( row, 2 )->text().trimmed() : QStringLiteral( "10.0.2.15" );
			const QString gp = ports->item( row, 3 ) ? ports->item( row, 3 )->text().trimmed() : QString();
			if( host.isEmpty() || gp.isEmpty() )
				continue;
			for( int o = 0; o < others.count(); ++o )
			{
				if( ! others.at( o ) || others.at( o ) == vm )
					continue;
				const QList<VM_Net_Card_Native> cards = others.at( o )->Get_Network_Cards_Nativ();
				for( int c = 0; c < cards.count(); ++c )
				{
					if( cards.at( c ).Get_HostFwd().contains( QStringLiteral( "::" ) + host ) )
						used << QObject::tr( "%1 uses host port %2" ).arg( others.at( o )->Get_Machine_Name(), host );
				}
			}
			hostfwd << ( proto + QStringLiteral( "::" ) + host + QLatin1Char( '-' ) + gip + QLatin1Char( ':' ) + gp );
		}
		if( ! used.isEmpty() )
			conflict->setText( used.join( QStringLiteral( "; " ) ) );
		QList<VM_Net_Card_Native> cards = vm->Get_Network_Cards_Nativ();
		if( ! cards.isEmpty() )
		{
			QString joined;
			for( int i = 0; i < hostfwd.count(); ++i )
			{
				if( i )
					joined += QStringLiteral( ",hostfwd=" );
				joined += hostfwd.at( i );
			}
			cards[ 0 ].Use_HostFwd( ! joined.isEmpty() );
			cards[ 0 ].Set_HostFwd( joined );
			cards[ 0 ].Use_Restrict( air->isChecked() );
			cards[ 0 ].Set_Restrict( air->isChecked() );
			vm->Set_Network_Cards_Nativ( cards );
		}
		opt.insert( QStringLiteral( "airgap" ), air->isChecked() ? QStringLiteral( "on" ) : QString() );
		opt.insert( QStringLiteral( "accel_policy" ), accel->currentData().toString() );
		opt.insert( QStringLiteral( "gdb" ), gdb->isChecked() ? QStringLiteral( "on" ) : QString() );
		opt.insert( QStringLiteral( "gdb_port" ), QString::number( gdb_port->value() ) );
		opt.insert( QStringLiteral( "gdb_wait" ), gdb_wait->isChecked() ? QStringLiteral( "on" ) : QString() );
		opt.insert( QStringLiteral( "rr" ), rr->currentData().toString() );
		opt.insert( QStringLiteral( "rrfile" ), rrfile->text().trimmed() );
		opt.insert( QStringLiteral( "icount_shift" ), QString::number( shift->value() ) );
		if( ! rr->currentData().toString().isEmpty() )
		{
			vm->Set_ICount( QStringLiteral( "shift=%1,rr=%2,rrfile=%3" )
				.arg( shift->value() )
				.arg( rr->currentData().toString() )
				.arg( rrfile->text().trimmed() ) );
		}
		else
			vm->Set_ICount( QString() );
		opt.insert( QStringLiteral( "cache_mode" ), cache->currentData().toString() );
		opt.insert( QStringLiteral( "aio" ), aio->currentData().toString() );
		opt.insert( QStringLiteral( "iops" ), iops->text().trimmed() );
		opt.insert( QStringLiteral( "bps" ), bps->text().trimmed() );
		opt.insert( QStringLiteral( "floppy" ), floppy->text().trimmed() );
		opt.insert( QStringLiteral( "pflash" ), pflash->text().trimmed() );
		opt.insert( QStringLiteral( "tcg_plugin" ), plugin->text().trimmed() );
		if( ! sd->text().trimmed().isEmpty() )
		{
			vm->Use_SecureDigital_File( true );
			vm->Set_SecureDigital_File( sd->text().trimmed() );
		}
		if( ! mtd->text().trimmed().isEmpty() )
		{
			vm->Use_MTDBlock_File( true );
			vm->Set_MTDBlock_File( mtd->text().trimmed() );
		}
		if( ! kernel->text().trimmed().isEmpty() || ! initrd->text().trimmed().isEmpty() )
			vm->Set_Use_Linux_Boot( true );
		vm->Set_App_Kernel_Path( kernel->text().trimmed() );
		vm->Set_Initrd_Path( initrd->text().trimmed() );
		vm->Set_Kernel_ComLine( append->text() );
		vm->Set_DeviceTree_Path( dtb->text().trimmed() );
		vm->Set_RTC_Clock( rtc->currentText() );
		vm->Use_RTC_TD_Hack( drift->isChecked() );
		opt.insert( QStringLiteral( "aarch64_off" ), arm32->isChecked() ? QStringLiteral( "on" ) : QString() );
		QStringList fw_out;
		for( int row = 0; row < fwcfg->rowCount(); ++row )
		{
			const QString name = fwcfg->item( row, 0 ) ? fwcfg->item( row, 0 )->text().trimmed() : QString();
			QString kind = fwcfg->item( row, 1 ) ? fwcfg->item( row, 1 )->text().trimmed() : QStringLiteral( "file" );
			const QString value = fwcfg->item( row, 2 ) ? fwcfg->item( row, 2 )->text().trimmed() : QString();
			if( name.isEmpty() || value.isEmpty() )
				continue;
			if( kind != QLatin1String( "string" ) )
				kind = QStringLiteral( "file" );
			fw_out << ( QStringLiteral( "name=" ) + name + QLatin1Char( ',' ) + kind + QLatin1Char( '=' ) + value );
		}
		vm->Set_FW_CFG_Lines( fw_out.join( QLatin1Char( '\n' ) ) );
		vm->Set_Sandbox( sandbox->currentData().toString() );
		if( tablet->isChecked() )
			vm->Set_Mouse_Type( QStringLiteral( "usb-tablet" ) );
		opt.insert( QStringLiteral( "keyboard" ), usb_kbd->isChecked() ? QStringLiteral( "usb" ) : QString() );
		if( ! card->currentData().toString().isEmpty() && ! Device_Listed( vm, QStringLiteral( "usb-ccid" ) ) )
		{
			AQGraphic_Warning( QObject::tr( "Smartcard" ), QObject::tr( "This QEMU build does not list usb-ccid, so the smartcard was not enabled." ) );
			opt.insert( QStringLiteral( "smartcard" ), QString() );
		}
		else
			opt.insert( QStringLiteral( "smartcard" ), card->currentData().toString() );
		opt.insert( QStringLiteral( "guest_agent" ), ga->isChecked() ? QStringLiteral( "on" ) : QString() );
		opt.insert( QStringLiteral( "guest_exec" ), ga_exec->isChecked() ? QStringLiteral( "on" ) : QString() );
		if( period->currentData().toString() == QLatin1String( "dos" ) || period->currentData().toString() == QLatin1String( "win9x" ) )
		{
			VM::Sound_Cards sound;
			sound.Audio_sb16 = true;
			vm->Set_Audio_Cards( sound );
			vm->Set_Video_Card( QStringLiteral( "cirrus" ) );
		}
		else if( period->currentData().toString() == QLatin1String( "ac97" ) )
		{
			VM::Sound_Cards sound;
			sound.Audio_AC97 = true;
			vm->Set_Audio_Cards( sound );
			vm->Set_Video_Card( QStringLiteral( "std" ) );
		}
		opt.insert( QStringLiteral( "heads" ), QString::number( heads->value() ) );
		opt.insert( QStringLiteral( "virgl" ), virgl->isChecked() ? QStringLiteral( "on" ) : QString() );
		opt.insert( QStringLiteral( "vfio" ), vfio->text().trimmed() );
		opt.insert( QStringLiteral( "usb_rules" ), usb_rules->text().replace( QLatin1Char( ';' ), QLatin1Char( '\n' ) ) );
		opt.insert( QStringLiteral( "cpu_pin" ), cpu_pin->text().trimmed() );
		opt.insert( QStringLiteral( "mem_prealloc" ), prealloc->isChecked() ? QStringLiteral( "on" ) : QString() );
		vm->Use_Mem_Prealloc( prealloc->isChecked() );
		opt.insert( QStringLiteral( "chardevs" ), chardevs->text().replace( QLatin1Char( ';' ), QLatin1Char( '\n' ) ) );
		opt.insert( QStringLiteral( "serial_chardev" ), serial->text().trimmed() );
		if( tpm->isChecked() )
		{
			vm->Set_TPM_Type( QStringLiteral( "emulator" ) );
			vm->Set_TPM_Path( tpm_path->text().trimmed() );
		}
		vm->Use_UEFI( secure->isChecked() );
		if( ! secret->text().isEmpty() )
		{
			Store_Secret( vm->Get_Machine_Name(), secret->text() );
			const QString path = Load_Secret_File( vm->Get_Machine_Name() );
			opt.insert( QStringLiteral( "secret_file" ), path );
			opt.insert( QStringLiteral( "secret_id" ), QStringLiteral( "sec0" ) );
		}
		opt.insert( QStringLiteral( "luks_image" ), luks->text().trimmed() );
		if( ramfb->isChecked() && ! Device_Listed( vm, QStringLiteral( "virtio-ramfb" ) ) )
		{
			AQGraphic_Warning( QObject::tr( "virtio-ramfb" ), QObject::tr( "The selected QEMU binary does not list virtio-ramfb, so it was not enabled." ) );
			opt.insert( QStringLiteral( "ramfb" ), QString() );
		}
		else
			opt.insert( QStringLiteral( "ramfb" ), ramfb->isChecked() ? QStringLiteral( "on" ) : QString() );
		opt.insert( QStringLiteral( "binary" ), binary->text().trimmed() );
		opt.insert( QStringLiteral( "xemu_mcpx" ), mcpx->text().trimmed() );
		opt.insert( QStringLiteral( "xemu_bios" ), bios->text().trimmed() );
		opt.insert( QStringLiteral( "xemu_hdd" ), xhdd->text().trimmed() );
		opt.insert( QStringLiteral( "xemu_dvd" ), dvd->text().trimmed() );
		opt.insert( QStringLiteral( "xemu_eeprom" ), eeprom->text().trimmed() );
		opt.insert( QStringLiteral( "android_sdk" ), sdk->text().trimmed() );
		opt.insert( QStringLiteral( "android_avd" ), avd->text().trimmed() );
		vm->Set_OpenCore_Boot_Path( oc->text().trimmed() );
		opt.insert( QStringLiteral( "nitro_eif" ), eif->text().trimmed() );
		opt.insert( QStringLiteral( "vsock" ), vsock->text().trimmed() );
		opt.insert( QStringLiteral( "darwin_dir" ), darwin->text().trimmed() );
#ifndef Q_OS_WIN
		if( sev->isChecked() && ! QFileInfo( QStringLiteral( "/dev/sev" ) ).exists() )
		{
			AQGraphic_Warning( QObject::tr( "SEV-SNP" ), QObject::tr( "/dev/sev is not present, so SEV-SNP was not enabled." ) );
			opt.insert( QStringLiteral( "sev" ), QString() );
		}
		else
			opt.insert( QStringLiteral( "sev" ), sev->isChecked() ? QStringLiteral( "snp" ) : QString() );
		opt.insert( QStringLiteral( "tdx" ), tdx->isChecked() ? QStringLiteral( "on" ) : QString() );
#else
		opt.insert( QStringLiteral( "sev" ), QString() );
		opt.insert( QStringLiteral( "tdx" ), QString() );
#endif
		if( shares->currentRow() >= 0 )
		{
			QList<VM_Shared_Folder> list = vm->Get_Shared_Folders_List();
			if( shares->currentRow() < list.count() )
			{
				list[ shares->currentRow() ].Set_Security_Model( sec->currentText() );
				list[ shares->currentRow() ].Set_Mount_Tag( tag->text().trimmed() );
				list[ shares->currentRow() ].Set_Read_Only( ro->isChecked() );
				list[ shares->currentRow() ].Set_Share_Kind( kind->currentText() );
				vm->Set_Shared_Folders_List( list );
			}
		}
		if( ! mcpx->text().isEmpty() || ! bios->text().isEmpty() )
		{
			const QString toml = QDir( AQEMU_User_Data_Dir() ).filePath( vm->Get_Machine_Name() + QStringLiteral( "-xemu.toml" ) );
			QFile file( toml );
			if( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
			{
				QTextStream ts( &file );
				ts << "[sys.files]\n";
				if( ! mcpx->text().isEmpty() )
					ts << "bootrom_path = '" << mcpx->text() << "'\n";
				if( ! bios->text().isEmpty() )
					ts << "flashrom_path = '" << bios->text() << "'\n";
				if( ! xhdd->text().isEmpty() )
					ts << "hdd_path = '" << xhdd->text() << "'\n";
				if( ! dvd->text().isEmpty() )
					ts << "dvd_path = '" << dvd->text() << "'\n";
				if( ! eeprom->text().isEmpty() )
					ts << "eeprom_path = '" << eeprom->text() << "'\n";
			}
			opt.insert( QStringLiteral( "xemu_toml" ), toml );
		}
		vm->Set_Lab_Options( AQ_Lab_Blob( opt ) );
		vm->Save_VM();
		dlg.accept();
	} );
	dlg.exec();
}

static QString Redact( const QString &text )
{
	QString out = text;
	const QStringList keys = QStringList() << QStringLiteral( "token" ) << QStringLiteral( "password" )
		<< QStringLiteral( "osk" ) << QStringLiteral( "secret" ) << QStringLiteral( "ipsw" );
	const QStringList lines = out.split( QLatin1Char( '\n' ) );
	QStringList kept;
	for( int i = 0; i < lines.count(); ++i )
	{
		bool drop = false;
		for( int k = 0; k < keys.count(); ++k )
		{
			if( lines.at( i ).contains( keys.at( k ), Qt::CaseInsensitive ) )
			{
				drop = true;
				break;
			}
		}
		if( ! drop )
			kept << lines.at( i );
	}
	return kept.join( QLatin1Char( '\n' ) );
}

static void Zip_Put16( QByteArray &out, quint16 value )
{
	out.append( char( value & 0xff ) );
	out.append( char( ( value >> 8 ) & 0xff ) );
}

static void Zip_Put32( QByteArray &out, quint32 value )
{
	out.append( char( value & 0xff ) );
	out.append( char( ( value >> 8 ) & 0xff ) );
	out.append( char( ( value >> 16 ) & 0xff ) );
	out.append( char( ( value >> 24 ) & 0xff ) );
}

static quint32 Zip_Crc32( const QByteArray &data )
{
	quint32 crc = 0xffffffffu;
	for( int i = 0; i < data.size(); ++i )
	{
		crc ^= quint32( quint8( data.at( i ) ) );
		for( int bit = 0; bit < 8; ++bit )
			crc = ( crc & 1u ) ? ( ( crc >> 1 ) ^ 0xedb88320u ) : ( crc >> 1 );
	}
	return crc ^ 0xffffffffu;
}

static void Zip_Add( QByteArray &zip, QByteArray &central, int &count, const QString &name, const QByteArray &data )
{
	const QByteArray raw_name = name.toUtf8();
	const quint32 crc = Zip_Crc32( data );
	const quint32 offset = quint32( zip.size() );
	zip.append( "PK\x03\x04", 4 );
	Zip_Put16( zip, 20 );
	Zip_Put16( zip, 0 );
	Zip_Put16( zip, 0 );
	Zip_Put16( zip, 0 );
	Zip_Put16( zip, 0 );
	Zip_Put32( zip, crc );
	Zip_Put32( zip, quint32( data.size() ) );
	Zip_Put32( zip, quint32( data.size() ) );
	Zip_Put16( zip, quint16( raw_name.size() ) );
	Zip_Put16( zip, 0 );
	zip.append( raw_name );
	zip.append( data );

	central.append( "PK\x01\x02", 4 );
	Zip_Put16( central, 20 );
	Zip_Put16( central, 20 );
	Zip_Put16( central, 0 );
	Zip_Put16( central, 0 );
	Zip_Put16( central, 0 );
	Zip_Put16( central, 0 );
	Zip_Put32( central, crc );
	Zip_Put32( central, quint32( data.size() ) );
	Zip_Put32( central, quint32( data.size() ) );
	Zip_Put16( central, quint16( raw_name.size() ) );
	Zip_Put16( central, 0 );
	Zip_Put16( central, 0 );
	Zip_Put16( central, 0 );
	Zip_Put16( central, 0 );
	Zip_Put32( central, 0 );
	Zip_Put32( central, offset );
	central.append( raw_name );
	++count;
}

void AQ_Lab_Support_Bundle( QWidget *parent, Virtual_Machine *vm )
{
	const QString path = QFileDialog::getSaveFileName( parent, QObject::tr( "Support bundle" ),
		QDir( AQEMU_User_Data_Dir() ).filePath( QStringLiteral( "aqemu-support.zip" ) ),
		QObject::tr( "Zip archive (*.zip)" ) );
	if( path.isEmpty() )
		return;
	QSettings settings;
	QString settings_text = QStringLiteral( "AQEMU " ) + QStringLiteral( CURRENT_AQEMU_VERSION ) + QLatin1Char( '\n' );
	QFile ini( settings.fileName() );
	if( ini.open( QIODevice::ReadOnly ) )
		settings_text += Redact( QString::fromUtf8( ini.readAll() ) );
	QString command;
	if( vm )
	{
		QStringList args = vm->Build_QEMU_Args();
		for( int i = 0; i < args.count(); ++i )
		{
			const QString lower = args.at( i ).toLower();
			if( lower.contains( QLatin1String( "osk" ) ) || lower.contains( QLatin1String( "ipsw" ) ) ||
			    lower.contains( QLatin1String( "secret" ) ) || lower.contains( QLatin1String( "token" ) ) ||
			    lower.contains( QLatin1String( "password" ) ) )
				args[ i ] = QStringLiteral( "[redacted]" );
		}
		command = args.join( QStringLiteral( " " ) ) + QLatin1Char( '\n' )
			+ AQ_Lab_Effective_Accel( vm ) + QLatin1Char( '\n' );
	}
	QString boot_log;
	QFile blog( QDir( AQEMU_User_Data_Dir() ).filePath( QStringLiteral( "qemu-boot.log" ) ) );
	if( blog.open( QIODevice::ReadOnly ) )
		boot_log = Redact( QString::fromUtf8( blog.readAll() ) );

	QByteArray zip, central;
	int count = 0;
	Zip_Add( zip, central, count, QStringLiteral( "version.txt" ), QByteArray( CURRENT_AQEMU_VERSION ) + '\n' );
	Zip_Add( zip, central, count, QStringLiteral( "settings.txt" ), settings_text.toUtf8() );
	Zip_Add( zip, central, count, QStringLiteral( "qemu-command.txt" ), command.toUtf8() );
	Zip_Add( zip, central, count, QStringLiteral( "qemu-boot.log" ), boot_log.toUtf8() );
	const quint32 central_off = quint32( zip.size() );
	zip.append( central );
	zip.append( "PK\x05\x06", 4 );
	Zip_Put16( zip, 0 );
	Zip_Put16( zip, 0 );
	Zip_Put16( zip, quint16( count ) );
	Zip_Put16( zip, quint16( count ) );
	Zip_Put32( zip, quint32( central.size() ) );
	Zip_Put32( zip, central_off );
	Zip_Put16( zip, 0 );
	QFile file( path );
	if( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
		file.write( zip );
}

void AQ_Lab_Convert( QWidget *parent, Virtual_Machine *vm )
{
	QDialog dlg( parent );
	dlg.setWindowTitle( QObject::tr( "Convert disk" ) );
	QFormLayout *form = new QFormLayout( &dlg );
	QLineEdit *src = new QLineEdit( vm ? vm->Get_HDA().Get_File_Name() : QString(), &dlg );
	QLineEdit *dst = new QLineEdit( &dlg );
	QComboBox *fmt = new QComboBox( &dlg );
	fmt->addItems( QStringList() << QStringLiteral( "qcow2" ) << QStringLiteral( "raw" ) << QStringLiteral( "vmdk" ) << QStringLiteral( "vhdx" ) << QStringLiteral( "vpc" ) );
	QCheckBox *fourk = new QCheckBox( QObject::tr( "4K cluster / sector alignment" ), &dlg );
	QPushButton *go = new QPushButton( QObject::tr( "Convert" ), &dlg );
	form->addRow( QObject::tr( "Source" ), src );
	form->addRow( QObject::tr( "Destination" ), dst );
	form->addRow( QObject::tr( "Format" ), fmt );
	form->addRow( fourk );
	form->addRow( go );
	QObject::connect( go, &QPushButton::clicked, &dlg, [&]() {
		QStringList args;
		args << QStringLiteral( "convert" ) << QStringLiteral( "-O" ) << fmt->currentText();
		if( fourk->isChecked() && fmt->currentText() == QLatin1String( "qcow2" ) )
			args << QStringLiteral( "-o" ) << QStringLiteral( "cluster_size=4096" );
		args << src->text() << dst->text();
		int code = 0;
		const QString out = Run_Tool( Find_Qemu_Img( vm ), args, 600000, &code );
		QMessageBox::information( &dlg, QObject::tr( "Convert" ), code == 0 ? QObject::tr( "Convert finished." ) : out.left( 1200 ) );
	} );
	dlg.exec();
}

void AQ_Lab_Chain_Studio( QWidget *parent, Virtual_Machine *vm )
{
	if( ! vm )
		return;
	QDialog dlg( parent );
	dlg.setWindowTitle( QObject::tr( "Chain Studio" ) );
	QVBoxLayout *lay = new QVBoxLayout( &dlg );
	QPlainTextEdit *view = new QPlainTextEdit( &dlg );
	view->setReadOnly( true );
	const QString disk = vm->Get_HDA().Get_File_Name();
	int code = 0;
	view->setPlainText( Run_Tool( Find_Qemu_Img( vm ), QStringList() << QStringLiteral( "info" ) << QStringLiteral( "--backing-chain" ) << disk, 8000, &code ) );
	lay->addWidget( view );
	QPushButton *overlay = new QPushButton( QObject::tr( "Create overlay" ), &dlg );
	QPushButton *commit = new QPushButton( QObject::tr( "Commit" ), &dlg );
	QPushButton *rebase = new QPushButton( QObject::tr( "Rebase" ), &dlg );
	lay->addWidget( overlay );
	lay->addWidget( commit );
	lay->addWidget( rebase );
	QObject::connect( overlay, &QPushButton::clicked, &dlg, [vm, disk, &dlg]() {
		const QString dest = QFileDialog::getSaveFileName( &dlg, QObject::tr( "Overlay" ), disk + QStringLiteral( ".overlay.qcow2" ) );
		if( dest.isEmpty() )
			return;
		int code = 0;
		Run_Tool( Find_Qemu_Img( vm ), QStringList() << QStringLiteral( "create" ) << QStringLiteral( "-f" ) << QStringLiteral( "qcow2" )
			<< QStringLiteral( "-b" ) << disk << QStringLiteral( "-F" ) << QStringLiteral( "qcow2" ) << dest, 20000, &code );
		if( code == 0 )
		{
			AQ_Lab_Set( vm, QStringLiteral( "check_chain" ), QStringLiteral( "on" ) );
			vm->Save_VM();
		}
	} );
	QObject::connect( commit, &QPushButton::clicked, &dlg, [vm, disk]() {
		int code = 0;
		Run_Tool( Find_Qemu_Img( vm ), QStringList() << QStringLiteral( "commit" ) << disk, 600000, &code );
	} );
	QObject::connect( rebase, &QPushButton::clicked, &dlg, [vm, disk, &dlg]() {
		const QString base = QFileDialog::getOpenFileName( &dlg, QObject::tr( "New base" ) );
		if( base.isEmpty() )
			return;
		int code = 0;
		Run_Tool( Find_Qemu_Img( vm ), QStringList() << QStringLiteral( "rebase" ) << QStringLiteral( "-b" ) << base << disk, 600000, &code );
	} );
	dlg.exec();
}

void AQ_Lab_Catalog( QWidget *parent, Virtual_Machine *vm )
{
	QDialog dlg( parent );
	dlg.setWindowTitle( QObject::tr( "QEMU catalog" ) );
	QVBoxLayout *lay = new QVBoxLayout( &dlg );
	QListWidget *list = new QListWidget( &dlg );
	Available_Devices dev;
	const QString arch = vm ? vm->Get_Computer_Type() : QStringLiteral( "qemu-system-x86_64" );
	if( QEMU_Probe_Catalog::Load_Architecture( arch, dev ) )
	{
		for( int i = 0; i < dev.Machine_List.count(); ++i )
			list->addItem( QObject::tr( "machine %1" ).arg( dev.Machine_List.at( i ).Caption ) );
		for( int i = 0; i < dev.CPU_List.count(); ++i )
			list->addItem( QObject::tr( "cpu %1" ).arg( dev.CPU_List.at( i ).Caption ) );
		for( int i = 0; i < dev.Network_Card_List.count(); ++i )
			list->addItem( QObject::tr( "nic %1" ).arg( dev.Network_Card_List.at( i ).Caption ) );
		for( int i = 0; i < dev.Video_Card_List.count(); ++i )
			list->addItem( QObject::tr( "video %1" ).arg( dev.Video_Card_List.at( i ).Caption ) );
	}
	lay->addWidget( list );
	dlg.resize( 480, 420 );
	dlg.exec();
}

void AQ_Lab_Health( QWidget *parent, Virtual_Machine *vm )
{
	QString report;
	const QString bin = vm ? vm->Get_Current_Emulator_Binary_Path( vm->Get_Computer_Type() ) : QString();
	if( ! bin.isEmpty() )
	{
		int code = 0;
		report += Run_Tool( bin, QStringList() << QStringLiteral( "-accel" ) << QStringLiteral( "help" ), 4000, &code );
	}
#ifdef Q_OS_WIN
	int code = 0;
	report += QLatin1Char( '\n' );
	report += Run_Tool( QStringLiteral( "reg" ), QStringList() << QStringLiteral( "query" )
		<< QStringLiteral( "HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard" )
		<< QStringLiteral( "/v" ) << QStringLiteral( "EnableVirtualizationBasedSecurity" ), 4000, &code );
#endif
#ifdef Q_OS_WIN
	const bool failed = report.contains( QLatin1String( "whpx" ), Qt::CaseInsensitive ) == false && ! bin.isEmpty();
#else
	const bool failed = false;
#endif
	QSettings settings;
	settings.setValue( QStringLiteral( "Lab/Whpx_Probe_Failed" ), failed );
	QMessageBox box( parent );
	box.setWindowTitle( QObject::tr( "Hyper-V / WHPX health" ) );
	box.setText( report.left( 1800 ) );
	box.setInformativeText( QObject::tr( "Optional Features: Hyper-V, Virtual Machine Platform, Windows Hypervisor Platform. Core isolation is Memory Integrity. A failed probe offers Force TCG the next time a VM starts." ) );
	QPushButton *features = box.addButton( QObject::tr( "Optional Features" ), QMessageBox::ActionRole );
	QPushButton *core = box.addButton( QObject::tr( "Core isolation" ), QMessageBox::ActionRole );
	box.addButton( QMessageBox::Close );
	box.exec();
	if( box.clickedButton() == features )
		QDesktopServices::openUrl( QUrl( QStringLiteral( "ms-settings:optionalfeatures" ) ) );
	else if( box.clickedButton() == core )
		QDesktopServices::openUrl( QUrl( QStringLiteral( "ms-settings:windowsdefender" ) ) );
}

void AQ_Lab_Auditor( QWidget *parent )
{
	QString text;
	QDir dir( QCoreApplication::applicationDirPath() );
	const QStringList bins = dir.entryList( QStringList() << QStringLiteral( "qemu-system-*" ), QDir::Files );
	text += QObject::tr( "qemu-system binaries next to AQEMU: %1 (expect at least 25 in a full bundle)\n" ).arg( bins.count() );
	text += QFileInfo( dir.filePath( QStringLiteral( "qemu-img" ) ) ).exists() ||
	        QFileInfo( dir.filePath( QStringLiteral( "qemu-img.exe" ) ) ).exists()
		? QObject::tr( "qemu-img: present\n" ) : QObject::tr( "qemu-img: missing\n" );
	const QStringList fw = QStringList() << QStringLiteral( "share/qemu/bios-256k.bin" ) << QStringLiteral( "share/bios-256k.bin" );
	bool have_fw = false;
	for( int i = 0; i < fw.count(); ++i )
	{
		if( QFileInfo( dir.filePath( fw.at( i ) ) ).exists() )
			have_fw = true;
	}
	text += have_fw ? QObject::tr( "Firmware: present\n" ) : QObject::tr( "Firmware: bios-256k.bin was not next to AQEMU\n" );
	QMessageBox::information( parent, QObject::tr( "QEMU bundle" ), text );
}

void AQ_Lab_Snippets( Virtual_Machine *vm, QWidget *parent )
{
	if( ! vm )
		return;
	QDialog dlg( parent );
	dlg.setWindowTitle( QObject::tr( "Argument snippets" ) );
	QVBoxLayout *lay = new QVBoxLayout( &dlg );
	QListWidget *list = new QListWidget( &dlg );
	QMap<QString, QString> snippets;
	snippets.insert( QObject::tr( "Nested virtualization" ), QStringLiteral( "-cpu max,vmx=on" ) );
	snippets.insert( QObject::tr( "Debug console" ), QStringLiteral( "-debugcon stdio" ) );
	snippets.insert( QObject::tr( "XP vapic off" ), QStringLiteral( "-global apic.vapic=off" ) );
	for( QMap<QString, QString>::const_iterator it = snippets.constBegin(); it != snippets.constEnd(); ++it )
		list->addItem( it.key() );
	lay->addWidget( list );
	QPushButton *apply = new QPushButton( QObject::tr( "Append to additional arguments" ), &dlg );
	lay->addWidget( apply );
	QObject::connect( apply, &QPushButton::clicked, &dlg, [vm, list, &snippets]() {
		if( ! list->currentItem() )
			return;
		const QString extra = snippets.value( list->currentItem()->text() );
		const QString cur = vm->Get_Additional_Args().trimmed();
		vm->Set_Additional_Args( cur.isEmpty() ? extra : ( cur + QLatin1Char( ' ' ) + extra ) );
		vm->Save_VM();
	} );
	dlg.exec();
}

void AQ_Lab_NBD( QWidget *parent, Virtual_Machine *vm )
{
	if( vm && vm->Get_State() == VM::VMS_Running )
	{
		const int answer = QMessageBox::question( parent, QObject::tr( "qemu-nbd" ),
			QObject::tr( "This VM is running. Export only a snapshot, not the live read-write disk." ),
			QObject::tr( "I have a snapshot" ), QObject::tr( "Cancel" ) );
		if( answer != 0 )
			return;
	}
	const QString disk = QFileDialog::getOpenFileName( parent, QObject::tr( "Disk to export" ),
		vm ? vm->Get_HDA().Get_File_Name() : QString() );
	if( disk.isEmpty() )
		return;
	QDialog dlg( parent );
	dlg.setWindowTitle( QObject::tr( "qemu-nbd" ) );
	QVBoxLayout *lay = new QVBoxLayout( &dlg );
	lay->addWidget( new QLabel( QObject::tr( "Read-only export on 127.0.0.1:10809. Close this window to stop it." ), &dlg ) );
	QProcess proc( &dlg );
	proc.start( QStringLiteral( "qemu-nbd" ), QStringList()
		<< QStringLiteral( "--read-only" )
		<< QStringLiteral( "-b" ) << QStringLiteral( "127.0.0.1" )
		<< QStringLiteral( "-p" ) << QStringLiteral( "10809" )
		<< disk );
	QObject::connect( &dlg, &QDialog::finished, &dlg, [&proc]() {
		if( proc.state() == QProcess::NotRunning )
			return;
		proc.terminate();
		if( ! proc.waitForFinished( 2000 ) )
			proc.kill();
	} );
	dlg.exec();
}

void AQ_Lab_Pack( QWidget *parent, Virtual_Machine *vm, bool do_import )
{
	if( do_import )
	{
		const QString path = QFileDialog::getOpenFileName( parent, QObject::tr( "Import lab pack" ), QString(), QStringLiteral( "*.xml" ) );
		if( path.isEmpty() )
			return;
		QMessageBox::information( parent, QObject::tr( "Lab pack" ),
			QObject::tr( "Selected %1. Open it with the existing VM list import if it is an AQEMU XML file. Disks stay linked, not embedded." ).arg( path ) );
		return;
	}
	if( ! vm )
		return;
	const QString path = QFileDialog::getSaveFileName( parent, QObject::tr( "Export lab pack" ),
		vm->Get_Machine_Name() + QStringLiteral( ".xml" ) );
	if( path.isEmpty() )
		return;
	vm->Save_VM( path );
	QFile file( path );
	QByteArray data;
	if( file.open( QIODevice::ReadOnly ) )
		data = file.readAll();
	QFile sum( path + QStringLiteral( ".sha256" ) );
	if( sum.open( QIODevice::WriteOnly ) )
		sum.write( QCryptographicHash::hash( data, QCryptographicHash::Sha256 ).toHex() );
	QMessageBox::information( parent, QObject::tr( "Lab pack" ),
		QObject::tr( "Wrote the VM XML and a checksum. Disk files stay where they are." ) );
}

void AQ_Lab_Quickemu( QWidget *parent, Virtual_Machine *vm )
{
	if( ! vm )
		return;
	const QString path = QFileDialog::getOpenFileName( parent, QObject::tr( "Quickemu conf" ), QString(), QStringLiteral( "*.conf" ) );
	if( path.isEmpty() )
		return;
	QFile file( path );
	if( ! file.open( QIODevice::ReadOnly ) )
		return;
	QString extra;
	while( ! file.atEnd() )
	{
		const QString line = QString::fromUtf8( file.readLine() ).trimmed();
		if( line.startsWith( QLatin1String( "qemu_args=" ) ) || line.startsWith( QLatin1String( "extra=" ) ) )
			extra += QLatin1Char( ' ' ) + line.section( QLatin1Char( '=' ), 1 );
	}
	if( path.contains( QLatin1String( "windows" ), Qt::CaseInsensitive ) )
	{
		vm->Set_TPM_Type( QStringLiteral( "emulator" ) );
		vm->Use_UEFI( true );
	}
	const QString cur = vm->Get_Additional_Args().trimmed();
	vm->Set_Additional_Args( cur + extra );
	vm->Save_VM();
	QMessageBox::information( parent, QObject::tr( "Quickemu" ),
		QObject::tr( "Imported arguments from the conf. No ISO was downloaded." ) );
}

void AQ_Lab_Snapshot_Timeline( Virtual_Machine *vm, QWidget *parent )
{
	if( ! vm )
		return;
	if( vm->Get_State() == VM::VMS_Running )
		AQGraphic_Warning( QObject::tr( "Snapshots" ), QObject::tr( "This VM is running. An internal savevm can stall I/O. An overlay does not replace the running disk until you shut down." ) );
	QDialog dlg( parent );
	dlg.setWindowTitle( QObject::tr( "Snapshot timeline" ) );
	QVBoxLayout *lay = new QVBoxLayout( &dlg );
	QPlainTextEdit *view = new QPlainTextEdit( &dlg );
	const QString disk = vm->Get_HDA().Get_File_Name();
	int code = 0;
	view->setPlainText( Run_Tool( Find_Qemu_Img( vm ), QStringList() << QStringLiteral( "snapshot" ) << QStringLiteral( "-l" ) << disk, 8000, &code ) );
	QLineEdit *note = new QLineEdit( &dlg );
	note->setPlaceholderText( QObject::tr( "Note for the next overlay" ) );
	QPushButton *make = new QPushButton( QObject::tr( "Create overlay and use it" ), &dlg );
	QPushButton *internal = new QPushButton( QObject::tr( "Internal savevm" ), &dlg );
	QPushButton *classic = new QPushButton( QObject::tr( "Classic snapshot list" ), &dlg );
	lay->addWidget( view );
	lay->addWidget( note );
	lay->addWidget( make );
	lay->addWidget( internal );
	lay->addWidget( classic );
	QObject::connect( make, &QPushButton::clicked, &dlg, [vm, disk, note, view]() {
		if( vm->Get_State() == VM::VMS_Running )
		{
			view->appendPlainText( QObject::tr( "Shut the VM down before replacing its disk with an overlay." ) );
			return;
		}
		const QString dest = disk + QStringLiteral( "." ) + QDateTime::currentDateTime().toString( QStringLiteral( "yyyyMMddhhmmss" ) ) + QStringLiteral( ".qcow2" );
		int code = 0;
		const QString out = Run_Tool( Find_Qemu_Img( vm ), QStringList() << QStringLiteral( "create" ) << QStringLiteral( "-f" ) << QStringLiteral( "qcow2" )
			<< QStringLiteral( "-b" ) << disk << QStringLiteral( "-F" ) << QStringLiteral( "qcow2" ) << dest, 20000, &code );
		if( code != 0 )
		{
			view->appendPlainText( out.left( 800 ) );
			return;
		}
		QFile notes( dest + QStringLiteral( ".txt" ) );
		if( notes.open( QIODevice::WriteOnly ) )
			notes.write( note->text().toUtf8() );
		VM_HDD hda = vm->Get_HDA();
		hda.Set_File_Name( dest );
		VM_Native_Storage_Device native = hda.Get_Native_Device();
		native.Use_File_Path( true );
		native.Set_File_Path( dest );
		hda.Set_Native_Device( native );
		vm->Set_HDA( hda );
		AQ_Lab_Set( vm, QStringLiteral( "check_chain" ), QStringLiteral( "on" ) );
		vm->Save_VM();
		view->appendPlainText( QObject::tr( "This VM now boots %1." ).arg( dest ) );
	} );
	QObject::connect( internal, &QPushButton::clicked, &dlg, [vm, note, view]() {
		if( ! vm->Get_QMP() || ! vm->Get_QMP()->Is_Connected() )
		{
			view->appendPlainText( QObject::tr( "Start the VM and wait for QMP before an internal savevm." ) );
			return;
		}
		QString tag = note->text().trimmed();
		if( tag.isEmpty() )
			tag = QStringLiteral( "aqemu" );
		tag.replace( QLatin1Char( ' ' ), QLatin1Char( '_' ) );
		vm->Get_QMP()->Human_Monitor( QStringLiteral( "savevm " ) + tag );
		view->appendPlainText( QObject::tr( "Sent savevm %1." ).arg( tag ) );
	} );
	QObject::connect( classic, &QPushButton::clicked, &dlg, [vm, parent]() {
		Snapshots_Window snapshot_win( parent );
		snapshot_win.Set_VM( vm );
		snapshot_win.exec();
	} );
	dlg.exec();
}

void AQ_Lab_Command_Diff( Virtual_Machine *vm, QWidget *parent )
{
	if( ! vm )
		return;
	QSettings settings;
	const QString previous = settings.value( QStringLiteral( "Lab/Last_Command/" ) + vm->Get_UID() ).toString();
	const QString now = vm->Build_QEMU_Args().join( QStringLiteral( "\n" ) );
	QDialog dlg( parent );
	dlg.setWindowTitle( QObject::tr( "Command diff" ) );
	dlg.resize( 700, 480 );
	QVBoxLayout *lay = new QVBoxLayout( &dlg );
	QPlainTextEdit *view = new QPlainTextEdit( &dlg );
	view->setPlainText( QObject::tr( "Previous launch\n%1\n\nCurrent\n%2\n\nAdditional arguments\n%3" )
		.arg( previous, now, vm->Get_Additional_Args() ) );
	lay->addWidget( view );
	QPushButton *exp = new QPushButton( QObject::tr( "Export shell script" ), &dlg );
	lay->addWidget( exp );
	QObject::connect( exp, &QPushButton::clicked, &dlg, [vm, &dlg]() {
		const QString path = QFileDialog::getSaveFileName( &dlg, QObject::tr( "Script" ) );
		if( path.isEmpty() )
			return;
		QFile file( path );
		if( file.open( QIODevice::WriteOnly ) )
		{
			QTextStream ts( &file );
			ts << "#!/bin/sh\nexec " << vm->Build_QEMU_Args_For_Script().join( QStringLiteral( " " ) ) << "\n";
		}
	} );
	dlg.exec();
}

void AQ_Lab_Block_Jobs( Virtual_Machine *vm, QWidget *parent )
{
	if( ! vm || ! vm->Get_QMP() || ! vm->Get_QMP()->Is_Connected() )
	{
		AQGraphic_Warning( QObject::tr( "Block jobs" ), QObject::tr( "Start the VM and wait for QMP." ) );
		return;
	}
	QDialog dlg( parent );
	dlg.setWindowTitle( QObject::tr( "Block jobs" ) );
	QFormLayout *form = new QFormLayout( &dlg );
	QLineEdit *node = new QLineEdit( QStringLiteral( "disk0" ), &dlg );
	QLineEdit *target = new QLineEdit( &dlg );
	QPushButton *stream = new QPushButton( QObject::tr( "block-stream" ), &dlg );
	QPushButton *commit = new QPushButton( QObject::tr( "block-commit" ), &dlg );
	QPushButton *mirror = new QPushButton( QObject::tr( "drive-mirror" ), &dlg );
	QPushButton *cancel = new QPushButton( QObject::tr( "Cancel job" ), &dlg );
	QLineEdit *bitmap = new QLineEdit( QStringLiteral( "bitmap0" ), &dlg );
	QPushButton *bitmap_add = new QPushButton( QObject::tr( "Add dirty bitmap" ), &dlg );
	QPushButton *backup = new QPushButton( QObject::tr( "drive-backup" ), &dlg );
	form->addRow( QObject::tr( "Node" ), node );
	form->addRow( QObject::tr( "Target" ), target );
	form->addRow( stream );
	form->addRow( commit );
	form->addRow( mirror );
	form->addRow( cancel );
	QObject::connect( stream, &QPushButton::clicked, &dlg, [vm, node]() {
		QJsonObject args;
		args.insert( QStringLiteral( "device" ), node->text() );
		vm->Get_QMP()->Send_Command( QStringLiteral( "block-stream" ), args );
	} );
	QObject::connect( commit, &QPushButton::clicked, &dlg, [vm, node]() {
		QJsonObject args;
		args.insert( QStringLiteral( "device" ), node->text() );
		vm->Get_QMP()->Send_Command( QStringLiteral( "block-commit" ), args );
	} );
	QObject::connect( mirror, &QPushButton::clicked, &dlg, [vm, node, target]() {
		QJsonObject args;
		args.insert( QStringLiteral( "device" ), node->text() );
		args.insert( QStringLiteral( "target" ), target->text() );
		args.insert( QStringLiteral( "sync" ), QStringLiteral( "full" ) );
		vm->Get_QMP()->Send_Command( QStringLiteral( "drive-mirror" ), args );
	} );
	QObject::connect( cancel, &QPushButton::clicked, &dlg, [vm, node]() {
		QJsonObject args;
		args.insert( QStringLiteral( "device" ), node->text() );
		vm->Get_QMP()->Send_Command( QStringLiteral( "block-job-cancel" ), args );
	} );
	form->addRow( QObject::tr( "Bitmap" ), bitmap );
	form->addRow( bitmap_add );
	form->addRow( backup );
	QObject::connect( bitmap_add, &QPushButton::clicked, &dlg, [vm, node, bitmap]() {
		QJsonObject args;
		args.insert( QStringLiteral( "node" ), node->text() );
		args.insert( QStringLiteral( "name" ), bitmap->text() );
		vm->Get_QMP()->Send_Command( QStringLiteral( "block-dirty-bitmap-add" ), args );
	} );
	QObject::connect( backup, &QPushButton::clicked, &dlg, [vm, node, target]() {
		QJsonObject args;
		args.insert( QStringLiteral( "device" ), node->text() );
		args.insert( QStringLiteral( "target" ), target->text() );
		args.insert( QStringLiteral( "sync" ), QStringLiteral( "full" ) );
		vm->Get_QMP()->Send_Command( QStringLiteral( "drive-backup" ), args );
	} );
	dlg.exec();
}

void AQ_Lab_Migrate( Virtual_Machine *vm, QWidget *parent )
{
	if( ! vm || ! vm->Get_QMP() || ! vm->Get_QMP()->Is_Connected() )
	{
		AQGraphic_Warning( QObject::tr( "Migration" ), QObject::tr( "Start the VM and wait for QMP." ) );
		return;
	}
	QDialog dlg( parent );
	QFormLayout *form = new QFormLayout( &dlg );
	QLineEdit *uri = new QLineEdit( vm->Get_Incoming_URI(), &dlg );
	QSpinBox *bw = new QSpinBox( &dlg );
	bw->setRange( 0, 100000 );
	bw->setValue( 0 );
	QPushButton *go = new QPushButton( QObject::tr( "Migrate" ), &dlg );
	QPushButton *cancel = new QPushButton( QObject::tr( "Cancel" ), &dlg );
	QPushButton *query = new QPushButton( QObject::tr( "Query progress" ), &dlg );
	form->addRow( QObject::tr( "URI" ), uri );
	form->addRow( QObject::tr( "Bandwidth MB/s (0 = default)" ), bw );
	form->addRow( go );
	form->addRow( query );
	form->addRow( cancel );
	QObject::connect( go, &QPushButton::clicked, &dlg, [vm, uri, bw]() {
		if( bw->value() > 0 )
		{
			QJsonObject params;
			params.insert( QStringLiteral( "max-bandwidth" ), bw->value() * 1024 * 1024 );
			vm->Get_QMP()->Send_Command( QStringLiteral( "migrate-set-parameters" ), params );
		}
		vm->Get_QMP()->Migrate( uri->text().trimmed() );
	} );
	QObject::connect( query, &QPushButton::clicked, &dlg, [vm]() { vm->Get_QMP()->Query_Migrate(); } );
	QObject::connect( cancel, &QPushButton::clicked, &dlg, [vm]() { vm->Get_QMP()->Migrate_Cancel(); } );
	dlg.exec();
}

void AQ_Lab_WSL( QWidget *parent )
{
	int code = 0;
	const QString distros = Run_Tool( QStringLiteral( "wsl" ), QStringList() << QStringLiteral( "-l" ) << QStringLiteral( "-q" ), 8000, &code );
	const bool kvm = WSL_Has_KVM( QString(), true );
	QMessageBox box( parent );
	box.setWindowTitle( QObject::tr( "WSL" ) );
	box.setText( QObject::tr( "Distros:\n%1\n/dev/kvm: %2" ).arg( distros.left( 800 ) ).arg( kvm ? QObject::tr( "present" ) : QObject::tr( "not confirmed" ) ) );
	QPushButton *repair = box.addButton( QObject::tr( "Repair KVM access" ), QMessageBox::ActionRole );
	box.addButton( QMessageBox::Close );
	box.exec();
	if( box.clickedButton() == repair )
		WSL_Ensure_KVM_Access( QString() );
}

void AQ_Lab_Firmware( Virtual_Machine *vm, QWidget *parent )
{
	if( ! vm )
		return;
	const QString code = QFileDialog::getOpenFileName( parent, QObject::tr( "OVMF CODE" ) );
	if( code.isEmpty() )
		return;
	const QString vars = QFileDialog::getSaveFileName( parent, QObject::tr( "Private VARS copy" ) );
	if( vars.isEmpty() )
		return;
	const QString src_vars = QFileDialog::getOpenFileName( parent, QObject::tr( "Template VARS" ) );
	if( ! src_vars.isEmpty() )
		QFile::copy( src_vars, vars );
	vm->Use_UEFI( true );
	vm->Set_UEFI_CODE_File( code );
	vm->Set_UEFI_VARS_File( vars );
	vm->Save_VM();
}

void AQ_Lab_First_Run( QWidget *parent )
{
	QDialog dlg( parent );
	dlg.setWindowTitle( QObject::tr( "Start with a scenario" ) );
	QVBoxLayout *lay = new QVBoxLayout( &dlg );
	lay->addWidget( new QLabel( QObject::tr( "Each card opens the existing wizard. Accelerated macOS is not offered." ), &dlg ) );
	const QStringList cards = QStringList()
		<< QObject::tr( "Windows 11 on ARM" )
		<< QObject::tr( "TrueNAS 4Kn" )
		<< QObject::tr( "iOS (Inferno)" )
		<< QObject::tr( "Retro Windows 98" )
		<< QObject::tr( "Import OVA" );
	const QStringList os = QStringList()
		<< QStringLiteral( "Windows 11" )
		<< QStringLiteral( "TrueNAS SCALE" )
		<< QStringLiteral( "iOS (ARM64)" )
		<< QStringLiteral( "Windows 98" )
		<< QString();
	for( int i = 0; i < cards.count(); ++i )
	{
		QPushButton *btn = new QPushButton( cards.at( i ), &dlg );
		lay->addWidget( btn );
		const QString name = os.at( i );
		const bool appliance = cards.at( i ).contains( QLatin1String( "OVA" ) );
		QObject::connect( btn, &QPushButton::clicked, &dlg, [parent, name, appliance, &dlg]() {
			VM_Wizard_Window *wizard = new VM_Wizard_Window( parent );
			if( appliance )
				wizard->Select_Appliance();
			else
				wizard->Select_Guest_OS( name );
			dlg.accept();
			wizard->exec();
			wizard->deleteLater();
		} );
	}
	dlg.exec();
}

QList<Virtual_Machine*> AQ_Lab_Linked_Clones( Virtual_Machine *base, QWidget *parent )
{
	QList<Virtual_Machine*> created;
	if( ! base )
		return created;
	bool ok = false;
	const int count = QInputDialog::getInt( parent, QObject::tr( "Linked clones" ), QObject::tr( "How many overlays?" ), 2, 1, 20, 1, &ok );
	if( ! ok )
		return created;
	qint64 ram = base->Get_Memory_Size();
	ram *= count;
	if( ram > 16384 )
		AQGraphic_Warning( QObject::tr( "Linked clones" ), QObject::tr( "These clones request about %1 MB of RAM together." ).arg( ram ) );
	const QString disk = base->Get_HDA().Get_File_Name();
	for( int i = 1; i <= count; ++i )
	{
		Virtual_Machine *vm = new Virtual_Machine( *base );
		const QString name = base->Get_Machine_Name() + QStringLiteral( "-" ) + QString::number( i );
		vm->Set_Machine_Name( name );
		vm->Set_UID( QUuid::createUuid().toString() );
		vm->Set_UUID( QUuid::createUuid().toString() );
		const QString overlay = disk + QStringLiteral( "." ) + name + QStringLiteral( ".qcow2" );
		int code = 0;
		Run_Tool( Find_Qemu_Img( base ), QStringList() << QStringLiteral( "create" ) << QStringLiteral( "-f" ) << QStringLiteral( "qcow2" )
			<< QStringLiteral( "-b" ) << disk << QStringLiteral( "-F" ) << QStringLiteral( "qcow2" ) << overlay, 20000, &code );
		if( code == 0 )
		{
			VM_HDD hda = vm->Get_HDA();
			hda.Set_File_Name( overlay );
			VM_Native_Storage_Device native = hda.Get_Native_Device();
			native.Use_File_Path( true );
			native.Set_File_Path( overlay );
			native.Set_Disk_Serial( QStringLiteral( "aqemu-%1" ).arg( i, 2, 10, QLatin1Char( '0' ) ) );
			hda.Set_Native_Device( native );
			vm->Set_HDA( hda );
		}
		QList<VM_Net_Card_Native> cards = vm->Get_Network_Cards_Nativ();
		if( ! cards.isEmpty() )
		{
			cards[ 0 ].Set_MAC_Address( QStringLiteral( "52:54:00:%1:%2:%3" )
				.arg( ( i >> 8 ) & 0xff, 2, 16, QLatin1Char( '0' ) )
				.arg( i & 0xff, 2, 16, QLatin1Char( '0' ) )
				.arg( ( i * 3 ) & 0xff, 2, 16, QLatin1Char( '0' ) ) );
			vm->Set_Network_Cards_Nativ( cards );
		}
		vm->Set_SMBIOS_Serial( QStringLiteral( "aqemu-clone-%1" ).arg( i ) );
		AQ_Lab_Set( vm, QStringLiteral( "tag" ), base->Get_Machine_Name() );
		AQ_Lab_Set( vm, QStringLiteral( "check_chain" ), QStringLiteral( "on" ) );
		created << vm;
	}
	return created;
}

void AQ_Lab_Start_Group( const QList<Virtual_Machine*> &vms, QWidget *parent, bool stop )
{
	bool ok = false;
	const QString tag = QInputDialog::getText( parent, QObject::tr( "VM group" ), QObject::tr( "Tag" ), QLineEdit::Normal, QString(), &ok );
	if( ! ok || tag.isEmpty() )
		return;
	qint64 ram = 0;
	for( int i = 0; i < vms.count(); ++i )
	{
		if( AQ_Lab_Get( vms.at( i ), QStringLiteral( "tag" ) ) == tag )
			ram += vms.at( i )->Get_Memory_Size();
	}
	if( ! stop && ram > 16384 )
		AQGraphic_Warning( QObject::tr( "VM group" ), QObject::tr( "Tagged VMs request about %1 MB of RAM." ).arg( ram ) );
	for( int i = 0; i < vms.count(); ++i )
	{
		if( AQ_Lab_Get( vms.at( i ), QStringLiteral( "tag" ) ) != tag )
			continue;
		if( stop )
			AQEMU_Service::get().call( QStringLiteral( "stop" ), vms.at( i ) );
		else
			AQEMU_Service::get().call( QStringLiteral( "start" ), vms.at( i ) );
	}
}
