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

#include <QDir>
#include <QProcess>
#include <QMessageBox>
#include <QTranslator>
#include <QFileDialog>

#include "First_Start_Wizard.h"
#include "AQ_UI_Style.h"
#include "Utils.h"
#include "System_Info.h"
#include "Advanced_Settings_Window.h"
#include "Emulator_Options_Window.h"

First_Start_Wizard::First_Start_Wizard( QWidget *parent )
	: QDialog( parent )
{
	ui.setupUi( this );
	AQ_Cap_Content_Width( this, 720 );

	connect( ui.RB_FS_QEMU_Built_In, SIGNAL(toggled(bool)), this, SLOT(On_FS_QEMU_Source_Toggled(bool)) );
	connect( ui.RB_FS_QEMU_System, SIGNAL(toggled(bool)), this, SLOT(On_FS_QEMU_Source_Toggled(bool)) );
	connect( ui.RB_FS_QEMU_Custom, SIGNAL(toggled(bool)), this, SLOT(On_FS_QEMU_Source_Toggled(bool)) );
	connect( ui.Edit_Add_Emulator_Path, SIGNAL(textChanged(const QString &)), this, SLOT(on_Edit_Add_Emulator_Path_textChanged()) );

	const bool has_bundled = AQ_Has_Bundled_QEMU();
	const bool has_system = AQ_Has_System_QEMU();
	ui.RB_FS_QEMU_Built_In->setEnabled( has_bundled );
	ui.RB_FS_QEMU_System->setEnabled( has_system );

	if( has_bundled )
	{
		ui.RB_FS_QEMU_Built_In->setChecked( true );
		ui.Edit_Add_Emulator_Path->setText( QDir::toNativeSeparators( AQ_Get_Bundled_QEMU_Dir() ) );
	}
	else if( has_system )
	{
		ui.RB_FS_QEMU_System->setChecked( true );
		ui.Edit_Add_Emulator_Path->setText( QDir::toNativeSeparators( AQ_Get_System_QEMU_Dir() ) );
	}
	else
	{
		ui.RB_FS_QEMU_Custom->setChecked( true );
	}

	retranslateUi();
	Load_Settings();

	ui.All_Pages->setCurrentIndex( 0 );
	on_All_Pages_currentChanged( 0 );
}

bool First_Start_Wizard::Find_Emulators()
{
	System_Info::Auto_Find_And_Save_Emulators();
	return Save_Settings();
}

void First_Start_Wizard::on_Button_Cancel_clicked()
{
	if( QMessageBox::information( this, tr( "Warning!" ),
		tr( "Are you sure? You can configure AQEMU at any time in File → Settings." ),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) == QMessageBox::Yes )
	{
		reject();
	}
}

void First_Start_Wizard::on_Button_Back_clicked()
{
	const int currentIndex = ui.All_Pages->currentIndex();
	if( currentIndex > 0 )
	{
		ui.All_Pages->setCurrentIndex( currentIndex - 1 );
	}
}

