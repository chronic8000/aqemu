/****************************************************************************
** Import VMware (.vmx) and VirtualBox (.vbox) configs into AQEMU VMs.
****************************************************************************/

#include "Foreign_VM_Import.h"
#include "Utils.h"
#include "VM.h"
#include "VM_Devices.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QTextStream>
#include <QToolButton>
#include <QVBoxLayout>
#include <QXmlStreamReader>

static QString Unquote_VMX( QString v )
{
	v = v.trimmed();
	if( ( v.startsWith( QLatin1Char( '"' ) ) && v.endsWith( QLatin1Char( '"' ) ) ) ||
	    ( v.startsWith( QLatin1Char( '\'' ) ) && v.endsWith( QLatin1Char( '\'' ) ) ) )
		v = v.mid( 1, v.size() - 2 );
	return v.trimmed();
}

static QString Resolve_Beside( const QString &config_path, const QString &rel )
{
	if( rel.isEmpty() )
		return QString();
	const QFileInfo fi( rel );
	if( fi.isAbsolute() )
		return QDir::cleanPath( rel );
	return QDir::cleanPath( QFileInfo( config_path ).absoluteDir().filePath( rel ) );
}

static QString Map_Nic( const QString &raw )
{
	const QString a = raw.trimmed().toLower();
	if( a.contains( QLatin1String( "virtio" ) ) )
		return QStringLiteral( "virtio-net-pci" );
	if( a.contains( QLatin1String( "vmxnet" ) ) || a.contains( QLatin1String( "e1000" ) ) )
		return QStringLiteral( "e1000" );
	if( a.contains( QLatin1String( "pcnet" ) ) || a.contains( QLatin1String( "Am79C" ) ) )
		return QStringLiteral( "pcnet" );
	if( a.contains( QLatin1String( "rtl" ) ) || a.contains( QLatin1String( "8139" ) ) )
		return QStringLiteral( "rtl8139" );
	return QStringLiteral( "e1000" );
}

