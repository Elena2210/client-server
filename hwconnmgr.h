#ifndef HWCONNMGR_H
#define HWCONNMGR_H

#include <QObject>

#include "headers/server_cdgrms.h"
#include "headers/client_cdgrms.h"

// TCP
#include "network/tcpserver.h"
#include "network/tcpclient.h"

//====================================================================================
class HwConnMgr : public QObject
{
	Q_OBJECT
public:

	explicit HwConnMgr( ushort port, QObject * parent = 0 );

	// Состояние сервера
	bool ok()
	{
		return bOk;
	}

signals:

	// Отправка данных для клиента
	void signalClientSendData( QByteArray );

    void signalInfo( QString );

    void signalUpdateForm( QByteArray );

    void signalVisible( bool );

public slots:

	// При подключении клиента
	void slotClientConnected( ServClient * client );

	// При отключении клиента
	void slotClientDisconnected( ServClient * client );

	// При получении данных
	void slotClientRecvData( ServClient *, QByteArray );

private:

	bool bOk;
};

#endif // HWCONNMGR_H
