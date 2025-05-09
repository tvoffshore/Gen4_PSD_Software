#include "connector.h"

#include <QDebug>
#include <QSerialPortInfo>

Connector::Connector(Ui::MainWindow *ui, SerialPort *serialPort, QObject *parent)
    : QObject{parent}
    , ui(ui)
    , serialPort(serialPort)
{
    connect(ui->comboBoxPortName, &QComboBox::currentTextChanged, this, [=](const QString &text) {
        if (text != portName && text.isEmpty() == false &&
            (portName.isEmpty() == true || portListIsUpdating == false))
        {
            // If new not empty port selected and current port is empty or port updating isn't in progress
            qDebug() << "Select port:" << text;
            portName = text;
        }
    });

    connect(ui->pushButtonPortConnect, &QPushButton::clicked, this, &Connector::onPortConnect);

    connect(serialPort, &SerialPort::opened, this, &Connector::onPortOpened);
    connect(serialPort, &SerialPort::closed, this, &Connector::onPortClosed);

    updatePortList();

    ui->tabWidget->setEnabled(false);
    ui->tabDownload->setEnabled(false);
}

void Connector::updatePortList()
{
    portListIsUpdating = true;
    ui->comboBoxPortName->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports)
    {
        ui->comboBoxPortName->addItem(info.portName());
    }
    portListIsUpdating = false;

    if (ui->comboBoxPortName->currentIndex() < 0)
    {
        ui->pushButtonPortConnect->setEnabled(false);
        portName.clear();
        qWarning() << "No ports to select";
    }
    else
    {
        ui->pushButtonPortConnect->setEnabled(true);
        ui->comboBoxPortName->setCurrentText(portName);

        QString currentPort = ui->comboBoxPortName->currentText();
        if (portName != currentPort)
        {
            qWarning() << "Port changed:" << portName << "->" << currentPort;
            portName = currentPort;
        }
    }
}

void Connector::onPortConnect()
{
    bool isPortOpened = serialPort->isOpened();
    if (isPortOpened == false)
    {
        int baudRate = ui->comboBoxBaudRate->currentText().toInt();
        qDebug() << "Open" << portName << "baudrate" << baudRate;

        bool result = serialPort->open(portName, baudRate);
        if (result == false)
        {
            onPortClosed();
        }
    }
    else
    {
        qDebug() << "Close" << portName;

        serialPort->close();
    }
}

void Connector::onPortOpened()
{
    ui->pushButtonPortConnect->setCheckable(true);
    ui->pushButtonPortConnect->setChecked(true);
    ui->pushButtonPortConnect->setText("Close");

    ui->comboBoxPortName->setEnabled(false);
    ui->comboBoxBaudRate->setEnabled(false);

    ui->tabWidget->setEnabled(true);
    ui->tabDownload->setEnabled(true);
}

void Connector::onPortClosed()
{
    ui->pushButtonPortConnect->setText("Open");
    ui->pushButtonPortConnect->setChecked(false);
    ui->pushButtonPortConnect->setCheckable(false);

    ui->comboBoxPortName->setEnabled(true);
    ui->comboBoxBaudRate->setEnabled(true);
    updatePortList();

    ui->tabWidget->setEnabled(false);
    ui->tabDownload->setEnabled(false);
}