bool AQ_Parse_VMX_File( const QString &path, Foreign_VM_Config *out, QString *error )
{
	if( ! out )
		return false;
	*out = Foreign_VM_Config();
	out->source_path = path;
	out->source_kind = QStringLiteral( "vmx" );

	QFile f( path );
	if( ! f.open( QIODevice::ReadOnly | QIODevice::Text ) )
	{
		if( error )
			*error = QObject::tr( "Cannot open VMX file." );
		return false;
	}

	QHash<QString, QString> kv;
	QTextStream ts( &f );
	while( ! ts.atEnd() )
	{
		QString line = ts.readLine().trimmed();
		if( line.isEmpty() || line.startsWith( QLatin1Char( '#' ) ) || line.startsWith( QLatin1String( "//" ) ) )
			continue;
		const int eq = line.indexOf( QLatin1Char( '=' ) );
		if( eq <= 0 )
			continue;
		const QString key = line.left( eq ).trimmed().toLower();
		const QString val = Unquote_VMX( line.mid( eq + 1 ) );
		kv.insert( key, val );
	}

	out->name = kv.value( QStringLiteral( "displayname" ),
		kv.value( QStringLiteral( "displayName" ), QFileInfo( path ).completeBaseName() ) );
	if( out->name.isEmpty() )
		out->name = QFileInfo( path ).completeBaseName();
	out->cpus = qMax( 1, kv.value( QStringLiteral( "numvcpus" ), QStringLiteral( "1" ) ).toInt() );
	out->memory_mb = qMax( 64, kv.value( QStringLiteral( "memsize" ), QStringLiteral( "1024" ) ).toInt() );
	const QString fw = kv.value( QStringLiteral( "firmware" ),
		kv.value( QStringLiteral( "uefi.secureboot.enabled" ) ) ).toLower();
	out->uefi = fw.contains( QLatin1String( "efi" ) ) || fw == QLatin1String( "true" );

	if( kv.contains( QStringLiteral( "encryption.keysafe" ) ) ||
	    kv.value( QStringLiteral( "data.encryption" ) ) == QLatin1String( "true" ) )
	{
		out->warnings << QObject::tr( "Encrypted VMware VM detected. Decrypt it in VMware first." );
		if( error )
			*error = out->warnings.last();
		return false;
	}

	for( int i = 0; i < 4; ++i )
	{
		const QString present = kv.value( QStringLiteral( "ethernet%1.present" ).arg( i ) ).toLower();
		if( present != QLatin1String( "true" ) && i > 0 )
			continue;
		if( i == 0 || present == QLatin1String( "true" ) )
		{
			out->nic_model = Map_Nic( kv.value( QStringLiteral( "ethernet%1.virtualdev" ).arg( i ),
				QStringLiteral( "e1000" ) ) );
			out->mac = kv.value( QStringLiteral( "ethernet%1.address" ).arg( i ) );
			if( out->mac.isEmpty() )
				out->mac = kv.value( QStringLiteral( "ethernet%1.generatedaddress" ).arg( i ) );
			break;
		}
	}
	if( out->nic_model.isEmpty() )
		out->nic_model = QStringLiteral( "e1000" );

	auto add_disk = [&]( const QString &file_key, const QString &bus, bool cdrom ) {
		const QString rel = kv.value( file_key );
		if( rel.isEmpty() )
			return;
		Foreign_Disk d;
		d.path = Resolve_Beside( path, rel );
		d.bus = bus;
		d.cdrom = cdrom;
		if( ! QFile::exists( d.path ) )
			out->warnings << QObject::tr( "Missing disk/CD: %1" ).arg( d.path );
		out->disks << d;
	};

	for( int i = 0; i < 4; ++i )
	{
		if( kv.value( QStringLiteral( "ide%1:0.present" ).arg( i ) ).toLower() == QLatin1String( "true" ) )
			add_disk( QStringLiteral( "ide%1:0.filename" ).arg( i ), QStringLiteral( "ide" ),
				kv.value( QStringLiteral( "ide%1:0.devicetype" ).arg( i ) ).contains( QLatin1String( "cdrom" ), Qt::CaseInsensitive ) );
		if( kv.value( QStringLiteral( "ide%1:1.present" ).arg( i ) ).toLower() == QLatin1String( "true" ) )
			add_disk( QStringLiteral( "ide%1:1.filename" ).arg( i ), QStringLiteral( "ide" ),
				kv.value( QStringLiteral( "ide%1:1.devicetype" ).arg( i ) ).contains( QLatin1String( "cdrom" ), Qt::CaseInsensitive ) );
	}
	for( int c = 0; c < 4; ++c )
		for( int u = 0; u < 8; ++u )
		{
			const QString p = QStringLiteral( "scsi%1:%2.present" ).arg( c ).arg( u );
			if( kv.value( p ).toLower() != QLatin1String( "true" ) )
				continue;
			add_disk( QStringLiteral( "scsi%1:%2.filename" ).arg( c ).arg( u ), QStringLiteral( "scsi" ), false );
		}
	for( int c = 0; c < 4; ++c )
		for( int u = 0; u < 8; ++u )
		{
			const QString p = QStringLiteral( "sata%1:%2.present" ).arg( c ).arg( u );
			if( kv.value( p ).toLower() != QLatin1String( "true" ) )
				continue;
			add_disk( QStringLiteral( "sata%1:%2.filename" ).arg( c ).arg( u ), QStringLiteral( "sata" ),
				kv.value( QStringLiteral( "sata%1:%2.devicetype" ).arg( c ).arg( u ) )
					.contains( QLatin1String( "cdrom" ), Qt::CaseInsensitive ) );
		}
	for( int c = 0; c < 4; ++c )
		for( int u = 0; u < 8; ++u )
		{
			const QString p = QStringLiteral( "nvme%1:%2.present" ).arg( c ).arg( u );
			if( kv.value( p ).toLower() != QLatin1String( "true" ) )
				continue;
			add_disk( QStringLiteral( "nvme%1:%2.filename" ).arg( c ).arg( u ), QStringLiteral( "nvme" ), false );
		}

	if( out->disks.isEmpty() )
		out->warnings << QObject::tr( "No disks were found in the VMX file." );

	out->notes << QObject::tr( "Imported from VMware VMX. Guest tools / 3D / shared folders were skipped." );
	out->notes << QObject::tr( "Defaulted to SATA/e1000-compatible devices so the guest can boot without VirtIO drivers." );
	return true;
}

