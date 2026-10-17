#include "Guest_Files.h"

#include <QByteArray>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>

#include <QtGlobal>

static void Put16( QByteArray &b, int off, quint16 v )
{
	b[ off ] = char( v & 0xff );
	b[ off + 1 ] = char( ( v >> 8 ) & 0xff );
}

static void Put32( QByteArray &b, int off, quint32 v )
{
	Put16( b, off, quint16( v & 0xffff ) );
	Put16( b, off + 2, quint16( ( v >> 16 ) & 0xffff ) );
}

static quint16 Get16( const QByteArray &b, int off )
{
	return quint16( quint8( b.at( off ) ) | ( quint16( quint8( b.at( off + 1 ) ) ) << 8 ) );
}

static quint32 Get32( const QByteArray &b, int off )
{
	return quint32( Get16( b, off ) ) | ( quint32( Get16( b, off + 2 ) ) << 16 );
}

struct Fat_Item
{
	QString display;
	QByteArray short_name;
	QByteArray data;
};

static QByteArray Make_Short_Name( const QString &file_name, int index )
{
	const QFileInfo fi( file_name );
	auto clean = []( const QString &in, int limit ) -> QString {
		QString out;
		const QString up = in.toUpper();
		for( int i = 0; i < up.size(); ++i )
		{
			const QChar c = up.at( i );
			if( c.isLetterOrNumber() )
				out += c;
			if( out.size() >= limit )
				break;
		}
		return out;
	};
	QString base = clean( fi.completeBaseName(), 8 );
	if( base.isEmpty() )
		base = QStringLiteral( "FILE" );
	const QString ext = clean( fi.suffix(), 3 );
	const QString num = QString::number( index );
	const int room = 8 - 1 - num.size();
	if( room < 1 )
		base = QStringLiteral( "F" );
	else if( base.size() > room )
		base = base.left( room );
	base += QLatin1Char( '~' ) + num;

	QByteArray out( 11, ' ' );
	for( int i = 0; i < base.size() && i < 8; ++i )
		out[ i ] = base.at( i ).toLatin1();
	for( int i = 0; i < ext.size() && i < 3; ++i )
		out[ 8 + i ] = ext.at( i ).toLatin1();
	return out;
}

static quint8 Short_Checksum( const QByteArray &name11 )
{
	quint8 sum = 0;
	for( int i = 0; i < 11; ++i )
		sum = quint8( ( ( sum & 1 ) << 7 ) + ( sum >> 1 ) + quint8( name11.at( i ) ) );
	return sum;
}

static void Write_Lfn_Slot( QByteArray &dir, int off, int seq, bool last,
                            const QString &name, quint8 checksum )
{
	for( int i = 0; i < 32; ++i )
		dir[ off + i ] = 0;
	dir[ off ] = char( seq | ( last ? 0x40 : 0 ) );
	dir[ off + 11 ] = char( 0x0f );
	dir[ off + 13 ] = char( checksum );
	const int lfn_off[ 13 ] = { 1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30 };
	for( int i = 0; i < 13; ++i )
	{
		const int pos = ( seq - 1 ) * 13 + i;
		quint16 ch = 0xffff;
		if( pos < name.size() )
			ch = name.at( pos ).unicode();
		else if( pos == name.size() )
			ch = 0;
		dir[ off + lfn_off[ i ] ] = char( ch & 0xff );
		dir[ off + lfn_off[ i ] + 1 ] = char( ( ch >> 8 ) & 0xff );
	}
}

static QStringList Collect_Files( const QStringList &inputs, QString *error )
{
	QStringList out;
	for( int i = 0; i < inputs.size(); ++i )
	{
		const QFileInfo fi( inputs.at( i ) );
		if( ! fi.exists() )
		{
			if( error )
				*error = QStringLiteral( "Not found: %1" ).arg( inputs.at( i ) );
			return QStringList();
		}
		if( fi.isDir() )
		{
			QDirIterator it( fi.absoluteFilePath(), QDir::Files, QDirIterator::Subdirectories );
			while( it.hasNext() )
				out << it.next();
		}
		else
			out << fi.absoluteFilePath();
	}
	if( out.isEmpty() && error )
		*error = QStringLiteral( "No files to copy." );
	return out;
}

