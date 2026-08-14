#include "Apple_SoC_Restore_Window.h"
#include "Apple_SoC_FS_Patch_Window.h"
#include "Apple_SoC_Support.h"
#include "Inferno_Companion_Setup.h"
#include "Main_Window.h"
#include "VM.h"
#include "Utils.h"
#include "WSL_Launch.h"
#include "WSL_Wizard_Window.h"
#include "AQ_UI_Style.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryFile>

namespace {

QString Shell_Single_Quote( const QString &raw )
{
	QString s = raw;
	s.replace( QLatin1Char( '\'' ), QLatin1String( "'\\''" ) );
	return QLatin1Char( '\'' ) + s + QLatin1Char( '\'' );
}

bool Is_Safe_Unix_Socket_Path( const QString &path )
{
	if( ! path.startsWith( QLatin1Char( '/' ) ) || path.contains( QLatin1String( ".." ) ) )
		return false;
	static const QRegularExpression re( QStringLiteral( "^[A-Za-z0-9_./-]+$" ) );
	return re.match( path ).hasMatch();
}

bool Is_Safe_IPv4_Or_Host( const QString &host )
{
	static const QRegularExpression re( QStringLiteral( "^[A-Za-z0-9.-]+$" ) );
	return ! host.isEmpty() && re.match( host ).hasMatch() && ! host.contains( QLatin1String( ".." ) );
}

// Linux TASK_COMM_LEN is 15, so pgrep -x qemu-system-x86_64-inferno never matches.
// Kill by /proc/PID/exe instead.
constexpr const char *kKill_Inferno_Companions = R"bash(
kill_inferno_companions() {
  sig="${1:--TERM}"
  for p in /proc/[0-9]*; do
    pid=${p#/proc/}
    exe=$(readlink -f "$p/exe" 2>/dev/null || true)
    case "$exe" in
      */qemu-system-x86_64-inferno)
        echo "Stopping companion PID $pid ($sig)..."
        kill "$sig" "$pid" 2>/dev/null || true
        ;;
    esac
  done
}
)bash";

} // namespace

