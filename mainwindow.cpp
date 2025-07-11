#include "mainwindow.h"

#include "communicator.h"
#include "connector.h"
#include "downloader.h"
#include "logger.h"
#include "serialport.h"

#include <QDebug>
#include <QMessageBox>
#include <QVersionNumber>

namespace
{
Communicator *communicator = nullptr;
Connector *connector = nullptr;
Downloader *downloader = nullptr;
Logger *logger = nullptr;
SerialPort *serialPort = nullptr;

// Major application version
constexpr int versionMajor = 0;
// Minor application version
constexpr int versionMinor = 3;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Setup filter to catch specific UI events
    ui->comboBoxPortName->installEventFilter(this);

    connect(ui->actionAboutApplication, &QAction::triggered, this, [=](){
        QVersionNumber version(versionMajor, versionMinor);
        QMessageBox::about(this, "About application", "Device assistant version " + version.toString());
    });

    connect(ui->actionAboutQt, &QAction::triggered, this, [=](){
        QMessageBox::aboutQt(this, "About Qt");
    });

    logger = new Logger(ui, this);
    serialPort = new SerialPort(this);
    connector = new Connector(ui, serialPort, this);
    communicator = new Communicator(serialPort, this);
    downloader = new Downloader(ui, communicator, this);
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
