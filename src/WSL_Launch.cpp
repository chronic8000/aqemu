/****************************************************************************
** WSL/KVM launch helpers (Windows host → Linux QEMU inside WSL)
****************************************************************************/

#include "WSL_Launch.h"
#include "Utils.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QTextCodec>

#ifdef Q_OS_WIN32

namespace {

QMutex g_wsl_cache_mutex;
bool g_wsl_avail_valid = false;
bool g_wsl_avail = false;
qint64 g_wsl_avail_ms = 0;

bool g_wsl_kvm_valid = false;
bool g_wsl_kvm = false;
QString g_wsl_kvm_distro;
qint64 g_wsl_kvm_ms = 0;

const qint64 kSuccessCacheTtlMs = 60 * 1000;
const qint64 kFailureCacheTtlMs = 3 * 1000;
const int kWslTimeoutMs = 15000;

bool Cache_Fresh( qint64 stamped_ms, bool was_ok )
{
	if( stamped_ms <= 0 )
		return false;
	const qint64 ttl = was_ok ? kSuccessCacheTtlMs : kFailureCacheTtlMs;
	return ( QDateTime::currentMSecsSinceEpoch() - stamped_ms ) < ttl;
}

QString Decode_WSL_Output( const QByteArray &raw )
{
	if( raw.isEmpty() )
		return QString();

	// wsl.exe --status (and some other commands) emit UTF-16LE
	if( raw.size() >= 2 )
	{
		const uchar b0 = uchar( raw.at( 0 ) );
		const uchar b1 = uchar( raw.at( 1 ) );
		const bool bom_le = ( b0 == 0xFF && b1 == 0xFE );
		const bool bom_be = ( b0 == 0xFE && b1 == 0xFF );
		const bool looks_utf16le = bom_le ||
			( raw.size() >= 4 && b1 == 0x00 && uchar( raw.at( 3 ) ) == 0x00 );
		if( bom_le || bom_be || looks_utf16le )
		{
			QTextCodec *codec = QTextCodec::codecForName( bom_be ? "UTF-16BE" : "UTF-16LE" );
			if( codec )
				return codec->toUnicode( raw ).trimmed();
		}
	}
	return QString::fromLocal8Bit( raw ).trimmed();
}

QStringList Distro_Args( const QString &distro )
{
	QStringList args;
	const QString d = distro.trimmed();
	if( ! d.isEmpty() )
		args << QStringLiteral( "-d" ) << d;
	return args;
}

bool Run_WSL( const QStringList &args, int timeout_ms, QString *stdout_text = nullptr )
{
	QProcess p;
	p.start( QStringLiteral( "wsl.exe" ), args );
	if( ! p.waitForFinished( timeout_ms ) )
	{
		p.kill();
		p.waitForFinished( 1000 );
		return false;
	}
	if( p.exitStatus() != QProcess::NormalExit )
		return false;
	if( stdout_text )
		*stdout_text = Decode_WSL_Output( p.readAllStandardOutput() );
	return p.exitCode() == 0;
}

} // namespace

void WSL_Clear_Probe_Cache()
{
	QMutexLocker lock( &g_wsl_cache_mutex );
	g_wsl_avail_valid = false;
	g_wsl_kvm_valid = false;
	g_wsl_avail_ms = 0;
	g_wsl_kvm_ms = 0;
}

bool WSL_Is_Available( bool force_refresh )
{
	{
		QMutexLocker lock( &g_wsl_cache_mutex );
		if( ! force_refresh && g_wsl_avail_valid && Cache_Fresh( g_wsl_avail_ms, g_wsl_avail ) )
			return g_wsl_avail;
	}

	// Prefer a cheap execute probe — `wsl --status` is UTF-16 and often slow to start.
	bool ok = Run_WSL( QStringList() << QStringLiteral( "-e" ) << QStringLiteral( "true" ),
	                   kWslTimeoutMs );
	if( ! ok )
		ok = Run_WSL( QStringList() << QStringLiteral( "--" ) << QStringLiteral( "true" ),
		              kWslTimeoutMs );

	QMutexLocker lock( &g_wsl_cache_mutex );
	g_wsl_avail = ok;
	g_wsl_avail_valid = true;
	g_wsl_avail_ms = QDateTime::currentMSecsSinceEpoch();
	return ok;
}

