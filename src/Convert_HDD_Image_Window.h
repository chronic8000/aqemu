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

#ifndef CONVERT_HDD_IMAGE_WINDOW_H
#define CONVERT_HDD_IMAGE_WINDOW_H

#include <QProgressDialog>
#include <QThread>
#include <QStringList>
#include "ui_Convert_HDD_Image_Window.h"
#include "HDD_Image_Info.h"

class Convert_HDD_Thread : public QThread
{
	Q_OBJECT

	public:
		Convert_HDD_Thread();
		void Set_Jobs( const QList<QStringList> &jobs );
		const QString &Get_Error_Message() const;
		const QString &Get_Log() const;

	protected:
		void run();

	signals:
		void Conversion_Complete( bool ok );

	private:
		QString Error_Message;
		QString Log;
		QList<QStringList> Jobs;
};

/** Full qemu-img front end: convert (batch), resize, info, check, snapshots. */
class Convert_HDD_Image_Window: public QDialog
{
	Q_OBJECT

	public:
		Convert_HDD_Image_Window( QWidget *parent = 0 );

	private slots:
		void on_Button_Browse_Base_clicked();
		void on_Button_Browse_Output_clicked();
		void on_CB_Output_Format_currentIndexChanged( const QString &text );
		void on_Button_Convert_clicked();
		void on_Button_Add_Images_clicked();
		void on_Button_Output_Dir_clicked();
		void Update_Info( bool ok );

		void Conversion_Done( bool ok );
		void Cancel_Convertion();

		bool Get_QEMU_IMG_Info();

		void Browse_Tool_Image();
		void Run_Info();
		void Run_Check();
		void Run_Resize();
		void Snapshot_Refresh();
		void Snapshot_Create();
		void Snapshot_Apply();
		void Snapshot_Delete();

	private:
		void Append_Log( const QString &text );
		QString Tool_Image_Path() const;
		bool Run_Qemu_Img( const QStringList &args, QString *out, QString *err );
		QWidget *Build_Resize_Tab();
		QWidget *Build_Info_Check_Tab();
		QWidget *Build_Snapshots_Tab();

		Ui::Convert_HDD_Image_Window ui;
		Convert_HDD_Thread Conv_Thread;
		QProgressDialog *ProgDial;

		HDD_Image_Info *HDD_Info;
		bool Valid_Info;
		class QListWidget *Batch_List;
		class QPlainTextEdit *Log_View;
		class QCheckBox *CH_Beside;
		class QLineEdit *Edit_Output_Dir;
		class QComboBox *CB_Preallocation;

		class QLineEdit *Edit_Tool_Image;
		class QDoubleSpinBox *SB_Resize_Size;
		class QComboBox *CB_Resize_Unit;
		class QCheckBox *CH_Resize_Shrink;
		class QCheckBox *CH_Check_Repair;
		class QListWidget *Snapshot_List;
		class QLineEdit *Edit_Snapshot_Name;
};

#endif
