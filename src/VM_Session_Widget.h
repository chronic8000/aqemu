/****************************************************************************
** Full-window VM session: toolbar + guest display (SPICE preferred, VNC fallback)
****************************************************************************/
#ifndef VM_SESSION_WIDGET_H
#define VM_SESSION_WIDGET_H

#include <QWidget>
#include <QToolBar>
#include <QAction>
#include <QStackedWidget>
#include <QLabel>

class Virtual_Machine;
class QMP_Client;
class Spice_View;
class Guest_Display_View;

#ifdef VNC_DISPLAY
class MachineView;
#endif

class VM_Session_Widget : public QWidget
{
	Q_OBJECT
	public:
		explicit VM_Session_Widget( QWidget *parent = 0 );
		~VM_Session_Widget();

		void Attach_VM( Virtual_Machine *vm, QMP_Client *qmp,
		                const QString &display_host,
		                int spice_port, int vnc_port,
		                const QString &preferred_backend );
		void Detach();

	signals:
		void Exit_Session_View();
		void Request_Stop();
		void Request_Shutdown();
		void Request_Reset();

	private slots:
		void On_Change_CD();
		void On_Eject_CD();
		void On_Change_FD0();
		void On_Eject_FD0();
		void On_Change_FD1();
		void On_Eject_FD1();
		void On_CAD();
		void On_Fullscreen();
		void On_Exit_View();
		void On_Display_Connected();
		void On_Display_Error( const QString &msg );

	private:
		void Build_Toolbar();
		QString Pick_Backend( const QString &preferred ) const;

		Virtual_Machine *VM;
		QMP_Client *QMP;
		QToolBar *Toolbar;
		QStackedWidget *Stack;
		QLabel *Placeholder;
		Spice_View *Spice;
#ifdef VNC_DISPLAY
		MachineView *Vnc;
#endif
		QString Host;
		int Spice_Port;
		int Vnc_Port;
		QString Backend;
};

#endif
