#include "Storage_Guest_Tab.h"

#include "TrueNAS_Guest_API.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QListWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QMessageBox>

Storage_Guest_Tab::Storage_Guest_Tab( QWidget *parent )
	: QWidget( parent )
	, Loading( false )
	, Client( new TrueNAS_Guest_Client( this ) )
{
	QVBoxLayout *lay = new QVBoxLayout( this );
	CH_Enable = new QCheckBox( tr( "Enable TrueNAS Guest Integration" ), this );
	lay->addWidget( CH_Enable );

	QLabel *note = new QLabel( tr(
		"AQEMU asks the running TrueNAS guest for pool health and ZFS snapshots. "
		"The token is saved with this VM on this computer and is sent only to the address below. "
		"The guest certificate is accepted for this tab. "
		"Rollback asks TrueNAS to roll back its own snapshot. AQEMU does not change the virtual disk file, "
		"and this is separate from VM → Manage Snapshots." ), this );
	note->setWordWrap( true );
	lay->addWidget( note );

	QHBoxLayout *hostLay = new QHBoxLayout();
	Edit_Host = new QLineEdit( this );
	Edit_Host->setPlaceholderText( tr( "TrueNAS address" ) );
	SB_Port = new QSpinBox( this );
	SB_Port->setRange( 1, 65535 );
	SB_Port->setValue( 443 );
	hostLay->addWidget( new QLabel( tr( "Address:" ), this ) );
	hostLay->addWidget( Edit_Host, 1 );
	hostLay->addWidget( new QLabel( tr( "Port:" ), this ) );
	hostLay->addWidget( SB_Port );
	lay->addLayout( hostLay );

	QHBoxLayout *tokenLay = new QHBoxLayout();
	Edit_Token = new QLineEdit( this );
	Edit_Token->setEchoMode( QLineEdit::Password );
	Edit_Token->setPlaceholderText( tr( "API token" ) );
	tokenLay->addWidget( new QLabel( tr( "Token:" ), this ) );
	tokenLay->addWidget( Edit_Token, 1 );
	lay->addLayout( tokenLay );

	Button_Refresh = new QPushButton( tr( "Refresh from TrueNAS" ), this );
	lay->addWidget( Button_Refresh );

	Pools = new QPlainTextEdit( this );
	Pools->setReadOnly( true );
	Pools->setPlaceholderText( tr( "Guest integration is off." ) );
	lay->addWidget( Pools, 1 );

	Snapshots = new QListWidget( this );
	lay->addWidget( Snapshots, 1 );
	Button_Rollback = new QPushButton( tr( "Roll back in TrueNAS" ), this );
	lay->addWidget( Button_Rollback );

	connect( CH_Enable, &QCheckBox::toggled, this, &Storage_Guest_Tab::Integration_Toggled );
	connect( Edit_Host, &QLineEdit::editingFinished, this, &Storage_Guest_Tab::Save_Form );
	connect( SB_Port, QOverload<int>::of( &QSpinBox::valueChanged ), this, [this]( int ) { Save_Form(); } );
	connect( Edit_Token, &QLineEdit::editingFinished, this, &Storage_Guest_Tab::Save_Form );
	connect( Button_Refresh, &QPushButton::clicked, this, &Storage_Guest_Tab::Refresh );
	connect( Button_Rollback, &QPushButton::clicked, this, &Storage_Guest_Tab::Rollback );

	connect( Client, &TrueNAS_Guest_Client::Pools_Ready, this, [this]( const QList<TrueNAS_Pool_View> &pools ) {
		QString text;
		if( pools.isEmpty() )
			text = tr( "TrueNAS reported no pools." );
		for( int i = 0; i < pools.size(); ++i )
		{
			if( ! text.isEmpty() )
				text += QLatin1Char( '\n' );
			text += pools[i].line;
			if( pools[i].size_bytes >= 0 && pools[i].allocated_bytes >= 0 )
			{
				text += tr( "\nAllocated %1 GiB of %2 GiB." )
					.arg( pools[i].allocated_bytes / ( 1024.0 * 1024.0 * 1024.0 ), 0, 'f', 1 )
					.arg( pools[i].size_bytes / ( 1024.0 * 1024.0 * 1024.0 ), 0, 'f', 1 );
			}
		}
		Pools->setPlainText( text );
	} );
	connect( Client, &TrueNAS_Guest_Client::Snapshots_Ready, this, [this]( const QList<TrueNAS_Snapshot_View> &snaps ) {
		Snapshots->clear();
		for( int i = 0; i < snaps.size(); ++i )
		{
			QListWidgetItem *item = new QListWidgetItem( snaps[i].label, Snapshots );
			item->setData( Qt::UserRole, snaps[i].id );
		}
	} );
	connect( Client, &TrueNAS_Guest_Client::Rollback_Done, this, [this]( const QString &id ) {
		Pools->appendPlainText( tr( "TrueNAS accepted the rollback of %1." ).arg( id ) );
		Refresh();
	} );
	connect( Client, &TrueNAS_Guest_Client::Failed, this, [this]( const QString &message ) {
		Pools->setPlainText( message );
	} );

	Apply_Enabled_State();
}

