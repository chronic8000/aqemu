/****************************************************************************
** Download a URL to a local file (wizard network install).
****************************************************************************/

#include "URL_Fetch.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QSaveFile>
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

	QSaveFile out( dest_path );
	if( ! out.open( QIODevice::WriteOnly ) )
	{
		QMessageBox::warning( parent, QObject::tr( "Download" ),
			QObject::tr( "Cannot write:\n%1" ).arg( dest_path ) );
		return QString();
	}

	QNetworkReply *reply = nam.get( req );
	QProgressDialog prog( title.isEmpty()
		? QObject::tr( "Downloading %1…" ).arg( qurl.fileName() )
		: title,
		QObject::tr( "Cancel" ), 0, 100, parent );
	prog.setWindowModality( Qt::WindowModal );
	prog.setMinimumDuration( 0 );
	prog.setValue( 0 );

	bool write_ok = true;
	QString write_err;

	QEventLoop loop;
	QObject::connect( reply, &QNetworkReply::finished, &loop, &QEventLoop::quit );
	QObject::connect( &prog, &QProgressDialog::canceled, reply, &QNetworkReply::abort );
	QObject::connect( reply, &QNetworkReply::readyRead, &prog, [reply, &out, &write_ok, &write_err, &loop]() {
		if( ! write_ok )
			return;
		const QByteArray chunk = reply->readAll();
		if( chunk.isEmpty() )
			return;
		const qint64 n = out.write( chunk );
		if( n != chunk.size() )
		{
			write_ok = false;
			write_err = out.errorString().isEmpty()
				? QObject::tr( "Short write (disk full?)" )
				: out.errorString();
			reply->abort();
			loop.quit();
		}
	} );
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

	// Overall idle timeout: abort if no progress for 10 minutes.
	QTimer watchdog;
	watchdog.setInterval( 10 * 60 * 1000 );
	watchdog.setSingleShot( true );
	QObject::connect( &watchdog, &QTimer::timeout, &prog, [reply, &loop]() {
		reply->abort();
		loop.quit();
	} );
	QObject::connect( reply, &QNetworkReply::downloadProgress, &watchdog, [&watchdog]( qint64, qint64 ) {
		watchdog.start();
	} );
	watchdog.start();

	loop.exec();
	prog.reset();

	// Drain any remaining buffered data after finished.
	if( write_ok && reply->error() == QNetworkReply::NoError && reply->bytesAvailable() > 0 )
	{
		const QByteArray rest = reply->readAll();
		if( ! rest.isEmpty() )
		{
			const qint64 n = out.write( rest );
			if( n != rest.size() )
			{
				write_ok = false;
				write_err = out.errorString().isEmpty()
					? QObject::tr( "Short write (disk full?)" )
					: out.errorString();
			}
		}
	}

	const QNetworkReply::NetworkError net_err = reply->error();
	const QString net_err_str = reply->errorString();
	reply->deleteLater();

	if( ! write_ok )
	{
		out.cancelWriting();
		QMessageBox::warning( parent, QObject::tr( "Download" ),
			QObject::tr( "Failed to write file:\n%1\n%2" ).arg( dest_path, write_err ) );
		return QString();
	}

	if( net_err != QNetworkReply::NoError )
	{
		out.cancelWriting();
		if( net_err != QNetworkReply::OperationCanceledError )
		{
			QMessageBox::warning( parent, QObject::tr( "Download" ),
				QObject::tr( "Download failed:\n%1" ).arg( net_err_str ) );
		}
		return QString();
	}

	if( ! out.commit() )
	{
		QMessageBox::warning( parent, QObject::tr( "Download" ),
			QObject::tr( "Failed to finalize file:\n%1\n%2" )
				.arg( dest_path, out.errorString() ) );
		return QString();
	}

	return QDir::toNativeSeparators( dest_path );
}
