#include "iOS_Firmware_Tool_Window.h"

#include "Apple_SoC_FS_Patch_Window.h"
#include "AQ_UI_Style.h"
#include "Utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QIODevice>
#include <QStandardPaths>
#include <QProcessEnvironment>
#include <QUrl>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QWidget>

iOS_Firmware_Tool_Window::iOS_Firmware_Tool_Window( QWidget *parent )
	: QDialog( parent )
	, Process( new QProcess( this ) )
	, Chain_Sep_Ticket( false )
	, Tried_Pip( false )
	, Last_Operation( Pending_Op::None )
{
	setWindowTitle( tr( "iOS Firmware Tool" ) );
	resize( AQ_Px( 780, this ), AQ_Px( 860, this ) );
	PyIMG4_Exe = Find_PyIMG4_Executable();
	Img4_Exe = Find_Img4_Executable();

	connect( Process, &QProcess::readyReadStandardOutput, this, &iOS_Firmware_Tool_Window::On_Process_Output );
	connect( Process, &QProcess::readyReadStandardError, this, &iOS_Firmware_Tool_Window::On_Process_Output );
	connect( Process, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
	         this, &iOS_Firmware_Tool_Window::On_Process_Finished );

	Setup_Ui();
}

QString iOS_Firmware_Tool_Window::Find_PyIMG4_Executable() const
{
	return AQ_Resolve_Host_Tool(
		QStringLiteral( "Apple_SoC_Firmware/pyimg4" ),
		QStringList() << QStringLiteral( "pyimg4" ) << QStringLiteral( "pyimg4.exe" ),
		QStringList() << QStringLiteral( "pyimg4.exe" )
		              << QStringLiteral( "tools/pyimg4.exe" ) );
}

QString iOS_Firmware_Tool_Window::Find_Img4_Executable() const
{
	return AQ_Resolve_Host_Tool(
		QStringLiteral( "Apple_SoC_Firmware/img4" ),
		QStringList() << QStringLiteral( "img4" ) << QStringLiteral( "img4.exe" ),
		QStringList() << QStringLiteral( "img4.exe" )
		              << QStringLiteral( "img4" )
		              << QStringLiteral( "tools/img4.exe" )
		              << QStringLiteral( "img4lib/img4.exe" )
		              << QStringLiteral( "img4lib/img4" )
		              << QStringLiteral( "extras/Inferno/img4.exe" ) );
}

bool iOS_Firmware_Tool_Window::Ensure_Img4_Available()
{
	Img4_Exe = Find_Img4_Executable();
	if( Img4_Exe.isEmpty() || ! QFile::exists( Img4_Exe ) )
	{
		QMessageBox::warning( this, tr( "img4 not found" ),
			tr( "img4 (xerub img4lib) was not found.\n\n"
			    "Set File → Configure → iOS firmware tools → img4, "
			    "or place img4.exe beside aqemu.exe / on PATH." ) );
		return false;
	}
	return true;
}

