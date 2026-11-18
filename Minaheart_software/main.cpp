#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Rocket Monitoring System");
    QApplication::setOrganizationName("RMS");
    MainWindow window;
    window.show();
    return app.exec();
}
