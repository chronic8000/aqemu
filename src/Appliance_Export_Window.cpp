/****************************************************************************
**
** Appliance Export Window (OVA / OVF)
**
****************************************************************************/

#include "Appliance_Export_Window.h"
#include "OVF_Parser.h"
#include "Utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QProcess>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>

Appliance_Export_Window::Appliance_Export_Window( const QList<Virtual_Machine*> &vms, int current_index, QWidget *parent )
	: QDialog( parent ), Available_VMs( vms )
{
	setWindowTitle( tr( "Export Virtual Appliance (OVA)" ) );
	resize( 600, 360 );
	Setup_UI();

	for( int i = 0; i < Available_VMs.size(); ++i )
	{
		const Virtual_Machine *v = Available_VMs[i];
		CB_VM_Selection->addItem( v->Get_Machine_Name() );
	}

	if( current_index >= 0 && current_index < Available_VMs.size() )
		CB_VM_Selection->setCurrentIndex( current_index );

	On_VM_Selection_Changed( CB_VM_Selection->currentIndex() );
}

void Appliance_Export_Window::Setup_UI()
{
	QVBoxLayout *main_lay = new QVBoxLayout( this );

	QGroupBox *gb_vm = new QGroupBox( tr( "Virtual Machine Selection" ), this );
	QVBoxLayout *vm_lay = new QVBoxLayout( gb_vm );
	CB_VM_Selection = new QComboBox( gb_vm );
	vm_lay->addWidget( CB_VM_Selection );
	main_lay->addWidget( gb_vm );

	QGroupBox *gb_dest = new QGroupBox( tr( "Export Destination" ), this );
	QHBoxLayout *dest_lay = new QHBoxLayout( gb_dest );
	Edit_Export_Path = new QLineEdit( gb_dest );
	Edit_Export_Path->setPlaceholderText( tr( "Destination path for .ova archive" ) );
	TB_Browse_Export = new QToolButton( gb_dest );
	TB_Browse_Export->setText( QStringLiteral( "..." ) );
	dest_lay->addWidget( Edit_Export_Path, 1 );
	dest_lay->addWidget( TB_Browse_Export );
	main_lay->addWidget( gb_dest );

	Progress_Bar = new QProgressBar( this );
	Progress_Bar->setRange( 0, 100 );
	Progress_Bar->setValue( 0 );
	Progress_Bar->setVisible( false );
	main_lay->addWidget( Progress_Bar );

	Label_Status = new QLabel( this );
	Label_Status->setStyleSheet( QStringLiteral( "color: #94a3b8; font-size: 0.9em;" ) );
	main_lay->addWidget( Label_Status );

	QHBoxLayout *btn_lay = new QHBoxLayout();
	btn_lay->addStretch();
	Btn_Cancel = new QPushButton( tr( "Cancel" ), this );
	Btn_Export = new QPushButton( tr( "Export Appliance" ), this );
	Btn_Export->setDefault( true );
	btn_lay->addWidget( Btn_Cancel );
	btn_lay->addWidget( Btn_Export );
	main_lay->addLayout( btn_lay );

	connect( CB_VM_Selection, QOverload<int>::of( &QComboBox::currentIndexChanged ),
	         this, &Appliance_Export_Window::On_VM_Selection_Changed );
	connect( TB_Browse_Export, &QToolButton::clicked,
	         this, &Appliance_Export_Window::On_Browse_Destination );
	connect( Btn_Export, &QPushButton::clicked,
	         this, &Appliance_Export_Window::On_Start_Export );
	connect( Btn_Cancel, &QPushButton::clicked,
	         this, &QDialog::reject );
}

void Appliance_Export_Window::On_VM_Selection_Changed( int index )
{
	if( index < 0 || index >= Available_VMs.size() )
		return;

	const QString vm_name = Available_VMs[index]->Get_Machine_Name();
	const QString default_path = QDir( QDir::homePath() ).filePath( vm_name + QStringLiteral( ".ova" ) );
	Edit_Export_Path->setText( QDir::toNativeSeparators( default_path ) );
}

