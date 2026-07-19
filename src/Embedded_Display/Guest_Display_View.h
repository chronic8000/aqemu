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
		virtual QString Backend_Name() const = 0;

	signals:
		void Connected();
		void Disconnected();
		void Connection_Error( const QString &message );
};

#endif
