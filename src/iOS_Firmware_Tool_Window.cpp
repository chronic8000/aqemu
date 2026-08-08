#include "iOS_Firmware_Tool_Window.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QProcessEnvironment>
#include "AQ_UI_Style.h"
#include "Utils.h"

iOS_Firmware_Tool_Window::iOS_Firmware_Tool_Window( QWidget *parent )
	: QDialog( parent )
	, Process( new QProcess( this ) )
	, Last_Operation( Pending_Op::None )
{
	setWindowTitle( tr( "iOS Firmware Tool" ) );
	resize( AQ_Px( 780, this ), AQ_Px( 560, this ) );
	PyIMG4_Exe = Find_PyIMG4_Executable();

	connect( Process, &QProcess::readyReadStandardOutput, this, &iOS_Firmware_Tool_Window::On_Process_Output );
	connect( Process, &QProcess::readyReadStandardError, this, &iOS_Firmware_Tool_Window::On_Process_Output );
	connect( Process, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
	         this, &iOS_Firmware_Tool_Window::On_Process_Finished );

	Setup_Ui();
}

QString iOS_Firmware_Tool_Window::Find_PyIMG4_Executable() const
{
	const QString app_dir = QCoreApplication::applicationDirPath();
	const QStringList candidates = {
		app_dir + "/pyimg4.exe",
		app_dir + "/tools/pyimg4.exe",
		QStandardPaths::findExecutable( "pyimg4" ),
		QStandardPaths::findExecutable( "pyimg4.exe" )
	};

	for( const QString &path : candidates )
	{
		if( ! path.isEmpty() && QFile::exists( path ) )
			return QDir::toNativeSeparators( path );
	}
	return QString();
}

bool iOS_Firmware_Tool_Window::Ensure_PyIMG4_Available()
{
	if( PyIMG4_Exe.isEmpty() )
		PyIMG4_Exe = Find_PyIMG4_Executable();
	if( PyIMG4_Exe.isEmpty() || ! QFile::exists( PyIMG4_Exe ) )
	{
		QMessageBox::warning( this, tr( "pyimg4 not found" ),
			tr( "pyimg4 was not found next to AQEMU or on PATH.\n\n"
			    "Install pyimg4 and ensure it is available as `pyimg4` "
			    "(or place pyimg4.exe beside aqemu.exe)." ) );
		return false;
	}
	return true;
}

