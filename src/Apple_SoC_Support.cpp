#include "Apple_SoC_Support.h"
#include "Apple_SoC_FS_Patch_Window.h"
#include "VM.h"
#include "Utils.h"
#include "WSL_Launch.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStorageInfo>
#include <QObject>
#include <QProcess>
#include <QMessageBox>
#include <QWidget>
#include <QComboBox>
#include <QSpinBox>

#include <cstring>

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

QString AQ_Apple_SoC_Suggested_Append()
{
	return QStringLiteral(
		"tlto_us=-1 mtxspin=-1 agm-genuine=1 agm-authentic=1 agm-trusted=1 "
		"serial=3 wdt=-1 -vm_compressor_wk_sw" );
}

bool AQ_Validate_Apple_SoC_Boot_Files( const Virtual_Machine *vm, QString *error_out )
{
	if( ! vm )
	{
		if( error_out )
			*error_out = QObject::tr( "No virtual machine." );
		return false;
	}

	auto fail = [&]( const QString &msg ) -> bool {
		if( error_out )
			*error_out = msg;
		return false;
	};

	auto require_file = [&]( const QString &label, const QString &path ) -> bool {
		const QString p = path.trimmed();
		if( p.isEmpty() )
		{
			return fail( QObject::tr(
				"Missing %1.\n\n"
				"On the MACHINE tab, scroll to Options and choose the file "
				"for \"%1\"." ).arg( label ) );
		}
		if( ! QFile::exists( p ) )
		{
			return fail( QObject::tr(
				"%1 file not found:\n%2\n\n"
				"Pick an existing file in Options on the MACHINE tab." )
				.arg( label, p ) );
		}
		return true;
	};

	if( vm->Get_Machine_Type().trimmed().isEmpty() )
	{
		return fail( QObject::tr(
			"Choose a Machine type on the MACHINE tab "
			"(for Inferno, typically t8030 / iphone11 / similar)." ) );
	}

	if( ! require_file( QObject::tr( "Kernel" ), vm->Get_App_Kernel_Path() ) )
		return false;
	if( ! require_file( QObject::tr( "DeviceTree" ), vm->Get_DeviceTree_Path() ) )
		return false;

	// Boot Arguments (-append) are optional: leave blank, or type guest boot args
	// in that MACHINE-tab field. Never invent them here at launch.

	const QString securerom = vm->Get_Apple_SecureROM_Path().trimmed();
	if( ! securerom.isEmpty() )
	{
		if( ! require_file( QObject::tr( "SecureROM (optional)" ), securerom ) )
			return false;
	}
	else if( ! require_file( QObject::tr( "Trustcache" ), vm->Get_Apple_Trustcache_Path() ) )
	{
		return false;
	}

	const QString ticket = vm->Get_Apple_Ticket_Path().trimmed();
	if( ! ticket.isEmpty() && ! QFile::exists( ticket ) )
	{
		return fail( QObject::tr(
			"Restore ticket path is set but the file was not found:\n%1\n\n"
			"Fix or clear \"Restore ticket\" under Options on the MACHINE tab." )
			.arg( ticket ) );
	}

	const QString sep_fw = vm->Get_Apple_SEP_FW_Path().trimmed();
	const QString sep_rom = vm->Get_Apple_SEP_ROM_Path().trimmed();
	if( sep_fw.isEmpty() != sep_rom.isEmpty() )
	{
		return fail( QObject::tr(
			"SEP firmware and SEP ROM must both be set, or both left empty.\n"
			"Use the \"SEP firmware\" and \"SEP ROM\" fields under Options on the MACHINE tab." ) );
	}
	if( ! sep_fw.isEmpty() )
	{
		if( ! require_file( QObject::tr( "SEP firmware" ), sep_fw ) )
			return false;
		if( ! require_file( QObject::tr( "SEP ROM" ), sep_rom ) )
			return false;
	}

	const QString usb_type = vm->Get_Apple_USB_Conn_Type().trimmed().toLower();
	if( ! usb_type.isEmpty() )
	{
		if( vm->Get_Apple_USB_Conn_Addr().trimmed().isEmpty() )
		{
			return fail( QObject::tr(
				"USB remote type is set, but the address is empty.\n"
				"Fill \"USB remote\" under Options on the MACHINE tab "
				"(recommended: IPv4 127.0.0.1 port 8030, or UNIX /tmp/InfernoUSBRemote)." ) );
		}
		if( usb_type == QLatin1String( "ipv4" ) && vm->Get_Apple_USB_Conn_Port() <= 0 )
		{
			return fail( QObject::tr(
				"USB remote is IPv4 but the TCP port is missing.\n"
				"Set the port next to \"USB remote\" on the MACHINE tab." ) );
		}
	}

	return true;
}

