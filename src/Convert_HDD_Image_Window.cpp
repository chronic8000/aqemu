/****************************************************************************
**
** Copyright (C) 2008-2010 Andrey Rijov <ANDron142@yandex.ru>
**
** This file is part of AQEMU.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor,
** Boston, MA  02110-1301, USA.
**
****************************************************************************/

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegExp>
#include <QSettings>
#include <QTabWidget>
#include <QVBoxLayout>

#include "Utils.h"
#include "VM_Devices.h"
#include "Convert_HDD_Image_Window.h"

// Convert_HDD_Thread -------------------------------------------------------

Convert_HDD_Thread::Convert_HDD_Thread()
{
	Error_Message = "";
}

void Convert_HDD_Thread::Set_Jobs( const QList<QStringList> &jobs )
{
	Jobs = jobs;
}

const QString &Convert_HDD_Thread::Get_Error_Message() const
{
	return Error_Message;
}

const QString &Convert_HDD_Thread::Get_Log() const
{
	return Log;
}

void Convert_HDD_Thread::run()
{
	Log.clear();
	Error_Message.clear();
	bool ok = true;
	for( int i = 0; i < Jobs.count(); ++i )
	{
		QProcess proc;
		proc.start( Get_QEMU_IMG_Path(), Jobs.at( i ) );
		Log += Get_QEMU_IMG_Path() + QLatin1String( " " ) + Jobs.at( i ).join( QLatin1Char( ' ' ) ) + QLatin1Char( '\n' );
		if( ! proc.waitForStarted( 2000 ) )
		{
			Log += QStringLiteral( "qemu-img did not start.\n" );
			Error_Message = QStringLiteral( "qemu-img did not start." );
			ok = false;
			break;
		}
		if( ! proc.waitForFinished( -1 ) )
		{
			proc.kill();
			Log += QStringLiteral( "qemu-img did not finish.\n" );
			Error_Message = QStringLiteral( "qemu-img did not finish." );
			ok = false;
			break;
		}
		const QString out = QString::fromLocal8Bit( proc.readAllStandardOutput() );
		const QString err = QString::fromLocal8Bit( proc.readAllStandardError() );
		if( ! out.isEmpty() )
			Log += out;
		if( ! err.isEmpty() )
			Log += err;
		if( proc.exitCode() != 0 )
		{
			Error_Message = err.isEmpty() ? QStringLiteral( "qemu-img failed." ) : err;
			ok = false;
			break;
		}
		Log += QStringLiteral( "Converted.\n" );
	}
	emit Conversion_Complete( ok );
}

//===========================================================================

