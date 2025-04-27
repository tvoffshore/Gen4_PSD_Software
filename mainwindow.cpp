#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "connector.h"
#include "serialport.h"

#include <QDateTime>
#include <QDebug>

namespace
{
Connector *connector = nullptr;
SerialPort *serialPort = nullptr;
QTextBrowser *textBrowser = nullptr;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Setup logging to text browser
    textBrowser = ui->textBrowserPortLogs;
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &context, const QString &msg) {
        (void)context;
        QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
        QString text = QString("%1: %2").arg(timestamp, msg);
        switch (type) {
        case QtMsgType::QtCriticalMsg:
            textBrowser->setTextColor(Qt::darkRed);
            break;
        case QtMsgType::QtWarningMsg:
            textBrowser->setTextColor(Qt::darkYellow);
            break;
        case QtMsgType::QtInfoMsg:
            textBrowser->setTextColor(Qt::darkGreen);
            break;
        default:
            textBrowser->setTextColor(Qt::black);
            break;
        }
        textBrowser->append(text);
    });

    // Setup filter to catch specific UI events
    ui->comboBoxPortName->installEventFilter(this);

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
