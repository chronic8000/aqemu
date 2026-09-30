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

#include "Storage_Recovery_Window.h"

#include <QAbstractItemView>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

#include "Utils.h"
#include "VM.h"

namespace {

struct Disk_Slot
{
	bool hda;
	int storage_index;
	VM_Native_Storage_Device device;
};

bool Is_File_Disk( const VM_Native_Storage_Device &device )
{
	if( ! device.Use_File_Path() || device.Get_File_Path().isEmpty() )
		return false;
	if( device.Use_Media() && device.Get_Media() == VM::DM_CD_ROM )
		return false;
	return true;
}

QList<Disk_Slot> Collect_File_Disks( Virtual_Machine *vm )
{
	QList<Disk_Slot> disk_slots;
	const VM_HDD hda = vm->Get_HDA();
	if( hda.Get_Enabled() && Is_File_Disk( hda.Get_Native_Device() ) )
	{
		Disk_Slot slot;
		slot.hda = true;
		slot.storage_index = -1;
		slot.device = hda.Get_Native_Device();
		disk_slots << slot;
	}
	const QList<VM_Native_Storage_Device> extra = vm->Get_Storage_Devices_List();
	for( int i = 0; i < extra.size(); ++i )
	{
		if( ! Is_File_Disk( extra[i] ) )
			continue;
		Disk_Slot slot;
		slot.hda = false;
		slot.storage_index = i;
		slot.device = extra[i];
		disk_slots << slot;
	}
	return disk_slots;
}

void Apply_Bindings( Virtual_Machine *vm, const QList<Disk_Slot> &disk_slots, const QList<Drive_Binding> &bindings )
{
	VM_HDD hda = vm->Get_HDA();
	QList<VM_Native_Storage_Device> storage = vm->Get_Storage_Devices_List();
	for( int i = 0; i < disk_slots.size() && i < bindings.size(); ++i )
	{
		VM_Native_Storage_Device device = disk_slots[i].device;
		device.Use_File_Path( true );
		device.Set_File_Path( bindings[i].path );
		device.Set_Read_Only( bindings[i].read_only );
		if( disk_slots[i].hda )
		{
			hda.Set_Enabled( true );
			hda.Set_File_Name( bindings[i].path );
			hda.Set_Native_Device( device );
		}
		else if( disk_slots[i].storage_index >= 0 && disk_slots[i].storage_index < storage.size() )
		{
			storage[disk_slots[i].storage_index] = device;
		}
	}
	vm->Set_HDA( hda );
	vm->Set_Storage_Devices_List( storage );
}

QString Qemu_Version_Line()
{
	QProcess process;
	process.start( QStringLiteral( "qemu-system-x86_64" ), QStringList() << QStringLiteral( "--version" ) );
	if( ! process.waitForFinished( 3000 ) )
		return QStringLiteral( "not available" );
	const QString text = QString::fromLocal8Bit( process.readAllStandardOutput() ).trimmed();
	const int nl = text.indexOf( QLatin1Char( '\n' ) );
	if( text.isEmpty() )
		return QStringLiteral( "not available" );
	return nl < 0 ? text : text.left( nl );
}

QString Attempt_Directory( Virtual_Machine *vm )
{
	const QFileInfo xml( vm->Get_VM_XML_File_Path() );
	return xml.dir().filePath( vm->Get_Machine_Name() + QStringLiteral( "-recovery-checkpoints/attempt-1" ) );
}

QString Report_File( Virtual_Machine *vm )
{
	const QFileInfo xml( vm->Get_VM_XML_File_Path() );
	return xml.dir().filePath( vm->Get_Machine_Name() + QStringLiteral( "-recovery-report.txt" ) );
}

bool Is_Disk_Image_Name( const QString &name )
{
	const QString lower = name.toLower();
	return lower.endsWith( QLatin1String( ".vmdk" ) )
		|| lower.endsWith( QLatin1String( ".qcow2" ) )
		|| lower.endsWith( QLatin1String( ".raw" ) )
		|| lower.endsWith( QLatin1String( ".img" ) )
		|| lower.endsWith( QLatin1String( ".vhdx" ) )
		|| lower.endsWith( QLatin1String( ".vdi" ) );
}

QString Guid_Serial( quint64 guid )
{
	return QStringLiteral( "aqemu-%1" ).arg( guid, 16, 16, QLatin1Char( '0' ) );
}

} // namespace

