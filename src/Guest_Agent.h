/****************************************************************************
** Host side of the QEMU guest agent, plus the clipboard file frames
** sent by the AQEMU guest helper.
****************************************************************************/
#ifndef GUEST_AGENT_H
#define GUEST_AGENT_H

#include <QAtomicInt>
#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

class QJsonObject;
class QJsonValue;
class QLocalSocket;

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

		bool Open( const QString &socket_path, QString *error );
		bool Copy_To_Guest( const QString &host_path, const QString &guest_path, QString *error );
		bool Copy_From_Guest( const QString &guest_path, const QString &host_path, QString *error );
		void Set_Cancel( QAtomicInt *flag );

	private:
		QJsonValue Exec( const QString &command, const QJsonObject &arguments, QString *error );
		void Close_Handle( int handle );
		bool Cancelled( QString *error );
		QLocalSocket *socket;
		QByteArray incoming;
		QAtomicInt *cancel_flag;
};

/** True for machines that can accept virtio-serial-pci. */
bool AQ_Machine_Supports_Guest_Channels( const QString &machine );

/** Add the virtio ports. Paths are a private socket or a Windows named pipe. */
void AQ_Append_Guest_Channels( QStringList &args, const QString &agent_path, const QString &files_path );

#endif
