/****************************************************************************
** Full-architecture QEMU option catalogs from qemu_probe_full_v3/*.json
****************************************************************************/

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QRegExp>
#include <QSettings>
#include <QStringList>

#include "QEMU_Probe_Catalog.h"
#include "Utils.h"

namespace {

bool List_Has_QEMU_Name( const QList<Device_Map> &list, const QString &qemu_name )
{
	for( int i = 0; i < list.count(); ++i )
	{
		if( list[i].QEMU_Name.compare( qemu_name, Qt::CaseInsensitive ) == 0 )
			return true;
	}
	return false;
}

void Append_Unique( QList<Device_Map> &list, const Device_Map &entry )
{
	if( entry.QEMU_Name.isEmpty() )
		return;
	if( List_Has_QEMU_Name( list, entry.QEMU_Name ) )
		return;
	list << entry;
}

QString Pretty_Caption( const QString &qemu_name, const QString &desc )
{
	if( ! desc.isEmpty() )
		return QString( "%1 — %2" ).arg( qemu_name, desc );
	return qemu_name;
}

QString Normalize_Display_Alias( const QString &raw )
{
	const QString n = raw.trimmed();
	if( n.compare( "VGA", Qt::CaseInsensitive ) == 0 )
		return "std";
	if( n.compare( "cirrus-vga", Qt::CaseInsensitive ) == 0 )
		return "cirrus";
	if( n.compare( "vmware-svga", Qt::CaseInsensitive ) == 0 )
		return "vmware";
	if( n.compare( "virtio-vga", Qt::CaseInsensitive ) == 0 )
		return "virtio";
	if( n.compare( "virtio-gpu", Qt::CaseInsensitive ) == 0 )
		return "virtio-gpu-pci";
	return n;
}

void Apply_Audio_Token( VM::Sound_Cards &audio, const QString &name )
{
	const QString n = name.toLower();
	if( n == "sb16" || n.contains( "sb16" ) )
		audio.Audio_sb16 = true;
	else if( n == "es1370" || n.contains( "es1370" ) )
		audio.Audio_es1370 = true;
	else if( n == "adlib" || n.contains( "adlib" ) )
		audio.Audio_Adlib = true;
	else if( n == "gus" || n == "name \"gus\"" )
		audio.Audio_GUS = true;
	else if( n.contains( "ac97" ) )
		audio.Audio_AC97 = true;
	else if( n.contains( "intel-hda" ) || n.contains( "hda-duplex" ) || n == "hda" )
		audio.Audio_HDA = true;
	else if( n.contains( "cs4231a" ) )
		audio.Audio_cs4231a = true;
	else if( n.contains( "virtio-sound" ) || n.contains( "virtio-snd" ) )
		audio.Audio_VirtIO = true;
	else if( n.contains( "usb-audio" ) )
		audio.Audio_USB = true;
	else if( n.contains( "pcspk" ) || n.contains( "isa-pcspk" ) )
		audio.Audio_PC_Speaker = true;
}

QStringList Candidate_Probe_Dirs()
{
	QStringList dirs;
	const QString app = QCoreApplication::applicationDirPath();
	dirs << QDir::cleanPath( app + "/qemu_probe_full_v3" );
	dirs << QDir::cleanPath( app + "/../qemu_probe_full_v3" );
	dirs << QDir::cleanPath( app + "/../../qemu_probe_full_v3" );
	dirs << QDir::cleanPath( app + "/../../../qemu_probe_full_v3" );

	QSettings s;
	const QString data = s.value( "AQEMU_Data_Folder", "" ).toString();
	if( ! data.isEmpty() )
		dirs << QDir::cleanPath( data + "/qemu_probe_full_v3" );

	dirs << QDir::cleanPath( QStringLiteral( "/usr/share/aqemu/qemu_probe_full_v3" ) );
	dirs << QDir::cleanPath( QStringLiteral( "/usr/local/share/aqemu/qemu_probe_full_v3" ) );

	// Dev tree next to sources when running from build_win/
	dirs << QDir::cleanPath( app + "/../qemu_probe_full_v3" );

	dirs.removeDuplicates();
	return dirs;
}

QString g_binary_hint;

QStringList Catalog_Folder_Candidates( const QString &folder_name )
{
	QStringList dirs;
	const QString app = QCoreApplication::applicationDirPath();
	const QStringList bases = QStringList()
		<< app
		<< QDir( app ).absoluteFilePath( QStringLiteral( ".." ) )
		<< QDir( app ).absoluteFilePath( QStringLiteral( "../.." ) )
		<< QDir( app ).absoluteFilePath( QStringLiteral( "../../.." ) );
	for( int i = 0; i < bases.count(); ++i )
		dirs << QDir::cleanPath( bases.at( i ) + QLatin1Char( '/' ) + folder_name );
	dirs.removeDuplicates();
	return dirs;
}

QString First_Catalog_Dir( const QStringList &dirs )
{
	for( int i = 0; i < dirs.count(); ++i )
	{
		QDir d( dirs.at( i ) );
		if( d.exists( QStringLiteral( "x86_64.json" ) ) || d.exists( QStringLiteral( "aarch64.json" ) ) )
			return d.absolutePath();
	}
	return QString();
}

void Fill_Device_Group( const QString &section, const Device_Map &entry, Available_Devices &out )
{
	const QString low = section.toLower();
	if( low.startsWith( QLatin1String( "storage" ) ) )
		Append_Unique( out.Storage_Device_List, entry );
	else if( low.startsWith( QLatin1String( "sound" ) ) )
		Append_Unique( out.Sound_Device_List, entry );
	else if( low.startsWith( QLatin1String( "usb" ) ) )
		Append_Unique( out.USB_Device_List, entry );
	else if( low.startsWith( QLatin1String( "input" ) ) )
		Append_Unique( out.Input_Device_List, entry );
	else if( low.startsWith( QLatin1String( "controller" ) ) )
		Append_Unique( out.Controller_Device_List, entry );
	else if( low.startsWith( QLatin1String( "misc" ) ) )
		Append_Unique( out.Misc_Device_List, entry );
	else if( low.startsWith( QLatin1String( "watchdog" ) ) )
		Append_Unique( out.Watchdog_Device_List, entry );
	else if( low.startsWith( QLatin1String( "cpu" ) ) )
		Append_Unique( out.CPU_Device_List, entry );
	else if( ! section.trimmed().isEmpty() )
		Append_Unique( out.Other_Device_List, entry );
}

void Parse_Grouped_Devices( const QStringList &lines, Available_Devices &out )
{
	QString section;
	QRegExp rx_name( "name\\s+\"([^\"]+)\"" );
	QRegExp rx_desc( "desc\\s+\"([^\"]+)\"" );
	for( int i = 0; i < lines.count(); ++i )
	{
		const QString line = lines.at( i );
		const QString t = line.trimmed();
		if( t.endsWith( QLatin1String( "devices:" ), Qt::CaseInsensitive ) )
		{
			section = t;
			continue;
		}
		if( rx_name.indexIn( line ) < 0 )
			continue;
		const QString name = rx_name.cap( 1 );
		QString desc;
		if( rx_desc.indexIn( line ) >= 0 )
			desc = rx_desc.cap( 1 );
		Fill_Device_Group( section, Device_Map( Pretty_Caption( name, desc ), name ), out );
	}
}

QStringList Bare_Tokens( const QStringList &lines )
{
	QStringList out;
	for( int i = 0; i < lines.count(); ++i )
	{
		const QString t = lines.at( i ).trimmed();
		if( t.isEmpty() || t.contains( QLatin1Char( ' ' ) ) )
			continue;
		if( t.endsWith( QLatin1Char( ':' ) ) )
			continue;
		out << t;
	}
	out.removeDuplicates();
	return out;
}

QStringList Format_Tokens( const QStringList &lines )
{
	QStringList out;
	for( int i = 0; i < lines.count(); ++i )
	{
		QString t = lines.at( i ).trimmed();
		const int colon = t.indexOf( QLatin1Char( ':' ) );
		if( colon >= 0 )
			t = t.mid( colon + 1 ).trimmed();
		if( t.startsWith( QLatin1Char( '(' ) ) )
			continue;
		const QStringList words = t.split( QRegExp( QStringLiteral( "\\s+" ) ), QString::SkipEmptyParts );
		for( int w = 0; w < words.count(); ++w )
		{
			const QString tok = words.at( w ).trimmed();
			if( tok.isEmpty() || tok.startsWith( QLatin1Char( '(' ) ) )
				continue;
			out << tok;
		}
	}
	out.removeDuplicates();
	return out;
}

void Set_Binary_Hint( const QString &path )
{
	g_binary_hint = path;
}

QString Binary_Hint()
{
	return g_binary_hint;
}

QString Resolve_Catalog_Dir( const QString &hint )
{
	const QString n = QDir::fromNativeSeparators( hint ).toLower();
	const bool x64 = n.contains( QLatin1String( "qemu_probe_full_v3_x64" ) )
		|| ( n.contains( QLatin1String( "qemu-install" ) )
		     && ! n.contains( QLatin1String( "arm64" ) ) );
	if( x64 )
	{
		const QString d = First_Catalog_Dir( Catalog_Folder_Candidates( QStringLiteral( "qemu_probe_full_v3_x64" ) ) );
		if( ! d.isEmpty() )
			return d;
	}
	const QString arm = First_Catalog_Dir( Catalog_Folder_Candidates( QStringLiteral( "qemu_probe_full_v3" ) ) );
	if( ! arm.isEmpty() )
		return arm;
	return QString();
}

QString Catalog_Dir_For( const QString &computer_type_or_binary, const QString &key )
{
	QString hint = Binary_Hint();
	if( computer_type_or_binary.contains( QLatin1Char( '/' ) )
	    || computer_type_or_binary.contains( QLatin1Char( '\\' ) ) )
		hint = computer_type_or_binary;
	QString dir = hint.isEmpty() ? QString() : Resolve_Catalog_Dir( hint );
	if( dir.isEmpty() || ! QFile::exists( QDir( dir ).filePath( key + QStringLiteral( ".json" ) ) ) )
	{
		const QString fallback = QEMU_Probe_Catalog::Probe_Directory();
		if( ! fallback.isEmpty()
		    && QFile::exists( QDir( fallback ).filePath( key + QStringLiteral( ".json" ) ) ) )
			return fallback;
	}
	return dir;
}

QStringList Json_String_List( const QJsonObject &obj, const char *key )
{
	QStringList out;
	const QJsonValue v = obj.value( QLatin1String( key ) );
	if( ! v.isArray() )
		return out;
	const QJsonArray arr = v.toArray();
	for( int i = 0; i < arr.size(); ++i )
		out << arr.at( i ).toString();
	return out;
}

} // namespace

