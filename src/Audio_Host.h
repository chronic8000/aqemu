/****************************************************************************
** This file is part of AQEMU.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License.
****************************************************************************/

#ifndef AUDIO_HOST_H
#define AUDIO_HOST_H

#include <QPair>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>

/** Host -audiodev profile. Keys are QEMU property names (in.voices, latency, out.dev). */
struct AQ_Audio_Profile
{
	QString backend;
	QMap<QString, QString> opt;

	QString option( const QString &key ) const { return opt.value( key ); }
	void set_option( const QString &key, const QString &value );
};

QString AQ_Audio_Options_To_String( const QMap<QString, QString> &opt );
QMap<QString, QString> AQ_Audio_Options_From_String( const QString &text );
QString AQ_Audio_Escape( const QString &value );

AQ_Audio_Profile AQ_Audio_Host_Load();
void AQ_Audio_Host_Save( const AQ_Audio_Profile &profile );

/** Drivers this QEMU reported, or the known list when help cannot be read. */
QStringList AQ_Audio_Backend_Names( const QString &qemu_binary );

/** True when -audiodev <backend>,help mentions a dev property. */
bool AQ_Audio_Backend_Has_Dev( const QString &qemu_binary, const QString &backend );

/** Friendly label, value. First entry is System default with an empty value. */
QList<QPair<QString, QString> > AQ_Audio_List_Devices( const QString &backend, bool capture );

QString AQ_Audio_Device_Key( const QString &backend, bool capture );

/** Playback-only unless the VM saved duplex or micro. */
QString AQ_Audio_HDA_Codec_Name( const QString &saved );

/**
 * One -audiodev argument body: driver,id=snd0,...
 * forced_backend replaces the profile driver (WSL Linux QEMU).
 */
QString AQ_Audio_Build_Audiodev(
	bool use_host_defaults,
	const QString &vm_options,
	const QString &legacy_backend,
	int legacy_timer_us,
	const QString &qemu_binary,
	const QString &forced_backend );

/** True when -machine TYPE,help advertises pcspk-audiodev. */
bool AQ_Audio_Machine_Has_Pcspk_Property( const QString &qemu_binary, const QString &machine );

#endif