Convert_HDD_Image_Window::Convert_HDD_Image_Window( QWidget *parent )
	: QDialog( parent )
{
	ui.setupUi( this );
	setWindowTitle( tr( "qemu-img Tools" ) );
	Batch_List = nullptr;
	Log_View = nullptr;
	CH_Beside = nullptr;
	Edit_Output_Dir = nullptr;
	CB_Preallocation = nullptr;
	Edit_Tool_Image = nullptr;
	SB_Resize_Size = nullptr;
	CB_Resize_Unit = nullptr;
	CH_Resize_Shrink = nullptr;
	CH_Check_Repair = nullptr;
	Snapshot_List = nullptr;
	Edit_Snapshot_Name = nullptr;
	ProgDial = nullptr;

	HDD_Info = new HDD_Image_Info();
	connect( HDD_Info, SIGNAL(Completed(bool)),
			 this, SLOT(Update_Info(bool)) );
	Valid_Info = false;

	if( ! Get_QEMU_IMG_Info() )
	{
		AQWarning( "Convert_HDD_Image_Window::Convert_HDD_Image_Window( QWidget *parent )",
				   "Cannot get qemu-img info!" );

		QStringList formats = Probe_QEMU_IMG_Formats();
		if( formats.isEmpty() )
			formats << "qcow2" << "qcow" << "vmdk" << "vpc" << "vhdx" << "vdi" << "cow" << "raw" << "cloop";
		ui.CB_Output_Format->addItems( formats );
		const QString pref = Preferred_QEMU_IMG_Format( formats );
		const int ix = ui.CB_Output_Format->findText( pref, Qt::MatchFixedString );
		if( ix >= 0 )
			ui.CB_Output_Format->setCurrentIndex( ix );
	}

	// Preallocation option on the convert page
	CB_Preallocation = new QComboBox( this );
	CB_Preallocation->addItem( tr( "off (default)" ), QStringLiteral( "off" ) );
	CB_Preallocation->addItem( tr( "metadata" ), QStringLiteral( "metadata" ) );
	CB_Preallocation->addItem( tr( "falloc" ), QStringLiteral( "falloc" ) );
	CB_Preallocation->addItem( tr( "full" ), QStringLiteral( "full" ) );
	if( QLayout *out_lay = ui.groupBox_2->layout() )
	{
		if( QGridLayout *og = qobject_cast<QGridLayout *>( out_lay ) )
		{
			og->addWidget( new QLabel( tr( "Preallocation:" ), this ), 5, 0 );
			og->addWidget( CB_Preallocation, 5, 1, 1, 2 );
		}
	}

	QGridLayout *old_grid = qobject_cast<QGridLayout *>( layout() );
	if( ! old_grid )
		return;

	// Batch convert controls (stay on Convert tab)
	Batch_List = new QListWidget( this );
	CH_Beside = new QCheckBox( tr( "Write each image next to its source" ), this );
	Edit_Output_Dir = new QLineEdit( this );
	QPushButton *add = new QPushButton( tr( "Add images" ), this );
	QPushButton *dir_btn = new QPushButton( tr( "Output folder" ), this );
	connect( add, SIGNAL(clicked()), this, SLOT(on_Button_Add_Images_clicked()) );
	connect( dir_btn, SIGNAL(clicked()), this, SLOT(on_Button_Output_Dir_clicked()) );

	QWidget *convert_page = new QWidget( this );
	QVBoxLayout *convert_lay = new QVBoxLayout( convert_page );
	convert_lay->setContentsMargins( 6, 6, 6, 6 );

	// Re-home the designer convert widgets onto the Convert tab.
	ui.groupBox->setParent( convert_page );
	ui.groupBox_2->setParent( convert_page );
	ui.Button_Convert->setParent( convert_page );
	convert_lay->addWidget( ui.groupBox );
	convert_lay->addWidget( ui.groupBox_2 );
	convert_lay->addWidget( new QLabel( tr( "More images (batch)" ), convert_page ) );
	convert_lay->addWidget( Batch_List );
	QHBoxLayout *batch_btns = new QHBoxLayout();
	batch_btns->addWidget( add );
	batch_btns->addWidget( CH_Beside );
	batch_btns->addStretch();
	convert_lay->addLayout( batch_btns );
	QHBoxLayout *dir_row = new QHBoxLayout();
	dir_row->addWidget( Edit_Output_Dir );
	dir_row->addWidget( dir_btn );
	convert_lay->addLayout( dir_row );
	QHBoxLayout *go_row = new QHBoxLayout();
	go_row->addStretch();
	go_row->addWidget( ui.Button_Convert );
	convert_lay->addLayout( go_row );

	QTabWidget *tabs = new QTabWidget( this );
	tabs->addTab( convert_page, tr( "Convert" ) );
	tabs->addTab( Build_Resize_Tab(), tr( "Resize" ) );
	tabs->addTab( Build_Info_Check_Tab(), tr( "Info / Check" ) );
	tabs->addTab( Build_Snapshots_Tab(), tr( "Snapshots" ) );

	Log_View = new QPlainTextEdit( this );
	Log_View->setReadOnly( true );
	Log_View->setPlaceholderText( tr( "qemu-img output appears here." ) );
	Log_View->setMinimumHeight( 140 );

	QPushButton *close_btn = ui.Button_Cancel;
	close_btn->setText( tr( "Close" ) );
	close_btn->setParent( this );

	// Replace the original dialog layout with tabs + shared console.
	QLayoutItem *child;
	while( ( child = old_grid->takeAt( 0 ) ) != nullptr )
	{
		if( child->widget() && child->widget() != close_btn )
		{
			// Already reparented convert widgets; ignore leftovers.
		}
		delete child;
	}
	delete layout();

	QVBoxLayout *root = new QVBoxLayout( this );
	root->addWidget( tabs, 1 );
	root->addWidget( new QLabel( tr( "Console" ), this ) );
	root->addWidget( Log_View );
	QHBoxLayout *bottom = new QHBoxLayout();
	bottom->addStretch();
	bottom->addWidget( close_btn );
	root->addLayout( bottom );

	resize( 640, 720 );
}

