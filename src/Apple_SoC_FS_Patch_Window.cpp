#include "Apple_SoC_FS_Patch_Window.h"

#include "Apple_SoC_Support.h"
#include "Utils.h"
#include "VM.h"
#include "WSL_Launch.h"
#include "WSL_Secure_Credentials.h"
#include "WSL_Wizard_Window.h"
#include "AQ_UI_Style.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QSettings>
#include <QVBoxLayout>

QString AQ_Inferno_Extras_Dir()
{
	const QString app = QCoreApplication::applicationDirPath();
	const QStringList candidates = {
		app + QStringLiteral( "/extras/Inferno" ),
		app + QStringLiteral( "/../extras/Inferno" ),
		app + QStringLiteral( "/../../extras/Inferno" ),
		QDir( app ).absoluteFilePath( QStringLiteral( "../CURSOR-PROJECTS/aqemu/extras/Inferno" ) ),
	};
	for( const QString &c : candidates )
	{
		const QString s = QDir::cleanPath( c );
		if( QFileInfo::exists( s + QStringLiteral( "/apply-fs-patches-wsl.sh" ) ) )
			return s;
	}
	// Dev tree: walk up from applicationDirPath looking for extras/Inferno
	QDir d( app );
	for( int i = 0; i < 6; ++i )
	{
		const QString try_path = d.filePath( QStringLiteral( "extras/Inferno" ) );
		if( QFileInfo::exists( try_path + QStringLiteral( "/apply-fs-patches-wsl.sh" ) ) )
			return QDir::cleanPath( try_path );
		if( ! d.cdUp() )
			break;
	}
	return QString();
}

void AQ_Show_Apple_SoC_FS_Patch_Window( Virtual_Machine *vm, QWidget *parent )
{
	QWidget *host = parent ? parent : QApplication::activeWindow();
	auto *dlg = new Apple_SoC_FS_Patch_Window( vm, host );
	dlg->setAttribute( Qt::WA_DeleteOnClose );
	dlg->setWindowModality( Qt::NonModal );
	QObject::connect( dlg, &Apple_SoC_FS_Patch_Window::Patches_Applied,
	                  dlg, []( Virtual_Machine *v ) {
		if( ! v )
			return;
		v->Set_Apple_Initrd_Path( QString() );
		v->Save_VM();
		// Clear MACHINE-tab field so Power On / Save does not re-apply -initrd.
		for( QWidget *tl : QApplication::topLevelWidgets() )
		{
			if( QLineEdit *e = tl->findChild<QLineEdit *>(
				QStringLiteral( "Edit_Apple_Initrd" ) ) )
			{
				e->clear();
				break;
			}
		}
	} );
	dlg->show();
	dlg->raise();
	dlg->activateWindow();
}

Apple_SoC_FS_Patch_Window::Apple_SoC_FS_Patch_Window( Virtual_Machine *vm, QWidget *parent )
	: QDialog( parent )
	, VM( vm )
	, Process( new QProcess( this ) )
{
	setWindowTitle( tr( "Apply iOS filesystem patches (Inferno)" ) );
	resize( AQ_Px( 720, this ), AQ_Px( 520, this ) );

	auto *lay = new QVBoxLayout( this );
	lay->addWidget( new QLabel( tr(
		"<b>Required after idevicerestore</b> for SpringBoard / setup to appear.<br>"
		"Patches the <b>iOS guest</b> <code>root</code> NVMe image "
		"(not the IPSW restore companion).<br>"
		"ChefKiss guide: "
		"<a href=\"https://chefkiss.dev/guides/inferno/fs-patches/\">"
		"chefkiss.dev/guides/inferno/fs-patches</a>" ) ) );

	auto *form = new QFormLayout();
	Edit_Root = new QLineEdit();
	Edit_Root->setPlaceholderText( tr( "…/<VM>_inferno/root" ) );
	form->addRow( tr( "Guest root disk:" ), Edit_Root );
	lay->addLayout( form );

	auto *row = new QHBoxLayout();
	QPushButton *btnBrowse = new QPushButton( tr( "Browse…" ) );
	connect( btnBrowse, &QPushButton::clicked, this, &Apple_SoC_FS_Patch_Window::Browse_Root );
	QPushButton *btnVm = new QPushButton( tr( "Use current VM root" ) );
	connect( btnVm, &QPushButton::clicked, this, &Apple_SoC_FS_Patch_Window::Use_Current_VM_Root );
	row->addWidget( btnBrowse );
	row->addWidget( btnVm );
	row->addStretch();
	lay->addLayout( row );

	Label_Status = new QLabel();
	Label_Status->setWordWrap( true );
	lay->addWidget( Label_Status );

	Btn_Start = new QPushButton( tr( "Apply filesystem patches…" ) );
	Btn_Start->setToolTip( tr(
		"On Windows: runs a temporary Ubuntu KVM guest in WSL with linux-apfs "
		"(InfernoFSPatcher + LaunchDaemons). Power Off the iOS VM first." ) );
	connect( Btn_Start, &QPushButton::clicked, this, &Apple_SoC_FS_Patch_Window::Start_Patch );
	lay->addWidget( Btn_Start );

	Text_Log = new QTextEdit();
	Text_Log->setReadOnly( true );
	lay->addWidget( new QLabel( tr( "Output:" ) ) );
	lay->addWidget( Text_Log, 1 );

	auto *bottom = new QHBoxLayout();
	bottom->addStretch();
	QPushButton *btnClose = new QPushButton( tr( "Close" ) );
	connect( btnClose, &QPushButton::clicked, this, &QDialog::accept );
	bottom->addWidget( btnClose );
	lay->addLayout( bottom );

	connect( Process, &QProcess::readyReadStandardOutput, this, &Apple_SoC_FS_Patch_Window::On_Process_Output );
	connect( Process, &QProcess::readyReadStandardError, this, &Apple_SoC_FS_Patch_Window::On_Process_Output );
	connect( Process, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
	         this, &Apple_SoC_FS_Patch_Window::On_Process_Finished );

	Use_Current_VM_Root();
	Refresh_Status();
}

