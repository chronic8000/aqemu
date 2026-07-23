/****************************************************************************
**
** Copyright (C) 2008-2010 Andrey Rijov <ANDron142@yandex.ru>
** Copyright (C) 2016 Tobias Gläßer
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

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QDateTime>
#include <QMessageBox>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QCoreApplication>
#include <QSettings>
#include <QRegExp>
#include <QRegularExpression>
#include <QHash>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStringList>
#include <QTextStream>
#include <QObject>
#include <QSysInfo>
#include <QTemporaryDir>

#include <cmath>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

#ifdef Q_OS_WIN32
#include <iostream>
#include <windows.h>
HANDLE Console_HANDLE = GetStdHandle( STD_OUTPUT_HANDLE );
#else
#include <QDebug>
#include <iostream>
#endif

#include "Utils.h"
#include "System_Info.h"

static uint Messages_Index = 0;

static bool Save_Messages_To_Log = true;
static QString Log_Path = "";

static bool Use_Stdout;
static bool Stdout_Debug;
static bool Stdout_Warning;
static bool Stdout_Error;

static bool Show_User_Graphic_Warning = true;

static QStringList Recent_CD_Images;
static QStringList Recent_FDD_Images;

Disable_User_Graphic_Warning::Disable_User_Graphic_Warning()
{
    Show_User_Graphic_Warning = false;
}
Disable_User_Graphic_Warning::~Disable_User_Graphic_Warning()
{
    Show_User_Graphic_Warning = true;
}

void AQDebugStdCout(const QString& s)
{
    std::cout << s.toLatin1().constData() << std::endl;
}

void AQEMU_Startup_Log( const char *stage )
{
	if( ! stage )
		return;
	std::cout << "[AQEMU] " << stage << std::endl;
	std::cout.flush();
#ifdef Q_OS_WIN32
	if( Console_HANDLE != INVALID_HANDLE_VALUE )
		SetConsoleTextAttribute( Console_HANDLE, 7 );
#endif
}

void AQDebug( const QString &sender, const QString &mes )
{
	if( Use_Stdout && Stdout_Debug )
	{
	#ifdef Q_OS_WIN32
		SetConsoleTextAttribute( Console_HANDLE, 10 );
		std::cout << QString( "\nAQEMU Debug [%1] >>>\nSender: %2\nMessage: %3" )
							  .arg(Messages_Index).arg(sender).arg(mes).toStdString();
	#else
		AQDebugStdCout( QString(
			"\n\33[32mAQEMU Debug\33[0m [%1] >>>\n\33[32mSender:\33[0m %2\n\33[32mMessage:\33[0m %3")
			.arg(Messages_Index).arg(sender).arg(mes));
	#endif
	}
	
	if( Save_Messages_To_Log && Stdout_Debug )
		AQSave_To_Log( "Debug", sender, mes );
	
	Messages_Index++;
}

void AQWarning( const QString &sender, const QString &mes )
{
	if( Use_Stdout && Stdout_Warning )
	{
	#ifdef Q_OS_WIN32
		SetConsoleTextAttribute( Console_HANDLE, 14 );
		std::cout << QString( "\nAQEMU Warning [%1] >>>\nSender: %2\nMessage: %3" )
							  .arg(Messages_Index).arg(sender).arg(mes).toStdString();
	#else
		AQDebugStdCout( QString(
			"\n\33[34mAQEMU Warning\33[0m [%1] >>>\n\33[34mSender:\33[0m %2\n\33[34mMessage:\33[0m %3")
			.arg(Messages_Index).arg(sender).arg(mes));
	#endif
	}
	
	if( Save_Messages_To_Log && Stdout_Warning )
		AQSave_To_Log( "Warning", sender, mes );
	
	Messages_Index++;
}

void AQError( const QString &sender, const QString &mes )
{
	if( Use_Stdout && Stdout_Error )
	{
	#ifdef Q_OS_WIN32
		SetConsoleTextAttribute( Console_HANDLE, 12 );
		std::cout << QString( "\nAQEMU Error [%1] >>>\nSender: %2\nMessage: %3" )
							  .arg(Messages_Index).arg(sender).arg(mes).toStdString();
	#else
		AQDebugStdCout( QString(
			"\n\33[31mAQEMU Error\33[0m [%1] >>>\n\33[31mSender:\33[0m %2\n\33[31mMessage:\33[0m %3")
			.arg(Messages_Index).arg(sender).arg(mes));
	#endif
	}
	
	if( Save_Messages_To_Log && Stdout_Error )
		AQSave_To_Log( "Error", sender, mes );
	
	Messages_Index++;
}

void AQGraphic_Warning( const QString &caption, const QString &mes )
{
    if ( Show_User_Graphic_Warning == false )
        return;

	QMessageBox::warning( NULL, caption, mes, QMessageBox::Ok );
}

void AQGraphic_Warning( const QString &sender, const QString &caption, const QString &mes, bool fatal )
{
    if ( Show_User_Graphic_Warning == false )
        return;

	if( fatal )
	{
		QMessageBox::warning( NULL, caption,
					QString("Sender: %1\nMessage: %2\n").arg(sender).arg(mes) +
					QObject::tr("This Fatal Error. Recomend Close AQEMU."),
					QMessageBox::Ok );
	}
	else
	{
		QMessageBox::warning( NULL, caption,
					QString("Sender: %1\nMessage: %2").arg(sender).arg(mes),
					QMessageBox::Ok );
	}
	
	if( Save_Messages_To_Log )
		AQSave_To_Log( "Warning", sender, mes );
}

void AQGraphic_Error( const QString &sender, const QString &caption, const QString &mes, bool fatal )
{
	if( fatal )
	{
		QMessageBox::critical( NULL, caption,
					QString("Sender: %1\nMessage: %2\n").arg(sender).arg(mes) +
					QObject::tr("Fatal error. It's recommended to close AQEMU"),
					QMessageBox::Ok );
	}
	else
	{
		QMessageBox::critical( NULL, caption,
					QString("Sender: %1\nMessage: %2").arg(sender).arg(mes),
					QMessageBox::Ok );
	}
	
	if( Save_Messages_To_Log )
		AQSave_To_Log( "Error", sender, mes );
}

void AQUse_Log( bool use )
{
	Save_Messages_To_Log = use;
}

void AQUse_Debug_Output( bool use, bool d, bool w, bool e )
{
	Use_Stdout = use;
	Stdout_Debug = d;
	Stdout_Warning = w;
	Stdout_Error = e;
}

void AQLog_Path( const QString& path )
{
	Log_Path = path;
}

void AQSave_To_Log( const QString &mes_type, const QString &sender, const QString &mes )
{
	if( Log_Path.isEmpty() ) return;

	QFile log_file( Log_Path );
	
	if( ! log_file.open(QIODevice::Append | QIODevice::Text) )
	{
		AQUse_Log( false ); // off loging
		AQError( "void AQSave_To_Log( const QString& mes_type, const QString& sender, const QString& mes )",
				 "Cannot Open Log file to Write! Log Path: \"" + Log_Path + "\"" );
	}
	else
	{
		QTextStream out( &log_file );
		out << "Type: " << mes_type << " Num: " << Messages_Index << "\nDate: "
			<< QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss zzz")
			<< "\nSender: " << sender << "\nMessage: " << mes << "\n\n";
	}
}

QString Get_QEMU_IMG_Path()
{
	QSettings settings;
	QString configured = settings.value( "QEMU-IMG_Path", "" ).toString().trimmed();

#ifdef Q_OS_WIN32
	const QString exe_suffix = ".exe";
#else
	const QString exe_suffix = "";
#endif

	auto path_ok = []( const QString &p ) -> bool {
		return ! p.isEmpty() && QFile::exists( p ) && QFileInfo( p ).isFile();
	};

	if( path_ok( configured ) )
		return QDir::toNativeSeparators( configured );

	// Bare name like "qemu-img" is not usable on Windows without PATH — rediscover.
	QStringList candidates;

	// Bundled next to aqemu (AQEMU_BUNDLE_QEMU / submodule install)
	const QString app_dir = QCoreApplication::applicationDirPath();
	candidates << app_dir + QDir::separator() + "qemu-img" + exe_suffix;
	candidates << app_dir + QDir::separator() + "qemu" + QDir::separator() + "qemu-img" + exe_suffix;

	// Next to any configured emulator binary
	const Emulator &emul = Get_Default_Emulator();
	QMap<QString, QString> bins = emul.Get_Binary_Files();
	for( QMap<QString, QString>::const_iterator it = bins.constBegin(); it != bins.constEnd(); ++it )
	{
		if( it.value().isEmpty() )
			continue;
		QFileInfo fi( it.value() );
		if( fi.exists() )
			candidates << fi.absolutePath() + "/qemu-img" + exe_suffix;
	}

	candidates
		<< "C:/Program Files/qemu/qemu-img" + exe_suffix
		<< "C:/Program Files (x86)/qemu/qemu-img" + exe_suffix
		<< "/usr/bin/qemu-img"
		<< "/usr/local/bin/qemu-img"
		<< "qemu-img" + exe_suffix
		<< "qemu-img";

	for( int i = 0; i < candidates.size(); ++i )
	{
		QString c = QDir::toNativeSeparators( candidates[i] );
		if( path_ok( c ) )
		{
			settings.setValue( "QEMU-IMG_Path", c );
			AQDebug( "QString Get_QEMU_IMG_Path()", "Using qemu-img: " + c );
			return c;
		}
	}

	AQWarning( "QString Get_QEMU_IMG_Path()",
			   "qemu-img not found; falling back to configured value: " + configured );
	if( ! configured.isEmpty() )
		return configured;
	return QString( "qemu-img" ) + exe_suffix;
}

bool Create_New_HDD_Image( bool encrypted, const QString &base_image,
						   const QString &file_name, const QString &format, VM::Device_Size size, bool verbose )
{
	// Ensure destination directory exists
	QFileInfo outInfo( file_name );
	if( ! outInfo.absolutePath().isEmpty() )
		QDir().mkpath( outInfo.absolutePath() );

	// Create command line
	QStringList args;
	args << "create";
	
	if( encrypted )
		args << "-e";
	
	if( ! base_image.isEmpty() )
		args << "-b" << base_image;
	
	args << "-f" << format;
	
	args << file_name;
	
	switch( size.Suffix )
	{
		case VM::Size_Suf_Mb: // MB
			args << QString::number( size.Size ) + "M";
			break;
			
		case VM::Size_Suf_Gb: // GB
			args << QString::number( size.Size ) + "G";
			break;
			
		default: // KG
			args << QString::number( size.Size );
			break;
	}
	
	// Start qemu-img process
	QProcess qemu_img;
	QString qemu_img_path = Get_QEMU_IMG_Path();
	qemu_img.start( qemu_img_path, args );
	
	if( ! qemu_img.waitForStarted(2000) )
	{
		AQGraphic_Error( "bool Create_New_HDD_Image( bool encrypted, const QString &base_image,"
						 "const QString &file_name, const QString &format, VM::Device_Size size, bool verbose )",
						 QObject::tr("Error!"),
						 QObject::tr("Cannot Start qemu-img! Image isn't Created!\nTried: %1")
							 .arg( qemu_img_path ) );
		return false;
	}
	
	if( ! qemu_img.waitForFinished(10000) )
	{
		AQGraphic_Error( "bool Create_New_HDD_Image( bool encrypted, const QString &base_image,"
						 "const QString &file_name, const QString &format, VM::Device_Size size, bool verbose )",
						 QObject::tr("Error!"), QObject::tr("qemu-img Cannot Finish! Image isn't Created!") );
		return false;
	}
	else
	{
		QByteArray err = qemu_img.readAllStandardError();
		QByteArray out = qemu_img.readAllStandardOutput();
		
		if( err.count() > 0 )
		{
			AQGraphic_Error( "bool Create_New_HDD_Image( bool encrypted, const QString &base_image,"
							 "const QString &file_name, const QString &format, VM::Device_Size size, bool verbose )",
							 QObject::tr("Error!"), QObject::tr("Cannot Create Image!\nInformation: ") + err );
		}
		
		QRegExp rx( "Format*ing*fmt*size*" );
		rx.setPatternSyntax( QRegExp::Wildcard );
		
		if( verbose )
		{
			if( rx.exactMatch( out ) )
			{
				QMessageBox::information( NULL, QObject::tr("Complete!"),
						QObject::tr("QEMU-IMG is Creates HDD Image.\nAdditional Information:\n") + out );
			}
			else
			{
				QMessageBox::information( NULL, QObject::tr("Warning!"),
						 QObject::tr("QEMU-IMG is Returned non Standard Message!.\nAdditional Information:\n") + out );
			}
		}
		
		return true;
	}
}

bool Create_New_HDD_Image( const QString &file_name, VM::Device_Size size )
{
	QSettings settings;
	QString format = settings.value( "Default_HDD_Image_Format", "qcow2" ).toString();
	
	return Create_New_HDD_Image( false, "", file_name, format, size, false );
}

bool Format_HDD_Image( const QString &file_name, VM::Disk_Info info )
{
	if( file_name.isEmpty() )
	{
		AQError( "bool Format_HDD_Image( const QString &file_name )",
				 "File Name is Empty!" );
		
		return false;
	}
	
	VM_HDD tmp_hd = VM_HDD( true, file_name );
	tmp_hd.Set_Disk_Info( info );
	QString hd_format = tmp_hd.Get_Image_Format();
	
	if( hd_format.isEmpty() )
	{
		AQError( "bool Format_HDD_Image( const QString &file_name )",
				 "Format is Empty!" );
		
		return false;
	}
	
	return Create_New_HDD_Image( false, "", file_name, hd_format, tmp_hd.Get_Virtual_Size(), false );
}

QList<QString> Get_Templates_List()
{
	QList<QString> all_templates;
	QSettings settings;
	
	// VM Templates
	QDir sys_templates_dir( QDir::toNativeSeparators(settings.value("AQEMU_Data_Folder", "").toString() + "/os_templates/") );
	
	QFileInfoList sys_templates_list = sys_templates_dir.entryInfoList(
			QStringList("*.aqvmt"), QDir::Files, QDir::Unsorted );
	
	QDir user_templates_dir( QDir::toNativeSeparators(settings.value("VM_Directory", "").toString() + "/os_templates/") );
	
	QFileInfoList user_templates_list = user_templates_dir.entryInfoList(
			QStringList("*.aqvmt"), QDir::Files, QDir::Unsorted );
	
	for( int tx = 0; tx < sys_templates_list.count(); ++tx )
	{
		for( int ux = 0; ux < user_templates_list.count(); ++ux )
		{
			if( sys_templates_list[tx].completeBaseName() ==
						 user_templates_list[ux].completeBaseName() )
			{
				sys_templates_list.takeAt( tx ); // delete system template
				tx -= 1;
				ux = user_templates_list.count();
			}
		}
	}
	
	// OK. In Template Lists Only Unique Values
	for( int ix = 0; ix < sys_templates_list.count(); ++ix )
		all_templates.append( sys_templates_list[ix].absoluteFilePath() );
	
	for( int ix = 0; ix < user_templates_list.count(); ++ix )
		all_templates.append( user_templates_list[ix].absoluteFilePath() );
	
	// sort
	qSort( all_templates );
	
	return all_templates;
}


QString Get_FS_Compatible_VM_Name( const QString &name )
{
	//QRegExp vm_name_val = QRegExp( "[^a-zA-Z0-9_]" ); // old style
	QRegExp vm_name_val = QRegExp( "[^\\w\\.]" );
	
	QString tmp = name;
	tmp = tmp.replace( vm_name_val, "_" );
	
	return tmp.replace( "__", "_" );
}

QString Get_Complete_VM_File_Path( const QString &vm_name )
{
	//QRegExp vm_name_val = QRegExp( "[^a-zA-Z0-9_]" ); // old style
	QRegExp vm_name_val = QRegExp( "[^\\w]" );
	
	QString tmp = vm_name;
	tmp = tmp.replace( vm_name_val, "_" );
	
	QString new_name = tmp.replace( "__", "_" );
	QString tmp_str = new_name;
	
	QSettings settings;
	
	QString ret_str = settings.value("VM_Directory", "").toString() + tmp_str;
	
	if( ! ret_str.endsWith(".aqemu") )
		ret_str += ".aqemu";
	
	for( int ix = 0; QFile::exists(ret_str); ++ix )
		tmp_str = new_name + QString::number( ix );
	
	return ret_str;
}

QString Get_TR_Size_Suffix( VM::Device_Size suf )
{
	switch( suf.Suffix )
	{
		case VM::Size_Suf_Kb:
			return QObject::tr("Kb");
				
		case VM::Size_Suf_Mb:
			return QObject::tr("Mb");
				
		case VM::Size_Suf_Gb:
			return QObject::tr("Gb");
				
		default:
			return QObject::tr("Mb");
	}
}

QString AQ_Normalize_File_Path( const QString &path )
{
	QString p = path.trimmed();
	while( p.size() >= 2 )
	{
		const QChar a = p.at( 0 );
		const QChar b = p.at( p.size() - 1 );
		if( ( a == QLatin1Char( '"' ) && b == QLatin1Char( '"' ) ) ||
		    ( a == QLatin1Char( '\'' ) && b == QLatin1Char( '\'' ) ) )
		{
			p = p.mid( 1, p.size() - 2 ).trimmed();
			continue;
		}
		break;
	}
	return p;
}

QString AQ_Qemu_Drive_File_Key( const QString &path )
{
	QString p = QDir::fromNativeSeparators( AQ_Normalize_File_Path( path ) );
	// QEMU treats ',' as option separator — escape as ',,'
	p.replace( QLatin1Char( ',' ), QLatin1String( ",," ) );
	// Prefer dotted form whenever the path is non-trivial (spaces, drive letters, etc.)
	if( p.contains( QLatin1Char( ' ' ) ) || p.contains( QLatin1Char( '=' ) ) ||
	    p.contains( QLatin1Char( ',' ) ) || p.contains( QLatin1Char( ':' ) ) )
		return QStringLiteral( "file.driver=file,file.filename=%1" ).arg( p );
	return QStringLiteral( "file=%1" ).arg( p );
}

bool AQ_Is_Apple_Partition_Map_Image( const QString &path )
{
	QFile f( AQ_Normalize_File_Path( path ) );
	if( ! f.open( QIODevice::ReadOnly ) )
		return false;
	const QByteArray sector0 = f.read( 2 );
	if( sector0 != QByteArrayLiteral( "ER" ) )
		return false;
	if( ! f.seek( 512 ) )
		return false;
	return f.read( 2 ) == QByteArrayLiteral( "PM" );
}

bool AQ_Is_Iso9660_Image( const QString &path )
{
	QFile f( AQ_Normalize_File_Path( path ) );
	if( ! f.open( QIODevice::ReadOnly ) )
		return false;
	// ISO9660 PVD starts at byte 32768; identifier "CD001" at +1.
	if( ! f.seek( 32769 ) )
		return false;
	return f.read( 5 ) == QByteArrayLiteral( "CD001" );
}

qint64 AQ_Apple_HFS_Partition_Offset( const QString &path )
{
	QFile f( AQ_Normalize_File_Path( path ) );
	if( ! f.open( QIODevice::ReadOnly ) )
		return -1;

	const QByteArray ddm = f.read( 512 );
	if( ddm.size() < 4 || ! ddm.startsWith( "ER" ) )
		return -1;

	quint16 block_size = 512;
	if( ddm.size() >= 4 )
	{
		const quint16 bs = ( quint8( ddm.at( 2 ) ) << 8 ) | quint8( ddm.at( 3 ) );
		if( bs == 512 || bs == 1024 || bs == 2048 )
			block_size = bs;
	}

	int map_count = 1;
	for( int i = 1; i <= map_count && i < 64; ++i )
	{
		if( ! f.seek( qint64( i ) * block_size ) )
			return -1;
		const QByteArray ent = f.read( block_size );
		if( ent.size() < 80 || ! ent.startsWith( "PM" ) )
			break;

		const auto be32 = [&ent]( int off ) -> quint32 {
			return ( quint32( quint8( ent.at( off ) ) ) << 24 ) |
			       ( quint32( quint8( ent.at( off + 1 ) ) ) << 16 ) |
			       ( quint32( quint8( ent.at( off + 2 ) ) ) << 8 ) |
			       quint32( quint8( ent.at( off + 3 ) ) );
		};

		if( i == 1 )
		{
			const int mc = int( be32( 4 ) );
			if( mc > 0 && mc < 64 )
				map_count = mc;
		}

		const quint32 part_start = be32( 8 );
		const QByteArray typ = ent.mid( 48, 32 );
		const int z = typ.indexOf( '\0' );
		const QByteArray typ_z = ( z >= 0 ) ? typ.left( z ) : typ;
		if( typ_z.startsWith( "Apple_HFS" ) ) // Apple_HFS / Apple_HFSX
			return qint64( part_start ) * qint64( block_size );
	}
	return -1;
}

QString AQ_Ensure_OpenCore_Boot_With_PartitionDxe( const QString &opencore_iso,
                                                   const QString &dest_boot_img )
{
	const QString iso = AQ_Normalize_File_Path( opencore_iso );
	const QString dest = AQ_Normalize_File_Path( dest_boot_img );
	if( iso.isEmpty() || dest.isEmpty() || ! QFile::exists( iso ) )
		return QString();

	QString dxe = QDir( QCoreApplication::applicationDirPath() )
	                  .filePath( QStringLiteral( "OpenPartitionDxe.efi" ) );
	if( ! QFile::exists( dxe ) )
	{
		// Dev-tree fallback next to the binary's ../../third_party
		dxe = QDir( QCoreApplication::applicationDirPath() )
		          .absoluteFilePath( QStringLiteral( "../third_party/opencore/OpenPartitionDxe.efi" ) );
	}
	if( ! QFile::exists( dxe ) )
		return QString();

	QFileInfo dest_info( dest );
	QFileInfo iso_info( iso );
	QFileInfo dxe_info( dxe );
	if( dest_info.exists() &&
	    dest_info.lastModified() >= iso_info.lastModified() &&
	    dest_info.lastModified() >= dxe_info.lastModified() &&
	    dest_info.size() > 1024 * 1024 )
		return dest;

	QDir().mkpath( QFileInfo( dest ).absolutePath() );

#ifdef Q_OS_WIN32
	// Prefer the already-tested WSL+mtools path (Windows hosts run Intel macOS via WSL/KVM).
	const QString wsl_iso = QStringLiteral( "/mnt/%1%2" )
		.arg( iso.at( 0 ).toLower() )
		.arg( QDir::fromNativeSeparators( iso.mid( 2 ) ) );
	const QString wsl_dest = QStringLiteral( "/mnt/%1%2" )
		.arg( dest.at( 0 ).toLower() )
		.arg( QDir::fromNativeSeparators( dest.mid( 2 ) ) );
	const QString wsl_dxe = QStringLiteral( "/mnt/%1%2" )
		.arg( dxe.at( 0 ).toLower() )
		.arg( QDir::fromNativeSeparators( dxe.mid( 2 ) ) );

	QProcess p;
	p.setProgram( QStringLiteral( "wsl" ) );
	p.setArguments( QStringList()
		<< QStringLiteral( "-e" ) << QStringLiteral( "bash" ) << QStringLiteral( "-lc" )
		<< QStringLiteral(
			"set -e; TMP=$(mktemp -d); "
			"7z e -y -o\"$TMP\" \"%1\" BOOT.img >/dev/null; "
			"cp \"$TMP/BOOT.img\" \"%2\"; "
			"export MTOOLS_SKIP_CHECK=1; "
			"mcopy -o -i \"%2\" \"%3\" ::/EFI/OC/Drivers/OpenPartitionDxe.efi; "
			"rm -rf \"$TMP\"" )
			.arg( wsl_iso, wsl_dest, wsl_dxe ) );
	p.start();
	if( ! p.waitForFinished( 120000 ) || p.exitCode() != 0 )
		return QString();
#else
	Q_UNUSED( iso );
	return QString();
#endif

	return QFile::exists( dest ) ? dest : QString();
}

bool AQ_Resolve_Display_Size( const QString &mode, int *out_w, int *out_h )
{
	if( ! out_w || ! out_h )
		return false;
	*out_w = 0;
	*out_h = 0;

	const QString m = mode.trimmed().toLower();
	if( m.isEmpty() || m == QLatin1String( "auto" ) )
		return false;

	int rw = 0, rh = 0;
	if( m == QLatin1String( "native" ) )
	{
		QScreen *screen = QGuiApplication::primaryScreen();
		if( ! screen )
			return false;
		const qreal dpr = screen->devicePixelRatio();
		const QSize logical = screen->size();
		rw = qRound( logical.width() * dpr );
		rh = qRound( logical.height() * dpr );
	}
	else
	{
		const QStringList parts = mode.split( QLatin1Char( 'x' ), QString::SkipEmptyParts );
		if( parts.size() != 2 )
			return false;
		bool okw = false, okh = false;
		rw = parts.at( 0 ).trimmed().toInt( &okw );
		rh = parts.at( 1 ).trimmed().toInt( &okh );
		if( ! okw || ! okh )
			return false;
	}

	// Soft clamp — extreme modes often fall back to 1280x800 in macOS guests.
	if( rw < 1024 ) rw = 1024;
	if( rh < 768 ) rh = 768;
	if( rw > 4096 ) rw = 4096;
	if( rh > 2160 ) rh = 2160;

	*out_w = rw;
	*out_h = rh;
	return true;
}

static bool AQ_Rewrite_OpenCore_Plist_Resolution( const QString &plist_path,
                                                   const QString &resolution_wxh )
{
	QFile f( plist_path );
	if( ! f.open( QIODevice::ReadOnly ) )
		return false;
	QByteArray data = f.readAll();
	f.close();
	if( data.isEmpty() )
		return false;

	const QByteArray res = resolution_wxh.toUtf8();
	QRegExp re_res( QStringLiteral( "(<key>Resolution</key>\\s*<string>)[^<]*(</string>)" ) );
	re_res.setMinimal( true );
	QString text = QString::fromUtf8( data );
	if( re_res.indexIn( text ) < 0 )
		return false;
	{
		const QString replaced = re_res.cap( 1 ) + QString::fromUtf8( res ) + re_res.cap( 2 );
		text.replace( re_res.pos( 0 ), re_res.matchedLength(), replaced );
	}

	// Prefer ForceResolution so OC does not silently pick Max/empty.
	QRegExp re_force( QStringLiteral(
		"(<key>ForceResolution</key>\\s*)<(true|false)\\s*/>" ) );
	re_force.setMinimal( true );
	if( re_force.indexIn( text ) >= 0 )
	{
		const QString replaced = re_force.cap( 1 ) + QStringLiteral( "<true/>" );
		text.replace( re_force.pos( 0 ), re_force.matchedLength(), replaced );
	}

	if( ! f.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
		return false;
	const QByteArray out = text.toUtf8();
	const bool ok = f.write( out ) == out.size();
	f.close();
	return ok;
}

