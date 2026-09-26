/****************************************************************************
** Abstract guest framebuffer view (SPICE or VNC)
****************************************************************************/
#ifndef GUEST_DISPLAY_VIEW_H
#define GUEST_DISPLAY_VIEW_H

#include <QWidget>
#include <QString>

class Guest_Display_View : public QWidget
{
	Q_OBJECT
	public:
		explicit Guest_Display_View( QWidget *parent = 0 ) : QWidget( parent ) {}
		virtual ~Guest_Display_View();

		virtual void Connect_To( const QString &host, int port ) = 0;
		virtual void Disconnect() = 0;
		virtual bool Is_Connected() const = 0;
		virtual void Send_CAD() = 0; // Ctrl-Alt-Delete
		/** Win11 Setup: open cmd (Shift+F10). Default no-op for VNC. */
		virtual void Send_Shift_F10() {}
		virtual QString Backend_Name() const = 0;

	signals:
		void Connected();
		void Disconnected();
		void Connection_Error( const QString &message );
};

#include <QRect>
#include <QImage>

/**
 * Detect active display rectangle if the guest framebuffer has an inactive border
 * (e.g. virtio-gpu fixed scanouts or UEFI GOP before guest driver loads).
 * Returns the active rectangle within the frame, or img.rect() if normal.
 */
inline QRect AQ_Detect_Active_Frame_Rect( const QImage &img )
{
	if( img.isNull() || img.width() <= 0 || img.height() <= 0 )
		return QRect();

	const int w = img.width();
	const int h = img.height();

	if( w <= 800 || h <= 600 )
		return QRect( 0, 0, w, h );

	// Sample rows to find max X with non-black pixel
	const int test_rows[] = { h / 8, h / 4, h * 3 / 8, h / 2, h * 5 / 8, h * 3 / 4 };
	const int n_rows = sizeof( test_rows ) / sizeof( test_rows[0] );
	int max_x = 0;
	for( int i = 0; i < n_rows; ++i )
	{
		const int y = test_rows[i];
		for( int x = w - 1; x > 0; x -= 8 )
		{
			const QRgb p = img.pixel( x, y );
			if( qRed( p ) > 4 || qGreen( p ) > 4 || qBlue( p ) > 4 )
			{
				if( x > max_x )
					max_x = x;
				break;
			}
		}
	}

	// Sample columns to find max Y with non-black pixel
	const int test_cols[] = { w / 8, w / 4, w * 3 / 8, w / 2, w * 5 / 8, w * 3 / 4 };
	const int n_cols = sizeof( test_cols ) / sizeof( test_cols[0] );
	int max_y = 0;
	for( int i = 0; i < n_cols; ++i )
	{
		const int x = test_cols[i];
		for( int y = h - 1; y > 0; y -= 8 )
		{
			const QRgb p = img.pixel( x, y );
			if( qRed( p ) > 4 || qGreen( p ) > 4 || qBlue( p ) > 4 )
			{
				if( y > max_y )
					max_y = y;
				break;
			}
		}
	}

	// If content reaches within 32px of the border, treat as full frame
	if( max_x >= w - 32 )
		max_x = w;
	else
		max_x = qMin( w, ( ( max_x + 15 ) / 16 ) * 16 );

	if( max_y >= h - 32 )
		max_y = h;
	else
		max_y = qMin( h, ( ( max_y + 15 ) / 16 ) * 16 );

	// Guard against completely black frames
	if( max_x < 320 || max_y < 240 )
		return QRect( 0, 0, w, h );

	return QRect( 0, 0, max_x, max_y );
}

#endif
