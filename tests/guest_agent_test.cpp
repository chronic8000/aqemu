#include "Guest_Agent.h"

#include <cstdio>

static int fail( const char *msg )
{
	std::fprintf( stderr, "%s\n", msg );
	return 1;
}

int main()
{
	AQ_Guest_Frame_Parser parser;
	QByteArray frame( "AQEMU1\nFROM-GUEST\n1\nnotes.txt\n5\nhello" );
	parser.Add( frame.left( 10 ) );
	parser.Add( frame.mid( 10 ) );
	const QList<AQ_Guest_File> files = parser.Take();
	if( parser.Error().isEmpty() == false )
		return fail( parser.Error().toUtf8().constData() );
	if( files.size() != 1 || files.at( 0 ).name != QLatin1String( "notes.txt" ) || files.at( 0 ).data != "hello" )
		return fail( "frame mismatch" );

	AQ_Guest_Frame_Parser bad;
	bad.Add( QByteArray( "NOPE\n" ) );
	if( bad.Error().isEmpty() )
		return fail( "bad magic accepted" );
	return 0;
}
