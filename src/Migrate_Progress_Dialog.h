/****************************************************************************
** Live migration progress / cancel (QMP query-migrate)
****************************************************************************/
#ifndef MIGRATE_PROGRESS_DIALOG_H
#define MIGRATE_PROGRESS_DIALOG_H

#include <QDialog>
#include <QString>

class QMP_Client;
class QLabel;
class QProgressBar;
class QPushButton;
class QTimer;

class Migrate_Progress_Dialog : public QDialog
{
	Q_OBJECT
	public:
		explicit Migrate_Progress_Dialog( QMP_Client *qmp, const QString &uri,
		                                  QWidget *parent = 0 );

	private slots:
		void On_Poll();
		void On_Cancel();
		void On_Reply( const QJsonObject &reply );

	private:
		QMP_Client *QMP;
		QString URI;
		QLabel *Status_Label;
		QLabel *Detail_Label;
		QProgressBar *Bar;
		QPushButton *Cancel_Btn;
		QTimer *Poll_Timer;
		bool Finished;
};

#endif
