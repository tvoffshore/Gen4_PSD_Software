#include "logger.h"

#include <QDateTime>
#include <QDebug>

namespace
{
QTextBrowser *textBrowser = nullptr;
}

Logger::Logger(Ui::MainWindow *ui, QObject *parent)
    : QObject{parent}
    , ui(ui)
{
    // Setup logging to text browser
    textBrowser = ui->textBrowserLog;
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &context, const QString &msg) {
        (void)context;
        switch (type) {
        case QtMsgType::QtCriticalMsg:
        case QtMsgType::QtFatalMsg:
            textBrowser->setTextColor(Qt::darkRed);
            break;
        case QtMsgType::QtWarningMsg:
            textBrowser->setTextColor(Qt::darkYellow);
            break;
        case QtMsgType::QtInfoMsg:
            textBrowser->setTextColor(Qt::darkGreen);
            break;
        default:
#ifdef QT_DEBUG
            textBrowser->setTextColor(Qt::black);
            break;
#else
            // Ignore debug messages in no Debug build
            return;
#endif // QT_DEBUG
        }
        QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
        QString text = QString("%1: %2").arg(timestamp, msg);
        textBrowser->append(text);
    });

    connect(ui->pushButtonLogClear, &QPushButton::clicked, this, [=](){
        ui->textBrowserLog->clear();
    });
}

Logger::~Logger()
{
}