bool iOS_Firmware_Tool_Window::Ensure_PyIMG4_Available()
{
	PyIMG4_Exe = Find_PyIMG4_Executable();
	if( PyIMG4_Exe.isEmpty() || ! QFile::exists( PyIMG4_Exe ) )
	{
		QMessageBox::warning( this, tr( "pyimg4 not found" ),
			tr( "pyimg4 was not found.\n\n"
			    "Set File → Configure → iOS firmware tools → pyimg4, "
			    "or place pyimg4.exe beside aqemu.exe / on PATH." ) );
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
		"Stay in this window: unpack the IPSW, forge restore/SEP tickets, pack SEP firmware, "
		"then process DeviceTree/kernel IM4P. Python / img4 / pyimg4: File → Configure → "
		"iOS firmware tools (or PATH / next to aqemu.exe)." ) );
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

	QGroupBox *gb_tick = new QGroupBox( tr(
		"Step 2: Forge restore + SEP tickets (this IPSW — no terminal)" ) );
	QGridLayout *gridT = new QGridLayout( gb_tick );
	gridT->addWidget( new QLabel( tr(
		"Uses bundled extras/Inferno scripts. Needs Python 3 with pyasn1 "
		"(AQEMU will pip-install if missing). "
		"ticket.shsh2 is not in the IPSW and is not shipped by AQEMU — "
		"get it from ChefKiss Inferno file setup, then Browse…" ) ),
		0, 0, 1, 3 );

	gridT->addWidget( new QLabel( tr( "Model:" ) ), 1, 0 );
	CB_Ticket_Model = new QComboBox();
	CB_Ticket_Model->addItem( tr( "n104ap — iPhone 11 / t8030" ), QStringLiteral( "n104ap" ) );
	gridT->addWidget( CB_Ticket_Model, 1, 1, 1, 2 );

	gridT->addWidget( new QLabel( tr( "BuildManifest.plist:" ) ), 2, 0 );
	Edit_Manifest = new QLineEdit();
	Edit_Manifest->setPlaceholderText( tr( "Filled after Unpack IPSW" ) );
	gridT->addWidget( Edit_Manifest, 2, 1 );
	auto *btnMan = new QPushButton( tr( "Browse..." ) );
	connect( btnMan, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Browse_Manifest );
	gridT->addWidget( btnMan, 2, 2 );

	gridT->addWidget( new QLabel( tr( "ticket.shsh2:" ) ), 3, 0 );
	Edit_SHSH = new QLineEdit();
	Edit_SHSH->setPlaceholderText( tr( "Not in the IPSW — Browse after you save ChefKiss’s file" ) );
	gridT->addWidget( Edit_SHSH, 3, 1 );
	auto *shshRow = new QHBoxLayout();
	auto *btnShsh = new QPushButton( tr( "Browse..." ) );
	connect( btnShsh, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Browse_SHSH );
	auto *btnShshWeb = new QPushButton( tr( "How to get this…" ) );
	btnShshWeb->setToolTip( tr(
		"Opens ChefKiss Inferno file setup. They host ticket.shsh2 for the unsigned IPSW flow. "
		"AQEMU does not download or ship that blob (Apple Img4 ticket data)." ) );
	connect( btnShshWeb, &QPushButton::clicked, this, []() {
		QDesktopServices::openUrl( QUrl( QStringLiteral(
			"https://chefkiss.dev/guides/inferno/file-setup/" ) ) );
	} );
	shshRow->addWidget( btnShsh );
	shshRow->addWidget( btnShshWeb );
	QWidget *shshBtns = new QWidget();
	shshBtns->setLayout( shshRow );
	gridT->addWidget( shshBtns, 3, 2 );

	gridT->addWidget( new QLabel( tr( "Restore ticket (.der):" ) ), 4, 0 );
	Edit_Ticket_Out = new QLineEdit();
	Edit_Ticket_Out->setPlaceholderText( tr( "Forge writes here — Browse to choose path" ) );
	gridT->addWidget( Edit_Ticket_Out, 4, 1 );
	auto *btnTicketOut = new QPushButton( tr( "Browse..." ) );
	connect( btnTicketOut, &QPushButton::clicked, this, [this]() {
		QString start = Edit_Ticket_Out->text().trimmed();
		if( start.isEmpty() )
			start = QDir( Edit_Output_Dir->text() ).filePath( QStringLiteral( "root_ticket.der" ) );
		const QString file = QFileDialog::getSaveFileName(
			this, tr( "Restore ticket (.der)" ), start,
			tr( "Ticket (*.der);;All (*)" ) );
		if( ! file.isEmpty() )
			Edit_Ticket_Out->setText( QDir::toNativeSeparators( file ) );
	} );
	gridT->addWidget( btnTicketOut, 4, 2 );

	gridT->addWidget( new QLabel( tr( "SEP ticket (.der):" ) ), 5, 0 );
	Edit_Sep_Ticket_Out = new QLineEdit();
	Edit_Sep_Ticket_Out->setPlaceholderText( tr( "Forge writes here — Browse to choose path" ) );
	gridT->addWidget( Edit_Sep_Ticket_Out, 5, 1 );
	auto *btnSepTicketOut = new QPushButton( tr( "Browse..." ) );
	connect( btnSepTicketOut, &QPushButton::clicked, this, [this]() {
		QString start = Edit_Sep_Ticket_Out->text().trimmed();
		if( start.isEmpty() )
			start = QDir( Edit_Output_Dir->text() ).filePath( QStringLiteral( "sep_root_ticket.der" ) );
		const QString file = QFileDialog::getSaveFileName(
			this, tr( "SEP ticket (.der)" ), start,
			tr( "Ticket (*.der);;All (*)" ) );
		if( ! file.isEmpty() )
			Edit_Sep_Ticket_Out->setText( QDir::toNativeSeparators( file ) );
	} );
	gridT->addWidget( btnSepTicketOut, 5, 2 );

	Btn_Forge_Tickets = new QPushButton( tr( "Forge restore + SEP tickets" ) );
	Btn_Forge_Tickets->setToolTip( tr(
		"Writes root_ticket.der (MACHINE → Restore ticket) and sep_root_ticket.der "
		"(input for img4 SEP firmware pack). Applies restore ticket to the current VM." ) );
	connect( Btn_Forge_Tickets, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Run_Forge_Tickets );
	gridT->addWidget( Btn_Forge_Tickets, 6, 1, 1, 2 );
	main_lay->addWidget( gb_tick );

	{
		QSettings s;
		const QString extras = Extras_Dir();
		const QString bundled = QDir( extras ).filePath( QStringLiteral( "ticket.shsh2" ) );
		Edit_SHSH->setText( QDir::toNativeSeparators(
			s.value( QStringLiteral( "Apple_SoC_Firmware/SHSH" ),
			         QFile::exists( bundled ) ? bundled : QString() ).toString() ) );
	}

	QGroupBox *gb_sep = new QGroupBox( tr(
		"Step 3: Pack SEP firmware (img4 — same ChefKiss recipe, no cmd.exe)" ) );
	QGridLayout *gridS = new QGridLayout( gb_sep );
	gridS->addWidget( new QLabel( tr(
		"Needs img4 (xerub img4lib) next to AQEMU or on PATH. "
		"IV+Key concatenated from The Apple Wiki for this SEP IM4P. "
		"Decrypt then wrap with the SEP ticket from Step 2." ) ),
		0, 0, 1, 3 );

	gridS->addWidget( new QLabel( tr( "SEP .im4p:" ) ), 1, 0 );
	Edit_SEP_IM4P = new QLineEdit();
	Edit_SEP_IM4P->setPlaceholderText( tr( "Firmware/all_flash/sep-firmware.n104.RELEASE.im4p" ) );
	gridS->addWidget( Edit_SEP_IM4P, 1, 1 );
	auto *btnSepIm4p = new QPushButton( tr( "Browse..." ) );
	connect( btnSepIm4p, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Browse_SEP_IM4P );
	gridS->addWidget( btnSepIm4p, 1, 2 );

	gridS->addWidget( new QLabel( tr( "IVKEY (IV then key, no space):" ) ), 2, 0 );
	Edit_SEP_IVKEY = new QLineEdit();
	Edit_SEP_IVKEY->setPlaceholderText( tr(
		"SEP-Firmware only: 32-char IV then 64-char Key, no spaces (96 hex)" ) );
	Edit_SEP_IVKEY->setToolTip( tr(
		"Apple Wiki → your IPSW build → iPhone12,1 → heading SEP-Firmware "
		"(sep-firmware.n104.RELEASE.im4p). Paste IV immediately followed by Key. "
		"Do not use iBoot/iBEC keys. Build 18A373 keys do not decrypt 18A5351d." ) );
	gridS->addWidget( Edit_SEP_IVKEY, 2, 1 );
	auto *btnWiki = new QPushButton( tr( "Apple Wiki…" ) );
	connect( btnWiki, &QPushButton::clicked, this, []() {
		QDesktopServices::openUrl( QUrl( QStringLiteral(
			"https://theapplewiki.com/wiki/Firmware_Keys" ) ) );
	} );
	gridS->addWidget( btnWiki, 2, 2 );

	gridS->addWidget( new QLabel( tr( "Decrypted SEP:" ) ), 3, 0 );
	Edit_SEP_Dec = new QLineEdit();
	Edit_SEP_Dec->setPlaceholderText( tr( "img4 decrypt output" ) );
	gridS->addWidget( Edit_SEP_Dec, 3, 1 );
	auto *btnSepDec = new QPushButton( tr( "Browse..." ) );
	connect( btnSepDec, &QPushButton::clicked, this, [this]() {
		QString start = Edit_SEP_Dec->text().trimmed();
		if( start.isEmpty() )
			start = QDir( Edit_Output_Dir->text() ).filePath( QStringLiteral( "sep-firmware.n104.RELEASE" ) );
		const QString file = QFileDialog::getSaveFileName(
			this, tr( "Decrypted SEP" ), start, tr( "All (*)" ) );
		if( ! file.isEmpty() )
			Edit_SEP_Dec->setText( QDir::toNativeSeparators( file ) );
	} );
	gridS->addWidget( btnSepDec, 3, 2 );

	gridS->addWidget( new QLabel( tr( "Packed SEP (.img4):" ) ), 4, 0 );
	Edit_SEP_Out = new QLineEdit();
	Edit_SEP_Out->setPlaceholderText( tr( "MACHINE → SEP firmware" ) );
	gridS->addWidget( Edit_SEP_Out, 4, 1 );
	auto *btnSepOut = new QPushButton( tr( "Browse..." ) );
	connect( btnSepOut, &QPushButton::clicked, this, [this]() {
		QString start = Edit_SEP_Out->text().trimmed();
		if( start.isEmpty() )
			start = QDir( Edit_Output_Dir->text() ).filePath(
				QStringLiteral( "sep-firmware.n104.RELEASE.new.img4" ) );
		const QString file = QFileDialog::getSaveFileName(
			this, tr( "Packed SEP firmware" ), start,
			tr( "IMG4 (*.img4);;All (*)" ) );
		if( ! file.isEmpty() )
			Edit_SEP_Out->setText( QDir::toNativeSeparators( file ) );
	} );
	gridS->addWidget( btnSepOut, 4, 2 );

	Btn_Pack_SEP = new QPushButton( tr( "Decrypt + pack SEP firmware" ) );
	Btn_Pack_SEP->setToolTip( tr(
		"Runs img4 decrypt then img4 -A -F -T rsep with sep_root_ticket.der. "
		"Sets MACHINE → SEP firmware." ) );
	connect( Btn_Pack_SEP, &QPushButton::clicked, this, &iOS_Firmware_Tool_Window::Run_Pack_SEP );
	gridS->addWidget( Btn_Pack_SEP, 5, 1, 1, 2 );
	main_lay->addWidget( gb_sep );

	QGroupBox *gb_im4p = new QGroupBox( tr( "Step 4: Process IM4P (DeviceTree / kernel — pyimg4)" ) );
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