void First_Start_Wizard::on_Button_Next_clicked()
{
	const int currentIndex = ui.All_Pages->currentIndex();
	if( currentIndex == 0 )
	{
		retranslateUi();
		ui.All_Pages->setCurrentIndex( 1 );
	}
	else if( currentIndex == 1 )
	{
		ui.All_Pages->setCurrentIndex( 2 );
	}
	else if( currentIndex == 2 )
	{
		// Apply selected QEMU before proceeding to Finish page
		if( ui.RB_FS_QEMU_Built_In->isChecked() )
		{
			if( ! AQ_Has_Bundled_QEMU() ||
			    ! AQ_Apply_QEMU_Dir_As_Default_Emulator( AQ_Get_Bundled_QEMU_Dir(), tr( "Bundled QEMU" ) ) )
			{
				QMessageBox::warning( this, tr( "QEMU Configuration" ),
					tr( "Could not configure bundled QEMU binaries." ) );
				return;
			}
			AQ_Set_QEMU_Source_Mode( QStringLiteral( "bundled" ) );
		}
		else if( ui.RB_FS_QEMU_System->isChecked() )
		{
			if( ! AQ_Has_System_QEMU() ||
			    ! AQ_Apply_QEMU_Dir_As_Default_Emulator( AQ_Get_System_QEMU_Dir(), tr( "System QEMU" ) ) )
			{
				QMessageBox::warning( this, tr( "QEMU Configuration" ),
					tr( "Could not configure system-installed QEMU." ) );
				return;
			}
			AQ_Set_QEMU_Source_Mode( QStringLiteral( "system" ) );
		}
		else
		{
			const QString path = ui.Edit_Add_Emulator_Path->text().trimmed();
			if( path.isEmpty() )
			{
				QMessageBox::warning( this, tr( "QEMU Configuration" ),
					tr( "Please specify a custom QEMU executable or folder." ) );
				return;
			}
			if( ! AQ_Apply_QEMU_Dir_As_Default_Emulator( path, tr( "Custom QEMU" ) ) )
			{
				QMessageBox::warning( this, tr( "QEMU Configuration" ),
					tr( "No QEMU binaries or executable found in:\n%1" ).arg( path ) );
				return;
			}
			AQ_Set_QEMU_Source_Mode( QStringLiteral( "custom" ) );
		}

		ui.All_Pages->setCurrentIndex( 3 );
	}
	else if( currentIndex == ui.All_Pages->count() - 1 )
	{
		if( Save_Settings() )
		{
			Advanced_Settings_Window asw;
			asw.done( QDialog::Accepted );
			accept();
		}
	}
}

void First_Start_Wizard::On_FS_QEMU_Source_Toggled( bool )
{
	const bool custom = ui.RB_FS_QEMU_Custom->isChecked();
	ui.Widget_Custom_QEMU->setEnabled( custom );

	if( ui.RB_FS_QEMU_Built_In->isChecked() && AQ_Has_Bundled_QEMU() )
	{
		ui.Edit_Add_Emulator_Path->setText( QDir::toNativeSeparators( AQ_Get_Bundled_QEMU_Dir() ) );
	}
	else if( ui.RB_FS_QEMU_System->isChecked() && AQ_Has_System_QEMU() )
	{
		ui.Edit_Add_Emulator_Path->setText( QDir::toNativeSeparators( AQ_Get_System_QEMU_Dir() ) );
	}

	Update_QEMU_Validation();
}

void First_Start_Wizard::on_Edit_Add_Emulator_Path_textChanged()
{
	Update_QEMU_Validation();
}

