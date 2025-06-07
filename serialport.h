#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QByteArray>
#include <QObject>
#include <QSerialPort>
#include <QString>
#include <QTimer>

class SerialPort : public QObject
{
    Q_OBJECT
public:
    explicit SerialPort(QObject *parent = nullptr);
    ~SerialPort();

    bool isOpened();
    bool open(const QString &portName, int baudRate);
    void close();
    bool write(const QByteArray &data);

signals:
    void opened();
    void closed();
    void read(const QByteArray &data);

private slots:
    void onPortError(QSerialPort::SerialPortError error);
    void onPortReadData();
    void onPortWritten(qint64 bytes);
    void onWriteTimeout();

private:
    QSerialPort *qSerialPort = nullptr;
    QTimer *writeTimer = nullptr;
    qint64 bytesToWrite = 0;
};

#endif // SERIALPORT_H
