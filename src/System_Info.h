/****************************************************************************
**
** Copyright (C) 2008-2010 Andrey Rijov <ANDron142@yandex.ru>
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

#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include "Utils.h"
#include "VM_Devices.h"

/** Host PCI display / HDMI-audio GPU info for AMD Metal / VFIO UI. */
class Host_GPU
{
	public:
		Host_GPU();

		QString Name;
		QString Vendor;       // "AMD", "NVIDIA", "Intel", "Other"
		QString PCI_Address;  // BB:DD.F (empty on Windows when unknown)
		QString Vendor_ID;    // e.g. "1002"
		QString Device_ID;
		bool Is_Display;
		bool Is_Audio;
		QString IOMMU_Group;
		QString Driver;
};

class System_Info
{
	public:
		System_Info();
		
		static bool Update_VM_Computers_List();
		static bool Auto_Find_And_Save_Emulators();
		
		static VM::Emulator_Version Get_Emulator_Version( const QString &path = "" );
		
		static QMap<QString, QString> Find_QEMU_Binary_Files( const QString &path );
		//static QMap<QString, QString> Find_KVM_Binary_Files( const QString &path );
		static QString Find_IMG( const QStringList &paths );
		
		static Available_Devices Get_Emulator_Info( const QString &path, bool *ok,
												   VM::Emulator_Version version, const QString &internalName );
		static void Normalize_Virt_Arch_Devices( Available_Devices &dev );
		static void Filter_Video_Card_List( Available_Devices &dev );
		static QString Sanitize_Video_Card( const QString &computer_type, const QString &video_card,
		                                    const QString &machine_type = QString() );
		static QString Default_Video_Card( const QString &computer_type );
		static bool Uses_Device_Based_Video( const QString &computer_type );
		static QString Get_Emulator_Help_Output( const QString &path );
		static QString Get_Emulator_Output( const QString &path, const QStringList &args );
		
		static const QList<VM_USB> &Get_All_Host_USB();
		static const QList<VM_USB> &Get_Used_USB_List();
		static bool Add_To_Used_USB_List( const VM_USB &device );
		static bool Delete_From_Used_USB_List( const VM_USB &device );
		static bool Update_Host_USB();

		static bool Update_Host_GPU();
		static const QList<Host_GPU> &Get_Host_GPU_List();
		static bool Has_AMD_Display_GPU();
		/** Native Linux with PCI (not Windows, not WSL). Required for vfio-pci into QEMU. */
		static bool Host_Supports_PCI_Passthrough();
		static bool Host_Is_WSL();
		/** Suggest AMD HDMI audio BDF for a display GPU (same slot .1 or IOMMU group). */
		static QString Suggest_AMD_Audio_For( const QString &display_pci_address );
		
		static void Get_Free_Memory_Size( int &allRAM, int &freeRAM );
		
		static QStringList Get_Host_FDD_List();
		static QStringList Get_Host_CDROM_List();
		
        //removed legacy support for all QEMU versions before 2.0
		static QMap<QString, Available_Devices> Emulator_QEMU_2_0;

	private:
		#ifdef Q_OS_LINUX
		static bool Scan_USB_Sys( QList<VM_USB> &list );
		static bool Scan_USB_Proc( QList<VM_USB> &list );
		static bool Read_SysFS_File( const QString &path, QString &data );
		static bool Scan_Host_GPU_Sysfs( QList<Host_GPU> &list );
		#endif
		#ifdef Q_OS_WIN32
		static bool Scan_Host_GPU_Windows( QList<Host_GPU> &list );
		#endif
		static QString Vendor_Name_From_ID( const QString &vendor_id );
		
		static QList<VM_USB> All_Host_USB;
		static QList<VM_USB> Used_Host_USB;
		static QList<Host_GPU> All_Host_GPU;
};

#endif
