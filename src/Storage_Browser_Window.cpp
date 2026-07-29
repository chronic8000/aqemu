/****************************************************************************
** Simple storage browser over VM_Directory (virt-manager “pool” lite).
****************************************************************************/

#include "Storage_Browser_Window.h"
#include "Utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QDialogButtonBox>

Storage_Browser_Window::Storage_Browser_Window( QWidget *parent )
	: QDialog( parent )
	, Filter_Mode( QStringLiteral( "all" ) )
{
	setWindowTitle( tr( "Storage browser" ) );
	resize( 560, 420 );

	QSettings s;
	#ifdef Q_OS_WIN32
	Root = s.value( QStringLiteral( "VM_Directory" ),
		AQEMU_Default_VM_Directory() ).toString();
	#else
	Root = s.value( QStringLiteral( "VM_Directory" ),
		QDir::homePath() + QStringLiteral( "/.aqemu/" ) ).toString();
	#endif

	QVBoxLayout *lay = new QVBoxLayout( this );
	lay->addWidget( new QLabel( tr(
		"Browse disk images and ISOs in your VM folder (like a virt-manager storage pool)." ) ) );

	QHBoxLayout *rootLay = new QHBoxLayout();
	Edit_Root = new QLineEdit( Root );
	QPushButton *browse = new QPushButton( tr( "Browse…" ) );
	QPushButton *refresh = new QPushButton( tr( "Refresh" ) );
	rootLay->addWidget( new QLabel( tr( "Folder:" ) ) );
	rootLay->addWidget( Edit_Root, 1 );
	rootLay->addWidget( browse );
	rootLay->addWidget( refresh );
	lay->addLayout( rootLay );

	List = new QListWidget();
	lay->addWidget( List, 1 );

	Label_Info = new QLabel();
	Label_Info->setWordWrap( true );
	lay->addWidget( Label_Info );

	QDialogButtonBox *box = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
	lay->addWidget( box );

	connect( browse, &QPushButton::clicked, this, &Storage_Browser_Window::Browse_Root );
	connect( refresh, &QPushButton::clicked, this, &Storage_Browser_Window::Refresh );
	connect( Edit_Root, &QLineEdit::editingFinished, this, &Storage_Browser_Window::Refresh );
	connect( box, &QDialogButtonBox::accepted, this, &Storage_Browser_Window::Accept_Selection );
	connect( box, &QDialogButtonBox::rejected, this, &QDialog::reject );
	connect( List, &QListWidget::itemDoubleClicked, this, &Storage_Browser_Window::On_Double_Click );

	Refresh();
}

void Storage_Browser_Window::Set_Filter_Mode( const QString &mode )
{
	Filter_Mode = mode;
	Refresh();
}

QString Storage_Browser_Window::Selected_Path() const
{
	return Chosen;
}

void Storage_Browser_Window::Browse_Root()
{
	const QString d = QFileDialog::getExistingDirectory( this, tr( "VM storage folder" ), Edit_Root->text() );
	if( ! d.isEmpty() )
	{
		Edit_Root->setText( QDir::toNativeSeparators( d ) );
		Refresh();
	}
}

void Storage_Browser_Window::Refresh()
{
	Root = Edit_Root->text().trimmed();
	List->clear();
	QDir dir( Root );
	if( ! dir.exists() )
	{
		Label_Info->setText( tr( "Folder does not exist." ) );
		return;
	}

	QStringList filters;
	if( Filter_Mode == QLatin1String( "iso" ) )
		filters << "*.iso" << "*.ISO";
	else if( Filter_Mode == QLatin1String( "floppy" ) )
		filters << "*.img" << "*.ima" << "*.vfd" << "*.dsk";
	else if( Filter_Mode == QLatin1String( "disk" ) )
		filters << "*.qcow2" << "*.qcow" << "*.vmdk" << "*.vdi" << "*.raw" << "*.img" << "*.vhdx";
	else
		filters << "*.qcow2" << "*.qcow" << "*.vmdk" << "*.vdi" << "*.raw" << "*.img"
		        << "*.iso" << "*.vhdx" << "*.ima" << "*.vfd";

	const QFileInfoList files = dir.entryInfoList( filters, QDir::Files, QDir::Name );
	qint64 total = 0;
	for( int i = 0; i < files.size(); ++i )
	{
		const QFileInfo &fi = files[i];
		total += fi.size();
		const QString label = QString( "%1  (%2 MiB)" )
			.arg( fi.fileName() )
			.arg( fi.size() / ( 1024.0 * 1024.0 ), 0, 'f', 1 );
		QListWidgetItem *it = new QListWidgetItem( label );
		it->setData( Qt::UserRole, fi.absoluteFilePath() );
		List->addItem( it );
	}
	Label_Info->setText( tr( "%1 file(s), ~%2 GiB under %3" )
		.arg( files.size() )
		.arg( total / ( 1024.0 * 1024.0 * 1024.0 ), 0, 'f', 2 )
		.arg( QDir::toNativeSeparators( Root ) ) );
}

void Storage_Browser_Window::Accept_Selection()
{
	QListWidgetItem *it = List->currentItem();
	if( ! it )
	{
		reject();
		return;
	}
	Chosen = it->data( Qt::UserRole ).toString();
	accept();
}

void Storage_Browser_Window::On_Double_Click()
{
	Accept_Selection();
}
