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
#include <QStandardItemModel>

#include "Utils.h"
#include "System_Info.h"
#include "Add_New_Device_Window.h"

Add_New_Device_Window::Add_New_Device_Window( QWidget *parent )
	: QDialog( parent )
{
	ui.setupUi( this );
	connect( ui.CB_Media, SIGNAL(currentIndexChanged(int)),
		 this, SLOT(on_CB_Media_currentIndexChanged(int)) );
}

VM_Native_Storage_Device Add_New_Device_Window::Get_Device() const
{
	return Device;
}

void Add_New_Device_Window::Set_Device( const VM_Native_Storage_Device &dev )
{
	Device = dev;
	
	// Update View...
	ui.CH_Interface->setChecked( Device.Use_Interface() );
	
	// Interface
	switch( Device.Get_Interface() )
	{
		case VM::DI_IDE:
			ui.CB_Interface->setCurrentIndex( 0 );
			break;
			
		case VM::DI_SCSI:
			ui.CB_Interface->setCurrentIndex( 1 );
			break;
			
		case VM::DI_SD:
			ui.CB_Interface->setCurrentIndex( 2 );
			break;
			
		case VM::DI_MTD:
			ui.CB_Interface->setCurrentIndex( 3 );
			break;
			
		case VM::DI_Floppy:
			ui.CB_Interface->setCurrentIndex( 4 );
			break;
			
		case VM::DI_PFlash:
			ui.CB_Interface->setCurrentIndex( 5 );
			break;
			
		case VM::DI_Virtio:
			ui.CB_Interface->setCurrentIndex( 6 );
			break;

        case VM::DI_Virtio_SCSI:
			ui.CB_Interface->setCurrentIndex( 7 );
			break;

		case VM::DI_NVMe:
			ui.CB_Interface->setCurrentIndex( 8 );
			break;

		case VM::DI_AHCI:
			ui.CB_Interface->setCurrentIndex( 9 );
			break;
			
		default:
            AQError( "void Add_New_Device_Window::Set_Device( const VM_Native_Storage_Device &dev )",
					 "Interface Default Section! Use IDE!" );
			break;
	}
	
	// Media
	ui.CH_Media->setChecked( Device.Use_Media() );
	
	switch( Device.Get_Media() )
	{
		case VM::DM_Disk:
			ui.CB_Media->setCurrentIndex( 0 );
			break;
			
		case VM::DM_CD_ROM:
			ui.CB_Media->setCurrentIndex( 1 );
			break;
			
		default:
            AQError( "void Add_New_Device_Window::Set_Device( const VM_Native_Storage_Device &dev )",
					 "Media Default Section! Use Disk!" );
			break;
	}
	
	// File Path
	ui.CH_File->setChecked( Device.Use_File_Path() );
	ui.Edit_File_Path->setText( Device.Get_File_Path() );
	
	// Index
	ui.CH_Index->setChecked( Device.Use_Index() );
	ui.SB_Index->setValue( Device.Get_Index() );
	
	// Bus, Unit
	ui.CH_Bus_Unit->setChecked( Device.Use_Bus_Unit() );
	ui.SB_Bus->setValue( Device.Get_Bus() );
	ui.SB_Unit->setValue( Device.Get_Unit() );
	
	// Snapshot
	ui.CH_Snapshot->setChecked( Device.Use_Snapshot() );
	ui.CB_Snapshot->setCurrentIndex( Device.Get_Snapshot() ? 0 : 1 );
	
	// Cache
	ui.CH_Cache->setChecked( Device.Use_Cache() );
	int index = ui.CB_Cache->findText( Device.Get_Cache() );
	if( index != -1 )
		ui.CB_Cache->setCurrentIndex( index );
	else
        AQError( "void Add_New_Device_Window::Set_Device( const VM_Native_Storage_Device &dev )",
				 "Cache: " + Device.Get_Cache() );
	
	// AIO
	ui.CH_AIO->setChecked( Device.Use_AIO() );
	index = ui.CB_AIO->findText( Device.Get_AIO() );
	if( index != -1 ) ui.CB_AIO->setCurrentIndex( index );
	else
        AQError( "void Add_New_Device_Window::Set_Device( const VM_Native_Storage_Device &dev )",
				 "AIO: " + Device.Get_AIO() );
	
	// Boot
	ui.CH_Boot->setChecked( Device.Use_Boot() );
	ui.CB_Boot->setCurrentIndex( Device.Get_Boot() ? 0 : 1 );

	// Discard
	ui.CH_Discard->setChecked( Device.Use_Discard() );
	ui.CB_Discard->setCurrentIndex( Device.Get_Discard() ? 0 : 1 );

	
	// cyls, heads, secs, trans
	ui.GB_hdachs_Settings->setChecked( Device.Use_hdachs() );
	ui.Edit_Cyls->setText( QString::number(Device.Get_Cyls()) );
	ui.Edit_Heads->setText( QString::number(Device.Get_Heads()) );
	ui.Edit_Secs->setText( QString::number(Device.Get_Secs()) );
	ui.Edit_Trans->setText( QString::number(Device.Get_Trans()) );
}

