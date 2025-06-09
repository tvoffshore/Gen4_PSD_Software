#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTimer>

#include "serialport.h"

class Communicator : public QObject
{
    enum class RxState
    {
        WaitBinMagic,
        WaitBinCrcLsb,
        WaitBinCrcMsb,
        WaitBinLengthLsb,
        WaitBinLengthMsb,
        WaitBinData,
        WaitEndLine,
    };

    enum class AckState
    {
        None,
        WaitRx,
        Received,
    };

    enum class SendState
    {
        None,
        InProgress,
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
    bool requestDownloadData(QByteArray &data, int &chunkId);
    bool requestDownloadNext();

signals:
    void textDataReceived(const QString &string);
    void binDataReceived(const QByteArray &data);
    void ackReceived();

private slots:
    void onPortOpened();
    void onPortClosed();
    void onPortRead(QByteArray data);
    void onKeepAliveTimeout();

private:
    void resetRxState(bool waitBinData = false);
    void sendKeepAlive();
    bool sendCommand(const QByteArray &data, std::chrono::milliseconds timeout, bool waitBinData = false);
    bool waitForAck(std::chrono::milliseconds timeout);

    SerialPort *serialPort = nullptr;
    QTimer *keepAliveTimer = nullptr;

    RxState rxState = RxState::WaitEndLine;
    AckState ackState = AckState::None;
    SendState sendState = SendState::None;
    QString textData;
    BinHeader binHeader;
    QByteArray binData;
};

#endif // COMMUNICATOR_H