Storage_Recovery_Window::Storage_Recovery_Window( QWidget *parent )
	: QDialog( parent ), Created_VM( 0 )
{
	setWindowTitle( tr( "Storage Recovery" ) );
	resize( 760, 640 );

	QVBoxLayout *lay = new QVBoxLayout( this );
	QLabel *intro = new QLabel( tr(
		"AQEMU reads disk labels and builds a read-only recovery VM. "
		"The guest decides whether a pool imports." ) );
	intro->setWordWrap( true );
	lay->addWidget( intro );

	QHBoxLayout *name_row = new QHBoxLayout();
	name_row->addWidget( new QLabel( tr( "VM name" ) ) );
	Edit_Name = new QLineEdit( QStringLiteral( "Storage Recovery" ) );
	name_row->addWidget( Edit_Name );
	lay->addLayout( name_row );

	Source_List = new QListWidget();
	lay->addWidget( Source_List, 1 );

	QHBoxLayout *source_buttons = new QHBoxLayout();
	QPushButton *add_files = new QPushButton( tr( "Add disk files…" ) );
	QPushButton *add_folder = new QPushButton( tr( "Add folder…" ) );
	QPushButton *add_phys = new QPushButton( tr( "Add physical disks…" ) );
	QPushButton *remove = new QPushButton( tr( "Remove" ) );
	source_buttons->addWidget( add_files );
	source_buttons->addWidget( add_folder );
	source_buttons->addWidget( add_phys );
	source_buttons->addWidget( remove );
	source_buttons->addStretch( 1 );
	lay->addLayout( source_buttons );

	QHBoxLayout *iso_row = new QHBoxLayout();
	iso_row->addWidget( new QLabel( tr( "Boot ISO" ) ) );
	Edit_Iso = new QLineEdit();
	QPushButton *browse_iso = new QPushButton( tr( "Browse…" ) );
	iso_row->addWidget( Edit_Iso );
	iso_row->addWidget( browse_iso );
	lay->addLayout( iso_row );

	QPushButton *analyze = new QPushButton( tr( "Analyze" ) );
	lay->addWidget( analyze );

	Report_View = new QPlainTextEdit();
	Report_View->setReadOnly( true );
	Report_View->setPlaceholderText( tr( "The recovery report appears here before any VM is created." ) );
	lay->addWidget( Report_View, 2 );

	QHBoxLayout *bottom = new QHBoxLayout();
	Button_Create = new QPushButton( tr( "Create Recovery VM" ) );
	Button_Create->setEnabled( false );
	QPushButton *close_button = new QPushButton( tr( "Close" ) );
	bottom->addStretch( 1 );
	bottom->addWidget( Button_Create );
	bottom->addWidget( close_button );
	lay->addLayout( bottom );

	connect( add_files, SIGNAL(clicked()), this, SLOT(Add_Files()) );
	connect( add_folder, SIGNAL(clicked()), this, SLOT(Add_Folder()) );
	connect( add_phys, SIGNAL(clicked()), this, SLOT(Add_Physical()) );
	connect( remove, SIGNAL(clicked()), this, SLOT(Remove_Selected()) );
	connect( browse_iso, SIGNAL(clicked()), this, SLOT(Browse_Iso()) );
	connect( analyze, SIGNAL(clicked()), this, SLOT(Analyze()) );
	connect( Button_Create, SIGNAL(clicked()), this, SLOT(Create_Vm()) );
	connect( close_button, SIGNAL(clicked()), this, SLOT(reject()) );
	connect( Edit_Iso, SIGNAL(textChanged(QString)), this, SLOT(Invalidate_Report()) );
}

Virtual_Machine *Storage_Recovery_Window::Get_VM() const
{
	return Created_VM;
}

void Storage_Recovery_Window::Invalidate_Report()
{
	Report_Text.clear();
	Probes.clear();
	Pools.clear();
	Report_View->clear();
	Button_Create->setEnabled( false );
}

void Storage_Recovery_Window::Refresh_Source_List()
{
	Source_List->clear();
	for( int i = 0; i < Source_Paths.size(); ++i )
		Source_List->addItem( Source_Paths[i] );
}

bool Storage_Recovery_Window::Already_Listed( const QString &path ) const
{
	for( int i = 0; i < Source_Paths.size(); ++i )
	{
		if( Source_Paths[i] == path )
			return true;
	}
	return false;
}

