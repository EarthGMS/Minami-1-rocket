#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("RocketMonitoringSystem");
    QCoreApplication::setApplicationName("RocketMonitoringSystem");
    QApplication::setStyle("Fusion");
    MainWindow window;
    window.show();
    return app.exec();
}
