#ifndef APPLE_SOC_DEVICE_TOOLS_WINDOW_H
#define APPLE_SOC_DEVICE_TOOLS_WINDOW_H

#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QProcess>
#include <QLabel>
#include <QTabWidget>

class Virtual_Machine;

/** Companion-backed libimobiledevice helpers + reverse-tether (guest internet). */
class Apple_SoC_Device_Tools_Window : public QDialog
{
	Q_OBJECT
public:
	explicit Apple_SoC_Device_Tools_Window( Virtual_Machine *vm, QWidget *parent = nullptr );
	void Set_VM( Virtual_Machine *vm );
	/** 0 = Internet (default), 1 = Device */
	void Select_Tab( int index );

private slots:
	void Run_Device_Info();
	void Run_List_Apps();
	void Run_Install_IPA();
	void Run_Screenshot();
	void Run_Network_Diagnose();
	void Enable_Guest_Internet();
	void Apply_Reverse_Tether_Hints();
	void On_Process_Output();
	void On_Process_Finished( int code, QProcess::ExitStatus st );

private:
	void Append_Log( const QString &text );
	bool Ensure_WSL_Creds( QString *distro_out, QString *user_out );
	QStringList WSL_Bash_Args( const QString &distro, const QString &user,
	                           const QString &script ) const;
	bool Start_Companion_SSH( const QString &remote_cmd );
	QString SSH_Base() const;

	Virtual_Machine *VM;
	QLineEdit *Edit_SSH_User;
	QLineEdit *Edit_SSH_Password;
	QLineEdit *Edit_SSH_Port;
	QTabWidget *Tabs;
	QTextEdit *Text_Log;
	QLabel *Label_Status;
	QProcess *Process;
	QCheckBox *Chk_Sign_IPA;
};

void AQ_Show_Apple_SoC_Device_Tools_Window( Virtual_Machine *vm, QWidget *parent = nullptr,
                                            int start_tab = 0 );

#endif