void Appliance_Export_Window::On_Browse_Destination()
{
	const QString current_path = Edit_Export_Path->text().trimmed();
	const QString path = QFileDialog::getSaveFileName( this, tr( "Save Virtual Appliance As" ),
	                                                   current_path.isEmpty() ? QDir::homePath() : current_path,
	                                                   tr( "Open Virtualization Appliance (*.ova)" ) );
	if( ! path.isEmpty() )
	{
		QString p = path;
		if( ! p.endsWith( QStringLiteral( ".ova" ), Qt::CaseInsensitive ) )
			p += QStringLiteral( ".ova" );
		Edit_Export_Path->setText( QDir::toNativeSeparators( p ) );
	}
}

void Appliance_Export_Window::On_Start_Export()
{
	if( Is_Working )
		return;

	if( CB_VM_Selection->currentIndex() < 0 || CB_VM_Selection->currentIndex() >= Available_VMs.size() )
	{
		QMessageBox::warning( this, tr( "No VM Selected" ), tr( "Please select a virtual machine to export." ) );
		return;
	}

	if( Edit_Export_Path->text().trimmed().isEmpty() )
	{
		QMessageBox::warning( this, tr( "No Destination" ), tr( "Please specify a destination file path." ) );
		return;
	}

	Is_Working = true;
	Btn_Export->setEnabled( false );
	Btn_Cancel->setEnabled( false );
	Progress_Bar->setVisible( true );
	Progress_Bar->setValue( 0 );

	const bool ok = Execute_Export();

	Is_Working = false;
	Btn_Cancel->setEnabled( true );
	Progress_Bar->setVisible( false );

	if( ok )
	{
		QMessageBox::information( this, tr( "Export Complete" ),
		                          tr( "Appliance was successfully exported to:\n%1" )
		                          .arg( Edit_Export_Path->text() ) );
		accept();
	}
	else
	{
		Btn_Export->setEnabled( true );
	}
}

