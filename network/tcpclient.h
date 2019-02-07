#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>

#include <QTcpSocket>

//-----------------------------------------------------------------------------
// Класс сетевого клиента для подключения к серверу TCP
class TcpClient : public QObject
{
	Q_OBJECT

public:

	explicit TcpClient( QString host = "", unsigned short port = 0, QObject * parent = 0 );
	~TcpClient();

	// Состояние сокета
	QAbstractSocket::SocketState getState() const;

	// Пытается оптимизировать сокет для уменьшения времени ожидания
	void setSocketLowDelay( bool state = true );

signals:

	// Сигнал при подключении к серверу
	void signalConnected();

	// Сигнал при отключении от сервера
	void signalDisconnected();

	// Сигнал при приеме клиентом порции данных
	void signalRecvData( QByteArray );

public slots:

	// Слот отправки данных (принимает массив данных для отправки)
	void slotSendData( QByteArray data );

	// Слот переподключения к хосту, разрывает текущее соединение
	void slotReconnect( QString host, ushort port );

	// Слот попытки подключения при отключенном состоянии
	void slotTryConnect();

	// Слот отключения от хоста
	void slotDisconnect();

	// Запрос на чтение заданного объема данных
	///void readData( int data_length );

private slots:

	// Внутренний слот прием данных
	void slotRecvData();

private:

	// Сокет
	QTcpSocket * sock;

	// Заголовок для приема пакета целиком, содержит размер пакета в байтах
	quint32 hdr;

	// Параметры подключения
	QString Host;
	unsigned short Port;

};

#endif // TCPCLIENT_H
