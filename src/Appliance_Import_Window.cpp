/****************************************************************************
**
** Appliance Import Window (OVA / OVF)
**
****************************************************************************/

#include "Appliance_Import_Window.h"
#include "Utils.h"
#include "System_Info.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QApplication>
#include <QProcess>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>

Appliance_Import_Window::Appliance_Import_Window( QWidget *parent )
	: QDialog( parent )
{
	setWindowTitle( tr( "Import Virtual Appliance (OVA / OVF)" ) );
	resize( 680, 560 );
	Setup_UI();
	Populate_OS_List();
}

void Appliance_Import_Window::Setup_UI()
{
	QVBoxLayout *main_lay = new QVBoxLayout( this );

	// 1. Source Appliance File
	QGroupBox *gb_source = new QGroupBox( tr( "Appliance Source" ), this );
	QHBoxLayout *src_lay = new QHBoxLayout( gb_source );
	Edit_Appliance_Path = new QLineEdit( gb_source );
	Edit_Appliance_Path->setPlaceholderText( tr( "Path to .ova archive or .ovf descriptor" ) );
	TB_Browse_Appliance = new QToolButton( gb_source );
	TB_Browse_Appliance->setText( QStringLiteral( "..." ) );
	TB_Browse_Appliance->setToolTip( tr( "Browse for OVA or OVF file" ) );
	src_lay->addWidget( Edit_Appliance_Path, 1 );
	src_lay->addWidget( TB_Browse_Appliance );
	main_lay->addWidget( gb_source );

	// 2. Virtual System Settings
	QGroupBox *gb_settings = new QGroupBox( tr( "Appliance Settings" ), this );
	QGridLayout *grid = new QGridLayout( gb_settings );

	grid->addWidget( new QLabel( tr( "Virtual Machine Name:" ), gb_settings ), 0, 0 );
	Edit_VM_Name = new QLineEdit( gb_settings );
	grid->addWidget( Edit_VM_Name, 0, 1 );

	grid->addWidget( new QLabel( tr( "Guest Operating System:" ), gb_settings ), 1, 0 );
	CB_Guest_OS = new QComboBox( gb_settings );
	CB_Guest_OS->setEditable( true );
	grid->addWidget( CB_Guest_OS, 1, 1 );

	grid->addWidget( new QLabel( tr( "vCPU Cores:" ), gb_settings ), 2, 0 );
	SB_CPU = new QSpinBox( gb_settings );
	SB_CPU->setRange( 1, 64 );
	SB_CPU->setValue( 2 );
	grid->addWidget( SB_CPU, 2, 1 );

	grid->addWidget( new QLabel( tr( "Memory (RAM in MB):" ), gb_settings ), 3, 0 );
	SB_RAM = new QSpinBox( gb_settings );
	SB_RAM->setRange( 128, 131072 );
	SB_RAM->setSingleStep( 512 );
	SB_RAM->setValue( 2048 );
	grid->addWidget( SB_RAM, 3, 1 );

	grid->addWidget( new QLabel( tr( "Target Disk Format:" ), gb_settings ), 4, 0 );
	CB_Disk_Format = new QComboBox( gb_settings );
	CB_Disk_Format->addItem( tr( "QCOW2 (Recommended - Dynamic & Compressed)" ), QStringLiteral( "qcow2" ) );
	CB_Disk_Format->addItem( tr( "QCOW2 with 4K Cluster Alignment (TrueNAS / 4Kn / ZFS)" ), QStringLiteral( "qcow2_4k" ) );
	CB_Disk_Format->addItem( tr( "Keep Original Format (.vmdk)" ), QStringLiteral( "vmdk" ) );
	grid->addWidget( CB_Disk_Format, 4, 1 );

	main_lay->addWidget( gb_settings );

	// 3. Storage Disks Table
	QGroupBox *gb_disks = new QGroupBox( tr( "Appliance Storage Disks" ), this );
	QVBoxLayout *disk_lay = new QVBoxLayout( gb_disks );
	Table_Disks = new QTableWidget( gb_disks );
	Table_Disks->setColumnCount( 3 );
	Table_Disks->setHorizontalHeaderLabels( QStringList() << tr( "Disk Name" ) << tr( "Capacity" ) << tr( "Format" ) );
	Table_Disks->horizontalHeader()->setStretchLastSection( true );
	Table_Disks->setSelectionBehavior( QAbstractItemView::SelectRows );
	Table_Disks->setEditTriggers( QAbstractItemView::NoEditTriggers );
	disk_lay->addWidget( Table_Disks );
	main_lay->addWidget( gb_disks, 1 );

	// 4. Destination Directory
	QGroupBox *gb_dest = new QGroupBox( tr( "Destination Storage" ), this );
	QHBoxLayout *dest_lay = new QHBoxLayout( gb_dest );
	Edit_Dest_Dir = new QLineEdit( gb_dest );
	const QString default_dir = Settings.value( "VM_Directory", QDir::homePath() + "/.aqemu/" ).toString();
	Edit_Dest_Dir->setText( QDir::toNativeSeparators( default_dir ) );
	TB_Browse_Dest = new QToolButton( gb_dest );
	TB_Browse_Dest->setText( QStringLiteral( "..." ) );
	dest_lay->addWidget( Edit_Dest_Dir, 1 );
	dest_lay->addWidget( TB_Browse_Dest );
	main_lay->addWidget( gb_dest );

	// 5. Progress and Status
	Progress_Bar = new QProgressBar( this );
	Progress_Bar->setRange( 0, 100 );
	Progress_Bar->setValue( 0 );
	Progress_Bar->setVisible( false );
	main_lay->addWidget( Progress_Bar );

	Label_Status = new QLabel( this );
	Label_Status->setStyleSheet( QStringLiteral( "color: #94a3b8; font-size: 0.9em;" ) );
	main_lay->addWidget( Label_Status );

	// 6. Action Buttons
	QHBoxLayout *btn_lay = new QHBoxLayout();
	btn_lay->addStretch();
	Btn_Cancel = new QPushButton( tr( "Cancel" ), this );
	Btn_Import = new QPushButton( tr( "Import Appliance" ), this );
	Btn_Import->setDefault( true );
	Btn_Import->setEnabled( false );
	btn_lay->addWidget( Btn_Cancel );
	btn_lay->addWidget( Btn_Import );
	main_lay->addLayout( btn_lay );

	// Connections
	connect( TB_Browse_Appliance, &QToolButton::clicked, this, &Appliance_Import_Window::On_Browse_Appliance );
	connect( TB_Browse_Dest, &QToolButton::clicked, this, &Appliance_Import_Window::On_Browse_Destination );
	connect( Edit_Appliance_Path, &QLineEdit::textChanged, this, &Appliance_Import_Window::On_Appliance_Path_Changed );
	connect( Btn_Import, &QPushButton::clicked, this, &Appliance_Import_Window::On_Start_Import );
	connect( Btn_Cancel, &QPushButton::clicked, this, &QDialog::reject );
}

