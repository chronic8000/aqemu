/****************************************************************************
** Live migration progress / cancel
****************************************************************************/
#include "Migrate_Progress_Dialog.h"
#include "QMP_Client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>
#include <QtGlobal>

Migrate_Progress_Dialog::Migrate_Progress_Dialog( QMP_Client *qmp, const QString &uri,
                                                  QWidget *parent )
	: QDialog( parent )
	, QMP( qmp )
	, URI( uri )
	, Finished( false )
{
	setWindowTitle( tr( "Migrating VM" ) );
	setWindowModality( Qt::ApplicationModal );
	resize( 420, 160 );
	setMinimumWidth( 380 );

	QVBoxLayout *lay = new QVBoxLayout( this );
	Status_Label = new QLabel( tr( "Starting migration to %1…" ).arg( uri ), this );
	Status_Label->setWordWrap( true );
	lay->addWidget( Status_Label );

	Bar = new QProgressBar( this );
	Bar->setRange( 0, 100 );
	Bar->setValue( 0 );
	lay->addWidget( Bar );

	Detail_Label = new QLabel( tr( "Waiting for QEMU…" ), this );
	Detail_Label->setWordWrap( true );
	lay->addWidget( Detail_Label );

	QHBoxLayout *btns = new QHBoxLayout();
	btns->addStretch();
	Cancel_Btn = new QPushButton( tr( "Cancel migration" ), this );
	btns->addWidget( Cancel_Btn );
	lay->addLayout( btns );

	Poll_Timer = new QTimer( this );
	Poll_Timer->setInterval( 500 );
	connect( Poll_Timer, SIGNAL(timeout()), this, SLOT(On_Poll()) );
	connect( Cancel_Btn, SIGNAL(clicked()), this, SLOT(On_Cancel()) );
	if( QMP )
		connect( QMP, SIGNAL(Reply_Received(QJsonObject)), this, SLOT(On_Reply(QJsonObject)) );

	if( ! QMP || ! QMP->Is_Connected() )
	{
		Status_Label->setText( tr( "QMP is not connected." ) );
		Cancel_Btn->setText( tr( "Close" ) );
		Finished = true;
		return;
	}

	if( ! QMP->Migrate( uri ) )
	{
		Status_Label->setText( tr( "Failed to send migrate command." ) );
		Cancel_Btn->setText( tr( "Close" ) );
		Finished = true;
		return;
	}

	Poll_Timer->start();
	On_Poll();
}

void Migrate_Progress_Dialog::On_Poll()
{
	if( Finished || ! QMP || ! QMP->Is_Connected() )
		return;
	QMP->Query_Migrate();
}

void Migrate_Progress_Dialog::On_Cancel()
{
	if( Finished )
	{
		reject();
		return;
	}
	if( QMP && QMP->Is_Connected() )
		QMP->Migrate_Cancel();
	Status_Label->setText( tr( "Cancelling migration…" ) );
	Cancel_Btn->setEnabled( false );
	QTimer::singleShot( 800, this, SLOT(reject()) );
}

void Migrate_Progress_Dialog::On_Reply( const QJsonObject &reply )
{
	if( Finished )
		return;

	// Only care about query-migrate returns (object with status)
	const QJsonValue ret = reply.value( QStringLiteral( "return" ) );
	if( ! ret.isObject() )
		return;
	const QJsonObject obj = ret.toObject();
	if( ! obj.contains( QStringLiteral( "status" ) ) )
		return;

	const QString status = obj.value( QStringLiteral( "status" ) ).toString();
	Status_Label->setText( tr( "Status: %1 → %2" ).arg( status, URI ) );

	double pct = 0.0;
	const QJsonObject ram = obj.value( QStringLiteral( "ram" ) ).toObject();
	if( ! ram.isEmpty() )
	{
		const double total = ram.value( QStringLiteral( "total" ) ).toDouble();
		const double remaining = ram.value( QStringLiteral( "remaining" ) ).toDouble();
		const double transferred = ram.value( QStringLiteral( "transferred" ) ).toDouble();
		if( total > 0.0 )
			pct = qBound( 0.0, 100.0 * ( 1.0 - remaining / total ), 100.0 );
		Detail_Label->setText(
			tr( "RAM transferred: %1 / remaining: %2 (total %3)" )
				.arg( transferred, 0, 'f', 0 )
				.arg( remaining, 0, 'f', 0 )
				.arg( total, 0, 'f', 0 ) );
	}
	else
	{
		Detail_Label->setText( tr( "Migration in progress…" ) );
	}
	Bar->setValue( static_cast<int>( pct ) );

	if( status == QLatin1String( "completed" ) )
	{
		Finished = true;
		Poll_Timer->stop();
		Bar->setValue( 100 );
		Status_Label->setText( tr( "Migration completed." ) );
		Cancel_Btn->setText( tr( "Close" ) );
		Cancel_Btn->setEnabled( true );
	}
	else if( status == QLatin1String( "failed" ) ||
	         status == QLatin1String( "cancelled" ) )
	{
		Finished = true;
		Poll_Timer->stop();
		Status_Label->setText( tr( "Migration %1." ).arg( status ) );
		Cancel_Btn->setText( tr( "Close" ) );
		Cancel_Btn->setEnabled( true );
	}
}
