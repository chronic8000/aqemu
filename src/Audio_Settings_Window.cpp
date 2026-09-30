/****************************************************************************
** This file is part of AQEMU.
****************************************************************************/

#include "Audio_Settings_Window.h"
#include "Audio_Host.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

static QSpinBox *AQ_Opt_Spin( int min, int max, QWidget *parent )
{
	QSpinBox *s = new QSpinBox( parent );
	s->setRange( min, max );
	s->setSpecialValueText( QObject::tr( "default" ) );
	s->setValue( min );
	return s;
}

static void AQ_Fill_Tri( QComboBox *box )
{
	box->addItem( QObject::tr( "default" ), QString() );
	box->addItem( QObject::tr( "on" ), QStringLiteral( "on" ) );
	box->addItem( QObject::tr( "off" ), QStringLiteral( "off" ) );
}

static void AQ_Set_Tri( QComboBox *box, const QString &value )
{
	const int ix = box->findData( value.trimmed().toLower() );
	box->setCurrentIndex( ix < 0 ? 0 : ix );
}

static QString AQ_Tri( const QComboBox *box )
{
	return box->currentData().toString();
}

static void AQ_Fill_Formats( QComboBox *box )
{
	box->addItem( QObject::tr( "default" ), QString() );
	const char *fmts[] = { "s16", "s32", "f32", "s8", "u8", "u16", "u32", 0 };
	for( int i = 0; fmts[i]; ++i )
		box->addItem( QString::fromLatin1( fmts[i] ), QString::fromLatin1( fmts[i] ) );
}