QString QEMU_Probe_Catalog::Probe_Directory()
{
	const QStringList dirs = Candidate_Probe_Dirs();
	for( int i = 0; i < dirs.count(); ++i )
	{
		QDir d( dirs[i] );
		if( d.exists() && d.exists( "x86_64.json" ) )
			return d.absolutePath();
	}
	return QString();
}

void QEMU_Probe_Catalog::Use_Binary_Path( const QString &qemu_binary )
{
	Set_Binary_Hint( qemu_binary );
}

QString QEMU_Probe_Catalog::Probe_Directory_For( const QString &qemu_binary_or_hint )
{
	const QString dir = Resolve_Catalog_Dir( qemu_binary_or_hint );
	return dir.isEmpty() ? Probe_Directory() : dir;
}

QString QEMU_Probe_Catalog::Property_Help( const QString &computer_type_or_binary,
                                           const QString &kind,
                                           const QString &name )
{
	const QString key = Architecture_Key( computer_type_or_binary );
	if( key.isEmpty() || name.trimmed().isEmpty() )
		return QString();
	const QString dir = Catalog_Dir_For( computer_type_or_binary, key );
	if( dir.isEmpty() )
		return QString();
	QFile f( QDir( dir ).filePath( key + QStringLiteral( ".properties.json" ) ) );
	if( ! f.open( QIODevice::ReadOnly ) )
		return QString();
	const QJsonDocument doc = QJsonDocument::fromJson( f.readAll() );
	if( ! doc.isObject() )
		return QString();
	const QString bucket = kind.trimmed().isEmpty() ? QStringLiteral( "devices" ) : kind.trimmed();
	return doc.object().value( bucket ).toObject().value( name.trimmed() ).toString();
}

