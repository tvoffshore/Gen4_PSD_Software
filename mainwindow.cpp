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

    serialPort = new SerialPort(this);
    connector = new Connector(ui, serialPort, this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
