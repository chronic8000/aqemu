#ifndef APPLE_SOC_SUPPORT_H
#define APPLE_SOC_SUPPORT_H

#include <QString>
#include <QStringList>

class Virtual_Machine;

/** True when this VM should use the Inferno Apple SoC launch profile. */
bool AQ_Is_Apple_SoC_VM( const Virtual_Machine *vm );

/** Default research-kernel boot args used by Inferno t8030 restore/boot recipes. */
QString AQ_Apple_SoC_Default_Append();

/**
 * Per-VM directory for Inferno raw images (sep_nvram, root, …).
 * Never the shared VM_Directory root — that would collide across guests.
 */
QString AQ_Apple_SoC_Image_Dir( const Virtual_Machine *vm );

/**
 * Ensure the standard raw image set exists under image_dir.
 * Creates/grows missing or undersized files. Returns false on I/O / disk-space failure.
 */
bool AQ_Ensure_Apple_SoC_Disk_Images( const QString &image_dir, QString *error_out = nullptr );

/**
 * Build Inferno-specific argv pieces that AQEMU would not emit from generic UI:
 * SEP pflash, multi-ns NVMe layout. Does not create missing images unless
 * create_missing_images is true (real Start only — never for args preview/script).
 * On create failure returns an empty list and sets error_out.
 */
QStringList AQ_Build_Apple_SoC_Extra_Args( const Virtual_Machine *vm,
                                           bool for_script_mode,
                                           bool via_wsl,
                                           bool create_missing_images = false,
                                           QString *error_out = nullptr );

/** Comma-joined Inferno -machine value (t8030,trustcache=…,usb-conn-type=…). */
QString AQ_Build_Apple_SoC_Machine_Props( const Virtual_Machine *vm, bool via_wsl );

/** Linux Inferno binary path / command name inside WSL. */
QString AQ_Apple_SoC_WSL_Qemu_Binary();

/**
 * Drop -machine / -append from Additional Args for Apple SoC VMs — those are owned by
 * the Inferno profile builders (legacy recipes often duplicated them).
 */
QStringList AQ_Filter_Apple_SoC_Additional_Args( const QStringList &args );

#endif
