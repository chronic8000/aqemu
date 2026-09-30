#ifndef TRUENAS_GUEST_API_H
#define TRUENAS_GUEST_API_H

#include <QObject>
#include <QString>

#include "Storage_Recovery.h"

Q_DECLARE_METATYPE( TrueNAS_Pool_View )
Q_DECLARE_METATYPE( TrueNAS_Snapshot_View )

class QNetworkAccessManager;
class QNetworkReply;

class TrueNAS_Guest_Client : public QObject
{
	Q_OBJECT

	public:
		explicit TrueNAS_Guest_Client( QObject *parent = nullptr );
		void Set_Target( const QString &host, int port, const QString &token );
		void Fetch_Pools();
		void Fetch_Snapshots();
		void Rollback( const QString &snapshot_id );

	signals:
		void Pools_Ready( const QList<TrueNAS_Pool_View> &pools );
		void Snapshots_Ready( const QList<TrueNAS_Snapshot_View> &snapshots );
		void Rollback_Done( const QString &snapshot_id );
		void Failed( const QString &message );

	private:
		void Get( const QString &api_path, const char *kind );
		void Post_Rollback( const QString &snapshot_id );

		QNetworkAccessManager *Net;
		QString Host;
		int Port;
		QString Token;
};

#endif
