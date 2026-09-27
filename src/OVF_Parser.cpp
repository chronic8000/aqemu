/****************************************************************************
**
** OVF (Open Virtualization Format) and OVA (Archive) Parser & Generator
**
****************************************************************************/

#include "OVF_Parser.h"
#include "VM.h"
#include "Utils.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <cstring>

#pragma pack(push, 1)
struct TarUStarHeader
{
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag;
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
	char padding[12];
};
#pragma pack(pop)

bool OVF_Parser::Is_OVA_Archive( const QString &path )
{
	return path.endsWith( QStringLiteral( ".ova" ), Qt::CaseInsensitive );
}

bool OVF_Parser::Is_OVF_File( const QString &path )
{
	return path.endsWith( QStringLiteral( ".ovf" ), Qt::CaseInsensitive );
}

static qint64 Parse_Octal( const char *str, int max_len )
{
	qint64 val = 0;
	int i = 0;
	while( i < max_len && ( str[i] == ' ' || str[i] == '0' ) )
		++i;
	for( ; i < max_len; ++i )
	{
		const char c = str[i];
		if( c < '0' || c > '7' )
			break;
		val = ( val << 3 ) | ( c - '0' );
	}
	return val;
}

bool OVF_Parser::Extract_OVA( const QString &ova_path, const QString &dest_dir,
                             QString &out_ovf_path, QStringList &out_extracted_files,
                             QString &error_msg,
                             std::function<void(int progress, const QString &status)> progress_cb )
{
	QFile file( ova_path );
	if( ! file.open( QIODevice::ReadOnly ) )
	{
		error_msg = QObject::tr( "Cannot open OVA archive: %1" ).arg( file.errorString() );
		return false;
	}

	QDir dir( dest_dir );
	if( ! dir.exists() )
		dir.mkpath( QStringLiteral( "." ) );

	const qint64 total_size = file.size();
	qint64 processed_bytes = 0;

	TarUStarHeader hdr;
	while( file.read( reinterpret_cast<char*>( &hdr ), 512 ) == 512 )
	{
		// Two consecutive 512-byte zero blocks signal end of TAR
		bool all_zero = true;
		const char *raw = reinterpret_cast<const char*>( &hdr );
		for( int i = 0; i < 512; ++i )
		{
			if( raw[i] != 0 )
			{
				all_zero = false;
				break;
			}
		}
		if( all_zero )
			break;

		QString name = QString::fromLatin1( hdr.name, qstrnlen( hdr.name, 100 ) ).trimmed();
		if( hdr.prefix[0] != '\0' )
		{
			const QString prefix = QString::fromLatin1( hdr.prefix, qstrnlen( hdr.prefix, 155 ) ).trimmed();
			if( ! prefix.isEmpty() )
				name = prefix + QStringLiteral( "/" ) + name;
		}

		if( name.isEmpty() )
			continue;

		const qint64 entry_size = Parse_Octal( hdr.size, 12 );
		const char type = hdr.typeflag;

		const QString out_file_path = dir.filePath( name );

		if( type == '5' || name.endsWith( QLatin1Char( '/' ) ) )
		{
			// Directory entry
			dir.mkpath( name );
		}
		else
		{
			// Regular file
			QFileInfo fi( out_file_path );
			dir.mkpath( fi.path() );

			QFile out_file( out_file_path );
			if( ! out_file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
			{
				error_msg = QObject::tr( "Cannot create file '%1': %2" ).arg( out_file_path, out_file.errorString() );
				return false;
			}

			qint64 remaining = entry_size;
			constexpr qint64 chunk_size = 1024 * 1024; // 1 MB buffer
			QByteArray buffer;
			buffer.resize( chunk_size );

			while( remaining > 0 )
			{
				const qint64 to_read = qMin( remaining, chunk_size );
				const qint64 bytes_read = file.read( buffer.data(), to_read );
				if( bytes_read <= 0 )
				{
					error_msg = QObject::tr( "Unexpected end of archive while reading '%1'" ).arg( name );
					return false;
				}
				out_file.write( buffer.constData(), bytes_read );
				remaining -= bytes_read;
				processed_bytes += bytes_read;

				if( progress_cb && total_size > 0 )
				{
					const int pct = qBound( 0, static_cast<int>( ( processed_bytes * 100 ) / total_size ), 100 );
					progress_cb( pct, QObject::tr( "Extracting %1..." ).arg( name ) );
				}
			}
			out_file.close();

			out_extracted_files << out_file_path;
			if( name.endsWith( QStringLiteral( ".ovf" ), Qt::CaseInsensitive ) )
				out_ovf_path = out_file_path;

			// TAR entries are padded to 512-byte boundaries
			const qint64 pad = ( 512 - ( entry_size % 512 ) ) % 512;
			if( pad > 0 )
				file.seek( file.pos() + pad );
		}
	}

	if( out_ovf_path.isEmpty() )
	{
		// Search for any extracted .ovf
		for( const QString &f : out_extracted_files )
		{
			if( f.endsWith( QStringLiteral( ".ovf" ), Qt::CaseInsensitive ) )
			{
				out_ovf_path = f;
				break;
			}
		}
	}

	if( out_ovf_path.isEmpty() )
	{
		error_msg = QObject::tr( "No .ovf descriptor found in the OVA archive." );
		return false;
	}

	return true;
}

bool OVF_Parser::Pack_OVA( const QStringList &file_paths, const QString &dest_ova_path,
                          QString &error_msg,
                          std::function<void(int progress, const QString &status)> progress_cb )
{
	QFile out_archive( dest_ova_path );
	if( ! out_archive.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
		error_msg = QObject::tr( "Cannot open destination OVA file: %1" ).arg( out_archive.errorString() );
		return false;
	}

	qint64 total_bytes = 0;
	for( const QString &fp : file_paths )
	{
		QFileInfo fi( fp );
		if( fi.exists() )
			total_bytes += fi.size();
	}

	qint64 written_bytes = 0;

	for( const QString &fp : file_paths )
	{
		QFile in_file( fp );
		if( ! in_file.open( QIODevice::ReadOnly ) )
		{
			error_msg = QObject::tr( "Cannot read file '%1': %2" ).arg( fp, in_file.errorString() );
			return false;
		}

		QFileInfo fi( fp );
		const QString name = fi.fileName();
		const qint64 size = in_file.size();

		TarUStarHeader hdr;
		std::memset( &hdr, 0, sizeof( hdr ) );

		const QByteArray name_bytes = name.toLatin1();
		std::strncpy( hdr.name, name_bytes.constData(), qMin( static_cast<int>( sizeof( hdr.name ) ), name_bytes.size() ) );
		std::snprintf( hdr.mode, sizeof( hdr.mode ), "%07o", 0644 );
		std::snprintf( hdr.uid, sizeof( hdr.uid ), "%07o", 0 );
		std::snprintf( hdr.gid, sizeof( hdr.gid ), "%07o", 0 );
		std::snprintf( hdr.size, sizeof( hdr.size ), "%011llo", static_cast<unsigned long long>( size ) );
		std::snprintf( hdr.mtime, sizeof( hdr.mtime ), "%011llo", static_cast<unsigned long long>( fi.lastModified().toSecsSinceEpoch() ) );
		hdr.typeflag = '0';
		std::memcpy( hdr.magic, "ustar  ", 6 );

		// Checksum: sum of all bytes in header with checksum field treated as 8 spaces ' '
		std::memset( hdr.chksum, ' ', sizeof( hdr.chksum ) );
		unsigned int sum = 0;
		const unsigned char *raw = reinterpret_cast<const unsigned char*>( &hdr );
		for( size_t i = 0; i < sizeof( hdr ); ++i )
			sum += raw[i];
		std::snprintf( hdr.chksum, sizeof( hdr.chksum ), "%06o", sum );

		out_archive.write( reinterpret_cast<const char*>( &hdr ), 512 );

		constexpr qint64 chunk_size = 1024 * 1024;
		QByteArray buf;
		buf.resize( chunk_size );

		qint64 remaining = size;
		while( remaining > 0 )
		{
			const qint64 to_read = qMin( remaining, chunk_size );
			const qint64 bytes_read = in_file.read( buf.data(), to_read );
			if( bytes_read <= 0 )
				break;
			out_archive.write( buf.constData(), bytes_read );
			remaining -= bytes_read;
			written_bytes += bytes_read;

			if( progress_cb && total_bytes > 0 )
			{
				const int pct = qBound( 0, static_cast<int>( ( written_bytes * 100 ) / total_bytes ), 100 );
				progress_cb( pct, QObject::tr( "Archiving %1..." ).arg( name ) );
			}
		}

		const qint64 pad = ( 512 - ( size % 512 ) ) % 512;
		if( pad > 0 )
		{
			const QByteArray zero_pad( pad, '\0' );
			out_archive.write( zero_pad );
		}
	}

	// Two 512-byte zero blocks terminate archive
	const QByteArray end_pad( 1024, '\0' );
	out_archive.write( end_pad );
	out_archive.close();

	return true;
}

struct OVF_File_Ref
{
	QString id;
	QString href;
	qint64 size = 0;
};

bool OVF_Parser::Parse_OVF( const QString &ovf_path, OVF_Appliance &appliance, QString &error_msg )
{
	QFile file( ovf_path );
	if( ! file.open( QIODevice::ReadOnly ) )
	{
		error_msg = QObject::tr( "Cannot open OVF file: %1" ).arg( file.errorString() );
		return false;
	}

	appliance = OVF_Appliance();
	appliance.ovf_file_path = ovf_path;
	appliance.base_dir = QFileInfo( ovf_path ).absolutePath();

	QMap<QString, OVF_File_Ref> file_refs;
	QMap<QString, OVF_Disk> disks_by_id;

	int cim_os_id = -1;

	QXmlStreamReader xml( &file );
	while( ! xml.atEnd() && ! xml.hasError() )
	{
		xml.readNext();
		if( ! xml.isStartElement() )
			continue;

		const QStringRef tag = xml.name();

		if( tag == QLatin1String( "File" ) )
		{
			const QXmlStreamAttributes attrs = xml.attributes();
			OVF_File_Ref r;
			r.id = attrs.value( QLatin1String( "id" ) ).toString();
			if( r.id.isEmpty() )
				r.id = attrs.value( QStringLiteral( "http://schemas.dmtf.org/ovf/envelope/1" ), QStringLiteral( "id" ) ).toString();
			r.href = attrs.value( QLatin1String( "href" ) ).toString();
			if( r.href.isEmpty() )
				r.href = attrs.value( QStringLiteral( "http://schemas.dmtf.org/ovf/envelope/1" ), QStringLiteral( "href" ) ).toString();
			r.size = attrs.value( QLatin1String( "size" ) ).toLongLong();
			if( ! r.id.isEmpty() )
				file_refs.insert( r.id, r );
		}
		else if( tag == QLatin1String( "Disk" ) )
		{
			const QXmlStreamAttributes attrs = xml.attributes();
			OVF_Disk d;
			d.disk_id = attrs.value( QLatin1String( "diskId" ) ).toString();
			if( d.disk_id.isEmpty() )
				d.disk_id = attrs.value( QStringLiteral( "http://schemas.dmtf.org/ovf/envelope/1" ), QStringLiteral( "diskId" ) ).toString();
			d.file_ref = attrs.value( QLatin1String( "fileRef" ) ).toString();
			if( d.file_ref.isEmpty() )
				d.file_ref = attrs.value( QStringLiteral( "http://schemas.dmtf.org/ovf/envelope/1" ), QStringLiteral( "fileRef" ) ).toString();
			d.capacity_bytes = attrs.value( QLatin1String( "capacity" ) ).toLongLong();
			d.format = attrs.value( QLatin1String( "format" ) ).toString();
			if( ! d.disk_id.isEmpty() )
				disks_by_id.insert( d.disk_id, d );
		}
		else if( tag == QLatin1String( "Network" ) )
		{
			OVF_Network net;
			net.name = xml.attributes().value( QLatin1String( "name" ) ).toString();
			appliance.networks.append( net );
		}
		else if( tag == QLatin1String( "VirtualSystem" ) )
		{
			if( appliance.name.isEmpty() )
				appliance.name = xml.attributes().value( QLatin1String( "id" ) ).toString();
		}
		else if( tag == QLatin1String( "Name" ) )
		{
			if( appliance.name.isEmpty() || appliance.name == xml.attributes().value( QLatin1String( "id" ) ).toString() )
				appliance.name = xml.readElementText().trimmed();
		}
		else if( tag == QLatin1String( "OperatingSystemSection" ) )
		{
			const QString id_str = xml.attributes().value( QLatin1String( "id" ) ).toString();
			if( ! id_str.isEmpty() )
				cim_os_id = id_str.toInt();
			const QString vbox_os = xml.attributes().value( QStringLiteral( "http://www.virtualbox.org/ovf/machine" ), QStringLiteral( "ostype" ) ).toString();
			if( ! vbox_os.isEmpty() )
				appliance.os_type_raw = vbox_os;
		}
		else if( tag == QLatin1String( "Description" ) )
		{
			if( appliance.os_type_raw.isEmpty() )
				appliance.os_type_raw = xml.readElementText().trimmed();
		}
		else if( tag == QLatin1String( "Item" ) )
		{
			// Parse virtual hardware items
			int res_type = -1;
			qint64 quantity = 0;
			QString host_res;
			QString caption;

			while( ! xml.atEnd() && ! ( xml.isEndElement() && xml.name() == QLatin1String( "Item" ) ) )
			{
				xml.readNext();
				if( xml.isStartElement() )
				{
					const QStringRef sub = xml.name();
					if( sub == QLatin1String( "ResourceType" ) )
						res_type = xml.readElementText().toInt();
					else if( sub == QLatin1String( "VirtualQuantity" ) )
						quantity = xml.readElementText().toLongLong();
					else if( sub == QLatin1String( "HostResource" ) )
						host_res = xml.readElementText().trimmed();
					else if( sub == QLatin1String( "Caption" ) )
						caption = xml.readElementText().trimmed();
				}
			}

			if( res_type == 3 && quantity > 0 ) // CPU
				appliance.cpu_count = static_cast<int>( quantity );
			else if( res_type == 4 && quantity > 0 ) // Memory
				appliance.memory_mb = static_cast<int>( quantity );
			else if( res_type == 17 ) // Hard disk
			{
				QString disk_id;
				if( host_res.contains( QLatin1Char( '/' ) ) )
					disk_id = host_res.section( QLatin1Char( '/' ), -1 );
				else
					disk_id = host_res;

				if( disks_by_id.contains( disk_id ) )
				{
					OVF_Disk d = disks_by_id.value( disk_id );
					if( file_refs.contains( d.file_ref ) )
					{
						d.href = file_refs.value( d.file_ref ).href;
						d.local_extracted_path = QDir( appliance.base_dir ).filePath( d.href );
					}
					appliance.disks.append( d );
				}
			}
		}
	}

	if( xml.hasError() )
	{
		error_msg = QObject::tr( "XML parsing error: %1 (line %2)" ).arg( xml.errorString() ).arg( xml.lineNumber() );
		return false;
	}

	// Resolve any unlinked disks from References
	if( appliance.disks.isEmpty() )
	{
		for( auto it = disks_by_id.constBegin(); it != disks_by_id.constEnd(); ++it )
		{
			OVF_Disk d = it.value();
			if( file_refs.contains( d.file_ref ) )
			{
				d.href = file_refs.value( d.file_ref ).href;
				d.local_extracted_path = QDir( appliance.base_dir ).filePath( d.href );
			}
			appliance.disks.append( d );
		}
	}

	if( appliance.name.isEmpty() )
		appliance.name = QFileInfo( ovf_path ).baseName();

	appliance.aqemu_profile_name = Map_OS_To_AQEMU_Profile( appliance.os_type_raw, cim_os_id );
	return true;
}

QString OVF_Parser::Map_OS_To_AQEMU_Profile( const QString &os_str, int cim_os_id )
{
	const QString t = os_str.toLower();

	// TrueNAS & FreeNAS
	if( t.contains( QStringLiteral( "truenas" ) ) || t.contains( QStringLiteral( "freenas" ) ) )
	{
		if( t.contains( QStringLiteral( "scale" ) ) )
			return QStringLiteral( "TrueNAS SCALE" );
		return QStringLiteral( "TrueNAS CORE" );
	}

	// Windows
	if( t.contains( QStringLiteral( "win11" ) ) || t.contains( QStringLiteral( "windows11" ) ) || t.contains( QStringLiteral( "windows 11" ) ) )
		return QStringLiteral( "Windows 11" );
	if( t.contains( QStringLiteral( "win10" ) ) || t.contains( QStringLiteral( "windows10" ) ) || t.contains( QStringLiteral( "windows 10" ) ) || cim_os_id == 115 || cim_os_id == 116 )
		return t.contains( QStringLiteral( "32" ) ) ? QStringLiteral( "Windows 10 (32-bit)" ) : QStringLiteral( "Windows 10 (64-bit)" );
	if( t.contains( QStringLiteral( "win8" ) ) || t.contains( QStringLiteral( "windows8" ) ) || cim_os_id == 105 )
		return QStringLiteral( "Windows 8.1 (64-bit)" );
	if( t.contains( QStringLiteral( "win7" ) ) || t.contains( QStringLiteral( "windows7" ) ) || cim_os_id == 102 || cim_os_id == 103 )
		return t.contains( QStringLiteral( "32" ) ) ? QStringLiteral( "Windows 7 (32-bit)" ) : QStringLiteral( "Windows 7 (64-bit)" );
	if( t.contains( QStringLiteral( "winxp" ) ) || t.contains( QStringLiteral( "windowsxp" ) ) || cim_os_id == 69 || cim_os_id == 70 )
		return QStringLiteral( "Windows XP (32-bit)" );
	if( t.contains( QStringLiteral( "win98" ) ) || cim_os_id == 59 )
		return QStringLiteral( "Windows 98" );
	if( t.contains( QStringLiteral( "win95" ) ) || cim_os_id == 58 )
		return QStringLiteral( "Windows 95" );
	if( t.contains( QStringLiteral( "dos" ) ) )
		return QStringLiteral( "MS-DOS" );

	// Linux & BSD
	if( t.contains( QStringLiteral( "ubuntu" ) ) || cim_os_id == 94 )
		return t.contains( QStringLiteral( "32" ) ) ? QStringLiteral( "Ubuntu (32-bit)" ) : QStringLiteral( "Ubuntu (64-bit)" );
	if( t.contains( QStringLiteral( "debian" ) ) || cim_os_id == 95 )
		return t.contains( QStringLiteral( "32" ) ) ? QStringLiteral( "Debian (32-bit)" ) : QStringLiteral( "Debian (64-bit)" );
	if( t.contains( QStringLiteral( "fedora" ) ) )
		return QStringLiteral( "Fedora (64-bit)" );
	if( t.contains( QStringLiteral( "arch" ) ) )
		return QStringLiteral( "Arch Linux (64-bit)" );
	if( t.contains( QStringLiteral( "freebsd" ) ) || ( cim_os_id >= 36 && cim_os_id <= 40 ) )
		return t.contains( QStringLiteral( "32" ) ) ? QStringLiteral( "FreeBSD (32-bit)" ) : QStringLiteral( "FreeBSD (64-bit)" );
	if( t.contains( QStringLiteral( "openbsd" ) ) )
		return QStringLiteral( "OpenBSD (64-bit)" );
	if( t.contains( QStringLiteral( "netbsd" ) ) )
		return QStringLiteral( "NetBSD (64-bit)" );
	if( t.contains( QStringLiteral( "solaris" ) ) || t.contains( QStringLiteral( "illumos" ) ) )
		return QStringLiteral( "Solaris x86" );
	if( t.contains( QStringLiteral( "macos" ) ) || t.contains( QStringLiteral( "darwin" ) ) )
		return QStringLiteral( "macOS" );
	if( t.contains( QStringLiteral( "reactos" ) ) )
		return QStringLiteral( "ReactOS (32-bit)" );
	if( t.contains( QStringLiteral( "haiku" ) ) )
		return QStringLiteral( "Haiku (64-bit)" );

	return QStringLiteral( "Generic Linux (64-bit)" );
}

QString OVF_Parser::Generate_OVF_XML( const Virtual_Machine &vm, const QList<OVF_Disk> &disks )
{
	QString xml_out;
	QXmlStreamWriter xml( &xml_out );
	xml.setAutoFormatting( true );
	xml.writeStartDocument();

	xml.writeStartElement( QStringLiteral( "Envelope" ) );
	xml.writeAttribute( QStringLiteral( "xmlns" ), QStringLiteral( "http://schemas.dmtf.org/ovf/envelope/1" ) );
	xml.writeAttribute( QStringLiteral( "xmlns:ovf" ), QStringLiteral( "http://schemas.dmtf.org/ovf/envelope/1" ) );
	xml.writeAttribute( QStringLiteral( "xmlns:rasd" ), QStringLiteral( "http://schemas.dmtf.org/wbem/wscim/1/cim-schema/2/CIM_ResourceAllocationSettingData" ) );
	xml.writeAttribute( QStringLiteral( "xmlns:vssd" ), QStringLiteral( "http://schemas.dmtf.org/wbem/wscim/1/cim-schema/2/CIM_VirtualSystemSettingData" ) );
	xml.writeAttribute( QStringLiteral( "xmlns:xsi" ), QStringLiteral( "http://www.w3.org/2001/XMLSchema-instance" ) );

	// References Section
	xml.writeStartElement( QStringLiteral( "References" ) );
	for( int i = 0; i < disks.size(); ++i )
	{
		const OVF_Disk &d = disks[i];
		xml.writeStartElement( QStringLiteral( "File" ) );
		xml.writeAttribute( QStringLiteral( "ovf:id" ), QStringLiteral( "file%1" ).arg( i + 1 ) );
		xml.writeAttribute( QStringLiteral( "ovf:href" ), d.href );
		QFileInfo fi( d.local_extracted_path );
		if( fi.exists() )
			xml.writeAttribute( QStringLiteral( "ovf:size" ), QString::number( fi.size() ) );
		xml.writeEndElement(); // File
	}
	xml.writeEndElement(); // References

	// DiskSection
	xml.writeStartElement( QStringLiteral( "DiskSection" ) );
	xml.writeTextElement( QStringLiteral( "Info" ), QStringLiteral( "List of the virtual disks used in the package" ) );
	for( int i = 0; i < disks.size(); ++i )
	{
		const OVF_Disk &d = disks[i];
		xml.writeStartElement( QStringLiteral( "Disk" ) );
		xml.writeAttribute( QStringLiteral( "ovf:diskId" ), QStringLiteral( "vmdisk%1" ).arg( i + 1 ) );
		xml.writeAttribute( QStringLiteral( "ovf:fileRef" ), QStringLiteral( "file%1" ).arg( i + 1 ) );
		xml.writeAttribute( QStringLiteral( "ovf:capacity" ), QString::number( d.capacity_bytes > 0 ? d.capacity_bytes : ( 20ULL * 1024 * 1024 * 1024 ) ) );
		xml.writeAttribute( QStringLiteral( "ovf:format" ), QStringLiteral( "http://www.vmware.com/interfaces/specifications/vmdk.html#streamOptimized" ) );
		xml.writeEndElement(); // Disk
	}
	xml.writeEndElement(); // DiskSection

	// NetworkSection
	xml.writeStartElement( QStringLiteral( "NetworkSection" ) );
	xml.writeTextElement( QStringLiteral( "Info" ), QStringLiteral( "Logical networks used in the package" ) );
	xml.writeStartElement( QStringLiteral( "Network" ) );
	xml.writeAttribute( QStringLiteral( "ovf:name" ), QStringLiteral( "NAT" ) );
	xml.writeTextElement( QStringLiteral( "Description" ), QStringLiteral( "Logical network used on that interface" ) );
	xml.writeEndElement(); // Network
	xml.writeEndElement(); // NetworkSection

	// VirtualSystem
	const QString vm_name = vm.Get_Machine_Name().isEmpty() ? QStringLiteral( "AQEMU-VM" ) : vm.Get_Machine_Name();
	xml.writeStartElement( QStringLiteral( "VirtualSystem" ) );
	xml.writeAttribute( QStringLiteral( "ovf:id" ), vm_name );
	xml.writeTextElement( QStringLiteral( "Info" ), QStringLiteral( "Virtual Machine exported from AQEMU" ) );
	xml.writeTextElement( QStringLiteral( "Name" ), vm_name );

	// OperatingSystemSection
	xml.writeStartElement( QStringLiteral( "OperatingSystemSection" ) );
	xml.writeAttribute( QStringLiteral( "ovf:id" ), QStringLiteral( "102" ) ); // other/generic
	xml.writeTextElement( QStringLiteral( "Info" ), QStringLiteral( "Guest Operating System" ) );
	xml.writeTextElement( QStringLiteral( "Description" ), vm.Get_Computer_Type().isEmpty() ? vm_name : vm.Get_Computer_Type() );
	xml.writeEndElement(); // OperatingSystemSection

	// VirtualHardwareSection
	xml.writeStartElement( QStringLiteral( "VirtualHardwareSection" ) );
	xml.writeTextElement( QStringLiteral( "Info" ), QStringLiteral( "Virtual hardware requirements" ) );

	int instance_id = 1;
	const int cpu_count = qMax( 1, vm.Get_SMP_CPU_Count() );

	// CPU Item
	xml.writeStartElement( QStringLiteral( "Item" ) );
	xml.writeTextElement( QStringLiteral( "rasd:Caption" ), QStringLiteral( "%1 virtual CPU" ).arg( cpu_count ) );
	xml.writeTextElement( QStringLiteral( "rasd:InstanceID" ), QString::number( instance_id++ ) );
	xml.writeTextElement( QStringLiteral( "rasd:ResourceType" ), QStringLiteral( "3" ) );
	xml.writeTextElement( QStringLiteral( "rasd:VirtualQuantity" ), QString::number( cpu_count ) );
	xml.writeEndElement(); // Item (CPU)

	// Memory Item
	xml.writeStartElement( QStringLiteral( "Item" ) );
	xml.writeTextElement( QStringLiteral( "rasd:Caption" ), QStringLiteral( "%1 MB of memory" ).arg( vm.Get_Memory_Size() ) );
	xml.writeTextElement( QStringLiteral( "rasd:InstanceID" ), QString::number( instance_id++ ) );
	xml.writeTextElement( QStringLiteral( "rasd:ResourceType" ), QStringLiteral( "4" ) );
	xml.writeTextElement( QStringLiteral( "rasd:AllocationUnits" ), QStringLiteral( "MegaBytes" ) );
	xml.writeTextElement( QStringLiteral( "rasd:VirtualQuantity" ), QString::number( vm.Get_Memory_Size() ) );
	xml.writeEndElement(); // Item (Memory)

	// Storage Controller Item
	const int controller_id = instance_id++;
	xml.writeStartElement( QStringLiteral( "Item" ) );
	xml.writeTextElement( QStringLiteral( "rasd:Caption" ), QStringLiteral( "SATA Controller" ) );
	xml.writeTextElement( QStringLiteral( "rasd:InstanceID" ), QString::number( controller_id ) );
	xml.writeTextElement( QStringLiteral( "rasd:ResourceType" ), QStringLiteral( "20" ) ); // SATA
	xml.writeEndElement();

	// Hard Disks Items
	for( int i = 0; i < disks.size(); ++i )
	{
		xml.writeStartElement( QStringLiteral( "Item" ) );
		xml.writeTextElement( QStringLiteral( "rasd:AddressOnParent" ), QString::number( i ) );
		xml.writeTextElement( QStringLiteral( "rasd:Caption" ), QStringLiteral( "harddisk%1" ).arg( i + 1 ) );
		xml.writeTextElement( QStringLiteral( "rasd:HostResource" ), QStringLiteral( "/disk/vmdisk%1" ).arg( i + 1 ) );
		xml.writeTextElement( QStringLiteral( "rasd:InstanceID" ), QString::number( instance_id++ ) );
		xml.writeTextElement( QStringLiteral( "rasd:Parent" ), QString::number( controller_id ) );
		xml.writeTextElement( QStringLiteral( "rasd:ResourceType" ), QStringLiteral( "17" ) ); // HardDisk
		xml.writeEndElement();
	}

	xml.writeEndElement(); // VirtualHardwareSection
	xml.writeEndElement(); // VirtualSystem
	xml.writeEndElement(); // Envelope

	xml.writeEndDocument();
	return xml_out;
}
