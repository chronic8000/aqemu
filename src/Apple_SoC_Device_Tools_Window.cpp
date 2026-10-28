#include "Apple_SoC_Device_Tools_Window.h"

#include "Apple_SoC_FS_Patch_Window.h"
#include "Apple_SoC_Support.h"
#include "Utils.h"
#include "VM.h"
#include "WSL_Launch.h"
#include "WSL_Secure_Credentials.h"
#include "WSL_Wizard_Window.h"
#include "AQ_UI_Style.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFormLayout>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QSettings>
#include <QStandardPaths>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QCoreApplication>

namespace {

QString Shell_Quote( QString s )
{
	return s.replace( QLatin1Char( '\'' ), QStringLiteral( "'\\''" ) );
}

} // namespace

void AQ_Show_Apple_SoC_Device_Tools_Window( Virtual_Machine *vm, QWidget *parent,
                                            int start_tab )
{
	auto *dlg = new Apple_SoC_Device_Tools_Window( vm, parent ? parent : QApplication::activeWindow() );
	dlg->setAttribute( Qt::WA_DeleteOnClose );
	dlg->setWindowModality( Qt::NonModal );
	dlg->Select_Tab( start_tab );
	dlg->show();
	dlg->raise();
	dlg->activateWindow();
}

Apple_SoC_Device_Tools_Window::Apple_SoC_Device_Tools_Window( Virtual_Machine *vm, QWidget *parent )
	: QDialog( parent )
	, VM( vm )
	, Tabs( nullptr )
	, Process( new QProcess( this ) )
{
	setWindowTitle( tr( "iOS Device Tools / Guest Internet" ) );

	QVBoxLayout *root_lay = new QVBoxLayout( this );
	root_lay->setContentsMargins( 8, 8, 8, 8 );
	root_lay->setSpacing( 6 );

	QScrollArea *scroll = new QScrollArea( this );
	scroll->setWidgetResizable( true );
	scroll->setFrameShape( QFrame::NoFrame );
	scroll->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	scroll->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );

	QWidget *content = new QWidget( scroll );
	content->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );
	QVBoxLayout *lay = new QVBoxLayout( content );
	lay->setContentsMargins( 4, 4, 4, 4 );
	lay->setSpacing( 8 );
	lay->setSizeConstraint( QLayout::SetMinimumSize );

	QLabel *intro = new QLabel( tr(
		"<b>Guest internet</b> is reverse-tether through the USB bridge on this PC "
		"(WSL on Windows, native on Linux). "
		"Power On iOS with USB remote <code>127.0.0.1:8030</code>. "
		"AQEMU picks the x86_64 or ARM64 tools for this machine and starts "
		"<code>usbmuxd</code> itself. No companion virtual machine." ) );
	intro->setWordWrap( true );
	lay->addWidget( intro );

	Edit_SSH_User = nullptr;
	Edit_SSH_Password = nullptr;
	Edit_SSH_Port = nullptr;

	Tabs = new QTabWidget( this );

	// --- Internet (first) ---
	auto *net = new QWidget();
	auto *netLay = new QVBoxLayout( net );
	QLabel *lblNet = new QLabel( tr(
		"<p>This shares this PC's network with iOS over the USB bridge:</p>"
		"<ol>"
		"<li><code>usbmuxd</code> with <code>USBMUXD_DEFAULT_DEVICE_MODE=3</code> (CDC ethernet)</li>"
		"<li>Address <code>10.42.0.1</code> on the USB ethernet interface</li>"
		"<li>NAT and DHCP on that interface. <code>idevice_id</code> stays empty in this mode</li>"
		"</ol>"
		"<p>Then open Safari / App Store on the guest. "
		"(ChefKiss <a href=\"https://github.com/ChefKissInc/Inferno/discussions/192\">#192</a>)</p>" ) );
	lblNet->setWordWrap( true );
	lblNet->setOpenExternalLinks( true );
	netLay->addWidget( lblNet );

	auto *btnEnable = new QPushButton( tr( "Enable guest internet (reverse-tether)" ) );
	btnEnable->setMinimumHeight( AQ_Px( 36, this ) );
	btnEnable->setStyleSheet( QStringLiteral(
		"QPushButton { font-weight: 600; padding: 8px 14px; }" ) );
	btnEnable->setToolTip( tr(
		"SSH into the companion and apply usbmuxd MODE=3 + nmcli Shared "
		"on non-primary ethernet interfaces." ) );
	connect( btnEnable, &QPushButton::clicked, this, &Apple_SoC_Device_Tools_Window::Enable_Guest_Internet );
	netLay->addWidget( btnEnable );

	auto *netBtns = new QHBoxLayout();
	auto *btnDiag = new QPushButton( tr( "Diagnose USB / idevice" ) );
	auto *btnHints = new QPushButton( tr( "Copy manual commands" ) );
	auto *btnGuide = new QPushButton( tr( "Open ChefKiss setup..." ) );
	connect( btnDiag, &QPushButton::clicked, this, &Apple_SoC_Device_Tools_Window::Run_Network_Diagnose );
	connect( btnHints, &QPushButton::clicked, this, &Apple_SoC_Device_Tools_Window::Apply_Reverse_Tether_Hints );
	connect( btnGuide, &QPushButton::clicked, this, []() {
		QDesktopServices::openUrl( QUrl( QStringLiteral(
			"https://chefkiss.dev/guides/inferno/companion-setup/" ) ) );
	} );
	netBtns->addWidget( btnDiag );
	netBtns->addWidget( btnHints );
	netBtns->addWidget( btnGuide );
	netBtns->addStretch();
	netLay->addLayout( netBtns );
	netLay->addStretch();
	Tabs->addTab( net, tr( "Internet" ) );

	// --- Device ---
	auto *dev = new QWidget();
	auto *devLay = new QVBoxLayout( dev );
	auto *devBtns = new QHBoxLayout();
	auto *btnInfo = new QPushButton( tr( "Device info" ) );
	auto *btnList = new QPushButton( tr( "List apps" ) );
	auto *btnIpa = new QPushButton( tr( "Install IPA..." ) );
	auto *btnShot = new QPushButton( tr( "Screenshot" ) );
	connect( btnInfo, &QPushButton::clicked, this, &Apple_SoC_Device_Tools_Window::Run_Device_Info );
	connect( btnList, &QPushButton::clicked, this, &Apple_SoC_Device_Tools_Window::Run_List_Apps );
	connect( btnIpa, &QPushButton::clicked, this, &Apple_SoC_Device_Tools_Window::Run_Install_IPA );
	connect( btnShot, &QPushButton::clicked, this, &Apple_SoC_Device_Tools_Window::Run_Screenshot );
	devBtns->addWidget( btnInfo );
	devBtns->addWidget( btnList );
	devBtns->addWidget( btnIpa );
	devBtns->addWidget( btnShot );
	devBtns->addStretch();
	devLay->addLayout( devBtns );

	Chk_Sign_IPA = new QCheckBox( tr( "Ad hoc sign IPA before install" ), dev );
	Chk_Sign_IPA->setChecked( false );
	Chk_Sign_IPA->setVisible( false );

	QLabel *lblDevInfo = new QLabel( tr(
		"Install IPA copies the app onto the iOS disk and does not call installd. "
		"Power Off iOS first. The IPA keeps its own signature and entitlements. "
		"The next Power On passes "
		"<code>amfi_get_out_of_my_way=1</code> "
		"on the kernel command line, which is the Boot Args line in the serial log. "
		"The first install builds a helper VM and copies the root image, so it takes a while." ) );
	lblDevInfo->setWordWrap( true );
	devLay->addWidget( lblDevInfo );
	devLay->addStretch();
	Tabs->addTab( dev, tr( "Device" ) );

	lay->addWidget( Tabs );

	Label_Status = new QLabel( tr(
		"Idle. Power On iOS with USB 127.0.0.1:8030. Tools run in WSL on Windows, and natively on Linux." ) );
	Label_Status->setWordWrap( true );
	lay->addWidget( Label_Status );

	Text_Log = new QTextEdit();
	Text_Log->setReadOnly( true );
	Text_Log->setLineWrapMode( QTextEdit::WidgetWidth );
	Text_Log->setMinimumHeight( AQ_Px( 120, this ) );
	lay->addWidget( new QLabel( tr( "Output:" ) ) );
	lay->addWidget( Text_Log, 1 );

	scroll->setWidget( content );
	root_lay->addWidget( scroll, 1 );

	auto *bottom = new QHBoxLayout();
	bottom->addStretch();
	auto *btnClose = new QPushButton( tr( "Close" ) );
	connect( btnClose, &QPushButton::clicked, this, &QDialog::accept );
	bottom->addWidget( btnClose );
	root_lay->addLayout( bottom );

	AQ_Intelligently_Size_Dialog( this, 750, 600 );

	connect( Process, &QProcess::readyReadStandardOutput, this, &Apple_SoC_Device_Tools_Window::On_Process_Output );
	connect( Process, &QProcess::readyReadStandardError, this, &Apple_SoC_Device_Tools_Window::On_Process_Output );
	connect( Process, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
	         this, &Apple_SoC_Device_Tools_Window::On_Process_Finished );
}