QString AQ_Apple_SoC_WSL_Qemu_Binary()
{
	QSettings s;
	const QString override = s.value( QStringLiteral( "WSL_Launch/AppleSoC_Binary" ), QString() ).toString().trimmed();
	if( ! override.isEmpty() )
		return override;
	return QStringLiteral( "/usr/local/bin/qemu-system-applesoc" );
}

QString AQ_Apple_SoC_Image_Dir( const Virtual_Machine *vm )
{
	if( ! vm )
		return QDir::currentPath();
	const QFileInfo fi( vm->Get_VM_XML_File_Path() );
	QString base = fi.completeBaseName();
	if( base.isEmpty() )
		base = QStringLiteral( "applesoc_vm" );
	QDir parent = fi.dir();
	if( ! parent.exists() )
		parent = QDir::current();
	return parent.filePath( base + QStringLiteral( "_inferno" ) );
}

QString AQ_Apple_SoC_QEMU_Log_Path( const Virtual_Machine *vm )
{
	return QDir( AQ_Apple_SoC_Image_Dir( vm ) ).filePath( QStringLiteral( "qemu-boot.log" ) );
}

static bool Ensure_Raw_Image( const QString &path, qint64 size_bytes, QString *error_out )
{
	QFileInfo fi( path );
	if( fi.exists() && fi.size() >= size_bytes )
		return true;

	QFile f( path );
	if( ! f.open( QIODevice::ReadWrite ) )
	{
		if( error_out )
			*error_out = QObject::tr( "Cannot create %1" ).arg( path );
		return false;
	}
	f.close();

#ifdef Q_OS_WIN32
	// Mark sparse before sizing so multi-GiB logical size does not fill the disk.
	QProcess fsutil;
	fsutil.start( QStringLiteral( "fsutil" ),
		QStringList() << QStringLiteral( "sparse" ) << QStringLiteral( "setflag" ) << path );
	fsutil.waitForFinished( 5000 );
#endif

	if( ! f.open( QIODevice::ReadWrite ) )
	{
		if( error_out )
			*error_out = QObject::tr( "Cannot reopen %1" ).arg( path );
		return false;
	}

	if( ! f.resize( size_bytes ) )
	{
		f.close();
		if( QFileInfo( path ).size() == 0 )
			QFile::remove( path );
		if( error_out )
		{
			*error_out = QObject::tr(
				"Cannot size %1 to %2 MiB.\n"
				"Free some disk space, or replace this file with a larger image." )
				.arg( path )
				.arg( size_bytes / ( 1024 * 1024 ) );
		}
		return false;
	}
	f.close();
	return true;
}

static const char kSeedPartName[] = "AQEMU_SEED";
static const int kNvmeLba = 4096;
static const quint64 kSeedPartStartLba = 256;

static quint32 Gpt_Crc32( const char *data, int len )
{
	quint32 crc = 0xFFFFFFFFu;
	for( int i = 0; i < len; ++i )
	{
		crc ^= static_cast<quint8>( data[ i ] );
		for( int b = 0; b < 8; ++b )
			crc = ( crc >> 1 ) ^ ( ( crc & 1u ) ? 0xEDB88320u : 0u );
	}
	return ~crc;
}

static void Put_Le32( char *p, quint32 v )
{
	p[ 0 ] = char( v & 0xFFu );
	p[ 1 ] = char( ( v >> 8 ) & 0xFFu );
	p[ 2 ] = char( ( v >> 16 ) & 0xFFu );
	p[ 3 ] = char( ( v >> 24 ) & 0xFFu );
}

static void Put_Le64( char *p, quint64 v )
{
	for( int i = 0; i < 8; ++i )
		p[ i ] = char( ( v >> ( 8 * i ) ) & 0xFFu );
}

static quint64 Get_Le64( const char *p )
{
	quint64 v = 0;
	for( int i = 0; i < 8; ++i )
		v |= quint64( quint8( p[ i ] ) ) << ( 8 * i );
	return v;
}

static void Put_Gpt_Guid( char *dst, const quint8 rfc[ 16 ] )
{
	dst[ 0 ] = char( rfc[ 3 ] );
	dst[ 1 ] = char( rfc[ 2 ] );
	dst[ 2 ] = char( rfc[ 1 ] );
	dst[ 3 ] = char( rfc[ 0 ] );
	dst[ 4 ] = char( rfc[ 5 ] );
	dst[ 5 ] = char( rfc[ 4 ] );
	dst[ 6 ] = char( rfc[ 7 ] );
	dst[ 7 ] = char( rfc[ 6 ] );
	std::memcpy( dst + 8, rfc + 8, 8 );
}

static bool Efi_Part_At( QFile &f, qint64 off, qint64 sz )
{
	if( off < 0 || off + 8 > sz )
		return false;
	if( ! f.seek( off ) )
		return false;
	return f.read( 8 ) == QByteArrayLiteral( "EFI PART" );
}