void Appliance_Import_Window::Populate_OS_List()
{
	CB_Guest_OS->clear();
	const QString json_path = Settings.value( "AQEMU_Data_Folder", "" ).toString() + "/wizard_trees.json";
	QFile f( json_path.isEmpty() ? QStringLiteral( ":/wizard_trees.json" ) : json_path );
	if( f.open( QIODevice::ReadOnly ) )
	{
		const QJsonDocument doc = QJsonDocument::fromJson( f.readAll() );
		const QJsonObject os_tree = doc.object().value( QStringLiteral( "operating_systems" ) ).toObject();
		for( auto cat = os_tree.constBegin(); cat != os_tree.constEnd(); ++cat )
		{
			const QJsonArray arr = cat.value().toArray();
			for( const QJsonValue &v : arr )
			{
				if( CB_Guest_OS->findText( v.toString() ) < 0 )
					CB_Guest_OS->addItem( v.toString() );
			}
		}
	}

	if( CB_Guest_OS->count() == 0 )
	{
		CB_Guest_OS->addItems( QStringList()
			<< QStringLiteral( "Ubuntu (64-bit)" )
			<< QStringLiteral( "Debian (64-bit)" )
			<< QStringLiteral( "TrueNAS CORE" )
			<< QStringLiteral( "TrueNAS SCALE" )
			<< QStringLiteral( "FreeBSD (64-bit)" )
			<< QStringLiteral( "Windows 11" )
			<< QStringLiteral( "Windows 10 (64-bit)" )
			<< QStringLiteral( "Generic Linux (64-bit)" ) );
	}
}

