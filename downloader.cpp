#include "downloader.h"

#include <QDebug>

namespace
{
constexpr int sensorTypeCount = 6;
constexpr int dataTypeCount = 3;

const char *fileNames[sensorTypeCount][dataTypeCount] =
{
    {"PSD_ADC1", "STAT_ADC1", "RAW_/ADC1"},
    {"PSD_ADC2", "STAT_ADC2", "RAW_ADC2"},
    {"PSD_ACC", "STAT_ACC", "RAW_ACC"},
    {"PSD_ACC_RES", "STAT_ACC_RES", "RAW_ACC_RES"},
    {"PSD_GYR", "STAT_GYR", "RAW_GYR"},
    {"PSD_ANG", "STAT_ANG", "RAW_ANG"},
};
}

Downloader::Downloader(Ui::MainWindow *ui, Communicator *communicator, QObject *parent)
    : QObject{parent}
    , communicator(communicator)
    , ui(ui)
    , file(new QFile())
{
    ui->pushButtonHistoricRequest->setEnabled(false);
    ui->pushButtonRecentRequest->setEnabled(false);
    ui->pushButtonSize->setEnabled(true);
    ui->pushButtonDownload->setEnabled(true);
    ui->spinBoxTypeSensor->setMaximum(sensorTypeCount - 1);
    ui->spinBoxTypeData->setMaximum(dataTypeCount - 1);

    connect(ui->radioButtonHistoric, &QRadioButton::clicked, this, [=](){
        qDebug() << "Historic checked";
        ui->pushButtonHistoricRequest->setEnabled(true);
        ui->pushButtonRecentRequest->setEnabled(false);
    });

    connect(ui->radioButtonRecent, &QRadioButton::clicked, this, [=](){
        qDebug() << "Recent checked";
        ui->pushButtonRecentRequest->setEnabled(true);
        ui->pushButtonHistoricRequest->setEnabled(false);
    });

    connect(ui->pushButtonHistoricRequest, &QPushButton::clicked, this, [=](){
        QDateTime dateTime = ui->dateTimeEditHistoric->dateTime();
        time_t startTime = dateTime.toSecsSinceEpoch();
        startId = ui->spinBoxHistoricPacketFrom->value();
        endId = ui->spinBoxHistoricPacketTo->value();
        communicator->requestDownloadHitoric(startTime, startId, endId);
    });

    connect(ui->pushButtonRecentRequest, &QPushButton::clicked, this, [=](){
        startId = ui->spinBoxRecentPackFrom->value();
        endId = ui->spinBoxRecentPackTo->value();
        communicator->requestDownloadRecent(startId, endId);
    });

    connect(ui->pushButtonTypeRequest, &QPushButton::clicked, this, [=](){
        int sensorType = ui->spinBoxTypeSensor->value();
        int dataType = ui->spinBoxTypeData->value();
        communicator->requestDownloadType(sensorType, dataType);
    });

    connect(ui->pushButtonSize, &QPushButton::clicked, this, [=](){
        int size = 0;
        communicator->requestDownloadSize(size);
    });

    connect(ui->pushButtonDownload, &QPushButton::clicked, this, [=](){
        if (endId >= startId)
        {
            int sensorType = ui->spinBoxTypeSensor->value();
            int dataType = ui->spinBoxTypeData->value();
            QDateTime dateTime = QDateTime::currentDateTime();
            QString fileName = QString(fileNames[sensorType][dataType]) + "_" + dateTime.toString("yyyyMMdd_hhmmss") + ".bin";

            qDebug() << "Open file:" << fileName;

            file->setFileName(fileName);
            if (file->open(QIODevice::WriteOnly))
            {
                fileId = startId;
                communicator->requestDownloadData();
            }
            else
            {
                qWarning() << "File open failed:" << file->errorString();
            }
        }
    });

    connect(communicator, &Communicator::binDataRead, this, &Downloader::onBinDataRead);

    QDateTime dateTime = QDateTime::currentDateTime();
    ui->dateTimeEditHistoric->setDateTime(dateTime);
}

void Downloader::onBinDataRead(QByteArray data)
{
    qDebug() << "Bin data read, size" << data.size();

    if (file->isOpen())
    {
        file->write(data);
        if (fileId >= endId)
        {
            file->close();
            qDebug() << "File closed:" << file->fileName();
        }
        else
        {
            fileId++;
        }
    }
}
