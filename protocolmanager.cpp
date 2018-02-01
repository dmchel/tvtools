#include "protocolmanager.h"

#include <QByteArray>
#include <QTimer>

SerialPacket::SerialPacket(quint8 addr, quint8 cmd, quint8 len)
{
    address = addr;
    command = cmd;
    length = len;
    data.clear();
    if(length) {
       data.fill(0, length);
    }
}

void SerialPacket::clear()
{
    address = 0;
    command = 0;
    length = 0;
    data.clear();
}

bool SerialPacket::isValid() const
{
    bool fValid = true;
    if((address & 0x80) || (command & 0x80)) {
        fValid  = false;
    }
    return fValid;
}

ProtocolManager::ProtocolManager(QObject *parent) : QObject(parent)
{
    rxBuffer.clear();
    rxState = ReceiverState::WAIT_START_FRAME;
    currPack.clear();
    resetCrc();
    qRegisterMetaType<SerialPacket>("SerialPacket");
    qRegisterMetaType<CommunicationStatistic>("CommunicationStatistic");
    checkConnectPeriod = 5000;
    memset(&statistic, 0, sizeof(statistic));

    stateTimer = new QTimer(this);
    stateTimer->setTimerType(Qt::PreciseTimer);
    stateTimer->setSingleShot(true);
    connect(stateTimer, &QTimer::timeout, this, &ProtocolManager::transferTimeout);

    onlineTimer = new QTimer(this);
    onlineTimer->setTimerType(Qt::PreciseTimer);
    onlineTimer->setSingleShot(true);
    connect(onlineTimer, &QTimer::timeout, this, &ProtocolManager::onlineTimeout);
}

/*ProtocolManager::CommunicationStatistic ProtocolManager::commStatus() const
{
    return statistic;
}*/

/**
 * @brief ProtocolManager::connectionLostTimeout
 * @return Таймаут определения потери связи по каналу
 */
int ProtocolManager::connectionLostTimeout() const
{
    return checkConnectPeriod;
}

/**
 * @brief ProtocolManager::setConnectionLostTimeout
 *  Установить таймаут определения потери связи по каналу
 * @param value, ms
 */
void ProtocolManager::setConnectionLostTimeout(int value)
{
    checkConnectPeriod = value;
}

void ProtocolManager::stop()
{
    emit finished();
}

/**
 * @brief ProtocolManager::putData
 * Новые данные для обработки
 * @param data
 */
void ProtocolManager::receiveData(const QByteArray &data)
{
    statistic.rxBytes += data.size();
    rxBuffer.append(data);
    stateTimer->start(1000);
    dataHandler();
    emit sendCommStatistic(statistic);
}

/**
 * @brief ProtocolManager::sendPacket
 * Отправка пакета данных в последовательный порт
 * (Отправляются только валидные пакеты)
 * Выполняется байт-стаффинг здесь ((С)Йода)
 * @param pack пакет для отправки
 * ret массив данных для отправки (это для тестов)
 */
QByteArray ProtocolManager::sendPacket(const SerialPacket &pack)
{
    QByteArray dataToSend;
    if(pack.isValid()) {
        dataToSend.append(FSTART);
        dataToSend.append(pack.address);
        dataToSend.append(pack.command);
        //попытка байт-стаффинга длины пакета
        quint8 stuffByte = tryToStuffByte(pack.length);
        if(stuffByte != pack.length) {
            dataToSend.append(FESC);
            dataToSend.append(stuffByte);
        }
        else {
            dataToSend.append(pack.length);
        }
        //в пакете присутствуют данные
        if(pack.length) {
            for(int index = 0; index < pack.data.size(); index++) {
                //проверка байта на стаффинг
                stuffByte = tryToStuffByte(pack.data.at(index));
                if(stuffByte != pack.data.at(index)) {
                    dataToSend.append(FESC);
                    dataToSend.append(stuffByte);
                }
                else {
                    dataToSend.append(pack.data.at(index));
                }
            }
        }
        //вычисление контрольной суммы оригинального пакета (без учета стаффинга)
        quint16 crc16 = computeCRC16(pack);
        uint8_t *crc = (uint8_t *) &crc16;
        //попытка байт-стаффинга контрольной суммы
        for(int i = 0; i < 2; i++) {
            stuffByte = tryToStuffByte(crc[i]);
            if(stuffByte != crc[i]) {
                dataToSend.append(FESC);
                dataToSend.append(stuffByte);
            }
            else {
                dataToSend.append(crc[i]);
            }
        }
        emit transmitData(dataToSend);
        statistic.txBytes += dataToSend.size();
        statistic.txPackets++;
        emit sendCommStatistic(statistic);
    }
    return dataToSend;
}

