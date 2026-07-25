/****************************************************************************
** Guest serial console (TCP) — virt-manager-like, separate from QEMU monitor.
****************************************************************************/
#ifndef SERIAL_CONSOLE_WINDOW_H
#define SERIAL_CONSOLE_WINDOW_H

#include <QDialog>
#include <QTcpSocket>

class QPlainTextEdit;
class QLineEdit;
class Virtual_Machine;

class Serial_Console_Window : public QDialog
{
	Q_OBJECT
	public:
		explicit Serial_Console_Window( QWidget *parent = 0 );
		void Attach( Virtual_Machine *vm );
		void Detach();

	private slots:
		void Connect_Now();
		void On_Ready_Read();
		void On_Connected();
		void On_Disconnected();
		void On_Error( QAbstractSocket::SocketError err );
		void Send_Line();

	protected:
		void closeEvent( QCloseEvent *event ) override;
		bool eventFilter( QObject *obj, QEvent *event ) override;

	private:
		Virtual_Machine *VM;
		QTcpSocket *Sock;
		QPlainTextEdit *Out;
		QLineEdit *In;
		QString Host;
		quint16 Port;
};

#endif
