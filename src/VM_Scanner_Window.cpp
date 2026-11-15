/****************************************************************************
** Search a folder for .aqemu files and foreign VMs (vmx/vbox/ovf/ova).
****************************************************************************/

#include "VM_Scanner_Window.h"

#include "Main_Window.h"
#include "Foreign_VM_Import.h"
#include "Appliance_Import_Window.h"
#include "Utils.h"

#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QApplication>

static QStringList Default_Scan_Roots()
{
	QStringList roots;
	const QString home = QDir::homePath();
	roots << QSettings().value( QStringLiteral( "VM_Directory" ), AQEMU_Default_VM_Directory() ).toString();
	roots << home + QStringLiteral( "/Documents/Virtual Machines" );
	roots << home + QStringLiteral( "/VirtualBox VMs" );
	roots << home + QStringLiteral( "/vmware" );
	roots << home + QStringLiteral( "/Virtual Machines" );
#ifdef Q_OS_WIN
	roots << home + QStringLiteral( "\\Documents\\Virtual Machines" );
	roots << home + QStringLiteral( "\\VirtualBox VMs" );
#else
	roots << home + QStringLiteral( "/.local/share/libvirt/images" );
#endif
	roots.removeDuplicates();
	return roots;
}

VM_Scanner_Window::VM_Scanner_Window( Main_Window *owner, QWidget *parent )
	: QDialog( parent )
	, Owner( owner )
	, Stop_Requested( false )
{
	setWindowTitle( tr( "Virtual Machine Scanner" ) );
	resize( 700, 520 );

	QVBoxLayout *lay = new QVBoxLayout( this );
	lay->addWidget( new QLabel( tr(
		"Pick a folder. AQEMU searches for .aqemu files to open in place, and for "
		"VMware (.vmx), VirtualBox (.vbox), and OVF/OVA appliances to import." ) ) );

	QHBoxLayout *folder_row = new QHBoxLayout();
	Edit_Folder = new QLineEdit();
	const QStringList defaults = Default_Scan_Roots();
	for( int i = 0; i < defaults.count(); ++i )
	{
		if( QDir( defaults.at( i ) ).exists() )
		{
			Edit_Folder->setText( QDir::toNativeSeparators( defaults.at( i ) ) );
			break;
		}
	}
	if( Edit_Folder->text().isEmpty() )
		Edit_Folder->setText( QDir::toNativeSeparators( AQEMU_Default_VM_Directory() ) );
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
	int foreign_auto = 0; // 0 ask, 1 yes-all, 2 no-all
	QApplication::processEvents();

	QStringList aqemu_files;
	QStringList foreign_files;
	QDirIterator it( info.absoluteFilePath(),
		QStringList()
			<< QStringLiteral( "*.aqemu" )
			<< QStringLiteral( "*.vmx" )
			<< QStringLiteral( "*.vbox" )
			<< QStringLiteral( "*.ovf" )
			<< QStringLiteral( "*.ova" ),
		QDir::Files | QDir::Hidden | QDir::NoSymLinks,
		QDirIterator::Subdirectories );
	while( it.hasNext() )
	{
		const QString path = it.next();
		const QString lower = path.toLower();
		if( lower.endsWith( QLatin1String( ".aqemu" ) ) )
			aqemu_files << path;
		else
			foreign_files << path;
		if( Stop_Requested )
			break;
		if( ( ( aqemu_files.count() + foreign_files.count() ) % 50 ) == 0 )
		{
			Label_Status->setText( tr( "Searching… %1 file(s) so far" )
				.arg( aqemu_files.count() + foreign_files.count() ) );
			QApplication::processEvents();
		}
	}
	aqemu_files.sort();
	foreign_files.sort();

	int added = 0;
	int skipped = 0;
	int failed = 0;
	int imported = 0;
	QStringList lines;

	for( int i = 0; i < aqemu_files.size(); ++i )
	{
		if( Stop_Requested )
			break;
		QString detail;
		const Main_Window::VM_Scan_Add result = Owner->Add_Scanned_VM( aqemu_files.at( i ), &detail );
		if( result == Main_Window::VM_Scan_Added )
			++added;
		else if( result == Main_Window::VM_Scan_Skipped )
			++skipped;
		else
			++failed;
		lines << detail;
		Label_Status->setText( tr( "Adding %1 of %2…" ).arg( i + 1 ).arg( aqemu_files.size() ) );
		QApplication::processEvents();
	}

	for( int i = 0; i < foreign_files.size(); ++i )
	{
		if( Stop_Requested )
			break;
		const QString path = foreign_files.at( i );
		const QString lower = path.toLower();
		QString kind = QStringLiteral( "foreign" );
		if( lower.endsWith( QLatin1String( ".vmx" ) ) )
			kind = QStringLiteral( "VMware" );
		else if( lower.endsWith( QLatin1String( ".vbox" ) ) )
			kind = QStringLiteral( "VirtualBox" );
		else if( lower.endsWith( QLatin1String( ".ova" ) ) || lower.endsWith( QLatin1String( ".ovf" ) ) )
			kind = QStringLiteral( "OVF/OVA" );

		int mode = QMessageBox::Yes;
		if( foreign_auto == 1 )
			mode = QMessageBox::Yes;
		else if( foreign_auto == 2 )
			mode = QMessageBox::No;
		else
		{
			QMessageBox box( this );
			box.setWindowTitle( tr( "Import %1 VM" ).arg( kind ) );
			box.setText( tr( "Found %1:\n%2\n\nImport this virtual machine into AQEMU?" )
				.arg( kind, QDir::toNativeSeparators( path ) ) );
			box.setStandardButtons( QMessageBox::Yes | QMessageBox::No |
				QMessageBox::YesToAll | QMessageBox::NoToAll );
			box.setDefaultButton( QMessageBox::Yes );
			mode = box.exec();
			if( mode == QMessageBox::YesToAll )
			{
				foreign_auto = 1;
				mode = QMessageBox::Yes;
			}
			else if( mode == QMessageBox::NoToAll )
			{
				foreign_auto = 2;
				mode = QMessageBox::No;
			}
		}

		if( mode != QMessageBox::Yes )
		{
			lines << tr( "Skipped %1: %2" ).arg( kind, path );
			continue;
		}

		bool ok = false;
		if( lower.endsWith( QLatin1String( ".vmx" ) ) || lower.endsWith( QLatin1String( ".vbox" ) ) )
		{
			Foreign_VM_Import_Window win( this );
			win.Set_Source_Path( path );
			if( win.exec() == QDialog::Accepted && win.Get_Imported_VM() )
			{
				Owner->Add_VM_To_List( win.Get_Imported_VM() );
				ok = true;
			}
		}
		else
		{
			Appliance_Import_Window win( this );
			win.Set_Appliance_Path( path );
			if( win.exec() == QDialog::Accepted && win.Get_Imported_VM() )
			{
				Owner->Add_VM_To_List( win.Get_Imported_VM() );
				ok = true;
			}
		}
		if( ok )
		{
			++imported;
			lines << tr( "Imported %1: %2" ).arg( kind, path );
		}
		else
		{
			++failed;
			lines << tr( "Import cancelled/failed: %1" ).arg( path );
		}
		QApplication::processEvents();
	}

	QString summary = tr( "Found %1 .aqemu and %2 foreign VM file(s).\n"
	                      "Opened %3. Already open: %4. Imported: %5. Not added: %6." )
		.arg( aqemu_files.size() )
		.arg( foreign_files.size() )
		.arg( added )
		.arg( skipped )
		.arg( imported )
		.arg( failed );
	if( Stop_Requested )
		summary += QLatin1String( "\n" ) + tr( "Stopped before the scan finished." );
	summary += QLatin1String( "\n\n" );
	summary += lines.join( QLatin1String( "\n" ) );
	Log->setPlainText( summary );
	Label_Status->setText( tr( "Done. Opened %1, imported %2." ).arg( added ).arg( imported ) );

	Button_Start->setEnabled( true );
	Button_Stop->setEnabled( false );
	Edit_Folder->setEnabled( true );
	Stop_Requested = false;
}
