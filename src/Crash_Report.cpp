/****************************************************************************
** Local crash reports. The handler avoids the heap. Qt is used only on
** the next launch, when the process is healthy again.
****************************************************************************/
#include "Crash_Report.h"

#include "Utils.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QSysInfo>
#include <QUrl>
#include <QtGlobal>

#include <stdio.h>
#include <string.h>

#ifdef Q_OS_WIN
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <dbghelp.h>
#else
#  include <errno.h>
#  include <fcntl.h>
#  include <signal.h>
#  include <stdlib.h>
#  include <sys/stat.h>
#  include <time.h>
#  include <unistd.h>
#  ifdef __GLIBC__
#    include <execinfo.h>
#  endif
#endif

static const int k_keep = 10;

#ifdef Q_OS_WIN
static wchar_t g_dir[1024];
static wchar_t g_version[64];
static char g_qt[64];
static volatile LONG g_handling = 0;

static LONG WINAPI Crash_Filter( EXCEPTION_POINTERS *info )
{
	if( InterlockedCompareExchange( &g_handling, 1, 0 ) != 0 )
		return EXCEPTION_CONTINUE_SEARCH;
	if( g_dir[ 0 ] == 0 || ! info || ! info->ExceptionRecord )
		return EXCEPTION_CONTINUE_SEARCH;

	SYSTEMTIME st;
	GetSystemTime( &st );
	wchar_t stamp[ 32 ];
	wsprintfW( stamp, L"%04u%02u%02u-%02u%02u%02u",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond );

	wchar_t dump_path[ 1200 ];
	wchar_t text_path[ 1200 ];
	wsprintfW( dump_path, L"%s\\aqemu-%s-%s.dmp", g_dir, g_version, stamp );
	wsprintfW( text_path, L"%s\\aqemu-%s-%s.txt", g_dir, g_version, stamp );

	HANDLE file = CreateFileW( dump_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( file != INVALID_HANDLE_VALUE )
	{
		MINIDUMP_EXCEPTION_INFORMATION ex;
		ex.ThreadId = GetCurrentThreadId();
		ex.ExceptionPointers = info;
		ex.ClientPointers = FALSE;
		const MINIDUMP_TYPE kind = (MINIDUMP_TYPE)(
			MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithDataSegs );
		MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), file, kind, &ex, NULL, NULL );
		CloseHandle( file );
	}

	char text[ 1024 ];
	const DWORD code = info->ExceptionRecord->ExceptionCode;
	const void *addr = info->ExceptionRecord->ExceptionAddress;
#if defined( _M_ARM64 ) || defined( __aarch64__ )
	const char *arch = "ARM64";
#elif defined( _M_AMD64 ) || defined( __x86_64__ )
	const char *arch = "x64";
#else
	const char *arch = "x86";
