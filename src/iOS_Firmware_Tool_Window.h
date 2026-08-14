#ifndef IOS_FIRMWARE_TOOL_WINDOW_H
#define IOS_FIRMWARE_TOOL_WINDOW_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QProcess>

class iOS_Firmware_Tool_Window : public QDialog
{
	Q_OBJECT

public:
	explicit iOS_Firmware_Tool_Window( QWidget *parent = nullptr );
	~iOS_Firmware_Tool_Window() = default;

signals:
	void DeviceTree_Path_Suggested( const QString &path );
	void Restore_Ticket_Suggested( const QString &path );
	void Ipsw_Path_Suggested( const QString &path );
	void Restore_Ramdisk_Suggested( const QString &path );
	void Sep_Firmware_Suggested( const QString &path );

private slots:
	void Browse_IPSW_File();
	void Browse_Output_Dir();
	void Browse_IM4P_File();
	void Browse_Manifest();
	void Browse_SHSH();
	void Browse_SEP_IM4P();
	void Run_Pack_SEP();

	void Run_IPSW_Extraction();
	void Run_IM4P_Operation();
	void Run_Forge_Tickets();
	void Copy_Output_Paths();

	void On_Process_Output();
	void On_Process_Finished( int exitCode, QProcess::ExitStatus exitStatus );

private:
	void Setup_Ui();
	QString Find_PyIMG4_Executable() const;
	bool Ensure_PyIMG4_Available();
	bool Find_Python( QString *exe_out, QStringList *prefix_args_out ) const;
	bool Ensure_Python_Ready();
	void Suggest_From_Extract_Dir();
	QString Restore_Ramdisk_From_Manifest( const QString &manifest_path, const QString &extract_dir ) const;
	bool Ensure_Process_Idle();
	bool Start_Ticket_Script( bool sep_ticket );
	QString Extras_Dir() const;
	QString Ticket_Script( bool sep_ticket ) const;
	QString Find_Img4_Executable() const;
	bool Ensure_Img4_Available();
	bool Start_Img4_Pack();

	QLineEdit *Edit_IPSW_Path;
	QLineEdit *Edit_Output_Dir;
	QPushButton *Btn_Browse_IPSW;
	QPushButton *Btn_Browse_OutDir;
	QPushButton *Btn_Start_Unpack;

	QComboBox *CB_Ticket_Model;
	QLineEdit *Edit_Manifest;
	QLineEdit *Edit_SHSH;
	QLineEdit *Edit_Ticket_Out;
	QLineEdit *Edit_Sep_Ticket_Out;
	QPushButton *Btn_Forge_Tickets;

	QLineEdit *Edit_SEP_IM4P;
	QLineEdit *Edit_SEP_IVKEY;
	QLineEdit *Edit_SEP_Dec;
	QLineEdit *Edit_SEP_Out;
	QPushButton *Btn_Pack_SEP;

	QLineEdit *Edit_IM4P_Path;
	QLineEdit *Edit_AES_IV;
	QLineEdit *Edit_AES_Key;
	QComboBox *CB_IM4P_Action;
	QPushButton *Btn_Browse_IM4P;
	QPushButton *Btn_Start_IM4P;

	QTextEdit *Text_Console_Log;
	QTextEdit *Text_Result_Paths;
	QPushButton *Btn_Copy_Paths;

	QProcess *Process;
	QString PyIMG4_Exe;
	QString Python_Exe;
	QStringList Python_Prefix;
	QString Img4_Exe;
	QString Last_IM4P_Output;
	QString Last_Ticket_Out;
	QString Img4_Stdout_Buf;
	QString Last_Img4_Version;
	bool Chain_Sep_Ticket;
	bool Tried_Pip;

	enum class Pending_Op { None, IpswExtract, Im4pOp, PipInstall, TicketAp, TicketSep,
	                        Img4Decrypt, Img4Pack };
	Pending_Op Last_Operation;
};

#endif
