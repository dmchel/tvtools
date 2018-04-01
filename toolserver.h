#ifndef TOOLSERVER_H
#define TOOLSERVER_H

#include <QObject>
#include <QString>

#include "protocoldata.h"

class ProtocolData;
class SerialHandler;
class QTimer;
class LedRecordModel;

class ToolServer : public QObject
{
    Q_OBJECT
public:
    enum class PlayStatus {
        PLAYING,
        PAUSED,
        STARTING,
        STOPPED
    };

    explicit ToolServer(QObject *parent = nullptr);
    ~ToolServer();

    LedRecordModel *createLedData();

signals:
    void stopSerial();
    void sendConnectionStatus(bool fConnected);
    void sendConnectionInfo(const QJsonObject &info);
    void printDebug(const QString &mess);
    void printDebugData(const QByteArray &data);

    void ping();
    void mode(quint8 data);
    void playControl(quint8 data);
    void tabTask(const QByteArray &data);
    void brightness(int value);
    void demo(int num, int step);

public slots:
    void openSerialPort(const QString &name);
    void closeSerialPort();

    void playRequest();
    void pauseRequest();
    void stopRequest();
    void brightnessRequest(int value);
    void demoRequest(int num, int step);

private slots:
    void onOpenSerialPort();
    void onCloseSerialPort();
    void updateConnectionInfo(const ProtocolManager::CommunicationStatistic &commStat);
    void playHandler();

private:
    ProtocolManager *protocol;
    SerialHandler *serialDevice;
    ProtocolData *dataVault;
    LedRecordModel *ledData = Q_NULLPTR;

    QTimer *pingTimer = Q_NULLPTR;
    QTimer *playTimer = Q_NULLPTR;

    PlayStatus playState = PlayStatus::STOPPED;
};

#endif // TOOLSERVER_H