QWidget *Convert_HDD_Image_Window::Build_Resize_Tab()
{
	QWidget *page = new QWidget( this );
	QVBoxLayout *lay = new QVBoxLayout( page );
	QHBoxLayout *file_row = new QHBoxLayout();
	Edit_Tool_Image = new QLineEdit( page );
	QPushButton *browse = new QPushButton( page );
	browse->setIcon( QIcon( QStringLiteral( ":/open-file.png" ) ) );
	connect( browse, SIGNAL(clicked()), this, SLOT(Browse_Tool_Image()) );
	file_row->addWidget( new QLabel( tr( "Image:" ), page ) );
	file_row->addWidget( Edit_Tool_Image, 1 );
	file_row->addWidget( browse );
	lay->addLayout( file_row );

	QHBoxLayout *size_row = new QHBoxLayout();
	SB_Resize_Size = new QDoubleSpinBox( page );
	SB_Resize_Size->setRange( 0.001, 1048576.0 );
	SB_Resize_Size->setDecimals( 3 );
	SB_Resize_Size->setValue( 20.0 );
	CB_Resize_Unit = new QComboBox( page );
	CB_Resize_Unit->addItems( QStringList() << QStringLiteral( "G" ) << QStringLiteral( "M" ) << QStringLiteral( "K" ) );
	CH_Resize_Shrink = new QCheckBox( tr( "Allow shrink (destructive)" ), page );
	size_row->addWidget( new QLabel( tr( "New size:" ), page ) );
	size_row->addWidget( SB_Resize_Size );
	size_row->addWidget( CB_Resize_Unit );
	size_row->addWidget( CH_Resize_Shrink );
	size_row->addStretch();
	lay->addLayout( size_row );

	QPushButton *go = new QPushButton( tr( "Resize" ), page );
	go->setIcon( QIcon( QStringLiteral( ":/go.png" ) ) );
	connect( go, SIGNAL(clicked()), this, SLOT(Run_Resize()) );
	lay->addWidget( go, 0, Qt::AlignRight );
	lay->addStretch();
	lay->addWidget( new QLabel(
		tr( "Grow is safe. Shrink requires the guest filesystem to be smaller first. "
		    "AQEMU never overwrites a different file; resize edits this image in place." ),
		page ) );
	return page;
}

QWidget *Convert_HDD_Image_Window::Build_Info_Check_Tab()
{
	QWidget *page = new QWidget( this );
	QVBoxLayout *lay = new QVBoxLayout( page );
	lay->addWidget( new QLabel(
		tr( "Uses the image path from the Resize tab (or pick one below if empty)." ), page ) );

	QHBoxLayout *btns = new QHBoxLayout();
	QPushButton *info = new QPushButton( tr( "Info" ), page );
	QPushButton *check = new QPushButton( tr( "Check" ), page );
	CH_Check_Repair = new QCheckBox( tr( "Repair (-r all)" ), page );
	connect( info, SIGNAL(clicked()), this, SLOT(Run_Info()) );
	connect( check, SIGNAL(clicked()), this, SLOT(Run_Check()) );
	btns->addWidget( info );
	btns->addWidget( check );
	btns->addWidget( CH_Check_Repair );
	btns->addStretch();
	lay->addLayout( btns );
	lay->addStretch();
	return page;
}

