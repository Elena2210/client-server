#include "servwgt.h"
#include "ui_servwgt.h"

#include "headers/server_cdgrms.h"
#include "headers/client_cdgrms.h"

ServWgt::ServWgt(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ServWgt)
{
    ui->setupUi(this);
    ui->Info->setText( "Приложение запущено." );

    memset( &pack, 0, sizeof( pack ) );

    ui->frame->setVisible(false);

    indicators_s.append(ui->indicator_1);
    indicators_s.append(ui->indicator_2);
    indicators_s.append(ui->indicator_3);
    indicators_s.append(ui->indicator_4);

    for(int i = 0; i < 4; i++)
    {
        indicators_s[i]->setPixmap(circle_gray);
//        indicators_s[i]->setScaledContents(true);
        indicators_s[i]->setAutoFillBackground(true);
    }

//========================== СВЯЗЬ ===========================

    // Порт сервера железа
    ushort hws_port = 33059;

    // Хост
    QString host = "localhost";

    // Менеджер подключений (по всем host)
    mgr = new HwConnMgr( hws_port );

    connect(mgr,&HwConnMgr::signalInfo,
            this, &ServWgt::Info);

    connect( this, &ServWgt::signalSendData,
             mgr, &HwConnMgr::signalClientSendData );

    connect( mgr, &HwConnMgr::signalUpdateForm,
             this, &ServWgt::slotUpdateForm );

    connect( mgr, &HwConnMgr::signalVisible,
             this, &ServWgt::slotChangeForm );


    if( !mgr->ok() )
    {
//		return -1;
    }

/*
    ui->Info_textEdit->append(QString("Host: %1, Port: %2").arg(host).arg(QString::number(port)));
    TcpClient * cli = new TcpClient( host, port, this );

*/
}
//==========================================================================================================
ServWgt::~ServWgt()
{
    delete ui;
}
//==========================================================================================================
void ServWgt::Info(QString str)
{
    ui->Info->append( str );
}

//==========================================================================================================
void ServWgt::slotUpdateForm(QByteArray data)
{
    auto c_msg = reinterpret_cast< const cli::MsgJobTest * >( data.data() );
    pack = c_msg->param;

    ui->num_double->setValue(pack.num_double);

    ui->int1->setValue(pack.num_int[0]);
    ui->int2->setValue(pack.num_int[1]);
    ui->int3->setValue(pack.num_int[2]);

    for(int i = 0; i < 4; i++)
    {
        if (i == pack.mode)
        {
            indicators_s[i]->setPixmap(circle_green);
        }
        else
        {
            indicators_s[i]->setPixmap(circle_yellow);
        }
    }
}
//==========================================================================================================
// Состояние видимости формы отправки
void ServWgt::slotChangeForm(bool rez)
{
    ui->frame->setVisible(rez);
}
//==========================================================================================================
void ServWgt::on_btnSendMsg_clicked()
{
    pack::SPackParam msg;
//    msg.description = QString("Отправка от сервера");
    msg.num_double = ui->num_double->value();
    msg.num_int[0] = ui->int1->value();
    msg.num_int[1] = ui->int2->value();
    msg.num_int[2] = ui->int3->value();
    msg.mode = 2;

    serv::MsgTest msgT ( msg );
    emit signalSendData( toQByteArray( msgT ) );

    Info("Сообщение отправлено");
}
//==========================================================================================================