void Apple_SoC_FS_Patch_Window::Set_VM( Virtual_Machine *vm )
{
	VM = vm;
	Use_Current_VM_Root();
}

void Apple_SoC_FS_Patch_Window::Set_Root_Image( const QString &path )
{
	Edit_Root->setText( QDir::toNativeSeparators( path ) );
	Refresh_Status();
}

void Apple_SoC_FS_Patch_Window::Use_Current_VM_Root()
{
	if( ! VM || ! AQ_Is_Apple_SoC_VM( VM ) )
	{
		Refresh_Status();
		return;
	}
	const QString root = AQ_Apple_SoC_Image_Dir( VM ) + QStringLiteral( "/root" );
	Edit_Root->setText( QDir::toNativeSeparators( root ) );
	Refresh_Status();
}

void Apple_SoC_FS_Patch_Window::Browse_Root()
{
	const QString f = QFileDialog::getOpenFileName( this, tr( "Select iOS root NVMe image" ),
		Edit_Root->text(), tr( "Raw disk (*);;All (*)" ) );
	if( ! f.isEmpty() )
		Set_Root_Image( f );
}

QString Apple_SoC_FS_Patch_Window::Resolve_Root_Path() const
{
	QString p = Edit_Root->text().trimmed();
	if( p.isEmpty() )
		return QString();
	QFileInfo fi( p );
	if( fi.isSymLink() )
	{
		const QString t = fi.symLinkTarget();
		if( ! t.isEmpty() )
			return QFileInfo( t ).absoluteFilePath();
	}
	return fi.absoluteFilePath();
}

void Apple_SoC_FS_Patch_Window::Refresh_Status()
{
	const QString root = Resolve_Root_Path();
	const QString extras = AQ_Inferno_Extras_Dir();
	QStringList lines;
	if( root.isEmpty() || ! QFileInfo::exists( root ) )
		lines << tr( "Root image not found — Power Off iOS, or pick *_inferno/root." );
	else
		lines << tr( "Root: %1 (%2 MiB)" )
			.arg( QDir::toNativeSeparators( root ) )
			.arg( QFileInfo( root ).size() / ( 1024 * 1024 ) );
	if( extras.isEmpty() )
		lines << tr( "WARNING: extras/Inferno scripts not found next to aqemu.exe "
		             "(expected apply-fs-patches-wsl.sh)." );
	else
		lines << tr( "Tooling: %1" ).arg( QDir::toNativeSeparators( extras ) );
	lines << tr( "Power Off the iOS guest before patching (exclusive write lock on root)." );
	Label_Status->setText( lines.join( QLatin1Char( '\n' ) ) );
	Btn_Start->setEnabled( ! root.isEmpty() && QFileInfo::exists( root ) && ! extras.isEmpty() );
}

QString Apple_SoC_FS_Patch_Window::Find_Patch_Script() const
{
	const QString extras = AQ_Inferno_Extras_Dir();
	if( extras.isEmpty() )
		return QString();
	return extras + QStringLiteral( "/apply-fs-patches-wsl.sh" );
}

bool Apple_SoC_FS_Patch_Window::Ensure_WSL_Creds( QString *distro_out, QString *user_out )
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