void Storage_Recovery_Window::Remember_Path( const QString &path )
{
	if( path.isEmpty() || Already_Listed( path ) )
		return;
	if( Path_Is_Host_Boot_Disk( path ) )
	{
		QMessageBox::warning( this, tr( "Storage Recovery" ),
			tr( "The host boot disk is not offered as a recovery source." ) );
		return;
	}
	Source_Paths << path;
}

void Storage_Recovery_Window::Add_Files()
{
	const QStringList paths = QFileDialog::getOpenFileNames( this, tr( "Disk images" ), QString(),
		tr( "Disk images (*.vmdk *.qcow2 *.raw *.img *.vhdx *.vdi);;All files (*)" ) );
	for( int i = 0; i < paths.size(); ++i )
		Remember_Path( paths[i] );
	Refresh_Source_List();
	Invalidate_Report();
	Refresh_Source_List();
}

void Storage_Recovery_Window::Add_Folder()
{
	const QString folder = QFileDialog::getExistingDirectory( this, tr( "Folder of disk images" ) );
	if( folder.isEmpty() )
		return;
	const QFileInfoList files = QDir( folder ).entryInfoList( QDir::Files, QDir::Name );
	for( int i = 0; i < files.size(); ++i )
	{
		if( ! Is_Disk_Image_Name( files[i].fileName() ) )
			continue;
		Remember_Path( files[i].absoluteFilePath() );
	}
	Refresh_Source_List();
	Invalidate_Report();
	Refresh_Source_List();
}

void Storage_Recovery_Window::Add_Physical()
{
	QString note;
	const QStringList disks = List_Recovery_Physical_Disks( &note );
	if( disks.isEmpty() )
	{
		QMessageBox::information( this, tr( "Physical disks" ),
			note.isEmpty() ? tr( "No physical disks were found." ) : note );
		return;
	}
	QDialog picker( this );
	picker.setWindowTitle( tr( "Physical disks" ) );
	QVBoxLayout *lay = new QVBoxLayout( &picker );
	lay->addWidget( new QLabel( tr( "The host boot disk is not listed." ) ) );
	QListWidget *list = new QListWidget();
	list->setSelectionMode( QAbstractItemView::MultiSelection );
	for( int i = 0; i < disks.size(); ++i )
		list->addItem( disks[i] );
	lay->addWidget( list );
	QHBoxLayout *row = new QHBoxLayout();
	QPushButton *ok = new QPushButton( tr( "Add" ) );
	QPushButton *cancel = new QPushButton( tr( "Cancel" ) );
	row->addStretch( 1 );
	row->addWidget( ok );
	row->addWidget( cancel );
	lay->addLayout( row );
	connect( ok, SIGNAL(clicked()), &picker, SLOT(accept()) );
	connect( cancel, SIGNAL(clicked()), &picker, SLOT(reject()) );
	if( picker.exec() != QDialog::Accepted )
		return;
	const QList<QListWidgetItem *> picked = list->selectedItems();
	for( int i = 0; i < picked.size(); ++i )
		Remember_Path( picked[i]->text() );
	Refresh_Source_List();
	Invalidate_Report();
	Refresh_Source_List();
}

void Storage_Recovery_Window::Remove_Selected()
{
	const int row = Source_List->currentRow();
	if( row < 0 || row >= Source_Paths.size() )
		return;
	Source_Paths.removeAt( row );
	Refresh_Source_List();
	Invalidate_Report();
	Refresh_Source_List();
}

void Storage_Recovery_Window::Browse_Iso()
{
	const QString path = QFileDialog::getOpenFileName( this, tr( "Boot ISO" ), QString(),
		tr( "ISO images (*.iso);;All files (*)" ) );
	if( ! path.isEmpty() )
		Edit_Iso->setText( path );
}

void Storage_Recovery_Window::Analyze()
{
	if( Source_Paths.isEmpty() )
	{
		Invalidate_Report();
		return;
	}
	QList<Disk_Probe> probed;
	for( int i = 0; i < Source_Paths.size(); ++i )
		probed << Probe_Disk( Source_Paths[i] );
	Probes = probed;
	Pools = Group_Probable_Pools( Probes );
	Report_Facts facts;
	facts.aqemu_version = QStringLiteral( CURRENT_AQEMU_VERSION );
	facts.qemu_version = Qemu_Version_Line();
	facts.sources = Probes;
	facts.pools = Pools;
	facts.guest_os = QStringLiteral( "TrueNAS SCALE" );
	facts.machine = QStringLiteral( "q35" );
	facts.controller = QStringLiteral( "VirtIO-SCSI" );
	int sector = Sector_Size_From_Pools( Pools );
	for( int i = 0; i < Probes.size(); ++i )
	{
		if( Four_Kn_Sector( Probes[i].descriptor_logical, Probes[i].descriptor_physical, Probes[i].ashift ) == 4096 )
			sector = 4096;
	}
	facts.sector_size = sector;
	facts.boot_media = Edit_Iso->text().trimmed();
	facts.host_boot_disk_excluded = true;
	facts.checkpoint_text = QStringLiteral( "not created yet\n" );
	Report_Text = Build_Recovery_Report( facts );
	Report_View->setPlainText( Report_Text );
	Button_Create->setEnabled( Recovery_Wizard_Can_Continue( Probes ) );
}