QString QEMU_Probe_Catalog::Architecture_Key( const QString &computer_type_or_binary )
{
	QString s = computer_type_or_binary.trimmed().toLower();
	if( s.isEmpty() )
		return QString();

	// "PowerPC (qemu-system-ppc)" / captions
	const int paren = s.lastIndexOf( "qemu-system-" );
	if( paren >= 0 )
	{
		s = s.mid( paren );
		s.remove( ')' );
	}

	s.replace( '\\', '/' );
	if( s.contains( '/' ) )
		s = QFileInfo( s ).fileName();
#ifdef Q_OS_WIN
	if( s.endsWith( ".exe" ) )
		s.chop( 4 );
#endif

	if( s.startsWith( "qemu-system-" ) )
		s = s.mid( QString( "qemu-system-" ).size() );
	else if( s.startsWith( "qemu-" ) )
		s = s.mid( QString( "qemu-" ).size() );

	// Each qemu-system-*.exe keeps its own catalog. applesoc32, inferno, and
	// the Reims binaries do not share one machine list.

	// Strip trailing noise from captions like "aarch64 (softmmu)"
	const int sp = s.indexOf( ' ' );
	if( sp > 0 )
		s = s.left( sp );
	s.remove( '(' ).remove( ')' );
	return s;
}

QStringList QEMU_Probe_Catalog::Audio_Drivers( const QString &computer_type_or_binary )
{
	static QMap<QString, QStringList> cache;
	const QString key = Architecture_Key( computer_type_or_binary );
	if( key.isEmpty() )
		return QStringList();
	const QString dir = Catalog_Dir_For( computer_type_or_binary, key );
	const QString cache_key = dir + QLatin1Char( '\n' ) + key;
	if( cache.contains( cache_key ) )
		return cache.value( cache_key );

	QStringList out;
	if( ! dir.isEmpty() )
	{
		QFile f( QDir( dir ).filePath( key + QStringLiteral( ".json" ) ) );
		if( f.open( QIODevice::ReadOnly ) )
		{
			const QJsonDocument doc = QJsonDocument::fromJson( f.readAll() );
			const QString audio = doc.object().value( QLatin1String( "raw" ) ).toObject()
				.value( QLatin1String( "audio" ) ).toString();
			const QStringList lines = audio.split( QRegExp( QStringLiteral( "[\\r\\n]+" ) ),
				QString::SkipEmptyParts );
			for( int i = 0; i < lines.count(); ++i )
			{
				const QString t = lines.at( i ).trimmed().toLower();
				if( t.isEmpty() || t.contains( QLatin1Char( ' ' ) ) ||
				    t.startsWith( QLatin1String( "available" ) ) )
					continue;
				out << t;
			}
		}
	}
	out.removeDuplicates();
	cache.insert( cache_key, out );
	return out;
}

