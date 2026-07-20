/****************************************************************************
** SPICE guest display — spice-client-glib → QImage paint + input forward
****************************************************************************/
#include "Spice_View.h"

#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QShowEvent>
#include <QEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QApplication>
#include <cstring>

#include "Utils.h"

#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
// GLib headers use a member named "signals"; Qt #defines signals → Q_SIGNALS.
#pragma push_macro( "signals" )
#undef signals
#include <glib-object.h>
#include <spice-client.h>
#pragma pop_macro( "signals" )

namespace {

static void Quiet_GSpice_Log( const gchar *domain, GLogLevelFlags flags,
                              const gchar *message, gpointer )
{
	if( message && ( strstr( message, "gl-scanout" ) || strstr( message, "UsbDk" ) ) )
		return;
	g_log_default_handler( domain, flags, message, nullptr );
}

static void Install_Quiet_GSpice_Once()
{
	static bool done = false;
	if( done )
		return;
	done = true;
	g_log_set_handler( "GSpice",
	                   static_cast<GLogLevelFlags>( G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE ),
	                   Quiet_GSpice_Log, nullptr );
}

} // namespace
#endif

namespace {

#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
// PC XT (set 1) scancodes. Extended keys use 0x100 | code (spice-glib convention).
static unsigned xt_letter( int qt_key )
{
	static const unsigned map[] = {
		/* A-Z */ 0x1e,0x30,0x2e,0x20,0x12,0x21,0x22,0x23,0x17,0x24,
		0x25,0x26,0x32,0x31,0x18,0x19,0x10,0x13,0x1f,0x14,0x16,0x2f,0x11,0x2d,0x15,0x2c
	};
	if( qt_key >= Qt::Key_A && qt_key <= Qt::Key_Z )
		return map[ qt_key - Qt::Key_A ];
	return 0;
}
#endif

} // namespace

Spice_View::Spice_View( QWidget *parent )
	: Guest_Display_View( parent )
	, Status( new QLabel( this ) )
	, Glib_Timer( new QTimer( this ) )
	, Port( 0 )
	, Connected_Flag( false )
	, Connected_Emitted( false )
	, Primary_Width( 0 )
	, Primary_Height( 0 )
	, Primary_Stride( 0 )
	, Primary_Format( 0 )
	, Primary_Data( nullptr )
#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
	, Session( nullptr )
	, Display( nullptr )
	, Inputs( nullptr )
	, Button_Mask( 0 )
	, Display_Id( 0 )
#endif
{
	setFocusPolicy( Qt::StrongFocus );
	setMouseTracking( true );
	setAttribute( Qt::WA_OpaquePaintEvent );
	setMinimumSize( 160, 100 );

#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
	Install_Quiet_GSpice_Once();
#endif

	QVBoxLayout *layout = new QVBoxLayout( this );
	layout->setContentsMargins( 0, 0, 0, 0 );
	Status->setAlignment( Qt::AlignCenter );
	Status->setWordWrap( true );
	layout->addWidget( Status );

	if( Spice_Available() )
		Status->setText( tr( "SPICE client ready." ) );
	else
		Status->setText( tr(
			"SPICE client (spice-client-glib) was not compiled into this AQEMU build.\n"
			"Embedded session uses VNC fallback with QEMU -display none.\n"
			"cmake -DAQEMU_WITH_SPICE_GTK=ON (set PKG_CONFIG_PATH to spice-client-glib-2.0)." ) );

	Glib_Timer->setInterval( 10 );
	connect( Glib_Timer, SIGNAL(timeout()), this, SLOT(Pump_GLib()) );
}

Spice_View::~Spice_View()
{
	Disconnect();
}

bool Spice_View::Spice_GTK_Available() const
{
#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
	return true;
#else
	return false;
#endif
}

QString Spice_View::Backend_Name() const
{
	return Spice_Available() ? QStringLiteral( "spice" ) : QStringLiteral( "spice-unavailable" );
}

void Spice_View::Pump_GLib()
{
#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
	// Drain the default GLib context on the Qt GUI thread.
	while( g_main_context_pending( nullptr ) )
		g_main_context_iteration( nullptr, FALSE );
#endif
}

