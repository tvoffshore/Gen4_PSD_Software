#include "downloader.h"

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QProgressDialog>

namespace
{
// Sensor names list
const char *sensorNames[] = {
    "ACC_X",
    "ACC_Y",
    "ACC_Z",
    "ACC_RES",
    "GYR_X",
    "GYR_Y",
    "GYR_Z",
    "ROLL",
    "PITCH",
    "ADC1",
    "ADC2",
};

// Data names list
const char *dataNames[] = {
    "PSD",
    "STAT",
    "RAW",
};
}

Downloader::Downloader(Ui::MainWindow *ui, Communicator *communicator, QObject *parent)
    : QObject{parent}
    , communicator(communicator)
    , ui(ui)
{
    connect(ui->pushButtonDownload, &QPushButton::clicked, this, [=](){
        ui->pushButtonDownload->setEnabled(false);
        ui->textBrowserDownload->clear();

        qInfo() << "Start downloading";
        bool result = download();
        if (result == true)
        {
            qInfo() << "Downloading finished";
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

    int downloadSize = 0;
    result = communicator->requestDownloadSize(downloadSize);
    if (result == false)
    {
        qCritical() << "Request download size failed";
        return false;
    }

    qInfo() << "Download size:" << downloadSize << "bytes";

    QDateTime dateTime = QDateTime::currentDateTime();
    QString fileName = QString(dataNames[dataType]) + "_" + QString(sensorNames[sensorType]) +
                       "_" + dateTime.toString("yyyyMMdd_hhmmss") + ".bin";

    QFile file;
    file.setFileName(fileName);
    qDebug() << "Open file:" << file.fileName();
    result = file.open(QIODevice::WriteOnly);
    if (result == false)
    {
        qCritical() << "File open failed:" << file.errorString();
        return false;
    }

    QProgressDialog progress("Downloading packets...", "Cancel", 0, downloadSize);
    progress.setWindowTitle("Device assistant");
    progress.setModal(true);
    progress.setValue(0);
    progress.show();

    int downloadOffset = 0;
    while (downloadOffset < downloadSize && progress.wasCanceled() == false)
    {
        QByteArray data;
        int chunkId = 0;
        result = communicator->requestDownloadData(data, chunkId);
        if (result == false)
        {
            qCritical() << "Download data chunk failed";
            break;
        }

        file.write(data);
        downloadOffset += data.size();
        qInfo() << "Packet" << chunkId << "is ready, total" << downloadOffset << "bytes";

        if (progress.wasCanceled() == false)
        {
            progress.setValue(downloadOffset);
        }

        result = communicator->requestDownloadNext();
        if (result == false)
        {
            qCritical() << "Request next data chunk failed";
            break;
        }
    }

    progress.close();
    file.close();
    qDebug() << "File closed:" << file.fileName();

    return result;
}
