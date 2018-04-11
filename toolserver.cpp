#include "toolserver.h"
#include "tabtask.h"

#include "dev/serialhandler.h"
#include "recordModel/ledrecordmodel.h"

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

    playTimer = new QTimer(this);
    playTimer->setTimerType(Qt::PreciseTimer);
    playTimer->setSingleShot(true);

    connect(playTimer, &QTimer::timeout, this, &ToolServer::playHandler);

    dataVault = new ProtocolData(this);
    connect(this, &ToolServer::ping, dataVault, &ProtocolData::sendPing);
    connect(this, &ToolServer::mode, dataVault, &ProtocolData::sendMode);
    connect(this, &ToolServer::brightness, dataVault, &ProtocolData::sendBrightness);
    connect(this, &ToolServer::demo, dataVault, &ProtocolData::sendDemo);
    connect(this, &ToolServer::playControl, dataVault, &ProtocolData::sendPlayControl);
    connect(this, &ToolServer::tabTask, dataVault, &ProtocolData::sendTabTask);
    connect(dataVault, &ProtocolData::printDebug, this, &ToolServer::printDebug);
    connect(dataVault, &ProtocolData::printDebugData, this, &ToolServer::printDebugData);

    pingTimer->start(1000);
}

ToolServer::~ToolServer()
{
    emit stopSerial();
}

LedRecordModel *ToolServer::createLedData()
{
    if(ledData) {
        delete ledData;
    }
    ledData = new LedRecordModel(this);
    return ledData;
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

void ToolServer::playRequest()
{
    if(playState != ToolServer::PlayStatus::PLAYING) {
        //set mode to debug
        emit mode(ProtocolData::TV_DEBUG);
        playTimer->start(100);
        playState = ToolServer::PlayStatus::STARTING;
    }
}

void ToolServer::pauseRequest()
{
    emit playControl(0x02);
    playState = ToolServer::PlayStatus::PAUSED;
}

void ToolServer::stopRequest()
{
    emit playControl(0x04);
    playState = ToolServer::PlayStatus::STOPPED;
}

void ToolServer::brightnessRequest(int value)
{
    emit brightness(value);
}

void ToolServer::demoRequest(int num, int step)
{
    if(playState != ToolServer::PlayStatus::PLAYING) {
        emit mode(ProtocolData::TV_DEMO);
        emit demo(num, step);
    }
}

/**
 * Private methods
 */

void ToolServer::onOpenSerialPort()
{
    emit sendConnectionStatus(true);
    emit printDebug("Serial port opened.");
}

void ToolServer::onCloseSerialPort()
{
    emit sendConnectionStatus(false);
    protocol = Q_NULLPTR;
    serialDevice = Q_NULLPTR;
    emit printDebug("Serial port closed.");
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

void ToolServer::playHandler()
{
    switch(playState) {
    case ToolServer::PlayStatus::PAUSED:
        break;
    case ToolServer::PlayStatus::PLAYING:
        break;
    case ToolServer::PlayStatus::STARTING:
        if(ledData) {
            QByteArray data;
            QList<LedRecordItem> ledRecords = ledData->readAllData();
            for(auto record : ledRecords) {
                TabTask task = TabTask(record.timestamp, record.duration, record.ledColor, record.ledData);
                quint32 ts = task.timestamp;
                quint16 dur = task.duration;
                data.append(ts & 0xFF);
                data.append((ts >> 8) & 0xFF);
                data.append((ts >> 16) & 0xFF);
                data.append(dur & 0xFF);
                data.append((dur >> 8) & 0xFF);
                for(int i = 0; i < TAB_TASK_MAX_SIZE; i++) {
                    if(task.data[i].fretNum == 0xFF) {
                        break;
                    }
                    data.append(task.data[i].fretNum);
                    quint16 ld = 0;
                    for(int j = 0; j < 6; j++) {
                        ld |= ((task.data[i].leds[j] & 0x3) << (j * 2));
                    }
                    data.append(ld & 0xFF);
                    data.append((ld >> 8) & 0xFF);
                }
            }
            //send tab data
            emit tabTask(data);
            //start playing
            emit playControl(0x01);
        }
        playState = ToolServer::PlayStatus::PLAYING;
        break;
    case ToolServer::PlayStatus::STOPPED:
        break;
    }
}

