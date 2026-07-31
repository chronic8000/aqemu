/****************************************************************************
** Full-architecture QEMU option catalogs from qemu_probe_full_v3/*.json
** Used by the Main Window VM config panel (not the New VM wizard).
****************************************************************************/

#ifndef QEMU_PROBE_CATALOG_H
#define QEMU_PROBE_CATALOG_H

#include "VM_Devices.h"

class QEMU_Probe_Catalog
{
	public:
		/** Directory containing {arch}.json probes, or empty if not found. */
		static QString Probe_Directory();

		/** Map "qemu-system-ppc" / "ppc" / caption text → probe stem ("ppc"). */
		static QString Architecture_Key( const QString &computer_type_or_binary );

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

		/** Prefer candidates that exist in qemu_probe_full_v3 for this arch. */
		static QString First_Available_CPU( const QString &computer_type_or_binary,
		                                    const QStringList &candidates );
		static bool Architecture_Has_CPU( const QString &computer_type_or_binary,
		                                  const QString &cpu_name );
};

#endif