void Add_New_Device_Window::Set_Emulator_Devices( const Available_Devices &devices )
{
	if( devices.PSO_Drive_File )
	{
		ui.CH_File->setVisible( true );
		ui.Edit_File_Path->setVisible( true );
		ui.TB_File_Path_Browse->setVisible( true );
	}
	else
	{
		ui.CH_File->setVisible( false );
		ui.Edit_File_Path->setVisible( false );
		ui.TB_File_Path_Browse->setVisible( false );
	}
	
	if( devices.PSO_Drive_If )
	{
		ui.CH_Interface->setVisible( true );
		ui.CB_Interface->setVisible( true );
	}
	else
	{
		ui.CH_Interface->setVisible( false );
		ui.CB_Interface->setVisible( false );
	}
	
	if( devices.PSO_Drive_Bus_Unit )
	{
		ui.CH_Bus_Unit->setVisible( true );
		ui.SB_Bus->setVisible( true );
		ui.SB_Unit->setVisible( true );
	}
	else
	{
		ui.CH_Bus_Unit->setVisible( false );
		ui.SB_Bus->setVisible( false );
		ui.SB_Unit->setVisible( false );
	}
	
	if( devices.PSO_Drive_Index )
	{
		ui.CH_Index->setVisible( true );
		ui.SB_Index->setVisible( true );
	}
	else
	{
		ui.CH_Index->setVisible( false );
		ui.SB_Index->setVisible( false );
	}
		
	if( devices.PSO_Drive_Media )
	{
		ui.CH_Media->setVisible( true );
		ui.CB_Media->setVisible( true );
	}
	else
	{
		ui.CH_Media->setVisible( false );
		ui.CB_Media->setVisible( false );
	}
		
	if( devices.PSO_Drive_Cyls_Heads_Secs_Trans )
		ui.GB_hdachs_Settings->setVisible( true );
	else
		ui.GB_hdachs_Settings->setVisible( false );
		
	if( devices.PSO_Drive_Snapshot )
	{
		ui.CH_Snapshot->setVisible( true );
		ui.CB_Snapshot->setVisible( true );
	}
	else
	{
		ui.CH_Snapshot->setVisible( false );
		ui.CB_Snapshot->setVisible( false );
	}
		
	if( devices.PSO_Drive_Cache )
	{
		ui.CH_Cache->setVisible( true );
		ui.CB_Cache->setVisible( true );
	}
	else
	{
		ui.CH_Cache->setVisible( false );
		ui.CB_Cache->setVisible( false );
	}
		
	if( devices.PSO_Drive_AIO )
	{
		ui.CH_AIO->setVisible( true );
		ui.CB_AIO->setVisible( true );
	}
	else
	{
		ui.CH_AIO->setVisible( false );
		ui.CB_AIO->setVisible( false );
	}
		
	/* FIXME
	if( devices.PSO_Drive_Format )
	{
		ui.->setVisible( true );
		ui.->setVisible( true );
	}
	else
	{
		ui.->setVisible( false );
		ui.->setVisible( false );
	}
	
	if( devices.PSO_Drive_Serial )
	{
		ui.->setVisible( true );
		ui.->setVisible( true );
	}
	else
	{
		ui.->setVisible( false );
		ui.->setVisible( false );
	}
	
	if( devices.PSO_Drive_ADDR )
	{
		ui.->setVisible( true );
		ui.->setVisible( true );
	}
	else
	{
		ui.->setVisible( false );
		ui.->setVisible( false );
	}*/
	
	if( devices.PSO_Drive_Boot )
	{
		ui.CH_Boot->setVisible( true );
		ui.CB_Boot->setVisible( true );
	}
	else
	{
		ui.CH_Boot->setVisible( false );
		ui.CB_Boot->setVisible( false );
	}

	Target_Computer = devices.System.QEMU_Name;
	Enforce_Interface_Honesty();
	
	// Minimum Size
	resize( minimumSizeHint().width(), minimumSizeHint().height() );
}

