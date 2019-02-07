#include "hwconnmgr.h"

#include <QThread>

//=================================================================================
// Конструктор системы управления подключениями к серверу железа
HwConnMgr::HwConnMgr( ushort port, QObject * parent ) :
	QObject( parent )
{
	TcpServer * srv = new TcpServer( this );

	connect( srv, &TcpServer::signalClientConnected,
			 this, &HwConnMgr::slotClientConnected );

	connect( srv, &TcpServer::signalClientDisconnected,
			 this, &HwConnMgr::slotClientDisconnected );

	connect( srv, &TcpServer::signalClientRecvData,
			 this, &HwConnMgr::slotClientRecvData );

	connect( this, &HwConnMgr::signalClientSendData,
			 srv, &TcpServer::slotSendData );

	// Прием подключений только от localhost
	if( !srv->listen( QHostAddress::AnyIPv4, port ) )
	{
        bOk = false;
        emit signalInfo("Ошибка... not listening");
	}

    emit signalInfo("Оk. Ожидание входящего подключения...");
    bOk = true;
}

//=================================================================================
// При подключении клиента
void HwConnMgr::slotClientConnected( ServClient * client )
{
    emit signalInfo("Подключен клиент " + client->getName());
    emit signalVisible(true);
}
//=================================================================================
// При отключении клиента
void HwConnMgr::slotClientDisconnected( ServClient * client )
{
    emit signalInfo("Отключен клиент " + client->getName());
    emit signalVisible(false);
}
//=================================================================================
// При получении данных
void HwConnMgr::slotClientRecvData( ServClient * client, QByteArray data )
{
    emit signalVisible(true);
    // Анализ заголовка пакета
    auto msg = reinterpret_cast< const BaseMsg<> * >( data.data() );

    if( msg->hdr == MTypeCli)
    {
        emit signalInfo("Приняты данные от Cli."+client->getName());
        if( msg->magic == cli::MagicJobTest )
        {
//            auto c_msg = reinterpret_cast< const cli::MsgJobTest * >( data.data() );
            emit signalUpdateForm( data );
        }
    }
}
//=================================================================================
