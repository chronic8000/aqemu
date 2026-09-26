/****************************************************************************
**
** Copyright (C) 2009-2010 Andrey Rijov <ANDron142@yandex.ru>
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

#ifndef FIRST_START_WIZARD_H
#define FIRST_START_WIZARD_H

#include <QDialog>
#include <QSettings>
#include "VM_Devices.h"
#include "ui_First_Start_Wizard.h"

class First_Start_Wizard: public QDialog
{
	Q_OBJECT

	public:
		First_Start_Wizard( QWidget *parent = 0 );
		bool Find_Emulators();

	private slots:
		void on_Button_Cancel_clicked();
		void on_Button_Back_clicked();
		void on_Button_Next_clicked();
		void on_Edit_VM_Dir_textChanged();
		void on_TB_Browse_VM_Dir_clicked();

		void on_TB_Add_Emulator_Browse_File_clicked();
		void on_TB_Add_Emulator_Browse_clicked();
		void on_Edit_Add_Emulator_Path_textChanged();
		void on_Button_Add_Emulator_Manual_Mode_clicked();
		void On_FS_QEMU_Source_Toggled( bool checked );

		void on_All_Pages_currentChanged( int index );

	private:
		void Load_Settings();
		bool Save_Settings();
		void retranslateUi();
		void Update_QEMU_Validation();

		Ui::First_Start_Wizard ui;
		QStringList Header_Captions;
		QSettings Settings;
		Emulator Emul;
};

#endif
