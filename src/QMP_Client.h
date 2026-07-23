/****************************************************************************
** AQEMU — QMP (QEMU Machine Protocol) JSON client
****************************************************************************/
#ifndef QMP_CLIENT_H
#define QMP_CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QHash>
#include <QTimer>

class QMP_Client : public QObject
{
	Q_OBJECT
	public:
		explicit QMP_Client( QObject *parent = 0 );

		bool Connect_To( const QString &host, quint16 port );
		void Disconnect();
		bool Is_Connected() const;
		bool Is_Connecting() const;

		bool Send_Command( const QString &execute, const QJsonObject &arguments = QJsonObject() );
		bool Change_Medium( const QString &device_id, const QString &file_path );
		bool Eject_Medium( const QString &device_id, bool force = true );
		bool System_Powerdown();
		bool System_Reset();
		bool Quit_QEMU();
		bool Query_Block();
		bool Query_Blockstats();
		bool Human_Monitor( const QString &command_line );
		/** Outgoing migration: uri e.g. tcp:host:port. Optional blk/inc for storage migration. */
		bool Migrate( const QString &uri, bool blk = false, bool inc = false );
		bool Query_Migrate();
		bool Migrate_Cancel();
		/** Non-blocking connect (no waitForConnected). Capabilities still async. */
		void Connect_Async( const QString &host, quint16 port );

		quint16 Port() const { return Port_; }

	signals:
		void Connected();
		void Disconnected();
		void Error( const QString &message );
		void Event_Received( const QJsonObject &event );
		void Reply_Received( const QJsonObject &reply );
		void Block_Info( const QJsonArray &devices );
		void Block_Stats( const QJsonArray &stats );

	private slots:
		void On_Ready_Read();
		void On_Socket_Error( QAbstractSocket::SocketError err );
		void On_Connected();

	private:
		void Handle_Line( const QByteArray &line );
		QTcpSocket *Socket;
		QByteArray Buffer;
		quint16 Port_;
		bool Capabilities_Sent;
		int Next_Id;
		QHash<int, QString> Pending_Commands;
};

quint16 Find_Free_TCP_Port( quint16 start = 4444 );

#endif
