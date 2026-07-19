/****************************************************************************
**
** Copyright (C) 2008-2010 Andrey Rijov <ANDron142@yandex.ru>
** COpyirght (C) 2016 Tobias Gläßer
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
#include <QRegExp>
#include <QFileDialog>
#include <QLabel>
#include <QRadioButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QToolButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QFileInfo>

#include "Utils.h"
#include "VM_Wizard_Window.h"
#include "System_Info.h"
#include "VM_Devices.h"

#ifdef Q_OS_WIN32
#include <QSysInfo>
QString Get_My_System_Architecture()
{
    QString arch = QSysInfo::currentCpuArchitecture();
    if( arch == "i386" ) return "i686";
    if( arch == "arm64" ) return "aarch64";
    return arch;
}
#else
#include <sys/utsname.h>
#include <stdio.h>

QString Get_My_System_Architecture()
{
    struct utsname name;
    uname(&name);
    return QString(name.machine);
}
#endif

VM_Wizard_Window::VM_Wizard_Window( QWidget *parent )
	: QDialog(parent)
{
	ui.setupUi( this );
    ui.Wizard_Pages->setCurrentIndex(0);
	
	New_VM = new Virtual_Machine();
	Win11_ARM_Page = nullptr;
	
	// Hide release date widgets
	ui.Label_Relese_Date->hide();
	ui.CB_Relese_Date->hide();
	
	Build_Windows11_ARM_Page();
	
	// Loadind All Templates
	if( Load_OS_Templates() )
	{
		// Find default template
		for( int ix = 0; ix < ui.CB_OS_Type->count(); ++ix )
		{
			if( ui.CB_OS_Type->itemText(ix) == Settings.value("Default_VM_Template", "Linux 2.6").toString() )
				ui.CB_OS_Type->setCurrentIndex( ix );
		}
	}
	else
	{
		AQWarning( "void VM_Wizard_Window::on_Button_Next_clicked()",
				   "No VM Templates Found!" );
	}

    connect(ui.RB_Emulator_KVM, SIGNAL(toggled(bool)),this, SLOT(KVM_toggled(bool)));
}

void VM_Wizard_Window::Build_Windows11_ARM_Page()
{
	Win11_ARM_Page = new QWidget();
	QVBoxLayout *mainLay = new QVBoxLayout( Win11_ARM_Page );
	
	QLabel *intro = new QLabel( tr(
		"<b>Windows 11 ARM setup</b><br>"
		"Uses VirtIO for disk, network, GPU, keyboard, RNG, balloon, and sound by default. "
		"Provide a slipstreamed Windows 11 ARM ISO (VirtIO drivers already included)." ) );
	intro->setWordWrap( true );
	mainLay->addWidget( intro );
	
	QGroupBox *diskBox = new QGroupBox( tr("Virtual disk") );
	QVBoxLayout *diskLay = new QVBoxLayout( diskBox );
	RB_Win11_New_Disk = new QRadioButton( tr("Create a new disk image") );
	RB_Win11_Existing_Disk = new QRadioButton( tr("Use an existing disk image") );
	RB_Win11_New_Disk->setChecked( true );
	diskLay->addWidget( RB_Win11_New_Disk );
	diskLay->addWidget( RB_Win11_Existing_Disk );
	
	QHBoxLayout *existLay = new QHBoxLayout();
	Edit_Win11_Existing_Disk = new QLineEdit();
	Edit_Win11_Existing_Disk->setEnabled( false );
	TB_Win11_Existing_Disk_Browse = new QToolButton();
	TB_Win11_Existing_Disk_Browse->setText( "..." );
	TB_Win11_Existing_Disk_Browse->setEnabled( false );
	existLay->addWidget( Edit_Win11_Existing_Disk );
	existLay->addWidget( TB_Win11_Existing_Disk_Browse );
	diskLay->addLayout( existLay );
	mainLay->addWidget( diskBox );
	
	CH_Win11_Already_Installed = new QCheckBox( tr("Windows is already installed on this disk") );
	mainLay->addWidget( CH_Win11_Already_Installed );
	
	QGroupBox *isoBox = new QGroupBox( tr("Install media") );
	QVBoxLayout *isoLay = new QVBoxLayout( isoBox );
	QHBoxLayout *isoPathLay = new QHBoxLayout();
	Edit_Win11_ISO = new QLineEdit();
	TB_Win11_ISO_Browse = new QToolButton();
	TB_Win11_ISO_Browse->setText( "..." );
	isoPathLay->addWidget( new QLabel( tr("Windows 11 ARM ISO:") ) );
	isoPathLay->addWidget( Edit_Win11_ISO );
	isoPathLay->addWidget( TB_Win11_ISO_Browse );
	isoLay->addLayout( isoPathLay );
	
	CH_Win11_VirtIO_ISO = new QCheckBox( tr("Also attach virtio-win.iso (only if drivers were not slipstreamed)") );
	isoLay->addWidget( CH_Win11_VirtIO_ISO );
	QHBoxLayout *virtioLay = new QHBoxLayout();
	Edit_Win11_VirtIO_ISO = new QLineEdit();
	Edit_Win11_VirtIO_ISO->setEnabled( false );
	TB_Win11_VirtIO_ISO_Browse = new QToolButton();
	TB_Win11_VirtIO_ISO_Browse->setText( "..." );
	TB_Win11_VirtIO_ISO_Browse->setEnabled( false );
	virtioLay->addWidget( Edit_Win11_VirtIO_ISO );
	virtioLay->addWidget( TB_Win11_VirtIO_ISO_Browse );
	isoLay->addLayout( virtioLay );
	mainLay->addWidget( isoBox );
	
	Label_Win11_UEFI_Status = new QLabel();
	Label_Win11_UEFI_Status->setWordWrap( true );
	mainLay->addWidget( Label_Win11_UEFI_Status );
	
	Label_Win11_Finish_Help = new QLabel( tr(
		"<b>After Finish:</b> Start the VM, install Windows from the ISO, then set boot order to the hard disk "
		"(or uncheck the CD-ROM) and start again for daily use." ) );
	Label_Win11_Finish_Help->setWordWrap( true );
	mainLay->addWidget( Label_Win11_Finish_Help );
	mainLay->addStretch();
	
	ui.Wizard_Pages->addWidget( Win11_ARM_Page );
	
	connect( RB_Win11_New_Disk, SIGNAL(toggled(bool)), this, SLOT(on_RB_Win11_New_Disk_toggled(bool)) );
	connect( CH_Win11_Already_Installed, SIGNAL(toggled(bool)), this, SLOT(on_CH_Win11_Already_Installed_toggled(bool)) );
	connect( TB_Win11_ISO_Browse, SIGNAL(clicked()), this, SLOT(on_TB_Win11_ISO_Browse_clicked()) );
	connect( TB_Win11_Existing_Disk_Browse, SIGNAL(clicked()), this, SLOT(on_TB_Win11_Existing_Disk_Browse_clicked()) );
	connect( TB_Win11_VirtIO_ISO_Browse, SIGNAL(clicked()), this, SLOT(on_TB_Win11_VirtIO_ISO_Browse_clicked()) );
	connect( CH_Win11_VirtIO_ISO, SIGNAL(toggled(bool)), this, SLOT(on_CH_Win11_VirtIO_ISO_toggled(bool)) );
}

void VM_Wizard_Window::on_CH_Win11_VirtIO_ISO_toggled( bool on )
{
	Edit_Win11_VirtIO_ISO->setEnabled( on );
	TB_Win11_VirtIO_ISO_Browse->setEnabled( on );
}

bool VM_Wizard_Window::Is_Windows11_ARM_Template() const
{
	if( ! ui.RB_VM_Template->isChecked() )
		return false;
	if( ui.CB_OS_Type->currentIndex() <= 0 )
		return false;
	return ui.CB_OS_Type->currentText().contains( "Windows 11 ARM", Qt::CaseInsensitive );
}

void VM_Wizard_Window::on_RB_Win11_New_Disk_toggled( bool on )
{
	Edit_Win11_Existing_Disk->setEnabled( ! on );
	TB_Win11_Existing_Disk_Browse->setEnabled( ! on );
}

void VM_Wizard_Window::on_CH_Win11_Already_Installed_toggled( bool on )
{
	Edit_Win11_ISO->setEnabled( ! on );
	TB_Win11_ISO_Browse->setEnabled( ! on );
	CH_Win11_VirtIO_ISO->setEnabled( ! on );
	if( on )
	{
		CH_Win11_VirtIO_ISO->setChecked( false );
		Edit_Win11_VirtIO_ISO->setEnabled( false );
		TB_Win11_VirtIO_ISO_Browse->setEnabled( false );
	}
}

void VM_Wizard_Window::on_TB_Win11_ISO_Browse_clicked()
{
	QString file = QFileDialog::getOpenFileName( this, tr("Select Windows 11 ARM ISO"),
		Get_Last_Dir_Path( Edit_Win11_ISO->text() ),
		tr("ISO Images (*.iso);;All Files (*)") );
	if( ! file.isEmpty() )
		Edit_Win11_ISO->setText( QDir::toNativeSeparators( file ) );
}

void VM_Wizard_Window::on_TB_Win11_Existing_Disk_Browse_clicked()
{
	QString file = QFileDialog::getOpenFileName( this, tr("Select existing disk image"),
		Get_Last_Dir_Path( Edit_Win11_Existing_Disk->text() ),
		tr("Disk Images (*.qcow2 *.qcow *.img *.vhd *.vhdx *.raw);;All Files (*)") );
	if( ! file.isEmpty() )
		Edit_Win11_Existing_Disk->setText( QDir::toNativeSeparators( file ) );
}

void VM_Wizard_Window::on_TB_Win11_VirtIO_ISO_Browse_clicked()
{
	QString file = QFileDialog::getOpenFileName( this, tr("Select virtio-win ISO"),
		Get_Last_Dir_Path( Edit_Win11_VirtIO_ISO->text() ),
		tr("ISO Images (*.iso);;All Files (*)") );
	if( ! file.isEmpty() )
		Edit_Win11_VirtIO_ISO->setText( QDir::toNativeSeparators( file ) );
}

void VM_Wizard_Window::KVM_toggled(bool toggled)
{
    if ( toggled )
        ui.toolBox_accelInfo->setCurrentIndex(1);
    else
        ui.toolBox_accelInfo->setCurrentIndex(0);
}

void VM_Wizard_Window::Set_VM_List( QList<Virtual_Machine*> *list )
{
	VM_List = list;
}

void VM_Wizard_Window::on_Button_Back_clicked()
{
	ui.Button_Next->setEnabled( true );
	
	if( ui.Wizard_Mode_Page == ui.Wizard_Pages->currentWidget() )
	{
		//ui.Wizard_Pages->setCurrentWidget( ui.Welcome_Page );
		//ui.Label_Page->setText( tr("New Virtual Machine Wizard") );
	}
	else if( ui.Template_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.Wizard_Mode_Page );
		ui.Label_Page->setText( tr("Wizard Mode") );
	    ui.Button_Back->setEnabled( false );
	}
	else if( ui.Accelerator_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.Template_Page );
		ui.Label_Page->setText( tr("VM Hardware Template") );
	}
	else if( ui.General_Settings_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.Accelerator_Page );
		ui.Label_Page->setText( tr("Accelerator") );
	}
	else if( ui.Memory_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.General_Settings_Page );
		ui.Label_Page->setText( tr("VM Name and CPU Type") );
		ui.Label_Caption_CPU_Type->setVisible( true );
		ui.Line_CPU_Type->setVisible( true );
		ui.Label_CPU_Type->setVisible( true );
		ui.CB_CPU_Type->setVisible( true );
	}
	else if( ui.Typical_HDD_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.General_Settings_Page );
		ui.Label_Page->setText( tr("Virtual Machine Name") );
		ui.Label_Caption_CPU_Type->setVisible( false );
		ui.Line_CPU_Type->setVisible( false );
		ui.Label_CPU_Type->setVisible( false );
		ui.CB_CPU_Type->setVisible( false );
	}
	else if( Win11_ARM_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( ui.RB_Typical->isChecked() )
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Typical_HDD_Page );
			ui.Label_Page->setText( tr("Hard Disk Size") );
		}
		else
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Custom_HDD_Page );
			ui.Label_Page->setText( tr("Virtual Hard Disk") );
		}
	}
	else if( ui.Custom_HDD_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.Memory_Page );
		ui.Label_Page->setText( tr("Memory") );
	}
	else if( ui.Network_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( Is_Windows11_ARM_Template() )
		{
			ui.Wizard_Pages->setCurrentWidget( Win11_ARM_Page );
			ui.Label_Page->setText( tr("Windows 11 ARM Install") );
		}
		else if( ui.RB_Typical->isChecked() ) // typical or custom mode
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Typical_HDD_Page );
			ui.Label_Page->setText( tr("Hard Disk Size") );
		}
		else
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Custom_HDD_Page );
			ui.Label_Page->setText( tr("Virtual Hard Disk") );
		}
	}
	else if( ui.Finish_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.Network_Page );
		ui.Label_Page->setText( tr("Network") );
		ui.Button_Next->setText( tr("&Next") );
	}
	else
	{
		// Default
		AQError( "void VM_Wizard_Window::on_Button_Back_clicked()",
				 "Default Section!" );
	}
}

void VM_Wizard_Window::on_Button_Next_clicked()
{
	if( ui.Wizard_Mode_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.Template_Page );
		ui.Label_Page->setText( tr("Template For VM") );
		on_RB_VM_Template_toggled( ui.RB_VM_Template->isChecked() );

        Current_Emulator = Get_Default_Emulator();

        // All Find Systems FIXME ^^^
        All_Systems = Current_Emulator.Get_Devices();
        if( All_Systems.isEmpty() )
        {
            AQError( "void VM_Wizard_Window::on_Button_Next_clicked()",
                     "Cannot get devices!" );
            return;
        }

        // Comp types
        ui.CB_Computer_Type->clear();
        for( QMap<QString, Available_Devices>::const_iterator it = All_Systems.constBegin(); it != All_Systems.constEnd(); it++ )
        {
            ui.CB_Computer_Type->addItem( it.value().System.Caption );
        }

        // Prefer host CPU architecture when available in the emulator list
        int defaultIndex = 0;
        QString host_arch = Get_My_System_Architecture();
        for( int ix = 0; ix < ui.CB_Computer_Type->count(); ++ix )
        {
            QString text = ui.CB_Computer_Type->itemText( ix );
            QString lower = text.toLower();
            if( host_arch.contains( "aarch64" ) || host_arch.contains( "arm64" ) )
            {
                if( lower.contains( "aarch64" ) || lower.contains( "arm 64" ) )
                {
                    defaultIndex = ix;
                    break;
                }
            }
            else if( host_arch.contains( "x86_64" ) || host_arch.contains( "amd64" ) )
            {
                if( text.contains( "64Bit" ) || lower.contains( "x86_64" ) )
                {
                    defaultIndex = ix;
                    break;
                }
            }
            else if( lower.contains( host_arch ) )
            {
                defaultIndex = ix;
                break;
            }
        }
        if( ui.CB_Computer_Type->count() > 0 )
        {
            ui.CB_Computer_Type->setCurrentIndex( defaultIndex );
        }

        // Auto-select first template if none is selected
        if( ui.CB_OS_Type->currentIndex() == 0 && ui.CB_OS_Type->count() > 1 )
        {
            ui.CB_OS_Type->setCurrentIndex( 1 );
        }

        ui.Button_Next->setEnabled( true );
		ui.Button_Back->setEnabled( true );
	}
	else if( ui.Accelerator_Page == ui.Wizard_Pages->currentWidget() )
	{
        applyTemplate();
        ui.Wizard_Pages->setCurrentWidget( ui.General_Settings_Page );

	}
	else if( ui.Template_Page == ui.Wizard_Pages->currentWidget() )
	{
        Use_Accelerator_Page = true;
		ui.Wizard_Pages->setCurrentWidget( ui.Accelerator_Page );

        // Prefer KVM/WHPX when host arch matches guest; otherwise TCG
        QString host_arch = Get_My_System_Architecture();
        bool guest_is_aarch64 = false;
        if( ui.RB_VM_Template->isChecked() && ui.CB_OS_Type->currentIndex() > 0 )
        {
            // Peek template computer type from filename / selection
            guest_is_aarch64 = ui.CB_OS_Type->currentText().contains( "ARM", Qt::CaseInsensitive ) ||
                               ui.CB_OS_Type->currentText().contains( "Raspberry", Qt::CaseInsensitive );
        }
        else if( ui.RB_Generate_VM->isChecked() )
        {
            guest_is_aarch64 = ui.CB_Computer_Type->currentText().contains( "AArch64", Qt::CaseInsensitive ) ||
                               ui.CB_Computer_Type->currentText().contains( "ARM", Qt::CaseInsensitive );
        }

        bool host_is_aarch64 = ( host_arch == "aarch64" || host_arch == "arm64" );
        if( guest_is_aarch64 && ! host_is_aarch64 )
            ui.RB_Emulator_QEMU->setChecked( true ); // TCG
        else if( ui.RB_Generate_VM->isChecked() && ui.CB_Computer_Type->currentText() != "IBM PC 64Bit" && ! guest_is_aarch64 )
            ui.RB_Emulator_QEMU->setChecked( true );
        else
            ui.RB_Emulator_KVM->setChecked( true );

		ui.Label_Page->setText( tr("Accelerator") );
	}
	else if( ui.General_Settings_Page == ui.Wizard_Pages->currentWidget() )
	{
		for( int vx = 0; vx < VM_List->count(); ++vx )
		{
			if( VM_List->at(vx)->Get_Machine_Name() == ui.Edit_VM_Name->text() )
			{
				AQGraphic_Warning( tr("Warning"), tr("This VM Name Does Already Exist!") );
				return;
			}
		}

		if( ui.RB_Typical->isChecked() )
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Typical_HDD_Page );
			ui.Label_Page->setText( tr("Hard Disk Size") );
		}
		else
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Memory_Page );
			ui.Label_Page->setText( tr("Memory") );
		}
	}
	else if( ui.Memory_Page == ui.Wizard_Pages->currentWidget() )
	{
		on_CH_Remove_RAM_Size_Limitation_stateChanged( Qt::Unchecked ); // update max available RAM size
		ui.Wizard_Pages->setCurrentWidget( ui.Custom_HDD_Page );
		ui.Label_Page->setText( tr("Virtual Hard Disk") );
	}
	else if( ui.Typical_HDD_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( Is_Windows11_ARM_Template() )
		{
			// Refresh UEFI status label
			QString qemu_bin;
			Emulator emul = Get_Default_Emulator();
			QMap<QString, QString> bins = emul.Get_Binary_Files();
			if( bins.contains( "qemu-system-aarch64" ) )
				qemu_bin = bins[ "qemu-system-aarch64" ];
			QString code = Find_UEFI_Firmware_CODE( qemu_bin );
			if( code.isEmpty() )
				Label_Win11_UEFI_Status->setText( tr("<font color='orange'>UEFI firmware (AAVMF/EDK2 aarch64) not found automatically. "
					"Install QEMU EDK2/AAVMF or set UEFI paths after creating the VM.</font>") );
			else
				Label_Win11_UEFI_Status->setText( tr("UEFI CODE found: %1").arg( code ) );
			
			ui.Wizard_Pages->setCurrentWidget( Win11_ARM_Page );
			ui.Label_Page->setText( tr("Windows 11 ARM Install") );
		}
		else
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Network_Page );
			ui.Label_Page->setText( tr("Network") );
		}
	}
	else if( Win11_ARM_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( ! CH_Win11_Already_Installed->isChecked() && Edit_Win11_ISO->text().isEmpty() )
		{
			AQGraphic_Warning( tr("Windows 11 ARM"),
				tr("Please select a Windows 11 ARM ISO, or check \"Windows is already installed on this disk\".") );
			return;
		}
		if( RB_Win11_Existing_Disk->isChecked() && Edit_Win11_Existing_Disk->text().isEmpty() )
		{
			AQGraphic_Warning( tr("Windows 11 ARM"),
				tr("Please select an existing disk image, or choose \"Create a new disk image\".") );
			return;
		}
		ui.Wizard_Pages->setCurrentWidget( ui.Network_Page );
		ui.Label_Page->setText( tr("Network") );
	}
	else if( ui.Custom_HDD_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( Is_Windows11_ARM_Template() )
		{
			ui.Wizard_Pages->setCurrentWidget( Win11_ARM_Page );
			ui.Label_Page->setText( tr("Windows 11 ARM Install") );
		}
		else
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Network_Page );
			ui.Label_Page->setText( tr("Network") );
		}
	}
	else if( ui.Network_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.Finish_Page );
		ui.Button_Next->setText( tr("&Finish") );
		ui.Label_Page->setText( tr("Finish!") );
        Create_New_VM(true);
        ui.VM_Information_Text->setHtml(New_VM->GenerateHTMLInfoText(3));
		Update_Finish_Page_Guidance();
	}
	else if( ui.Finish_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( Create_New_VM() ) accept();
	}
	else
	{
		AQError( "void VM_Wizard_Window::on_Button_Next_clicked()",
				 "Default Section!" );
	}
}

void VM_Wizard_Window::applyTemplate()
{
	// Use Selected Template
	if( ui.RB_VM_Template->isChecked() )
	{
		if( ! New_VM->Load_VM(OS_Templates_List[ui.CB_OS_Type->currentIndex()-1].filePath()) )
		{
			AQGraphic_Error( "void VM_Wizard_Window::Create_New_VM()", tr("Error!"),
							 tr("Cannot Create New VM from Template!") );
			return;
		}
	}
	
	// Emulator
	New_VM->Set_Emulator( Current_Emulator );
	
	// Find CPU List For This Template
	bool devices_found = false;
	
	if( ui.RB_VM_Template->isChecked() )
	{
		if( ui.RB_Emulator_KVM->isChecked() )
			New_VM->Set_Machine_Accelerator(VM::KVM);
		else
			New_VM->Set_Machine_Accelerator(VM::TCG);
			
		Current_Devices = &All_Systems[ New_VM->Get_Computer_Type() ];
		devices_found = true;
	}
	else
	{
		if( ui.RB_Emulator_KVM->isChecked() )
		{
			New_VM->Set_Machine_Accelerator(VM::KVM);
			New_VM->Set_Computer_Type( "qemu-system-x86_64" );
			
			if( New_VM->Get_Audio_Cards().Audio_es1370 )
			{
				VM::Sound_Cards tmp_audio = New_VM->Get_Audio_Cards();
				tmp_audio.Audio_es1370 = false;
				tmp_audio.Audio_AC97 = true;
				New_VM->Set_Audio_Cards( tmp_audio );
			}
			
			Current_Devices = &All_Systems[ New_VM->Get_Computer_Type() ];
			devices_found = true;
		}
		else
		{
			New_VM->Set_Machine_Accelerator(VM::TCG);
			New_VM->Set_Computer_Type( "qemu-system-x86_64" );

			Current_Devices = &All_Systems[ New_VM->Get_Computer_Type() ];
			devices_found = true;
		}
	}
	
	// Use Selected Template
	if( ui.RB_VM_Template->isChecked() )
	{
		// Name
		ui.Edit_VM_Name->setText( New_VM->Get_Machine_Name() );
		
		// Memory
		ui.Memory_Size->setValue( New_VM->Get_Memory_Size() );

		// CPU Cores
		ui.SB_CPU_Cores->setValue( New_VM->Get_SMP_CPU_Count() );
		
		// HDA
		double hda_size = New_VM->Get_HDA().Get_Virtual_Size_in_GB();
		
		if( hda_size != 0.0 )
			ui.SB_HDD_Size->setValue( hda_size );
		else
			ui.SB_HDD_Size->setValue( 10.0 );
		
		// Network
		ui.RB_User_Mode_Network->setChecked( New_VM->Get_Use_Network() );

		// Find CPU List For This Template
		Current_Devices = &All_Systems[ New_VM->Get_Computer_Type() ];
		if( ! Current_Devices->System.QEMU_Name.isEmpty() ) devices_found = true;
	}
	else // Create New VM in Date Mode
	{
		ui.Memory_Size->setValue( 2048 );
		ui.SB_HDD_Size->setValue( 20.0 );
		ui.SB_CPU_Cores->setValue( 1 );

		// Find CPU List For This Template
		QString compCaption = ui.CB_Computer_Type->currentText();
		for( QMap<QString, Available_Devices>::const_iterator it = All_Systems.constBegin(); it != All_Systems.constEnd(); it++ )
		{
			if( it.value().System.Caption == compCaption )
			{
				Current_Devices = &it.value();
				/*if( ! Current_Devices->System.QEMU_Name.isEmpty() )*/ devices_found = true;
				New_VM->Set_Computer_Type( Current_Devices->System.QEMU_Name );
			}
		}
	}

	if( ! devices_found )
	{
		AQGraphic_Error( "void VM_Wizard_Window::applyTemplate()", tr("Error!"),
						tr("Cannot Find Emulator System ID!") );
	}
	else
	{
		// Add CPU's
		ui.CB_CPU_Type->clear();
		for( int cx = 0; cx < Current_Devices->CPU_List.count(); ++cx )
			ui.CB_CPU_Type->addItem( Current_Devices->CPU_List[cx].Caption );
	}

	// Typical or custom mode
    Typical_Or_Custom();
}


