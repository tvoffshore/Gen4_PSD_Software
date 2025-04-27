#include "serialport.h"

#include <QDebug>

SerialPort::SerialPort(QObject *parent)
    : QObject{parent}
{
    qSerialPort = new QSerialPort(this);

    connect(qSerialPort, &QSerialPort::errorOccurred, this, [=](QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError)
        {
            qWarning() << "Port error:" << error;
            if (error == QSerialPort::ResourceError)
            {
                close();
            }
        }
    });
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
            qInfo() << "Port opened:" << portName;
            emit opened();
        }
        else
        {
            qCritical() << "Failed to open port" << portName << ":" << qSerialPort->errorString();
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

bool SerialPort::isOpened()
{
    return qSerialPort->isOpen();
}
