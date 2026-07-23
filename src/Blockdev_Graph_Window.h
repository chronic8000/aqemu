/****************************************************************************
** Blockdev graph editor — extra -blockdev nodes (qemu-doc block layer)
****************************************************************************/
#ifndef BLOCKDEV_GRAPH_WINDOW_H
#define BLOCKDEV_GRAPH_WINDOW_H

#include <QDialog>
#include <QString>

class QTableWidget;
class QPlainTextEdit;
class QComboBox;
class QLineEdit;
class QLabel;

class Blockdev_Graph_Window : public QDialog
{
	Q_OBJECT
	public:
		explicit Blockdev_Graph_Window( QWidget *parent = 0 );

		/** One -blockdev argument per line (without the -blockdev flag). */
		QString Get_Lines() const;
		void Set_Lines( const QString &lines );

	private slots:
		void On_Add();
		void On_Remove();
		void On_Template_Changed( int index );
		void On_Selection_Changed();
		void Sync_Preview();

	private:
		void Apply_Template_To_Form( int index );
		QString Form_To_Line() const;
		void Load_Row_To_Form( int row );
		void Add_Line_As_Row( const QString &line );

		QTableWidget *Table;
		QComboBox *CB_Template;
		QLineEdit *Edit_Node;
		QComboBox *CB_Driver;
		QLineEdit *Edit_File;
		QLineEdit *Edit_Backing;
		QLineEdit *Edit_Extra;
		QPlainTextEdit *Preview;
};

#endif