void Add_New_Device_Window::Set_Machine_Type( const QString &machine_type )
{
	Target_Machine = machine_type;
	Enforce_Interface_Honesty();
}

VM::Device_Interface Add_New_Device_Window::Interface_From_Combo_Index( int index ) const
{
	switch( index )
	{
		case 0: return VM::DI_IDE;
		case 1: return VM::DI_SCSI;
		case 2: return VM::DI_SD;
		case 3: return VM::DI_MTD;
		case 4: return VM::DI_Floppy;
		case 5: return VM::DI_PFlash;
		case 6: return VM::DI_Virtio;
		case 7: return VM::DI_Virtio_SCSI;
		case 8: return VM::DI_NVMe;
		case 9: return VM::DI_AHCI;
		default: return VM::DI_IDE;
	}
}

int Add_New_Device_Window::Combo_Index_From_Interface( VM::Device_Interface iface ) const
{
	switch( iface )
	{
		case VM::DI_IDE: return 0;
		case VM::DI_SCSI: return 1;
		case VM::DI_SD: return 2;
		case VM::DI_MTD: return 3;
		case VM::DI_Floppy: return 4;
		case VM::DI_PFlash: return 5;
		case VM::DI_Virtio: return 6;
		case VM::DI_Virtio_SCSI: return 7;
		case VM::DI_NVMe: return 8;
		case VM::DI_AHCI: return 9;
		default: return 0;
	}
}

void Add_New_Device_Window::Enforce_Interface_Honesty()
{
	const bool optical = ( ui.CB_Media->currentIndex() == 1 );
	auto *model = qobject_cast<QStandardItemModel *>( ui.CB_Interface->model() );
	ui.CB_Interface->blockSignals( true );

	for( int i = 0; i < ui.CB_Interface->count(); ++i )
	{
		const VM::Device_Interface iface = Interface_From_Combo_Index( i );
		const bool allowed = Target_Computer.isEmpty()
			? true
			: System_Info::Is_Disk_Bus_Allowed( Target_Computer, Target_Machine, iface, optical );

		if( model )
		{
			QStandardItem *item = model->item( i );
			if( item )
			{
				if( allowed )
					item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
				else
					item->setFlags( item->flags() & ~( Qt::ItemIsEnabled | Qt::ItemIsSelectable ) );
			}
		}
	}

	const int cur = ui.CB_Interface->currentIndex();
	const VM::Device_Interface cur_iface = Interface_From_Combo_Index( cur );
	if( ! Target_Computer.isEmpty() &&
	    ! System_Info::Is_Disk_Bus_Allowed( Target_Computer, Target_Machine, cur_iface, optical ) )
	{
		const VM::Device_Interface safe =
			System_Info::Sanitize_Disk_Bus( Target_Computer, Target_Machine, cur_iface, optical );
		ui.CB_Interface->setCurrentIndex( Combo_Index_From_Interface( safe ) );
	}

	ui.CB_Interface->blockSignals( false );
}

void Add_New_Device_Window::Set_Enabled( bool enabled )
{
	ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled( enabled );
}

