#include "tcpserver.h"

#include <QDataStream>
#include <QTimer>

//=============================================================================
//                                Клиент
//=============================================================================
ServClient::ServClient( QTcpSocket * socket, QObject * parent )
	: QObject( parent ), sock( socket ), hdr( 0 ),
	  data_recv( 0 ), data_send( 0 )
{
	// Формируем уникальное имя клиента: адрес и порт
	sName = QString( "%1:%2" ).arg( sock->peerAddress().toString() )
			.arg( sock->peerPort() );

	qDebug() << "New client: " << sName;

	// Сигнал на потерю связи
	connect( sock, &QTcpSocket::disconnected,
			 [this]
	{
		qDebug() << "sock" << sName << "been dc";
		emit this->signalDisconnected( sName );
	}
		   );
	/* require QMAKE_CXXFLAGS += -std=c++11 */

	// Сигнал на прием данных
	connect( sock, &QTcpSocket::readyRead,
			 this, &ServClient::slotRecvData );

	// Фиксируем время подключения
	conn_time = QDateTime::currentDateTime();
}

//=============================================================================
// Деструктор - ничего не делает
ServClient::~ServClient()
{
	/**
	 *The socket is created as a child of the server, which means that it is automatically deleted when the QTcpServer object is destroyed.
	*/

	// Сокет будет освобожден как дочерний компонент сервера

	qDebug() << "destructor ServClient" << sName;
}

///////////////////////////////////////////////////////////////////////////////
// Пытается оптимизировать сокет для уменьшения времени ожидания
void ServClient::setSocketLowDelay( bool state )
{
	sock->setSocketOption( QAbstractSocket::LowDelayOption, state );
}

//=============================================================================
// При принудительном отключении
void ServClient::slotDisconnect()
{
	sock->disconnectFromHost();
}

//=============================================================================
// Внутренний слот прием данных до целого блока
void ServClient::slotRecvData()
{
	QDataStream in( sock );
	in.setVersion( QDataStream::Qt_4_0 );

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
			data_recv += sizeof( hdr );
		}

		// Проверка достаточности данных во входном буфере для формирования пакета
		if( sock->bytesAvailable() < hdr )
		{
			//qDebug() << "read body failed: available" << sock->bytesAvailable();
			return;
		}

		// Данных достаточно - чтение пакета
		QByteArray data;
		in >> data;
		data_recv += data.size();

		// Очистка заголовка
		hdr = 0;

		// Отправка сигнала о доступности нового пакета данных
		if( !data.isEmpty() )
		{
			emit signalRecvData( data );
		}
	}
}

//=============================================================================
// Слот отправки блока данных
void ServClient::slotWriteData( QByteArray data )
{
	QByteArray block;
	QDataStream out( &block, QIODevice::WriteOnly );
	out.setVersion( QDataStream::Qt_4_0 );

	// Подготовка заголовка
	out << ( quint32 )0;

	// Подготовка пакета
	out << data;

	// Запись размера пакета
	out.device()->seek( 0 );
	out << ( quint32 )( block.size() - sizeof( quint32 ) );

	// Отправка данных
	data_send += sock->write( block );
}


//=============================================================================
//                                Сервер
//=============================================================================
TcpServer::TcpServer( QObject * parent ) :
	QTcpServer( parent ), LowDelayOption( false )
#ifdef _USE_COMPRESSION
	,timer_period( 50 ),		// отправка каждые 50 мс
	bandwidth( 2e6 ),		// 2 МБайт/с
	compressionLevel( -1 )	// по умолчанию
#endif
{

#ifdef _USE_COMPRESSION

	QTimer * timer = new QTimer( this );
	connect( timer, &QTimer::timeout, this, &TcpServer::slotSendCompressedData );
	timer->start( timer_period );

	delta = 0;
	limited_mode = false;
	ratio = 0;
	complete_ratio = 0;

	// Пороги ограничения
	threshold1 = 1000;
	threshold2 = 2;

#endif

	// При подключении клиента - нужно пошаманить
	connect( this, &TcpServer::newConnection,
			 this, &TcpServer::slotNewCliConnected );
}

