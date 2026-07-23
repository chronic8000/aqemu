/****************************************************************************
** Full-window VM session: toolbar + guest display (SPICE preferred, VNC fallback)
****************************************************************************/
#ifndef VM_SESSION_WIDGET_H
#define VM_SESSION_WIDGET_H

#include <QVBoxLayout>
#include <QWidget>
#include <QToolBar>
#include <QAction>
#include <QStackedWidget>
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QAbstractAnimation>
#include <QHash>
#include <QJsonArray>
#include <QPoint>

#include "VM_Devices.h"

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
		void Request_Pause();
		void Request_Save();

	protected:
		bool eventFilter( QObject *obj, QEvent *event ) override;
		void resizeEvent( QResizeEvent *event ) override;
		void changeEvent( QEvent *event ) override;

	private slots:
		void On_Change_CD();
		void On_Eject_CD();
		void On_Change_FD0();
		void On_Eject_FD0();
		void On_Change_FD1();
		void On_Eject_FD1();
		void On_CAD();
		void On_Shift_F10();
		void On_Fullscreen();
		void On_Pause();
		void On_Save();
		void On_Power_Off();
		void On_Shutdown();
		void On_Reset();
		void On_Exit_View();
		void On_Display_Connected();
		void On_Display_Disconnected();
		void On_Display_Error( const QString &msg );
		void Try_Connect_Display();
		void Schedule_Display_Connect( int delay_ms );
		bool Tcp_Port_Is_Open( const QString &host, int port ) const;
		void On_Toolbar_Hide_Timeout();
		void On_Toolbar_Show_Timeout();
		void On_QMP_Connected();
		void On_Drive_Poll();
		void On_Block_Stats( const QJsonArray &stats );

	private:
		void Build_Toolbar();
		void Update_Pause_Action();
		QString Pick_Backend( const QString &preferred ) const;
		QAction *Add_Toolbar_Action( const QIcon &icon, const QString &tip, const char *slot );
		void Set_Toolbar_In_Layout();
		void Set_Toolbar_Fullscreen_Overlay();
		void Show_Fullscreen_Toolbar();
		void Hide_Fullscreen_Toolbar();
		void Update_Fullscreen_State();
		void Set_Main_Window_Chrome_Visible( bool visible );
		void Handle_Fullscreen_Mouse( const QPoint &global_pos );
		bool Hot_Zone_Contains_Global( const QPoint &global_pos ) const;
		bool Toolbar_Contains_Global( const QPoint &global_pos ) const;
		QMP_Client *Active_QMP() const;
		bool Change_Medium_Id( const QString &block_id, const QString &path );
		bool Eject_Medium_Id( const QString &block_id );
		void Send_CAD_To_Guest();
		void Send_Monitor( const QString &cmd );
		void Enable_Boot_Device( VM::Boot_Device type );
		void Apply_Runtime_Boot_Order();
		void Persist_Media_Config();
		void Update_Media_Actions();
		void Set_Drive_Light( QLabel *light, bool loaded, bool active, const QString &tip );
		QLabel *Make_Drive_Light( const QString &letter );
		QString Hmp_Device_Name( const QString &block_id ) const;
		static QString Media_Base_Name( const QString &path );

		Virtual_Machine *VM;
		QMP_Client *QMP;
		QVBoxLayout *Main_Layout;
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

		QAction *Act_Fullscreen;
		QAction *Act_Pause;
		QAction *Act_Insert_CD;
		QAction *Act_Eject_CD;
		QAction *Act_Insert_FD0;
		QAction *Act_Eject_FD0;
		QAction *Act_Insert_FD1;
		QAction *Act_Eject_FD1;

		QLabel *Light_FD0;
		QLabel *Light_FD1;
		QLabel *Light_CD;
		QLabel *Light_HD;
		QTimer *Toolbar_Hide_Timer;
		QTimer *Toolbar_Show_Timer;
		QTimer *Drive_Poll_Timer;
		QTimer *Display_Connect_Timer;
		int Display_Connect_Attempts;
		bool Display_Connect_In_Progress;
		QHash<QString, qint64> Last_Drive_IO;
		bool Fullscreen_Active;
		bool Fullscreen_Toolbar_Visible;
		bool Main_Chrome_Hidden;
};

#endif
