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
#include "AQ_UI_Style.h"
#include "Utils.h"

iOS_Firmware_Tool_Window::iOS_Firmware_Tool_Window( QWidget *parent )
	: QDialog( parent )
	, Process( new QProcess( this ) )
{
	setWindowTitle( tr( "iOS Firmware Unpacker & pyimg4 Processor" ) );
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
		"C:/msys64/ucrt64/bin/pyimg4.exe",
		"C:/msys64/ucrt64/bin/pyimg4",
		QStandardPaths::findExecutable( "pyimg4" )
	};

	for( const QString &path : candidates )
	{
		if( ! path.isEmpty() && QFile::exists( path ) )
			return QDir::toNativeSeparators( path );
	}
	return QStringLiteral( "pyimg4" );
}

void iOS_Firmware_Tool_Window::Setup_Ui()
{
	QVBoxLayout *main_lay = new QVBoxLayout( this );

	// Top Banner / Intro
	QLabel *title = new QLabel( tr( "<b>Apple iOS Firmware Unpacker & pyimg4 Processor</b>" ) );
	title->setStyleSheet( QStringLiteral( "font-size: 14px; color: #1a5fb4;" ) );
	main_lay->addWidget( title );

	QLabel *subtitle = new QLabel( tr( "Automates IPSW extraction and Image4 payload processing (SEP, iBSS, iBEC) for QEMU Apple SoC emulation." ) );
	subtitle->setWordWrap( true );
	main_lay->addWidget( subtitle );

	Stack_Pages = new QStackedWidget( this );

	// Page 1: IPSW Unpack & Extract
	QWidget *page1 = new QWidget();
	QVBoxLayout *p1_lay = new QVBoxLayout( page1 );
	QGroupBox *gb_ipsw = new QGroupBox( tr( "Step 1: Unpack IPSW Archive" ) );
	QGridLayout *grid1 = new QGridLayout( gb_ipsw );

	grid1->addWidget( new QLabel( tr( "IPSW File:" ) ), 0, 0 );
	Edit_IPSW_Path = new QLineEdit();
	Edit_IPSW_Path->setPlaceholderText( tr( "Select downloaded iOS IPSW file (e.g., iPhone12,1_14.0_Restore.ipsw)..." ) );
	grid1->addWidget( Edit_IPSW_Path, 0, 1 );
	Btn_Browse_IPSW = new QPushButton( tr( "Browse..." ) );
	connect( Btn_Browse_IPSW, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Browse_IPSW_File );
	grid1->addWidget( Btn_Browse_IPSW, 0, 2 );

	grid1->addWidget( new QLabel( tr( "Extraction Output Directory:" ) ), 1, 0 );
	Edit_Output_Dir = new QLineEdit();
	Edit_Output_Dir->setText( QDir::toNativeSeparators( QCoreApplication::applicationDirPath() + "/firmware_extracted" ) );
	grid1->addWidget( Edit_Output_Dir, 1, 1 );
	Btn_Browse_OutDir = new QPushButton( tr( "Browse..." ) );
	connect( Btn_Browse_OutDir, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Browse_Output_Dir );
	grid1->addWidget( Btn_Browse_OutDir, 1, 2 );

	Btn_Start_Unpack = new QPushButton( tr( "Unpack IPSW Payload" ) );
	Btn_Start_Unpack->setStyleSheet( QStringLiteral( "font-weight: bold; background-color: #3584e4; color: white; padding: 6px;" ) );
	connect( Btn_Start_Unpack, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Run_IPSW_Extraction );
	grid1->addWidget( Btn_Start_Unpack, 2, 1, 1, 2 );

	p1_lay->addWidget( gb_ipsw );

	// Page 2: IM4P Payload Operation
	QGroupBox *gb_im4p = new QGroupBox( tr( "Step 2: Process IM4P Payload (pyimg4)" ) );
	QGridLayout *grid2 = new QGridLayout( gb_im4p );

	grid2->addWidget( new QLabel( tr( "IM4P Payload File:" ) ), 0, 0 );
	Edit_IM4P_Path = new QLineEdit();
	Edit_IM4P_Path->setPlaceholderText( tr( "Select IM4P file (e.g. sep-firmware.im4p or iBEC.im4p)..." ) );
	grid2->addWidget( Edit_IM4P_Path, 0, 1 );
	Btn_Browse_IM4P = new QPushButton( tr( "Browse..." ) );
	connect( Btn_Browse_IM4P, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Browse_IM4P_File );
	grid2->addWidget( Btn_Browse_IM4P, 0, 2 );

	grid2->addWidget( new QLabel( tr( "Operation:" ) ), 1, 0 );
	CB_IM4P_Action = new QComboBox();
	CB_IM4P_Action->addItem( tr( "Extract Raw Payload (pyimg4 im4p extract)" ), "extract_raw" );
	CB_IM4P_Action->addItem( tr( "Decrypt Payload with Key (pyimg4 im4p extract --key --iv)" ), "decrypt" );
	CB_IM4P_Action->addItem( tr( "View Payload Stats (pyimg4 im4p stats)" ), "stats" );
	grid2->addWidget( CB_IM4P_Action, 1, 1, 1, 2 );

	grid2->addWidget( new QLabel( tr( "AES IV (Hex, optional):" ) ), 2, 0 );
	Edit_AES_IV = new QLineEdit();
	grid2->addWidget( Edit_AES_IV, 2, 1, 1, 2 );

	grid2->addWidget( new QLabel( tr( "AES Key (Hex, optional):" ) ), 3, 0 );
	Edit_AES_Key = new QLineEdit();
	grid2->addWidget( Edit_AES_Key, 3, 1, 1, 2 );

	Btn_Start_IM4P = new QPushButton( tr( "Execute pyimg4 Action" ) );
	connect( Btn_Start_IM4P, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Run_IM4P_Operation );
	grid2->addWidget( Btn_Start_IM4P, 4, 1, 1, 2 );

	p1_lay->addWidget( gb_im4p );
	Stack_Pages->addWidget( page1 );

	main_lay->addWidget( Stack_Pages, 1 );

	// Output Console Log
	QGroupBox *gb_log = new QGroupBox( tr( "Execution Output Log" ) );
	QVBoxLayout *log_lay = new QVBoxLayout( gb_log );
	Text_Console_Log = new QTextEdit();
	Text_Console_Log->setReadOnly( true );
	Text_Console_Log->setStyleSheet( QStringLiteral( "font-family: Consolas, monospace; font-size: 11px; background-color: #1e1e1e; color: #d4d4d4;" ) );
	log_lay->addWidget( Text_Console_Log );
	main_lay->addWidget( gb_log, 1 );

	// Output File Paths summary card
	QGroupBox *gb_paths = new QGroupBox( tr( "Extracted Output File Locations" ) );
	QHBoxLayout *paths_lay = new QHBoxLayout( gb_paths );
	Text_Result_Paths = new QTextEdit();
	Text_Result_Paths->setReadOnly( true );
	Text_Result_Paths->setMaximumHeight( AQ_Px( 70, this ) );
	paths_lay->addWidget( Text_Result_Paths, 1 );

	Btn_Copy_Paths = new QPushButton( tr( "Copy Locations" ) );
	Btn_Copy_Paths->setIcon( QIcon( QStringLiteral( ":/edit-copy.png" ) ) );
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

void iOS_Firmware_Tool_Window::Browse_IM4M_File()
{
	const QString file = QFileDialog::getOpenFileName(
		this, tr( "Select IM4M Manifest File" ),
		Edit_Repack_IM4M ? Edit_Repack_IM4M->text() : QString(),
		tr( "Image4 Manifest (*.im4m *.plist);;All Files (*.*)" ) );
	if( ! file.isEmpty() && Edit_Repack_IM4M )
		Edit_Repack_IM4M->setText( QDir::toNativeSeparators( file ) );
}

void iOS_Firmware_Tool_Window::Run_IPSW_Extraction()
{
	const QString ipsw = Edit_IPSW_Path->text().trimmed();
	const QString out_dir = Edit_Output_Dir->text().trimmed();

	if( ipsw.isEmpty() || ! QFile::exists( ipsw ) )
	{
		QMessageBox::warning( this, tr( "Missing IPSW" ), tr( "Please select a valid .ipsw or .zip firmware file." ) );
		return;
	}
	QDir().mkpath( out_dir );

	Text_Console_Log->clear();
	Text_Console_Log->append( QString( "Unpacking IPSW: %1\nTarget Directory: %2\n" ).arg( ipsw, out_dir ) );

	// Use powershell Expand-Archive on Windows for zero-dependency native zip unpack
	const QString cmd = QString( "Expand-Archive -Path '%1' -DestinationPath '%2' -Force" )
	                        .arg( ipsw, out_dir );

	Process->start( "powershell.exe", QStringList() << "-Command" << cmd );
}

void iOS_Firmware_Tool_Window::Run_IM4P_Operation()
{
	const QString im4p = Edit_IM4P_Path->text().trimmed();
	if( im4p.isEmpty() || ! QFile::exists( im4p ) )
	{
		QMessageBox::warning( this, tr( "Missing IM4P" ), tr( "Please select a valid .im4p payload file." ) );
		return;
	}

	const QString action = CB_IM4P_Action->currentData().toString();
	const QString out_raw = im4p + ".dec";

	QStringList args;
	args << "im4p";

	if( action == "stats" )
	{
		args << "stats" << "-i" << im4p;
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
	Process->start( PyIMG4_Exe, args );
}

void iOS_Firmware_Tool_Window::Run_Repackage_IMG4()
{
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
	if( exitCode == 0 )
	{
		Text_Console_Log->append( tr( "\n✓ Task Completed Successfully!" ) );
		const QString out_dir = Edit_Output_Dir->text().trimmed();

		QString summary;
		summary += QString( "Extraction Folder: %1\n" ).arg( out_dir );

		QString dtb_found;
		QDir d( out_dir + "/Firmware/all_flash" );
		if( d.exists() )
		{
			const QStringList files = d.entryList( QStringList() << "*.im4p" << "*.dmg" << "*.dtb" << "*.dec", QDir::Files );
			for( const QString &f : files )
			{
				const QString full = QDir::toNativeSeparators( d.absoluteFilePath( f ) );
				summary += QString( "Payload: %1\n" ).arg( full );
				if( ( f.contains( "DeviceTree", Qt::CaseInsensitive ) || f.contains( "dtb", Qt::CaseInsensitive ) ) && dtb_found.isEmpty() )
					dtb_found = full;
			}
		}
		Text_Result_Paths->setText( QDir::toNativeSeparators( summary ) );

		if( ! dtb_found.isEmpty() && parentWidget() )
		{
			// If an active VM has its DeviceTree path empty, auto-populate it
			QLineEdit *dt_edit = parentWidget()->findChild<QLineEdit*>( QStringLiteral( "Edit_DeviceTree_Path" ) );
			if( dt_edit )
			{
				dt_edit->setText( dtb_found );
				Text_Console_Log->append( QString( tr( "👉 Auto-assigned DeviceTree path to active VM: %1" ) ).arg( dtb_found ) );
			}
		}
	}
	else
	{
		Text_Console_Log->append( QString( tr( "\n❌ Process failed with exit code %1" ) ).arg( exitCode ) );
	}
}

void iOS_Firmware_Tool_Window::Copy_Output_Paths()
{
	const QString txt = Text_Result_Paths->toPlainText().trimmed();
	if( ! txt.isEmpty() )
	{
		QApplication::clipboard()->setText( txt );
		QMessageBox::information( this, tr( "Copied" ), tr( "Extracted file locations copied to clipboard!" ) );
	}
}