bool AQ_Patch_OpenCore_FAT_Resolution( const QString &fat_img, const QString &resolution_wxh )
{
	const QString img = AQ_Normalize_File_Path( fat_img );
	const QString res = resolution_wxh.trimmed();
	if( img.isEmpty() || res.isEmpty() || ! QFile::exists( img ) )
		return false;

	const QString stamp = img + QStringLiteral( ".aqemu-resolution" );
	{
		QFile sf( stamp );
		if( sf.open( QIODevice::ReadOnly ) &&
		    QString::fromUtf8( sf.readAll() ).trimmed() == res )
			return true;
	}

#ifdef Q_OS_WIN32
	QTemporaryDir tmp;
	if( ! tmp.isValid() )
		return false;
	const QString host_plist = tmp.filePath( QStringLiteral( "config.plist" ) );
	const QString wsl_img = QStringLiteral( "/mnt/%1%2" )
		.arg( img.at( 0 ).toLower() )
		.arg( QDir::fromNativeSeparators( img.mid( 2 ) ) );
	const QString wsl_plist = QStringLiteral( "/mnt/%1%2" )
		.arg( host_plist.at( 0 ).toLower() )
		.arg( QDir::fromNativeSeparators( host_plist.mid( 2 ) ) );

	QProcess extract;
	extract.setProgram( QStringLiteral( "wsl" ) );
	extract.setArguments( QStringList()
		<< QStringLiteral( "-e" ) << QStringLiteral( "bash" ) << QStringLiteral( "-lc" )
		<< QStringLiteral(
			"set -e; export MTOOLS_SKIP_CHECK=1; "
			"mcopy -n -i \"%1\" ::/EFI/OC/config.plist \"%2\"" )
			.arg( wsl_img, wsl_plist ) );
	extract.start();
	if( ! extract.waitForFinished( 60000 ) || extract.exitCode() != 0 )
		return false;

	if( ! AQ_Rewrite_OpenCore_Plist_Resolution( host_plist, res ) )
		return false;

	QProcess inject;
	inject.setProgram( QStringLiteral( "wsl" ) );
	inject.setArguments( QStringList()
		<< QStringLiteral( "-e" ) << QStringLiteral( "bash" ) << QStringLiteral( "-lc" )
		<< QStringLiteral(
			"set -e; export MTOOLS_SKIP_CHECK=1; "
			"mcopy -o -i \"%1\" \"%2\" ::/EFI/OC/config.plist" )
			.arg( wsl_img, wsl_plist ) );
	inject.start();
	if( ! inject.waitForFinished( 60000 ) || inject.exitCode() != 0 )
		return false;
#else
	QTemporaryDir tmp;
	if( ! tmp.isValid() )
		return false;
	const QString host_plist = tmp.filePath( QStringLiteral( "config.plist" ) );
	QProcess extract;
	extract.start( QStringLiteral( "mcopy" ), QStringList()
		<< QStringLiteral( "-n" ) << QStringLiteral( "-i" ) << img
		<< QStringLiteral( "::/EFI/OC/config.plist" ) << host_plist );
	if( ! extract.waitForFinished( 60000 ) || extract.exitCode() != 0 )
	{
		// mtools may need MTOOLS_SKIP_CHECK
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert( QStringLiteral( "MTOOLS_SKIP_CHECK" ), QStringLiteral( "1" ) );
		extract.setProcessEnvironment( env );
		extract.start( QStringLiteral( "mcopy" ), QStringList()
			<< QStringLiteral( "-n" ) << QStringLiteral( "-i" ) << img
			<< QStringLiteral( "::/EFI/OC/config.plist" ) << host_plist );
		if( ! extract.waitForFinished( 60000 ) || extract.exitCode() != 0 )
			return false;
	}
	if( ! AQ_Rewrite_OpenCore_Plist_Resolution( host_plist, res ) )
		return false;
	QProcess inject;
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert( QStringLiteral( "MTOOLS_SKIP_CHECK" ), QStringLiteral( "1" ) );
	inject.setProcessEnvironment( env );
	inject.start( QStringLiteral( "mcopy" ), QStringList()
		<< QStringLiteral( "-o" ) << QStringLiteral( "-i" ) << img
		<< host_plist << QStringLiteral( "::/EFI/OC/config.plist" ) );
	if( ! inject.waitForFinished( 60000 ) || inject.exitCode() != 0 )
		return false;
#endif

	QFile stamp_out( stamp );
	if( stamp_out.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
		stamp_out.write( res.toUtf8() );
		stamp_out.close();
	}
	return true;
}

