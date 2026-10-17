/****************************************************************************
**
** Copyright (C) 2026 Chronic Engineering
**
** Boot set for a new virtual machine: guest preference, then the
** qemu_probe_full_v3 whitelist for that binary.
**
****************************************************************************/

#ifndef GUEST_BOOT_H
#define GUEST_BOOT_H

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

struct Guest_Boot_Field
{
	QString id;
	QString label;
	bool required = false;
	bool is_file = true;
};

struct Guest_Boot_Set
{
	bool probe_loaded = false;
	QString machine;
	QString cpu;
	QString disk_bus;
	QString disk_format;
	QString nic;
	QString netdev;
	QString sound;
	QString video;
	QString display;
	QString audiodev;
	QString usb;
	bool native_accel = false;
	QList<Guest_Boot_Field> fields;
};

Guest_Boot_Set AQ_Resolve_Guest_Boot( const QString &os_name,
	const QString &target,
	const QString &machine,
	const QJsonObject &profile );

/** Profiles that cannot boot from a disk image alone. */
QStringList AQ_Guests_That_Need_Boot_Files();

#endif
