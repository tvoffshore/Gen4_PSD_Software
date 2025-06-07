#include "communicator.h"

#include <QDebug>

namespace
{
constexpr std::chrono::seconds rxWaitTimeout = std::chrono::seconds{1};
constexpr std::chrono::seconds keepAlivePeriod = std::chrono::seconds{2};
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
    , keepAliveTimer(new QTimer(this))
{
    connect(serialPort, &SerialPort::opened, this, &Communicator::onPortOpened);
    connect(serialPort, &SerialPort::closed, this, &Communicator::onPortClosed);
    connect(serialPort, &SerialPort::read, this, &Communicator::onPortRead);

    connect(keepAliveTimer, &QTimer::timeout, this, &Communicator::onKeepAliveTimeout);
    keepAliveTimer->setSingleShot(false);
}

Communicator::~Communicator()
{
    delete keepAliveTimer;
}

bool Communicator::requestDownloadRecent(int startId, int endId)
{
    QString data = downloadRecentCmd;
    data += QString::number(startId);
    data += ',';
    data += QString::number(endId);
    data += endOfLine;

    bool result = sendCommandWithAck(data.toUtf8());
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

    bool result = sendCommandWithAck(data.toUtf8());
    return result;
}

bool Communicator::requestDownloadType(int sensorType, int dataType)
{
    QString data = downloadTypeCmd;
    data += QString::number(sensorType);
    data += ',';
    data += QString::number(dataType);
    data += endOfLine;

    bool result = sendCommandWithAck(data.toUtf8());
    return result;
}

bool Communicator::requestDownloadSize(int &size)
{
    bool result = sendCommandWithAck(downloadSizeCmd, ackWaitLongTimeout);
    if (result == true && textData.length() > 0)
    {
        size = textData.toInt();
    }

    return result;
}

bool Communicator::requestDownloadData(QByteArray &data, int &chunkId)
{
    bool result = sendCommandWithAck(downloadDataCmd, ackWaitLongTimeout, true);
    if (result == true && binData.size() > 0 && textData.length() > 0)
    {
        data = binData;
        chunkId = textData.toInt();
    }

    return result;
}

bool Communicator::requestDownloadNext()
{
    bool result = sendCommandWithAck(downloadNextCmd);
    return result;
}

void Communicator::onPortOpened()
{
    keepAliveTimer->start(keepAlivePeriod);
    sendKeepAlive();
}

void Communicator::onPortClosed()
{
    keepAliveTimer->stop();
}

void Communicator::onPortRead(QByteArray data)
{
    keepAliveTimer->start(keepAlivePeriod);

    for (uint8_t byte : data)
    {
        switch (rxState)
        {
        case RxState::WaitEndLine:
            if (byte == endOfLine)
            {
                isAckReceived = true;
                emit ackReceived();

                if (rxString.length() > 0)
                {
                    textData = rxString;
                    rxString.clear();
                    qDebug() << "Text data read:" << textData;
                    emit textDataReceived(textData);
                }
            }
            else
            {
                rxString += (char)byte;
            }
            break;

        case RxState::WaitBinMagic:
            binHeader.magic >>= 8;
            binHeader.magic |= (byte << 24);
            if (binHeader.magic == magicPattern)
            {
                qDebug() << "Magic found";
                rxState = RxState::WaitBinCrcLsb;
            }
            break;

        case RxState::WaitBinCrcLsb:
            binHeader.crc16 = byte;
            rxState = RxState::WaitBinCrcMsb;
            break;

        case RxState::WaitBinCrcMsb:
            binHeader.crc16 |= byte << 8;
            rxState = RxState::WaitBinLengthLsb;
            break;

        case RxState::WaitBinLengthLsb:
            binHeader.length = byte;
            rxState = RxState::WaitBinLengthMsb;
            break;

        case RxState::WaitBinLengthMsb:
            binHeader.length |= byte << 8;
            qDebug() << "Wait" << binHeader.length << "bytes bin data";
            rxState = RxState::WaitBinData;
            binData.clear();
            break;

        case RxState::WaitBinData:
            binData += byte;
            if (binData.size() >= binHeader.length)
            {
                uint16_t crc16 = crc16_modbus(binData);
                if (crc16 == binHeader.crc16)
                {
                    qDebug() << "Bin data read, size" << binHeader.length;
                    emit binDataReceived(binData);
                }
                resetRxState();
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

void Communicator::resetRxState()
{
    isAckReceived = false;
    rxState = RxState::WaitEndLine;
    rxString.clear();
}

void Communicator::sendKeepAlive()
{
    bool result = serialPort->write(keepAliveCmd);
    if (result == false)
    {
        qWarning() << "Keep alive skipped";
    }
}

bool Communicator::sendCommandWithAck(const QByteArray &data, std::chrono::milliseconds timeout, bool waitBinData)
{
    if (isSendInProgress == false)
    {
        isSendInProgress = true;
    }
    else
    {
        return false;
    }

    if (waitBinData == true)
    {
        rxState = RxState::WaitBinMagic;
    }
    else
    {
        resetRxState();
    }

    bool result = serialPort->write(data);
    if (result == true)
    {
        result = waitForAck(timeout);
        if (result == true)
        {
            qDebug() << "Ack received";
        }
        else
        {
            qWarning() << "No ack";
        }
    }

    if (result != true)
    {
        qWarning() << "Send command failed";
    }

    isSendInProgress = false;

    return result;
}

bool Communicator::waitForAck(std::chrono::milliseconds timeout)
{
    QEventLoop loop;
    QTimer timeoutTimer;

    timeoutTimer.setSingleShot(true);
    connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(this, &Communicator::ackReceived, &loop, &QEventLoop::quit);

    timeoutTimer.start(timeout);
    loop.exec();

    return isAckReceived;
}
