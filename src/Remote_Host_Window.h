/****************************************************************************
** Remote QEMU host / optional libvirt URI helper (borrowed from virt-manager UX).
****************************************************************************/
#ifndef REMOTE_HOST_WINDOW_H
#define REMOTE_HOST_WINDOW_H

#include <QDialog>
#include <QString>

class QLineEdit;
class QComboBox;
class QPlainTextEdit;
class QListWidget;

class Remote_Host_Window : public QDialog
{
	Q_OBJECT
	public:
		explicit Remote_Host_Window( QWidget *parent = 0 );

	private slots:
		void Save_Connection();
		void Delete_Connection();
		void Load_Selected();
		void Open_Ssh_Tunnels();
		void Launch_Libvirt_Helper();
		void Show_Migrate_Help();

	private:
		void Reload_List();
		QString Settings_Key() const;

		QListWidget *List;
		QComboBox *CB_Type;
		QLineEdit *Edit_Name;
		QLineEdit *Edit_Host;
		QLineEdit *Edit_User;
		QLineEdit *Edit_Ssh_Port;
		QLineEdit *Edit_Qmp_Port;
		QLineEdit *Edit_Spice_Port;
		QLineEdit *Edit_Libvirt_Uri;
		QPlainTextEdit *Help;
};

#endif
