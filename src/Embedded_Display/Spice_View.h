/****************************************************************************
** SPICE guest display — spice-client-glib → QImage paint + input forward
****************************************************************************/
#ifndef SPICE_VIEW_H
#define SPICE_VIEW_H

#include "Guest_Display_View.h"

#include <QImage>
#include <QPoint>
#include <QRectF>

class QLabel;
class QTimer;
class QPaintEvent;
class QResizeEvent;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class QShowEvent;
class QEvent;

#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
struct _SpiceSession;
struct _SpiceDisplayChannel;
struct _SpiceInputsChannel;
struct _SpiceChannel;
typedef struct _SpiceSession SpiceSession;
typedef struct _SpiceDisplayChannel SpiceDisplayChannel;
typedef struct _SpiceInputsChannel SpiceInputsChannel;
typedef struct _SpiceChannel SpiceChannel;
#endif

class Spice_View : public Guest_Display_View
{
	Q_OBJECT
	public:
		explicit Spice_View( QWidget *parent = 0 );
		~Spice_View();

		void Connect_To( const QString &host, int port ) override;
		void Disconnect() override;
		bool Is_Connected() const override;
		void Send_CAD() override;
		QString Backend_Name() const override;

		bool Spice_GTK_Available() const;
		bool Spice_Available() const { return Spice_GTK_Available(); }

	protected:
		void paintEvent( QPaintEvent *event ) override;
		void resizeEvent( QResizeEvent *event ) override;
		void showEvent( QShowEvent *event ) override;
		void enterEvent( QEvent *event ) override;
		void leaveEvent( QEvent *event ) override;
		void mouseMoveEvent( QMouseEvent *event ) override;
		void mousePressEvent( QMouseEvent *event ) override;
		void mouseReleaseEvent( QMouseEvent *event ) override;
		void wheelEvent( QWheelEvent *event ) override;
		void keyPressEvent( QKeyEvent *event ) override;
		void keyReleaseEvent( QKeyEvent *event ) override;

	private slots:
		void Pump_GLib();

	private:
#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
		static void On_Channel_New( SpiceSession *session, SpiceChannel *channel, void *user );
		static void On_Channel_Destroy( SpiceSession *session, SpiceChannel *channel, void *user );
		static void On_Channel_Event( SpiceChannel *channel, int event, void *user );
		static void On_Primary_Create( SpiceChannel *channel, int format, int width, int height,
		                              int stride, int shmid, void *imgdata, void *user );
		static void On_Primary_Destroy( SpiceChannel *channel, void *user );
		static void On_Invalidate( SpiceChannel *channel, int x, int y, int w, int h, void *user );

		void Attach_Channel( SpiceChannel *channel );
		void Detach_Channel( SpiceChannel *channel );
		void Copy_Primary( int format, int width, int height, int stride, void *imgdata );
		void Copy_Invalidate( int x, int y, int w, int h );
		void Copy_Rect_From_Primary( int x, int y, int w, int h );
		void Emit_Connected_Once();
		void Handle_Channel_Event( SpiceChannel *channel, int event );

		QPoint Guest_From_Widget( const QPoint &widget_pos ) const;
		QRectF Scaled_Dest_Rect() const;
		void Send_Mouse_Buttons( int spice_button, bool press );
		void Send_Key( QKeyEvent *event, bool press );
		static int Qt_Button_To_Spice( Qt::MouseButton button );
		static int Qt_Buttons_To_Mask( Qt::MouseButtons buttons );
		static unsigned Qt_Key_To_XT( int qt_key, quint32 native_scan );
#endif
		void Set_Local_Cursor_Hidden( bool hidden );

		QLabel *Status;
		QTimer *Glib_Timer;
		QString Host;
		int Port;
		bool Connected_Flag;
		bool Connected_Emitted;

		QImage Frame;
		QByteArray Primary_Bytes;
		int Primary_Width;
		int Primary_Height;
		int Primary_Stride;
		int Primary_Format;
		const unsigned char *Primary_Data; // spice buffer (invalid after destroy)

#if defined(AQEMU_HAVE_SPICE_GLIB) || defined(AQEMU_HAVE_SPICE_GTK)
		SpiceSession *Session;
		SpiceDisplayChannel *Display;
		SpiceInputsChannel *Inputs;
		int Button_Mask;
		int Display_Id;
#endif
};

#endif
