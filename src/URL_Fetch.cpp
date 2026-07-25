/****************************************************************************
** Download a URL to a local file (wizard network install).
****************************************************************************/

#include "URL_Fetch.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProgressDialog>
#include <QMessageBox>
#include <QUrl>
#include <QTimer>

QString AQ_Download_URL_To_File( QWidget *parent,
                                 const QString &url,
                                 const QString &dest_path,
                                 const QString &title )
{
	const QUrl qurl( url.trimmed() );
	if( ! qurl.isValid() || qurl.scheme().isEmpty() )
	{
		QMessageBox::warning( parent, QObject::tr( "Download" ),
			QObject::tr( "Invalid URL:\n%1" ).arg( url ) );
		return QString();
	}

	QFileInfo fi( dest_path );
	QDir().mkpath( fi.absolutePath() );

	QNetworkAccessManager nam;
	QNetworkRequest req( qurl );
	req.setAttribute( QNetworkRequest::FollowRedirectsAttribute, true );
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	req.setAttribute( QNetworkRequest::RedirectPolicyAttribute,
	                  QNetworkRequest::NoLessSafeRedirectPolicy );
#endif

	QNetworkReply *reply = nam.get( req );
	QProgressDialog prog( title.isEmpty()
		? QObject::tr( "Downloading %1…" ).arg( qurl.fileName() )
		: title,
		QObject::tr( "Cancel" ), 0, 100, parent );
	prog.setWindowModality( Qt::WindowModal );
	prog.setMinimumDuration( 0 );
	prog.setValue( 0 );

	QEventLoop loop;
	QObject::connect( reply, &QNetworkReply::finished, &loop, &QEventLoop::quit );
	QObject::connect( &prog, &QProgressDialog::canceled, reply, &QNetworkReply::abort );
	QObject::connect( reply, &QNetworkReply::downloadProgress,
		&prog, [&prog]( qint64 rec, qint64 total ) {
			if( total > 0 )
			{
				prog.setMaximum( 100 );
				prog.setValue( static_cast<int>( ( rec * 100 ) / total ) );
			}
			else
			{
				prog.setMaximum( 0 ); // busy
			}
		} );

	loop.exec();
	prog.reset();

	if( reply->error() != QNetworkReply::NoError )
	{
		if( reply->error() != QNetworkReply::OperationCanceledError )
		{
			QMessageBox::warning( parent, QObject::tr( "Download" ),
				QObject::tr( "Download failed:\n%1" ).arg( reply->errorString() ) );
		}
		reply->deleteLater();
		return QString();
	}

	QFile out( dest_path );
	if( ! out.open( QIODevice::WriteOnly ) )
	{
		QMessageBox::warning( parent, QObject::tr( "Download" ),
			QObject::tr( "Cannot write:\n%1" ).arg( dest_path ) );
		reply->deleteLater();
		return QString();
	}
	out.write( reply->readAll() );
	out.close();
	reply->deleteLater();
	return QDir::toNativeSeparators( dest_path );
}