void VM_Wizard_Window::Typical_Or_Custom()
{
	if( ui.RB_Typical->isChecked() )
	{
		ui.Label_Page->setText( tr("Virtual Machine Name") );
		on_Edit_VM_Name_textEdited( ui.Edit_VM_Name->text() );

		ui.Label_Caption_CPU_Type->setVisible( false );
		ui.Line_CPU_Type->setVisible( false );
		ui.Label_CPU_Type->setVisible( false );
		ui.CB_CPU_Type->setVisible( false );

		ui.Label_Caption_CPU_Cores->setVisible( false );
		ui.Line_CPU_Cores->setVisible( false );
		ui.Label_CPU_Cores->setVisible( false );
		ui.SB_CPU_Cores->setVisible( false );
	}
	else
	{
		ui.Label_Page->setText( tr("VM Name and CPU Type") );
		on_Edit_VM_Name_textEdited( ui.Edit_VM_Name->text() );

		ui.Label_Caption_CPU_Type->setVisible( true );
		ui.Line_CPU_Type->setVisible( true );
		ui.Label_CPU_Type->setVisible( true );
		ui.CB_CPU_Type->setVisible( true );

		ui.Label_Caption_CPU_Cores->setVisible( true );
		ui.Line_CPU_Cores->setVisible( true );
		ui.Label_CPU_Cores->setVisible( true );
		ui.SB_CPU_Cores->setVisible( true );
	}
}