void Spice_View::Connect_To( const QString &host, int port )
{
	Host = host;
	Port = port;
	Disconnect();

#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
	Status->show();
	Status->setText( tr( "SPICE: connecting to %1:%2 …" ).arg( host ).arg( port ) );

	Session = spice_session_new();
	const QByteArray host_ba = host.toUtf8();
	const QByteArray port_ba = QByteArray::number( port );
	g_object_set( Session,
	              "host", host_ba.constData(),
	              "port", port_ba.constData(),
	              nullptr );

	g_signal_connect( Session, "channel-new",
	                  G_CALLBACK( Spice_View::On_Channel_New ), this );
	g_signal_connect( Session, "channel-destroy",
	                  G_CALLBACK( Spice_View::On_Channel_Destroy ), this );

	if( ! spice_session_connect( Session ) )
	{
		Status->setText( tr( "SPICE session connect failed." ) );
		Connected_Flag = false;
		g_clear_object( &Session );
		emit Connection_Error( tr( "spice_session_connect failed" ) );
		return;
	}

	Glib_Timer->start();
	AQDebug( "Spice_View::Connect_To()",
	         QString( "spice session connecting %1:%2" ).arg( host ).arg( port ) );
#else
	Status->setText( tr( "No spice-client-glib — session shell will use VNC on port %1." ).arg( port ) );
	Connected_Flag = false;
	emit Connection_Error( tr( "spice-client-glib not available" ) );
#endif
}

void Spice_View::Disconnect()
{
	Glib_Timer->stop();

#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
	if( Session )
	{
		g_signal_handlers_disconnect_by_data( Session, this );
		spice_session_disconnect( Session );
		// Pump a few times so disconnect completes without hanging the UI.
		for( int i = 0; i < 50; ++i )
		{
			if( ! g_main_context_pending( nullptr ) )
				break;
			g_main_context_iteration( nullptr, FALSE );
		}
		g_clear_object( &Session );
	}
	Display = nullptr;
	Inputs = nullptr;
	Primary_Data = nullptr;
	Button_Mask = 0;
#endif

	const bool was = Connected_Flag;
	Connected_Flag = false;
	Connected_Emitted = false;
	Primary_Width = Primary_Height = Primary_Stride = 0;
	Primary_Bytes.clear();
	Frame = QImage();
	Set_Local_Cursor_Hidden( false );

	if( was )
		emit Disconnected();
}

bool Spice_View::Is_Connected() const
{
	return Connected_Flag;
}

void Spice_View::Send_CAD()
{
#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
	if( ! Inputs )
		return;
	// Ctrl + Alt + Del (XT: 0x1d, 0x38, 0x153)
	spice_inputs_channel_key_press( Inputs, 0x1d );
	spice_inputs_channel_key_press( Inputs, 0x38 );
	spice_inputs_channel_key_press( Inputs, 0x153 );
	spice_inputs_channel_key_release( Inputs, 0x153 );
	spice_inputs_channel_key_release( Inputs, 0x38 );
	spice_inputs_channel_key_release( Inputs, 0x1d );
	AQDebug( "Spice_View::Send_CAD()", "sent Ctrl+Alt+Del" );
#else
	AQDebug( "Spice_View::Send_CAD()", Backend_Name() );
#endif
}

#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)

void Spice_View::On_Channel_New( SpiceSession *, SpiceChannel *channel, void *user )
{
	static_cast<Spice_View *>( user )->Attach_Channel( channel );
}

void Spice_View::On_Channel_Destroy( SpiceSession *, SpiceChannel *channel, void *user )
{
	static_cast<Spice_View *>( user )->Detach_Channel( channel );
}

void Spice_View::On_Channel_Event( SpiceChannel *channel, int event, void *user )
{
	static_cast<Spice_View *>( user )->Handle_Channel_Event( channel, event );
}

void Spice_View::On_Primary_Create( SpiceChannel *, int format, int width, int height,
                                    int stride, int, void *imgdata, void *user )
{
	static_cast<Spice_View *>( user )->Copy_Primary( format, width, height, stride, imgdata );
}

void Spice_View::On_Primary_Destroy( SpiceChannel *, void *user )
{
	Spice_View *self = static_cast<Spice_View *>( user );
	self->Primary_Data = nullptr;
	self->Primary_Width = self->Primary_Height = self->Primary_Stride = 0;
	self->Primary_Bytes.clear();
	self->Frame = QImage();
	self->update();
}

void Spice_View::On_Invalidate( SpiceChannel *, int x, int y, int w, int h, void *user )
{
	static_cast<Spice_View *>( user )->Copy_Invalidate( x, y, w, h );
}

