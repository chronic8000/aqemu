/****************************************************************************
**
** Copyright (C) 2026 Chronic Engineering
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

#ifndef STORAGE_RECOVERY_WINDOW_H
#define STORAGE_RECOVERY_WINDOW_H

#include <QDialog>
#include <QSettings>

#include "Storage_Recovery.h"

class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class Virtual_Machine;

class Storage_Recovery_Window: public QDialog
{
	Q_OBJECT

	public:
		explicit Storage_Recovery_Window( QWidget *parent = 0 );
		Virtual_Machine *Get_VM() const;

	private slots:
		void Add_Files();
		void Add_Folder();
		void Add_Physical();
		void Remove_Selected();
		void Browse_Iso();
		void Analyze();
		void Create_Vm();
		void Invalidate_Report();

	private:
		void Refresh_Source_List();
		bool Already_Listed( const QString &path ) const;
		void Remember_Path( const QString &path );

		QSettings Settings;
		QListWidget *Source_List;
		QLineEdit *Edit_Name;
		QLineEdit *Edit_Iso;
		QPlainTextEdit *Report_View;
		QPushButton *Button_Create;
		QStringList Source_Paths;
		QList<Disk_Probe> Probes;
		QList<Probable_Pool> Pools;
		QString Report_Text;
		Virtual_Machine *Created_VM;
};

class Recovery_Checkpoint_Window: public QDialog
{
	Q_OBJECT

	public:
		explicit Recovery_Checkpoint_Window( Virtual_Machine *vm, QWidget *parent = 0 );

	private slots:
		void Enable_Checkpoint();
		void Discard_Checkpoint();
		void Keep_This_Checkpoint();

	private:
		QString Attempt_Dir() const;
		QString Manifest_Path() const;
		QString Report_Path() const;
		void Note_Checkpoint( const QString &text );

		Virtual_Machine *VM;
};

#endif
