/****************************************************************************
** QEMU help browser — probe -M / -cpu / -device / -netdev help like the CLI.
****************************************************************************/

#include "QEMU_Help_Browser.h"

#include <QRegExp>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QProcess>
#include <QFileInfo>
#include <QDialogButtonBox>

#include "Utils.h"
#include "System_Info.h"

QEMU_Help_Browser::QEMU_Help_Browser( QWidget *parent )
	: QDialog( parent )
{
	setWindowTitle( tr( "QEMU Help Browser (CLI probes)" ) );
	resize( 780, 560 );

	QVBoxLayout *root = new QVBoxLayout( this );

	QLabel *hint = new QLabel( tr(
		"Runs the same probes as the QEMU command line: "
		"<code>-M help</code>, <code>-cpu help</code>, <code>-device help</code>, "
		"<code>-netdev help</code>, <code>-display help</code>. "
		"Use this when building custom VMs or Additional Args." ), this );
	hint->setWordWrap( true );
	hint->setTextFormat( Qt::RichText );
	root->addWidget( hint );

	QFormLayout *form = new QFormLayout();
	CB_Binary = new QComboBox( this );
	CB_Binary->setMinimumContentsLength( 40 );
	form->addRow( tr( "Binary:" ), CB_Binary );

	CB_Topic = new QComboBox( this );
	CB_Topic->addItem( tr( "Machines (-M help)" ), QStringLiteral( "machine" ) );
	CB_Topic->addItem( tr( "CPUs (-cpu help)" ), QStringLiteral( "cpu" ) );
	CB_Topic->addItem( tr( "Devices (-device help)" ), QStringLiteral( "device" ) );
	CB_Topic->addItem( tr( "Netdev backends (-netdev help)" ), QStringLiteral( "netdev" ) );
	CB_Topic->addItem( tr( "Display backends (-display help)" ), QStringLiteral( "display" ) );
	CB_Topic->addItem( tr( "Audio backends (-audiodev help)" ), QStringLiteral( "audiodev" ) );
	CB_Topic->addItem( tr( "Full -help (truncated)" ), QStringLiteral( "help" ) );
	form->addRow( tr( "Topic:" ), CB_Topic );

	Edit_Filter = new QLineEdit( this );
	Edit_Filter->setPlaceholderText( tr( "Filter lines (e.g. virtio, raspi, usb-host)…" ) );
	form->addRow( tr( "Filter:" ), Edit_Filter );
	root->addLayout( form );

	QHBoxLayout *btns = new QHBoxLayout();
	QPushButton *probe = new QPushButton( tr( "Probe" ), this );
	btns->addWidget( probe );
	btns->addStretch();
	root->addLayout( btns );

	Edit_Output = new QPlainTextEdit( this );
	Edit_Output->setReadOnly( true );
	Edit_Output->setLineWrapMode( QPlainTextEdit::NoWrap );
	QFont mono = Edit_Output->font();
	mono.setFamily( QStringLiteral( "Consolas" ) );
	mono.setStyleHint( QFont::Monospace );
	Edit_Output->setFont( mono );
	root->addWidget( Edit_Output, 1 );

	QDialogButtonBox *box = new QDialogButtonBox( QDialogButtonBox::Close, this );
	connect( box, SIGNAL(rejected()), this, SLOT(reject()) );
	connect( box, SIGNAL(accepted()), this, SLOT(accept()) );
	root->addWidget( box );

	connect( probe, SIGNAL(clicked()), this, SLOT(On_Probe_Clicked()) );
	connect( Edit_Filter, SIGNAL(textChanged(QString)), this, SLOT(On_Filter_Changed(QString)) );
	connect( CB_Binary, SIGNAL(currentIndexChanged(int)), this, SLOT(On_Binary_Changed(int)) );

	Fill_Binaries();
}

void QEMU_Help_Browser::Fill_Binaries()
{
	CB_Binary->clear();
	const QList<Emulator> emuls = Get_Emulators_List();
	for( int e = 0; e < emuls.count(); ++e )
	{
		const QMap<QString, QString> bins = emuls[e].Get_Binary_Files();
		for( auto it = bins.constBegin(); it != bins.constEnd(); ++it )
		{
			const QString path = it.value();
			if( path.isEmpty() || ! QFileInfo( path ).exists() )
				continue;
			CB_Binary->addItem( QStringLiteral( "%1 — %2" ).arg( it.key(), path ), path );
		}
	}
	if( CB_Binary->count() == 0 )
		Edit_Output->setPlainText( tr( "No QEMU binaries found. Run the First Start Wizard." ) );
}

void QEMU_Help_Browser::On_Binary_Changed( int )
{
	// keep output until next probe
}

void QEMU_Help_Browser::On_Filter_Changed( const QString & )
{
	Apply_Filter();
}

void QEMU_Help_Browser::Apply_Filter()
{
	const QString f = Edit_Filter->text().trimmed();
	if( f.isEmpty() )
	{
		Edit_Output->setPlainText( Raw_Output );
		return;
	}
	QStringList out;
	const QStringList lines = Raw_Output.split( QRegExp( QStringLiteral( "[\\r\\n]+" ) ) );
	for( int i = 0; i < lines.count(); ++i )
	{
		if( lines[i].contains( f, Qt::CaseInsensitive ) )
			out << lines[i];
	}
	Edit_Output->setPlainText( out.join( QLatin1Char( '\n' ) ) );
}

void QEMU_Help_Browser::On_Probe_Clicked()
{
	const QString bin = CB_Binary->currentData().toString();
	if( bin.isEmpty() )
	{
		Edit_Output->setPlainText( tr( "Select a QEMU binary first." ) );
		return;
	}

	const QString topic = CB_Topic->currentData().toString();
	QStringList args;
	if( topic == QLatin1String( "machine" ) )
		args << QStringLiteral( "-M" ) << QStringLiteral( "help" );
	else if( topic == QLatin1String( "cpu" ) )
		args << QStringLiteral( "-cpu" ) << QStringLiteral( "help" );
	else if( topic == QLatin1String( "device" ) )
		args << QStringLiteral( "-device" ) << QStringLiteral( "help" );
	else if( topic == QLatin1String( "netdev" ) )
		args << QStringLiteral( "-netdev" ) << QStringLiteral( "help" );
	else if( topic == QLatin1String( "display" ) )
		args << QStringLiteral( "-display" ) << QStringLiteral( "help" );
	else if( topic == QLatin1String( "audiodev" ) )
		args << QStringLiteral( "-audiodev" ) << QStringLiteral( "help" );
	else
		args << QStringLiteral( "-help" );

	QProcess proc;
	proc.setProcessChannelMode( QProcess::MergedChannels );
	proc.start( bin, args );
	if( ! proc.waitForFinished( 20000 ) )
	{
		proc.kill();
		Edit_Output->setPlainText( tr( "Probe timed out." ) );
		return;
	}

	Raw_Output = QString::fromLocal8Bit( proc.readAllStandardOutput() );
	if( Raw_Output.trimmed().isEmpty() )
		Raw_Output = tr( "(empty output — this QEMU build may not support that help topic)" );
	Apply_Filter();
}