void Spice_View::Attach_Channel( SpiceChannel *channel )
{
	g_signal_connect( channel, "channel-event",
	                  G_CALLBACK( Spice_View::On_Channel_Event ), this );

	if( SPICE_IS_DISPLAY_CHANNEL( channel ) )
	{
		Display = SPICE_DISPLAY_CHANNEL( channel );
		g_object_get( channel, "channel-id", &Display_Id, nullptr );
		g_signal_connect( channel, "display-primary-create",
		                  G_CALLBACK( Spice_View::On_Primary_Create ), this );
		g_signal_connect( channel, "display-primary-destroy",
		                  G_CALLBACK( Spice_View::On_Primary_Destroy ), this );
		g_signal_connect( channel, "display-invalidate",
		                  G_CALLBACK( Spice_View::On_Invalidate ), this );
		spice_channel_connect( channel );
	}
	else if( SPICE_IS_INPUTS_CHANNEL( channel ) )
	{
		Inputs = SPICE_INPUTS_CHANNEL( channel );
		spice_channel_connect( channel );
	}
	else
	{
		// Main / cursor / playback / etc. — open so the session completes.
		spice_channel_connect( channel );
	}
}

void Spice_View::Detach_Channel( SpiceChannel *channel )
{
	if( channel == SPICE_CHANNEL( Display ) )
		Display = nullptr;
	if( channel == SPICE_CHANNEL( Inputs ) )
		Inputs = nullptr;
}

void Spice_View::Handle_Channel_Event( SpiceChannel *channel, int event )
{
	Q_UNUSED( channel );
	switch( event )
	{
		case SPICE_CHANNEL_ERROR_CONNECT:
		case SPICE_CHANNEL_ERROR_TLS:
		case SPICE_CHANNEL_ERROR_LINK:
		case SPICE_CHANNEL_ERROR_AUTH:
		case SPICE_CHANNEL_ERROR_IO:
			Connected_Flag = false;
			Status->show();
			Status->setText( tr( "SPICE channel error (%1)" ).arg( event ) );
			emit Connection_Error( tr( "SPICE channel error %1" ).arg( event ) );
			break;
		default:
			break;
	}
}

void Spice_View::Emit_Connected_Once()
{
	if( Connected_Emitted )
		return;
	Connected_Emitted = true;
	Connected_Flag = true;
	Status->hide();
	// Guest (or SPICE cursor channel) draws the pointer — hide the host arrow
	// so we do not show two cursors stacked on tablet/absolute mice.
	Set_Local_Cursor_Hidden( true );
	AQDebug( "Spice_View", QString( "SPICE connected %1x%2" )
	         .arg( Primary_Width ).arg( Primary_Height ) );
	emit Connected();
}

void Spice_View::Copy_Primary( int format, int width, int height, int stride, void *imgdata )
{
	Primary_Format = format;
	Primary_Width = width;
	Primary_Height = height;
	Primary_Stride = stride;
	Primary_Data = static_cast<const unsigned char *>( imgdata );

	QImage::Format qfmt = QImage::Format_Invalid;
	int bpp = 4;
	switch( format )
	{
		case SPICE_SURFACE_FMT_32_xRGB:
		case SPICE_SURFACE_FMT_32_ARGB:
			// RGB32 matches LE xRGB byte order; keep opaque for clean scaling edges.
			qfmt = QImage::Format_RGB32;
			bpp = 4;
			break;
		case SPICE_SURFACE_FMT_16_565:
			qfmt = QImage::Format_RGB16;
			bpp = 2;
			break;
		default:
			AQWarning( "Spice_View::Copy_Primary()",
			           QString( "unsupported spice surface format %1" ).arg( format ) );
			return;
	}

	if( ! imgdata || width <= 0 || height <= 0 || stride == 0 )
		return;

	// Own a tightly packed copy. Do not memcpy stride*height when stride may be
	// negative (bottom-up) or padded — that caused edge speckles / corruption.
	Frame = QImage( width, height, qfmt );
	Copy_Rect_From_Primary( 0, 0, width, height );
	Q_UNUSED( bpp );

	Emit_Connected_Once();
	update();
}

