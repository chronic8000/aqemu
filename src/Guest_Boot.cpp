/****************************************************************************
**
** Copyright (C) 2026 Chronic Engineering
**
****************************************************************************/

#include "Guest_Boot.h"

#include <QJsonArray>

#include "Guest_Capabilities.h"
#include "QEMU_Probe_Catalog.h"
#include "Utils.h"

namespace {

QStringList Profile_List( const QJsonObject &profile, const char *key )
{
	QStringList out;
	const QJsonValue v = profile.value( QLatin1String( key ) );
	if( v.isArray() )
	{
		const QJsonArray arr = v.toArray();
		for( int i = 0; i < arr.size(); ++i )
		{
			const QString s = arr.at( i ).toString().trimmed();
			if( ! s.isEmpty() )
				out << s;
		}
	}
	else if( v.isString() )
	{
		const QString s = v.toString().trimmed();
		if( ! s.isEmpty() )
			out << s;
	}
	return out;
}

QString Pick( const QStringList &prefer, const QStringList &have, bool probe_loaded )
{
	if( have.isEmpty() )
		return ( probe_loaded || prefer.isEmpty() ) ? QString() : prefer.first();
	/* Empty fallback: only return a name the probe actually lists. */
	return Guest_Capabilities::Resolve_Optimal_Device( prefer, have, QString() );
}

QString Canon_Sound( const QString &sound )
{
	const QString s = sound.trimmed().toLower();
	if( s == QLatin1String( "intel-hda" ) || s == QLatin1String( "ich9-intel-hda" )
		|| s == QLatin1String( "hda-duplex" ) )
		return QStringLiteral( "hda" );
	return s;
}

QStringList Disk_Buses( const Architecture_Hardware_Capabilities &caps )
{
	QStringList ids;
	if( ! caps.valid )
		return ids;
	if( caps.has_ide ) ids << QStringLiteral( "ide" );
	if( caps.has_ahci )
	{
		ids << QStringLiteral( "ahci" );
		ids << QStringLiteral( "sata" );
	}
	if( caps.has_nvme ) ids << QStringLiteral( "nvme" );
	if( caps.has_virtio_disk ) ids << QStringLiteral( "virtio" );
	if( caps.has_virtio_disk || caps.has_scsi ) ids << QStringLiteral( "virtio-scsi" );
	if( caps.has_scsi ) ids << QStringLiteral( "scsi" );
	return ids;
}

bool Legacy_Guest( const QString &os )
{
	return os == QLatin1String( "MS-DOS" ) || os == QLatin1String( "PC DOS" )
		|| os == QLatin1String( "DR-DOS" ) || os == QLatin1String( "FreeDOS" )
		|| os == QLatin1String( "Windows 95" ) || os == QLatin1String( "Windows 98" )
		|| os == QLatin1String( "Windows ME" ) || os == QLatin1String( "Windows 1.x" )
		|| os == QLatin1String( "Windows 2.x" ) || os == QLatin1String( "Windows 3.x" )
		|| os == QLatin1String( "Windows NT 3.x" ) || os == QLatin1String( "Windows NT 4.0" )
		|| os == QLatin1String( "Windows 2000" ) || os.startsWith( QLatin1String( "Windows XP" ) );
}

QString Pick_USB( const QString &os, const QStringList &controllers, bool probe_loaded )
{
	QStringList prefer;
	if( Legacy_Guest( os ) )
		prefer << QStringLiteral( "piix3-usb-uhci" ) << QStringLiteral( "piix4-usb-uhci" )
		       << QStringLiteral( "ich9-usb-uhci1" );
	else
		prefer << QStringLiteral( "qemu-xhci" ) << QStringLiteral( "nec-usb-xhci" )
		       << QStringLiteral( "ich9-usb-ehci1" );
	return Pick( prefer, controllers, probe_loaded );
}

QString Pick_Host_Audio( const QStringList &drivers, bool probe_loaded )
{
	QStringList prefer;
	QStringList usable;
#ifdef Q_OS_WIN
	prefer << QStringLiteral( "dsound" ) << QStringLiteral( "sdl" ) << QStringLiteral( "none" );
#else
	prefer << QStringLiteral( "alsa" ) << QStringLiteral( "pa" ) << QStringLiteral( "pipewire" )
	       << QStringLiteral( "sdl" ) << QStringLiteral( "spice" );
#endif
	prefer << QStringLiteral( "none" );
	for( int i = 0; i < drivers.count(); ++i )
	{
		const QString n = drivers.at( i ).trimmed().toLower();
		if( AQ_Audio_Host_Can_Use( n ) )
			usable << n;
	}
	return Pick( prefer, usable, probe_loaded );
}

QString Pick_Display( const QStringList &backends, bool probe_loaded )
{
	QStringList prefer;
#ifdef Q_OS_WIN
	prefer << QStringLiteral( "spice-app" ) << QStringLiteral( "gtk" ) << QStringLiteral( "sdl" );
#else
	prefer << QStringLiteral( "gtk" ) << QStringLiteral( "sdl" ) << QStringLiteral( "spice-app" )
	       << QStringLiteral( "cocoa" );
#endif
	return Pick( prefer, backends, probe_loaded );
}

}

