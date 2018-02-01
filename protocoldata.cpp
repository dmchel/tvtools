#include "protocoldata.h"

#include <QTimer>

ProtocolData::ProtocolData(QObject *parent) : QObject(parent)
{

}

void ProtocolData::packetHandler(const SerialPacket &pack)
{
    if(pack.isValid()) {
        fValidPackArrived = true;
        //here we take care off some packs

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
