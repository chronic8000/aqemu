#include "Apple_SoC_Button_Pad.h"

#include "AQ_UI_Style.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QToolButton>
#include <QTimer>
#include <QDateTime>

Apple_SoC_Button_Pad::Apple_SoC_Button_Pad( QWidget *parent )
	: QWidget( parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint )
	, Btn_Home( nullptr )
	, Btn_Power( nullptr )
	, Btn_Vol_Down( nullptr )
	, Btn_Vol_Up( nullptr )
	, Btn_SOS( nullptr )
	, Home_Click_Timer( new QTimer( this ) )
	, Home_Click_Pending( false )
{
	setObjectName( QStringLiteral( "Apple_SoC_Button_Pad" ) );
	setWindowTitle( tr( "iOS buttons" ) );

	Home_Click_Timer->setSingleShot( true );
	Home_Click_Timer->setInterval( 280 );
	connect( Home_Click_Timer, &QTimer::timeout, this, [this]() {
		if( Home_Click_Pending )
		{
			Home_Click_Pending = false;
			emit Home_Clicked();
		}
	} );

	auto *lay = new QHBoxLayout( this );
	lay->setContentsMargins( AQ_Px( 8, this ), AQ_Px( 6, this ), AQ_Px( 8, this ), AQ_Px( 6, this ) );
	lay->setSpacing( AQ_Px( 6, this ) );

	auto make_btn = [this]( const QString &text, const QString &tip ) {
		auto *b = new QToolButton( this );
		b->setText( text );
		b->setToolTip( tip );
		b->setAutoRaise( false );
		b->setToolButtonStyle( Qt::ToolButtonTextOnly );
		b->setMinimumWidth( AQ_Px( 52, this ) );
		return b;
	};

	Btn_Vol_Down = make_btn( tr( "Vol−" ), tr( "Volume down (Inferno F3)" ) );
	Btn_Vol_Up = make_btn( tr( "Vol+" ), tr( "Volume up (Inferno F4)" ) );
	Btn_Home = make_btn( tr( "Home" ),
		tr( "Home / Menu (Inferno F6)\nClick once: Home screen\nDouble-click: App Switcher" ) );
	Btn_Power = make_btn( tr( "Side" ),
		tr( "Power / Side button (Inferno F5)\nClick: sleep/wake\nHold ~2s: power menu" ) );
	Btn_SOS = make_btn( tr( "SOS" ),
		tr( "SOS / slide-to-power-off combo\n(Vol up hold, then Side — ChefKiss guide)" ) );

	lay->addWidget( Btn_Vol_Down );
	lay->addWidget( Btn_Vol_Up );
	lay->addWidget( Btn_Home );
	lay->addWidget( Btn_Power );
	lay->addWidget( Btn_SOS );

	connect( Btn_Vol_Down, &QToolButton::clicked, this, &Apple_SoC_Button_Pad::Vol_Down );
	connect( Btn_Vol_Up, &QToolButton::clicked, this, &Apple_SoC_Button_Pad::Vol_Up );
	connect( Btn_SOS, &QToolButton::clicked, this, &Apple_SoC_Button_Pad::SOS_Triggered );
	connect( Btn_Home, &QToolButton::clicked, this, [this]() {
		Home_Click_Pending = true;
		Home_Click_Timer->start();
	} );

	Btn_Home->installEventFilter( this );
	Btn_Power->installEventFilter( this );

	setStyleSheet( QStringLiteral(
		"#Apple_SoC_Button_Pad {"
		"  background: rgba(36, 38, 44, 230);"
		"  border: 1px solid rgba(255,255,255,40);"
		"  border-radius: %1px;"
		"}"
		"QToolButton {"
		"  color: #eee;"
		"  background: rgba(55, 58, 66, 220);"
		"  border: 1px solid rgba(255,255,255,28);"
		"  border-radius: %2px;"
		"  padding: %3px %4px;"
		"  font-weight: 600;"
		"}"
		"QToolButton:hover { background: rgba(70, 74, 84, 240); }"
		"QToolButton:pressed { background: rgba(40, 42, 48, 255); }" )
		.arg( AQ_Px( 10, this ) )
		.arg( AQ_Px( 6, this ) )
		.arg( AQ_Px( 4, this ) )
		.arg( AQ_Px( 8, this ) ) );

	adjustSize();
}

bool Apple_SoC_Button_Pad::eventFilter( QObject *obj, QEvent *event )
{
	if( obj == Btn_Home && event->type() == QEvent::MouseButtonDblClick )
	{
		Home_Click_Pending = false;
		Home_Click_Timer->stop();
		emit Home_Double_Clicked();
		return true;
	}

	if( obj == Btn_Power )
	{
		static qint64 press_ms = 0;
		if( event->type() == QEvent::MouseButtonPress )
		{
			auto *me = static_cast<QMouseEvent *>( event );
			if( me->button() == Qt::LeftButton )
				press_ms = QDateTime::currentMSecsSinceEpoch();
		}
		else if( event->type() == QEvent::MouseButtonRelease )
		{
			auto *me = static_cast<QMouseEvent *>( event );
			if( me->button() == Qt::LeftButton )
			{
				const qint64 held = QDateTime::currentMSecsSinceEpoch() - press_ms;
				if( held >= 1800 )
					emit Power_Hold();
				else
					emit Power_Clicked();
				return true;
			}
		}
	}

	return QWidget::eventFilter( obj, event );
}