static QJsonObject Load_Probe_Root( const QString &computer_type_or_binary )
{
	const QString key = QEMU_Probe_Catalog::Architecture_Key( computer_type_or_binary );
	if( key.isEmpty() )
		return QJsonObject();
	const QString dir = Catalog_Dir_For( computer_type_or_binary, key );
	if( dir.isEmpty() )
		return QJsonObject();
	QFile f( QDir( dir ).filePath( key + QStringLiteral( ".json" ) ) );
	if( ! f.open( QIODevice::ReadOnly ) )
		return QJsonObject();
	const QJsonDocument doc = QJsonDocument::fromJson( f.readAll() );
	return doc.isObject() ? doc.object() : QJsonObject();
}

QStringList QEMU_Probe_Catalog::Section_Tokens( const QString &computer_type_or_binary,
                                                const QString &json_key )
{
	static QMap<QString, QStringList> cache;
	const QString stem = Architecture_Key( computer_type_or_binary );
	const QString key = Catalog_Dir_For( computer_type_or_binary, stem )
		+ QLatin1Char( '\n' ) + stem + QLatin1Char( '\n' ) + json_key;
	if( cache.contains( key ) )
		return cache.value( key );

	QStringList out;
	const QJsonObject root = Load_Probe_Root( computer_type_or_binary );
	const QJsonArray arr = root.value( json_key ).toArray();
	for( int i = 0; i < arr.size(); ++i )
	{
		QString line = arr.at( i ).toString().trimmed();
		if( line.isEmpty() )
			continue;
		const int colon = line.indexOf( QLatin1Char( ':' ) );
		if( colon >= 0 && ! line.startsWith( QLatin1String( "name " ) ) )
			line = line.mid( colon + 1 ).trimmed();
		const QStringList words = line.split( QRegExp( QStringLiteral( "\\s+" ) ),
			QString::SkipEmptyParts );
		for( int w = 0; w < words.size(); ++w )
		{
			QString tok = words.at( w ).trimmed().toLower();
			if( tok.isEmpty() || tok.contains( QLatin1Char( '"' ) ) )
				continue;
			if( tok == QLatin1String( "supported" ) || tok == QLatin1String( "available" )
				|| tok == QLatin1String( "formats" ) || tok == QLatin1String( "formats(read-only)" )
				|| tok == QLatin1String( "(read-only)" ) || tok.startsWith( QLatin1String( "some" ) )
				|| tok.startsWith( QLatin1String( "for" ) ) || tok.startsWith( QLatin1String( "list" ) )
				|| tok.startsWith( QLatin1String( "accelerators" ) ) || tok.startsWith( QLatin1String( "backend" ) ) )
				continue;
			if( tok.contains( QLatin1Char( ',' ) ) || tok.contains( QLatin1Char( '=' ) ) )
				continue;
			out << tok;
		}
	}
	out.removeDuplicates();
	cache.insert( key, out );
	return out;
}

QStringList QEMU_Probe_Catalog::USB_Controllers( const QString &computer_type_or_binary )
{
	static QMap<QString, QStringList> cache;
	const QString key = Architecture_Key( computer_type_or_binary );
	if( key.isEmpty() )
		return QStringList();
	if( cache.contains( key ) )
		return cache.value( key );

	QStringList out;
	const QJsonObject root = Load_Probe_Root( computer_type_or_binary );
	const QJsonArray arr = root.value( QLatin1String( "devices" ) ).toArray();
	QRegExp rx( "name\\s+\"([^\"]+)\"" );
	for( int i = 0; i < arr.size(); ++i )
	{
		const QString line = arr.at( i ).toString();
		if( rx.indexIn( line ) < 0 )
			continue;
		const QString name = rx.cap( 1 );
		const QString low = name.toLower();
		const bool usb = low.contains( QLatin1String( "xhci" ) )
			|| low.contains( QLatin1String( "uhci" ) )
			|| low.contains( QLatin1String( "ehci" ) )
			|| low.contains( QLatin1String( "ohci" ) );
		if( usb )
			out << name;
	}
	out.removeDuplicates();
	cache.insert( key, out );
	return out;
}