static QString Gpt_First_Part_Name( QFile &f, qint64 header_off, qint64 lba )
{
	if( ! f.seek( header_off + 72 ) )
		return QString();
	const QByteArray lb = f.read( 8 );
	if( lb.size() != 8 )
		return QString();
	const qint64 name_off =
		qint64( Get_Le64( lb.constData() ) ) * lba + 56;
	if( name_off < 0 || ! f.seek( name_off ) )
		return QString();
	const QByteArray nb = f.read( 72 );
	QString s;
	for( int i = 0; i + 1 < nb.size(); i += 2 )
	{
		const ushort c = ushort( quint8( nb.at( i ) ) ) |
			ushort( quint8( nb.at( i + 1 ) ) << 8 );
		if( c == 0 )
			break;
		s.append( QChar( c ) );
	}
	return s;
}

static bool Root_Image_Has_Efi_Signature( const QString &path )
{
	QFile f( path );
	if( ! f.open( QIODevice::ReadOnly ) )
		return false;
	const qint64 sz = f.size();
	return Efi_Part_At( f, 512, sz ) || Efi_Part_At( f, kNvmeLba, sz );
}

static QString Root_Image_First_Part_Name( const QString &path )
{
	QFile f( path );
	if( ! f.open( QIODevice::ReadOnly ) )
		return QString();
	const qint64 sz = f.size();
	if( Efi_Part_At( f, kNvmeLba, sz ) )
		return Gpt_First_Part_Name( f, kNvmeLba, kNvmeLba );
	if( Efi_Part_At( f, 512, sz ) )
		return Gpt_First_Part_Name( f, 512, 512 );
	return QString();
}

static bool Partition_Has_Nxsb( const QString &path )
{
	QFile f( path );
	if( ! f.open( QIODevice::ReadOnly ) )
		return false;
	const qint64 off = qint64( kSeedPartStartLba ) * kNvmeLba + 32;
	if( ! f.seek( off ) )
		return false;
	return f.read( 4 ) == QByteArrayLiteral( "NXSB" );
}

static bool Root_Image_Needs_Placeholder_Gpt( const QString &path )
{
	if( Partition_Has_Nxsb( path ) )
		return false;
	if( ! Root_Image_Has_Efi_Signature( path ) )
		return true;
	const QString name = Root_Image_First_Part_Name( path );
	return name.isEmpty() || name == QLatin1String( kSeedPartName );
}

#ifdef Q_OS_WIN32
static bool Format_Seed_Apfs_Via_Wsl( const QString &path, QString *error_out )
{
	if( Partition_Has_Nxsb( path ) )
		return true;
	const QString extras = AQ_Inferno_Extras_Dir();
	const QString script = extras + QStringLiteral( "/format-seed-apfs.sh" );
	if( extras.isEmpty() || ! QFileInfo::exists( script ) )
	{
		if( error_out )
			*error_out = QObject::tr(
				"Cannot format seed APFS: extras/Inferno/format-seed-apfs.sh not found." );
		return false;
	}
	QSettings s;
	const QString distro = s.value( QStringLiteral( "WSL_Launch/Distro" ), QString() ).toString();
	const QString cmd = QStringLiteral( "tr -d '\\r' < '%1' | sh -s -- '%2'" )
				    .arg( Windows_Path_To_WSL( script ),
					  Windows_Path_To_WSL( QFileInfo( path ).absoluteFilePath() ) );
	if( ! WSL_Run_Privileged_Script( distro, cmd, 300000 ) )
	{
		AQWarning( "Format_Seed_Apfs_Via_Wsl",
			   QStringLiteral( "WSL mkapfs skipped or failed for %1" ).arg( path ) );
		return false;
	}
	if( ! Partition_Has_Nxsb( path ) )
	{
		if( error_out )
			*error_out = QObject::tr(
				"Seed APFS format ran but NXSB was not found on:\n%1" ).arg( path );
		return false;
	}
	return true;
}
#endif

// GPT + APFS Data volume so ramrod verify_storage_for_update can pass while
// CreateFilesystemPartitions stays false. AQEMU_SEED is not a finished restore.
int AQ_Default_Apple_SoC_Nand_GiB()
{
	return 32;
}

int AQ_Min_Apple_SoC_Nand_GiB()
{
	return 16;
}

int AQ_Max_Apple_SoC_Nand_GiB()
{
	return 2048;
}

int AQ_Clamp_Apple_SoC_Nand_GiB( int gib )
{
	if( gib < AQ_Min_Apple_SoC_Nand_GiB() )
		return AQ_Min_Apple_SoC_Nand_GiB();
	if( gib > AQ_Max_Apple_SoC_Nand_GiB() )
		return AQ_Max_Apple_SoC_Nand_GiB();
	return gib;
}