void Apple_SoC_Device_Tools_Window::Select_Tab( int index )
{
	if( Tabs && index >= 0 && index < Tabs->count() )
		Tabs->setCurrentIndex( index );
}

void Apple_SoC_Device_Tools_Window::Set_VM( Virtual_Machine *vm )
{
	VM = vm;
}

void Apple_SoC_Device_Tools_Window::Append_Log( const QString &text )
{
	if( text.isEmpty() )
		return;
	QString cleaned = text;
	cleaned.remove( QChar( '\r' ) );
	cleaned.replace( QRegularExpression( QStringLiteral( "\x1B\\[[0-9;?]*[A-Za-z]" ) ), QString() );
	cleaned.replace( QChar( 0x1B ), QString() );
	cleaned.replace( QStringLiteral( "\u2026" ), QStringLiteral( "..." ) );
	cleaned.replace( QStringLiteral( "\u2014" ), QStringLiteral( "-" ) );
	cleaned.replace( QStringLiteral( "\u2013" ), QStringLiteral( "-" ) );
	// Mojibake when UTF-8 was mis-decoded as Latin-1/CP1252
	cleaned.replace( QStringLiteral( "â€¦" ), QStringLiteral( "..." ) );
	cleaned.replace( QStringLiteral( "â€”" ), QStringLiteral( "-" ) );
	cleaned.replace( QStringLiteral( "â€“" ), QStringLiteral( "-" ) );
	cleaned.replace( QStringLiteral( "â€™" ), QStringLiteral( "'" ) );
	cleaned.replace( QStringLiteral( "â€œ" ), QStringLiteral( "\"" ) );
	cleaned.replace( QStringLiteral( "â€" ), QStringLiteral( "\"" ) );
	if( ! cleaned.trimmed().isEmpty() )
		Text_Log->append( cleaned );
}