bool AQ_Is_Plausible_Apple_SMC_OSK( const QString &osk )
{
	const QString s = osk.trimmed();
	if( s.size() < 20 || s.size() > 128 )
		return false;

	const QString lower = s.toLower();
	// Reject Proxmox / QEMU network fragments people paste by mistake
	static const char *bad[] = {
		"bridge=", "net0", "net1", "e1000", "virtio", "vmbr", "vlan=",
		"firewall=", "model=", "macaddr=", "queues=", "rate="
	};
	for( const char *b : bad )
	{
		if( lower.contains( QLatin1String( b ) ) )
			return false;
	}

	// Real OSKs are mostly letters/digits with a few punctuation chars (e.g. () ).
	int weird = 0;
	for( const QChar &ch : s )
	{
		if( ch.isLetterOrNumber() || ch == QLatin1Char( '(' ) || ch == QLatin1Char( ')' ) ||
		    ch == QLatin1Char( '.' ) || ch == QLatin1Char( '-' ) || ch == QLatin1Char( '_' ) )
			continue;
		++weird;
	}
	return weird <= 4;
}

QString Get_Last_Dir_Path( const QString &path )
{
	QFileInfo info( AQ_Normalize_File_Path( path ) );
	QString dir = info.path();
	
	if( dir.isEmpty() ) return "/";
	else return dir;
}