qint64 AQ_Apple_SoC_Nand_Bytes( int gib )
{
	return qint64( AQ_Clamp_Apple_SoC_Nand_GiB( gib ) ) * 1024LL * 1024LL * 1024LL;
}

static const int kNandCustom = -1;

static void Fill_Nand_Combo( QComboBox *cb )
{
	cb->clear();
	for( int s : { 16, 32, 64, 128, 256 } )
		cb->addItem( QObject::tr( "%1 GiB" ).arg( s ), s );
	cb->addItem( QObject::tr( "Custom…" ), kNandCustom );
}

void AQ_Setup_Apple_SoC_Nand_Spin( QSpinBox *spin )
{
	if( ! spin )
		return;
	spin->setRange( AQ_Min_Apple_SoC_Nand_GiB(), AQ_Max_Apple_SoC_Nand_GiB() );
	spin->setSingleStep( 1 );
	spin->setSuffix( QObject::tr( " GiB" ) );
	spin->setToolTip( QObject::tr(
		"Custom NAND size in GiB (16–2048). Sparse on Windows — logical size, not fully allocated." ) );
}

void AQ_Populate_Apple_SoC_Nand_Combo( QComboBox *cb, int current_gib )
{
	if( ! cb )
		return;
	AQ_Apply_Apple_SoC_Nand_Controls( cb, nullptr, current_gib );
}

void AQ_Apply_Apple_SoC_Nand_Controls( QComboBox *cb, QSpinBox *spin, int gib )
{
	const int cur = AQ_Clamp_Apple_SoC_Nand_GiB( gib <= 0 ? AQ_Default_Apple_SoC_Nand_GiB() : gib );
	if( cb )
	{
		cb->blockSignals( true );
		Fill_Nand_Combo( cb );
		int idx = cb->findData( cur );
		if( idx < 0 )
			idx = cb->findData( kNandCustom );
		cb->setCurrentIndex( idx >= 0 ? idx : 0 );
		cb->blockSignals( false );
	}
	if( spin )
	{
		spin->blockSignals( true );
		AQ_Setup_Apple_SoC_Nand_Spin( spin );
		spin->setValue( cur );
		const bool custom = cb && cb->currentData().toInt() == kNandCustom;
		spin->setEnabled( custom || ! cb );
		spin->blockSignals( false );
	}
}

int AQ_Read_Apple_SoC_Nand_Controls( const QComboBox *cb, const QSpinBox *spin )
{
	if( spin && spin->isEnabled() )
		return AQ_Clamp_Apple_SoC_Nand_GiB( spin->value() );
	if( cb )
	{
		const int d = cb->currentData().toInt();
		if( d > 0 )
			return AQ_Clamp_Apple_SoC_Nand_GiB( d );
	}
	if( spin )
		return AQ_Clamp_Apple_SoC_Nand_GiB( spin->value() );
	return AQ_Default_Apple_SoC_Nand_GiB();
}

void AQ_On_Apple_SoC_Nand_Combo_Changed( QComboBox *cb, QSpinBox *spin )
{
	if( ! cb || ! spin )
		return;
	const int d = cb->currentData().toInt();
	if( d > 0 )
	{
		spin->blockSignals( true );
		spin->setValue( d );
		spin->setEnabled( false );
		spin->blockSignals( false );
	}
	else
	{
		spin->setEnabled( true );
		spin->setFocus();
	}
}

void AQ_On_Apple_SoC_Nand_Spin_Changed( QComboBox *cb, QSpinBox *spin )
{
	if( ! cb || ! spin )
		return;
	const int v = AQ_Clamp_Apple_SoC_Nand_GiB( spin->value() );
	int idx = cb->findData( v );
	if( idx < 0 )
		idx = cb->findData( kNandCustom );
	cb->blockSignals( true );
	cb->setCurrentIndex( idx >= 0 ? idx : cb->count() - 1 );
	cb->blockSignals( false );
	spin->setEnabled( cb->currentData().toInt() == kNandCustom );
}

