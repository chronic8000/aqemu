/****************************************************************************
**
** Appliance Import Window (OVA / OVF)
**
****************************************************************************/

#ifndef APPLIANCE_IMPORT_WINDOW_H
#define APPLIANCE_IMPORT_WINDOW_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QSettings>

#include "OVF_Parser.h"
#include "VM.h"

class Appliance_Import_Window : public QDialog
{
	Q_OBJECT

public:
	explicit Appliance_Import_Window( QWidget *parent = nullptr );
	~Appliance_Import_Window() override = default;

	void Set_Appliance_Path( const QString &path );
	Virtual_Machine *Get_Imported_VM() const { return Imported_VM; }

private slots:
	void On_Browse_Appliance();
	void On_Browse_Destination();
	void On_Appliance_Path_Changed( const QString &path );
	void On_Start_Import();

private:
	void Setup_UI();
	void Populate_OS_List();
	void Update_Appliance_Details( const OVF_Appliance &appliance );
	bool Execute_Extraction_And_Conversion();

	QSettings Settings;

	QLineEdit *Edit_Appliance_Path = nullptr;
	QToolButton *TB_Browse_Appliance = nullptr;

	QLineEdit *Edit_VM_Name = nullptr;
	QComboBox *CB_Guest_OS = nullptr;
	QSpinBox *SB_CPU = nullptr;
	QSpinBox *SB_RAM = nullptr;
	QComboBox *CB_Disk_Format = nullptr;

	QTableWidget *Table_Disks = nullptr;

	QLineEdit *Edit_Dest_Dir = nullptr;
	QToolButton *TB_Browse_Dest = nullptr;

	QProgressBar *Progress_Bar = nullptr;
	QLabel *Label_Status = nullptr;

	QPushButton *Btn_Import = nullptr;
	QPushButton *Btn_Cancel = nullptr;

	OVF_Appliance Current_Appliance;
	Virtual_Machine *Imported_VM = nullptr;
	bool Is_Working = false;
};

#endif // APPLIANCE_IMPORT_WINDOW_H
