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
    QObject::connect(&w, &MainWindow::play, &tool, &ToolServer::playRequest);
    QObject::connect(&w, &MainWindow::stop, &tool, &ToolServer::stopRequest);
    QObject::connect(&w, &MainWindow::pause, &tool, &ToolServer::pauseRequest);
    QObject::connect(&w, &MainWindow::demo, &tool, &ToolServer::demoRequest);
    QObject::connect(&w, &MainWindow::brightness, &tool, &ToolServer::brightnessRequest);

    w.setLedModel(tool.createLedData());
    w.show();

    return a.exec();
}
