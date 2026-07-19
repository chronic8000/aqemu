/****************************************************************************
** SPICE guest display — spice-gtk when AQEMU_HAVE_SPICE_GTK, else null/VNC bridge stub
****************************************************************************/
#ifndef SPICE_VIEW_H
#define SPICE_VIEW_H

#include "Guest_Display_View.h"

class QLabel;
class QVBoxLayout;

#ifdef AQEMU_HAVE_SPICE_GTK
// Forward GObject types without pulling headers into public consumers
struct _SpiceSession;
struct _SpiceDisplay;
typedef struct _SpiceSession SpiceSession;
typedef struct _SpiceDisplay SpiceDisplay;
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

	private:
		QVBoxLayout *Layout;
		QLabel *Status;
		QString Host;
		int Port;
		bool Connected_Flag;

#ifdef AQEMU_HAVE_SPICE_GTK
		SpiceSession *Session;
		QWidget *Spice_Container;
#endif
};

#endif
