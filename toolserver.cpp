#include "toolserver.h"

#include "dev/serialhandler.h"

#include <QThread>
#include <QTimer>
#include <QJsonObject>

ToolServer::ToolServer(QObject *parent) : QObject(parent)
{
    protocol = Q_NULLPTR;
    serialDevice = Q_NULLPTR;

    pingTimer = new QTimer(this);
    pingTimer->setTimerType(Qt::PreciseTimer);
    connect(pingTimer, &QTimer::timeout, this, &ToolServer::ping);

    dataVault = new ProtocolData(this);
    connect(this, &ToolServer::ping, dataVault, &ProtocolData::sendPing);

    pingTimer->start(1000);
}

ToolServer::~ToolServer()
{
    emit stopSerial();
}

/**
 * @brief SUZBoard::openSerialPort
 * Создание нового потока для работы с COM-портом
 */
void ToolServer::openSerialPort(const QString &name)
{
    //порт уже открыт?
    if(protocol != Q_NULLPTR) {
        return;
    }
    protocol = new ProtocolManager();
    serialDevice = new SerialHandler(name);
    SerialHandler::SerialSettings settings;
    settings.baudRate = QSerialPort::Baud115200;
    settings.dataBits = QSerialPort::Data8;
    settings.flowControl = QSerialPort::NoFlowControl;
    settings.name = name;
    settings.parity = QSerialPort::EvenParity;
    settings.stopBits = QSerialPort::OneStop;
    serialDevice->setSettings(settings);

    QThread *serialThread = new QThread();
    serialDevice->moveToThread(serialThread);
    protocol->moveToThread(serialThread);

    connect(serialThread, &QThread::started, serialDevice, &SerialHandler::start);
    connect(serialDevice, &SerialHandler::finished, serialThread, &QThread::quit);
    connect(serialDevice, &SerialHandler::opened, this, &ToolServer::onOpenSerialPort);
    connect(serialDevice, &SerialHandler::finished, this, &ToolServer::onCloseSerialPort);
    connect(this, &ToolServer::stopSerial, serialDevice, &SerialHandler::stop);
    connect(serialDevice, &SerialHandler::finished, serialDevice, &SerialHandler::deleteLater);
    connect(serialThread, &QThread::finished, serialThread, &QThread::deleteLater);
    connect(serialDevice, &SerialHandler::dataArrived, protocol, &ProtocolManager::receiveData);
    connect(protocol, &ProtocolManager::finished, serialThread, &QThread::quit);
    connect(protocol, &ProtocolManager::finished, protocol, &ProtocolManager::deleteLater);
    connect(protocol, &ProtocolManager::transmitData, serialDevice, &SerialHandler::putData);
    connect(protocol, &ProtocolManager::sendCommStatistic, this, &ToolServer::updateConnectionInfo);

    connect(dataVault, &ProtocolData::generatePacket, protocol, &ProtocolManager::sendPacket);
    connect(protocol, &ProtocolManager::packRecieved, dataVault, &ProtocolData::packetHandler);

    serialThread->start(QThread::TimeCriticalPriority);
}

void ToolServer::closeSerialPort()
{
    emit stopSerial();
}

/**
 * Private methods
 */

void ToolServer::onOpenSerialPort()
{
    emit sendConnectionStatus(true);
}

void ToolServer::onCloseSerialPort()
{
    emit sendConnectionStatus(false);
    protocol = Q_NULLPTR;
    serialDevice = Q_NULLPTR;
}

void ToolServer::updateConnectionInfo(const ProtocolManager::CommunicationStatistic &commStat)
{
    QJsonObject info;
    info["fOnline"] = QJsonValue(commStat.fConnected);
    info["txPacks"] = QJsonValue((int)commStat.txPackets);
    info["txBytes"] = QJsonValue((int)commStat.txBytes);
    info["rxPacks"] = QJsonValue((int)commStat.rxPackets);
    info["rxBytes"] = QJsonValue((int)commStat.rxBytes);
    info["errors"] = QJsonValue((int)commStat.overallErrors);
    emit sendConnectionInfo(info);
}

