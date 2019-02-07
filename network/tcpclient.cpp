#include "tcpclient.h"

#include <QDataStream>

#include <QTimer>

#include <QDebug>

// Использовать ли сжатие данных
//#define _USE_COMPRESS

///////////////////////////////////////////////////////////////////////////////
// Конструктор сетевого клиента
TcpClient::TcpClient( QString host, unsigned short port, QObject * parent ) :
	QObject( parent ), hdr( 0 ), Host( host ), Port( port )
{
	// Создание сокета
	sock = new QTcpSocket( this );

	// Сигнал при подключении к серверу
	connect( sock, &QTcpSocket::connected, this, &TcpClient::signalConnected );

	// Сигнал при потере связи
	connect( sock, &QTcpSocket::disconnected, this, &TcpClient::signalDisconnected );

	// Сигнал готовности данных для чтения
	connect( sock, &QTcpSocket::readyRead, this, &TcpClient::slotRecvData );

	QTimer * timer = new QTimer( this );
	connect( timer, &QTimer::timeout, this, &TcpClient::slotTryConnect );
	timer->start( 1000 );

}

///////////////////////////////////////////////////////////////////////////////
// Деструктор сетевого клиента
TcpClient::~TcpClient()
{
	// Закрытие и освобождение
	sock->close();
	delete sock;
}

///////////////////////////////////////////////////////////////////////////////
// Состояние сокета
QAbstractSocket::SocketState TcpClient::getState() const
{
	return sock->state();
}

///////////////////////////////////////////////////////////////////////////////
// Пытается оптимизировать сокет для уменьшения времени ожидания
void TcpClient::setSocketLowDelay( bool state )
{
	sock->setSocketOption( QAbstractSocket::LowDelayOption, state );
}

///////////////////////////////////////////////////////////////////////////////
// Слот отправки данных
void TcpClient::slotSendData( QByteArray data )
{
	if( sock->state() != QAbstractSocket::ConnectedState )
	{
		return;
	}

#ifndef _USE_COMPRESS

	QByteArray block;
	QDataStream out( &block, QIODevice::WriteOnly );
	out.setVersion( QDataStream::Qt_4_0 );

	// Резерв места для заголовка
	out << ( quint32 )0;

	// Подготовка пакета
	out << data;

	// Установка размера заголовка
	out.device()->seek( 0 );
	out << ( quint32 )( block.size() - sizeof( quint32 ) );

	// Отправка блока данных
	sock->write( block );

#else

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

#endif // _USE_COMPRESS

}

///////////////////////////////////////////////////////////////////////////////
// Слот переподключения к хосту
void TcpClient::slotReconnect( QString host, ushort port )
{
	// Если подключен - разорвать соединение
	if( sock->state() == QAbstractSocket::ConnectedState )
	{
		// Честный синхронный вариант
		//sock->disconnectFromHost();
		//sock->waitForDisconnected();

	}

	Host = host;
	Port = port;

	// Разрыв текущего соединения и сброс сокета
	sock->abort();

	// Подключиться к хосту
	sock->connectToHost( host, port );

}

///////////////////////////////////////////////////////////////////////////////
// Слот попытки подключения при отключенном состоянии
void TcpClient::slotTryConnect()
{
	if( sock->state() == QAbstractSocket::UnconnectedState )
	{
		sock->connectToHost( Host, Port );
		//sock->waitForConnected();
	}
}

///////////////////////////////////////////////////////////////////////////////
// Слот отключения от хоста
void TcpClient::slotDisconnect()
{
	sock->close();
}

///////////////////////////////////////////////////////////////////////////////
// Прием данных
void TcpClient::slotRecvData()
{
#ifndef _USE_COMPRESS

	QDataStream in( sock );
	in.setVersion( QDataStream::Qt_4_0 );

	while( true )
	{
		// Если нужно прочитать заголовок пакета
		if( hdr == 0 )
		{
			if( sock->bytesAvailable() < sizeof( hdr ) )
			{
				return;
			}

			// Чтение заголовка пакета
			in >> hdr;
			//qDebug() << "hdr read ok:" << hdr;
		}

		// Проверка достаточности данных во входном буфере для формирования пакета
		if( sock->bytesAvailable() < hdr )
		{
			//qDebug() << "read body failed: available" << sock->bytesAvailable()
			//		 << "required" << hdr;
			return;
		}

		// Чтение пакета
		QByteArray data;
		in >> data; //= sock->read( hdr );

		//qDebug() << "body read complete:" << data.size();

		// Очистка заголовка (признак готовности к принятию следующего пакета)
		hdr = 0;

		// Отправка сигнала о доступности новых данных
		if( !data.isEmpty() )
		{
			emit signalRecvData( data );
		}
	}

#else

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
				emit signalRecvData( msg );
				//qDebug() << "got Message" << lhdr << "and" << msg.size();
			}
		}
	}

#endif // _USE_COMPRESS

}