Architecture_Hardware_Capabilities QEMU_Probe_Catalog::Get_Hardware_Capabilities( const QString &computer_type_or_binary )
{
	static QMap<QString, Architecture_Hardware_Capabilities> s_cache;
	const QString key = Architecture_Key( computer_type_or_binary );
	if( key.isEmpty() )
		return Architecture_Hardware_Capabilities();

	const QString dir = Catalog_Dir_For( computer_type_or_binary, key );
	const QString cache_key = dir + QLatin1Char( '\n' ) + key;
	if( s_cache.contains( cache_key ) )
		return s_cache.value( cache_key );
	if( dir.isEmpty() )
		return Architecture_Hardware_Capabilities();

	const QString path = QDir( dir ).filePath( key + ".json" );
	QFile f( path );
	if( ! f.open( QIODevice::ReadOnly ) )
		return Architecture_Hardware_Capabilities();

	const QJsonDocument doc = QJsonDocument::fromJson( f.readAll() );
	if( ! doc.isObject() )
		return Architecture_Hardware_Capabilities();

	const QJsonObject root = doc.object();
	Architecture_Hardware_Capabilities caps;
	caps.valid = true;

	const QStringList devs = Json_String_List( root, "devices" );
	const QString all_devs = devs.join( QLatin1Char( ' ' ) );

	caps.has_ide = all_devs.contains( "ide-hd" ) || all_devs.contains( "ide-drive" ) ||
	               all_devs.contains( "piix3-ide" ) || all_devs.contains( "piix4-ide" ) ||
	               all_devs.contains( "cmd646-ide" );
	caps.has_ahci = all_devs.contains( "ahci" ) || all_devs.contains( "ich9-ahci" );
	caps.has_nvme = all_devs.contains( "nvme" );
	caps.has_virtio_disk = all_devs.contains( "virtio-blk-pci" ) ||
	                       all_devs.contains( "virtio-blk-device" ) ||
	                       all_devs.contains( "virtio-blk-ccw" );
	caps.has_virtio_net = all_devs.contains( "virtio-net-pci" ) ||
	                      all_devs.contains( "virtio-net-device" ) ||
	                      all_devs.contains( "virtio-net-ccw" );
	caps.has_virtio_gpu = all_devs.contains( "virtio-gpu-pci" ) ||
	                      all_devs.contains( "virtio-gpu-device" ) ||
	                      all_devs.contains( "virtio-vga" );
	caps.has_virtio_sound = all_devs.contains( "virtio-sound-pci" ) ||
	                        all_devs.contains( "virtio-sound-device" ) ||
	                        all_devs.contains( "virtio-snd" );
	caps.has_scsi = all_devs.contains( "scsi-hd" ) || all_devs.contains( "lsi53c895a" ) ||
	                all_devs.contains( "virtio-scsi-pci" ) || all_devs.contains( "virtio-scsi-device" ) ||
	                all_devs.contains( "esp" );
	caps.has_pci = all_devs.contains( "bus PCI" ) || all_devs.contains( "pci-host" ) ||
	               all_devs.contains( "pci-bridge" );

	QList<Device_Map> net_list, vid_list;
	VM::Sound_Cards audio_cards;
	Parse_Device_Help_Lines( devs, net_list, vid_list, &audio_cards );

	for( const auto &dm : net_list )
		caps.supported_network_cards.append( dm.QEMU_Name );
	for( const auto &dm : vid_list )
		caps.supported_video_cards.append( dm.QEMU_Name );

	caps.has_sound = audio_cards.isEnabled() || all_devs.contains( "intel-hda", Qt::CaseInsensitive ) ||
	                 all_devs.contains( "ac97", Qt::CaseInsensitive ) || all_devs.contains( "sb16", Qt::CaseInsensitive ) ||
	                 all_devs.contains( "es1370", Qt::CaseInsensitive ) || all_devs.contains( "cs4231a", Qt::CaseInsensitive ) ||
	                 all_devs.contains( "usb-audio", Qt::CaseInsensitive ) || all_devs.contains( "virtio-sound", Qt::CaseInsensitive );

	if( caps.has_pci && caps.has_sound )
	{
		if( all_devs.contains( "virtio-sound" ) )
			audio_cards.Audio_VirtIO = true;
		if( all_devs.contains( "usb-audio" ) )
			audio_cards.Audio_USB = true;
		if( all_devs.contains( "intel-hda" ) || all_devs.contains( "hda-duplex" ) )
			audio_cards.Audio_HDA = true;
	}
	else if( ! caps.has_sound )
	{
		audio_cards = VM::Sound_Cards();
	}
	caps.supported_sound_cards = audio_cards;

	const QStringList accels = Json_String_List( root, "accelerators" );
	caps.has_tcg = accels.contains( "tcg", Qt::CaseInsensitive );
	caps.has_kvm = accels.contains( "kvm", Qt::CaseInsensitive ) || accels.contains( "whpx", Qt::CaseInsensitive );

	s_cache.insert( cache_key, caps );
	return caps;
}

void QEMU_Probe_Catalog::Parse_Machine_Help_Lines( const QStringList &lines,
                                                   QList<Device_Map> &out )
{
	out.clear();
	QRegExp rx( "^(\\S+)\\s+(\\S.*)$" );
	for( int i = 0; i < lines.count(); ++i )
	{
		QString line = lines[i].trimmed();
		if( line.isEmpty() )
			continue;
		if( line.toLower().startsWith( "supported machines" ) )
			continue;
		if( ! rx.exactMatch( line ) )
			continue;
		const QString id = rx.cap( 1 );
		QString desc = rx.cap( 2 ).trimmed();
		// Drop "(alias of …)" / "(default)" noise from caption but keep id
		Append_Unique( out, Device_Map( Pretty_Caption( id, desc ), id ) );
	}
}