QWidget *Convert_HDD_Image_Window::Build_Snapshots_Tab()
{
	QWidget *page = new QWidget( this );
	QVBoxLayout *lay = new QVBoxLayout( page );
	lay->addWidget( new QLabel(
		tr( "Internal qcow2 snapshots (qemu-img snapshot). Uses the Resize tab image path." ), page ) );

	Snapshot_List = new QListWidget( page );
	lay->addWidget( Snapshot_List, 1 );

	QHBoxLayout *name_row = new QHBoxLayout();
	Edit_Snapshot_Name = new QLineEdit( page );
	Edit_Snapshot_Name->setPlaceholderText( tr( "Snapshot name" ) );
	name_row->addWidget( Edit_Snapshot_Name, 1 );
	lay->addLayout( name_row );

	QHBoxLayout *btns = new QHBoxLayout();
	QPushButton *refresh = new QPushButton( tr( "Refresh" ), page );
	QPushButton *create = new QPushButton( tr( "Create" ), page );
	QPushButton *apply = new QPushButton( tr( "Apply" ), page );
	QPushButton *del = new QPushButton( tr( "Delete" ), page );
	connect( refresh, SIGNAL(clicked()), this, SLOT(Snapshot_Refresh()) );
	connect( create, SIGNAL(clicked()), this, SLOT(Snapshot_Create()) );
	connect( apply, SIGNAL(clicked()), this, SLOT(Snapshot_Apply()) );
	connect( del, SIGNAL(clicked()), this, SLOT(Snapshot_Delete()) );
	btns->addWidget( refresh );
	btns->addWidget( create );
	btns->addWidget( apply );
	btns->addWidget( del );
	btns->addStretch();
	lay->addLayout( btns );
	return page;
}

void Convert_HDD_Image_Window::Append_Log( const QString &text )
{
	if( ! Log_View )
		return;
	if( ! Log_View->toPlainText().isEmpty() )
		Log_View->appendPlainText( QString() );
	Log_View->appendPlainText( text );
}

QString Convert_HDD_Image_Window::Tool_Image_Path() const
{
	if( Edit_Tool_Image && ! Edit_Tool_Image->text().trimmed().isEmpty() )
		return Edit_Tool_Image->text().trimmed();
	return ui.Edit_Base_File_Name->text().trimmed();
}

bool Convert_HDD_Image_Window::Run_Qemu_Img( const QStringList &args, QString *out, QString *err )
{
	QProcess proc;
	const QString bin = Get_QEMU_IMG_Path();
	Append_Log( bin + QLatin1Char( ' ' ) + args.join( QLatin1Char( ' ' ) ) );
	proc.start( bin, args );
	if( ! proc.waitForStarted( 3000 ) )
	{
		if( err )
			*err = tr( "qemu-img did not start." );
		Append_Log( tr( "qemu-img did not start." ) );
		return false;
	}
	if( ! proc.waitForFinished( -1 ) )
	{
		proc.kill();
		if( err )
			*err = tr( "qemu-img did not finish." );
		Append_Log( tr( "qemu-img did not finish." ) );
		return false;
	}
	const QString o = QString::fromLocal8Bit( proc.readAllStandardOutput() );
	const QString e = QString::fromLocal8Bit( proc.readAllStandardError() );
	if( out )
		*out = o;
	if( err )
		*err = e;
	if( ! o.isEmpty() )
		Append_Log( o );
	if( ! e.isEmpty() )
		Append_Log( e );
	return proc.exitCode() == 0;
}

void Convert_HDD_Image_Window::Browse_Tool_Image()
{
	QString fileName = QFileDialog::getOpenFileName( this, tr( "Select disk image" ),
		Get_Last_Dir_Path( Tool_Image_Path() ), Disk_Image_File_Filter( false, false ) );
	if( fileName.isEmpty() )
		return;
	fileName = QDir::toNativeSeparators( fileName );
	if( Edit_Tool_Image )
		Edit_Tool_Image->setText( fileName );
	if( ui.Edit_Base_File_Name->text().trimmed().isEmpty() )
		ui.Edit_Base_File_Name->setText( fileName );
}