void First_Start_Wizard::Update_QEMU_Validation()
{
	const bool onAddEmulatorPage = ( ui.All_Pages->currentIndex() == 2 );

	if( ui.RB_FS_QEMU_Built_In->isChecked() )
	{
		if( AQ_Has_Bundled_QEMU() )
		{
			QString ver = System_Info::Get_Emulator_Version_Label( AQ_Get_Bundled_QEMU_Dir() );
			if( ver.isEmpty() || ver == QLatin1String( "QEMU" ) )
				ver = QStringLiteral( "QEMU 11.1.1" );
			ui.Label_Add_Emulator_Version->setText(
				tr( "<font color=\"#2e7d32\"><b>✓ %1 detected and ready in application directory</b></font>" ).arg( ver ) );
			if( onAddEmulatorPage ) ui.Button_Next->setEnabled( true );
		}
		else
		{
			ui.Label_Add_Emulator_Version->setText(
				tr( "<font color=\"#d32f2f\"><b>⚠ Bundled QEMU binaries not found next to application executable</b></font>" ) );
			if( onAddEmulatorPage ) ui.Button_Next->setEnabled( false );
		}
	}
	else if( ui.RB_FS_QEMU_System->isChecked() )
	{
		if( AQ_Has_System_QEMU() )
		{
			QString ver = System_Info::Get_Emulator_Version_Label( AQ_Get_System_QEMU_Dir() );
			if( ver.isEmpty() || ver == QLatin1String( "QEMU" ) )
				ver = QStringLiteral( "QEMU 11.1.1" );
			ui.Label_Add_Emulator_Version->setText(
				tr( "<font color=\"#2e7d32\"><b>✓ %1 detected in system directory (%2)</b></font>" ).arg( ver ).arg( AQ_Get_System_QEMU_Dir() ) );
			if( onAddEmulatorPage ) ui.Button_Next->setEnabled( true );
		}
		else
		{
			ui.Label_Add_Emulator_Version->setText(
				tr( "<font color=\"#d32f2f\"><b>⚠ No system QEMU installation detected</b></font>" ) );
			if( onAddEmulatorPage ) ui.Button_Next->setEnabled( false );
		}
	}
	else // Custom
	{
		const QString p = ui.Edit_Add_Emulator_Path->text().trimmed();
		if( p.isEmpty() )
		{
			ui.Label_Add_Emulator_Version->setText(
				tr( "<font color=\"#757575\">Please specify a QEMU binary (e.g. qemu.exe, qemu-system-x86_64.exe) or directory</font>" ) );
			if( onAddEmulatorPage ) ui.Button_Next->setEnabled( false );
		}
		else
		{
			QMap<QString, QString> bins = System_Info::Find_QEMU_Binary_Files( p );
			int count = 0;
			for( auto it = bins.constBegin(); it != bins.constEnd(); ++it )
			{
				if( ! it.value().isEmpty() && QFile::exists( it.value() ) )
					++count;
			}
			if( count > 0 )
			{
				QString ver = System_Info::Get_Emulator_Version_Label( p );
				if( ver.isEmpty() || ver == QLatin1String( "QEMU" ) )
					ver = QStringLiteral( "QEMU 11.1.1" );
				ui.Label_Add_Emulator_Version->setText(
					tr( "<font color=\"#2e7d32\"><b>✓ %1 detected (%2 target architecture(s) found)</b></font>" ).arg( ver ).arg( count ) );
				if( onAddEmulatorPage ) ui.Button_Next->setEnabled( true );
			}
			else
			{
				ui.Label_Add_Emulator_Version->setText(
					tr( "<font color=\"#d32f2f\"><b>⚠ No QEMU executable or targets found at specified path</b></font>" ) );
				if( onAddEmulatorPage ) ui.Button_Next->setEnabled( false );
			}
		}
	}
}

void First_Start_Wizard::on_TB_Add_Emulator_Browse_File_clicked()
{
#ifdef Q_OS_WIN32
	const QString filter = tr( "QEMU Executables (qemu*.exe *.exe);;All Files (*.*)" );
#else
	const QString filter = tr( "QEMU Executables (qemu*);;All Files (*)" );
#endif
	const QString file = QFileDialog::getOpenFileName(
		this, tr( "Select QEMU Executable" ),
		ui.Edit_Add_Emulator_Path->text(),
		filter );
	if( ! file.isEmpty() )
	{
		ui.Edit_Add_Emulator_Path->setText( QDir::toNativeSeparators( file ) );
		ui.RB_FS_QEMU_Custom->setChecked( true );
	}
}

void First_Start_Wizard::on_TB_Add_Emulator_Browse_clicked()
{
	const QString folder = QFileDialog::getExistingDirectory(
		this, tr( "Select QEMU Installation Folder" ),
		ui.Edit_Add_Emulator_Path->text() );
	if( ! folder.isEmpty() )
	{
		ui.Edit_Add_Emulator_Path->setText( QDir::toNativeSeparators( folder ) );
		ui.RB_FS_QEMU_Custom->setChecked( true );
	}
}

void First_Start_Wizard::on_Button_Add_Emulator_Manual_Mode_clicked()
{
	Emulator_Options_Window *win = new Emulator_Options_Window( this );
	win->Set_Emulator( Emulator() );
	if( win->exec() == QDialog::Accepted )
	{
		Emulator em = win->Get_Emulator();
		em.Set_Default( true );
		Remove_All_Emulators_Files();
		em.Save();
		AQ_Set_QEMU_Source_Mode( QStringLiteral( "custom" ) );
		ui.Edit_Add_Emulator_Path->setText( em.Get_Path() );
		ui.RB_FS_QEMU_Custom->setChecked( true );
		Update_QEMU_Validation();
	}
	delete win;
}

void First_Start_Wizard::on_Edit_VM_Dir_textChanged()
{
	if( ui.All_Pages->currentIndex() == 1 )
	{
		ui.Button_Next->setEnabled( ! ui.Edit_VM_Dir->text().trimmed().isEmpty() );
	}
}