void QEMU_Probe_Catalog::Parse_CPU_Help_Lines( const QStringList &lines,
                                               QList<Device_Map> &out )
{
	out.clear();
	// "  603              PVR …" or "x86 pentium3" or "MIPS '4Kc'"
	QRegExp rx_ws( "^\\s*(\\S+)\\s+(\\S.*)$" );
	QRegExp rx_mips( ".*MIPS\\s+'([^']+)'.*" );
	QRegExp rx_x86( ".*x86\\s+\\[?([\\w.-]+)\\]?.*" );
	QRegExp rx_ppc( ".*PowerPC\\s+([\\w._-]+).*" );

	for( int i = 0; i < lines.count(); ++i )
	{
		QString line = lines[i];
		if( line.trimmed().isEmpty() )
			continue;
		if( line.contains( "Available CPUs", Qt::CaseInsensitive ) )
			continue;

		QString id;
		QString desc;
		if( rx_mips.exactMatch( line ) )
		{
			id = rx_mips.cap( 1 );
		}
		else if( rx_x86.exactMatch( line ) )
		{
			id = rx_x86.cap( 1 );
		}
		else if( rx_ppc.exactMatch( line ) )
		{
			id = rx_ppc.cap( 1 );
		}
		else if( rx_ws.exactMatch( line ) )
		{
			id = rx_ws.cap( 1 );
			desc = rx_ws.cap( 2 ).trimmed();
		}
		else
		{
			// Single-token line (e.g., "  apple-a13-cpu", "  cortex-a53")
			const QString item = line.trimmed();
			if( ! item.contains( ' ' ) && ! item.startsWith( '#' ) && ! item.startsWith( '-' ) )
			{
				id = item;
				desc = "";
			}
			else
				continue;
		}

		if( id.isEmpty() || id == "Available" || id == "Available CPUs:" )
			continue;
		Append_Unique( out, Device_Map( Pretty_Caption( id, desc ), id ) );
	}
}

void QEMU_Probe_Catalog::Parse_Device_Help_Lines( const QStringList &lines,
                                                  QList<Device_Map> &network_out,
                                                  QList<Device_Map> &display_out,
                                                  VM::Sound_Cards *audio_out )
{
	network_out.clear();
	display_out.clear();

	enum Section { Sec_None, Sec_Network, Sec_Display, Sec_Sound, Sec_Other };
	Section sec = Sec_None;

	QRegExp rx_name( "name\\s+\"([^\"]+)\"" );
	QRegExp rx_desc( "desc\\s+\"([^\"]+)\"" );

	for( int i = 0; i < lines.count(); ++i )
	{
		const QString line = lines[i];
		const QString t = line.trimmed();
		if( t.endsWith( "devices:", Qt::CaseInsensitive ) ||
		    t.endsWith( "Devices:" ) )
		{
			const QString low = t.toLower();
			if( low.startsWith( "network" ) )
				sec = Sec_Network;
			else if( low.startsWith( "display" ) )
				sec = Sec_Display;
			else if( low.startsWith( "sound" ) )
				sec = Sec_Sound;
			else
				sec = Sec_Other;
			continue;
		}

		if( rx_name.indexIn( line ) < 0 )
			continue;

		const QString name = rx_name.cap( 1 );
		QString desc;
		if( rx_desc.indexIn( line ) >= 0 )
			desc = rx_desc.cap( 1 );

		if( audio_out && ( sec == Sec_Sound ||
		                   name.contains( "hda", Qt::CaseInsensitive ) ||
		                   name.contains( "ac97", Qt::CaseInsensitive ) ||
		                   name.contains( "sb16", Qt::CaseInsensitive ) ||
		                   name.contains( "es1370", Qt::CaseInsensitive ) ||
		                   name.contains( "virtio-sound", Qt::CaseInsensitive ) ||
		                   name.contains( "usb-audio", Qt::CaseInsensitive ) ) )
			Apply_Audio_Token( *audio_out, name );

		if( sec == Sec_Network )
			Append_Unique( network_out, Device_Map( Pretty_Caption( name, desc ), name ) );
		else if( sec == Sec_Display )
		{
			const QString alias = Normalize_Display_Alias( name );
			Append_Unique( display_out,
			               Device_Map( Pretty_Caption( alias, desc.isEmpty() ? name : desc ), alias ) );
			// Also keep the raw device name when it differs (e.g. ati-vga)
			if( alias.compare( name, Qt::CaseInsensitive ) != 0 )
				Append_Unique( display_out, Device_Map( Pretty_Caption( name, desc ), name ) );
		}
	}

	// Always offer none
	Append_Unique( display_out, Device_Map( QObject::tr( "None" ), "none" ) );
}