void Convert_HDD_Image_Window::Run_Info()
{
	const QString path = Tool_Image_Path();
	if( path.isEmpty() || ! QFile::exists( path ) )
	{
		AQGraphic_Warning( tr( "Image" ), tr( "Choose an existing disk image." ) );
		return;
	}
	QString out, err;
	if( ! Run_Qemu_Img( QStringList() << QStringLiteral( "info" ) << path, &out, &err ) )
		AQGraphic_Warning( tr( "qemu-img info" ), err.isEmpty() ? tr( "info failed." ) : err );
}

void Convert_HDD_Image_Window::Run_Check()
{
	const QString path = Tool_Image_Path();
	if( path.isEmpty() || ! QFile::exists( path ) )
	{
		AQGraphic_Warning( tr( "Image" ), tr( "Choose an existing disk image." ) );
		return;
	}
	QStringList args;
	args << QStringLiteral( "check" );
	if( CH_Check_Repair && CH_Check_Repair->isChecked() )
		args << QStringLiteral( "-r" ) << QStringLiteral( "all" );
	args << path;
	QString out, err;
	if( ! Run_Qemu_Img( args, &out, &err ) )
		AQGraphic_Warning( tr( "qemu-img check" ), err.isEmpty() ? tr( "check reported problems." ) : err );
	else
		QMessageBox::information( this, tr( "qemu-img check" ), tr( "Check completed." ) );
}

void Convert_HDD_Image_Window::Run_Resize()
{
	const QString path = Tool_Image_Path();
	if( path.isEmpty() || ! QFile::exists( path ) )
	{
		AQGraphic_Warning( tr( "Image" ), tr( "Choose an existing disk image." ) );
		return;
	}
	if( ! SB_Resize_Size || ! CB_Resize_Unit )
		return;
	const QString size = QString::number( SB_Resize_Size->value(), 'f', 3 )
		+ CB_Resize_Unit->currentText();
	QStringList args;
	args << QStringLiteral( "resize" );
	if( CH_Resize_Shrink && CH_Resize_Shrink->isChecked() )
		args << QStringLiteral( "--shrink" );
	args << path << size;
	QString out, err;
	if( ! Run_Qemu_Img( args, &out, &err ) )
		AQGraphic_Warning( tr( "qemu-img resize" ), err.isEmpty() ? tr( "resize failed." ) : err );
	else
		QMessageBox::information( this, tr( "qemu-img resize" ), tr( "Resize completed." ) );
}

void Convert_HDD_Image_Window::Snapshot_Refresh()
{
	const QString path = Tool_Image_Path();
	if( path.isEmpty() || ! QFile::exists( path ) )
	{
		AQGraphic_Warning( tr( "Image" ), tr( "Choose an existing disk image." ) );
		return;
	}
	QString out, err;
	Run_Qemu_Img( QStringList() << QStringLiteral( "snapshot" ) << QStringLiteral( "-l" ) << path, &out, &err );
	if( ! Snapshot_List )
		return;
	Snapshot_List->clear();
	const QStringList lines = out.split( QRegExp( QStringLiteral( "[\\r\\n]+" ) ), QString::SkipEmptyParts );
	for( int i = 0; i < lines.count(); ++i )
	{
		const QString line = lines.at( i ).trimmed();
		if( line.startsWith( QLatin1Char( '-' ) ) ||
		    line.contains( QLatin1String( "Snapshot list" ), Qt::CaseInsensitive ) ||
		    ( line.contains( QLatin1String( "ID" ), Qt::CaseInsensitive ) &&
		      line.contains( QLatin1String( "TAG" ), Qt::CaseInsensitive ) ) )
			continue;
		Snapshot_List->addItem( line );
	}
}