Audio_Settings_Window::Audio_Settings_Window( bool host_defaults, QWidget *parent )
	: QDialog( parent ), Host_Mode( host_defaults )
{
	setWindowTitle( host_defaults ? tr( "Audio" ) : tr( "Audio for this VM" ) );
	resize( 560, 640 );

	QVBoxLayout *outer = new QVBoxLayout( this );
	Use_Defaults = new QCheckBox( tr( "Use File → Audio defaults" ), this );
	Use_Defaults->setVisible( ! host_defaults );
	outer->addWidget( Use_Defaults );

	QScrollArea *scroll = new QScrollArea( this );
	scroll->setWidgetResizable( true );
	QWidget *body = new QWidget( scroll );
	QFormLayout *form = new QFormLayout( body );

	Backend = new QComboBox( body );
	const QStringList names = AQ_Audio_Backend_Names( QString() );
	for( int i = 0; i < names.count(); ++i )
		Backend->addItem( names.at( i ) );
	form->addRow( tr( "Method" ), Backend );

	Out_Device = new QComboBox( body );
	Out_Device->setEditable( true );
	In_Device = new QComboBox( body );
	In_Device->setEditable( true );
	QPushButton *refresh = new QPushButton( tr( "Refresh devices" ), body );
	form->addRow( tr( "Output" ), Out_Device );
	form->addRow( tr( "Input" ), In_Device );
	form->addRow( QString(), refresh );

	Out_Voices = AQ_Opt_Spin( -1, 32, body );
	In_Voices = AQ_Opt_Spin( -1, 32, body );
	In_Voices->setValue( 0 );
	Timer_Period = AQ_Opt_Spin( -1, 1000000, body );
	form->addRow( tr( "Output voices" ), Out_Voices );
	form->addRow( tr( "Input voices" ), In_Voices );
	form->addRow( tr( "Timer period (µs)" ), Timer_Period );

	Out_Frequency = AQ_Opt_Spin( -1, 384000, body );
	In_Frequency = AQ_Opt_Spin( -1, 384000, body );
	Out_Channels = AQ_Opt_Spin( -1, 8, body );
	In_Channels = AQ_Opt_Spin( -1, 8, body );
	Out_Format = new QComboBox( body );
	In_Format = new QComboBox( body );
	AQ_Fill_Formats( Out_Format );
	AQ_Fill_Formats( In_Format );
	Out_Buffer = AQ_Opt_Spin( -1, 5000000, body );
	In_Buffer = AQ_Opt_Spin( -1, 5000000, body );
	Out_Mix = new QComboBox( body );
	In_Mix = new QComboBox( body );
	Out_Fixed = new QComboBox( body );
	In_Fixed = new QComboBox( body );
	AQ_Fill_Tri( Out_Mix );
	AQ_Fill_Tri( In_Mix );
	AQ_Fill_Tri( Out_Fixed );
	AQ_Fill_Tri( In_Fixed );
	form->addRow( tr( "Output rate (Hz)" ), Out_Frequency );
	form->addRow( tr( "Input rate (Hz)" ), In_Frequency );
	form->addRow( tr( "Output channels" ), Out_Channels );
	form->addRow( tr( "Input channels" ), In_Channels );
	form->addRow( tr( "Output format" ), Out_Format );
	form->addRow( tr( "Input format" ), In_Format );
	form->addRow( tr( "Output buffer (µs)" ), Out_Buffer );
	form->addRow( tr( "Input buffer (µs)" ), In_Buffer );
	form->addRow( tr( "Output mixing engine" ), Out_Mix );
	form->addRow( tr( "Input mixing engine" ), In_Mix );
	form->addRow( tr( "Output fixed settings" ), Out_Fixed );
	form->addRow( tr( "Input fixed settings" ), In_Fixed );

	Detail = new QWidget( body );
	QFormLayout *detail = new QFormLayout( Detail );
	detail->setContentsMargins( 0, 0, 0, 0 );
	Latency = AQ_Opt_Spin( -1, 5000000, Detail );
	Out_Buffer_Count = AQ_Opt_Spin( -1, 64, Detail );
	In_Buffer_Count = AQ_Opt_Spin( -1, 64, Detail );
	Server = new QLineEdit( Detail );
	Out_Stream = new QLineEdit( Detail );
	In_Stream = new QLineEdit( Detail );
	Out_Latency = AQ_Opt_Spin( -1, 5000000, Detail );
	In_Latency = AQ_Opt_Spin( -1, 5000000, Detail );
	Out_Period = AQ_Opt_Spin( -1, 5000000, Detail );
	In_Period = AQ_Opt_Spin( -1, 5000000, Detail );
	Out_Poll = new QComboBox( Detail );
	In_Poll = new QComboBox( Detail );
	AQ_Fill_Tri( Out_Poll );
	AQ_Fill_Tri( In_Poll );
	Threshold = AQ_Opt_Spin( -1, 5000000, Detail );
	Wav_Path = new QLineEdit( Detail );
	Nsamp = AQ_Opt_Spin( -1, 100000, Detail );
	Jack_Server = new QLineEdit( Detail );
	Jack_Client = new QLineEdit( Detail );
	detail->addRow( tr( "Latency (µs)" ), Latency );
	detail->addRow( tr( "Output buffer count" ), Out_Buffer_Count );
	detail->addRow( tr( "Input buffer count" ), In_Buffer_Count );
	detail->addRow( tr( "Server" ), Server );
	detail->addRow( tr( "Output stream name" ), Out_Stream );
	detail->addRow( tr( "Input stream name" ), In_Stream );
	detail->addRow( tr( "Output latency (µs)" ), Out_Latency );
	detail->addRow( tr( "Input latency (µs)" ), In_Latency );
	detail->addRow( tr( "Output period (µs)" ), Out_Period );
	detail->addRow( tr( "Input period (µs)" ), In_Period );
	detail->addRow( tr( "Output try-poll" ), Out_Poll );
	detail->addRow( tr( "Input try-poll" ), In_Poll );
	detail->addRow( tr( "ALSA threshold (µs)" ), Threshold );
	detail->addRow( tr( "WAV path" ), Wav_Path );
	detail->addRow( tr( "D-Bus samples" ), Nsamp );
	detail->addRow( tr( "JACK server" ), Jack_Server );
	detail->addRow( tr( "JACK client" ), Jack_Client );
	form->addRow( Detail );

	Virt_Box = new QWidget( body );
	QFormLayout *virt = new QFormLayout( Virt_Box );
	virt->setContentsMargins( 0, 0, 0, 0 );
	Virt_Jacks = AQ_Opt_Spin( -1, 8, Virt_Box );
	Virt_Streams = AQ_Opt_Spin( -1, 8, Virt_Box );
	Virt_Chmaps = AQ_Opt_Spin( -1, 8, Virt_Box );
	virt->addRow( tr( "VirtIO jacks" ), Virt_Jacks );
	virt->addRow( tr( "VirtIO streams" ), Virt_Streams );
	virt->addRow( tr( "VirtIO channel maps" ), Virt_Chmaps );
	Virt_Box->setVisible( ! host_defaults );
	form->addRow( Virt_Box );

	scroll->setWidget( body );
	outer->addWidget( scroll );
	QDialogButtonBox *buttons = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this );
	outer->addWidget( buttons );

	connect( Backend, SIGNAL(currentIndexChanged(int)), this, SLOT(Apply_Backend_Page()) );
	connect( refresh, SIGNAL(clicked()), this, SLOT(Refresh_Devices()) );
	connect( buttons, SIGNAL(accepted()), this, SLOT(Save_And_Accept()) );
	connect( buttons, SIGNAL(rejected()), this, SLOT(reject()) );
	connect( Use_Defaults, SIGNAL(toggled(bool)), body, SLOT(setDisabled(bool)) );

	if( host_defaults )
		Load_Profile( true );
	Apply_Backend_Page();
}

