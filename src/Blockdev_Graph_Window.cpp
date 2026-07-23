/****************************************************************************
** Blockdev graph editor
****************************************************************************/
#include "Blockdev_Graph_Window.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QRegularExpression>
#include <QAbstractItemView>

Blockdev_Graph_Window::Blockdev_Graph_Window( QWidget *parent )
	: QDialog( parent )
{
	setWindowTitle( tr( "Blockdev Graph" ) );
	resize( 720, 520 );

	QVBoxLayout *root = new QVBoxLayout( this );

	root->addWidget( new QLabel( tr(
		"Extra -blockdev nodes appended after auto-generated disk nodes.\n"
		"Use Modern -blockdev disks for HDA/HDB VirtIO/NVMe; add NBD/backing/filter nodes here." ), this ) );

	Table = new QTableWidget( 0, 1, this );
	Table->setHorizontalHeaderLabels( QStringList() << tr( "Blockdev argument" ) );
	Table->horizontalHeader()->setStretchLastSection( true );
	Table->setSelectionBehavior( QAbstractItemView::SelectRows );
	Table->setSelectionMode( QAbstractItemView::SingleSelection );
	Table->verticalHeader()->setVisible( false );
	root->addWidget( Table, 1 );

	QHBoxLayout *row_btns = new QHBoxLayout();
	QPushButton *btn_add = new QPushButton( tr( "Add from form" ), this );
	QPushButton *btn_rem = new QPushButton( tr( "Remove" ), this );
	row_btns->addWidget( btn_add );
	row_btns->addWidget( btn_rem );
	row_btns->addStretch();
	root->addLayout( row_btns );

	QFormLayout *form = new QFormLayout();
	CB_Template = new QComboBox( this );
	CB_Template->addItem( tr( "file + format (qcow2/raw)" ) );
	CB_Template->addItem( tr( "file only" ) );
	CB_Template->addItem( tr( "nbd" ) );
	CB_Template->addItem( tr( "qcow2 with backing" ) );
	CB_Template->addItem( tr( "blank / custom" ) );
	form->addRow( tr( "Template:" ), CB_Template );

	Edit_Node = new QLineEdit( this );
	Edit_Node->setPlaceholderText( QStringLiteral( "node-name e.g. aq-extra0" ) );
	form->addRow( tr( "Node name:" ), Edit_Node );

	CB_Driver = new QComboBox( this );
	CB_Driver->setEditable( true );
	CB_Driver->addItems( QStringList()
		<< QStringLiteral( "file" ) << QStringLiteral( "qcow2" )
		<< QStringLiteral( "raw" ) << QStringLiteral( "nbd" )
		<< QStringLiteral( "throttle" ) << QStringLiteral( "copy-on-read" ) );
	form->addRow( tr( "Driver:" ), CB_Driver );

	Edit_File = new QLineEdit( this );
	Edit_File->setPlaceholderText( tr( "filename= / server= / file= node" ) );
	form->addRow( tr( "File / path:" ), Edit_File );

	Edit_Backing = new QLineEdit( this );
	Edit_Backing->setPlaceholderText( tr( "optional backing= node-name" ) );
	form->addRow( tr( "Backing:" ), Edit_Backing );

	Edit_Extra = new QLineEdit( this );
	Edit_Extra->setPlaceholderText( tr( "extra props, e.g. cache.direct=on,read-only=on" ) );
	form->addRow( tr( "Extra:" ), Edit_Extra );
	root->addLayout( form );

	root->addWidget( new QLabel( tr( "Preview (emitted lines):" ), this ) );
	Preview = new QPlainTextEdit( this );
	Preview->setReadOnly( true );
	Preview->setMaximumHeight( 100 );
	root->addWidget( Preview );

	QDialogButtonBox *bbox = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this );
	root->addWidget( bbox );

	connect( btn_add, SIGNAL(clicked()), this, SLOT(On_Add()) );
	connect( btn_rem, SIGNAL(clicked()), this, SLOT(On_Remove()) );
	connect( CB_Template, SIGNAL(currentIndexChanged(int)), this, SLOT(On_Template_Changed(int)) );
	connect( Table, SIGNAL(itemSelectionChanged()), this, SLOT(On_Selection_Changed()) );
	connect( bbox, SIGNAL(accepted()), this, SLOT(accept()) );
	connect( bbox, SIGNAL(rejected()), this, SLOT(reject()) );

	Apply_Template_To_Form( 0 );
	Sync_Preview();
}

