#include "communicator.h"

#include <QDebug>

namespace
{
constexpr int commandRetryCountMax = 3;

constexpr std::chrono::seconds keepAlivePeriod = std::chrono::seconds{2};
constexpr std::chrono::seconds ackNoWaitTimeout = std::chrono::seconds{0};
constexpr std::chrono::seconds ackWaitShortTimeout = std::chrono::seconds{3};
constexpr std::chrono::seconds ackWaitLongTimeout = std::chrono::seconds{10};

const char *keepAliveCmd = "!123:KPLV\r";
const char *downloadRecentCmd = "!123:DWNR=";
const char *downloadHistoricCmd = "!123:DWNH=";
const char *downloadTypeCmd = "!123:DWNT=";
const char *downloadSizeCmd = "!123:DWNS?\r";
const char *downloadDataCmd = "!123:DWND?\r";
const char *downloadNextCmd = "!123:DWNN\r";
const char endOfLine = '\r';
constexpr uint32_t magicPattern = 0xFEDCBA98;

uint16_t crc16_modbus(const QByteArray &data)
{
    uint16_t crc = 0xFFFF;

    for (uint8_t byte : data)
    {
        crc ^= byte;

        for (int i = 0; i < 8; ++i)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }

    return crc;
}
}

Communicator::Communicator(SerialPort *serialPort, QObject *parent)
    : QObject{parent}
    , serialPort(serialPort)
{
    connect(serialPort, &SerialPort::opened, this, &Communicator::onPortOpened);
    connect(serialPort, &SerialPort::closed, this, &Communicator::onPortClosed);
    connect(serialPort, &SerialPort::read, this, &Communicator::onPortRead);

    keepAliveTimer.setSingleShot(false);
    connect(&keepAliveTimer, &QTimer::timeout, this, &Communicator::onKeepAliveTimeout);

    ackTimeoutTimer.setSingleShot(true);
    connect(&ackTimeoutTimer, &QTimer::timeout, this, [=](){
        if (ackEventLoop.isRunning())
        {
            // Ack timeout due to no ACK RX for specified time
            ackEventLoop.exit(static_cast<int>(AckResult::Timeout));
        }
    });

    connect(this, &Communicator::ackReceived, this, [=](){
        if (ackEventLoop.isRunning())
        {
            ackEventLoop.exit(static_cast<int>(AckResult::Ok));
        }
    });
}

Communicator::~Communicator()
{
}

bool Communicator::requestDownloadRecent(int startId, int endId)
{
    QString data = downloadRecentCmd;
    data += QString::number(startId);
    data += ',';
    data += QString::number(endId);
    data += endOfLine;

    bool result = sendCommand(data.toUtf8(), ackWaitShortTimeout);
    return result;
}

bool Communicator::requestDownloadHitoric(time_t startTime, int startId, int endId)
{
    QString data = downloadHistoricCmd;
    data += QString::number(startTime);
    data += ',';
    data += QString::number(startId);
    data += ',';
    data += QString::number(endId);
    data += endOfLine;

    bool result = sendCommand(data.toUtf8(), ackWaitShortTimeout);
    return result;
}

bool Communicator::requestDownloadType(int sensorType, int dataType)
{
    QString data = downloadTypeCmd;
    data += QString::number(sensorType);
    data += ',';
    data += QString::number(dataType);
    data += endOfLine;

    bool result = sendCommand(data.toUtf8(), ackWaitShortTimeout);
    return result;
}

bool Communicator::requestDownloadSize(int &size)
{
    bool result = sendCommand(downloadSizeCmd, ackWaitLongTimeout);
    if (result == true && rxTextData.length() > 0)
    {
        size = rxTextData.toInt();
    }

    return result;
}

bool Communicator::requestDownloadData(int &packetId, QByteArray &data)
{
    bool result = sendCommand(downloadDataCmd, ackWaitLongTimeout, true);
    if (result == true && rxBinData.size() > 0 && rxTextData.length() > 0)
    {
        packetId = rxTextData.toInt();
        data = rxBinData;
    }

    return result;
}

bool Communicator::requestDownloadNext()
{
    bool result = sendCommand(downloadNextCmd, ackWaitShortTimeout);
    return result;
}

void Communicator::onPortOpened()
{
    keepAliveTimer.start(keepAlivePeriod);
    sendKeepAlive();
}

void Communicator::onPortClosed()
{
    keepAliveTimer.stop();
    if (ackEventLoop.isRunning())
    {
        ackEventLoop.exit(static_cast<int>(AckResult::Error));
    }
}

