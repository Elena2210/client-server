#ifndef SSLSERVER_H
#define SSLSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QSslSocket>

#include <QStringList>
#include <QHash>

#include <QSsl>
#include <QSslCertificate>
#include <QSslKey>
#include <QMutexLocker>

#include "pcktypes.h"

//===========================================================================
//         класс для обслуживания подключившегося к Серверу Клиента
//===========================================================================
class SslClient : public QSslSocket
{
	Q_OBJECT

public:

	explicit SslClient( QObject * parent = 0 );
	~SslClient();

	// Возвращает имя клиента в формате "address:port"
	QString name()
	{
		return sName;
	}

	// Тип клиента
	EUnit & type() { return _type; }

	// Время подключения
	qint64 time() { return _time; }

signals:

	// Сигнал при приеме порции данных
	void signalRecvData( QByteArray );

public slots:

	// Слот отправки данных
	void slotWriteData( QByteArray data );

private slots:

	// Внутренний слот прием данных
	void slotRecvData();

	void slotEncrypted();

private:

	// Заголовок для приема пакета целиком, содержит размер пакета в байтах
	quint32 hdr;

	// Имя клиента
	QString sName;

	// Время подключения, мс
	qint64 _time;

	// Тип клиента
	EUnit _type;
};


//===========================================================================
//                               класс Сервер
//===========================================================================
class SslServer : public QTcpServer
{
	Q_OBJECT
public:

	// Конструктор
	explicit SslServer( QObject * parent = 0,
						int period = 50, // период отправки данных, мс
						int level = -1,  // уровень сжатия данных
						int bw = 225000  // назначенная пропускная способность
					  );
	~SslServer();

	// Доступ к сертификату, приватному ключу и типу протокола
	const QSslCertificate & getSslLocalCertificate() const;
	const QSslKey & getSslPrivateKey() const;
	QSsl::SslProtocol getSslProtocol() const;

	// Установка сертификата
	void setSslLocalCertificate( const QSslCertificate & certificate );
	bool setSslLocalCertificate( const QString & path, QSsl::EncodingFormat format = QSsl::Pem );

	// Установка закрытого ключа
	void setSslPrivateKey( const QSslKey & key );
	bool setSslPrivateKey( const QString & fileName, QSsl::KeyAlgorithm algorithm = QSsl::Rsa, QSsl::EncodingFormat format = QSsl::Pem, const QByteArray & passPhrase = QByteArray() );

	// Установка протокола
	void setSslProtocol( QSsl::SslProtocol protocol );

	// Получение коэффициента сжатия данных
	double getCurrRatio()
	{
		return ratio;
	}
	double getTotalRatio()
	{
		return complete_ratio;
	}

	// Установка порогов ограничения
	void setThresholds( double th1, double th2 )
	{
		threshold1 = th1;
		threshold2 = th2;
	}

	// Список подключенных клиентов
	QList< SslClient * > & clients() { return _clients; }

protected:

	// При наличии подключения нового клиента
	void incomingConnection( qintptr socketDescriptor ) override final;

signals:

	// Сигнал при подключении клиента
	void signalClientConnected( SslClient * cli );

	// Сигнал при отключении клиента
	void signalClientDisconnected( SslClient * cli );

	// Сигнал при приеме от клиента пакета данных
	void signalClientRecvData( SslClient *, QByteArray );

	// Сигнал со списком ошибок SSL
	void sslErrors( SslClient * cli, const QList<QSslError> & errors );

public slots:

	// Слот приема данных для отправки
	void slotSendData( QByteArray data );

private slots:

	// Слот отправки подготовленных сжатых данных
	void slotSendCompressedData();

private:

	// Список клиентов
	QList< SslClient * > _clients;

	// Сертификат
	QSslCertificate m_sslLocalCertificate;

	// Закрытый ключ
	QSslKey m_sslPrivateKey;

	// Протокол соединения
	QSsl::SslProtocol m_sslProtocol;

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
};

#endif // SSLSERVER_H

