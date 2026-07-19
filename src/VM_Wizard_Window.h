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
		
		void on_RB_Win11_New_Disk_toggled( bool on );
		void on_CH_Win11_Already_Installed_toggled( bool on );
		void on_TB_Win11_ISO_Browse_clicked();
		void on_TB_Win11_Existing_Disk_Browse_clicked();
		void on_TB_Win11_VirtIO_ISO_Browse_clicked();
		void on_CH_Win11_VirtIO_ISO_toggled( bool on );
		
	private:
        void applyTemplate();
        void By_Year();
        void Typical_Or_Custom();
		bool Is_Windows11_ARM_Template() const;
		void Apply_Windows11_ARM_Profile( bool simulate );
		void Apply_AArch64_Generic_Profile( bool simulate );
		void Build_Windows11_ARM_Page();
		void Update_Finish_Page_Guidance();

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
};

#endif