void iOS_Firmware_Tool_Window::Setup_Ui()
{
	QVBoxLayout *main_lay = new QVBoxLayout( this );

	QLabel *title = new QLabel( tr( "<b>iOS Firmware Unpacker</b>" ) );
	title->setStyleSheet( QStringLiteral( "font-size: 14px;" ) );
	main_lay->addWidget( title );

	QLabel *subtitle = new QLabel( tr(
		"Unpack IPSW archives and process Image4 payloads (DeviceTree, kernelcache, SEP) "
		"for Apple SoC QEMU. Requires an external pyimg4 install for IM4P steps." ) );
	subtitle->setWordWrap( true );
	main_lay->addWidget( subtitle );

	QGroupBox *gb_ipsw = new QGroupBox( tr( "Step 1: Unpack IPSW Archive" ) );
	QGridLayout *grid1 = new QGridLayout( gb_ipsw );

	grid1->addWidget( new QLabel( tr( "IPSW File:" ) ), 0, 0 );
	Edit_IPSW_Path = new QLineEdit();
	Edit_IPSW_Path->setPlaceholderText( tr( "Select an iOS IPSW or ZIP…" ) );
	grid1->addWidget( Edit_IPSW_Path, 0, 1 );
	Btn_Browse_IPSW = new QPushButton( tr( "Browse..." ) );
	connect( Btn_Browse_IPSW, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Browse_IPSW_File );
	grid1->addWidget( Btn_Browse_IPSW, 0, 2 );

	grid1->addWidget( new QLabel( tr( "Extraction Output Directory:" ) ), 1, 0 );
	Edit_Output_Dir = new QLineEdit();
	Edit_Output_Dir->setText( QDir::toNativeSeparators(
		QCoreApplication::applicationDirPath() + "/firmware_extracted" ) );
	grid1->addWidget( Edit_Output_Dir, 1, 1 );
	Btn_Browse_OutDir = new QPushButton( tr( "Browse..." ) );
	connect( Btn_Browse_OutDir, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Browse_Output_Dir );
	grid1->addWidget( Btn_Browse_OutDir, 1, 2 );

	Btn_Start_Unpack = new QPushButton( tr( "Unpack IPSW" ) );
	connect( Btn_Start_Unpack, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Run_IPSW_Extraction );
	grid1->addWidget( Btn_Start_Unpack, 2, 1, 1, 2 );
	main_lay->addWidget( gb_ipsw );

	QGroupBox *gb_im4p = new QGroupBox( tr( "Step 2: Process IM4P Payload (pyimg4)" ) );
	QGridLayout *grid2 = new QGridLayout( gb_im4p );

	grid2->addWidget( new QLabel( tr( "IM4P Payload File:" ) ), 0, 0 );
	Edit_IM4P_Path = new QLineEdit();
	Edit_IM4P_Path->setPlaceholderText( tr( "Select an .im4p file…" ) );
	grid2->addWidget( Edit_IM4P_Path, 0, 1 );
	Btn_Browse_IM4P = new QPushButton( tr( "Browse..." ) );
	connect( Btn_Browse_IM4P, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Browse_IM4P_File );
	grid2->addWidget( Btn_Browse_IM4P, 0, 2 );

	grid2->addWidget( new QLabel( tr( "Operation:" ) ), 1, 0 );
	CB_IM4P_Action = new QComboBox();
	CB_IM4P_Action->addItem( tr( "Extract raw payload" ), "extract_raw" );
	CB_IM4P_Action->addItem( tr( "Decrypt payload (--key / --iv)" ), "decrypt" );
	CB_IM4P_Action->addItem( tr( "Show payload info" ), "info" );
	grid2->addWidget( CB_IM4P_Action, 1, 1, 1, 2 );

	grid2->addWidget( new QLabel( tr( "AES IV (hex, optional):" ) ), 2, 0 );
	Edit_AES_IV = new QLineEdit();
	grid2->addWidget( Edit_AES_IV, 2, 1, 1, 2 );

	grid2->addWidget( new QLabel( tr( "AES Key (hex, optional):" ) ), 3, 0 );
	Edit_AES_Key = new QLineEdit();
	grid2->addWidget( Edit_AES_Key, 3, 1, 1, 2 );

	Btn_Start_IM4P = new QPushButton( tr( "Run pyimg4" ) );
	connect( Btn_Start_IM4P, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Run_IM4P_Operation );
	grid2->addWidget( Btn_Start_IM4P, 4, 1, 1, 2 );
	main_lay->addWidget( gb_im4p );

	QGroupBox *gb_log = new QGroupBox( tr( "Output log" ) );
	QVBoxLayout *log_lay = new QVBoxLayout( gb_log );
	Text_Console_Log = new QTextEdit();
	Text_Console_Log->setReadOnly( true );
	Text_Console_Log->setStyleSheet(
		QStringLiteral( "font-family: Consolas, monospace; font-size: 11px;" ) );
	log_lay->addWidget( Text_Console_Log );
	main_lay->addWidget( gb_log, 1 );

	QGroupBox *gb_paths = new QGroupBox( tr( "Extracted file locations" ) );
	QHBoxLayout *paths_lay = new QHBoxLayout( gb_paths );
	Text_Result_Paths = new QTextEdit();
	Text_Result_Paths->setReadOnly( true );
	Text_Result_Paths->setMaximumHeight( AQ_Px( 70, this ) );
	paths_lay->addWidget( Text_Result_Paths, 1 );

	Btn_Copy_Paths = new QPushButton( tr( "Copy locations" ) );
	connect( Btn_Copy_Paths, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Copy_Output_Paths );
	paths_lay->addWidget( Btn_Copy_Paths );
	main_lay->addWidget( gb_paths );

	QHBoxLayout *btn_lay = new QHBoxLayout();
	btn_lay->addStretch( 1 );
	QPushButton *btn_close = new QPushButton( tr( "Close" ) );
	connect( btn_close, &QPushButton::clicked, this, &QDialog::accept );
	btn_lay->addWidget( btn_close );
	main_lay->addLayout( btn_lay );
}

