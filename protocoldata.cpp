#include "protocoldata.h"

#include <QTimer>

ProtocolData::ProtocolData(QObject *parent) : QObject(parent)
{
    pingTimer = new QTimer(this);
    pingTimer->setTimerType(Qt::PreciseTimer);
    pingTimer->setInterval(1000);
    connect(pingTimer, &QTimer::timeout, this, &ProtocolData::sendPing);
    pingTimer->start();
}

void ProtocolData::packetHandler(const SerialPacket &pack)
{
    if(pack.isValid()) {
        fValidPackArrived = true;
    }
}

void ProtocolData::sendPing()
{
    SerialPacket pingPack;
    pingPack.address = 0x2;
    pingPack.command = 0x1;
    pingPack.length = 0;
    emit generatePacket(pingPack);
}
