/****************************************************************************
** Install-media OS guess: filename + ISO9660 volume ID (+ optional libosinfo).
****************************************************************************/

#include "ISO_Guess.h"

#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QProcess>
#include <QtGlobal>

QString AQ_Read_ISO9660_Volume_Id( const QString &path )
{
	QFile f( path );
	if( ! f.open( QIODevice::ReadOnly ) )
		return QString();

	// ISO9660 PVD starts at sector 16 (2048-byte sectors) = offset 32768
	if( ! f.seek( 16 * 2048 ) )
		return QString();
	const QByteArray sector = f.read( 2048 );
	if( sector.size() < 88 )
		return QString();
	// Type 1 = Primary Volume Descriptor; magic "CD001" at offset 1
	if( static_cast<unsigned char>( sector.at( 0 ) ) != 1 )
		return QString();
	if( ! sector.mid( 1, 5 ).startsWith( "CD001" ) )
		return QString();
	// Volume identifier at offset 40, length 32
	QString vol = QString::fromLatin1( sector.mid( 40, 32 ) ).trimmed();
	while( vol.endsWith( QLatin1Char( ' ' ) ) )
		vol.chop( 1 );
	return vol;
}

static ISO_Guess_Result guess_from_text( const QString &text, const char *source )
{
	ISO_Guess_Result r;
	const QString t = text.toLower();
	auto hit = [&]( const QString &os, const char *level, const QString &tip ) {
		r.os_name = os;
		r.confidence = QString::fromUtf8( level );
		r.tip = tip + QObject::tr( " (via %1)" ).arg( QString::fromUtf8( source ) );
	};

	if( t.contains( "win11" ) || t.contains( "windows 11" ) || t.contains( "windows11" ) )
		hit( QStringLiteral( "Windows 11" ), "high", QObject::tr( "Detected Windows 11." ) );
	else if( t.contains( "win10" ) || t.contains( "windows 10" ) || t.contains( "windows10" ) )
		hit( QStringLiteral( "Windows 10 (64-bit)" ), "high", QObject::tr( "Detected Windows 10." ) );
	else if( t.contains( "win7" ) || t.contains( "windows 7" ) )
		hit( QStringLiteral( "Windows 7 (64-bit)" ), "medium", QObject::tr( "Detected Windows 7." ) );
	else if( t.contains( "winxp" ) || t.contains( "windows xp" ) || t.contains( "xrpfreed" ) )
		hit( QStringLiteral( "Windows XP (32-bit)" ), "medium", QObject::tr( "Detected Windows XP." ) );
	else if( t.contains( "win98" ) || t.contains( "windows 98" ) )
		hit( QStringLiteral( "Windows 98" ), "high", QObject::tr( "Detected Windows 98." ) );
	else if( t.contains( "ubuntu" ) )
		hit( QStringLiteral( "Ubuntu (64-bit)" ), "high", QObject::tr( "Detected Ubuntu." ) );
	else if( t.contains( "debian" ) )
		hit( QStringLiteral( "Debian (64-bit)" ), "high", QObject::tr( "Detected Debian." ) );
	else if( t.contains( "fedora" ) )
		hit( QStringLiteral( "Fedora (64-bit)" ), "high", QObject::tr( "Detected Fedora." ) );
	else if( t.contains( "arch" ) && t.contains( "linux" ) )
		hit( QStringLiteral( "Arch Linux (64-bit)" ), "medium", QObject::tr( "Detected Arch Linux." ) );
	else if( t.contains( "kali" ) )
		hit( QStringLiteral( "Kali Linux" ), "high", QObject::tr( "Detected Kali." ) );
	else if( t.contains( "opensuse" ) || t.contains( "suse" ) )
		hit( QStringLiteral( "openSUSE (64-bit)" ), "medium", QObject::tr( "Detected openSUSE." ) );
	else if( t.contains( "rocky" ) )
		hit( QStringLiteral( "Rocky Linux" ), "high", QObject::tr( "Detected Rocky." ) );
	else if( t.contains( "alma" ) )
		hit( QStringLiteral( "AlmaLinux" ), "high", QObject::tr( "Detected AlmaLinux." ) );
	else if( t.contains( "centos" ) )
		hit( QStringLiteral( "CentOS Stream" ), "medium", QObject::tr( "Detected CentOS." ) );
	else if( t.contains( "mint" ) )
		hit( QStringLiteral( "Linux Mint (64-bit)" ), "high", QObject::tr( "Detected Linux Mint." ) );
	else if( t.contains( "freebsd" ) )
		hit( QStringLiteral( "FreeBSD (64-bit)" ), "high", QObject::tr( "Detected FreeBSD." ) );
	else if( t.contains( "reactos" ) )
		hit( QStringLiteral( "ReactOS (32-bit)" ), "high", QObject::tr( "Detected ReactOS." ) );
	else if( t.contains( "haiku" ) )
		hit( QStringLiteral( "Haiku (64-bit)" ), "high", QObject::tr( "Detected Haiku." ) );
	else if( t.contains( "freedos" ) )
		hit( QStringLiteral( "FreeDOS" ), "high", QObject::tr( "Detected FreeDOS." ) );
	else if( t.contains( "msdos" ) || t.contains( "ms-dos" ) )
		hit( QStringLiteral( "MS-DOS" ), "medium", QObject::tr( "Detected MS-DOS." ) );
	else if( t.contains( "linux" ) )
		hit( QStringLiteral( "Generic Linux (64-bit)" ), "low", QObject::tr( "Generic Linux hint." ) );
	return r;
}

