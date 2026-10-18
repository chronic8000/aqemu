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
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegExp>
#include <QSettings>

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
	Batch_List = nullptr;
	Log_View = nullptr;
	CH_Beside = nullptr;
	Edit_Output_Dir = nullptr;
	resize( width(), minimumSizeHint().height() );
	
	HDD_Info = new HDD_Image_Info();
	connect( HDD_Info, SIGNAL(Completed(bool)),
			 this, SLOT(Update_Info(bool)) );
	Valid_Info = false;
	
	//Possible_Encrypte = false;
	
	if( ! Get_QEMU_IMG_Info() )
	{
		AQWarning( "Convert_HDD_Image_Window::Convert_HDD_Image_Window( QWidget *parent )",
				   "Cannot get qemu-img info!" );
		
		QStringList formats = Probe_QEMU_IMG_Formats();
		if( formats.isEmpty() )
			formats << "qcow2" << "qcow" << "vmdk" << "vpc" << "vhdx" << "cow" << "raw" << "cloop";
		ui.CB_Output_Format->addItems( formats );
		const QString pref = Preferred_QEMU_IMG_Format( formats );
		const int ix = ui.CB_Output_Format->findText( pref, Qt::MatchFixedString );
	if( ix >= 0 )
		ui.CB_Output_Format->setCurrentIndex( ix );
	}

	QGridLayout *grid = qobject_cast<QGridLayout *>( layout() );
	if( ! grid )
		return;
	Batch_List = new QListWidget( this );
	CH_Beside = new QCheckBox( tr( "Write each image next to its source" ), this );
	Edit_Output_Dir = new QLineEdit( this );
	QPushButton *add = new QPushButton( tr( "Add images" ), this );
	QPushButton *dir_btn = new QPushButton( tr( "Output folder" ), this );
	Log_View = new QPlainTextEdit( this );
	Log_View->setReadOnly( true );
	Log_View->setPlaceholderText( tr( "qemu-img output appears here." ) );
	connect( add, SIGNAL(clicked()), this, SLOT(on_Button_Add_Images_clicked()) );
	connect( dir_btn, SIGNAL(clicked()), this, SLOT(on_Button_Output_Dir_clicked()) );
	grid->addWidget( new QLabel( tr( "More images" ), this ), 4, 0, 1, 3 );
	grid->addWidget( Batch_List, 5, 0, 1, 2 );
	grid->addWidget( add, 5, 2 );
	grid->addWidget( CH_Beside, 6, 0, 1, 3 );
	grid->addWidget( Edit_Output_Dir, 7, 0, 1, 2 );
	grid->addWidget( dir_btn, 7, 2 );
	grid->addWidget( Log_View, 8, 0, 1, 3 );
	resize( 520, 640 );
}

void Convert_HDD_Image_Window::on_Button_Browse_Base_clicked()
{
	QString fileName = QFileDialog::getOpenFileName( this, tr("Select Base HDD Image File"),
													 Get_Last_Dir_Path(ui.Edit_Base_File_Name->text()),
													 Disk_Image_File_Filter( false, false ) );
	
	if( fileName.isEmpty() ) return;

	fileName = QDir::toNativeSeparators( fileName );
	ui.Edit_Base_File_Name->setText( fileName );
	
	if( ! QFile::exists(fileName) )
	{
		AQGraphic_Warning( tr("Error!"), tr("Cannot Locate Input File!") );
		return;
	}
	
	// Get info about image
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
		ProgDial->accept();
		Conv_Thread.terminate();
		QMessageBox::information( this, tr("Information:"), tr("Conversion completed sucessfuly!") );
		accept();
	}
	else
	{
		ProgDial->accept();
		Conv_Thread.terminate();
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
