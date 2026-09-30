/****************************************************************************
**
** Copyright (C) 2026 Chronic Engineering
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

#include "Storage_Recovery.h"

#include <cstring>

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winioctl.h>
#endif

static const int kLabelBytes = 256 * 1024;
static const int kNvlistOffset = 16 * 1024;
static const quint32 kMdadmMagic = 0xa92b4efc;

QString Checkpoint_Failure_Text()
{
	return QStringLiteral( "Recovery checkpoint could not be created. No checkpoint was enabled and the original source disks were not modified." );
}

QString Discard_Confirm_Text()
{
	return QStringLiteral( "Discarding the checkpoint deletes the recovery overlays. Original source disks are unaffected." );
}

QString Keep_Button_Subtitle()
{
	return QStringLiteral( "Keeps the recovery overlays as this VM’s disks. Original source disks remain unchanged." );
}

QString Checkpoint_Section_Marker()
{
	return QStringLiteral( "--- checkpoint ---" );
}

static void Append_Be32( QByteArray &out, quint32 value )
{
	char b[4];
	b[0] = char( (value >> 24) & 0xff );
	b[1] = char( (value >> 16) & 0xff );
	b[2] = char( (value >> 8) & 0xff );
	b[3] = char( value & 0xff );
	out.append( b, 4 );
}

static void Append_Be64( QByteArray &out, quint64 value )
{
	Append_Be32( out, quint32( value >> 32 ) );
	Append_Be32( out, quint32( value & 0xffffffffu ) );
}

static void Append_Xdr_String( QByteArray &out, const QByteArray &text )
{
	Append_Be32( out, quint32( text.size() ) );
	out.append( text );
	const int pad = (4 - (text.size() % 4)) % 4;
	for( int i = 0; i < pad; ++i )
		out.append( '\0' );
}

static QByteArray Nv_Pair( const QByteArray &name, qint32 type, qint32 nelem, const QByteArray &value )
{
	QByteArray body;
	Append_Xdr_String( body, name );
	Append_Be32( body, quint32( type ) );
	Append_Be32( body, quint32( nelem ) );
	body.append( value );
	QByteArray pair;
	const quint32 packed = quint32( body.size() + 8 );
	Append_Be32( pair, packed );
	Append_Be32( pair, packed );
	pair.append( body );
	return pair;
}

static QByteArray Nv_End()
{
	QByteArray end;
	Append_Be32( end, 0 );
	Append_Be32( end, 0 );
	return end;
}

static QByteArray Nv_List( const QByteArray &pairs )
{
	QByteArray nv;
	Append_Be32( nv, 0 );
	Append_Be32( nv, 1 );
	nv.append( pairs );
	nv.append( Nv_End() );
	return nv;
}

static QByteArray Nv_U64( const char *name, quint64 value )
{
	QByteArray raw;
	Append_Be64( raw, value );
	return Nv_Pair( QByteArray( name ), 8, 1, raw );
}

static QByteArray Nv_Str( const char *name, const QString &text )
{
	const QByteArray utf = text.toUtf8();
	QByteArray raw;
	Append_Xdr_String( raw, utf );
	return Nv_Pair( QByteArray( name ), 9, 1, raw );
}

static QByteArray Nv_Nvlist( const char *name, const QByteArray &nvlist )
{
	return Nv_Pair( QByteArray( name ), 19, 1, nvlist );
}

static QByteArray Nv_Nvlist_Array( const char *name, const QList<QByteArray> &lists )
{
	QByteArray raw;
	for( int i = 0; i < lists.size(); ++i )
		raw.append( lists[i] );
	return Nv_Pair( QByteArray( name ), 20, lists.size(), raw );
}

QByteArray Build_Zfs_Label( const QString &pool_name, quint64 pool_guid, quint64 disk_guid,
	int ashift, const QList<quint64> &member_guids )
{
	QList<QByteArray> children;
	for( int i = 0; i < member_guids.size(); ++i )
	{
		QByteArray pairs;
		pairs.append( Nv_Str( "type", QStringLiteral( "disk" ) ) );
		pairs.append( Nv_U64( "guid", member_guids[i] ) );
		children << Nv_List( pairs );
	}

	QByteArray tree_pairs;
	tree_pairs.append( Nv_Str( "type", QStringLiteral( "root" ) ) );
	tree_pairs.append( Nv_Nvlist_Array( "children", children ) );
	const QByteArray tree = Nv_List( tree_pairs );

	QByteArray top;
	top.append( Nv_Str( "name", pool_name ) );
	top.append( Nv_U64( "pool_guid", pool_guid ) );
	top.append( Nv_U64( "guid", disk_guid ) );
	top.append( Nv_U64( "ashift", quint64( ashift ) ) );
	top.append( Nv_Nvlist( "vdev_tree", tree ) );

	QByteArray label( kLabelBytes, '\0' );
	const QByteArray packed = Nv_List( top );
	const int room = kLabelBytes - kNvlistOffset;
	const int n = qMin( packed.size(), room );
	memcpy( label.data() + kNvlistOffset, packed.constData(), size_t( n ) );
	return label;
}

bool Write_Zfs_Label_Fixture( const QString &path, qint64 size, const QByteArray &label, QString *error )
{
	if( size < kLabelBytes || label.size() != kLabelBytes )
	{
		if( error )
			*error = QStringLiteral( "fixture size is too small for a vdev label" );
		return false;
	}
	QFile file( path );
	if( ! file.open( QIODevice::WriteOnly ) )
	{
		if( error )
			*error = file.errorString();
		return false;
	}
	if( ! file.resize( size ) )
	{
		if( error )
			*error = file.errorString();
		return false;
	}
	const qint64 tails[] = { 0, kLabelBytes, size - (2 * kLabelBytes), size - kLabelBytes };
	for( unsigned i = 0; i < 4; ++i )
	{
		if( tails[i] < 0 || tails[i] + label.size() > size )
			continue;
		if( ! file.seek( tails[i] ) || file.write( label ) != label.size() )
		{
			if( error )
				*error = file.errorString();
			return false;
		}
	}
	return true;
}

struct Cursor
{
	const QByteArray *bytes;
	int i;
	int end;
};

static bool Read_Be32( Cursor &c, quint32 *out )
{
	if( c.i + 4 > c.end || c.i < 0 )
		return false;
	const uchar *p = reinterpret_cast<const uchar *>( c.bytes->constData() + c.i );
	*out = (quint32( p[0] ) << 24) | (quint32( p[1] ) << 16) | (quint32( p[2] ) << 8) | quint32( p[3] );
	c.i += 4;
	return true;
}

static bool Read_Xdr_String( Cursor &c, QString *out )
{
	quint32 len = 0;
	if( ! Read_Be32( c, &len ) )
		return false;
	if( len > 4096 || c.i + int( len ) > c.end )
		return false;
	*out = QString::fromUtf8( c.bytes->constData() + c.i, int( len ) );
	c.i += int( len );
	const int pad = (4 - (int( len ) % 4)) % 4;
	if( c.i + pad > c.end )
		return false;
	c.i += pad;
	return true;
}

struct Parsed_Level
{
	QString type;
	QString name;
	quint64 pool_guid = 0;
	quint64 guid = 0;
	int ashift = 0;
	bool has_pool_guid = false;
	bool has_guid = false;
	bool has_ashift = false;
	QList<quint64> disk_guids;
};

static bool Parse_Nvlist( Cursor &c, Parsed_Level *level, int depth );

static bool Parse_Pairs( Cursor &c, Parsed_Level *level, int depth )
{
	while( c.i + 8 <= c.end )
	{
		const int start = c.i;
		quint32 enc = 0;
		quint32 dec = 0;
		if( ! Read_Be32( c, &enc ) || ! Read_Be32( c, &dec ) )
			return false;
		if( enc == 0 && dec == 0 )
			return true;
		if( enc < 8 )
			return false;
		const int pair_end = start + int( enc );
		if( pair_end > c.end || pair_end < c.i )
			return false;

		Cursor body = c;
		body.end = pair_end;
		QString key;
		quint32 type = 0;
		quint32 nelem = 0;
		if( Read_Xdr_String( body, &key ) && Read_Be32( body, &type ) && Read_Be32( body, &nelem ) && nelem < 4096 )
		{
			if( type == 8 && nelem == 1 && body.i + 8 <= body.end )
			{
				quint32 hi = 0;
				quint32 lo = 0;
				if( Read_Be32( body, &hi ) && Read_Be32( body, &lo ) )
				{
					const quint64 value = (quint64( hi ) << 32) | quint64( lo );
					if( key == QLatin1String( "pool_guid" ) )
					{
						level->pool_guid = value;
						level->has_pool_guid = true;
					}
					else if( key == QLatin1String( "guid" ) )
					{
						level->guid = value;
						level->has_guid = true;
					}
					else if( key == QLatin1String( "ashift" ) && value < 20 )
					{
						level->ashift = int( value );
						level->has_ashift = true;
					}
				}
			}
			else if( type == 9 && nelem == 1 )
			{
				QString text;
				if( Read_Xdr_String( body, &text ) )
				{
					if( key == QLatin1String( "name" ) && level->name.isEmpty() )
						level->name = text;
					else if( key == QLatin1String( "type" ) )
						level->type = text;
				}
			}
			else if( type == 19 && nelem == 1 && depth < 8 )
			{
				Parsed_Level child;
				if( Parse_Nvlist( body, &child, depth + 1 ) )
					level->disk_guids.append( child.disk_guids );
			}
			else if( type == 20 && depth < 8 )
			{
				for( quint32 n = 0; n < nelem; ++n )
				{
					Parsed_Level child;
					if( ! Parse_Nvlist( body, &child, depth + 1 ) )
						break;
					level->disk_guids.append( child.disk_guids );
				}
			}
		}
		c.i = pair_end;
	}
	return true;
}

static bool Parse_Nvlist( Cursor &c, Parsed_Level *level, int depth )
{
	quint32 version = 0;
	quint32 flags = 0;
	if( ! Read_Be32( c, &version ) || ! Read_Be32( c, &flags ) )
		return false;
	if( version > 1 )
		return false;
	Parsed_Level local;
	if( ! Parse_Pairs( c, &local, depth ) )
		return false;
	if( local.type == QLatin1String( "disk" ) && local.has_guid )
		local.disk_guids.append( local.guid );
	if( depth == 0 )
		*level = local;
	else
	{
		level->disk_guids.append( local.disk_guids );
		if( ! local.has_pool_guid )
		{
			// Keep the child's own identity in disk_guids only.
		}
		if( local.type == QLatin1String( "disk" ) && local.has_guid )
		{
			// already appended
		}
	}
	if( depth == 0 )
	{
		level->disk_guids = local.disk_guids;
		return true;
	}
	*level = local;
	return true;
}

static bool Parse_Label_Nvlist( const QByteArray &label, Parsed_Level *level )
{
	if( label.size() < kNvlistOffset + 32 )
		return false;
	const QByteArray nv = label.mid( kNvlistOffset );
	Cursor c;
	c.bytes = &nv;
	c.i = 0;
	c.end = nv.size();
	return Parse_Nvlist( c, level, 0 ) && level->has_pool_guid && level->has_guid;
}

static QString Type_Label_For_Path( const QString &path )
{
	if( path.startsWith( QLatin1String( "/dev/" ) ) || path.startsWith( QLatin1String( "\\\\.\\" ) ) )
		return QStringLiteral( "physical disk" );
	const QString lower = path.toLower();
	if( lower.endsWith( QLatin1String( ".vmdk" ) ) )
		return QStringLiteral( "VMDK" );
	if( lower.endsWith( QLatin1String( ".qcow2" ) ) )
		return QStringLiteral( "qcow2" );
	if( lower.endsWith( QLatin1String( ".vhdx" ) ) )
		return QStringLiteral( "VHDX" );
	if( lower.endsWith( QLatin1String( ".vdi" ) ) )
		return QStringLiteral( "VDI" );
	if( lower.endsWith( QLatin1String( ".img" ) ) || lower.endsWith( QLatin1String( ".raw" ) ) )
		return QStringLiteral( "raw" );
	return QStringLiteral( "disk image" );
}

static quint64 Read_Le64( const QByteArray &bytes, int offset )
{
	if( offset < 0 || offset + 8 > bytes.size() )
		return 0;
	const uchar *p = reinterpret_cast<const uchar *>( bytes.constData() + offset );
	quint64 value = 0;
	for( int i = 7; i >= 0; --i )
		value = (value << 8) | quint64( p[i] );
	return value;
}

static int Sector_Value_From_Line( const QString &line, const QString &key )
{
	const int at = line.indexOf( key, 0, Qt::CaseInsensitive );
	if( at < 0 )
		return 0;
	const int eq = line.indexOf( QLatin1Char( '=' ), at );
	if( eq < 0 )
		return 0;
	QString value = line.mid( eq + 1 ).trimmed();
	if( value.startsWith( QLatin1Char( '"' ) ) && value.endsWith( QLatin1Char( '"' ) ) && value.size() >= 2 )
		value = value.mid( 1, value.size() - 2 );
	const int sector = value.toInt();
	if( sector == 512 || sector == 4096 )
		return sector;
	return 0;
}

static Vmdk_Sector_Size Sectors_From_Descriptor( const QByteArray &text )
{
	Vmdk_Sector_Size found;
	const QString body = QString::fromLatin1( text );
	const QStringList lines = body.split( QLatin1Char( '\n' ) );
	for( int i = 0; i < lines.size(); ++i )
	{
		const int logical = Sector_Value_From_Line( lines[i], QStringLiteral( "ddb.logicalSectorSize" ) );
		const int physical = Sector_Value_From_Line( lines[i], QStringLiteral( "ddb.physicalSectorSize" ) );
		if( logical )
			found.logical = logical;
		if( physical )
			found.physical = physical;
	}
	return found;
}

Vmdk_Sector_Size Read_Vmdk_Sector_Size( const QString &path )
{
	Vmdk_Sector_Size found;
	QFile file( path );
	if( ! file.open( QIODevice::ReadOnly ) )
		return found;
	const QByteArray head = file.read( 512 );
	if( head.size() >= 4 && head.startsWith( "KDMV" ) )
	{
		const quint64 offset_sectors = Read_Le64( head, 28 );
		const quint64 size_sectors = Read_Le64( head, 36 );
		if( offset_sectors == 0 || size_sectors == 0 || size_sectors > 512 )
			return found;
		if( ! file.seek( qint64( offset_sectors ) * 512 ) )
			return found;
		return Sectors_From_Descriptor( file.read( qint64( size_sectors ) * 512 ) );
	}
	QByteArray text = head;
	if( text.contains( "ddb." ) || text.startsWith( "# Disk DescriptorFile" ) || text.startsWith( "version=" ) )
	{
		text.append( file.read( 64 * 1024 ) );
		return Sectors_From_Descriptor( text );
	}
	return found;
}

int Four_Kn_Sector( int logical_sector, int physical_sector, int ashift )
{
	if( logical_sector >= 4096 || physical_sector >= 4096 || ashift == 12 )
		return 4096;
	return 0;
}

bool Parse_Vmw_Disk_Property( const QString &key, QString *controller, int *controller_index,
	int *unit, QString *field )
{
	const QRegularExpression re( QStringLiteral(
		"^([A-Za-z]+)(\\d+):(\\d+)\\.(serialNumber|logicalSectorSize|physicalSectorSize)$" ) );
	const QRegularExpressionMatch match = re.match( key.trimmed() );
	if( ! match.hasMatch() )
		return false;
	if( controller )
		*controller = match.captured( 1 ).toLower();
	if( controller_index )
		*controller_index = match.captured( 2 ).toInt();
	if( unit )
		*unit = match.captured( 3 ).toInt();
	if( field )
		*field = match.captured( 4 );
	return true;
}

QString Stable_Disk_Serial( const QString &ovf_serial, quint64 zfs_disk_guid, int one_based_index )
{
	const QString trimmed = ovf_serial.trimmed();
	if( ! trimmed.isEmpty() )
		return trimmed;
	if( zfs_disk_guid != 0 )
		return QStringLiteral( "aqemu-%1" ).arg( zfs_disk_guid, 16, 16, QLatin1Char( '0' ) );
	return QStringLiteral( "aqemu-disk-%1" ).arg( qMax( 1, one_based_index ) );
}

QList<Lab_Disk_Spec> Plan_Lab_Disks( const QString &name_key, int count, bool qcow2, bool nvme )
{
	QList<Lab_Disk_Spec> disks;
	const int n = qBound( 1, count, 8 );
	const QByteArray tag = QCryptographicHash::hash( name_key.toUtf8(), QCryptographicHash::Md5 ).toHex().left( 6 );
	const QString ext = qcow2 ? QStringLiteral( "qcow2" ) : QStringLiteral( "raw" );
	for( int i = 0; i < n; ++i )
	{
		Lab_Disk_Spec disk;
		disk.filename = QStringLiteral( "disk-%1.%2" ).arg( i + 1, 2, 10, QLatin1Char( '0' ) ).arg( ext );
		disk.serial = QStringLiteral( "aqemu-%1-%2" )
			.arg( QString::fromLatin1( tag ) )
			.arg( i + 1, 2, 10, QLatin1Char( '0' ) );
		disk.logical_sector = 4096;
		disk.nvme = nvme;
		disks << disk;
	}
	return disks;
}

Storage_Network_Plan Plan_Storage_Network( const QString &name_key )
{
	Storage_Network_Plan plan;
	plan.localaddr = QStringLiteral( "127.0.0.1" );
	const QByteArray tag = QCryptographicHash::hash( name_key.toUtf8(), QCryptographicHash::Md5 ).toHex();
	bool port_ok = false;
	const int port = 45000 + ( tag.left( 4 ).toInt( &port_ok, 16 ) % 10000 );
	plan.mcast = QStringLiteral( "230.0.0.1:%1,localaddr=%2" )
		.arg( port_ok ? port : 45678 )
		.arg( plan.localaddr );

	const struct {
		const char *role;
		const char *suffix;
		int ram_mb;
		int disk_count;
		int disk_gb;
		bool four_kn;
		bool virtio_scsi;
		const char *nic;
	} rows[] = {
		{ "primary", "", 8192, 4, 16, true, true, "virtio-net-pci" },
		{ "secondary", " Secondary", 8192, 2, 16, true, true, "virtio-net-pci" },
		{ "linux", " Linux", 2048, 1, 32, false, true, "virtio-net-pci" },
		{ "windows", " Windows", 4096, 1, 64, false, false, "e1000" }
	};
	for( int i = 0; i < 4; ++i )
	{
		Storage_Node_Spec node;
		node.role = QString::fromLatin1( rows[i].role );
		node.title_suffix = QString::fromLatin1( rows[i].suffix );
		node.ram_mb = rows[i].ram_mb;
		node.disk_count = rows[i].disk_count;
		node.disk_gb = rows[i].disk_gb;
		node.four_kn = rows[i].four_kn;
		node.uefi = true;
		node.virtio_scsi = rows[i].virtio_scsi;
		node.nic_model = QString::fromLatin1( rows[i].nic );
		node.mac = QStringLiteral( "52:54:00:%1:%2:%3" )
			.arg( QString::fromLatin1( tag.mid( 0, 2 ) ) )
			.arg( QString::fromLatin1( tag.mid( 2, 2 ) ) )
			.arg( i + 1, 2, 16, QLatin1Char( '0' ) );
		plan.nodes << node;
	}
	return plan;
}

QString TrueNAS_API_URL( const QString &host, int port, const QString &api_path, QString *error )
{
	if( error )
		error->clear();
	QString h = host.trimmed();
	if( h.isEmpty() )
	{
		if( error )
			*error = QStringLiteral( "Enter the TrueNAS address." );
		return QString();
	}
	if( ! h.startsWith( QLatin1String( "http://" ), Qt::CaseInsensitive ) &&
	    ! h.startsWith( QLatin1String( "https://" ), Qt::CaseInsensitive ) )
		h = QStringLiteral( "https://" ) + h;
	QUrl url( h );
	if( ! url.isValid() || url.host().isEmpty() )
	{
		if( error )
			*error = QStringLiteral( "The TrueNAS address is not a valid URL." );
		return QString();
	}
	if( ! url.userName().isEmpty() || ! url.password().isEmpty() )
	{
		if( error )
			*error = QStringLiteral( "Put the API token in the token field." );
		return QString();
	}
	if( url.port() < 0 && port > 0 )
		url.setPort( port );
	QString path = api_path.startsWith( QLatin1Char( '/' ) ) ? api_path : ( QLatin1Char( '/' ) + api_path );
	url.setPath( path );
	url.setQuery( QString() );
	url.setFragment( QString() );
	return url.toString();
}

QString TrueNAS_Rollback_Path( const QString &snapshot_id )
{
	return QStringLiteral( "/api/v2.0/zfs/snapshot/id/" )
		+ QString::fromUtf8( QUrl::toPercentEncoding( snapshot_id ) )
		+ QStringLiteral( "/rollback" );
}

QString TrueNAS_Pool_Line( const QString &name, const QString &status )
{
	const QString shown = status.trimmed().isEmpty()
		? QStringLiteral( "UNKNOWN" )
		: status.trimmed().toUpper();
	return QStringLiteral( "Pool: %1 [%2]" ).arg( name.trimmed(), shown );
}

static QString Snapshot_Label( const QJsonObject &obj )
{
	const QString id = obj.value( QStringLiteral( "id" ) ).toString();
	if( ! id.isEmpty() )
		return id;
	const QString full = obj.value( QStringLiteral( "snapshot_name" ) ).toString();
	if( ! full.isEmpty() )
		return full;
	const QString dataset = obj.value( QStringLiteral( "dataset" ) ).toString();
	const QString name = obj.value( QStringLiteral( "name" ) ).toString();
	if( ! dataset.isEmpty() && ! name.isEmpty() )
		return dataset + QLatin1Char( '@' ) + name;
	return name;
}

QList<TrueNAS_Pool_View> Parse_TrueNAS_Pools( const QByteArray &json, QString *error )
{
	if( error )
		error->clear();
	QJsonParseError perr;
	const QJsonDocument doc = QJsonDocument::fromJson( json, &perr );
	if( perr.error != QJsonParseError::NoError || ! doc.isArray() )
	{
		if( error )
			*error = QStringLiteral( "TrueNAS did not return a pool list." );
		return QList<TrueNAS_Pool_View>();
	}
	QList<TrueNAS_Pool_View> pools;
	const QJsonArray arr = doc.array();
	for( int i = 0; i < arr.size(); ++i )
	{
		if( ! arr.at( i ).isObject() )
			continue;
		const QJsonObject obj = arr.at( i ).toObject();
		TrueNAS_Pool_View pool;
		pool.name = obj.value( QStringLiteral( "name" ) ).toString();
		if( pool.name.isEmpty() )
			continue;
		pool.status = obj.value( QStringLiteral( "status" ) ).toString();
		pool.line = TrueNAS_Pool_Line( pool.name, pool.status );
		if( obj.contains( QStringLiteral( "size" ) ) )
			pool.size_bytes = static_cast<qint64>( obj.value( QStringLiteral( "size" ) ).toDouble() );
		if( obj.contains( QStringLiteral( "allocated" ) ) )
			pool.allocated_bytes = static_cast<qint64>( obj.value( QStringLiteral( "allocated" ) ).toDouble() );
		pools << pool;
	}
	return pools;
}

QList<TrueNAS_Snapshot_View> Parse_TrueNAS_Snapshots( const QByteArray &json, QString *error )
{
	if( error )
		error->clear();
	QJsonParseError perr;
	const QJsonDocument doc = QJsonDocument::fromJson( json, &perr );
	if( perr.error != QJsonParseError::NoError || ! doc.isArray() )
	{
		if( error )
			*error = QStringLiteral( "TrueNAS did not return a snapshot list." );
		return QList<TrueNAS_Snapshot_View>();
	}
	QList<TrueNAS_Snapshot_View> snaps;
	const QJsonArray arr = doc.array();
	for( int i = 0; i < arr.size(); ++i )
	{
		if( ! arr.at( i ).isObject() )
			continue;
		const QString id = Snapshot_Label( arr.at( i ).toObject() );
		if( id.isEmpty() )
			continue;
		TrueNAS_Snapshot_View snap;
		snap.id = id;
		snap.label = id;
		snaps << snap;
	}
	return snaps;
}

QString Disk_Format_For_Overlay( const QString &path )
{
	if( path.startsWith( QLatin1String( "/dev/" ) ) || path.startsWith( QLatin1String( "\\\\.\\" ) ) )
		return QStringLiteral( "raw" );
	const QString lower = path.toLower();
	if( lower.endsWith( QLatin1String( ".qcow2" ) ) )
		return QStringLiteral( "qcow2" );
	if( lower.endsWith( QLatin1String( ".vmdk" ) ) )
		return QStringLiteral( "vmdk" );
	if( lower.endsWith( QLatin1String( ".vhdx" ) ) )
		return QStringLiteral( "vhdx" );
	if( lower.endsWith( QLatin1String( ".vdi" ) ) )
		return QStringLiteral( "vdi" );
	return QStringLiteral( "raw" );
}

static bool Read_At( QFile &file, qint64 offset, int count, QByteArray *out )
{
	if( offset < 0 || count <= 0 )
		return false;
	if( ! file.seek( offset ) )
		return false;
	*out = file.read( count );
	return out->size() == count;
}

static bool Looks_Like_Mdadm( QFile &file )
{
	QByteArray magic;
	if( ! Read_At( file, 4096, 4, &magic ) )
		return false;
	const uchar *p = reinterpret_cast<const uchar *>( magic.constData() );
	const quint32 le = quint32( p[0] ) | (quint32( p[1] ) << 8) | (quint32( p[2] ) << 16) | (quint32( p[3] ) << 24 );
	return le == kMdadmMagic;
}

Disk_Probe Probe_Disk( const QString &path )
{
	Disk_Probe probe;
	probe.path = path;
	probe.type_label = Type_Label_For_Path( path );
	QFileInfo info( path );
	probe.size = info.size();

	QFile file( path );
	if( ! file.open( QIODevice::ReadOnly ) )
	{
		probe.open_error = file.errorString();
		return probe;
	}
	probe.opened = true;
	probe.size = file.size();

	const qint64 offsets[] = {
		0,
		kLabelBytes,
		probe.size - (2 * qint64( kLabelBytes )),
		probe.size - qint64( kLabelBytes )
	};
	for( unsigned i = 0; i < 4; ++i )
	{
		if( offsets[i] < 0 || offsets[i] + kLabelBytes > probe.size )
			continue;
		QByteArray label;
		if( ! Read_At( file, offsets[i], kLabelBytes, &label ) )
			continue;
		Parsed_Level level;
		if( ! Parse_Label_Nvlist( label, &level ) )
			continue;
		probe.zfs_labels = true;
		probe.pool_name = level.name;
		probe.pool_guid = level.pool_guid;
		probe.disk_guid = level.guid;
		probe.ashift = level.ashift;
		probe.expected_members = level.disk_guids;
		break;
	}
	if( ! probe.zfs_labels )
		probe.linux_raid = Looks_Like_Mdadm( file );
	const Vmdk_Sector_Size vmdk = Read_Vmdk_Sector_Size( path );
	probe.descriptor_logical = vmdk.logical;
	probe.descriptor_physical = vmdk.physical;
	return probe;
}

static QString Guid_Hex( quint64 guid )
{
	return QStringLiteral( "%1" ).arg( guid, 16, 16, QLatin1Char( '0' ) );
}

QString Source_Identity( const Disk_Probe &probe )
{
	if( probe.zfs_labels && probe.disk_guid != 0 )
		return QStringLiteral( "guid:" ) + Guid_Hex( probe.disk_guid );
	return probe.path + QStringLiteral( ":" ) + QString::number( probe.size );
}

QList<Probable_Pool> Group_Probable_Pools( const QList<Disk_Probe> &probes )
{
	QList<Probable_Pool> pools;
	for( int i = 0; i < probes.size(); ++i )
	{
		const Disk_Probe &probe = probes[i];
		if( ! probe.zfs_labels )
			continue;
		int slot = -1;
		for( int p = 0; p < pools.size(); ++p )
		{
			if( pools[p].pool_guid == probe.pool_guid )
			{
				slot = p;
				break;
			}
		}
		if( slot < 0 )
		{
			Probable_Pool pool;
			pool.name = probe.pool_name;
			pool.pool_guid = probe.pool_guid;
			pool.ashift = probe.ashift;
			pool.expected_members = probe.expected_members;
			pools << pool;
			slot = pools.size() - 1;
		}
		else
		{
			if( pools[slot].ashift == 0 )
				pools[slot].ashift = probe.ashift;
			if( pools[slot].expected_members.isEmpty() )
				pools[slot].expected_members = probe.expected_members;
		}
		if( probe.disk_guid != 0 && ! pools[slot].present_members.contains( probe.disk_guid ) )
			pools[slot].present_members << probe.disk_guid;
	}
	for( int p = 0; p < pools.size(); ++p )
	{
		Probable_Pool &pool = pools[p];
		for( int m = 0; m < pool.expected_members.size(); ++m )
		{
			const quint64 guid = pool.expected_members[m];
			if( ! pool.present_members.contains( guid ) && ! pool.missing_members.contains( guid ) )
				pool.missing_members << guid;
		}
		pool.incomplete = ! pool.expected_members.isEmpty() && ! pool.missing_members.isEmpty();
	}
	return pools;
}

int Sector_Size_From_Pools( const QList<Probable_Pool> &pools )
{
	int ashift = 0;
	for( int i = 0; i < pools.size(); ++i )
	{
		if( pools[i].ashift > ashift )
			ashift = pools[i].ashift;
	}
	if( ashift < 9 || ashift > 16 )
		return 512;
	return 1 << ashift;
}

bool Recovery_Wizard_Can_Continue( const QList<Disk_Probe> &probes )
{
	for( int i = 0; i < probes.size(); ++i )
	{
		if( ! probes[i].path.isEmpty() )
			return true;
	}
	return false;
}

QString Build_Recovery_Report( const Report_Facts &facts )
{
	QString text;
	text += QStringLiteral( "AQEMU storage recovery report\n" );
	text += QStringLiteral( "AQEMU version: " ) + facts.aqemu_version + QStringLiteral( "\n" );
	text += QStringLiteral( "QEMU version: " ) + facts.qemu_version + QStringLiteral( "\n\n" );
	text += QStringLiteral( "Sources\n" );
	if( facts.sources.isEmpty() )
		text += QStringLiteral( "(none)\n" );
	for( int i = 0; i < facts.sources.size(); ++i )
	{
		const Disk_Probe &src = facts.sources[i];
		text += QStringLiteral( "%1. type: %2\n" ).arg( i + 1 ).arg( src.type_label );
		text += QStringLiteral( "   path: %1\n" ).arg( src.path );
		text += QStringLiteral( "   size: %1\n" ).arg( src.size );
		text += QStringLiteral( "   read-only: yes\n" );
		if( src.descriptor_logical || src.descriptor_physical )
		{
			text += QStringLiteral( "   descriptor logical sector: %1\n" ).arg( src.descriptor_logical );
			text += QStringLiteral( "   descriptor physical sector: %1\n" ).arg( src.descriptor_physical );
		}
		if( src.zfs_labels )
			text += QStringLiteral( "   detection: detected ZFS labels\n" );
		else if( src.linux_raid )
			text += QStringLiteral( "   detection: Linux mdadm superblock\n" );
		else if( ! src.opened )
			text += QStringLiteral( "   detection: not opened (%1)\n" ).arg( src.open_error );
		else
			text += QStringLiteral( "   detection: Unknown\n" );
	}
	text += QStringLiteral( "\ndetected ZFS labels\n" );
	if( facts.pools.isEmpty() )
	{
		text += QStringLiteral( "No ZFS labels were read from the sources.\n" );
	}
	for( int p = 0; p < facts.pools.size(); ++p )
	{
		const Probable_Pool &pool = facts.pools[p];
		text += QStringLiteral( "pool name: %1\n" ).arg( pool.name );
		text += QStringLiteral( "pool GUID: %1\n" ).arg( Guid_Hex( pool.pool_guid ) );
		text += QStringLiteral( "ashift: %1\n" ).arg( pool.ashift );
		text += QStringLiteral( "sector size: %1\n" ).arg( pool.ashift >= 9 ? (1 << pool.ashift) : 512 );
		if( pool.incomplete )
			text += QStringLiteral( "summary: probable pool — incomplete\n" );
		else
			text += QStringLiteral( "summary: probable pool\n" );
		text += QStringLiteral( "expected pool members: %1\n" ).arg( pool.expected_members.size() );
		text += QStringLiteral( "present: %1\n" ).arg( pool.present_members.size() );
		text += QStringLiteral( "missing: %1\n" ).arg( pool.missing_members.size() );
		for( int m = 0; m < pool.missing_members.size(); ++m )
			text += QStringLiteral( "missing GUID: %1\n" ).arg( Guid_Hex( pool.missing_members[m] ) );
		if( pool.incomplete )
			text += QStringLiteral( "A degraded import is the guest's decision.\n" );
	}
	text += QStringLiteral( "\nRecommended VM\n" );
	text += QStringLiteral( "guest: %1\n" ).arg( facts.guest_os );
	text += QStringLiteral( "machine: %1\n" ).arg( facts.machine );
	text += QStringLiteral( "controller: %1\n" ).arg( facts.controller );
	text += QStringLiteral( "sector size: %1\n" ).arg( facts.sector_size );
	text += QStringLiteral( "serial policy: aqemu- plus ZFS disk GUID\n" );
	text += QStringLiteral( "boot media: %1\n" ).arg( facts.boot_media.isEmpty() ? QStringLiteral( "user supplied (none selected)" ) : facts.boot_media );
	text += QStringLiteral( "\nSafety\n" );
	text += QStringLiteral( "probe: read-only\n" );
	text += QStringLiteral( "host boot disk excluded: %1\n" ).arg( facts.host_boot_disk_excluded ? QStringLiteral( "yes" ) : QStringLiteral( "no" ) );
	text += QStringLiteral( "original sources opened read-write: NO\n" );
	text += QStringLiteral( "\n" );
	text += Checkpoint_Section_Marker();
	text += QStringLiteral( "\n" );
	text += facts.checkpoint_text.isEmpty() ? QStringLiteral( "not created yet\n" ) : facts.checkpoint_text;
	if( ! facts.checkpoint_text.endsWith( QLatin1Char( '\n' ) ) )
		text += QStringLiteral( "\n" );
	return text;
}

QString Format_Guest_Import_Status( bool guest_reported_success, const QString &guest_text )
{
	if( guest_reported_success )
		return QStringLiteral( "Guest-reported status: " ) + guest_text;
	return QStringLiteral( "Guest-reported status: the guest reported that the import failed.\n" ) + guest_text;
}

QString Whole_Disk_Device( const QString &path )
{
	if( path.startsWith( QLatin1String( "\\\\.\\" ) ) )
		return path;
	const QFileInfo info( path );
	QString name = info.fileName();
	const bool numbered_namespace = name.contains( QLatin1String( "mmcblk" ) )
		|| name.contains( QLatin1String( "nvme" ) )
		|| name.startsWith( QLatin1String( "loop" ) );
	if( numbered_namespace )
	{
		int end = name.size();
		int i = end;
		while( i > 0 && name[i - 1].isDigit() )
			--i;
		if( i > 0 && i < end && name[i - 1] == QLatin1Char( 'p' ) )
			name = name.left( i - 1 );
	}
	else
	{
		int i = name.size();
		while( i > 0 && name[i - 1].isDigit() )
			--i;
		if( i > 0 && i < name.size() )
			name = name.left( i );
	}
	const QString dir = info.path();
	if( dir.isEmpty() || dir == QLatin1String( "." ) )
		return name;
	return dir + QStringLiteral( "/" ) + name;
}

static QString Boot_Source_From_Mounts()
{
	QFile mounts( QStringLiteral( "/proc/mounts" ) );
	if( ! mounts.open( QIODevice::ReadOnly ) )
		return QString();
	while( ! mounts.atEnd() )
	{
		const QString line = QString::fromLocal8Bit( mounts.readLine() );
		const QStringList parts = line.split( QLatin1Char( ' ' ) );
		if( parts.size() >= 2 && parts[1] == QLatin1String( "/" ) )
			return parts[0];
	}
	return QString();
}

static QString Boot_Source_From_Cmdline()
{
	QFile cmd( QStringLiteral( "/proc/cmdline" ) );
	if( ! cmd.open( QIODevice::ReadOnly ) )
		return QString();
	const QString text = QString::fromLocal8Bit( cmd.readAll() );
	const QStringList parts = text.split( QLatin1Char( ' ' ) );
	for( int i = 0; i < parts.size(); ++i )
	{
		if( parts[i].startsWith( QLatin1String( "root=" ) ) )
			return parts[i].mid( 5 );
	}
	return QString();
}

QString Host_Boot_Disk_Path()
{
#ifdef Q_OS_WIN
	wchar_t windir[MAX_PATH];
	const UINT n = GetWindowsDirectoryW( windir, MAX_PATH );
	if( n == 0 || n >= MAX_PATH )
		return QString();
	const QString root = QString::fromWCharArray( windir );
	if( root.size() < 2 )
		return QString();
	const QString volume = QStringLiteral( "\\\\.\\" ) + root.left( 2 );
	HANDLE handle = CreateFileW( reinterpret_cast<LPCWSTR>( volume.utf16() ),
		GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
	if( handle == INVALID_HANDLE_VALUE )
		return QString();
	VOLUME_DISK_EXTENTS extents;
	DWORD bytes = 0;
	const BOOL ok = DeviceIoControl( handle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
		NULL, 0, &extents, sizeof( extents ), &bytes, NULL );
	CloseHandle( handle );
	if( ! ok || extents.NumberOfDiskExtents < 1 )
		return QString();
	return QStringLiteral( "\\\\.\\PhysicalDrive%1" ).arg( extents.Extents[0].DiskNumber );
#else
	QString src = Boot_Source_From_Mounts();
	if( src.isEmpty() || src == QLatin1String( "/dev/root" ) || ! src.startsWith( QLatin1String( "/dev/" ) ) )
	{
		const QString root = Boot_Source_From_Cmdline();
		if( root.startsWith( QLatin1String( "/dev/" ) ) )
			src = root;
	}
	if( ! src.startsWith( QLatin1String( "/dev/" ) ) )
		return QString();
	const QString canon = QFileInfo( src ).canonicalFilePath();
	return Whole_Disk_Device( canon.isEmpty() ? src : canon );
#endif
}

bool Path_Is_Host_Boot_Disk( const QString &path )
{
	const QString boot = Host_Boot_Disk_Path();
	if( boot.isEmpty() || path.isEmpty() )
		return false;
	const QString canon = QFileInfo( path ).canonicalFilePath();
	const QString whole = Whole_Disk_Device( canon.isEmpty() ? path : canon );
	return whole == boot || QFileInfo( whole ).canonicalFilePath() == QFileInfo( boot ).canonicalFilePath();
}

QStringList List_Recovery_Physical_Disks( QString *note )
{
	QStringList disks;
	const QString boot = Host_Boot_Disk_Path();
	if( boot.isEmpty() )
	{
		if( note )
			*note = QStringLiteral( "Physical disks were not listed because the host boot disk could not be identified." );
		return disks;
	}
#ifdef Q_OS_WIN
	for( int i = 0; i < 32; ++i )
	{
		const QString path = QStringLiteral( "\\\\.\\PhysicalDrive%1" ).arg( i );
		if( path.compare( boot, Qt::CaseInsensitive ) == 0 )
			continue;
		QFile file( path );
		if( ! file.open( QIODevice::ReadOnly ) )
			continue;
		file.close();
		disks << path;
	}
#else
	QDir block( QStringLiteral( "/sys/block" ) );
	const QStringList names = block.entryList( QDir::Dirs | QDir::NoDotAndDotDot );
	for( int i = 0; i < names.size(); ++i )
	{
		const QString name = names[i];
		if( name.startsWith( QLatin1String( "loop" ) ) || name.startsWith( QLatin1String( "ram" ) )
			|| name.startsWith( QLatin1String( "dm-" ) ) || name.startsWith( QLatin1String( "md" ) )
			|| name.startsWith( QLatin1String( "sr" ) ) || name.startsWith( QLatin1String( "fd" ) )
			|| name.startsWith( QLatin1String( "zram" ) ) )
			continue;
		const QString path = QStringLiteral( "/dev/" ) + name;
		if( Path_Is_Host_Boot_Disk( path ) )
			continue;
		if( QFileInfo( path ).exists() )
			disks << path;
	}
#endif
	if( note )
		note->clear();
	return disks;
}

static bool Overlay_Name_Ok( const QString &name )
{
	if( name.contains( QLatin1Char( '/' ) ) || name.contains( QLatin1Char( '\\' ) ) || name.contains( QLatin1String( ".." ) ) )
		return false;
	return name.startsWith( QLatin1String( "disk-" ) ) && name.endsWith( QLatin1String( ".qcow2" ) );
}

static QString Manifest_Path_For( const QString &attempt_dir )
{
	return QDir( attempt_dir ).filePath( QStringLiteral( "manifest.json" ) );
}

bool Load_Checkpoint_Manifest( const QString &manifest_path, Checkpoint_Manifest *out, QString *error )
{
	QFile file( manifest_path );
	if( ! file.open( QIODevice::ReadOnly ) )
	{
		if( error )
			*error = file.errorString();
		return false;
	}
	QJsonParseError parse_error;
	const QJsonDocument doc = QJsonDocument::fromJson( file.readAll(), &parse_error );
	if( parse_error.error != QJsonParseError::NoError || ! doc.isObject() )
	{
		if( error )
			*error = QStringLiteral( "manifest.json is not a checkpoint manifest" );
		return false;
	}
	const QJsonObject obj = doc.object();
	Checkpoint_Manifest manifest;
	manifest.directory = QFileInfo( manifest_path ).absolutePath();
	manifest.checkpoint_id = obj.value( QStringLiteral( "checkpoint_id" ) ).toString();
	manifest.created = obj.value( QStringLiteral( "created" ) ).toString();
	manifest.state = obj.value( QStringLiteral( "state" ) ).toString();
	const QJsonArray disks = obj.value( QStringLiteral( "disks" ) ).toArray();
	for( int i = 0; i < disks.size(); ++i )
	{
		const QJsonObject disk = disks[i].toObject();
		Recovery_Disk entry;
		entry.source_path = disk.value( QStringLiteral( "source_path" ) ).toString();
		entry.source_identity = disk.value( QStringLiteral( "source_identity" ) ).toString();
		entry.source_size = qint64( disk.value( QStringLiteral( "source_size" ) ).toDouble() );
		entry.source_format = disk.value( QStringLiteral( "source_format" ) ).toString();
		entry.overlay_filename = disk.value( QStringLiteral( "overlay_filename" ) ).toString();
		entry.was_read_only = disk.value( QStringLiteral( "was_read_only" ) ).toBool( true );
		if( ! Overlay_Name_Ok( entry.overlay_filename ) )
		{
			if( error )
				*error = QStringLiteral( "manifest names a file outside this checkpoint" );
			return false;
		}
		manifest.disks << entry;
	}
	if( manifest.disks.isEmpty() || manifest.checkpoint_id.isEmpty() )
	{
		if( error )
			*error = QStringLiteral( "manifest has no checkpoint disks" );
		return false;
	}
	*out = manifest;
	return true;
}

static bool Write_Manifest( const Checkpoint_Manifest &manifest, QString *error )
{
	QJsonArray disks;
	for( int i = 0; i < manifest.disks.size(); ++i )
	{
		const Recovery_Disk &disk = manifest.disks[i];
		QJsonObject obj;
		obj.insert( QStringLiteral( "source_path" ), disk.source_path );
		obj.insert( QStringLiteral( "source_identity" ), disk.source_identity );
		obj.insert( QStringLiteral( "source_size" ), double( disk.source_size ) );
		obj.insert( QStringLiteral( "source_format" ), disk.source_format );
		obj.insert( QStringLiteral( "overlay_filename" ), disk.overlay_filename );
		obj.insert( QStringLiteral( "was_read_only" ), disk.was_read_only );
		disks.append( obj );
	}
	QJsonObject root;
	root.insert( QStringLiteral( "checkpoint_id" ), manifest.checkpoint_id );
	root.insert( QStringLiteral( "created" ), manifest.created );
	root.insert( QStringLiteral( "state" ), manifest.state );
	root.insert( QStringLiteral( "disks" ), disks );
	const QString path = Manifest_Path_For( manifest.directory );
	QSaveFile file( path );
	if( ! file.open( QIODevice::WriteOnly ) )
	{
		if( error )
			*error = file.errorString();
		return false;
	}
	file.write( QJsonDocument( root ).toJson( QJsonDocument::Indented ) );
	if( ! file.commit() )
	{
		if( error )
			*error = file.errorString();
		return false;
	}
	return true;
}

static void Remove_Created( const QStringList &paths )
{
	for( int i = 0; i < paths.size(); ++i )
		QFile::remove( paths[i] );
}

Checkpoint_Outcome Create_Checkpoint( const QString &attempt_dir,
	const QList<Recovery_Disk> &sources, const Overlay_Writer &write_overlay )
{
	Checkpoint_Outcome outcome;
	outcome.user_message = Checkpoint_Failure_Text();
	if( sources.isEmpty() || ! write_overlay )
	{
		outcome.detail = QStringLiteral( "No source disks were available for a checkpoint." );
		return outcome;
	}
	const QString manifest_path = Manifest_Path_For( attempt_dir );
	if( QFileInfo::exists( manifest_path ) )
	{
		outcome.detail = QStringLiteral( "A recovery checkpoint already exists." );
		return outcome;
	}
	QDir().mkpath( attempt_dir );

	QStringList created;
	QList<Recovery_Disk> recorded;
	for( int i = 0; i < sources.size(); ++i )
	{
		Recovery_Disk disk = sources[i];
		disk.overlay_filename = QStringLiteral( "disk-%1.qcow2" ).arg( i + 1, 2, 10, QLatin1Char( '0' ) );
		const QString overlay = QDir( attempt_dir ).filePath( disk.overlay_filename );
		if( QFileInfo::exists( overlay ) )
			QFile::remove( overlay );
		QString write_error;
		const bool wrote = write_overlay( disk.source_path, disk.source_format, overlay, &write_error );
		if( ! wrote || ! QFileInfo::exists( overlay ) )
		{
			if( QFileInfo::exists( overlay ) )
				QFile::remove( overlay );
			Remove_Created( created );
			QFile::remove( manifest_path );
			outcome.detail = write_error;
			return outcome;
		}
		created << overlay;
		recorded << disk;
	}

	Checkpoint_Manifest manifest;
	manifest.directory = QDir( attempt_dir ).absolutePath();
	manifest.checkpoint_id = QStringLiteral( "attempt-1" );
	manifest.created = QDateTime::currentDateTimeUtc().toString( Qt::ISODate );
	manifest.state = QStringLiteral( "enabled" );
	manifest.disks = recorded;
	QString write_error;
	if( ! Write_Manifest( manifest, &write_error ) )
	{
		Remove_Created( created );
		QFile::remove( manifest_path );
		outcome.detail = write_error;
		return outcome;
	}
	outcome.ok = true;
	outcome.user_message.clear();
	outcome.manifest = manifest;
	return outcome;
}

bool Discard_Checkpoint( const QString &manifest_path, QString *error )
{
	Checkpoint_Manifest manifest;
	if( ! Load_Checkpoint_Manifest( manifest_path, &manifest, error ) )
		return false;
	for( int i = 0; i < manifest.disks.size(); ++i )
	{
		const QString overlay = QDir( manifest.directory ).filePath( manifest.disks[i].overlay_filename );
		QFile::remove( overlay );
	}
	if( ! QFile::remove( manifest_path ) )
	{
		if( error )
			*error = QStringLiteral( "The overlays were removed but manifest.json is still present." );
		return false;
	}
	return true;
}

bool Keep_Checkpoint( const QString &manifest_path, QString *error )
{
	Checkpoint_Manifest manifest;
	if( ! Load_Checkpoint_Manifest( manifest_path, &manifest, error ) )
		return false;
	manifest.state = QStringLiteral( "kept" );
	return Write_Manifest( manifest, error );
}

bool Create_Qcow2_Backing_Overlay( const QString &qemu_img, const QString &source_path,
	const QString &format, const QString &overlay_path, QString *error )
{
	if( qemu_img.isEmpty() )
	{
		if( error )
			*error = QStringLiteral( "qemu-img was not found." );
		return false;
	}
	QProcess process;
	process.start( qemu_img, QStringList()
		<< QStringLiteral( "create" )
		<< QStringLiteral( "-f" ) << QStringLiteral( "qcow2" )
		<< QStringLiteral( "-b" ) << source_path
		<< QStringLiteral( "-F" ) << format
		<< overlay_path );
	if( ! process.waitForStarted( 5000 ) || ! process.waitForFinished( 120000 ) )
	{
		if( error )
			*error = QStringLiteral( "qemu-img did not finish." );
		return false;
	}
	if( process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0 || ! QFileInfo::exists( overlay_path ) )
	{
		if( error )
			*error = QString::fromLocal8Bit( process.readAllStandardError() );
		return false;
	}
	return true;
}

QList<Drive_Binding> Apply_Checkpoint_Result( const QList<Drive_Binding> &original,
	const Checkpoint_Outcome &outcome )
{
	if( ! outcome.ok )
		return original;
	QList<Drive_Binding> bindings;
	for( int i = 0; i < outcome.manifest.disks.size(); ++i )
	{
		Drive_Binding binding;
		binding.path = QDir( outcome.manifest.directory ).filePath( outcome.manifest.disks[i].overlay_filename );
		binding.read_only = false;
		bindings << binding;
	}
	return bindings;
}

QList<Drive_Binding> Bindings_After_Discard( const Checkpoint_Manifest &manifest )
{
	QList<Drive_Binding> bindings;
	for( int i = 0; i < manifest.disks.size(); ++i )
	{
		Drive_Binding binding;
		binding.path = manifest.disks[i].source_path;
		binding.read_only = manifest.disks[i].was_read_only;
		bindings << binding;
	}
	return bindings;
}

QList<Drive_Binding> Bindings_After_Keep( const Checkpoint_Manifest &manifest )
{
	QList<Drive_Binding> bindings;
	for( int i = 0; i < manifest.disks.size(); ++i )
	{
		Drive_Binding binding;
		binding.path = QDir( manifest.directory ).filePath( manifest.disks[i].overlay_filename );
		binding.read_only = false;
		bindings << binding;
	}
	return bindings;
}

bool Update_Report_Checkpoint( const QString &report_path, const QString &checkpoint_text, QString *error )
{
	QFile file( report_path );
	if( ! file.open( QIODevice::ReadOnly ) )
	{
		if( error )
			*error = file.errorString();
		return false;
	}
	QString text = QString::fromUtf8( file.readAll() );
	file.close();
	const QString marker = Checkpoint_Section_Marker();
	const int at = text.indexOf( marker );
	if( at < 0 )
	{
		if( error )
			*error = QStringLiteral( "The recovery report has no checkpoint section." );
		return false;
	}
	text = text.left( at + marker.size() ) + QStringLiteral( "\n" ) + checkpoint_text;
	if( ! text.endsWith( QLatin1Char( '\n' ) ) )
		text += QStringLiteral( "\n" );
	QSaveFile out( report_path );
	if( ! out.open( QIODevice::WriteOnly ) )
	{
		if( error )
			*error = out.errorString();
		return false;
	}
	out.write( text.toUtf8() );
	if( ! out.commit() )
	{
		if( error )
			*error = out.errorString();
		return false;
	}
	return true;
}