bool VM_Wizard_Window::Load_OS_Templates()
{
	QList<QString> tmp_list = Get_Templates_List();

	for( int ax = 0; ax < tmp_list.count(); ++ax )
	{
		OS_Templates_List.append( QFileInfo(tmp_list[ax]) );
	}

	for( int ix = 0; ix < OS_Templates_List.count(); ++ix )
	{
		ui.CB_OS_Type->addItem( OS_Templates_List[ix].completeBaseName() );
	}

	 // no items found
	if( ui.CB_OS_Type->count() < 2 ) return false;
	else return true;
}

bool VM_Wizard_Window::Create_New_VM(bool simulate)
{
	// Icon
	QString icon_path = Find_OS_Icon( ui.Edit_VM_Name->text() );
	if( icon_path.isEmpty() )
	{
		AQWarning( "void VM_Wizard_Window::Create_New_VM()", "Icon for new VM not Found!" );
		New_VM->Set_Icon_Path( ":/other.png" );
	}
	else
	{
		New_VM->Set_Icon_Path( icon_path );
	}

	// Name
	New_VM->Set_Machine_Name( ui.Edit_VM_Name->text() );

	// Create path valid string
	QString VM_File_Name = Get_FS_Compatible_VM_Name( ui.Edit_VM_Name->text() );

	// Set Computer Type?
	if( ui.RB_Generate_VM->isChecked() )
	{
		New_VM->Set_Computer_Type( Current_Devices->System.QEMU_Name );
	}

	// RAM
	New_VM->Set_Memory_Size( ui.Memory_Size->value() );

	// CPU Cores
	New_VM->Set_SMP_CPU_Count( ui.SB_CPU_Cores->value() );

	// Wizard Mode
	if( ui.RB_Typical->isChecked() )
	{
		QString hd_path = Settings.value( "VM_Directory", "~" ).toString() + VM_File_Name;
		
		if( Is_Windows11_ARM_Template() && RB_Win11_Existing_Disk->isChecked() )
		{
			New_VM->Set_HDA( VM_HDD(true, Edit_Win11_Existing_Disk->text()) );
		}
		else
		{
			// Hard Disk
			VM::Device_Size hd_size;
			hd_size.Size = ui.SB_HDD_Size->value();
			hd_size.Suffix = VM::Size_Suf_Gb;

			if ( ! simulate )
				Create_New_HDD_Image( hd_path + "_HDA.img", hd_size );

			New_VM->Set_HDA( VM_HDD(true, hd_path + "_HDA.img") );
		}

		// Other HDD's
		if( New_VM->Get_HDB().Get_Enabled() )
		{
            if ( ! simulate )
                Create_New_HDD_Image( hd_path + "_HDB.img", New_VM->Get_HDB().Get_Virtual_Size() );
			New_VM->Set_HDB( VM_HDD(true, hd_path + "_HDB.img") );
		}
		
		if( New_VM->Get_HDC().Get_Enabled() )
		{
            if ( ! simulate )
                Create_New_HDD_Image( hd_path + "_HDC.img", New_VM->Get_HDC().Get_Virtual_Size() );
			New_VM->Set_HDC( VM_HDD(true, hd_path + "_HDC.img") );
		}
		
		if( New_VM->Get_HDD().Get_Enabled() )
		{
            if ( ! simulate )
                Create_New_HDD_Image( hd_path + "_HDD.img", New_VM->Get_HDD().Get_Virtual_Size() );
			New_VM->Set_HDD( VM_HDD(true, hd_path + "_HDD.img") );
		}
	}
	else
	{
		bool devices_found = false;
		
		// CPU Type
		if( ui.RB_VM_Template->isChecked() )
		{
			Current_Devices = &All_Systems[ New_VM->Get_Computer_Type() ];
			if( ! Current_Devices->System.QEMU_Name.isEmpty() ) devices_found = true;
		}
		else
		{
			// Find QEMU System Name in CB_Computer_Type
			if( ui.RB_Emulator_KVM->isChecked() )
			{
				Current_Devices = &All_Systems[ "qemu-system-x86_64" ];
				if( ! Current_Devices->System.QEMU_Name.isEmpty() ) devices_found = true;
			}
			else // QEMU
			{
				for( QMap<QString, Available_Devices>::const_iterator it = All_Systems.constBegin(); it != All_Systems.constEnd(); it++ )
				{
					if( it.value().System.Caption == ui.CB_Computer_Type->currentText() )
					{
						Current_Devices = &it.value();
						devices_found = true;
						break;
					}
				}
			}
		}
		
		if( ! devices_found )
		{
			AQGraphic_Error( "bool VM_Wizard_Window::Create_New_VM()", tr("Error!"),
							 tr("Cannot Find QEMU System ID!") );
			return false;
		}
		
		New_VM->Set_CPU_Type( Current_Devices->CPU_List[ui.CB_CPU_Type->currentIndex()].QEMU_Name );
		
		// Hard Disk
		if( ! ui.Edit_HDA_File_Name->text().isEmpty() )
			New_VM->Set_HDA( VM_HDD(true, ui.Edit_HDA_File_Name->text()) );
		else
			New_VM->Set_HDA( VM_HDD(false, "") );
	}
	
	// Network
	if( ui.RB_User_Mode_Network->isChecked() )
	{
		if( New_VM->Get_Network_Cards_Count() == 0 )
		{
			New_VM->Set_Use_Network( true );
			VM_Net_Card net_card;
			net_card.Set_Net_Mode( VM::Net_Mode_Usermode );
			
			New_VM->Add_Network_Card( net_card );
		}
	}
	else if( ui.RB_No_Network->isChecked() )
	{
		New_VM->Set_Use_Network( false );
		
		for( int rx = 0; rx < New_VM->Get_Network_Cards_Count(); ++rx )
		{
			New_VM->Delete_Network_Card( 0 );
		}
	}
	
	// Set Emulator Name (version) to Default ("")
	Emulator tmp_emul = New_VM->Get_Emulator();
	tmp_emul.Set_Name( "" );
	New_VM->Set_Emulator( tmp_emul );
	
	if( Is_Windows11_ARM_Template() )
		Apply_Windows11_ARM_Profile( simulate );
	else if( New_VM->Get_Computer_Type().contains( "aarch64", Qt::CaseInsensitive ) ||
			 New_VM->Get_Computer_Type().contains( "qemu-system-arm", Qt::CaseInsensitive ) )
		Apply_AArch64_Generic_Profile( simulate );
	
    if ( ! simulate )
    {
        // Create New VM XML File
        New_VM->Create_VM_File( Settings.value("VM_Directory", "~").toString() + VM_File_Name + ".aqemu", false );
    }

	return true;
}

