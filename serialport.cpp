#include "serialport.h"

#include <chrono>

#include <QDebug>

namespace
{
constexpr std::chrono::seconds writeTimeout = std::chrono::seconds{5};
}

SerialPort::SerialPort(QObject *parent)
    : QObject{parent}
    , qSerialPort(new QSerialPort(this))
    , writeTimer(new QTimer(this))
{
    connect(qSerialPort, &QSerialPort::errorOccurred, this, &SerialPort::onPortError);
    connect(qSerialPort, &QSerialPort::bytesWritten, this, &SerialPort::onPortWritten);
    connect(qSerialPort, &QSerialPort::readyRead, this, &SerialPort::onPortReadData);

    connect(writeTimer, &QTimer::timeout, this, &SerialPort::onWriteTimeout);
    writeTimer->setSingleShot(true);
}

bool SerialPort::isOpened()
{
    return qSerialPort->isOpen();
}

bool SerialPort::open(const QString &portName, int baudRate)
{
    bool result = false;

    if (qSerialPort->isOpen() == false)
    {
        if (baudRate > QSerialPort::BaudRate::Baud115200)
        {
            baudRate = QSerialPort::BaudRate::Baud115200;
        }
        else if (baudRate < QSerialPort::BaudRate::Baud1200)
        {
            baudRate = QSerialPort::BaudRate::Baud1200;
        }

        qSerialPort->setPortName(portName);
        qSerialPort->setBaudRate(baudRate);
        qSerialPort->setDataBits(QSerialPort::Data8);
        qSerialPort->setParity(QSerialPort::NoParity);
        qSerialPort->setStopBits(QSerialPort::OneStop);
        qSerialPort->setFlowControl(QSerialPort::NoFlowControl);

        result = qSerialPort->open(QIODevice::ReadWrite);
        if (result)
        {
            qInfo() << "Port opened:" << qSerialPort->portName() << qSerialPort->baudRate();
            emit opened();
        }
        else
        {
            qCritical() << "Failed to open port" << qSerialPort->portName() << ":" << qSerialPort->errorString();
        }
    }
    else
    {
        qWarning() << "Port" << qSerialPort->portName() << "already opened";
    }

    return result;
}

void SerialPort::close()
{
    if (qSerialPort->isOpen() == true)
    {
        qSerialPort->close();
        qInfo() << "Port closed: " << qSerialPort->portName();
        emit closed();
    }
    else
    {
        qWarning() << "Port already closed";
    }
}

void SerialPort::write(const QByteArray &data)
{
    qDebug() << "Write:" << data;
    const qint64 written = qSerialPort->write(data);
    if (written == data.size())
    {
        bytesToWrite += written;
        writeTimer->start(writeTimeout);
    }
    else
    {
        qWarning() << "Port" << qSerialPort->portName() << "write failed:" << qSerialPort->errorString();
    }
}

void SerialPort::onPortError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError)
    {
        qWarning() << "Port" << qSerialPort->portName() << "error:" << error;
        if (error == QSerialPort::ResourceError)
        {
            close();
        }
    }
}

void SerialPort::onPortReadData()
{
    const QByteArray data = qSerialPort->readAll();
    emit read(data);
}

void SerialPort::onPortWritten(qint64 bytes)
{
    bytesToWrite -= bytes;
    if (bytesToWrite == 0)
    {
        writeTimer->stop();
    }
}

void SerialPort::onWriteTimeout()
{
    qWarning() << "Port" << qSerialPort->portName() << "write timeout:" << qSerialPort->errorString();
}