bool It_Host_Device( const QString &path )
{
	#ifdef Q_OS_WIN32
	// FIXME
	return false;
	#else
	if( path.startsWith("/dev/") ) return true;
	else return false;
	#endif
}

void Check_AQEMU_Permissions()
{
	QSettings settings;
	QFileInfo test_perm;
	
	#ifndef Q_OS_WIN32
	// This Section For Unix Like OS.
	test_perm = QFileInfo( settings.fileName() );
	
	if( test_perm.exists() )
	{
		if( ! test_perm.isWritable() )
		{
			AQGraphic_Error( "void Check_AQEMU_Permissions()", QObject::tr("Error!"),
							 QObject::tr("AQEMU Config File is Read Only!\nCheck Permissions For File: ") +
							 settings.fileName(), true );
		}
	}
	#endif
	
	// Check VM Dir Permissions
	test_perm = QFileInfo( settings.value("VM_Directory", "~").toString() );
	
	if( test_perm.exists() )
	{
		if( ! test_perm.isWritable() )
		{
			AQGraphic_Error( "void Check_AQEMU_Permissions()", QObject::tr("Error!"),
							 QObject::tr("AQEMU VM Directory is Read Only!\nCheck Permissions For: ") +
							 settings.value("VM_Directory", "~").toString(), true );
		}
	}
	
	// Check VM Templates Dir Permissions
	test_perm = QFileInfo( settings.value("VM_Directory", "~").toString() + "os_templates" );
	
	if( test_perm.exists() )
	{
		if( ! test_perm.isWritable() )
		{
			AQGraphic_Error( "void Check_AQEMU_Permissions()", QObject::tr("Error!"),
							 QObject::tr("AQEMU VM Template Directory is Read Only!\nCheck Permissions For: ") +
							 settings.value("VM_Directory", "~").toString() + "os_templates", true );
		}
	}
	
	// Check AQEMU Log File Permissions
	if( ! settings.value("Log/Log_Path", "").toString().isEmpty() )
	{
		test_perm = QFileInfo( settings.value("Log/Log_Path", "").toString() );
		
		if( test_perm.exists() )
		{
			if( ! test_perm.isWritable() )
			{
				AQGraphic_Error( "void Check_AQEMU_Permissions()", QObject::tr("Error!"),
								 QObject::tr("AQEMU Log File is Read Only!\nCheck Permissions For File: ") +
								 settings.value("Log/Log_Path", "").toString(), false );
			}
		}
	}
}

VM::Emulator_Version String_To_Emulator_Version( const QString &str )
{
	if( str == "QEMU 0.9.0" ) return VM::QEMU_2_0;
	else if( str == "QEMU 0.9.1" ) return VM::QEMU_2_0;
	else if( str == "QEMU 0.10.X" ) return VM::QEMU_2_0;
	else if( str == "QEMU 0.11.X" ) return VM::QEMU_2_0;
	else if( str == "QEMU 0.12.X" ) return VM::QEMU_2_0;
	else if( str == "QEMU 0.13.X" ) return VM::QEMU_2_0;
	else if( str == "QEMU 0.14.X" ) return VM::QEMU_2_0;
	else if( str == "QEMU 0.15.X" ) return VM::QEMU_2_0;
	else if( str == "QEMU 1.0" ) return VM::QEMU_2_0;
	else if( str == "QEMU 2.0" ) return VM::QEMU_2_0;
	else if( str == "KVM 7X" ) return VM::QEMU_2_0;
	else if( str == "KVM 8X" ) return VM::QEMU_2_0;
	else if( str == "KVM 0.11.X" ) return VM::QEMU_2_0;
	else if( str == "KVM 0.12.X" ) return VM::QEMU_2_0;
	else if( str == "KVM 0.13.X" ) return VM::QEMU_2_0;
	else if( str == "KVM 0.14.X" ) return VM::QEMU_2_0;
	else if( str == "KVM 0.15.X" ) return VM::QEMU_2_0;
	else if( str == "KVM 1.0" ) return VM::QEMU_2_0;
	else if( str == "Obsolete" ) return VM::Obsolete;
	else
	{
		AQError( "VM::Emulator_Version String_To_Emulator_Version( const QString &str )",
				 QString("Emulator version \"%1\" not valid!").arg(str) );
		return VM::Obsolete;
	}
}

QString Emulator_Version_To_String( VM::Emulator_Version ver )
{
	switch( ver )
	{
		case VM::QEMU_2_0:
			return "QEMU 2.0";
			
		case VM::Obsolete:
			return "Obsolete";
			
		default:
			AQError( "QString Emulator_Version_To_String( VM::Emulator_Version ver )",
					 QString("Emulator version \"%1\" not valid!").arg((int)ver) );
			return "";
	}
}

static QList<Emulator> Emulators_List;
static QList<Emulator> Empty_Emul_List;
static Emulator Empty_Emul;