void VM_Wizard_Window::Update_Finish_Page_Guidance()
{
	QString help;
	
	if( Is_Windows11_ARM_Template() )
	{
		if( CH_Win11_Already_Installed->isChecked() )
		{
			help = tr( "<p><b>Windows 11 ARM - ready to boot</b><br>"
				"Defaults match the win11-pi5-kiosk QEMU profile (virt + VirtIO disk/net/GPU, UEFI, USB audio). "
				"You can change any of these later in the main window.</p>" );
		}
		else
		{
			help = tr( "<p><b>Windows 11 ARM - install checklist</b></p><ol>"
				"<li>Start the VM (boots the ISO under UEFI).</li>"
				"<li>Install Windows onto the VirtIO disk.</li>"
				"<li>After install, remove or uncheck the CD-ROM / set boot order to Hard Disk.</li>"
				"<li>Start again for daily use.</li>"
				"</ol>"
				"<p>All machine/CPU/device options remain editable in the main GUI after creation.</p>" );
		}
	}
	else if( ui.RB_Generate_VM->isChecked() &&
			 ( ui.CB_Computer_Type->currentText().contains( "AArch64", Qt::CaseInsensitive ) ||
			   ui.CB_Computer_Type->currentText().contains( "ARM", Qt::CaseInsensitive ) ) )
	{
		help = tr( "<p><b>AArch64 / ARM custom VM</b><br>"
			"Starts with machine <code>virt</code> and VirtIO-friendly defaults so Linux and other ARM guests work. "
			"Pick any machine/CPU/video/network option in the main window for your OS.</p>" );
	}
	else
	{
		return;
	}
	
	ui.VM_Information_Text->setHtml( ui.VM_Information_Text->toHtml() + help );
}