void Spice_View::Copy_Rect_From_Primary( int x, int y, int w, int h )
{
	if( ! Primary_Data || Frame.isNull() || w <= 0 || h <= 0 )
		return;
	if( Frame.width() != Primary_Width || Frame.height() != Primary_Height )
		return;

	const int bpp = ( Primary_Format == SPICE_SURFACE_FMT_16_565 ) ? 2 : 4;
	const int copy_bytes = w * bpp;

	for( int row = 0; row < h; ++row )
	{
		// Pointer arithmetic handles negative stride (spice bottom-up surfaces).
		const unsigned char *src = Primary_Data + ( y + row ) * Primary_Stride + x * bpp;
		unsigned char *dst = Frame.scanLine( y + row ) + x * bpp;
		std::memcpy( dst, src, size_t( copy_bytes ) );
	}
}

void Spice_View::Copy_Invalidate( int x, int y, int w, int h )
{
	if( ! Primary_Data || Frame.isNull() || w <= 0 || h <= 0 )
		return;

	if( x < 0 ) { w += x; x = 0; }
	if( y < 0 ) { h += y; y = 0; }
	if( x + w > Primary_Width ) w = Primary_Width - x;
	if( y + h > Primary_Height ) h = Primary_Height - y;
	if( w <= 0 || h <= 0 )
		return;

	Copy_Rect_From_Primary( x, y, w, h );

	// Full widget update avoids partial-scale edge artifacts on dirty strips.
	update();
}

QRectF Spice_View::Scaled_Dest_Rect() const
{
	if( Primary_Width <= 0 || Primary_Height <= 0 || width() <= 0 || height() <= 0 )
		return QRectF();

	const qreal sx = qreal( width() ) / qreal( Primary_Width );
	const qreal sy = qreal( height() ) / qreal( Primary_Height );
	const qreal s = qMin( sx, sy );
	const qreal dw = Primary_Width * s;
	const qreal dh = Primary_Height * s;
	return QRectF( ( width() - dw ) * 0.5, ( height() - dh ) * 0.5, dw, dh );
}

QPoint Spice_View::Guest_From_Widget( const QPoint &widget_pos ) const
{
	const QRectF dest = Scaled_Dest_Rect();
	if( dest.isEmpty() )
		return QPoint( -1, -1 );

	const qreal gx = ( widget_pos.x() - dest.x() ) * Primary_Width / dest.width();
	const qreal gy = ( widget_pos.y() - dest.y() ) * Primary_Height / dest.height();
	const int ix = qBound( 0, int( gx ), Primary_Width - 1 );
	const int iy = qBound( 0, int( gy ), Primary_Height - 1 );
	if( gx < 0 || gy < 0 || gx >= Primary_Width || gy >= Primary_Height )
		return QPoint( -1, -1 );
	return QPoint( ix, iy );
}

int Spice_View::Qt_Button_To_Spice( Qt::MouseButton button )
{
	switch( button )
	{
		case Qt::LeftButton:   return SPICE_MOUSE_BUTTON_LEFT;
		case Qt::MiddleButton: return SPICE_MOUSE_BUTTON_MIDDLE;
		case Qt::RightButton:  return SPICE_MOUSE_BUTTON_RIGHT;
		default:               return SPICE_MOUSE_BUTTON_INVALID;
	}
}

int Spice_View::Qt_Buttons_To_Mask( Qt::MouseButtons buttons )
{
	int mask = 0;
	if( buttons & Qt::LeftButton )   mask |= SPICE_MOUSE_BUTTON_MASK_LEFT;
	if( buttons & Qt::MiddleButton ) mask |= SPICE_MOUSE_BUTTON_MASK_MIDDLE;
	if( buttons & Qt::RightButton )  mask |= SPICE_MOUSE_BUTTON_MASK_RIGHT;
	return mask;
}

void Spice_View::Send_Mouse_Buttons( int spice_button, bool press )
{
	if( ! Inputs || spice_button == SPICE_MOUSE_BUTTON_INVALID )
		return;
	if( press )
		spice_inputs_channel_button_press( Inputs, spice_button, Button_Mask );
	else
		spice_inputs_channel_button_release( Inputs, spice_button, Button_Mask );
}