bool Apple_SoC_Device_Tools_Window::Ensure_WSL_Creds( QString *distro_out, QString *user_out )
{
	QSettings s;
	QString distro = s.value( QStringLiteral( "WSL_Launch/Distro" ), QString() ).toString();
	QString user = s.value( QStringLiteral( "WSL_Launch/Username" ), QString() ).toString();
	if( distro.trimmed().isEmpty() || ! WSL_Is_Valid_Username( user ) )
	{
		WSL_Wizard_Window wizard( this );
		if( wizard.exec() != QDialog::Accepted )
			return false;
		distro = s.value( QStringLiteral( "WSL_Launch/Distro" ), QString() ).toString();
		user = s.value( QStringLiteral( "WSL_Launch/Username" ), QString() ).toString();
	}
	if( distro.trimmed().isEmpty() || ! WSL_Is_Valid_Username( user ) )
		return false;
	if( distro_out )
		*distro_out = distro.trimmed();
	if( user_out )
		*user_out = user;
	return true;
}

QStringList Apple_SoC_Device_Tools_Window::WSL_Bash_Args( const QString &distro, const QString &user,
                                                         const QString &script ) const
{
	QStringList args;
	args << QStringLiteral( "-d" ) << distro;
	const QString safe_user = WSL_Sanitize_Username( user );
	if( ! safe_user.isEmpty() )
		args << QStringLiteral( "-u" ) << safe_user;
	args << QStringLiteral( "-e" ) << QStringLiteral( "bash" )
	     << QStringLiteral( "--noprofile" ) << QStringLiteral( "--norc" )
	     << QStringLiteral( "-c" ) << script;
	return args;
}

