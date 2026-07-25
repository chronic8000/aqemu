/****************************************************************************
**
** Guest capability matrix for the New VM Wizard.
** Only offer devices/accelerators that make sense for the selected guest.
** Power users can still unlock the full QEMU list via "Show all options".
**
****************************************************************************/

#ifndef GUEST_CAPABILITIES_H
#define GUEST_CAPABILITIES_H

#include <QString>
#include <QStringList>
#include <QList>

/** Disk bus / interface choices the wizard understands. */
struct Guest_Disk_Option {
	QString id;       // ide | virtio | scsi | virtio-scsi | none
	QString caption;
};

struct Guest_Named_Option {
	QString id;       // qemu model / preset id
	QString caption;
	QString tip;      // optional why/when
};

struct Guest_Capabilities
{
	enum Class {
		Unknown = 0,
		Legacy_DOS,
		Legacy_Win9x,
		Legacy_NT,          // NT4 / 2000 / XP / 2003
		Modern_Windows,     // Vista+ (VirtIO optional with drivers)
		Modern_Unix,        // Linux / BSD / illumos — VirtIO preferred
		Classic_Mac,        // Mac OS 7–9 / OS X PPC on mac99
		Intel_Mac,          // macOS on q35
		OS2_Family,
		ReactOS,
		AIX_POWER,
		Retro_MIPS_SGI,     // IRIX etc.
		Embedded_ARM,
		Generic
	};

	Class guest_class = Unknown;
	QString summary;            // one-line human explanation

	bool allow_virtio_disk = false;
	bool allow_virtio_net = false;
	bool allow_virtio_gpu = false;
	bool allow_virtio_sound = false;
	bool allow_virtio_extras = false; // rng/balloon/keyboard
	bool prefer_virtio = false;       // default to VirtIO when allowed

	bool force_tcg = false;           // WHPX/KVM unsafe or useless
	bool allow_kvm_whpx = true;
	bool allow_gpu_passthrough = false; // VFIO / Windows GPU assign — modern only

	bool allow_acpi = true;
	bool allow_smp = true;

	QString default_disk;   // ide / virtio / …
	QString default_nic;
	QString default_sound;  // preset id for Apply_Sound_Preset
	QString default_video;  // qemu video id, empty = board default

	QList<Guest_Disk_Option> disk_options;
	QList<Guest_Named_Option> nic_options;
	QList<Guest_Named_Option> sound_options;
	QList<Guest_Named_Option> video_options;
};

/**
 * Compute safe option sets and defaults for a guest.
 * @param os_name   Wizard OS leaf (may be empty for Platform/Arch paths)
 * @param target    qemu-system target (x86_64, ppc, aarch64, …)
 * @param machine   -machine id (q35, mac99, virt, …)
 * @param flags     os_profiles.flags from wizard_trees.json
 */
Guest_Capabilities AQ_Compute_Guest_Capabilities(
	const QString &os_name,
	const QString &target,
	const QString &machine,
	const QStringList &flags = QStringList() );

#endif