static bool Seed_Root_Placeholder_Gpt( const QString &path, QString *error_out, bool force = false )
{
	if( ! force && ! Root_Image_Needs_Placeholder_Gpt( path ) )
		return true;

	QFile f( path );
	if( ! f.open( QIODevice::ReadWrite ) )
	{
		if( error_out )
			*error_out = QObject::tr( "Cannot write GPT on %1" ).arg( path );
		return false;
	}
	const qint64 sz = f.size();
	if( sz < qint64( kNvmeLba ) * 64 )
	{
		if( error_out )
			*error_out = QObject::tr( "Root image too small to hold a GPT:\n%1" ).arg( path );
		return false;
	}
	if( sz % kNvmeLba != 0 )
	{
		if( error_out )
			*error_out = QObject::tr( "Root image size is not a multiple of 4096:\n%1" ).arg( path );
		return false;
	}

	const quint64 n_lba = quint64( sz / kNvmeLba );
	const quint64 array_bytes = 128ull * 128ull;
	const quint64 array_lbas = ( array_bytes + quint64( kNvmeLba ) - 1 ) / quint64( kNvmeLba );
	const quint64 first_usable = 2 + array_lbas;
	const quint64 last_usable = n_lba - 1 - array_lbas - 1;
	const quint64 start_lba = kSeedPartStartLba;
	if( last_usable < start_lba )
	{
		if( error_out )
			*error_out = QObject::tr( "Root image has no usable GPT space:\n%1" ).arg( path );
		return false;
	}

	static const quint8 kApfsRfc[ 16 ] = {
		0x7C, 0x34, 0x57, 0xEF, 0x00, 0x00, 0x11, 0xAA,
		0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC
	};
	static const quint8 kDiskRfc[ 16 ] = {
		0xA0, 0xE0, 0x00, 0x01, 0x47, 0x50, 0x54, 0x41,
		0x81, 0x00, 0x41, 0x51, 0x45, 0x4D, 0x55, 0x01
	};
	static const quint8 kPartRfc[ 16 ] = {
		0xA0, 0xE0, 0x00, 0x02, 0x47, 0x50, 0x54, 0x41,
		0x81, 0x00, 0x41, 0x51, 0x45, 0x4D, 0x55, 0x02
	};

	QByteArray entries( int( array_bytes ), '\0' );
	char *e = entries.data();
	Put_Gpt_Guid( e + 0, kApfsRfc );
	Put_Gpt_Guid( e + 16, kPartRfc );
	Put_Le64( e + 32, start_lba );
	Put_Le64( e + 40, last_usable );
	const QString name = QString::fromLatin1( kSeedPartName );
	for( int i = 0; i < name.size() && i < 36; ++i )
	{
		const ushort c = name.at( i ).unicode();
		e[ 56 + i * 2 ] = char( c & 0xFFu );
		e[ 57 + i * 2 ] = char( ( c >> 8 ) & 0xFFu );
	}
	const quint32 part_crc = Gpt_Crc32( entries.constData(), entries.size() );

	char disk_guid[ 16 ];
	Put_Gpt_Guid( disk_guid, kDiskRfc );

	auto make_hdr = [ & ]( quint64 my, quint64 alt, quint64 part_lba ) -> QByteArray {
		QByteArray h( 92, '\0' );
		char *p = h.data();
		std::memcpy( p, "EFI PART", 8 );
		Put_Le32( p + 8, 0x00010000u );
		Put_Le32( p + 12, 92 );
		Put_Le64( p + 24, my );
		Put_Le64( p + 32, alt );
		Put_Le64( p + 40, first_usable );
		Put_Le64( p + 48, last_usable );
		std::memcpy( p + 56, disk_guid, 16 );
		Put_Le64( p + 72, part_lba );
		Put_Le32( p + 80, 128 );
		Put_Le32( p + 84, 128 );
		Put_Le32( p + 88, part_crc );
		Put_Le32( p + 16, Gpt_Crc32( p, 92 ) );
		return h;
	};

	const quint64 backup_hdr_lba = n_lba - 1;
	const quint64 backup_arr_lba = n_lba - 1 - array_lbas;
	const QByteArray primary = make_hdr( 1, backup_hdr_lba, 2 );
	const QByteArray backup = make_hdr( backup_hdr_lba, 1, backup_arr_lba );

	QByteArray mbr( kNvmeLba, '\0' );
	mbr[ 446 ] = 0;
	mbr[ 447 ] = 0x00;
	mbr[ 448 ] = 0x02;
	mbr[ 449 ] = 0x00;
	mbr[ 450 ] = char( 0xEE );
	mbr[ 451 ] = char( 0xFF );
	mbr[ 452 ] = char( 0xFF );
	mbr[ 453 ] = char( 0xFF );
	Put_Le32( mbr.data() + 454, 1 );
	const quint32 sz32 = quint32( qMin( quint64( 0xFFFFFFFFu ), n_lba - 1 ) );
	Put_Le32( mbr.data() + 458, sz32 );
	mbr[ 510 ] = char( 0x55 );
	mbr[ 511 ] = char( 0xAA );

	QByteArray primary_lba( kNvmeLba, '\0' );
	std::memcpy( primary_lba.data(), primary.constData(), 92 );
	QByteArray backup_lba( kNvmeLba, '\0' );
	std::memcpy( backup_lba.data(), backup.constData(), 92 );

	auto wr = [ & ]( qint64 off, const QByteArray &buf ) -> bool {
		if( ! f.seek( off ) )
			return false;
		return f.write( buf ) == buf.size();
	};
	if( ! wr( 0, mbr ) ||
	    ! wr( kNvmeLba, primary_lba ) ||
	    ! wr( qint64( 2 ) * kNvmeLba, entries ) ||
	    ! wr( qint64( backup_arr_lba ) * kNvmeLba, entries ) ||
	    ! wr( qint64( backup_hdr_lba ) * kNvmeLba, backup_lba ) )
	{
		if( error_out )
			*error_out = QObject::tr( "Failed to write placeholder GPT to %1" ).arg( path );
		return false;
	}
	f.close();
#ifdef Q_OS_WIN32
	// mkapfs is best-effort. Never block Power On if WSL/git/loop fails.
	Format_Seed_Apfs_Via_Wsl( path, nullptr );
#endif
	return true;
}

