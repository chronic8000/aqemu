/****************************************************************************
** Simple storage browser over VM_Directory (virt-manager “pool” lite).
****************************************************************************/
#ifndef STORAGE_BROWSER_WINDOW_H
#define STORAGE_BROWSER_WINDOW_H

#include <QDialog>
#include <QString>

class QListWidget;
class QLabel;
class QLineEdit;

class Storage_Browser_Window : public QDialog
{
	Q_OBJECT
	public:
		explicit Storage_Browser_Window( QWidget *parent = 0 );

		/** Selected file path, or empty if cancelled / none. */
		QString Selected_Path() const;

		/** Filter: "all", "disk", "iso", "floppy" */
		void Set_Filter_Mode( const QString &mode );

	private slots:
		void Refresh();
		void Browse_Root();
		void Accept_Selection();
		void On_Double_Click();

	private:
		QString Root;
		QString Filter_Mode;
		QLineEdit *Edit_Root;
		QListWidget *List;
		QLabel *Label_Info;
		QString Chosen;
};

#endif
