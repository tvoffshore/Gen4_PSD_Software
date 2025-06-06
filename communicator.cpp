#include "communicator.h"

#include <QDebug>

namespace
{
constexpr std::chrono::seconds readWaitTimeout = std::chrono::seconds{5};
const char *downloadRecentCmd = "!123:DWNR=";
const char *downloadHistoricCmd = "!123:DWNH=";
const char *downloadTypeCmd = "!123:DWNT=";
const char *downloadSizeCmd = "!123:DWNS?\r";
const char *downloadDataCmd = "!123:DWND?\r";
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
    , waitTimer(new QTimer(this))
{
    connect(serialPort, &SerialPort::opened, this, &Communicator::onPortOpened);
    connect(serialPort, &SerialPort::closed, this, &Communicator::onPortClosed);
    connect(serialPort, &SerialPort::read, this, &Communicator::onPortRead);

    connect(waitTimer, &QTimer::timeout, this, &Communicator::onWaitTimeout);
    waitTimer->setSingleShot(true);
}

Communicator::~Communicator()
{
    delete waitTimer;
}

bool Communicator::requestDownloadRecent(int startId, int endId)
{
    QString data = downloadRecentCmd;
    data += QString::number(startId);
    data += ',';
    data += QString::number(endId);
    data += endOfLine;

    serialPort->write(data.toUtf8());
    return true;
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

    serialPort->write(data.toUtf8());
    return true;
}

bool Communicator::requestDownloadType(int sensorType, int dataType)
{
    QString data = downloadTypeCmd;
    data += QString::number(sensorType);
    data += ',';
    data += QString::number(dataType);
    data += endOfLine;

    serialPort->write(data.toUtf8());
    return true;
}

bool Communicator::requestDownloadSize(int &size)
{
    serialPort->write(downloadSizeCmd);
    size = 10;
    return true;
}

bool Communicator::requestDownloadData()
{
    serialPort->write(downloadDataCmd);
    return true;
}

void Communicator::onPortOpened()
{
}

void Communicator::onPortClosed()
{

}

void Communicator::onPortRead(QByteArray data)
{
    // qDebug() << "Read:" << data.size();

    waitTimer->start(readWaitTimeout);

    for (uint8_t byte : data)
    {
        switch (state)
        {
        case State::WaitBinMagic:
            binHeader.magic >>= 8;
            binHeader.magic |= (byte << 24);
            if (binHeader.magic == magicPattern)
            {
                qDebug() << "Magic found";
                state = State::WaitBinCrcLsb;
            }
            break;

        case State::WaitBinCrcLsb:
            binHeader.crc16 = byte;
            state = State::WaitBinCrcMsb;
            break;

        case State::WaitBinCrcMsb:
            binHeader.crc16 |= byte << 8;
            state = State::WaitBinLengthLsb;
            break;

        case State::WaitBinLengthLsb:
            binHeader.length = byte;
            state = State::WaitBinLengthMsb;
            break;

        case State::WaitBinLengthMsb:
            binHeader.length |= byte << 8;
            state = State::WaitBinData;
            binData.clear();
            break;

        case State::WaitBinData:
            binData += byte;
            if (binData.size() >= binHeader.length)
            {
                uint16_t crc16 = crc16_modbus(binData);
                if (crc16 == binHeader.crc16)
                {
                    emit binDataRead(binData);
                }
                state = State::WaitBinMagic;
            }
            break;

        case State::WaitEndLine:
            state = State::WaitBinMagic;
            waitTimer->stop();
            break;

        default:
            break;
        }

    }
}

void Communicator::onWaitTimeout()
{
    state = State::WaitBinMagic;
}