bool Update_Emulators_List()
{
	// Clear old emulator list
	Emulators_List.clear();
	
	// Get dir path
	QSettings settings;
	QFileInfo info( settings.fileName() );
	QString aqemuSettingsFolder = info.absoluteDir().absolutePath();
	if( ! (aqemuSettingsFolder.endsWith("/") || aqemuSettingsFolder.endsWith("\\")) )
		aqemuSettingsFolder = QDir::toNativeSeparators( aqemuSettingsFolder + "/" );
	
	if( ! QFile::exists(aqemuSettingsFolder) )
	{
		AQError( "bool Update_Emulators_List()",
				 QString("Cannot get path for save emulator! Folder \"%1\" not exists!").arg(aqemuSettingsFolder) );
		return false;
	}
	
	// Get all *.emulators files
	QDir emulDir( aqemuSettingsFolder );
	QStringList emulFiles = emulDir.entryList( QStringList("*.emulator"), QDir::Files, QDir::Name );
	
	if( emulFiles.isEmpty() )
	{
		AQDebug( "bool Update_Emulators_List()", "No emulators configured, running auto-discovery..." );
		if( System_Info::Auto_Find_And_Save_Emulators() )
		{
			// Re-scan
			emulFiles = emulDir.entryList( QStringList("*.emulator"), QDir::Files, QDir::Name );
		}
	}
	
	if( emulFiles.isEmpty() )
	{
		AQWarning( "bool Update_Emulators_List()",
				   QString("No emulators found in \"%1\"").arg(aqemuSettingsFolder) );
		return false;
	}
	
	// Check default emulators
	bool qDef = false;
	
	// Load emulators
	for( int ex = 0; ex < emulFiles.count(); ++ex )
	{
		Emulator tmp;
		
		if( ! tmp.Load(aqemuSettingsFolder + emulFiles[ex]) )
		{
			AQError( "bool Update_Emulators_List()",
					 QString("Cannot load emulator from file: \"%1\"").arg(emulFiles[ex]) );
			continue;
		}
		
		// Default?
		if( tmp.Get_Default() )
		{
			if( qDef )
			{
				AQWarning( "bool Update_Emulators_List()",
						   "Default QEMU emulator already loaded." );
				tmp.Set_Default( false );
			}
			else qDef = true;
		}
		
		// Update available options?
		if( tmp.Get_Check_Available_Options() )
		{
			QMap<QString, QString> tmpBinFiles = tmp.Get_Binary_Files();
			QMap<QString, Available_Devices> tmpDevMap; // result
			
			for( QMap<QString, QString>::const_iterator it = tmpBinFiles.constBegin(); it != tmpBinFiles.constEnd(); ++it )
			{
				if( ! it.value().isEmpty() )
				{
					bool ok = false;
					Available_Devices tmpDev = System_Info::Get_Emulator_Info( it.value(), &ok, tmp.Get_Version(), it.key() );
					
					if( ! ok )
					{
						AQError( "bool Update_Emulators_List()",
								 "Cannot set new emulator available options!" );
						continue;
					}
					
					// Adding device
					tmpDevMap[ it.key() ] = tmpDev;
				}
			}
			
			// Set all devices
			tmp.Set_Devices( tmpDevMap );
		}
		
		// Check version?
		if( tmp.Get_Check_Version() )
		{
			// Get bin file path
			QString binFilePath = "";
			QMap<QString, QString> tmpBinFiles = tmp.Get_Binary_Files();
			for( QMap<QString, QString>::const_iterator it = tmpBinFiles.constBegin(); it != tmpBinFiles.constEnd(); ++it )
			{
				if( QFile::exists(it.value()) )
				{
					binFilePath = it.value();
					break;
				}
			}
			
			if( binFilePath.isEmpty() )
			{
				AQError( "bool Update_Emulators_List()",
						 QString("Cannot find exists emulator binary file for emulator \"%1\"!").arg(tmp.Get_Name()) );
			}
			
			// All OK, check version
			tmp.Set_Version( System_Info::Get_Emulator_Version(binFilePath) );
		}
		
		// Adding emulator
		Emulators_List << tmp;
	}
	
	// Emulator loaded?
	if( Emulators_List.isEmpty() )
	{
		AQWarning( "bool Update_Emulators_List()",
				   "No emulators loaded!" );
		return false;
	}
	
	// Check defaults
	if( qDef == false )
	{
		for( int ex = 0; ex < Emulators_List.count(); ++ex )
	    {
		    Emulators_List[ex].Set_Default( true );
		    break;
		}
	}
	
	// All OK
	return true;
}

const QList<Emulator> &Get_Emulators_List()
{
	if( Update_Emulators_List() ) return Emulators_List; // FIXME Update
	else
	{
		AQError( "QList<Emulator> Get_Emulators_List()",
				 "Cannot Update Emulators List" );
		return Empty_Emul_List;
	}
}

bool Remove_All_Emulators_Files()
{
	// Get emulators dir path
	QSettings settings;
	QFileInfo info( settings.fileName() );
	QString aqemuSettingsFolder = info.absoluteDir().absolutePath();
	aqemuSettingsFolder += aqemuSettingsFolder.endsWith( QDir::toNativeSeparators("/") )
						   ? ""
						   : QDir::toNativeSeparators( "/" );
	
	if( ! QFile::exists(aqemuSettingsFolder) )
	{
		AQError( "bool Remove_All_Emulators_Files()",
				 QString("Cannot get path for save emulator! Folder \"%1\" not exists!").arg(aqemuSettingsFolder) );
		return false;
	}
	else
	{
		// Get all *.emulators files
		QDir emulDir( aqemuSettingsFolder );
		QStringList emulFiles = emulDir.entryList( QStringList("*.emulator"), QDir::Files, QDir::Name );
		
		bool allFilesRemoved = true;
		for( int dx = 0; dx < emulFiles.count(); ++dx )
		{
			if( ! QFile::remove(aqemuSettingsFolder + emulFiles[dx]) )
			{
				AQError( "bool Remove_All_Emulators_Files()",
						 QString("Cannot delete file \"%1\"!").arg(emulFiles[dx]) );
				allFilesRemoved = false;
			}
		}
		
		return allFilesRemoved;
	}
}

const Emulator &Get_Default_Emulator()
{
	if( Emulators_List.count() <= 0 )
		AQError( "const Emulator &Get_Default_Emulator()",
				 "Emulator Count == 0" );
	else
	{
		for( int ix = 0; ix < Emulators_List.count(); ix++ )
		{
			if( Emulators_List[ix].Get_Default() )
		    {
				return Emulators_List[ ix ];
		    }
		}
	}
	
	AQWarning( "const Emulator &Get_Default_Emulator()",
			   "Cannot Find!" );
	return Empty_Emul;
}

const Emulator &Get_Emulator_By_Name( const QString &name )
{
	if( Emulators_List.count() <= 0 )
		AQError( "const Emulator Get_Emulator_By_Name( const QString &name )",
				 "Emulator Count == 0" );
	else
	{
		for( int ix = 0; ix < Emulators_List.count(); ix++ )
		{
			if( Emulators_List[ix].Get_Name() == name )
				return Emulators_List[ ix ];
		}
	}
	
	AQWarning( "const Emulator Get_Emulator_By_Name( const QString &name )",
			   "Cannot Find!" );
	return Empty_Emul;
}

int Get_Random( int min, int max )
{
	if( min < 0 || max > RAND_MAX || min > max )
	{
		return -1;
	}
	
	qsrand( QTime::currentTime().msec() );
	
	return int( qrand() / (RAND_MAX + 1.0) * (max + 1 - min) + min );
}

void Load_Recent_Images_List()
{
	QSettings settings;
	
	// CD
	int max_cd = settings.value( "CD_ROM_Existing_Images/Max", "5" ).toString().toInt();
	
	Recent_CD_Images.clear();
	
	for( int ix = 0; ix < max_cd; ++ix )
	{
		QString tmp = settings.value( "CD_ROM_Existing_Images/item" + QString::number(ix), "" ).toString();
		
		if( ! tmp.isEmpty() ) Recent_CD_Images << tmp;
	}
	
	// FDD
	int max_fdd = settings.value( "Floppy_Existing_Images/Max", "5" ).toString().toInt();
	
	Recent_FDD_Images.clear();
	
	for( int ix = 0; ix < max_fdd; ++ix )
	{
		QString tmp = settings.value( "Floppy_Existing_Images/item" + QString::number(ix), "" ).toString();
		
		if( ! tmp.isEmpty() ) Recent_FDD_Images << tmp;
	}
}

const QStringList &Get_CD_Recent_Images_List()
{
	return Recent_CD_Images;
}

void Add_To_Recent_CD_Files( const QString &path )
{
	QSettings settings;
	int max = settings.value( "CD_ROM_Existing_Images/Max", "5" ).toString().toInt();
	
	// This Unique Path?
	for( int fx = 0; fx < Recent_CD_Images.count() && fx < max; ++fx )
	{
		if( Recent_CD_Images[fx] == path )
		{
			AQDebug( "void Add_To_Recent_CD_Files( const QString &path )",
					 "CD-ROM Path Not Unique." );
			
			// Up it path in list
			if( fx < Recent_CD_Images.count()-1 )
			{
				// swap items
				QString tmp = Recent_CD_Images[ fx+1 ];
				Recent_CD_Images[ fx+1 ] = Recent_CD_Images[ fx ];
				Recent_CD_Images[ fx ] = tmp;
			}
			else return;
		}
	}
	
	// Add to List
	if( Recent_CD_Images.count() < max )
	{
		Recent_CD_Images << path;
		settings.setValue( "CD_ROM_Existing_Images/item" + QString::number(Recent_CD_Images.count()-1), path );
	}
	else
	{
		// Delete first element and add new to end
		for( int ix = 0; ix < Recent_CD_Images.count() -1 && ix < max -1; ++ix )
		{
			Recent_CD_Images[ ix ] = Recent_CD_Images[ ix +1 ];
		}
		
		Recent_CD_Images[ max -1 ] = path;
		
		// Save Items
		for( int ix = 0; ix < Recent_CD_Images.count(); ix++ )
		{
			settings.setValue( "CD_ROM_Existing_Images/item" + QString::number(ix), Recent_CD_Images[ix] );
		}
	}
}

const QStringList &Get_FDD_Recent_Images_List()
{
	return Recent_FDD_Images;
}