bool AQ_Parse_VBox_File( const QString &path, Foreign_VM_Config *out, QString *error )
{
	if( ! out )
		return false;
	*out = Foreign_VM_Config();
	out->source_path = path;
	out->source_kind = QStringLiteral( "vbox" );

	QFile f( path );
	if( ! f.open( QIODevice::ReadOnly ) )
	{
		if( error )
			*error = QObject::tr( "Cannot open VirtualBox file." );
		return false;
	}

	QHash<QString, QString> media;
	QString current_bus = QStringLiteral( "sata" );
	bool in_attached = false;
	bool attached_cdrom = false;

	QXmlStreamReader xml( &f );
	while( ! xml.atEnd() )
	{
		xml.readNext();
		if( ! xml.isStartElement() && ! xml.isEndElement() )
			continue;
		const QStringRef name = xml.name();
		if( xml.isStartElement() )
		{
			if( name == QLatin1String( "Machine" ) )
				out->name = xml.attributes().value( QStringLiteral( "name" ) ).toString();
			else if( name == QLatin1String( "Memory" ) )
				out->memory_mb = qMax( 64, xml.attributes().value( QStringLiteral( "RAMSize" ) ).toInt() );
			else if( name == QLatin1String( "CPU" ) )
				out->cpus = qMax( 1, xml.attributes().value( QStringLiteral( "count" ) ).toString().isEmpty()
					? 1 : xml.attributes().value( QStringLiteral( "count" ) ).toInt() );
			else if( name == QLatin1String( "Firmware" ) )
				out->uefi = xml.attributes().value( QStringLiteral( "type" ) )
					.toString().contains( QLatin1String( "EFI" ), Qt::CaseInsensitive );
			else if( name == QLatin1String( "BIOS" ) )
				out->uefi = out->uefi ||
					xml.attributes().value( QStringLiteral( "EFI" ) )
						.compare( QLatin1String( "true" ), Qt::CaseInsensitive ) == 0;
			else if( name == QLatin1String( "Adapter" ) && out->nic_model.isEmpty() )
			{
				out->nic_model = Map_Nic( xml.attributes().value( QStringLiteral( "type" ) )
					.toString().isEmpty() ? QStringLiteral( "82540EM" )
					: xml.attributes().value( QStringLiteral( "type" ) ).toString() );
				out->mac = xml.attributes().value( QStringLiteral( "MACAddress" ) ).toString();
				if( out->mac.size() == 12 )
				{
					QString pretty;
					for( int i = 0; i < 12; i += 2 )
					{
						if( ! pretty.isEmpty() )
							pretty += QLatin1Char( ':' );
						pretty += out->mac.mid( i, 2 );
					}
					out->mac = pretty;
				}
			}
			else if( name == QLatin1String( "HardDisk" ) || name == QLatin1String( "DVDImage" ) )
			{
				const QString uuid = xml.attributes().value( QStringLiteral( "uuid" ) ).toString().trimmed();
				const QString loc = xml.attributes().value( QStringLiteral( "location" ) ).toString().trimmed();
				if( ! uuid.isEmpty() && ! loc.isEmpty() )
					media.insert( uuid, Resolve_Beside( path, loc ) );
			}
			else if( name == QLatin1String( "StorageController" ) )
			{
				QString bus = xml.attributes().value( QStringLiteral( "type" ) ).toString().toLower();
				if( bus.contains( QLatin1String( "ahci" ) ) || bus.contains( QLatin1String( "sata" ) ) )
					current_bus = QStringLiteral( "sata" );
				else if( bus.contains( QLatin1String( "scsi" ) ) || bus.contains( QLatin1String( "lsi" ) ) )
					current_bus = QStringLiteral( "scsi" );
				else if( bus.contains( QLatin1String( "nvme" ) ) )
					current_bus = QStringLiteral( "nvme" );
				else if( bus.contains( QLatin1String( "virtio" ) ) )
					current_bus = QStringLiteral( "virtio" );
				else
					current_bus = QStringLiteral( "ide" );
			}
			else if( name == QLatin1String( "AttachedDevice" ) )
			{
				in_attached = true;
				const QString type = xml.attributes().value( QStringLiteral( "type" ) ).toString().toLower();
				attached_cdrom = type.contains( QLatin1String( "dvd" ) ) || type.contains( QLatin1String( "cd" ) );
			}
			else if( in_attached && name == QLatin1String( "Image" ) )
			{
				const QString uuid = xml.attributes().value( QStringLiteral( "uuid" ) ).toString().trimmed();
				Foreign_Disk d;
				d.path = media.value( uuid );
				d.bus = current_bus;
				d.cdrom = attached_cdrom;
				if( d.path.isEmpty() )
					out->warnings << QObject::tr( "Unresolved media UUID %1" ).arg( uuid );
				else if( ! QFile::exists( d.path ) )
					out->warnings << QObject::tr( "Missing disk/CD: %1" ).arg( d.path );
				out->disks << d;
			}
		}
		else if( xml.isEndElement() && name == QLatin1String( "AttachedDevice" ) )
			in_attached = false;
	}
	if( xml.hasError() )
	{
		if( error )
			*error = QObject::tr( "Invalid VirtualBox XML: %1" ).arg( xml.errorString() );
		return false;
	}
	if( out->name.isEmpty() )
		out->name = QFileInfo( path ).completeBaseName();
	if( out->cpus < 1 )
		out->cpus = 1;
	if( out->memory_mb < 64 )
		out->memory_mb = 1024;
	if( out->nic_model.isEmpty() )
		out->nic_model = QStringLiteral( "e1000" );
	if( out->disks.isEmpty() )
		out->warnings << QObject::tr( "No disks were found in the VirtualBox file." );

	out->notes << QObject::tr( "Imported from VirtualBox. Guest Additions / shared folders / 3D were skipped." );
	out->notes << QObject::tr( "Defaulted to compatible disk/NIC models for boot without VirtIO drivers." );
	return true;
}

