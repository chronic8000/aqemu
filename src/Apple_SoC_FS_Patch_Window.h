#ifndef APPLE_SOC_FS_PATCH_WINDOW_H
#define APPLE_SOC_FS_PATCH_WINDOW_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QProcess>

class Virtual_Machine;

/** Apply ChefKiss Inferno filesystem patches to the iOS guest root NVMe image.
 *  Target is *_inferno/root (NOT the IPSW restore companion). */
class Apple_SoC_FS_Patch_Window : public QDialog
{
	Q_OBJECT
public:
	explicit Apple_SoC_FS_Patch_Window( Virtual_Machine *vm, QWidget *parent = nullptr );
	void Set_VM( Virtual_Machine *vm );
	void Set_Root_Image( const QString &path );

signals:
	/** Emitted after a successful patch; callers should clear Restore ramdisk (-initrd). */
	void Patches_Applied( Virtual_Machine *vm );

private slots:
	void Browse_Root();
	void Use_Current_VM_Root();
	void Start_Patch();
	void On_Process_Output();
	void On_Process_Finished( int code, QProcess::ExitStatus st );

private:
	void Refresh_Status();
	void Append_Log( const QString &text );
	QString Resolve_Root_Path() const;
	QString Find_Patch_Script() const;
	bool Ensure_WSL_Creds( QString *distro_out, QString *user_out );
	QStringList WSL_Bash_Args( const QString &distro, const QString &user,
	                           const QString &script ) const;

	Virtual_Machine *VM;
	QLineEdit *Edit_Root;
	QLabel *Label_Status;
	QTextEdit *Text_Log;
	QPushButton *Btn_Start;
	QProcess *Process;
};

/** Open the FS-patch dialog for an Apple SoC VM (post-restore or menu). */
void AQ_Show_Apple_SoC_FS_Patch_Window( Virtual_Machine *vm, QWidget *parent = nullptr );

/** Directory containing apply-fs-patches-wsl.sh (next to aqemu.exe or repo extras/). */
QString AQ_Inferno_Extras_Dir();

#endif
