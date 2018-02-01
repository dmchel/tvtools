#include "mainwindow.h"
#include <QApplication>

#include "toolserver.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    ToolServer tool;

    QObject::connect(&w, &MainWindow::requestConnection, &tool, &ToolServer::openSerialPort);
    QObject::connect(&w, &MainWindow::dropConnection, &tool, &ToolServer::closeSerialPort);
    QObject::connect(&tool, &ToolServer::sendConnectionStatus, &w, &MainWindow::setConnectionStatus);
    QObject::connect(&tool, &ToolServer::sendConnectionInfo, &w, &MainWindow::setConnectionInfo);

    w.show();

    return a.exec();
}
