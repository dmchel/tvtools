#include "protocoldata.h"

#include <QTimer>

ProtocolData::ProtocolData(QObject *parent) : QObject(parent)
{

}

quint8 ProtocolData::tabVisionMode() const
{
    return mode;
}

void ProtocolData::packetHandler(const SerialPacket &pack)
{
    if(pack.isValid()) {
        fValidPackArrived = true;
        //here we take care off some packs
        if(pack.address == (quint8)ProtocolData::AbonentAddress::ADDR_HOST) {
            switch (pack.command) {
            case (quint8) ProtocolData::DebugResponce::RESP_PING:
                if(pack.length == 1) {
                    mode = pack.data.at(0);
                }
                break;
            case (quint8) ProtocolData::DebugResponce::RESP_CONF:
                break;
            case (quint8) ProtocolData::DebugResponce::RESP_INFO:
                break;
            case (quint8) ProtocolData::DebugResponce::RESP_ERROR:
                if(pack.length == 2) {

                }
                break;
            case (quint8) ProtocolData::DebugResponce::RESP_CONFIRM:
                if(pack.length == 2) {

                }
                break;
            default:
                break;
            }
        }
    }
}

void ProtocolData::sendPing()
{
    SerialPacket pingPack;
    pingPack.address = (quint8) ProtocolData::AbonentAddress::ADDR_TAB_VISION;
    pingPack.command = (quint8) ProtocolData::DebugCommands::CMD_PING;
    pingPack.length = 0;
    emit generatePacket(pingPack);
}

void ProtocolData::sendMode(quint8 data)
{
    SerialPacket modePack;
    modePack.address = (quint8) ProtocolData::AbonentAddress::ADDR_TAB_VISION;
    modePack.command = (quint8) ProtocolData::DebugCommands::CMD_MODE;
    modePack.length = 1;
    modePack.data[0] = data;
    emit generatePacket(modePack);
}

void ProtocolData::sendPlayControl(quint8 data)
{
    SerialPacket pack;
    pack.address = (quint8) ProtocolData::AbonentAddress::ADDR_TAB_VISION;
    pack.command = (quint8) ProtocolData::DebugCommands::CMD_PLAY_CONTROL;
    pack.length = 1;
    pack.data[0] = data;
    emit generatePacket(pack);
}

void ProtocolData::sendTabTask(const QByteArray &data)
{
    SerialPacket pack;
    pack.address = (quint8) ProtocolData::AbonentAddress::ADDR_TAB_VISION;
    pack.command = (quint8) ProtocolData::DebugCommands::CMD_LED_CONTROL;
    pack.length = data.size();
    pack.data = data;
    emit generatePacket(pack);
}

void ProtocolData::sendBrightness(int value)
{
    SerialPacket pack;
    pack.address = (quint8) ProtocolData::AbonentAddress::ADDR_TAB_VISION;
    pack.command = (quint8) ProtocolData::DebugCommands::CMD_LED_BRIGHTNESS;
    pack.length = 1;
    pack.data[0] = value;
    emit generatePacket(pack);
}

void ProtocolData::sendDemo(int num, int step)
{
    SerialPacket pack;
    pack.address = (quint8) ProtocolData::AbonentAddress::ADDR_TAB_VISION;
    pack.command = (quint8) ProtocolData::DebugCommands::CMD_DEMO;
    pack.length = 3;
    pack.data[0] = num;
    quint16 st = step;
    pack.data[1] = st & 0xFF;
    pack.data[2] = st >> 8;
    emit generatePacket(pack);
}
