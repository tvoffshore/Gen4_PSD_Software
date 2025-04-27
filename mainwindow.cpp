#include "mainwindow.h"

#include "connector.h"
#include "logger.h"
#include "serialport.h"

#include <QDebug>

namespace
{
Connector *connector = nullptr;
Logger *logger = nullptr;
SerialPort *serialPort = nullptr;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Setup filter to catch specific UI events
    ui->comboBoxPortName->installEventFilter(this);

    logger = new Logger(ui, this);
    serialPort = new SerialPort(this);
    connector = new Connector(ui, serialPort, this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->comboBoxPortName)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            connector->updatePortList();
        }
    }

    return QMainWindow::eventFilter(watched, event);
}