QStringList AQ_File_Paths_From_Mime( const QMimeData *mime )
{
	QStringList paths;
	if( ! mime )
		return paths;
	const QList<QUrl> urls = mime->urls();
	for( int i = 0; i < urls.size(); ++i )
	{
		const QString local = urls.at( i ).toLocalFile();
		if( ! local.isEmpty() )
			paths << local;
	}
	if( ! paths.isEmpty() )
		return paths;
	const QString text = mime->text();
	const QStringList lines = text.split( QLatin1Char( '\n' ), QString::SkipEmptyParts );
	for( int i = 0; i < lines.size(); ++i )
	{
		QString line = lines.at( i ).trimmed();
		if( line.startsWith( QLatin1String( "file://" ) ) )
			line = QUrl( line ).toLocalFile();
		if( ! line.isEmpty() && QFileInfo( line ).exists() )
			paths << line;
	}
	return paths;
}

bool AQ_Write_Guest_Fat_Image( const QString &image_path, const QStringList &source_files,
                               bool *floppy_out, QString *error )
{
	if( floppy_out )
		*floppy_out = false;
	const QStringList files = Collect_Files( source_files, error );
	if( files.isEmpty() )
		return false;

	QList<Fat_Item> items;
	qint64 total = 0;
	for( int i = 0; i < files.size(); ++i )
	{
		QFile f( files.at( i ) );
		if( ! f.open( QIODevice::ReadOnly ) )
		{
			if( error )
				*error = QStringLiteral( "Cannot read %1" ).arg( files.at( i ) );
			return false;
		}
		Fat_Item item;
		item.display = QFileInfo( files.at( i ) ).fileName();
		item.short_name = Make_Short_Name( item.display, i + 1 );
		item.data = f.readAll();
		f.close();
		total += item.data.size();
		items << item;
	}
	if( total > qint64( 512 ) * 1024 * 1024 )
	{
		if( error )
			*error = QStringLiteral( "Those files are larger than 512 MB. Copy them in smaller groups." );
		return false;
	}

	const bool floppy = total <= 1400 * 1024 && items.size() <= 40;
	if( floppy_out )
		*floppy_out = floppy;

	const int bytes_per_sector = 512;
	int sectors_per_cluster = 1;
	int reserved = 1;
	int fats = 2;
	int root_entries = floppy ? 224 : 512;
	int sectors_per_fat = floppy ? 9 : 0;
	int total_sectors = floppy ? 2880 : 0;
	const quint8 media = floppy ? 0xf0 : 0xf8;

	if( ! floppy )
	{
		const int options[] = { 1, 2, 4, 8, 16, 32, 64 };
		bool picked = false;
		for( int oi = 0; oi < 7; ++oi )
		{
			sectors_per_cluster = options[ oi ];
			const qint64 cluster_bytes = qint64( bytes_per_sector ) * sectors_per_cluster;
			const qint64 clusters = ( total + cluster_bytes - 1 ) / cluster_bytes + items.size() + 4;
			if( clusters >= 65500 )
				continue;
			const int fat_bytes = int( ( clusters + 2 ) * 2 );
			sectors_per_fat = ( fat_bytes + bytes_per_sector - 1 ) / bytes_per_sector;
			const int root_sectors = ( root_entries * 32 ) / bytes_per_sector;
			const int data_sectors = int( clusters ) * sectors_per_cluster;
			total_sectors = reserved + fats * sectors_per_fat + root_sectors + data_sectors;
			picked = true;
			break;
		}
		if( ! picked )
		{
			if( error )
				*error = QStringLiteral( "Could not fit those files in a FAT16 image." );
			return false;
		}
	}

	const int root_sectors = ( root_entries * 32 ) / bytes_per_sector;
	const int data_start = reserved + fats * sectors_per_fat + root_sectors;
	if( floppy && data_start + int( ( total + 511 ) / 512 ) > total_sectors )
	{
		if( error )
			*error = QStringLiteral( "Those files do not fit on a 1.44 MB floppy." );
		return false;
	}

	QByteArray image( total_sectors * bytes_per_sector, '\0' );
	image[ 0 ] = char( 0xeb );
	image[ 1 ] = char( 0x3c );
	image[ 2 ] = char( 0x90 );
	const QByteArray oem( "AQEMU   " );
	for( int i = 0; i < 8; ++i )
		image[ 3 + i ] = oem.at( i );
	Put16( image, 11, quint16( bytes_per_sector ) );
	image[ 13 ] = char( sectors_per_cluster );
	Put16( image, 14, quint16( reserved ) );
	image[ 16 ] = char( fats );
	Put16( image, 17, quint16( root_entries ) );
	if( total_sectors < 65536 )
		Put16( image, 19, quint16( total_sectors ) );
	image[ 21 ] = char( media );
	Put16( image, 22, quint16( sectors_per_fat ) );
	Put16( image, 24, floppy ? 18 : 32 );
	Put16( image, 26, floppy ? 2 : 64 );
	Put32( image, 32, total_sectors >= 65536 ? quint32( total_sectors ) : 0 );
	image[ 36 ] = char( 0x80 );
	image[ 38 ] = char( 0x29 );
	Put32( image, 39, 0x4151454d );
	const QByteArray label( "AQEMU      " );
	for( int i = 0; i < 11; ++i )
		image[ 43 + i ] = label.at( i );
	const QByteArray fstype( floppy ? "FAT12   " : "FAT16   " );
	for( int i = 0; i < 8; ++i )
		image[ 54 + i ] = fstype.at( i );
	image[ 510 ] = char( 0x55 );
	image[ 511 ] = char( 0xaa );

	QByteArray fat( sectors_per_fat * bytes_per_sector, '\0' );
	auto set_fat = [&]( int cluster, int value ) {
		if( floppy )
		{
			const int offset = cluster + cluster / 2;
			if( cluster & 1 )
			{
				fat[ offset ] = char( ( quint8( fat.at( offset ) ) & 0x0f ) | ( ( value << 4 ) & 0xf0 ) );
				fat[ offset + 1 ] = char( ( value >> 4 ) & 0xff );
			}
			else
			{
				fat[ offset ] = char( value & 0xff );
				fat[ offset + 1 ] = char( ( quint8( fat.at( offset + 1 ) ) & 0xf0 ) | ( ( value >> 8 ) & 0x0f ) );
			}
		}
		else
		{
			Put16( fat, cluster * 2, quint16( value ) );
		}
	};
	set_fat( 0, floppy ? 0x0ff8 : 0xfff8 );
	set_fat( 1, 0xffff );
	for( int f = 0; f < fats; ++f )
	{
		const int off = ( reserved + f * sectors_per_fat ) * bytes_per_sector;
		for( int i = 0; i < fat.size(); ++i )
			image[ off + i ] = fat.at( i );
	}

	QByteArray root( root_entries * 32, '\0' );
	int dir_index = 1;
	root[ 0 ] = 'A'; root[ 1 ] = 'Q'; root[ 2 ] = 'E'; root[ 3 ] = 'M'; root[ 4 ] = 'U';
	for( int i = 5; i < 11; ++i )
		root[ i ] = ' ';
	root[ 11 ] = char( 0x08 );

	int next_cluster = 2;
	for( int n = 0; n < items.size(); ++n )
	{
		const Fat_Item &item = items.at( n );
		const int nslots = ( item.display.size() + 12 ) / 13;
		if( dir_index + nslots + 1 >= root_entries )
		{
			if( error )
				*error = QStringLiteral( "Too many files for one disk." );
			return false;
		}
		const quint8 sum = Short_Checksum( item.short_name );
		for( int s = nslots; s >= 1; --s )
		{
			Write_Lfn_Slot( root, dir_index * 32, s, s == nslots, item.display, sum );
			++dir_index;
		}
		const int ent = dir_index * 32;
		for( int i = 0; i < 11; ++i )
			root[ ent + i ] = item.short_name.at( i );
		root[ ent + 11 ] = char( 0x20 );
		int start = 0;
		if( ! item.data.isEmpty() )
		{
			start = next_cluster;
			const int cluster_bytes = bytes_per_sector * sectors_per_cluster;
			int left = item.data.size();
			int src = 0;
			int cluster = start;
			while( left > 0 )
			{
				const int take = qMin( left, cluster_bytes );
				const int sector = data_start + ( cluster - 2 ) * sectors_per_cluster;
				const int off = sector * bytes_per_sector;
				if( off + take > image.size() )
				{
					if( error )
						*error = QStringLiteral( "The disk image ran out of space." );
					return false;
				}
				for( int b = 0; b < take; ++b )
					image[ off + b ] = item.data.at( src + b );
				left -= take;
				src += take;
				const int nxt = next_cluster + 1;
				++next_cluster;
				if( left > 0 )
					set_fat( cluster, nxt );
				else
					set_fat( cluster, 0xffff );
				cluster = nxt;
			}
		}
		Put16( root, ent + 26, quint16( start ) );
		Put32( root, ent + 28, quint32( item.data.size() ) );
		++dir_index;
	}

	for( int f = 0; f < fats; ++f )
	{
		const int off = ( reserved + f * sectors_per_fat ) * bytes_per_sector;
		for( int i = 0; i < fat.size(); ++i )
			image[ off + i ] = fat.at( i );
	}
	const int root_off = ( reserved + fats * sectors_per_fat ) * bytes_per_sector;
	for( int i = 0; i < root.size(); ++i )
		image[ root_off + i ] = root.at( i );

	QFile out( image_path );
	if( ! out.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
		if( error )
			*error = QStringLiteral( "Cannot write %1" ).arg( image_path );
		return false;
	}
	if( out.write( image ) != image.size() )
	{
		if( error )
			*error = QStringLiteral( "Could not finish writing %1" ).arg( image_path );
		return false;
	}
	return true;
}