void iOS_Firmware_Tool_Window::Browse_Manifest()
{
	const QString file = QFileDialog::getOpenFileName(
		this, tr( "BuildManifest.plist" ),
		Edit_Manifest->text().isEmpty() ? Edit_Output_Dir->text() : Edit_Manifest->text(),
		tr( "Property list (*.plist);;All (*)" ) );
	if( ! file.isEmpty() )
		Edit_Manifest->setText( QDir::toNativeSeparators( file ) );
}

void iOS_Firmware_Tool_Window::Browse_SHSH()
{
	const QString file = QFileDialog::getOpenFileName(
		this, tr( "ChefKiss ticket.shsh2" ),
		Edit_SHSH->text(),
		tr( "SHSH2 (*.shsh2);;All (*)" ) );
	if( file.isEmpty() )
		return;
	Edit_SHSH->setText( QDir::toNativeSeparators( file ) );
	QSettings s;
	s.setValue( QStringLiteral( "Apple_SoC_Firmware/SHSH" ), Edit_SHSH->text() );
}

void iOS_Firmware_Tool_Window::Browse_SEP_IM4P()
{
	const QString start = Edit_SEP_IM4P->text().isEmpty()
		? Edit_Output_Dir->text() : Edit_SEP_IM4P->text();
	const QString file = QFileDialog::getOpenFileName(
		this, tr( "SEP firmware IM4P" ), start,
		tr( "IM4P (*.im4p);;All (*)" ) );
	if( ! file.isEmpty() )
		Edit_SEP_IM4P->setText( QDir::toNativeSeparators( file ) );
}

