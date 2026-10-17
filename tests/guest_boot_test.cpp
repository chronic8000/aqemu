#include "Guest_Boot.h"
#include "QEMU_Probe_Catalog.h"
#include "Utils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <cstdio>

void AQDebug( const QString &, const QString & )
{
}

/* The real constructor lives in VM_Devices.cpp, which pulls the rest of the VM. */
Available_Devices::Available_Devices()
{
}

static int g_failures = 0;

#define CHECK( cond ) \
	do { \
		if( !( cond ) ) { \
			std::fprintf( stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #cond ); \
			++g_failures; \
		} \
	} while( 0 )

static bool Token_Has( const QStringList &have, const QString &want )
{
	for( int i = 0; i < have.size(); ++i )
	{
		if( have.at( i ).compare( want, Qt::CaseInsensitive ) == 0 )
			return true;
	}
	return false;
}

int main( int argc, char **argv )
{
	QCoreApplication app( argc, argv );
	const QDir app_dir( QCoreApplication::applicationDirPath() );
	QString trees = app_dir.filePath( QStringLiteral( "../resources/wizard_trees.json" ) );
	if( ! QFile::exists( trees ) )
		trees = app_dir.filePath( QStringLiteral( "../../resources/wizard_trees.json" ) );
	QFile file( trees );
	CHECK( file.open( QIODevice::ReadOnly ) );
	const QJsonObject root = QJsonDocument::fromJson( file.readAll() ).object();
	const QJsonObject profiles = root.value( QStringLiteral( "os_profiles" ) ).toObject();
	CHECK( profiles.size() > 50 );

	const QString probe_dir = QEMU_Probe_Catalog::Probe_Directory();
	CHECK( ! probe_dir.isEmpty() );
	std::fprintf( stderr, "probes %s\nprofiles %d\n",
		qPrintable( probe_dir ), profiles.size() );

	const QStringList need_files = AQ_Guests_That_Need_Boot_Files();
	for( int i = 0; i < need_files.size(); ++i )
		CHECK( profiles.contains( need_files.at( i ) ) );

	const QStringList names = profiles.keys();
	for( int i = 0; i < names.size(); ++i )
	{
		const QString os = names.at( i );
		const QJsonObject profile = profiles.value( os ).toObject();
		const QString target = profile.value( QStringLiteral( "target" ) ).toString();
		const QString machine_hint = profile.value( QStringLiteral( "machine" ) ).toString();
		const Guest_Boot_Set boot = AQ_Resolve_Guest_Boot( os, target, machine_hint, profile );
		if( need_files.contains( os ) )
		{
			bool required = false;
			for( int f = 0; f < boot.fields.size(); ++f )
				required = required || boot.fields.at( f ).required;
			if( ! required )
				std::fprintf( stderr, "missing required boot field: %s\n", qPrintable( os ) );
			CHECK( required );
		}
		if( ! boot.probe_loaded )
		{
			if( target != QLatin1String( "host" ) )
				std::fprintf( stderr, "no probe for %s target %s\n", qPrintable( os ), qPrintable( target ) );
			CHECK( target == QLatin1String( "host" ) || target.isEmpty() );
			continue;
		}

		const Architecture_Hardware_Capabilities caps =
			QEMU_Probe_Catalog::Get_Hardware_Capabilities( target );
		CHECK( caps.valid );
		if( ! boot.machine.isEmpty() )
		{
			const QStringList machines = QEMU_Probe_Catalog::Section_Tokens( target, QStringLiteral( "machines" ) );
			if( ! Token_Has( machines, boot.machine ) )
				std::fprintf( stderr, "%s machine %s not in probe\n", qPrintable( os ), qPrintable( boot.machine ) );
			CHECK( Token_Has( machines, boot.machine ) );
		}
		if( ! boot.cpu.isEmpty() )
		{
			if( ! QEMU_Probe_Catalog::Architecture_Has_CPU( target, boot.cpu ) )
				std::fprintf( stderr, "%s cpu %s not in probe\n", qPrintable( os ), qPrintable( boot.cpu ) );
			CHECK( QEMU_Probe_Catalog::Architecture_Has_CPU( target, boot.cpu ) );
		}
		if( ! boot.disk_bus.isEmpty() && ! caps.Is_Disk_Bus_Supported( boot.disk_bus ) )
		{
			std::fprintf( stderr, "%s disk %s unsupported\n", qPrintable( os ), qPrintable( boot.disk_bus ) );
			CHECK( false );
		}
		if( ! boot.disk_format.isEmpty() )
		{
			const QStringList formats = QEMU_Probe_Catalog::Section_Tokens( target, QStringLiteral( "drive_formats" ) );
			CHECK( Token_Has( formats, boot.disk_format ) );
		}
		if( ! boot.nic.isEmpty() && ! caps.Is_NIC_Supported( boot.nic ) )
		{
			std::fprintf( stderr, "%s nic %s unsupported\n", qPrintable( os ), qPrintable( boot.nic ) );
			CHECK( false );
		}
		if( ! boot.netdev.isEmpty() )
		{
			const QStringList netdevs = QEMU_Probe_Catalog::Section_Tokens( target, QStringLiteral( "netdev" ) );
			CHECK( Token_Has( netdevs, boot.netdev ) );
		}
		if( ! boot.sound.isEmpty() && ! caps.Is_Sound_Supported( boot.sound ) )
		{
			std::fprintf( stderr, "%s sound %s unsupported\n", qPrintable( os ), qPrintable( boot.sound ) );
			CHECK( false );
		}
		if( ! boot.video.isEmpty() && ! caps.Is_Video_Supported( boot.video ) )
		{
			std::fprintf( stderr, "%s video %s unsupported\n", qPrintable( os ), qPrintable( boot.video ) );
			CHECK( false );
		}
		if( ! boot.display.isEmpty() )
		{
			const QStringList displays = QEMU_Probe_Catalog::Section_Tokens( target, QStringLiteral( "display_backends" ) );
			CHECK( Token_Has( displays, boot.display ) );
		}
		if( ! boot.audiodev.isEmpty() )
		{
			QStringList audio = QEMU_Probe_Catalog::Section_Tokens( target, QStringLiteral( "audio_backends" ) );
			if( audio.isEmpty() )
				audio = QEMU_Probe_Catalog::Audio_Drivers( target );
			CHECK( Token_Has( audio, boot.audiodev ) );
		}
		if( ! boot.usb.isEmpty() )
		{
			const QStringList usb = QEMU_Probe_Catalog::USB_Controllers( target );
			CHECK( Token_Has( usb, boot.usb ) );
		}
	}

	if( g_failures )
		std::fprintf( stderr, "%d failure(s)\n", g_failures );
	else
		std::fprintf( stderr, "ok\n" );
	return g_failures ? 1 : 0;
}