bool AQ_Ensure_Apple_SoC_Disk_Images( const QString &image_dir, QString *error_out )
{
	return AQ_Ensure_Apple_SoC_Disk_Images( image_dir,
		AQ_Apple_SoC_Nand_Bytes( AQ_Default_Apple_SoC_Nand_GiB() ), error_out );
}

bool AQ_Ensure_Apple_SoC_Disk_Images( const QString &image_dir, qint64 root_bytes, QString *error_out )
{
	if( ! QDir().mkpath( image_dir ) )
	{
		if( error_out )
			*error_out = QObject::tr( "Cannot create Apple SoC image directory:\n%1" ).arg( image_dir );
		return false;
	}

	const qint64 nand = root_bytes >= AQ_Apple_SoC_Nand_Bytes( AQ_Min_Apple_SoC_Nand_GiB() )
		? root_bytes
		: AQ_Apple_SoC_Nand_Bytes( AQ_Default_Apple_SoC_Nand_GiB() );
	const QString root = QDir( image_dir ).filePath( QStringLiteral( "root" ) );
	qint64 root_target = nand;
	bool restore_gpt = false;
	const qint64 old_root = QFileInfo( root ).exists() ? QFileInfo( root ).size() : 0;
	if( old_root > 0 )
	{
		QFile f( root );
		if( f.open( QIODevice::ReadOnly ) )
		{
			const qint64 sz = f.size();
			auto real_gpt = [ &f, sz ]( qint64 header_off, qint64 lba ) -> bool
			{
				if( ! Efi_Part_At( f, header_off, sz ) )
					return false;
				const QString name = Gpt_First_Part_Name( f, header_off, lba );
				if( name.isEmpty() || name == QLatin1String( kSeedPartName ) )
					return false;
				return true;
			};
			restore_gpt = real_gpt( kNvmeLba, kNvmeLba ) || real_gpt( 512, 512 );
		}
		if( restore_gpt )
			root_target = old_root;
	}

	struct Spec { const char *name; qint64 bytes; };
	const Spec specs[] = {
		{ "sep_nvram", 64 * 1024 },
		{ "sep_ssc", 128 * 1024 },
		{ "root", root_target },
		{ "firmware", 256LL * 1024 * 1024 },
		{ "syscfg", 1 * 1024 * 1024 },
		{ "ctrl_bits", 1 * 1024 * 1024 },
		{ "nvram", 1 * 1024 * 1024 },
		{ "effaceable", 1 * 1024 * 1024 },
		{ "panic_log", 16 * 1024 * 1024 },
	};

	qint64 need = 0;
	for( const Spec &s : specs )
	{
		const QString path = QDir( image_dir ).filePath( QString::fromLatin1( s.name ) );
		const QFileInfo fi( path );
		if( ! fi.exists() || fi.size() < s.bytes )
			need += ( s.bytes - ( fi.exists() ? fi.size() : 0 ) );
	}

	QStorageInfo storage( image_dir );
	if( storage.isValid() && storage.bytesAvailable() >= 0 &&
	    storage.bytesAvailable() < need + ( 512LL * 1024 * 1024 ) )
	{
		if( error_out )
		{
			*error_out = QObject::tr(
				"Not enough free disk space for Apple SoC images under:\n%1\n\n"
				"Need about %2 MiB free (have %3 MiB)." )
				.arg( image_dir )
				.arg( ( need + 512LL * 1024 * 1024 ) / ( 1024 * 1024 ) )
				.arg( storage.bytesAvailable() / ( 1024 * 1024 ) );
		}
		return false;
	}

	for( const Spec &s : specs )
	{
		const QString path = QDir( image_dir ).filePath( QString::fromLatin1( s.name ) );
		if( ! Ensure_Raw_Image( path, s.bytes, error_out ) )
			return false;
	}

	if( restore_gpt )
		return true;
	const qint64 new_root = QFileInfo( root ).size();
	const bool grew = ( old_root > 0 && new_root != old_root );
	return Seed_Root_Placeholder_Gpt( root, error_out, grew );
}

static const char *Apple_SoC_Disk_Names[] = {
	"sep_nvram", "sep_ssc", "root", "firmware", "syscfg",
	"ctrl_bits", "nvram", "effaceable", "panic_log"
};

