/****************************************************************************
** Remote QEMU host / optional libvirt URI helper (borrowed from virt-manager UX).
****************************************************************************/

#include "Remote_Host_Window.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>
#include <QIntValidator>

Remote_Host_Window::Remote_Host_Window( QWidget *parent )
	: QDialog( parent )
{
	setWindowTitle( tr( "Remote hosts & external hypervisors" ) );
	resize( 720, 520 );

	QVBoxLayout *lay = new QVBoxLayout( this );
	lay->addWidget( new QLabel( tr(
		"AQEMU is QEMU-native. Remote QEMU uses SSH port forwards to QMP/SPICE. "
		"Xen/LXC and full multi-host libvirt management are available by launching "
		"virt-manager / virsh when installed (same idea as virt-manager’s connection list)." ) ) );

	QHBoxLayout *split = new QHBoxLayout();
	List = new QListWidget();
	split->addWidget( List, 1 );

	QWidget *formW = new QWidget();
	QFormLayout *form = new QFormLayout( formW );
	Edit_Name = new QLineEdit();
	CB_Type = new QComboBox();
	CB_Type->addItem( tr( "Remote QEMU (SSH tunnels)" ), QStringLiteral( "qemu-ssh" ) );
	CB_Type->addItem( tr( "libvirt URI (virt-manager / virsh)" ), QStringLiteral( "libvirt" ) );
	CB_Type->addItem( tr( "Xen via libvirt" ), QStringLiteral( "xen" ) );
	CB_Type->addItem( tr( "LXC via libvirt" ), QStringLiteral( "lxc" ) );
	Edit_Host = new QLineEdit();
	Edit_User = new QLineEdit( QStringLiteral( "root" ) );
	Edit_Ssh_Port = new QLineEdit( QStringLiteral( "22" ) );
	Edit_Qmp_Port = new QLineEdit( QStringLiteral( "4444" ) );
	Edit_Spice_Port = new QLineEdit( QStringLiteral( "5930" ) );
	auto *port_val = new QIntValidator( 1, 65535, this );
	Edit_Ssh_Port->setValidator( port_val );
	Edit_Qmp_Port->setValidator( port_val );
	Edit_Spice_Port->setValidator( port_val );
	Edit_Libvirt_Uri = new QLineEdit();
	Edit_Libvirt_Uri->setPlaceholderText( QStringLiteral( "qemu+ssh://user@host/system" ) );
	form->addRow( tr( "Name" ), Edit_Name );
	form->addRow( tr( "Type" ), CB_Type );
	form->addRow( tr( "Host" ), Edit_Host );
	form->addRow( tr( "SSH user" ), Edit_User );
	form->addRow( tr( "SSH port" ), Edit_Ssh_Port );
	form->addRow( tr( "Remote QMP port" ), Edit_Qmp_Port );
	form->addRow( tr( "Remote SPICE port" ), Edit_Spice_Port );
	form->addRow( tr( "libvirt URI" ), Edit_Libvirt_Uri );
	split->addWidget( formW, 2 );
	lay->addLayout( split, 1 );

	QHBoxLayout *btns = new QHBoxLayout();
	QPushButton *save = new QPushButton( tr( "Save" ) );
	QPushButton *del = new QPushButton( tr( "Delete" ) );
	QPushButton *ssh = new QPushButton( tr( "Open SSH tunnels…" ) );
	QPushButton *libvirt = new QPushButton( tr( "Open in virt-manager / virsh…" ) );
	QPushButton *mig = new QPushButton( tr( "Live migrate help…" ) );
	QPushButton *close = new QPushButton( tr( "Close" ) );
	btns->addWidget( save );
	btns->addWidget( del );
	btns->addWidget( ssh );
	btns->addWidget( libvirt );
	btns->addWidget( mig );
	btns->addStretch( 1 );
	btns->addWidget( close );
	lay->addLayout( btns );

	Help = new QPlainTextEdit();
	Help->setReadOnly( true );
	Help->setMaximumHeight( 120 );
	Help->setPlainText( tr(
		"Remote QEMU: on the remote host start QEMU with -qmp tcp:0.0.0.0:4444,server,nowait "
		"and SPICE similarly. Then Open SSH tunnels to forward those ports to localhost.\n"
		"Live migrate: on the destination run QEMU with -incoming tcp:0:4444 (same machine type/disks). "
		"From the session toolbar choose Migrate → tcp:DEST:4444.\n"
		"Xen/LXC: install libvirt + virt-manager; save a libvirt URI here and open it externally." ) );
	lay->addWidget( Help );

	connect( save, &QPushButton::clicked, this, &Remote_Host_Window::Save_Connection );
	connect( del, &QPushButton::clicked, this, &Remote_Host_Window::Delete_Connection );
	connect( List, &QListWidget::currentRowChanged, this, &Remote_Host_Window::Load_Selected );
	connect( ssh, &QPushButton::clicked, this, &Remote_Host_Window::Open_Ssh_Tunnels );
	connect( libvirt, &QPushButton::clicked, this, &Remote_Host_Window::Launch_Libvirt_Helper );
	connect( mig, &QPushButton::clicked, this, &Remote_Host_Window::Show_Migrate_Help );
	connect( close, &QPushButton::clicked, this, &QDialog::accept );

	Reload_List();
}