void Audio_Settings_Window::Set_VM( bool use_host_defaults, const QString &options_blob,
	int jacks, int streams, int chmaps, const QString &qemu_binary )
{
	Qemu_Binary = qemu_binary;
	const QStringList names = AQ_Audio_Backend_Names( qemu_binary );
	Backend->clear();
	for( int i = 0; i < names.count(); ++i )
		Backend->addItem( names.at( i ) );
	Use_Defaults->setChecked( use_host_defaults );
	if( use_host_defaults )
		Load_Profile( true );
	else
	{
		AQ_Audio_Profile p;
		p.opt = AQ_Audio_Options_From_String( options_blob );
		p.backend = p.option( QStringLiteral( "backend" ) );
		if( ! p.backend.isEmpty() )
		{
			const int ix = Backend->findText( p.backend );
			if( ix >= 0 )
				Backend->setCurrentIndex( ix );
			else
			{
				Backend->addItem( p.backend );
				Backend->setCurrentIndex( Backend->count() - 1 );
			}
		}
		const QString b = Backend->currentText();
		Fill_Combo( Out_Device, b, false, p.option( AQ_Audio_Device_Key( b, false ) ) );
		Fill_Combo( In_Device, b, true, p.option( AQ_Audio_Device_Key( b, true ) ) );
		Out_Voices->setValue( p.option( QStringLiteral( "out.voices" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "out.voices" ) ).toInt() );
		In_Voices->setValue( p.option( QStringLiteral( "in.voices" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "in.voices" ) ).toInt() );
		Timer_Period->setValue( p.option( QStringLiteral( "timer-period" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "timer-period" ) ).toInt() );
		Out_Frequency->setValue( p.option( QStringLiteral( "out.frequency" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "out.frequency" ) ).toInt() );
		In_Frequency->setValue( p.option( QStringLiteral( "in.frequency" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "in.frequency" ) ).toInt() );
		Out_Channels->setValue( p.option( QStringLiteral( "out.channels" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "out.channels" ) ).toInt() );
		In_Channels->setValue( p.option( QStringLiteral( "in.channels" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "in.channels" ) ).toInt() );
		const int of = Out_Format->findData( p.option( QStringLiteral( "out.format" ) ) );
		Out_Format->setCurrentIndex( of < 0 ? 0 : of );
		const int inf = In_Format->findData( p.option( QStringLiteral( "in.format" ) ) );
		In_Format->setCurrentIndex( inf < 0 ? 0 : inf );
		Out_Buffer->setValue( p.option( QStringLiteral( "out.buffer-length" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "out.buffer-length" ) ).toInt() );
		In_Buffer->setValue( p.option( QStringLiteral( "in.buffer-length" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "in.buffer-length" ) ).toInt() );
		AQ_Set_Tri( Out_Mix, p.option( QStringLiteral( "out.mixing-engine" ) ) );
		AQ_Set_Tri( In_Mix, p.option( QStringLiteral( "in.mixing-engine" ) ) );
		AQ_Set_Tri( Out_Fixed, p.option( QStringLiteral( "out.fixed-settings" ) ) );
		AQ_Set_Tri( In_Fixed, p.option( QStringLiteral( "in.fixed-settings" ) ) );
		Latency->setValue( p.option( QStringLiteral( "latency" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "latency" ) ).toInt() );
		Out_Buffer_Count->setValue( p.option( QStringLiteral( "out.buffer-count" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "out.buffer-count" ) ).toInt() );
		In_Buffer_Count->setValue( p.option( QStringLiteral( "in.buffer-count" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "in.buffer-count" ) ).toInt() );
		Server->setText( p.option( QStringLiteral( "server" ) ) );
		Out_Stream->setText( p.option( QStringLiteral( "out.stream-name" ) ) );
		In_Stream->setText( p.option( QStringLiteral( "in.stream-name" ) ) );
		Out_Latency->setValue( p.option( QStringLiteral( "out.latency" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "out.latency" ) ).toInt() );
		In_Latency->setValue( p.option( QStringLiteral( "in.latency" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "in.latency" ) ).toInt() );
		Out_Period->setValue( p.option( QStringLiteral( "out.period-length" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "out.period-length" ) ).toInt() );
		In_Period->setValue( p.option( QStringLiteral( "in.period-length" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "in.period-length" ) ).toInt() );
		AQ_Set_Tri( Out_Poll, p.option( QStringLiteral( "out.try-poll" ) ) );
		AQ_Set_Tri( In_Poll, p.option( QStringLiteral( "in.try-poll" ) ) );
		Threshold->setValue( p.option( QStringLiteral( "threshold" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "threshold" ) ).toInt() );
		Wav_Path->setText( p.option( QStringLiteral( "path" ) ) );
		Nsamp->setValue( p.option( QStringLiteral( "nsamples" ) ).isEmpty() ? -1 : p.option( QStringLiteral( "nsamples" ) ).toInt() );
		Jack_Server->setText( p.option( QStringLiteral( "out.server-name" ) ) );
		Jack_Client->setText( p.option( QStringLiteral( "out.client-name" ) ) );
	}
	Virt_Jacks->setValue( jacks );
	Virt_Streams->setValue( streams );
	Virt_Chmaps->setValue( chmaps );
	Apply_Backend_Page();
}

