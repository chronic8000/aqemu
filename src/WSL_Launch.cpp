/****************************************************************************
** WSL/KVM launch helpers (Windows host → Linux QEMU inside WSL)
****************************************************************************/

#include "WSL_Launch.h"
#include "Utils.h"
#include "System_Info.h"
#include "WSL_Secure_Credentials.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QTextCodec>
#include <QSettings>

#ifdef Q_OS_WIN32

namespace {

QMutex g_wsl_cache_mutex;
bool g_wsl_avail_valid = false;
bool g_wsl_avail = false;
qint64 g_wsl_avail_ms = 0;

bool g_wsl_kvm_valid = false;
bool g_wsl_kvm = false;
QString g_wsl_kvm_distro;
QString g_wsl_kvm_user;
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

/** Username safe for shell interpolation (letters, digits, _ and - only). */
QString Sanitize_WSL_Username( const QString &raw )
{
	const QString user = raw.trimmed();
	if( user.isEmpty() )
		return QString();
	for( const QChar &ch : user )
	{
		if( ! ( ch.isLetterOrNumber() || ch == QLatin1Char( '_' ) || ch == QLatin1Char( '-' ) ) )
			return QString();
	}
	return user;
}

QString Configured_WSL_Username()
{
	QSettings s;
	return Sanitize_WSL_Username(
		s.value( QStringLiteral( "WSL_Launch/Username" ), QString() ).toString() );
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

/** Clear cached WSL/KVM probe results (e.g. after Settings Probe). */
void WSL_Clear_Probe_Cache()
{
	QMutexLocker lock( &g_wsl_cache_mutex );
	g_wsl_avail_valid = false;
	g_wsl_kvm_valid = false;
	g_wsl_avail_ms = 0;
	g_wsl_kvm_ms = 0;
	g_wsl_kvm_user.clear();
}

QString WSL_Sanitize_Username( const QString &raw )
{
	return Sanitize_WSL_Username( raw );
}

bool WSL_Is_Valid_Username( const QString &raw )
{
	return ! Sanitize_WSL_Username( raw ).isEmpty();
}

QStringList WSL_Get_Installed_Distros()
{
	QStringList distros;
	QString out;
	// wsl.exe -l -q lists installed distros line-by-line
	if( Run_WSL( QStringList() << QStringLiteral( "-l" ) << QStringLiteral( "-q" ), 5000, &out ) )
	{
		const QStringList lines = out.split( QRegularExpression( QStringLiteral( "[\\r\\n]+" ) ), QString::SkipEmptyParts );
		for( const QString &line : lines )
		{
			const QString t = line.trimmed();
			if( ! t.isEmpty() && ! distros.contains( t ) )
				distros << t;
		}
	}
	return distros;
}

QString WSL_Get_Distro_Default_User( const QString &distro )
{
	QString user;
	QStringList who = Distro_Args( distro );
	who << QStringLiteral( "--" ) << QStringLiteral( "whoami" );
	if( Run_WSL( who, 5000, &user ) && ! user.trimmed().isEmpty() )
	{
		user = user.trimmed();
		// Defensive: only return safe username characters
		for( const QChar &ch : user )
		{
			if( ! ( ch.isLetterOrNumber() || ch == QLatin1Char( '_' ) || ch == QLatin1Char( '-' ) ) )
			{
				return QStringLiteral( "root" );
			}
		}
		return user;
	}
	return QStringLiteral( "root" );
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
	const QString wslUser = Configured_WSL_Username();
	{
		QMutexLocker lock( &g_wsl_cache_mutex );
		if( ! force_refresh && g_wsl_kvm_valid && g_wsl_kvm_distro == d &&
		    g_wsl_kvm_user == wslUser &&
		    Cache_Fresh( g_wsl_kvm_ms, g_wsl_kvm ) )
			return g_wsl_kvm;
	}

	// Probe as the configured launch user so results match Build_WSL_Launch_Args.
	QStringList args = Distro_Args( d );
	if( ! wslUser.isEmpty() )
		args << QStringLiteral( "-u" ) << wslUser;
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
	g_wsl_kvm_user = wslUser;
	g_wsl_kvm_ms = QDateTime::currentMSecsSinceEpoch();
	return ok;
}

bool WSL_Ensure_KVM_Access( const QString &distro )
{
	const QString d = distro.trimmed();

	if( ! WSL_Is_Available( false ) )
		return false;

	// Never keep plaintext passwords in QSettings.
	{
		QSettings s;
		s.remove( QStringLiteral( "WSL_Launch/Password" ) );
	}

	const QString wslUser = Configured_WSL_Username();

	auto try_script = [&]( const QString &script ) -> bool
	{
		return WSL_Run_Privileged_Script( d, script );
	};

	// macOS guests (and many OSX-KVM recipes) need ignore_msrs=1 or the
	// kernel hangs after OpenCore with "still waiting for root device".
	try_script( QStringLiteral(
		"if [ -w /sys/module/kvm/parameters/ignore_msrs ]; then "
		"echo 1 > /sys/module/kvm/parameters/ignore_msrs; fi; "
		"mkdir -p /etc/modprobe.d 2>/dev/null || true; "
		"printf 'options kvm ignore_msrs=Y report_ignored_msrs=0\\n' "
		"> /etc/modprobe.d/aqemu-kvm-macos.conf 2>/dev/null || true; "
		"exit 0" ) );

	if( WSL_Has_KVM( d, true ) )
		return true;

	QString user = wslUser;
	if( user.isEmpty() )
	{
		QStringList who = Distro_Args( d );
		who << QStringLiteral( "--" ) << QStringLiteral( "whoami" );
		QString whoami;
		if( Run_WSL( who, kWslTimeoutMs, &whoami ) )
			user = Sanitize_WSL_Username( whoami );
		if( user.isEmpty() )
			user = QStringLiteral( "root" );
	}

	QString script = QStringLiteral(
		"getent group kvm >/dev/null 2>&1 || groupadd -r kvm 2>/dev/null || true; "
		"if [ -c /dev/kvm ]; then chgrp kvm /dev/kvm 2>/dev/null || true; chmod 660 /dev/kvm 2>/dev/null || true; fi; " );
	if( ! user.isEmpty() && user != QLatin1String( "root" ) )
	{
		script += QStringLiteral( "usermod -aG kvm '%1' 2>/dev/null || true; " ).arg( user );
		script += QStringLiteral(
			"mkdir -p /etc/udev/rules.d 2>/dev/null || true; "
			"printf 'KERNEL==\"kvm\", GROUP=\"kvm\", MODE=\"0660\"\\n' "
			"> /etc/udev/rules.d/99-aqemu-kvm.rules 2>/dev/null || true; " );
	}
	script += QStringLiteral( "exit 0" );

	try_script( script );

	WSL_Clear_Probe_Cache();
	return WSL_Has_KVM( d, true );
}

bool WSL_Run_Privileged_Script( const QString &distro, const QString &script, int timeout_ms )
{
	const QString d = distro.trimmed();
	const int wait_ms = timeout_ms > 0 ? timeout_ms : kWslTimeoutMs;

	// Preferred: wsl -u root (no password).
	{
		QStringList args = Distro_Args( d );
		args << QStringLiteral( "-u" ) << QStringLiteral( "root" )
		     << QStringLiteral( "-e" ) << QStringLiteral( "sh" ) << QStringLiteral( "-c" )
		     << script;
		if( Run_WSL( args, wait_ms ) )
			return true;
	}

	// Optional fallback: stored sudo password via Credential Manager (stdin only).
	if( ! WSL_Secure_Password_Available() )
		return false;
	QSettings s;
	if( ! s.value( QStringLiteral( "WSL_Launch/Remember_Password" ), false ).toBool() )
		return false;

	const QString user = Configured_WSL_Username();
	if( user.isEmpty() || user == QLatin1String( "root" ) )
		return false;

	QString password = WSL_Load_Secure_Password();
	if( password.isEmpty() )
		return false;

	QStringList args = Distro_Args( d );
	args << QStringLiteral( "-u" ) << user
	     << QStringLiteral( "-e" ) << QStringLiteral( "sudo" )
	     << QStringLiteral( "-S" ) << QStringLiteral( "-p" ) << QString()
	     << QStringLiteral( "sh" ) << QStringLiteral( "-c" ) << script;

	QProcess p;
	p.start( QStringLiteral( "wsl.exe" ), args );
	if( ! p.waitForStarted( 5000 ) )
	{
		password.fill( QLatin1Char( '\0' ) );
		return false;
	}
	p.write( password.toUtf8() );
	p.write( "\n" );
	password.fill( QLatin1Char( '\0' ) );
	p.closeWriteChannel();
	if( ! p.waitForFinished( wait_ms ) )
	{
		p.kill();
		p.waitForFinished( 1000 );
		return false;
	}
	return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

bool WSL_Has_Dozen_ICD( const QString &distro )
{
	// Only accept a native-arch ICD. Multiarch WSL often has
	// /usr/lib/aarch64-linux-gnu/libvulkan_dzn.so while the host is amd64 —
	// treating that as success left Vulkan broken for everyone on multiarch.
	//
	// Must use wsl -e (exec), not wsl -- …: the default-shell form expands
	// $(…) in this script before sh -c sees it, leaving MA empty and always
	// reporting "no Dozen" even when the ICD is installed.
	QStringList args = Distro_Args( distro.trimmed() );
	args << QStringLiteral( "-e" ) << QStringLiteral( "sh" ) << QStringLiteral( "-c" )
	     << QStringLiteral(
		"MA=\"\"; "
		"if command -v dpkg-architecture >/dev/null 2>&1; then "
		"  MA=$(dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null); "
		"elif [ \"$(uname -m)\" = x86_64 ]; then MA=x86_64-linux-gnu; "
		"elif [ \"$(uname -m)\" = aarch64 ]; then MA=aarch64-linux-gnu; "
		"fi; "
		"[ -n \"$MA\" ] && [ -e \"/usr/lib/$MA/libvulkan_dzn.so\" ] && exit 0; "
		"[ -e /usr/lib/x86_64-linux-gnu/libvulkan_dzn.so ] && exit 0; "
		"[ -e /usr/lib/aarch64-linux-gnu/libvulkan_dzn.so ] && [ \"$(uname -m)\" = aarch64 ] && exit 0; "
		"[ -e /usr/lib64/libvulkan_dzn.so ] && exit 0; "
		"exit 1" );
	return Run_WSL( args, 10000 );
}

static QString Reims_Dzn_Install_Script()
{
	// Install Mesa Dozen (Vulkan-on-D3D12) for the *native* WSL architecture only.
	// Avoid foreign-arch mesa-vulkan-drivers (common amd64+arm64 WSL) which:
	//  - share /usr/bin/spirv2dxil and break dpkg upgrades
	//  - make naive "find libvulkan_dzn.so" report success for the wrong arch
	return QStringLiteral(
		"set +e; "
		"export DEBIAN_FRONTEND=noninteractive; "
		"native_ma() { "
		"  if command -v dpkg-architecture >/dev/null 2>&1; then "
		"    dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null; "
		"  elif [ \"$(uname -m)\" = x86_64 ]; then echo x86_64-linux-gnu; "
		"  elif [ \"$(uname -m)\" = aarch64 ]; then echo aarch64-linux-gnu; "
		"  fi; "
		"}; "
		"have_dzn() { "
		"  MA=$(native_ma); "
		"  [ -n \"$MA\" ] && [ -e \"/usr/lib/$MA/libvulkan_dzn.so\" ] && return 0; "
		"  [ -e /usr/lib64/libvulkan_dzn.so ] && return 0; "
		"  return 1; "
		"}; "
		"if have_dzn; then echo AQEMU_DZN_OK; exit 0; fi; "
		"if command -v apt-get >/dev/null 2>&1; then "
		"  ARCH=$(dpkg --print-architecture 2>/dev/null || echo amd64); "
		"  for fa in $(dpkg --print-foreign-architectures 2>/dev/null); do "
		"    if dpkg -l \"mesa-vulkan-drivers:$fa\" 2>/dev/null | grep -qE '^i[iUF]'; then "
		"      apt-get remove -y \"mesa-vulkan-drivers:$fa\" >/dev/null 2>&1 || true; "
		"    fi; "
		"  done; "
		"  dpkg --configure -a >/dev/null 2>&1 || true; "
		"  apt-get -f install -y -o Dpkg::Options::=--force-overwrite >/dev/null 2>&1 || true; "
		"  apt-get update -qq; "
		"  apt-get install -y software-properties-common ca-certificates >/dev/null 2>&1 || true; "
		"  apt-get install -y -o Dpkg::Options::=--force-overwrite "
		"    \"mesa-vulkan-drivers:${ARCH}\" vulkan-tools >/dev/null 2>&1 || true; "
		"  if have_dzn; then echo AQEMU_DZN_OK; exit 0; fi; "
		"  if command -v add-apt-repository >/dev/null 2>&1; then "
		"    add-apt-repository -y ppa:kisak/kisak-mesa >/dev/null 2>&1 || true; "
		"    apt-get update -qq; "
		"    apt-get install -y -o Dpkg::Options::=--force-overwrite "
		"      \"mesa-vulkan-drivers:${ARCH}\" vulkan-tools >/dev/null 2>&1 || "
		"      apt-get install -y -o Dpkg::Options::=--force-overwrite "
		"        mesa-vulkan-drivers vulkan-tools >/dev/null 2>&1 || true; "
		"  fi; "
		"  dpkg --configure -a >/dev/null 2>&1 || true; "
		"elif command -v pacman >/dev/null 2>&1; then "
		"  pacman -Sy --noconfirm mesa vulkan-icd-loader vulkan-dzn 2>/dev/null || "
		"    pacman -Sy --noconfirm mesa vulkan-icd-loader 2>/dev/null || true; "
		"elif command -v dnf >/dev/null 2>&1; then "
		"  dnf install -y mesa-vulkan-drivers vulkan-tools 2>/dev/null || true; "
		"fi; "
		"if have_dzn; then echo AQEMU_DZN_OK; exit 0; fi; "
		"echo AQEMU_DZN_MISSING; exit 1" );
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

static QStringList Split_Qemu_Option_Commas( const QString &opt )
{
	// QEMU uses ,, for a literal comma in option values (tobimensch/PR#1 / Qodo)
	QStringList parts;
	QString cur;
	for( int i = 0; i < opt.size(); ++i )
	{
		if( opt.at( i ) == QLatin1Char( ',' ) )
		{
			if( i + 1 < opt.size() && opt.at( i + 1 ) == QLatin1Char( ',' ) )
			{
				cur += QLatin1Char( ',' );
				++i;
			}
			else
			{
				parts << cur;
				cur.clear();
			}
		}
		else
		{
			cur += opt.at( i );
		}
	}
	parts << cur;
	return parts;
}

static QString Escape_Qemu_Option_Commas( QString val )
{
	return val.replace( QLatin1Char( ',' ), QLatin1String( ",," ) );
}

static QString Rewrite_Drive_File_Option( const QString &opt )
{
	// Rewrite file= / file.filename= / Inferno machine path props inside -drive / -machine
	QStringList parts = Split_Qemu_Option_Commas( opt );
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
			key == QLatin1String( "romfile" ) ||
			key.endsWith( QLatin1String( ".filename" ) ) ||
			key.endsWith( QLatin1String( ".path" ) ) ||
			key == QLatin1String( "trustcache" ) ||
			key == QLatin1String( "ticket" ) ||
			key == QLatin1String( "sep-fw" ) ||
			key == QLatin1String( "sep-rom" ) ||
			key == QLatin1String( "securerom" ) ||
			key == QLatin1String( "usb-conn-addr" );
		if( ! is_path_key )
			continue;
		QString val = part.mid( eq + 1 );
		// Keep pure Linux unix socket paths as-is; convert Windows host paths.
		if( key == QLatin1String( "usb-conn-addr" ) &&
		    ( val.startsWith( QLatin1Char( '/' ) ) || val.contains( QLatin1Char( '.' ) ) ) &&
		    ! val.contains( QLatin1Char( ':' ) ) && ! val.contains( QLatin1Char( '\\' ) ) )
		{
			continue;
		}
		val = AQ_Normalize_File_Path( val );
		parts[i] = key + QLatin1Char( '=' ) + Escape_Qemu_Option_Commas( Windows_Path_To_WSL( val ) );
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
		      a == QLatin1String( "-L" ) ||
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
		    a == QLatin1String( "-fsdev" ) || a == QLatin1String( "-chardev" ) ||
		    a == QLatin1String( "-machine" ) || a == QLatin1String( "-device" ) )
		{
			out << a;
			if( i + 1 < win_args.size() )
			{
				++i;
				out << Rewrite_Drive_File_Option( win_args.at( i ) );
			}
			continue;
		}

		// Inline file=/romfile= on same token (rare)
		if( ( a.contains( QLatin1String( "file=" ) ) || a.contains( QLatin1String( "romfile=" ) ) ) &&
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
	// Run QEMU as the configured WSL user so KVM group membership matches
	// WSL_Ensure_KVM_Access / WSL_Has_KVM probes.
	const QString wslUser = Configured_WSL_Username();
	if( ! wslUser.isEmpty() )
		args << QStringLiteral( "-u" ) << wslUser;
	// -e/--exec: run without the default Linux shell. Using `wsl -- cmd …`
	// goes through bash -c, which breaks OSK strings containing '(c)' and
	// paths with spaces.
	args << QStringLiteral( "-e" );

	const QString qemu_bin = linux_qemu_binary.isEmpty()
		? QStringLiteral( "qemu-system-x86_64" )
		: linux_qemu_binary;
	const bool is_reims3d = qemu_bin.contains( QLatin1String( "reims3d" ), Qt::CaseInsensitive )
		|| qemu_bin.contains( QLatin1String( "reims" ), Qt::CaseInsensitive );

	// Reims needs Vulkan backend + Windows-mounted GPU libs on PATH (any vendor ICD).
	if( is_reims3d )
	{
		args << QStringLiteral( "env" );
		const QStringList env = WSL_Vulkan_Launch_Env( distro );
		for( int i = 0; i < env.size(); ++i )
			args << env.at( i );
	}

	args << qemu_bin;

	const bool runs_windows_exe = linux_qemu_binary.endsWith( QLatin1String( ".exe" ), Qt::CaseInsensitive );
	QStringList rewritten = runs_windows_exe ? qemu_args : Rewrite_Args_For_WSL( qemu_args );
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

QString WSL_Probe_Accelerated_Vulkan_GPU( const QString &distro )
{
	const QStringList all = WSL_List_Accelerated_Vulkan_GPUs( distro );
	return all.isEmpty() ? QString() : all.first();
}

namespace {

QString &Preferred_Vulkan_Device_Storage()
{
	static QString pref;
	return pref;
}

struct WSL_Vk_Dev
{
	QString name;
	QString vendor_id;
	QString device_id;
};

QList<WSL_Vk_Dev> Parse_Vulkaninfo_Devices( const QString &out )
{
	QList<WSL_Vk_Dev> devices;
	WSL_Vk_Dev cur;
	bool in_gpu = false;

	const QStringList lines = out.split( QRegularExpression( QStringLiteral( "[\\r\\n]+" ) ) );
	auto flush = [&]() {
		if( cur.name.isEmpty() )
			return;
		const QString lower = cur.name.toLower();
		if( lower.contains( QLatin1String( "llvmpipe" ) ) ||
		    lower.contains( QLatin1String( "lavapipe" ) ) ||
		    lower.contains( QLatin1String( "softpipe" ) ) ||
		    lower.contains( QLatin1String( "swiftshader" ) ) )
		{
			cur = WSL_Vk_Dev();
			return;
		}
		devices << cur;
		cur = WSL_Vk_Dev();
	};

	for( int i = 0; i < lines.size(); ++i )
	{
		const QString raw = lines.at( i );
		const QString line = raw.trimmed();
		if( line.startsWith( QLatin1String( "GPU" ) ) && line.contains( QLatin1Char( ':' ) ) )
		{
			flush();
			in_gpu = true;
			continue;
		}
		if( ! in_gpu )
			continue;
		if( line.startsWith( QLatin1String( "deviceName" ) ) )
		{
			const int colon = line.indexOf( QLatin1Char( ':' ) );
			if( colon >= 0 )
				cur.name = line.mid( colon + 1 ).trimmed();
		}
		else if( line.startsWith( QLatin1String( "vendorID" ) ) )
		{
			const int eq = line.indexOf( QLatin1Char( '=' ) );
			QString v = ( eq >= 0 ) ? line.mid( eq + 1 ).trimmed() : QString();
			if( v.startsWith( QLatin1String( "0x" ), Qt::CaseInsensitive ) )
				v = v.mid( 2 );
			cur.vendor_id = v.toLower();
		}
		else if( line.startsWith( QLatin1String( "deviceID" ) ) )
		{
			const int eq = line.indexOf( QLatin1Char( '=' ) );
			QString v = ( eq >= 0 ) ? line.mid( eq + 1 ).trimmed() : QString();
			if( v.startsWith( QLatin1String( "0x" ), Qt::CaseInsensitive ) )
				v = v.mid( 2 );
			cur.device_id = v.toLower();
		}
	}
	flush();
	return devices;
}

QList<WSL_Vk_Dev> Cached_Vulkan_Devices( const QString &distro, bool force )
{
	static QMutex mutex;
	static QList<WSL_Vk_Dev> cached;
	static qint64 cached_ms = 0;
	static QString cached_distro;

	const QString d = distro.trimmed();
	{
		QMutexLocker lock( &mutex );
		if( ! force && cached_distro == d && Cache_Fresh( cached_ms, true ) )
			return cached;
	}

	QString adapter = QStringLiteral( "NVIDIA" );
	if( System_Info::Has_AMD_Display_GPU() && ! System_Info::Has_NVIDIA_Display_GPU() )
		adapter = QStringLiteral( "AMD" );
	else if( System_Info::Has_Intel_Display_GPU() &&
	         ! System_Info::Has_NVIDIA_Display_GPU() &&
	         ! System_Info::Has_AMD_Display_GPU() )
		adapter = QStringLiteral( "Intel" );

	QStringList args = Distro_Args( d );
	// -e: do not let the login shell expand $(…) / $MA in this probe script.
	args << QStringLiteral( "-e" ) << QStringLiteral( "sh" ) << QStringLiteral( "-c" )
	     << QStringLiteral(
		"export LD_LIBRARY_PATH=/usr/lib/wsl/lib:${LD_LIBRARY_PATH:-}; "
		"export GALLIUM_DRIVER=d3d12; "
		"export MESA_D3D12_DEFAULT_ADAPTER_NAME='%1'; "
		"MA=$(dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null || true); "
		"if [ -z \"$MA\" ] && [ \"$(uname -m)\" = x86_64 ]; then MA=x86_64-linux-gnu; fi; "
		"if [ -n \"$MA\" ] && [ -e \"/usr/lib/$MA/libvulkan_dzn.so\" ] && "
		"   [ -e /usr/share/vulkan/icd.d/dzn_icd.json ]; then "
		"  export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/dzn_icd.json; "
		"fi; "
		"vulkaninfo --summary 2>/dev/null || true" ).arg( adapter );

	QString out;
	Run_WSL( args, 15000, &out );
	const QList<WSL_Vk_Dev> devices = Parse_Vulkaninfo_Devices( out );

	{
		QMutexLocker lock( &mutex );
		cached = devices;
		cached_distro = d;
		cached_ms = QDateTime::currentMSecsSinceEpoch();
	}
	return devices;
}

} // namespace

QStringList WSL_List_Accelerated_Vulkan_GPUs( const QString &distro )
{
	QStringList names;
	const QList<WSL_Vk_Dev> devices = Cached_Vulkan_Devices( distro, false );
	for( int i = 0; i < devices.size(); ++i )
		names << devices.at( i ).name;
	return names;
}

bool WSL_Ensure_Reims_Vulkan_Stack( const QString &distro, QString *status_message,
                                    bool force_reinstall )
{
	const QString d = distro.trimmed();
	QSettings s;

	auto set_msg = [&]( const QString &m ) {
		if( status_message )
			*status_message = m;
	};

	auto mark_ready = [&]( const QString &msg ) {
		set_msg( msg );
		s.setValue( QStringLiteral( "WSL_Launch/Reims_Vulkan_Ready" ), true );
		s.setValue( QStringLiteral( "WSL_Launch/Reims_Dzn_Install_Count" ), 0 );
		s.remove( QStringLiteral( "WSL_Launch/Reims_Dzn_Install_Attempted" ) );
	};

	// Native Dozen ICD is enough for Reims. Do not require vulkaninfo — Dozen often
	// segfaults during probe while libvulkan_dzn.so is still usable at launch.
	// Check this *before* install-count short-circuit so a manual fix or prior
	// install is not stuck behind Reims_Dzn_Install_Count >= 2.
	if( ! force_reinstall && WSL_Has_Dozen_ICD( d ) )
	{
		const QList<WSL_Vk_Dev> devices = Cached_Vulkan_Devices( d, false );
		if( ! devices.isEmpty() )
			mark_ready( QStringLiteral( "WSL Vulkan OK: %1" ).arg( devices.first().name ) );
		else
			mark_ready( QStringLiteral(
				"Native Mesa Dozen (dzn) is installed. vulkaninfo could not list devices "
				"(Dozen is experimental and sometimes crashes during probe). "
				"AQEMU will launch Reims with VK_ICD_FILENAMES=dzn_icd.json." ) );
		return true;
	}

	// Already have accelerated Vulkan (RADV, ANV, NVIDIA ICD, …) without Dozen.
	{
		const QList<WSL_Vk_Dev> devices = Cached_Vulkan_Devices( d, force_reinstall );
		if( ! devices.isEmpty() )
		{
			mark_ready( QStringLiteral( "WSL Vulkan OK: %1" ).arg( devices.first().name ) );
			return true;
		}
	}

	if( force_reinstall )
	{
		s.remove( QStringLiteral( "WSL_Launch/Reims_Dzn_Install_Attempted" ) );
		s.setValue( QStringLiteral( "WSL_Launch/Reims_Dzn_Install_Count" ), 0 );
	}

	const int install_count = s.value( QStringLiteral( "WSL_Launch/Reims_Dzn_Install_Count" ), 0 ).toInt();
	// Cap automatic apt attempts so a broken distro does not reinstall forever,
	// but allow a couple of retries (false multiarch "success" used to burn the only try).
	if( install_count >= 2 && ! force_reinstall )
	{
		set_msg( QStringLiteral(
			"WSL still has no native-arch Mesa Dozen ICD after automatic setup. "
			"In WSL run: sudo add-apt-repository -y ppa:kisak/kisak-mesa && "
			"sudo apt-get install -y -o Dpkg::Options::=--force-overwrite "
			"mesa-vulkan-drivers:$(dpkg --print-architecture)" ) );
		return false;
	}

	set_msg( QStringLiteral( "Installing native-arch Mesa Dozen (dzn) Vulkan-on-D3D12 ICD inside WSL…" ) );
	s.setValue( QStringLiteral( "WSL_Launch/Reims_Dzn_Install_Attempted" ), true );
	s.setValue( QStringLiteral( "WSL_Launch/Reims_Dzn_Install_Count" ), install_count + 1 );

	WSL_Run_Privileged_Script( d, Reims_Dzn_Install_Script(), 15 * 60 * 1000 );
	WSL_Clear_Probe_Cache();

	const bool has_dzn = WSL_Has_Dozen_ICD( d );
	const QList<WSL_Vk_Dev> devices = Cached_Vulkan_Devices( d, true );
	if( ! devices.isEmpty() )
	{
		mark_ready( QStringLiteral( "WSL Vulkan ready for Reims: %1" ).arg( devices.first().name ) );
		return true;
	}

	// Dozen is present but vulkaninfo failed/segfaulted (known experimental). Treat
	// native ICD install as provision success so Reims can still launch.
	if( has_dzn )
	{
		mark_ready( QStringLiteral(
			"Native Mesa Dozen (dzn) is installed. vulkaninfo could not list devices "
			"(Dozen is experimental and sometimes crashes during probe). "
			"AQEMU will launch Reims with VK_ICD_FILENAMES=dzn_icd.json." ) );
		return true;
	}

	set_msg( QStringLiteral(
		"Could not install native-arch Mesa Dozen (libvulkan_dzn). "
		"Multiarch foreign mesa-vulkan-drivers packages are removed automatically; "
		"then install: sudo add-apt-repository -y ppa:kisak/kisak-mesa && "
		"sudo apt-get install -y -o Dpkg::Options::=--force-overwrite "
		"mesa-vulkan-drivers:$(dpkg --print-architecture)" ) );
	s.setValue( QStringLiteral( "WSL_Launch/Reims_Vulkan_Ready" ), false );
	return false;
}

void WSL_Set_Preferred_Vulkan_Device( const QString &device_name_or_empty )
{
	Preferred_Vulkan_Device_Storage() = device_name_or_empty.trimmed();
}

QString WSL_Preferred_Vulkan_Device()
{
	QSettings s;
	const QString from_settings = s.value( QStringLiteral( "WSL_Launch/Vulkan_Device" ), QString() )
		.toString().trimmed();
	if( ! Preferred_Vulkan_Device_Storage().isEmpty() )
		return Preferred_Vulkan_Device_Storage();
	return from_settings;
}

QStringList WSL_Vulkan_Launch_Env( const QString &distro )
{
	QStringList env;
	env << QStringLiteral( "REIMS_VGPU_BACKEND=vulkan" )
	    << QStringLiteral( "LD_LIBRARY_PATH=/usr/lib/wsl/lib:/usr/lib/x86_64-linux-gnu" )
	    // Prefer WSLg D3D12 path when Mesa Dozen (dzn) is installed.
	    << QStringLiteral( "GALLIUM_DRIVER=d3d12" );

	// Host window (winit + VkSurfaceKHR): opt-in. Mesa Dozen on WSLg NVIDIA
	// currently segfaults inside vkCreateDevice during window init, which kills
	// the whole QEMU process (and AQEMU's VNC session). Default off so early
	// boot (GOP/VNC) stays up; Advanced Settings can force WINDOW=1.
	{
		QSettings s;
		const bool want_window =
			s.value( QStringLiteral( "WSL_Launch/Reims_Host_Window" ), false ).toBool();
		env << ( want_window
			? QStringLiteral( "REIMS_VGPU_WINDOW=1" )
			: QStringLiteral( "REIMS_VGPU_WINDOW=0" ) );
	}

	// WSLg session vars (needed if/when the host window is enabled).
	env << QStringLiteral( "DISPLAY=:0" )
	    << QStringLiteral( "WAYLAND_DISPLAY=wayland-0" );

	{
		static QMutex uid_mutex;
		static QString cached_uid;
		static QString cached_distro;
		QString uid;
		{
			QMutexLocker lock( &uid_mutex );
			if( cached_distro == distro.trimmed() && ! cached_uid.isEmpty() )
				uid = cached_uid;
		}
		if( uid.isEmpty() )
		{
			QStringList id_args = Distro_Args( distro.trimmed() );
			id_args << QStringLiteral( "-e" ) << QStringLiteral( "id" ) << QStringLiteral( "-u" );
			QString out;
			if( Run_WSL( id_args, 5000, &out ) )
			{
				uid = out.trimmed();
				bool ok = false;
				const int n = uid.toInt( &ok );
				if( ! ok || n <= 0 )
					uid.clear();
			}
			if( uid.isEmpty() )
				uid = QStringLiteral( "1000" );
			QMutexLocker lock( &uid_mutex );
			cached_uid = uid;
			cached_distro = distro.trimmed();
		}
		env << QStringLiteral( "XDG_RUNTIME_DIR=/run/user/%1" ).arg( uid );
	}

	if( System_Info::Has_NVIDIA_Display_GPU() )
		env << QStringLiteral( "MESA_D3D12_DEFAULT_ADAPTER_NAME=NVIDIA" );
	else if( System_Info::Has_AMD_Display_GPU() )
		env << QStringLiteral( "MESA_D3D12_DEFAULT_ADAPTER_NAME=AMD" );
	else if( System_Info::Has_Intel_Display_GPU() )
		env << QStringLiteral( "MESA_D3D12_DEFAULT_ADAPTER_NAME=Intel" );

	// Pin the Vulkan loader to native Dozen when present. Otherwise Mesa's
	// asahi/intel/freedreno ICDs spam /dev/dri errors and can hide D3D12.
	if( WSL_Has_Dozen_ICD( distro ) )
		env << QStringLiteral( "VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/dzn_icd.json" );

	const QString prefer = WSL_Preferred_Vulkan_Device();
	if( prefer.isEmpty() || prefer.compare( QLatin1String( "auto" ), Qt::CaseInsensitive ) == 0 )
		return env;

	const QList<WSL_Vk_Dev> devices = Cached_Vulkan_Devices( distro, false );
	for( int i = 0; i < devices.size(); ++i )
	{
		const WSL_Vk_Dev &d = devices.at( i );
		if( ! d.name.contains( prefer, Qt::CaseInsensitive ) &&
		    prefer.compare( d.name, Qt::CaseInsensitive ) != 0 )
			continue;
		if( ! d.vendor_id.isEmpty() && ! d.device_id.isEmpty() )
		{
			env << QStringLiteral( "MESA_VK_DEVICE_SELECT=%1:%2" )
			           .arg( d.vendor_id, d.device_id );
			env << QStringLiteral( "MESA_VK_DEVICE_SELECT_FORCE_DEFAULT_DEVICE=1" );
		}
		break;
	}
	return env;
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
bool WSL_Run_Privileged_Script( const QString &, const QString &, int ) { return false; }
QString WSL_Sanitize_Username( const QString & ) { return QString(); }
bool WSL_Is_Valid_Username( const QString & ) { return false; }
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
QString WSL_Probe_Accelerated_Vulkan_GPU( const QString & ) { return QString(); }
QStringList WSL_List_Accelerated_Vulkan_GPUs( const QString & ) { return QStringList(); }
void WSL_Set_Preferred_Vulkan_Device( const QString & ) {}
QString WSL_Preferred_Vulkan_Device() { return QString(); }
QStringList WSL_Vulkan_Launch_Env( const QString & ) { return QStringList(); }
bool WSL_Has_Dozen_ICD( const QString & ) { return false; }
bool WSL_Ensure_Reims_Vulkan_Stack( const QString &, QString *msg, bool )
{
	if( msg )
		*msg = QStringLiteral( "Reims Vulkan provisioning is only required on Windows/WSL." );
	return true;
}

#endif