#endif
	OSVERSIONINFOEXW ver;
	ZeroMemory( &ver, sizeof( ver ) );
	ver.dwOSVersionInfoSize = sizeof( ver );
	typedef LONG (WINAPI *RtlGetVersionFn)( OSVERSIONINFOEXW * );
	HMODULE ntdll = GetModuleHandleW( L"ntdll.dll" );
	RtlGetVersionFn rtl = ntdll ? (RtlGetVersionFn)GetProcAddress( ntdll, "RtlGetVersion" ) : NULL;
	DWORD major = 0, minor = 0, build = 0;
	if( rtl && rtl( &ver ) == 0 )
	{
		major = ver.dwMajorVersion;
		minor = ver.dwMinorVersion;
		build = ver.dwBuildNumber;
	}
	int n = _snprintf( text, sizeof( text ),
		"AQEMU %s\r\nArch %s\r\nWindows %lu.%lu.%lu\r\nQt %s\r\nException 0x%08lX at %p\r\n",
		CURRENT_AQEMU_VERSION, arch,
		(unsigned long)major, (unsigned long)minor, (unsigned long)build,
		g_qt, (unsigned long)code, addr );
	if( code == EXCEPTION_ACCESS_VIOLATION && info->ExceptionRecord->NumberParameters >= 2 && n > 0 && n < (int)sizeof( text ) )
	{
		const char *op = info->ExceptionRecord->ExceptionInformation[ 0 ] ? "write" : "read";
		_snprintf( text + n, sizeof( text ) - (size_t)n,
			"Access violation %s at %p\r\n",
			op, (void *)info->ExceptionRecord->ExceptionInformation[ 1 ] );
	}
	HANDLE txt = CreateFileW( text_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( txt != INVALID_HANDLE_VALUE )
	{
		DWORD wrote = 0;
		WriteFile( txt, text, (DWORD)strlen( text ), &wrote, NULL );
		CloseHandle( txt );
	}

	wchar_t pending[ 1200 ];
	wsprintfW( pending, L"%s\\pending.txt", g_dir );
	HANDLE mark = CreateFileW( pending, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( mark != INVALID_HANDLE_VALUE )
	{
		char mark_utf[ 1200 ];
		const int bytes = WideCharToMultiByte( CP_UTF8, 0, text_path, -1, mark_utf, (int)sizeof( mark_utf ), NULL, NULL );
		if( bytes > 1 )
		{
			DWORD wrote = 0;
			WriteFile( mark, mark_utf, (DWORD)( bytes - 1 ), &wrote, NULL );
		}
		CloseHandle( mark );
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

static void Crash_Prepare_Dir()
{
	wchar_t local[ 512 ];
	const DWORD n = GetEnvironmentVariableW( L"LOCALAPPDATA", local, 512 );
	if( n == 0 || n >= 512 )
		return;
	wchar_t root[ 600 ];
	wsprintfW( root, L"%s\\AQEMU", local );
	wsprintfW( g_dir, L"%s\\CrashDumps", root );
	CreateDirectoryW( root, NULL );
	CreateDirectoryW( g_dir, NULL );
	MultiByteToWideChar( CP_UTF8, 0, CURRENT_AQEMU_VERSION, -1, g_version, 64 );
	const char *qt = qVersion();
	if( ! qt )
		qt = "";
	_snprintf( g_qt, sizeof( g_qt ), "%s", qt );
	SetUnhandledExceptionFilter( Crash_Filter );
}
#else
static char g_dir[ 1024 ];
static char g_qt[ 64 ];
static volatile sig_atomic_t g_handling = 0;

static void Crash_Write_All( int fd, const char *data, size_t len )
{
	while( len > 0 )
	{
		const ssize_t n = write( fd, data, len );
		if( n < 0 )
		{
			if( errno == EINTR )
				continue;
			return;
		}
		data += n;
		len -= (size_t)n;
	}
}

/* Returning from SIGSEGV, SIGBUS, or SIGILL restarts the faulting instruction. */
static void Crash_Abort( int sig )
{
	struct sigaction sa;
	memset( &sa, 0, sizeof( sa ) );
	sa.sa_handler = SIG_DFL;
	sigemptyset( &sa.sa_mask );
	sigaction( sig, &sa, NULL );

	sigset_t set;
	sigemptyset( &set );
	sigaddset( &set, sig );
	sigprocmask( SIG_UNBLOCK, &set, NULL );
	raise( sig );
	_exit( 128 + sig );
}

static void Crash_Signal( int sig, siginfo_t *info, void * )
{
	if( g_handling )
		Crash_Abort( sig );
	g_handling = 1;
	if( g_dir[ 0 ] == 0 )
		Crash_Abort( sig );

	char stamp[ 32 ];
	time_t now = time( NULL );
	struct tm tm_buf;
	gmtime_r( &now, &tm_buf );
	snprintf( stamp, sizeof( stamp ), "%04d%02d%02d-%02d%02d%02d",
		tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
		tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec );

	char path[ 1200 ];
	snprintf( path, sizeof( path ), "%s/aqemu-%s-%s.txt", g_dir, CURRENT_AQEMU_VERSION, stamp );
	const int fd = open( path, O_CREAT | O_WRONLY | O_TRUNC, 0600 );
	if( fd >= 0 )
	{
		char head[ 512 ];
		const void *fault = info ? info->si_addr : NULL;
		const int n = snprintf( head, sizeof( head ),
			"AQEMU %s\nQt %s\nSignal %d\nFault address %p\n\n",
			CURRENT_AQEMU_VERSION, g_qt, sig, fault );
		if( n > 0 )
			Crash_Write_All( fd, head, (size_t)n );
	}

	char pending[ 1200 ];
	snprintf( pending, sizeof( pending ), "%s/pending.txt", g_dir );
	const int mark = open( pending, O_CREAT | O_WRONLY | O_TRUNC, 0600 );
	if( mark >= 0 )
	{
		Crash_Write_All( mark, path, strlen( path ) );
		close( mark );
	}

#ifdef __GLIBC__
	if( fd >= 0 )
	{
		void *frames[ 64 ];
		const int count = backtrace( frames, 64 );
		backtrace_symbols_fd( frames, count, fd );
	}
#endif
	if( fd >= 0 )
		close( fd );
	Crash_Abort( sig );
}

static void Crash_Prepare_Dir()
{
	const char *home = getenv( "HOME" );
	if( ! home || ! home[ 0 ] )
		return;
	char share[ 700 ];
	char root[ 800 ];
	snprintf( share, sizeof( share ), "%s/.local/share", home );
	snprintf( root, sizeof( root ), "%s/AQEMU", share );
	snprintf( g_dir, sizeof( g_dir ), "%s/crash", root );
	mkdir( share, 0700 );
	mkdir( root, 0700 );
	mkdir( g_dir, 0700 );
	const char *qt = qVersion();
	if( ! qt )
		qt = "";
	snprintf( g_qt, sizeof( g_qt ), "%s", qt );

	struct sigaction sa;
	memset( &sa, 0, sizeof( sa ) );
	sa.sa_sigaction = Crash_Signal;
	sigemptyset( &sa.sa_mask );
	sigaddset( &sa.sa_mask, SIGSEGV );
	sigaddset( &sa.sa_mask, SIGABRT );
	sigaddset( &sa.sa_mask, SIGBUS );
	sigaddset( &sa.sa_mask, SIGILL );
	sa.sa_flags = SA_SIGINFO;
	sigaction( SIGSEGV, &sa, NULL );
	sigaction( SIGABRT, &sa, NULL );
	sigaction( SIGBUS, &sa, NULL );
	sigaction( SIGILL, &sa, NULL );
}
#endif

static void Crash_Prune()
{
	const QString dir_path = AQ_Crash_Directory();
	QDir dir( dir_path );
	if( ! dir.exists() )
		return;
	const QStringList names = dir.entryList(
		QStringList() << QStringLiteral( "aqemu-*.dmp" ) << QStringLiteral( "aqemu-*.txt" ),
		QDir::Files, QDir::Time );
	int dmp = 0;
	int txt = 0;
	for( int i = 0; i < names.size(); ++i )
	{
		const QString name = names.at( i );
		if( name.endsWith( QLatin1String( ".dmp" ) ) )
		{
			if( ++dmp > k_keep )
				QFile::remove( dir.filePath( name ) );
		}
		else if( name.endsWith( QLatin1String( ".txt" ) ) && name != QLatin1String( "pending.txt" ) )
		{
			if( ++txt > k_keep )
				QFile::remove( dir.filePath( name ) );
		}
	}
}

void AQ_Install_Crash_Handler()
{
	Crash_Prepare_Dir();
	Crash_Prune();
}

void AQ_Test_Crash()
{
	volatile int *pointer = NULL;
	*pointer = 1;
}

QString AQ_Crash_Directory()
{
#ifdef Q_OS_WIN
	wchar_t local[ 512 ];
	const DWORD n = GetEnvironmentVariableW( L"LOCALAPPDATA", local, 512 );
	QString root = ( n > 0 && n < 512 )
		? QString::fromWCharArray( local )
		: QDir::homePath() + QStringLiteral( "/AppData/Local" );
	const QString dir = QDir( root ).filePath( QStringLiteral( "AQEMU/CrashDumps" ) );
#else
	const QString dir = QDir::homePath() + QStringLiteral( "/.local/share/AQEMU/crash" );
#endif
	QDir().mkpath( dir );
	return dir;
}

void AQ_Crash_Open_Directory()
{
	QDesktopServices::openUrl( QUrl::fromLocalFile( AQ_Crash_Directory() ) );
}

bool AQ_Crash_Consume_Pending( QString *report_path )
{
	const QString pending = QDir( AQ_Crash_Directory() ).filePath( QStringLiteral( "pending.txt" ) );
	QFile file( pending );
	if( ! file.exists() || ! file.open( QIODevice::ReadOnly ) )
		return false;
	const QString path = QString::fromUtf8( file.readAll() ).trimmed();
	file.close();
	QFile::remove( pending );
	if( report_path )
		*report_path = path;
	return true;
}

QString AQ_Crash_Setup_Markdown()
{
	return QStringLiteral(
		"| | |\n|--|--|\n| AQEMU | %1 |\n| Qt | %2 |\n| Host | %3 |\n| Architecture | %4 |\n| Crash folder | %5 |\n" )
		.arg( QLatin1String( CURRENT_AQEMU_VERSION ),
		      QLatin1String( qVersion() ),
		      QSysInfo::prettyProductName(),
		      QSysInfo::currentCpuArchitecture(),
		      AQ_Crash_Directory() );
}

void AQ_Crash_Prompt_If_Pending( QWidget *parent )
{
	QString report;
	if( ! AQ_Crash_Consume_Pending( &report ) )
		return;
	QMessageBox box( parent );
	box.setIcon( QMessageBox::Warning );
	box.setWindowTitle( QObject::tr( "AQEMU closed unexpectedly" ) );
	box.setText( QObject::tr( "AQEMU closed unexpectedly. The report stays on this computer until you attach it to an issue." ) );
	if( ! report.isEmpty() )
		box.setInformativeText( report );
	QPushButton *open = box.addButton( QObject::tr( "Open the crash folder" ), QMessageBox::AcceptRole );
	QPushButton *copy = box.addButton( QObject::tr( "Copy system info" ), QMessageBox::ActionRole );
	QPushButton *issue = box.addButton( QObject::tr( "Open a GitHub issue" ), QMessageBox::ActionRole );
	box.addButton( QObject::tr( "Close" ), QMessageBox::RejectRole );
	box.exec();
	if( box.clickedButton() == open )
		AQ_Crash_Open_Directory();
	else if( box.clickedButton() == copy )
	{
		QGuiApplication::clipboard()->setText( AQ_Crash_Setup_Markdown() );
		QMessageBox::information( parent, QObject::tr( "Copy setup" ),
			QObject::tr( "Setup details are on the clipboard. Paste them into a GitHub issue." ) );
	}
	else if( box.clickedButton() == issue )
		QDesktopServices::openUrl( QUrl( QStringLiteral( "https://github.com/chronic8000/aqemu/issues/new" ) ) );
}
