#include "brukerScan.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow win;

    const QIcon *cabinetIcon = new QIcon(":/My-Icons/fileCabinet.png");
    app.setWindowIcon(*cabinetIcon);

    return app.exec();
}