Apple_SoC_Restore_Window::Apple_SoC_Restore_Window( Virtual_Machine *vm, QWidget *parent )
	: QDialog( parent )
	, VM( vm )
	, Process( new QProcess( this ) )
	, Companion_Process( new QProcess( this ) )
{
	setWindowTitle( tr( "Apple SoC Restore (Inferno companion)" ) );
	resize( AQ_Px( 780, this ), AQ_Px( 680, this ) );

	QVBoxLayout *lay = new QVBoxLayout( this );
	QLabel *intro = new QLabel( tr(
		"<p><b>IPSW restore (ChefKiss Inferno) — two guests at once</b></p>"
		"<ol>"
		"<li><b>Start companion</b> here (Inferno + <code>usb-tcp-remote</code> + your "
		"<code>companion.qcow2</code>). Do <b>not</b> also Power On the Ubuntu companion "
		"in the AQEMU list — same disk, one lock.</li>"
		"<li>Leave this window open <i>or</i> close it — companion now keeps running "
		"detached in WSL. SSH: <code>ssh -p 32222 USER@127.0.0.1</code>.</li>"
		"<li>In AQEMU, Power On <b>iOS (ARM64)</b> with USB remote "
		"<code>ipv4 127.0.0.1:8030</code> (same as below).</li>"
		"<li>Enter companion SSH user/password below, then "
		"<b>Restore IPSW via SSH</b> — AQEMU uploads the IPSW and runs "
		"<code>idevicerestore</code> inside the companion (no manual scp).</li>"
		"</ol>"
		"<p>Docs: "
		"<a href=\"https://chefkiss.dev/guides/inferno/companion-setup/\">Companion setup</a> · "
		"<a href=\"https://chefkiss.dev/guides/inferno/troubleshooting/\">Troubleshooting</a>.</p>" ) );
	intro->setWordWrap( true );
	intro->setOpenExternalLinks( true );
	lay->addWidget( intro );

	QFormLayout *form = new QFormLayout();
	Edit_IPSW = new QLineEdit( vm ? vm->Get_Apple_IPSW_Path() : QString() );
	QPushButton *btnIpsw = new QPushButton( tr( "Browse…" ) );
	connect( btnIpsw, &QPushButton::clicked, this, &Apple_SoC_Restore_Window::Browse_IPSW );
	QHBoxLayout *ipswLay = new QHBoxLayout();
	ipswLay->addWidget( Edit_IPSW, 1 );
	ipswLay->addWidget( btnIpsw );
	form->addRow( tr( "IPSW:" ), ipswLay );

	{
		QSettings s;
		Edit_Companion_Disk = new QLineEdit(
			s.value( QStringLiteral( "Apple_SoC_Restore/Companion_Disk" ) ).toString() );
		Edit_Companion_Disk->setPlaceholderText(
			tr( "Required: Linux disk image for companion VM (.qcow2 / .img / .raw)" ) );
		QPushButton *btnDisk = new QPushButton( tr( "Browse…" ) );
		connect( btnDisk, &QPushButton::clicked, this, &Apple_SoC_Restore_Window::Browse_Companion_Disk );
		QPushButton *btnCreate = new QPushButton( tr( "Create…" ) );
		btnCreate->setToolTip( tr(
			"Download Ubuntu Server ISO, create companion.qcow2, and show setup steps.\n"
			"(Same helper as New VM → Apple → iPhone IPSW Restore Companion.)" ) );
		connect( btnCreate, &QPushButton::clicked, this, &Apple_SoC_Restore_Window::Create_Companion_Helper );
		QHBoxLayout *diskLay = new QHBoxLayout();
		diskLay->addWidget( Edit_Companion_Disk, 1 );
		diskLay->addWidget( btnDisk );
		diskLay->addWidget( btnCreate );
		form->addRow( tr( "Companion disk:" ), diskLay );
	}

	{
		QSettings s;
		Edit_SSH_User = new QLineEdit(
			s.value( QStringLiteral( "Apple_SoC_Restore/SSH_User" ),
				QStringLiteral( "bob" ) ).toString() );
		Edit_SSH_User->setPlaceholderText( tr( "Linux user in companion (e.g. bob)" ) );
		Edit_SSH_Password = new QLineEdit();
		Edit_SSH_Password->setEchoMode( QLineEdit::Password );
		Edit_SSH_Password->setPlaceholderText( tr( "SSH / sudo password for that user" ) );
		form->addRow( tr( "Companion SSH user:" ), Edit_SSH_User );
		form->addRow( tr( "Companion SSH password:" ), Edit_SSH_Password );
	}

	CB_Conn_Type = new QComboBox();
	// Visual / ChefKiss: localhost (ipv4) often works better than unix — params must match both VMs.
	CB_Conn_Type->addItem( tr( "IPv4 localhost (recommended — match both VMs)" ), QStringLiteral( "ipv4" ) );
	CB_Conn_Type->addItem( tr( "UNIX socket" ), QStringLiteral( "unix" ) );
	QString ctype = vm ? vm->Get_Apple_USB_Conn_Type().trimmed().toLower() : QString();
#ifdef Q_OS_WIN32
	// Default / empty / leftover unix → ipv4 127.0.0.1:8030 (Inferno tip).
	if( ctype.isEmpty() || ctype == QLatin1String( "unix" ) )
	{
		const QString addr = vm ? vm->Get_Apple_USB_Conn_Addr().trimmed() : QString();
		// Keep unix only if the VM already has an explicit UNIX path.
		if( !( ctype == QLatin1String( "unix" ) && addr.startsWith( QLatin1Char( '/' ) ) ) )
			ctype = QStringLiteral( "ipv4" );
	}
#endif
	CB_Conn_Type->setCurrentIndex( ctype == QLatin1String( "unix" ) ? 1 : 0 );
	form->addRow( tr( "USB remote type:" ), CB_Conn_Type );

	Edit_Conn_Addr = new QLineEdit();
	{
		QString saved = vm ? vm->Get_Apple_USB_Conn_Addr().trimmed() : QString();
#ifdef Q_OS_WIN32
		if( CB_Conn_Type->currentData().toString() == QLatin1String( "ipv4" ) )
		{
			if( saved.isEmpty() || saved.startsWith( QLatin1Char( '/' ) ) )
				saved = QStringLiteral( "127.0.0.1" );
		}
		else if( saved.isEmpty() || ! saved.startsWith( QLatin1Char( '/' ) ) )
		{
			saved = QStringLiteral( "/tmp/InfernoUSBRemote" );
		}
#endif
		Edit_Conn_Addr->setText( saved );
		Edit_Conn_Addr->setPlaceholderText(
			tr( "IPv4: 127.0.0.1   |   UNIX: /tmp/InfernoUSBRemote" ) );
	}
	form->addRow( tr( "USB remote addr:" ), Edit_Conn_Addr );

	SB_Conn_Port = new QSpinBox();
	SB_Conn_Port->setRange( 1, 65535 );
	SB_Conn_Port->setValue( vm && vm->Get_Apple_USB_Conn_Port() > 0 ? vm->Get_Apple_USB_Conn_Port() : 8030 );
	SB_Conn_Port->setToolTip( tr( "IPv4 TCP port — must match companion and iOS guest (ChefKiss default 8030)." ) );
	form->addRow( tr( "USB remote port:" ), SB_Conn_Port );

	QPushButton *btnLocalhost = new QPushButton( tr( "Apply localhost preset (127.0.0.1:8030)" ) );
	btnLocalhost->setToolTip( tr(
		"Sets IPv4 + 127.0.0.1 + 8030 on this dialog and the VM "
		"(same values on companion and main VM — Inferno guidance)." ) );
	connect( btnLocalhost, &QPushButton::clicked, this, [this]() {
		CB_Conn_Type->setCurrentIndex( 0 ); // ipv4
		Edit_Conn_Addr->setText( QStringLiteral( "127.0.0.1" ) );
		SB_Conn_Port->setValue( 8030 );
		Sync_Conn_To_VM();
		if( VM )
			VM->Save_VM();
		Refresh_Companion_Snippet();
		QMessageBox::information( this, tr( "USB remote" ),
			tr( "Saved IPv4 127.0.0.1 port 8030 on this VM.\n\n"
			    "Also set MACHINE → USB remote to the same values if the UI "
			    "still shows UNIX/(none), then restart the iOS guest "
			    "after the companion is listening." ) );
	} );
	form->addRow( QString(), btnLocalhost );
	lay->addLayout( form );

	Label_Status = new QLabel();
	Label_Status->setWordWrap( true );
	lay->addWidget( Label_Status );

	Text_Companion = new QTextEdit();
	Text_Companion->setReadOnly( true );
	Text_Companion->setMaximumHeight( AQ_Px( 120, this ) );
	lay->addWidget( new QLabel( tr( "Companion QEMU (Linux guest + usb-tcp-remote; start BEFORE iOS):" ) ) );
	lay->addWidget( Text_Companion );

	QHBoxLayout *btns = new QHBoxLayout();
	QPushButton *btnCopy = new QPushButton( tr( "Copy companion command" ) );
	connect( btnCopy, &QPushButton::clicked, this, [this]() {
		QApplication::clipboard()->setText( Text_Companion->toPlainText() );
		QMessageBox::information( this, tr( "Copied" ), tr( "Companion command copied." ) );
	} );
	QPushButton *btnCompanion = new QPushButton( tr( "Start companion in WSL" ) );
	btnCompanion->setToolTip(
		tr( "Boots companion Linux with usb-tcp-remote (detached). SSH: localhost:32222" ) );
	connect( btnCompanion, &QPushButton::clicked, this, &Apple_SoC_Restore_Window::Start_Companion_WSL );
	QPushButton *btnStopCompanion = new QPushButton( tr( "Stop companion" ) );
	btnStopCompanion->setToolTip( tr( "Stop the detached Inferno companion in WSL" ) );
	connect( btnStopCompanion, &QPushButton::clicked, this, &Apple_SoC_Restore_Window::Stop_Companion_WSL );
	QPushButton *btnDiag = new QPushButton( tr( "Diagnose USB bridge" ) );
	connect( btnDiag, &QPushButton::clicked, this, &Apple_SoC_Restore_Window::Run_Diagnose_WSL );
	QPushButton *btnRestore = new QPushButton( tr( "Restore IPSW via SSH…" ) );
	btnRestore->setToolTip( tr(
		"Upload the IPSW into the companion over SSH and run idevicerestore there" ) );
	connect( btnRestore, &QPushButton::clicked, this, &Apple_SoC_Restore_Window::Run_IDeviceRestore );
	QPushButton *btnFsPatch = new QPushButton( tr( "Apply filesystem patches…" ) );
	btnFsPatch->setToolTip( tr(
		"After restore completes: patch the iOS guest root disk "
		"(InfernoFSPatcher + LaunchDaemons). Not the companion." ) );
	connect( btnFsPatch, &QPushButton::clicked, this, [this]() {
		AQ_Show_Apple_SoC_FS_Patch_Window( VM, this );
	} );
	btns->addWidget( btnCopy );
	btns->addWidget( btnCompanion );
	btns->addWidget( btnStopCompanion );
	btns->addWidget( btnDiag );
	btns->addWidget( btnRestore );
	btns->addWidget( btnFsPatch );
	btns->addStretch();
	QPushButton *btnClose = new QPushButton( tr( "Close" ) );
	connect( btnClose, &QPushButton::clicked, this, &QDialog::accept );
	btns->addWidget( btnClose );
	lay->addLayout( btns );

	Text_Log = new QTextEdit();
	Text_Log->setReadOnly( true );
	lay->addWidget( new QLabel( tr( "Output:" ) ) );
	lay->addWidget( Text_Log, 1 );

	connect( CB_Conn_Type, QOverload<int>::of( &QComboBox::currentIndexChanged ),
	         this, [this]( int ) {
		const QString t = Conn_Type();
		const QString a = Conn_Addr();
		if( t == QLatin1String( "ipv4" ) &&
		    ( a.isEmpty() || a.startsWith( QLatin1Char( '/' ) ) ) )
			Edit_Conn_Addr->setText( QStringLiteral( "127.0.0.1" ) );
		else if( t == QLatin1String( "unix" ) &&
		         ( a.isEmpty() || a == QLatin1String( "127.0.0.1" ) ||
		           a == QLatin1String( "localhost" ) ) )
			Edit_Conn_Addr->setText( QStringLiteral( "/tmp/InfernoUSBRemote" ) );
		Refresh_Companion_Snippet();
	} );
	connect( Edit_Conn_Addr, &QLineEdit::textChanged,
	         this, [this]( const QString & ) { Refresh_Companion_Snippet(); } );
	connect( SB_Conn_Port, QOverload<int>::of( &QSpinBox::valueChanged ),
	         this, [this]( int ) { Refresh_Companion_Snippet(); } );
	connect( Edit_Companion_Disk, &QLineEdit::textChanged,
	         this, [this]( const QString & ) { Refresh_Companion_Snippet(); } );

	connect( Process, &QProcess::readyReadStandardOutput, this, &Apple_SoC_Restore_Window::On_Process_Output );
	connect( Process, &QProcess::readyReadStandardError, this, &Apple_SoC_Restore_Window::On_Process_Output );
	connect( Process, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
	         this, &Apple_SoC_Restore_Window::On_Process_Finished );
	connect( Companion_Process, &QProcess::readyReadStandardOutput, this, &Apple_SoC_Restore_Window::On_Process_Output );
	connect( Companion_Process, &QProcess::readyReadStandardError, this, &Apple_SoC_Restore_Window::On_Process_Output );
	connect( Companion_Process, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
	         this, &Apple_SoC_Restore_Window::On_Companion_Finished );

	Refresh_Companion_Snippet();
}

QString Apple_SoC_Restore_Window::Conn_Type() const
{
	return CB_Conn_Type->currentData().toString();
}

QString Apple_SoC_Restore_Window::Conn_Addr() const
{
	return Edit_Conn_Addr->text().trimmed();
}

int Apple_SoC_Restore_Window::Conn_Port() const
{
	return SB_Conn_Port->value();
}

QString Apple_SoC_Restore_Window::Companion_Disk() const
{
	return Edit_Companion_Disk->text().trimmed();
}

void Apple_SoC_Restore_Window::Sync_Conn_To_VM()
{
	if( ! VM )
		return;
	VM->Set_Apple_USB_Conn_Type( Conn_Type() );
	VM->Set_Apple_USB_Conn_Addr( Conn_Addr() );
	VM->Set_Apple_USB_Conn_Port( Conn_Port() );
	if( ! Edit_IPSW->text().trimmed().isEmpty() )
		VM->Set_Apple_IPSW_Path( Edit_IPSW->text().trimmed() );
	QSettings s;
	s.setValue( QStringLiteral( "Apple_SoC_Restore/Companion_Disk" ), Companion_Disk() );
}

QString Apple_SoC_Restore_Window::Companion_Device_Arg() const
{
	const QString type = Conn_Type();
	const QString addr = Conn_Addr();
	QString remote = QStringLiteral( "usb-tcp-remote,bus=ehci.0,conn-type=%1" ).arg( type );
	if( type == QLatin1String( "unix" ) )
		remote += QStringLiteral( ",conn-addr=%1" ).arg( addr );
	else
		remote += QStringLiteral( ",conn-addr=%1,conn-port=%2" ).arg( addr ).arg( Conn_Port() );
	return remote;
}

