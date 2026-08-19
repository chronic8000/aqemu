#ifndef APPLE_SOC_RESTORE_WINDOW_H
#define APPLE_SOC_RESTORE_WINDOW_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QProcess>
#include <QLabel>

class Virtual_Machine;

class Apple_SoC_Restore_Window : public QDialog
{
	Q_OBJECT
public:
	explicit Apple_SoC_Restore_Window( Virtual_Machine *vm, QWidget *parent = nullptr );
	void Set_VM( Virtual_Machine *vm );

private slots:
	void Browse_IPSW();
	void Browse_Companion_Disk();
	void Create_Companion_Helper();
	void Refresh_Companion_Snippet();
	void Start_Companion_WSL();
	void Stop_Companion_WSL();
	void Run_Diagnose_WSL();
	void Run_IDeviceRestore();
	void Wipe_Inferno_Disks();
	void On_Process_Output();
	void On_Process_Finished( int code, QProcess::ExitStatus st );
	void On_Companion_Finished( int code, QProcess::ExitStatus st );

private:
	void Sync_Conn_To_VM();
	QString Conn_Type() const;
	QString Conn_Addr() const;
	int Conn_Port() const;
	QString Companion_Disk() const;
	QString Companion_Device_Arg() const;
	void Append_Log( const QString &text );
	bool Ensure_WSL_Creds( QString *distro_out, QString *user_out );
	QStringList WSL_Bash_Args( const QString &distro, const QString &user,
	                           const QString &script ) const;

	Virtual_Machine *VM;
	QLineEdit *Edit_IPSW;
	QComboBox *CB_Nand_Size;
	QSpinBox *SB_Nand_Size;
	QLineEdit *Edit_Companion_Disk;
	QLineEdit *Edit_SSH_User;
	QLineEdit *Edit_SSH_Password;
	QLineEdit *Edit_Conn_Addr;
	QComboBox *CB_Conn_Type;
	QSpinBox *SB_Conn_Port;
	QLabel *Label_Status;
	QLabel *Label_VM_File;
	QTextEdit *Text_Log;
	QTextEdit *Text_Companion;
	QProcess *Process;
	QProcess *Companion_Process;
};

#endif