void Storage_Recovery_Window::Create_Vm()
{
	if( Report_Text.isEmpty() || Probes.isEmpty() )
		return;

	QString name = Edit_Name->text().trimmed();
	name.replace( QLatin1Char( '/' ), QLatin1Char( '-' ) );
	name.replace( QLatin1Char( '\\' ), QLatin1Char( '-' ) );
	if( name.isEmpty() )
		name = QStringLiteral( "Storage Recovery" );

	const QString vm_dir = Settings.value( "VM_Directory", QDir::homePath() + "/.aqemu/" ).toString();
	QDir().mkpath( vm_dir );
	QString vm_file = QDir( vm_dir ).filePath( name + QStringLiteral( ".aqemu" ) );
	int suffix = 2;
	while( QFileInfo::exists( vm_file ) )
	{
		vm_file = QDir( vm_dir ).filePath( QStringLiteral( "%1 %2.aqemu" ).arg( name ).arg( suffix ) );
		++suffix;
	}
	const QString clean_vm_path = QDir::cleanPath( vm_file );
	if( ! clean_vm_path.startsWith( QDir::cleanPath( vm_dir ) ) )
	{
		QMessageBox::critical( this, tr( "Path Error" ), tr( "VM configuration path escapes the VM directory." ) );
		return;
	}
	const QString final_name = QFileInfo( vm_file ).completeBaseName();

	Virtual_Machine *vm = new Virtual_Machine();
	vm->Set_Machine_Name( final_name );
	vm->Set_Computer_Type( QStringLiteral( "qemu-system-x86_64" ) );
	vm->Set_Machine_Type( QStringLiteral( "q35" ) );
	vm->Update_Current_Emulator_Devices();
	vm->Set_SMP_CPU_Count( 2 );
	vm->Set_Memory_Size( 8192 );
	vm->Set_Use_Network( true );
	VM_Net_Card net;
	net.Set_Net_Mode( VM::Net_Mode_Usermode );
	net.Set_Card_Model( QStringLiteral( "virtio-net-pci" ) );
	vm->Add_Network_Card( net );

	const int vm_sector = Sector_Size_From_Pools( Pools );
	QList<VM_Native_Storage_Device> extra;
	for( int i = 0; i < Probes.size(); ++i )
	{
		const Disk_Probe &probe = Probes[i];
		VM_Native_Storage_Device native;
		native.Use_File_Path( true );
		native.Set_File_Path( probe.path );
		native.Use_Interface( true );
		native.Set_Interface( VM::DI_Virtio_SCSI );
		native.Use_Media( true );
		native.Set_Media( VM::DM_Disk );
		native.Set_Read_Only( true );
		int sector = Four_Kn_Sector( probe.descriptor_logical, probe.descriptor_physical, probe.ashift );
		if( sector == 0 && probe.zfs_labels )
			sector = vm_sector;
		if( sector == 4096 )
		{
			native.Use_Block_Size( true );
			native.Set_Logical_Block_Size( 4096 );
			native.Set_Physical_Block_Size( 4096 );
		}
		if( probe.zfs_labels && probe.disk_guid != 0 )
			native.Set_Disk_Serial( Guid_Serial( probe.disk_guid ) );
		else
			native.Set_Disk_Serial( QStringLiteral( "aqemu-disk-%1" ).arg( i + 1 ) );

		if( i == 0 )
		{
			VM_HDD hda( true, probe.path );
			hda.Set_Native_Device( native );
			vm->Set_HDA( hda );
		}
		else
		{
			extra << native;
		}
	}
	const QString iso = Edit_Iso->text().trimmed();
	if( ! iso.isEmpty() )
	{
		VM_Native_Storage_Device cd;
		cd.Use_File_Path( true );
		cd.Set_File_Path( iso );
		cd.Use_Interface( true );
		cd.Set_Interface( VM::DI_Virtio_SCSI );
		cd.Use_Media( true );
		cd.Set_Media( VM::DM_CD_ROM );
		cd.Set_Read_Only( true );
		extra << cd;
	}
	if( ! extra.isEmpty() )
		vm->Set_Storage_Devices_List( extra );

	vm->Set_VM_XML_File_Path( vm_file );
	if( ! vm->Save_VM( vm_file ) )
	{
		delete vm;
		QMessageBox::critical( this, tr( "Save Error" ),
			tr( "Failed to save the recovery VM configuration." ) );
		return;
	}
	QFile report( Report_File( vm ) );
	if( report.open( QIODevice::WriteOnly ) )
		report.write( Report_Text.toUtf8() );

	Created_VM = vm;
	accept();
}

