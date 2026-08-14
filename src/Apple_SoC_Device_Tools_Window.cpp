#include "Apple_SoC_Device_Tools_Window.h"

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
#include <QHBoxLayout>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QSettings>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QCoreApplication>

namespace {

QString Shell_Quote( QString s )
{
	return s.replace( QLatin1Char( '\'' ), QStringLiteral( "'\\''" ) );
}

/** Commands run on the companion (inside ssh_do). ASCII-only. */
QString Companion_Ensure_Ideviceinstaller()
{
	return QStringLiteral(
		"if ! command -v ideviceinstaller >/dev/null 2>&1 && "
		"[ ! -x /usr/local/bin/ideviceinstaller ]; then "
		"  echo trying apt install ideviceinstaller...; "
		"  sudo -n apt-get update -qq 2>/dev/null; "
		"  sudo -n apt-get install -y ideviceinstaller 2>/dev/null "
		"    || sudo apt-get install -y ideviceinstaller 2>/dev/null "
		"    || true; "
		"fi; "
		"if [ -x /usr/local/bin/ideviceinstaller ]; then "
		"  export PATH=/usr/local/bin:$PATH; "
		"fi; "
		"if ! command -v ideviceinstaller >/dev/null 2>&1; then "
		"  echo ERROR: ideviceinstaller not found on companion.; "
		"  echo Install with: sudo apt-get install -y ideviceinstaller; "
		"  echo Or build https://github.com/libimobiledevice/ideviceinstaller "
		"into /usr/local/bin next to your other idevice tools.; "
		"  exit 127; "
		"fi; " );
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
	resize( AQ_Px( 740, this ), AQ_Px( 580, this ) );

	auto *lay = new QVBoxLayout( this );
	lay->addWidget( new QLabel( tr(
		"<b>Guest internet</b> is reverse-tether via the Ubuntu companion USB bridge "
		"(not the AQEMU Network tab / user-mode NIC). "
		"Start companion + iOS with USB remote <code>127.0.0.1:8030</code>, "
		"then use <b>Enable guest internet</b> below." ) ) );

	QSettings s;
	auto *form = new QFormLayout();
	Edit_SSH_User = new QLineEdit(
		s.value( QStringLiteral( "Apple_SoC_Restore/SSH_User" ), QString() ).toString() );
	Edit_SSH_Password = new QLineEdit();
	Edit_SSH_Password->setEchoMode( QLineEdit::Password );
	Edit_SSH_Password->setPlaceholderText( tr( "SSH password (also used for sudo on companion)" ) );
	Edit_SSH_Port = new QLineEdit( QStringLiteral( "32222" ) );
	form->addRow( tr( "Companion SSH user:" ), Edit_SSH_User );
	form->addRow( tr( "Companion SSH password:" ), Edit_SSH_Password );
	form->addRow( tr( "SSH port:" ), Edit_SSH_Port );
	lay->addLayout( form );

	Tabs = new QTabWidget( this );

	// --- Internet (first) ---
	auto *net = new QWidget();
	auto *netLay = new QVBoxLayout( net );
	netLay->addWidget( new QLabel( tr(
		"<p>This configures the <b>companion</b> for reverse-tether:</p>"
		"<ol>"
		"<li><code>usbmuxd</code> with <code>USBMUXD_DEFAULT_DEVICE_MODE=3</code></li>"
		"<li>NetworkManager <b>Shared</b> on USB/NCM ethernet (not the companion's main NIC)</li>"
		"<li>Checks <code>idevice_id</code> can see the iPhone guest</li>"
		"</ol>"
		"<p>Then open Safari / App Store on the guest. "
		"(ChefKiss <a href=\"https://github.com/ChefKissInc/Inferno/discussions/192\">#192</a>)</p>" ) ) );

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
	devLay->addWidget( new QLabel( tr(
		"IPA install uses <code>ideviceinstaller</code> on the companion "
		"(auto-tries <code>apt install ideviceinstaller</code> if missing). "
		"Enable guest internet first if the device is not listed." ) ) );
	devLay->addStretch();
	Tabs->addTab( dev, tr( "Device" ) );

	lay->addWidget( Tabs );

	Label_Status = new QLabel( tr(
		"Idle - companion must be running with SSH. Enter the same password as Apple SoC Restore." ) );
	Label_Status->setWordWrap( true );
	lay->addWidget( Label_Status );

	Text_Log = new QTextEdit();
	Text_Log->setReadOnly( true );
	lay->addWidget( new QLabel( tr( "Output:" ) ) );
	lay->addWidget( Text_Log, 1 );

	auto *bottom = new QHBoxLayout();
	bottom->addStretch();
	auto *btnClose = new QPushButton( tr( "Close" ) );
	connect( btnClose, &QPushButton::clicked, this, &QDialog::accept );
	bottom->addWidget( btnClose );
	lay->addLayout( bottom );

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

QString Apple_SoC_Device_Tools_Window::SSH_Base() const
{
	const QString guest = Edit_SSH_User->text().trimmed();
	const QString port = Edit_SSH_Port->text().trimmed().isEmpty()
		? QStringLiteral( "32222" ) : Edit_SSH_Port->text().trimmed();
	return QStringLiteral(
		"export SSHPASS='%1'; "
		"GUEST='%2'; "
		"PORT='%3'; "
		"if ! command -v sshpass >/dev/null 2>&1; then "
		"  sudo -n apt-get install -y sshpass 2>/dev/null || true; "
		"fi; "
		"SSH_OPTS=(-o StrictHostKeyChecking=accept-new "
		"-o UserKnownHostsFile=\"$HOME/.aqemu-companion-known_hosts\" "
		"-o ConnectTimeout=8 -o LogLevel=ERROR); "
		"ssh_do() { sshpass -e ssh -p \"$PORT\" \"${SSH_OPTS[@]}\" \"$GUEST@127.0.0.1\" \"$@\"; }; "
		"scp_do() { sshpass -e scp -P \"$PORT\" \"${SSH_OPTS[@]}\" \"$@\"; }; " )
		.arg( Shell_Quote( Edit_SSH_Password->text() ),
		      Shell_Quote( guest ),
		      Shell_Quote( port ) );
}

bool Apple_SoC_Device_Tools_Window::Start_Companion_SSH( const QString &remote_cmd )
{
	if( Process->state() != QProcess::NotRunning )
	{
		QMessageBox::information( this, tr( "Busy" ), tr( "A command is already running." ) );
		return false;
	}
	if( Edit_SSH_User->text().trimmed().isEmpty() || Edit_SSH_Password->text().isEmpty() )
	{
		QMessageBox::warning( this, tr( "SSH" ),
			tr( "Enter companion SSH user and password (same as Apple SoC Restore)." ) );
		return false;
	}

#ifdef Q_OS_WIN
	QString distro, user;
	if( ! Ensure_WSL_Creds( &distro, &user ) )
		return false;

	QSettings s;
	s.setValue( QStringLiteral( "Apple_SoC_Restore/SSH_User" ), Edit_SSH_User->text().trimmed() );

	const QString bash = SSH_Base() + remote_cmd;
	Text_Log->clear();
	Label_Status->setText( tr( "Running on companion via WSL SSH..." ) );
	Process->setProgram( QStringLiteral( "wsl.exe" ) );
	Process->setArguments( WSL_Bash_Args( distro, user, bash ) );
	Process->setProcessChannelMode( QProcess::MergedChannels );
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	Process->setProcessEnvironment( env );
	Process->start();
	if( ! Process->waitForStarted( 8000 ) )
	{
		Label_Status->setText( tr( "Failed to start wsl.exe" ) );
		return false;
	}
	return true;
#else
	Q_UNUSED( remote_cmd );
	QMessageBox::information( this, tr( "Host OS" ),
		tr( "Device Tools SSH helper is currently wired for Windows + WSL." ) );
	return false;
#endif
}

void Apple_SoC_Device_Tools_Window::Run_Device_Info()
{
	Start_Companion_SSH(
		"echo '=== idevice_id ==='; "
		"ssh_do 'idevice_id -l || true'; "
		"echo '=== ideviceinfo (short) ==='; "
		"ssh_do 'ideviceinfo -s 2>/dev/null || ideviceinfo | head -80'; "
		"echo DONE" );
}

void Apple_SoC_Device_Tools_Window::Run_List_Apps()
{
	// Remote script must stay ASCII (ellipsis becomes mojibake through WSL).
	const QString remote =
		Companion_Ensure_Ideviceinstaller() +
		QStringLiteral( "ideviceinstaller -l" );
	Start_Companion_SSH(
		QStringLiteral( "ssh_do '%1'; echo DONE" )
			.arg( Shell_Quote( remote ) ) );
}

void Apple_SoC_Device_Tools_Window::Run_Install_IPA()
{
	const QString ipa = QFileDialog::getOpenFileName( this, tr( "Select IPA" ),
		QString(), tr( "iOS apps (*.ipa);;All (*)" ) );
	if( ipa.isEmpty() )
		return;
	const QString wsl = Windows_Path_To_WSL( QFileInfo( ipa ).absoluteFilePath() );
	const QString remote_path = QStringLiteral( "/tmp/aqemu-install.ipa" );
	const QString remote =
		Companion_Ensure_Ideviceinstaller() +
		QStringLiteral( "ideviceinstaller -i " ) + remote_path;
	Start_Companion_SSH(
		QStringLiteral(
			"echo Uploading IPA...; "
			"scp_do '%1' \"$GUEST@127.0.0.1:%2\" || exit 1; "
			"echo Running ideviceinstaller...; "
			"ssh_do '%3'; "
			"echo DONE" )
			.arg( Shell_Quote( wsl ),
			      remote_path,
			      Shell_Quote( remote ) ) );
}

void Apple_SoC_Device_Tools_Window::Run_Screenshot()
{
	Start_Companion_SSH(
		"ssh_do 'idevicescreenshot /tmp/aqemu-ios.png 2>&1'; "
		"echo DONE - file on companion: /tmp/aqemu-ios.png" );
}

void Apple_SoC_Device_Tools_Window::Run_Network_Diagnose()
{
	Start_Companion_SSH(
		"echo '=== lsusb (Apple) ==='; "
		"ssh_do 'lsusb 2>/dev/null | grep -i apple || lsusb || true'; "
		"echo '=== idevice_id ==='; "
		"ssh_do 'idevice_id -l 2>&1 || true'; "
		"echo '=== usbmuxd ==='; "
		"ssh_do 'systemctl is-active usbmuxd 2>/dev/null; "
		"ps aux | grep -i usbmuxd | grep -v grep || true'; "
		"echo '=== USB net ifaces ==='; "
		"ssh_do 'nmcli -t -f DEVICE,TYPE,STATE,CONNECTION device 2>/dev/null || ip -br link'; "
		"echo DONE" );
}

void Apple_SoC_Device_Tools_Window::Enable_Guest_Internet()
{
	if( Edit_SSH_User->text().trimmed().isEmpty() || Edit_SSH_Password->text().isEmpty() )
	{
		QMessageBox::warning( this, tr( "SSH" ),
			tr( "Enter companion SSH user and password first "
			    "(same as Apple SoC Restore)." ) );
		return;
	}

	const QString pw = Shell_Quote( Edit_SSH_Password->text() );
	// ASCII-only remote script. Companion must already see the iOS USB device.
	const QString remote =
		QStringLiteral(
			"set +e; "
			"echo '=== 1) usbmuxd MODE=3 ==='; "
			"echo '%1' | sudo -S -p '' bash -c \""
			"mkdir -p /etc/systemd/system/usbmuxd.service.d && "
			"printf '%s\\n' '[Service]' 'Environment=USBMUXD_DEFAULT_DEVICE_MODE=3' "
			"> /etc/systemd/system/usbmuxd.service.d/aqemu-mode3.conf\"; "
			"echo '%1' | sudo -S -p '' systemctl daemon-reload; "
			"echo '%1' | sudo -S -p '' systemctl restart usbmuxd "
			"  || echo '%1' | sudo -S -p '' service usbmuxd restart; "
			"sleep 2; "
			"echo '=== 2) Wait for idevice ==='; "
			"ok=0; "
			"for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do "
			"  if idevice_id -l 2>/dev/null | grep -q .; then ok=1; break; fi; "
			"  echo waiting_for_device $i; sleep 2; "
			"done; "
			"idevice_id -l 2>&1; "
			"if [ \"$ok\" != 1 ]; then "
			"  echo WARN: no idevice yet. Power On iOS with USB 127.0.0.1:8030, then retry.; "
			"fi; "
			"echo '=== 3) Share non-primary ethernet (nmcli) ==='; "
			"if ! command -v nmcli >/dev/null 2>&1; then "
			"  echo ERROR: nmcli missing. Install NetworkManager on companion.; exit 2; "
			"fi; "
			/* Ubuntu Server marks ethernet strictly unmanaged; allow USB (enx*) only. */
			"echo '%1' | sudo -S -p '' bash -c \""
			"mkdir -p /etc/NetworkManager/conf.d && "
			"printf '%s\\n' "
			"'[keyfile]' "
			"'unmanaged-devices=*,except:type:wifi,except:type:wwan,"
			"except:type:ethernet' "
			"> /etc/NetworkManager/conf.d/10-globally-managed-devices.conf && "
			"printf '%s\\n' "
			"'[device]' "
			"'match-device=interface-name:enx*' "
			"'managed=true' "
			"> /etc/NetworkManager/conf.d/99-aqemu-usb-ethernet.conf\"; "
			"echo '%1' | sudo -S -p '' systemctl reload NetworkManager "
			"  || echo '%1' | sudo -S -p '' systemctl restart NetworkManager; "
			"sleep 2; "
			"PRIMARY=$(ip route 2>/dev/null | awk '/default/{print $5; exit}'); "
			"echo primary_iface=$PRIMARY; "
			"shared=0; "
			"IFACES=$(nmcli -t -f DEVICE,TYPE device 2>/dev/null | awk -F: '$2==\"ethernet\"{print $1}'); "
			"if [ -z \"$IFACES\" ]; then "
			"  IFACES=$(ip -o link show 2>/dev/null | awk -F': ' '{print $2}' | "
			"    grep -E '^(enx|usb)' || true); "
			"fi; "
			"for d in $IFACES; do "
			"  [ -z \"$d\" ] && continue; "
			"  [ \"$d\" = lo ] && continue; "
			"  [ -n \"$PRIMARY\" ] && [ \"$d\" = \"$PRIMARY\" ] && continue; "
			"  echo Sharing_iface $d; "
			"  echo '%1' | sudo -S -p '' nmcli device set \"$d\" managed yes; "
			"  echo '%1' | sudo -S -p '' nmcli device set \"$d\" autoconnect yes; "
			"  CON=aqemu-share-$d; "
			"  if ! nmcli -t -f NAME connection show 2>/dev/null | grep -Fx \"$CON\" >/dev/null; then "
			"    echo '%1' | sudo -S -p '' nmcli connection add type ethernet ifname \"$d\" "
			"      con-name \"$CON\" connection.interface-name \"$d\" "
			"      ipv4.method shared ipv6.method ignore autoconnect yes; "
			"  else "
			"    echo '%1' | sudo -S -p '' nmcli connection modify \"$CON\" "
			"      connection.interface-name \"$d\" "
			"      ipv4.method shared ipv6.method ignore autoconnect yes; "
			"  fi; "
			"  echo '%1' | sudo -S -p '' nmcli connection up \"$CON\" ifname \"$d\" "
			"    || echo '%1' | sudo -S -p '' nmcli device connect \"$d\"; "
			"  nmcli -t -f DEVICE,STATE,CONNECTION device | grep -F \"$d\" || true; "
			"  shared=1; "
			"done; "
			"if [ \"$shared\" != 1 ]; then "
			"  echo No secondary ethernet yet. Boot iOS, wait for USB NCM, click Enable again.; "
			"  nmcli -t -f DEVICE,TYPE,STATE,CONNECTION device; "
			"  ip -br link; "
			"  exit 3; "
			"fi; "
			"echo '=== 4) idevice recheck ==='; "
			"idevice_id -l 2>&1; "
			"echo DONE - try Safari or App Store on the guest." )
			.arg( pw );

	Start_Companion_SSH(
		QStringLiteral( "ssh_do '%1'; echo DONE" )
			.arg( Shell_Quote( remote ) ) );
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