void VM_Wizard_Window::Apply_Windows11_ARM_Profile( bool simulate )
{
	QString vm_dir = Settings.value( "VM_Directory", "~" ).toString();
	QString vm_base = Get_FS_Compatible_VM_Name( ui.Edit_VM_Name->text() );
	
	// CPU: host with Linux KVM; max on Windows / TCG (Windows aarch64 QEMU has no 'host')
	if( New_VM->Get_Machine_Accelerator() == VM::KVM )
	{
		#ifdef Q_OS_WIN32
		New_VM->Set_CPU_Type( "max" );
		#else
		New_VM->Set_CPU_Type( "host" );
		#endif
	}
	else if( New_VM->Get_CPU_Type().isEmpty() )
		New_VM->Set_CPU_Type( "max" );
	
	New_VM->Set_Machine_Type( "virt" );
	New_VM->Set_Video_Card( "virtio-gpu-pci" );
	New_VM->Set_SMP_CPU_Count( 4 );
	New_VM->Set_Memory_Size( 8192 );
	New_VM->Use_USB_Hub( true );
	New_VM->Use_VirtIO_RNG( true );
	New_VM->Use_VirtIO_Balloon( true );
	New_VM->Use_VirtIO_Keyboard( true );
	
	VM::Sound_Cards audio;
	// win11-pi5-kiosk uses usb-audio (virtio-win has no Windows ARM sound driver)
	audio.Audio_USB = true;
	audio.Audio_VirtIO = false;
	New_VM->Set_Audio_Cards( audio );
	
	// Network virtio-net-pci
	if( New_VM->Get_Network_Cards_Count() > 0 )
	{
		VM_Net_Card card = New_VM->Get_Network_Card( 0 );
		card.Set_Card_Model( "virtio-net-pci" );
		New_VM->Set_VM_Network_Card( 0, card );
	}
	
	// VirtIO disk
	VM_HDD hda = New_VM->Get_HDA();
	if( hda.Get_Enabled() )
	{
		VM_Native_Storage_Device native;
		native.Use_File_Path( true );
		native.Set_File_Path( hda.Get_File_Name() );
		native.Use_Interface( true );
		native.Set_Interface( VM::DI_Virtio );
		native.Use_Cache( true );
		#ifdef Q_OS_WIN32
		// cache=none (O_DIRECT) breaks qcow2 on Windows with a misleading format error
		native.Set_Cache( "writeback" );
		#else
		native.Set_Cache( "none" );
		#endif
		native.Use_AIO( true );
		native.Set_AIO( "threads" );
		native.Use_Media( true );
		native.Set_Media( VM::DM_Disk );
		hda.Set_Native_Device( native );
		New_VM->Set_HDA( hda );
	}
	
	// Install ISO / boot order
	if( CH_Win11_Already_Installed->isChecked() )
	{
		New_VM->Set_CD_ROM( VM_Storage_Device( false, "" ) );
		QList<VM::Boot_Order> boot;
		VM::Boot_Order b;
		b.Type = VM::Boot_From_HDD;
		b.Enabled = true;
		boot << b;
		New_VM->Set_Boot_Order_List( boot );
	}
	else if( ! Edit_Win11_ISO->text().isEmpty() )
	{
		VM_Storage_Device cd( true, Edit_Win11_ISO->text() );
		New_VM->Set_CD_ROM( cd );
		
		QList<VM::Boot_Order> boot;
		VM::Boot_Order bcd;
		bcd.Type = VM::Boot_From_CDROM;
		bcd.Enabled = true;
		boot << bcd;
		VM::Boot_Order bhdd;
		bhdd.Type = VM::Boot_From_HDD;
		bhdd.Enabled = true;
		boot << bhdd;
		New_VM->Set_Boot_Order_List( boot );
		
		if( CH_Win11_VirtIO_ISO->isChecked() && ! Edit_Win11_VirtIO_ISO->text().isEmpty() )
		{
			VM_Native_Storage_Device virtio_cd;
			virtio_cd.Use_File_Path( true );
			virtio_cd.Set_File_Path( Edit_Win11_VirtIO_ISO->text() );
			virtio_cd.Use_Media( true );
			virtio_cd.Set_Media( VM::DM_CD_ROM );
			QList<VM_Native_Storage_Device> extras = New_VM->Get_Storage_Devices_List();
			extras.append( virtio_cd );
			New_VM->Set_Storage_Devices_List( extras );
		}
	}
	
	// UEFI dual pflash
	Emulator emul = Get_Default_Emulator();
	QString qemu_bin;
	QMap<QString, QString> bins = emul.Get_Binary_Files();
	if( bins.contains( "qemu-system-aarch64" ) )
		qemu_bin = bins[ "qemu-system-aarch64" ];
	
	QString code = Find_UEFI_Firmware_CODE( qemu_bin );
	QString vars_dest = vm_dir + vm_base + "_VARS.fd";
	
	New_VM->Use_UEFI( true );
	if( ! code.isEmpty() )
		New_VM->Set_UEFI_CODE_File( code );
	
	New_VM->Set_UEFI_VARS_File( vars_dest );
	if( ! simulate )
		Prepare_UEFI_VARS_File( vars_dest, qemu_bin );
}

