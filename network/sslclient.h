#ifndef SSLCLIENT_H
#define SSLCLIENT_H

#include <QObject>

#include <QSslSocket>

///////////////////////////////////////////////////////////////////////////////
// Класс сетевого клиента для подключения к серверу TCP
class SslClient : public QObject
{
	Q_OBJECT

public:

	// Конструктор
	explicit SslClient( QString Host, ushort Port, int id, QObject * parent = 0 );

	// Деструктор
	~SslClient();

	// Состояние сокета
	QAbstractSocket::SocketState getState() const;

	// Параметры адреса
	void setHost( QString Host )
	{
		host = Host;
	}
	void setPort( ushort Port )
	{
		port = Port;
	}
	QString Host() const
	{
		return host;
	}
	ushort Port() const
	{
		return port;
	}

	bool isEncrypted() { return sock->isEncrypted(); }

	// Установка сертификата SSL
	void setCAcrt( const QList<QSslCertificate> & certificates );

	// Принятые и отправленные данные - размер
	qint64 rbps() { return _rbps; }
	qint64 tbps() { return _tbps; }

signals:

	// Сигнал при подключении к серверу
	void signalConnected();

	// Сигнал при отключении от сервера
	void signalDisconnected();

	// Сигнал при приеме клиентом порции данных
	void signalRecvData( int, QByteArray );

	// Сигнал со списком ошибок SSL
	void sslErrors( const QList<QSslError> & errors );

	// Сигнал для игнорирования ошибок SSL
	void signalIgnoreErrors();

	// Сигнал с состоянием соединения
	void signalState( int /* QAbstractSocket::SocketState */ );

	// Сигнал с ошибкой соединения
	void signalError( QString descr, int code );

public slots:

	// Слот отправки данных (принимает массив данных для отправки)
	void slotSendData( QByteArray data );

	// Слот подключения к хосту
	void slotConnect();

	// Переподключение к хосту (задается адрес хоста и порт)
	void slotReConnect( QString Host, ushort Port );

	// Слот отключения от хоста
	void slotDisconnect();

	// Запрос статуса соединения
	void slotGetState();

private slots:

	// Внутренний слот прием данных
	void slotRecvData();

	// Слот при ошибках сокета
	void slotError( QAbstractSocket::SocketError socketError );

private:

	// Сокет
	QSslSocket * sock;

	// Заголовок для приема пакета целиком, содержит размер пакета в байтах
	quint32 hdr;

	// Параметры подключения
	QString host;
	ushort port;

	// Идентификатор клиента
	int clid;

	// Размер принятых и отправленных данных
	qint64 _rbps, _tbps;
};

#endif // SSLCLIENT_H
