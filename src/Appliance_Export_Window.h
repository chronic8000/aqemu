/****************************************************************************
**
** Appliance Export Window (OVA / OVF)
**
****************************************************************************/

#ifndef APPLIANCE_EXPORT_WINDOW_H
#define APPLIANCE_EXPORT_WINDOW_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>

#include "VM.h"

class Appliance_Export_Window : public QDialog
{
	Q_OBJECT

public:
	explicit Appliance_Export_Window( const QList<Virtual_Machine*> &vms, int current_index = 0, QWidget *parent = nullptr );
	~Appliance_Export_Window() override = default;

private slots:
	void On_VM_Selection_Changed( int index );
	void On_Browse_Destination();
	void On_Start_Export();

private:
	void Setup_UI();
	bool Execute_Export();

	QComboBox *CB_VM_Selection = nullptr;
	QLineEdit *Edit_Export_Path = nullptr;
	QToolButton *TB_Browse_Export = nullptr;

	QProgressBar *Progress_Bar = nullptr;
	QLabel *Label_Status = nullptr;

	QPushButton *Btn_Export = nullptr;
	QPushButton *Btn_Cancel = nullptr;

	QList<Virtual_Machine*> Available_VMs;
	bool Is_Working = false;
};

#endif // APPLIANCE_EXPORT_WINDOW_H
