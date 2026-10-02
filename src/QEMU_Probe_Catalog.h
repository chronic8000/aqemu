/****************************************************************************
** Full-architecture QEMU option catalogs from qemu_probe_full_v3/*.json
** Used by the Main Window VM config panel (not the New VM wizard).
****************************************************************************/

#ifndef QEMU_PROBE_CATALOG_H
#define QEMU_PROBE_CATALOG_H

#include "VM_Devices.h"

struct Architecture_Hardware_Capabilities {
	bool valid = false;
	bool has_ide = false;
	bool has_ahci = false;
	bool has_nvme = false;
	bool has_virtio_disk = false;
	bool has_virtio_net = false;
	bool has_virtio_gpu = false;
	bool has_virtio_sound = false;
	bool has_scsi = false;
	bool has_sound = false;
	bool has_pci = false;
	bool has_kvm = false;
	bool has_tcg = true;
	QStringList supported_network_cards;
	QStringList supported_video_cards;
	VM::Sound_Cards supported_sound_cards;

	bool Is_Disk_Bus_Supported( const QString &bus_id ) const
	{
		const QString b = bus_id.trimmed().toLower();
		if( b == QLatin1String( "ide" ) ) return has_ide;
		if( b == QLatin1String( "sata" ) || b == QLatin1String( "ahci" ) ) return has_ahci;
		if( b == QLatin1String( "nvme" ) ) return has_nvme;
		if( b == QLatin1String( "virtio" ) || b == QLatin1String( "virtio-blk" ) ) return has_virtio_disk;
		if( b == QLatin1String( "virtio-scsi" ) ) return has_virtio_disk || has_scsi;
		if( b == QLatin1String( "scsi" ) ) return has_scsi;
		if( b == QLatin1String( "none" ) ) return true;
		return true;
	}

	bool Is_Disk_Bus_Supported( VM::Device_Interface iface ) const
	{
		switch( iface )
		{
			case VM::DI_IDE: return has_ide;
			case VM::DI_AHCI: return has_ahci;
			case VM::DI_NVMe: return has_nvme;
			case VM::DI_Virtio: return has_virtio_disk;
			case VM::DI_Virtio_SCSI: return has_virtio_disk || has_scsi;
			case VM::DI_SCSI: return has_scsi;
			case VM::DI_SD: return has_pci || false;
			case VM::DI_Floppy: return has_ide || has_pci;
			default: return true;
		}
	}

	bool Is_Sound_Supported( const QString &sound_preset ) const
	{
		const QString s = sound_preset.trimmed().toLower();
		if( s == QLatin1String( "none" ) ) return true;
		if( ! has_sound ) return false;
		if( s == QLatin1String( "hda" ) ) return supported_sound_cards.Audio_HDA;
		if( s == QLatin1String( "ac97" ) ) return supported_sound_cards.Audio_AC97;
		if( s == QLatin1String( "sb16" ) || s.startsWith( QLatin1String( "sb16_" ) ) ) return supported_sound_cards.Audio_sb16;
		if( s == QLatin1String( "es1370" ) || s.startsWith( QLatin1String( "es1370_" ) ) ) return supported_sound_cards.Audio_es1370;
		if( s == QLatin1String( "virtio" ) ) return supported_sound_cards.Audio_VirtIO;
		if( s == QLatin1String( "hda_virtio" ) ) return supported_sound_cards.Audio_HDA || supported_sound_cards.Audio_VirtIO;
		if( s == QLatin1String( "pcspk" ) ) return supported_sound_cards.Audio_PC_Speaker;
		return true;
	}

	bool Is_NIC_Supported( const QString &nic_model ) const
	{
		if( supported_network_cards.isEmpty() ) return true;
		const QString m = nic_model.trimmed().toLower();
		for( const QString &net : supported_network_cards )
		{
			if( net.compare( m, Qt::CaseInsensitive ) == 0 ) return true;
		}
		return false;
	}

	bool Is_Video_Supported( const QString &video_model ) const
	{
		if( supported_video_cards.isEmpty() ) return true;
		const QString v = video_model.trimmed().toLower();
		if( v.isEmpty() || v == QLatin1String( "none" ) ) return true;
		for( const QString &vid : supported_video_cards )
		{
			if( vid.compare( v, Qt::CaseInsensitive ) == 0 ) return true;
		}
		return false;
	}
};

class QEMU_Probe_Catalog
{
	public:
		/** Directory containing {arch}.json probes, or empty if not found. */
		static QString Probe_Directory();

		/** Map "qemu-system-ppc" / "ppc" / caption text → probe stem ("ppc"). */
		static QString Architecture_Key( const QString &computer_type_or_binary );

		/** Return hardware capabilities (disk bus, audio, PCI, net, video) for arch. */
		static Architecture_Hardware_Capabilities Get_Hardware_Capabilities(
			const QString &computer_type_or_binary );

		/** Load one probe into Available_Devices lists (machines/CPUs/net/video/audio). */
		static bool Load_Architecture( const QString &computer_type_or_binary,
		                               Available_Devices &out );

		/**
		 * Replace Machine/CPU/Network/Video lists (and OR audio flags) from the
		 * probe when present. Keeps PSO_* and System caption from `dev`.
		 */
		static bool Merge_Into( Available_Devices &dev );

		/** Parse -device help / probe "devices" lines into net + display maps. */
		static void Parse_Device_Help_Lines( const QStringList &lines,
		                                     QList<Device_Map> &network_out,
		                                     QList<Device_Map> &display_out,
		                                     VM::Sound_Cards *audio_out = nullptr );

		static void Parse_Machine_Help_Lines( const QStringList &lines,
		                                      QList<Device_Map> &out );
		static void Parse_CPU_Help_Lines( const QStringList &lines,
		                                  QList<Device_Map> &out );

		/** Host -audiodev drivers from the probe's raw.audio text. */
		static QStringList Audio_Drivers( const QString &computer_type_or_binary );

		/** Prefer candidates that exist in qemu_probe_full_v3 for this arch. */
		static QString First_Available_CPU( const QString &computer_type_or_binary,
		                                    const QStringList &candidates );
		static bool Architecture_Has_CPU( const QString &computer_type_or_binary,
		                                  const QString &cpu_name );
};

#endif
