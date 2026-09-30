#include "TrueNAS_Guest_API.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUrl>

TrueNAS_Guest_Client::TrueNAS_Guest_Client( QObject *parent )
	: QObject( parent )
	, Net( new QNetworkAccessManager( this ) )
	, Port( 443 )
{
	qRegisterMetaType<TrueNAS_Pool_View>( "TrueNAS_Pool_View" );
	qRegisterMetaType<TrueNAS_Snapshot_View>( "TrueNAS_Snapshot_View" );
	qRegisterMetaType<QList<TrueNAS_Pool_View>>( "QList<TrueNAS_Pool_View>" );
	qRegisterMetaType<QList<TrueNAS_Snapshot_View>>( "QList<TrueNAS_Snapshot_View>" );
}

void TrueNAS_Guest_Client::Set_Target( const QString &host, int port, const QString &token )
{
	Host = host.trimmed();
	Port = port;
	Token = token;
}

static void Arm_Reply( QNetworkReply *reply )
{
	reply->ignoreSslErrors();
	QTimer *timer = new QTimer( reply );
	timer->setSingleShot( true );
	QObject::connect( timer, &QTimer::timeout, reply, &QNetworkReply::abort );
	timer->start( 20000 );
}

void TrueNAS_Guest_Client::Get( const QString &api_path, const char *kind )
{
	QString url_error;
	const QString url = TrueNAS_API_URL( Host, Port, api_path, &url_error );
	if( url.isEmpty() )
	{
		emit Failed( url_error );
		return;
	}
	if( Token.trimmed().isEmpty() )
	{
		emit Failed( tr( "Enter the TrueNAS API token." ) );
		return;
	}

	QNetworkRequest req{ QUrl( url ) };
	req.setRawHeader( "Authorization", QByteArray( "Bearer " ) + Token.toUtf8() );
	req.setHeader( QNetworkRequest::ContentTypeHeader, QStringLiteral( "application/json" ) );
	QNetworkReply *reply = Net->get( req );
	Arm_Reply( reply );
	const QString kind_text = QString::fromLatin1( kind );
	connect( reply, &QNetworkReply::finished, this, [this, reply, kind_text]() {
		reply->deleteLater();
		if( reply->error() != QNetworkReply::NoError )
		{
			emit Failed( tr( "TrueNAS did not answer (%1)." ).arg( reply->errorString() ) );
			return;
		}
		const QByteArray body = reply->readAll();
		QString parse_error;
		if( kind_text == QLatin1String( "pools" ) )
		{
			const QList<TrueNAS_Pool_View> pools = Parse_TrueNAS_Pools( body, &parse_error );
			if( ! parse_error.isEmpty() )
				emit Failed( parse_error );
			else
				emit Pools_Ready( pools );
		}
		else
		{
			const QList<TrueNAS_Snapshot_View> snaps = Parse_TrueNAS_Snapshots( body, &parse_error );
			if( ! parse_error.isEmpty() )
				emit Failed( parse_error );
			else
				emit Snapshots_Ready( snaps );
		}
	} );
}

void TrueNAS_Guest_Client::Fetch_Pools()
{
	Get( QStringLiteral( "/api/v2.0/pool" ), "pools" );
}

void TrueNAS_Guest_Client::Fetch_Snapshots()
{
	Get( QStringLiteral( "/api/v2.0/zfs/snapshot" ), "snapshots" );
}

void TrueNAS_Guest_Client::Rollback( const QString &snapshot_id )
{
	if( snapshot_id.trimmed().isEmpty() )
	{
		emit Failed( tr( "Select a snapshot." ) );
		return;
	}
	QString url_error;
	const QString url = TrueNAS_API_URL( Host, Port, TrueNAS_Rollback_Path( snapshot_id ), &url_error );
	if( url.isEmpty() )
	{
		emit Failed( url_error );
		return;
	}
	if( Token.trimmed().isEmpty() )
	{
		emit Failed( tr( "Enter the TrueNAS API token." ) );
		return;
	}

	QNetworkRequest req{ QUrl( url ) };
	req.setRawHeader( "Authorization", QByteArray( "Bearer " ) + Token.toUtf8() );
	req.setHeader( QNetworkRequest::ContentTypeHeader, QStringLiteral( "application/json" ) );
	const QByteArray body = QJsonDocument( QJsonObject() ).toJson( QJsonDocument::Compact );
	QNetworkReply *reply = Net->post( req, body );
	Arm_Reply( reply );
	const QString id = snapshot_id;
	connect( reply, &QNetworkReply::finished, this, [this, reply, id]() {
		reply->deleteLater();
		if( reply->error() != QNetworkReply::NoError )
		{
			emit Failed( tr( "TrueNAS refused the rollback (%1)." ).arg( reply->errorString() ) );
			return;
		}
		emit Rollback_Done( id );
	} );
}