/**
 * Private methods
 */

/**
 * @brief ProtocolManager::transferTimeout
 *  Таймаут приема пакета
 */
void ProtocolManager::transferTimeout()
{
    if(rxState != ReceiverState::WAIT_START_FRAME) {
        rxState = ReceiverState::FRAME_ERROR;
        statistic.timeoutErrors++;
        statistic.overallErrors++;
        emit sendCommStatistic(statistic);
    }
}

/**
 * @brief ProtocolManager::onlineTimeout
 *  Таймаут фиксации отсуствия связи по каналу
 */
void ProtocolManager::onlineTimeout()
{
    statistic.fConnected = false;
    emit sendCommStatistic(statistic);
}

void ProtocolManager::dataHandler()
{
    //побайтный разбор данных
    while(!rxBuffer.isEmpty()) {
        quint8 c = rxBuffer[0];
        switch (rxState) {
        case ReceiverState::WAIT_START_FRAME:
            rxState = checkStartMark(c);
            break;
        case ReceiverState::WAIT_ADDRESS:
            rxState = checkAddress(c);
            break;
        case ReceiverState::WAIT_CMD:
            rxState = checkCmd(c);
            break;
        case ReceiverState::WAIT_LEN:
            rxState = checkLen(c);
            break;
        case ReceiverState::DATA_FLOW:
            rxState = checkData(c);
            break;
        case ReceiverState::STUFFING:
            rxState = doSomeStuff(rxState, c);
            break;
        case ReceiverState::WAIT_CRC:
            rxState = checkCrc(c);
            break;
        case ReceiverState::WAIT_CRC_EMPTY_PACK:
            rxState = checkCrc(c);
            break;
        case ReceiverState::FRAME_ERROR:
            currPack.clear();
            resetCrc();
            rxState = checkStartMark(c);
            break;
        case ReceiverState::STUFF_LEN:
            rxState = doSomeStuff(rxState, c);
            break;
        case ReceiverState::STUFF_CRC:
            rxState = doSomeStuff(rxState, c);
            break;
        default:
            break;
        }
        rxBuffer.remove(0, 1);
    }
}

ProtocolManager::ReceiverState ProtocolManager::checkStartMark(quint8 byte)
{
    if(byte == FSTART) {
        return ReceiverState::WAIT_ADDRESS;
    }
    return ReceiverState::FRAME_ERROR;
}

ProtocolManager::ReceiverState ProtocolManager::checkAddress(quint8 byte)
{
    if(byte & 0x80) {
        onFormatError();
        return ReceiverState::FRAME_ERROR;
    }
    currPack.address = byte;
    return ReceiverState::WAIT_CMD;
}

ProtocolManager::ReceiverState ProtocolManager::checkCmd(quint8 byte)
{
    if(byte & 0x80) {
        onFormatError();
        return ReceiverState::FRAME_ERROR;
    }
    currPack.command = byte;
    return ReceiverState::WAIT_LEN;
}