bool Apple_SoC_Device_Tools_Window::Start_On_Host( int mode, const QString &script )
{
	if( Process->state() != QProcess::NotRunning )
	{
		QMessageBox::information( this, tr( "Busy" ), tr( "A command is already running." ) );
		return false;
	}
	const QString extras = AQ_Inferno_Extras_Dir();
	const QString host = extras + QStringLiteral( "/ios-host.sh" );
	if( extras.isEmpty() || ! QFile::exists( host ) )
	{
		QMessageBox::warning( this, tr( "iOS tools" ),
			tr( "Could not find extras/Inferno/ios-host.sh next to aqemu.exe." ) );
		return false;
	}

	auto quote = []( QString s ) {
		s.replace( QLatin1Char( '\'' ), QStringLiteral( "'\\''" ) );
		return QLatin1Char( '\'' ) + s + QLatin1Char( '\'' );
	};
	const QString inner = quote( script );
	const QString mode_s = QString::number( mode );

#ifdef Q_OS_WIN
	QString distro, user;
	if( ! Ensure_WSL_Creds( &distro, &user ) )
		return false;
	Q_UNUSED( user );
	const QString host_wsl = Windows_Path_To_WSL( host );
	const QString bash = QStringLiteral(
		"sed -i 's/\\r$//' %1 2>/dev/null || true; "
		"bash %1 %2 -- bash -c %3" )
		.arg( quote( host_wsl ), mode_s, inner );
	Text_Log->clear();
	Label_Status->setText( tr( "Running on this PC's WSL (native CPU)..." ) );
	Process->setProgram( QStringLiteral( "wsl.exe" ) );
	Process->setArguments( WSL_Bash_Args( distro, QStringLiteral( "root" ), bash ) );
	Process->setProcessChannelMode( QProcess::MergedChannels );
	Process->start();
	if( ! Process->waitForStarted( 8000 ) )
	{
		Label_Status->setText( tr( "Failed to start wsl.exe" ) );
		return false;
	}
	return true;
#else
	const QString bash = QStringLiteral(
		"sed -i 's/\\r$//' %1 2>/dev/null || true; "
		"bash %1 %2 -- bash -c %3" )
		.arg( quote( host ), mode_s, inner );
	Text_Log->clear();
	Label_Status->setText( tr( "Running on this PC..." ) );
	Process->setProgram( QStringLiteral( "bash" ) );
	Process->setArguments( QStringList() << QStringLiteral( "-c" ) << bash );
	Process->setProcessChannelMode( QProcess::MergedChannels );
	Process->start();
	if( ! Process->waitForStarted( 8000 ) )
	{
		Label_Status->setText( tr( "Failed to start bash" ) );
		return false;
	}
	return true;
#endif
}

