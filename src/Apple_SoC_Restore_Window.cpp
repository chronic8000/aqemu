#include "Apple_SoC_Restore_Window.h"
#include "Apple_SoC_Support.h"
#include "VM.h"
#include "Utils.h"
#include "WSL_Launch.h"
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

Apple_SoC_Restore_Window::Apple_SoC_Restore_Window( Virtual_Machine *vm, QWidget *parent )
	: QDialog( parent )
	, VM( vm )
	, Process( new QProcess( this ) )
{
	setWindowTitle( tr( "Apple SoC Restore (Inferno companion)" ) );
	resize( AQ_Px( 720, this ), AQ_Px( 560, this ) );

	QVBoxLayout *lay = new QVBoxLayout( this );
	QLabel *intro = new QLabel( tr(
		"<p><b>IPSW restore needs a companion VM</b> (ChefKiss Inferno).</p>"
		"<ol>"
		"<li>Start the companion QEMU first with <code>usb-tcp-remote</code> matching the "
		"guest <code>usb-conn-*</code> flags.</li>"
		"<li>Start the Apple SoC guest (AQEMU forces Linux Inferno under WSL on Windows).</li>"
		"<li>In the same WSL distro, run patched <code>idevicerestore</code> against the "
		"virtual iPhone (not a raw Windows PE QEMU — UNIX sockets / usbmux).</li>"
		"</ol>"
		"<p>See <a href=\"https://chefkiss.dev/ar/guides/inferno/companion-setup/\">Companion VM setup</a>.</p>" ) );
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

	CB_Conn_Type = new QComboBox();
	CB_Conn_Type->addItem( tr( "UNIX socket (WSL / Linux)" ), QStringLiteral( "unix" ) );
	CB_Conn_Type->addItem( tr( "IPv4 TCP" ), QStringLiteral( "ipv4" ) );
	const QString ctype = vm ? vm->Get_Apple_USB_Conn_Type() : QString();
	CB_Conn_Type->setCurrentIndex( ctype.compare( QLatin1String( "ipv4" ), Qt::CaseInsensitive ) == 0 ? 1 : 0 );
	form->addRow( tr( "USB remote type:" ), CB_Conn_Type );

	Edit_Conn_Addr = new QLineEdit( vm && ! vm->Get_Apple_USB_Conn_Addr().isEmpty()
		? vm->Get_Apple_USB_Conn_Addr()
		: QStringLiteral( "127.0.0.1" ) );
	form->addRow( tr( "USB remote addr:" ), Edit_Conn_Addr );
	lay->addLayout( form );

	Text_Companion = new QTextEdit();
	Text_Companion->setReadOnly( true );
	Text_Companion->setMaximumHeight( AQ_Px( 120, this ) );
	lay->addWidget( new QLabel( tr( "Companion QEMU snippet (EHCI + usb-tcp-remote):" ) ) );
	lay->addWidget( Text_Companion );
	Copy_Companion_Command();
	connect( CB_Conn_Type, QOverload<int>::of( &QComboBox::currentIndexChanged ),
	         this, [this]( int ) { Copy_Companion_Command(); } );
	connect( Edit_Conn_Addr, &QLineEdit::textChanged, this, [this]( const QString & ) { Copy_Companion_Command(); } );

	QHBoxLayout *btns = new QHBoxLayout();
	QPushButton *btnCopy = new QPushButton( tr( "Copy companion command" ) );
	connect( btnCopy, &QPushButton::clicked, this, [this]() {
		QApplication::clipboard()->setText( Text_Companion->toPlainText() );
		QMessageBox::information( this, tr( "Copied" ), tr( "Companion command copied." ) );
	} );
	QPushButton *btnRestore = new QPushButton( tr( "Run idevicerestore in WSL" ) );
	connect( btnRestore, &QPushButton::clicked, this, &Apple_SoC_Restore_Window::Run_IDeviceRestore );
	btns->addWidget( btnCopy );
	btns->addWidget( btnRestore );
	btns->addStretch();
	QPushButton *btnClose = new QPushButton( tr( "Close" ) );
	connect( btnClose, &QPushButton::clicked, this, &QDialog::accept );
	btns->addWidget( btnClose );
	lay->addLayout( btns );

	Text_Log = new QTextEdit();
	Text_Log->setReadOnly( true );
	lay->addWidget( new QLabel( tr( "Output:" ) ) );
	lay->addWidget( Text_Log, 1 );

	connect( Process, &QProcess::readyReadStandardOutput, this, &Apple_SoC_Restore_Window::On_Process_Output );
	connect( Process, &QProcess::readyReadStandardError, this, &Apple_SoC_Restore_Window::On_Process_Output );
	connect( Process, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
	         this, &Apple_SoC_Restore_Window::On_Process_Finished );
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

void Apple_SoC_Restore_Window::Copy_Companion_Command()
{
	const QString type = CB_Conn_Type->currentData().toString();
	const QString addr = Edit_Conn_Addr->text().trimmed().isEmpty()
		? ( type == QLatin1String( "unix" ) ? QStringLiteral( "/tmp/InfernoUSBRemote" ) : QStringLiteral( "127.0.0.1" ) )
		: Edit_Conn_Addr->text().trimmed();
	const int port = VM && VM->Get_Apple_USB_Conn_Port() > 0 ? VM->Get_Apple_USB_Conn_Port() : 8030;

	if( VM )
	{
		VM->Set_Apple_USB_Conn_Type( type );
		VM->Set_Apple_USB_Conn_Addr( addr );
	}

	QString remote = QStringLiteral( "usb-tcp-remote,bus=ehci.0,conn-type=%1" ).arg( type );
	if( type == QLatin1String( "unix" ) )
		remote += QStringLiteral( ",conn-addr=%1" ).arg( addr );
	else
		remote += QStringLiteral( ",conn-addr=%1,conn-port=%2" ).arg( addr ).arg( port );

	Text_Companion->setPlainText(
		tr( "# Start companion BEFORE the Apple SoC guest\n"
		    "qemu-system-x86_64 -M q35 -m 2G -accel kvm \\\n"
		    "  -usb -device usb-ehci,id=ehci -device %1\n\n"
		    "# Guest machine must use matching usb-conn-type / addr / port.\n"
		    "# idevicerestore must be the Inferno-patched build inside WSL/Linux." )
			.arg( remote ) );
}

void Apple_SoC_Restore_Window::Run_IDeviceRestore()
{
	const QString ipsw = Edit_IPSW->text().trimmed();
	if( ipsw.isEmpty() || ! QFile::exists( ipsw ) )
	{
		QMessageBox::warning( this, tr( "IPSW" ), tr( "Select a valid IPSW file first." ) );
		return;
	}
#ifdef Q_OS_WIN32
	QSettings s;
	const QString distro = s.value( QStringLiteral( "WSL_Launch/Distro" ), QString() ).toString();
	const QString wsl_ipsw = Windows_Path_To_WSL( ipsw );
	QStringList args;
	if( ! distro.trimmed().isEmpty() )
		args << QStringLiteral( "-d" ) << distro.trimmed();
	const QString user = WSL_Sanitize_Username(
		s.value( QStringLiteral( "WSL_Launch/Username" ), QString() ).toString() );
	if( ! user.isEmpty() )
		args << QStringLiteral( "-u" ) << user;
	args << QStringLiteral( "-e" ) << QStringLiteral( "idevicerestore" )
	     << QStringLiteral( "-d" ) << QStringLiteral( "-R" ) << wsl_ipsw;
	Text_Log->append( QStringLiteral( "wsl.exe %1\n" ).arg( args.join( QLatin1Char( ' ' ) ) ) );
	Process->start( QStringLiteral( "wsl.exe" ), args );
#else
	Process->start( QStringLiteral( "idevicerestore" ),
		QStringList() << QStringLiteral( "-d" ) << QStringLiteral( "-R" ) << ipsw );
#endif
}

void Apple_SoC_Restore_Window::On_Process_Output()
{
	const QString out = QString::fromLocal8Bit( Process->readAllStandardOutput() );
	const QString err = QString::fromLocal8Bit( Process->readAllStandardError() );
	if( ! out.isEmpty() ) Text_Log->append( out );
	if( ! err.isEmpty() ) Text_Log->append( err );
}

void Apple_SoC_Restore_Window::On_Process_Finished( int code, QProcess::ExitStatus )
{
	Text_Log->append( tr( "\nidevicerestore finished with exit code %1" ).arg( code ) );
}