void Audio_Settings_Window::Load_Profile( bool from_host )
{
	Q_UNUSED( from_host );
	const AQ_Audio_Profile p = AQ_Audio_Host_Load();
	const int ix = Backend->findText( p.backend );
	if( ix >= 0 )
		Backend->setCurrentIndex( ix );
	else if( ! p.backend.isEmpty() )
	{
		Backend->addItem( p.backend );
		Backend->setCurrentIndex( Backend->count() - 1 );
	}
	Set_VM( false, AQ_Audio_Options_To_String( p.opt ), -1, -1, -1, Qemu_Binary );
	if( ! p.backend.isEmpty() )
	{
		const int bix = Backend->findText( p.backend );
		if( bix >= 0 )
			Backend->setCurrentIndex( bix );
	}
	Use_Defaults->setChecked( false );
}

void Audio_Settings_Window::Fill_Combo( QComboBox *box, const QString &backend, bool capture, const QString &current )
{
	const QString keep = current;
	box->clear();
	const QList<QPair<QString, QString> > devs = AQ_Audio_List_Devices( backend, capture );
	int select = 0;
	for( int i = 0; i < devs.count(); ++i )
	{
		box->addItem( devs.at( i ).first, devs.at( i ).second );
		if( ! keep.isEmpty() && devs.at( i ).second == keep )
			select = i;
	}
	if( ! keep.isEmpty() && box->findData( keep ) < 0 )
	{
		box->addItem( keep, keep );
		select = box->count() - 1;
	}
	box->setCurrentIndex( select );
	const bool gated = ( backend == QLatin1String( "dsound" ) || backend == QLatin1String( "sdl" ) );
	const bool allow = ! gated || AQ_Audio_Backend_Has_Dev( Qemu_Binary, backend );
	box->setEnabled( allow );
	if( ! allow )
		box->setToolTip( tr( "This QEMU build has no device picker for %1. System default is used." ).arg( backend ) );
	else
		box->setToolTip( QString() );
}

