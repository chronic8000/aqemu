#include "Guest_Files.h"

#include <QFile>
#include <QTemporaryDir>

#include <cstdio>

static int fail( const char *msg )
{
	std::fprintf( stderr, "%s\n", msg );
	return 1;
}

int main()
{
	QTemporaryDir dir;
	if( ! dir.isValid() )
		return fail( "temp dir" );

	const QString src = dir.filePath( QStringLiteral( "My Document.txt" ) );
	QFile in( src );
	if( ! in.open( QIODevice::WriteOnly ) )
		return fail( "write source" );
	const QByteArray body( "hello from aqemu" );
	in.write( body );
	in.close();

	const QString img = dir.filePath( QStringLiteral( "paste.img" ) );
	QString error;
	bool floppy = false;
	if( ! AQ_Write_Guest_Fat_Image( img, QStringList() << src, &floppy, &error ) )
		return fail( error.toUtf8().constData() );
	if( ! floppy )
		return fail( "small file should be a floppy" );

	const QByteArray back = AQ_Read_Guest_Fat_File( img, QStringLiteral( "My Document.txt" ) );
	if( back != body )
		return fail( "round trip mismatch" );

	QFile big( dir.filePath( QStringLiteral( "big.bin" ) ) );
	if( ! big.open( QIODevice::WriteOnly ) )
		return fail( "big" );
	big.write( QByteArray( 2 * 1024 * 1024, 'Z' ) );
	big.close();
	const QString img2 = dir.filePath( QStringLiteral( "paste2.img" ) );
	bool floppy2 = true;
	if( ! AQ_Write_Guest_Fat_Image( img2, QStringList() << big.fileName(), &floppy2, &error ) )
		return fail( error.toUtf8().constData() );
	if( floppy2 )
		return fail( "large file should be a hard-disk image" );
	const QByteArray back2 = AQ_Read_Guest_Fat_File( img2, QStringLiteral( "big.bin" ) );
	if( back2.size() != 2 * 1024 * 1024 || back2.at( 0 ) != 'Z' || back2.at( back2.size() - 1 ) != 'Z' )
		return fail( "fat16 round trip" );

	QFile just( dir.filePath( QStringLiteral( "just.bin" ) ) );
	if( ! just.open( QIODevice::WriteOnly ) )
		return fail( "just" );
	just.write( QByteArray( 1500 * 1024, 'Q' ) );
	just.close();
	const QString img3 = dir.filePath( QStringLiteral( "paste3.img" ) );
	bool floppy3 = true;
	if( ! AQ_Write_Guest_Fat_Image( img3, QStringList() << just.fileName(), &floppy3, &error ) )
		return fail( error.toUtf8().constData() );
	if( floppy3 )
		return fail( "just over floppy limit should be a hard-disk image" );
	QFile disk( img3 );
	if( ! disk.open( QIODevice::ReadOnly ) )
		return fail( "open disk" );
	const QByteArray head = disk.read( 512 );
	disk.close();
	if( head.size() < 512 || quint8( head.at( 510 ) ) != 0x55 || quint8( head.at( 511 ) ) != 0xaa )
		return fail( "missing mbr" );
	if( quint8( head.at( 0 ) ) == 0xeb )
		return fail( "usb image is a superfloppy" );
	const int lba = int( quint8( head.at( 454 ) ) | ( quint8( head.at( 455 ) ) << 8 )
		| ( quint8( head.at( 456 ) ) << 16 ) | ( quint8( head.at( 457 ) ) << 24 ) );
	if( lba != 2048 )
		return fail( "partition start" );
	const QByteArray back3 = AQ_Read_Guest_Fat_File( img3, QStringLiteral( "just.bin" ) );
	if( back3.size() != 1500 * 1024 || back3.at( 0 ) != 'Q' )
		return fail( "partition round trip" );

	const QString img4 = dir.filePath( QStringLiteral( "paste4.img" ) );
	bool floppy4 = true;
	if( ! AQ_Write_Guest_Fat_Image( img4, QStringList() << src, &floppy4, &error, false ) )
		return fail( error.toUtf8().constData() );
	if( floppy4 )
		return fail( "forced hard disk" );
	if( AQ_Read_Guest_Fat_File( img4, QStringLiteral( "My Document.txt" ) ) != body )
		return fail( "small file on usb" );

	return 0;
}
