/****************************************************************************
** This file is part of AQEMU.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License.
****************************************************************************/

#include "Audio_Host.h"

#include "Utils.h"

#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include <objbase.h>
#include <dsound.h>
#endif

void AQ_Audio_Profile::set_option( const QString &key, const QString &value )
{
	const QString k = key.trimmed();
	if( k.isEmpty() || value.isEmpty() )
		opt.remove( k );
	else
		opt.insert( k, value );
}

QString AQ_Audio_Options_To_String( const QMap<QString, QString> &opt )
{
	QStringList lines;
	for( QMap<QString, QString>::const_iterator it = opt.constBegin(); it != opt.constEnd(); ++it )
	{
		if( it.key().trimmed().isEmpty() )
			continue;
		lines << ( it.key().trimmed() + QLatin1Char( '=' ) + it.value() );
	}
	return lines.join( QLatin1Char( '\n' ) );
}

QMap<QString, QString> AQ_Audio_Options_From_String( const QString &text )
{
	QMap<QString, QString> opt;
	const QStringList lines = text.split( QLatin1Char( '\n' ), QString::SkipEmptyParts );
	for( int i = 0; i < lines.count(); ++i )
	{
		const QString line = lines.at( i ).trimmed();
		const int eq = line.indexOf( QLatin1Char( '=' ) );
		if( eq <= 0 )
			continue;
		opt.insert( line.left( eq ).trimmed(), line.mid( eq + 1 ) );
	}
	return opt;
}

QString AQ_Audio_Escape( const QString &value )
{
	QString s = value;
	s.replace( QLatin1String( "," ), QLatin1String( ",," ) );
	return s;
}

static QStringList AQ_Known_Backends()
{
	return QStringList()
		<< QStringLiteral( "sdl" )
		<< QStringLiteral( "dsound" )
		<< QStringLiteral( "pa" )
		<< QStringLiteral( "pipewire" )
		<< QStringLiteral( "alsa" )
		<< QStringLiteral( "oss" )
		<< QStringLiteral( "coreaudio" )
		<< QStringLiteral( "jack" )
		<< QStringLiteral( "sndio" )
		<< QStringLiteral( "spice" )
		<< QStringLiteral( "dbus" )
		<< QStringLiteral( "wav" )
		<< QStringLiteral( "none" );
}

static QString AQ_Audio_Help_Text( const QString &qemu_binary, const QStringList &args )
{
	if( qemu_binary.trimmed().isEmpty() || ! QFile::exists( qemu_binary ) )
		return QString();
	QProcess p;
	p.start( qemu_binary, args );
	if( ! p.waitForFinished( 2500 ) )
	{
		p.kill();
		return QString();
	}
	QString out = QString::fromUtf8( p.readAllStandardOutput() );
	out += QString::fromUtf8( p.readAllStandardError() );
	return out;
}

QStringList AQ_Audio_Backend_Names( const QString &qemu_binary )
{
	const QString help = AQ_Audio_Help_Text( qemu_binary,
		QStringList() << QStringLiteral( "-audiodev" ) << QStringLiteral( "help" ) );
	QStringList found;
	const QStringList known = AQ_Known_Backends();
	const QStringList lines = help.split( QRegularExpression( QStringLiteral( "[\\r\\n]+" ) ),
		QString::SkipEmptyParts );
	for( int i = 0; i < lines.count(); ++i )
	{
		const QString t = lines.at( i ).trimmed().toLower();
		if( known.contains( t ) )
			found << t;
	}
	found.removeDuplicates();
	return found.isEmpty() ? known : found;
}

bool AQ_Audio_Backend_Has_Dev( const QString &qemu_binary, const QString &backend )
{
	const QString b = backend.trimmed().toLower();
	if( b.isEmpty() )
		return false;
	static QMap<QString, bool> cache;
	const QString key = qemu_binary + QLatin1Char( '|' ) + b;
	if( cache.contains( key ) )
		return cache.value( key );
	const QString help = AQ_Audio_Help_Text( qemu_binary,
		QStringList() << QStringLiteral( "-audiodev" ) << ( b + QStringLiteral( ",help" ) ) );
	const bool has = help.contains( QLatin1String( "dev=" ) ) ||
		help.contains( QLatin1String( ".dev" ) ) ||
		help.contains( QLatin1String( "out.dev" ) ) ||
		help.contains( QLatin1String( "in.dev" ) ) ||
		help.contains( QLatin1String( "in|out.dev" ) );
	// Pulse, PipeWire, ALSA, OSS, JACK, and sndio accept a device in upstream QEMU
	// even when a short help probe fails.
	const bool native = ( b == QLatin1String( "pa" ) || b == QLatin1String( "pipewire" ) ||
		b == QLatin1String( "alsa" ) || b == QLatin1String( "oss" ) ||
		b == QLatin1String( "jack" ) || b == QLatin1String( "sndio" ) );
	const bool ok = native || ( ! help.isEmpty() && has );
	cache.insert( key, ok );
	return ok;
}

