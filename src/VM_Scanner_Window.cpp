/****************************************************************************
** Search a folder for .aqemu files and add them to the VM list.
****************************************************************************/

#include "VM_Scanner_Window.h"

#include "Main_Window.h"

#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QApplication>

VM_Scanner_Window::VM_Scanner_Window( Main_Window *owner, QWidget *parent )
	: QDialog( parent )
	, Owner( owner )
	, Stop_Requested( false )
{
	setWindowTitle( tr( "Virtual Machine Scanner" ) );
	resize( 640, 460 );

	QVBoxLayout *lay = new QVBoxLayout( this );
	lay->addWidget( new QLabel( tr(
		"Pick a folder. AQEMU searches that folder and every folder inside it "
		"for .aqemu files, then opens each virtual machine from where it already is." ) ) );

	QHBoxLayout *folder_row = new QHBoxLayout();
	Edit_Folder = new QLineEdit();
	const QString vm_dir = QSettings().value(
		QStringLiteral( "VM_Directory" ),
		QDir::homePath() + QStringLiteral( "/.aqemu/" ) ).toString();
	Edit_Folder->setText( QDir::toNativeSeparators( vm_dir ) );
	QPushButton *browse = new QPushButton( tr( "Browse…" ) );
	folder_row->addWidget( Edit_Folder, 1 );
	folder_row->addWidget( browse );
	lay->addLayout( folder_row );

	Label_Status = new QLabel( tr( "Choose a folder, then click Start." ) );
	lay->addWidget( Label_Status );

	Log = new QPlainTextEdit();
	Log->setReadOnly( true );
	lay->addWidget( Log, 1 );

	QHBoxLayout *buttons = new QHBoxLayout();
	Button_Start = new QPushButton( tr( "Start" ) );
	Button_Stop = new QPushButton( tr( "Stop" ) );
	Button_Stop->setEnabled( false );
	QPushButton *close = new QPushButton( tr( "Close" ) );
	buttons->addWidget( Button_Start );
	buttons->addWidget( Button_Stop );
	buttons->addStretch( 1 );
	buttons->addWidget( close );
	lay->addLayout( buttons );

	connect( browse, &QPushButton::clicked, this, &VM_Scanner_Window::Browse );
	connect( Button_Start, &QPushButton::clicked, this, &VM_Scanner_Window::Start );
	connect( Button_Stop, &QPushButton::clicked, this, &VM_Scanner_Window::Stop );
	connect( close, &QPushButton::clicked, this, &QDialog::reject );
}

void VM_Scanner_Window::Browse()
{
	const QString start = Edit_Folder->text().trimmed().isEmpty()
		? QDir::homePath() : Edit_Folder->text().trimmed();
	const QString dir = QFileDialog::getExistingDirectory(
		this, tr( "Folder to scan" ), start );
	if( ! dir.isEmpty() )
		Edit_Folder->setText( QDir::toNativeSeparators( dir ) );
}

void VM_Scanner_Window::Stop()
{
	Stop_Requested = true;
}

void VM_Scanner_Window::Start()
{
	if( ! Owner )
		return;

	const QString folder = Edit_Folder->text().trimmed();
	QFileInfo info( folder );
	if( folder.isEmpty() || ! info.isDir() )
	{
		Label_Status->setText( tr( "That folder does not exist." ) );
		return;
	}

	Stop_Requested = false;
	Button_Start->setEnabled( false );
	Button_Stop->setEnabled( true );
	Edit_Folder->setEnabled( false );
	Log->clear();
	Label_Status->setText( tr( "Searching…" ) );
	QApplication::processEvents();

	QStringList found;
	QDirIterator it( info.absoluteFilePath(),
		QStringList() << QStringLiteral( "*.aqemu" ),
		QDir::Files | QDir::Hidden | QDir::NoSymLinks,
		QDirIterator::Subdirectories );
	while( it.hasNext() )
	{
		found << it.next();
		if( Stop_Requested )
			break;
		if( ( found.count() % 50 ) == 0 )
		{
			Label_Status->setText( tr( "Searching… %1 file(s) so far" ).arg( found.count() ) );
			QApplication::processEvents();
		}
	}
	found.sort();

	int added = 0;
	int skipped = 0;
	int failed = 0;
	QStringList lines;
	for( int i = 0; i < found.size(); ++i )
	{
		if( Stop_Requested )
			break;
		QString detail;
		const Main_Window::VM_Scan_Add result = Owner->Add_Scanned_VM( found.at( i ), &detail );
		if( result == Main_Window::VM_Scan_Added )
			++added;
		else if( result == Main_Window::VM_Scan_Skipped )
			++skipped;
		else
			++failed;
		lines << detail;
		Label_Status->setText( tr( "Adding %1 of %2…" ).arg( i + 1 ).arg( found.size() ) );
		QApplication::processEvents();
	}

	QString summary = tr( "Found %1 .aqemu file(s).\nAdded %2. Already open: %3. Not added: %4." )
		.arg( found.size() )
		.arg( added )
		.arg( skipped )
		.arg( failed );
	if( Stop_Requested )
		summary += QLatin1String( "\n" ) + tr( "Stopped before the scan finished." );
	summary += QLatin1String( "\n\n" );
	summary += lines.join( QLatin1String( "\n" ) );
	Log->setPlainText( summary );
	Label_Status->setText( tr( "Found %1. Added %2." ).arg( found.size() ).arg( added ) );

	Button_Start->setEnabled( true );
	Button_Stop->setEnabled( false );
	Edit_Folder->setEnabled( true );
	Stop_Requested = false;
}
