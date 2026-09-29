/****************************************************************************
**
** Guest capability matrix for the New VM Wizard.
**
****************************************************************************/

#include "Guest_Capabilities.h"

#include <QObject>
#include <QtGlobal>

static Guest_Named_Option N( const char *id, const char *caption, const char *tip = "" )
{
	Guest_Named_Option o;
	o.id = QString::fromUtf8( id );
	o.caption = QObject::tr( caption );
	o.tip = tip ? QObject::tr( tip ) : QString();
	return o;
}

static Guest_Disk_Option D( const char *id, const char *caption )
{
	Guest_Disk_Option o;
	o.id = QString::fromUtf8( id );
	o.caption = QObject::tr( caption );
	return o;
}

static void add_unique( QList<Guest_Named_Option> &list, const Guest_Named_Option &opt )
{
	for( int i = 0; i < list.size(); ++i )
	{
		if( list[i].id == opt.id )
			return;
	}
	list << opt;
}

static bool has_flag( const QStringList &flags, const char *name )
{
	return flags.contains( QLatin1String( name ) );
}

Guest_Capabilities AQ_Compute_Guest_Capabilities(
	const QString &os_name,
	const QString &target,
	const QString &machine,
	const QStringList &flags )
{
	Guest_Capabilities c;
	const QString os = os_name;
	const QString tgt = target.toLower();
	const QString mach = machine.toLower();

	// --- Classify ---
	const bool classic_mac =
		os == QLatin1String( "Mac OS 7" ) || os == QLatin1String( "Mac OS 8" ) ||
		os == QLatin1String( "Mac OS 9" ) || os == QLatin1String( "Mac OS X PPC" ) ||
		( mach.contains( QLatin1String( "mac99" ) ) && tgt.startsWith( QLatin1String( "ppc" ) ) );

	const bool intel_mac =
		os == QLatin1String( "macOS" ) || os == QLatin1String( "Mac OS X Intel" ) ||
		has_flag( flags, "intel_macos" );

	const bool win9x =
		has_flag( flags, "win95_98" ) ||
		os == QLatin1String( "Windows 95" ) || os == QLatin1String( "Windows 98" ) ||
		os == QLatin1String( "Windows ME" );

	const bool dos_family =
		os == QLatin1String( "MS-DOS" ) || os == QLatin1String( "PC DOS" ) ||
		os == QLatin1String( "DR-DOS" ) || os == QLatin1String( "FreeDOS" ) ||
		os == QLatin1String( "Windows 1.x" ) || os == QLatin1String( "Windows 2.x" ) ||
		os == QLatin1String( "Windows 3.x" );

	const bool legacy_nt =
		os == QLatin1String( "Windows NT 3.x" ) || os == QLatin1String( "Windows NT 4.0" ) ||
		os == QLatin1String( "Windows 2000" ) ||
		os.startsWith( QLatin1String( "Windows XP" ) ) ||
		os == QLatin1String( "Windows Server 2000" ) ||
		os == QLatin1String( "Windows Server 2003" );

	const bool modern_win =
		os.contains( QLatin1String( "Windows" ) ) && ! win9x && ! dos_family && ! legacy_nt &&
		! os.contains( QLatin1String( "NT 3" ) ) && ! os.contains( QLatin1String( "NT 4" ) );

	const bool os2 =
		os == QLatin1String( "OS/2" ) || os == QLatin1String( "eComStation" ) ||
		os == QLatin1String( "ArcaOS" );

	const bool reactos = os.startsWith( QLatin1String( "ReactOS" ) );
	const bool aix = ( os == QLatin1String( "AIX" ) );
	const bool irix = os.startsWith( QLatin1String( "IRIX" ) );

	const bool modern_unix =
		os.contains( QLatin1String( "Linux" ) ) || os.contains( QLatin1String( "Ubuntu" ) ) ||
		os.contains( QLatin1String( "Debian" ) ) || os.contains( QLatin1String( "Fedora" ) ) ||
		os.contains( QLatin1String( "BSD" ) ) || os.contains( QLatin1String( "illumos" ) ) ||
		os.contains( QLatin1String( "Solaris" ) ) || os == QLatin1String( "NixOS" ) ||
		os == QLatin1String( "SteamOS" ) || os.contains( QLatin1String( "Chrome" ) ) ||
		os == QLatin1String( "Haiku (64-bit)" ) || os == QLatin1String( "SerenityOS" ) ||
		os == QLatin1String( "Alpine Linux (64-bit)" ) || os == QLatin1String( "Pop!_OS" ) ||
		os.contains( QLatin1String( "TrueNAS" ), Qt::CaseInsensitive ) ||
		os.contains( QLatin1String( "FreeNAS" ), Qt::CaseInsensitive ) ||
		has_flag( flags, "truenas" );

	const bool embedded_arm =
		tgt == QLatin1String( "aarch64" ) || tgt == QLatin1String( "arm" ) ||
		mach.contains( QLatin1String( "virt" ) ) || mach.contains( QLatin1String( "raspi" ) );

	if( classic_mac )
		c.guest_class = Guest_Capabilities::Classic_Mac;
	else if( intel_mac )
		c.guest_class = Guest_Capabilities::Intel_Mac;
	else if( dos_family )
		c.guest_class = Guest_Capabilities::Legacy_DOS;
	else if( win9x )
		c.guest_class = Guest_Capabilities::Legacy_Win9x;
	else if( legacy_nt )
		c.guest_class = Guest_Capabilities::Legacy_NT;
	else if( os2 )
		c.guest_class = Guest_Capabilities::OS2_Family;
	else if( reactos )
		c.guest_class = Guest_Capabilities::ReactOS;
	else if( aix )
		c.guest_class = Guest_Capabilities::AIX_POWER;
	else if( irix )
		c.guest_class = Guest_Capabilities::Retro_MIPS_SGI;
	else if( modern_win )
		c.guest_class = Guest_Capabilities::Modern_Windows;
	else if( modern_unix )
		c.guest_class = Guest_Capabilities::Modern_Unix;
	else if( embedded_arm && os.isEmpty() )
		c.guest_class = Guest_Capabilities::Embedded_ARM;
	else
		c.guest_class = Guest_Capabilities::Generic;

	// Defaults that are always safe to start from for PC-ish guests
	c.default_disk = QStringLiteral( "ide" );
	c.default_nic = QStringLiteral( "e1000" );
	c.default_sound = QStringLiteral( "hda" );
	c.default_video = QStringLiteral( "std" );
	c.allow_acpi = true;
	c.allow_smp = true;
	c.allow_kvm_whpx = true;
	c.force_tcg = false;

	switch( c.guest_class )
	{
	case Guest_Capabilities::Legacy_DOS:
		c.summary = QObject::tr(
			"DOS / Win3.x: IDE disk, NE2000, Sound Blaster / AdLib. No VirtIO (no drivers)." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_acpi = false;
		c.allow_smp = false;
		c.allow_virtio_disk = false;
		c.allow_virtio_net = false;
		c.allow_virtio_gpu = false;
		c.allow_virtio_sound = false;
		c.allow_gpu_passthrough = false;
		c.default_disk = QStringLiteral( "ide" );
		c.default_nic = QStringLiteral( "ne2k_pci" );
		c.default_sound = QStringLiteral( "sb16_adlib_pcspk" );
		c.default_video = QStringLiteral( "cirrus" );
		c.disk_options << D( "ide", "IDE (compatible)" );
		c.nic_options << N( "ne2k_pci", "NE2000 PCI", "Best for DOS packet drivers" )
		              << N( "ne2k_isa", "NE2000 ISA" )
		              << N( "pcnet", "PCNet" );
		c.sound_options << N( "sb16_adlib_pcspk", "Sound Blaster 16 + AdLib + PC Speaker" )
		                << N( "sb16", "Sound Blaster 16" )
		                << N( "pcspk", "PC Speaker only" )
		                << N( "none", "No sound" );
		c.video_options << N( "cirrus", "Cirrus VGA" ) << N( "std", "Standard VGA" );
		break;

	case Guest_Capabilities::Legacy_Win9x:
		c.summary = QObject::tr(
			"Windows 9x: IDE, NE2000/rtl8139, Sound Blaster. VirtIO and modern GPU passthrough are unavailable — "
			"these guests have no VirtIO drivers and WHPX often hangs at the splash screen." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_acpi = false;
		c.allow_smp = false;
		c.allow_virtio_disk = false;
		c.allow_virtio_net = false;
		c.allow_virtio_gpu = false;
		c.allow_virtio_sound = false;
		c.allow_gpu_passthrough = false;
		c.default_disk = QStringLiteral( "ide" );
		c.default_nic = QStringLiteral( "ne2k_pci" );
		c.default_sound = QStringLiteral( "sb16" );
		c.default_video = QStringLiteral( "cirrus" );
		c.disk_options << D( "ide", "IDE (required for setup)" );
		c.nic_options << N( "ne2k_pci", "NE2000 PCI" )
		              << N( "rtl8139", "Realtek RTL8139" )
		              << N( "pcnet", "PCNet" );
		c.sound_options << N( "sb16", "Sound Blaster 16" )
		                << N( "sb16_adlib_pcspk", "SB16 + AdLib + PC Speaker" )
		                << N( "es1370", "ENS1370" )
		                << N( "none", "No sound" );
		c.video_options << N( "cirrus", "Cirrus VGA (recommended)" )
		                << N( "std", "Standard VGA" );
		break;

	case Guest_Capabilities::Legacy_NT:
		c.summary = QObject::tr(
			"Windows NT/2000/XP: IDE disk, e1000/rtl8139. No VirtIO during setup (install drivers later if you want). "
			"TCG is preferred — WHPX often blacks out text-mode setup." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_virtio_disk = false;
		c.allow_virtio_net = false;
		c.allow_virtio_gpu = false;
		c.allow_virtio_sound = false;
		c.allow_gpu_passthrough = false;
		c.allow_smp = false;
		c.default_disk = QStringLiteral( "ide" );
		c.default_nic = QStringLiteral( "e1000" );
		c.default_sound = QStringLiteral( "ac97" );
		c.default_video = QStringLiteral( "std" );
		c.disk_options << D( "ide", "IDE" );
		c.nic_options << N( "e1000", "Intel e1000" )
		              << N( "rtl8139", "Realtek RTL8139" )
		              << N( "pcnet", "PCNet" );
		c.sound_options << N( "ac97", "AC97" )
		                << N( "es1370", "ENS1370" )
		                << N( "sb16", "Sound Blaster 16" )
		                << N( "none", "No sound" );
		c.video_options << N( "std", "Standard VGA (best for XP setup)" )
		                << N( "cirrus", "Cirrus VGA" );
		break;

	case Guest_Capabilities::OS2_Family:
		c.summary = QObject::tr(
			"OS/2 / ArcaOS: IDE only (LVM has no VirtIO). ES1370 audio, rtl8139. ACPI off." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_acpi = false;
		c.allow_smp = false;
		c.allow_virtio_disk = false;
		c.allow_virtio_net = false;
		c.allow_virtio_gpu = false;
		c.allow_gpu_passthrough = false;
		c.default_disk = QStringLiteral( "ide" );
		c.default_nic = QStringLiteral( "rtl8139" );
		c.default_sound = QStringLiteral( "es1370" );
		c.default_video = QStringLiteral( "cirrus" );
		c.disk_options << D( "ide", "IDE" );
		c.nic_options << N( "rtl8139", "Realtek RTL8139" ) << N( "e1000", "Intel e1000" );
		c.sound_options << N( "es1370", "ENS1370" ) << N( "ac97", "AC97" ) << N( "none", "No sound" );
		c.video_options << N( "cirrus", "Cirrus VGA" ) << N( "std", "Standard VGA" );
		break;

	case Guest_Capabilities::ReactOS:
		c.summary = QObject::tr(
			"ReactOS: classic -hda, e1000, AC97, std VGA (wiki-proven). VirtIO not default." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_virtio_disk = false;
		c.allow_virtio_net = false;
		c.allow_virtio_gpu = false;
		c.allow_gpu_passthrough = false;
		c.allow_smp = false;
		c.default_disk = QStringLiteral( "ide" );
		c.default_nic = QStringLiteral( "e1000" );
		c.default_sound = QStringLiteral( "ac97" );
		c.default_video = QStringLiteral( "std" );
		c.disk_options << D( "ide", "IDE / classic -hda" );
		c.nic_options << N( "e1000", "Intel e1000" ) << N( "rtl8139", "RTL8139" );
		c.sound_options << N( "ac97", "AC97" ) << N( "none", "No sound" );
		c.video_options << N( "std", "Standard VGA" ) << N( "cirrus", "Cirrus (needs NT driver)" );
		break;

	case Guest_Capabilities::Classic_Mac:
		c.summary = QObject::tr(
			"Classic Mac (mac99): board video + sungem NIC. No AdLib/SB16 (those are PC ISA toys). "
			"Screamer audio is used when available." );
		c.force_tcg = false;
		c.allow_kvm_whpx = false; // host accel rarely helps ppc on x86
		c.allow_acpi = false;
		c.allow_smp = false;
		c.allow_virtio_disk = false;
		c.allow_virtio_net = false;
		c.allow_virtio_gpu = false;
		c.allow_virtio_sound = false;
		c.allow_gpu_passthrough = false;
		c.default_disk = QStringLiteral( "ide" );
		c.default_nic = QStringLiteral( "sungem" );
		c.default_sound = QStringLiteral( "none" ); // screamer via additional args
		c.default_video = QString(); // onboard
		c.disk_options << D( "ide", "IDE (macio)" );
		c.nic_options << N( "sungem", "SunGEM (recommended)" )
		              << N( "macio-built-in", "MacIO built-in" );
		c.sound_options << N( "none", "Board default / Screamer" );
		c.video_options << N( "", "Board default (mac99)" );
		break;

	case Guest_Capabilities::Intel_Mac:
		c.summary = QObject::tr(
			"Intel macOS: Q35 + VirtIO-friendly devices. GPU passthrough (AMD) is optional on Linux/KVM for Metal." );
		c.allow_virtio_disk = true;
		c.allow_virtio_net = true;
		c.allow_virtio_gpu = false; // OpenCore/Apple graphics path differs
		c.allow_virtio_sound = true;
		c.allow_virtio_extras = true;
		c.prefer_virtio = true;
		c.allow_gpu_passthrough = true;
		c.default_disk = QStringLiteral( "virtio" );
		c.default_nic = QStringLiteral( "virtio-net-pci" );
		c.default_sound = QStringLiteral( "hda" );
		c.default_video = QStringLiteral( "VGA" );
		c.disk_options << D( "virtio", "VirtIO disk (recommended)" )
		               << D( "ide", "IDE" ) << D( "sata", "AHCI / SATA" );
		c.nic_options << N( "virtio-net-pci", "VirtIO network" )
		              << N( "e1000-82545em", "e1000-82545em (Apple-friendly)" )
		              << N( "e1000", "Intel e1000" );
		c.sound_options << N( "hda", "Intel HDA" ) << N( "none", "No sound" );
		c.video_options << N( "VGA", "VGA" ) << N( "virtio", "VirtIO-GPU" );
		break;

	case Guest_Capabilities::AIX_POWER:
		c.summary = QObject::tr(
			"AIX on pseries: VirtIO-SCSI, TCG on non-POWER hosts. No PC AdLib/cirrus." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_virtio_disk = true;
		c.allow_virtio_net = true;
		c.prefer_virtio = true;
		c.allow_gpu_passthrough = false;
		c.default_disk = QStringLiteral( "virtio-scsi" );
		c.default_nic = QStringLiteral( "virtio-net-pci" );
		c.default_sound = QStringLiteral( "none" );
		c.default_video = QStringLiteral( "std" );
		c.disk_options << D( "virtio-scsi", "VirtIO-SCSI" ) << D( "scsi", "SCSI" );
		c.nic_options << N( "virtio-net-pci", "VirtIO network" ) << N( "e1000", "e1000" );
		c.sound_options << N( "none", "No sound" );
		c.video_options << N( "std", "Standard VGA" );
		break;

	case Guest_Capabilities::Retro_MIPS_SGI:
		c.summary = QObject::tr(
			"SGI / MIPS guest: board-specific devices. Avoid PC ISA sound (AdLib) — it will not exist here." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_virtio_disk = false;
		c.allow_virtio_net = false;
		c.allow_gpu_passthrough = false;
		c.default_disk = QStringLiteral( "scsi" );
		c.default_nic = QStringLiteral( "dp83932" );
		c.default_sound = QStringLiteral( "none" );
		c.default_video = QString();
		c.disk_options << D( "scsi", "SCSI" ) << D( "ide", "IDE (if board supports)" );
		c.nic_options << N( "dp83932", "SONIC / dp83932" ) << N( "e1000", "e1000 (experimental)" );
		c.sound_options << N( "none", "No sound / board default" );
		c.video_options << N( "", "Board default" );
		break;

	case Guest_Capabilities::Modern_Windows:
		c.summary = QObject::tr(
			"Modern Windows: e1000 + IDE/SATA work out of box. VirtIO is faster but needs virtio-win drivers "
			"(or a slipstreamed ISO). GPU passthrough is optional when using KVM/WHPX." );
		c.allow_virtio_disk = true;
		c.allow_virtio_net = true;
		c.allow_virtio_gpu = true;
		c.allow_virtio_sound = true;
		c.allow_virtio_extras = true;
		c.prefer_virtio = false; // safer install path
		c.allow_gpu_passthrough = true;
		if( os == QLatin1String( "Windows 11" ) || os.contains( QLatin1String( "2025" ) ) )
			c.default_disk = QStringLiteral( "nvme" );
		else if( os.contains( QLatin1String( "Vista" ) ) || os.startsWith( QLatin1String( "Windows 7" ) ) ||
		         os.startsWith( QLatin1String( "Windows 8" ) ) || os.startsWith( QLatin1String( "Windows 10" ) ) ||
		         os.contains( QLatin1String( "Server" ) ) )
			c.default_disk = QStringLiteral( "sata" );
		else
			c.default_disk = QStringLiteral( "ide" );

		c.default_nic = QStringLiteral( "e1000" );
		c.default_sound = QStringLiteral( "hda" );
		c.default_video = QStringLiteral( "std" );
		c.disk_options << D( "nvme", "NVMe (Windows 11 / high performance)" )
		               << D( "sata", "AHCI / SATA (standard install)" )
		               << D( "ide", "IDE (maximum compatibility)" )
		               << D( "virtio", "VirtIO disk (needs virtio-win drivers)" );
		c.nic_options << N( "e1000", "Intel e1000 (inbox drivers)" )
		              << N( "virtio-net-pci", "VirtIO network (needs drivers)" )
		              << N( "rtl8139", "RTL8139" );
		c.sound_options << N( "hda", "Intel HDA" )
		                << N( "ac97", "AC97" )
		                << N( "hda_virtio", "HDA + VirtIO sound" )
		                << N( "none", "No sound" );
		c.video_options << N( "std", "Standard VGA" )
		                << N( "qxl", "QXL (SPICE)" )
		                << N( "virtio", "VirtIO-GPU (needs drivers)" );
		break;

	case Guest_Capabilities::Modern_Unix:
	case Guest_Capabilities::Embedded_ARM:
	{
		c.summary = QObject::tr(
			"Modern Linux/BSD/ARM: VirtIO disk/net/GPU preferred. KVM/WHPX when guest matches host. "
			"GPU passthrough available if you want a real NVIDIA/AMD card in the guest." );
		c.allow_virtio_disk = true;
		c.allow_virtio_net = true;
		c.allow_virtio_gpu = true;
		c.allow_virtio_sound = true;
		c.allow_virtio_extras = true;
		c.prefer_virtio = true;
		c.allow_gpu_passthrough = true;
		const bool is_arm_virt =
			tgt == QLatin1String( "aarch64" ) || tgt == QLatin1String( "arm" ) ||
			mach.contains( QLatin1String( "virt" ) );

		if( is_arm_virt )
		{
			c.default_disk = QStringLiteral( "virtio" );
			c.default_nic = QStringLiteral( "virtio-net-pci" );
			c.default_sound = QStringLiteral( "hda" );
			c.default_video = QStringLiteral( "virtio-gpu-pci" );
			c.disk_options << D( "virtio", "VirtIO disk (recommended)" )
			               << D( "nvme", "NVMe disk" )
			               << D( "virtio-scsi", "VirtIO-SCSI" )
			               << D( "scsi", "SCSI" );
			c.nic_options << N( "virtio-net-pci", "VirtIO network (recommended)" )
			              << N( "e1000", "Intel e1000" );
			c.sound_options << N( "hda", "Intel HDA" )
			                << N( "none", "No sound" );
			c.video_options << N( "virtio-gpu-pci", "VirtIO-GPU (KMS modesetting)" )
			                << N( "ramfb", "Simple Framebuffer (ramfb)" )
			                << N( "bochs-display", "Bochs Display" );
		}
		else
		{
			c.default_disk = QStringLiteral( "virtio" );
			c.default_nic = QStringLiteral( "virtio-net-pci" );
			c.default_sound = QStringLiteral( "hda_virtio" );
			c.default_video = QStringLiteral( "virtio" );
			c.disk_options << D( "virtio", "VirtIO disk (recommended)" )
			               << D( "nvme", "NVMe disk" )
			               << D( "virtio-scsi", "VirtIO-SCSI" )
			               << D( "ide", "IDE" ) << D( "sata", "AHCI / SATA" );
			c.nic_options << N( "virtio-net-pci", "VirtIO network (recommended)" )
			              << N( "e1000", "Intel e1000" )
			              << N( "rtl8139", "RTL8139" );
			c.sound_options << N( "hda_virtio", "HDA + VirtIO" )
			                << N( "hda", "Intel HDA" )
			                << N( "virtio", "VirtIO sound" )
			                << N( "none", "No sound" );
			c.video_options << N( "virtio", "VirtIO-GPU" )
#ifdef Q_OS_WIN32
			                << N( "std", "Standard VGA" )
#else
			                << N( "virtio-vga-gl", "VirtIO-GPU + OpenGL" )
			                << N( "std", "Standard VGA" )
#endif
			                << N( "qxl", "QXL (SPICE)" );
		}
		break;
	}

	case Guest_Capabilities::Generic:
	default:
		c.summary = QObject::tr(
			"Generic guest: sensible PC defaults. Enable “Show all QEMU options” if you need exotic devices. "
			"VirtIO offered only when the architecture typically supports it." );
		{
			const bool pcish =
				tgt == QLatin1String( "x86_64" ) || tgt == QLatin1String( "i386" ) ||
				tgt == QLatin1String( "aarch64" ) || tgt == QLatin1String( "arm" ) ||
				tgt.contains( QLatin1String( "riscv" ) );
			const bool can_virtio = pcish && ! mach.contains( QLatin1String( "mac99" ) );
			c.allow_virtio_disk = can_virtio;
			c.allow_virtio_net = can_virtio;
			c.allow_virtio_gpu = can_virtio;
			c.allow_virtio_sound = can_virtio;
			c.allow_virtio_extras = can_virtio;
			c.prefer_virtio = can_virtio && (
				tgt == QLatin1String( "aarch64" ) || tgt.contains( QLatin1String( "riscv" ) ) );
			c.allow_gpu_passthrough = can_virtio;
			if( c.prefer_virtio )
			{
				c.default_disk = QStringLiteral( "virtio" );
				c.default_nic = QStringLiteral( "virtio-net-pci" );
				c.default_video = QStringLiteral( "virtio" );
				c.default_sound = QStringLiteral( "hda_virtio" );
			}
			c.disk_options << D( "ide", "IDE" );
			if( can_virtio )
				c.disk_options << D( "virtio", "VirtIO disk" ) << D( "sata", "AHCI / SATA" );
			c.nic_options << N( "e1000", "Intel e1000" );
			if( can_virtio )
				c.nic_options << N( "virtio-net-pci", "VirtIO network" );
			c.nic_options << N( "rtl8139", "RTL8139" );
			c.sound_options << N( "hda", "Intel HDA" ) << N( "ac97", "AC97" )
			                << N( "sb16", "Sound Blaster 16 (PC guests only)" )
			                << N( "none", "No sound" );
			if( can_virtio )
				add_unique( c.sound_options, N( "hda_virtio", "HDA + VirtIO" ) );
			c.video_options << N( "std", "Standard VGA" ) << N( "cirrus", "Cirrus VGA" );
			if( can_virtio )
				c.video_options << N( "virtio", "VirtIO-GPU" );
		}
		break;
	}

	// Host accelerator rules: Windows host WHPX/KVM is only valid for x86_64 and i386
#ifdef Q_OS_WIN32
	if( tgt != QLatin1String( "x86_64" ) && tgt != QLatin1String( "i386" ) )
	{
		c.allow_kvm_whpx = false;
		c.force_tcg = true;
	}
#endif

	// Platform/arch-only: refine from machine or target when no OS name
	if( tgt == QLatin1String( "sparc" ) || tgt == QLatin1String( "sparc64" ) )
	{
		c.summary = QObject::tr( "Sun SPARCstation: SCSI disk (esp/cmd646), lance/hme network, tcx/cgthree/std video." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_virtio_disk = false;
		c.allow_virtio_net = false;
		c.allow_virtio_gpu = false;
		c.default_disk = QStringLiteral( "scsi" );
		c.default_nic = ( tgt == QLatin1String( "sparc64" ) ) ? QStringLiteral( "hme" ) : QStringLiteral( "lance" );
		c.default_sound = QStringLiteral( "none" );
		c.default_video = ( tgt == QLatin1String( "sparc" ) ) ? QStringLiteral( "tcx" ) : QStringLiteral( "std" );
		c.disk_options.clear(); c.disk_options << D( "scsi", "SCSI" ) << D( "ide", "IDE" );
		c.nic_options.clear(); c.nic_options << N( "lance", "Sun Lance" ) << N( "hme", "Sun Happy Meal (HME)" ) << N( "e1000", "e1000" );
		c.sound_options.clear(); c.sound_options << N( "none", "No sound / onboard" );
		c.video_options.clear(); c.video_options << N( "tcx", "Sun TCX" ) << N( "cgthree", "Sun CGThree" ) << N( "std", "Standard VGA" );
	}
	else if( tgt == QLatin1String( "s390x" ) )
	{
		c.summary = QObject::tr( "IBM S/390 Mainframe: CCW bus devices (virtio-blk-ccw, virtio-net-ccw). No PCI/ISA." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_virtio_disk = true;
		c.allow_virtio_net = true;
		c.allow_virtio_gpu = true;
		c.default_disk = QStringLiteral( "virtio" );
		c.default_nic = QStringLiteral( "virtio-net-ccw" );
		c.default_sound = QStringLiteral( "none" );
		c.default_video = QStringLiteral( "none" );
		c.disk_options.clear(); c.disk_options << D( "virtio", "VirtIO-CCW disk" );
		c.nic_options.clear(); c.nic_options << N( "virtio-net-ccw", "VirtIO-CCW Network" );
		c.sound_options.clear(); c.sound_options << N( "none", "No sound" );
		c.video_options.clear(); c.video_options << N( "none", "Headless / Console" ) << N( "virtio", "VirtIO-GPU-CCW" );
	}
	else if( tgt == QLatin1String( "alpha" ) || tgt == QLatin1String( "hppa" ) )
	{
		c.summary = QObject::tr( "DEC Alpha / HPPA Workstation: SCSI controller (lsi53c895a), tulip/e1000 network." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_virtio_disk = false;
		c.default_disk = QStringLiteral( "scsi" );
		c.default_nic = QStringLiteral( "tulip" );
		c.default_sound = QStringLiteral( "none" );
		c.default_video = ( tgt == QLatin1String( "hppa" ) ) ? QStringLiteral( "artist" ) : QStringLiteral( "std" );
		c.disk_options.clear(); c.disk_options << D( "scsi", "SCSI (lsi53c895a)" ) << D( "ide", "IDE" );
		c.nic_options.clear(); c.nic_options << N( "tulip", "DEC Tulip" ) << N( "e1000", "e1000" );
		c.sound_options.clear(); c.sound_options << N( "none", "No sound" );
		c.video_options.clear(); c.video_options << N( "std", "Standard VGA" ) << N( "artist", "HP Artist Framebuffer" );
	}
	else if( tgt == QLatin1String( "avr" ) || tgt == QLatin1String( "rx" ) || tgt == QLatin1String( "tricore" ) )
	{
		c.summary = QObject::tr( "Microcontroller target: No external disk, network, or display controllers." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_virtio_disk = false;
		c.allow_virtio_net = false;
		c.allow_virtio_gpu = false;
		c.default_disk = QStringLiteral( "none" );
		c.default_nic = QStringLiteral( "none" );
		c.default_sound = QStringLiteral( "none" );
		c.default_video = QStringLiteral( "none" );
		c.disk_options.clear(); c.disk_options << D( "none", "None / Flash onboard" );
		c.nic_options.clear(); c.nic_options << N( "none", "No network" );
		c.sound_options.clear(); c.sound_options << N( "none", "No sound" );
		c.video_options.clear(); c.video_options << N( "none", "No display" );
	}
	else if( tgt.startsWith( QLatin1String( "mips" ) ) )
	{
		c.summary = QObject::tr( "MIPS platform (Malta / SGI Magnum): IDE/SCSI disk, pcnet/e1000/dp83932 NIC, std/cirrus VGA." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_virtio_disk = false;
		c.allow_virtio_net = false;
		c.allow_virtio_gpu = false;
		c.default_disk = QStringLiteral( "ide" );
		c.default_nic = QStringLiteral( "pcnet" );
		c.default_sound = QStringLiteral( "none" );
		c.default_video = QStringLiteral( "std" );
		c.disk_options.clear(); c.disk_options << D( "ide", "IDE" ) << D( "scsi", "SCSI" );
		c.nic_options.clear(); c.nic_options << N( "pcnet", "AMD PCNet" ) << N( "e1000", "e1000" ) << N( "dp83932", "SONIC dp83932" );
		c.sound_options.clear(); c.sound_options << N( "none", "No sound" );
		c.video_options.clear(); c.video_options << N( "std", "Standard VGA" ) << N( "cirrus", "Cirrus VGA" );
	}
	else if( tgt == QLatin1String( "m68k" ) )
	{
		c.summary = QObject::tr( "Motorola 68k (q800 / NeXTcube / ColdFire): SCSI/IDE disk, mcf_fec/dp83932 NIC, onboard video." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_virtio_disk = false;
		c.allow_virtio_net = false;
		c.allow_virtio_gpu = false;
		c.default_disk = QStringLiteral( "scsi" );
		c.default_nic = QStringLiteral( "mcf_fec" );
		c.default_sound = QStringLiteral( "none" );
		c.default_video = QString();
		c.disk_options.clear(); c.disk_options << D( "scsi", "SCSI" ) << D( "ide", "IDE" );
		c.nic_options.clear(); c.nic_options << N( "mcf_fec", "ColdFire FEC" ) << N( "dp83932", "SONIC dp83932" );
		c.sound_options.clear(); c.sound_options << N( "none", "No sound / onboard" );
		c.video_options.clear(); c.video_options << N( "", "Board default" );
	}
	else if( tgt == QLatin1String( "loongarch64" ) )
	{
		c.summary = QObject::tr( "LoongArch 64-bit virt: VirtIO disk/net/GPU preferred." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_virtio_disk = true;
		c.allow_virtio_net = true;
		c.allow_virtio_gpu = true;
		c.default_disk = QStringLiteral( "virtio" );
		c.default_nic = QStringLiteral( "virtio-net-pci" );
		c.default_sound = QStringLiteral( "none" );
		c.default_video = QStringLiteral( "virtio" );
		c.disk_options.clear(); c.disk_options << D( "virtio", "VirtIO disk" ) << D( "scsi", "SCSI" );
		c.nic_options.clear(); c.nic_options << N( "virtio-net-pci", "VirtIO network" ) << N( "e1000", "e1000" );
		c.sound_options.clear(); c.sound_options << N( "none", "No sound" );
		c.video_options.clear(); c.video_options << N( "virtio", "VirtIO-GPU" ) << N( "std", "Standard VGA" );
	}
	else if( tgt.startsWith( QLatin1String( "microblaze" ) ) || tgt == QLatin1String( "or1k" ) || tgt.startsWith( QLatin1String( "xtensa" ) ) )
	{
		c.summary = QObject::tr( "Embedded soft-core target: VirtIO or onboard ethernet (xilinx_ethlite/open_eth)." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_virtio_disk = true;
		c.allow_virtio_net = true;
		c.allow_virtio_gpu = false;
		c.default_disk = QStringLiteral( "virtio" );
		c.default_nic = ( tgt.startsWith( QLatin1String( "microblaze" ) ) ) ? QStringLiteral( "xilinx_ethlite" ) : QStringLiteral( "open_eth" );
		c.default_sound = QStringLiteral( "none" );
		c.default_video = QStringLiteral( "none" );
		c.disk_options.clear(); c.disk_options << D( "virtio", "VirtIO device" );
		c.nic_options.clear(); c.nic_options << N( "xilinx_ethlite", "Xilinx EthLite" ) << N( "open_eth", "OpenEthernet" ) << N( "none", "No network" );
		c.sound_options.clear(); c.sound_options << N( "none", "No sound" );
		c.video_options.clear(); c.video_options << N( "none", "Headless / Console" );
	}
	else if( tgt.startsWith( QLatin1String( "sh4" ) ) )
	{
		c.summary = QObject::tr( "Hitachi SuperH 4 (r2d): IDE disk, Realtek RTL8139 network, SM501 / std video." );
		c.force_tcg = true;
		c.allow_kvm_whpx = false;
		c.allow_virtio_disk = false;
		c.default_disk = QStringLiteral( "ide" );
		c.default_nic = QStringLiteral( "rtl8139" );
		c.default_sound = QStringLiteral( "none" );
		c.default_video = QStringLiteral( "std" );
		c.disk_options.clear(); c.disk_options << D( "ide", "IDE" );
		c.nic_options.clear(); c.nic_options << N( "rtl8139", "Realtek RTL8139" );
		c.sound_options.clear(); c.sound_options << N( "none", "No sound" );
		c.video_options.clear(); c.video_options << N( "std", "Standard VGA" ) << N( "sm501", "Silicon Motion SM501" );
	}
	else if( os.isEmpty() )
	{
		if( mach.contains( QLatin1String( "mac99" ) ) )
		{
			c.guest_class = Guest_Capabilities::Classic_Mac;
			c.summary = QObject::tr( "mac99 platform: classic Mac devices only (no PC ISA sound)." );
		}
		else if( mach.contains( QLatin1String( "raspi" ) ) || mach == QLatin1String( "virt" ) )
		{
			if( c.guest_class == Guest_Capabilities::Generic )
				c.guest_class = Guest_Capabilities::Embedded_ARM;
		}
	}

	return c;
}

QString Guest_Capabilities::Resolve_Optimal_Device(
	const QStringList &priority_candidates,
	const QStringList &probed_available,
	const QString &fallback )
{
	if( probed_available.isEmpty() )
	{
		return priority_candidates.isEmpty() ? fallback : priority_candidates.first();
	}

	// Tier 1: Case-insensitive Exact Match in candidate priority order
	for( const QString &cand : priority_candidates )
	{
		const QString c = cand.trimmed();
		if( c.isEmpty() )
			continue;

		for( const QString &probed : probed_available )
		{
			if( probed.compare( c, Qt::CaseInsensitive ) == 0 )
				return probed;
		}
	}

	// Tier 2: Prefix / Version Alias Match
	// e.g. "pc-q35" -> "pc-q35-11.0", "pc-i440fx" -> "pc-i440fx-11.0", "virtio-net" -> "virtio-net-pci"
	for( const QString &cand : priority_candidates )
	{
		const QString c = cand.trimmed().toLower();
		if( c.isEmpty() )
			continue;

		QString best_match;
		double best_ver = -1.0;

		for( const QString &probed : probed_available )
		{
			const QString p = probed.trimmed().toLower();
			if( p.isEmpty() )
				continue;

			bool matches = false;
			if( p.startsWith( c + QLatin1Char( '-' ) ) || p.startsWith( c + QLatin1Char( '_' ) ) )
				matches = true;
			else if( c.startsWith( p + QLatin1Char( '-' ) ) || c.startsWith( p + QLatin1Char( '_' ) ) )
				matches = true;
			else if( ( c == QLatin1String( "q35" ) || c == QLatin1String( "pc-q35" ) ) && p.contains( QLatin1String( "q35" ) ) )
				matches = true;
			else if( ( c == QLatin1String( "pc" ) || c == QLatin1String( "i440fx" ) || c == QLatin1String( "pc-i440fx" ) ) &&
			         ( p.contains( QLatin1String( "i440fx" ) ) || p == QLatin1String( "pc" ) ) )
				matches = true;
			else if( c == QLatin1String( "virtio" ) && p.contains( QLatin1String( "virtio" ) ) )
				matches = true;
			else if( ( c == QLatin1String( "ahci" ) || c == QLatin1String( "sata" ) ) &&
			         ( p == QLatin1String( "ahci" ) || p == QLatin1String( "sata" ) ) )
				matches = true;
			else if( ( c == QLatin1String( "hda" ) || c == QLatin1String( "intel-hda" ) ) &&
			         ( p == QLatin1String( "hda" ) || p == QLatin1String( "intel-hda" ) ) )
				matches = true;

			if( matches )
			{
				int dash = p.lastIndexOf( QLatin1Char( '-' ) );
				double ver = 0.0;
				if( dash >= 0 )
				{
					bool ok = false;
					double parsed = p.mid( dash + 1 ).toDouble( &ok );
					if( ok ) ver = parsed;
				}
				if( ver > best_ver )
				{
					best_ver = ver;
					best_match = probed;
				}
				else if( best_match.isEmpty() )
				{
					best_match = probed;
				}
			}
		}

		if( ! best_match.isEmpty() )
			return best_match;
	}

	// Tier 3: Sanitized Fallback
	// If fallback is provided and in probed_available, return it
	if( ! fallback.isEmpty() )
	{
		for( const QString &probed : probed_available )
		{
			if( probed.compare( fallback, Qt::CaseInsensitive ) == 0 )
				return probed;
		}
	}

	// Avoid unbootable or non-general host boards (e.g. none, microvm)
	const QStringList blacklist = {
		QStringLiteral( "none" ),
		QStringLiteral( "microvm" ),
		QStringLiteral( "null" ),
		QStringLiteral( "isapc" ),
		QStringLiteral( "xenpv" ),
		QStringLiteral( "xenfv" )
	};

	for( const QString &probed : probed_available )
	{
		const QString p = probed.trimmed().toLower();
		if( p.isEmpty() )
			continue;

		bool blacklisted = false;
		for( const QString &b : blacklist )
		{
			if( p == b || p.startsWith( b + QLatin1Char( '-' ) ) )
			{
				blacklisted = true;
				break;
			}
		}
		if( ! blacklisted )
			return probed;
	}

	return fallback.isEmpty() ? probed_available.first() : fallback;
}
