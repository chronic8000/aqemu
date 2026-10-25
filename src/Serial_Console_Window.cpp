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
#include <QGuiApplication>
#include <QClipboard>

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
		"Guest COM1 and the firmware debug port (0x402). Text typed here is sent to the guest. "
		"QEMU commands such as info pci go in VM → QEMU monitor." ) ) );

	Out = new QPlainTextEdit();
	Out->setReadOnly( true );
	Out->setStyleSheet( QStringLiteral( "QPlainTextEdit { background: #111; color: #0f0; font-family: Consolas, monospace; }" ) );
	lay->addWidget( Out, 1 );

	QHBoxLayout *row = new QHBoxLayout();
	In = new QLineEdit();
	In->setPlaceholderText( tr( "Type and press Enter (Ctrl+Enter sends raw line)" ) );
	QPushButton *send = new QPushButton( tr( "Send" ) );
	QPushButton *conn = new QPushButton( tr( "Connect" ) );
	QPushButton *copy = new QPushButton( tr( "Copy" ) );
	QPushButton *clear = new QPushButton( tr( "Clear" ) );
	row->addWidget( In, 1 );
	row->addWidget( send );
	row->addWidget( conn );
	row->addWidget( copy );
	row->addWidget( clear );
	lay->addLayout( row );

	connect( send, &QPushButton::clicked, this, &Serial_Console_Window::Send_Line );
	connect( conn, &QPushButton::clicked, this, &Serial_Console_Window::Connect_Now );
	connect( copy, &QPushButton::clicked, this, [this]() {
		QGuiApplication::clipboard()->setText( Out->toPlainText() );
	} );
	connect( clear, &QPushButton::clicked, Out, &QPlainTextEdit::clear );
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
	Ansi_Pending.clear();
	if( Sock->state() == QAbstractSocket::ConnectedState )
		return;
	Out->appendPlainText( tr( "[AQEMU] Connecting to %1:%2 …" ).arg( Host ).arg( Port ) );
	Sock->connectToHost( Host, Port );
}

QString Serial_Console_Window::Strip_Terminal( const QByteArray &chunk )
{
	// OVMF paints its console with cursor and clear sequences. This window is a
	// log, so those codes are dropped instead of being shown as garbage.
	Ansi_Pending.append( chunk );
	QByteArray out;
	int i = 0;
	const int n = Ansi_Pending.size();
	while( i < n )
	{
		const unsigned char c = static_cast<unsigned char>( Ansi_Pending.at( i ) );
		if( c == 0x1B )
		{
			if( i + 1 >= n )
				break;
			const unsigned char next = static_cast<unsigned char>( Ansi_Pending.at( i + 1 ) );
			if( next == '[' )
			{
				int j = i + 2;
				bool done = false;
				while( j < n )
				{
					const unsigned char d = static_cast<unsigned char>( Ansi_Pending.at( j ) );
					++j;
					if( d >= 0x40 && d <= 0x7E )
					{
						done = true;
						break;
					}
					if( d < 0x20 || d > 0x3F )
					{
						done = true;
						--j;
						break;
					}
				}
				if( ! done )
					break;
				i = j;
				continue;
			}
			if( next == ']' )
			{
				int j = i + 2;
				bool done = false;
				while( j < n )
				{
					const unsigned char d = static_cast<unsigned char>( Ansi_Pending.at( j ) );
					if( d == 0x07 )
					{
						++j;
						done = true;
						break;
					}
					if( d == 0x1B && j + 1 < n && Ansi_Pending.at( j + 1 ) == '\\' )
					{
						j += 2;
						done = true;
						break;
					}
					++j;
				}
				if( ! done )
					break;
				i = j;
				continue;
			}
			i += 2;
			continue;
		}
		if( c == '\n' || c == '\t' || c >= 0x20 )
			out.append( static_cast<char>( c ) );
		++i;
	}
	Ansi_Pending.remove( 0, i );
	if( Ansi_Pending.size() > 128 )
		Ansi_Pending.clear();
	return QString::fromLatin1( out );
}

void Serial_Console_Window::On_Ready_Read()
{
	const QString text = Strip_Terminal( Sock->readAll() );
	if( text.isEmpty() )
		return;
	Out->moveCursor( QTextCursor::End );
	Out->insertPlainText( text );
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
	// Closing the window turns the monitor off. The next start does not
	// open it again until VM → Serial monitor is enabled.
	emit User_Closed();
	event->accept();
}