ProtocolManager::ReceiverState ProtocolManager::checkLen(quint8 byte)
{
    ReceiverState state = ReceiverState::FRAME_ERROR;
    //байт стаффинг
    if(byte == FESC) {
        state = ReceiverState::STUFF_LEN;
    }
    //стартовая метка не должна быть здесь
    else if(byte == FSTART) {
        onFormatError();
    }
    else {
        currPack.length = byte;
        if(currPack.length) {
            state = ReceiverState::DATA_FLOW;
        }
        //пустой пакет
        else {
            state = ReceiverState::WAIT_CRC_EMPTY_PACK;
        }
    }
    return state;
}

ProtocolManager::ReceiverState ProtocolManager::checkData(quint8 byte)
{
    ReceiverState state = ReceiverState::FRAME_ERROR;
    if(byte == FESC) {
        state = ReceiverState::STUFFING;
    }
    //стартовая метка не должна быть здесь
    else if(byte == FSTART) {
        onFormatError();
    }
    else {
        currPack.data.append(byte);
        if(currPack.data.size() == (int) currPack.length) {
            state = ReceiverState::WAIT_CRC;
        }
        else {
            state = ReceiverState::DATA_FLOW;
        }
    }
    return state;
}

ProtocolManager::ReceiverState ProtocolManager::doSomeStuff(ReceiverState state, quint8 byte)
{
    ReceiverState res = ReceiverState::FRAME_ERROR;
    bool fStuff = false;
    quint8 stuffByte = 0x00;
    switch (byte) {
    case TFSTART:
        stuffByte = FSTART;
        fStuff = true;
        break;
    case TFEND:
        stuffByte = FESC;
        fStuff = true;
        break;
    default:
        onStuffError();
        break;
    }
    //произведена замена байт
    if(fStuff) {
        //где произошла замена?
        switch (state) {
        //замена в данных
        case ReceiverState::STUFFING:
            currPack.data.append(stuffByte);
            if(currPack.data.size() == (int) currPack.length) {
                res = ReceiverState::WAIT_CRC;
            }
            else {
                res = ReceiverState::DATA_FLOW;
            }
            break;
        //замена в длине пакета
        case ReceiverState::STUFF_LEN:
            currPack.length = stuffByte;
            res = ReceiverState::DATA_FLOW;
            break;
        //замена в контрольной сумме
        case ReceiverState::STUFF_CRC:
            if(!fCrcNextStep) {
                currCrc = byte;
                fCrcNextStep = true;
                res = ReceiverState::WAIT_CRC;
            }
            else {
                res = finalCrcCheck(stuffByte);
            }
            break;
        default:
            break;
        }
    }
    return res;
}

ProtocolManager::ReceiverState ProtocolManager::checkCrc(quint8 byte)
{
    ReceiverState res = ReceiverState::FRAME_ERROR;
    switch (byte) {
    case FSTART:
        break;
    case FESC:
        res = ReceiverState::STUFF_CRC;
        break;
    default:
        if(!fCrcNextStep) {
            currCrc = byte;
            fCrcNextStep = true;
            res = ReceiverState::WAIT_CRC;
        }
        else {
            res = finalCrcCheck(byte);
        }
        break;
    }
    return res;
}

quint8 ProtocolManager::computeCRC(const SerialPacket &pack)
{
    quint8 crc = 0;
    QByteArray crcData;
    crcData.append(FSTART);
    crcData.append(pack.address);
    crcData.append(pack.command);
    crcData.append(pack.length);
    if(!pack.data.isEmpty() && (pack.data.size() <= 0xFF)) {
        crcData.append(pack.data);
    }
    crc = crc8_xxx(reinterpret_cast<quint8*>(crcData.data()), crcData.size());
    return crc;
}

quint16 ProtocolManager::computeCRC16(const SerialPacket &pack)
{
    quint16 crc16 = CRC16_CCITT_INIT;
    QByteArray crcData;
    crcData.append(FSTART);
    crcData.append(pack.address);
    crcData.append(pack.command);
    crcData.append(pack.length);
    if(!pack.data.isEmpty() && (pack.data.size() <= 0xFF)) {
        crcData.append(pack.data);
    }
    crc16 = crc16_ccitt(crc16, reinterpret_cast<quint8*>(crcData.data()), crcData.size());
    return ~crc16;
}

