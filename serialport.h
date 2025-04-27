#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QObject>
#include <QSerialPort>
#include <QString>

class SerialPort : public QObject
{
    Q_OBJECT
public:
    explicit SerialPort(QObject *parent = nullptr);

    bool open(const QString &portName, int baudRate);
    void close();
    bool isOpened();

signals:
    void opened();
    void closed();

private:
    QSerialPort *qSerialPort;
};

#endif // SERIALPORT_H