Recovery_Checkpoint_Window::Recovery_Checkpoint_Window( Virtual_Machine *vm, QWidget *parent )
	: QDialog( parent ), VM( vm )
{
	setWindowTitle( tr( "Recovery Checkpoint" ) );
	resize( 520, 280 );
	QVBoxLayout *lay = new QVBoxLayout( this );
	QLabel *intro = new QLabel( tr(
		"A recovery checkpoint puts a qcow2 overlay in front of each source disk. "
		"Original source disks stay the backing files and are not modified." ) );
	intro->setWordWrap( true );
	lay->addWidget( intro );

	QPushButton *enable = new QPushButton( tr( "Enable Recovery Checkpoint" ) );
	QPushButton *discard = new QPushButton( tr( "Discard" ) );
	QPushButton *keep = new QPushButton( tr( "Keep" ) );
	QLabel *subtitle = new QLabel( Keep_Button_Subtitle() );
	subtitle->setWordWrap( true );
	QPushButton *close_button = new QPushButton( tr( "Close" ) );
	lay->addWidget( enable );
	lay->addWidget( discard );
	lay->addWidget( keep );
	lay->addWidget( subtitle );
	lay->addStretch( 1 );
	lay->addWidget( close_button );

	connect( enable, SIGNAL(clicked()), this, SLOT(Enable_Checkpoint()) );
	connect( discard, SIGNAL(clicked()), this, SLOT(Discard_Checkpoint()) );
	connect( keep, SIGNAL(clicked()), this, SLOT(Keep_This_Checkpoint()) );
	connect( close_button, SIGNAL(clicked()), this, SLOT(accept()) );
}

QString Recovery_Checkpoint_Window::Attempt_Dir() const
{
	return Attempt_Directory( VM );
}

QString Recovery_Checkpoint_Window::Manifest_Path() const
{
	return QDir( Attempt_Dir() ).filePath( QStringLiteral( "manifest.json" ) );
}

QString Recovery_Checkpoint_Window::Report_Path() const
{
	return Report_File( VM );
}

void Recovery_Checkpoint_Window::Note_Checkpoint( const QString &text )
{
	QString error;
	Update_Report_Checkpoint( Report_Path(), text, &error );
}

void Recovery_Checkpoint_Window::Enable_Checkpoint()
{
	const QList<Disk_Slot> disk_slots = Collect_File_Disks( VM );
	if( disk_slots.isEmpty() )
	{
		QMessageBox::warning( this, tr( "Recovery Checkpoint" ),
			tr( "This VM has no file-backed disks to checkpoint." ) );
		return;
	}
	QList<Recovery_Disk> sources;
	QList<Drive_Binding> original;
	for( int i = 0; i < disk_slots.size(); ++i )
	{
		const QString path = disk_slots[i].device.Get_File_Path();
		const Disk_Probe probe = Probe_Disk( path );
		Recovery_Disk disk;
		disk.source_path = path;
		disk.source_identity = Source_Identity( probe );
		disk.source_size = probe.size;
		disk.source_format = Disk_Format_For_Overlay( path );
		disk.was_read_only = disk_slots[i].device.Get_Read_Only();
		sources << disk;
		Drive_Binding binding;
		binding.path = path;
		binding.read_only = disk_slots[i].device.Get_Read_Only();
		original << binding;
	}

	const QString qemu_img = QStandardPaths::findExecutable( QStringLiteral( "qemu-img" ) );
	Overlay_Writer writer = [qemu_img]( const QString &source, const QString &format,
		const QString &overlay, QString *error ) -> bool {
		return Create_Qcow2_Backing_Overlay( qemu_img, source, format, overlay, error );
	};
	const Checkpoint_Outcome outcome = Create_Checkpoint( Attempt_Dir(), sources, writer );
	if( ! outcome.ok )
	{
		QMessageBox box( QMessageBox::Warning, tr( "Recovery Checkpoint" ), Checkpoint_Failure_Text(),
			QMessageBox::Ok, this );
		if( ! outcome.detail.isEmpty() )
			box.setDetailedText( outcome.detail );
		box.exec();
		return;
	}

	const QList<Drive_Binding> wired = Apply_Checkpoint_Result( original, outcome );
	Apply_Bindings( VM, disk_slots, wired );
	if( ! VM->Save_VM() )
	{
		Apply_Bindings( VM, disk_slots, original );
		QString error;
		::Discard_Checkpoint( Manifest_Path(), &error );
		QMessageBox::warning( this, tr( "Recovery Checkpoint" ), Checkpoint_Failure_Text() );
		return;
	}
	Note_Checkpoint( QStringLiteral( "checkpoint id: %1\ncreated: %2\nVM drives point at the recovery overlays. Original source disks were not modified.\n" )
		.arg( outcome.manifest.checkpoint_id, outcome.manifest.created ) );
	QMessageBox::information( this, tr( "Recovery Checkpoint" ),
		tr( "Recovery checkpoint enabled. VM drives point at the overlays." ) );
}