QString Blockdev_Graph_Window::Get_Lines() const
{
	QStringList out;
	for( int r = 0; r < Table->rowCount(); ++r )
	{
		QTableWidgetItem *it = Table->item( r, 0 );
		if( ! it ) continue;
		const QString line = it->text().trimmed();
		if( ! line.isEmpty() )
			out << line;
	}
	return out.join( QLatin1Char( '\n' ) );
}

void Blockdev_Graph_Window::Set_Lines( const QString &lines )
{
	Table->setRowCount( 0 );
	const QStringList parts = lines.split( QLatin1Char( '\n' ), QString::SkipEmptyParts );
	for( const QString &raw : parts )
	{
		const QString line = raw.trimmed();
		if( line.isEmpty() || line.startsWith( QLatin1Char( '#' ) ) )
			continue;
		Add_Line_As_Row( line );
	}
	Sync_Preview();
}

void Blockdev_Graph_Window::Add_Line_As_Row( const QString &line )
{
	const int r = Table->rowCount();
	Table->insertRow( r );
	Table->setItem( r, 0, new QTableWidgetItem( line ) );
}

void Blockdev_Graph_Window::Apply_Template_To_Form( int index )
{
	switch( index )
	{
		case 0: // file + format — adds TWO nodes when Add is clicked? We'll do format that references file node
			Edit_Node->setText( QStringLiteral( "aq-extra0" ) );
			CB_Driver->setCurrentText( QStringLiteral( "qcow2" ) );
			Edit_File->setText( QStringLiteral( "C:/path/disk.qcow2" ) );
			Edit_Backing->clear();
			Edit_Extra->setText( QStringLiteral( "pair=file+format" ) );
			break;
		case 1:
			Edit_Node->setText( QStringLiteral( "aq-file0" ) );
			CB_Driver->setCurrentText( QStringLiteral( "file" ) );
			Edit_File->setText( QStringLiteral( "C:/path/disk.img" ) );
			Edit_Backing->clear();
			Edit_Extra->clear();
			break;
		case 2:
			Edit_Node->setText( QStringLiteral( "aq-nbd0" ) );
			CB_Driver->setCurrentText( QStringLiteral( "nbd" ) );
			Edit_File->setText( QStringLiteral( "127.0.0.1" ) );
			Edit_Backing->clear();
			Edit_Extra->setText( QStringLiteral( "port=10809,export=exportname" ) );
			break;
		case 3:
			Edit_Node->setText( QStringLiteral( "aq-overlay0" ) );
			CB_Driver->setCurrentText( QStringLiteral( "qcow2" ) );
			Edit_File->setText( QStringLiteral( "C:/path/overlay.qcow2" ) );
			Edit_Backing->setText( QStringLiteral( "aqbd-base" ) );
			Edit_Extra->setText( QStringLiteral( "pair=file+format" ) );
			break;
		default:
			Edit_Node->clear();
			CB_Driver->setCurrentText( QStringLiteral( "file" ) );
			Edit_File->clear();
			Edit_Backing->clear();
			Edit_Extra->clear();
			break;
	}
}

void Blockdev_Graph_Window::On_Template_Changed( int index )
{
	Apply_Template_To_Form( index );
}

QString Blockdev_Graph_Window::Form_To_Line() const
{
	// Handled in On_Add for pair templates
	return QString();
}