QString Remote_Host_Window::Settings_Key() const
{
	return QStringLiteral( "Remote_Hosts/connections_json" );
}

void Remote_Host_Window::Reload_List()
{
	List->clear();
	QSettings s;
	const QJsonArray arr = QJsonDocument::fromJson(
		s.value( Settings_Key() ).toByteArray() ).array();
	for( int i = 0; i < arr.size(); ++i )
	{
		const QJsonObject o = arr.at( i ).toObject();
		QListWidgetItem *it = new QListWidgetItem(
			o.value( "name" ).toString( tr( "(unnamed)" ) ) );
		it->setData( Qt::UserRole, o );
		List->addItem( it );
	}
}

void Remote_Host_Window::Save_Connection()
{
	QJsonObject o;
	o.insert( "name", Edit_Name->text().trimmed().isEmpty()
		? Edit_Host->text().trimmed() : Edit_Name->text().trimmed() );
	o.insert( "type", CB_Type->currentData().toString() );
	o.insert( "host", Edit_Host->text().trimmed() );
	o.insert( "user", Edit_User->text().trimmed() );
	o.insert( "ssh_port", Edit_Ssh_Port->text().trimmed() );
	o.insert( "qmp_port", Edit_Qmp_Port->text().trimmed() );
	o.insert( "spice_port", Edit_Spice_Port->text().trimmed() );
	o.insert( "libvirt_uri", Edit_Libvirt_Uri->text().trimmed() );

	QSettings s;
	QJsonArray arr = QJsonDocument::fromJson( s.value( Settings_Key() ).toByteArray() ).array();
	bool replaced = false;
	for( int i = 0; i < arr.size(); ++i )
	{
		if( arr.at( i ).toObject().value( "name" ).toString() == o.value( "name" ).toString() )
		{
			arr.replace( i, o );
			replaced = true;
			break;
		}
	}
	if( ! replaced )
		arr.append( o );
	s.setValue( Settings_Key(), QJsonDocument( arr ).toJson( QJsonDocument::Compact ) );
	Reload_List();
}

void Remote_Host_Window::Delete_Connection()
{
	QListWidgetItem *it = List->currentItem();
	if( ! it ) return;
	const QString name = it->text();
	QSettings s;
	QJsonArray arr = QJsonDocument::fromJson( s.value( Settings_Key() ).toByteArray() ).array();
	QJsonArray next;
	for( int i = 0; i < arr.size(); ++i )
	{
		if( arr.at( i ).toObject().value( "name" ).toString() != name )
			next.append( arr.at( i ) );
	}
	s.setValue( Settings_Key(), QJsonDocument( next ).toJson( QJsonDocument::Compact ) );
	Reload_List();
}

void Remote_Host_Window::Load_Selected()
{
	QListWidgetItem *it = List->currentItem();
	if( ! it ) return;
	const QJsonObject o = it->data( Qt::UserRole ).toJsonObject();
	Edit_Name->setText( o.value( "name" ).toString() );
	const int ix = CB_Type->findData( o.value( "type" ).toString() );
	if( ix >= 0 ) CB_Type->setCurrentIndex( ix );
	Edit_Host->setText( o.value( "host" ).toString() );
	Edit_User->setText( o.value( "user" ).toString( QStringLiteral( "root" ) ) );
	Edit_Ssh_Port->setText( o.value( "ssh_port" ).toString( QStringLiteral( "22" ) ) );
	Edit_Qmp_Port->setText( o.value( "qmp_port" ).toString( QStringLiteral( "4444" ) ) );
	Edit_Spice_Port->setText( o.value( "spice_port" ).toString( QStringLiteral( "5930" ) ) );
	Edit_Libvirt_Uri->setText( o.value( "libvirt_uri" ).toString() );
}