void Apple_SoC_Device_Tools_Window::Run_Device_Info()
{
	// MODE=1 is lockdown. MODE=3 hides the phone from idevice_id.
	Start_On_Host( 1, QStringLiteral(
		"echo '=== Wait for idevice (MODE=1) ==='; "
		"ok=0; "
		"for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do "
		"  if idevice_id -l 2>/dev/null | grep -q .; then ok=1; break; fi; "
		"  echo waiting_for_device $i; sleep 2; "
		"done; "
		"echo '=== idevice_id ==='; "
		"idevice_id -l 2>&1 || true; "
		"echo '=== ideviceinfo (short) ==='; "
		"if [ \"$ok\" != 1 ]; then "
		"  echo 'ERROR: No device found. Power On iOS with USB 127.0.0.1:8030.'; "
		"  exit 1; "
		"fi; "
		"ideviceinfo -s 2>/dev/null || ideviceinfo | head -80; "
		"echo DONE" ) );
}

void Apple_SoC_Device_Tools_Window::Run_List_Apps()
{
	Start_On_Host( 1, QStringLiteral(
		"echo '=== Wait for idevice (MODE=1) ==='; "
		"ok=0; "
		"for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do "
		"  if idevice_id -l 2>/dev/null | grep -q .; then ok=1; break; fi; "
		"  echo waiting_for_device $i; sleep 2; "
		"done; "
		"if [ \"$ok\" != 1 ]; then "
		"  echo 'ERROR: No device found. Power On iOS with USB 127.0.0.1:8030.'; "
		"  exit 1; "
		"fi; "
		"if ! command -v ideviceinstaller >/dev/null 2>&1; then "
		"  echo 'ERROR: ideviceinstaller is not in this build of the host tools.'; "
		"  exit 127; "
		"fi; "
		"ideviceinstaller -l; "
		"echo DONE" ) );
}

