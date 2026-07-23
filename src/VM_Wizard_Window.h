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
#include <QCheckBox>
#include <QLineEdit>
#include <QToolButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QTreeWidget>
#include <QListWidget>
#include <QJsonObject>

#include "VM.h"
#include "ui_VM_Wizard_Window.h"
#include "Create_HDD_Image_Window.h"

class VM_Wizard_Window: public QDialog
{
	Q_OBJECT
	
	public:
		VM_Wizard_Window( QWidget *parent = 0 );
		void Set_VM_List( QList<Virtual_Machine*> *list );
		
		Virtual_Machine *New_VM;
		
	private slots:
        void KVM_toggled(bool toggled);
		bool Load_OS_Templates();
        bool Create_New_VM(bool simulate = false);
		QString Find_OS_Icon( const QString os_name );
		
		void on_Button_Back_clicked();
		void on_Button_Next_clicked();
		
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
		
	private:
        void applyTemplate();
        void By_Year();
        void Typical_Or_Custom();
		bool Is_Windows11_ARM_Template() const;
		void Apply_Windows11_ARM_Profile( bool simulate );
		void Apply_AArch64_Generic_Profile( bool simulate );
		void Build_Windows11_ARM_Page();
		bool Is_Intel_MacOS_Template() const;
		void Apply_Intel_MacOS_Profile( bool simulate );
		void Build_Intel_MacOS_Page();
		void Show_Intel_MacOS_Page();
		void Probe_WSL_For_Intel_Mac_Page();
		void Update_Finish_Page_Guidance();

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
		QRadioButton *RB_Method_Guest_OS;
		QRadioButton *RB_Method_Platform;
		QRadioButton *RB_Method_Architecture;
		QRadioButton *RB_Method_Custom;
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
		VM::Sound_Cards Guest_Sound;
		QString Guest_Compat_Tip;
		QLabel *Label_Guest_Compat_Tip;
		QLabel *Label_Arch_Summary;
		bool Guest_Suggest_Win2K_Hack;

		void Apply_Sound_Preset( const QString &preset );
		void Update_Architecture_Page_Chrome();
		void Update_Guest_Compat_Tip();
		void Apply_Guest_Hardware_To_New_VM();
};

#endif