bool WSL_Has_KVM( const QString &distro, bool force_refresh )
{
	const QString d = distro.trimmed();
	{
		QMutexLocker lock( &g_wsl_cache_mutex );
		if( ! force_refresh && g_wsl_kvm_valid && g_wsl_kvm_distro == d &&
		    Cache_Fresh( g_wsl_kvm_ms, g_wsl_kvm ) )
			return g_wsl_kvm;
	}

	// Require read+write — `test -e` alone is not enough (Permission denied on open).
	QStringList args = Distro_Args( d );
	args << QStringLiteral( "--" ) << QStringLiteral( "test" )
	     << QStringLiteral( "-r" ) << QStringLiteral( "/dev/kvm" )
	     << QStringLiteral( "-a" )
	     << QStringLiteral( "-w" ) << QStringLiteral( "/dev/kvm" );

	bool ok = Run_WSL( args, kWslTimeoutMs );

	if( ok )
	{
		// Successful KVM probe also proves WSL itself is available.
		QMutexLocker lock( &g_wsl_cache_mutex );
		g_wsl_avail = true;
		g_wsl_avail_valid = true;
		g_wsl_avail_ms = QDateTime::currentMSecsSinceEpoch();
	}

	QMutexLocker lock( &g_wsl_cache_mutex );
	g_wsl_kvm = ok;
	g_wsl_kvm_valid = true;
	g_wsl_kvm_distro = d;
	g_wsl_kvm_ms = QDateTime::currentMSecsSinceEpoch();
	return ok;
}

bool WSL_Ensure_KVM_Access( const QString &distro )
{
	const QString d = distro.trimmed();

	if( ! WSL_Is_Available( false ) )
		return false;

	// macOS guests (and many OSX-KVM recipes) need ignore_msrs=1 or the
	// kernel hangs after OpenCore with "still waiting for root device".
	{
		QStringList root_msrs;
		if( ! d.isEmpty() )
			root_msrs << QStringLiteral( "-d" ) << d;
		root_msrs << QStringLiteral( "-u" ) << QStringLiteral( "root" ) << QStringLiteral( "--" )
		          << QStringLiteral( "bash" ) << QStringLiteral( "-lc" )
		          << QStringLiteral(
				"if [ -w /sys/module/kvm/parameters/ignore_msrs ]; then "
				"echo 1 > /sys/module/kvm/parameters/ignore_msrs; fi; "
				"mkdir -p /etc/modprobe.d 2>/dev/null || true; "
				"printf 'options kvm ignore_msrs=Y report_ignored_msrs=0\\n' "
				"> /etc/modprobe.d/aqemu-kvm-macos.conf 2>/dev/null || true; "
				"exit 0" );
		Run_WSL( root_msrs, kWslTimeoutMs );
	}

	if( WSL_Has_KVM( d, true ) )
		return true;

	// Resolve the default (non-root) username for this distro
	QString user;
	{
		QStringList who = Distro_Args( d );
		who << QStringLiteral( "--" ) << QStringLiteral( "whoami" );
		if( ! Run_WSL( who, kWslTimeoutMs, &user ) || user.trimmed().isEmpty() )
			user = QStringLiteral( "root" ); // shouldn't happen; chmod still helps
		user = user.trimmed();
		// Defensive: only allow safe username chars
		for( const QChar &ch : user )
		{
			if( ! ( ch.isLetterOrNumber() || ch == QLatin1Char( '_' ) || ch == QLatin1Char( '-' ) ) )
			{
				user.clear();
				break;
			}
		}
	}

	// WSL root is typically passwordless from Windows — fix group + immediate access.
	QStringList root_args;
	if( ! d.isEmpty() )
		root_args << QStringLiteral( "-d" ) << d;
	root_args << QStringLiteral( "-u" ) << QStringLiteral( "root" ) << QStringLiteral( "--" )
	          << QStringLiteral( "bash" ) << QStringLiteral( "-lc" );

	QString script = QStringLiteral(
		"getent group kvm >/dev/null 2>&1 || groupadd -r kvm 2>/dev/null || true; "
		"if [ -c /dev/kvm ]; then chmod 666 /dev/kvm 2>/dev/null || true; fi; " );
	if( ! user.isEmpty() && user != QLatin1String( "root" ) )
	{
		script += QStringLiteral( "usermod -aG kvm '%1' 2>/dev/null || true; " ).arg( user );
		// Persist across WSL restarts when possible (udev/rules or module load)
		script += QStringLiteral(
			"mkdir -p /etc/udev/rules.d 2>/dev/null || true; "
			"printf 'KERNEL==\"kvm\", GROUP=\"kvm\", MODE=\"0666\"\\n' "
			"> /etc/udev/rules.d/99-aqemu-kvm.rules 2>/dev/null || true; " );
	}
	script += QStringLiteral( "exit 0" );
	root_args << script;

	Run_WSL( root_args, kWslTimeoutMs );

	WSL_Clear_Probe_Cache();
	return WSL_Has_KVM( d, true );
}

