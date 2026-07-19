/****************************************************************************
** AQEMU — QMP (QEMU Machine Protocol) JSON client
****************************************************************************/
#ifndef QMP_CLIENT_H
#define QMP_CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>

class QMP_Client : public QObject
{
	Q_OBJECT
	public:
		explicit QMP_Client( QObject *parent = 0 );

		bool Connect_To( const QString &host, quint16 port );
		void Disconnect();
		bool Is_Connected() const;

		bool Send_Command( const QString &execute, const QJsonObject &arguments = QJsonObject() );
		bool Change_Medium( const QString &device_id, const QString &file_path );
		bool Eject_Medium( const QString &device_id, bool force = true );
		bool System_Powerdown();
		bool System_Reset();
		bool Quit_QEMU();
		bool Query_Block();

		quint16 Port() const { return Port_; }

	signals:
		void Connected();
		void Disconnected();
		void Error( const QString &message );
		void Event_Received( const QJsonObject &event );
		void Reply_Received( const QJsonObject &reply );
		void Block_Info( const QJsonArray &devices );

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
};

quint16 Find_Free_TCP_Port( quint16 start = 4444 );

#endif
