/****************************************************************************
** Host side of the QEMU guest agent, plus the clipboard file frames
** sent by the AQEMU guest helper.
****************************************************************************/
#ifndef GUEST_AGENT_H
#define GUEST_AGENT_H

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

class QJsonObject;
class QJsonValue;
class QTcpSocket;

struct AQ_Guest_File
{
	QString name;
	QByteArray data;
};

/** Bytes from org.aqemu.files. One FROM-GUEST batch becomes one list entry. */
class AQ_Guest_Frame_Parser
{
	public:
		AQ_Guest_Frame_Parser();
		void Add( const QByteArray &chunk );
		QList<AQ_Guest_File> Take();
		QString Error() const { return error; }

	private:
		enum Step { Magic, Kind, Count, Name, Size, Body };
		Step step;
		QByteArray pending;
		int files_left;
		QString current_name;
		int body_left;
		QByteArray body;
		QList<AQ_Guest_File> batch;
		QList<QList<AQ_Guest_File> > done;
		QString error;
		bool Take_Line( QByteArray &line );
		void Fail( const QString &message );
};

class AQ_Guest_Agent
{
	public:
		AQ_Guest_Agent();
		~AQ_Guest_Agent();

		bool Open( quint16 port, QString *error );
		bool Copy_To_Guest( const QString &host_path, const QString &guest_path, QString *error );
		bool Copy_From_Guest( const QString &guest_path, const QString &host_path, QString *error );

	private:
		QJsonValue Exec( const QString &command, const QJsonObject &arguments, QString *error );
		QTcpSocket *socket;
		QByteArray incoming;
};

/** Add the virtio ports. Ports must already be chosen and different. */
void AQ_Append_Guest_Channels( QStringList &args, int agent_port, int files_port );

#endif
