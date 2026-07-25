/****************************************************************************
** Download a URL to a local file (wizard network install).
****************************************************************************/
#ifndef URL_FETCH_H
#define URL_FETCH_H

#include <QString>
#include <QWidget>

/** Blocking download with a modal progress dialog. Returns local path or empty on failure/cancel. */
QString AQ_Download_URL_To_File( QWidget *parent,
                                 const QString &url,
                                 const QString &dest_path,
                                 const QString &title = QString() );

#endif