void Remote_Host_Window::Open_Ssh_Tunnels()
{
	const QString host = Edit_Host->text().trimmed();
	if( host.isEmpty() )
	{
		QMessageBox::warning( this, tr( "SSH" ), tr( "Enter a host name first." ) );
		return;
	}
	const QString user = Edit_User->text().trimmed().isEmpty()
		? QStringLiteral( "root" ) : Edit_User->text().trimmed();

	auto parse_port = [this]( QLineEdit *edit, const QString &label, int fallback, int *out ) -> bool {
		const QString t = edit->text().trimmed();
		const QString use = t.isEmpty() ? QString::number( fallback ) : t;
		bool ok = false;
		const int p = use.toInt( &ok );
		if( ! ok || p < 1 || p > 65535 )
		{
			QMessageBox::warning( this, tr( "SSH" ),
				tr( "%1 must be a TCP port between 1 and 65535." ).arg( label ) );
			return false;
		}
		*out = p;
		return true;
	};

	int ssh_port = 22, qmp = 4444, spice = 5930;
	if( ! parse_port( Edit_Ssh_Port, tr( "SSH port" ), 22, &ssh_port ) )
		return;
	if( ! parse_port( Edit_Qmp_Port, tr( "Remote QMP port" ), 4444, &qmp ) )
		return;
	if( ! parse_port( Edit_Spice_Port, tr( "Remote SPICE port" ), 5930, &spice ) )
		return;

	// Canonical decimal strings for the SSH command line.
	Edit_Ssh_Port->setText( QString::number( ssh_port ) );
	Edit_Qmp_Port->setText( QString::number( qmp ) );
	Edit_Spice_Port->setText( QString::number( spice ) );

	const QStringList args = QStringList()
		<< QStringLiteral( "-N" )
		<< QStringLiteral( "-p" ) << QString::number( ssh_port )
		<< QStringLiteral( "-L" )
		<< QStringLiteral( "127.0.0.1:%1:127.0.0.1:%1" ).arg( qmp )
		<< QStringLiteral( "-L" )
		<< QStringLiteral( "127.0.0.1:%1:127.0.0.1:%1" ).arg( spice )
		<< ( user + QLatin1Char( '@' ) + host );

	const bool ok = QProcess::startDetached( QStringLiteral( "ssh" ), args );
	if( ! ok )
	{
		QMessageBox::information( this, tr( "SSH tunnels" ),
			tr( "Could not start ssh. Run manually:\n\nssh -N -p %1 -L 127.0.0.1:%2:127.0.0.1:%2 "
			    "-L 127.0.0.1:%3:127.0.0.1:%3 %4@%5" )
				.arg( ssh_port ).arg( qmp ).arg( spice ).arg( user, host ) );
		return;
	}
	QMessageBox::information( this, tr( "SSH tunnels" ),
		tr( "SSH tunnel started (background).\n"
		    "Local QMP: 127.0.0.1:%1\nLocal SPICE: 127.0.0.1:%2\n"
		    "Point a remote viewer / future AQEMU remote session at those ports." )
			.arg( qmp ).arg( spice ) );
}

void Remote_Host_Window::Launch_Libvirt_Helper()
{
	QString uri = Edit_Libvirt_Uri->text().trimmed();
	const QString type = CB_Type->currentData().toString();
	if( uri.isEmpty() )
	{
		if( type == QLatin1String( "xen" ) )
			uri = QStringLiteral( "xen:///" );
		else if( type == QLatin1String( "lxc" ) )
			uri = QStringLiteral( "lxc:///" );
		else if( ! Edit_Host->text().trimmed().isEmpty() )
			uri = QString( "qemu+ssh://%1@%2/system" )
				.arg( Edit_User->text().trimmed().isEmpty()
					? QStringLiteral( "root" ) : Edit_User->text().trimmed(),
				      Edit_Host->text().trimmed() );
	}

	// Prefer virt-manager connection; fall back to virsh
	if( QProcess::startDetached( QStringLiteral( "virt-manager" ),
		QStringList() << QStringLiteral( "--connect" ) << uri ) )
	{
		return;
	}
	if( QProcess::startDetached( QStringLiteral( "virsh" ),
		QStringList() << QStringLiteral( "-c" ) << uri << QStringLiteral( "list" ) << QStringLiteral( "--all" ) ) )
	{
		QMessageBox::information( this, tr( "libvirt" ),
			tr( "Started virsh against:\n%1\n\nInstall virt-manager for a full GUI." ).arg( uri ) );
		return;
	}

	QMessageBox::information( this, tr( "libvirt / Xen / LXC" ),
		tr( "Neither virt-manager nor virsh was found on PATH.\n\n"
		    "AQEMU remains QEMU-native. For Xen/LXC and multi-host libvirt, install:\n"
		    "  • virt-manager (GUI)\n  • libvirt + virsh (CLI)\n\n"
		    "Suggested URI:\n%1" ).arg( uri.isEmpty() ? QStringLiteral( "qemu:///system" ) : uri ) );
}

void Remote_Host_Window::Show_Migrate_Help()
{
	QMessageBox::information( this, tr( "Live migrate between hosts" ),
		tr( "QEMU live migration (same family as virt-manager’s migrate):\n\n"
		    "1. Destination: start a matching QEMU with shared/copied disks and\n"
		    "   -incoming tcp:0:4444\n"
		    "2. Source (running VM in AQEMU): Session toolbar → Migrate…\n"
		    "   URI example: tcp:192.168.1.20:4444\n"
		    "3. Optional: use SSH tunnels (this dialog) so the URI can be tcp:127.0.0.1:4444\n\n"
		    "Disks must be accessible on both hosts (shared storage or pre-copied)." ) );
}
