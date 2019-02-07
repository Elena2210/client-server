#include "sslclient.h"

#include <QDataStream>

#include <QDebug>

///////////////////////////////////////////////////////////////////////////////
// Конструктор сетевого клиента
SslClient::SslClient( QString Host, ushort Port, int id, QObject * parent ) :
	QObject( parent ), hdr( 0 ), host( Host ), port( Port ), clid( id ),
	_rbps( 0 ), _tbps( 0 )
{
	if( !QSslSocket::supportsSsl() )
	{
		qDebug() << "*** Your version of Qt does support SSL ***";
	}

	qRegisterMetaType<QAbstractSocket::SocketState>( "QAbstractSocket::SocketState" );
	qRegisterMetaType<QAbstractSocket::SocketError>( "QAbstractSocket::SocketError" );

	// Создание сокета
	sock = new QSslSocket( this );

	// QSslSocket emits the encrypted() signal after the encrypted connection is established
	connect( sock, &QSslSocket::encrypted, this, &SslClient::signalConnected );

	// Сигнал при потере связи
	connect( sock, &QSslSocket::disconnected, this, &SslClient::signalDisconnected );

	// Сигнал готовности данных для чтения
	connect( sock, &QSslSocket::readyRead, this, &SslClient::slotRecvData );

	// Report any SSL errors that occur
	connect( sock, SIGNAL( sslErrors( const QList<QSslError> & ) ), this, SIGNAL( sslErrors( const QList<QSslError> & ) ) );

	// Для игнорирования ошибок SSL
	connect( this, SIGNAL( signalIgnoreErrors() ), sock, SLOT( ignoreSslErrors() ) );

	// Состояние сокета по сигналу изменения состояния
	connect( sock, &QSslSocket::stateChanged,
			 [this]( QAbstractSocket::SocketState socketState )
	{
		emit signalState( socketState );
	}
		   );

	// Сигнал об ошибке соединения
	connect( sock, SIGNAL( error( QAbstractSocket::SocketError ) ),
			 this, SLOT( slotError( QAbstractSocket::SocketError ) ) );
}

///////////////////////////////////////////////////////////////////////////////
// Деструктор сетевого клиента
SslClient::~SslClient()
{
	// Закрытие и освобождение
	if( sock->isOpen() )
	{
		sock->close();
	}

	delete sock;
}

///////////////////////////////////////////////////////////////////////////////
// Состояние сокета
QAbstractSocket::SocketState SslClient::getState() const
{
	return sock->state();
}

///////////////////////////////////////////////////////////////////////////////
// Установка сертификата SSL
void SslClient::setCAcrt( const QList<QSslCertificate> & certificates )
{
	// Добавление сертификата CA
	sock->setCaCertificates( certificates );
}

///////////////////////////////////////////////////////////////////////////////
// Слот отправки данных
void SslClient::slotSendData( QByteArray data )
{
	QByteArray block;
	QDataStream out( &block, QIODevice::WriteOnly );
	out.setVersion( QDataStream::Qt_4_0 );

	// Резерв места для заголовка
	out << ( quint32 )0;

	// Подготовка пакета
	out << qCompress( data, -1 );

	// Установка размера заголовка
	out.device()->seek( 0 );
	out << ( quint32 )( block.size() - sizeof( quint32 ) );

	// Отправка блока данных
	sock->write( block );

	_tbps += block.size();
}

///////////////////////////////////////////////////////////////////////////////
// Слот подключения к хосту
void SslClient::slotConnect()
{
	// Если сокет не подключен - подключить
	if( sock->state() == QAbstractSocket::UnconnectedState )
	{
		sock->connectToHostEncrypted( host, port );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Слот подключения к хосту
void SslClient::slotReConnect( QString Host, ushort Port )
{
	host = Host;
	port = Port;

	// Отключить сокет
	if( sock->state() != QAbstractSocket::UnconnectedState )
	{
		sock->abort();
	}

	// Если сокет отключен - начать соединение
	if( sock->state() == QAbstractSocket::UnconnectedState )
	{
		sock->connectToHostEncrypted( host, port );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Слот отключения от хоста
void SslClient::slotDisconnect()
{
	sock->close();
}

///////////////////////////////////////////////////////////////////////////////
// Запрос статуса соединения
void SslClient::slotGetState()
{
	emit signalState( sock->state() );
}

///////////////////////////////////////////////////////////////////////////////
// Прием данных
void SslClient::slotRecvData()
{
	QDataStream in( sock );
	in.setVersion( QDataStream::Qt_4_0 );

	// Цикл для приема ВСЕХ сообщений в очереди
	while( true )
	{
		// Если нужно прочитать заголовок пакета
		if( hdr == 0 )
		{
			if( sock->bytesAvailable() < sizeof( hdr ) )
			{
				//qDebug() << "Recv" << sock->bytesAvailable() << "is less than hdr size";
				return;
			}

			// Чтение заголовка пакета
			in >> hdr;
			//qDebug() << "hdr read ok:" << hdr;
			_rbps += 4;
		}

		// Проверка достаточности данных во входном буфере для формирования пакета
		if( sock->bytesAvailable() < hdr )
		{
			//qDebug() << "read body failed: available" << sock->bytesAvailable();
			return;
		}

		// Чтение пакета
		QByteArray data;
		in >> data; //= sock->read( hdr );
		_rbps += data.size();

		//qDebug() << "body read complete:" << data.size();

		// Очистка заголовка (признак готовности к принятию следующего пакета)
		hdr = 0;

		// Отправка сигнала о доступности новых данных
		if( !data.isEmpty() )
		{
			QByteArray u = qUncompress( data );

			QDataStream in2( &u, QIODevice::ReadOnly );
			in2.setVersion( QDataStream::Qt_4_0 );
			quint32 lhdr = 0;

			while( !in2.atEnd() )
			{
				in2 >> lhdr;
				QByteArray msg;
				in2 >> msg;
				emit signalRecvData( clid, msg );
				//qDebug() << "got Message" << lhdr << "and" << msg.size();
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Слот при ошибках сокета
void SslClient::slotError( QAbstractSocket::SocketError socketError )
{
	emit signalError( sock->errorString(), socketError );
}
