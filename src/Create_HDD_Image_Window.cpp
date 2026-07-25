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

#include <QFileDialog>
#include <QMessageBox>

#include "Utils.h"
#include "VM_Devices.h"
#include "Create_HDD_Image_Window.h"

Create_HDD_Image_Window::Create_HDD_Image_Window( QWidget *parent )
	: QDialog( parent )
{
	ui.setupUi( this );

	// Populate formats from live qemu-img (qemu-doc §3.7)
	ui.CB_Format->clear();
	const QStringList formats = Probe_QEMU_IMG_Formats();
	if( ! formats.isEmpty() )
		ui.CB_Format->addItems( formats );
	else
		ui.CB_Format->addItems( QStringList() << "qcow2" << "qcow" << "vmdk" << "raw" << "vpc" << "vhdx" << "cow" << "cloop" );
	const QString pref = Preferred_QEMU_IMG_Format(
		formats.isEmpty() ? QStringList() << "qcow2" : formats );
	const int pref_ix = ui.CB_Format->findText( pref, Qt::MatchFixedString );
	if( pref_ix >= 0 )
		ui.CB_Format->setCurrentIndex( pref_ix );
	
	resize( width(), minimumSizeHint().height() );
}

const QString &Create_HDD_Image_Window::Get_Image_File_Name()
{
	Image_File_Name = ui.Edit_File_Name->text();
	return Image_File_Name;
}

void Create_HDD_Image_Window::Set_Image_File_Name( const QString &path )
{
	Image_File_Name = path;
	ui.Edit_File_Name->setText( path );
}

void Create_HDD_Image_Window::Set_Image_Info( VM::Disk_Info info )
{
	// This function word only in format mode. Change button caption
	ui.Button_Create->setText( tr("F&ormat") );
	
	// Format
	int format_ix = ui.CB_Format->findText( info.Disk_Format );
	
	if( format_ix != -1 )
	{
		ui.CB_Format->setCurrentIndex( format_ix );
	}
	else
	{
		AQError( "void Create_HDD_Image_Window::Set_Image_Info( VM::Disk_Info info )",
				 "Cannot Find Format" );
	}
	
	// Size
	switch( info.Virtual_Size.Suffix )
	{
		case VM::Size_Suf_Mb: // MB
			ui.CB_Suffix->setCurrentIndex( 1 );
			break;
			
		case VM::Size_Suf_Gb: // GB
			ui.CB_Suffix->setCurrentIndex( 2 );
			break;
			
		default: // KG
			ui.CB_Suffix->setCurrentIndex( 0 );
			break;
	}
	
	ui.SB_Size->setValue( info.Virtual_Size.Size );
}

void Create_HDD_Image_Window::Set_Image_Size( double gb )
{
	ui.SB_Size->setValue( gb );
}

void Create_HDD_Image_Window::on_Button_Browse_Base_Image_clicked()
{
	QString fileName = QFileDialog::getOpenFileName( this, tr("Select Base HDD Image File"),
													 Get_Last_Dir_Path(ui.Edit_Base_Image_File_Name->text()),
													 Disk_Image_File_Filter( false, false ) );
	
	if( ! fileName.isEmpty() )
		ui.Edit_Base_Image_File_Name->setText( QDir::toNativeSeparators(fileName) );
}

void Create_HDD_Image_Window::on_Button_Browse_New_Image_clicked()
{
	QString fileName = QFileDialog::getSaveFileName( this, tr("Create HDD Image File"),
													 Get_Last_Dir_Path(ui.Edit_File_Name->text()),
													 Disk_Image_File_Filter( false, false ) );
	
	if( ! fileName.isEmpty() )
		ui.Edit_File_Name->setText( QDir::toNativeSeparators(fileName) );
}

void Create_HDD_Image_Window::on_CB_Format_currentIndexChanged( const QString &text )
{
	/*if( text == "qcow2" || text == "qcow" )
	{
		ui.CH_Encrypted->setEnabled( true );
	}
	else
	{
		ui.CH_Encrypted->setEnabled( false );
	}*/
}

void Create_HDD_Image_Window::on_Button_Create_clicked()
{
	if( ui.Edit_File_Name->text().isEmpty() )
	{
		AQGraphic_Warning( tr("Error!"), tr("Image File Name is Empty!") );
		return;
	}
	
	if( ui.SB_Size->value() < 1 || ui.SB_Size->value() > 1024 )
	{
		AQGraphic_Warning( tr("Error!"), tr("Invalid image size!") );
		return;
	}
	
	bool Create_OK = false;
	
	VM::Device_Size hd_size;
	hd_size.Size = ui.SB_Size->value();
	
	switch( ui.CB_Suffix->currentIndex() )
	{
		case 1: // MB
			hd_size.Suffix = VM::Size_Suf_Mb;
			break;
			
		case 2: // GB
			hd_size.Suffix = VM::Size_Suf_Gb;
			break;
			
		default: // KG
			hd_size.Suffix= VM::Size_Suf_Kb;
			break;
	}
	
	if( ui.CH_Base_Image->isChecked() )
	{
		if( ! QFile::exists(ui.Edit_Base_Image_File_Name->text()) )
		{
			AQGraphic_Warning( tr("Error!"), tr("Base Image File doesn't Exists!") );
			return;
		}
		else
		{
			Create_OK = Create_New_HDD_Image( false, ui.Edit_Base_Image_File_Name->text(),
											  ui.Edit_File_Name->text(), ui.CB_Format->currentText(), hd_size, true );
		}
	}
	else
	{
		Create_OK = Create_New_HDD_Image( false, "", ui.Edit_File_Name->text(),
										  ui.CB_Format->currentText(), hd_size, true );
	}
	
	if( Create_OK )
	{
		accept();
	}
	else
	{
		AQGraphic_Warning( tr("Error!"), tr("Image was Not Created!") );
		return;
	}
}

void Create_HDD_Image_Window::on_Button_Format_Help_clicked()
{
	QMessageBox::information( this, tr("QEMU-IMG Supported formats"),
		QEMU_IMG_Format_Help_Text( Probe_QEMU_IMG_Formats() ) );
}
