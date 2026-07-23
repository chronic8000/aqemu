/****************************************************************************
**
** Copyright (C) 2008-2010 Andrey Rijov <ANDron142@yandex.ru>
** Copyright (C) 2016      Tobias Gläßer
**
** This file is part of AQEMU.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor,
** Boston, MA  02110-1301, USA.
**
****************************************************************************/

#ifndef UTILS_H
#define UTILS_H

#define CURRENT_AQEMU_VERSION "0.9.8"
#define CURRENT_AQEMU_RELEASE_DATE "2026-07-17"

#include <functional>
#include <QString>
#include <QList>

#include "VM_Devices.h"

class Disable_User_Graphic_Warning
{
    public:
        Disable_User_Graphic_Warning();
        ~Disable_User_Graphic_Warning();
};

void AQDebug( const QString &sender, const QString &mes );
void AQWarning( const QString &sender, const QString &mes );
void AQError( const QString &sender, const QString &mes );

void AQGraphic_Warning( const QString &caption, const QString &mes );
void AQGraphic_Warning( const QString &sender, const QString &caption, const QString &mes, bool fatal = false );
void AQGraphic_Error( const QString &sender, const QString &caption, const QString &mes, bool fatal = false );

void AQUse_Log( bool use );
void AQUse_Debug_Output( bool use, bool d, bool w, bool e );
void AQLog_Path( const QString &path );
void AQSave_To_Log( const QString &mes_type, const QString &sender, const QString &mes );

bool Create_New_HDD_Image( bool encrypted, const QString &base_image,
						   const QString &file_name, const QString &format, VM::Device_Size size, bool verbose );
bool Create_New_HDD_Image( const QString &file_name, VM::Device_Size size );
bool Format_HDD_Image( const QString &file_name, VM::Disk_Info info );
QString Get_QEMU_IMG_Path();

QList<QString> Get_Templates_List();

QString Get_FS_Compatible_VM_Name( const QString &name );
QString Get_Complete_VM_File_Path( const QString &vm_name );

QString Get_TR_Size_Suffix( VM::Device_Size suf );

QString Get_Last_Dir_Path( const QString &path );

/** Trim and strip a single layer of surrounding "..." or '...' from a file path. */
QString AQ_Normalize_File_Path( const QString &path );

/**
 * QEMU -drive file= key for a host path. Escapes commas and uses
 * file.driver=file,file.filename=… when the path has spaces or special chars.
 */
QString AQ_Qemu_Drive_File_Key( const QString &path );

/** True if s looks like a real AppleSMC OSK (not network/Proxmox junk). */
bool AQ_Is_Plausible_Apple_SMC_OSK( const QString &osk );

/**
 * True if path looks like an Apple Partition Map disk image (MIST/Pyenb "*.iso"):
 * Driver Descriptor Map "ER" at LBA 0 and Partition Map "PM" at LBA 1.
 * These are NOT ISO9660 — OpenCore only lists them when attached as ide-hd.
 */
bool AQ_Is_Apple_Partition_Map_Image( const QString &path );

/** True if path has an ISO9660 primary volume descriptor (CD001 at 0x8001). */
bool AQ_Is_Iso9660_Image( const QString &path );

/**
 * Byte offset of the first Apple_HFS/Apple_HFSX partition in an APM image, or -1.
 * Used as a fallback when OpenPartitionDxe.efi is unavailable.
 */
qint64 AQ_Apple_HFS_Partition_Offset( const QString &path );

/**
 * LongQT OpenCore ISOs enable OpenPartitionDxe in config.plist but omit the .efi.
 * Builds dest_boot_img (FAT) from the ISO's BOOT.img and injects OpenPartitionDxe.efi.
 * Returns dest path on success, or empty QString on failure.
 */
QString AQ_Ensure_OpenCore_Boot_With_PartitionDxe( const QString &opencore_iso,
                                                   const QString &dest_boot_img );

bool It_Host_Device( const QString &path );

void Check_AQEMU_Permissions();

VM::Emulator_Version String_To_Emulator_Version( const QString &str );
QString Emulator_Version_To_String( VM::Emulator_Version ver );

bool Update_Emulators_List();
const QList<Emulator> &Get_Emulators_List();
bool Remove_All_Emulators_Files();
const Emulator &Get_Default_Emulator( );
const Emulator &Get_Emulator_By_Name( const QString &name );

int Get_Random( int min, int max );

void Load_Recent_Images_List();
const QStringList &Get_CD_Recent_Images_List();
void Add_To_Recent_CD_Files( const QString &path );
const QStringList &Get_FDD_Recent_Images_List();
void Add_To_Recent_FDD_Files( const QString &path );

bool Get_Show_Error_Window();
void Set_Show_Error_Window( bool show );

class QWidget;
class QCheckBox;
class QWidget;

void Checkbox_Dependend_Set_Enabled(QList<QWidget*>& children_to_enable, QCheckBox* checkbox, bool enabled);

double calculateContrast(const QColor& col1, const QColor& col2);

/** Show a modal busy dialog (no cancel) while work() runs on the GUI thread. */
void AQ_Run_With_Busy_Dialog( QWidget *parent, const QString &message,
                              const std::function<void()> &work );

// Find UEFI firmware CODE (default aarch64 AAVMF/EDK2; pass "x86_64" for OVMF)
QString Find_UEFI_Firmware_CODE( const QString &qemu_binary_path = QString(),
                                 const QString &arch = QStringLiteral( "aarch64" ) );
// Template VARS file (read-only source to copy for a new VM)
QString Find_UEFI_Firmware_VARS_Template( const QString &qemu_binary_path = QString(),
                                          const QString &arch = QStringLiteral( "aarch64" ) );
// Copy VARS template into dest_path; returns true on success
bool Prepare_UEFI_VARS_File( const QString &dest_path, const QString &qemu_binary_path = QString(),
                             const QString &arch = QStringLiteral( "aarch64" ) );

// Windows 11 ARM helpers (BVM-style UCPD.sys removal + OOBE skip recipes)
QString Win11_OOBE_Bypass_Guest_Commands();
QString Win11_UCPD_Guest_Commands();
// Offline removal from qcow2/raw (Linux: qemu-nbd + NTFS). On Windows returns false with guidance.
bool Remove_UCPD_From_Disk_Image( const QString &disk_path, QString *result_message );

/** Normalize CPU arch strings (x86_64, aarch64, i386, …). */
QString AQ_Normalize_CPU_Architecture( const QString &raw );
/** Cached host CPU architecture (detected once via QSysInfo / uname). */
QString AQ_Get_Host_CPU_Architecture();
/** True when guest QEMU target can use host native accel (KVM/WHPX/HVF). */
bool AQ_Guest_Matches_Host_Architecture( const QString &guest_target );
/**
 * Prefer KVM (maps to WHPX on Windows) when guest matches host.
 * Forces TCG for cross-arch guests and for i386 guests on Windows.
 */
bool AQ_Should_Prefer_Native_Accelerator( const QString &guest_target );

#endif