bool AQ_Wipe_Apple_SoC_Disk_Images( const Virtual_Machine *vm, QString *error_out )
{
	if( ! vm )
	{
		if( error_out )
			*error_out = QObject::tr( "No VM selected." );
		return false;
	}
	const QString dir = AQ_Apple_SoC_Image_Dir( vm );
	if( ! QDir( dir ).exists() )
		return true;
	for( const char *name : Apple_SoC_Disk_Names )
	{
		const QString path = QDir( dir ).filePath( QString::fromLatin1( name ) );
		if( ! QFile::exists( path ) )
			continue;
		if( ! QFile::remove( path ) )
		{
			if( error_out )
				*error_out = QObject::tr( "Could not delete %1 (is iOS still Powered On?)" ).arg( path );
			return false;
		}
	}
	return true;
}

bool AQ_Apple_SoC_Root_Has_GPT( const Virtual_Machine *vm )
{
	if( ! vm )
		return false;
	const QString path = QDir( AQ_Apple_SoC_Image_Dir( vm ) ).filePath( QStringLiteral( "root" ) );
	QFile f( path );
	if( ! f.open( QIODevice::ReadOnly ) )
		return false;
	const qint64 sz = f.size();
	auto real_gpt = [ &f, sz ]( qint64 header_off, qint64 lba ) -> bool
	{
		if( ! Efi_Part_At( f, header_off, sz ) )
			return false;
		const QString name = Gpt_First_Part_Name( f, header_off, lba );
		if( name.isEmpty() || name == QLatin1String( kSeedPartName ) )
			return false;
		return true;
	};
	// Placeholder AQEMU_SEED GPT is not a finished restore.
	return real_gpt( kNvmeLba, kNvmeLba ) || real_gpt( 512, 512 );
}

