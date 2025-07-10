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

    enum class AckResult
    {
        Ok,
        Timeout,
        Error,
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

    bool setDownloadRecent(int startId, int endId);
    bool setDownloadHistoric(time_t startTime, int startId, int endId);
    bool setDownloadType(int sensorType, int dataType);
    bool setDownloadId(int id);
    bool getDownloadSize(int &size);
    bool getDownloadData(int &packetId, QByteArray &data);

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
    AckResult waitForAck(std::chrono::milliseconds timeout);

    SerialPort *serialPort = nullptr;
    QTimer keepAliveTimer;
    QEventLoop ackEventLoop;

    RxState rxState = RxState::WaitEndLine;
    AckState ackState = AckState::None;
    SendState sendState = SendState::None;
    QString rxTextData;
    BinHeader rxBinHeader;
    QByteArray rxBinData;
};

#endif // COMMUNICATOR_H