QString AQ_Audio_Device_Key( const QString &backend, bool capture )
{
	const QString b = backend.trimmed().toLower();
	const QString side = capture ? QStringLiteral( "in." ) : QStringLiteral( "out." );
	if( b == QLatin1String( "pa" ) || b == QLatin1String( "pipewire" ) )
		return side + QStringLiteral( "name" );
	if( b == QLatin1String( "jack" ) )
		return side + QStringLiteral( "connect-ports" );
	return side + QStringLiteral( "dev" );
}

static void AQ_Append_Cmd_Devices( QList<QPair<QString, QString> > *list,
	const QString &program, const QStringList &args, bool name_is_second_field )
{
	QProcess p;
	p.start( program, args );
	if( ! p.waitForFinished( 2000 ) )
	{
		p.kill();
		return;
	}
	const QStringList lines = QString::fromUtf8( p.readAllStandardOutput() )
		.split( QRegularExpression( QStringLiteral( "[\\r\\n]+" ) ), QString::SkipEmptyParts );
	QString pending_name;
	for( int i = 0; i < lines.count(); ++i )
	{
		const QString line = lines.at( i );
		if( name_is_second_field )
		{
			const QStringList f = line.split( QRegularExpression( QStringLiteral( "\\s+" ) ),
				QString::SkipEmptyParts );
			if( f.count() >= 2 )
				list->append( qMakePair( f.at( 1 ), f.at( 1 ) ) );
			continue;
		}
		if( ! line.startsWith( QLatin1Char( ' ' ) ) && ! line.startsWith( QLatin1Char( '\t' ) ) )
		{
			pending_name = line.trimmed();
			if( ! pending_name.isEmpty() && pending_name != QLatin1String( "null" ) )
				list->append( qMakePair( pending_name, pending_name ) );
		}
	}
}

#ifdef Q_OS_WIN
static BOOL CALLBACK AQ_DSound_Enum( LPGUID guid, LPCWSTR desc, LPCWSTR, LPVOID ctx )
{
	QList<QPair<QString, QString> > *list = static_cast<QList<QPair<QString, QString> > *>( ctx );
	if( ! guid || ! desc )
		return TRUE;
	OLECHAR buf[64];
	if( StringFromGUID2( *guid, buf, 64 ) <= 0 )
		return TRUE;
	const QString g = QString::fromWCharArray( buf );
	const QString n = QString::fromWCharArray( desc );
	list->append( qMakePair( n, g ) );
	return TRUE;
}
#endif

QList<QPair<QString, QString> > AQ_Audio_List_Devices( const QString &backend, bool capture )
{
	QList<QPair<QString, QString> > list;
	list.append( qMakePair( QStringLiteral( "System default" ), QString() ) );
	const QString b = backend.trimmed().toLower();
#ifdef Q_OS_WIN
	if( b == QLatin1String( "dsound" ) || b == QLatin1String( "sdl" ) )
	{
		CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
		if( capture )
			DirectSoundCaptureEnumerateW( AQ_DSound_Enum, &list );
		else
			DirectSoundEnumerateW( AQ_DSound_Enum, &list );
		return list;
	}
#else
	Q_UNUSED( capture );
#endif
	if( b == QLatin1String( "pa" ) )
	{
		AQ_Append_Cmd_Devices( &list, QStringLiteral( "pactl" ),
			QStringList() << QStringLiteral( "list" ) << QStringLiteral( "short" )
				<< ( capture ? QStringLiteral( "sources" ) : QStringLiteral( "sinks" ) ),
			true );
	}
	else if( b == QLatin1String( "pipewire" ) )
	{
		AQ_Append_Cmd_Devices( &list, QStringLiteral( "wpctl" ),
			QStringList() << QStringLiteral( "status" ), false );
		if( list.count() <= 1 )
		{
			AQ_Append_Cmd_Devices( &list, QStringLiteral( "pw-cli" ),
				QStringList() << QStringLiteral( "list-objects" ), false );
		}
	}
	else if( b == QLatin1String( "alsa" ) )
	{
		AQ_Append_Cmd_Devices( &list,
			capture ? QStringLiteral( "arecord" ) : QStringLiteral( "aplay" ),
			QStringList() << QStringLiteral( "-L" ), false );
	}
	return list;
}

QString AQ_Audio_HDA_Codec_Name( const QString &saved )
{
	const QString c = saved.trimmed().toLower();
	if( c == QLatin1String( "duplex" ) || c == QLatin1String( "hda-duplex" ) )
		return QStringLiteral( "hda-duplex" );
	if( c == QLatin1String( "micro" ) || c == QLatin1String( "hda-micro" ) )
		return QStringLiteral( "hda-micro" );
	return QStringLiteral( "hda-output" );
}