bool QEMU_Probe_Catalog::Load_Architecture( const QString &computer_type_or_binary,
                                            Available_Devices &out )
{
	const QString key = Architecture_Key( computer_type_or_binary );
	if( key.isEmpty() )
		return false;

	const QString dir = Catalog_Dir_For( computer_type_or_binary, key );
	if( dir.isEmpty() )
		return false;

	const QString path = QDir( dir ).filePath( key + ".json" );
	QFile f( path );
	if( ! f.open( QIODevice::ReadOnly ) )
		return false;

	const QJsonDocument doc = QJsonDocument::fromJson( f.readAll() );
	if( ! doc.isObject() )
		return false;

	const QJsonObject root = doc.object();
	Available_Devices ad;
	ad.System.QEMU_Name = QString( "qemu-system-%1" ).arg( key );
	QString caption = key;
	if( ! caption.isEmpty() )
		caption[0] = caption[0].toUpper();
	ad.System.Caption = QObject::tr( "%1 (%2)" ).arg( caption, ad.System.QEMU_Name );

	Parse_Machine_Help_Lines( Json_String_List( root, "machines" ), ad.Machine_List );
	Parse_CPU_Help_Lines( Json_String_List( root, "cpus" ), ad.CPU_List );

	VM::Sound_Cards audio;
	Parse_Device_Help_Lines( Json_String_List( root, "devices" ),
	                         ad.Network_Card_List, ad.Video_Card_List, &audio );
	ad.Audio_Card_List = audio;

	const QString all_devs_str = Json_String_List( root, "devices" ).join( QLatin1Char( ' ' ) );
	const bool arch_has_pci = all_devs_str.contains( "bus PCI" ) || all_devs_str.contains( "pci-host" );
	const bool arch_has_sound = ad.Audio_Card_List.isEnabled() || all_devs_str.contains( "intel-hda", Qt::CaseInsensitive ) ||
	                            all_devs_str.contains( "ac97", Qt::CaseInsensitive ) || all_devs_str.contains( "sb16", Qt::CaseInsensitive );

	if( arch_has_pci && arch_has_sound )
	{
		if( all_devs_str.contains( "virtio-sound" ) )
			ad.Audio_Card_List.Audio_VirtIO = true;
		if( all_devs_str.contains( "usb-audio" ) )
			ad.Audio_Card_List.Audio_USB = true;
		if( all_devs_str.contains( "intel-hda" ) || all_devs_str.contains( "hda-duplex" ) )
			ad.Audio_Card_List.Audio_HDA = true;
	}
	else if( ! arch_has_sound )
	{
		ad.Audio_Card_List = VM::Sound_Cards();
	}

	const QStringList device_lines = Json_String_List( root, "devices" );
	Parse_Grouped_Devices( device_lines, ad );

	const QStringList object_lines = Json_String_List( root, "object_types" );
	for( int oi = 0; oi < object_lines.count(); ++oi )
	{
		const QString name = object_lines.at( oi ).trimmed();
		if( name.isEmpty() || name.contains( QLatin1Char( ' ' ) ) || name.endsWith( QLatin1Char( ':' ) ) )
			continue;
		Append_Unique( ad.Object_Type_List, Device_Map( name, name ) );
	}

	ad.Netdev_Backends = Bare_Tokens( Json_String_List( root, "netdev" ) );
	ad.Drive_Formats = Format_Tokens( Json_String_List( root, "drive_formats" ) );
	ad.Chardev_Backends = Bare_Tokens( Json_String_List( root, "chardev_backends" ) );
	ad.Display_Backends = Bare_Tokens( Json_String_List( root, "display_backends" ) );
	ad.Audio_Backends = Bare_Tokens( Json_String_List( root, "audio_backends" ) );
	ad.Accelerators = Bare_Tokens( Json_String_List( root, "accelerators" ) );

	if( ad.Machine_List.isEmpty() && ad.CPU_List.isEmpty() )
		return false;

	out = ad;
	AQDebug( "QEMU_Probe_Catalog::Load_Architecture",
	         QString( "Loaded %1: machines=%2 cpus=%3 net=%4 video=%5" )
	             .arg( key )
	             .arg( out.Machine_List.count() )
	             .arg( out.CPU_List.count() )
	             .arg( out.Network_Card_List.count() )
	             .arg( out.Video_Card_List.count() ) );
	return true;
}

bool QEMU_Probe_Catalog::Architecture_Has_CPU( const QString &computer_type_or_binary,
                                               const QString &cpu_name )
{
	if( cpu_name.isEmpty() )
		return false;
	Available_Devices ad;
	if( ! Load_Architecture( computer_type_or_binary, ad ) )
		return false;
	for( int i = 0; i < ad.CPU_List.count(); ++i )
	{
		if( ad.CPU_List[i].QEMU_Name.compare( cpu_name, Qt::CaseInsensitive ) == 0 )
			return true;
	}
	return false;
}

QString QEMU_Probe_Catalog::First_Available_CPU( const QString &computer_type_or_binary,
                                                 const QStringList &candidates )
{
	Available_Devices ad;
	const bool have = Load_Architecture( computer_type_or_binary, ad );
	for( int i = 0; i < candidates.count(); ++i )
	{
		const QString c = candidates[i].trimmed();
		if( c.isEmpty() )
			continue;
		if( ! have )
			return c; // no probe — trust caller order
		for( int j = 0; j < ad.CPU_List.count(); ++j )
		{
			if( ad.CPU_List[j].QEMU_Name.compare( c, Qt::CaseInsensitive ) == 0 )
				return ad.CPU_List[j].QEMU_Name;
		}
	}
	// Never fall back to list[0] if that is 486 when better options exist
	if( have )
	{
		for( int j = 0; j < ad.CPU_List.count(); ++j )
		{
			const QString n = ad.CPU_List[j].QEMU_Name;
			if( n.compare( QLatin1String( "486" ), Qt::CaseInsensitive ) == 0 ||
			    n.startsWith( QLatin1String( "486-" ), Qt::CaseInsensitive ) )
				continue;
			return n;
		}
		if( ! ad.CPU_List.isEmpty() )
			return ad.CPU_List.first().QEMU_Name;
	}
	return candidates.isEmpty() ? QString() : candidates.first();
}

