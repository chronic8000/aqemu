#include "Inferno_Companion_Setup.h"

#include "URL_Fetch.h"
#include "Utils.h"
#include "VM.h"
#include "VM_Devices.h"
#include "AQ_UI_Style.h"

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QUuid>

QString AQ_Inferno_Companion_OS_Name()
{
	return QStringLiteral( "iPhone IPSW Restore Companion" );
}

bool AQ_Is_Inferno_Companion_OS( const QString &os_name )
{
	const QString n = os_name.trimmed();
	return n.compare( AQ_Inferno_Companion_OS_Name(), Qt::CaseInsensitive ) == 0 ||
	       n.contains( QLatin1String( "IPSW Restore Companion" ), Qt::CaseInsensitive ) ||
	       ( n.contains( QLatin1String( "Inferno Companion" ), Qt::CaseInsensitive ) &&
	         n.contains( QLatin1String( "iPhone" ), Qt::CaseInsensitive ) );
}

QString AQ_Inferno_Companion_Ubuntu_ISO_URL()
{
	// Official releases mirror; FollowRedirects handles CDN hops.
	return QStringLiteral(
		"https://releases.ubuntu.com/24.04/ubuntu-24.04.3-live-server-amd64.iso" );
}

QString AQ_Inferno_Companion_Ubuntu_ISO_FileName()
{
	return QStringLiteral( "ubuntu-24.04.3-live-server-amd64.iso" );
}