void First_Start_Wizard::on_TB_Browse_VM_Dir_clicked()
{
	QString folder = QFileDialog::getExistingDirectory( this, tr( "Set Folder for your VMs" ),
														Settings.value( "VM_Directory", "~" ).toString() );
	if( ! folder.isEmpty() )
	{
		if( ! ( folder.endsWith( "/" ) || folder.endsWith( "\\" ) ) )
			folder += "/";

		ui.Edit_VM_Dir->setText( QDir::toNativeSeparators( folder ) );
	}
}

void First_Start_Wizard::on_All_Pages_currentChanged( int index )
{
	if( index < 0 || index >= ui.All_Pages->count() )
		return;

	// Back, Next Buttons State
	if( index == 0 )
	{
		ui.Button_Back->setEnabled( false );
		ui.Button_Next->setText( tr( "&Next" ) );
		ui.Button_Next->setEnabled( true );
	}
	else if( index == 1 )
	{
		ui.Button_Back->setEnabled( true );
		ui.Button_Next->setText( tr( "&Next" ) );
		ui.Button_Next->setEnabled( ! ui.Edit_VM_Dir->text().trimmed().isEmpty() );
	}
	else if( index == 2 )
	{
		ui.Button_Back->setEnabled( true );
		ui.Button_Next->setText( tr( "&Next" ) );
		Update_QEMU_Validation();
	}
	else if( index == ui.All_Pages->count() - 1 )
	{
		ui.Button_Back->setEnabled( true );
		ui.Button_Next->setText( tr( "&Finish" ) );
		ui.Button_Next->setEnabled( true );
	}

	if( index < Header_Captions.count() )
		ui.Label_Caption->setText( Header_Captions[index] );
}

void First_Start_Wizard::Load_Settings()
{
	// Find All Language Files (*.qm)
	QDir data_dir( Settings.value( "AQEMU_Data_Folder", "/usr/share/aqemu/" ).toString() );
	QFileInfoList lang_files = data_dir.entryInfoList( QStringList( "*.qm" ), QDir::Files, QDir::Name );

	if( lang_files.count() > 0 )
	{
		for( int dd = 0; dd < lang_files.count(); ++dd )
		{
			ui.CB_Language->addItem( lang_files[dd].completeBaseName() );

			if( lang_files[dd].completeBaseName() == Settings.value( "Language", "en" ).toString() )
				ui.CB_Language->setCurrentIndex( dd + 1 ); // First item English
		}
	}

	// Virtual Machines Folder
#ifdef Q_OS_WIN32
	ui.Edit_VM_Dir->setText( QDir::toNativeSeparators(
		Settings.value( "VM_Directory", AQEMU_Default_VM_Directory() ).toString() ) );
#else
	ui.Edit_VM_Dir->setText( QDir::toNativeSeparators(
		Settings.value( "VM_Directory", QDir::homePath() + "/.aqemu/" ).toString() ) );
#endif
}

bool First_Start_Wizard::Save_Settings()
{
	QDir dir;
	if( ! dir.exists( ui.Edit_VM_Dir->text() ) )
	{
		if( ! dir.mkpath( ui.Edit_VM_Dir->text() ) )
		{
			AQGraphic_Error( "bool First_Start_Wizard::Save_Settings()", tr( "Error!" ),
							 tr( "Cannot create directory \"%1\"!" ).arg( ui.Edit_VM_Dir->text() ) );
			return false;
		}
	}

	Settings.setValue( "VM_Directory", ui.Edit_VM_Dir->text() );

	// Interface Language
	if( ui.CB_Language->currentIndex() == 0 )
		Settings.setValue( "Language", "en" );
	else
		Settings.setValue( "Language", ui.CB_Language->currentText() );

	// First Start flag
	Settings.setValue( "First_Start", "no" );

	return true;
}

