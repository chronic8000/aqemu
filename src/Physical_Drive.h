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

#ifndef PHYSICAL_DRIVE_H
#define PHYSICAL_DRIVE_H

#include <QString>
#include <QStringList>

class QWidget;

// Whole disks (\\.\PhysicalDriveN, /dev/sda, /dev/nvme0n1) and partitions
// (\\.\HarddiskNPartitionM, /dev/sda1, /dev/nvme0n1p1).
bool AQ_Is_Physical_Disk_Path( const QString &path );

// QFile::exists is false for \\.\PhysicalDriveN even when the drive is there.
bool AQ_Storage_Path_Present( const QString &path );

// Empty paths: make sure this process can list and open host disks.
// Windows restarts AQEMU through UAC when it is not already elevated.
// Linux asks for sudo and grants the current user access to the drives.
// A non-empty list checks those devices, and refuses the host startup disk.
// Returns false when the user cancels, elevation was started, or access failed.
bool AQ_Require_Physical_Disk_Privilege( QWidget *parent, const QStringList &paths );

// Two steps: entire drive or partitions, then a list. The host startup disk
// is omitted. allow_many is for recovery, where several drives can be added.
QStringList AQ_Pick_Physical_Drives( QWidget *parent, bool allow_many );

#endif