void Appliance_Import_Window::Set_Appliance_Path( const QString &path )
{
	Edit_Appliance_Path->setText( QDir::toNativeSeparators( path ) );
}

void Appliance_Import_Window::On_Browse_Appliance()
{
	const QString filter = tr( "Virtual Appliances (*.ova *.ovf);;OVA Archive (*.ova);;OVF Descriptor (*.ovf);;All Files (*)" );
	const QString path = QFileDialog::getOpenFileName( this, tr( "Select Virtual Appliance" ),
	                                                   QDir::homePath(), filter );
	if( ! path.isEmpty() )
		Set_Appliance_Path( path );
}

void Appliance_Import_Window::On_Browse_Destination()
{
	const QString dir = QFileDialog::getExistingDirectory( this, tr( "Select Destination Storage Folder" ),
	                                                       Edit_Dest_Dir->text() );
	if( ! dir.isEmpty() )
		Edit_Dest_Dir->setText( QDir::toNativeSeparators( dir ) );
}

void Appliance_Import_Window::On_Appliance_Path_Changed( const QString &path )
{
	const QString trimmed = path.trimmed();
	if( ! QFile::exists( trimmed ) )
	{
		Btn_Import->setEnabled( false );
		Label_Status->setText( tr( "Please select an existing .ova or .ovf appliance." ) );
		return;
	}

	Label_Status->setText( tr( "Reading appliance metadata..." ) );
	QApplication::processEvents();

	QString err;
	QString ovf_to_parse = trimmed;
	QTemporaryDir temp_scan;

	if( OVF_Parser::Is_OVA_Archive( trimmed ) )
	{
		// Extract OVA to temp to read the OVF
		QString out_ovf;
		QStringList out_files;
		temp_scan.setAutoRemove( false ); // Keep for inspection
		if( ! OVF_Parser::Extract_OVA( trimmed, temp_scan.path(), out_ovf, out_files, err ) )
		{
			Label_Status->setText( tr( "Failed to inspect OVA: %1" ).arg( err ) );
			Btn_Import->setEnabled( false );
			return;
		}
		ovf_to_parse = out_ovf;
	}

	if( ! OVF_Parser::Parse_OVF( ovf_to_parse, Current_Appliance, err ) )
	{
		Label_Status->setText( tr( "Failed to parse OVF: %1" ).arg( err ) );
		Btn_Import->setEnabled( false );
		return;
	}

	Current_Appliance.source_archive_path = trimmed;
	Update_Appliance_Details( Current_Appliance );
	Btn_Import->setEnabled( true );
	Label_Status->setText( tr( "Appliance ready to import." ) );
}

void Appliance_Import_Window::Update_Appliance_Details( const OVF_Appliance &appliance )
{
	Edit_VM_Name->setText( appliance.name );
	SB_CPU->setValue( qBound( 1, appliance.cpu_count, 64 ) );
	SB_RAM->setValue( qBound( 128, appliance.memory_mb, 131072 ) );

	if( ! appliance.aqemu_profile_name.isEmpty() )
	{
		const int idx = CB_Guest_OS->findText( appliance.aqemu_profile_name );
		if( idx >= 0 )
			CB_Guest_OS->setCurrentIndex( idx );
		else
			CB_Guest_OS->setEditText( appliance.aqemu_profile_name );
	}

	Table_Disks->setRowCount( appliance.disks.size() );
	for( int i = 0; i < appliance.disks.size(); ++i )
	{
		const OVF_Disk &d = appliance.disks[i];
		const QString name = d.href.isEmpty() ? d.disk_id : d.href;
		Table_Disks->setItem( i, 0, new QTableWidgetItem( name ) );

		const double gb = static_cast<double>( d.capacity_bytes ) / ( 1024.0 * 1024.0 * 1024.0 );
		const QString cap_str = gb > 0 ? QStringLiteral( "%1 GB" ).arg( gb, 0, 'f', 1 ) : tr( "(dynamic)" );
		Table_Disks->setItem( i, 1, new QTableWidgetItem( cap_str ) );

		Table_Disks->setItem( i, 2, new QTableWidgetItem( d.format.isEmpty() ? QStringLiteral( "VMDK" ) : d.format ) );
	}
}