void iOS_Firmware_Tool_Window::Run_IPSW_Extraction()
{
	if( ! Ensure_Process_Idle() )
		return;
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
	if( ! Ensure_Process_Idle() )
		return;
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

QString iOS_Firmware_Tool_Window::Extras_Dir() const
{
	return AQ_Inferno_Extras_Dir();
}

QString iOS_Firmware_Tool_Window::Ticket_Script( bool sep_ticket ) const
{
	return QDir( Extras_Dir() ).filePath( sep_ticket
		? QStringLiteral( "create_septicket.py" )
		: QStringLiteral( "create_apticket.py" ) );
}

bool iOS_Firmware_Tool_Window::Find_Python( QString *exe_out, QStringList *prefix_args_out ) const
{
	if( ! exe_out || ! prefix_args_out )
		return false;
	prefix_args_out->clear();
	*exe_out = AQ_Resolve_Host_Tool(
		QStringLiteral( "Apple_SoC_Firmware/Python" ),
		QStringList() << QStringLiteral( "py" ) << QStringLiteral( "py.exe" )
		              << QStringLiteral( "python3" ) << QStringLiteral( "python3.exe" )
		              << QStringLiteral( "python" ) << QStringLiteral( "python.exe" ),
		QStringList() << QStringLiteral( "python.exe" ) << QStringLiteral( "python3.exe" )
		              << QStringLiteral( "py.exe" ) << QStringLiteral( "python" )
		              << QStringLiteral( "python3" ) << QStringLiteral( "py" ) );
	if( exe_out->isEmpty() )
		return false;
	if( QFileInfo( *exe_out ).completeBaseName().compare( QLatin1String( "py" ), Qt::CaseInsensitive ) == 0 )
		*prefix_args_out << QStringLiteral( "-3" );
	return true;
}

bool iOS_Firmware_Tool_Window::Ensure_Python_Ready()
{
	if( ! Find_Python( &Python_Exe, &Python_Prefix ) )
	{
		QMessageBox::warning( this, tr( "Python 3" ),
			tr( "Python 3 was not found.\n\n"
			    "Set File → Configure → iOS firmware tools → Python 3, "
			    "or install Python and add it to PATH. AQEMU starts it — you do not type commands." ) );
		return false;
	}
	if( ! QFile::exists( Ticket_Script( false ) ) )
	{
		QMessageBox::warning( this, tr( "Missing script" ),
			tr( "create_apticket.py was not found:\n%1" ).arg( Ticket_Script( false ) ) );
		return false;
	}
	if( ! QFile::exists( Ticket_Script( true ) ) )
	{
		QMessageBox::warning( this, tr( "Missing script" ),
			tr( "create_septicket.py was not found:\n%1" ).arg( Ticket_Script( true ) ) );
		return false;
	}
	return true;
}

bool iOS_Firmware_Tool_Window::Ensure_Process_Idle()
{
	if( Process->state() == QProcess::NotRunning )
		return true;
	QMessageBox::information( this, tr( "Busy" ), tr( "Wait for the current task to finish." ) );
	return false;
}

QString iOS_Firmware_Tool_Window::Restore_Ramdisk_From_Manifest( const QString &manifest_path,
                                                                const QString &extract_dir ) const
{
	QFile f( manifest_path );
	if( ! f.open( QIODevice::ReadOnly ) )
		return QString();
	const QByteArray data = f.readAll();
	f.close();

	const QString model = CB_Ticket_Model
		? CB_Ticket_Model->currentData().toString() : QString();
	const QRegularExpression path_xml(
		QStringLiteral( "<key>Path</key>\\s*<string>([^<]+\\.dmg)</string>" ),
		QRegularExpression::CaseInsensitiveOption );
	const QRegularExpression numbered_dmg( QStringLiteral( "([0-9]{3}-[0-9]+-[0-9]+\\.dmg)" ) );

	QString best_path;
	int best_score = -1;
	int pos = 0;
	const QByteArray needle( "RestoreRamDisk" );
	while( ( pos = data.indexOf( needle, pos ) ) >= 0 )
	{
		const QString ahead = QString::fromUtf8( data.mid( pos, 4096 ) );
		if( ahead.contains( QLatin1String( "CustomerRamDisk" ), Qt::CaseInsensitive )
		    && ! ahead.contains( QLatin1String( ".dmg" ), Qt::CaseInsensitive ) )
		{
			pos += needle.size();
			continue;
		}

		QString name;
		const QRegularExpressionMatch xml = path_xml.match( ahead );
		if( xml.hasMatch() )
			name = QFileInfo( xml.captured( 1 ).trimmed() ).fileName();
		if( name.isEmpty() )
		{
			const QRegularExpressionMatch dmg = numbered_dmg.match( ahead );
			if( dmg.hasMatch() )
				name = dmg.captured( 1 );
		}
		if( name.isEmpty() || ! name.endsWith( QLatin1String( ".dmg" ), Qt::CaseInsensitive ) )
		{
			pos += needle.size();
			continue;
		}

		const QString cand = QDir( extract_dir ).filePath( name );
		if( ! QFile::exists( cand ) )
		{
			pos += needle.size();
			continue;
		}

		const QString back = QString::fromUtf8( data.mid( qMax( 0, pos - 8000 ), 8000 ) );
		int score = 1;
		if( ! model.isEmpty() && back.contains( model, Qt::CaseInsensitive ) )
			score += 10;
		if( back.contains( QLatin1String( "Erase" ), Qt::CaseInsensitive ) )
			score += 5;
		if( back.contains( QLatin1String( "Customer" ), Qt::CaseInsensitive ) )
			score += 8;
		// Update ramdisk takes verify_storage_for_update + newfs_apfs System.
		// Inferno GUI restore needs Customer/Erase (038-44135 on 18A5351d).
		if( back.contains( QLatin1String( "Update" ), Qt::CaseInsensitive )
		    && ! back.contains( QLatin1String( "Erase" ), Qt::CaseInsensitive ) )
			score -= 20;
		if( score > best_score )
		{
			best_score = score;
			best_path = cand;
		}
		pos += needle.size();
	}

	if( ! best_path.isEmpty() )
		return best_path;

	const QString restore_plist = QDir( extract_dir ).filePath( QStringLiteral( "Restore.plist" ) );
	QFile rf( restore_plist );
	if( rf.open( QIODevice::ReadOnly ) )
	{
		const QString rtext = QString::fromUtf8( rf.readAll() );
		QRegularExpression user_re(
			QStringLiteral( "RestoreRamDisks</key>\\s*<dict>[\\s\\S]{0,400}?<key>User</key>\\s*<string>([^<]+\\.dmg)</string>" ),
			QRegularExpression::CaseInsensitiveOption );
		const QRegularExpressionMatch um = user_re.match( rtext );
		if( um.hasMatch() )
		{
			const QString cand = QDir( extract_dir ).filePath(
				QFileInfo( um.captured( 1 ).trimmed() ).fileName() );
			if( QFile::exists( cand ) )
				return cand;
		}
	}
	return QString();
}

void iOS_Firmware_Tool_Window::Suggest_From_Extract_Dir()
{
	const QString out_dir = Edit_Output_Dir->text().trimmed();
	if( out_dir.isEmpty() )
		return;
	QDir d( out_dir );
	const QString man = d.filePath( QStringLiteral( "BuildManifest.plist" ) );
	if( QFile::exists( man ) )
		Edit_Manifest->setText( QDir::toNativeSeparators( man ) );
	if( Edit_Ticket_Out->text().trimmed().isEmpty() )
		Edit_Ticket_Out->setText( QDir::toNativeSeparators(
			d.filePath( QStringLiteral( "root_ticket.der" ) ) ) );
	if( Edit_Sep_Ticket_Out->text().trimmed().isEmpty() )
		Edit_Sep_Ticket_Out->setText( QDir::toNativeSeparators(
			d.filePath( QStringLiteral( "sep_root_ticket.der" ) ) ) );

	QStringList dt = d.entryList( QStringList() << QStringLiteral( "*DeviceTree*.im4p" ), QDir::Files );
	QDir flash( d.filePath( QStringLiteral( "Firmware/all_flash" ) ) );
	if( dt.isEmpty() && flash.exists() )
		dt = flash.entryList( QStringList() << QStringLiteral( "*DeviceTree*.im4p" ), QDir::Files );
	if( ! dt.isEmpty() && Edit_IM4P_Path->text().trimmed().isEmpty() )
	{
		const QString base = ( flash.exists() && QFile::exists( flash.absoluteFilePath( dt.first() ) ) )
			? flash.absoluteFilePath( dt.first() ) : d.absoluteFilePath( dt.first() );
		Edit_IM4P_Path->setText( QDir::toNativeSeparators( base ) );
	}

	QDir flash2( d.filePath( QStringLiteral( "Firmware/all_flash" ) ) );
	const QString sep_im4p = flash2.filePath( QStringLiteral( "sep-firmware.n104.RELEASE.im4p" ) );
	if( QFile::exists( sep_im4p ) && Edit_SEP_IM4P->text().trimmed().isEmpty() )
		Edit_SEP_IM4P->setText( QDir::toNativeSeparators( sep_im4p ) );
	if( Edit_SEP_Dec->text().trimmed().isEmpty() )
		Edit_SEP_Dec->setText( QDir::toNativeSeparators(
			d.filePath( QStringLiteral( "sep-firmware.n104.RELEASE" ) ) ) );
	if( Edit_SEP_Out->text().trimmed().isEmpty() )
		Edit_SEP_Out->setText( QDir::toNativeSeparators(
			d.filePath( QStringLiteral( "sep-firmware.n104.RELEASE.new.img4" ) ) ) );

	QString ramdisk = Restore_Ramdisk_From_Manifest(
		Edit_Manifest->text().trimmed().isEmpty() ? man : Edit_Manifest->text().trimmed(),
		out_dir );
	if( ramdisk.isEmpty() )
	{
		Text_Console_Log->append( tr(
			"No RestoreRamDisk in BuildManifest — not filling restore ramdisk "
			"(do not use the largest IPSW .dmg; that is usually the OS image)." ) );
	}
	else
	{
		Text_Console_Log->append( tr( "Restore ramdisk (BuildManifest): %1" ).arg(
			QDir::toNativeSeparators( ramdisk ) ) );
		emit Restore_Ramdisk_Suggested( QDir::toNativeSeparators( ramdisk ) );
	}
}

void iOS_Firmware_Tool_Window::Run_Forge_Tickets()
{
	if( ! Ensure_Process_Idle() )
		return;
	if( ! Ensure_Python_Ready() )
		return;
	const QString man = Edit_Manifest->text().trimmed();
	const QString shsh = Edit_SHSH->text().trimmed();
	if( man.isEmpty() || ! QFile::exists( man ) )
	{
		QMessageBox::warning( this, tr( "BuildManifest" ),
			tr( "Unpack the IPSW first (Step 1), or browse to BuildManifest.plist." ) );
		return;
	}
	if( shsh.isEmpty() || ! QFile::exists( shsh ) )
	{
		QMessageBox::warning( this, tr( "ticket.shsh2" ),
			tr( "ticket.shsh2 is not inside the IPSW and AQEMU does not ship it.\n\n"
			    "On ChefKiss Inferno file setup, download the ticket SHSH they provide "
			    "for this flow, save it, then Browse… to that file.\n"
			    "https://chefkiss.dev/guides/inferno/file-setup/" ) );
		return;
	}
	QSettings s;
	s.setValue( QStringLiteral( "Apple_SoC_Firmware/SHSH" ), shsh );
	if( Edit_Ticket_Out->text().trimmed().isEmpty() )
		Edit_Ticket_Out->setText( QDir::toNativeSeparators(
			QDir( Edit_Output_Dir->text() ).filePath( QStringLiteral( "root_ticket.der" ) ) ) );
	if( Edit_Sep_Ticket_Out->text().trimmed().isEmpty() )
		Edit_Sep_Ticket_Out->setText( QDir::toNativeSeparators(
			QDir( Edit_Output_Dir->text() ).filePath( QStringLiteral( "sep_root_ticket.der" ) ) ) );

	Tried_Pip = false;
	Chain_Sep_Ticket = true;
	Last_Operation = Pending_Op::PipInstall;
	Text_Console_Log->append( tr( "\nChecking Python packages (pyasn1)…" ) );
	QStringList args = Python_Prefix;
	args << QStringLiteral( "-c" )
	     << QStringLiteral( "import pyasn1, pyasn1_modules; print('pyasn1-ok')" );
	Process->start( Python_Exe, args );
}

bool iOS_Firmware_Tool_Window::Start_Ticket_Script( bool sep_ticket )
{
	const QString out = sep_ticket
		? Edit_Sep_Ticket_Out->text().trimmed()
		: Edit_Ticket_Out->text().trimmed();
	if( out.isEmpty() || ! QFile::exists( Ticket_Script( sep_ticket ) ) )
		return false;
	QDir().mkpath( QFileInfo( out ).absolutePath() );
	Last_Ticket_Out = out;
	Last_Operation = sep_ticket ? Pending_Op::TicketSep : Pending_Op::TicketAp;
	QStringList args = Python_Prefix;
	args << Ticket_Script( sep_ticket )
	     << CB_Ticket_Model->currentData().toString()
	     << Edit_Manifest->text().trimmed()
	     << Edit_SHSH->text().trimmed()
	     << out;
	Text_Console_Log->append( tr( "\nForging %1…" )
		.arg( sep_ticket ? tr( "SEP ticket" ) : tr( "restore ticket" ) ) );
	Process->start( Python_Exe, args );
	return true;
}

void iOS_Firmware_Tool_Window::Run_Pack_SEP()
{
	if( ! Ensure_Process_Idle() )
		return;
	if( ! Ensure_Img4_Available() )
		return;
	const QString im4p = Edit_SEP_IM4P->text().trimmed();
	QString ivkey = Edit_SEP_IVKEY->text();
	ivkey.replace( QRegularExpression( QStringLiteral( "(?i)\\biv\\s*:" ) ), QString() );
	ivkey.replace( QRegularExpression( QStringLiteral( "(?i)\\bkey\\s*:" ) ), QString() );
	ivkey.replace( QRegularExpression( QStringLiteral( "[^0-9A-Fa-f]" ) ), QString() );
	ivkey = ivkey.toLower();
	const QString ticket = Edit_Sep_Ticket_Out->text().trimmed();
	if( im4p.isEmpty() || ! QFile::exists( im4p ) )
	{
		QMessageBox::warning( this, tr( "SEP IM4P" ),
			tr( "Unpack the IPSW first, or browse to sep-firmware.n104.RELEASE.im4p." ) );
		return;
	}
	if( ivkey.size() != 96 )
	{
		QMessageBox::warning( this, tr( "IVKEY" ),
			tr( "Need the SEP-Firmware IV (32 hex) immediately followed by its Key (64 hex), "
			    "96 characters total. Not iBoot. Must match this IPSW build.\n\n"
			    "You pasted %1 hex character(s)." ).arg( ivkey.size() ) );
		return;
	}
	if( ticket.isEmpty() || ! QFile::exists( ticket ) )
	{
		QMessageBox::warning( this, tr( "SEP ticket" ),
			tr( "Forge restore + SEP tickets (Step 2) first." ) );
		return;
	}
	if( Edit_SEP_Dec->text().trimmed().isEmpty() )
		Edit_SEP_Dec->setText( QDir::toNativeSeparators(
			QDir( Edit_Output_Dir->text() ).filePath( QStringLiteral( "sep-firmware.n104.RELEASE" ) ) ) );
	if( Edit_SEP_Out->text().trimmed().isEmpty() )
		Edit_SEP_Out->setText( QDir::toNativeSeparators(
			QDir( Edit_Output_Dir->text() ).filePath( QStringLiteral( "sep-firmware.n104.RELEASE.new.img4" ) ) ) );
	QDir().mkpath( QFileInfo( Edit_SEP_Dec->text() ).absolutePath() );
	Img4_Stdout_Buf.clear();
	Last_Operation = Pending_Op::Img4Decrypt;
	QStringList args;
	args << QStringLiteral( "-v" )
	     << QStringLiteral( "-i" ) << im4p
	     << QStringLiteral( "-o" ) << Edit_SEP_Dec->text().trimmed()
	     << QStringLiteral( "-k" ) << ivkey;
	Text_Console_Log->append( tr( "\nDecrypting SEP firmware with img4…" ) );
	Process->start( Img4_Exe, args );
}

bool iOS_Firmware_Tool_Window::Start_Img4_Pack()
{
	const QString dec = Edit_SEP_Dec->text().trimmed();
	const QString out = Edit_SEP_Out->text().trimmed();
	const QString ticket = Edit_Sep_Ticket_Out->text().trimmed();
	if( ! QFile::exists( dec ) )
		return false;
	Last_Operation = Pending_Op::Img4Pack;
	QStringList args;
	args << QStringLiteral( "-A" ) << QStringLiteral( "-F" )
	     << QStringLiteral( "-o" ) << out
	     << QStringLiteral( "-i" ) << dec
	     << QStringLiteral( "-M" ) << ticket
	     << QStringLiteral( "-T" ) << QStringLiteral( "rsep" )
	     << QStringLiteral( "-V" ) << Last_Img4_Version;
	Text_Console_Log->append( tr( "\nPacking SEP firmware (tag rsep, version %1)…" )
		.arg( Last_Img4_Version ) );
	Process->start( Img4_Exe, args );
	return true;
}

void iOS_Firmware_Tool_Window::On_Process_Output()
{
	const QString out = QString::fromLocal8Bit( Process->readAllStandardOutput() );
	const QString err = QString::fromLocal8Bit( Process->readAllStandardError() );
	if( ! out.isEmpty() )
	{
		Text_Console_Log->append( out );
		if( Last_Operation == Pending_Op::Img4Decrypt )
			Img4_Stdout_Buf += out;
	}
	if( ! err.isEmpty() ) Text_Console_Log->append( err );
}

void iOS_Firmware_Tool_Window::On_Process_Finished( int exitCode, QProcess::ExitStatus exitStatus )
{
	Q_UNUSED( exitStatus );
	const Pending_Op op = Last_Operation;
	Last_Operation = Pending_Op::None;

	if( op == Pending_Op::PipInstall )
	{
		if( exitCode == 0 )
		{
			if( ! Start_Ticket_Script( false ) )
			{
				QMessageBox::warning( this, tr( "Restore ticket" ),
					tr( "Could not start create_apticket.py:\n%1" )
						.arg( Ticket_Script( false ) ) );
			}
			return;
		}
		if( ! Tried_Pip )
		{
			Tried_Pip = true;
			Last_Operation = Pending_Op::PipInstall;
			Text_Console_Log->append( tr( "Installing pyasn1 + pyasn1-modules (pip)…" ) );
			QStringList args = Python_Prefix;
			args << QStringLiteral( "-m" ) << QStringLiteral( "pip" )
			     << QStringLiteral( "install" ) << QStringLiteral( "--user" )
			     << QStringLiteral( "pyasn1" ) << QStringLiteral( "pyasn1-modules" );
			Process->start( Python_Exe, args );
			return;
		}
		Text_Console_Log->append( tr( "\nCould not import/install pyasn1. Install Python 3 and retry." ) );
		return;
	}

	if( exitCode != 0 )
	{
		Text_Console_Log->append(
			tr( "\nProcess failed with exit code %1" ).arg( exitCode ) );
		return;
	}

	Text_Console_Log->append( tr( "\nTask completed successfully." ) );

	if( op == Pending_Op::TicketAp )
	{
		Text_Result_Paths->append( tr( "Restore ticket: %1" ).arg( Last_Ticket_Out ) );
		emit Restore_Ticket_Suggested( Last_Ticket_Out );
		if( Chain_Sep_Ticket && ! Start_Ticket_Script( true ) )
		{
			QMessageBox::warning( this, tr( "SEP ticket" ),
				tr( "Restore ticket was written, but create_septicket.py could not be started:\n%1" )
					.arg( Ticket_Script( true ) ) );
			Text_Console_Log->append( tr( "SEP ticket step skipped — missing script or output path." ) );
		}
		return;
	}
	if( op == Pending_Op::TicketSep )
	{
		Text_Result_Paths->append( tr( "SEP ticket: %1" ).arg( Last_Ticket_Out ) );
		Text_Console_Log->append( tr(
			"Next: Step 3 — paste Apple Wiki IVKEY and Decrypt + pack SEP firmware." ) );
		return;
	}

	if( op == Pending_Op::Img4Decrypt )
	{
		const QStringList lines = Img4_Stdout_Buf.split( QRegularExpression( QStringLiteral( "[\\r\\n]+" ) ),
			Qt::SkipEmptyParts );
		Last_Img4_Version = lines.isEmpty() ? QStringLiteral( "none" ) : lines.last().trimmed();
		Text_Console_Log->append( tr( "img4 decrypt version token: %1" ).arg( Last_Img4_Version ) );
		if( ! Start_Img4_Pack() )
			Text_Console_Log->append( tr( "Could not start img4 pack (decrypted SEP missing)." ) );
		return;
	}
	if( op == Pending_Op::Img4Pack )
	{
		const QString packed = Edit_SEP_Out->text().trimmed();
		Text_Result_Paths->append( tr( "SEP firmware: %1" ).arg( packed ) );
		emit Sep_Firmware_Suggested( packed );
		Text_Console_Log->append( tr( "SEP firmware applied to MACHINE if the field was empty or updated." ) );
		return;
	}

	if( op == Pending_Op::Im4pOp )
	{
		if( ! Last_IM4P_Output.isEmpty() )
		{
			Text_Result_Paths->setText( QDir::toNativeSeparators(
				tr( "IM4P output: %1\n" ).arg( Last_IM4P_Output ) ) );
			Text_Console_Log->append(
				tr( "IM4P output: %1" ).arg( Last_IM4P_Output ) );
			if( Last_IM4P_Output.endsWith( QLatin1String( ".dec" ), Qt::CaseInsensitive ) ||
			    Last_IM4P_Output.endsWith( QLatin1String( ".dtb" ), Qt::CaseInsensitive ) )
			{
				if( Edit_IM4P_Path->text().contains( QLatin1String( "DeviceTree" ), Qt::CaseInsensitive ) )
					emit DeviceTree_Path_Suggested( Last_IM4P_Output );
			}
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
		const QStringList files = d.entryList(
			QStringList() << "*.dtb" << "*.dec" << "*.im4p" << "*.dmg", QDir::Files );
		for( const QString &f : files )
		{
			const QString full = QDir::toNativeSeparators( d.absoluteFilePath( f ) );
			summary += QString( "Payload: %1\n" ).arg( full );
			const bool is_dt = f.contains( "DeviceTree", Qt::CaseInsensitive );
			const bool usable = f.endsWith( QLatin1String( ".dtb" ), Qt::CaseInsensitive ) ||
			                    f.endsWith( QLatin1String( ".dec" ), Qt::CaseInsensitive );
			if( is_dt && usable && dtb_found.isEmpty() )
				dtb_found = full;
		}
	}
	Text_Result_Paths->setText( QDir::toNativeSeparators( summary ) );
	Suggest_From_Extract_Dir();
	emit Ipsw_Path_Suggested( Edit_IPSW_Path->text().trimmed() );

	if( ! dtb_found.isEmpty() )
	{
		Text_Console_Log->append(
			tr( "Suggested DeviceTree (extracted): %1" ).arg( dtb_found ) );
		emit DeviceTree_Path_Suggested( dtb_found );
	}
	Text_Console_Log->append( tr( "Next: Step 2 — Forge restore + SEP tickets." ) );
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