/**
 * @brief ProtocolManager::tryToStuffByte
 *  Попытка выполнить байт-стаффинг. Если БС не нужен, то
 * возвращается тот же самый байт, иначе его замена (без FESC)
 * @param byte байт данных
 * @return байт данных с учетом байт-стаффинга
 */
quint8 ProtocolManager::tryToStuffByte(quint8 byte)
{
    if(byte == FSTART) {
        return TFSTART;
    }
    else if(byte == FESC) {
        return TFEND;
    }
    else {
        return byte;
    }
}

/**
 * @brief ProtocolManager::finalCrcCheck
 *  Здесь выполняется финальная проверка
 *  контрольной суммы текущего пакета
 * @param byte значение crc
 */
ProtocolManager::ReceiverState ProtocolManager::finalCrcCheck(quint8 byte)
{
    ReceiverState res = ReceiverState::FRAME_ERROR;
    currCrc |= (byte << 8);
    //CRC OK
    if(computeCRC16(currPack) == currCrc) {
        emit packRecieved(currPack);
        res = ReceiverState::WAIT_START_FRAME;
        onValidPackReceive();
    }
    //CRC BAD
    else {
       onCrcError();
    }
    currPack.clear();
    resetCrc();
    return res;
}

/**
 * @brief ProtocolManager::resetCrc
 *  Сброс текущего значения принятой контрольной суммы
 */
void ProtocolManager::resetCrc()
{
    fCrcNextStep = false;
    currCrc = 0;
}

/**
 * @brief ProtocolManager::onValidPackReceive
 *  Получен валидный пакет
 */
void ProtocolManager::onValidPackReceive()
{
    statistic.fConnected = true;
    statistic.rxPackets++;
    emit sendCommStatistic(statistic);
    onlineTimer->start(checkConnectPeriod);
}

/**
 * @brief ProtocolManager::onCrcError
 *  Ошибка контрольной суммы
 */
void ProtocolManager::onCrcError()
{
    statistic.crcErrors++;
    statistic.overallErrors++;
    emit sendCommStatistic(statistic);
}

/**
 * @brief ProtocolManager::onFormatError
 *  Ошибка формата пакета
 * На данный момент эта ошибка означает
 * присутствие метки начала пакета не в начале пакета)
 */
void ProtocolManager::onFormatError()
{
    statistic.formatErrors++;
    statistic.overallErrors++;
    emit sendCommStatistic(statistic);
}

/**
 * @brief ProtocolManager::onStuffError
 *  Ошибка выполнения обратного байт-стаффинга
 */
void ProtocolManager::onStuffError()
{
    statistic.stuffErrors++;
    statistic.overallErrors++;
    emit sendCommStatistic(statistic);
}

quint8 ProtocolManager::crc8_xxx(quint8 *pcBlock, quint8 len)
{
    quint8 crc = POLY_CONST;

    while(len--) {
        crc = crc8Table[crc ^ *pcBlock++];
    }
    return crc;
}

/*
 * Calculate a new sum given the current sum and the new data.
 * Use 0xffff as the initial sum value.
 * Do not forget to invert the final checksum value.
 */
quint16 ProtocolManager::crc16_ccitt (quint16 sum, quint8 *buf, quint16 len)
{
    if (len) do {
        sum = (sum >> 8) ^ crc16Table [*buf++ ^ (unsigned char) sum];
    }
    while (--len);
    return sum;
}

quint16 ProtocolManager::crc16_ccitt_byte (quint16 sum, quint8 byte)
{
    sum = (sum >> 8) ^ crc16Table [byte ^ (unsigned char) sum];
    return sum;
}