bool AQ_Inferno_Companion_Prepare_Assets( QWidget *parent,
                                         const QString &vm_folder,
                                         QString *disk_out,
                                         QString *iso_out,
                                         QString *error_out,
                                         const QString &preferred_local_iso )
{
	auto fail = [&]( const QString &msg ) -> bool {
		if( error_out )
			*error_out = msg;
		return false;
	};

	const QString folder = QDir::cleanPath( vm_folder );
	if( folder.isEmpty() )
		return fail( QObject::tr( "VM folder is empty." ) );
	if( ! QDir().mkpath( folder ) )
		return fail( QObject::tr( "Cannot create folder:\n%1" ).arg( folder ) );

	const QString iso_path = QDir::toNativeSeparators(
		folder + QLatin1Char( '/' ) + AQ_Inferno_Companion_Ubuntu_ISO_FileName() );
	const QString disk_path = QDir::toNativeSeparators(
		folder + QLatin1String( "/companion.qcow2" ) );

	const QString local = QDir::toNativeSeparators( preferred_local_iso.trimmed() );
	if( ! local.isEmpty() )
	{
		if( ! QFile::exists( local ) )
			return fail( QObject::tr( "Local Ubuntu ISO not found:\n%1" ).arg( local ) );
		if( QDir::fromNativeSeparators( local ).compare(
			QDir::fromNativeSeparators( iso_path ), Qt::CaseInsensitive ) != 0 )
		{
			if( QFile::exists( iso_path ) )
				QFile::remove( iso_path );
			if( ! QFile::copy( local, iso_path ) )
			{
				// Fall back to using the user's path in place (no copy).
				if( disk_out )
					*disk_out = disk_path;
				if( iso_out )
					*iso_out = local;
				if( ! QFile::exists( disk_path ) )
				{
					VM::Device_Size sz;
					sz.Size = 20;
					sz.Suffix = VM::Size_Suf_Gb;
					if( ! Create_New_HDD_Image( disk_path, sz ) )
						return fail( QObject::tr( "Failed to create companion.qcow2 via qemu-img." ) );
				}
				AQ_Inferno_Companion_Remember_Disk( disk_path );
				return true;
			}
		}
	}
	else if( ! QFile::exists( iso_path ) )
	{
		const auto ans = QMessageBox::question( parent,
			QObject::tr( "Download Ubuntu Server ISO" ),
			QObject::tr(
				"<p><b>IPSW Restore Companion needs Ubuntu Server.</b></p>"
				"<p>AQEMU will download the official Ubuntu Server 24.04 live ISO "
				"(~2–3 GB) into:</p>"
				"<p><code>%1</code></p>"
				"<p>The ISO is mounted as the VM’s CD-ROM so Power On boots the installer. "
				"You do <b>not</b> need to pick an ISO on the previous wizard pages.</p>"
				"<p>Continue download?</p>" )
				.arg( iso_path ),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
		if( ans != QMessageBox::Yes )
			return fail( QObject::tr( "ISO download cancelled." ) );

		const QString got = AQ_Download_URL_To_File( parent,
			AQ_Inferno_Companion_Ubuntu_ISO_URL(),
			iso_path,
			QObject::tr( "Downloading Ubuntu Server ISO for IPSW companion…" ) );
		if( got.isEmpty() || ! QFile::exists( iso_path ) )
			return fail( QObject::tr(
				"Failed to download Ubuntu Server ISO.\n"
				"Check your network, or place the ISO manually at:\n%1\n\n"
				"URL:\n%2" )
				.arg( iso_path, AQ_Inferno_Companion_Ubuntu_ISO_URL() ) );
	}

	if( ! QFile::exists( disk_path ) )
	{
		VM::Device_Size sz;
		sz.Size = 20;
		sz.Suffix = VM::Size_Suf_Gb;
		if( ! Create_New_HDD_Image( disk_path, sz ) )
			return fail( QObject::tr( "Failed to create companion.qcow2 via qemu-img." ) );
	}

	if( disk_out )
		*disk_out = disk_path;
	if( iso_out )
		*iso_out = iso_path;
	AQ_Inferno_Companion_Remember_Disk( disk_path );
	return true;
}

QString AQ_Inferno_Companion_Post_Install_Notes( const QString &disk_path,
                                                const QString &iso_path,
                                                const QString &ssh_user_hint )
{
	const QString user = ssh_user_hint.trimmed().isEmpty()
		? QStringLiteral( "YOURUSER" )
		: ssh_user_hint.trimmed();

	return QObject::tr(
		"════════════════════════════════════════════════════════════\n"
		"  AQEMU — iPhone IPSW Restore Companion (Ubuntu Server)\n"
		"════════════════════════════════════════════════════════════\n"
		"\n"
		"YES — YOU RUN TWO VMs AT THE SAME TIME\n"
		"  1) This companion Ubuntu VM  (install tools / later hosts USB)\n"
		"  2) Your iOS / Apple SoC VM   (the emulated iPhone)\n"
		"  AQEMU can power both on. For IPSW restore, start the companion\n"
		"  USB bridge FIRST, then the iOS guest.\n"
		"\n"
		"WHAT THIS VM IS\n"
		"  ChefKiss Inferno cannot expose the emulated iPhone USB to Windows.\n"
		"  This Ubuntu VM is where usbmuxd + idevicerestore run. The phone\n"
		"  appears inside THIS Linux guest only (via Inferno usb-tcp-remote).\n"
		"\n"
		"FILES (already prepared)\n"
		"  Disk:  %1\n"
		"  ISO:   %2   ← mounted as CD-ROM on this AQEMU VM\n"
		"\n"
		"────────────────────────────────────────────────────────────\n"
		"STEP 1 — Install Ubuntu (in AQEMU) — do this first\n"
		"────────────────────────────────────────────────────────────\n"
		"  1. In the AQEMU machine list, select\n"
		"       \"iPhone IPSW Restore Companion\" (or the name you chose).\n"
		"  2. Power On — Ubuntu Server installer should boot from the ISO.\n"
		"  3. Complete install (enable OpenSSH when asked).\n"
		"  4. Create a user (remember the name for SSH).\n"
		"  5. Shut down the guest when install finishes.\n"
		"  6. Device Manager: eject/remove the install ISO\n"
		"     (or set boot order Hard Disk first), then Power On again.\n"
		"\n"
		"────────────────────────────────────────────────────────────\n"
		"STEP 2 — SSH from Windows/WSL (optional — AQEMU console works too)\n"
		"────────────────────────────────────────────────────────────\n"
		"  On Windows this companion runs under WSL/KVM, so use:\n"
		"    wsl ssh -p 32222 %3@127.0.0.1\n"
		"  (plain ssh to Windows 127.0.0.1:32222 will be refused)\n"
		"\n"
		"────────────────────────────────────────────────────────────\n"
		"STEP 3 — Install iDevice tools INSIDE Ubuntu (once)\n"
		"────────────────────────────────────────────────────────────\n"
		"  sudo apt update\n"
		"  sudo apt install -y build-essential pkg-config git libtool\n"
		"  sudo apt install -y automake autoconf libssl-dev libcurl4-openssl-dev\n"
		"  sudo apt install -y libusb-1.0-0-dev usbmuxd usbutils\n"
		"\n"
		"  Build libimobiledevice stack from source (releases are too old).\n"
		"  Order: libplist → libimobiledevice-glue → libusbmuxd →\n"
		"    libimobiledevice → libirecovery → libtatsu → usbmuxd → idevicerestore\n"
		"  For each: PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./autogen.sh &&\n"
		"    make -j$(nproc) && sudo make install && sudo ldconfig\n"
		"  idevicerestore: apply ChefKiss patch BEFORE build (required).\n"
		"  Without it restore fails: Unable to discover device type\n"
		"  (Inferno HardwareModel is N104DEV; patch rewrites to N104AP).\n"
		"  curl -fsSL -o idevicerestore.patch \\\n"
		"    https://chefkiss.dev/Extras/Inferno/idevicerestore.patch\n"
		"  # or copy from AQEMU extras/Inferno/idevicerestore.patch\n"
		"  cd idevicerestore && git apply ../idevicerestore.patch\n"
		"  https://chefkiss.dev/guides/inferno/companion-setup/\n"
		"  Also generate a real root_ticket.der for the iOS VM / -T flag.\n"
		"\n"
		"  sudo systemctl enable --now usbmuxd\n"
		"\n"
		"────────────────────────────────────────────────────────────\n"
		"STEP 4 — Every IPSW restore (two VMs)\n"
		"────────────────────────────────────────────────────────────\n"
		"  A. Apple SoC Restore: Companion disk = path above; USB =\n"
		"       IPv4 127.0.0.1 port 8030 (same on iOS MACHINE tab).\n"
		"  B. Start companion in WSL  (Inferno + usb-tcp-remote) FIRST\n"
		"       — or Power On this Ubuntu VM only for SSH/tools;\n"
		"         the USB bridge still needs Inferno Start companion.\n"
		"  C. Power On / restart the iOS guest to recovery.\n"
		"  D. SSH into companion: lsusb ; idevice_id -l\n"
		"  E. idevicerestore -d -R ~/your.ipsw\n"
		"\n"
		"  scp -P 32222 /mnt/h/path/to/restore.ipsw %3@127.0.0.1:~/\n"
		"\n"
		"────────────────────────────────────────────────────────────\n"
		"TROUBLESHOOTING\n"
		"────────────────────────────────────────────────────────────\n"
		"  • idevice_id on WSL host fails — tools belong IN companion Ubuntu.\n"
		"  • Companion USB bridge before iOS guest.\n"
		"  • Match ipv4 127.0.0.1:8030 on both sides.\n"
		"\n" )
		.arg( disk_path, iso_path, user );
}

void AQ_Inferno_Companion_Show_Notes( QWidget *parent, const QString &notes )
{
	QDialog dlg( parent );
	dlg.setWindowTitle( QObject::tr( "Companion setup — do these steps next" ) );
	dlg.resize( AQ_Px( 720, &dlg ), AQ_Px( 560, &dlg ) );

	QVBoxLayout *lay = new QVBoxLayout( &dlg );
	QLabel *intro = new QLabel( QObject::tr(
		"<b>Companion VM is ready in AQEMU.</b> The Ubuntu Server ISO is mounted as "
		"CD-ROM — <b>Power On</b> that machine to install. You can run it "
		"<b>at the same time</b> as your iOS VM. Copy the checklist below anytime." ) );
	intro->setWordWrap( true );
	lay->addWidget( intro );

	QTextEdit *edit = new QTextEdit;
	edit->setReadOnly( true );
	edit->setPlainText( notes );
	edit->setFont( QFont( QStringLiteral( "Consolas" ), 10 ) );
	lay->addWidget( edit, 1 );

	QHBoxLayout *btns = new QHBoxLayout;
	QPushButton *btnCopy = new QPushButton( QObject::tr( "Copy all" ) );
	QObject::connect( btnCopy, &QPushButton::clicked, &dlg, [edit]() {
		QApplication::clipboard()->setText( edit->toPlainText() );
	} );
	QPushButton *btnClose = new QPushButton( QObject::tr( "Close" ) );
	QObject::connect( btnClose, &QPushButton::clicked, &dlg, &QDialog::accept );
	btns->addWidget( btnCopy );
	btns->addStretch();
	btns->addWidget( btnClose );
	lay->addLayout( btns );

	dlg.exec();
}

void AQ_Inferno_Companion_Remember_Disk( const QString &disk_path )
{
	if( disk_path.trimmed().isEmpty() )
		return;
	QSettings s;
	s.setValue( QStringLiteral( "Apple_SoC_Restore/Companion_Disk" ),
	            QDir::toNativeSeparators( disk_path ) );
}

void AQ_Apply_Inferno_Companion_VM_Defaults( Virtual_Machine *vm,
                                             const QString &disk_path,
                                             const QString &iso_path )
{
	if( ! vm )
		return;

	// Strict Ubuntu x86_64 companion — never Inferno Apple SoC.
	vm->Use_Apple_SoC_Profile( false );
	vm->Set_Computer_Type( QStringLiteral( "qemu-system-x86_64" ) );
	vm->Set_Machine_Type( QStringLiteral( "q35" ) );
	vm->Set_Memory_Size( 2048 );
	vm->Set_SMP_CPU_Count( 2 );
	vm->Set_CPU_Type( QStringLiteral( "max" ) );
	vm->Set_Video_Card( QStringLiteral( "virtio-vga" ) );
	vm->Use_UEFI( false );
	vm->Set_UEFI_CODE_File( QString() );
	vm->Set_UEFI_VARS_File( QString() );
	vm->Set_Use_Linux_Boot( false );
	vm->Set_bzImage_Path( QString() );
	vm->Set_Initrd_Path( QString() );
	vm->Set_DeviceTree_Path( QString() );
	vm->Set_App_Kernel_Path( QString() );
	vm->Set_Kernel_ComLine( QString() );
	vm->Set_Apple_Trustcache_Path( QString() );
	vm->Set_Apple_Ticket_Path( QString() );
	vm->Set_Apple_SEP_FW_Path( QString() );
	vm->Set_Apple_SEP_ROM_Path( QString() );
	vm->Set_Apple_SecureROM_Path( QString() );
	vm->Use_Apple_KASLR_Off( false );

	VM::Sound_Cards no_sound;
	vm->Set_Audio_Cards( no_sound );

	// No legacy floppies (avoids IDE/index clashes with CD on some builds).
	vm->Set_FD0( VM_Storage_Device() );
	vm->Set_FD1( VM_Storage_Device() );

	// On Windows, run companion under WSL so -accel kvm works (same as when
	// Ubuntu installed successfully). SSH hostfwd then lives in WSL:
	//   wsl ssh -p 32222 USER@127.0.0.1
	// Inferno usb-tcp-remote remains a separate Apple SoC Restore step.
#ifdef Q_OS_WIN32
	vm->Use_Launch_Via_WSL( true );
#endif
	vm->Set_Use_Network( true );
	vm->Set_Use_Redirections( true );

	if( vm->Get_Network_Cards_Count() == 0 )
	{
		VM_Net_Card net;
		net.Set_Net_Mode( VM::Net_Mode_Usermode );
		net.Set_Card_Model( QStringLiteral( "virtio-net-pci" ) );
		vm->Add_Network_Card( net );
	}

	if( vm->Get_Network_Redirections_Count() == 0 )
	{
		VM_Redirection r;
		r.Set_Protocol( QStringLiteral( "TCP" ) );
		r.Set_Host_Port( 32222 );
		r.Set_Guest_IP( QStringLiteral( "10.0.2.15" ) );
		r.Set_Guest_Port( 22 );
		vm->Add_Network_Redirection( r );
	}

	if( ! disk_path.isEmpty() )
	{
		VM_HDD hda( true, disk_path );
		VM_Native_Storage_Device native = hda.Get_Native_Device();
		native.Use_Interface( true );
		native.Set_Interface( VM::DI_Virtio );
		native.Use_File_Path( true );
		native.Set_File_Path( disk_path );
		native.Use_Media( true );
		native.Set_Media( VM::DM_Disk );
		hda.Set_Native_Device( native );
		vm->Set_HDA( hda );
	}

	if( ! iso_path.isEmpty() && QFile::exists( iso_path ) )
	{
		// Non-native CD → Build_QEMU_Args uses if=ide,index=2 (no clash with virtio HDA).
		VM_Storage_Device cd( true, iso_path );
		cd.Set_Native_Device( VM_Native_Storage_Device() );
		vm->Set_CD_ROM( cd );
		QList<VM::Boot_Order> boot;
		VM::Set_Boot_Order_Enabled( boot, VM::Boot_From_CDROM, VM::Boot_From_HDD );
		vm->Set_Boot_Order_List( boot );
	}

	AQ_Inferno_Companion_Remember_Disk( disk_path );

	// Never put human notes in Additional_Args — QEMU treats bare tokens (e.g. "#")
	// as extra disk images → "drive with bus=0, unit=0 (index=0): exists".
	vm->Set_Additional_Args( QString() );
	vm->Set_HDB( VM_HDD() );
	vm->Set_HDC( VM_HDD() );
	vm->Set_HDD( VM_HDD() );
	vm->Set_Storage_Devices_List( QList<VM_Native_Storage_Device>() );
}

Virtual_Machine *AQ_Inferno_Companion_Create_VM( QWidget *parent,
                                                 const QString &preferred_name,
                                                 QString *error_out )
{
	auto fail = [&]( const QString &msg ) -> Virtual_Machine * {
		if( error_out )
			*error_out = msg;
		return nullptr;
	};

	const QString display = preferred_name.trimmed().isEmpty()
		? AQ_Inferno_Companion_OS_Name()
		: preferred_name.trimmed();
	const QString xml = Get_Complete_VM_File_Path( display );
	const QString base = QFileInfo( xml ).completeBaseName();
	const QString vm_dir = QFileInfo( xml ).absolutePath();
	const QString vm_folder = QDir( vm_dir ).filePath( base );

	QString disk, iso, err;
	if( ! AQ_Inferno_Companion_Prepare_Assets( parent, vm_folder, &disk, &iso, &err ) )
		return fail( err.isEmpty() ? QObject::tr( "Asset prepare cancelled." ) : err );

	Virtual_Machine *vm = new Virtual_Machine();
	vm->Set_Machine_Name( display );
	vm->Set_Computer_Type( QStringLiteral( "qemu-system-x86_64" ) );
	vm->Set_Emulator( Get_Default_Emulator() );
	vm->Set_Icon_Path( QStringLiteral( ":/other/other.png" ) );
	vm->Set_UID( QUuid::createUuid().toString() );
	vm->Set_VM_XML_File_Path( xml );

	AQ_Apply_Inferno_Companion_VM_Defaults( vm, disk, iso );

	if( ! vm->Create_VM_File( xml, false ) )
	{
		delete vm;
		return fail( QObject::tr( "Failed to write VM file:\n%1" ).arg( xml ) );
	}

	return vm;
}
