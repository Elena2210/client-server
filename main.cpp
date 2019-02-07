#include "servwgt.h"
#include <QApplication>
#include <QApplication>
#include <QDesktopWidget>
#include <QStyle>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ServWgt w;
    w.setWindowTitle( " TEST_SERVER " );
    w.show();

    // Расположение окна
    w.setGeometry(
            QStyle::alignedRect(
                Qt::LeftToRight,
                Qt::AlignCenter,
                w.size(),
                qApp->desktop()->availableGeometry()
            )
        );

    return a.exec();
}