bool AQ_Parse_Foreign_VM( const QString &path, Foreign_VM_Config *out, QString *error )
{
	const QString lower = path.toLower();
	if( lower.endsWith( QLatin1String( ".vmx" ) ) )
		return AQ_Parse_VMX_File( path, out, error );
	if( lower.endsWith( QLatin1String( ".vbox" ) ) )
		return AQ_Parse_VBox_File( path, out, error );
	if( error )
		*error = QObject::tr( "Unsupported foreign VM format (use .vmx or .vbox)." );
	return false;
}

Virtual_Machine *AQ_Create_VM_From_Foreign( const Foreign_VM_Config &cfg,
                                            const QString &dest_dir,
                                            bool convert_to_qcow2,
                                            QString *error,
                                            QString *summary )
{
	QStringList sum;
	sum << QObject::tr( "Source: %1 (%2)" ).arg( cfg.source_path, cfg.source_kind );
	sum << QObject::tr( "Name: %1" ).arg( cfg.name );
	sum << QObject::tr( "CPU: %1  RAM: %2 MiB  Firmware: %3" )
		.arg( cfg.cpus ).arg( cfg.memory_mb )
		.arg( cfg.uefi ? QObject::tr( "UEFI" ) : QObject::tr( "BIOS" ) );
	sum << cfg.notes;
	sum << cfg.warnings;

	QString vm_name = cfg.name.trimmed();
	vm_name.remove( QLatin1Char( '/' ) );
	vm_name.remove( QLatin1Char( '\\' ) );
	vm_name.remove( QLatin1Char( ':' ) );
	if( vm_name.isEmpty() )
		vm_name = QStringLiteral( "Imported_VM" );

	const QString dest_folder = QDir( dest_dir ).filePath( vm_name );
	QDir().mkpath( dest_folder );

	QStringList disk_paths;
	QString cd_path;
	const QString qemu_img = Get_QEMU_IMG_Path();
	for( int i = 0; i < cfg.disks.count(); ++i )
	{
		const Foreign_Disk &d = cfg.disks.at( i );
		if( d.path.isEmpty() || ! QFile::exists( d.path ) )
			continue;
		if( d.cdrom )
		{
			if( cd_path.isEmpty() )
				cd_path = d.path;
			continue;
		}
		if( ! convert_to_qcow2 )
		{
			disk_paths << d.path;
			sum << QObject::tr( "Disk (in place): %1 [%2]" ).arg( d.path, d.bus );
			continue;
		}
		const QString out = QDir( dest_folder ).filePath(
			QFileInfo( d.path ).completeBaseName() + QStringLiteral( ".qcow2" ) );
		if( QFile::exists( out ) )
		{
			if( error )
				*error = QObject::tr( "Refused to overwrite existing file:\n%1" ).arg( out );
			return nullptr;
		}
		QProcess proc;
		QStringList args;
		args << QStringLiteral( "convert" ) << QStringLiteral( "-O" ) << QStringLiteral( "qcow2" )
		     << d.path << out;
		proc.start( qemu_img, args );
		if( ! proc.waitForFinished( -1 ) || proc.exitCode() != 0 )
		{
			if( error )
				*error = QObject::tr( "qemu-img convert failed for %1:\n%2" )
					.arg( d.path, QString::fromLocal8Bit( proc.readAllStandardError() ) );
			return nullptr;
		}
		disk_paths << out;
		sum << QObject::tr( "Disk (converted): %1 → %2" ).arg( d.path, out );
	}

	if( disk_paths.isEmpty() )
	{
		if( error )
			*error = QObject::tr( "No usable hard disks were found to import." );
		return nullptr;
	}

	Virtual_Machine *vm = new Virtual_Machine();
	vm->Set_Machine_Name( vm_name );
	vm->Set_Computer_Type( QStringLiteral( "qemu-system-x86_64" ) );
	vm->Set_Machine_Type( cfg.uefi ? QStringLiteral( "q35" ) : QStringLiteral( "pc" ) );
	vm->Set_SMP_CPU_Count( cfg.cpus );
	vm->Set_Memory_Size( cfg.memory_mb );
	vm->Use_UEFI( cfg.uefi );
	vm->Update_Current_Emulator_Devices();

	vm->Set_Use_Network( true );
	VM_Net_Card net;
	net.Set_Net_Mode( VM::Net_Mode_Usermode );
	net.Set_Card_Model( cfg.nic_model.isEmpty() ? QStringLiteral( "e1000" ) : cfg.nic_model );
	if( ! cfg.mac.isEmpty() )
	{
		net.Set_MAC_Address( cfg.mac );
	}
	vm->Add_Network_Card( net );

	for( int i = 0; i < disk_paths.count() && i < 4; ++i )
	{
		VM_HDD hdd;
		hdd.Set_Enabled( true );
		hdd.Set_File_Name( disk_paths.at( i ) );
		VM_Native_Storage_Device native = hdd.Get_Native_Device();
		native.Use_File_Path( true );
		native.Set_File_Path( disk_paths.at( i ) );
		native.Use_Interface( true );
		QString bus = QStringLiteral( "sata" );
		// Match hard-disk entries (skip CD rows) for bus mapping.
		int hard_idx = -1;
		for( int d = 0; d < cfg.disks.count(); ++d )
		{
			if( cfg.disks.at( d ).cdrom )
				continue;
			++hard_idx;
			if( hard_idx == i )
			{
				bus = cfg.disks.at( d ).bus;
				break;
			}
		}
		if( bus == QLatin1String( "virtio" ) )
			native.Set_Interface( VM::DI_Virtio );
		else if( bus == QLatin1String( "nvme" ) )
			native.Set_Interface( VM::DI_NVMe );
		else if( bus == QLatin1String( "scsi" ) )
			native.Set_Interface( VM::DI_AHCI );
		else if( bus == QLatin1String( "ide" ) )
			native.Set_Interface( VM::DI_IDE );
		else
			native.Set_Interface( VM::DI_AHCI );
		hdd.Set_Native_Device( native );
		if( i == 0 ) vm->Set_HDA( hdd );
		else if( i == 1 ) vm->Set_HDB( hdd );
		else if( i == 2 ) vm->Set_HDC( hdd );
		else vm->Set_HDD( hdd );
	}

	if( ! cd_path.isEmpty() )
	{
		VM_Storage_Device cd;
		cd.Set_Enabled( true );
		cd.Set_File_Name( cd_path );
		vm->Set_CD_ROM( cd );
		sum << QObject::tr( "CD/DVD: %1" ).arg( cd_path );
	}

	const QString xml = QDir( dest_folder ).filePath( vm_name + QStringLiteral( ".aqemu" ) );
	vm->Set_VM_XML_File_Path( xml );
	if( ! vm->Save_VM() )
	{
		delete vm;
		if( error )
			*error = QObject::tr( "Could not write %1" ).arg( xml );
		return nullptr;
	}
	sum << QObject::tr( "Saved: %1" ).arg( xml );
	if( summary )
		*summary = sum.join( QLatin1Char( '\n' ) );
	return vm;
}

