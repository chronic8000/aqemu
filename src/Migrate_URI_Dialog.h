/****************************************************************************
** Build a QEMU migrate URI with host/port helpers.
****************************************************************************/
#ifndef MIGRATE_URI_DIALOG_H
#define MIGRATE_URI_DIALOG_H

#include <QDialog>
#include <QString>

class QLineEdit;
class QCheckBox;

class Migrate_URI_Dialog : public QDialog
{
	Q_OBJECT
	public:
		explicit Migrate_URI_Dialog( QWidget *parent = 0 );
		QString URI() const;
		bool Copy_Storage() const; // hint only — QMP blk flag

	private:
		QLineEdit *Edit_Host;
		QLineEdit *Edit_Port;
		QLineEdit *Edit_Raw;
		QCheckBox *CH_Blk;
};

#endif