void VM_Wizard_Window::Apply_AArch64_Generic_Profile( bool simulate )
{
	// Flexible defaults for any aarch64/ARM guest (Linux, BSD, etc.) - not Win11-specific
	if( New_VM->Get_Machine_Type().isEmpty() )
		New_VM->Set_Machine_Type( "virt" );
	
	if( New_VM->Get_CPU_Type().isEmpty() )
	{
		#ifdef Q_OS_WIN32
		New_VM->Set_CPU_Type( "max" );
		#else
		if( New_VM->Get_Machine_Accelerator() == VM::KVM )
			New_VM->Set_CPU_Type( "host" );
		else
			New_VM->Set_CPU_Type( "max" );
		#endif
	}
	
	if( New_VM->Get_Video_Card().isEmpty() || New_VM->Get_Video_Card() == "std" )
		New_VM->Set_Video_Card( "virtio-gpu-pci" );
	
	// VirtIO-friendly defaults; user can change everything in the main window
	New_VM->Use_USB_Hub( true );
	New_VM->Use_VirtIO_RNG( true );
	New_VM->Use_VirtIO_Keyboard( true );
	
	if( New_VM->Get_Network_Cards_Count() > 0 )
	{
		VM_Net_Card card = New_VM->Get_Network_Card( 0 );
		QString model = card.Get_Card_Model();
		if( model.isEmpty() || model == "virtio" || model == "ne2k_pci" )
		{
			card.Set_Card_Model( "virtio-net-pci" );
			New_VM->Set_VM_Network_Card( 0, card );
		}
	}
	
	VM_HDD hda = New_VM->Get_HDA();
	if( hda.Get_Enabled() && ! hda.Get_Native_Mode() )
	{
		VM_Native_Storage_Device native;
		native.Use_File_Path( true );
		native.Set_File_Path( hda.Get_File_Name() );
		native.Use_Interface( true );
		native.Set_Interface( VM::DI_Virtio );
		native.Use_Cache( true );
		native.Set_Cache( "writeback" );
		native.Use_AIO( true );
		native.Set_AIO( "threads" );
		native.Use_Media( true );
		native.Set_Media( VM::DM_Disk );
		hda.Set_Native_Device( native );
		New_VM->Set_HDA( hda );
	}
	
	// UEFI when firmware is available (modern ARM guests)
	Emulator emul = Get_Default_Emulator();
	QMap<QString, QString> bins = emul.Get_Binary_Files();
	QString qemu_bin;
	if( bins.contains( New_VM->Get_Computer_Type() ) )
		qemu_bin = bins[ New_VM->Get_Computer_Type() ];
	else if( bins.contains( "qemu-system-aarch64" ) )
		qemu_bin = bins[ "qemu-system-aarch64" ];
	
	QString code = Find_UEFI_Firmware_CODE( qemu_bin );
	if( ! code.isEmpty() )
	{
		QString vm_dir = Settings.value( "VM_Directory", "~" ).toString();
		QString vm_base = Get_FS_Compatible_VM_Name( ui.Edit_VM_Name->text() );
		QString vars_dest = vm_dir + vm_base + "_VARS.fd";
		New_VM->Use_UEFI( true );
		New_VM->Set_UEFI_CODE_File( code );
		New_VM->Set_UEFI_VARS_File( vars_dest );
		if( ! simulate )
			Prepare_UEFI_VARS_File( vars_dest, qemu_bin );
	}
}