Foreign_VM_Import_Window::Foreign_VM_Import_Window( QWidget *parent )
	: QDialog( parent )
{
	Setup_UI();
}

void Foreign_VM_Import_Window::Setup_UI()
{
	setWindowTitle( tr( "Import VMware / VirtualBox VM" ) );
	resize( 640, 520 );
	QVBoxLayout *lay = new QVBoxLayout( this );

	QHBoxLayout *src = new QHBoxLayout();
	Edit_Source = new QLineEdit( this );
	Edit_Source->setPlaceholderText( tr( "Path to .vmx or .vbox" ) );
	QToolButton *browse = new QToolButton( this );
	browse->setText( QStringLiteral( "..." ) );
	connect( browse, SIGNAL(clicked()), this, SLOT(Browse_Source()) );
	connect( Edit_Source, SIGNAL(editingFinished()), this, SLOT(Reload_Source()) );
	src->addWidget( Edit_Source, 1 );
	src->addWidget( browse );
	lay->addLayout( src );

	QHBoxLayout *name_row = new QHBoxLayout();
	name_row->addWidget( new QLabel( tr( "VM name:" ), this ) );
	Edit_Name = new QLineEdit( this );
	name_row->addWidget( Edit_Name, 1 );
	lay->addLayout( name_row );

	QHBoxLayout *dest = new QHBoxLayout();
	Edit_Dest = new QLineEdit( this );
	Edit_Dest->setText( QDir::toNativeSeparators( AQEMU_Default_VM_Directory() ) );
	QToolButton *browse_dest = new QToolButton( this );
	browse_dest->setText( QStringLiteral( "..." ) );
	connect( browse_dest, SIGNAL(clicked()), this, SLOT(Browse_Dest()) );
	dest->addWidget( new QLabel( tr( "Save under:" ), this ) );
	dest->addWidget( Edit_Dest, 1 );
	dest->addWidget( browse_dest );
	lay->addLayout( dest );

	CB_Mode = new QComboBox( this );
	CB_Mode->addItem( tr( "Use disks in place (QEMU opens vmdk/vdi/vhdx)" ), QStringLiteral( "inplace" ) );
	CB_Mode->addItem( tr( "Convert disks to qcow2 (never overwrite)" ), QStringLiteral( "convert" ) );
	lay->addWidget( CB_Mode );

	Summary = new QPlainTextEdit( this );
	Summary->setReadOnly( true );
	lay->addWidget( Summary, 1 );

	Progress = new QProgressBar( this );
	Progress->setVisible( false );
	lay->addWidget( Progress );
	Status = new QLabel( this );
	lay->addWidget( Status );

	QHBoxLayout *btns = new QHBoxLayout();
	Btn_Import = new QPushButton( tr( "Import" ), this );
	QPushButton *cancel = new QPushButton( tr( "Cancel" ), this );
	connect( Btn_Import, SIGNAL(clicked()), this, SLOT(Start_Import()) );
	connect( cancel, SIGNAL(clicked()), this, SLOT(reject()) );
	btns->addStretch();
	btns->addWidget( Btn_Import );
	btns->addWidget( cancel );
	lay->addLayout( btns );
}

