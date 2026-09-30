/****************************************************************************
**
** OVF (Open Virtualization Format) and OVA (Archive) Parser & Generator
**
****************************************************************************/

#ifndef OVF_PARSER_H
#define OVF_PARSER_H

#include <QString>
#include <QStringList>
#include <QList>
#include <functional>

class Virtual_Machine;

struct OVF_Disk
{
	QString disk_id;
	QString file_ref;
	qint64 capacity_bytes = 0;
	QString format; // e.g. vmdk, qcow2, raw
	QString href;   // filename inside archive (e.g. disk1.vmdk)
	QString local_extracted_path;
	QString controller_type; // ide, sata, scsi, virtio
	int bus = 0;
	int unit = 0;
	QString serial;
	int logical_sector = 0;
	int physical_sector = 0;
};

struct OVF_Network
{
	QString name;
	QString connection;
	QString adapter_type; // e1000, virtio, pcnet, etc.
};

struct OVF_Appliance
{
	QString name;
	QString description;
	QString os_type_raw;
	QString aqemu_profile_name; // mapped to one of the 195 profiles!
	int cpu_count = 1;
	int memory_mb = 2048;
	QList<OVF_Disk> disks;
	QList<OVF_Network> networks;
	QString base_dir;
	QString ovf_file_path;
	QString source_archive_path;
};

class OVF_Parser
{
public:
	static bool Is_OVA_Archive( const QString &path );
	static bool Is_OVF_File( const QString &path );

	// Extract .ova TAR archive to destination directory
	static bool Extract_OVA( const QString &ova_path, const QString &dest_dir,
	                         QString &out_ovf_path, QStringList &out_extracted_files,
	                         QString &error_msg,
	                         std::function<void(int progress, const QString &status)> progress_cb = nullptr );

	// Extract only the OVF descriptor from OVA without extracting disk payloads (fast inspection)
	static bool Extract_OVF_Only( const QString &ova_path, const QString &dest_dir,
	                             QString &out_ovf_path, QString &error_msg );

	// Pack files into a standard POSIX ustar .ova archive
	static bool Pack_OVA( const QStringList &file_paths, const QString &dest_ova_path,
	                      QString &error_msg,
	                      std::function<void(int progress, const QString &status)> progress_cb = nullptr );

	// Parse .ovf XML descriptor
	static bool Parse_OVF( const QString &ovf_path, OVF_Appliance &appliance, QString &error_msg );

	// Map OVF/VirtualBox/VMware/CIM OS string to closest AQEMU profile name
	static QString Map_OS_To_AQEMU_Profile( const QString &os_str, int cim_os_id = -1 );

	// Generate standard DMTF OVF 1.0 XML from an AQEMU Virtual_Machine
	static QString Generate_OVF_XML( const Virtual_Machine &vm, const QList<OVF_Disk> &disks );
};

#endif // OVF_PARSER_H
