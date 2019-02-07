#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>

#include <QStringList>
#include <QHash>
#include <QDateTime>
#include <QMutexLocker>

//#define _USE_COMPRESSION

//===========================================================================
//         класс для обслуживания подключившегося к Серверу Клиента
//===========================================================================
class ServClient : public QObject
{
	Q_OBJECT

public:

	explicit ServClient( QTcpSocket * socket, QObject * parent = 0 );
	~ServClient();

	// Возвращает имя клиента в формате "address:port"
	QString getName()
	{
		return sName;
	}

	// Доступ к статистике отправленных и принятых байт
	qint64 getRecvSize() const
	{
		return data_recv;
	}
	qint64 getSendSize() const
	{
		return data_send;
	}

	// Доступ к времени подключения
	QDateTime getConnTime() const
	{
		return conn_time;
	}

	// Пытается оптимизировать сокет для уменьшения времени ожидания
	void setSocketLowDelay( bool state = true );

signals:

	// Сигнал при отключении от хоста
	void signalDisconnected( QString );

	// Сигнал при приеме порции данных
	void signalRecvData( QByteArray );

public slots:

	// Слот отключения от хоста
	void slotDisconnect();

	// Слот отправки данных
	void slotWriteData( QByteArray data );

private slots:

	// Внутренний слот прием данных
	void slotRecvData();

private:

	// Сокет
	QTcpSocket * sock;

	// Заголовок для приема пакета целиком, содержит размер пакета в байтах
	quint32 hdr;

	// Имя клиента
	QString sName;

	// Статистика соединения
	QDateTime conn_time;
	qint64 data_recv;
	qint64 data_send;

};


//===========================================================================
//                               класс Сервер
//===========================================================================
class TcpServer : public QTcpServer
{
	Q_OBJECT
public:

	explicit TcpServer( QObject * parent = 0 );
	~TcpServer();

#if 0
	// Проверка на наличие клиента в списке
	bool hasClient( QString name ) const
	{
		for( auto client = _clients.begin(); client != _clients.end(); ++client )
		{
			if( ( *client )->getName() == name )
			{
				return true;
			}
		}

		return false;
	}
#endif

	// Пытается оптимизировать сокет для уменьшения времени ожидания
	void setSocketLowDelay( bool state = true );

signals:

	// Сигнал при подключении клиента
	void signalClientConnected( ServClient * );

	// Сигнал при отключении клиента
	void signalClientDisconnected( ServClient * );

	// Сигнал при приеме от клиента пакета данных
	void signalClientRecvData( ServClient *, QByteArray );

public slots:

	// Слот отправки данных всем клиентам
	void slotSendData( QByteArray data );

private slots:

	// Внутренний слот при подключении нового клиента
	void slotNewCliConnected();

#ifdef _USE_COMPRESSION
private slots:

	// Слот отправки подготовленных сжатых данных
	void slotSendCompressedData();
#endif

private:

	// Список клиентов
	QList< ServClient * > _clients;

	bool LowDelayOption;

#ifdef _USE_COMPRESSION

	// Буфер данных для отправки
	QByteArray buffer;

	// Средство синхронизации буфера
	QMutex mutex;

	// Превышение над назначенной пропускной способностью
	int delta;

	// Флаг ограничения режима передачи
	bool limited_mode;

	// Пороги ограничения
	double threshold1, threshold2;

	// Коэффициент сжатия данных
	double ratio, complete_ratio;

	// Период отправки данных, мс
	const int timer_period;

	// Назначенная полоса пропускания канала, Мбайт/с
	const int bandwidth;

	// Степень сжатия данных
	const int compressionLevel;

#endif

};

#endif // TCPSERVER_H

