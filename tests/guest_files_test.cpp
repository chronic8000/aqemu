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

	return 0;
}