void Add_New_Device_Window::on_CB_Interface_currentIndexChanged( const QString &text )
{
	if( text == "ide" || text == "floppy" )
	{
		ui.CH_Index->setEnabled( true );
		ui.SB_Index->setEnabled( true );
		
		ui.CH_Bus_Unit->setEnabled( false );
		ui.SB_Bus->setEnabled( false );
		ui.SB_Unit->setEnabled( false );
	}
	else if( text == "scsi" )
	{
		ui.CH_Index->setEnabled( false );
		ui.SB_Index->setEnabled( false );
		
		ui.CH_Bus_Unit->setEnabled( true );
		ui.SB_Bus->setEnabled( true );
		ui.SB_Unit->setEnabled( true );
	}
	else if( text == "virtio" )
	{
		ui.CH_Index->setEnabled( true );
		ui.SB_Index->setEnabled( true );
		
		ui.CH_Bus_Unit->setEnabled( true );
		ui.SB_Bus->setEnabled( true );
		ui.SB_Unit->setEnabled( true );
	}
	else // ahci, nvme, virtio-scsi, etc. — port/unit assigned automatically
	{
		ui.CH_Index->setEnabled( false );
		ui.SB_Index->setEnabled( false );
		
		ui.CH_Bus_Unit->setEnabled( false );
		ui.SB_Bus->setEnabled( false );
		ui.SB_Unit->setEnabled( false );
	}
}

void Add_New_Device_Window::on_CB_Media_currentIndexChanged( int )
{
	// Selecting CD-ROM implies media=cdrom must be emitted (tobimensch#110)
	if( ui.CB_Media->currentIndex() == 1 )
		ui.CH_Media->setChecked( true );
	Enforce_Interface_Honesty();
}

void Add_New_Device_Window::on_TB_File_Path_Browse_clicked()
{
	const bool optical = ( ui.CB_Media->currentIndex() == 1 );
	const bool floppy = ( ui.CB_Interface->currentText() == QLatin1String( "floppy" ) );
	QString file_name = QFileDialog::getOpenFileName( this, tr("Select your device"),
													  Get_Last_Dir_Path(ui.Edit_File_Path->text()),
													  Disk_Image_File_Filter( optical, floppy ) );
	
	if( ! file_name.isEmpty() )
		ui.Edit_File_Path->setText( QDir::toNativeSeparators(file_name) );
}