void Audio_Settings_Window::Apply_Backend_Page()
{
	const QString b = Backend->currentText().trimmed().toLower();
	const bool ds = ( b == QLatin1String( "dsound" ) );
	const bool sdl = ( b == QLatin1String( "sdl" ) );
	const bool pa = ( b == QLatin1String( "pa" ) || b == QLatin1String( "pipewire" ) );
	const bool alsa = ( b == QLatin1String( "alsa" ) || b == QLatin1String( "oss" ) );
	const bool jack = ( b == QLatin1String( "jack" ) || b == QLatin1String( "sndio" ) );
	Latency->setVisible( ds || jack );
	Out_Buffer_Count->setVisible( sdl || alsa || b == QLatin1String( "coreaudio" ) );
	In_Buffer_Count->setVisible( sdl || alsa || b == QLatin1String( "coreaudio" ) );
	Server->setVisible( pa );
	Out_Stream->setVisible( pa );
	In_Stream->setVisible( pa );
	Out_Latency->setVisible( pa || jack );
	In_Latency->setVisible( pa || jack );
	Out_Period->setVisible( b == QLatin1String( "alsa" ) );
	In_Period->setVisible( b == QLatin1String( "alsa" ) );
	Out_Poll->setVisible( alsa );
	In_Poll->setVisible( alsa );
	Threshold->setVisible( b == QLatin1String( "alsa" ) );
	Wav_Path->setVisible( b == QLatin1String( "wav" ) );
	Nsamp->setVisible( b == QLatin1String( "dbus" ) );
	Jack_Server->setVisible( jack );
	Jack_Client->setVisible( b == QLatin1String( "jack" ) );
	Refresh_Devices();
}

void Audio_Settings_Window::Refresh_Devices()
{
	const QString b = Backend->currentText();
	const QString out_cur = Out_Device->currentData().toString();
	const QString in_cur = In_Device->currentData().toString();
	Fill_Combo( Out_Device, b, false, out_cur.isEmpty() ? Out_Device->currentText() : out_cur );
	Fill_Combo( In_Device, b, true, in_cur.isEmpty() ? In_Device->currentText() : in_cur );
}

static void AQ_Put_Spin( QMap<QString, QString> *opt, const QString &key, const QSpinBox *box )
{
	if( box->value() < 0 )
		opt->remove( key );
	else
		opt->insert( key, QString::number( box->value() ) );
}

static void AQ_Put_Text( QMap<QString, QString> *opt, const QString &key, const QString &text )
{
	const QString t = text.trimmed();
	if( t.isEmpty() )
		opt->remove( key );
	else
		opt->insert( key, t );
}