void Blockdev_Graph_Window::On_Add()
{
	const QString node = Edit_Node->text().trimmed();
	if( node.isEmpty() )
	{
		QMessageBox::warning( this, tr( "Blockdev" ), tr( "Node name is required." ) );
		return;
	}
	const QString driver = CB_Driver->currentText().trimmed();
	const QString file = Edit_File->text().trimmed();
	const QString backing = Edit_Backing->text().trimmed();
	const QString extra = Edit_Extra->text().trimmed();
	const bool pair = extra.contains( QLatin1String( "pair=file+format" ) );

	if( pair )
	{
		const QString file_node = node + QStringLiteral( "-file" );
		QString file_bd = QStringLiteral( "driver=file,node-name=" ) + file_node +
		                  QStringLiteral( ",filename=" ) + file;
		QString fmt = driver;
		if( fmt == QLatin1String( "file" ) )
			fmt = QStringLiteral( "raw" );
		QString fmt_bd = QStringLiteral( "driver=" ) + fmt + QStringLiteral( ",node-name=" ) +
		                 node + QStringLiteral( ",file=" ) + file_node;
		if( ! backing.isEmpty() )
			fmt_bd += QStringLiteral( ",backing=" ) + backing;
		// strip pair= from remaining extra
		QString rest = extra;
		rest.replace( QLatin1String( "pair=file+format" ), QString() );
		rest.replace( QRegularExpression( QStringLiteral( ",{2,}" ) ), QStringLiteral( "," ) );
		rest = rest.trimmed();
		if( rest.startsWith( QLatin1Char( ',' ) ) ) rest = rest.mid( 1 );
		if( rest.endsWith( QLatin1Char( ',' ) ) ) rest.chop( 1 );
		if( ! rest.isEmpty() )
			fmt_bd += QLatin1Char( ',' ) + rest;
		Add_Line_As_Row( file_bd );
		Add_Line_As_Row( fmt_bd );
	}
	else if( driver == QLatin1String( "nbd" ) )
	{
		QString line = QStringLiteral( "driver=nbd,node-name=" ) + node +
		               QStringLiteral( ",server.type=inet,server.host=" ) + file;
		if( ! extra.isEmpty() )
			line += QLatin1Char( ',' ) + extra;
		Add_Line_As_Row( line );
	}
	else if( driver == QLatin1String( "file" ) )
	{
		QString line = QStringLiteral( "driver=file,node-name=" ) + node +
		               QStringLiteral( ",filename=" ) + file;
		if( ! extra.isEmpty() )
			line += QLatin1Char( ',' ) + extra;
		Add_Line_As_Row( line );
	}
	else
	{
		QString line = QStringLiteral( "driver=" ) + driver + QStringLiteral( ",node-name=" ) + node;
		if( ! file.isEmpty() )
			line += QStringLiteral( ",file=" ) + file;
		if( ! backing.isEmpty() )
			line += QStringLiteral( ",backing=" ) + backing;
		if( ! extra.isEmpty() )
			line += QLatin1Char( ',' ) + extra;
		Add_Line_As_Row( line );
	}
	Sync_Preview();
}

void Blockdev_Graph_Window::On_Remove()
{
	const int r = Table->currentRow();
	if( r >= 0 )
		Table->removeRow( r );
	Sync_Preview();
}

void Blockdev_Graph_Window::On_Selection_Changed()
{
	const int r = Table->currentRow();
	if( r >= 0 )
		Load_Row_To_Form( r );
}

void Blockdev_Graph_Window::Load_Row_To_Form( int row )
{
	QTableWidgetItem *it = Table->item( row, 0 );
	if( ! it ) return;
	Edit_Extra->setText( it->text() );
}

void Blockdev_Graph_Window::Sync_Preview()
{
	Preview->setPlainText( Get_Lines() );
}
