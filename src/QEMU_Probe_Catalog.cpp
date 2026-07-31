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

	// Strip trailing noise from captions like "aarch64 (softmmu)"
	const int sp = s.indexOf( ' ' );
	if( sp > 0 )
		s = s.left( sp );
	s.remove( '(' ).remove( ')' );
	return s;
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

	const QString dir = Probe_Directory();
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

	// Modern PCI guests can take these even when -soundhw is gone
	ad.Audio_Card_List.Audio_VirtIO = true;
	ad.Audio_Card_List.Audio_USB = true;
	if( ! ad.Audio_Card_List.Audio_HDA )
		ad.Audio_Card_List.Audio_HDA = true;

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