QStringList Apple_SoC_FS_Patch_Window::WSL_Bash_Args( const QString &distro, const QString &user,
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

void Apple_SoC_FS_Patch_Window::Append_Log( const QString &text )
{
	const QString cleaned = QString( text ).remove( QChar( '\r' ) );
	if( ! cleaned.trimmed().isEmpty() )
		Text_Log->append( cleaned );
}

void Apple_SoC_FS_Patch_Window::Start_Patch()
{
	Refresh_Status();
	const QString root = Resolve_Root_Path();
	const QString script_win = Find_Patch_Script();
	if( root.isEmpty() || ! QFile::exists( root ) )
	{
		QMessageBox::warning( this, tr( "Missing root" ),
			tr( "Select the iOS guest root disk (*_inferno/root)." ) );
		return;
	}
	if( script_win.isEmpty() || ! QFile::exists( script_win ) )
	{
		QMessageBox::warning( this, tr( "Missing tooling" ),
			tr( "Could not find extras/Inferno/apply-fs-patches-wsl.sh next to aqemu.exe." ) );
		return;
	}

	if( Process->state() != QProcess::NotRunning )
	{
		QMessageBox::information( this, tr( "Busy" ), tr( "A patch run is already in progress." ) );
		return;
	}

#ifdef Q_OS_WIN
	QString distro, user;
	if( ! Ensure_WSL_Creds( &distro, &user ) )
		return;

	const QString root_wsl = Windows_Path_To_WSL( root );
	const QString script_wsl = Windows_Path_To_WSL( script_win );
	const QString extras_wsl = Windows_Path_To_WSL( AQ_Inferno_Extras_Dir() );
	auto shell_quote = []( QString s ) {
		return s.replace( QLatin1Char( '\'' ), QStringLiteral( "'\\''" ) );
	};

	// sed CRLF if the script was checked out on Windows; then run with ROOT_IMG.
	const QString bash =
		QStringLiteral(
			"set -e; "
			"SCRIPT='%1'; "
			"sed -i 's/\\r$//' \"$SCRIPT\" 2>/dev/null || true; "
			"export ROOT_IMG='%2'; "
			"export EXTRAS_DIR='%3'; "
			"export WORK=\"${WORK:-$HOME/aqemu-inferno-fs-patch}\"; "
			"bash \"$SCRIPT\"" )
			.arg( shell_quote( script_wsl ),
			      shell_quote( root_wsl ),
			      shell_quote( extras_wsl ) );

	Text_Log->clear();
	Append_Log( tr( "Starting filesystem patch via WSL (Ubuntu KVM + linux-apfs)…" ) );
	Append_Log( tr( "ROOT_IMG=%1" ).arg( root_wsl ) );
	Btn_Start->setEnabled( false );

	Process->setProgram( QStringLiteral( "wsl.exe" ) );
	Process->setArguments( WSL_Bash_Args( distro, user, bash ) );
	Process->setProcessChannelMode( QProcess::MergedChannels );
	Process->start();
	if( ! Process->waitForStarted( 8000 ) )
	{
		Btn_Start->setEnabled( true );
		QMessageBox::critical( this, tr( "WSL" ),
			tr( "Failed to start wsl.exe. Configure File → WSL Launch first." ) );
	}
#else
	Q_UNUSED( script_win );
	QMessageBox::information( this, tr( "Host OS" ),
		tr( "Automated FS patching from this dialog is currently wired for Windows + WSL.\n"
		    "On macOS use hdiutil per https://chefkiss.dev/guides/inferno/fs-patches/\n"
		    "On Linux run extras/Inferno/apply-fs-patches-wsl.sh with ROOT_IMG set." ) );
#endif
}

void Apple_SoC_FS_Patch_Window::On_Process_Output()
{
	Append_Log( QString::fromLocal8Bit( Process->readAll() ) );
}

void Apple_SoC_FS_Patch_Window::On_Process_Finished( int code, QProcess::ExitStatus st )
{
	Btn_Start->setEnabled( true );
	On_Process_Output();
	const bool ok = ( st == QProcess::NormalExit && code == 0 );
	if( ok )
	{
		Append_Log( tr( "\n=== SUCCESS ===" ) );
		emit Patches_Applied( VM );
		QMessageBox::information( this, tr( "Filesystem patches applied" ),
			tr( "Dyld cache + LaunchDaemons updated on the iOS guest root disk "
			    "(not the companion).\n\n"
			    "Restore ramdisk (-initrd) was cleared automatically.\n"
			    "Power On the iOS VM and wait for data migration / setup." ) );
	}
	else
	{
		Append_Log( tr( "\n=== FAILED (exit %1) ===" ).arg( code ) );
		QMessageBox::warning( this, tr( "Filesystem patch failed" ),
			tr( "See the log above. Ensure the iOS VM is Powered Off and "
			    "WSL has KVM (nested virtualization)." ) );
	}
	Refresh_Status();
}
