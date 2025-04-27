#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <QObject>
#include <QString>

#include "serialport.h"
#include "ui_MainWindow.h"

class Connector : public QObject
{
    Q_OBJECT
public:
    explicit Connector(Ui::MainWindow *ui, SerialPort *serialPort, QObject *parent = nullptr);

    void updatePortList();

signals:

private slots:
    void onPortConnect();
    void onPortOpened();
    void onPortClosed();

private:
    Ui::MainWindow *ui = nullptr;
    SerialPort *serialPort = nullptr;
    QString portName;
    bool portListIsUpdating = false;
};

#endif // CONNECTOR_H