unsigned Spice_View::Qt_Key_To_XT( int qt_key, quint32 native_scan )
{
	// Prefer OS scan code when it looks like set-1 (Windows / many USB HID stacks).
	if( native_scan > 0 && native_scan < 0x200 )
	{
		unsigned sc = native_scan & 0xff;
		// Qt on Windows may already OR 0x100 for extended; otherwise map common extended keys below.
		if( native_scan & 0x100 )
			return native_scan & 0x1ff;
		// Keep raw 8-bit for non-extended; override known extended Qt keys below.
		switch( qt_key )
		{
			case Qt::Key_Insert:
			case Qt::Key_Delete:
			case Qt::Key_Home:
			case Qt::Key_End:
			case Qt::Key_PageUp:
			case Qt::Key_PageDown:
			case Qt::Key_Left:
			case Qt::Key_Up:
			case Qt::Key_Right:
			case Qt::Key_Down:
			case Qt::Key_Meta:
			case Qt::Key_Super_L:
			case Qt::Key_Super_R:
			case Qt::Key_Menu:
			case Qt::Key_NumLock:
			case Qt::Key_Enter:
			case Qt::Key_Control: // when right ctrl
				break; // fall through to table for extended
			default:
				if( sc != 0 )
					return sc;
		}
	}

	if( unsigned letter = xt_letter( qt_key ) )
		return letter;

	if( qt_key >= Qt::Key_F1 && qt_key <= Qt::Key_F10 )
		return 0x3b + unsigned( qt_key - Qt::Key_F1 );
	if( qt_key == Qt::Key_F11 ) return 0x57;
	if( qt_key == Qt::Key_F12 ) return 0x58;

	if( qt_key >= Qt::Key_0 && qt_key <= Qt::Key_9 )
	{
		static const unsigned dig[] = { 0x0b,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a };
		return dig[ qt_key - Qt::Key_0 ];
	}

	switch( qt_key )
	{
		case Qt::Key_Escape:    return 0x01;
		case Qt::Key_Tab:       return 0x0f;
		case Qt::Key_Backtab:   return 0x0f;
		case Qt::Key_Backspace: return 0x0e;
		case Qt::Key_Return:    return 0x1c;
		case Qt::Key_Enter:     return 0x11c; // KP Enter
		case Qt::Key_Space:     return 0x39;
		case Qt::Key_Shift:     return 0x2a; // Left Shift (Right=0x36)
		case Qt::Key_Control:   return 0x1d;
		case Qt::Key_Alt:       return 0x38;
		case Qt::Key_AltGr:     return 0x138;
		case Qt::Key_Meta:
		case Qt::Key_Super_L:   return 0x15b;
		case Qt::Key_Super_R:   return 0x15c;
		case Qt::Key_Menu:      return 0x15d;
		case Qt::Key_CapsLock:  return 0x3a;
		case Qt::Key_NumLock:   return 0x145;
		case Qt::Key_ScrollLock:return 0x46;
		case Qt::Key_Pause:     return 0x45;
		case Qt::Key_Insert:    return 0x152;
		case Qt::Key_Delete:    return 0x153;
		case Qt::Key_Home:      return 0x147;
		case Qt::Key_End:       return 0x14f;
		case Qt::Key_PageUp:    return 0x149;
		case Qt::Key_PageDown:  return 0x151;
		case Qt::Key_Left:      return 0x14b;
		case Qt::Key_Up:        return 0x148;
		case Qt::Key_Right:     return 0x14d;
		case Qt::Key_Down:      return 0x150;
		case Qt::Key_Minus:     return 0x0c;
		case Qt::Key_Equal:     return 0x0d;
		case Qt::Key_BracketLeft:  return 0x1a;
		case Qt::Key_BracketRight: return 0x1b;
		case Qt::Key_Semicolon: return 0x27;
		case Qt::Key_Apostrophe:return 0x28;
		case Qt::Key_QuoteLeft: return 0x29;
		case Qt::Key_Backslash: return 0x2b;
		case Qt::Key_Comma:     return 0x33;
		case Qt::Key_Period:    return 0x34;
		case Qt::Key_Slash:     return 0x35;
		default:
			break;
	}

	if( native_scan > 0 )
		return unsigned( native_scan & 0x1ff );
	return 0;
}

void Spice_View::Send_Key( QKeyEvent *event, bool press )
{
	if( ! Inputs )
		return;
	const unsigned sc = Qt_Key_To_XT( event->key(), event->nativeScanCode() );
	if( sc == 0 )
		return;
	if( press )
		spice_inputs_channel_key_press( Inputs, sc );
	else
		spice_inputs_channel_key_release( Inputs, sc );
}

#endif // AQEMU_HAVE_SPICE_*