QString Storage_Guest_Tab::Settings_Group() const
{
	return QStringLiteral( "TrueNASGuest/" ) + VM_UID;
}

void Storage_Guest_Tab::Show_VM( const QString &vm_uid )
{
	if( vm_uid == VM_UID )
		return;
	Save_Form();
	VM_UID = vm_uid;
	Loading = true;
	QSettings settings( QStringLiteral( "aqemu" ), QStringLiteral( "AQEMU" ) );
	settings.beginGroup( Settings_Group() );
	CH_Enable->setChecked( settings.value( QStringLiteral( "enabled" ), false ).toBool() );
	Edit_Host->setText( settings.value( QStringLiteral( "host" ) ).toString() );
	SB_Port->setValue( settings.value( QStringLiteral( "port" ), 443 ).toInt() );
	Edit_Token->setText( settings.value( QStringLiteral( "token" ) ).toString() );
	settings.endGroup();
	Loading = false;
	Pools->clear();
	Snapshots->clear();
	if( ! CH_Enable->isChecked() )
		Pools->setPlaceholderText( tr( "Guest integration is off." ) );
	Apply_Enabled_State();
}

void Storage_Guest_Tab::Integration_Toggled( bool on )
{
	Q_UNUSED( on );
	Apply_Enabled_State();
	Save_Form();
	if( ! CH_Enable->isChecked() )
	{
		Pools->clear();
		Snapshots->clear();
		Pools->setPlaceholderText( tr( "Guest integration is off." ) );
	}
}

void Storage_Guest_Tab::Apply_Enabled_State()
{
	const bool on = CH_Enable->isChecked() && ! VM_UID.isEmpty();
	Edit_Host->setEnabled( on );
	SB_Port->setEnabled( on );
	Edit_Token->setEnabled( on );
	Button_Refresh->setEnabled( on );
	Button_Rollback->setEnabled( on );
	Snapshots->setEnabled( on );
	CH_Enable->setEnabled( ! VM_UID.isEmpty() );
}

void Storage_Guest_Tab::Save_Form()
{
	if( Loading || VM_UID.isEmpty() )
		return;
	QSettings settings( QStringLiteral( "aqemu" ), QStringLiteral( "AQEMU" ) );
	settings.beginGroup( Settings_Group() );
	settings.setValue( QStringLiteral( "enabled" ), CH_Enable->isChecked() );
	settings.setValue( QStringLiteral( "host" ), Edit_Host->text().trimmed() );
	settings.setValue( QStringLiteral( "port" ), SB_Port->value() );
	settings.setValue( QStringLiteral( "token" ), Edit_Token->text() );
	settings.endGroup();
}

void Storage_Guest_Tab::Refresh()
{
	if( ! CH_Enable->isChecked() )
		return;
	Save_Form();
	Client->Set_Target( Edit_Host->text(), SB_Port->value(), Edit_Token->text() );
	Pools->setPlainText( tr( "Asking TrueNAS…" ) );
	Client->Fetch_Pools();
	Client->Fetch_Snapshots();
}

void Storage_Guest_Tab::Rollback()
{
	QListWidgetItem *item = Snapshots->currentItem();
	if( ! item )
	{
		Pools->setPlainText( tr( "Select a snapshot." ) );
		return;
	}
	const QString id = item->data( Qt::UserRole ).toString();
	const auto answer = QMessageBox::question( this, tr( "Roll back in TrueNAS" ),
		tr( "TrueNAS will roll %1 back inside the guest. AQEMU does not change the virtual disk file. This is not a QEMU snapshot." ).arg( id ),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
	if( answer != QMessageBox::Yes )
		return;
	Save_Form();
	Client->Set_Target( Edit_Host->text(), SB_Port->value(), Edit_Token->text() );
	Client->Rollback( id );
}
