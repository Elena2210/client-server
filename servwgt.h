#ifndef SERVWGT_H
#define SERVWGT_H

#include <QWidget>
#include <QLabel>
#include <QPixmap>

// Менеджер клиентских сетевых подключений
#include "hwconnmgr.h"

namespace Ui {
class ServWgt;
}

class ServWgt : public QWidget
{
    Q_OBJECT

public:
    explicit ServWgt(QWidget *parent = nullptr);
    ~ServWgt();

public slots:
    // по установке TCP соединения
//    void slotConnect();
//    void slotDisconnect();

//    // Чтение
//    void slotRecvData( QByteArray data );

    // Информация
    void Info( QString str);

    // Изменение формы
    void slotUpdateForm(QByteArray data);
    void slotChangeForm( bool rez);

private slots:
    void on_btnSendMsg_clicked();

signals:
    void signalSendData(QByteArray);

private:
    Ui::ServWgt *ui;

    HwConnMgr * mgr;

    packC::SPack pack;

    QPixmap circle_red			= QPixmap( ":/image/circle_red.png" );
    QPixmap circle_green		= QPixmap( ":/image/circle_green.png" );
    QPixmap circle_gray         = QPixmap( ":/image/circle_grey.png" );
    QPixmap circle_yellow		= QPixmap( ":/image/circle_yellow.png" );

    QVector< QLabel * > indicators_s;
};

#endif // SERVWGT_H