QString Windows_Path_To_WSL( const QString &windows_path )
{
	QString p = AQ_Normalize_File_Path( windows_path );
	if( p.isEmpty() )
		return p;

	p = QDir::fromNativeSeparators( p );

	// Already a WSL path
	if( p.startsWith( QLatin1String( "/mnt/" ) ) || p.startsWith( QLatin1Char( '/' ) ) )
		return p;

	// UNC / relative — leave as-is (caller may not rewrite)
	QRegularExpression drive_re( QStringLiteral( "^([A-Za-z]):(/.*)?$" ) );
	QRegularExpressionMatch m = drive_re.match( p );
	if( ! m.hasMatch() )
		return p;

	const QString letter = m.captured( 1 ).toLower();
	QString rest = m.captured( 2 );
	if( rest.isEmpty() )
		rest = QStringLiteral( "/" );
	return QStringLiteral( "/mnt/%1%2" ).arg( letter, rest );
}

static QString Rewrite_Drive_File_Option( const QString &opt )
{
	// Rewrite file= / file.filename= path segments inside -drive / similar options
	QStringList parts = opt.split( QLatin1Char( ',' ) );
	for( int i = 0; i < parts.size(); ++i )
	{
		const QString &part = parts.at( i );
		const int eq = part.indexOf( QLatin1Char( '=' ) );
		if( eq <= 0 )
			continue;
		const QString key = part.left( eq ).trimmed().toLower();
		const bool is_path_key =
			key == QLatin1String( "file" ) ||
			key == QLatin1String( "path" ) ||
			key == QLatin1String( "filename" ) ||
			key == QLatin1String( "file.filename" ) ||
			key.endsWith( QLatin1String( ".filename" ) ) ||
			key.endsWith( QLatin1String( ".path" ) );
		if( ! is_path_key )
			continue;
		QString val = AQ_Normalize_File_Path( part.mid( eq + 1 ) );
		parts[i] = key + QLatin1Char( '=' ) + Windows_Path_To_WSL( val );
	}
	return parts.join( QLatin1Char( ',' ) );
}

QStringList Rewrite_Args_For_WSL( const QStringList &win_args )
{
	QStringList out;
	out.reserve( win_args.size() );

	for( int i = 0; i < win_args.size(); ++i )
	{
		const QString &a = win_args.at( i );

		// Flag that takes a path as the next argument
		if( ( a == QLatin1String( "-hda" ) || a == QLatin1String( "-hdb" ) ||
		      a == QLatin1String( "-hdc" ) || a == QLatin1String( "-hdd" ) ||
		      a == QLatin1String( "-cdrom" ) || a == QLatin1String( "-fda" ) ||
		      a == QLatin1String( "-fdb" ) || a == QLatin1String( "-kernel" ) ||
		      a == QLatin1String( "-initrd" ) || a == QLatin1String( "-bios" ) ||
		      a == QLatin1String( "-pflash" ) || a == QLatin1String( "-dtb" ) ||
		      a == QLatin1String( "-append" ) ) &&
		    i + 1 < win_args.size() )
		{
			out << a;
			++i;
			QString path = AQ_Normalize_File_Path( win_args.at( i ) );
			// Leave -append alone (kernel cmdline), only convert path-like flags
			if( a == QLatin1String( "-append" ) )
				out << win_args.at( i );
			else
				out << Windows_Path_To_WSL( path );
			continue;
		}

		if( a == QLatin1String( "-drive" ) || a == QLatin1String( "-blockdev" ) ||
		    a == QLatin1String( "-fsdev" ) || a == QLatin1String( "-chardev" ) )
		{
			out << a;
			if( i + 1 < win_args.size() )
			{
				++i;
				out << Rewrite_Drive_File_Option( win_args.at( i ) );
			}
			continue;
		}

		// Inline file= on same token (rare)
		if( a.contains( QLatin1String( "file=" ) ) &&
		    ( a.contains( QLatin1Char( ':' ) ) || a.contains( QLatin1Char( '\\' ) ) ) )
		{
			out << Rewrite_Drive_File_Option( a );
			continue;
		}

		out << a;
	}

	return out;
}