void Apple_SoC_Restore_Window::Append_Log( const QString &text )
{
	if( text.isEmpty() )
		return;
	// Strip ANSI / SeaBIOS clears and normalize punctuation so the log stays readable.
	QString cleaned = text;
	cleaned.replace( QRegularExpression( QStringLiteral( "\x1B\\[[0-9;?]*[A-Za-z]" ) ), QString() );
	cleaned.replace( QChar( 0x1B ), QString() );
	cleaned.replace( QStringLiteral( "\u201C" ), QStringLiteral( "\"" ) );
	cleaned.replace( QStringLiteral( "\u201D" ), QStringLiteral( "\"" ) );
	cleaned.replace( QStringLiteral( "\u2014" ), QStringLiteral( "-" ) ); // em dash
	cleaned.replace( QStringLiteral( "\u2013" ), QStringLiteral( "-" ) ); // en dash
	cleaned.replace( QStringLiteral( "\u2026" ), QStringLiteral( "..." ) );
	// Mojibake when UTF-8 was mis-decoded as Latin-1/CP1252
	cleaned.replace( QStringLiteral( "â€œ" ), QStringLiteral( "\"" ) );
	cleaned.replace( QStringLiteral( "â€" ), QStringLiteral( "\"" ) );
	cleaned.replace( QStringLiteral( "â€”" ), QStringLiteral( "-" ) );
	cleaned.replace( QStringLiteral( "â€“" ), QStringLiteral( "-" ) );
	cleaned.replace( QStringLiteral( "â€¦" ), QStringLiteral( "..." ) );
	cleaned.replace( QStringLiteral( "â†’" ), QStringLiteral( "->" ) );
	if( ! cleaned.trimmed().isEmpty() )
		Text_Log->append( cleaned );
}

bool Apple_SoC_Restore_Window::Ensure_WSL_Creds( QString *distro_out, QString *user_out )
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

