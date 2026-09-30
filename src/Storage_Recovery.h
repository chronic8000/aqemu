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

#ifndef STORAGE_RECOVERY_H
#define STORAGE_RECOVERY_H

#include <QList>
#include <QString>
#include <QStringList>
#include <functional>

// Phase 1 storage recovery. AQEMU identifies labels, describes them, builds a
// read-only VM, and can isolate writes in a qcow2 checkpoint. The guest decides
// whether a pool imports. This code never imports or repairs a pool.

struct Disk_Probe
{
	QString path;
	qint64 size = 0;
	QString type_label;
	bool opened = false;
	QString open_error;
	bool zfs_labels = false;
	bool linux_raid = false;
	QString pool_name;
	quint64 pool_guid = 0;
	quint64 disk_guid = 0;
	int ashift = 0;
	int descriptor_logical = 0;
	int descriptor_physical = 0;
	QList<quint64> expected_members;
};

struct Probable_Pool
{
	QString name;
	quint64 pool_guid = 0;
	int ashift = 0;
	QList<quint64> expected_members;
	QList<quint64> present_members;
	QList<quint64> missing_members;
	bool incomplete = false;
};

struct Report_Facts
{
	QString aqemu_version;
	QString qemu_version;
	QList<Disk_Probe> sources;
	QList<Probable_Pool> pools;
	QString guest_os;
	QString machine;
	QString controller;
	int sector_size = 512;
	QString boot_media;
	bool host_boot_disk_excluded = true;
	QString checkpoint_text;
};

struct Recovery_Disk
{
	QString source_path;
	QString source_identity;
	qint64 source_size = 0;
	QString source_format;
	QString overlay_filename;
	bool was_read_only = true;
};

struct Checkpoint_Manifest
{
	QString directory;
	QString checkpoint_id;
	QString created;
	QString state;
	QList<Recovery_Disk> disks;
};

struct Checkpoint_Outcome
{
	bool ok = false;
	QString user_message;
	QString detail;
	Checkpoint_Manifest manifest;
};

struct Drive_Binding
{
	QString path;
	bool read_only = true;
};

typedef std::function<bool(const QString &source_path, const QString &format,
	const QString &overlay_path, QString *error)> Overlay_Writer;

QString Checkpoint_Failure_Text();
QString Discard_Confirm_Text();
QString Keep_Button_Subtitle();

QString Whole_Disk_Device( const QString &path );
QString Host_Boot_Disk_Path();
bool Path_Is_Host_Boot_Disk( const QString &path );
QStringList List_Recovery_Physical_Disks( QString *note );

Disk_Probe Probe_Disk( const QString &path );
QList<Probable_Pool> Group_Probable_Pools( const QList<Disk_Probe> &probes );
int Sector_Size_From_Pools( const QList<Probable_Pool> &pools );
bool Recovery_Wizard_Can_Continue( const QList<Disk_Probe> &probes );
QString Build_Recovery_Report( const Report_Facts &facts );
QString Format_Guest_Import_Status( bool guest_reported_success, const QString &guest_text );

struct Vmdk_Sector_Size
{
	int logical = 0;
	int physical = 0;
};

QString Disk_Format_For_Overlay( const QString &path );
Vmdk_Sector_Size Read_Vmdk_Sector_Size( const QString &path );
int Four_Kn_Sector( int logical_sector, int physical_sector, int ashift );
bool Parse_Vmw_Disk_Property( const QString &key, QString *controller, int *controller_index,
	int *unit, QString *field );
QString Stable_Disk_Serial( const QString &ovf_serial, quint64 zfs_disk_guid, int one_based_index );

struct Lab_Disk_Spec
{
	QString filename;
	QString serial;
	int logical_sector = 4096;
	bool nvme = false;
};

QList<Lab_Disk_Spec> Plan_Lab_Disks( const QString &name_key, int count, bool qcow2, bool nvme );

struct Storage_Node_Spec
{
	QString role;
	QString title_suffix;
	int ram_mb = 2048;
	int disk_count = 1;
	int disk_gb = 16;
	bool four_kn = false;
	bool uefi = false;
	bool virtio_scsi = true;
	QString nic_model;
	QString mac;
};

struct Storage_Network_Plan
{
	QString mcast;
	QString localaddr;
	QList<Storage_Node_Spec> nodes;
};

Storage_Network_Plan Plan_Storage_Network( const QString &name_key );

struct TrueNAS_Pool_View
{
	QString name;
	QString status;
	QString line;
	qint64 size_bytes = -1;
	qint64 allocated_bytes = -1;
};

struct TrueNAS_Snapshot_View
{
	QString id;
	QString label;
};

QString TrueNAS_API_URL( const QString &host, int port, const QString &api_path, QString *error );
QString TrueNAS_Rollback_Path( const QString &snapshot_id );
QString TrueNAS_Pool_Line( const QString &name, const QString &status );
QList<TrueNAS_Pool_View> Parse_TrueNAS_Pools( const QByteArray &json, QString *error );
QList<TrueNAS_Snapshot_View> Parse_TrueNAS_Snapshots( const QByteArray &json, QString *error );
QString Source_Identity( const Disk_Probe &probe );

Checkpoint_Outcome Create_Checkpoint( const QString &attempt_dir,
	const QList<Recovery_Disk> &sources, const Overlay_Writer &write_overlay );
bool Load_Checkpoint_Manifest( const QString &manifest_path, Checkpoint_Manifest *out, QString *error );
bool Discard_Checkpoint( const QString &manifest_path, QString *error );
bool Keep_Checkpoint( const QString &manifest_path, QString *error );
bool Create_Qcow2_Backing_Overlay( const QString &qemu_img, const QString &source_path,
	const QString &format, const QString &overlay_path, QString *error );

QList<Drive_Binding> Apply_Checkpoint_Result( const QList<Drive_Binding> &original,
	const Checkpoint_Outcome &outcome );
QList<Drive_Binding> Bindings_After_Discard( const Checkpoint_Manifest &manifest );
QList<Drive_Binding> Bindings_After_Keep( const Checkpoint_Manifest &manifest );

QByteArray Build_Zfs_Label( const QString &pool_name, quint64 pool_guid, quint64 disk_guid,
	int ashift, const QList<quint64> &member_guids );
bool Write_Zfs_Label_Fixture( const QString &path, qint64 size, const QByteArray &label, QString *error );

QString Checkpoint_Section_Marker();
bool Update_Report_Checkpoint( const QString &report_path, const QString &checkpoint_text, QString *error );

#endif