void Foreign_VM_Import_Window::Set_Source_Path( const QString &path )
{
	Edit_Source->setText( QDir::toNativeSeparators( path ) );
	Reload_Source();
}

void Foreign_VM_Import_Window::Browse_Source()
{
	const QString path = QFileDialog::getOpenFileName( this, tr( "Select VMware or VirtualBox VM" ),
		Get_Last_Dir_Path( Edit_Source->text() ),
		tr( "Foreign VMs (*.vmx *.vbox);;VMware (*.vmx);;VirtualBox (*.vbox);;All files (*)" ) );
	if( path.isEmpty() )
		return;
	Edit_Source->setText( QDir::toNativeSeparators( path ) );
	Reload_Source();
}

void Foreign_VM_Import_Window::Browse_Dest()
{
	const QString dir = QFileDialog::getExistingDirectory( this, tr( "Destination folder" ),
		Edit_Dest->text() );
	if( ! dir.isEmpty() )
		Edit_Dest->setText( QDir::toNativeSeparators( dir ) );
}

void Foreign_VM_Import_Window::Show_Config( const Foreign_VM_Config &cfg )
{
	QStringList lines;
	lines << tr( "Source: %1 (%2)" ).arg( cfg.source_path, cfg.source_kind.toUpper() );
	lines << tr( "Name: %1" ).arg( cfg.name );
	lines << tr( "CPUs: %1   RAM: %2 MiB   Firmware: %3" )
		.arg( cfg.cpus ).arg( cfg.memory_mb )
		.arg( cfg.uefi ? tr( "UEFI" ) : tr( "BIOS/SeaBIOS" ) );
	lines << tr( "NIC: %1 %2" ).arg( cfg.nic_model, cfg.mac );
	lines << tr( "Disks:" );
	for( int i = 0; i < cfg.disks.count(); ++i )
	{
		const Foreign_Disk &d = cfg.disks.at( i );
		lines << QStringLiteral( "  - [%1] %2%3" )
			.arg( d.bus, d.path, d.cdrom ? QStringLiteral( " (CD)" ) : QString() );
	}
	if( ! cfg.notes.isEmpty() )
	{
		lines << tr( "Notes:" );
		lines << cfg.notes;
	}
	if( ! cfg.warnings.isEmpty() )
	{
		lines << tr( "Warnings:" );
		lines << cfg.warnings;
	}
	Summary->setPlainText( lines.join( QLatin1Char( '\n' ) ) );
}