QString VM_Wizard_Window::Find_OS_Icon( const QString os_name )
{
	if( os_name.isEmpty() )
	{
		AQError( "QString VM_Wizard_Window::Find_OS_Icon( const QString os_name )",
				 "os_name is Empty!" );
		return "";
	}

	// Find all os icons
	QDir icons_dir( QDir::toNativeSeparators(Settings.value("AQEMU_Data_Folder","").toString() + "/os_icons/") );
	QFileInfoList all_os_icons = icons_dir.entryInfoList( QStringList("*.png"), QDir::Files, QDir::Unsorted );

	QRegExp rex;
	rex.setPatternSyntax( QRegExp::Wildcard );
	rex.setCaseSensitivity( Qt::CaseInsensitive );

	for( int i = 0; i < all_os_icons.count(); i++ )
	{
		rex.setPattern( "*" + all_os_icons[i].baseName() + "*" );

		if( rex.exactMatch(os_name) )
		{
			return all_os_icons[ i ].absoluteFilePath();
		}
	}

	// select os family...

	// Linux
	rex.setPattern( "*linux*" );
	if( rex.exactMatch(os_name) )
		return ":/default_linux.png";

	// Windows
	rex.setPattern( "*windows*" );
	if( rex.exactMatch(os_name) )
		return ":/default_windows.png";

	return ":/other.png";
}

void VM_Wizard_Window::on_RB_VM_Template_toggled( bool on )
{
	if( on )
	{
		if( ui.CB_OS_Type->currentIndex() == 0 )
			ui.Button_Next->setEnabled( false );
		else
			ui.Button_Next->setEnabled( true );
	}
}

void VM_Wizard_Window::on_RB_Generate_VM_toggled( bool on )
{
	if( on )
	{
		// Make sure a computer type is selected
		if( (ui.CB_Computer_Type->currentIndex() == -1 || ui.CB_Computer_Type->currentIndex() == 0) && ui.CB_Computer_Type->count() > 0 )
		{
			// Pre-select x86_64 or first item
			int defaultIndex = 0;
			for( int ix = 0; ix < ui.CB_Computer_Type->count(); ++ix )
			{
				QString text = ui.CB_Computer_Type->itemText( ix );
				if( text.contains("64Bit") || text.contains("x86_64") )
				{
					defaultIndex = ix;
					break;
				}
			}
			ui.CB_Computer_Type->setCurrentIndex( defaultIndex );
		}

		if( ui.CB_Computer_Type->currentIndex() == -1 )
			ui.Button_Next->setEnabled( false );
		else
			ui.Button_Next->setEnabled( true );
	}
}

void VM_Wizard_Window::on_CB_OS_Type_currentIndexChanged( int index )
{
	if( index == 0 )
		ui.Button_Next->setEnabled( false );
	else
		ui.Button_Next->setEnabled( true );
}

void VM_Wizard_Window::on_CB_Computer_Type_currentIndexChanged( int index )
{
	if( index == -1 )
	{
		ui.Button_Next->setEnabled( false );
	}
	else
	{
		ui.Button_Next->setEnabled( true );
	}
}

void VM_Wizard_Window::on_Memory_Size_valueChanged( int value )
{
	int cursorPos = ui.CB_RAM_Size->lineEdit()->cursorPosition();
	
	if( value % 1024 == 0 )
	    ui.CB_RAM_Size->setEditText( QString("%1 GB").arg(value / 1024) );
	else
	    ui.CB_RAM_Size->setEditText( QString("%1 MB").arg(value) );
	
	ui.CB_RAM_Size->lineEdit()->setCursorPosition( cursorPos );
}