void Apple_SoC_Device_Tools_Window::Run_Install_IPA()
{
	const QString ipa = QFileDialog::getOpenFileName( this, tr( "Select IPA" ),
		QString(), tr( "iOS apps (*.ipa);;All (*)" ) );
	if( ipa.isEmpty() )
		return;
	if( ! VM || ! AQ_Is_Apple_SoC_VM( VM ) )
	{
		QMessageBox::warning( this, tr( "Install IPA" ),
			tr( "Open Device Tools from an Apple SoC VM." ) );
		return;
	}
	const VM::VM_State st = VM->Get_State();
	if( st == VM::VMS_Running || st == VM::VMS_Pause || st == VM::VMS_Saved )
	{
		QMessageBox::warning( this, tr( "Install IPA" ),
			tr( "Power Off iOS first. The root disk cannot be changed while the guest is running." ) );
		return;
	}
	const QString root = AQ_Apple_SoC_Image_Dir( VM ) + QStringLiteral( "/root" );
	if( ! QFileInfo::exists( root ) )
	{
		QMessageBox::warning( this, tr( "Install IPA" ),
			tr( "iOS root image was not found:\n%1" ).arg( QDir::toNativeSeparators( root ) ) );
		return;
	}
	const QString extras = AQ_Inferno_Extras_Dir();
	const QString script_win = extras + QStringLiteral( "/install-app-wsl.sh" );
	if( extras.isEmpty() || ! QFile::exists( script_win ) )
	{
		QMessageBox::warning( this, tr( "Install IPA" ),
			tr( "Could not find extras/Inferno/install-app-wsl.sh next to aqemu.exe." ) );
		return;
	}
	if( Process->state() != QProcess::NotRunning )
	{
		QMessageBox::information( this, tr( "Busy" ), tr( "A command is already running." ) );
		return;
	}

#ifdef Q_OS_WIN
	QString distro, user;
	if( ! Ensure_WSL_Creds( &distro, &user ) )
		return;

	auto shell_quote = []( QString s ) {
		return s.replace( QLatin1Char( '\'' ), QStringLiteral( "'\\''" ) );
	};
	const QString bash = QStringLiteral(
		"set -e; "
		"SCRIPT='%1'; "
		"sed -i 's/\\r$//' \"$SCRIPT\" 2>/dev/null || true; "
		"export ROOT_IMG='%2'; "
		"export IPA='%3'; "
		"bash \"$SCRIPT\"" )
		.arg( shell_quote( Windows_Path_To_WSL( script_win ) ),
		      shell_quote( Windows_Path_To_WSL( root ) ),
		      shell_quote( Windows_Path_To_WSL( QFileInfo( ipa ).absoluteFilePath() ) ) );

	Text_Log->clear();
	Append_Log( tr( "Copying the app onto the iOS root. installd is not used." ) );
	Append_Log( tr( "The IPA's own signature and entitlements are kept." ) );
	Append_Log( tr( "ROOT_IMG=%1" ).arg( Windows_Path_To_WSL( root ) ) );
	Label_Status->setText( tr( "Installing onto the guest disk..." ) );
	Process->setProgram( QStringLiteral( "wsl.exe" ) );
	Process->setArguments( WSL_Bash_Args( distro, user, bash ) );
	Process->setProcessChannelMode( QProcess::MergedChannels );
	Process->start();
	if( ! Process->waitForStarted( 8000 ) )
		Label_Status->setText( tr( "Failed to start wsl.exe" ) );
#else
	auto shell_quote = []( QString s ) {
		return s.replace( QLatin1Char( '\'' ), QStringLiteral( "'\\''" ) );
	};
	const QString bash = QStringLiteral(
		"set -e; "
		"SCRIPT='%1'; "
		"sed -i 's/\\r$//' \"$SCRIPT\" 2>/dev/null || true; "
		"export ROOT_IMG='%2'; "
		"export IPA='%3'; "
		"bash \"$SCRIPT\"" )
		.arg( shell_quote( script_win ),
		      shell_quote( root ),
		      shell_quote( QFileInfo( ipa ).absoluteFilePath() ) );
	Text_Log->clear();
	Append_Log( tr( "Copying the app onto the iOS root. installd is not used." ) );
	Label_Status->setText( tr( "Installing onto the guest disk..." ) );
	Process->setProgram( QStringLiteral( "bash" ) );
	Process->setArguments( QStringList() << QStringLiteral( "-c" ) << bash );
	Process->setProcessChannelMode( QProcess::MergedChannels );
	Process->start();
	if( ! Process->waitForStarted( 8000 ) )
		Label_Status->setText( tr( "Failed to start bash" ) );
#endif
}

void Apple_SoC_Device_Tools_Window::Run_Screenshot()
{
	QString pictures = QStandardPaths::writableLocation( QStandardPaths::PicturesLocation );
	if( pictures.isEmpty() )
		pictures = QDir::homePath();
	const QString local = QFileDialog::getSaveFileName(
		this, tr( "Save iPhone screenshot" ),
		QDir( pictures ).filePath( QStringLiteral( "aqemu-ios.png" ) ),
		tr( "PNG image (*.png);;All (*)" ) );
	if( local.isEmpty() )
		return;

	const QFileInfo dest( local );
	QDir().mkpath( dest.absolutePath() );
#ifdef Q_OS_WIN
	const QString out_path = Windows_Path_To_WSL( dest.absoluteFilePath() );
#else
	const QString out_path = dest.absoluteFilePath();
#endif
	const QString shown = QDir::toNativeSeparators( dest.absoluteFilePath() );

	Start_On_Host( 1, QStringLiteral(
		"echo '=== idevicescreenshot ==='; "
		"idevicescreenshot %1; "
		"if [ ! -s %1 ]; then "
		"  echo 'ERROR: no screenshot file.'; "
		"  echo 'screenshotr needs a Developer disk image mounted on the phone.'; "
		"  exit 1; "
		"fi; "
		"echo 'DONE - saved: %2'" )
		.arg( Shell_Quote( out_path ), Shell_Quote( shown ) ) );
}