void Add_To_Recent_FDD_Files( const QString &path )
{
	QSettings settings;
	int max = settings.value( "Floppy_Existing_Images/Max", "5" ).toString().toInt();
	
	// This Unique Path?
	for( int fx = 0; fx < Recent_FDD_Images.count() && fx < max; ++fx )
	{
		if( Recent_FDD_Images[fx] == path )
		{
			AQDebug( "void Add_To_Recent_FDD_Files( const QString &path )",
					 "Floppy Path Not Unique." );
			
			// Up it path in list
			if( fx < Recent_FDD_Images.count()-1 )
			{
				// swap items
				QString tmp = Recent_FDD_Images[ fx+1 ];
				Recent_FDD_Images[ fx+1 ] = Recent_FDD_Images[ fx ];
				Recent_FDD_Images[ fx ] = tmp;
			}
			else return;
		}
	}
	
	// Add to List
	if( Recent_FDD_Images.count() < max )
	{
		Recent_FDD_Images << path;
		settings.setValue( "Floppy_Existing_Images/item" + QString::number(Recent_FDD_Images.count()-1), path );
	}
	else
	{
		// Delete first element and add new to end
		for( int ix = 0; ix < Recent_FDD_Images.count() -1 && ix < max -1; ++ix )
		{
			Recent_FDD_Images[ ix ] = Recent_FDD_Images[ ix +1 ];
		}
		
		Recent_FDD_Images[ max -1 ] = path;
		
		// Save Items
		for( int ix = 0; ix < Recent_FDD_Images.count(); ix++ )
		{
			settings.setValue( "Floppy_Existing_Images/item" + QString::number(ix), Recent_FDD_Images[ix] );
		}
	}
}

static bool Show_Error_Window;

bool Get_Show_Error_Window()
{
	return Show_Error_Window;
}

void Set_Show_Error_Window( bool show )
{
	Show_Error_Window = show;
}

#include <QCheckBox>
#include <QProgressDialog>
#include <QApplication>

void Checkbox_Dependend_Set_Enabled(QList<QWidget*>& children_to_enable, QCheckBox* checkbox, bool enabled)
{
    checkbox->setEnabled(enabled);

    if ( ! checkbox->isChecked() )
        enabled = false;

    for( int i = 0; i < children_to_enable.count(); i++)
        children_to_enable[i]->setEnabled(enabled);
}


//calculate contrast difference between two colors
//(typically a background and a foreground color)
//this code was based on the formula in this
//stackoverflow post: http://stackoverflow.com/a/302961/6427928
double calculateContrast(const QColor& col1, const QColor& col2)
{
    double l1, l2;

    l1 = ( 0.2126 * pow((double)col1.red()/255.0, 2.2) +
           0.7152 * pow((double)col1.green()/255.0, 2.2) +
           0.0722 * pow((double)col1.blue()/255.0, 2.2) );
    l2 = ( 0.2126 * pow((double)col2.red()/255.0, 2.2) +
           0.7152 * pow((double)col2.green()/255.0, 2.2) +
           0.0722 * pow((double)col2.blue()/255.0, 2.2) );

    return (l1 > l2) ? (l1 / l2) : (l2 / l1);
}

void AQ_Run_With_Busy_Dialog( QWidget *parent, const QString &message,
                              const std::function<void()> &work )
{
	QProgressDialog dlg( message, QString(), 0, 0, parent );
	dlg.setWindowTitle( QObject::tr( "Please wait" ) );
	dlg.setWindowModality( Qt::ApplicationModal );
	dlg.setMinimumDuration( 0 );
	dlg.setCancelButton( nullptr );
	dlg.setAutoClose( false );
	dlg.setAutoReset( false );
	dlg.show();
	QApplication::processEvents();
	if( work )
		work();
	dlg.close();
	QApplication::processEvents();
}

QString AQ_Normalize_CPU_Architecture( const QString &raw )
{
	const QString a = raw.trimmed().toLower();
	if( a.isEmpty() )
		return QString();
	if( a == QLatin1String( "x86_64" ) || a == QLatin1String( "amd64" ) ||
	    a == QLatin1String( "x64" ) || a.contains( QLatin1String( "x86_64" ) ) )
		return QStringLiteral( "x86_64" );
	if( a == QLatin1String( "i386" ) || a == QLatin1String( "i686" ) ||
	    a == QLatin1String( "x86" ) || a == QLatin1String( "i586" ) )
		return QStringLiteral( "i386" );
	if( a == QLatin1String( "aarch64" ) || a == QLatin1String( "arm64" ) )
		return QStringLiteral( "aarch64" );
	if( a == QLatin1String( "arm" ) || a.startsWith( QLatin1String( "armv" ) ) )
		return QStringLiteral( "arm" );
	if( a.contains( QLatin1String( "ppc64" ) ) || a == QLatin1String( "powerpc64" ) )
		return QStringLiteral( "ppc64" );
	if( a.contains( QLatin1String( "ppc" ) ) || a.contains( QLatin1String( "powerpc" ) ) )
		return QStringLiteral( "ppc" );
	// Strip qemu-system- prefix if a binary name was passed
	if( a.startsWith( QLatin1String( "qemu-system-" ) ) )
		return AQ_Normalize_CPU_Architecture( a.mid( 12 ) );
	return a;
}

QString AQ_Get_Host_CPU_Architecture()
{
	static QString cached;
	if( ! cached.isEmpty() )
		return cached;

	cached = AQ_Normalize_CPU_Architecture( QSysInfo::currentCpuArchitecture() );
	if( cached.isEmpty() )
		cached = QStringLiteral( "x86_64" );
	return cached;
}

bool AQ_Guest_Matches_Host_Architecture( const QString &guest_target )
{
	const QString host = AQ_Get_Host_CPU_Architecture();
	const QString guest = AQ_Normalize_CPU_Architecture( guest_target );
	if( host.isEmpty() || guest.isEmpty() )
		return false;

	if( host == guest )
		return true;

	// 64-bit hosts can natively accelerate matching 32-bit guests of the same family
	if( host == QLatin1String( "x86_64" ) && guest == QLatin1String( "i386" ) )
		return true;
	if( host == QLatin1String( "aarch64" ) && guest == QLatin1String( "arm" ) )
		return true;
	if( host == QLatin1String( "ppc64" ) && guest == QLatin1String( "ppc" ) )
		return true;

	return false;
}

bool AQ_Should_Prefer_Native_Accelerator( const QString &guest_target )
{
	const QString guest = AQ_Normalize_CPU_Architecture( guest_target );
	if( ! AQ_Guest_Matches_Host_Architecture( guest ) )
		return false;

	// WHPX on Windows is unreliable for 32-bit x86 guests — prefer TCG
#ifdef Q_OS_WIN32
	if( guest == QLatin1String( "i386" ) )
		return false;
#endif
	return true;
}

QStringList AQ_QEMU_List_Display_Backends( const QString &qemu_binary )
{
	static QHash<QString, QStringList> cache;
	const QString key = QDir::toNativeSeparators( qemu_binary );
	if( key.isEmpty() || ! QFile::exists( key ) )
		return QStringList();
	if( cache.contains( key ) )
		return cache.value( key );

	QProcess p;
	p.setProcessChannelMode( QProcess::MergedChannels );
	p.start( key, QStringList() << QStringLiteral( "-display" ) << QStringLiteral( "help" ) );
	if( ! p.waitForFinished( 5000 ) )
	{
		p.kill();
		p.waitForFinished( 1000 );
		cache.insert( key, QStringList() );
		return QStringList();
	}

	QStringList backends;
	const QString out = QString::fromLocal8Bit( p.readAll() );
	const QStringList lines = out.split( QRegularExpression( QStringLiteral( "[\\r\\n]+" ) ),
	                                     QString::SkipEmptyParts );
	bool in_list = false;
	for( int i = 0; i < lines.count(); ++i )
	{
		const QString line = lines.at( i ).trimmed();
		if( line.contains( QLatin1String( "Available display" ), Qt::CaseInsensitive ) )
		{
			in_list = true;
			continue;
		}
		if( ! in_list )
			continue;
		if( line.startsWith( QLatin1String( "Some display" ), Qt::CaseInsensitive ) ||
		    line.startsWith( QLatin1String( "For a short" ), Qt::CaseInsensitive ) )
			break;
		// Backend names are single tokens: none, gtk, sdl, …
		if( QRegularExpression( QStringLiteral( "^[A-Za-z0-9_-]+$" ) ).match( line ).hasMatch() )
			backends << line.toLower();
	}
	cache.insert( key, backends );
	return backends;
}

QString AQ_QEMU_Pick_Native_Display( const QString &qemu_binary )
{
	const QStringList backends = AQ_QEMU_List_Display_Backends( qemu_binary );
	if( backends.contains( QLatin1String( "sdl" ) ) )
		return QStringLiteral( "sdl" );
	if( backends.contains( QLatin1String( "gtk" ) ) )
		return QStringLiteral( "gtk" );
	return QString();
}

QString AQ_Find_QEMU_Binary_With_Native_Display( const QString &system_name,
                                                 const QString &preferred_path )
{
	QString exe = system_name.trimmed();
	if( exe.isEmpty() )
		exe = QStringLiteral( "qemu-system-i386" );
	if( ! exe.endsWith( QLatin1String( ".exe" ), Qt::CaseInsensitive ) )
	{
#ifdef Q_OS_WIN32
		exe += QLatin1String( ".exe" );
#endif
	}

	QStringList candidates;
	if( ! preferred_path.trimmed().isEmpty() )
		candidates << QDir::toNativeSeparators( preferred_path );

#ifdef Q_OS_WIN32
	candidates << QDir::toNativeSeparators(
		QStringLiteral( "C:/Program Files/qemu/" ) + exe );
	candidates << QDir::toNativeSeparators(
		QStringLiteral( "C:/Program Files (x86)/qemu/" ) + exe );
#endif

	const QString app_dir = QCoreApplication::applicationDirPath();
	if( ! app_dir.isEmpty() )
		candidates << QDir::toNativeSeparators( app_dir + QLatin1Char( '/' ) + exe );

	for( int i = 0; i < candidates.count(); ++i )
	{
		const QString c = candidates.at( i );
		if( QFile::exists( c ) && ! AQ_QEMU_Pick_Native_Display( c ).isEmpty() )
			return c;
	}
	return preferred_path;
}