void Foreign_VM_Import_Window::Reload_Source()
{
	const QString path = Edit_Source->text().trimmed();
	if( path.isEmpty() )
		return;
	QString err;
	Foreign_VM_Config cfg;
	if( ! AQ_Parse_Foreign_VM( path, &cfg, &err ) )
	{
		Summary->setPlainText( err );
		Btn_Import->setEnabled( false );
		return;
	}
	Current = cfg;
	Edit_Name->setText( cfg.name );
	Show_Config( cfg );
	Btn_Import->setEnabled( true );
}

void Foreign_VM_Import_Window::Start_Import()
{
	if( Current.source_path.isEmpty() )
		Reload_Source();
	if( Current.source_path.isEmpty() )
		return;

	Foreign_VM_Config cfg = Current;
	if( ! Edit_Name->text().trimmed().isEmpty() )
		cfg.name = Edit_Name->text().trimmed();

	const bool convert = CB_Mode->currentData().toString() == QLatin1String( "convert" );
	Progress->setVisible( true );
	Progress->setRange( 0, 0 );
	Status->setText( tr( "Importing…" ) );
	Btn_Import->setEnabled( false );
	QApplication::processEvents();

	QString err, summary;
	Virtual_Machine *vm = AQ_Create_VM_From_Foreign(
		cfg, Edit_Dest->text().trimmed(), convert, &err, &summary );
	Progress->setVisible( false );
	Btn_Import->setEnabled( true );
	if( ! vm )
	{
		QMessageBox::critical( this, tr( "Import failed" ), err );
		Status->setText( tr( "Import failed." ) );
		return;
	}
	Imported_VM = vm;
	Summary->setPlainText( summary );
	Status->setText( tr( "Import complete." ) );
	QMessageBox::information( this, tr( "Import complete" ),
		tr( "Created %1\n\n%2" ).arg( vm->Get_Machine_Name(), summary.left( 800 ) ) );
	accept();
}