bool Appliance_Export_Window::Execute_Export()
{
	Virtual_Machine *vm = Available_VMs[ CB_VM_Selection->currentIndex() ];
	const QString dest_ova = Edit_Export_Path->text().trimmed();
	const QString qemu_img = Get_QEMU_IMG_Path();

	QTemporaryDir temp_dir;
	if( ! temp_dir.isValid() )
	{
		QMessageBox::critical( this, tr( "Temporary Directory Error" ),
		                      tr( "Could not create temporary working directory." ) );
		return false;
	}

	// 1. Gather all attached virtual hard disks
	QStringList source_disks;
	if( vm->Get_HDA().Get_Enabled() && ! vm->Get_HDA().Get_File_Name().isEmpty() )
		source_disks << vm->Get_HDA().Get_File_Name();
	if( vm->Get_HDB().Get_Enabled() && ! vm->Get_HDB().Get_File_Name().isEmpty() )
		source_disks << vm->Get_HDB().Get_File_Name();
	if( vm->Get_HDC().Get_Enabled() && ! vm->Get_HDC().Get_File_Name().isEmpty() )
		source_disks << vm->Get_HDC().Get_File_Name();
	if( vm->Get_HDD().Get_Enabled() && ! vm->Get_HDD().Get_File_Name().isEmpty() )
		source_disks << vm->Get_HDD().Get_File_Name();

	for( const VM_Native_Storage_Device &sd : vm->Get_Storage_Devices_List() )
	{
		if( sd.Use_File_Path() && ! sd.Get_File_Path().isEmpty() )
		{
			if( ! sd.Use_Media() || sd.Get_Media() == VM::DM_Disk )
				source_disks << sd.Get_File_Path();
		}
	}

	// Check all source disks exist up front (no incomplete exports)
	QStringList missing_disks;
	for( const QString &src : source_disks )
	{
		if( ! QFile::exists( src ) )
			missing_disks << src;
	}
	if( ! missing_disks.isEmpty() )
	{
		QMessageBox::critical( this, tr( "Missing Disk File" ),
		                      tr( "The following virtual disk file configured for this VM could not be found:\n%1\n\nExport cannot continue." )
		                      .arg( missing_disks.join( QStringLiteral( "\n" ) ) ) );
		return false;
	}

	QList<OVF_Disk> ovf_disks;
	QStringList files_to_pack;

	// 2. Convert each disk to streamOptimized VMDK for VMware/VirtualBox compatibility
	for( int i = 0; i < source_disks.size(); ++i )
	{
		const QString src = source_disks[i];
		const QString vmdk_name = QStringLiteral( "%1-disk%2.vmdk" ).arg( vm->Get_Machine_Name() ).arg( i + 1 );
		const QString vmdk_path = QDir( temp_dir.path() ).filePath( vmdk_name );

		Label_Status->setText( tr( "Converting disk %1 to VMDK..." ).arg( i + 1 ) );
		Progress_Bar->setValue( ( i * 60 ) / qMax( 1, source_disks.size() ) );
		QApplication::processEvents();

		// Query true virtual disk capacity
		qint64 virtual_size = 0;
		QProcess info_proc;
		info_proc.start( qemu_img, QStringList() << QStringLiteral( "info" ) << QStringLiteral( "--output=json" ) << src );
		if( info_proc.waitForFinished( 5000 ) && info_proc.exitCode() == 0 )
		{
			const QJsonDocument doc = QJsonDocument::fromJson( info_proc.readAllStandardOutput() );
			if( doc.isObject() )
			{
				virtual_size = doc.object().value( QStringLiteral( "virtual-size" ) ).toVariant().toLongLong();
			}
		}
		if( virtual_size <= 0 )
		{
			virtual_size = QFileInfo( src ).size();
		}

		QStringList args;
		args << QStringLiteral( "convert" )
		     << QStringLiteral( "-O" ) << QStringLiteral( "vmdk" )
		     << QStringLiteral( "-o" ) << QStringLiteral( "subformat=streamOptimized" )
		     << src << vmdk_path;

		QProcess proc;
		proc.start( qemu_img, args );
		if( ! proc.waitForFinished( -1 ) || proc.exitCode() != 0 )
		{
			// Fallback to monolithicSparse if streamOptimized is unsupported
			args.clear();
			args << QStringLiteral( "convert" )
			     << QStringLiteral( "-O" ) << QStringLiteral( "vmdk" )
			     << src << vmdk_path;
			proc.start( qemu_img, args );
			if( ! proc.waitForFinished( -1 ) || proc.exitCode() != 0 )
			{
				QMessageBox::critical( this, tr( "Conversion Error" ),
				                      tr( "Failed to convert disk to VMDK:\n%1" )
				                      .arg( QString::fromLocal8Bit( proc.readAllStandardError() ) ) );
				return false;
			}
		}

		OVF_Disk od;
		od.disk_id = QStringLiteral( "vmdisk%1" ).arg( i + 1 );
		od.href = vmdk_name;
		od.local_extracted_path = vmdk_path;
		od.capacity_bytes = virtual_size;
		ovf_disks << od;
		files_to_pack << vmdk_path;
	}

	// 3. Generate OVF descriptor XML
	Label_Status->setText( tr( "Writing OVF descriptor..." ) );
	Progress_Bar->setValue( 70 );
	QApplication::processEvents();

	const QString ovf_xml = OVF_Parser::Generate_OVF_XML( *vm, ovf_disks );
	const QString ovf_path = QDir( temp_dir.path() ).filePath( vm->Get_Machine_Name() + QStringLiteral( ".ovf" ) );
	QFile ovf_file( ovf_path );
	if( ! ovf_file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
		QMessageBox::critical( this, tr( "File Error" ), tr( "Could not write OVF XML descriptor." ) );
		return false;
	}
	ovf_file.write( ovf_xml.toUtf8() );
	ovf_file.close();

	// OVF descriptor must be the FIRST file in the TAR archive according to OVF specification!
	files_to_pack.prepend( ovf_path );

	// 4. Pack into .ova TAR archive
	Label_Status->setText( tr( "Creating OVA archive..." ) );
	auto pack_cb = [this]( int pct, const QString &status ) {
		Progress_Bar->setValue( 70 + ( pct * 30 ) / 100 );
		Label_Status->setText( status );
		QApplication::processEvents();
	};

	QString pack_err;
	if( ! OVF_Parser::Pack_OVA( files_to_pack, dest_ova, pack_err, pack_cb ) )
	{
		QMessageBox::critical( this, tr( "Packaging Error" ),
		                      tr( "Failed to create OVA package:\n%1" ).arg( pack_err ) );
		return false;
	}

	Progress_Bar->setValue( 100 );
	Label_Status->setText( tr( "Export Succeeded!" ) );
	return true;
}