void Apple_SoC_Device_Tools_Window::Run_Network_Diagnose()
{
	Start_On_Host( 1, QStringLiteral(
		"echo '=== arch ==='; uname -m; "
		"echo '=== listener 8030 ==='; "
		"ss -ltnp 'sport = :8030' 2>/dev/null || netstat -ltn 2>/dev/null | grep 8030 || true; "
		"echo '=== lsusb (Apple) ==='; "
		"lsusb 2>/dev/null | grep -i apple || lsusb || true; "
		"echo '=== idevice_id ==='; "
		"idevice_id -l 2>&1 || true; "
		"echo '=== usbmuxd ==='; "
		"pgrep -a usbmuxd || echo NONE; "
		"echo '=== USB net ifaces ==='; "
		"ip -br link; "
		"echo DONE" ) );
}

void Apple_SoC_Device_Tools_Window::Enable_Guest_Internet()
{
	Start_On_Host( 3, QStringLiteral(
		"set +e; "
		"echo '=== USB ethernet (MODE=3) ==='; "
		"echo 'MODE=3 does not list the phone in idevice_id.'; "
		"ok=0; "
		"for i in 1 2 3 4 5 6 7 8 9 10; do "
		"  if ip -o link show 2>/dev/null | grep -q enx; then ok=1; break; fi; "
		"  echo waiting_for_usb_ethernet $i; sleep 2; "
		"done; "
		"ip -br link; "
		"if [ \"$ok\" != 1 ]; then "
		"  echo 'ERROR: no enx USB ethernet. Power On iOS with USB 127.0.0.1:8030 and retry.'; "
		"  exit 3; "
		"fi; "
		"if ! command -v dnsmasq >/dev/null 2>&1 || ! command -v iptables >/dev/null 2>&1; then "
		"  apt-get update -qq || true; "
		"  apt-get install -y dnsmasq iptables || true; "
		"fi; "
		"if ! command -v dnsmasq >/dev/null 2>&1; then "
		"  echo 'ERROR: dnsmasq is not installed.'; exit 2; "
		"fi; "
		"PRIMARY=$(ip route 2>/dev/null | awk '/default/{print $5; exit}'); "
		"echo primary_iface=$PRIMARY; "
		"sysctl -w net.ipv4.ip_forward=1; "
		"shared=0; "
		"IFACES=$(ip -o link show 2>/dev/null | awk -F': ' '{print $2}' | "
		"  sed 's/@.*//' | grep -E '^(enx|usb)' || true); "
		"for d in $IFACES; do "
		"  [ -z \"$d\" ] && continue; "
		"  [ \"$d\" = lo ] && continue; "
		"  [ -n \"$PRIMARY\" ] && [ \"$d\" = \"$PRIMARY\" ] && continue; "
		"  echo Sharing_iface $d; "
		"  ip link set \"$d\" up || true; "
		"  ip addr replace 10.42.0.1/24 dev \"$d\" || echo \"ERROR: could not address $d\"; "
		"  if [ -n \"$PRIMARY\" ]; then "
		"    iptables -t nat -C POSTROUTING -o \"$PRIMARY\" -j MASQUERADE 2>/dev/null || "
		"      iptables -t nat -A POSTROUTING -o \"$PRIMARY\" -j MASQUERADE; "
		"    iptables -C FORWARD -i \"$d\" -o \"$PRIMARY\" -j ACCEPT 2>/dev/null || "
		"      iptables -A FORWARD -i \"$d\" -o \"$PRIMARY\" -j ACCEPT; "
		"    iptables -C FORWARD -i \"$PRIMARY\" -o \"$d\" "
		"      -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null || "
		"      iptables -A FORWARD -i \"$PRIMARY\" -o \"$d\" "
		"      -m state --state RELATED,ESTABLISHED -j ACCEPT; "
		"  fi; "
		"  if [ -f /run/aqemu-dnsmasq-$d.pid ]; then "
		"    kill \"$(cat /run/aqemu-dnsmasq-$d.pid)\" 2>/dev/null || true; "
		"  fi; "
		"  dnsmasq --conf-file=/dev/null --interface=\"$d\" "
		"    --bind-interfaces --except-interface=lo "
		"    --dhcp-range=10.42.0.10,10.42.0.100,12h "
		"    --dhcp-option=option:router,10.42.0.1 "
		"    --dhcp-option=option:dns-server,10.42.0.1 "
		"    --pid-file=/run/aqemu-dnsmasq-$d.pid "
		"    --log-facility=/tmp/aqemu-dnsmasq-$d.log || echo \"ERROR: dnsmasq failed on $d\"; "
		"  shared=1; "
		"done; "
		"if [ \"$shared\" != 1 ]; then "
		"  echo 'No USB ethernet interface to share.'; "
		"  ip -br link; "
		"  exit 3; "
		"fi; "
		"echo '=== share recheck ==='; "
		"ip -br link; "
		"echo 'Gateway on this PC is 10.42.0.1.'; "
		"echo 'idevice_id stays empty in MODE=3.'; "
		"echo DONE" ) );
}