void Audio_Settings_Window::Save_And_Accept()
{
	if( ! Host_Mode && Use_Defaults->isChecked() )
	{
		accept();
		return;
	}
	QMap<QString, QString> opt;
	const QString b = Backend->currentText().trimmed().toLower();
	AQ_Put_Text( &opt, AQ_Audio_Device_Key( b, false ), Out_Device->currentData().toString().isEmpty()
		? Out_Device->currentText().trimmed() : Out_Device->currentData().toString() );
	AQ_Put_Text( &opt, AQ_Audio_Device_Key( b, true ), In_Device->currentData().toString().isEmpty()
		? In_Device->currentText().trimmed() : In_Device->currentData().toString() );
	if( opt.value( AQ_Audio_Device_Key( b, false ) ) == QLatin1String( "System default" ) )
		opt.remove( AQ_Audio_Device_Key( b, false ) );
	if( opt.value( AQ_Audio_Device_Key( b, true ) ) == QLatin1String( "System default" ) )
		opt.remove( AQ_Audio_Device_Key( b, true ) );
	AQ_Put_Spin( &opt, QStringLiteral( "out.voices" ), Out_Voices );
	AQ_Put_Spin( &opt, QStringLiteral( "in.voices" ), In_Voices );
	AQ_Put_Spin( &opt, QStringLiteral( "timer-period" ), Timer_Period );
	AQ_Put_Spin( &opt, QStringLiteral( "out.frequency" ), Out_Frequency );
	AQ_Put_Spin( &opt, QStringLiteral( "in.frequency" ), In_Frequency );
	AQ_Put_Spin( &opt, QStringLiteral( "out.channels" ), Out_Channels );
	AQ_Put_Spin( &opt, QStringLiteral( "in.channels" ), In_Channels );
	AQ_Put_Text( &opt, QStringLiteral( "out.format" ), Out_Format->currentData().toString() );
	AQ_Put_Text( &opt, QStringLiteral( "in.format" ), In_Format->currentData().toString() );
	AQ_Put_Spin( &opt, QStringLiteral( "out.buffer-length" ), Out_Buffer );
	AQ_Put_Spin( &opt, QStringLiteral( "in.buffer-length" ), In_Buffer );
	AQ_Put_Text( &opt, QStringLiteral( "out.mixing-engine" ), AQ_Tri( Out_Mix ) );
	AQ_Put_Text( &opt, QStringLiteral( "in.mixing-engine" ), AQ_Tri( In_Mix ) );
	AQ_Put_Text( &opt, QStringLiteral( "out.fixed-settings" ), AQ_Tri( Out_Fixed ) );
	AQ_Put_Text( &opt, QStringLiteral( "in.fixed-settings" ), AQ_Tri( In_Fixed ) );
	AQ_Put_Spin( &opt, QStringLiteral( "latency" ), Latency );
	AQ_Put_Spin( &opt, QStringLiteral( "out.buffer-count" ), Out_Buffer_Count );
	AQ_Put_Spin( &opt, QStringLiteral( "in.buffer-count" ), In_Buffer_Count );
	AQ_Put_Text( &opt, QStringLiteral( "server" ), Server->text() );
	AQ_Put_Text( &opt, QStringLiteral( "out.stream-name" ), Out_Stream->text() );
	AQ_Put_Text( &opt, QStringLiteral( "in.stream-name" ), In_Stream->text() );
	AQ_Put_Spin( &opt, QStringLiteral( "out.latency" ), Out_Latency );
	AQ_Put_Spin( &opt, QStringLiteral( "in.latency" ), In_Latency );
	AQ_Put_Spin( &opt, QStringLiteral( "out.period-length" ), Out_Period );
	AQ_Put_Spin( &opt, QStringLiteral( "in.period-length" ), In_Period );
	AQ_Put_Text( &opt, QStringLiteral( "out.try-poll" ), AQ_Tri( Out_Poll ) );
	AQ_Put_Text( &opt, QStringLiteral( "in.try-poll" ), AQ_Tri( In_Poll ) );
	AQ_Put_Spin( &opt, QStringLiteral( "threshold" ), Threshold );
	AQ_Put_Text( &opt, QStringLiteral( "path" ), Wav_Path->text() );
	AQ_Put_Spin( &opt, QStringLiteral( "nsamples" ), Nsamp );
	AQ_Put_Text( &opt, QStringLiteral( "out.server-name" ), Jack_Server->text() );
	AQ_Put_Text( &opt, QStringLiteral( "in.server-name" ), Jack_Server->text() );
	AQ_Put_Text( &opt, QStringLiteral( "out.client-name" ), Jack_Client->text() );
	AQ_Put_Text( &opt, QStringLiteral( "in.client-name" ), Jack_Client->text() );
	opt.insert( QStringLiteral( "backend" ), b );

	if( Host_Mode )
	{
		AQ_Audio_Profile p;
		p.backend = b;
		p.opt = opt;
		p.opt.remove( QStringLiteral( "backend" ) );
		AQ_Audio_Host_Save( p );
	}
	else
	{
		Saved_Blob = AQ_Audio_Options_To_String( opt );
	}
	accept();
}

bool Audio_Settings_Window::Use_Host_Defaults() const
{
	return ! Host_Mode && Use_Defaults->isChecked();
}

QString Audio_Settings_Window::Options_Blob() const
{
	return Saved_Blob;
}

int Audio_Settings_Window::VirtIO_Jacks() const { return Virt_Jacks->value(); }
int Audio_Settings_Window::VirtIO_Streams() const { return Virt_Streams->value(); }
int Audio_Settings_Window::VirtIO_Chmaps() const { return Virt_Chmaps->value(); }