void Convert_HDD_Image_Window::Snapshot_Create()
{
	const QString path = Tool_Image_Path();
	const QString name = Edit_Snapshot_Name ? Edit_Snapshot_Name->text().trimmed() : QString();
	if( path.isEmpty() || ! QFile::exists( path ) )
	{
		AQGraphic_Warning( tr( "Image" ), tr( "Choose an existing disk image." ) );
		return;
	}
	if( name.isEmpty() )
	{
		AQGraphic_Warning( tr( "Snapshot" ), tr( "Enter a snapshot name." ) );
		return;
	}
	QString out, err;
	if( ! Run_Qemu_Img( QStringList() << QStringLiteral( "snapshot" ) << QStringLiteral( "-c" ) << name << path, &out, &err ) )
		AQGraphic_Warning( tr( "Create snapshot" ), err.isEmpty() ? tr( "create failed." ) : err );
	else
		Snapshot_Refresh();
}

void Convert_HDD_Image_Window::Snapshot_Apply()
{
	const QString path = Tool_Image_Path();
	QString name = Edit_Snapshot_Name ? Edit_Snapshot_Name->text().trimmed() : QString();
	if( name.isEmpty() && Snapshot_List && Snapshot_List->currentItem() )
	{
		const QString line = Snapshot_List->currentItem()->text();
		const QStringList parts = line.split( QRegExp( QStringLiteral( "\\s+" ) ), QString::SkipEmptyParts );
		if( parts.count() >= 2 )
			name = parts.at( 1 );
	}
	if( path.isEmpty() || name.isEmpty() )
	{
		AQGraphic_Warning( tr( "Apply snapshot" ), tr( "Choose an image and snapshot name." ) );
		return;
	}
	QString out, err;
	if( ! Run_Qemu_Img( QStringList() << QStringLiteral( "snapshot" ) << QStringLiteral( "-a" ) << name << path, &out, &err ) )
		AQGraphic_Warning( tr( "Apply snapshot" ), err.isEmpty() ? tr( "apply failed." ) : err );
}

void Convert_HDD_Image_Window::Snapshot_Delete()
{
	const QString path = Tool_Image_Path();
	QString name = Edit_Snapshot_Name ? Edit_Snapshot_Name->text().trimmed() : QString();
	if( name.isEmpty() && Snapshot_List && Snapshot_List->currentItem() )
	{
		const QString line = Snapshot_List->currentItem()->text();
		const QStringList parts = line.split( QRegExp( QStringLiteral( "\\s+" ) ), QString::SkipEmptyParts );
		if( parts.count() >= 2 )
			name = parts.at( 1 );
	}
	if( path.isEmpty() || name.isEmpty() )
	{
		AQGraphic_Warning( tr( "Delete snapshot" ), tr( "Choose an image and snapshot name." ) );
		return;
	}
	QString out, err;
	if( ! Run_Qemu_Img( QStringList() << QStringLiteral( "snapshot" ) << QStringLiteral( "-d" ) << name << path, &out, &err ) )
		AQGraphic_Warning( tr( "Delete snapshot" ), err.isEmpty() ? tr( "delete failed." ) : err );
	else
		Snapshot_Refresh();
}

void Convert_HDD_Image_Window::on_Button_Browse_Base_clicked()
{
	QString fileName = QFileDialog::getOpenFileName( this, tr("Select Base HDD Image File"),
													 Get_Last_Dir_Path(ui.Edit_Base_File_Name->text()),
													 Disk_Image_File_Filter( false, false ) );

	if( fileName.isEmpty() ) return;

	fileName = QDir::toNativeSeparators( fileName );
	ui.Edit_Base_File_Name->setText( fileName );
	if( Edit_Tool_Image && Edit_Tool_Image->text().trimmed().isEmpty() )
		Edit_Tool_Image->setText( fileName );

	if( ! QFile::exists(fileName) )
	{
		AQGraphic_Warning( tr("Error!"), tr("Cannot Locate Input File!") );
		return;
	}

	HDD_Info->Update_Disk_Info( fileName );
}

