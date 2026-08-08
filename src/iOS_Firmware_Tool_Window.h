#ifndef IOS_FIRMWARE_TOOL_WINDOW_H
#define IOS_FIRMWARE_TOOL_WINDOW_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QStackedWidget>
#include <QLabel>
#include <QProcess>

class iOS_Firmware_Tool_Window : public QDialog
{
	Q_OBJECT

public:
	explicit iOS_Firmware_Tool_Window( QWidget *parent = nullptr );
	~iOS_Firmware_Tool_Window() = default;

private slots:
	void Browse_IPSW_File();
	void Browse_Output_Dir();
	void Browse_IM4P_File();
	void Browse_IM4M_File();

	void Run_IPSW_Extraction();
	void Run_IM4P_Operation();
	void Run_Repackage_IMG4();
	void Copy_Output_Paths();

	void On_Process_Output();
	void On_Process_Finished( int exitCode, QProcess::ExitStatus exitStatus );

private:
	void Setup_Ui();
	QString Find_PyIMG4_Executable() const;

	// UI Controls
	QStackedWidget *Stack_Pages;

	// Page 1: IPSW Unpack & IM4P Extraction
	QLineEdit *Edit_IPSW_Path;
	QLineEdit *Edit_Output_Dir;
	QPushButton *Btn_Browse_IPSW;
	QPushButton *Btn_Browse_OutDir;
	QPushButton *Btn_Start_Unpack;

	// Page 2: IM4P Payload Operation (Extract / Decrypt / Stats)
	QLineEdit *Edit_IM4P_Path;
	QLineEdit *Edit_AES_IV;
	QLineEdit *Edit_AES_Key;
	QComboBox *CB_IM4P_Action;
	QPushButton *Btn_Browse_IM4P;
	QPushButton *Btn_Start_IM4P;

	// Page 3: Repackage Bootable IMG4
	QLineEdit *Edit_Repack_IM4P;
	QLineEdit *Edit_Repack_IM4M;
	QLineEdit *Edit_Repack_Output;
	QPushButton *Btn_Browse_Repack_IM4P;
	QPushButton *Btn_Browse_Repack_IM4M;
	QPushButton *Btn_Start_Repack;

	// Output Console & Paths
	QTextEdit *Text_Console_Log;
	QTextEdit *Text_Result_Paths;
	QPushButton *Btn_Copy_Paths;

	// Process Execution
	QProcess *Process;
	QString PyIMG4_Exe;
};

#endif // IOS_FIRMWARE_TOOL_WINDOW_H