#ifndef Q_OS_WIN
static ISO_Guess_Result try_libosinfo( const QString &path )
{
	ISO_Guess_Result r;
	// Soft dependency: if libosinfo-1.0 is present, call via glib-style is too heavy.
	// Instead look for `osinfo-detect` CLI shipped with libosinfo tools.
	QProcess proc;
	proc.start( QStringLiteral( "osinfo-detect" ),
		QStringList() << QStringLiteral( "--type=media" ) << path );
	if( ! proc.waitForFinished( 8000 ) )
	{
		proc.kill();
		return r;
	}
	const QString out = QString::fromLocal8Bit( proc.readAllStandardOutput() )
		+ QString::fromLocal8Bit( proc.readAllStandardError() );
	if( out.isEmpty() )
		return r;
	r = guess_from_text( out, "libosinfo/osinfo-detect" );
	if( r.os_name.isEmpty() )
	{
		r.tip = QObject::tr( "osinfo-detect output:\n%1" ).arg( out.left( 400 ) );
		r.confidence = QStringLiteral( "low" );
	}
	return r;
}
#endif

ISO_Guess_Result AQ_Guess_OS_From_Media( const QString &path )
{
	ISO_Guess_Result r;
	if( path.trimmed().isEmpty() )
		return r;

	const QFileInfo fi( path );
	const QString name = fi.fileName().toLower();
	const QString base = fi.completeBaseName().toLower();

#ifndef Q_OS_WIN
	{
		ISO_Guess_Result lo = try_libosinfo( path );
		if( ! lo.os_name.isEmpty() )
		{
			lo.volume_id = AQ_Read_ISO9660_Volume_Id( path );
			return lo;
		}
	}
#endif

	const QString vol = AQ_Read_ISO9660_Volume_Id( path );
	r.volume_id = vol;
	if( ! vol.isEmpty() )
	{
		const ISO_Guess_Result from_vol = guess_from_text( vol, "ISO9660 volume ID" );
		if( ! from_vol.os_name.isEmpty() )
		{
			r = from_vol;
			r.volume_id = vol;
			return r;
		}
	}

	auto hit = [&]( const QString &os, const char *level, const QString &tip ) {
		r.os_name = os;
		r.confidence = QString::fromUtf8( level );
		r.tip = tip;
	};

	if( name.contains( "win11" ) || name.contains( "windows11" ) || name.contains( "windows_11" ) )
		hit( QStringLiteral( "Windows 11" ), "high", QObject::tr( "Filename looks like Windows 11." ) );
	else if( name.contains( "win10" ) || name.contains( "windows10" ) )
		hit( name.contains( "x86" ) && ! name.contains( "x64" ) && ! name.contains( "64" )
			? QStringLiteral( "Windows 10 (32-bit)" )
			: QStringLiteral( "Windows 10 (64-bit)" ),
		     "high", QObject::tr( "Filename looks like Windows 10." ) );
	else if( name.contains( "win7" ) || name.contains( "windows7" ) )
		hit( name.contains( "x64" ) || name.contains( "64" )
			? QStringLiteral( "Windows 7 (64-bit)" )
			: QStringLiteral( "Windows 7 (32-bit)" ),
		     "medium", QObject::tr( "Filename looks like Windows 7." ) );
	else if( name.contains( "winxp" ) || name.contains( "windowsxp" ) || name.contains( "wxp" ) )
		hit( QStringLiteral( "Windows XP (32-bit)" ), "medium",
		     QObject::tr( "Filename looks like Windows XP." ) );
	else if( name.contains( "win98" ) || name.contains( "windows98" ) )
		hit( QStringLiteral( "Windows 98" ), "high",
		     QObject::tr( "Filename looks like Windows 98." ) );
	else if( name.contains( "win95" ) || name.contains( "windows95" ) )
		hit( QStringLiteral( "Windows 95" ), "high",
		     QObject::tr( "Filename looks like Windows 95." ) );
	else if( name.contains( "msdos" ) || name.contains( "freedos" ) )
		hit( name.contains( "freedos" ) ? QStringLiteral( "FreeDOS" ) : QStringLiteral( "MS-DOS" ),
		     "medium", QObject::tr( "Filename looks like DOS." ) );
	else if( name.contains( "ubuntu" ) )
		hit( name.contains( "i386" ) || name.contains( "32" )
			? QStringLiteral( "Ubuntu (32-bit)" )
			: QStringLiteral( "Ubuntu (64-bit)" ),
		     "high", QObject::tr( "Filename looks like Ubuntu." ) );
	else if( name.contains( "debian" ) )
		hit( name.contains( "i386" ) || name.contains( "32" )
			? QStringLiteral( "Debian (32-bit)" )
			: QStringLiteral( "Debian (64-bit)" ),
		     "high", QObject::tr( "Filename looks like Debian." ) );
	else if( name.contains( "fedora" ) )
		hit( QStringLiteral( "Fedora (64-bit)" ), "high",
		     QObject::tr( "Filename looks like Fedora." ) );
	else if( name.contains( "archlinux" ) || name.contains( "arch-linux" ) ||
	         ( name.startsWith( "arch" ) && name.contains( "iso" ) ) )
		hit( QStringLiteral( "Arch Linux (64-bit)" ), "medium",
		     QObject::tr( "Filename looks like Arch Linux." ) );
	else if( name.contains( "kali" ) )
		hit( QStringLiteral( "Kali Linux" ), "high",
		     QObject::tr( "Filename looks like Kali Linux." ) );
	else if( name.contains( "opensuse" ) || name.contains( "leap" ) || name.contains( "tumbleweed" ) )
		hit( QStringLiteral( "openSUSE (64-bit)" ), "medium",
		     QObject::tr( "Filename looks like openSUSE." ) );
	else if( name.contains( "centos" ) || name.contains( "rocky" ) || name.contains( "alma" ) )
		hit( name.contains( "rocky" ) ? QStringLiteral( "Rocky Linux" )
		     : name.contains( "alma" ) ? QStringLiteral( "AlmaLinux" )
		                               : QStringLiteral( "CentOS Stream" ),
		     "medium", QObject::tr( "Filename looks like an RHEL-family ISO." ) );
	else if( name.contains( "mint" ) )
		hit( QStringLiteral( "Linux Mint (64-bit)" ), "high",
		     QObject::tr( "Filename looks like Linux Mint." ) );
	else if( name.contains( "reactos" ) )
		hit( QStringLiteral( "ReactOS (32-bit)" ), "high",
		     QObject::tr( "Filename looks like ReactOS." ) );
	else if( name.contains( "haiku" ) )
		hit( QStringLiteral( "Haiku (64-bit)" ), "high",
		     QObject::tr( "Filename looks like Haiku." ) );
	else if( name.contains( "freebsd" ) )
		hit( name.contains( "i386" ) || name.contains( "32" )
			? QStringLiteral( "FreeBSD (32-bit)" )
			: QStringLiteral( "FreeBSD (64-bit)" ),
		     "high", QObject::tr( "Filename looks like FreeBSD." ) );
	else if( name.contains( "netbsd" ) )
		hit( QStringLiteral( "NetBSD (64-bit)" ), "medium",
		     QObject::tr( "Filename looks like NetBSD." ) );
	else if( name.contains( "openbsd" ) )
		hit( QStringLiteral( "OpenBSD (64-bit)" ), "medium",
		     QObject::tr( "Filename looks like OpenBSD." ) );
	else if( name.contains( "solaris" ) || name.contains( "illumos" ) || name.contains( "omnios" ) )
		hit( name.contains( "sparc" ) ? QStringLiteral( "Solaris SPARC" )
		                              : QStringLiteral( "Solaris x86" ),
		     "medium", QObject::tr( "Filename looks like Solaris/illumos." ) );
	else if( name.contains( "macos" ) || name.contains( "osx" ) || name.contains( "mac_os" ) )
		hit( QStringLiteral( "macOS" ), "low",
		     QObject::tr( "Filename hints at macOS — confirm Intel vs PPC carefully." ) );
	else if( base.contains( "linux" ) || name.contains( "linux" ) )
		hit( QStringLiteral( "Generic Linux (64-bit)" ), "low",
		     QObject::tr( "Generic Linux hint from filename." ) );
	else
	{
		if( ! vol.isEmpty() )
			r.tip = QObject::tr( "ISO volume “%1” — could not map to a known OS. Pick Guest OS manually." )
				.arg( vol );
		else
			r.tip = QObject::tr( "Could not guess OS from “%1” — pick Guest OS manually." )
				.arg( fi.fileName() );
		r.confidence = QStringLiteral( "none" );
	}

	if( ! vol.isEmpty() && r.tip.indexOf( vol ) < 0 )
		r.tip += QObject::tr( " Volume ID: %1." ).arg( vol );

	return r;
}