void iOS_Firmware_Tool_Window::Browse_IPSW_File()
{
	const QString file = QFileDialog::getOpenFileName(
		this, tr( "Select Apple iOS IPSW File" ),
		Edit_IPSW_Path->text(),
		tr( "Apple IPSW Firmware (*.ipsw *.zip);;All Files (*.*)" ) );
	if( ! file.isEmpty() )
		Edit_IPSW_Path->setText( QDir::toNativeSeparators( file ) );
}

void iOS_Firmware_Tool_Window::Browse_Output_Dir()
{
	const QString dir = QFileDialog::getExistingDirectory(
		this, tr( "Select Target Extraction Folder" ), Edit_Output_Dir->text() );
	if( ! dir.isEmpty() )
		Edit_Output_Dir->setText( QDir::toNativeSeparators( dir ) );
}

void iOS_Firmware_Tool_Window::Browse_IM4P_File()
{
	const QString file = QFileDialog::getOpenFileName(
		this, tr( "Select IM4P Payload File" ),
		Edit_IM4P_Path->text(),
		tr( "Image4 Payload (*.im4p);;All Files (*.*)" ) );
	if( ! file.isEmpty() )
		Edit_IM4P_Path->setText( QDir::toNativeSeparators( file ) );
}

void iOS_Firmware_Tool_Window::Run_IPSW_Extraction()
{
	const QString ipsw = Edit_IPSW_Path->text().trimmed();
	const QString out_dir = Edit_Output_Dir->text().trimmed();

	if( ipsw.isEmpty() || ! QFile::exists( ipsw ) )
	{
		QMessageBox::warning( this, tr( "Missing IPSW" ),
			tr( "Please select a valid .ipsw or .zip firmware file." ) );
		return;
	}
	QDir().mkpath( out_dir );

	Text_Console_Log->clear();
	Text_Console_Log->append( QString( "Unpacking IPSW: %1\nTarget Directory: %2\n" ).arg( ipsw, out_dir ) );

	// Pass paths via environment variables so they never enter -Command text.
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert( QStringLiteral( "AQEMU_IPSW_PATH" ), ipsw );
	env.insert( QStringLiteral( "AQEMU_IPSW_OUT" ), out_dir );
	Process->setProcessEnvironment( env );
	Last_Operation = Pending_Op::IpswExtract;
	Process->start( QStringLiteral( "powershell.exe" ), QStringList()
		<< QStringLiteral( "-NoProfile" )
		<< QStringLiteral( "-NonInteractive" )
		<< QStringLiteral( "-Command" )
		<< QStringLiteral(
			"Expand-Archive -LiteralPath $env:AQEMU_IPSW_PATH "
			"-DestinationPath $env:AQEMU_IPSW_OUT -Force" ) );
}

void iOS_Firmware_Tool_Window::Run_IM4P_Operation()
{
	if( ! Ensure_PyIMG4_Available() )
		return;

	const QString im4p = Edit_IM4P_Path->text().trimmed();
	if( im4p.isEmpty() || ! QFile::exists( im4p ) )
	{
		QMessageBox::warning( this, tr( "Missing IM4P" ),
			tr( "Please select a valid .im4p payload file." ) );
		return;
	}

	const QString action = CB_IM4P_Action->currentData().toString();
	const QString out_raw = im4p + ".dec";

	QStringList args;
	args << "im4p";

	if( action == "info" )
	{
		args << "info" << "-i" << im4p;
	}
	else
	{
		args << "extract" << "-i" << im4p << "-o" << out_raw;
		if( action == "decrypt" )
		{
			if( ! Edit_AES_IV->text().trimmed().isEmpty() )
				args << "--iv" << Edit_AES_IV->text().trimmed();
			if( ! Edit_AES_Key->text().trimmed().isEmpty() )
				args << "--key" << Edit_AES_Key->text().trimmed();
		}
	}

	Text_Console_Log->append( QString( "\nExecuting: %1 %2\n" ).arg( PyIMG4_Exe, args.join( " " ) ) );
	Last_IM4P_Output = ( action == QLatin1String( "info" ) ) ? QString() : out_raw;
	Last_Operation = Pending_Op::Im4pOp;
	Process->start( PyIMG4_Exe, args );
}

