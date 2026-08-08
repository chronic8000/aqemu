#ifndef APPLE_SOC_RESTORE_WINDOW_H
#define APPLE_SOC_RESTORE_WINDOW_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QProcess>

class Virtual_Machine;

class Apple_SoC_Restore_Window : public QDialog
{
	Q_OBJECT
public:
	explicit Apple_SoC_Restore_Window( Virtual_Machine *vm, QWidget *parent = nullptr );

private slots:
	void Browse_IPSW();
	void Copy_Companion_Command();
	void Run_IDeviceRestore();
	void On_Process_Output();
	void On_Process_Finished( int code, QProcess::ExitStatus st );

private:
	Virtual_Machine *VM;
	QLineEdit *Edit_IPSW;
	QLineEdit *Edit_Conn_Addr;
	QComboBox *CB_Conn_Type;
	QTextEdit *Text_Log;
	QTextEdit *Text_Companion;
	QProcess *Process;
};

#endif
