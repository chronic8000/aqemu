/****************************************************************************
**
** Appliance Import Window (OVA / OVF)
**
****************************************************************************/

#include "Appliance_Import_Window.h"
#include "Storage_Recovery.h"
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
#include <QFile>
#include <QDir>

namespace {

void Add_OS_Leaves( QComboBox *combo, const QJsonValue &node )
{
	if( node.isArray() )
	{
		const QJsonArray arr = node.toArray();
		for( const QJsonValue &v : arr )
		{
			if( v.isString() )
			{
				const QString name = v.toString();
				if( ! name.isEmpty() && combo->findText( name ) < 0 )
					combo->addItem( name );
			}
			else if( v.isArray() || v.isObject() )
				Add_OS_Leaves( combo, v );
		}
	}
	else if( node.isObject() )
	{
		const QJsonObject obj = node.toObject();
		for( auto it = obj.constBegin(); it != obj.constEnd(); ++it )
			Add_OS_Leaves( combo, it.value() );
	}
}

QJsonObject Load_OS_Profiles()
{
	QFile f( QStringLiteral( ":/wizard_trees.json" ) );
	if( ! f.open( QIODevice::ReadOnly ) )
		return QJsonObject();
	const QJsonDocument doc = QJsonDocument::fromJson( f.readAll() );
	return doc.object().value( QStringLiteral( "os_profiles" ) ).toObject();
}

QString First_Profile_String( const QJsonValue &value )
{
	if( value.isString() )
		return value.toString();
	if( value.isArray() )
	{
		const QJsonArray arr = value.toArray();
		for( const QJsonValue &v : arr )
		{
			if( v.isString() && ! v.toString().isEmpty() )
				return v.toString();
		}
	}
	return QString();
}

QString Profile_To_QEMU_Binary( const QString &target )
{
	const QString t = target.trimmed().toLower();
	if( t.isEmpty() || t == QLatin1String( "x86_64" ) || t == QLatin1String( "amd64" ) )
		return QStringLiteral( "qemu-system-x86_64" );
	if( t == QLatin1String( "i386" ) || t == QLatin1String( "x86" ) )
		return QStringLiteral( "qemu-system-i386" );
	if( t.startsWith( QLatin1String( "qemu-system-" ) ) )
		return t;
	return QStringLiteral( "qemu-system-" ) + t;
}

void Apply_Profile_Sound( Virtual_Machine *vm, const QString &preset )
{
	if( preset.isEmpty() || ! vm )
		return;
	VM::Sound_Cards cards;
	if( preset == QLatin1String( "sb16" ) || preset.startsWith( QLatin1String( "sb16_" ) ) )
		cards.Audio_sb16 = true;
	else if( preset == QLatin1String( "es1370" ) || preset.startsWith( QLatin1String( "es1370" ) ) )
		cards.Audio_es1370 = true;
	else if( preset == QLatin1String( "ac97" ) )
		cards.Audio_AC97 = true;
	else if( preset == QLatin1String( "pcspk" ) )
		cards.Audio_PC_Speaker = true;
	else if( preset == QLatin1String( "virtio" ) )
		cards.Audio_VirtIO = true;
	else if( preset == QLatin1String( "none" ) )
	{
		vm->Set_Audio_Cards( cards );
		return;
	}
	else
		cards.Audio_HDA = true;
	vm->Set_Audio_Cards( cards );
}

}

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
	Table_Disks->setColumnCount( 5 );
	Table_Disks->setHorizontalHeaderLabels( QStringList()
		<< tr( "Disk Name" ) << tr( "Capacity" ) << tr( "Format" ) << tr( "Serial" ) << tr( "Sector" ) );
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
	QStringList paths;
	paths << QStringLiteral( ":/wizard_trees.json" );
	const QString data = Settings.value( "AQEMU_Data_Folder", "" ).toString();
	if( ! data.isEmpty() )
		paths << QDir( data ).filePath( QStringLiteral( "wizard_trees.json" ) );

	for( const QString &json_path : paths )
	{
		QFile f( json_path );
		if( ! f.open( QIODevice::ReadOnly ) )
			continue;
		const QJsonDocument doc = QJsonDocument::fromJson( f.readAll() );
		const QJsonObject os_tree = doc.object().value( QStringLiteral( "operating_systems" ) ).toObject();
		for( auto cat = os_tree.constBegin(); cat != os_tree.constEnd(); ++cat )
			Add_OS_Leaves( CB_Guest_OS, cat.value() );
		if( CB_Guest_OS->count() > 0 )
			break;
	}

	if( CB_Guest_OS->count() == 0 )
	{
		CB_Guest_OS->addItems( QStringList()
			<< QStringLiteral( "Ubuntu (64-bit)" )
			<< QStringLiteral( "Debian (64-bit)" )
			<< QStringLiteral( "TrueNAS CORE" )
			<< QStringLiteral( "TrueNAS SCALE" )
			<< QStringLiteral( "TrueNAS SCALE (ARM64)" )
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
		// Extract only the OVF descriptor to temp to inspect metadata (no disk extraction!)
		QString out_ovf;
		if( ! OVF_Parser::Extract_OVF_Only( trimmed, temp_scan.path(), out_ovf, err ) )
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
	const bool nas = Current_Appliance.aqemu_profile_name.contains( QLatin1String( "TrueNAS" ), Qt::CaseInsensitive )
		|| Current_Appliance.os_type_raw.contains( QLatin1String( "truenas" ), Qt::CaseInsensitive )
		|| Current_Appliance.os_type_raw.contains( QLatin1String( "freenas" ), Qt::CaseInsensitive );
	if( nas )
		Label_Status->setText( tr( "Appliance ready to import. Disk serials are kept. 4Kn follows disk geometry or ZFS labels." ) );
	else
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
		Table_Disks->setItem( i, 3, new QTableWidgetItem( d.serial.isEmpty() ? tr( "from disk label" ) : d.serial ) );
		QString sector = tr( "from disk" );
		if( d.logical_sector >= 4096 || d.physical_sector >= 4096 )
			sector = QStringLiteral( "4096" );
		else if( d.logical_sector == 512 || d.physical_sector == 512 )
			sector = QStringLiteral( "512" );
		Table_Disks->setItem( i, 4, new QTableWidgetItem( sector ) );
	}

	const bool nas = appliance.aqemu_profile_name.contains( QLatin1String( "TrueNAS" ), Qt::CaseInsensitive )
		|| appliance.aqemu_profile_name.contains( QLatin1String( "FreeNAS" ), Qt::CaseInsensitive )
		|| appliance.os_type_raw.contains( QLatin1String( "truenas" ), Qt::CaseInsensitive )
		|| appliance.os_type_raw.contains( QLatin1String( "freenas" ), Qt::CaseInsensitive );
	if( nas )
	{
		const int fmt = CB_Disk_Format->findData( QStringLiteral( "qcow2_4k" ) );
		if( fmt >= 0 )
			CB_Disk_Format->setCurrentIndex( fmt );
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
	QString vm_name = Edit_VM_Name->text().trimmed();
	if( vm_name.isEmpty() )
		vm_name = QStringLiteral( "Imported_Appliance" );
	// Sanitize VM name against path escape
	vm_name.remove( QLatin1Char( '/' ) );
	vm_name.remove( QLatin1Char( '\\' ) );
	vm_name.remove( QLatin1Char( ':' ) );
	vm_name.replace( QStringLiteral( ".." ), QStringLiteral( "_" ) );
	if( vm_name.isEmpty() )
		vm_name = QStringLiteral( "Imported_Appliance" );

	const QString base_dest = Edit_Dest_Dir->text().trimmed();
	const QString dest_folder = QDir( base_dest ).filePath( vm_name );
	const QString clean_dest = QDir::cleanPath( dest_folder );
	const QString clean_base = QDir::cleanPath( base_dest );
	if( ! clean_dest.startsWith( clean_base ) )
	{
		QMessageBox::critical( this, tr( "Invalid Destination" ), tr( "The destination folder escapes the configured directory." ) );
		return false;
	}
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

	// Verify all referenced disks exist before continuing (no partial/broken imports)
	QStringList missing_disks;
	for( const OVF_Disk &d : Current_Appliance.disks )
	{
		const QString src_disk = d.local_extracted_path.isEmpty()
			? QDir( QFileInfo( ovf_path ).absolutePath() ).filePath( d.href )
			: d.local_extracted_path;
		if( ! QFile::exists( src_disk ) )
			missing_disks << ( d.href.isEmpty() ? src_disk : d.href );
	}
	if( ! missing_disks.isEmpty() )
	{
		QMessageBox::critical( this, tr( "Missing Disks" ),
		                      tr( "The appliance references virtual disks that could not be found:\n%1\n\nImport aborted." )
		                      .arg( missing_disks.join( QStringLiteral( "\n" ) ) ) );
		return false;
	}

	const QString target_fmt = CB_Disk_Format->currentData().toString();
	const bool convert_to_qcow2 = target_fmt.startsWith( QStringLiteral( "qcow2" ) );
	const bool align_4k = ( target_fmt == QStringLiteral( "qcow2_4k" ) );
	const QString qemu_img = Get_QEMU_IMG_Path();

	QStringList final_disk_paths;
	QList<int> disk_sectors;
	QStringList disk_serials;
	QList<bool> disk_virtio_scsi;
	const QString guest_name = CB_Guest_OS->currentText();
	const bool nas = guest_name.contains( QLatin1String( "TrueNAS" ), Qt::CaseInsensitive )
		|| guest_name.contains( QLatin1String( "FreeNAS" ), Qt::CaseInsensitive )
		|| Current_Appliance.os_type_raw.contains( QLatin1String( "truenas" ), Qt::CaseInsensitive )
		|| Current_Appliance.os_type_raw.contains( QLatin1String( "freenas" ), Qt::CaseInsensitive );

	// Process Disks
	const int disk_count = Current_Appliance.disks.size();
	for( int i = 0; i < disk_count; ++i )
	{
		OVF_Disk &d = Current_Appliance.disks[i];
		const QString src_disk = d.local_extracted_path.isEmpty()
			? QDir( QFileInfo( ovf_path ).absolutePath() ).filePath( d.href )
			: d.local_extracted_path;

		const Disk_Probe probed = Probe_Disk( src_disk );
		const int logical = qMax( d.logical_sector, probed.descriptor_logical );
		const int physical = qMax( d.physical_sector, probed.descriptor_physical );
		const bool evidence_4k = Four_Kn_Sector( logical, physical, probed.ashift ) == 4096;
		const bool evidence_512 = ( logical == 512 || physical == 512 ) && ! evidence_4k;
		disk_sectors << ( ( evidence_4k || ( ( align_4k || nas ) && ! evidence_512 ) ) ? 4096 : 0 );
		QString serial = d.serial.trimmed();
		if( serial.isEmpty() && ( nas || probed.zfs_labels ) )
			serial = Stable_Disk_Serial( QString(), probed.disk_guid, i + 1 );
		disk_serials << serial;
		disk_virtio_scsi << ( disk_sectors.last() == 4096 || ! serial.isEmpty() );

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

	// Guest profile supplies the QEMU binary and machine. Old Windows is i386 + pc, not x86_64 + q35.
	const QString profile = CB_Guest_OS->currentText();
	const QJsonObject os_profile = Load_OS_Profiles().value( profile ).toObject();
	QString qemu_target = Profile_To_QEMU_Binary( os_profile.value( QStringLiteral( "target" ) ).toString() );
	if( os_profile.isEmpty() )
	{
		qemu_target = QStringLiteral( "qemu-system-x86_64" );
		if( profile.contains( QStringLiteral( "32-bit" ) ) || profile.contains( QStringLiteral( "MS-DOS" ) ) ||
		    profile.contains( QStringLiteral( "FreeDOS" ) ) || profile.contains( QStringLiteral( "Windows 9" ) ) ||
		    profile.contains( QStringLiteral( "Windows 3" ) ) || profile.contains( QStringLiteral( "Windows ME" ) ) ||
		    profile.contains( QStringLiteral( "Windows NT" ) ) || profile.contains( QStringLiteral( "Windows 2000" ) ) ||
		    profile.contains( QStringLiteral( "Server 2000" ) ) || profile.contains( QStringLiteral( "Server 2003" ) ) )
			qemu_target = QStringLiteral( "qemu-system-i386" );
		else if( profile.contains( QStringLiteral( "ARM" ) ) || profile.contains( QStringLiteral( "aarch64" ) ) )
			qemu_target = QStringLiteral( "qemu-system-aarch64" );
	}
	QString machine = First_Profile_String( os_profile.value( QStringLiteral( "machine" ) ) );
	if( machine.isEmpty() )
	{
		if( qemu_target == QStringLiteral( "qemu-system-aarch64" ) )
			machine = QStringLiteral( "virt" );
		else if( qemu_target == QStringLiteral( "qemu-system-x86_64" ) )
			machine = QStringLiteral( "q35" );
		else
			machine = QStringLiteral( "pc" );
	}
	vm->Set_Computer_Type( qemu_target );
	vm->Set_Machine_Type( machine );
	const QString cpu = os_profile.value( QStringLiteral( "cpu" ) ).toString();
	if( ! cpu.isEmpty() )
		vm->Set_CPU_Type( cpu );
	Apply_Profile_Sound( vm, First_Profile_String( os_profile.value( QStringLiteral( "sound" ) ) ) );
	const QString vga = First_Profile_String( os_profile.value( QStringLiteral( "vga" ) ) );
	if( ! vga.isEmpty() )
		vm->Set_Video_Card( vga );
	vm->Update_Current_Emulator_Devices();
	vm->Set_SMP_CPU_Count( SB_CPU->value() );
	vm->Set_Memory_Size( SB_RAM->value() );

	// Network Configuration
	vm->Set_Use_Network( true );
	VM_Net_Card net;
	net.Set_Net_Mode( VM::Net_Mode_Usermode );
	QString model = First_Profile_String( os_profile.value( QStringLiteral( "nic" ) ) );
	if( model.isEmpty() )
		model = QStringLiteral( "e1000" );
	if( ! Current_Appliance.networks.isEmpty() && ! Current_Appliance.networks.first().adapter_type.isEmpty() )
	{
		const QString a = Current_Appliance.networks.first().adapter_type.toLower();
		if( a.contains( QStringLiteral( "virtio" ) ) )
			model = QStringLiteral( "virtio-net-pci" );
		else if( a.contains( QStringLiteral( "pcnet" ) ) )
			model = QStringLiteral( "pcnet" );
		else if( a.contains( QStringLiteral( "rtl" ) ) )
			model = QStringLiteral( "rtl8139" );
		else if( a.contains( QStringLiteral( "e1000" ) ) )
			model = QStringLiteral( "e1000" );
	}
	net.Set_Card_Model( model );
	vm->Add_Network_Card( net );

	auto controller_to_interface = []( const QString &ctrl ) -> VM::Device_Interface {
		const QString c = ctrl.toLower();
		if( c == QStringLiteral( "sata" ) )
			return VM::DI_AHCI;
		if( c == QStringLiteral( "ide" ) )
			return VM::DI_IDE;
		if( c == QStringLiteral( "scsi" ) )
			return VM::DI_SCSI;
		if( c == QStringLiteral( "virtio" ) )
			return VM::DI_Virtio;
		return VM::DI_AHCI;
	};

	// Attach Disks
	if( ! final_disk_paths.isEmpty() )
	{
		const QString c_type0 = Current_Appliance.disks.isEmpty() ? QString() : Current_Appliance.disks[0].controller_type;
		const bool virtio0 = ! disk_virtio_scsi.isEmpty() && disk_virtio_scsi.first();
		const VM::Device_Interface iface0 = virtio0 ? VM::DI_Virtio_SCSI : controller_to_interface( c_type0 );

		VM_HDD hda( true, final_disk_paths.first() );
		VM_Native_Storage_Device native;
		native.Use_File_Path( true );
		native.Set_File_Path( final_disk_paths.first() );
		native.Use_Interface( true );
		native.Set_Interface( iface0 );
		native.Use_Media( true );
		native.Set_Media( VM::DM_Disk );
		if( ! disk_sectors.isEmpty() && disk_sectors.first() == 4096 )
		{
			native.Use_Block_Size( true );
			native.Set_Logical_Block_Size( 4096 );
			native.Set_Physical_Block_Size( 4096 );
		}
		if( ! disk_serials.isEmpty() && ! disk_serials.first().isEmpty() )
			native.Set_Disk_Serial( disk_serials.first() );
		hda.Set_Native_Device( native );
		vm->Set_HDA( hda );

		// Additional secondary disks
		QList<VM_Native_Storage_Device> extra_storage;
		for( int i = 1; i < final_disk_paths.size(); ++i )
		{
			const QString c_type = ( i < Current_Appliance.disks.size() ) ? Current_Appliance.disks[i].controller_type : QString();
			const bool virtio = i < disk_virtio_scsi.size() && disk_virtio_scsi[i];
			const VM::Device_Interface sec_iface = virtio ? VM::DI_Virtio_SCSI : controller_to_interface( c_type );

			VM_Native_Storage_Device sec;
			sec.Use_File_Path( true );
			sec.Set_File_Path( final_disk_paths[i] );
			sec.Use_Interface( true );
			sec.Set_Interface( sec_iface );
			sec.Use_Media( true );
			sec.Set_Media( VM::DM_Disk );
			if( i < disk_sectors.size() && disk_sectors[i] == 4096 )
			{
				sec.Use_Block_Size( true );
				sec.Set_Logical_Block_Size( 4096 );
				sec.Set_Physical_Block_Size( 4096 );
			}
			if( i < disk_serials.size() && ! disk_serials[i].isEmpty() )
				sec.Set_Disk_Serial( disk_serials[i] );
			extra_storage << sec;
		}
		if( ! extra_storage.isEmpty() )
			vm->Set_Storage_Devices_List( extra_storage );
	}

	// Save VM configuration XML
	const QString vm_dir = Settings.value( "VM_Directory", QDir::homePath() + "/.aqemu/" ).toString();
	const QString vm_file_path = QDir( vm_dir ).filePath( vm_name + QStringLiteral( ".aqemu" ) );
	const QString clean_vm_path = QDir::cleanPath( vm_file_path );
	if( ! clean_vm_path.startsWith( QDir::cleanPath( vm_dir ) ) )
	{
		delete vm;
		QMessageBox::critical( this, tr( "Path Error" ), tr( "VM configuration path escapes the VM directory." ) );
		return false;
	}

	vm->Set_VM_XML_File_Path( vm_file_path );
	if( ! vm->Save_VM( vm_file_path ) )
	{
		delete vm;
		QMessageBox::critical( this, tr( "Save Error" ), tr( "Failed to save the imported virtual machine configuration to '%1'." ).arg( vm_file_path ) );
		return false;
	}

	Imported_VM = vm;
	Progress_Bar->setValue( 100 );
	Label_Status->setText( tr( "Import Complete!" ) );
	return true;
}