static QString First_Existing_Path( const QStringList &candidates )
{
	for( int i = 0; i < candidates.count(); ++i )
	{
		if( QFile::exists( candidates[i] ) )
			return QDir::toNativeSeparators( candidates[i] );
	}
	return QString();
}

static bool UEFI_Arch_Is_X86( const QString &arch )
{
	const QString a = arch.trimmed().toLower();
	return a.contains( QLatin1String( "x86" ) ) || a.contains( QLatin1String( "amd64" ) ) ||
	       a == QLatin1String( "i386" ) || a == QLatin1String( "pc" );
}

QString Find_UEFI_Firmware_CODE( const QString &qemu_binary_path, const QString &arch )
{
	QStringList candidates;
	const bool x86 = UEFI_Arch_Is_X86( arch );

	#ifdef Q_OS_WIN32
	QString binDir;
	if( ! qemu_binary_path.isEmpty() )
		binDir = QFileInfo( qemu_binary_path ).absolutePath();

	if( ! binDir.isEmpty() )
	{
		if( x86 )
		{
			candidates << binDir + "/OVMF_CODE.fd"
			           << binDir + "/OVMF_CODE_4M.fd"
			           << binDir + "/edk2-x86_64-code.fd"
			           << binDir + "/share/OVMF_CODE.fd"
			           << binDir + "/share/OVMF_CODE_4M.fd"
			           << binDir + "/share/edk2-x86_64-code.fd"
			           << binDir + "/../share/qemu/OVMF_CODE.fd"
			           << binDir + "/../share/qemu/OVMF_CODE_4M.fd"
			           << binDir + "/../share/qemu/edk2-x86_64-code.fd"
			           << binDir + "/../share/edk2-x86_64-code.fd";
		}
		else
		{
			candidates << binDir + "/edk2-aarch64-code.fd"
			           << binDir + "/share/edk2-aarch64-code.fd"
			           << binDir + "/../share/qemu/edk2-aarch64-code.fd"
			           << binDir + "/../share/edk2-aarch64-code.fd"
			           << binDir + "/QEMU_EFI.fd"
			           << binDir + "/AAVMF_CODE.fd";
		}
	}
	if( x86 )
	{
		candidates << "C:/Program Files/qemu/share/OVMF_CODE.fd"
		           << "C:/Program Files/qemu/share/OVMF_CODE_4M.fd"
		           << "C:/Program Files/qemu/share/edk2-x86_64-code.fd"
		           << "C:/Program Files/qemu/OVMF_CODE.fd"
		           << "C:/msys64/ucrt64/share/qemu/OVMF_CODE.fd"
		           << "C:/msys64/ucrt64/share/qemu/OVMF_CODE_4M.fd"
		           << "C:/msys64/ucrt64/share/qemu/edk2-x86_64-code.fd"
		           << "C:/msys64/mingw64/share/qemu/OVMF_CODE.fd"
		           << "C:/msys64/mingw64/share/qemu/edk2-x86_64-code.fd";
	}
	else
	{
		candidates << "C:/Program Files/qemu/share/edk2-aarch64-code.fd"
		           << "C:/Program Files/qemu/edk2-aarch64-code.fd"
		           << "C:/msys64/ucrt64/share/qemu/edk2-aarch64-code.fd"
		           << "C:/msys64/mingw64/share/qemu/edk2-aarch64-code.fd";
	}
	#else
	if( x86 )
	{
		candidates << "/usr/share/OVMF/OVMF_CODE.fd"
		           << "/usr/share/OVMF/OVMF_CODE_4M.fd"
		           << "/usr/share/OVMF/OVMF_CODE.secboot.fd"
		           << "/usr/share/qemu/OVMF_CODE.fd"
		           << "/usr/share/qemu/edk2-x86_64-code.fd"
		           << "/usr/share/edk2/x64/OVMF_CODE.fd"
		           << "/usr/share/edk2-ovmf/x64/OVMF_CODE.fd";
	}
	else
	{
		candidates << "/usr/share/AAVMF/AAVMF_CODE.no-secboot.fd"
		           << "/usr/share/AAVMF/AAVMF_CODE.fd"
		           << "/usr/share/qemu-efi-aarch64/QEMU_EFI.fd"
		           << "/usr/share/qemu/edk2-aarch64-code.fd"
		           << "/usr/share/edk2/aarch64/QEMU_EFI.fd"
		           << "/usr/share/OVMF/AAVMF_CODE.fd";
	}
	#endif

	return First_Existing_Path( candidates );
}

QString Find_UEFI_Firmware_VARS_Template( const QString &qemu_binary_path, const QString &arch )
{
	QStringList candidates;
	const bool x86 = UEFI_Arch_Is_X86( arch );

	#ifdef Q_OS_WIN32
	QString binDir;
	if( ! qemu_binary_path.isEmpty() )
		binDir = QFileInfo( qemu_binary_path ).absolutePath();

	if( ! binDir.isEmpty() )
	{
		if( x86 )
		{
			candidates << binDir + "/OVMF_VARS.fd"
			           << binDir + "/OVMF_VARS_4M.fd"
			           << binDir + "/edk2-i386-vars.fd"
			           << binDir + "/share/OVMF_VARS.fd"
			           << binDir + "/share/OVMF_VARS_4M.fd"
			           << binDir + "/share/edk2-i386-vars.fd"
			           << binDir + "/../share/qemu/OVMF_VARS.fd"
			           << binDir + "/../share/qemu/OVMF_VARS_4M.fd"
			           << binDir + "/../share/qemu/edk2-i386-vars.fd";
		}
		else
		{
			candidates << binDir + "/edk2-arm-vars.fd"
			           << binDir + "/share/edk2-arm-vars.fd"
			           << binDir + "/../share/qemu/edk2-arm-vars.fd"
			           << binDir + "/../share/edk2-arm-vars.fd"
			           << binDir + "/QEMU_VARS.fd"
			           << binDir + "/AAVMF_VARS.fd";
		}
	}
	if( x86 )
	{
		candidates << "C:/Program Files/qemu/share/OVMF_VARS.fd"
		           << "C:/Program Files/qemu/share/OVMF_VARS_4M.fd"
		           << "C:/Program Files/qemu/share/edk2-i386-vars.fd"
		           << "C:/Program Files/qemu/OVMF_VARS.fd"
		           << "C:/msys64/ucrt64/share/qemu/OVMF_VARS.fd"
		           << "C:/msys64/ucrt64/share/qemu/OVMF_VARS_4M.fd"
		           << "C:/msys64/ucrt64/share/qemu/edk2-i386-vars.fd"
		           << "C:/msys64/mingw64/share/qemu/OVMF_VARS.fd"
		           << "C:/msys64/mingw64/share/qemu/edk2-i386-vars.fd";
	}
	else
	{
		candidates << "C:/Program Files/qemu/share/edk2-arm-vars.fd"
		           << "C:/Program Files/qemu/edk2-arm-vars.fd"
		           << "C:/msys64/ucrt64/share/qemu/edk2-arm-vars.fd"
		           << "C:/msys64/mingw64/share/qemu/edk2-arm-vars.fd";
	}
	#else
	if( x86 )
	{
		candidates << "/usr/share/OVMF/OVMF_VARS.fd"
		           << "/usr/share/OVMF/OVMF_VARS_4M.fd"
		           << "/usr/share/qemu/OVMF_VARS.fd"
		           << "/usr/share/qemu/edk2-i386-vars.fd"
		           << "/usr/share/edk2/x64/OVMF_VARS.fd"
		           << "/usr/share/edk2-ovmf/x64/OVMF_VARS.fd";
	}
	else
	{
		candidates << "/usr/share/AAVMF/AAVMF_VARS.fd"
		           << "/usr/share/qemu-efi-aarch64/QEMU_VARS.fd"
		           << "/usr/share/qemu/edk2-arm-vars.fd"
		           << "/usr/share/edk2/aarch64/QEMU_VARS.fd"
		           << "/usr/share/OVMF/AAVMF_VARS.fd";
	}
	#endif

	return First_Existing_Path( candidates );
}

bool Prepare_UEFI_VARS_File( const QString &dest_path, const QString &qemu_binary_path,
                             const QString &arch )
{
	if( dest_path.isEmpty() )
		return false;

	if( QFile::exists( dest_path ) )
		return true;

	QString tmpl = Find_UEFI_Firmware_VARS_Template( qemu_binary_path, arch );
	if( tmpl.isEmpty() )
	{
		AQError( "Prepare_UEFI_VARS_File", "UEFI VARS template not found" );
		return false;
	}
	
	QDir().mkpath( QFileInfo( dest_path ).absolutePath() );
	if( QFile::exists( dest_path ) )
		QFile::remove( dest_path );
	
	if( ! QFile::copy( tmpl, dest_path ) )
	{
		AQError( "Prepare_UEFI_VARS_File", "Failed to copy VARS template to " + dest_path );
		return false;
	}
	
	// Ensure VARS is writable
	QFile::setPermissions( dest_path,
		QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser |
		QFile::ReadGroup | QFile::ReadOther );
	
	return true;
}

