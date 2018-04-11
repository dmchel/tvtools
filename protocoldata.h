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

    enum class DebugCommands {
        CMD_NONE,
        CMD_PING = 0x1,
        CMD_MODE = 0x2,
        CMD_PLAY_CONTROL = 0x3,
        CMD_LED_CONTROL = 0x4,
        CMD_LED_BRIGHTNESS = 0x6,
        CMD_INFO_REQ = 0x7,
        CMD_CONF_REQ = 0x8,
        CMD_DEMO = 0x0F
    };

    enum class DebugResponce {
        RESP_PING = 0x1,
        RESP_INFO = 0x2,
        RESP_CONF = 0x3,
        RESP_ERROR = 0x4,
        RESP_CONFIRM = 0x5
    };

    enum class DebugError {
        ERR_NONE,
        ERR_UNDEFINED_CMD = 0x1,
        ERR_FORMAT = 0x2,
        ERR_EXECUTE = 0x3,
        ERR_TASK_QUEUE_VERFLOW = 0x4
    };

    /**
     * @brief The Abonents enum
     *  Адреса абонентов канала
     */
    enum class AbonentAddress {
        ADDR_BROAD   		= 0x00,		//широковещательный адрес
        ADDR_HOST    		= 0x01,    	//мастер канала (хост)
        ADDR_TAB_VISION    	= 0x02,  	//устройство TabVision
    };

    quint8 tabVisionMode() const;

signals:
    void generatePacket(const SerialPacket &pack);

public slots:
    void packetHandler(const SerialPacket &pack);
    void sendPing();
    void sendMode(quint8 data);
    void sendPlayControl(quint8 data);
    void sendTabTask(const QByteArray &data);
    void sendBrightness(int value);
    void sendDemo(int num, int step);

private slots:


private:
    bool fValidPackArrived = false;

    quint8 mode = 0;

};

#endif // PROTOCOLDATA_H