void Add_New_Device_Window::done(int r)
{
    if ( r == QDialog::Accepted )
    {
	    // Interface
	    switch( ui.CB_Interface->currentIndex() )
	    {
		    case 0:
			    Device.Set_Interface( VM::DI_IDE );
			    break;
			
		    case 1:
			    Device.Set_Interface( VM::DI_SCSI );
			    break;
			
		    case 2:
			    Device.Set_Interface( VM::DI_SD );
			    break;
			
		    case 3:
			    Device.Set_Interface( VM::DI_MTD );
			    break;
			
		    case 4:
			    Device.Set_Interface( VM::DI_Floppy );
			    break;
			
		    case 5:
			    Device.Set_Interface( VM::DI_PFlash );
			    break;
			
		    case 6:
			    Device.Set_Interface( VM::DI_Virtio );
			    break;
			
		    case 7:
                Device.Set_Interface( VM::DI_Virtio_SCSI );
			    break;

		    case 8:
			    Device.Set_Interface( VM::DI_NVMe );
			    break;

		    case 9:
			    Device.Set_Interface( VM::DI_AHCI );
			    break;

		    default:
			    AQError( "void Add_New_Device_Window::done(int)",
					     "Invalid Interface Index! Use IDE" );
			    Device.Set_Interface( VM::DI_IDE );
			    break;
	    }
	    {
		    const bool optical = ( ui.CB_Media->currentIndex() == 1 );
		    if( ! Target_Computer.isEmpty() )
			    Device.Set_Interface( System_Info::Sanitize_Disk_Bus(
				    Target_Computer, Target_Machine, Device.Get_Interface(), optical ) );
	    }
	
	    Device.Use_Interface( ui.CH_Interface->isChecked() );
	
	    // Media
	    switch( ui.CB_Media->currentIndex() )
	    {
		    case 0:
			    Device.Set_Media( VM::DM_Disk );
			    break;
			
		    case 1:
			    Device.Set_Media( VM::DM_CD_ROM );
			    // Always persist media=cdrom when CD-ROM is chosen (tobimensch#110)
			    ui.CH_Media->setChecked( true );
			    break;
			
		    default:
			    AQError( "void Add_New_Device_Window::done(int)",
					     "Invalid Media Index! Use Disk" );
			    Device.Set_Media( VM::DM_Disk );
			    break;
	    }
	
	    Device.Use_Media( ui.CH_Media->isChecked() || Device.Get_Media() == VM::DM_CD_ROM );
	
	    // File Path
	    if( ui.CH_File->isChecked() )
	    {
		    if( ! QFile::exists(ui.Edit_File_Path->text()) )
		    {
			    AQGraphic_Warning( tr("Error!"), tr("File does not exist!") );
			    return;
		    }
	    }
	
	    Device.Use_File_Path( ui.CH_File->isChecked() );
	    Device.Set_File_Path( ui.Edit_File_Path->text() );
	
	    // Index
	    Device.Use_Index( ui.CH_Index->isChecked() );
	    Device.Set_Index( ui.SB_Index->value() );
	
	    // Bus, Unit
	    Device.Use_Bus_Unit( ui.CH_Bus_Unit->isChecked() );
	    Device.Set_Bus( ui.SB_Bus->value() );
	    Device.Set_Unit( ui.SB_Unit->value() );
	
	    // Snapshot
	    Device.Use_Snapshot( ui.CH_Snapshot->isChecked() );
	    Device.Set_Snapshot( (ui.CB_Snapshot->currentIndex() == 0) ? true : false );
	
	    // Cache
	    Device.Use_Cache( ui.CH_Cache->isChecked() );
	    Device.Set_Cache( ui.CB_Cache->currentText() );
	
	    // AIO
	    Device.Use_AIO( ui.CH_AIO->isChecked() );
	    Device.Set_AIO( ui.CB_AIO->currentText() );
	
	    // Boot
	    Device.Use_Boot( ui.CH_Boot->isChecked() );
	    Device.Set_Boot( (ui.CB_Boot->currentIndex() == 0) ? true : false );
	

	// Discard
	Device.Use_Discard( ui.CH_Discard->isChecked() );
	Device.Set_Discard( (ui.CB_Discard->currentIndex() == 0) ? true : false );

	    // hdachs
	    if( ui.GB_hdachs_Settings->isChecked() )
	    {
		    bool ok;
		
		    qulonglong cyls = ui.Edit_Cyls->text().toULongLong( &ok, 10 );
		    if( ! ok )
		    {
			    AQGraphic_Warning( tr("Warning!"), tr("\"Cyls\" value is incorrect!") );
			    return;
		    }
		
		    qulonglong heads = ui.Edit_Heads->text().toULongLong( &ok, 10 );
		    if( ! ok )
		    {
			    AQGraphic_Warning( tr("Warning!"), tr("\"Heads\" value is incorrect!") );
			    return;
		    }
		
		    qulonglong secs = ui.Edit_Secs->text().toULongLong( &ok, 10) ;
		    if( ! ok )
		    {
			    AQGraphic_Warning( tr("Warning!"), tr("\"Secs\" value is incorrect!") );
			    return;
		    }
		
		    qulonglong trans = ui.Edit_Trans->text().toULongLong( &ok, 10 );
		    if( ! ok )
		    {
			    AQGraphic_Warning( tr("Warning!"), tr("\"Trans\" value is incorrect!") );
			    return;
		    }
		
		    Device.Use_hdachs( ui.GB_hdachs_Settings->isChecked() );
		    Device.Set_Cyls( cyls );
		    Device.Set_Heads( heads );
		    Device.Set_Secs( secs );
		    Device.Set_Trans( trans );
	    }
	}
	QDialog::done(r);
}