static int Fat_Entry( const QByteArray &fat, bool floppy, int cluster )
{
	if( floppy )
	{
		const int offset = cluster + cluster / 2;
		int value;
		if( cluster & 1 )
			value = ( ( quint8( fat.at( offset ) ) >> 4 ) | ( quint8( fat.at( offset + 1 ) ) << 4 ) ) & 0x0fff;
		else
			value = ( quint8( fat.at( offset ) ) | ( ( quint8( fat.at( offset + 1 ) ) & 0x0f ) << 8 ) ) & 0x0fff;
		return value;
	}
	return Get16( fat, cluster * 2 );
}

QByteArray AQ_Read_Guest_Fat_File( const QString &image_path, const QString &file_name )
{
	QFile f( image_path );
	if( ! f.open( QIODevice::ReadOnly ) )
		return QByteArray();
	const QByteArray image = f.readAll();
	if( image.size() < 512 )
		return QByteArray();
	const int bps = Get16( image, 11 );
	const int spc = quint8( image.at( 13 ) );
	const int reserved = Get16( image, 14 );
	const int fats = quint8( image.at( 16 ) );
	const int root_entries = Get16( image, 17 );
	int total = Get16( image, 19 );
	if( total == 0 )
		total = int( Get32( image, 32 ) );
	const int spf = Get16( image, 22 );
	const bool floppy = QString::fromLatin1( image.mid( 54, 5 ) ).startsWith( QLatin1String( "FAT12" ) );
	const int root_sectors = ( root_entries * 32 + bps - 1 ) / bps;
	const int root_off = ( reserved + fats * spf ) * bps;
	const int data_start = reserved + fats * spf + root_sectors;
	const QByteArray fat = image.mid( reserved * bps, spf * bps );

	QString lfn;
	for( int i = 0; i < root_entries; ++i )
	{
		const int off = root_off + i * 32;
		if( off + 32 > image.size() )
			break;
		const quint8 first = quint8( image.at( off ) );
		if( first == 0 )
			break;
		if( first == 0xe5 )
		{
			lfn.clear();
			continue;
		}
		if( quint8( image.at( off + 11 ) ) == 0x0f )
		{
			const int seq = first & 0x1f;
			const int lfn_off[ 13 ] = { 1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30 };
			QString part;
			for( int c = 0; c < 13; ++c )
			{
				const int p = off + lfn_off[ c ];
				const quint16 ch = quint16( quint8( image.at( p ) ) | ( quint16( quint8( image.at( p + 1 ) ) ) << 8 ) );
				if( ch == 0 || ch == 0xffff )
					break;
				part += QChar( ch );
			}
			lfn = part + lfn;
			continue;
		}
		const QString shown = lfn;
		lfn.clear();
		if( shown.compare( file_name, Qt::CaseInsensitive ) != 0 )
			continue;
		int cluster = Get16( image, off + 26 );
		const int size = int( Get32( image, off + 28 ) );
		const int end_of_chain = floppy ? 0x0ff8 : 0xfff8;
		QByteArray data;
		while( cluster >= 2 && cluster < end_of_chain && data.size() < size )
		{
			const int sector = data_start + ( cluster - 2 ) * spc;
			const int offb = sector * bps;
			const int take = qMin( spc * bps, size - data.size() );
			if( offb < 0 || offb + take > image.size() )
				break;
			data += image.mid( offb, take );
			const int next = Fat_Entry( fat, floppy, cluster );
			if( next >= end_of_chain || next == 0 )
				break;
			cluster = next;
		}
		return data.left( size );
	}
	Q_UNUSED( total );
	return QByteArray();
}