///////////////////////////////////////////////////////////////////////////////
// Пытается оптимизировать сокет для уменьшения времени ожидания
void TcpServer::setSocketLowDelay( bool state )
{
	LowDelayOption = state;

	for( auto client = _clients.begin(); client != _clients.end(); ++client )
	{
		( *client )->setSocketLowDelay( state );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Подключение нового клиента
void TcpServer::slotNewCliConnected()
{
	/**
	  The socket is created as a child of the server, which means that it is automatically deleted when the QTcpServer object is destroyed. It is still a good idea to delete the object explicitly when you are done with it, to avoid wasting memory.
	 */

	// Это сокет нового клиента
	QTcpSocket * sock = nextPendingConnection();

	// Установка режима времени ожидания
	//sock->setSocketOption( QAbstractSocket::LowDelayOption, LowDelayOption );

	// a good idea is implemented here...
	// При потере связи клиентский сокет сам себя освободит
	connect( sock, &QTcpSocket::disconnected,
			 sock, &QTcpSocket::deleteLater );

	// Создаем объект нового клиента, подключившегося к серверу
	ServClient * Client = new ServClient( sock, this );

	// Добавляем нового клиента в хеш
	_clients.append( Client );

	// Настраиваем действия при приеме данных клиентом
	connect( Client, &ServClient::signalRecvData,
			 [this, Client]( QByteArray data )
	{
		emit signalClientRecvData( Client, data );
	} );

	// Настраиваем действия при потере связи клиентом
	// (отключившиеся клиенты удаляются из хеша)
	connect( Client, &ServClient::signalDisconnected,
			 [this, Client]()
	{
		// Выдаем сигнал об отключении клиента
		emit signalClientDisconnected( Client );

		// Удаляем из списка
		_clients.removeAll( Client );

		// Удаляем сам объект
		Client->deleteLater();
	} );

	// Выдаем сигнал о подключении нового клиента
	emit signalClientConnected( Client );
}

///////////////////////////////////////////////////////////////////////////////
// Слот отправки данных всем клиентам
void TcpServer::slotSendData( QByteArray data )
{
#ifndef _USE_COMPRESSION

	// Отправить данные всем клиентам
	for( auto client = _clients.begin(); client != _clients.end(); ++client )
	{
		// Выполнение отправки данных
		( *client )->slotWriteData( data );
	}

#else

	QMutexLocker m( &mutex );

	if( limited_mode )
	{
		// Анализ пакета на предмет важности передачи
		return;
	}

	QByteArray block;
	QDataStream out( &block, QIODevice::WriteOnly );
	out.setVersion( QDataStream::Qt_4_0 );

	// Подготовка заголовка
	out << ( quint32 )0;

	// Подготовка пакета
	out << data;

	// Запись размера пакета
	out.device()->seek( 0 );
	out << ( quint32 )( block.size() - sizeof( quint32 ) );

	// Сжатие данных и добавление в буфер
	buffer.append( block );

#endif // _USE_COMPRESSION
}

///////////////////////////////////////////////////////////////////////////////
// Деструктор
TcpServer::~TcpServer()
{
	qDebug() << "destructor TcpServer";

	this->blockSignals( 1 );

	// Не принимать входящие соединения
	this->close();
}

#ifdef _USE_COMPRESSION
///////////////////////////////////////////////////////////////////////////////
// Слот отправки подготовленных сжатых данных
void TcpServer::slotSendCompressedData()
{
	QMutexLocker m( &mutex );

	QByteArray c;

	if( !buffer.isEmpty() )
	{
		c = qCompress( buffer, compressionLevel );

		// Отправить данные всем клиентам
		for( auto client = _clients.begin(); client != _clients.end(); ++client )
		{
			// Выполнение отправки данных
			( *client )->slotWriteData( c );
		}

		// Коэффициент сжатия для текущих данных
		ratio = ( double )buffer.size() / ( double )c.size();

		// Коэффициент сжатия с накоплением
		const double h = 0.5;
		complete_ratio = ( 1 - h ) * ratio + h * complete_ratio;

#if USE_COMPRESSED_RATIO_OUTPUT
		static int n = 0;

		if( ( ++n ) % 10 == 0 )
		{
			qDebug() << "Orig =" << buffer.size() << "Comp = " << c.size()
					 << "Compress ratio:" << ( double )buffer.size() / ( double )c.size();
		}

#endif

	}

	// Размер данных для передачи
	int size_to_send = c.size();

	// Размер данных посылки по назначенной пропускной способности
	const int needed_size = 0.001 * timer_period * bandwidth;

	delta += size_to_send - needed_size;

	if( delta < 0 )
	{
		delta = 0;
	}

	if( delta > threshold2 * needed_size )
	{
		// перейти в режим ограничения
		limited_mode = true;
	}

	if( delta < threshold1 * needed_size )
	{
		// вернуться в обычный режим
		limited_mode = false;
	}

	buffer.clear();
}
#endif