void Spice_View::Set_Local_Cursor_Hidden( bool hidden )
{
	setCursor( hidden ? Qt::BlankCursor : Qt::ArrowCursor );
}

void Spice_View::paintEvent( QPaintEvent *event )
{
	Q_UNUSED( event );
	QPainter p( this );
	p.fillRect( rect(), QColor( 20, 20, 20 ) );

	if( Frame.isNull() )
	{
		Guest_Display_View::paintEvent( event );
		return;
	}

	const QRectF dest = Scaled_Dest_Rect();
	// FastTransformation avoids bilinear fringe / rainbow speckles on scaled edges.
	p.setRenderHint( QPainter::SmoothPixmapTransform, false );
	p.drawImage( dest, Frame );
}

void Spice_View::resizeEvent( QResizeEvent *event )
{
	Guest_Display_View::resizeEvent( event );
	update();
}

void Spice_View::showEvent( QShowEvent *event )
{
	Guest_Display_View::showEvent( event );
	setFocus( Qt::OtherFocusReason );
}

void Spice_View::enterEvent( QEvent *event )
{
	Guest_Display_View::enterEvent( event );
	if( Connected_Flag )
		Set_Local_Cursor_Hidden( true );
}

void Spice_View::leaveEvent( QEvent *event )
{
	Guest_Display_View::leaveEvent( event );
	// Restore host cursor when leaving the guest pane (toolbar, etc.).
	Set_Local_Cursor_Hidden( false );
}

void Spice_View::mouseMoveEvent( QMouseEvent *event )
{
#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
	if( Inputs && Connected_Flag )
	{
		const QPoint g = Guest_From_Widget( event->pos() );
		if( g.x() >= 0 )
		{
			Button_Mask = Qt_Buttons_To_Mask( event->buttons() );
			spice_inputs_channel_position( Inputs, g.x(), g.y(), Display_Id, Button_Mask );
		}
	}
#endif
	Guest_Display_View::mouseMoveEvent( event );
}

void Spice_View::mousePressEvent( QMouseEvent *event )
{
	setFocus( Qt::MouseFocusReason );
#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
	if( Inputs && Connected_Flag )
	{
		const QPoint g = Guest_From_Widget( event->pos() );
		Button_Mask = Qt_Buttons_To_Mask( event->buttons() );
		if( g.x() >= 0 )
			spice_inputs_channel_position( Inputs, g.x(), g.y(), Display_Id, Button_Mask );
		Send_Mouse_Buttons( Qt_Button_To_Spice( event->button() ), true );
	}
#endif
	Guest_Display_View::mousePressEvent( event );
}

void Spice_View::mouseReleaseEvent( QMouseEvent *event )
{
#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
	if( Inputs && Connected_Flag )
	{
		Button_Mask = Qt_Buttons_To_Mask( event->buttons() );
		Send_Mouse_Buttons( Qt_Button_To_Spice( event->button() ), false );
		const QPoint g = Guest_From_Widget( event->pos() );
		if( g.x() >= 0 )
			spice_inputs_channel_position( Inputs, g.x(), g.y(), Display_Id, Button_Mask );
	}
#endif
	Guest_Display_View::mouseReleaseEvent( event );
}

void Spice_View::wheelEvent( QWheelEvent *event )
{
#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
	if( Inputs && Connected_Flag )
	{
		const QPoint angle = event->angleDelta();
		const int btn = ( angle.y() > 0 ) ? SPICE_MOUSE_BUTTON_UP : SPICE_MOUSE_BUTTON_DOWN;
		spice_inputs_channel_button_press( Inputs, btn, Button_Mask );
		spice_inputs_channel_button_release( Inputs, btn, Button_Mask );
		event->accept();
		return;
	}
#endif
	Guest_Display_View::wheelEvent( event );
}

void Spice_View::keyPressEvent( QKeyEvent *event )
{
#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
	if( Inputs && Connected_Flag && ! event->isAutoRepeat() )
	{
		Send_Key( event, true );
		event->accept();
		return;
	}
#endif
	Guest_Display_View::keyPressEvent( event );
}

void Spice_View::keyReleaseEvent( QKeyEvent *event )
{
#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
	if( Inputs && Connected_Flag && ! event->isAutoRepeat() )
	{
		Send_Key( event, false );
		event->accept();
		return;
	}
#endif
	Guest_Display_View::keyReleaseEvent( event );
}