Guest_Boot_Set AQ_Resolve_Guest_Boot( const QString &os_name,
	const QString &target,
	const QString &machine,
	const QJsonObject &profile )
{
	Guest_Boot_Set boot;
	const Architecture_Hardware_Capabilities caps =
		QEMU_Probe_Catalog::Get_Hardware_Capabilities( target );
	boot.probe_loaded = caps.valid;

	Available_Devices devices;
	const bool have_lists = QEMU_Probe_Catalog::Load_Architecture( target, devices );

	QStringList machines;
	QStringList cpus;
	if( have_lists )
	{
		for( int i = 0; i < devices.Machine_List.count(); ++i )
			machines << devices.Machine_List[i].QEMU_Name;
		for( int i = 0; i < devices.CPU_List.count(); ++i )
			cpus << devices.CPU_List[i].QEMU_Name;
	}

	QStringList machine_prefer = Profile_List( profile, "machine" );
	if( machine_prefer.isEmpty() && ! machine.trimmed().isEmpty() )
		machine_prefer << machine.trimmed();
	boot.machine = Pick( machine_prefer, machines, boot.probe_loaded );

	QStringList cpu_prefer = Profile_List( profile, "cpu" );
	if( cpu_prefer.isEmpty() )
	{
		if( Legacy_Guest( os_name ) )
			cpu_prefer << QStringLiteral( "pentium3" ) << QStringLiteral( "486" ) << QStringLiteral( "qemu32" );
		else
			cpu_prefer << QStringLiteral( "max" ) << QStringLiteral( "qemu64" ) << QStringLiteral( "host" )
			           << QStringLiteral( "cortex-a72" ) << QStringLiteral( "cortex-a57" );
	}
	boot.cpu = QEMU_Probe_Catalog::First_Available_CPU( target, cpu_prefer );
	if( boot.cpu.isEmpty() )
		boot.cpu = Pick( cpu_prefer, cpus, boot.probe_loaded );

	QStringList disk_prefer = Profile_List( profile, "disk_bus" );
	if( disk_prefer.isEmpty() )
		disk_prefer << ( Legacy_Guest( os_name ) ? QStringLiteral( "ide" ) : QStringLiteral( "virtio" ) )
		            << QStringLiteral( "ahci" ) << QStringLiteral( "scsi" ) << QStringLiteral( "ide" );
	boot.disk_bus = Pick( disk_prefer, Disk_Buses( caps ), boot.probe_loaded );

	const QStringList formats = QEMU_Probe_Catalog::Section_Tokens( target, QStringLiteral( "drive_formats" ) );
	boot.disk_format = Pick( QStringList() << QStringLiteral( "qcow2" ) << QStringLiteral( "raw" ), formats, boot.probe_loaded );

	QStringList nic_prefer = Profile_List( profile, "nic" );
	if( nic_prefer.isEmpty() )
		nic_prefer << ( Legacy_Guest( os_name ) ? QStringLiteral( "rtl8139" ) : QStringLiteral( "virtio-net-pci" ) )
		           << QStringLiteral( "e1000" ) << QStringLiteral( "rtl8139" );
	boot.nic = Pick( nic_prefer, caps.supported_network_cards, boot.probe_loaded );

	const QStringList netdevs = QEMU_Probe_Catalog::Section_Tokens( target, QStringLiteral( "netdev" ) );
	boot.netdev = Pick( QStringList() << QStringLiteral( "user" ) << QStringLiteral( "tap" ), netdevs, boot.probe_loaded );

	QStringList sound_prefer = Profile_List( profile, "sound" );
	for( int i = 0; i < sound_prefer.size(); ++i )
		sound_prefer[i] = Canon_Sound( sound_prefer.at( i ) );
	if( sound_prefer.isEmpty() )
		sound_prefer << ( Legacy_Guest( os_name ) ? QStringLiteral( "sb16" ) : QStringLiteral( "hda" ) )
		             << QStringLiteral( "ac97" ) << QStringLiteral( "none" );
	QStringList sound_have;
	if( caps.valid )
	{
		for( int i = 0; i < sound_prefer.size(); ++i )
		{
			if( caps.Is_Sound_Supported( sound_prefer.at( i ) ) )
				sound_have << sound_prefer.at( i );
		}
		if( ! sound_have.contains( QStringLiteral( "none" ) ) )
			sound_have << QStringLiteral( "none" );
	}
	boot.sound = Pick( sound_prefer, sound_have, boot.probe_loaded );

	QStringList video_prefer = Profile_List( profile, "vga" );
	if( video_prefer.isEmpty() )
		video_prefer << ( Legacy_Guest( os_name ) ? QStringLiteral( "cirrus" ) : QStringLiteral( "std" ) )
		             << QStringLiteral( "virtio" ) << QStringLiteral( "std" );
	boot.video = Pick( video_prefer, caps.supported_video_cards, boot.probe_loaded );

	boot.display = Pick_Display( QEMU_Probe_Catalog::Section_Tokens( target, QStringLiteral( "display_backends" ) ), boot.probe_loaded );
	boot.audiodev = Pick_Host_Audio( QEMU_Probe_Catalog::Section_Tokens( target, QStringLiteral( "audio_backends" ) ), boot.probe_loaded );
	if( boot.audiodev.isEmpty() )
		boot.audiodev = Pick_Host_Audio( QEMU_Probe_Catalog::Audio_Drivers( target ), boot.probe_loaded );

	boot.usb = Pick_USB( os_name, QEMU_Probe_Catalog::USB_Controllers( target ), boot.probe_loaded );

	const QStringList accels = QEMU_Probe_Catalog::Section_Tokens( target, QStringLiteral( "accelerators" ) );
	const bool has_whpx = accels.contains( QStringLiteral( "whpx" ) );
	const bool has_kvm = accels.contains( QStringLiteral( "kvm" ) );
	boot.native_accel = has_whpx || has_kvm;
	if( ! caps.valid )
		boot.native_accel = true;

	const QJsonArray fields = profile.value( QLatin1String( "boot_fields" ) ).toArray();
	for( int i = 0; i < fields.size(); ++i )
	{
		const QJsonObject o = fields.at( i ).toObject();
		Guest_Boot_Field field;
		field.id = o.value( QLatin1String( "id" ) ).toString().trimmed();
		field.label = o.value( QLatin1String( "label" ) ).toString().trimmed();
		field.required = o.value( QLatin1String( "required" ) ).toBool( true );
		field.is_file = o.value( QLatin1String( "kind" ) ).toString() != QLatin1String( "text" );
		if( field.id.isEmpty() )
			continue;
		if( field.label.isEmpty() )
			field.label = field.id;
		boot.fields.append( field );
	}
	return boot;
}

QStringList AQ_Guests_That_Need_Boot_Files()
{
	return QStringList()
		<< QStringLiteral( "PowerNV" )
		<< QStringLiteral( "s390x Debian" )
		<< QStringLiteral( "RISC-V virt lab" )
		<< QStringLiteral( "LoongArch virt lab" )
		<< QStringLiteral( "Original Xbox" )
		<< QStringLiteral( "Nitro Enclave" )
		<< QStringLiteral( "Darwin research shell" );
}