void VM_Wizard_Window::on_CB_RAM_Size_editTextChanged( const QString &text )
{
	if( text.isEmpty() )
	    return;
	
	QRegExp rx( "\\s*([\\d]+)\\s*(MB|GB|M|G|)\\s*" ); // like: 512MB or 512
	if( ! rx.exactMatch(text.toUpper()) )
	{
		AQGraphic_Warning( tr("Error"),
						   tr("Cannot convert \"%1\" to memory size!").arg(text) );
		return;
	}
	
	QStringList ramStrings = rx.capturedTexts();
	if( ramStrings.count() != 3 )
	{
		AQGraphic_Warning( tr("Error"),
						   tr("Cannot convert \"%1\" to memory size!").arg(text) );
		return;
	}
	
	bool ok = false;
	int value = ramStrings[1].toInt( &ok, 10 );
	if( ! ok )
	{
		AQGraphic_Warning( tr("Error"),
						   tr("Cannot convert \"%1\" to integer!").arg(ramStrings[1]) );
		return;
	}
	
	if( ramStrings[2] == "MB" || ramStrings[2] == "M" ); // Size in megabytes
	else if( ramStrings[2] == "GB" || ramStrings[2] == "G" ) value *= 1024;
	else
	{
		AQGraphic_Warning( tr("Error"),
						   tr("Cannot convert \"%1\" to size suffix! Valid suffixes: MB, GB").arg(ramStrings[2]) );
		return;
	}
	
	if( value <= 0 )
	{
		AQGraphic_Warning( tr("Error"), tr("Memory size < 0! Valid size is 1 or more") );
		return;
	}
	
	on_TB_Update_Available_RAM_Size_clicked();
	if( (value > ui.Memory_Size->maximum()) &&
		(ui.CH_Remove_RAM_Size_Limitation->isChecked() == false) )
	{
		AQGraphic_Warning( tr("Error"),
						   tr("Your memory size %1 MB > %2 MB - all free RAM on this system!\n"
							  "To setup this value, check \"Remove limitation on maximum amount of memory\".")
						   .arg(value).arg(ui.Memory_Size->maximum()) );
		
		on_Memory_Size_valueChanged( ui.Memory_Size->value() ); // Set valid size
		return;
	}
	
	// All OK. Set memory size
	ui.Memory_Size->setValue( value );
}

void VM_Wizard_Window::on_CH_Remove_RAM_Size_Limitation_stateChanged( int state )
{
	if( state == Qt::Checked )
	{
		ui.Memory_Size->setMaximum( 32768 );
		ui.Label_Available_Free_Memory->setText( "32 GB" );
		Update_RAM_Size_ComboBox( 32768 );
	}
	else
	{
		int allRAM = 0, freeRAM = 0;
		System_Info::Get_Free_Memory_Size( allRAM, freeRAM );
		
		if( allRAM < ui.Memory_Size->value() )
			AQGraphic_Warning( tr("Error"), tr("Current memory size bigger than all existing host memory!\nUsing maximum available size.") );
		
		ui.Memory_Size->setMaximum( allRAM );
		ui.Label_Available_Free_Memory->setText( QString("%1 MB").arg(allRAM) );
		Update_RAM_Size_ComboBox( allRAM );
	}
}

void VM_Wizard_Window::on_TB_Update_Available_RAM_Size_clicked()
{
	int allRAM = 0, freeRAM = 0;
	System_Info::Get_Free_Memory_Size( allRAM, freeRAM );
	ui.TB_Update_Available_RAM_Size->setText( tr("Free memory: %1 MB").arg(freeRAM) );
	
	if( ! ui.CH_Remove_RAM_Size_Limitation->isChecked() )
	{
		ui.Memory_Size->setMaximum( allRAM );
		Update_RAM_Size_ComboBox( allRAM );
	}
}

void VM_Wizard_Window::Update_RAM_Size_ComboBox( int freeRAM )
{
	static int oldRamSize = 0;
	if( freeRAM == oldRamSize ) return;
	else oldRamSize = freeRAM;
	
	QStringList ramSizes;
	ramSizes << "32 MB" << "64 MB" << "128 MB" << "256 MB" << "512 MB"
			 << "1 GB" << "2 GB" << "3 GB" << "4 GB" << "8 GB" << "16 GB" << "32 GB";
	int maxRamIndex = 0;
	if( freeRAM >= 32768 ) maxRamIndex = 12;
	else if( freeRAM >= 16384 ) maxRamIndex = 11;
	else if( freeRAM >= 8192 ) maxRamIndex = 10;
	else if( freeRAM >= 4096 ) maxRamIndex = 9;
	else if( freeRAM >= 3072 ) maxRamIndex = 8;
	else if( freeRAM >= 2048 ) maxRamIndex = 7;
	else if( freeRAM >= 1024 ) maxRamIndex = 6;
	else if( freeRAM >= 512 ) maxRamIndex = 5;
	else if( freeRAM >= 256 ) maxRamIndex = 4;
	else if( freeRAM >= 128 ) maxRamIndex = 3;
	else if( freeRAM >= 64 ) maxRamIndex = 2;
	else if( freeRAM >= 32 ) maxRamIndex = 1;
	else
	{
		AQGraphic_Warning( tr("Error"), tr("Free memory on this system is lower than 32 MB!") );
		return;
	}
	
	if( maxRamIndex > ramSizes.count() )
	{
		AQError( "void VM_Wizard_Window::Update_RAM_Size_ComboBox( int freeRAM )",
				 "maxRamIndex > ramSizes.count()" );
		return;
	}
	
	QString oldText = ui.CB_RAM_Size->currentText();
	
	ui.CB_RAM_Size->clear();
	for( int ix = 0; ix < maxRamIndex; ix++ ) ui.CB_RAM_Size->addItem( ramSizes[ix] );
	
	ui.CB_RAM_Size->setEditText( oldText );
}

void VM_Wizard_Window::on_Edit_VM_Name_textEdited( const QString &text )
{
	if( ui.Edit_VM_Name->text().isEmpty() ) ui.Button_Next->setEnabled( false );
	else ui.Button_Next->setEnabled( true );
}

void VM_Wizard_Window::on_Button_New_HDD_clicked()
{
	Create_HDD_Image_Window Create_HDD_Win( this );
	
	Create_HDD_Win.Set_Image_Size( ui.SB_HDD_Size->value() ); // Set Initial HDA Size
	
	if( Create_HDD_Win.exec() == QDialog::Accepted )
		ui.Edit_HDA_File_Name->setText( Create_HDD_Win.Get_Image_File_Name() );
}

void VM_Wizard_Window::on_Button_Existing_clicked()
{
	QString hddPath = QFileDialog::getOpenFileName( this, tr("Select HDD Image"),
													Get_Last_Dir_Path(ui.Edit_HDA_File_Name->text()),
													tr("All Files (*)") );

	if( ! hddPath.isEmpty() )
		ui.Edit_HDA_File_Name->setText( QDir::toNativeSeparators(hddPath) );
}