void iOS_Firmware_Tool_Window::On_Process_Output()
{
	const QString out = QString::fromLocal8Bit( Process->readAllStandardOutput() );
	const QString err = QString::fromLocal8Bit( Process->readAllStandardError() );
	if( ! out.isEmpty() ) Text_Console_Log->append( out );
	if( ! err.isEmpty() ) Text_Console_Log->append( err );
}

void iOS_Firmware_Tool_Window::On_Process_Finished( int exitCode, QProcess::ExitStatus exitStatus )
{
	Q_UNUSED( exitStatus );
	const Pending_Op op = Last_Operation;
	Last_Operation = Pending_Op::None;

	if( exitCode != 0 )
	{
		Text_Console_Log->append(
			tr( "\nProcess failed with exit code %1" ).arg( exitCode ) );
		return;
	}

	Text_Console_Log->append( tr( "\nTask completed successfully." ) );

	if( op == Pending_Op::Im4pOp )
	{
		if( ! Last_IM4P_Output.isEmpty() )
		{
			Text_Result_Paths->setText( QDir::toNativeSeparators(
				tr( "IM4P output: %1\n" ).arg( Last_IM4P_Output ) ) );
			Text_Console_Log->append(
				tr( "IM4P output: %1" ).arg( Last_IM4P_Output ) );
		}
		return;
	}

	if( op != Pending_Op::IpswExtract )
		return;

	const QString out_dir = Edit_Output_Dir->text().trimmed();
	QString summary;
	summary += QString( "Extraction Folder: %1\n" ).arg( out_dir );

	QString dtb_found;
	QDir d( out_dir + "/Firmware/all_flash" );
	if( d.exists() )
	{
		// Prefer extracted DeviceTree (.dtb / .dec), never raw .im4p for -dtb.
		const QStringList files = d.entryList(
			QStringList() << "*.dtb" << "*.dec" << "*.im4p" << "*.dmg", QDir::Files );
		for( const QString &f : files )
		{
			const QString full = QDir::toNativeSeparators( d.absoluteFilePath( f ) );
			summary += QString( "Payload: %1\n" ).arg( full );
			const bool is_dt = f.contains( "DeviceTree", Qt::CaseInsensitive ) ||
			                   f.contains( "dtb", Qt::CaseInsensitive );
			const bool usable = f.endsWith( QLatin1String( ".dtb" ), Qt::CaseInsensitive ) ||
			                    f.endsWith( QLatin1String( ".dec" ), Qt::CaseInsensitive );
			if( is_dt && usable && dtb_found.isEmpty() )
				dtb_found = full;
		}
	}
	Text_Result_Paths->setText( QDir::toNativeSeparators( summary ) );

	if( ! dtb_found.isEmpty() )
	{
		Text_Console_Log->append(
			tr( "Suggested DeviceTree (extracted): %1" ).arg( dtb_found ) );
		emit DeviceTree_Path_Suggested( dtb_found );
	}
}

void iOS_Firmware_Tool_Window::Copy_Output_Paths()
{
	const QString txt = Text_Result_Paths->toPlainText().trimmed();
	if( ! txt.isEmpty() )
	{
		QApplication::clipboard()->setText( txt );
		QMessageBox::information( this, tr( "Copied" ),
			tr( "Extracted file locations copied to clipboard." ) );
	}
}