void First_Start_Wizard::retranslateUi()
{
	setWindowTitle( tr( "First Start Wizard" ) );

	static QTranslator appTranslator;
	if( ! appTranslator.isEmpty() )
		QCoreApplication::removeTranslator( &appTranslator );

	if( ui.CB_Language->currentIndex() > 0 )
	{
		if( appTranslator.load( Settings.value( "AQEMU_Data_Folder", "" ).toString() +
							   ui.CB_Language->itemText( ui.CB_Language->currentIndex() ) + ".qm" ) )
		{
			QCoreApplication::installTranslator( &appTranslator );
		}
	}

	Header_Captions.clear();
	Header_Captions << tr( "Welcome to AQEMU" );
	Header_Captions << tr( "Virtual Machine Storage" );
	Header_Captions << tr( "QEMU Emulator Setup" );
	Header_Captions << tr( "Ready to Start" );

	if( ui.All_Pages->currentIndex() >= 0 && ui.All_Pages->currentIndex() < Header_Captions.count() )
		ui.Label_Caption->setText( Header_Captions[ui.All_Pages->currentIndex()] );

	ui.Button_Back->setText( tr( "&Back" ) );
	ui.Button_Cancel->setText( tr( "&Cancel" ) );
	if( ui.All_Pages->currentIndex() == ui.All_Pages->count() - 1 )
		ui.Button_Next->setText( tr( "&Finish" ) );
	else
		ui.Button_Next->setText( tr( "&Next" ) );

	ui.Label_Welcome_Text->setText(
		tr( "Welcome to AQEMU!\n\n"
		    "This setup wizard will help you configure your virtual machine environment in just a few simple steps.\n\n"
		    "Click Next to begin." ) );
	ui.Label_Select_Language->setText( tr( "Choose interface language:" ) );
	ui.Label_VM_Dir->setText( tr( "Please choose where virtual machine files and disk images should be stored:" ) );
	ui.Label_Add_Emulator_Help->setText(
		tr( "AQEMU requires QEMU binaries to run virtual machines. Select how AQEMU should locate QEMU:" ) );

	ui.RB_FS_QEMU_Built_In->setText( tr( "Use the QEMU binaries that ship with AQEMU (Recommended)" ) );
	ui.RB_FS_QEMU_System->setText( tr( "Use system-installed QEMU" ) );
	ui.RB_FS_QEMU_Custom->setText( tr( "Specify a custom QEMU executable or folder" ) );

	ui.Label_Add_Emulator_Path->setText( tr( "Path to QEMU executable (e.g. qemu.exe, qemu-system-x86_64.exe) or directory:" ) );
	ui.TB_Add_Emulator_Browse_File->setText( tr( "Browse File..." ) );
	ui.TB_Add_Emulator_Browse->setText( tr( "Browse Folder..." ) );
	ui.Button_Add_Emulator_Manual_Mode->setText( tr( "Advanced configuration..." ) );

	ui.Label_Finish_Text->setText(
		tr( "Congratulations!\n\n"
		    "AQEMU is now configured with modern QEMU 11.1.1 support and ready to create and run virtual machines.\n\n"
		    "Click Finish to launch AQEMU." ) );

	if( AQ_Has_Bundled_QEMU() )
	{
		QString bDir = AQ_Get_Bundled_QEMU_Dir();
		QString ver = System_Info::Get_Emulator_Version_Label( bDir );
		if( ver.isEmpty() || ver == QLatin1String( "QEMU" ) )
			ver = QStringLiteral( "QEMU 11.1.1" );
		ui.Label_QEMU_Built_In_Path->setText( tr( "Location: %1 (%2)" ).arg( bDir ).arg( ver ) );
	}
	else
	{
		ui.Label_QEMU_Built_In_Path->setText( tr( "No bundled QEMU binaries found next to AQEMU executable" ) );
	}

	if( AQ_Has_System_QEMU() )
	{
		QString sDir = AQ_Get_System_QEMU_Dir();
		QString ver = System_Info::Get_Emulator_Version_Label( sDir );
		if( ver.isEmpty() || ver == QLatin1String( "QEMU" ) )
			ver = QStringLiteral( "QEMU 11.1.1" );
		ui.Label_QEMU_System_Path->setText( tr( "Location: %1 (%2)" ).arg( sDir ).arg( ver ) );
	}
	else
	{
		ui.Label_QEMU_System_Path->setText( tr( "No system-wide QEMU installation found in standard paths" ) );
	}

	Update_QEMU_Validation();
}
