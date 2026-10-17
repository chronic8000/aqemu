#ifndef GUEST_FILE_CHANNEL_H
#define GUEST_FILE_CHANNEL_H

#include "Guest_Agent.h"

#include <QObject>
#include <QLocalSocket>
#include <QTimer>

class Guest_File_Channel : public QObject
{
	Q_OBJECT
	public:
		explicit Guest_File_Channel( QObject *parent = 0 );
		void Start( const QString &socket_path );
		void Stop();

	signals:
		void Files_Arrived( const QList<AQ_Guest_File> &files );
		void Channel_Error( const QString &message );

	private slots:
		void Try_Connect();
		void On_Ready_Read();
		void On_Disconnected();

	private:
		QString path;
		QLocalSocket socket;
		QTimer retry;
		AQ_Guest_Frame_Parser parser;
};

#endif