AQ_Audio_Profile AQ_Audio_Host_Load()
{
	QSettings s;
	s.beginGroup( QStringLiteral( "Audio" ) );
	AQ_Audio_Profile p;
	if( ! s.value( QStringLiteral( "Initialized" ), false ).toBool() )
	{
		s.endGroup();
#ifdef Q_OS_WIN
		p.backend = QStringLiteral( "sdl" );
#else
		p.backend = QStringLiteral( "pa" );
#endif
		QSettings root;
		const bool custom = root.value( QStringLiteral( "QEMU_AUDIO/Use_Default_Driver" ), QStringLiteral( "yes" ) )
			.toString() == QLatin1String( "no" );
		const QString old = root.value( QStringLiteral( "QEMU_AUDIO/QEMU_AUDIO_DRV" ) ).toString().trimmed();
		if( custom && ! old.isEmpty() )
			p.backend = old;
		p.set_option( QStringLiteral( "in.voices" ), QStringLiteral( "0" ) );
		AQ_Audio_Host_Save( p );
		return p;
	}
	p.backend = s.value( QStringLiteral( "Backend" ) ).toString().trimmed();
	p.opt = AQ_Audio_Options_From_String( s.value( QStringLiteral( "Options" ) ).toString() );
	s.endGroup();
	return p;
}

void AQ_Audio_Host_Save( const AQ_Audio_Profile &profile )
{
	QSettings s;
	s.beginGroup( QStringLiteral( "Audio" ) );
	s.setValue( QStringLiteral( "Initialized" ), true );
	s.setValue( QStringLiteral( "Backend" ), profile.backend.trimmed() );
	s.setValue( QStringLiteral( "Options" ), AQ_Audio_Options_To_String( profile.opt ) );
	s.endGroup();
}

QString AQ_Audio_Build_Audiodev(
	bool use_host_defaults,
	const QString &vm_options,
	const QString &legacy_backend,
	int legacy_timer_us,
	const QString &qemu_binary,
	const QString &forced_backend )
{
	AQ_Audio_Profile p = use_host_defaults ? AQ_Audio_Host_Load() : AQ_Audio_Profile();
	if( ! use_host_defaults )
	{
		p.opt = AQ_Audio_Options_From_String( vm_options );
		p.backend = p.opt.value( QStringLiteral( "backend" ) ).trimmed();
		p.opt.remove( QStringLiteral( "backend" ) );
	}
	if( ! forced_backend.trimmed().isEmpty() )
		p.backend = forced_backend.trimmed();
	else if( p.backend.trimmed().isEmpty() )
		p.backend = legacy_backend.trimmed();
	if( p.backend.trimmed().isEmpty() )
	{
#ifdef Q_OS_WIN
		p.backend = QStringLiteral( "sdl" );
#else
		p.backend = QStringLiteral( "pa" );
#endif
	}
	p.backend = p.backend.trimmed().toLower();
	if( legacy_timer_us > 0 && ! p.opt.contains( QStringLiteral( "timer-period" ) ) )
		p.set_option( QStringLiteral( "timer-period" ), QString::number( legacy_timer_us ) );

	const bool gated = ( p.backend == QLatin1String( "dsound" ) || p.backend == QLatin1String( "sdl" ) );
	if( gated && ! AQ_Audio_Backend_Has_Dev( qemu_binary, p.backend ) )
	{
		p.opt.remove( QStringLiteral( "in.dev" ) );
		p.opt.remove( QStringLiteral( "out.dev" ) );
	}

	QString arg = p.backend + QStringLiteral( ",id=snd0" );
	for( QMap<QString, QString>::const_iterator it = p.opt.constBegin(); it != p.opt.constEnd(); ++it )
	{
		const QString key = it.key().trimmed();
		const QString val = it.value().trimmed();
		if( key.isEmpty() || val.isEmpty() || key.startsWith( QLatin1String( "guest." ) ) ||
		    key == QLatin1String( "backend" ) )
			continue;
		if( val == QLatin1String( "default" ) &&
		    ( key.endsWith( QLatin1String( ".dev" ) ) || key.endsWith( QLatin1String( ".name" ) ) ) )
			continue;
		arg += QLatin1Char( ',' );
		arg += key;
		arg += QLatin1Char( '=' );
		arg += AQ_Audio_Escape( val );
	}
	return arg;
}

bool AQ_Audio_Machine_Has_Pcspk_Property( const QString &qemu_binary, const QString &machine )
{
	const QString m = machine.trimmed().isEmpty() ? QStringLiteral( "pc" ) : machine.trimmed();
	static QMap<QString, bool> cache;
	const QString key = qemu_binary + QLatin1Char( '|' ) + m;
	if( cache.contains( key ) )
		return cache.value( key );
	const QString help = AQ_Audio_Help_Text( qemu_binary,
		QStringList() << QStringLiteral( "-machine" ) << ( m + QStringLiteral( ",help" ) ) );
	const bool has = help.contains( QLatin1String( "pcspk-audiodev" ) );
	cache.insert( key, has );
	return has;
}
