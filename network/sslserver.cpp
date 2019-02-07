#include "sslserver.h"

#include <QDataStream>
#include <QSslKey>
#include <QFile>
#include <QTimer>

//=============================================================================
//                                Клиент
//=============================================================================
SslClient::SslClient( QObject * parent )
	: QSslSocket( parent ), hdr( 0 ),
	  _time( QDateTime::currentMSecsSinceEpoch() ),
	  _type( EUnit::undefined )
{
	static int id = 0;
	sName = "Undefined_" + QString::number( ++id );

	// Сигнал на прием данных
	connect( this, &SslClient::readyRead,
			 this, &SslClient::slotRecvData );

	// Сигнал на установку соединения
	connect( this, &SslClient::encrypted,
			 this, &SslClient::slotEncrypted );
}

///////////////////////////////////////////////////////////////////////////////
// Деструктор
SslClient::~SslClient()
{
	qDebug() << "Destruct sslClient" << sName;
}

///////////////////////////////////////////////////////////////////////////////
// При установке соединения
void SslClient::slotEncrypted()
{
	// QSslSocket enters encrypted mode
	sName = peerAddress().toString() + QString( ":%1" ).arg( peerPort() );
}

///////////////////////////////////////////////////////////////////////////////
// Внутренний слот прием данных до целого блока
void SslClient::slotRecvData()
{
	QDataStream in( this );
	in.setVersion( QDataStream::Qt_4_0 );

	while( true )
	{
		// Если нужно прочитать заголовок пакета
		if( hdr == 0 )
		{
			if( bytesAvailable() < sizeof( hdr ) )
			{
				//qDebug() << "Recv" << sock->bytesAvailable() << "is less than hdr size";
				return;
			}

			// Чтение заголовка пакета
			in >> hdr;
		}

		// Проверка достаточности данных во входном буфере для формирования пакета
		if( bytesAvailable() < hdr )
		{
			//qDebug() << "read body failed: available" << sock->bytesAvailable();
			return;
		}

		// Данных достаточно - чтение пакета
		QByteArray data;
		in >> data;

		// Очистка заголовка
		hdr = 0;

		// Отправка сигнала о доступности нового пакета данных
		if( !data.isEmpty() )
		{
			// Принятые данные восстанавливаются из сжатой версии
			emit signalRecvData( qUncompress( data ) );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Слот отправки блока данных
void SslClient::slotWriteData( QByteArray data )
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
	write( block );
}





//=============================================================================
//                                Сервер
//=============================================================================
SslServer::SslServer( QObject * parent, int period, int level, int bw ) :
	QTcpServer( parent ),
	m_sslLocalCertificate(),
	m_sslPrivateKey(),
	m_sslProtocol( QSsl::UnknownProtocol ),
	timer_period( period ),
	bandwidth( bw ),
	compressionLevel( level )
{
	QTimer * timer = new QTimer( this );
	connect( timer, &QTimer::timeout, this, &SslServer::slotSendCompressedData );
	timer->start( timer_period );

	delta = 0;
	limited_mode = false;
	ratio = 0;
	complete_ratio = 0;

	// Пороги ограничения
	threshold1 = 1000;
	threshold2 = 2;
}

//=============================================================================
// При наличии подключения нового клиента
void SslServer::incomingConnection( qintptr socketDescriptor )
{
	// Создаем объект нового клиента, подключившегося к серверу
	SslClient * socket = new SslClient( this );

	if( socket->setSocketDescriptor( socketDescriptor ) )
	{
		socket->setLocalCertificate( m_sslLocalCertificate );
		socket->setPrivateKey( m_sslPrivateKey );
		socket->setProtocol( m_sslProtocol );

		//socket->setPeerVerifyMode( QSslSocket::VerifyNone );
		//addPendingConnection( socket );
		socket->startServerEncryption();

		if( socket->waitForEncrypted( 1000 ) )
		{
			// Настраиваем действия при приеме данных клиентом
			connect( socket, &SslClient::signalRecvData,
					 [this, socket]( QByteArray data )
			{
				emit signalClientRecvData( socket, data );
			} );

			// Настраиваем действия при потере связи клиентом
			// (отключившиеся клиенты удаляются из хеша)
			connect( socket, &QSslSocket::disconnected,
					 [this, socket]()
			{
				// Выдаем сигнал об отключении клиента
				emit signalClientDisconnected( socket );

				// Удаляем из списка
				_clients.removeAll( socket );

				// Удаляем сам объект
				socket->deleteLater();
			} );

			// Сигнал со списком ошибок SSL
			void ( QSslSocket::* SslErrors )( const QList<QSslError> & ) = &QSslSocket::sslErrors;
			connect( socket, SslErrors, [this, socket]( const QList<QSslError> & errors )
			{
				emit sslErrors( socket, errors );
			} );

			// Добавляем нового клиента в список
			_clients.append( socket );

			// Выдаем сигнал о подключении нового клиента
			emit signalClientConnected( socket );
		}
		else
		{
			delete socket;
		}
	}
	else
	{
		delete socket;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Деструктор сервера
SslServer::~SslServer()
{
	qDebug() << "SslServer destroyed";

	blockSignals( 1 );

	// Не принимать входящие соединения
	close();
}

///////////////////////////////////////////////////////////////////////////////
// Доступ к локальному сертификату
const QSslCertificate & SslServer::getSslLocalCertificate() const
{
	return m_sslLocalCertificate;
}

///////////////////////////////////////////////////////////////////////////////
// Доступ к приватному ключу
const QSslKey & SslServer::getSslPrivateKey() const
{
	return m_sslPrivateKey;
}

///////////////////////////////////////////////////////////////////////////////
// Доступ к протоколу
QSsl::SslProtocol SslServer::getSslProtocol() const
{
	return m_sslProtocol;
}

///////////////////////////////////////////////////////////////////////////////
// Установка локального сертификата
void SslServer::setSslLocalCertificate( const QSslCertificate & certificate )
{
	m_sslLocalCertificate = certificate;
}

///////////////////////////////////////////////////////////////////////////////
// Загрузка локального сертификата из файла
bool SslServer::setSslLocalCertificate( const QString & path, QSsl::EncodingFormat format )
{
	QFile certificateFile( path );

	if( !certificateFile.open( QIODevice::ReadOnly ) )
	{
		return false;
	}

	m_sslLocalCertificate = QSslCertificate( certificateFile.readAll(), format );
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Установка приватного ключа
void SslServer::setSslPrivateKey( const QSslKey & key )
{
	m_sslPrivateKey = key;
}

///////////////////////////////////////////////////////////////////////////////
// Загрузка приватного ключа из файла
bool SslServer::setSslPrivateKey( const QString & fileName, QSsl::KeyAlgorithm algorithm, QSsl::EncodingFormat format, const QByteArray & passPhrase )
{
	QFile keyFile( fileName );

	if( !keyFile.open( QIODevice::ReadOnly ) )
	{
		return false;
	}

	m_sslPrivateKey = QSslKey( keyFile.readAll(), algorithm, format, QSsl::PrivateKey, passPhrase );
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Установка типа протокола
void SslServer::setSslProtocol( QSsl::SslProtocol protocol )
{
	m_sslProtocol = protocol;
}

///////////////////////////////////////////////////////////////////////////////
// Слот приема данных для отправки всем клиентам
void SslServer::slotSendData( QByteArray data )
{
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
}

///////////////////////////////////////////////////////////////////////////////
// Слот отправки подготовленных сжатых данных
void SslServer::slotSendCompressedData()
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
