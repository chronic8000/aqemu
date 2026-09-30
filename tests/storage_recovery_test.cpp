#include "Storage_Recovery.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <cstring>
#include <cstdio>

static int g_failures = 0;

#define CHECK( cond ) \
	do { \
		if( !( cond ) ) { \
			std::fprintf( stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #cond ); \
			++g_failures; \
		} \
	} while( 0 )

static QByteArray Read_All( const QString &path )
{
	QFile file( path );
	if( ! file.open( QIODevice::ReadOnly ) )
		return QByteArray();
	return file.readAll();
}

static bool Write_Bytes( const QString &path, const QByteArray &bytes )
{
	QFile file( path );
	if( ! file.open( QIODevice::WriteOnly ) )
		return false;
	return file.write( bytes ) == bytes.size();
}

static Overlay_Writer Dummy_Writer( int *calls, int fail_index )
{
	return [calls, fail_index]( const QString &, const QString &, const QString &overlay, QString *error ) -> bool {
		const int n = calls ? (*calls)++ : 0;
		QFile file( overlay );
		if( ! file.open( QIODevice::WriteOnly ) )
		{
			if( error )
				*error = file.errorString();
			return false;
		}
		file.write( fail_index == n ? "partial" : "overlay" );
		file.close();
		if( fail_index == n )
		{
			if( error )
				*error = QStringLiteral( "injected overlay failure" );
			return false;
		}
		return true;
	};
}

static void test_probe_does_not_change_source()
{
	QTemporaryDir dir;
	CHECK( dir.isValid() );
	const QString path = dir.filePath( QStringLiteral( "disk0.raw" ) );
	QString error;
	const QByteArray label = Build_Zfs_Label( QStringLiteral( "tank" ), 0x111, 0xA, 12, QList<quint64>() << 0xA );
	CHECK( Write_Zfs_Label_Fixture( path, 1024 * 1024, label, &error ) );
	const QByteArray before = Read_All( path );
	const QFileInfo info( path );
	const qint64 size = info.size();
	const QDateTime mtime = info.lastModified();
	const Disk_Probe probe = Probe_Disk( path );
	const QFileInfo after( path );
	CHECK( probe.zfs_labels );
	CHECK( probe.pool_name == QLatin1String( "tank" ) );
	CHECK( probe.ashift == 12 );
	CHECK( after.size() == size );
	CHECK( after.lastModified() == mtime );
	CHECK( Read_All( path ) == before );
}

static void test_enable_points_at_overlays()
{
	QTemporaryDir dir;
	CHECK( dir.isValid() );
	QList<Recovery_Disk> sources;
	QList<Drive_Binding> original;
	for( int i = 0; i < 2; ++i )
	{
		const QString path = dir.filePath( QStringLiteral( "src%1.raw" ).arg( i ) );
		CHECK( Write_Bytes( path, "SOURCE" ) );
		Recovery_Disk disk;
		disk.source_path = path;
		disk.source_identity = path + QStringLiteral( ":6" );
		disk.source_size = 6;
		disk.source_format = QStringLiteral( "raw" );
		disk.was_read_only = true;
		sources << disk;
		Drive_Binding binding;
		binding.path = path;
		binding.read_only = true;
		original << binding;
	}
	int calls = 0;
	const Checkpoint_Outcome outcome = Create_Checkpoint( dir.filePath( QStringLiteral( "attempt-1" ) ), sources, Dummy_Writer( &calls, -1 ) );
	CHECK( outcome.ok );
	CHECK( QFileInfo::exists( dir.filePath( QStringLiteral( "attempt-1/manifest.json" ) ) ) );
	const QList<Drive_Binding> wired = Apply_Checkpoint_Result( original, outcome );
	CHECK( wired.size() == 2 );
	CHECK( wired[0].path.endsWith( QStringLiteral( "disk-01.qcow2" ) ) );
	CHECK( wired[1].path.endsWith( QStringLiteral( "disk-02.qcow2" ) ) );
	CHECK( ! wired[0].read_only );
	CHECK( Read_All( sources[0].source_path ) == "SOURCE" );
	CHECK( Read_All( sources[1].source_path ) == "SOURCE" );
}

static void test_failed_overlay_removes_partial_checkpoint()
{
	QTemporaryDir dir;
	CHECK( dir.isValid() );
	const QString attempt = dir.filePath( QStringLiteral( "attempt-1" ) );
	CHECK( Write_Bytes( dir.filePath( QStringLiteral( "attempt-1-foreign.qcow2" ) ), "foreign" ) );
	QDir().mkpath( attempt );
	const QString foreign = QDir( attempt ).filePath( QStringLiteral( "notes.qcow2" ) );
	CHECK( Write_Bytes( foreign, "keep-me" ) );

	QList<Recovery_Disk> sources;
	QList<Drive_Binding> original;
	for( int i = 0; i < 3; ++i )
	{
		const QString path = dir.filePath( QStringLiteral( "src%1.raw" ).arg( i ) );
		CHECK( Write_Bytes( path, "SRC" ) );
		Recovery_Disk disk;
		disk.source_path = path;
		disk.source_format = QStringLiteral( "raw" );
		disk.source_size = 3;
		disk.was_read_only = true;
		sources << disk;
		Drive_Binding binding;
		binding.path = path;
		binding.read_only = true;
		original << binding;
	}
	int calls = 0;
	const Checkpoint_Outcome outcome = Create_Checkpoint( attempt, sources, Dummy_Writer( &calls, 2 ) );
	CHECK( ! outcome.ok );
	CHECK( outcome.user_message == Checkpoint_Failure_Text() );
	CHECK( ! QFileInfo::exists( QDir( attempt ).filePath( QStringLiteral( "manifest.json" ) ) ) );
	CHECK( ! QFileInfo::exists( QDir( attempt ).filePath( QStringLiteral( "disk-01.qcow2" ) ) ) );
	CHECK( ! QFileInfo::exists( QDir( attempt ).filePath( QStringLiteral( "disk-02.qcow2" ) ) ) );
	CHECK( ! QFileInfo::exists( QDir( attempt ).filePath( QStringLiteral( "disk-03.qcow2" ) ) ) );
	CHECK( Read_All( foreign ) == "keep-me" );
	const QList<Drive_Binding> wired = Apply_Checkpoint_Result( original, outcome );
	CHECK( wired.size() == 3 );
	CHECK( wired[0].path == original[0].path );
	CHECK( wired[1].path == original[1].path );
	CHECK( wired[2].path == original[2].path );
	CHECK( wired[2].read_only );
	CHECK( Read_All( sources[2].source_path ) == "SRC" );
}

static void test_discard_deletes_only_manifest_files()
{
	QTemporaryDir dir;
	CHECK( dir.isValid() );
	const QString attempt = dir.filePath( QStringLiteral( "attempt-1" ) );
	QList<Recovery_Disk> sources;
	const QString source = dir.filePath( QStringLiteral( "src.raw" ) );
	CHECK( Write_Bytes( source, "BYTES" ) );
	Recovery_Disk disk;
	disk.source_path = source;
	disk.source_format = QStringLiteral( "raw" );
	disk.source_size = 5;
	disk.was_read_only = true;
	sources << disk;
	int calls = 0;
	const Checkpoint_Outcome outcome = Create_Checkpoint( attempt, sources, Dummy_Writer( &calls, -1 ) );
	CHECK( outcome.ok );
	const QString foreign = QDir( attempt ).filePath( QStringLiteral( "other.qcow2" ) );
	CHECK( Write_Bytes( foreign, "foreign" ) );
	const QString manifest = QDir( attempt ).filePath( QStringLiteral( "manifest.json" ) );
	QString error;
	CHECK( Discard_Checkpoint( manifest, &error ) );
	CHECK( ! QFileInfo::exists( QDir( attempt ).filePath( QStringLiteral( "disk-01.qcow2" ) ) ) );
	CHECK( ! QFileInfo::exists( manifest ) );
	CHECK( Read_All( foreign ) == "foreign" );
	CHECK( Read_All( source ) == "BYTES" );
	const QList<Drive_Binding> restored = Bindings_After_Discard( outcome.manifest );
	CHECK( restored.size() == 1 );
	CHECK( restored[0].path == source );
	CHECK( restored[0].read_only );
}

static void test_keep_leaves_sources_and_subtitle()
{
	QTemporaryDir dir;
	CHECK( dir.isValid() );
	const QString attempt = dir.filePath( QStringLiteral( "attempt-1" ) );
	const QString source = dir.filePath( QStringLiteral( "src.raw" ) );
	CHECK( Write_Bytes( source, "ORIGINAL" ) );
	Recovery_Disk disk;
	disk.source_path = source;
	disk.source_format = QStringLiteral( "raw" );
	disk.source_size = 8;
	disk.was_read_only = true;
	QList<Recovery_Disk> sources;
	sources << disk;
	int calls = 0;
	const Checkpoint_Outcome outcome = Create_Checkpoint( attempt, sources, Dummy_Writer( &calls, -1 ) );
	CHECK( outcome.ok );
	const QString manifest = QDir( attempt ).filePath( QStringLiteral( "manifest.json" ) );
	QString error;
	CHECK( Keep_Checkpoint( manifest, &error ) );
	CHECK( Read_All( source ) == "ORIGINAL" );
	Checkpoint_Manifest loaded;
	CHECK( Load_Checkpoint_Manifest( manifest, &loaded, &error ) );
	CHECK( loaded.state == QLatin1String( "kept" ) );
	const QList<Drive_Binding> kept = Bindings_After_Keep( loaded );
	CHECK( kept.size() == 1 );
	CHECK( kept[0].path.endsWith( QStringLiteral( "disk-01.qcow2" ) ) );
	CHECK( QFileInfo::exists( kept[0].path ) );
	CHECK( Read_All( source ) == "ORIGINAL" );
	CHECK( Keep_Button_Subtitle() == QStringLiteral( "Keeps the recovery overlays as this VM’s disks. Original source disks remain unchanged." ) );
	CHECK( Discard_Confirm_Text().contains( QStringLiteral( "Original source disks are unaffected." ) ) );
}

static void test_guest_status_and_incomplete_pool()
{
	const QString ok = Format_Guest_Import_Status( true, QStringLiteral( "import finished in the guest" ) );
	CHECK( ok.startsWith( QStringLiteral( "Guest-reported status:" ) ) );
	CHECK( ok.contains( QStringLiteral( "import finished in the guest" ) ) );

	const QString failed = Format_Guest_Import_Status( false, QStringLiteral( "cannot import" ) );
	CHECK( failed.contains( QStringLiteral( "import failed" ) ) );
	CHECK( ! failed.contains( QStringLiteral( "recovered" ), Qt::CaseInsensitive ) );
	CHECK( ! failed.contains( QStringLiteral( "pool imported" ), Qt::CaseInsensitive ) );
	CHECK( ! failed.contains( QStringLiteral( "this pool is recoverable" ), Qt::CaseInsensitive ) );

	QTemporaryDir dir;
	CHECK( dir.isValid() );
	QList<quint64> members;
	members << 1 << 2 << 3 << 4 << 5 << 6;
	QList<Disk_Probe> probes;
	for( int i = 0; i < 4; ++i )
	{
		const QString path = dir.filePath( QStringLiteral( "m%1.raw" ).arg( i ) );
		QString error;
		const QByteArray label = Build_Zfs_Label( QStringLiteral( "tank" ), 0x55, quint64( i + 1 ), 12, members );
		CHECK( Write_Zfs_Label_Fixture( path, 1024 * 1024, label, &error ) );
		probes << Probe_Disk( path );
		CHECK( probes.last().zfs_labels );
	}
	const QList<Probable_Pool> pools = Group_Probable_Pools( probes );
	CHECK( pools.size() == 1 );
	CHECK( pools[0].incomplete );
	CHECK( pools[0].missing_members.size() == 2 );
	Report_Facts facts;
	facts.aqemu_version = QStringLiteral( "1.4.1" );
	facts.qemu_version = QStringLiteral( "test" );
	facts.sources = probes;
	facts.pools = pools;
	facts.guest_os = QStringLiteral( "TrueNAS SCALE" );
	facts.machine = QStringLiteral( "q35" );
	facts.controller = QStringLiteral( "VirtIO-SCSI" );
	facts.sector_size = Sector_Size_From_Pools( pools );
	facts.checkpoint_text = QStringLiteral( "not created yet\n" );
	const QString report = Build_Recovery_Report( facts );
	CHECK( report.contains( QStringLiteral( "detected ZFS labels" ) ) );
	CHECK( report.contains( QStringLiteral( "probable pool — incomplete" ) ) );
	CHECK( report.contains( QStringLiteral( "missing GUID: " ) ) );
	CHECK( report.contains( QStringLiteral( "not created yet" ) ) );
	CHECK( report.contains( QStringLiteral( "original sources opened read-write: NO" ) ) );
	CHECK( facts.sector_size == 4096 );
	CHECK( Recovery_Wizard_Can_Continue( probes ) );
}

static void test_unknown_and_linux_raid()
{
	QTemporaryDir dir;
	CHECK( dir.isValid() );
	const QString raw = dir.filePath( QStringLiteral( "plain.raw" ) );
	QFile file( raw );
	CHECK( file.open( QIODevice::WriteOnly ) );
	file.resize( 64 * 1024 );
	file.close();
	const Disk_Probe plain = Probe_Disk( raw );
	CHECK( plain.opened );
	CHECK( ! plain.zfs_labels );
	CHECK( ! plain.linux_raid );

	const QString raid = dir.filePath( QStringLiteral( "raid.raw" ) );
	QFile raid_file( raid );
	CHECK( raid_file.open( QIODevice::WriteOnly ) );
	raid_file.resize( 64 * 1024 );
	CHECK( raid_file.seek( 4096 ) );
	const char magic[4] = { char( 0xfc ), char( 0x4e ), char( 0x2b ), char( 0xa9 ) };
	CHECK( raid_file.write( magic, 4 ) == 4 );
	raid_file.close();
	const Disk_Probe md = Probe_Disk( raid );
	CHECK( md.linux_raid );
	CHECK( ! md.zfs_labels );
	CHECK( Whole_Disk_Device( QStringLiteral( "/dev/mmcblk0p2" ) ) == QStringLiteral( "/dev/mmcblk0" ) );
	CHECK( Whole_Disk_Device( QStringLiteral( "/dev/nvme0n1p1" ) ) == QStringLiteral( "/dev/nvme0n1" ) );
	CHECK( Whole_Disk_Device( QStringLiteral( "/dev/sda2" ) ) == QStringLiteral( "/dev/sda" ) );
}

static void test_truenas_lab_plan()
{
	const QList<Lab_Disk_Spec> disks = Plan_Lab_Disks( QStringLiteral( "TrueNAS lab" ), 4, true, false );
	CHECK( disks.size() == 4 );
	QStringList serials;
	QStringList names;
	for( int i = 0; i < disks.size(); ++i )
	{
		CHECK( disks[i].logical_sector == 4096 );
		CHECK( ! disks[i].nvme );
		CHECK( disks[i].filename.endsWith( QStringLiteral( ".qcow2" ) ) );
		CHECK( disks[i].serial.startsWith( QStringLiteral( "aqemu-" ) ) );
		CHECK( disks[i].serial.size() <= 20 );
		CHECK( ! serials.contains( disks[i].serial ) );
		CHECK( ! names.contains( disks[i].filename ) );
		serials << disks[i].serial;
		names << disks[i].filename;
	}
	CHECK( names[0] == QLatin1String( "disk-01.qcow2" ) );
	CHECK( names[3] == QLatin1String( "disk-04.qcow2" ) );
	const QList<Lab_Disk_Spec> raw = Plan_Lab_Disks( QStringLiteral( "pool-folder" ), 2, false, true );
	CHECK( raw.size() == 2 );
	CHECK( raw[0].nvme );
	CHECK( raw[0].filename.endsWith( QStringLiteral( ".raw" ) ) );
	CHECK( raw[0].logical_sector == 4096 );
	CHECK( raw[0].serial != raw[1].serial );
}

static void test_storage_network_and_guest_api()
{
	const Storage_Network_Plan plan = Plan_Storage_Network( QStringLiteral( "lab-a" ) );
	CHECK( plan.nodes.size() == 4 );
	CHECK( plan.localaddr == QLatin1String( "127.0.0.1" ) );
	CHECK( plan.mcast.contains( QStringLiteral( "230.0.0.1:" ) ) );
	CHECK( plan.mcast.contains( QStringLiteral( "localaddr=127.0.0.1" ) ) );
	CHECK( plan.nodes[0].role == QLatin1String( "primary" ) );
	CHECK( plan.nodes[0].disk_count == 4 );
	CHECK( plan.nodes[0].four_kn );
	CHECK( plan.nodes[0].virtio_scsi );
	CHECK( plan.nodes[1].disk_count == 2 );
	CHECK( plan.nodes[2].nic_model == QLatin1String( "virtio-net-pci" ) );
	CHECK( plan.nodes[2].disk_gb == 32 );
	CHECK( plan.nodes[3].nic_model == QLatin1String( "e1000" ) );
	CHECK( ! plan.nodes[3].virtio_scsi );
	CHECK( plan.nodes[3].disk_gb == 64 );
	QStringList macs;
	for( int i = 0; i < plan.nodes.size(); ++i )
	{
		CHECK( plan.nodes[i].mac.startsWith( QStringLiteral( "52:54:00:" ) ) );
		CHECK( ! macs.contains( plan.nodes[i].mac ) );
		macs << plan.nodes[i].mac;
		CHECK( plan.nodes[i].uefi );
	}
	const Storage_Network_Plan other = Plan_Storage_Network( QStringLiteral( "lab-b" ) );
	CHECK( other.mcast != plan.mcast || other.nodes[0].mac != plan.nodes[0].mac );

	QString url_error;
	const QString pool_url = TrueNAS_API_URL( QStringLiteral( "10.0.0.8" ), 443, QStringLiteral( "/api/v2.0/pool" ), &url_error );
	CHECK( url_error.isEmpty() );
	CHECK( pool_url == QLatin1String( "https://10.0.0.8:443/api/v2.0/pool" ) );
	QString token_error;
	CHECK( TrueNAS_API_URL( QStringLiteral( "https://user:secret@10.0.0.8" ), 443, QStringLiteral( "/api/v2.0/pool" ), &token_error ).isEmpty() );
	CHECK( token_error.contains( QStringLiteral( "token" ) ) );
	const QString rollback = TrueNAS_Rollback_Path( QStringLiteral( "tank@auto snap" ) );
	CHECK( rollback.startsWith( QStringLiteral( "/api/v2.0/zfs/snapshot/id/" ) ) );
	CHECK( rollback.endsWith( QStringLiteral( "/rollback" ) ) );
	CHECK( rollback.contains( QStringLiteral( "%40" ) ) );
	CHECK( ! rollback.contains( QStringLiteral( "@" ) ) );

	const QByteArray pools_json = QByteArrayLiteral(
		"[{\"name\":\"tank\",\"status\":\"ONLINE\",\"size\":10737418240,\"allocated\":1073741824},"
		"{\"name\":\"backup\",\"status\":\"DEGRADED\"}]" );
	QString parse_error;
	const QList<TrueNAS_Pool_View> pools = Parse_TrueNAS_Pools( pools_json, &parse_error );
	CHECK( parse_error.isEmpty() );
	CHECK( pools.size() == 2 );
	CHECK( pools[0].line == QLatin1String( "Pool: tank [ONLINE]" ) );
	CHECK( pools[1].line == QLatin1String( "Pool: backup [DEGRADED]" ) );
	CHECK( Parse_TrueNAS_Pools( QByteArrayLiteral( "{\"name\":\"tank\"}" ), &parse_error ).isEmpty() );
	CHECK( ! parse_error.isEmpty() );

	const QByteArray snaps_json = QByteArrayLiteral( "[{\"id\":\"tank@hourly\"},{\"dataset\":\"backup\",\"name\":\"manual\"}]" );
	const QList<TrueNAS_Snapshot_View> snaps = Parse_TrueNAS_Snapshots( snaps_json, &parse_error );
	CHECK( snaps.size() == 2 );
	CHECK( snaps[0].id == QLatin1String( "tank@hourly" ) );
	CHECK( snaps[1].id == QLatin1String( "backup@manual" ) );
}

static void test_vmdk_geometry_and_serials()
{
	QTemporaryDir dir;
	CHECK( dir.isValid() );
	const QString text_path = dir.filePath( QStringLiteral( "desc.vmdk" ) );
	CHECK( Write_Bytes( text_path, "# Disk DescriptorFile\nversion=1\nddb.logicalSectorSize = \"4096\"\nddb.physicalSectorSize = \"4096\"\n" ) );
	const Vmdk_Sector_Size text_sectors = Read_Vmdk_Sector_Size( text_path );
	CHECK( text_sectors.logical == 4096 );
	CHECK( text_sectors.physical == 4096 );
	const QFileInfo before( text_path );
	const qint64 size = before.size();
	const QDateTime mtime = before.lastModified();
	const Disk_Probe probed = Probe_Disk( text_path );
	const QFileInfo after( text_path );
	CHECK( probed.descriptor_logical == 4096 );
	CHECK( after.size() == size );
	CHECK( after.lastModified() == mtime );

	QByteArray sparse( 1024, '\0' );
	sparse[0] = 'K';
	sparse[1] = 'D';
	sparse[2] = 'M';
	sparse[3] = 'V';
	sparse[28] = 1;
	sparse[36] = 1;
	const QByteArray desc = "ddb.logicalSectorSize = \"4096\"\n";
	memcpy( sparse.data() + 512, desc.constData(), size_t( desc.size() ) );
	const QString sparse_path = dir.filePath( QStringLiteral( "sparse.vmdk" ) );
	CHECK( Write_Bytes( sparse_path, sparse ) );
	CHECK( Read_Vmdk_Sector_Size( sparse_path ).logical == 4096 );
	CHECK( Four_Kn_Sector( 4096, 0, 0 ) == 4096 );
	CHECK( Four_Kn_Sector( 512, 512, 9 ) == 0 );
	CHECK( Four_Kn_Sector( 0, 0, 12 ) == 4096 );

	QString controller;
	int index = -1;
	int unit = -1;
	QString field;
	CHECK( Parse_Vmw_Disk_Property( QStringLiteral( "scsi0:1.serialNumber" ), &controller, &index, &unit, &field ) );
	CHECK( controller == QLatin1String( "scsi" ) );
	CHECK( index == 0 );
	CHECK( unit == 1 );
	CHECK( field == QLatin1String( "serialNumber" ) );
	CHECK( Parse_Vmw_Disk_Property( QStringLiteral( "sata1:0.logicalSectorSize" ), &controller, &index, &unit, &field ) );
	CHECK( controller == QLatin1String( "sata" ) );
	CHECK( index == 1 );
	CHECK( field == QLatin1String( "logicalSectorSize" ) );
	CHECK( ! Parse_Vmw_Disk_Property( QStringLiteral( "not-a-disk-key" ), &controller, &index, &unit, &field ) );
	CHECK( Stable_Disk_Serial( QStringLiteral( "FROM-OVF" ), 0x11, 2 ) == QLatin1String( "FROM-OVF" ) );
	CHECK( Stable_Disk_Serial( QString(), 0xAB, 2 ) == QStringLiteral( "aqemu-00000000000000ab" ) );
	CHECK( Stable_Disk_Serial( QString(), 0, 3 ) == QLatin1String( "aqemu-disk-3" ) );
}

int main( int argc, char **argv )
{
	QCoreApplication app( argc, argv );
	Q_UNUSED( app );
	test_probe_does_not_change_source();
	test_enable_points_at_overlays();
	test_failed_overlay_removes_partial_checkpoint();
	test_discard_deletes_only_manifest_files();
	test_keep_leaves_sources_and_subtitle();
	test_guest_status_and_incomplete_pool();
	test_unknown_and_linux_raid();
	test_vmdk_geometry_and_serials();
	test_truenas_lab_plan();
	test_storage_network_and_guest_api();
	if( g_failures )
	{
		std::fprintf( stderr, "%d checks failed\n", g_failures );
		return 1;
	}
	std::fprintf( stdout, "storage recovery acceptance checks passed\n" );
	return 0;
}