QStringList Apple_SoC_Restore_Window::WSL_Bash_Args( const QString &distro, const QString &user,
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

void Apple_SoC_Restore_Window::Browse_IPSW()
{
	const QString f = QFileDialog::getOpenFileName( this, tr( "Select IPSW" ),
		Edit_IPSW->text(), tr( "IPSW (*.ipsw *.zip);;All (*)" ) );
	if( ! f.isEmpty() )
	{
		Edit_IPSW->setText( QDir::toNativeSeparators( f ) );
		if( VM )
			VM->Set_Apple_IPSW_Path( f );
	}
}

void Apple_SoC_Restore_Window::Browse_Companion_Disk()
{
	const QString f = QFileDialog::getOpenFileName( this, tr( "Select companion Linux disk" ),
		Edit_Companion_Disk->text(),
		tr( "Disk images (*.qcow2 *.img *.raw *.vmdk);;All (*)" ) );
	if( ! f.isEmpty() )
	{
		Edit_Companion_Disk->setText( QDir::toNativeSeparators( f ) );
		AQ_Inferno_Companion_Remember_Disk( f );
		Refresh_Companion_Snippet();
	}
}

void Apple_SoC_Restore_Window::Create_Companion_Helper()
{
	const auto ans = QMessageBox::question( this, tr( "Create IPSW restore companion VM" ),
		tr( "<p>AQEMU will create a real machine in your VM list:</p>"
		    "<ul>"
		    "<li>Download Ubuntu Server ISO (if needed)</li>"
		    "<li>Create <code>companion.qcow2</code></li>"
		    "<li><b>Mount the ISO as CD-ROM</b> and boot from it</li>"
		    "<li>Enable SSH on host port <b>32222</b></li>"
		    "</ul>"
		    "<p>You install Ubuntu in that VM, then run it "
		    "<b>alongside</b> your iOS VM for restore.</p>"
		    "<p>Continue?</p>" ),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
	if( ans != QMessageBox::Yes )
		return;

	QString err;
	Virtual_Machine *vm = AQ_Inferno_Companion_Create_VM( this,
		AQ_Inferno_Companion_OS_Name(), &err );
	if( ! vm )
	{
		QMessageBox::warning( this, tr( "Companion setup" ),
			err.isEmpty() ? tr( "Setup cancelled or failed." ) : err );
		return;
	}

	Edit_Companion_Disk->setText( QDir::toNativeSeparators( vm->Get_HDA().Get_File_Name() ) );
	Refresh_Companion_Snippet();

	// Add to main window list if we can find it.
	Main_Window *main = nullptr;
	for( QWidget *w = parentWidget(); w; w = w->parentWidget() )
	{
		main = qobject_cast<Main_Window *>( w );
		if( main )
			break;
	}
	if( main )
		main->Add_VM_To_List( vm );
	else
	{
		QMessageBox::information( this, tr( "Companion VM created" ),
			tr( "VM file written:\n%1\n\nRestart AQEMU or use File → Load VM if it "
			    "does not appear in the list." ).arg( vm->Get_VM_XML_File_Path() ) );
		delete vm;
		vm = nullptr;
	}

	QSettings s;
	const QString user_hint = s.value( QStringLiteral( "WSL_Launch/Username" ),
		QStringLiteral( "ubuntu" ) ).toString();
	const QString disk = Edit_Companion_Disk->text();
	const QString iso = disk.isEmpty() ? QString()
		: QDir( QFileInfo( disk ).absolutePath() )
			.filePath( AQ_Inferno_Companion_Ubuntu_ISO_FileName() );
	AQ_Inferno_Companion_Show_Notes( this,
		AQ_Inferno_Companion_Post_Install_Notes( disk, iso, user_hint ) );
}

void Apple_SoC_Restore_Window::Refresh_Companion_Snippet()
{
	const QString type = Conn_Type();
	const QString addr = Conn_Addr();
	const QString disk = Companion_Disk();
	SB_Conn_Port->setEnabled( type == QLatin1String( "ipv4" ) );

	if( addr.isEmpty() )
	{
		Text_Companion->setPlainText(
			tr( "# Set USB remote address (same value as the MACHINE tab).\n"
			    "# WSL: UNIX /tmp/InfernoUSBRemote" ) );
		Label_Status->setText( tr( "<span style='color:#a60'>Set USB remote address.</span>" ) );
		return;
	}

	const bool addr_ok = ( type == QLatin1String( "unix" ) )
		? Is_Safe_Unix_Socket_Path( addr )
		: Is_Safe_IPv4_Or_Host( addr );
	if( ! addr_ok )
	{
		Text_Companion->setPlainText(
			tr( "# Invalid USB remote address.\n"
			    "# UNIX path example: /tmp/InfernoUSBRemote" ) );
		Label_Status->setText( tr( "<span style='color:#a60'>Invalid USB remote address.</span>" ) );
		return;
	}

	Sync_Conn_To_VM();

	const QString remote = Companion_Device_Arg();
	QString disk_line = QStringLiteral( "# WARNING: no companion disk — USB bridge only; "
	                                    "idevicerestore cannot run (no Linux guest).\n" );
	QString mem = QStringLiteral( "512M" );
	// q35 defaults to an e1000e NIC that needs efi-e1000e.rom — disable unless we add SSH net.
	QString nic_args = QStringLiteral( "  -nic none \\\n" );
	if( ! disk.isEmpty() )
	{
		disk_line = QStringLiteral( "# Disk: %1\n" ).arg( disk );
		mem = QStringLiteral( "2048M" );
		const QString fmt = disk.endsWith( QLatin1String( ".qcow2" ), Qt::CaseInsensitive )
			? QStringLiteral( "qcow2" )
			: QStringLiteral( "raw" );
#ifdef Q_OS_WIN32
		const QString disk_path = Windows_Path_To_WSL( disk );
#else
		const QString disk_path = disk;
#endif
		nic_args = QStringLiteral(
			"  -drive file=%1,if=ide,format=%2,index=0,media=disk \\\n"
			"  -nic none \\\n"
			"  -netdev user,id=net0,hostfwd=tcp::32222-:22 \\\n"
			"  -device virtio-net-pci,netdev=net0,rombar=0 \\\n" )
			.arg( Shell_Single_Quote( disk_path ), fmt );
	}

	Text_Companion->setPlainText(
		disk_line +
		QStringLiteral(
			"/usr/local/bin/qemu-system-x86_64-inferno \\\n"
			"  -L /usr/share/qemu -bios /usr/share/seabios/bios-256k.bin \\\n"
			"  -M q35 -m %1 -accel tcg -nographic \\\n"
			"%2"
			"  -usb -device usb-ehci,id=ehci -device %3\n"
			"\n"
			"# Guest -machine must match (do not leave as Inferno UNIX default):\n"
			"#   usb-conn-type=%4,usb-conn-addr=%5%6\n"
			"# After boot: ssh -p 32222 user@127.0.0.1 → lsusb → idevicerestore\n" )
			.arg( mem,
			      nic_args,
			      Shell_Single_Quote( remote ),
			      type,
			      addr,
			      type == QLatin1String( "ipv4" )
				  ? QStringLiteral( ",usb-conn-port=%1" ).arg( Conn_Port() )
				  : QString() ) );

	if( disk.isEmpty() )
	{
		Label_Status->setText( tr(
			"<span style='color:#a60'><b>Missing companion disk.</b> "
			"A diskless companion only binds the USB socket — "
			"<code>idevice_id</code> on the WSL host will always fail "
			"(<code>Unable to retrieve device list</code>). "
			"Install a lightweight Linux in a qcow2/img and select it above.</span>" ) );
	}
	else if( type == QLatin1String( "ipv4" ) )
	{
		Label_Status->setText( tr(
			"<span style='color:#060'><b>IPv4 localhost:</b> companion and iOS guest must both "
			"use <code>usb-conn-type=ipv4,usb-conn-addr=%1,usb-conn-port=%2</code> "
			"(same WSL distro). Start companion, restart iOS, then "
			"<code>lsusb</code> / <code>idevicerestore</code> <b>inside companion Linux</b>.</span>" )
			.arg( addr.toHtmlEscaped() ).arg( Conn_Port() ) );
	}
	else
	{
		Label_Status->setText( tr(
			"<span style='color:#060'><b>UNIX mode:</b> if the bridge fails, try the "
			"<b>Apply localhost preset</b> (Inferno tip). Still run restore tools "
			"inside companion Linux.</span>" ) );
	}
}

void Apple_SoC_Restore_Window::Start_Companion_WSL()
{
	Refresh_Companion_Snippet();
	const QString type = Conn_Type();
	const QString addr = Conn_Addr();
	const QString disk = Companion_Disk();
	if( addr.isEmpty() )
	{
		QMessageBox::warning( this, tr( "USB remote" ),
			tr( "Set USB remote address first." ) );
		return;
	}
	if( type == QLatin1String( "unix" ) && ! Is_Safe_Unix_Socket_Path( addr ) )
	{
		QMessageBox::warning( this, tr( "USB remote" ),
			tr( "UNIX socket path is invalid." ) );
		return;
	}

	if( disk.isEmpty() || ! QFile::exists( disk ) )
	{
		const auto ans = QMessageBox::warning( this, tr( "Companion disk required" ),
			tr( "ChefKiss expects a full companion <b>Linux</b> VM.\n\n"
			    "Without a disk image, AQEMU can only start the USB socket bridge. "
			    "The iPhone will never show up for <code>idevice_id</code> / "
			    "<code>idevicerestore</code> on the WSL host.\n\n"
			    "Create a small Linux disk (Arch/Artix/Ubuntu), install usbmuxd + "
			    "patched idevicerestore inside it, then select that image here.\n\n"
			    "Start diskless bridge anyway?" ),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
		if( ans != QMessageBox::Yes )
			return;
	}

	// Do NOT Kill_Orphan here — that was killing the detached Inferno companion
	// (write lock loop). If AQEMU's Ubuntu companion is Running, Power Off that
	// VM in the list first; Start companion will fail with a clear lock error otherwise.

#ifdef Q_OS_WIN32
	QString distro, user;
	if( ! Ensure_WSL_Creds( &distro, &user ) )
	{
		QMessageBox::warning( this, tr( "WSL" ),
			tr( "WSL distro and username are required." ) );
		return;
	}

	if( Companion_Process->state() != QProcess::NotRunning )
	{
		QMessageBox::information( this, tr( "Companion" ),
			tr( "Companion launcher is still busy — wait a moment." ) );
		return;
	}

	const QString remote = Companion_Device_Arg();
	const QString ipsw_win = Edit_IPSW ? Edit_IPSW->text().trimmed() : QString();
	QString ipsw_dir_wsl;
	QString ipsw_file_wsl;
	QString ticket_file_wsl;
	if( ! ipsw_win.isEmpty() && QFile::exists( ipsw_win ) )
	{
		const QFileInfo fi( ipsw_win );
		ipsw_dir_wsl = Windows_Path_To_WSL( fi.absolutePath() );
		ipsw_file_wsl = Windows_Path_To_WSL( fi.absoluteFilePath() );
	}
	if( VM )
	{
		const QString ticket = VM->Get_Apple_Ticket_Path().trimmed();
		if( ! ticket.isEmpty() && QFile::exists( ticket ) &&
		    QFileInfo( ticket ).size() > 100 )
			ticket_file_wsl = Windows_Path_To_WSL( QFileInfo( ticket ).absoluteFilePath() );
	}

	QString script = QString::fromUtf8( kKill_Inferno_Companions ) + QStringLiteral(
		"set -e; "
		"BIN=/usr/local/bin/qemu-system-x86_64-inferno; "
		"test -x \"$BIN\" || BIN=$(command -v qemu-system-x86_64-inferno); "
		"test -n \"$BIN\" || { echo 'qemu-system-x86_64-inferno not found'; exit 127; }; "
		"DATADIR=; "
		"for d in /usr/share/qemu /usr/local/share/qemu; do "
		"  [ -d \"$d\" ] && DATADIR=$d && break; "
		"done; "
		"BIOS=; "
		"for b in /usr/share/seabios/bios-256k.bin "
		"/usr/share/qemu/bios-256k.bin /usr/local/share/qemu/bios-256k.bin; do "
		"  [ -f \"$b\" ] && BIOS=$b && break; "
		"done; "
		"test -n \"$BIOS\" || { "
		"  echo 'bios-256k.bin not found — install seabios (and qemu-system-data) in WSL'; "
		"  exit 127; "
		"}; "
		"EXTRA=(); "
		"[ -n \"$DATADIR\" ] && EXTRA+=(-L \"$DATADIR\"); "
		"EXTRA+=(-bios \"$BIOS\"); " );

	// Always disable the q35 default NIC first — missing efi-e1000e.rom aborts Inferno QEMU.
	script += QStringLiteral( "EXTRA+=(-nic none); " );

	if( ! disk.isEmpty() && QFile::exists( disk ) )
	{
		const QString wsl_disk = Windows_Path_To_WSL( disk );
		script += QStringLiteral(
			"DISK=%1; "
			"test -f \"$DISK\" || { echo \"Companion disk not found: $DISK\"; exit 127; }; "
			"case \"$DISK\" in *.qcow2|*.QCOW2) FMT=qcow2;; *) FMT=raw;; esac; "
			// IDE matches AQEMU install path (SeaBIOS). Virtio-blk needs efi-virtio.rom
			// and often prints nothing on -serial when the guest was installed via VNC.
			"EXTRA+=(-drive \"file=$DISK,if=ide,format=$FMT,index=0,media=disk\"); "
			"EXTRA+=(-netdev user,id=net0,hostfwd=tcp::32222-:22); "
			// romfile= avoids missing efi-virtio.rom abort / noise; guest already uses virtio-net.
			"EXTRA+=(-device virtio-net-pci,netdev=net0,romfile=,rombar=0); "
			"EXTRA+=(-m 2048M); "
			"echo \"Companion disk: $DISK ($FMT, ide)\"; "
			"echo 'SSH (after Linux boots - serial may stay quiet): ssh -p 32222 USER@127.0.0.1'; "
			"echo 'Note: little serial output is normal if Ubuntu was installed without console=ttyS0.'; "
			"echo 'TCG + disk on /mnt/c: first SSH can take 1-3 minutes after Start.'; " )
			.arg( Shell_Single_Quote( wsl_disk ) );
	}
	else
	{
		script += QStringLiteral(
			"EXTRA+=(-m 512M); "
			"echo 'WARNING: diskless companion — USB bridge only; restore tools will not see the iPhone on WSL host.'; " );
	}

	// Share IPSW folder via 9p. Apple IPSW names often contain commas (iPhone11,8,...)
	// which break guest shell/`test -f` and can be awkward on 9p — stage a stable
	// hardlink name on the NTFS volume (no extra space) and use that in Restore.
	if( ! ipsw_dir_wsl.isEmpty() && ! ipsw_file_wsl.isEmpty() )
	{
		script += QStringLiteral(
			"IPSW_DIR=%1; "
			"IPSW_FILE=%2; "
			"test -d \"$IPSW_DIR\" || { echo \"IPSW folder not found: $IPSW_DIR\"; exit 127; }; "
			"test -f \"$IPSW_FILE\" || { echo \"IPSW file not found: $IPSW_FILE\"; exit 127; }; "
			"IPSW_SHARE=$(readlink -f \"$IPSW_DIR\" 2>/dev/null || echo \"$IPSW_DIR\"); "
			"STAGE_NAME=aqemu-restore-current.ipsw; "
			"STAGE_PATH=\"$IPSW_SHARE/$STAGE_NAME\"; "
			"rm -f \"$STAGE_PATH\" 2>/dev/null || true; "
			"staged=0; "
			"if command -v wslpath >/dev/null 2>&1; then "
			"  w_link=$(wslpath -w \"$STAGE_PATH\"); "
			"  w_tgt=$(wslpath -w \"$IPSW_FILE\"); "
			"  if cmd.exe /c mklink /H \"${w_link}\" \"${w_tgt}\" "
			">/tmp/aqemu-ipsw-stage.err 2>&1; then "
			"    staged=1; echo \"Staged IPSW hardlink: $STAGE_PATH\"; "
			"  else echo 'Hardlink staging failed (will try symlink):'; "
			"    cat /tmp/aqemu-ipsw-stage.err 2>/dev/null || true; fi; "
			"fi; "
			"if [ \"$staged\" != 1 ]; then "
			"  ln -sfn \"$IPSW_FILE\" \"$STAGE_PATH\" && staged=1 && "
			"    echo \"Staged IPSW symlink: $STAGE_PATH\"; "
			"fi; "
			"test \"$staged\" = 1 || { echo 'Could not stage IPSW under a comma-free name'; exit 127; }; "
			"TICKET_FILE=%3; "
			"if [ -n \"$TICKET_FILE\" ] && [ -f \"$TICKET_FILE\" ]; then "
			"  TSTAGE=\"$IPSW_SHARE/aqemu-restore-ticket.der\"; "
			"  rm -f \"$TSTAGE\" 2>/dev/null || true; "
			"  if command -v wslpath >/dev/null 2>&1 && "
			"     cmd.exe /c mklink /H \"$(wslpath -w \"$TSTAGE\")\" \"$(wslpath -w \"$TICKET_FILE\")\" "
			"     >/tmp/aqemu-ticket-stage.err 2>&1; then "
			"    echo \"Staged ticket hardlink: $TSTAGE\"; "
			"  elif cp -f \"$TICKET_FILE\" \"$TSTAGE\" 2>/dev/null; then "
			"    echo \"Staged ticket copy: $TSTAGE\"; "
			"  else echo 'WARNING: could not stage root_ticket.der into IPSW share'; fi; "
			"else "
			"  echo 'WARNING: no usable restore ticket (VM ticket missing or tiny placeholder)'; "
			"fi; "
			"rm -f /var/tmp/aqemu-ipsw-share 2>/dev/null || true; "
			"EXTRA+=(-virtfs \"local,id=aqemu_ipsw,path=${IPSW_SHARE},"
			"security_model=none,readonly=on,mount_tag=aqemu_ipsw\"); "
			"echo \"IPSW 9p share: $IPSW_SHARE (mount_tag=aqemu_ipsw)\"; "
			"echo \"Guest path: /mnt/aqemu_ipsw/$STAGE_NAME\"; "
			"echo 'Note: .ipsw and restore .zip are both OK for idevicerestore.'; " )
			.arg( Shell_Single_Quote( ipsw_dir_wsl ),
			      Shell_Single_Quote( ipsw_file_wsl ),
			      Shell_Single_Quote( ticket_file_wsl ) );
	}
	else if( ! ipsw_dir_wsl.isEmpty() )
	{
		script += QStringLiteral(
			"IPSW_DIR=%1; "
			"test -d \"$IPSW_DIR\" || { echo \"IPSW folder not found: $IPSW_DIR\"; exit 127; }; "
			"IPSW_SHARE=$(readlink -f \"$IPSW_DIR\" 2>/dev/null || echo \"$IPSW_DIR\"); "
			"EXTRA+=(-virtfs \"local,id=aqemu_ipsw,path=${IPSW_SHARE},"
			"security_model=none,readonly=on,mount_tag=aqemu_ipsw\"); "
			"echo \"IPSW 9p share: $IPSW_SHARE (mount_tag=aqemu_ipsw)\"; " )
			.arg( Shell_Single_Quote( ipsw_dir_wsl ) );
	}
	else
	{
		script += QStringLiteral(
			"echo 'No IPSW selected - Restore needs Start with IPSW Browse set (9p share).'; " );
	}

	if( type == QLatin1String( "unix" ) )
		script += QStringLiteral( "rm -f %1; " ).arg( Shell_Single_Quote( addr ) );

	script += QStringLiteral(
		"echo \"Companion BIOS: $BIOS\"; "
		"[ -n \"$DATADIR\" ] && echo \"Companion -L: $DATADIR\"; "
		"LOG=/tmp/aqemu-inferno-companion.log; "
		"PIDF=/tmp/aqemu-inferno-companion.pid; "
		"WANT_USB=%2; "
		"WANT_IPSW=%3; "
		"if [ -f \"$PIDF\" ]; then "
		"  old=$(cat \"$PIDF\" 2>/dev/null || true); "
		"  if [ -n \"$old\" ] && kill -0 \"$old\" 2>/dev/null; then "
		"    cmd=$(tr '\\0' ' ' < /proc/$old/cmdline 2>/dev/null || true); "
		"    usb_ok=0; ipsw_ok=0; "
		"    echo \"$cmd\" | grep -Fq \"$WANT_USB\" && usb_ok=1; "
		"    if [ \"$WANT_IPSW\" = 0 ]; then ipsw_ok=1; "
		"    elif echo \"$cmd\" | grep -Fq 'mount_tag=aqemu_ipsw'; then ipsw_ok=1; fi; "
		"    if [ \"$usb_ok\" = 1 ] && [ \"$ipsw_ok\" = 1 ]; then "
		"      echo \"Companion already running (PID $old) with matching USB/IPSW share.\"; "
		"      echo \"SSH: ssh -p 32222 USER@127.0.0.1\"; "
		"      exit 0; "
		"    fi; "
		"    echo \"Companion PID $old needs restart (USB or IPSW 9p share mismatch).\"; "
		"    kill -TERM \"$old\" 2>/dev/null || true; "
		"    sleep 2; "
		"    kill -KILL \"$old\" 2>/dev/null || true; "
		"    rm -f \"$PIDF\"; "
		"    kill_inferno_companions -TERM; sleep 1; "
		"    kill_inferno_companions -KILL; sleep 1; "
		"  fi; "
		"fi; "
		// setsid: survive WSL bash exit (nohup alone still got SIGHUP on WSL).
		"setsid \"$BIN\" \"${EXTRA[@]}\" -M q35 -accel tcg "
		"-nographic -monitor none -serial mon:stdio "
		"-usb -device usb-ehci,id=ehci -device %1 "
		"</dev/null >\"$LOG\" 2>&1 & "
		"echo $! >\"$PIDF\"; "
		"sleep 1; "
		"pid=$(cat \"$PIDF\"); "
		"if kill -0 \"$pid\" 2>/dev/null; then "
		"  echo \"Companion detached PID $pid (log $LOG).\"; "
		"  echo 'Waiting for guest SSH on :32222 (TCG boot often needs 1-3 minutes)...'; "
		"  ssh_up=0; "
		"  for i in $(seq 1 60); do "
		"    out=$(timeout 8 ssh -o StrictHostKeyChecking=accept-new "
		"-o UserKnownHostsFile=\"$HOME/.aqemu-companion-known_hosts\" "
		"      -o PreferredAuthentications=none -o PubkeyAuthentication=no "
		"      -o PasswordAuthentication=no -o ConnectTimeout=4 "
		"      -p 32222 nobody@127.0.0.1 true 2>&1 || true); "
		"    if echo \"$out\" | grep -qiE 'password|permission denied|authentication|kex_exchange'; then "
		"      ssh_up=1; break; "
		"    fi; "
		"    if ! kill -0 \"$pid\" 2>/dev/null; then "
		"      echo 'Companion QEMU exited while waiting - log:'; "
		"      tail -n 40 \"$LOG\" || true; "
		"      exit 1; "
		"    fi; "
		"    echo \"  still booting... ($i/60)\"; "
		"    sleep 3; "
		"  done; "
		"  if [ \"$ssh_up\" = 1 ]; then "
		"    echo 'SSH is accepting connections on 127.0.0.1:32222.'; "
		"  else "
		"    echo 'SSH not ready yet after ~3 minutes. Check log:'; "
		"    tail -n 40 \"$LOG\" || true; "
		"    echo 'You can still wait and retry Restore / ssh -p 32222 ...'; "
		"  fi; "
		"  echo 'You may Close this dialog and Power On iOS in AQEMU.'; "
		"  echo 'SSH: ssh -p 32222 USER@127.0.0.1'; "
		"  echo 'Stop: use Stop companion in this dialog.'; "
		"  exit 0; "
		"else "
		"  echo 'Companion failed to stay up - last log lines:'; "
		"  tail -n 40 \"$LOG\" || true; "
		"  echo 'If write lock: Power Off the AQEMU Ubuntu companion VM, then retry.'; "
		"  exit 1; "
		"fi" )
		.arg( Shell_Single_Quote( remote ),
		      Shell_Single_Quote( remote ),
		      ipsw_dir_wsl.isEmpty() ? QStringLiteral( "0" ) : QStringLiteral( "1" ) );

	const QStringList args = WSL_Bash_Args( distro, user, script );
	Append_Log( QStringLiteral( "Starting companion:\nwsl.exe %1\n" ).arg( args.join( QLatin1Char( ' ' ) ) ) );
	Companion_Process->start( QStringLiteral( "wsl.exe" ), args );
#else
	Q_UNUSED( type );
	QMessageBox::information( this, tr( "Companion" ),
		tr( "On Linux, run the companion command shown above in a terminal." ) );
#endif
}

void Apple_SoC_Restore_Window::Set_VM( Virtual_Machine *vm )
{
	VM = vm;
	if( ! vm )
		return;
	if( Edit_IPSW && Edit_IPSW->text().trimmed().isEmpty() )
		Edit_IPSW->setText( vm->Get_Apple_IPSW_Path() );
	Refresh_Companion_Snippet();
}

void Apple_SoC_Restore_Window::Stop_Companion_WSL()
{
#ifdef Q_OS_WIN32
	QString distro, user;
	if( ! Ensure_WSL_Creds( &distro, &user ) )
	{
		QMessageBox::warning( this, tr( "WSL" ),
			tr( "WSL distro and username are required." ) );
		return;
	}

	// Cancel Start's SSH wait loop (otherwise it keeps printing "still booting").
	if( Companion_Process->state() != QProcess::NotRunning )
	{
		Append_Log( QStringLiteral( "Stopping companion launcher...\n" ) );
		Companion_Process->kill();
		Companion_Process->waitForFinished( 5000 );
	}

	const QString script = QString::fromUtf8( kKill_Inferno_Companions ) + QStringLiteral(
		"PIDF=/tmp/aqemu-inferno-companion.pid; "
		"if [ -f \"$PIDF\" ]; then "
		"  pid=$(cat \"$PIDF\" 2>/dev/null || true); "
		"  if [ -n \"$pid\" ]; then "
		"    echo \"Stopping companion PID $pid...\"; "
		"    kill -TERM \"$pid\" 2>/dev/null || true; "
		"    sleep 1; "
		"    kill -KILL \"$pid\" 2>/dev/null || true; "
		"  fi; "
		"  rm -f \"$PIDF\"; "
		"fi; "
		"kill_inferno_companions -TERM; "
		"sleep 1; "
		"kill_inferno_companions -KILL; "
		"echo 'Companion stop requested.'" );
	const QStringList args = WSL_Bash_Args( distro, user, script );
	Append_Log( QStringLiteral( "Stop companion:\nwsl.exe %1\n" ).arg( args.join( QLatin1Char( ' ' ) ) ) );
	Companion_Process->start( QStringLiteral( "wsl.exe" ), args );
#else
	QMessageBox::information( this, tr( "Companion" ),
		tr( "Stop the Inferno companion QEMU process in a terminal." ) );
#endif
}

void Apple_SoC_Restore_Window::Run_Diagnose_WSL()
{
	Sync_Conn_To_VM();
#ifdef Q_OS_WIN32
	QString distro, user;
	if( ! Ensure_WSL_Creds( &distro, &user ) )
		return;

	const QString type = Conn_Type();
	const QString addr = Conn_Addr().isEmpty()
		? ( type == QLatin1String( "unix" )
			? QStringLiteral( "/tmp/InfernoUSBRemote" )
			: QStringLiteral( "127.0.0.1" ) )
		: Conn_Addr();
	const int port = Conn_Port();

	QString script = QStringLiteral(
		"echo '=== Inferno USB bridge diagnose ==='; "
		"echo; " );

	if( type == QLatin1String( "unix" ) )
	{
		script += QStringLiteral(
			"echo '[unix socket]'; ls -la %1 2>&1 || true; " )
			.arg( Shell_Single_Quote( addr ) );
	}
	else
	{
		script += QStringLiteral(
			"echo '[ipv4 listen port %1]'; "
			"(ss -ltnp 2>/dev/null || netstat -ltnp 2>/dev/null || true) | grep -E ':%1\\b' || "
			"echo '(nothing listening on %1 — companion usb-tcp-remote may be unix or down)'; "
			"echo; echo '[companion usb-tcp-remote args]'; "
			"pid=$(pgrep -f qemu-system-x86_64-inferno | head -1); "
			"if [ -n \"$pid\" ]; then "
			"  tr '\\0' ' ' < /proc/$pid/cmdline | tr ',' '\\n' | grep -E 'usb-tcp-remote|conn-' || true; "
			"else echo '(no companion)'; fi; " )
			.arg( port );
	}

	script += QStringLiteral(
		"echo; echo '[companion qemu]'; "
		"pgrep -af qemu-system-x86_64-inferno 2>&1 || echo '(not running)'; "
		"echo; echo '[ios/applesoc qemu]'; "
		"pgrep -af qemu-system-applesoc 2>&1 | head -3 || echo '(not running)'; "
		"echo; echo '[guest -machine usb-conn*]'; "
		"pid=$(pgrep -f qemu-system-applesoc | head -1); "
		"if [ -n \"$pid\" ]; then "
		"  tr '\\0' ' ' < /proc/$pid/cmdline | tr ',' '\\n' | grep -E 'usb-conn|t8030|s8000' || "
		"  echo '(no explicit usb-conn; Inferno may default to unix+/tmp/InfernoUSBRemote)'; "
		"else echo '(no applesoc process)'; fi; "
		"echo; echo '[WSL host USB — iPhone should NOT be here]'; "
		"lsusb 2>&1 || true; "
		"echo; echo '[usbmuxd on WSL host]'; "
		"pgrep -a usbmuxd 2>&1 || echo '(not running — expected if using companion Linux)'; "
		"idevice_id -l 2>&1 || true; "
		"echo; "
		"echo 'EXPECTED: iPhone shows via lsusb INSIDE the companion Linux guest,'; "
		"echo 'not on this WSL host. Use: ssh -p 32222 user@127.0.0.1'; "
		"echo 'Then: sudo systemctl start usbmuxd; lsusb; idevicerestore …'; "
		"echo; "
		"echo 'Inferno tip: use ipv4 127.0.0.1:8030 on BOTH companion and main VM'; "
		"echo '(not mixed with unix). Guest cmdline must include usb-conn-type=ipv4…'; "
		"echo 'If companion still shows conn-type=unix: Stop companion, then Start companion again.'; " );

	const QStringList args = WSL_Bash_Args( distro, user, script );
	Append_Log( QStringLiteral( "Diagnose:\nwsl.exe %1\n" ).arg( args.join( QLatin1Char( ' ' ) ) ) );
	if( Process->state() != QProcess::NotRunning )
		Process->kill();
	Process->start( QStringLiteral( "wsl.exe" ), args );
#else
	Append_Log( tr( "Diagnose is implemented for the Windows/WSL path." ) );
#endif
}

void Apple_SoC_Restore_Window::Run_IDeviceRestore()
{
	const QString ipsw = Edit_IPSW->text().trimmed();
	const QString ssh_user = Edit_SSH_User ? Edit_SSH_User->text().trimmed() : QString();
	const QString ssh_pass = Edit_SSH_Password ? Edit_SSH_Password->text() : QString();
	Sync_Conn_To_VM();

	if( ipsw.isEmpty() || ! QFile::exists( ipsw ) )
	{
		QMessageBox::warning( this, tr( "IPSW" ),
			tr( "Select a valid IPSW (or restore .zip) file first." ) );
		return;
	}
	if( ssh_user.isEmpty() || ssh_pass.isEmpty() )
	{
		QMessageBox::warning( this, tr( "Companion SSH" ),
			tr( "Enter the companion Linux SSH username and password.\n"
			    "AQEMU mounts the IPSW folder into the companion (9p) and runs idevicerestore there." ) );
		return;
	}

	QSettings s;
	s.setValue( QStringLiteral( "Apple_SoC_Restore/SSH_User" ), ssh_user );

#ifdef Q_OS_WIN32
	QString distro, user;
	if( ! Ensure_WSL_Creds( &distro, &user ) )
	{
		QMessageBox::warning( this, tr( "WSL" ),
			tr( "WSL distro and username are required." ) );
		return;
	}

	if( Process->state() != QProcess::NotRunning )
	{
		QMessageBox::information( this, tr( "Busy" ),
			tr( "Another restore/diagnose command is still running." ) );
		return;
	}

	QTemporaryFile script_file(
		QDir::temp().filePath( QStringLiteral( "aqemu-restore-XXXXXX.sh" ) ) );
	script_file.setAutoRemove( false );
	if( ! script_file.open() )
	{
		QMessageBox::warning( this, tr( "Restore" ),
			tr( "Could not write temporary restore script." ) );
		return;
	}

	const QByteArray body = QStringLiteral(
		"#!/usr/bin/env bash\n"
		"set -euo pipefail\n"
		"if ! command -v sshpass >/dev/null 2>&1; then\n"
		"  echo 'Installing sshpass in WSL (once)...'\n"
		"  sudo -n apt-get install -y sshpass 2>/dev/null \\\n"
		"    || apt-get install -y sshpass 2>/dev/null \\\n"
		"    || { echo 'Install sshpass in WSL: sudo apt install sshpass'; exit 127; }\n"
		"fi\n"
		"if [ -z \"${SSHPASS:-}\" ]; then\n"
		"  echo 'SSHPASS was not passed into WSL (WSLENV).'; exit 127\n"
		"fi\n"
		"export SSHPASS\n"
		"SSH_OPTS=(-o StrictHostKeyChecking=accept-new "
		"-o UserKnownHostsFile=\"$HOME/.aqemu-companion-known_hosts\" "
		"-o PreferredAuthentications=password -o PubkeyAuthentication=no "
		"-o NumberOfPasswordPrompts=1 -o ConnectTimeout=15 "
		"-o LogLevel=ERROR)\n"
		"GUEST=%1\n"
		"ssh_do() { sshpass -e ssh -p 32222 \"${SSH_OPTS[@]}\" \"$GUEST@127.0.0.1\" \"$@\"; }\n"
		// Companion image was patched with NOPASSWD for bob. Use only sudo -n and
		// single-quoted remote commands. Never embed passwords / printf %% formats in
		// the remote argv (bash treats bare %% as job control: \"fg: no job control\").
		"echo 'Waiting for companion SSH on 127.0.0.1:32222...'\n"
		"ssh_ok=0\n"
		"for i in $(seq 1 40); do\n"
		"  if ssh_do 'echo SSH_OK' 2>/tmp/aqemu-ssh-err; then ssh_ok=1; break; fi\n"
		"  echo \"  SSH not ready yet ($i/40)...\"; tail -n 2 /tmp/aqemu-ssh-err 2>/dev/null || true\n"
		"  sleep 5\n"
		"done\n"
		"[ \"$ssh_ok\" = 1 ] || { echo 'Cannot SSH to companion after waiting.'; exit 1; }\n"
		"echo 'Checking companion disk space...'\n"
		"ssh_do 'df -h / /tmp /dev/shm 2>/dev/null || df -h' || true\n"
		"echo 'Checking passwordless sudo...'\n"
		"ssh_do 'sudo -n true' || {\n"
		"  echo 'Companion needs passwordless sudo for bob (disk patch missing).'; exit 1; }\n"
		"echo 'Freeing companion disk + fixing lockdown dirs (before USB wait)...'\n"
		"ssh_do 'sudo -n bash -c \""
		"journalctl --vacuum-size=8M >/dev/null 2>&1 || true; "
		"apt-get clean >/dev/null 2>&1 || true; "
		"rm -rf /var/cache/apt/archives/* /tmp/idevicerestore* /tmp/ipsw* 2>/dev/null || true; "
		"mkdir -p /var/lib/lockdown; chmod 777 /var/lib/lockdown; "
		"df -h /\"' || true\n"
		"echo 'Mounting IPSW 9p share inside companion...'\n"
		"ssh_do 'sudo -n mkdir -p /mnt/aqemu_ipsw'\n"
		"ssh_do 'mountpoint -q /mnt/aqemu_ipsw' 2>/dev/null || "
		"ssh_do 'sudo -n mount -t 9p -o trans=virtio,version=9p2000.L aqemu_ipsw /mnt/aqemu_ipsw' || "
		"ssh_do 'sudo -n mount -t 9p -o trans=virtio aqemu_ipsw /mnt/aqemu_ipsw'\n"
		"ssh_do 'mountpoint -q /mnt/aqemu_ipsw' || {\n"
		"  echo '9p mount failed. Is companion started with IPSW virtfs?'; exit 1; }\n"
		// Literal single-quoted remote path only. Do NOT use printf %%q here: Qt .arg()
		// has left '%%q' unconverted before, so the guest ran `test -f %q` and failed
		// even when aqemu-restore-current.ipsw was clearly listed on the mount.
		"ssh_do 'test -e /mnt/aqemu_ipsw/aqemu-restore-current.ipsw' || {\n"
		"  echo 'IPSW not on 9p mount at: /mnt/aqemu_ipsw/aqemu-restore-current.ipsw'\n"
		"  echo 'Stop companion, Start again with IPSW selected (stages the hardlink).'\n"
		"  echo 'Mount contents mentioning iPhone/Restore/aqemu-restore:'\n"
		"  ssh_do 'ls -la /mnt/aqemu_ipsw 2>/dev/null | grep -iE \"iphone|restore|aqemu-restore\" | head -30' || true\n"
		"  exit 3; }\n"
		"echo 'Using shared IPSW: /mnt/aqemu_ipsw/aqemu-restore-current.ipsw'\n"
		// Inferno reports HardwareModel N104DEV; stock libirecovery only knows N104AP.
		// ChefKiss patch rewrites DEV→AP (logs \"INFO: model is …\"). Without it:
		// \"Unable to discover device type\" after serial is seen.
		"echo 'Checking idevicerestore ChefKiss patch (DEV→AP)...'\n"
		"ssh_do 'command -v idevicerestore >/dev/null' || {\n"
		"  echo 'idevicerestore not found in companion PATH.'; exit 1; }\n"
		"if ! ssh_do 'strings \"$(command -v idevicerestore)\" 2>/dev/null | grep -Fq \"INFO: model is\"'; then\n"
		"  echo\n"
		"  echo 'FATAL: companion idevicerestore is missing the ChefKiss Inferno patch.'\n"
		"  echo 'Inferno reports model N104DEV; unpatched builds cannot map it →'\n"
		"  echo '  \"Unable to discover device type\" (after serial INFERNO_…).'\n"
		"  echo 'Fix inside companion (once):'\n"
		"  echo '  cd ~/idevicerestore  # or re-clone libimobiledevice/idevicerestore'\n"
		"  echo '  curl -fsSL -o ../idevicerestore.patch '\n"
		"  echo '    https://chefkiss.dev/Extras/Inferno/idevicerestore.patch'\n"
		"  echo '  git apply ../idevicerestore.patch   # or use #119 ASR patch if this fails'\n"
		"  echo '  PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./autogen.sh && make -j\"$(nproc)\" && sudo make install'\n"
		"  echo 'Also need a real root_ticket.der (not the 2-byte placeholder) for -T.'\n"
		"  exit 4\n"
		"fi\n"
		"echo 'idevicerestore looks ChefKiss-patched (INFO: model is present).'\n"
		"check_8030() {\n"
		// Modern ss prints ESTAB (not ESTABLISHED); match either.
		"  ss -tnp 2>/dev/null | grep -E ':8030\\b' | grep -iE 'estab' || true\n"
		"}\n"
		"echo\n"
		"echo '=== USB bridge (WSL host :8030) ==='\n"
		"(ss -ltnp 2>/dev/null || true) | grep -E ':8030\\b' || "
		"  echo 'WARNING: nothing listening on 8030 (Start companion first)'\n"
		"EST=$(check_8030)\n"
		"if [ -n \"$EST\" ]; then\n"
		"  echo \"TCP ESTABLISHED on :8030 already: $EST\"\n"
		"else\n"
		"  echo 'No ESTABLISHED on :8030 yet (iOS not connected - that is OK, Power On iOS now).'\n"
		"fi\n"
		"echo\n"
		"echo 'Power On the iOS guest NOW (companion SSH is up). Waiting for idevice...'\n"
		"echo 'IMPORTANT: iOS only waits ~120s for restore once the Apple logo appears.'\n"
		"DEV=\n"
		"for i in $(seq 1 60); do\n"
		"  ssh_do 'sudo -n systemctl start usbmuxd' 2>/dev/null || true\n"
		"  LS=$(ssh_do 'lsusb 2>/dev/null' || true)\n"
		"  DEV=$(ssh_do 'idevice_id -l 2>/dev/null | head -1' || true)\n"
		"  EST=$(check_8030)\n"
		"  if echo \"$LS\" | grep -qiE 'Apple|05ac:'; then\n"
		"    echo \"lsusb Apple device seen (try $i):\"\n"
		"    echo \"$LS\" | grep -iE 'Apple|05ac:' || true\n"
		"  fi\n"
		"  if [ -n \"$EST\" ]; then echo \"TCP ESTABLISHED on :8030: $EST\"; fi\n"
		"  if [ -n \"$DEV\" ]; then break; fi\n"
		"  echo \"  ($i/60) waiting for idevice...\"\n"
		"  sleep 3\n"
		"done\n"
		"if [ -z \"$DEV\" ]; then\n"
		"  echo\n"
		"  echo 'Still no device after waiting.'\n"
		"  echo 'Order: Start companion -> wait for SSH -> Power On iOS -> Restore quickly.'\n"
		"  ssh_do 'lsusb; idevice_id -l' || true\n"
		"  exit 2\n"
		"fi\n"
		"echo \"Device: $DEV\"\n"
		// Inferno UDID is like 00008030-1122334455667788; ECID is after the last '-'.
		// Use bash param expansion only - never printf '%%s' (Qt .arg leaves %% broken).
		"ECID_HEX=${DEV##*-}; "
		"ECID_ARG=; "
		"if [ -n \"$ECID_HEX\" ] && [ \"$ECID_HEX\" != \"$DEV\" ]; then "
		"  ECID_ARG=\"-i 0x$ECID_HEX\"; "
		"else "
		"  ECID_ARG='-i 0x1122334455667788'; "
		"fi; "
		"TICKET_ARG=; "
		"ssh_do 'test -s /mnt/aqemu_ipsw/aqemu-restore-ticket.der' 2>/dev/null && "
		"TICKET_ARG='-T /mnt/aqemu_ipsw/aqemu-restore-ticket.der'; "
		"echo \"ECID: ${ECID_ARG:-none}  ticket: ${TICKET_ARG:-none}\"\n"
		"EST=$(check_8030)\n"
		"if [ -n \"$EST\" ]; then echo \"Pre-restore bridge ESTABLISHED: $EST\"; "
		"else echo 'Pre-restore bridge: no ESTABLISHED on :8030 (check iOS USB remote config)'; fi\n"
		"echo 'Starting idevicerestore NOW (Inferno ~120s window)...'\n"
		"ok=0\n"
		"for attempt in 1 2 3 4 5; do\n"
		"  echo \"=== idevicerestore attempt $attempt/5 ===\"\n"
		"  if [ \"$attempt\" != 1 ]; then\n"
		"    echo 'Retry: Power Off iOS, wait 5s, Power On iOS, then click Restore again...'\n"
		"    DEV=\n"
		"    for j in $(seq 1 40); do\n"
		"      ssh_do 'sudo -n systemctl start usbmuxd' 2>/dev/null || true\n"
		"      DEV=$(ssh_do 'idevice_id -l 2>/dev/null | head -1' || true)\n"
		"      [ -n \"$DEV\" ] && break\n"
		"      sleep 3\n"
		"    done\n"
		"    echo 'Restarting usbmuxd (Inferno tip when mode discovery fails)...'\n"
		"    ssh_do 'sudo -n systemctl restart usbmuxd' 2>/dev/null || true\n"
		"    sleep 2\n"
		"  fi\n"
		"  if sshpass -e ssh -t -p 32222 \"${SSH_OPTS[@]}\" \"$GUEST@127.0.0.1\" "
		"\"export LD_LIBRARY_PATH=/usr/local/lib:\\${LD_LIBRARY_PATH:-}; "
		"sudo -n -E idevicerestore -d -y -e -R $ECID_ARG $TICKET_ARG "
		"/mnt/aqemu_ipsw/aqemu-restore-current.ipsw\"; then\n"
		"    ok=1; break\n"
		"  fi\n"
		"  echo \"attempt $attempt failed - USB recheck:\"\n"
		"  ssh_do 'lsusb | grep -iE \"Apple|05ac\" || true; idevice_id -l || true' || true\n"
		"  sleep 3\n"
		"done\n"
		"if [ \"$ok\" != 1 ]; then\n"
		"  echo\n"
		"  echo 'idevicerestore failed.'\n"
		"  echo 'If log said \"Unable to discover device type\" after serial INFERNO_*:'\n"
		"  echo '  companion idevicerestore lacks ChefKiss N104DEV→AP patch.'\n"
		"  echo '  https://chefkiss.dev/Extras/Inferno/idevicerestore.patch'\n"
		"  echo 'If log reached \"Done sending NORData\" then \"Could not read data (-256)\":'\n"
		"  echo '  USB bridge dropped mid-restore (common). After partial restore you MUST:'\n"
		"  echo '  1) Power Off iOS guest completely'\n"
		"  echo '  2) Delete NVMe images: <VM>.aqemu folder -> *_inferno\\ (root, firmware, ...)'\n"
		"  echo '     Or run: extras/Inferno/wipe-ios-nvme.ps1 -VmXml \"path\\\\iOS_ARM64_.aqemu\"'\n"
		"  echo '  3) Stop companion -> Start companion -> Power On iOS -> Restore again'\n"
		"  echo '  (issue #119 ASR retry patch helps later ASR failures, not this NORData drop)'\n"
		"  echo 'Checklist:'\n"
		"  echo '  - Companion SSH up BEFORE Power On iOS'\n"
		"  echo '  - ss shows ESTABLISHED on 127.0.0.1:8030 before idevicerestore starts'\n"
		"  echo '  - Click Restore within ~120s of Apple logo / empty bar'\n"
		"  echo '  - Patched idevicerestore (must log INFO: model is N104DEV)'\n"
		"  echo '  - Real root_ticket.der via -T (not empty placeholder)'\n"
		"  exit 1\n"
		"fi\n"
		"echo 'idevicerestore finished.'\n" )
		.arg( Shell_Single_Quote( ssh_user ) )
		.toUtf8();

	script_file.write( body );
	script_file.flush();
	const QString win_script = script_file.fileName();
	script_file.close();
	const QString wsl_script = Windows_Path_To_WSL( win_script );

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert( QStringLiteral( "SSHPASS" ), ssh_pass );
	QString wslenv = env.value( QStringLiteral( "WSLENV" ) );
	if( ! wslenv.isEmpty() && ! wslenv.endsWith( QLatin1Char( ':' ) ) )
		wslenv += QLatin1Char( ':' );
	if( ! wslenv.contains( QStringLiteral( "SSHPASS" ) ) )
		wslenv += QStringLiteral( "SSHPASS/u" );
	env.insert( QStringLiteral( "WSLENV" ), wslenv );

	const QString launcher = QStringLiteral(
		"bash %1; ec=$?; rm -f %1; exit $ec" )
		.arg( Shell_Single_Quote( wsl_script ) );
	const QStringList args = WSL_Bash_Args( distro, user, launcher );
	Append_Log( QStringLiteral(
		"Restore via companion SSH (9p IPSW share + idevicerestore)…\n" ) );
	Process->setProcessEnvironment( env );
	Process->start( QStringLiteral( "wsl.exe" ), args );
#else
	Q_UNUSED( ssh_user );
	Q_UNUSED( ssh_pass );
	QMessageBox::information( this, tr( "Restore" ),
		tr( "Automated companion SSH restore is implemented for the Windows/WSL path." ) );
#endif
}

void Apple_SoC_Restore_Window::On_Process_Output()
{
	QProcess *p = qobject_cast<QProcess *>( sender() );
	if( ! p )
		p = Process;
	const QString out = QString::fromUtf8( p->readAllStandardOutput() );
	const QString err = QString::fromUtf8( p->readAllStandardError() );
	Append_Log( out );
	Append_Log( err );
}

void Apple_SoC_Restore_Window::On_Process_Finished( int code, QProcess::ExitStatus )
{
	Process->setProcessEnvironment( QProcessEnvironment::systemEnvironment() );
	Append_Log( tr( "\nCommand finished with exit code %1" ).arg( code ) );
	if( code == 0 )
	{
		Append_Log( tr(
			"\nNext (required): apply ChefKiss filesystem patches to the guest "
			"root disk before SpringBoard will boot — otherwise VNC stays black.\n"
			"  https://chefkiss.dev/guides/inferno/fs-patches/\n"
			"Then clear MACHINE → Restore ramdisk (-initrd) and Power On." ) );
	}
	else
	{
		Append_Log( tr(
			"\nRestore notes:\n"
			"  - Companion + IPSW 9p can be fine while USB is still missing.\n"
			"  - Apple logo with empty bar = waiting for restore host; it times out\n"
			"    if the USB bridge never attaches (~120s window).\n"
			"  - \"Unable to discover device type\" (after serial): ChefKiss\n"
			"    idevicerestore.patch missing (N104DEV→AP). Ticket comes after that.\n"
			"  - \"Done sending NORData\" then \"Could not read data (-256)\": bridge dropped.\n"
			"    Wipe *_inferno NVMe images (see extras/Inferno/wipe-ios-nvme.ps1), then retry.\n"
			"  - See https://chefkiss.dev/guides/inferno/troubleshooting/" ) );
	}
}

void Apple_SoC_Restore_Window::On_Companion_Finished( int code, QProcess::ExitStatus )
{
	if( code == 0 )
		Append_Log( tr( "\nWSL companion command finished OK.\n" ) );
	else
		Append_Log( tr( "\nCompanion command failed with exit code %1\n" ).arg( code ) );
}
