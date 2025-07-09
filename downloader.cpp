#include "downloader.h"

#include <chrono>
#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProgressDialog>

#include "parser.h"

Downloader::Downloader(Ui::MainWindow *ui, Communicator *communicator, QObject *parent)
    : QObject{parent}
    , communicator(communicator)
    , ui(ui)
{
    connect(ui->pushButtonDownload, &QPushButton::clicked, this, [=](){
        ui->pushButtonDownload->setEnabled(false);
        ui->textBrowserDownload->clear();

        qInfo() << "Start downloading";
        auto startTime = std::chrono::high_resolution_clock::now();
        bool result = download();
        if (result == true)
        {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            qInfo() << "Downloading finished in " << durationMs;
        }
        else
        {
            qWarning() << "Downloading failed";
        }

        ui->pushButtonDownload->setEnabled(true);
    });

    QDateTime dateTime = QDateTime::currentDateTime();
    ui->dateTimeEditHistoric->setDateTime(dateTime);
}

Downloader::~Downloader()
{
}

bool Downloader::download()
{
    int packetFromId = ui->spinBoxPacketFrom->value();
    int packetToId = ui->spinBoxPacketTo->value();
    if (packetFromId > packetToId)
    {
        qCritical() << "Packet from > packet to";
        return false;
    }

    if (ui->radioButtonHistoric->isChecked())
    {
        QDateTime dateTime = ui->dateTimeEditHistoric->dateTime();
        time_t startTime = dateTime.toSecsSinceEpoch();

        bool result = communicator->requestDownloadHitoric(startTime, packetFromId, packetToId);
        if (result == false)
        {
            qCritical() << "Set historic data params failed";
            return false;
        }
    }
    else
    {
        bool result = communicator->requestDownloadRecent(packetFromId, packetToId);
        if (result == false)
        {
            qCritical() << "Set recent data params failed";
            return false;
        }
    }

    int sensorType = ui->comboBoxTypeSensor->currentIndex();
    int dataType = ui->comboBoxTypeData->currentIndex();
    bool result = communicator->requestDownloadType(sensorType, dataType);
    if (result == false)
    {
        qCritical() << "Set sensor and data types failed";
        return false;
    }

    QString headerText = QString("Download ") + ui->comboBoxTypeSensor->currentText() + " " +
                         ui->comboBoxTypeData->currentText() + ", requested " +
                         QString::number(packetToId - packetFromId + 1) + " " +
                         QString(ui->radioButtonHistoric->isChecked() ? "historical" : "recent") + " packet(s)";
    ui->textBrowserDownload->append(headerText);

    int downloadSize = 0;
    result = communicator->requestDownloadSize(downloadSize);
    if (result == false)
    {
        qCritical() << "Request download size failed";
        return false;
    }

    qInfo() << "Download size:" << downloadSize << "bytes";

    QDateTime dateTime = QDateTime::currentDateTime();
    QString dirPath = dateTime.toString("yyyy-MM-dd");
    QString fileName = dirPath + "/" + ui->comboBoxTypeData->currentText() + " " +
                       ui->comboBoxTypeSensor->currentText() + " " +
                       dateTime.toString("yyyyMMdd_hhmmss");

    QDir dir;
    result = dir.exists(dirPath);
    if (result == false)
    {
        qDebug() << "Create directory:" << dirPath;
        result = dir.mkpath(dirPath);
        if (result == false)
        {
            qCritical() << "Create directory failed";
            return false;
        }
    }

#ifdef QT_DEBUG
    QFile binfile;
    binfile.setFileName(fileName + ".bin");
    qDebug() << "Open file:" << binfile.fileName();
    result = binfile.open(QIODevice::WriteOnly);
    if (result == false)
    {
        qCritical() << "File open failed:" << binfile.errorString();
        return false;
    }
#endif // QT_DEBUG

    QFile jsonfile;
    jsonfile.setFileName(fileName + ".json");
    qDebug() << "Open file:" << jsonfile.fileName();
    result = jsonfile.open(QIODevice::WriteOnly);
    if (result == false)
    {
        qCritical() << "File open failed:" << jsonfile.errorString();
        return false;
    }

    QProgressDialog progress("", "Cancel", 0, downloadSize);
    progress.setWindowTitle("Downloading");
    progress.setModal(true);
    progress.setValue(0);
    progress.show();

    int downloadId = 0;
    int downloadOffset = 0;
    while (downloadOffset < downloadSize)
    {
        result = communicator->requestDownloadId(downloadId);
        if (result == false)
        {
            qCritical() << "Request packet id failed";
            break;
        }

        int packetId;
        QByteArray rawData;
        auto startTime = std::chrono::high_resolution_clock::now();
        result = communicator->requestDownloadData(packetId, rawData);
        if (result == false)
        {
            qCritical() << "Download data packet failed";
            break;
        }
        auto endTime = std::chrono::high_resolution_clock::now();

        if (packetId == downloadId)
        {
            downloadId++;
        }
        else
        {
            qWarning() << "Packet id" << QString::number(packetId)
            << "!= download id " << QString::number(downloadId);
            continue;
        }

#ifdef QT_DEBUG
        binfile.write(rawData);
#endif // QT_DEBUG

        QByteArray jsonData;
        result = Parser::toJson(rawData, jsonData);
        if (result == false)
        {
            qCritical() << "Parse data packet failed";
            break;
        }

        ui->textBrowserDownload->append("Packet " + QString::number(packetId) + ":");

        if (jsonData.isEmpty() == false)
        {
            jsonfile.write(jsonData);
            ui->textBrowserDownload->append(jsonData);
        }
        else
        {
            qWarning() << "Parsed data is empty";
        }

        downloadOffset += rawData.size();
        qInfo() << "Packet" << packetId << "is ready, total" << downloadOffset << "bytes";

        if (progress.wasCanceled())
        {
            qWarning() << "Download was cancelled";
            break;
        }

        // Calculate rate of raw data bytes downloading in kB/sec
        auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        double downloadRate = static_cast<double>(rawData.size()) * 1000 / (durationMs.count() * 1024);

        progress.setLabelText(QString::number(downloadRate, 'g', 2) + " kB/sec");
        progress.setValue(downloadOffset);
    }

    progress.close();
#ifdef QT_DEBUG
    binfile.close();
    qDebug() << "File closed:" << binfile.fileName();
#endif // QT_DEBUG
    jsonfile.close();
    qDebug() << "File closed:" << jsonfile.fileName();

    return result;
}
