/****************************************************************************
**
** Copyright (C) 2008-2010 Andrey Rijov <ANDron142@yandex.ru>
** Copyright (C) 2016 Tobias Gläßer
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

#ifndef VM_WIZARD_WINDOW_H
#define VM_WIZARD_WINDOW_H

#include <QFileInfoList>
#include <QSettings>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QLineEdit>
#include <QToolButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QTreeWidget>
#include <QListWidget>
#include <QComboBox>
#include <QJsonObject>

#include "VM.h"
#include "ui_VM_Wizard_Window.h"
#include "Create_HDD_Image_Window.h"
#include "Guest_Capabilities.h"

class VM_Wizard_Window: public QDialog
{
	Q_OBJECT
	
	public:
		VM_Wizard_Window( QWidget *parent = 0 );
		void Set_VM_List( QList<Virtual_Machine*> *list );
		
		Virtual_Machine *New_VM;
		
	protected:
		bool eventFilter( QObject *watched, QEvent *event ) override;
		
	private slots:
        void KVM_toggled(bool toggled);
		bool Load_OS_Templates();
        bool Create_New_VM(bool simulate = false);
		QString Find_OS_Icon( const QString os_name );
		
		void on_Button_Back_clicked();
		void on_Button_Next_clicked();

	protected slots:
		void on_RB_VM_Template_toggled( bool on );
		void on_RB_Generate_VM_toggled( bool on );
		void on_CB_OS_Type_currentIndexChanged( int index );
		void on_CB_Computer_Type_currentIndexChanged( int index );
		
		// Memory
		void on_Memory_Size_valueChanged( int value );
		void on_CB_RAM_Size_editTextChanged( const QString &text );
		void on_CH_Remove_RAM_Size_Limitation_stateChanged ( int state );
		void on_TB_Update_Available_RAM_Size_clicked();
		void Update_RAM_Size_ComboBox( int freeRAM );
		
		void on_Edit_VM_Name_textEdited( const QString &text );
		
		void on_Button_New_HDD_clicked();
		void on_Button_Existing_clicked();
		
		void Win11_New_Disk_Toggled( bool on );
		void Win11_Already_Installed_Toggled( bool on );
		void Win11_ISO_Browse_Clicked();
		void Win11_Existing_Disk_Browse_Clicked();
		void Win11_VirtIO_ISO_Browse_Clicked();
		void Win11_VirtIO_ISO_Toggled( bool on );

		void Intel_Mac_OpenCore_Browse_Clicked();
		void Intel_Mac_Disk_Browse_Clicked();
		void Intel_Mac_Recovery_Browse_Clicked();
		void Intel_Mac_New_Disk_Toggled( bool on );

		void Typical_New_Disk_Toggled( bool on );
		void Typical_Disk_Browse_Clicked();
		void Refresh_Typical_HDD_Defaults();
		bool Validate_Typical_HDD_Page();
		QString Default_Typical_HDA_Path() const;
		void Install_ISO_Browse_Clicked();
		void Apply_Install_ISO_Guess();
		void Storage_Browser_For_ISO_Clicked();
		void Storage_Browser_For_Disk_Clicked();
		void Install_Source_Mode_Changed();
		void Download_Install_ISO_URL_Clicked();
		void Download_Network_Kernel_Clicked();
		void Download_Network_Initrd_Clicked();
		
	private:
        void applyTemplate();
        void By_Year();
        void Typical_Or_Custom();
		bool Is_Windows11_ARM_Template() const;
		void Apply_Windows11_ARM_Profile( bool simulate );
		void Apply_AArch64_Generic_Profile( bool simulate );
		void Build_Windows11_ARM_Page();
		bool Is_Apple_Silicon_Or_iOS_Template() const;
		bool Is_Intel_MacOS_Template() const;
		void Apply_Intel_MacOS_Profile( bool simulate );
		void Build_Intel_MacOS_Page();
		void Show_Intel_MacOS_Page();
		void Probe_WSL_For_Intel_Mac_Page();
		void Update_Finish_Page_Guidance();
		void Enhance_Typical_HDD_Page();

		void Polish_Wizard_Chrome();
		void Sync_Wizard_Side_Steps();
		QFrame *Add_Method_Card( QVBoxLayout *parent_lay, QRadioButton *rb, const QString &hint );

		void Build_Devices_Page();
		void Refresh_Devices_Page();
		void Apply_Devices_Page_To_State();
		Guest_Capabilities Current_Guest_Capabilities() const;

		// Three-path wizard
		void Build_Three_Path_Pages();
		bool Load_Wizard_Trees();
		void Populate_OS_Tree();
		void Populate_Platform_Tree();
		void Populate_Arch_List();
		void Populate_Arch_Machines( const QString &arch_display );
		bool Ensure_Emulator_Ready();
		bool Apply_Selected_Computer_Type( const QString &target );
		void Apply_Platform_Binding( const QString &platform_display );
		void Apply_OS_Defaults( const QString &os_name );
		QString Selected_Tree_Leaf( QTreeWidget *tree ) const;
		void Goto_Hardware_Flow();
		void Prefer_Accelerator_For_Target( const QString &target );

		enum Creation_Method {
			Method_None = 0,
			Method_Guest_OS,
			Method_Platform,
			Method_Architecture,
			Method_Custom
		};

		QSettings Settings;
		Ui::VM_Wizard_Window ui;
		
		QFileInfoList OS_Templates_List;
		QList<Virtual_Machine*> *VM_List;
		
		Emulator Current_Emulator;
		const Available_Devices* Current_Devices;
		QMap<QString, Available_Devices> All_Systems;
		
		bool Use_Accelerator_Page;
		
		// Windows 11 ARM guided page (created in code)
		QWidget *Win11_ARM_Page;
		QRadioButton *RB_Win11_New_Disk;
		QRadioButton *RB_Win11_Existing_Disk;
		QLineEdit *Edit_Win11_Existing_Disk;
		QToolButton *TB_Win11_Existing_Disk_Browse;
		QCheckBox *CH_Win11_Already_Installed;
		QLineEdit *Edit_Win11_ISO;
		QToolButton *TB_Win11_ISO_Browse;
		QCheckBox *CH_Win11_VirtIO_ISO;
		QLineEdit *Edit_Win11_VirtIO_ISO;
		QToolButton *TB_Win11_VirtIO_ISO_Browse;
		QLabel *Label_Win11_UEFI_Status;
		QLabel *Label_Win11_Finish_Help;

		// Typical (quick) HDD page — create new vs use existing + path
		QButtonGroup *Group_Typical_Disk_Mode;
		QRadioButton *RB_Typical_New_Disk;
		QRadioButton *RB_Typical_Existing_Disk;
		QLineEdit *Edit_Typical_Disk_Path;
		QToolButton *TB_Typical_Disk_Browse;
		QWidget *Widget_Typical_Size_Row;
		QLineEdit *Edit_Install_ISO;
		QToolButton *TB_Install_ISO_Browse;
		QToolButton *TB_Install_ISO_Storage;
		QLabel *Label_Install_ISO_Guess;
		QString Guest_Install_ISO;

		// URL / network install (virt-manager-style)
		QButtonGroup *Group_Typical_Install_Media;
		QRadioButton *RB_Install_Local;
		QRadioButton *RB_Install_URL_ISO;
		QRadioButton *RB_Install_Network_Kernel;
		QWidget *Widget_Install_Local_Row;
		QWidget *Widget_Install_URL_Row;
		QWidget *Widget_Install_Kernel_Row;
		QLineEdit *Edit_Install_ISO_URL;
		QToolButton *TB_Download_ISO_URL;
		QLineEdit *Edit_Kernel_URL;
		QLineEdit *Edit_Initrd_URL;
		QLineEdit *Edit_Kernel_Append;
		QLineEdit *Edit_Kernel_Local;
		QLineEdit *Edit_Initrd_Local;
		QToolButton *TB_Download_Kernel;
		QToolButton *TB_Download_Initrd;
		QString Guest_Kernel_Path;
		QString Guest_Initrd_Path;
		QString Guest_Kernel_Append;

		// Intel macOS guided page (created in code)
		QWidget *Intel_MacOS_Page;
		QRadioButton *RB_Intel_Mac_New_Disk;
		QRadioButton *RB_Intel_Mac_Existing_Disk;
		QLineEdit *Edit_Intel_Mac_Existing_Disk;
		QToolButton *TB_Intel_Mac_Disk_Browse;
		QLineEdit *Edit_Intel_Mac_OpenCore;
		QToolButton *TB_Intel_Mac_OpenCore_Browse;
		QLineEdit *Edit_Intel_Mac_Recovery;
		QToolButton *TB_Intel_Mac_Recovery_Browse;
		QLineEdit *Edit_Intel_Mac_OSK;
		QCheckBox *CH_Intel_Mac_Supply_Files;
		QCheckBox *CH_Intel_Mac_Prefer_WSL;
		QLabel *Label_Intel_Mac_UEFI_Status;

		// Three-path pages
		QWidget *Creation_Method_Page;
		QWidget *OS_Tree_Page;
		QWidget *Platform_Tree_Page;
		QWidget *Arch_List_Page;
		QWidget *Arch_Machines_Page;
		QButtonGroup *Group_Creation_Method;
		QRadioButton *RB_Method_Guest_OS;
		QRadioButton *RB_Method_Platform;
		QRadioButton *RB_Method_Architecture;
		QRadioButton *RB_Method_Custom;
		QRadioButton *RB_Method_Import;
		QTreeWidget *Tree_OS;
		QTreeWidget *Tree_Platform;
		QListWidget *List_Arch;
		QTreeWidget *Tree_Arch_Machines;
		QJsonObject Wizard_Trees;
		Creation_Method Current_Method;
		bool Three_Path_Active;
		QString Selected_OS_Name;
		QString Selected_Platform_Name;
		QString Selected_Arch_Name;
		QString Selected_Target;
		QString Selected_Machine_Id;
		int Guest_RAM_MB;
		double Guest_HDD_GB;
		QString Guest_NIC_Model;
		QString Guest_CPU_Type;
		VM::Sound_Cards Guest_Sound;
		QString Guest_Compat_Tip;
		QString Guest_Disk_Bus;
		QString Guest_Video_Card;
		bool Guest_Use_VirtIO_Extras;
		bool Guest_Use_GPU_Passthrough;
		QString Guest_GPU_PCI;

		QLabel *Label_Guest_Compat_Tip;
		QLabel *Label_Arch_Summary;
		bool Guest_Suggest_Win2K_Hack;

		// Guest-aware devices page
		QWidget *Devices_Page;
		QLabel *Label_Devices_Summary;
		QComboBox *CB_Dev_Disk;
		QComboBox *CB_Dev_NIC;
		QComboBox *CB_Dev_Sound;
		QComboBox *CB_Dev_Video;
		QLineEdit *Edit_Dev_BIOS_File;
		QCheckBox *CH_Dev_VirtIO_Extras;
		QCheckBox *CH_Dev_GPU_Passthrough;
		QComboBox *CB_Dev_GPU;
		QCheckBox *CH_Dev_Show_All;
		QLabel *Label_Dev_GPU;

		void Apply_Sound_Preset( const QString &preset );
		void Update_Architecture_Page_Chrome();
		void Update_Guest_Compat_Tip();
		void Apply_Guest_Hardware_To_New_VM();
		/** Probe-validated default CPU for the selected OS / target (never blind list[0]). */
		QString Recommended_CPU_Type() const;
		void Select_Recommended_CPU_In_Combo();

		void Ensure_Machine_Catalog();
		void Append_Catalog_Machines( QTreeWidgetItem *parent, const QString &target );
		QStringList Probe_Live_Machines( const QString &target );
		QString Find_Emulator_Binary_For_Target( const QString &target ) const;
		void Refresh_Wizard_Machine_Combo();
		void Sync_Selected_Machine_From_Combo();

		QLabel *Label_Wizard_Machine;
		QComboBox *CB_Wizard_Machine;
		QJsonObject Machine_Catalog;
		bool Machine_Catalog_Loaded;
		QListWidget *List_Wizard_Steps;
};

#endif