void AQ_Prompt_Wipe_Apple_SoC_Disks( Virtual_Machine *vm, QWidget *parent )
{
	if( ! vm || ! AQ_Is_Apple_SoC_VM( vm ) )
	{
		QMessageBox::warning( parent, QObject::tr( "Wipe Inferno disks" ),
			QObject::tr( "Select the iOS (Apple SoC) VM in the list first." ) );
		return;
	}
	if( vm->Get_State() == VM::VMS_Running || vm->Get_State() == VM::VMS_Pause )
	{
		QMessageBox::warning( parent, QObject::tr( "Wipe Inferno disks" ),
			QObject::tr( "Power Off the iOS guest first.\n\nVM file:\n%1" )
				.arg( QDir::toNativeSeparators( vm->Get_VM_XML_File_Path() ) ) );
		return;
	}

	const QString xml = QDir::toNativeSeparators( vm->Get_VM_XML_File_Path() );
	const QString dir = QDir::toNativeSeparators( AQ_Apple_SoC_Image_Dir( vm ) );
	QStringList present;
	for( const char *name : Apple_SoC_Disk_Names )
	{
		const QString path = QDir( AQ_Apple_SoC_Image_Dir( vm ) ).filePath( QString::fromLatin1( name ) );
		if( QFile::exists( path ) )
			present << QString::fromLatin1( name );
	}

	const QString body = QObject::tr(
		"<p>This deletes Inferno NVMe images for the <b>selected iOS VM</b> "
		"(after a failed restore / <code>-256</code>). Next Power On recreates them at the "
		"MACHINE <b>NAND (root) size</b> (%1 GiB); "
		"<code>root</code> gets a placeholder GPT (not a finished iOS install) so ramrod "
		"can pass the header check when CreateFilesystemPartitions stays false.</p>"
		"<p><b>VM:</b> %2</p>"
		"<p><b>VM file (.aqemu):</b><br><code>%3</code></p>"
		"<p><b>Disks folder:</b><br><code>%4</code></p>"
		"<p><b>Will remove:</b> %5</p>"
		"<p>Does <b>not</b> delete the VM file, IPSW, firmware extract, or companion.qcow2.</p>" )
		.arg( AQ_Clamp_Apple_SoC_Nand_GiB( vm->Get_Apple_Nand_Size_GiB() ) )
		.arg( vm->Get_Machine_Name().toHtmlEscaped(),
		      xml.toHtmlEscaped(),
		      dir.toHtmlEscaped(),
		      present.isEmpty()
			      ? QObject::tr( "(folder empty — nothing to delete)" )
			      : present.join( QStringLiteral( ", " ) ) );

	if( QMessageBox::question( parent, QObject::tr( "Wipe Inferno disks" ), body,
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) != QMessageBox::Yes )
		return;

	QString err;
	if( ! AQ_Wipe_Apple_SoC_Disk_Images( vm, &err ) )
	{
		QMessageBox::warning( parent, QObject::tr( "Wipe Inferno disks" ), err );
		return;
	}
	QMessageBox::information( parent, QObject::tr( "Wipe Inferno disks" ),
		QObject::tr(
			"Done.\n\nStart companion (if needed), Power On iOS "
			"(that writes the placeholder GPT on root), then Restore IPSW via SSH.\n"
			"Do not apply filesystem patches until restore rewrites that GPT." ) );
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

	const QString image_dir = AQ_Apple_SoC_Image_Dir( vm );

	if( create_missing_images )
	{
		QString err;
		if( ! AQ_Ensure_Apple_SoC_Disk_Images( image_dir,
			AQ_Apple_SoC_Nand_Bytes( vm->Get_Apple_Nand_Size_GiB() ), &err ) )
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
		return QDir( image_dir ).filePath( name );
	};

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

QStringList AQ_Filter_Apple_SoC_Additional_Args( const QStringList &args )
{
	auto skip_drive = []( const QString &opt ) -> bool {
		if( opt.contains( QLatin1String( "if=pflash" ), Qt::CaseInsensitive ) )
			return true;
		static const char *ids[] = {
			"id=root", "id=firmware", "id=syscfg", "id=ctrl_bits",
			"id=nvram", "id=effaceable", "id=panic_log"
		};
		for( const char *id : ids )
		{
			if( opt.contains( QLatin1String( id ), Qt::CaseInsensitive ) )
				return true;
		}
		return false;
	};
	auto skip_device = []( const QString &opt ) -> bool {
		if( opt.startsWith( QLatin1String( "nvme-ns" ), Qt::CaseInsensitive ) )
			return true;
		if( opt.startsWith( QLatin1String( "apple-nvram" ), Qt::CaseInsensitive ) )
			return true;
		return false;
	};

	QStringList out;
	out.reserve( args.size() );
	for( int i = 0; i < args.size(); ++i )
	{
		const QString &a = args.at( i );
		if( a == QLatin1String( "-machine" ) || a.startsWith( QLatin1String( "-machine=" ) ) )
		{
			if( a == QLatin1String( "-machine" ) && i + 1 < args.size() )
				++i;
			continue;
		}
		if( a == QLatin1String( "-append" ) || a.startsWith( QLatin1String( "-append=" ) ) )
		{
			if( a == QLatin1String( "-append" ) && i + 1 < args.size() )
				++i;
			continue;
		}
		if( a == QLatin1String( "-drive" ) && i + 1 < args.size() && skip_drive( args.at( i + 1 ) ) )
		{
			++i;
			continue;
		}
		if( a.startsWith( QLatin1String( "-drive=" ) ) && skip_drive( a.mid( 7 ) ) )
			continue;
		if( a == QLatin1String( "-device" ) && i + 1 < args.size() && skip_device( args.at( i + 1 ) ) )
		{
			++i;
			continue;
		}
		if( a.startsWith( QLatin1String( "-device=" ) ) && skip_device( a.mid( 8 ) ) )
			continue;
		out << a;
	}
	return out;
}

QString AQ_Build_Apple_SoC_Machine_Props( const Virtual_Machine *vm, bool via_wsl )
{
	if( ! vm )
		return QString();

	// Machine type and every path/USB setting come only from the VM (MACHINE tab).
	// Never invent file paths or USB endpoints at launch time.
	QString machine = vm->Get_Machine_Type().trimmed();
	if( machine.isEmpty() )
		return QString();

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
	add_path_prop( "securerom", vm->Get_Apple_SecureROM_Path() );
	if( vm->Use_Apple_KASLR_Off() )
		props << QStringLiteral( "kaslr-off=true" );

	// After a successful restore, guest NVRAM keeps auto-boot=true. Passing -initrd
	// alone does NOT enter recovery — Inferno only prepends "-restore rd=md0 …" when
	// auto-boot is false. Force enter_recovery whenever a restore ramdisk is set.
	const QString initrd = AQ_Normalize_File_Path( vm->Get_Apple_Initrd_Path() );
	if( ! initrd.isEmpty() && QFileInfo( initrd ).isFile() )
		props << QStringLiteral( "boot-mode=enter_recovery" );

	const QString conn = vm->Get_Apple_USB_Conn_Type().trimmed().toLower();
	if( ! conn.isEmpty() )
	{
		props << QStringLiteral( "usb-conn-type=%1" ).arg( conn );
		const QString addr = vm->Get_Apple_USB_Conn_Addr().trimmed();
		if( ! addr.isEmpty() )
			props << QStringLiteral( "usb-conn-addr=%1" ).arg( addr );
		if( conn != QLatin1String( "unix" ) )
		{
			const int port = vm->Get_Apple_USB_Conn_Port();
			if( port > 0 )
				props << QStringLiteral( "usb-conn-port=%1" ).arg( port );
		}
	}

	Q_UNUSED( via_wsl );
	return props.join( QLatin1Char( ',' ) );
}