void Communicator::onPortRead(QByteArray data)
{
    // Restart keep alive on RX (no keep alive while device is sending data)
    keepAliveTimer.start(keepAlivePeriod);

    for (uint8_t byte : data)
    {
        switch (rxState)
        {
        case RxState::WaitBinMagic:
            rxBinHeader.magic >>= 8;
            rxBinHeader.magic |= (byte << 24);
            if (rxBinHeader.magic == magicPattern)
            {
                qDebug() << "Magic word found";
                rxState = RxState::WaitBinCrcLsb;
            }
            break;

        case RxState::WaitBinCrcLsb:
            rxBinHeader.crc16 = byte;
            rxState = RxState::WaitBinCrcMsb;
            break;

        case RxState::WaitBinCrcMsb:
            rxBinHeader.crc16 |= byte << 8;
            rxState = RxState::WaitBinLengthLsb;
            break;

        case RxState::WaitBinLengthLsb:
            rxBinHeader.length = byte;
            rxState = RxState::WaitBinLengthMsb;
            break;

        case RxState::WaitBinLengthMsb:
            rxBinHeader.length |= byte << 8;
            qDebug() << "Wait BIN data:" << rxBinHeader.length << "bytes";
            rxState = RxState::WaitBinData;
            rxBinData.clear();
            break;

        case RxState::WaitBinData:
            rxBinData += byte;
            if (rxBinData.size() >= rxBinHeader.length)
            {
                uint16_t crc16 = crc16_modbus(rxBinData);
                if (crc16 == rxBinHeader.crc16)
                {
                    qDebug() << "Received BIN data:" << rxBinHeader.length << "bytes";
                    emit binDataReceived(rxBinData);
                }
                rxState = RxState::WaitEndLine;
            }
            break;

        case RxState::WaitEndLine:
            if (byte == endOfLine)
            {
                if (ackState == AckState::WaitRx)
                {
                    ackState = AckState::Received;
                    qDebug() << "Ack received";
                    emit ackReceived();

                    if (rxTextData.length() > 0)
                    {
                        qDebug() << "Received TEXT data:" << rxTextData;
                        emit textDataReceived(rxTextData);
                    }
                }
            }
            else
            {
                if (ackState == AckState::WaitRx)
                {
                    rxTextData += (char)byte;
                }
            }
            break;

        default:
            break;
        }

    }
}

void Communicator::onKeepAliveTimeout()
{
    sendKeepAlive();
}

void Communicator::resetRxState(bool waitBinData)
{
    rxState = waitBinData ? RxState::WaitBinMagic : RxState::WaitEndLine;
    ackState = AckState::WaitRx;
    rxTextData.clear();
}

void Communicator::sendKeepAlive()
{
    bool result = serialPort->write(keepAliveCmd);
    if (result == true && ackEventLoop.isRunning())
    {
        // Ack timeout due to keep alive sending after no RX for specified time
        ackEventLoop.exit(static_cast<int>(AckResult::Timeout));
    }
}

bool Communicator::sendCommand(const QByteArray &data, std::chrono::milliseconds timeout, bool waitBinData)
{
    if (sendState == SendState::InProgress)
    {
        qWarning() << "Command sending is in progress";
        return false;
    }

    sendState = SendState::InProgress;

    bool result = false;
    int retryCount = 0;
    while (result == false && retryCount < commandRetryCountMax)
    {
        // Restart keep alive before command send (will be restarted on RX as well)
        keepAliveTimer.start();

        // Reset RX states before sending the request to get valid response
        resetRxState(waitBinData);

        result = serialPort->write(data);
        if (result == false)
        {
            qCritical() << "Command write failed";
            break;
        }

        if (timeout > ackNoWaitTimeout)
        {
            AckResult ackResult = waitForAck(timeout);
            result = (ackResult == AckResult::Ok);
            if (ackResult == AckResult::Timeout)
            {
                qWarning() << "Ack timeout";
            }
            else if (ackResult == AckResult::Error)
            {
                qCritical() << "Ack error";
                break;
            }
        }
        retryCount++;
    }

    sendState = SendState::None;

    return result;
}

Communicator::AckResult Communicator::waitForAck(std::chrono::milliseconds timeout)
{
    ackTimeoutTimer.start(timeout);

    int code = ackEventLoop.exec();

    return static_cast<AckResult>(code);
}
