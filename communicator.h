#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <QByteArray>
#include <QObject>
#include <QTimer>

#include "serialport.h"

class Communicator : public QObject
{
    enum class State
    {
        WaitBinMagic,
        WaitBinCrcLsb,
        WaitBinCrcMsb,
        WaitBinLengthLsb,
        WaitBinLengthMsb,
        WaitBinData,
        WaitEndLine,
    };

    struct BinHeader
    {
        uint32_t magic;
        uint16_t crc16;
        uint16_t length;
    };

    Q_OBJECT
public:
    explicit Communicator(SerialPort *serialPort, QObject *parent = nullptr);
    ~Communicator();

    bool requestDownloadRecent(int startId, int endId);
    bool requestDownloadHitoric(time_t startTime, int startId, int endId);
    bool requestDownloadType(int sensorType, int dataType);
    bool requestDownloadSize(int &size);
    bool requestDownloadData();

signals:
    void binDataRead(const QByteArray &data);

private slots:
    void onPortOpened();
    void onPortClosed();
    void onPortRead(QByteArray data);
    void onWaitTimeout();

private:
    SerialPort *serialPort = nullptr;
    QTimer *waitTimer = nullptr;

    State state = State::WaitBinMagic;
    BinHeader binHeader;
    QByteArray binData;
};

#endif // COMMUNICATOR_H