void Recovery_Checkpoint_Window::Discard_Checkpoint()
{
	Checkpoint_Manifest manifest;
	QString error;
	if( ! Load_Checkpoint_Manifest( Manifest_Path(), &manifest, &error ) )
	{
		QMessageBox::information( this, tr( "Recovery Checkpoint" ),
			tr( "No recovery checkpoint is recorded for this VM." ) );
		return;
	}
	if( QMessageBox::question( this, tr( "Discard checkpoint" ), Discard_Confirm_Text(),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) != QMessageBox::Yes )
		return;

	const QList<Disk_Slot> disk_slots = Collect_File_Disks( VM );
	const QList<Drive_Binding> restored = Bindings_After_Discard( manifest );
	if( disk_slots.size() != restored.size() )
	{
		QMessageBox::warning( this, tr( "Recovery Checkpoint" ),
			tr( "The VM disks no longer match this checkpoint, so it was left in place." ) );
		return;
	}
	QList<Disk_Slot> rewind = disk_slots;
	for( int i = 0; i < rewind.size(); ++i )
	{
		rewind[i].device.Set_File_Path( restored[i].path );
		rewind[i].device.Set_Read_Only( restored[i].read_only );
	}
	Apply_Bindings( VM, rewind, restored );
	if( ! VM->Save_VM() )
	{
		QMessageBox::warning( this, tr( "Recovery Checkpoint" ),
			tr( "The VM configuration could not be saved. The checkpoint overlays were left in place." ) );
		return;
	}
	if( ! ::Discard_Checkpoint( Manifest_Path(), &error ) )
	{
		QMessageBox::warning( this, tr( "Recovery Checkpoint" ),
			error.isEmpty() ? tr( "The checkpoint files could not be removed." ) : error );
		return;
	}
	Note_Checkpoint( QStringLiteral( "not created yet\ncheckpoint discarded. Original source disks were not modified.\n" ) );
}

void Recovery_Checkpoint_Window::Keep_This_Checkpoint()
{
	Checkpoint_Manifest manifest;
	QString error;
	if( ! Load_Checkpoint_Manifest( Manifest_Path(), &manifest, &error ) )
	{
		QMessageBox::information( this, tr( "Recovery Checkpoint" ),
			tr( "No recovery checkpoint is recorded for this VM." ) );
		return;
	}
	const QList<Disk_Slot> disk_slots = Collect_File_Disks( VM );
	const QList<Drive_Binding> kept = Bindings_After_Keep( manifest );
	if( disk_slots.size() != kept.size() )
	{
		QMessageBox::warning( this, tr( "Recovery Checkpoint" ),
			tr( "The VM disks no longer match this checkpoint." ) );
		return;
	}
	Apply_Bindings( VM, disk_slots, kept );
	if( ! VM->Save_VM() )
	{
		QMessageBox::warning( this, tr( "Recovery Checkpoint" ),
			tr( "The VM configuration could not be saved." ) );
		return;
	}
	::Keep_Checkpoint( Manifest_Path(), &error );
	Note_Checkpoint( QStringLiteral( "kept. Overlays are this VM’s disks. Original source disks remain unchanged.\n" ) );
}