void Convert_HDD_Image_Window::Update_Info( bool ok )
{
	Valid_Info = ok;

	if( ok )
	{
		ui.Label_HDD_Info_L->setText( tr("Image Format: %1\nAllocated Disk Space: %2 %3")
									  .arg(HDD_Info->Get_Disk_Info().Disk_Format)
									  .arg(HDD_Info->Get_Disk_Info().Disk_Size.Size)
									  .arg(Get_TR_Size_Suffix(HDD_Info->Get_Disk_Info().Disk_Size)) );

		ui.Label_HDD_Info_R->setText( tr("Virtual Size: %1 %2\nCluster Size: %3")
									  .arg(HDD_Info->Get_Disk_Info().Virtual_Size.Size)
									  .arg(Get_TR_Size_Suffix(HDD_Info->Get_Disk_Info().Virtual_Size))
									  .arg(HDD_Info->Get_Disk_Info().Cluster_Size) );
	}
	else
	{
		ui.Label_HDD_Info_L->setText( "Image Format: none\nAllocated Disk Space: 0" );
		ui.Label_HDD_Info_R->setText( "Virtual Size: 0\nCluster Size: 0" );
	}
}

void Convert_HDD_Image_Window::on_Button_Browse_Output_clicked()
{
	QString fileName = QFileDialog::getSaveFileName( this, tr("Save Out HDD Image File"),
													 Get_Last_Dir_Path(ui.Edit_Output_File_Name->text()),
													 Disk_Image_File_Filter( false, false ) );

	if( ! fileName.isEmpty() )
		ui.Edit_Output_File_Name->setText( QDir::toNativeSeparators(fileName) );
}

void Convert_HDD_Image_Window::on_CB_Output_Format_currentIndexChanged( const QString &text )
{
	if( text.contains("qcow", Qt::CaseInsensitive) )
	{
		ui.CH_Compressed->setEnabled( true );
	}
	else
	{
		ui.CH_Compressed->setEnabled( false );
	}
}

static QString Convert_Extension( const QString &format )
{
	const QString f = format.trimmed().toLower();
	if( f == QLatin1String( "raw" ) )
		return QStringLiteral( "img" );
	if( f == QLatin1String( "vpc" ) )
		return QStringLiteral( "vhd" );
	if( f == QLatin1String( "vdi" ) )
		return QStringLiteral( "vdi" );
	return f;
}

void Convert_HDD_Image_Window::on_Button_Add_Images_clicked()
{
	const QStringList files = QFileDialog::getOpenFileNames( this, tr( "Add disk images" ),
		Get_Last_Dir_Path( ui.Edit_Base_File_Name->text() ), Disk_Image_File_Filter( false, false ) );
	for( int i = 0; i < files.count(); ++i )
		Batch_List->addItem( QDir::toNativeSeparators( files.at( i ) ) );
}

void Convert_HDD_Image_Window::on_Button_Output_Dir_clicked()
{
	const QString dir = QFileDialog::getExistingDirectory( this, tr( "Output folder" ),
		Get_Last_Dir_Path( Edit_Output_Dir->text() ) );
	if( ! dir.isEmpty() )
		Edit_Output_Dir->setText( QDir::toNativeSeparators( dir ) );
}

