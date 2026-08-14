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

#ifndef ADVANCED_SETTINGS_WINDOW_H
#define ADVANCED_SETTINGS_WINDOW_H

#include <QSettings>
#include "VM_Devices.h"
#include "ui_Advanced_Settings_Window.h"

class Settings_Widget;
class QCheckBox;
class QLineEdit;
class QLabel;
class QToolButton;
class QRadioButton;
class QGroupBox;

class Advanced_Settings_Window: public QDialog
{
	Q_OBJECT
	
	public:
		Advanced_Settings_Window( QWidget *parent = 0 );
        ~Advanced_Settings_Window();
	
	public slots:
		void done(int);

	private slots:
        /// Old Setttings Window
		void on_Button_Create_Template_from_VM_clicked();
		void on_TB_VM_Folder_clicked();
		void CB_Language_currentIndexChanged( int index );
		void CB_Icons_Theme_currentIndexChanged( int index );
		void VNC_Warning( bool state );
		void Load_Templates();
        ///
		
		void on_TB_Browse_Before_clicked();
		void on_TB_Browse_After_clicked();
		void on_TB_Log_File_clicked();
		void on_TB_Screenshot_Folder_clicked();
		void on_TB_QEMU_IMG_Browse_clicked();
		
		void on_TB_Add_Emulator_clicked();
		void on_TB_Delete_Emulator_clicked();
		void on_TB_Edit_Emulator_clicked();
		void on_TB_Use_Default_clicked();
		void on_TB_Find_All_Emulators_clicked();
		void on_Emulators_Table_cellDoubleClicked( int row, int column );
		void on_Button_CDROM_Add_clicked();
		void on_Button_CDROM_Edit_clicked();
		void on_Button_CDROM_Delete_clicked();
		void On_TB_WSL_Probe_clicked();

		void On_QEMU_Source_Toggled( bool checked );
		void On_QEMU_Custom_Browse_clicked();
		void On_QEMU_Use_Built_In_clicked();
		void Update_QEMU_Source_Banner();
		
		bool Load_Emulators_Info();
		bool Save_Emulators_Info();
		void Update_Emulators_Info();
	
		QStringList Get_All_Emulators_Names() const;
		
	private:
		Ui::Advanced_Settings_Window ui;
		QSettings Settings;
        Settings_Widget* settings_widget;
		
		QList<Emulator> Emulators;

		QCheckBox *CH_WSL_Launch_Enabled;
		class QComboBox *CB_WSL_Distro;
		QLineEdit *Edit_WSL_User;
		QLineEdit *Edit_WSL_Password;
		QCheckBox *CH_WSL_Remember_Password;
		class QComboBox *CB_WSL_Vulkan_Device;
		QCheckBox *CH_WSL_Reims_Host_Window;
		QLineEdit *Edit_WSL_Qemu_Binary;
		QLineEdit *Edit_WSL_AppleSoC_Binary;
		QLabel *Label_WSL_KVM_Status;
		QToolButton *TB_WSL_Probe;

		QLineEdit *Edit_FW_Python;
		QLineEdit *Edit_FW_Pyimg4;
		QLineEdit *Edit_FW_Img4;

	private slots:
		void On_CB_WSL_Distro_currentIndexChanged( int index );

	private:
		QGroupBox *GB_QEMU_Source;
		QRadioButton *RB_QEMU_Built_In;
		QRadioButton *RB_QEMU_Custom;
		QLineEdit *Edit_QEMU_Custom_Path;
		QToolButton *TB_QEMU_Custom_Browse;
		QToolButton *TB_QEMU_Use_Built_In;
		QLabel *Label_QEMU_Built_In_Path;
};

#endif
