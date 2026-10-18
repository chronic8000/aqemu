/****************************************************************************
** Search a folder for .aqemu files and add them to the VM list.
****************************************************************************/

#ifndef VM_SCANNER_WINDOW_H
#define VM_SCANNER_WINDOW_H

#include <QDialog>

class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QLabel;
class Main_Window;

class VM_Scanner_Window : public QDialog
{
	Q_OBJECT

	public:
		explicit VM_Scanner_Window( Main_Window *owner, QWidget *parent = nullptr );

	private slots:
		void Browse();
		void Start();
		void Stop();

	private:
		Main_Window *Owner;
		QLineEdit *Edit_Folder;
		QPlainTextEdit *Log;
		QLabel *Label_Status;
		QPushButton *Button_Start;
		QPushButton *Button_Stop;
		bool Stop_Requested;
};

#endif