void Apple_SoC_Device_Tools_Window::Apply_Reverse_Tether_Hints()
{
	const QString hints = QStringLiteral(
		"# Companion reverse-tether (USB iface is usually enxdeadbeef2212)\n"
		"# Fix Ubuntu 'strictly unmanaged' ethernet, then Shared:\n"
		"\n"
		"sudo tee /etc/NetworkManager/conf.d/10-globally-managed-devices.conf >/dev/null <<'EOF'\n"
		"[keyfile]\n"
		"unmanaged-devices=*,except:type:wifi,except:type:wwan,except:type:ethernet\n"
		"EOF\n"
		"\n"
		"sudo tee /etc/NetworkManager/conf.d/99-aqemu-usb-ethernet.conf >/dev/null <<'EOF'\n"
		"[device]\n"
		"match-device=interface-name:enx*\n"
		"managed=true\n"
		"EOF\n"
		"\n"
		"sudo systemctl restart NetworkManager\n"
		"IFACE=$(ip -o link | awk -F': ' '{print $2}' | grep '^enx' | head -1)\n"
		"echo USB_IFACE=$IFACE\n"
		"sudo nmcli device set \"$IFACE\" managed yes\n"
		"sudo nmcli connection delete aqemu-share-$IFACE 2>/dev/null || true\n"
		"sudo nmcli connection add type ethernet ifname \"$IFACE\" "
		"con-name \"aqemu-share-$IFACE\" connection.interface-name \"$IFACE\" "
		"ipv4.method shared ipv6.method ignore autoconnect yes\n"
		"sudo nmcli connection up \"aqemu-share-$IFACE\" ifname \"$IFACE\"\n"
		"nmcli device\n"
		"idevice_id -l\n"
		"\n"
		"# Then try Safari / App Store on the guest.\n"
		"# Ref: https://github.com/ChefKissInc/Inferno/discussions/192\n" );
	Text_Log->clear();
	Append_Log( hints );
	QApplication::clipboard()->setText( hints );
	Label_Status->setText( tr( "Reverse-tether commands copied to clipboard." ) );
}

void Apple_SoC_Device_Tools_Window::On_Process_Output()
{
	const QByteArray raw = Process->readAll();
	// Prefer UTF-8; fall back if the companion console is not UTF-8.
	QString text = QString::fromUtf8( raw );
	if( text.contains( QChar::ReplacementCharacter ) )
		text = QString::fromLocal8Bit( raw );
	Append_Log( text );
}

void Apple_SoC_Device_Tools_Window::On_Process_Finished( int code, QProcess::ExitStatus st )
{
	On_Process_Output();
	const bool ok = ( st == QProcess::NormalExit && code == 0 );
	Label_Status->setText( ok ? tr( "Finished OK." )
	                          : tr( "Finished with errors (exit %1). See log." ).arg( code ) );
}
