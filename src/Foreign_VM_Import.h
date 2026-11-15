/****************************************************************************
** Import VMware (.vmx) and VirtualBox (.vbox) configs into AQEMU VMs.
****************************************************************************/
#ifndef FOREIGN_VM_IMPORT_H
#define FOREIGN_VM_IMPORT_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QDialog>

class Virtual_Machine;
class QLineEdit;
class QComboBox;
class QPlainTextEdit;
class QProgressBar;
class QLabel;
class QPushButton;

struct Foreign_Disk
{
	QString path;
	QString bus; // ide / sata / scsi / nvme / virtio
	bool cdrom = false;
};

struct Foreign_VM_Config
{
	QString source_path;
	QString source_kind; // vmx / vbox
	QString name;
	int cpus = 1;
	int memory_mb = 1024;
	bool uefi = false;
	QString nic_model;
	QString mac;
	QList<Foreign_Disk> disks;
	QStringList warnings;
	QStringList notes;
};

bool AQ_Parse_VMX_File( const QString &path, Foreign_VM_Config *out, QString *error );
bool AQ_Parse_VBox_File( const QString &path, Foreign_VM_Config *out, QString *error );
bool AQ_Parse_Foreign_VM( const QString &path, Foreign_VM_Config *out, QString *error );

/** Create a Virtual_Machine from a parsed foreign config. Caller owns the result. */
Virtual_Machine *AQ_Create_VM_From_Foreign( const Foreign_VM_Config &cfg,
                                            const QString &dest_dir,
                                            bool convert_to_qcow2,
                                            QString *error,
                                            QString *summary );

class Foreign_VM_Import_Window : public QDialog
{
	Q_OBJECT
	public:
		explicit Foreign_VM_Import_Window( QWidget *parent = nullptr );
		void Set_Source_Path( const QString &path );
		Virtual_Machine *Get_Imported_VM() const { return Imported_VM; }

	private slots:
		void Browse_Source();
		void Browse_Dest();
		void Reload_Source();
		void Start_Import();

	private:
		void Setup_UI();
		void Show_Config( const Foreign_VM_Config &cfg );

		QLineEdit *Edit_Source = nullptr;
		QLineEdit *Edit_Dest = nullptr;
		QLineEdit *Edit_Name = nullptr;
		QComboBox *CB_Mode = nullptr;
		QPlainTextEdit *Summary = nullptr;
		QProgressBar *Progress = nullptr;
		QLabel *Status = nullptr;
		QPushButton *Btn_Import = nullptr;

		Foreign_VM_Config Current;
		Virtual_Machine *Imported_VM = nullptr;
};

#endif