bool QEMU_Probe_Catalog::Merge_Into( Available_Devices &dev )
{
	Available_Devices probe;
	const QString key_src = ! dev.System.QEMU_Name.isEmpty()
		? dev.System.QEMU_Name
		: dev.System.Caption;
	if( ! Load_Architecture( key_src, probe ) )
		return false;

	if( ! probe.Machine_List.isEmpty() )
		dev.Machine_List = probe.Machine_List;
	if( ! probe.CPU_List.isEmpty() )
		dev.CPU_List = probe.CPU_List;
	if( ! probe.Network_Card_List.isEmpty() )
		dev.Network_Card_List = probe.Network_Card_List;
	if( ! probe.Video_Card_List.isEmpty() )
		dev.Video_Card_List = probe.Video_Card_List;
	if( ! probe.Storage_Device_List.isEmpty() )
		dev.Storage_Device_List = probe.Storage_Device_List;
	if( ! probe.Sound_Device_List.isEmpty() )
		dev.Sound_Device_List = probe.Sound_Device_List;
	if( ! probe.USB_Device_List.isEmpty() )
		dev.USB_Device_List = probe.USB_Device_List;
	if( ! probe.Input_Device_List.isEmpty() )
		dev.Input_Device_List = probe.Input_Device_List;
	if( ! probe.Controller_Device_List.isEmpty() )
		dev.Controller_Device_List = probe.Controller_Device_List;
	if( ! probe.Misc_Device_List.isEmpty() )
		dev.Misc_Device_List = probe.Misc_Device_List;
	if( ! probe.Watchdog_Device_List.isEmpty() )
		dev.Watchdog_Device_List = probe.Watchdog_Device_List;
	if( ! probe.CPU_Device_List.isEmpty() )
		dev.CPU_Device_List = probe.CPU_Device_List;
	if( ! probe.Other_Device_List.isEmpty() )
		dev.Other_Device_List = probe.Other_Device_List;
	if( ! probe.Object_Type_List.isEmpty() )
		dev.Object_Type_List = probe.Object_Type_List;
	if( ! probe.Netdev_Backends.isEmpty() )
		dev.Netdev_Backends = probe.Netdev_Backends;
	if( ! probe.Drive_Formats.isEmpty() )
		dev.Drive_Formats = probe.Drive_Formats;
	if( ! probe.Chardev_Backends.isEmpty() )
		dev.Chardev_Backends = probe.Chardev_Backends;
	if( ! probe.Display_Backends.isEmpty() )
		dev.Display_Backends = probe.Display_Backends;
	if( ! probe.Audio_Backends.isEmpty() )
		dev.Audio_Backends = probe.Audio_Backends;
	if( ! probe.Accelerators.isEmpty() )
		dev.Accelerators = probe.Accelerators;

	dev.Audio_Card_List.Audio_sb16 =
		dev.Audio_Card_List.Audio_sb16 || probe.Audio_Card_List.Audio_sb16;
	dev.Audio_Card_List.Audio_es1370 =
		dev.Audio_Card_List.Audio_es1370 || probe.Audio_Card_List.Audio_es1370;
	dev.Audio_Card_List.Audio_Adlib =
		dev.Audio_Card_List.Audio_Adlib || probe.Audio_Card_List.Audio_Adlib;
	dev.Audio_Card_List.Audio_PC_Speaker =
		dev.Audio_Card_List.Audio_PC_Speaker || probe.Audio_Card_List.Audio_PC_Speaker;
	dev.Audio_Card_List.Audio_GUS =
		dev.Audio_Card_List.Audio_GUS || probe.Audio_Card_List.Audio_GUS;
	dev.Audio_Card_List.Audio_AC97 =
		dev.Audio_Card_List.Audio_AC97 || probe.Audio_Card_List.Audio_AC97;
	dev.Audio_Card_List.Audio_HDA =
		dev.Audio_Card_List.Audio_HDA || probe.Audio_Card_List.Audio_HDA;
	dev.Audio_Card_List.Audio_cs4231a =
		dev.Audio_Card_List.Audio_cs4231a || probe.Audio_Card_List.Audio_cs4231a;
	dev.Audio_Card_List.Audio_VirtIO =
		dev.Audio_Card_List.Audio_VirtIO || probe.Audio_Card_List.Audio_VirtIO;
	dev.Audio_Card_List.Audio_USB =
		dev.Audio_Card_List.Audio_USB || probe.Audio_Card_List.Audio_USB;

	if( dev.System.QEMU_Name.isEmpty() )
		dev.System.QEMU_Name = probe.System.QEMU_Name;
	return true;
}