void Appliance_Import_Window::On_Start_Import()
{
	if( Is_Working )
		return;

	Is_Working = true;
	Btn_Import->setEnabled( false );
	Btn_Cancel->setEnabled( false );
	Progress_Bar->setVisible( true );
	Progress_Bar->setValue( 0 );

	const bool ok = Execute_Extraction_And_Conversion();

	Is_Working = false;
	Btn_Cancel->setEnabled( true );
	Progress_Bar->setVisible( false );

	if( ok )
	{
		QMessageBox::information( this, tr( "Appliance Imported" ),
		                          tr( "Virtual Appliance '%1' was successfully imported into AQEMU!" )
		                          .arg( Edit_VM_Name->text() ) );
		accept();
	}
	else
	{
		Btn_Import->setEnabled( true );
	}
}

bool Appliance_Import_Window::Execute_Extraction_And_Conversion()
{
	const QString vm_name = Edit_VM_Name->text().trimmed().isEmpty() ? QStringLiteral( "Imported_Appliance" ) : Edit_VM_Name->text().trimmed();
	const QString dest_folder = QDir( Edit_Dest_Dir->text() ).filePath( vm_name );
	QDir().mkpath( dest_folder );

	QString ovf_path;
	QStringList extracted_files;
	QString err;

	if( OVF_Parser::Is_OVA_Archive( Current_Appliance.source_archive_path ) )
	{
		Label_Status->setText( tr( "Extracting OVA archive..." ) );
		auto prog_cb = [this]( int pct, const QString &status ) {
			Progress_Bar->setValue( pct / 2 ); // 0% - 50% for extraction
			Label_Status->setText( status );
			QApplication::processEvents();
		};

		if( ! OVF_Parser::Extract_OVA( Current_Appliance.source_archive_path, dest_folder,
		                               ovf_path, extracted_files, err, prog_cb ) )
		{
			QMessageBox::critical( this, tr( "Extraction Error" ), tr( "Failed to unpack OVA archive:\n%1" ).arg( err ) );
			return false;
		}

		if( ! OVF_Parser::Parse_OVF( ovf_path, Current_Appliance, err ) )
		{
			QMessageBox::critical( this, tr( "OVF Error" ), tr( "Failed to parse extracted OVF descriptor:\n%1" ).arg( err ) );
			return false;
		}
	}
	else
	{
		ovf_path = Current_Appliance.ovf_file_path;
	}

	const QString target_fmt = CB_Disk_Format->currentData().toString();
	const bool convert_to_qcow2 = target_fmt.startsWith( QStringLiteral( "qcow2" ) );
	const bool align_4k = ( target_fmt == QStringLiteral( "qcow2_4k" ) );
	const QString qemu_img = Get_QEMU_IMG_Path();

	QStringList final_disk_paths;

	// Process Disks
	const int disk_count = Current_Appliance.disks.size();
	for( int i = 0; i < disk_count; ++i )
	{
		OVF_Disk &d = Current_Appliance.disks[i];
		const QString src_disk = d.local_extracted_path.isEmpty()
			? QDir( QFileInfo( ovf_path ).absolutePath() ).filePath( d.href )
			: d.local_extracted_path;

		if( ! QFile::exists( src_disk ) )
		{
			QMessageBox::warning( this, tr( "Disk Not Found" ),
			                      tr( "Could not locate virtual disk file:\n%1" ).arg( src_disk ) );
			continue;
		}

		if( convert_to_qcow2 )
		{
			const QString base_name = QFileInfo( src_disk ).completeBaseName();
			const QString out_qcow2 = QDir( dest_folder ).filePath( base_name + QStringLiteral( ".qcow2" ) );

			Label_Status->setText( tr( "Converting disk %1 to QCOW2..." ).arg( base_name ) );
			Progress_Bar->setValue( 50 + ( ( i * 50 ) / qMax( 1, disk_count ) ) );
			QApplication::processEvents();

			QStringList args;
			args << QStringLiteral( "convert" ) << QStringLiteral( "-O" ) << QStringLiteral( "qcow2" );
			if( align_4k )
				args << QStringLiteral( "-o" ) << QStringLiteral( "cluster_size=4k" );
			else
				args << QStringLiteral( "-o" ) << QStringLiteral( "cluster_size=64k" );
			args << src_disk << out_qcow2;

			QProcess proc;
			proc.start( qemu_img, args );
			if( ! proc.waitForFinished( -1 ) || proc.exitCode() != 0 )
			{
				QMessageBox::critical( this, tr( "Conversion Error" ),
				                      tr( "qemu-img failed to convert '%1':\n%2" )
				                      .arg( src_disk, QString::fromLocal8Bit( proc.readAllStandardError() ) ) );
				return false;
			}

			// Clean up extracted raw/vmdk if it was inside the destination folder and converted
			if( src_disk != out_qcow2 && QFileInfo( src_disk ).absolutePath() == QFileInfo( out_qcow2 ).absolutePath() )
				QFile::remove( src_disk );

			final_disk_paths << out_qcow2;
		}
		else
		{
			final_disk_paths << src_disk;
		}
	}

	Progress_Bar->setValue( 95 );
	Label_Status->setText( tr( "Configuring Virtual Machine..." ) );
	QApplication::processEvents();

	// Create and register Virtual_Machine
	Virtual_Machine *vm = new Virtual_Machine();
	vm->Set_Machine_Name( vm_name );
	vm->Set_Computer_Type( CB_Guest_OS->currentText() );
	vm->Set_SMP_CPU_Count( SB_CPU->value() );
	vm->Set_Memory_Size( SB_RAM->value() );
	vm->Set_Machine_Type( QStringLiteral( "q35" ) );

	// Attach Disks
	if( ! final_disk_paths.isEmpty() )
	{
		VM_HDD hda( true, final_disk_paths.first() );
		VM_Native_Storage_Device native;
		native.Use_File_Path( true );
		native.Set_File_Path( final_disk_paths.first() );
		native.Use_Interface( true );
		native.Set_Interface( VM::DI_Virtio );
		if( align_4k || CB_Guest_OS->currentText().contains( QStringLiteral( "TrueNAS" ), Qt::CaseInsensitive ) )
		{
			native.Use_Block_Size( true );
			native.Set_Logical_Block_Size( 4096 );
			native.Set_Physical_Block_Size( 4096 );
		}
		hda.Set_Native_Device( native );
		vm->Set_HDA( hda );

		// Additional secondary disks
		QList<VM_Native_Storage_Device> extra_storage;
		for( int i = 1; i < final_disk_paths.size(); ++i )
		{
			VM_Native_Storage_Device sec;
			sec.Use_File_Path( true );
			sec.Set_File_Path( final_disk_paths[i] );
			sec.Use_Interface( true );
			sec.Set_Interface( VM::DI_Virtio );
			sec.Use_Media( true );
			sec.Set_Media( VM::DM_Disk );
			if( align_4k || CB_Guest_OS->currentText().contains( QStringLiteral( "TrueNAS" ), Qt::CaseInsensitive ) )
			{
				sec.Use_Block_Size( true );
				sec.Set_Logical_Block_Size( 4096 );
				sec.Set_Physical_Block_Size( 4096 );
			}
			extra_storage << sec;
		}
		if( ! extra_storage.isEmpty() )
			vm->Set_Storage_Devices_List( extra_storage );
	}

	// Save VM configuration XML
	const QString vm_file_path = QDir( Settings.value( "VM_Directory", QDir::homePath() + "/.aqemu/" ).toString() )
		.filePath( vm_name + QStringLiteral( ".aqemu" ) );
	vm->Set_VM_XML_File_Path( vm_file_path );
	vm->Save_VM( vm_file_path );

	Imported_VM = vm;
	Progress_Bar->setValue( 100 );
	Label_Status->setText( tr( "Import Complete!" ) );
	return true;
}
