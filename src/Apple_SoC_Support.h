#ifndef APPLE_SOC_SUPPORT_H
#define APPLE_SOC_SUPPORT_H

#include <QString>
#include <QStringList>

class Virtual_Machine;
class QWidget;
class QComboBox;
class QSpinBox;

/** True when this VM should use the Inferno Apple SoC launch profile. */
bool AQ_Is_Apple_SoC_VM( const Virtual_Machine *vm );

/**
 * Suggested research-kernel boot args for new Inferno VMs (wizard prefill only).
 * Never injected at launch — Kernel command line must come from the MACHINE tab.
 */
QString AQ_Apple_SoC_Suggested_Append();

/**
 * Per-VM directory for Inferno raw images (sep_nvram, root, …).
 * Never the shared VM_Directory root — that would collide across guests.
 */
QString AQ_Apple_SoC_Image_Dir( const Virtual_Machine *vm );

/** Per-boot Inferno QEMU stderr/stdout log under the image dir (qemu-boot.log). */
QString AQ_Apple_SoC_QEMU_Log_Path( const Virtual_Machine *vm );

/**
 * Validate MACHINE-tab Inferno paths before Start. On failure sets error_out.
 * Does not invent or substitute missing files.
 */
bool AQ_Validate_Apple_SoC_Boot_Files( const Virtual_Machine *vm, QString *error_out = nullptr );

/** Default NAND size (GiB). Presets: 16, 32, 64, 128, 256, plus Custom (16–2048). */
int AQ_Default_Apple_SoC_Nand_GiB();
int AQ_Min_Apple_SoC_Nand_GiB();
int AQ_Max_Apple_SoC_Nand_GiB();
int AQ_Clamp_Apple_SoC_Nand_GiB( int gib );
qint64 AQ_Apple_SoC_Nand_Bytes( int gib );
void AQ_Setup_Apple_SoC_Nand_Spin( QSpinBox *spin );
void AQ_Populate_Apple_SoC_Nand_Combo( QComboBox *cb, int current_gib );
void AQ_Apply_Apple_SoC_Nand_Controls( QComboBox *cb, QSpinBox *spin, int gib );
int AQ_Read_Apple_SoC_Nand_Controls( const QComboBox *cb, const QSpinBox *spin );
void AQ_On_Apple_SoC_Nand_Combo_Changed( QComboBox *cb, QSpinBox *spin );
void AQ_On_Apple_SoC_Nand_Spin_Changed( QComboBox *cb, QSpinBox *spin );

/**
 * Ensure the standard raw image set exists under image_dir.
 * `root_bytes` is NAND size used when creating `root` (or growing a seed).
 * A finished restore GPT is never resized — Settings stays at the restore size
 * until Wipe + recreate + restore. Empty `root` gets a GPT APFS seed volume.
 */
bool AQ_Ensure_Apple_SoC_Disk_Images( const QString &image_dir, QString *error_out = nullptr );
bool AQ_Ensure_Apple_SoC_Disk_Images( const QString &image_dir, qint64 root_bytes,
                                      QString *error_out = nullptr );

/**
 * Delete Inferno NVMe raw images for this VM (root, nvram, firmware, …).
 * Image dir is derived from vm->Get_VM_XML_File_Path() (never a hardcoded .aqemu).
 * Does not delete the .aqemu file or companion.qcow2.
 */
bool AQ_Wipe_Apple_SoC_Disk_Images( const Virtual_Machine *vm, QString *error_out = nullptr );

/** True if guest root has a real restore GPT (not the AQEMU_SEED placeholder). */
bool AQ_Apple_SoC_Root_Has_GPT( const Virtual_Machine *vm );

/** Confirm + wipe in the GUI. Refuses while the iOS guest is running. */
void AQ_Prompt_Wipe_Apple_SoC_Disks( Virtual_Machine *vm, QWidget *parent );

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