void Convert_HDD_Image_Window::on_Button_Convert_clicked()
{
	QStringList sources;
	if( ! ui.Edit_Base_File_Name->text().trimmed().isEmpty() )
		sources << ui.Edit_Base_File_Name->text().trimmed();
	if( Batch_List )
	{
		for( int i = 0; i < Batch_List->count(); ++i )
			sources << Batch_List->item( i )->text();
	}
	sources.removeDuplicates();
	if( sources.isEmpty() )
	{
		AQGraphic_Warning( tr( "File Name is Empty!" ), tr( "Choose at least one disk image." ) );
		return;
	}
	const QString format = ui.CB_Output_Format->currentText().trimmed();
	if( format.isEmpty() )
	{
		AQGraphic_Warning( tr( "Error!" ), tr( "Choose an output format." ) );
		return;
	}
	const bool beside = CH_Beside && CH_Beside->isChecked();
	const QString out_dir = Edit_Output_Dir ? Edit_Output_Dir->text().trimmed() : QString();
	const QString prealloc = CB_Preallocation
		? CB_Preallocation->currentData().toString() : QStringLiteral( "off" );
	QList<QStringList> jobs;
	QString refused;
	for( int i = 0; i < sources.count(); ++i )
	{
		const QFileInfo src( sources.at( i ) );
		QString dest;
		if( beside || sources.count() > 1 )
		{
			const QString folder = beside ? src.absolutePath() : out_dir;
			if( folder.isEmpty() )
			{
				AQGraphic_Warning( tr( "Output folder" ), tr( "Choose an output folder, or write each image next to its source." ) );
				return;
			}
			dest = QDir( folder ).filePath( src.completeBaseName() + QLatin1Char( '.' ) + Convert_Extension( format ) );
		}
		else
			dest = ui.Edit_Output_File_Name->text().trimmed();
		if( dest.isEmpty() )
		{
			AQGraphic_Warning( tr( "File Name is Empty!" ), tr( "Please Enter Correct HDD Image filename!" ) );
			return;
		}
		if( QFileInfo( dest ) == src || QFileInfo( dest ).absoluteFilePath() == src.absoluteFilePath() )
		{
			refused += tr( "Refused to replace the source %1.\n" ).arg( src.absoluteFilePath() );
			continue;
		}
		if( QFileInfo::exists( dest ) )
		{
			refused += tr( "Refused to overwrite %1.\n" ).arg( dest );
			continue;
		}
		QStringList args;
		args << QStringLiteral( "convert" );
		if( ui.CH_Compressed->isEnabled() && ui.CH_Compressed->isChecked() )
			args << QStringLiteral( "-c" );
		args << QStringLiteral( "-O" ) << format;
		if( ! prealloc.isEmpty() && prealloc != QLatin1String( "off" ) )
			args << QStringLiteral( "-o" ) << ( QStringLiteral( "preallocation=" ) + prealloc );
		args << src.absoluteFilePath();
		args << dest;
		jobs << args;
	}
	if( Log_View )
		Log_View->setPlainText( refused );
	if( jobs.isEmpty() )
	{
		AQGraphic_Warning( tr( "Nothing to convert" ), refused.isEmpty() ? tr( "No image was converted." ) : refused );
		return;
	}

	connect( &Conv_Thread, SIGNAL(Conversion_Complete(bool)),
			 this, SLOT(Conversion_Done(bool)), Qt::UniqueConnection );

	Conv_Thread.Set_Jobs( jobs );
	Conv_Thread.start();

	ProgDial = new QProgressDialog( tr("Please Wait. Converting HDD Image..."), tr("Cancel"), 0, 0, this );

	connect( ProgDial, SIGNAL(canceled()),
			 this, SLOT(Cancel_Convertion()) );

	ProgDial->setWindowModality( Qt::WindowModal );
	ProgDial->setValue( 0 );

	ProgDial->exec();
}

void Convert_HDD_Image_Window::Conversion_Done( bool ok )
{
	if( Log_View )
		Log_View->appendPlainText( Conv_Thread.Get_Log() );
	if( ok )
	{
		if( ProgDial )
			ProgDial->accept();
		QMessageBox::information( this, tr("Information:"), tr("Conversion completed successfully!") );
	}
	else
	{
		if( ProgDial )
			ProgDial->accept();
		AQGraphic_Warning( tr("Error!"),
						   tr("Cannot convert image!\nDetails:\n%1").arg(Conv_Thread.Get_Error_Message()) );
	}
}

void Convert_HDD_Image_Window::Cancel_Convertion()
{
	Conv_Thread.terminate();
}

bool Convert_HDD_Image_Window::Get_QEMU_IMG_Info()
{
	const QStringList formatsList = Probe_QEMU_IMG_Formats();
	if( formatsList.isEmpty() )
	{
		AQError( "bool Convert_HDD_Image_Window::Get_QEMU_IMG_Info()",
				 "No formats from qemu-img!" );
		return false;
	}

	ui.CB_Output_Format->clear();
	ui.CB_Output_Format->addItems( formatsList );

	const QString pref = Preferred_QEMU_IMG_Format( formatsList );
	const int ix = ui.CB_Output_Format->findText( pref, Qt::MatchFixedString );
	if( ix >= 0 )
		ui.CB_Output_Format->setCurrentIndex( ix );

	return true;
}
