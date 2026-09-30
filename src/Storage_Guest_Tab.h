#ifndef STORAGE_GUEST_TAB_H
#define STORAGE_GUEST_TAB_H

#include <QWidget>

class QCheckBox;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QPlainTextEdit;
class QListWidget;
class TrueNAS_Guest_Client;

class Storage_Guest_Tab : public QWidget
{
	Q_OBJECT

	public:
		explicit Storage_Guest_Tab( QWidget *parent = nullptr );
		void Show_VM( const QString &vm_uid );

	private slots:
		void Integration_Toggled( bool on );
		void Save_Form();
		void Refresh();
		void Rollback();

	private:
		void Apply_Enabled_State();
		QString Settings_Group() const;

		QString VM_UID;
		bool Loading;
		QCheckBox *CH_Enable;
		QLineEdit *Edit_Host;
		QSpinBox *SB_Port;
		QLineEdit *Edit_Token;
		QPushButton *Button_Refresh;
		QPlainTextEdit *Pools;
		QListWidget *Snapshots;
		QPushButton *Button_Rollback;
		TrueNAS_Guest_Client *Client;
};

#endif
