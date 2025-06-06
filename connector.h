#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTimer>

#include "serialport.h"
#include "ui_MainWindow.h"

class Connector : public QObject
{
    Q_OBJECT
public:
    explicit Connector(Ui::MainWindow *ui, SerialPort *serialPort, QObject *parent = nullptr);
    ~Connector();

    void updatePortList();

signals:

private slots:
    void onPortConnect();
    void onPortOpened();
    void onPortClosed();
    void onPortRead(QByteArray data);
    void onDeviceOnlineTimeout();

private:
    QString portName;
    bool portListIsUpdating = false;
    bool isDeviceOnline = false;

    Ui::MainWindow *ui = nullptr;
    SerialPort *serialPort = nullptr;
    QTimer *deviceOnlineTimer = nullptr;
};

#endif // CONNECTOR_H
