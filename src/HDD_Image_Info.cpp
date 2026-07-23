/****************************************************************************
**
** Copyright (C) 2009-2010 Andrey Rijov <ANDron142@yandex.ru>
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

#include <QFile>
#include <QSettings>
#include <QRegExp>
#include <QStringList>

#include "Utils.h"
#include "HDD_Image_Info.h"

HDD_Image_Info::HDD_Image_Info( QObject *parent )
		: QObject( parent )
{
	QEMU_IMG_Proc = new QProcess( this );
}

HDD_Image_Info::~HDD_Image_Info()
{
    delete QEMU_IMG_Proc;
}

VM::Disk_Info HDD_Image_Info::Get_Disk_Info() const
{
	return Info;
}

void HDD_Image_Info::Update_Disk_Info( const QString &path )
{
	Info.Image_File_Name = path;
	
	if( Info.Image_File_Name.isEmpty() )
	{
		Clear_Info();
		return;
	}
	
	if( QFile::exists(Info.Image_File_Name) == false )
	{
		AQWarning( "void HDD_Image_Info::Update_Disk_Info( const QString &path )",
                   "Image \"" + Info.Image_File_Name + "\" does not exist!" );
		Clear_Info();
		return;
	}
	else
	{
		QStringList args;
		args << "info" << Info.Image_File_Name;
		
		QEMU_IMG_Proc = new QProcess( this );
		QEMU_IMG_Proc->start( Get_QEMU_IMG_Path(), args );
		
		connect( QEMU_IMG_Proc, SIGNAL(finished(int, QProcess::ExitStatus)),
				 this, SLOT(Parse_Info(int, QProcess::ExitStatus)), Qt::DirectConnection );
		
		connect( QEMU_IMG_Proc, SIGNAL(error(QProcess::ProcessError)),
				 this, SLOT(Clear_Info()), Qt::DirectConnection );
	}
}

void HDD_Image_Info::Clear_Info()
{
	AQDebug( "void HDD_Image_Info::Clear_Info()",
			 "HDD Info Not Read!" );
	
	VM_HDD tmp_hdd;
	Info.Disk_Format = "";
	Info.Virtual_Size = tmp_hdd.String_to_Device_Size( "0G" );
	Info.Disk_Size = tmp_hdd.String_to_Device_Size( "0G" );
	Info.Cluster_Size = 0;
	
	emit Completed( false );
}

static QString Normalize_Qemu_Img_Size_Token( QString token )
{
	token = token.trimmed();
	// Strip trailing parenthetical byte count: "40 GiB (42949672960 bytes)"
	int paren = token.indexOf( '(' );
	if( paren > 0 )
		token = token.left( paren ).trimmed();
	// "40 GiB" / "40G" / "2.0M" → compact form for String_to_Device_Size
	token.replace( QRegExp( "\\s+" ), "" );
	token.replace( "GiB", "G", Qt::CaseInsensitive );
	token.replace( "MiB", "M", Qt::CaseInsensitive );
	token.replace( "KiB", "K", Qt::CaseInsensitive );
	token.replace( "TiB", "T", Qt::CaseInsensitive );
	token.replace( "bytes", "", Qt::CaseInsensitive );
	// Bare byte count (e.g. VMDK extent "virtual size: 21474836480") → human units
	bool ok_bytes = false;
	const qulonglong bytes = token.toULongLong( &ok_bytes );
	if( ok_bytes && ! token.contains( QRegExp( "[KMGT]" , Qt::CaseInsensitive ) ) )
	{
		if( bytes >= ( 1024ull * 1024ull * 1024ull ) && ( bytes % ( 1024ull * 1024ull * 1024ull ) ) == 0 )
			return QString::number( bytes / ( 1024ull * 1024ull * 1024ull ) ) + QLatin1Char( 'G' );
		if( bytes >= ( 1024ull * 1024ull ) && ( bytes % ( 1024ull * 1024ull ) ) == 0 )
			return QString::number( bytes / ( 1024ull * 1024ull ) ) + QLatin1Char( 'M' );
		if( bytes >= 1024ull && ( bytes % 1024ull ) == 0 )
			return QString::number( bytes / 1024ull ) + QLatin1Char( 'K' );
		// Non-aligned: express as fractional GiB for the UI
		const double gib = double( bytes ) / ( 1024.0 * 1024.0 * 1024.0 );
		return QString::number( gib, 'f', 2 ) + QLatin1Char( 'G' );
	}
	return token;
}

void HDD_Image_Info::Parse_Info( int exitCode, QProcess::ExitStatus exitStatus )
{
	Q_UNUSED( exitCode );
	Q_UNUSED( exitStatus );

	QByteArray info_str_ba = QEMU_IMG_Proc->readAllStandardOutput();
	if( info_str_ba.isEmpty() )
		info_str_ba = QEMU_IMG_Proc->readAll();
	QString info_str = QString::fromLocal8Bit( info_str_ba );
	if( info_str.isEmpty() )
	{
		AQDebug( "void HDD_Image_Info::Parse_Info(...)",
				 "Data is empty." );
		Clear_Info();
		return;
	}

	QString image_line;
	QString format;
	QString virtual_size;
	QString disk_size;
	QString cluster_size;

	const QStringList lines = info_str.split( QRegExp( "[\r\n]+" ), QString::SkipEmptyParts );
	for( int i = 0; i < lines.count(); ++i )
	{
		const QString line = lines.at( i ).trimmed();
		if( line.startsWith( "image:", Qt::CaseInsensitive ) )
		{
			if( image_line.isEmpty() )
				image_line = line.mid( 6 ).trimmed();
		}
		else if( line.startsWith( "file format:", Qt::CaseInsensitive ) )
		{
			if( format.isEmpty() )
				format = line.mid( QString( "file format:" ).length() ).trimmed();
		}
		else if( line.startsWith( "virtual size:", Qt::CaseInsensitive ) )
		{
			// VMDK (and some other formats) repeat "virtual size:" under extents as a
			// bare byte count — keep the first human-readable summary only.
			if( virtual_size.isEmpty() )
				virtual_size = Normalize_Qemu_Img_Size_Token( line.mid( QString( "virtual size:" ).length() ) );
		}
		else if( line.startsWith( "disk size:", Qt::CaseInsensitive ) )
		{
			if( disk_size.isEmpty() )
				disk_size = Normalize_Qemu_Img_Size_Token( line.mid( QString( "disk size:" ).length() ) );
		}
		else if( line.startsWith( "cluster_size:", Qt::CaseInsensitive ) )
		{
			if( cluster_size.isEmpty() )
				cluster_size = line.mid( QString( "cluster_size:" ).length() ).trimmed();
		}
	}

	if( format.isEmpty() || virtual_size.isEmpty() )
	{
		AQError( "void HDD_Image_Info::Parse_Info(...)",
				 "Cannot parse qemu-img info for: " + Info.Image_File_Name + "\nData:\n" + info_str );
		Clear_Info();
		return;
	}

	if( ! image_line.isEmpty() && image_line != Info.Image_File_Name )
	{
		AQWarning( "void HDD_Image_Info::Parse_Info(...)",
				   QString( "Reported image path differs:\n[[%1]]\n[[%2]]" )
					   .arg( image_line ).arg( Info.Image_File_Name ) );
	}

	Info.Disk_Format = format;
	VM_HDD tmp_hdd;
	Info.Virtual_Size = tmp_hdd.String_to_Device_Size( virtual_size );
	Info.Disk_Size = tmp_hdd.String_to_Device_Size( disk_size.isEmpty() ? virtual_size : disk_size );
	Info.Cluster_Size = cluster_size.isEmpty() ? 0 : cluster_size.toInt();

	emit Completed( true );
}
