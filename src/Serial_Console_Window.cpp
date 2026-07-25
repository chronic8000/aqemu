/****************************************************************************
** Guest serial console (TCP) — virt-manager-like, separate from QEMU monitor.
****************************************************************************/

#include "Serial_Console_Window.h"
#include "VM.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QTextCursor>

Serial_Console_Window::Serial_Console_Window( QWidget *parent )
	: QDialog( parent )
	, VM( nullptr )
	, Sock( new QTcpSocket( this ) )
	, Host( QStringLiteral( "127.0.0.1" ) )
	, Port( 0 )
{
	setWindowTitle( tr( "Serial console" ) );
	resize( 720, 420 );

	QVBoxLayout *lay = new QVBoxLayout( this );
	lay->addWidget( new QLabel( tr(
		"Guest serial (COM1) — not the QEMU monitor. Useful for Linux/BSD installers and headless boards." ) ) );

	Out = new QPlainTextEdit();
	Out->setReadOnly( true );
	Out->setStyleSheet( QStringLiteral( "QPlainTextEdit { background: #111; color: #0f0; font-family: Consolas, monospace; }" ) );
	lay->addWidget( Out, 1 );

	QHBoxLayout *row = new QHBoxLayout();
	In = new QLineEdit();
	In->setPlaceholderText( tr( "Type and press Enter (Ctrl+Enter sends raw line)" ) );
	QPushButton *send = new QPushButton( tr( "Send" ) );
	QPushButton *conn = new QPushButton( tr( "Connect" ) );
	row->addWidget( In, 1 );
	row->addWidget( send );
	row->addWidget( conn );
	lay->addLayout( row );

	connect( send, &QPushButton::clicked, this, &Serial_Console_Window::Send_Line );
	connect( conn, &QPushButton::clicked, this, &Serial_Console_Window::Connect_Now );
	connect( In, &QLineEdit::returnPressed, this, &Serial_Console_Window::Send_Line );
	connect( Sock, &QTcpSocket::readyRead, this, &Serial_Console_Window::On_Ready_Read );
	connect( Sock, &QTcpSocket::connected, this, &Serial_Console_Window::On_Connected );
	connect( Sock, &QTcpSocket::disconnected, this, &Serial_Console_Window::On_Disconnected );
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	connect( Sock, &QTcpSocket::errorOccurred, this, &Serial_Console_Window::On_Error );
#else
	connect( Sock, SIGNAL(error(QAbstractSocket::SocketError)),
	         this, SLOT(On_Error(QAbstractSocket::SocketError)) );
#endif
	In->installEventFilter( this );
}

void Serial_Console_Window::Attach( Virtual_Machine *vm )
{
	VM = vm;
	Port = 0;
	if( VM )
		Port = static_cast<quint16>( VM->Get_Serial_Console_Port() );
	setWindowTitle( Port > 0
		? tr( "Serial console — %1:%2" ).arg( Host ).arg( Port )
		: tr( "Serial console" ) );
	if( Port > 0 )
		QTimer::singleShot( 400, this, SLOT(Connect_Now()) );
	else
		Out->appendPlainText( tr( "[AQEMU] No serial console port yet — start the VM first." ) );
}

void Serial_Console_Window::Detach()
{
	if( Sock->state() != QAbstractSocket::UnconnectedState )
		Sock->disconnectFromHost();
	VM = nullptr;
	Port = 0;
}

void Serial_Console_Window::Connect_Now()
{
	if( VM )
		Port = static_cast<quint16>( VM->Get_Serial_Console_Port() );
	if( Port == 0 )
	{
		Out->appendPlainText( tr( "[AQEMU] Serial port not allocated." ) );
		return;
	}
	if( Sock->state() == QAbstractSocket::ConnectedState )
		return;
	Out->appendPlainText( tr( "[AQEMU] Connecting to %1:%2 …" ).arg( Host ).arg( Port ) );
	Sock->connectToHost( Host, Port );
}

void Serial_Console_Window::On_Ready_Read()
{
	const QByteArray data = Sock->readAll();
	Out->moveCursor( QTextCursor::End );
	Out->insertPlainText( QString::fromLocal8Bit( data ) );
	Out->moveCursor( QTextCursor::End );
}

void Serial_Console_Window::On_Connected()
{
	Out->appendPlainText( tr( "[AQEMU] Connected." ) );
}

void Serial_Console_Window::On_Disconnected()
{
	Out->appendPlainText( tr( "[AQEMU] Disconnected." ) );
}

void Serial_Console_Window::On_Error( QAbstractSocket::SocketError )
{
	Out->appendPlainText( tr( "[AQEMU] %1" ).arg( Sock->errorString() ) );
}

void Serial_Console_Window::Send_Line()
{
	if( Sock->state() != QAbstractSocket::ConnectedState )
		Connect_Now();
	if( Sock->state() != QAbstractSocket::ConnectedState )
		return;
	QByteArray line = In->text().toLocal8Bit();
	line.append( '\r' );
	Sock->write( line );
	In->clear();
}

bool Serial_Console_Window::eventFilter( QObject *obj, QEvent *event )
{
	if( obj == In && event->type() == QEvent::KeyPress )
	{
		QKeyEvent *ke = static_cast<QKeyEvent*>( event );
		// Raw single keys useful for bootloader menus when line mode is awkward
		if( ke->modifiers() == Qt::ControlModifier && ke->key() == Qt::Key_C )
		{
			if( Sock->state() == QAbstractSocket::ConnectedState )
				Sock->write( "\x03", 1 );
			return true;
		}
	}
	return QDialog::eventFilter( obj, event );
}

void Serial_Console_Window::closeEvent( QCloseEvent *event )
{
	// Keep socket; user may reopen. Just hide.
	event->accept();
}
