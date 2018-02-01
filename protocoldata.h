#ifndef PROTOCOLDATA_H
#define PROTOCOLDATA_H

#include <QObject>

#include "protocolmanager.h"

class QTimer;

class ProtocolData : public QObject
{
    Q_OBJECT
public:
    explicit ProtocolData(QObject *parent = nullptr);

signals:
    void generatePacket(const SerialPacket &pack);

public slots:
    void packetHandler(const SerialPacket &pack);

private slots:
    void sendPing();

private:
    QTimer *pingTimer;

    bool fValidPackArrived = false;
};

#endif // PROTOCOLDATA_H