QString Win11_OOBE_Bypass_Guest_Commands()
{
	return QStringLiteral(
		"rem === Win11 ARM OOBE: skip Microsoft account / \"An error occurred\" ===\r\n"
		"rem Open guest CMD with Shift+F10 (or AQEMU toolbar Send Shift+F10), then try in order:\r\n"
		"\r\n"
		"rem 1) Local-account OOBE URI (works on many 24H2+ builds):\r\n"
		"start ms-cxh:localonly\r\n"
		"\r\n"
		"rem 2) Classic BypassNRO + reboot (if step 1 does nothing):\r\n"
		"reg add \"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OOBE\" /v BypassNRO /t REG_DWORD /d 1 /f\r\n"
		"shutdown /r /t 0\r\n"
		"\r\n"
		"rem 3) Optional: kill networking so OOBE offers offline path:\r\n"
		"ipconfig /release\r\n"
	);
}

QString Win11_UCPD_Guest_Commands()
{
	return QStringLiteral(
		"rem === Remove UCPD.sys (BVM fix for post-OOBE reboot / BSOD) ===\r\n"
		"rem Run in guest CMD (Shift+F10). Then reboot.\r\n"
		"\r\n"
		"takeown /f C:\\Windows\\System32\\drivers\\UCPD.sys\r\n"
		"icacls C:\\Windows\\System32\\drivers\\UCPD.sys /grant *S-1-5-32-544:F\r\n"
		"del /f /q C:\\Windows\\System32\\drivers\\UCPD.sys\r\n"
		"reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\UCPD\" /v Start /t REG_DWORD /d 4 /f\r\n"
		"shutdown /r /t 0\r\n"
	);
}

static QString Find_QEMU_NBD_Path()
{
	const QString img = Get_QEMU_IMG_Path();
	QFileInfo fi( img );
	QStringList candidates;
#ifdef Q_OS_WIN32
	const QString exe = QStringLiteral( ".exe" );
#else
	const QString exe;
#endif
	if( fi.exists() )
		candidates << fi.absolutePath() + QDir::separator() + "qemu-nbd" + exe;
	candidates
		<< QCoreApplication::applicationDirPath() + QDir::separator() + "qemu-nbd" + exe
		<< QStringLiteral( "/usr/bin/qemu-nbd" )
		<< QStringLiteral( "/usr/local/bin/qemu-nbd" )
		<< QStringLiteral( "qemu-nbd" ) + exe;

	for( const QString &c : candidates )
	{
		if( QFile::exists( c ) && QFileInfo( c ).isFile() )
			return QDir::toNativeSeparators( c );
	}
	return QString();
}

bool Remove_UCPD_From_Disk_Image( const QString &disk_path, QString *result_message )
{
	auto set_msg = [ result_message ]( const QString &m ) {
		if( result_message )
			*result_message = m;
	};

	if( disk_path.trimmed().isEmpty() || ! QFile::exists( disk_path ) )
	{
		set_msg( QObject::tr( "Disk image not found." ) );
		return false;
	}

#ifdef Q_OS_WIN32
	set_msg( QObject::tr(
		"Automatic UCPD.sys removal from the disk image needs Linux tools "
		"(qemu-nbd + NTFS mount), which are not available on this Windows host.\n\n"
		"Shut the VM down is optional — while OOBE is on screen, press Shift+F10 "
		"(or use Send Shift+F10) and paste the UCPD guest commands from the helper dialog." ) );
	return false;
#else
	const QString nbd_bin = Find_QEMU_NBD_Path();
	if( nbd_bin.isEmpty() )
	{
		set_msg( QObject::tr( "qemu-nbd not found next to qemu-img. Install qemu-utils." ) );
		return false;
	}

	// Prefer pkexec when not root so the helper can still work from the GUI.
	const bool need_elev = ( ::geteuid() != 0 );
	auto run_root = [ need_elev ]( const QString &prog, const QStringList &args,
								   QString *out, int timeout_ms = 60000 ) -> int {
		QProcess p;
		if( need_elev )
		{
			QStringList a;
			a << prog << args;
			p.start( QStringLiteral( "pkexec" ), a );
		}
		else
			p.start( prog, args );
		if( ! p.waitForFinished( timeout_ms ) )
		{
			p.kill();
			if( out ) *out = QObject::tr( "Command timed out: %1" ).arg( prog );
			return -1;
		}
		if( out )
			*out = QString::fromLocal8Bit( p.readAllStandardOutput() + p.readAllStandardError() ).trimmed();
		return p.exitCode();
	};

	QString err;
	run_root( QStringLiteral( "modprobe" ), QStringList() << QStringLiteral( "nbd" )
			  << QStringLiteral( "max_part=16" ), &err );

	QString nbd_dev;
	for( int i = 0; i < 16; ++i )
	{
		const QString candidate = QStringLiteral( "/dev/nbd%1" ).arg( i );
		if( ! QFile::exists( candidate ) )
			continue;
		QProcess lsblk;
		lsblk.start( QStringLiteral( "lsblk" ),
					 QStringList() << QStringLiteral( "-no" ) << QStringLiteral( "MOUNTPOINTS" ) << candidate );
		lsblk.waitForFinished( 5000 );
		const QString mounts = QString::fromLocal8Bit( lsblk.readAllStandardOutput() ).trimmed();
		if( mounts.isEmpty() )
		{
			nbd_dev = candidate;
			break;
		}
	}
	if( nbd_dev.isEmpty() )
	{
		set_msg( QObject::tr( "No free /dev/nbd* device found." ) );
		return false;
	}

	QString connect_out;
	const int connect_rc = run_root( nbd_bin,
		QStringList() << QStringLiteral( "--connect=" ) + nbd_dev << disk_path,
		&connect_out, 30000 );
	if( connect_rc != 0 )
	{
		set_msg( QObject::tr( "qemu-nbd connect failed:\n%1" ).arg( connect_out ) );
		return false;
	}

	auto disconnect_nbd = [ & ]() {
		QString dummy;
		run_root( nbd_bin, QStringList() << QStringLiteral( "--disconnect" ) << nbd_dev, &dummy, 15000 );
	};

	// Wait for partitions (Windows typically uses partition 3 or 4 for C:)
	QStringList part_candidates;
	for( int wait = 0; wait < 10; ++wait )
	{
		part_candidates.clear();
		for( int p = 1; p <= 8; ++p )
		{
			const QString part = nbd_dev + QStringLiteral( "p%1" ).arg( p );
			if( QFile::exists( part ) )
				part_candidates << part;
		}
		if( ! part_candidates.isEmpty() )
			break;
		QProcess::execute( QStringLiteral( "sleep" ), QStringList() << QStringLiteral( "1" ) );
	}
	if( part_candidates.isEmpty() )
	{
		disconnect_nbd();
		set_msg( QObject::tr( "No partitions appeared on %1 after connecting the disk." ).arg( nbd_dev ) );
		return false;
	}

	const QString mountpoint = QStringLiteral( "/tmp/aqemu-ucpd-%1" ).arg( QCoreApplication::applicationPid() );
	run_root( QStringLiteral( "mkdir" ), QStringList() << QStringLiteral( "-p" ) << mountpoint, &err );

	QString windows_root;
	QString mounted_part;
	for( const QString &part : part_candidates )
	{
		run_root( QStringLiteral( "umount" ), QStringList() << mountpoint, &err, 10000 );
		QString mout;
		int mrc = run_root( QStringLiteral( "mount" ),
			QStringList() << QStringLiteral( "-t" ) << QStringLiteral( "ntfs3" )
						  << QStringLiteral( "-o" ) << QStringLiteral( "rw,remove_hiberfile" )
						  << part << mountpoint,
			&mout, 20000 );
		if( mrc != 0 )
		{
			mrc = run_root( QStringLiteral( "mount" ),
				QStringList() << QStringLiteral( "-t" ) << QStringLiteral( "ntfs-3g" )
							  << QStringLiteral( "-o" ) << QStringLiteral( "rw,remove_hiberfile" )
							  << part << mountpoint,
				&mout, 20000 );
		}
		if( mrc != 0 )
			continue;

		if( QFile::exists( mountpoint + QStringLiteral( "/Windows/System32" ) ) )
		{
			windows_root = mountpoint;
			mounted_part = part;
			break;
		}
		run_root( QStringLiteral( "umount" ), QStringList() << mountpoint, &err, 10000 );
	}

	if( windows_root.isEmpty() )
	{
		run_root( QStringLiteral( "umount" ), QStringList() << mountpoint, &err, 10000 );
		disconnect_nbd();
		run_root( QStringLiteral( "rmdir" ), QStringList() << mountpoint, &err, 5000 );
		set_msg( QObject::tr(
			"Could not find a Windows partition (Windows\\System32) on this disk. "
			"Use the guest Shift+F10 UCPD commands instead." ) );
		return false;
	}

	const QString ucpd = windows_root + QStringLiteral( "/Windows/System32/drivers/UCPD.sys" );
	bool removed = false;
	if( QFile::exists( ucpd ) )
	{
		QString rm_out;
		const int rrc = run_root( QStringLiteral( "rm" ), QStringList() << QStringLiteral( "-f" ) << ucpd, &rm_out );
		removed = ( rrc == 0 && ! QFile::exists( ucpd ) );
		if( ! removed )
		{
			run_root( QStringLiteral( "umount" ), QStringList() << mountpoint, &err, 10000 );
			disconnect_nbd();
			run_root( QStringLiteral( "rmdir" ), QStringList() << mountpoint, &err, 5000 );
			set_msg( QObject::tr( "Failed to delete UCPD.sys:\n%1" ).arg( rm_out ) );
			return false;
		}
	}

	run_root( QStringLiteral( "umount" ), QStringList() << mountpoint, &err, 15000 );
	disconnect_nbd();
	run_root( QStringLiteral( "rmdir" ), QStringList() << mountpoint, &err, 5000 );

	if( removed )
		set_msg( QObject::tr( "Removed UCPD.sys from %1 (partition %2)." )
				 .arg( disk_path, mounted_part ) );
	else
		set_msg( QObject::tr( "UCPD.sys was already absent on %1." ).arg( disk_path ) );
	return true;
#endif
}
