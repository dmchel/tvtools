#ifndef TOOLSERVER_H
#define TOOLSERVER_H

#include <QObject>
#include <QString>

#include "protocoldata.h"

class ProtocolData;
class SerialHandler;
class QTimer;

class ToolServer : public QObject
{
    Q_OBJECT
public:
    explicit ToolServer(QObject *parent = nullptr);
    ~ToolServer();

signals:
    void stopSerial();
    void sendConnectionStatus(bool fConnected);
    void sendConnectionInfo(const QJsonObject &info);

    void ping();

public slots:
    void openSerialPort(const QString &name);
    void closeSerialPort();

private slots:
    void onOpenSerialPort();
    void onCloseSerialPort();
    void updateConnectionInfo(const ProtocolManager::CommunicationStatistic &commStat);

private:
    ProtocolManager *protocol;
    SerialHandler *serialDevice;
    ProtocolData *dataVault;

    QTimer *pingTimer = Q_NULLPTR;
};

#endif // TOOLSERVER_H
