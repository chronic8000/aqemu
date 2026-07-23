/****************************************************************************
** QEMU help browser — probe -M / -cpu / -device / -netdev help like the CLI.
****************************************************************************/

#ifndef QEMU_HELP_BROWSER_H
#define QEMU_HELP_BROWSER_H

#include <QDialog>

class QComboBox;
class QPlainTextEdit;
class QLineEdit;
class QProcess;

class QEMU_Help_Browser : public QDialog
{
	Q_OBJECT
public:
	explicit QEMU_Help_Browser( QWidget *parent = nullptr );

private slots:
	void On_Probe_Clicked();
	void On_Binary_Changed( int index );
	void On_Filter_Changed( const QString &text );

private:
	void Fill_Binaries();
	void Apply_Filter();

	QComboBox *CB_Binary;
	QComboBox *CB_Topic;
	QLineEdit *Edit_Filter;
	QPlainTextEdit *Edit_Output;
	QString Raw_Output;
};

#endif