QStringList Build_WSL_Launch_Args( const QString &distro,
                                   const QString &linux_qemu_binary,
                                   const QStringList &qemu_args )
{
	QStringList args;
	if( ! distro.trimmed().isEmpty() )
		args << QStringLiteral( "-d" ) << distro.trimmed();
	// -e/--exec: run without the default Linux shell. Using `wsl -- cmd …`
	// goes through bash -c, which breaks OSK strings containing '(c)' and
	// paths with spaces.
	args << QStringLiteral( "-e" );
	args << ( linux_qemu_binary.isEmpty()
	              ? QStringLiteral( "qemu-system-x86_64" )
	              : linux_qemu_binary );

	QStringList rewritten = Rewrite_Args_For_WSL( qemu_args );
	for( int i = 0; i < rewritten.size(); ++i )
	{
		QString &a = rewritten[i];
		// Drop script-mode wrapping quotes — they are not valid in argv form
		if( a.size() >= 2 &&
		    ( ( a.startsWith( QLatin1Char( '"' ) ) && a.endsWith( QLatin1Char( '"' ) ) ) ||
		      ( a.startsWith( QLatin1Char( '\'' ) ) && a.endsWith( QLatin1Char( '\'' ) ) ) ) )
			a = a.mid( 1, a.size() - 2 );
	}
	args << rewritten;
	return args;
}

QString WSL_Pick_Audio_Backend( const QString &distro,
                                const QString &linux_qemu_binary,
                                const QString &preferred )
{
	static QMutex mutex;
	static QString cached_key;
	static QStringList cached_drivers;
	static qint64 cached_ms = 0;

	const QString qemu = linux_qemu_binary.trimmed().isEmpty()
		? QStringLiteral( "qemu-system-x86_64" )
		: linux_qemu_binary.trimmed();
	const QString key = distro.trimmed() + QLatin1Char( '|' ) + qemu;

	QStringList drivers;
	{
		QMutexLocker lock( &mutex );
		if( cached_key == key && Cache_Fresh( cached_ms, ! cached_drivers.isEmpty() ) )
			drivers = cached_drivers;
	}

	if( drivers.isEmpty() )
	{
		QStringList probe = Distro_Args( distro );
		probe << QStringLiteral( "--" ) << qemu
		      << QStringLiteral( "-audiodev" ) << QStringLiteral( "help" );
		QString out;
		// help often exits non-zero; still capture stdout/stderr text
		QProcess p;
		p.start( QStringLiteral( "wsl.exe" ), probe );
		if( ! p.waitForFinished( kWslTimeoutMs ) )
		{
			p.kill();
			p.waitForFinished( 1000 );
		}
		else
		{
			out = Decode_WSL_Output( p.readAllStandardOutput() );
			if( out.isEmpty() )
				out = Decode_WSL_Output( p.readAllStandardError() );
		}

		const QStringList lines = out.split( QRegularExpression( QStringLiteral( "[\\r\\n]+" ) ),
		                                     QString::SkipEmptyParts );
		for( const QString &line : lines )
		{
			const QString t = line.trimmed().toLower();
			if( t.isEmpty() || t.contains( QLatin1Char( ' ' ) ) || t.contains( QLatin1Char( '\t' ) ) )
				continue;
			if( t.contains( QLatin1Char( ':' ) ) || t.startsWith( QLatin1String( "available" ) ) )
				continue;
			if( t.length() < 2 || t.length() > 16 )
				continue;
			if( ! drivers.contains( t ) )
				drivers << t;
		}

		// Known-good defaults if probe failed
		if( drivers.isEmpty() )
			drivers << QStringLiteral( "alsa" ) << QStringLiteral( "none" );

		QMutexLocker lock( &mutex );
		cached_key = key;
		cached_drivers = drivers;
		cached_ms = QDateTime::currentMSecsSinceEpoch();
	}

	auto supported = [ &drivers ]( const QString &name ) -> bool
	{
		return drivers.contains( name, Qt::CaseInsensitive );
	};

	const QString pref = preferred.trimmed().toLower();
	if( ! pref.isEmpty() && supported( pref ) )
		return pref;

	static const char *kOrder[] = { "alsa", "spice", "oss", "sdl", "pipewire", "pa", "wav", "none" };
	for( const char *cand : kOrder )
	{
		if( supported( QString::fromLatin1( cand ) ) )
			return QString::fromLatin1( cand );
	}
	return QStringLiteral( "none" );
}

#else // ! Q_OS_WIN32

void WSL_Clear_Probe_Cache() {}
bool WSL_Is_Available( bool ) { return false; }
bool WSL_Has_KVM( const QString &, bool ) { return false; }
bool WSL_Ensure_KVM_Access( const QString & ) { return false; }
QString Windows_Path_To_WSL( const QString &windows_path ) { return windows_path; }
QStringList Rewrite_Args_For_WSL( const QStringList &win_args ) { return win_args; }
QStringList Build_WSL_Launch_Args( const QString &, const QString &, const QStringList &qemu_args )
{
	return qemu_args;
}
QString WSL_Pick_Audio_Backend( const QString &, const QString &, const QString & )
{
	return QStringLiteral( "pa" );
}

#endif
