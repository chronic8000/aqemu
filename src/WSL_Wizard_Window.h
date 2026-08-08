#ifndef WSL_WIZARD_WINDOW_H
#define WSL_WIZARD_WINDOW_H

#include <QDialog>
#include <QSettings>

class QComboBox;
class QLineEdit;
class QLabel;

class WSL_Wizard_Window : public QDialog
{
	Q_OBJECT

	public:
		WSL_Wizard_Window( QWidget *parent = 0 );
		QString Get_Distro() const;
		QString Get_Username() const;

	private slots:
		void on_CB_Distro_currentIndexChanged( int index );
		void on_Btn_Ok_clicked();
		void on_Btn_Cancel_clicked();

	private:
		QComboBox *CB_Distro;
		QLineEdit *Edit_User;
		QSettings Settings;
};

#endif
